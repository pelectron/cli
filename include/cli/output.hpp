#ifndef CLI_OUPUT_HPP
#define CLI_OUPUT_HPP

#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "cli/string.hpp"

#include <concepts>
#include <type_traits>

namespace cli {

/**
 * @brief A CharStream is used to write a single character to an unbuffered
 * output stream.
 *
 * It is a callable that takes a character as input and returns a
 * cli::Error to indicate write success/failure.
 *
 * Example for an embedded target with UART:
 *
 * ```
 * cli::Error write(char c){
 *   HAL_Status status = HAL_UART_Transmit(c);
 *   return status_to_cli_err(status);
 * }
 *
 * static_assert(cli::CharWriter<decltype(&write), char>);
 * ```
 *
 * @tparam S the stream type
 * @tparam char_type the character type
 */
template <class S, typename char_type>
concept CharStream = requires(std::remove_cvref_t<S> &stream, char_type c) {
  { std::invoke(stream, c) }; // -> std::same_as<Error>;
};

/**
 * @brief A StringStream is used to write a string of characters to an
 * unbuffered output stream.
 *
 * It is a callable that takes a pointer to a string with its length as input
 * and returns a cli::Error to indicate write success.
 *
 * Example for an embedded target with UART:
 *
 * ```
 * cli::Error my_string_stream(const char* s, std::size_t len){
 *   for(std::size_t i = 0; i < len; ++i){
 *     HAL_Status status = HAL_UART_Transmit(s[i]);
 *     if(status != HAL_STAUS_OK)
 *       return status_to_cli_err(status);
 *   }
 *   return Error::none;
 * }
 *
 * static_assert(cli::StringStream<decltype(&my_string_stream), char>);
 * ```
 *
 * @tparam S the stream type
 * @tparam char_type the character type of the stream
 */
template <class S, typename char_type>
concept StringStream =
    requires(std::remove_cvref_t<S> &stream, View<const char_type> s) {
      { std::invoke(stream, s) } -> std::same_as<Error>;
    };

/**
 * @brief A BasicOutputStream is used to write raw characters to an unbuffered
 * output stream. A BasicOutputStream must satisfy the CharStream or the
 * StringStream concept, or both.
 *
 * @tparam S the stream type
 * @tparam char_type the character type
 */
template <class S>
concept BasicOutputStream =
    CharStream<S, char> or CharStream<S, unsigned char> or
    CharStream<S, signed char> or CharStream<S, char8_t> or
    CharStream<S, char16_t> or CharStream<S, char32_t> or
    StringStream<S, char> or StringStream<S, unsigned char> or
    StringStream<S, signed char> or StringStream<S, char8_t> or
    StringStream<S, char16_t> or StringStream<S, char32_t>;

/**
 * @brief Output is the interface Cli uses to write
 * to an unbuffered output stream.
 *
 * See cli::io::AnsiOutput for an example of an OutputStream.
 *
 * @tparam S the stream type
 * @tparam char_type the character type
 */
template <class S, typename char_type>
concept Output = requires(std::remove_cvref_t<S> &stream, char_type c,
                          View<const char_type> s, Control ctrl) {
  // writes a raw character c
  { stream.write(c) } -> std::same_as<Error>;
  // writes the raw string s
  { stream.write(s) } -> std::same_as<Error>;
  // executes a control
  { stream.control(ctrl) } -> std::same_as<Error>;
};

template <typename T> struct get_stream_char_type;

template <typename T>
  requires CharStream<T, char> or StringStream<T, char>
struct get_stream_char_type<T> {
  using type = char;
};

template <typename T>
  requires CharStream<T, signed char> or StringStream<T, signed char>
struct get_stream_char_type<T> {
  using type = signed char;
};

template <typename T>
  requires CharStream<T, unsigned char> or StringStream<T, unsigned char>
struct get_stream_char_type<T> {
  using type = unsigned char;
};

template <typename T>
  requires CharStream<T, char8_t> or StringStream<T, char8_t>
struct get_stream_char_type<T> {
  using type = char8_t;
};

template <typename T>
  requires CharStream<T, char16_t> or StringStream<T, char16_t>
struct get_stream_char_type<T> {
  using type = char16_t;
};

template <typename T>
  requires CharStream<T, char32_t> or StringStream<T, char32_t>
struct get_stream_char_type<T> {
  using type = char32_t;
};

template <typename T>
using get_stream_char_type_t = typename get_stream_char_type<T>::type;

/**
 * @brief The output device for ansi terminals. It relies on a BasicOutputStream
 * to actually output characters. Next to passing through character and string
 * writes, AnsiOutputStream transforms special events, such as backspace,
 * cursor movement, etc., into ansi escape sequences.
 * @tparam char_type the character type
 * @param Stream the BasicOutputStream used for actually writing characters
 */
template <Config Cfg, BasicOutputStream Stream> class AnsiOutput {
  Stream stream; //< the underlying output stream
public:
  template <Config Cfg_, BasicOutputStream S>
  AnsiOutput(Cfg_, S &&s) : stream(std::forward<S>(s)) {}

  using char_type = get_stream_char_type_t<Stream>;
  constexpr Error write(char_type c) {
    if constexpr (CharStream<Stream, char_type>) {
      return stream(c);
    } else {
      return stream(View<const char_type>{&c, 1});
    }
  }

  constexpr Error write(View<const char_type> s) {
    if constexpr (CharStream<Stream, char_type>) {
      for (const auto &ch : s)
        return stream(ch);
    } else {
      return stream(s);
    }
  }

  constexpr Error control(Control c) {
    char_type ch{};
    char_type s[4]{0x1B, '['};
    switch (c) {
    case Control::bell:
      ch = 0x07;
      return write(ch);
    case Control::backspace:
      ch = 0x08;
      s[0] = '\b';
      s[1] = ' ';
      s[2] = '\b';
      return write({s, 3});
    case Control::cursor_up:
      s[2] = 'A';
      return write({s, 3});
    case Control::cursor_down:
      s[2] = 'B';
      return write({s, 3});
    case Control::cursor_right:
      s[2] = 'C';
      return write({s, 3});
    case Control::cursor_left:
      s[2] = 'D';
      return write({s, 3});
    case Control::delete_char:
      s[2] = 'P';
      return write({s, 3});
    case Control::enter:
      if constexpr (Cfg::output_delimiter == Delimiter::lf) {
        return write('\n');
      }
      if constexpr (Cfg::output_delimiter == Delimiter::cr) {
        return write('\r');
      }
      if constexpr (Cfg::output_delimiter == Delimiter::crlf) {
        write('\r');
        return write('\r');
      }
    case Control::clear:
      s[2] = '2';
      s[3] = 'J';
      return write({s, 4});
    default:
      return Error::invalid_argument;
    }
  }
};

template <Config Cfg, BasicOutputStream S>
AnsiOutput(Cfg, S &&) -> AnsiOutput<Cfg, std::remove_cvref_t<S>>;

} // namespace cli

#endif
