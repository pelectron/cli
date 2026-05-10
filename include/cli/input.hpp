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

#include <cstdint>
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
        default:
          CLI_ASSERT(false);
      }
    }

    constexpr Error on_control(Control ctrl, std::uint8_t param = 1) {
      return buffer_.push_back(event_type(ctrl, param))
               ? Error::none
               : Error::buffer_overflow;
    }

  private:
    constexpr Error handle_normal(char_type c) {
      switch (c) {
        case 0x07: // bell
          return push_control(Control::bell, 1);
        case 0x08: // backspace
          return push_control(Control::backspace, 1);
        case 0x09: // tab
          if constexpr (Cfg::use_autocomplete)
            return push_control(Control::autocomplete, 1);
          else
            return push_char(0x09);
        case 0x0A: // linefeed
          if constexpr (config::input_delimiter_v<Cfg> == Delimiter::lf)
            return push_control(Control::enter, 1);
          else
            return push_char(c);
        case 0x0D: // carriage return
          if constexpr (config::input_delimiter_v<Cfg> == Delimiter::lf) {
            return push_char(c);
          } else if constexpr (config::input_delimiter_v<Cfg> ==
                               Delimiter::cr) {
            return push_control(Control::enter, 1);
          } else {
            state_ = State::delimiter;
            return Error::none;
          }
        case 0x1B: // escape
          state_ = State::escape_start;
          return Error::none;
        case 0x7F: // delete
          return push_control(Control::delete_char, 1);
        default:
          return push_char(c);
      }
    }

    constexpr Error handle_escape_start(char_type c) {
      if (c == static_cast<char_type>('[')) {
        param_ = 0;
        state_ = State::escape_bracket;
        return Error::none;
      }

      state_ = State::normal;

      if (buffer_.remaining_size() < 2)
        return Error::buffer_overflow;

      buffer_.push_back(static_cast<char_type>(0x1B));
      buffer_.push_back(c);
      return Error::none;
    }

    constexpr Error handle_escape_bracket(char_type c) {
      state_ = State::normal;
      switch (c) {
        case 'A':
          return push_control(Control::cursor_up, 1);
        case 'B':
          return push_control(Control::cursor_down, 1);
        case 'C':
          return push_control(Control::cursor_right, 1);
        case 'D':
          return push_control(Control::cursor_left, 1);
        case 'J':
          // unsupported
          return print_escape(c);
        case 'K':
          return push_control(Control::clear_line_to_end, 1);
        default:
          if (c >= '0' and c <= '9') {
            state_ = State::escape_param;
            param_ = static_cast<std::uint8_t>(c - '0');
            return Error::none;
          }
          return print_escape(c);
      }
    }

    constexpr Error handle_escape_param(char_type c) {
      state_ = State::normal;
      switch (c) {
        case 'A':
          return push_control(Control::cursor_up, param_ == 0 ? 1 : param_);
        case 'B':
          return push_control(Control::cursor_down, param_ == 0 ? 1 : param_);
        case 'C':
          return push_control(Control::cursor_right, param_ == 0 ? 1 : param_);
        case 'D':
          return push_control(Control::cursor_left, param_ == 0 ? 1 : param_);
        case 'J':
          if (param_ == 2) // clear screen escape code
            return push_control(Control::clear_screen, 1);
          return print_param(c);
        case 'K':
          switch (param_) {
            case 0:
              return push_control(Control::clear_line_to_end, 1);
            case 1:
              return push_control(Control::clear_line_to_begin, 1);
            case 2:
              return push_control(Control::clear_line, 1);
            default:
              return print_param(c);
          }
        default:
          if (c >= '0' and c <= '9') {
            std::uint8_t new_param =
              static_cast<std::uint8_t>(param_ * 10u + c - '0');
            if (new_param < param_) {
              // overflow
              return print_param(c);
            }
            param_ = new_param;
            state_ = State::escape_param;
            return Error::none;
          }
          return print_param(c);
      }
      return Error::none;
    }

    constexpr Error handle_delimiter(char_type c) {
      state_ = State::normal;
      if (c == '\n')
        return push_control(Control::enter, 1);

      // did not have \r\n
      if (buffer_.remaining_size() < 2)
        return Error::buffer_overflow;

      buffer_.push_back('\r');
      buffer_.push_back(c);

      return Error::none;
    }

    constexpr Error print_escape(char_type c) {
      if (buffer_.remaining_size() < 3)
        return Error::buffer_overflow;

      buffer_.push_back(static_cast<char_type>(0x1B));
      buffer_.push_back(static_cast<char_type>('['));
      buffer_.push_back(c);
      return Error::none;
    }

    constexpr Error print_param(char_type end) {
      char_type buf[10]{};
      cli::format::Int<std::uint8_t, char_type> fmt;
      cli::format::FormatResult res = fmt({buf, 10}, param_);
      CLI_ASSERT(res);

      if (buffer_.remaining_size() < 3 + res.size_written)
        return Error::buffer_overflow;

      buffer_.push_back(static_cast<char_type>(0x1B));
      buffer_.push_back(static_cast<char_type>('['));

      for (std::size_t i = 0; i < res.size_written; ++i) {
        buffer_.push_back(buf[i]);
      }

      buffer_.push_back(end);
      return Error::none;
    }

    enum class State : std::uint8_t {
      normal,
      escape_start,
      escape_bracket,
      escape_param,
      delimiter
    };

    constexpr Error push_control(Control c, uint8_t param) {
      return buffer_.push_back(event_type(c, param)) ? Error::none
                                                     : Error::buffer_overflow;
    }

    constexpr Error push_char(char_type c) {
      return buffer_.push_back(event_type(c)) ? Error::none
                                              : Error::buffer_overflow;
    }

    using event_t = std::conditional_t<config::use_volatile_input_buffer_v<Cfg>,
                                       volatile event_type,
                                       event_type>;

    using State_t = std::conditional_t<config::use_volatile_input_buffer_v<Cfg>,
                                       volatile State,
                                       State>;

    using Param_t = std::conditional_t<config::use_volatile_input_buffer_v<Cfg>,
                                       volatile std::uint8_t,
                                       std::uint8_t>;

    State_t state_{State::normal};
    Param_t param_{0};
    RingBuffer<event_t, config::input_size_v<Cfg>> buffer_{};
  };
} // namespace cli
#endif
