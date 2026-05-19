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

#include "cli/basic_format.hpp"
#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "cli/event.hpp"
#include "cli/ring_buffer.hpp"
#include "cli/util.hpp"

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
    using char_type = config::char_type_t<Cfg>;

    /// the event type produced by the Input.
    using event_type = Event<char_type>;

    /**
     * pops the next available event
     *
     * @param event where to store the popped event
     * @return returns false if no event is available, i.e. event is not set.
     */
    constexpr bool pop_event(event_type &event) noexcept {
      return buffer_.pop(event);
    }

    /**
     * resets/clears the input
     */
    constexpr void reset() noexcept {
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
    constexpr Error on_char(char_type c) noexcept {
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
          state_ = State::normal;
          return Error::none;
      }
    }

    constexpr Error on_control(Control ctrl, std::uint8_t param = 1) noexcept {
      switch (ctrl) {
        case Control::autocomplete:
          if constexpr (config::use_autocomplete_v<Cfg>)
            return push_control(ctrl, param);
          else
            return Error::none;
        case Control::cursor_up:
          [[fallthrough]];
        case Control::cursor_down:
          if constexpr (config::use_history_v<Cfg>)
            return push_control(ctrl, param);
          else
            return Error::none;
        case Control::cursor_left:
          [[fallthrough]];
        case Control::delete_char:
          [[fallthrough]];
        case Control::cursor_right:
          [[fallthrough]];
        case Control::clear_line_to_end:
          if constexpr (config::use_cursor_v<Cfg>)
            return push_control(ctrl, param);
          else
            return Error::none;
        case Control::character:
          [[fallthrough]];
        case Control::bell:
          [[fallthrough]];
        case Control::backspace:
          [[fallthrough]];
        case Control::clear_screen:
          [[fallthrough]];
        case Control::clear_line:
          [[fallthrough]];
        case Control::clear_line_to_begin:
          [[fallthrough]];
        default:
          return push_control(ctrl, param);
      }
    }

  private:
    constexpr Error handle_normal(char_type c) noexcept {
      switch (c) {
        case 0x07: // bell
          return push_control(Control::bell, 1);
        case 0x08: // backspace
          return push_control(Control::backspace, 1);
        case 0x09: // tab
          if constexpr (config::use_autocomplete_v<Cfg>)
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

    constexpr Error handle_escape_start(char_type c) noexcept {
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

    constexpr Error handle_escape_bracket(char_type c) noexcept {
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

    constexpr Error handle_escape_param(char_type c) noexcept {
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
    }

    constexpr Error handle_delimiter(char_type c) noexcept {
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

    constexpr Error print_escape(char_type c) noexcept {
      if (buffer_.remaining_size() < 3)
        return Error::buffer_overflow;

      buffer_.push_back(static_cast<char_type>(0x1B));
      buffer_.push_back(static_cast<char_type>('['));
      buffer_.push_back(c);
      return Error::none;
    }

    constexpr Error print_param(char_type end) noexcept {
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

    constexpr Error push_control(Control c, uint8_t param) noexcept {
      return buffer_.push_back(event_type(c, param)) ? Error::none
                                                     : Error::buffer_overflow;
    }

    constexpr Error push_char(char_type c) noexcept {
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

  /**
   * A simplified Input implementation.
   *
   * It handles a basic set of ascii characters (DEL/BS/ESC/tabs/feeds). Unlike
   * cli::Input, it does not recognize ANSI escape sequences.
   *
   * @ingroup Input
   * @tparam Cfg the cli config.
   */
  template<concepts::Config Cfg>
  class SimpleInput {
  public:
    /// the character type
    using char_type = config::char_type_t<Cfg>;

    /// the event type produced by the Input.
    using event_type = Event<char_type>;

    constexpr Error on_char(char_type c) {
      if constexpr (config::input_delimiter_v<Cfg> == Delimiter::crlf) {
        if (last_char_is_cr_) {
          last_char_is_cr_ = false;
          if (c == '\n') {
            return push_control(Control::enter, 1);
          } else {
            push_char('\r');
            return push_char(c);
          }
        }
      }

      switch (c) {
        case 0x07: // bell
          return push_control(Control::bell, 1);
        case 0x08: // backspace
          return push_control(Control::backspace, 1);
        case 0x09: // tab
          if constexpr (config::use_autocomplete_v<Cfg>)
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
            last_char_is_cr_ = true;
            return Error::none;
          }
        case 0x7F: // delete
          if constexpr (config::use_cursor_v<Cfg>)
            return push_control(Control::delete_char, 1);
          else
            return Error::none;
        default:
          return push_char(c);
      }
    }

    constexpr Error on_control(Control ctrl, std::uint8_t param) {
      switch (ctrl) {
        case Control::autocomplete:
          if constexpr (config::use_autocomplete_v<Cfg>)
            return push_control(ctrl, param);
          else
            return Error::none;
        case Control::cursor_up:
          [[fallthrough]];
        case Control::cursor_down:
          if constexpr (config::use_history_v<Cfg>)
            return push_control(ctrl, param);
          else
            return Error::none;
        case Control::cursor_left:
          [[fallthrough]];
        case Control::delete_char:
          [[fallthrough]];
        case Control::cursor_right:
          [[fallthrough]];
        case Control::clear_line_to_end:
          if constexpr (config::use_cursor_v<Cfg>)
            return push_control(ctrl, param);
          else
            return Error::none;
        case Control::character:
          [[fallthrough]];
        case Control::bell:
          [[fallthrough]];
        case Control::backspace:
          [[fallthrough]];
        case Control::clear_screen:
          [[fallthrough]];
        case Control::clear_line:
          [[fallthrough]];
        case Control::clear_line_to_begin:
          [[fallthrough]];
        default:
          return push_control(ctrl, param);
      }
    }

    constexpr bool pop_event(event_type &ev) { return buffer_.pop(ev); }

    constexpr void reset() noexcept {
      buffer_.clear();
      if constexpr (config::input_delimiter_v<Cfg> == Delimiter::crlf) {
        last_char_is_cr_ = false;
      }
    }

  private:
    constexpr Error push_control(Control c, uint8_t param) noexcept {
      return buffer_.push_back(event_type(c, param)) ? Error::none
                                                     : Error::buffer_overflow;
    }

    constexpr Error push_char(char_type c) noexcept {
      return buffer_.push_back(event_type(c)) ? Error::none
                                              : Error::buffer_overflow;
    }

    struct Empty {
      constexpr Empty(bool) {}
    };
    using last_char_t = std::conditional_t<
      config::input_delimiter_v<Cfg> == Delimiter::crlf,
      std::conditional_t<config::use_volatile_input_buffer_v<Cfg>,
                         volatile bool,
                         bool>,
      Empty>;

    using event_t = std::conditional_t<config::use_volatile_input_buffer_v<Cfg>,
                                       volatile event_type,
                                       event_type>;

    RingBuffer<event_t, config::input_size_v<Cfg>> buffer_{};
    CLI_NO_UNIQUE_ADDRESS [[maybe_unused]] last_char_t last_char_is_cr_{false};
  };
} // namespace cli
#endif
