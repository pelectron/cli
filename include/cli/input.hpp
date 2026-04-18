#ifndef CLI_INPUT_HPP
#define CLI_INPUT_HPP

#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "cli/ring_buffer.hpp"

#include <cassert>

namespace cli {

  // TODO: modify examples
  /**
   * This class represents an ansi input device, e.g. a keyboard or UART
   * input stream, and is used to send events to the cli.
   *
   * It processes data character by character.
   *
   * Function:
   * - handle special ascii characters (DEL/BS/ESC/tabs/feeds)
   * - handle a basc set of ansi escape sequences
   *
   * To use it, construct it and call on_char() in your input stream receive
   * callback, for example your UART handler.
   *
   * Example:
   *
   * ```
   * // the cli instance
   * constinit cli::Cli my_cli(...);
   *
   * // the input device
   * constinit cli:io::Input in;
   *
   * // then in the interrupt callback function used by your HAL/BSP
   * // you need call in's on_char method with the character that was just
   * // received.
   * void UART_RxCallback(Handle_t* h){
   *  cli::Error error = in.on_char(UART_GetChar(h));
   *  switch(error){
   *    case Error::none: ...
   *    case Error::buffer_overflow: ...
   *    case Error::invalid_esc_seq: ...
   *    default: ...
   *   }
   * }
   * ```
   *
   * If the input parsing is too expensive for your processor, i.e. characters
   * are lost because on_char takes too long to execute, then simply read into a
   * buffer and call on_char in your low priority thread, e.g. your main loop.
   * This is ideally done just before calling the cli's process method.
   *
   * Example:
   * ```
   * cli::RingBuffer<volatile char, 256> rx_buf;
   *
   * // the cli instance
   * constinit cli::Cli my_cli(...);
   *
   * // the input device
   * constinit cli:io::Input in;
   *
   * // then in the interrupt callback function used by your HAL/BSP
   * // you need call in's on_char method with the character that was just
   * void UART_RxCallback(Handle_t* h){
   *  rx_buf.push_back(UART_GetChar(h));
   * }
   *
   * int main(){
   *
   *  while(1){
   *    char c=0;
   *    while(rx_buf.pop(c)){
   *      cli::Error err = in.on_char(c);
   *      switch(err){
   *        ...
   *      }
   *    }
   *    cli::Error err = cli.process();
   *    switch(err){
   *      ...
   *    }
   *  }
   * }
   * ```
   *
   * @tparam Cfg the cli config
   */
  template<Config Cfg>
  class Input {
  public:
    using char_type = typename Cfg::char_type;

    struct event_type {
      bool is_char;
      union {
        Control ctrl;
        char_type c;
      };

      constexpr event_type &operator=(const event_type &o) {
        is_char = o.is_char;
        if (o.is_char) {
          c = o.c;
        } else {
          ctrl = o.ctrl;
        }
        return *this;
      }

      constexpr event_type &operator=(const volatile event_type &o) {
        is_char = o.is_char;
        if (o.is_char) {
          c = o.c;
        } else {
          ctrl = o.ctrl;
        }
        return *this;
      }

      constexpr volatile event_type &operator=(const event_type &o) volatile {
        is_char = o.is_char;
        if (o.is_char) {
          c = o.c;
        } else {
          ctrl = o.ctrl;
        }
        return *this;
      }

      constexpr volatile event_type &
      operator=(const volatile event_type &o) volatile {
        is_char = o.is_char;
        if (o.is_char) {
          c = o.c;
        } else {
          ctrl = o.ctrl;
        }
        return *this;
      }
    };

    /**
     * processes a character. This could be called within your ISR when a
     * character is received, or before calling process on your cli.
     *
     * @param c the received character
     * @return either Error::none or Error::buffer_overflow.
     */
    Error on_char(char_type c) {
      switch (state) {
        case State::normal:
          switch (c) {
            case 0x07: // bell
              return push_control(Control::bell);
            case 0x08: // backspace
              return push_control(Control::backspace);
            case 0x09: // tab
              if constexpr (Cfg::use_autocomplete)
                return push_control(Control::autocomplete);
              else
                return push_char(0x09);
            case 0x0A: // linefeed
              if (Cfg::input_delimiter == Delimiter::lf)
                return push_control(Control::enter);
              else
                return push_char(c);
            case 0x0D: // carriage return
              switch (Cfg::input_delimiter) {
                case Delimiter::cr:
                  return push_control(Control::enter);
                case Delimiter::lf:
                  return push_char(c);
                case Delimiter::crlf:
                  state = State::delimiter;
                  return Error::none;
              }
            case 0x1B: // escape
              state = State::escape_start;
              return Error::none;
            case 0x7F: // delete
              return push_control(Control::delete_char);
            default:
              return push_char(c);
          }
        case State::escape_start:
          if (c == '[') {
            state = State::escape_bracket;
          } else {
            state = State::normal;
          }
          return Error::none;
        case State::escape_bracket:
          state = State::normal;
          switch (c) {
            case 'A':
              return push_control(Control::cursor_up);
            case 'B':
              return push_control(Control::cursor_down);
            case 'C':
              return push_control(Control::cursor_right);
            case 'D':
              return push_control(Control::cursor_left);
              // case '@':
              //   return ctrl(Control::insert_char);
              // case 'P':
              //   return ctrl(Control::delete_char);
          }
          // ingore invalid escape sequences
          return Error::none;
        case State::delimiter:
          state = State::normal;
          if (c == '\n') {
            return push_control(Control::enter);
          } else {
            // did not have \r\n
            push_char('\r');
            return push_char(c);
          }
      }
      return Error::implementation_error;
    }

    /**
     * pops the next event available
     *
     * @param event where to store the popped event
     * @return returns false if no event is available
     */
    bool pop_event(event_type &event) { return buffer.pop(event); }

    /**
     * resets/clears the input
     */
    void reset() {
      state = State::normal;
      buffer.clear();
    }

  private:
    enum class State {
      normal,
      escape_start,
      escape_bracket,
      delimiter
    };

    Error push_control(Control c) {
      return buffer.push_back(event_type{.is_char = false, .ctrl = c})
               ? Error::none
               : Error::buffer_overflow;
    }

    Error push_char(char_type c) {
      return buffer.push_back(event_type{.is_char = true, .c = c})
               ? Error::none
               : Error::buffer_overflow;
    }

    using event_t = std::conditional_t<Cfg::use_volatile_input_buffer,
                                       volatile event_type,
                                       event_type>;

    State state{State::normal};
    RingBuffer<event_t, Cfg::rx_size> buffer{};
  };
} // namespace cli
#endif
