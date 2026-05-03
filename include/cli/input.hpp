/**
 * @defgroup Input
 *
 * Character input to the CLI system is handled by @ref cli::Input and classes
 * that satisfy the @ref cli::concepts::Input "Input concept".
 *
 * ## Using Custom Input Classes
 *
 * To use a custom input class, the cli::Config passed to cli::Engine must
 * contain an inner typedef called input_type. That input_type must satisfy the
 * @ref cli::concepts::Input "Input concept".
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

  // TODO: modify examples
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
   * This class template uses a single parameter, a cli::Config called Cfg. The
   * following properties from Cfg are used:
   * - `char_type`: the character type
   * - `use_volatile_input_buffer`: if set to true, this class uses a volatile
   *   buffer to store events. This should be set to true if you call on_char()
   *   in an ISR.
   * - `rx_size`: the number of events the internal ring buffer uses.
   * - `delimiter`: the delimiter used to enter a single command.
   * - `use_autocomplete`: if true, a tab will autocomplete the current command
   *   input.
   *
   * ## Recognized Escape Sequences And Special Characters
   *
   * In the following paragraph, *CSI* stands for [Control Sequence
   * Inroducer](https://en.wikipedia.org/wiki/ANSI_escape_code#Control_Sequence_Introducer_commands),
   * which is the character sequence ``0x1B 0x5B``, also commonly written as
   * ``ESC[``.
   *
   * These special characters and escape sequences are recognized by cli::Input:
   * - ``BEL`` (0x07) -> Control::bell
   * - ``backspace`` (0x08, \\b) -> Control::backspace. This means deleting the
   *   character before the cursor (or the last character in case
   *   Cfg::use_cursor is false).
   * - ``tab`` (0x09, \\t) -> passed through as is if autocomplete is not
   *   enabled, else Control:autocomplete.
   * - ``linefeed`` (0x0A, \\n) -> Control::enter if Cfg::delimiter is lf, else
   *   passed through as is.
   * - ``carriage return`` (0x0D, \\r) -> Control::enter if Cfg::delimiter is
   *   cr, else passed through as is.
   * - ``carriage return + linefeed`` ([0x0A, 0x0B], \\r\\n) -> Control::enter
   *   if Cfg::delimiter is crlf.
   * - ``delete`` (0x7F) -> Control::delete_char. Deletes the character under
   *   the cursor. If cursor is not enabled, this has no effect.
   * - ``CSI n A`` -> Control::cursor_up. cursor up movement. If history is
   *   enabled, this translates to going back in history. n is optional.
   * - ``CSI n B`` -> Control::cursor_down. cursor down movement. If history is
   *   enabled, this translates to moving forward in history. n is optional.
   * - ``CSI n C`` -> Control::cursor_right. cursor right movement. If cursor is
   *   enabled, this translates to moving the cursor to the right. If the cursor
   *   is at the end of the current input, nothing happens. n is optional.
   * - ``CSI n D`` -> Control::cursor_left: cursor left movement. If cursor is
   *   enabled, this translates to moving the cursor to the left. If the cursor
   *   is at the start of the current input, nothing happens. n is optional.
   * - ``CSI 0 K`` -> Control::clear_line_to_end: clears the line from the
   *   cursor to the end. The cursor position will not change. If the cursor is
   *   not enabled, this has no effect.
   * - ``CSI 1 K`` -> Control::clear_line_to_begin. Clears the line from the
   *   cursor to the beginning. The cursor moves to the beginning of the input.
   *   If the cursor is not enabled, this has no effect.
   * - ``CSI 2 K`` -> Control::clear_line. Clears the entire line. The cursor
   *   moves to the beginning of the input. If the cursor is not enabled, this
   *   has no effect.
   * - ``CSI 2 J`` -> Control::clear_screen. Clears the entire screen. The
   *   cursor moves to the top starting position. If the cursor is not enabled,
   *   this has no effect.
   *
   * @note Certain escape sequences effects differ from the ANSI standard
   * because CLI is intended to be used as a single line interface, not a fully
   * featured ANSI terminal. This affects the sequences ``CSI n K`` and
   * ``CSI 2 J``.
   * However, if your display/output is connected to a fully ANSI compliant
   * device, then you can use cli::AnsiOutput, which sends the needed cursor
   * move sequences to be ANSI compliant.
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
    constexpr bool pop_event(event_type &event) { return buffer.pop(event); }

    /**
     * resets/clears the input
     */
    constexpr void reset() {
      state = State::normal;
      buffer.clear();
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
      switch (state) {
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
              state = State::delimiter;
              return Error::none;
          }
        case 0x1B: // escape
          state = State::escape_start;
          return Error::none;
        case 0x7F: // delete
          return push_control(Control::Type::delete_char, 1);
        default:
          return push_char(c);
      }
    }

    constexpr Error handle_escape_start(char_type c) {
      if (c == static_cast<char_type>('[')) {
        state = State::escape_bracket;
        return Error::none;
      }
      if (Error e = push_char(static_cast<char_type>(0x1B)); e != Error::none)
        return e;
      if (Error e = push_char(c); e != Error::none)
        return e;
      state = State::normal;
      return Error::none;
    }

    constexpr Error handle_escape_bracket(char_type c) {
      state = State::normal;
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
            state = State::escape_param;
            param = static_cast<uint32_t>(c - '0');
            return Error::none;
          }
          return print_escape(c);
      }
    }

    constexpr Error handle_escape_param(char_type c) {
      state = State::normal;
      switch (c) {
        case 'A':
          return push_control(Control::Type::cursor_up, param);
        case 'B':
          return push_control(Control::Type::cursor_down, param);
        case 'C':
          return push_control(Control::Type::cursor_right, param);
        case 'D':
          return push_control(Control::Type::cursor_left, param);
        case 'J':
          if (param == 2) // clear screen escape code
            return push_control(Control::Type::clear_screen, param);
          return print_param(c);
        case 'K':
          switch (param) {
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
            param = param * 10u + static_cast<uint32_t>(c - '0');
            state = State::escape_param;
            return Error::none;
          }
          return print_param(c);
      }
    }

    constexpr Error handle_delimiter(char_type c) {
      state = State::normal;
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
      char_type buffer[10]{};
      cli::format::Format<uint32_t, char_type> fmt;
      cli::format::FormatResult res = fmt({buffer, 10}, param);
      if (not res) {
        // implementation_error
        return Error::implementation_error;
      }

      if (Error e = print_escape(); e != Error::none)
        return e;

      for (std::size_t i = 0; i < res.size_written; ++i) {
        if (Error e = push_char(buffer[i]); e != Error::none)
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
      return buffer.push_back(event_type(Control(c, param)))
               ? Error::none
               : Error::buffer_overflow;
    }

    constexpr Error push_char(char_type c) {
      return buffer.push_back(event_type(c)) ? Error::none
                                             : Error::buffer_overflow;
    }

    using event_t = std::conditional_t<config::use_volatile_input_buffer_v<Cfg>,
                                       volatile event_type,
                                       event_type>;

    State state{State::normal};
    uint32_t param{0};
    RingBuffer<event_t, config::input_size_v<Cfg>> buffer{};
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
