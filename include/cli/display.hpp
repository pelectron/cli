/**
 * @file cli/display.hpp
 * @brief This file contains the AnsiDisplay class.
 * @defgroup Output
 *
 * CLI uses the @ref cli::concept::Display "Display Concept" to output
 * characters to a screen.
 *
 * There are two types of Displays:
 * - ones that support a cursor (@ref cli::concepts::DisplayWithCursor
 *   "Displays With Cursor")
 * - ones that don't support a cursor (@ref cli::concepts::DisplayWithoutCursor
 *   "Displays Without Cursor")
 *
 * CLI provides a default implementation for displays, namely cli::AnsiDisplay.
 *
 * @note To use cursor movement in your cli, your Display must support cursor
 * movement.
 *
 */

#ifndef CLI_DISPLAY_HPP
#define CLI_DISPLAY_HPP

#include "cli/enums.hpp"
#include "cli/format.hpp"
#include "cli/string.hpp"
#include <cstdint>
#include <type_traits>

namespace cli {

  namespace dtl {
    template<typename T>
    struct get_output_char_type;

    template<typename Char>
    struct get_output_char_type<Error (*)(Char)> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<Error (*)(Char) noexcept> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<Error (&)(Char)> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<Error (&)(Char) noexcept> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<Error (*)(View<const Char>)> {
      using type = Char;
    };

    template<typename Char>
    struct get_output_char_type<Error (*)(View<const Char>) noexcept> {
      using type = Char;
    };

    template<typename Char>
    struct get_output_char_type<Error (&)(View<const Char>)> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<Error (&)(View<const Char>) noexcept> {
      using type = std::remove_const_t<Char>;
    };

    template<typename T>
      requires concepts::CharOutput<T, char> or concepts::StringOutput<T, char>
    struct get_output_char_type<T> {
      using type = char;
    };

    template<typename T>
      requires concepts::CharOutput<T, signed char> or
               concepts::StringOutput<T, signed char>
    struct get_output_char_type<T> {};

    template<typename T>
      requires concepts::CharOutput<T, unsigned char> or
               concepts::StringOutput<T, unsigned char>
    struct get_output_char_type<T> {
      using type = unsigned char;
    };

    template<typename T>
      requires concepts::CharOutput<T, char8_t> or
               concepts::StringOutput<T, char8_t>
    struct get_output_char_type<T> {
      using type = char8_t;
    };

    template<typename T>
      requires concepts::CharOutput<T, char16_t> or
               concepts::StringOutput<T, char16_t>
    struct get_output_char_type<T> {
      using type = char16_t;
    };

    template<typename T>
      requires concepts::CharOutput<T, char32_t> or
               concepts::StringOutput<T, char32_t>
    struct get_output_char_type<T> {
      using type = char32_t;
    };

    template<typename D, typename = void>
    struct is_multiline_display : std::false_type {};

    template<typename D>
    struct is_multiline_display<
      D,
      std::enable_if_t<
        std::convertible_to<decltype(D::is_multiline_display), bool>>> {
      static constexpr bool value = D::is_multiline_display;
    };

  } // namespace dtl

  template<concepts::Output O>
  using get_output_char_type_t = typename dtl::get_output_char_type<O>::type;

  template<typename D>
  inline constexpr bool is_multiline_display_v =
    dtl::is_multiline_display<D>::value;

  /**
   * @brief The AnsiDisplay represents an ANSI compliant display. It uses an
   * Output to write characters and supports cursor movement.
   *
   * This class satisfies the @ref DisplayWithCursor concept.
   *
   * @ingroup Output
   * @tparam Out the type to output characters.
   */
  template<concepts::Output Out>
  class AnsiDisplay {
  public:
    using char_type = get_output_char_type_t<Out>;
    static constexpr bool is_multiline_display = true;

    template<concepts::Output O>
    constexpr explicit AnsiDisplay(O &&output)
      : out_(std::forward<O>(output)) {}

    template<typename... Args>
    constexpr AnsiDisplay(Args &&...args)
      : out_(std::forward<Args>(args)...) {}

    /**
     * writes a character to the display
     *
     * @param c the character
     */
    constexpr Error write(char_type c) {
      if constexpr (concepts::CharOutput<Out, char_type>) {
        return out_(c);
      } else {
        return out_(View<const char_type>{&c, 1});
      }
    }

    /**
     * writes a string to the display
     *
     * @param c the character
     */
    constexpr Error write(View<const char_type> s) {
      if constexpr (concepts::StringOutput<Out, char_type>) {
        return out_(s);
      } else {
        for (const auto &ch : s) {
          if (Error e = stream(ch); e != Error::none)
            return e;
        }
        return Error::none;
      }
    }

    /**
     * deletes n characters from the display before the cursor location
     *
     * @param n the number of characters to delete
     */
    constexpr Error backspace(uint32_t n) {
      for (uint32_t i = 0; i < n; ++i)
        if (auto e = write(string_constant<char_type, '\b', ' ', '\b'>{});
            e != Error::none)
          return e;
      return Error::none;
    }

    /// clears the currently displayed line
    constexpr Error clear_line() {
      return write(string_constant<char_type,
                                   '\x1B',
                                   '[',
                                   '2',
                                   'K',
                                   '\x1B',
                                   '[',
                                   '1',
                                   'G'>{});
    }

    /// clears the entire screen and moves the cursor to the home position
    constexpr Error clear_screen() {
      return write(
        string_constant<char_type, '\x1B', '[', '2', 'J', '\x1B', 'H'>{});
    }

    /// writes a new line
    constexpr Error newline() { return write('\n'); }

    /**
     * moves the cursor n positions to the left.
     *
     * @param n how much to move
     */
    constexpr Error cursor_left(std::size_t n) {
      char_type buffer[32]{};
      const cli::format::Format<std::size_t, char_type> fmt;
      const cli::format::FormatResult res = fmt({buffer, 32}, n);
      if (not res)
        return res.error;

      Error e = write('\x1B');
      if (e != Error::none)
        return e;

      e = write('[');
      if (e != Error::none)
        return e;

      e = write({buffer, res.size_written});
      if (e != Error::none)
        return e;

      return write('D');
    }

    /**
     * moves the cursor n positions to the right
     *
     * @param n how mch to move
     */
    constexpr Error cursor_right(std::size_t n) {
      char_type buffer[32]{};
      const cli::format::Format<std::size_t, char_type> fmt;
      const cli::format::FormatResult res = fmt({buffer, 32}, n);
      if (not res)
        return res.error;

      Error e = write('\x1B');
      if (e != Error::none)
        return e;

      e = write('[');
      if (e != Error::none)
        return e;

      e = write({buffer, res.size_written});
      if (e != Error::none)
        return e;

      return write('C');
    }

  private:
    Out out_;
  };

  template<typename Out>
  AnsiDisplay(Out &&) -> AnsiDisplay<std::decay_t<Out>>;

} // namespace cli

#endif
