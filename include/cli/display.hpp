/**
 * @file cli/display.hpp
 * @brief This file contains the AnsiDisplay class.
 *
 * @defgroup Display Display
 *
 * CLI uses the @ref cli::concepts::Display "Display Concept" to output
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
 * For a detailed description of the Display concept, see
 * [here](docs.md#display).
 */

#ifndef CLI_DISPLAY_HPP
#define CLI_DISPLAY_HPP

#include "cli/concepts.hpp"
#include "cli/format.hpp"
#include "cli/string.hpp"
#include "cli/util.hpp"

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace cli {

  namespace dtl {
    template<typename T>
    struct get_output_char_type;

    template<typename Char>
    struct get_output_char_type<void (*)(Char)> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<void (*)(Char) noexcept> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<void (&)(Char)> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<void (&)(Char) noexcept> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<void (*)(View<const Char>)> {
      using type = Char;
    };

    template<typename Char>
    struct get_output_char_type<void (*)(View<const Char>) noexcept> {
      using type = Char;
    };

    template<typename Char>
    struct get_output_char_type<void (&)(View<const Char>)> {
      using type = std::remove_const_t<Char>;
    };

    template<typename Char>
    struct get_output_char_type<void (&)(View<const Char>) noexcept> {
      using type = std::remove_const_t<Char>;
    };

    template<concepts::AnyCharOutput T>
      requires(not concepts::AnyStringOutput<T>)
    struct get_output_char_type<T> {
      using type = std::remove_cvref_t<
        type_list::type_at_t<0, typename function_traits<T>::arguments>>;
    };

    template<concepts::AnyStringOutput T>
      requires(not concepts::AnyCharOutput<T>)
    struct get_output_char_type<T> {
      using type = typename std::remove_cvref_t<
        type_list::type_at_t<0, typename function_traits<T>::arguments>>::
        value_type;
    };

    template<typename T>
      requires(concepts::CharOutput<T, char> and
               concepts::StringOutput<T, char>)
    struct get_output_char_type<T> {
      using type = char;
    };

    template<typename T>
      requires(concepts::CharOutput<T, signed char> and
               concepts::StringOutput<T, signed char>)
    struct get_output_char_type<T> {
      using type = signed char;
    };

    template<typename T>
      requires(concepts::CharOutput<T, unsigned char> and
               concepts::StringOutput<T, unsigned char>)
    struct get_output_char_type<T> {
      using type = unsigned char;
    };

    template<typename T>
      requires(concepts::CharOutput<T, char8_t> and
               concepts::StringOutput<T, char8_t>)
    struct get_output_char_type<T> {
      using type = char8_t;
    };

    template<typename T>
      requires(concepts::CharOutput<T, char16_t> and
               concepts::StringOutput<T, char16_t>)
    struct get_output_char_type<T> {
      using type = char16_t;
    };

    template<typename T>
      requires(concepts::CharOutput<T, char32_t> and
               concepts::StringOutput<T, char32_t>)
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

    template<typename D>
    struct number_of_lines;

    template<typename D>
      requires(not is_multiline_display<D>::value)
    struct number_of_lines<D> {
      static constexpr std::size_t value = 1;
    };

    template<typename D>
      requires(is_multiline_display<D>::value) and requires() {
        { D::number_of_lines } -> std::convertible_to<std::size_t>;
      }
    struct number_of_lines<D> {
      static constexpr std::size_t value = D::number_of_lines;
    };

    template<typename D>
      requires(is_multiline_display<D>::value) and (not requires() {
                { D::number_of_lines } -> std::convertible_to<std::size_t>;
              })
    struct number_of_lines<D> {
      static constexpr std::size_t value =
        std::numeric_limits<std::size_t>::max();
    };

  } // namespace dtl

  /**
   * evaluates to the character type used by O
   *
   * @tparam O the output type
   */
  template<concepts::Output O>
  using get_output_char_type_t = typename dtl::get_output_char_type<O>::type;

  /**
   * is true if D is a multiline display, else false.
   *
   * @tparam D the display type
   */
  template<typename D>
  inline constexpr bool is_multiline_display_v =
    dtl::is_multiline_display<D>::value;

  template<typename D>
  inline constexpr std::size_t number_of_lines_v =
    dtl::number_of_lines<D>::value;

  inline constexpr std::size_t unlimited_lines =
    std::numeric_limits<std::size_t>::max();

  /**
   * @brief The AnsiDisplay represents an ANSI compliant display. It uses an
   * Output to write characters and supports cursor movement.
   *
   * This class satisfies the @ref cli::concepts::DisplayWithCursor
   * "Display With Cursor" concept.
   *
   * @ingroup Display
   * @tparam Out the type to output characters.
   * @tparam NumLines thenumber of lines in the displays.
   */
  template<concepts::Output Out, std::size_t NumLines = unlimited_lines>
  class AnsiDisplay {
  public:
    using char_type = get_output_char_type_t<Out>;
    static constexpr bool is_multiline_display = NumLines > 1;
    static constexpr std::size_t number_of_lines = NumLines;

    /**
     * construct an AnsiDisplay from an output.
     *
     * @param output  the output
     */
    template<concepts::Output O>
    constexpr explicit AnsiDisplay(O &&output)
      : out_(std::forward<O>(output)) {}

    /**
     * construct an AnsiDisplay from an output an a constant that set the number
     * of lines
     *
     * Example:
     * ```
     *  AnsiDisplay disp{output, cli::constant<10>{}};
     * ```
     * @param output the output
     * @param NLines the number of lines
     */
    template<concepts::Output O, auto NLines>
    constexpr AnsiDisplay(O &&output, constant<NLines>)
      : out_(std::forward<O>(output)) {}

    /**
     * constructs an AnsiDisplay by forwarding args to its output.
     *
     * @tparam Args
     * @param args
     * @return
     */
    template<typename... Args>
    constexpr AnsiDisplay(Args &&...args)
      : out_(std::forward<Args>(args)...) {}

    /**
     * writes a character to the display
     *
     * @param c the character
     */
    constexpr void write(char_type c) {
      if constexpr (concepts::CharOutput<Out, char_type>) {
        out_(c);
      } else {
        out_(View<const char_type>{&c, 1});
      }
    }

    /**
     * writes a string to the display
     *
     * @param s the string
     */
    constexpr void write(View<const char_type> s) {
      if constexpr (concepts::StringOutput<Out, char_type>) {
        out_(s);
      } else {
        for (const auto &ch : s) {
          out_(ch);
        }
      }
    }

    /**
     * deletes n characters from the display before the cursor location
     *
     * @param n the number of characters to delete
     */
    constexpr void backspace(std::size_t n) {
      for (std::size_t i = 0; i < n; ++i)
        write(string_constant<char_type, '\b', ' ', '\b'>{});
    }

    /// clears the currently displayed line
    constexpr void clear_line() {
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
    constexpr void clear_screen() {
      write(
        string_constant<char_type, '\x1B', '[', '2', 'J', '\x1B', '[', 'H'>{});
    }

    /// writes a new line
    constexpr void newline() { write('\n'); }

    /**
     * moves the cursor n positions to the left.
     *
     * @param n how much to move
     *e
     */
    constexpr void cursor_left(std::size_t n) {
      if (n == 0)
        return;
      char_type buffer[32]{};
      const cli::format::Format<std::size_t, char_type> fmt;
      const cli::format::FormatResult res = fmt({buffer, 32}, n);
      CLI_ASSERT(res);

      write('\x1B');
      write('[');
      write({buffer, res.size_written});
      write('D');
    }

    /**
     * moves the cursor n positions to the right
     *
     * @param n how mch to move
     */
    constexpr void cursor_right(std::size_t n) {
      if (n == 0)
        return;
      char_type buffer[32]{};
      const cli::format::Format<std::size_t, char_type> fmt;
      const cli::format::FormatResult res = fmt({buffer, 32}, n);
      CLI_ASSERT(res);
      write('\x1B');
      write('[');
      write({buffer, res.size_written});
      write('C');
    }

  private:
    CLI_NO_UNIQUE_ADDRESS Out out_;
  };

  template<typename Out>
  AnsiDisplay(Out &&) -> AnsiDisplay<std::decay_t<Out>>;

  template<typename Out, auto NumLines>
  AnsiDisplay(Out &&, constant<NumLines>)
    -> AnsiDisplay<std::decay_t<Out>, static_cast<std::size_t>(NumLines)>;

} // namespace cli

#endif
