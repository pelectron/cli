/**
 * @defgroup Input Input
 *
 * Character input to the CLI system is handled by @ref cli::Input and classes
 * that satisfy the @ref cli::concepts::Input "Input concept".
 *
 * See [here](docs.md#input) for a detailed explanation.
 *
 */
#ifndef CLI_INPUT_HPP
#define CLI_INPUT_HPP

#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "cli/event.hpp"
#include "cli/format.hpp"
#include "cli/ring_buffer.hpp"

#include <cassert>
#include <type_traits>

namespace cli {

  /**
   * This class represents an ANSI input device, e.g. a keyboard or UART
   * input stream.
   *
   * cli::Input processes the character input stream one character at a time and
   * produces a sequence of cli::Event, which are stored in its an interal ring
   * buffer. The main functions of cli::Input are:
   * - handling special ascii characters (DEL/BS/ESC/tabs/feeds)
   * - handlling a basc set of ANSI escape sequences
   * - passing through any "normal" character data.
   *
   * See [here](docs.md#input-class-template) for a detailed explanation of
   * which escape sequences are recognized.
   *
   * @ingroup Input
   * @tparam Cfg the cli config.
   */
  template<concepts::Config Cfg>
  class Input {
  public:
    /// the character type
    using char_type = typename Cfg::char_type;

    /// the event type produced by the Input.
    using event_type = Event<char_type>;

    /**
     * pops the next available event
     *
     * @param event where to store the popped event
     * @return returns false if no event is available, i.e. event is not set.
     */
    constexpr bool pop_event(event_type &event) { return buffer_.pop(event); }

    /**
     * resets/clears the input
     */
    constexpr void reset() {
      state_ = State::normal;
      buffer_.clear();
    }

    /**
     * processes a character. This could be called within your ISR when a
     * character is received, or before calling process on your cli. If you call
     * this method in an ISR, Cfg::use_volatile_input_buffer must be true.
     *
     * @param c the received character
     * @return either Error::none or Error::buffer_overflow.
     */
    constexpr Error on_char(char_type c) {
      switch (state_) {
        case State::normal:
          return handle_normal(c);
        case State::escape_start:
          return handle_escape_start(c);
        case State::escape_bracket:
          return handle_escape_bracket(c);
        case State::escape_param:
          return handle_escape_param(c);
        case State::delimiter:
          return handle_delimiter(c);
      }
      return Error::implementation_error;
    }

  private:
    constexpr Error handle_normal(char_type c) {
      switch (c) {
        case 0x07: // bell
          return push_control(Control::Type::bell, 1);
        case 0x08: // backspace
          return push_control(Control::Type::backspace, 1);
        case 0x09: // tab
          if constexpr (Cfg::use_autocomplete)
            return push_control(Control::Type::autocomplete, 1);
          else
            return push_char(0x09);
        case 0x0A: // linefeed
          if (config::input_delimiter_v<Cfg> == Delimiter::lf)
            return push_control(Control::Type::enter, 1);
          else
            return push_char(c);
        case 0x0D: // carriage return
          switch (config::input_delimiter_v<Cfg>) {
            case Delimiter::cr:
              return push_control(Control::Type::enter, 1);
            case Delimiter::lf:
              return push_char(c);
            case Delimiter::crlf:
              state_ = State::delimiter;
              return Error::none;
          }
        case 0x1B: // escape
          state_ = State::escape_start;
          return Error::none;
        case 0x7F: // delete
          return push_control(Control::Type::delete_char, 1);
        default:
          return push_char(c);
      }
    }

    constexpr Error handle_escape_start(char_type c) {
      if (c == static_cast<char_type>('[')) {
        state_ = State::escape_bracket;
        return Error::none;
      }
      if (Error e = push_char(static_cast<char_type>(0x1B)); e != Error::none)
        return e;
      if (Error e = push_char(c); e != Error::none)
        return e;
      state_ = State::normal;
      return Error::none;
    }

    constexpr Error handle_escape_bracket(char_type c) {
      state_ = State::normal;
      switch (c) {
        case 'A':
          return push_control(Control::Type::cursor_up, 1);
        case 'B':
          return push_control(Control::Type::cursor_down, 1);
        case 'C':
          return push_control(Control::Type::cursor_right, 1);
        case 'D':
          return push_control(Control::Type::cursor_left, 1);
        case 'J':
          // unsupported
          return print_escape(c);
        case 'K':
          return push_control(Control::Type::clear_line_to_end, 0);
        default:
          if (c >= '0' and c <= '9') {
            state_ = State::escape_param;
            param_ = static_cast<uint32_t>(c - '0');
            return Error::none;
          }
          return print_escape(c);
      }
    }

    constexpr Error handle_escape_param(char_type c) {
      state_ = State::normal;
      switch (c) {
        case 'A':
          return push_control(Control::Type::cursor_up, param_);
        case 'B':
          return push_control(Control::Type::cursor_down, param_);
        case 'C':
          return push_control(Control::Type::cursor_right, param_);
        case 'D':
          return push_control(Control::Type::cursor_left, param_);
        case 'J':
          if (param_ == 2) // clear screen escape code
            return push_control(Control::Type::clear_screen, param_);
          return print_param(c);
        case 'K':
          switch (param_) {
            case 0:
              return push_control(Control::Type::clear_line_to_end, 0);
            case 1:
              return push_control(Control::Type::clear_line_to_begin, 0);
            case 2:
              return push_control(Control::Type::clear_line, 0);
            default:
              return print_param(c);
          }
        default:
          if (c >= '0' and c <= '9') {
            param_ = param_ * 10u + static_cast<uint32_t>(c - '0');
            state_ = State::escape_param;
            return Error::none;
          }
          return print_param(c);
      }
    }

    constexpr Error handle_delimiter(char_type c) {
      state_ = State::normal;
      if (c == '\n') {
        return push_control(Control::Type::enter, 1);
      } else {
        // did not have \r\n
        Error e = push_char('\r');
        if (e != Error::none)
          return e;
        return push_char(c);
      }
    }

    constexpr Error print_escape() {
      if (Error e = push_char(static_cast<char_type>(0x1B)); e != Error::none)
        return e;
      return push_char(static_cast<char_type>('['));
    }

    constexpr Error print_escape(char_type c) {
      if (Error e = push_char(static_cast<char_type>(0x1B)); e != Error::none)
        return e;
      if (Error e = push_char(static_cast<char_type>('[')); e != Error::none)
        return e;
      return push_char(c);
    }

    constexpr Error print_param(char_type end) {
      char_type buf[10]{};
      cli::format::Format<uint32_t, char_type> fmt;
      cli::format::FormatResult res = fmt({buf, 10}, param_);
      if (not res) {
        // implementation_error
        return Error::implementation_error;
      }

      if (Error e = print_escape(); e != Error::none)
        return e;

      for (std::size_t i = 0; i < res.size_written; ++i) {
        if (Error e = push_char(buf[i]); e != Error::none)
          return e;
      }
      return push_char(end);
    }

    enum class State {
      normal,
      escape_start,
      escape_bracket,
      escape_param,
      delimiter
    };

    constexpr Error push_control(Control::Type c, uint32_t param) {
      return buffer_.push_back(event_type(Control(c, param)))
               ? Error::none
               : Error::buffer_overflow;
    }

    constexpr Error push_char(char_type c) {
      return buffer_.push_back(event_type(c)) ? Error::none
                                              : Error::buffer_overflow;
    }

    using event_t = std::conditional_t<config::use_volatile_input_buffer_v<Cfg>,
                                       volatile event_type,
                                       event_type>;

    State state_{State::normal};
    uint32_t param_{0};
    RingBuffer<event_t, config::input_size_v<Cfg>> buffer_{};
  };

  namespace dtl {
    template<typename T, typename = void>
    struct get_input_type {
      using type = cli::Input<T>;
    };
    template<typename T>
    struct get_input_type<T, std::void_t<typename T::input_type>> {
      using type = typename T::input_type;
    };
  } // namespace dtl

  template<concepts::Config Cfg>
  using get_input_type = typename dtl::get_input_type<Cfg>::type;
} // namespace cli
#endif
