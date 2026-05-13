/**
 * @file
 * @brief This file contains the utilities for parsing values from strings
 *
 * @defgroup Parsing Parsing
 *
 * Parsing in CLI is based on two things:
 * - the class cli::parse::ParseResult: the result of a parsing operation.
 * - cli::parse::Parser: the parser concept.
 *
 * A parser of T is a callable that parses a T from a string and returns a
 * ParseResult. It takes a cli::View<const CharT> as its first and only
 * argument and returns a cli::parse::ParseResult<T, CharT>.
 *
 * For more details, see [here](docs.md#parsing).
 */

#ifndef CLI_PARSE_HPP
#define CLI_PARSE_HPP

#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/string.hpp"
#include "cli/traits.hpp"
#include "cli/tuple.hpp"
#include "cli/type_list.hpp"
#include "cli/u64.hpp"
#include "cli/util.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace cli::parse {

  template<class T>
  concept Value = std::is_object_v<std::decay_t<T>>;

  template<class R, typename CharT>
  concept Result = requires(R &&r) {
    { r.error } -> std::convertible_to<Error>;
    { r.value } -> Value;
    { r.rest } -> std::convertible_to<View<const CharT>>;
    { static_cast<bool>(r) };
  };

  template<typename CharT, typename P>
  struct value_type {
    using type =
      std::remove_cvref_t<decltype(std::declval<decltype(std::declval<P &>()(
                                     std::declval<View<const CharT>>()))>()
                                     .value)>;
  };

  template<typename P>
  struct buffer_type {
    using type = std::remove_cvref_t<type_list::type_at_t<
      0,
      typename function_traits<std::decay_t<P>>::arguments>>;
  };

  template<typename P>
  using buffer_type_t = typename buffer_type<P>::type;

  template<typename P>
  struct result_type {
    using type = typename function_traits<std::decay_t<P>>::return_type;
  };

  template<typename P>
  using result_type_t = typename result_type<P>::type;

  template<typename P>
  struct value_type_ {
    using type = std::remove_cvref_t<
      decltype(std::declval<decltype(std::declval<P &>()(
                 std::declval<typename buffer_type<P>::type>()))>()
                 .value)>;
  };

  template<typename CharT, typename P>
  using value_type_t = typename value_type<CharT, std::remove_cvref_t<P>>::type;

  constexpr inline struct from_error_t {
  } from_error;

  constexpr inline struct from_value_t {
  } from_value;

  /**
   * @ingroup Parsing
   *
   * This struct is the result of a parse operation. It contains an error, a
   * value, and a rest.
   *
   * The error indicates if this is a successful parse result. cli::Error::none
   * means successful, any other Error means unsuccessful.
   *
   * The value is the parsed value.
   *
   * The rest is the remaining unparsed string.
   *
   * @tparam T the value type
   * @tparam CharT the character type of rest
   */
  template<class T, typename CharT>
  struct ParseResult {

    /// constructs a failed parse with the error reason
    constexpr ParseResult(Error e, View<const CharT> rest_str = {}) noexcept
      requires(not std::same_as<T, Error>)
      : error{e}, value{}, rest{rest_str} {}

    /**
     * construct a successful parse result from a value and the rest of the
     * string that hasn't been parsed.
     *
     * @param value the parse value
     * @param rest the unparsed rest of the string
     */
    constexpr ParseResult(const T &val,
                          View<const CharT> rest_str = {}) noexcept
      requires(not std::same_as<T, Error>)
      : error{Error::none}, value{val}, rest{rest_str} {}

    /**
     * construct a successful parse result from a value and the rest of the
     * string that hasn't been parsed.
     *
     * @param value the parse value
     * @param rest the unparsed rest of the string
     */
    constexpr ParseResult(T &&val, View<const CharT> rest_str = {}) noexcept
      requires(not std::same_as<T, Error>)
      : error{Error::none}, value{std::move(val)}, rest{rest_str} {}

    /**
     * construct a successful parse result from a value and the rest of the
     * string that hasn't been parsed.
     *
     * @param value the parse value
     * @param rest the unparsed rest of the string
     */
    template<class U>
    constexpr ParseResult(from_value_t,
                          U &&val,
                          View<const CharT> rest_str = {}) noexcept
      : error{Error::none}, value{std::forward<U>(val)}, rest{rest_str} {}

    /// constructs a failed parse with the error reason
    constexpr ParseResult(from_error_t,
                          Error e,
                          View<const CharT> rest_str = {}) noexcept
      : error{e}, value{}, rest{rest_str} {}

    /// returns true if this is a successful parse result, i.e. if the error is
    /// Error::none.
    constexpr operator bool() const noexcept { return error == Error::none; }

    constexpr auto operator<=>(const ParseResult &) const noexcept = default;

    Error error;
    T value;
    View<const CharT> rest;
  };

  template<typename T>
  inline constexpr bool is_parse_result_v = false;

  template<typename T, typename CharT>
  inline constexpr bool is_parse_result_v<ParseResult<T, CharT>> = true;

  /**
   * A parser of T is a callable that parses a T from a string and returns a
   * ParseResult. It takes a cli::View<const CharT> as its first and only
   * argument and returns a cli::parse::ParseResult<T, CharT>.
   *
   * @ingroup Parsing
   * @tparam P the parser
   * @tparam T the type of value to parse
   * @tparam CharT the character type
   */
  template<class P, class T, typename CharT>
  concept ParserOf = requires(std::decay_t<P> parse, View<const CharT> str) {
    { parse(str) } -> std::same_as<ParseResult<T, CharT>>;
  };

  /**
   * A parser turns a string into a T. It is a callable that takes a
   * cli::View<const CharT> as its first and only argument and returns a
   * cli::parse::ParseResult<T, CharT>.
   *
   * @ingroup Parsing
   * @tparam P the parser
   */
  template<class P>
  concept Parser =
    Callable<P> and cli::dtl::is_const_view_v<cli::parse::buffer_type_t<P>> and
    is_parse_result_v<cli::parse::result_type_t<P>>;

  template<typename CharT, class T>
  constexpr ParseResult<std::remove_cvref_t<T>, CharT> ok(T &&t) {
    return {std::forward<T>(t), {}};
  }

  template<typename CharT, class T>
  constexpr ParseResult<T, CharT> error(Error e) {
    return {from_error, e};
  }

  static_assert(Result<ParseResult<int, char>, char>);

  template<typename T, typename CharT>
  struct NoParse {
    ParseResult<T, CharT> operator()(View<const CharT>) const noexcept {
      return {Error::unimplemented};
    }
  };

  template<typename CharT>
  constexpr View<CharT> skip_ws(View<CharT> str) noexcept {
    return str.substr(str.find_first_not_of(View<const CharT>{
      string_constant<CharT, ' ', '\n', '\r', '\t', '\v', '\f'>{}}));
  }

  template<typename CharT>
  constexpr View<CharT> trim_ws(View<CharT> str) noexcept {
    str = skip_ws(str);
    std::size_t idx = str.find_last_not_of(View<const CharT>{
      string_constant<CharT, ' ', '\n', '\r', '\t', '\v', '\f'>{}});

    if (idx == View<CharT>::npos)
      return str;
    return str.substr(0, idx + 1);
  }

  /**
   * A parser for integers.
   *
   * @tparam T
   */
  template<typename T,
           typename CharT,
           Fmt Format = Fmt::normal | Fmt::hex | Fmt::binary>
  class Int {
    using traits = cli::traits::integer_traits<T>;
    using type = typename traits::type;
    using unsigned_type = typename traits::unsigned_type;

  protected:
    static constexpr std::size_t max_hex_length() noexcept {
      return sizeof(T) * 2;
    }

    static constexpr std::size_t max_bin_length() noexcept {
      return sizeof(T) * 8;
    }

    static constexpr std::size_t max_dec_length() noexcept {
      if constexpr (traits::is_signed) {
        switch (sizeof(type)) {
          case 1:
            return 4;
          case 2:
            return 7;
          case 4:
            return 11;
          default:
            return 11;
        }
      } else {
        switch (sizeof(type)) {
          case 1:
            return 3;
          case 2:
            return 6;
          case 4:
            return 10;
          default:
            return 10;
        }
      }
    }
    static constexpr ParseResult<T, CharT>
    parse_hex(View<const CharT> str, std::size_t offset) noexcept {
      constexpr auto to_hex = [](uint8_t c) -> uint8_t {
        if (c >= '0' and c <= '9') {
          return static_cast<CharT>(c - '0');
        } else if (c >= 'A' and c <= 'F') {
          return static_cast<CharT>(c - 'A' + 10);
        } else if (c >= 'a' and c <= 'f') {
          return static_cast<CharT>(c - 'a' + 10);
        } else {
          return 0xFFu;
        }
      };

      const std::size_t max_size = std::min(
        max_hex_length() + offset, str.size()); // the maximum amount of digits
                                                // we can consume with 32 bits.
      unsigned long long v = 0;
      for (std::size_t i = offset; i < max_size; ++i) {
        auto c = str[i];
        if (auto bits = to_hex(c); bits <= 0x0Fu) {
          v = (v * 16u) + bits;
        } else {
          // invalid character encountered, only an error if it is the first
          // character
          if (i == offset)
            return {Error::invalid_character, str};
          return {static_cast<type>(v), str.substr(i)};
        }
      }

      return {static_cast<type>(v), str.substr(max_size)};
    }

    static constexpr ParseResult<T, CharT>
    parse_bin(View<const CharT> str, std::size_t offset) noexcept {
      const std::size_t max_size = std::min(
        max_bin_length() + offset, str.size()); // the maximum amount of digits
                                                // we can consume with 32 bits.

      unsigned long long v = 0;
      for (std::size_t i = offset; i < max_size; ++i) {
        auto c = str[i];
        if (c == '0') {
          v = v << 1u;
        } else if (c == '1') {
          v = (v << 1u) | 1u;
        } else {
          // invalid character encountered, only an error if it is the first
          // character
          if (i == offset)
            return {Error::invalid_character, str};
          return {static_cast<type>(v), str.substr(i)};
        }
      }
      return {static_cast<type>(v), str.substr(max_size)};
    }

    static constexpr ParseResult<T, CharT>
    parse_dec(View<const CharT> str) noexcept {
      if constexpr (not traits::is_signed) {
        std::size_t offset = 0;
        if (str[0] == '+') {
          offset = 1;
          if (str.size() == 1)
            return {Error::too_few_characters, str};
        }

        unsigned long long v = 0;
        const std::size_t max_size =
          std::min(offset + max_dec_length(), str.size());
        for (std::size_t i = offset; i < max_size; ++i) {
          auto ch = str[i];
          if (ch >= '0' and ch <= '9') {
            v = v * 10 + (ch - '0');
          } else {
            // invalid character encountered, only an error if it is the first
            // character
            if (i == offset)
              return {Error::invalid_character, str};
            return {static_cast<type>(v), str.substr(i)};
          }
        }
        return {static_cast<type>(v), str.substr(max_size)};
      } else {
        std::size_t offset = 0;
        const bool negative = str[0] == '-';
        if (str[0] == '+' or negative) {
          offset = 1;
          if (str.size() == 1)
            return {Error::too_few_characters, str};
        }
        unsigned long long v = 0;
        const unsigned long long max_size =
          std::min(offset + max_dec_length(), str.size());
        for (std::size_t i = offset; i < max_size; ++i) {
          const auto ch = str[i];
          if (ch >= '0' and ch <= '9') {
            v = v * 10u + (ch - u'0');
          } else {
            // invalid character encountered, only an error if it is the first
            // character
            if (i == offset)
              return {Error::invalid_character, str};
            if (negative) {
              return {static_cast<type>(~v + 1), str.substr(i)};
            } else
              return {static_cast<type>(v), str.substr(i)};
          }
        }
        if (negative) {
          return {static_cast<type>(~v + 1), str.substr(max_size)};
        } else
          return {static_cast<type>(v), str.substr(max_size)};
      }
    }

  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      static_assert(static_cast<unsigned>(
                      Format & (Fmt::binary | Fmt::hex | Fmt::normal)) != 0,
                    "Invalid Fmt specified. Must be at least one of "
                    "Fmt::binary, Fmt::hex, or Fmt::normal");
      if (str.size() == 0)
        return Error::too_few_characters;

      if constexpr ((Format & Fmt::hex) == Fmt::hex) {
        // a hexadecimal number, starts with 0x or 0X
        if (str.size() > 2 and (str.starts_with("0x") or str.starts_with("0X")))
          return parse_hex(str, 2);
      }

      if constexpr ((Format & Fmt::binary) == Fmt::binary) {
        // a binary number, starts with 0b or 0B
        if (str.size() > 2 and (str.starts_with("0b") or str.starts_with("0B")))
          return parse_bin(str, 2);
      }

      if constexpr ((Format & Fmt::normal) == Fmt::normal) {
        // a decimal number
        return parse_dec(str);
      }

      return {Error::invalid_character, str};
    }
  };

  template<concepts::Character T, typename CharT>
  class Char {
  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      if (str.size() == 0)
        return Error::too_few_characters;

      CharT c = str[0];
      if (c == '\'') {
        if (str.size() < 3 or str[2] != '\'')
          return {Error::expected_endquote, str};
        return {static_cast<T>(str[1]), str.substr(3)};
      } else if (str.size() >= 3 and str[0] == '0' and
                 (str[1] == 'x' or str[1] == 'X')) {
        Int<T, CharT, Fmt::hex> parse;
        return parse(str);
      } else {
        return {static_cast<T>(c), str.substr(1)};
      }
    }
  };

  /**
   * A parser for stringviews.
   *
   */
  template<concepts::StringView T, typename CharT>
  class StringView {
  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      static_assert(
        std::is_same_v<CharT, typename T::value_type>,
        "The value_type of the stringview T must be the same as CharT");
      if (str.size() == 0)
        return Error::too_few_characters;

      if (str[0] == '"') {
        for (std::size_t i = 1; i < str.size(); ++i) {
          if (str[i] == '"' and str[i - 1] != '\\') {
            // reached end quote
            const auto value = str.substr(1, i - 1); // -1 to exclude quote
            return {T(value.data(), value.size()),
                    str.substr(i + 1)}; // +1 to exlude endquote
          }
        }
        return {Error::expected_endquote, str};
      } else {
        std::size_t end =
          str.find_first_of(View<const CharT>{string_constant<CharT,
                                                              ' ',
                                                              '\n',
                                                              '\r',
                                                              '\t',
                                                              '\v',
                                                              '\f',
                                                              '(',
                                                              ')',
                                                              '{',
                                                              '}',
                                                              ',',
                                                              '='>{}});
        if (end == 0)
          return {Error::invalid_character, str};
        View rest = str.substr(end);
        View value = str.substr(0, end);
        return {T(value.data(), value.size()), rest};
      }
    }
  };

  /**
   * @brief
   * @tparam CharT
   * @param str
   * @return
   */
  template<concepts::String T, typename CharT>
  class String {
  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      static_assert(
        std::is_same_v<CharT, typename T::value_type>,
        "The value_type of the stringview T must be the same as CharT");
      if (str.size() == 0)
        return Error::too_few_characters;

      T ret{};
      if (str[0] == '"') {
        for (std::size_t i = 1; i < str.size(); ++i) {
          if (i < (str.size() - 1) and str[i] == '\\' and str[i + 1] == '"') {
            ret.push_back('"');
            ++i;
          } else if (str[i] == '"' and str[i - 1] != '\\') {
            // reached end quote
            return {ret, str.substr(i + 1)}; // +1 to exlude endquote
          } else {
            ret.push_back(str[i]);
          }
        }
        return {Error::expected_endquote, str};
      } else {
        std::size_t end =
          str.find_first_of(View<const CharT>{string_constant<CharT,
                                                              ' ',
                                                              '\n',
                                                              '\r',
                                                              '\t',
                                                              '\v',
                                                              '\f',
                                                              '(',
                                                              ')',
                                                              '{',
                                                              '}',
                                                              ',',
                                                              '='>{}});
        if (end == 0) {
          return {Error::invalid_character, str};
        }

        for (std::size_t i = 0; i < str.size(); ++i) {
          if (View<const CharT>{string_constant<CharT,
                                                ' ',
                                                '\n',
                                                '\r',
                                                '\t',
                                                '\v',
                                                '\f',
                                                '(',
                                                ')',
                                                '{',
                                                '}',
                                                ',',
                                                '='>{}}
                .find(str[i]) != View<const CharT>::npos) {
            return {ret, str.substr(i)};
          } else if (i < (str.size() - 1) and str[i] == '\\' and
                     str[i + 1] == '"') {
            ret.push_back('"');
            ++i;
          } else {
            ret.push_back(str[i]);
          }
        }
        return {ret};
      }
    }
  };

  template<class T>
  class Float {
    static_assert(always_false<T>,
                  "The floating point parser is unimplemented for now");
  };

  template<concepts::FixPoint T,
           typename CharT,
           Fmt Format = Fmt::normal | Fmt::hex | Fmt::binary,
           CharT FixPointSeparator = '.'>
  class FixPoint : private Int<unsigned long long, CharT, Format> {
    using Base = Int<unsigned long long, CharT, Format>;
    using traits = traits::fixpoint_traits<T>;
    using unsigned_t = std::make_unsigned_t<typename traits::raw_value_type>;

    template<typename B, typename U>
    static constexpr B cxpow(B base, U exp) noexcept {
      if (exp == 0)
        return B(1);
      B ret = base;
      --exp;
      while (exp != 0) {
        --exp;
      }
      return ret;
    }

    static constexpr unsigned long long fraction_mask =
      (1u << traits::num_frac_digits) - 1u;

    static constexpr auto max_pow10_exp = [] {
      switch (traits::num_frac_digits) {
        case 0:
          return 19;
        case 1:
          return 18;
        case 2:
          return 18;
        case 3:
          return 18;
        case 4:
          return 18;
        case 5:
          return 17;
        case 6:
          return 17;
        case 7:
          return 17;
        case 8:
          return 16;
        case 9:
          return 16;
        case 10:
          return 16;
        case 11:
          return 15;
        case 12:
          return 15;
        case 13:
          return 15;
        case 14:
          return 15;
        case 15:
          return 14;
        case 16:
          return 14;
        case 17:
          return 14;
        case 18:
          return 13;
        case 19:
          return 13;
        case 20:
          return 13;
        case 21:
          return 12;
        case 22:
          return 12;
        case 23:
          return 12;
        case 24:
          return 12;
        case 25:
          return 11;
        case 26:
          return 11;
        case 27:
          return 11;
        case 28:
          return 10;
        case 29:
          return 10;
        case 30:
          return 10;
        case 31:
          return 9;
      }
    }();
    static constexpr u64 frac_factor = [] {
      u64 r{1};
      for (std::size_t i = 0; i < max_pow10_exp; ++i)
        r = 10 * r;
      return r;
    }();

    static constexpr u64 max_pow10 = [] {
      u64 r{1};
      for (std::size_t i = 0; i < max_pow10_exp; ++i)
        r = 10 * r;
      return r;
    }();
    static constexpr unsigned long long integer_mask =
      (1u << traits::num_int_digits) - 1u;

  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      // TODO: figure out a way to parse fixpoint numbers with a large
      // fractional portion, i.e. more than 10 decimal digits
      if (str.size() == 0)
        return Error::too_few_characters;

      if constexpr ((Format & Fmt::hex) == Fmt::hex) {
        // a hexadecimal number, starts with 0x, 0X, x, X, or #
        if (str.starts_with("0x") or str.starts_with("0X")) {
          if (auto res = Base::parse_hex(str, 2))
            return {typename traits::raw_value_type(res.value), res.rest};
          else
            return {res.error, res.rest};
        }
        if (str.find_first_of("xX#") == 0) {
          if (auto res = Base::parse_hex(str, 1))
            return {typename traits::raw_value_type(res.value), res.rest};
          else
            return {res.error, res.rest};
        }
      }
      if constexpr ((Format & Fmt::binary) == Fmt::binary) {
        // a binary number, starts with 0b, 0B, b, or B
        if (str.starts_with("0b") or str.starts_with("0B")) {
          if (auto res = Base::parse_bin(str, 2))
            return {typename traits::raw_value_type(res.value), res.rest};
          else
            return {res.error, res.rest};
        }
        if (str.find_first_of("bB") == 0) {
          {
            if (auto res = Base::parse_bin(str, 2))
              return {typename traits::raw_value_type(res.value), res.rest};
            else
              return {res.error, res.rest};
          }
        }
      }
      if constexpr ((Format & Fmt::normal) == Fmt::normal) {
        [[maybe_unused]] const bool negative = str[0] == '-';
        if constexpr (traits::is_signed) {
          if (negative)
            str = str.substr(1);
        } else {
          if (negative)
            return {Error::invalid_character, str};
        }
        // decimal format
        const auto int_res = Base::parse_dec(str);
        if (not int_res)
          return {int_res.error, int_res.rest};

        if (int_res.rest.size() == 0 or int_res.rest[0] != FixPointSeparator)
          return {T(typename traits::raw_value_type(
                    int_res.value << traits::num_frac_digits)),
                  int_res.rest};

        if (int_res.rest.size() < 2)
          return {Error::too_few_characters, str};

        str = int_res.rest.substr(1);

        const auto ch = str[0];
        if (ch < '0' or ch > '9')
          return {Error::invalid_character, str};

        // the fractional part (frac) is in base 10^-N=(2*5)^-N=5^-N*2^-N (N ==
        // string size) we want to convert it to base 2^-num_frac_digits -> frac
        // * 10^-N = i * 2^-num_frac_digits -> i is the integer value to store
        // as fractional bits
        //
        // i = frac * 10^-N / 2^-num_frac_digits = frac *
        // 2^num_frac_digits / 10^N = (frac << num_frac_digits) / 10^N

        // parsing the fractional part
        u64 value{static_cast<unsigned_t>(ch - '0')};
        u64 pow10{10};
        for (std::size_t i = 1; i < str.size(); ++i) {
          const auto c = str[i];
          if (c < '0' or c > '9') {
            str = str.substr(i);
            break;
          }
          pow10 = pow10 * 10;
          value = value * 10 + (c - '0');
        }

        u64 frac{value};

        const auto ret =
          [&int_res, &frac, &str, &negative]() -> ParseResult<T, CharT> {
          if constexpr (not traits::is_signed) {
            (void)negative;
            if (int_res.value > integer_mask)
              return {T(typename T::raw_value_type(integer_mask)), str};
            else
              return {T(typename T::raw_value_type(
                        (int_res.value << traits::num_frac_digits) |
                        (fraction_mask & frac.low()))),
                      str};
          } else {
            if (not negative) {
              if (int_res.value >= (1ull << (traits::num_frac_digits - 1))) {
                return {T(typename T::raw_value_type(
                          (1ull << (traits::num_frac_digits +
                                    traits::num_int_digits - 1)) -
                          1)),
                        str};
              } else {
                return {T(typename T::raw_value_type(
                          (int_res.value << traits::num_frac_digits) |
                          (fraction_mask & frac.low()))),
                        str};
              }
            }

            if (int_res.value > integer_mask) {
              return {T(typename T::raw_value_type(
                        (1ull << (traits::num_frac_digits +
                                  traits::num_int_digits - 1)) -
                        1)),
                      str};
            } else
              return {T(typename T::raw_value_type(
                        ~((int_res.value << traits::num_frac_digits) |
                          (fraction_mask & frac.low())) +
                        1)),
                      str};
          }
        };

        if (frac == 0)
          return ret();

        if (pow10 > max_pow10) {
          auto c = pow10 / max_pow10;
          frac = (frac / c) << traits::num_frac_digits;
          auto rest = u64::div64_with_mod(frac, max_pow10);
          if (rest > (max_pow10 >> 1) and frac != (fraction_mask)) {
            // need to round up
            frac = frac + 1;
          }
        } else {
          frac = frac << traits::num_frac_digits;
          auto rest = u64::div64_with_mod(frac, pow10);
          if (rest > (pow10 >> 1) and frac != (fraction_mask)) {
            // need to round up
            frac = frac + 1;
          }
        }

        return ret();
      }
      return {Error::invalid_value, str};
    }
  };

  template<concepts::Enum T, typename CharT, bool AllowNumbers>
  class Enum {
  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      if constexpr (traits::enum_traits<T>::is_flag) {
        T val{};
        bool read_one = false;
        bool has_or = false;
        View s = str;
        while (s.size() != 0) {
          bool found_name = false;
          for (const auto &[e, name] : ctti::dtl::enum_name_map<T>) {
            if (not s.starts_with(name))
              continue;

            read_one = true;
            found_name = true;
            val = val | e;
            s = s.substr(name.size());
            s = skip_ws(s);
            if (s.size() == 0 or s[0] != '|')
              return {from_value, val, s};
            has_or = true;
            s = skip_ws(s.substr(1));
          }

          if (has_or and not found_name)
            return {from_error, cli::Error::invalid_value, str};

          if (not found_name)
            break;
        }

        if (read_one)
          return {from_value, val, s};
        else {
          if constexpr (not AllowNumbers)
            return {from_error, Error::invalid_value, str};
          else {
            auto res = Int<std::underlying_type_t<T>, CharT>{}(str);
            if (not res or
                res.value > ctti::dtl::biggest_enum_value_v<T, CharT> or
                res.value < ctti::dtl::smallest_enum_value_v<T, CharT>)
              return {from_error, Error::invalid_value, str};
            return {from_value, static_cast<T>(res.value), res.rest};
          }
        }
      } else {
        T val{};
        View<const CharT> val_name{};
        bool has_val = false;
        for (const auto &[e, name] : ctti::dtl::enum_name_map<T>) {
          if (str.starts_with(name)) {
            val = e;
            val_name = name;
            has_val = true;
          }
        }

        if (has_val)
          return {from_value, val, str.substr(val_name.size())};

        if constexpr (not AllowNumbers) {
          return {from_error, Error::invalid_value, str};
        } else {
          using Parser = Int<std::underlying_type_t<T>, CharT>;
          auto res = Parser{}(str);
          if (not res or
              res.value > ctti::dtl::biggest_enum_value_v<T, CharT> or
              res.value < ctti::dtl::smallest_enum_value_v<T, CharT>)
            return {from_error, Error::invalid_value, str};
          return {from_value, static_cast<T>(res.value), res.rest};
        }
      }
    }
  };

  /**
   * @brief
   *
   */
  template<concepts::Sequence T,
           typename CharT,
           ParserOf<typename T::value_type, CharT> ElementParser,
           CharT Delimiter = ','>
  class Sequence {
  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      if (str.size() == 0)
        return {Error::too_few_characters, str};

      if (str[0] != '[')
        return {Error::expected_lbracket, str};

      View s = skip_ws(str.substr(1));

      if (s.size() == 0)
        return {Error::expected_rbracket, str};

      if (s[0] == ']')
        return {T{}, s.substr(1)};

      T sequence;

      while (s.size() > 0) {

        ParseResult res = ElementParser{}(s);

        if (not res)
          return {res.error, res.rest};

        if (sequence.size() < sequence.max_size())
          sequence.push_back(std::move(res.value));
        else
          return {Error::too_many_sequence_values, s};

        s = skip_ws(res.rest);

        if (s.size() == 0)
          return {Error::expected_rbracket, str};

        if (s[0] == ']')
          return {sequence, s.substr(1)};

        if (s[0] != Delimiter)
          return {Error::expected_delimiter, s};
        else
          s = skip_ws(s.substr(1));
      }
      return {Error::expected_rbracket, str};
    }
  };

  /**
   * @brief
   * @tparam T
   * @tparam CharT
   * @tparam ElementParser
   * @tparam Delimiter
   */
  template<concepts::FixedSizeSequence T,
           typename CharT,
           ParserOf<typename T::value_type, CharT> ElementParser,
           CharT Delimiter = ','>
  class FixedSizeSequence {
  public:
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT> str) const noexcept {
      if (str.size() == 0)
        return Error::too_few_characters;

      if (str[0] != '[')
        return {Error::expected_lbracket, str};

      View s = skip_ws(str.substr(1));

      if (s.size() == 0)
        return {Error::too_few_sequence_values, s};

      T sequence;
      std::size_t size = 0;

      if (s[0] == ']') {
        if (size < sequence.size())
          return {Error::too_few_sequence_values, str};
        return {T{}, s.substr(1)};
      }

      while (s.size() > 0) {

        ParseResult res = ElementParser{}(s);

        if (not res)
          return {Error::invalid_sequence_value, s};

        if (size >= sequence.size())
          return {Error::too_many_sequence_values, str};

        sequence[size] = std::move(res.value);
        ++size;

        s = skip_ws(res.rest);

        if (s.size() == 0) {
          if (size == sequence.size())
            return {Error::expected_rbracket, str};
          else
            return {Error::expected_delimiter, str};
        }

        if (s[0] == ']') {
          if (size != sequence.size())
            return {Error::too_few_sequence_values, str};
          return {sequence, s.substr(1)};
        }

        if (s[0] != Delimiter)
          return {Error::expected_delimiter, str};
        else
          s = skip_ws(s.substr(1));
      }
      return {Error::expected_rbracket, str};
    }
  };

  /**
   * @brief This class, and its various specializations, are used to parse
   * values.
   *
   * It is a hard error to use the unspecialized version.
   *
   * @ingroup Parsing
   * @tparam T the type to parse
   * @tparam CharT the character type of the buffer which is parsed from
   */
  template<class T, typename CharT>
  struct Parse {

    /**
     * @brief The call operator that is implemented in every specialization.
     *
     * @note the call operator doesn't have to be const.
     *
     * @param buf the buffer to parse from
     * @return ParseResult<T, CharT>
     */
    constexpr ParseResult<T, CharT>
    operator()(View<const CharT>) const noexcept {
      static_assert(always_false<T>, "No default parser for T available!");
    }
  };

  template<concepts::Character T, typename CharT>
  struct Parse<T, CharT> : public Char<T, CharT> {};

  template<concepts::Integer T, typename CharT>
  struct Parse<T, CharT> : public Int<T, CharT> {};

  template<concepts::Float T, typename CharT>
  struct Parse<T, CharT> : public Float<T> {};

  template<concepts::FixPoint T, typename CharT>
  struct Parse<T, CharT> : public FixPoint<T, CharT> {};

  template<concepts::StringView T, typename CharT>
  struct Parse<T, CharT> : public StringView<T, CharT> {};

  template<concepts::String T, typename CharT>
  struct Parse<T, CharT> : public String<T, CharT> {};

  template<concepts::Enum T, typename CharT>
  struct Parse<T, CharT> : Enum<T, CharT, false> {};

  template<typename CharT>
  struct Parse<bool, CharT> {
    static constexpr View<const CharT> truthy[]{
      "true", "TRUE", "1", "yes", "y"};
    static constexpr View<const CharT> falsy[]{
      "false", "FALSE", "0", "no", "n"};

  public:
    constexpr ParseResult<bool, CharT>
    operator()(View<const CharT> sv) const noexcept {
      if (sv.size() == 0)
        return Error::too_few_characters;

      for (const auto &true_str : truthy)
        if (sv.starts_with(true_str))
          return {true, sv.substr(true_str.size())};

      for (const auto &false_str : falsy)
        if (sv.starts_with(false_str))
          return {false, sv.substr(false_str.size())};

      return {Error::invalid_value, sv};
    }
  };

  template<concepts::Sequence T, typename CharT>
  struct Parse<T, CharT>
    : public Sequence<T, CharT, Parse<typename T::value_type, CharT>> {};

  template<concepts::FixedSizeSequence T, typename CharT>
  struct Parse<T, CharT>
    : public FixedSizeSequence<T, CharT, Parse<typename T::value_type, CharT>> {
  };

  template<typename CharT,
           class Name,
           auto DefaultValue,
           class Parser =
             Parse<std::remove_cvref_t<decltype(DefaultValue)>, CharT>>
  struct Field {
    using parser = Parser;
    using type = parse::value_type_t<CharT, Parser>;
    using name = Name;
    static constexpr bool has_default = true;
    static constexpr auto default_value = DefaultValue;
    type value = type(DefaultValue);
    Parser parse{};
  };

  template<typename CharT,
           class Name,
           class Type,
           class Parser = Parse<Type, CharT>>
  struct FieldWithOutDefault {
    using parser = Parser;
    using type = Type;
    using name = Name;
    static constexpr bool has_default = false;
    Type value{};
    Parser parse{};
  };

  template<typename CharT,
           CharT Assignment = '=',
           CharT MemberSeparator = ',',
           CharT Prefix = '{',
           CharT Postfix = '}',
           class... Fields>
  class FieldGroup {

    // Error error = Error::none;
    // std::size_t consumed = 0;
    // bool initialized[sizeof...(Fields)]{};
    // bool optional[sizeof...(Fields)]{};
    // cli::Tuple<Fields...> fields{};
    struct State {
      Error error = Error::none;
      std::size_t consumed = 0;
      bool initialized[sizeof...(Fields)]{};
      bool optional[sizeof...(Fields)]{};
      cli::Tuple<Fields...> fields{};
      constexpr bool all_fields_have_a_value() const {
        for (std::size_t i = 0; i < sizeof...(Fields); ++i) {
          if (not(initialized[i] or optional[i]))
            return false;
        }
        return true;
      }
    };
    using Pair =
      std::pair<View<const CharT>, void (*)(State &, View<const CharT> &)>;

    static constexpr Pair parsers[]{
      Pair{View<const CharT>(typename Fields::name{}),
           +[](State &state, View<const CharT> &sv) {
             constexpr auto index =
               type_list::index_of_v<Fields, cli::Tuple<Fields...>>;
             auto res = get<index>(state.fields).parse(sv);
             if (not res) {
               state.error = res.error;
               return;
             }

             // parse success -> set value and remember that this field
             // has been initialized
             get<index>(state.fields).value = res.value;
             state.initialized[index] = true;
             ++state.consumed;
             sv = skip_ws(res.rest);
           }}
      ...
    };

    State s;

    void reset_state() {
      s.error = Error::none;
      s.consumed = 0;
      for (auto &b : s.initialized)
        b = false;
      for_each(
        [](auto &field) {
          using F = std::remove_cvref_t<decltype(field)>;
          if constexpr (F::has_default)
            field.value = typename F::type(F::default_value);
          else
            field.value = typename F::type{};
        },
        s.fields);
    }

  public:
    template<class... Fs>
    constexpr FieldGroup(Fs &&...fields) noexcept
      : s{.fields{std::forward<Fs>(fields)...}} {
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, cli::Tuple<Fields...>>;
          if constexpr (F::has_default) {
            /// self.initialized[index] = true;
            self.optional[index] = true;
          }
        },
        s.fields,
        s);
    }

    template<class... Fs>
    constexpr FieldGroup(cli::Tuple<Fs...> &&fields) noexcept
      : s{.fields = std::move(fields)} {
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, cli::Tuple<Fields...>>;
          if constexpr (F::has_default) {
            /// self.initialized[index] = true;
            self.optional[index] = true;
          }
        },
        s.fields,
        s);
    }

    template<class... Fs>
    constexpr FieldGroup(const cli::Tuple<Fs...> &fields) noexcept
      : s{.fields = fields} {
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, cli::Tuple<Fields...>>;
          if constexpr (F::has_default) {
            /// self.initialized[index] = true;
            self.optional[index] = true;
          }
        },
        s.fields,
        s);
    }

    constexpr FieldGroup(const FieldGroup &) = default;
    constexpr FieldGroup(FieldGroup &&) = default;
    constexpr FieldGroup &operator=(const FieldGroup &) = default;
    constexpr FieldGroup &operator=(FieldGroup &&) = default;

    constexpr ParseResult<cli::Tuple<Fields...>, CharT>
    operator()(View<const CharT> str) noexcept {
      if (str.size() == 0) {
        if constexpr ((Fields::has_default and ...))
          return s.fields;
        else if constexpr (Prefix == '(') {
          return {Error::expected_lparen, str};
        } else if constexpr (Prefix == '{') {
          return {Error::expected_lbrace, str};
        } else {
          return {Error::too_few_characters, str};
        }
      }

      View sv = str;
      if constexpr (Prefix == ' ')
        sv = skip_ws(sv);
      else {
        if (sv[0] == Prefix) {
          sv = skip_ws(sv.substr(1));
        } else {
          if constexpr (Prefix == '(') {
            return {Error::expected_lparen, str};
          } else if constexpr (Prefix == '{') {
            return {Error::expected_lbrace, str};
          } else {
            return {Error::expected_group_opening, str};
          }
        }
      }

      if (sv.size() == 0) {
        if constexpr (Prefix == '(') {
          return {Error::expected_rparen};
        } else if constexpr (Prefix == '{') {
          return {Error::expected_rbrace};
        } else {
          return {Error::expected_group_closing};
        }
      }
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, type_list::TypeList<Fields...>>;
          if constexpr (F::has_default) {
            /// self.initialized[index] = true;
            self.optional[index] = true;
          }
        },
        s.fields,
        s);

      if (sv[0] == Postfix) {
        if (s.all_fields_have_a_value()) {
          auto res = std::move(s.fields);
          reset_state();
          return {std::move(res), sv.substr(1)};
        }
        if constexpr (Postfix == ')')
          return {Error::expected_args, sv};
        else
          return {Error::expected_field, sv};
      }

      std::size_t pos_index = 0;
      while (s.error == Error::none and s.consumed < sizeof...(Fields)) {
        std::size_t parser_index = 0;
        bool is_named_member = false;
        for (const auto &[name, parser] : parsers) {
          if (sv.starts_with(name)) {
            // stripping name
            auto rest = skip_ws(sv.substr(name.size()));
            // now the assignment character is expected. If it is not present,
            // then this has to be a value
            if (rest.size() == 0 or rest[0] != Assignment) {
              // wasn't really the name -> try value
              break;
            }

            // consume the assignment character and trailing whitespace
            rest = skip_ws(rest.substr(1));
            if (rest.size() == 0) {
              reset_state();
              return {Error::expected_value, sv};
            }

            sv = rest;
            is_named_member = true;
            break;
          }
          ++parser_index;
        }

        if (not is_named_member) {
          const auto cache_idx = pos_index;
          // member is unnamed, put it into the first uninitialized slot
          for (; pos_index < sizeof...(Fields); ++pos_index) {
            if (not s.initialized[pos_index]) {
              break;
            }
          }
          if (pos_index >= sizeof...(Fields)) {
            pos_index = cache_idx;
            bool found = false;
            for (; pos_index < sizeof...(Fields); ++pos_index) {
              if (s.optional[pos_index]) {
                found = true;
                break;
              }
            }
            if (not found) {
              reset_state();
              return Error::implementation_error;
            }
          }
          parser_index = pos_index;
        }

        (*(parsers[parser_index].second))(s, sv);

        if (s.error != Error::none) {
          auto err = s.error;
          reset_state();
          return {err, sv};
        }

        if (s.consumed == sizeof...(Fields)) {
          // TODO: check that fields are initialized
          if constexpr (Postfix == ' ') {
            auto fields = s.fields;
            reset_state();
            return {std::move(fields), sv};
          } else {
            if (sv.size() == 0) {
              reset_state();
              if constexpr (Postfix == ')') {
                return {Error::expected_rparen, str};
              } else if constexpr (Postfix == '}') {
                return {Error::expected_rbrace, str};
              } else {
                return {Error::expected_group_opening, str};
              }
            }
            if (sv[0] != Postfix) {
              reset_state();
              if constexpr (Postfix == ')') {
                return {Error::expected_rparen, str};
              } else if constexpr (Postfix == '}') {
                return {Error::expected_rbrace, str};
              } else {
                return {Error::expected_group_opening, str};
              }
            } else {
              auto fields = s.fields;
              reset_state();
              return {std::move(fields), sv.substr(1)};
            }
          }
        }

        if (sv.size() == 0) {
          if (s.all_fields_have_a_value() and Postfix == ' ') {
            auto fields = s.fields;
            reset_state();
            return {std::move(fields)};
          }

          reset_state();
          if constexpr (Postfix == ')')
            return {Error::expected_another_arg, sv};
          else
            return {Error::expected_another_field, sv};
        }

        if (sv[0] == Postfix and s.all_fields_have_a_value()) {
          auto fields = s.fields;
          reset_state();
          return {std::move(fields), sv.substr(1)};
        }

        if (sv[0] != MemberSeparator) {
          reset_state();
          return {Error::expected_delimiter, sv};
        }

        sv = skip_ws(sv.substr(1));
      }
      auto err = s.error;
      auto fields = s.fields;
      reset_state();
      if (err != Error::none)
        return {err, sv};
      else
        return {fields, sv};
    }
  };

  template<typename CttiField, typename CharT>
  struct to_parse_field {
    using name = typename CttiField::name;
    using type_ = typename CttiField::type;
    using parser = Parse<type_, CharT>;
    using type = FieldWithOutDefault<CharT, name, type_, parser>;
  };

  template<concepts::Struct T,
           typename CharT,
           CharT Assignment = '=',
           CharT MemberSeparator = ',',
           CharT Prefix = '{',
           CharT Postfix = '}'>
  struct field_parser_for {
    template<class... Fields>
    using type_ = FieldGroup<CharT,
                             Assignment,
                             MemberSeparator,
                             Prefix,
                             Postfix,
                             Fields...>;

    template<typename F>
    using tpf = to_parse_field<F, CharT>;

    using type = type_list::apply_t<
      type_,
      type_list::transform_t<tpf, typename ctti::TypeInfo<T>::fields>>;
  };

  template<concepts::Struct T,
           typename CharT,
           class Name = string_constant<CharT>,
           CharT Assignment = '=',
           CharT MemberSeparator = ',',
           CharT Prefix = '{',
           CharT Postfix = '}'>
  class Struct
    : field_parser_for<T, CharT, Assignment, MemberSeparator, Prefix, Postfix>::
        type {
    using Base = typename field_parser_for<T,
                                           CharT,
                                           Assignment,
                                           MemberSeparator,
                                           Prefix,
                                           Postfix>::type;

  public:
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) noexcept {
      if (str.size() == 0)
        return Error::too_few_characters;

      View sv = str;
      if constexpr (Name::string_size > 0) {
        constexpr StringLiteral name{Name{}};
        if (name.size() > 0 and sv.starts_with(name)) {
          // named struct
          sv = skip_ws(sv.substr(name.size()));

          if (sv.size() == 0 or sv[0] != Assignment)
            return {Error::expected_assignment, sv};

          sv = skip_ws(sv.substr(1));

          if (sv.size() == 0 or (Prefix != ' ' and sv[0] != Prefix))
            return {Error::expected_group_opening, sv};

          sv = skip_ws(sv.substr(1));
        }
      }

      if (auto res = static_cast<Base *>(this)->operator()(sv))
        return {ctti::from_tuple<T>(res.value), res.rest};
      else
        return {res.error, sv};
    }
  };

  template<concepts::Struct T, typename CharT>
  struct Parse<T, CharT> : public Struct<T, CharT> {};

  template<typename CharT>
  class NullParse {
  public:
    constexpr ParseResult<dummy, CharT>
    operator()(View<const CharT> str) const noexcept {
      return {dummy{}, str};
    }
  };

} // namespace cli::parse
#endif
