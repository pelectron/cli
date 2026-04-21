/**
 * @file
 * @brief This file contains the utilities for parsing values from strings
 *
 * @defgroup Parsing
 *
 * Parsing in CLI is based on two things:
 * - the class cli::parse::ParseResult: the result of a parsing operation.
 * - cli::parse::Parser: the parser concept.
 *
 * A parser of T is a callable that parses a T from a string and returns a
 * ParseResult. It takes a cli::View<const CharT> as its first and only
 * argument and returns a cli::parse::ParseResult<T, CharT>.
 *
 * Example:
 * ```
 *  cli::parse::ParseResult<bool, char>
 *  parse_bool(vli::View<const char> buf){
 *    if(buf.starts_with("true")){
 *      return {true, buf.substr(4)};
 *    }else if(buf.starts_with("false")){
 *      return {false, buf.substr(5)};
 *    }else{
 *      return cli::Error::invalid_character;
 *    }
 *  }
 * ```
 *
 * CLI provides defaults for parsing for the following types:
 * - bool
 * - characters
 * - enumerations
 * - integers
 * - fixpoint numbers
 * - sequences, i.e. arrays/lists/vectors
 * - strings
 * - aggregates, i.e. simple structs
 *
 * @section enum-parsing Enum Parsing
 *
 * enum parsing is available for enum classes. These must be enum class and not
 * weak C style enumerations, if the enum values have gaps in them. If your enum
 * is signed and only has values in the range [-128, 127] or your enum is
 * unsigned and has values in the range of [0, 255], then you don't have to
 * write anything to get enum class parsing suport. If that is not the case, or
 * your enum is a weaka enum, you will have to write your own enum traits. Do
 * this by defining cli::traits::enum_traits and adjusting the parameters.
 *
 * ```
 *  #include "cli/traits.hpp"
 *
 *  namespace your_namespace{
 *    enum class YourEnum : std::unit32_t{
 *        A = 5,
 *        ...
 *        ABCD = 300
 *    };
 *  }
 *
 *  namespace cli::traits{
 *    template<>
 *    struct enum_traits<your_namespace::YourEnum>{
 *      // the minimum value
 *      static constexpr unit32_t min = 5;
 *      // the maximum value
 *      static constexpr uint32_t max = 300;
 *      // set to false if your enum is not a flag enum, else true
 *      static constexpr bool is_flag = false;
 *    };
 *  }
 * ```
 *
 * Enumerations don't include the enum class name when formatted and parsed by
 * CLI. In the exampe above, the string "A" would be parsed as ``YourEnum::A``
 * and "ABCD" would be parsed as ``YourEnum::ABCD``.
 *
 * @section sequence-parsing Sequence Parsing
 *
 * To tell CLI that your type is a sequence, you must specialize
 * cli::traits::is_sequence or cli::traits::is_fixed_size_sequence.
 * If your type then also conforms to the (fixed size) sequence interface, your
 * type will be parsable by CLI.
 *
 * A fixed sequence is a list of fixed size, for example arrrays.
 * A non-fixed sequence is a list of variable size, for example
 * cli::FixedCapacityVector.
 *
 * Sequences have the following format:
 *
 * ``[e1, e2, e3, ..., en]``
 *
 * where ``ex`` are the elements of the sequence, delimited by commas.
 *
 * See the concepts cli::traits::Sequence and cli::traits::FixedSizeSequence for
 * the requirements on the interface.
 *
 * Example for specializing the traits structure:
 *
 * ```
 * #include "cli/traits.hpp"
 * namespace A{
 *  class Vec{
 *  public:
 *    using value_type = T;
 *    Vec();
 *    Vec(const Vec&);
 *    iterator begin();
 *    iterator end();
 *    std::size_t size() const;
 *    std::size_t max_size() const;
 *    void push_back(const T&);
 *  };
 *
 *  class Array{
 *  public:
 *    using value_type = T;
 *    Array();
 *    Array(const Array&);
 *    iterator begin();
 *    iterator end();
 *    std::size_t size() const;
 *    T& operator[](std::size_t i);
 *  };
 * }
 *
 * namespace cli::traits{
 *  template<>
 *  struct is_sequence<A::Vec> : std::true_type{};
 *
 *  template<>
 *  struct is_fixed_size_sequence<A::Array> : std::true_type{};
 * }
 *
 * static_assert(cli::traits::Sequence<A::Vec>);
 * static_assert(cli::traits::FixedSizeSequence<A::Array>);
 * ```
 * @section string-parsing String Parsing
 *
 * To enable parsing for your custom string type, specialize
 * cli::traits::is_string and conform to the interface detailed by the concept
 * cli::traits::String.
 *
 * Strings can either be unescaped, in which case they can't contain spaces, or
 * escaped with the " character. Inner " characters can be escaped by backslash.
 *
 * Examples of valid strings:
 *
 * ```
 * hello
 * "hello world"
 * "hello \"world\""
 * hello"world
 * ```
 *
 * Example:
 *
 * ```
 *  #include "cli/traits.hpp"
 *  class MyString{
 *    using value_type = char;
 *    MyString(const value_type* s, std::size_t n);
 *  };
 *
 *  namespace cli::traits{
 *    template<>
 *    struct is_string<MyString> : std::true_type{};
 *  }
 * ```
 *
 * There is also parsing for string views, i.e. non owning strings, available.
 * Keep in mind howver that escaped quotes will remain as they are and not
 * converted due to views not owning any memory. For string views,
 * cli::traits::is_string_view must be specialized.
 *
 * @section fixpoint-parsing Fixpoint Parsing
 *
 * TODO
 *
 * @section struct-parsing Struct Parsing
 *
 * Simple aggregates can be decomposed and parsed by CLI as long as each member
 * is parseable.
 *
 * The format for aggregates is
 *
 * ``{name1 = v1, name2 = v2, v3, name4 = v4}``
 *
 * namex are he member names, vx are the member values. Keyword elements
 * (namex = vx) and value elements (vx) can be mixed in any way.
 *
 * Concrete Example:
 *
 * ```
 * struct S{
 *   int i;
 *   char c;
 *   cli::View<const char> str;
 * };
 *
 * cli::parse::DefaultParse<S,char> parse{};
 *
 * cli::parse::ParseResult p = parse("{1,k,\"hello world\"}")
 * assert(p);
 * assert(p.value == S{1, 'k', "hello world"});
 *
 * p = parse("{c=k, 1, str = \"hello world\"}")
 * assert(p);
 * assert(p.value == S{1, 'k', "hello world"});
 *
 * p = parse("{str = \"hello world\", 1, k}")
 * assert(p);
 * assert(p.value == S{1, 'k', "hello world"});
 * ```
 *
 * @section custom-parsing Custom Parsing
 *
 * Custom parser just have to conform to the Parser concept.
 * To enable CLI to select your implementation by default, which is needed
 * if you want your types parsable when they are members of a struct, then
 * explicitly specialize cli::parse::DefaultParse and implement it's
 * call operator.
 *
 */

#ifndef CLI_PARSE_HPP
#define CLI_PARSE_HPP

#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/traits.hpp"
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
    constexpr ParseResult(Error error)
      : error{error}, value{}, rest{} {}

    /**
     * construct a successful parse result from a value and the rest of the
     * string that hasn't been parsed.
     *
     * @param value the parse value
     * @param rest the unparsed rest of the string
     */
    constexpr ParseResult(const T &value, View<const CharT> rest = {})
      : error{Error::none}, value{value}, rest{rest} {}

    /**
     * construct a successful parse result from a value and the rest of the
     * string that hasn't been parsed.
     *
     * @param value the parse value
     * @param rest the unparsed rest of the string
     */
    constexpr ParseResult(T &&value, View<const CharT> rest = {})
      : error{Error::none}, value{std::move(value)}, rest{rest} {}

    /**
     * construct a successful parse result from a value and the rest of the
     * string that hasn't been parsed.
     *
     * @param value the parse value
     * @param rest the unparsed rest of the string
     */
    template<class U>
    constexpr ParseResult(from_value_t, U &&value, View<const CharT> rest = {})
      : error{Error::none}, value{std::forward<U>(value)}, rest{rest} {}

    /// constructs a failed parse with the error reason
    constexpr ParseResult(from_error_t, Error e)
      : error{e}, value{}, rest{} {}

    /// returns true if this is a successful parse result, i.e. if the error is
    /// Error::none.
    constexpr operator bool() const noexcept { return error == Error::none; }

    constexpr auto operator<=>(const ParseResult &) const = default;

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
  concept ParserOf =
    requires(std::remove_cvref_t<P> &parse, View<const CharT> str) {
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
  concept Parser = Callable<P> and is_const_view_v<buffer_type_t<P>> and
                   is_parse_result_v<result_type_t<P>>;

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
    ParseResult<T, CharT> operator()(View<const CharT>) const {
      return {Error::unimplemented};
    }
  };

  template<typename CharT>
  constexpr View<CharT> skip_ws(View<CharT> str) {
    return str.substr(str.find_first_not_of(" \n\r\t\v\f"));
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
    static constexpr std::size_t max_hex_length() {
      switch (sizeof(type)) {
        case 1:
          return 2;
        case 2:
          return 4;
        case 4:
          return 8;
        default:
          return 8;
      }
    }
    static constexpr std::size_t max_bin_length() {
      switch (sizeof(type)) {
        case 1:
          return 8;
        case 2:
          return 16;
        case 4:
          return 32;
        default:
          return 32;
      }
    }
    static constexpr std::size_t max_dec_length() {
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
    static constexpr ParseResult<T, CharT> parse_hex(View<const CharT> str,
                                                     std::size_t offset) {
      constexpr auto to_hex = [](uint8_t c) -> uint8_t {
        if (c >= '0' and c <= '9') {
          return c - '0';
        } else if (c >= 'A' and c <= 'F') {
          return c - 'A' + 10;
        } else if (c >= 'a' and c <= 'f') {
          return c - 'a' + 10;
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
            return Error::invalid_character;
          return {static_cast<type>(v), str.substr(i)};
        }
      }

      return {static_cast<type>(v), str.substr(max_size)};
    }

    static constexpr ParseResult<T, CharT> parse_bin(View<const CharT> str,
                                                     std::size_t offset) {
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
            return Error::invalid_character;
          return {static_cast<type>(v), str.substr(i)};
        }
      }
      return {static_cast<type>(v), str.substr(max_size)};
    }

    static constexpr ParseResult<T, CharT> parse_dec(View<const CharT> str) {
      if constexpr (not traits::is_signed) {
        std::size_t offset = 0;
        if (str[0] == '+') {
          offset = 1;
          if (str.size() == 1)
            return Error::too_few_characters;
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
              return Error::invalid_character;
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
            return Error::too_few_characters;
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
              return Error::invalid_character;
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
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
      if (str.size() == 0)
        return Error::too_few_characters;

      if constexpr ((Format & Fmt::hex) == Fmt::hex) {
        // a hexadecimal number, starts with 0x, 0X, x, X, or #
        if (str.starts_with("0x") or str.starts_with("0X"))
          return parse_hex(str, 2);
        if (str.find_first_of("xX#") == 0)
          return parse_hex(str, 1);
      }

      if constexpr ((Format & Fmt::binary) == Fmt::binary) {
        // a binary number, starts with 0b, 0B, b, or B
        if (str.starts_with("0b") or str.starts_with("0B"))
          return parse_bin(str, 2);
        if (str.find_first_of("xX#") == 0)
          return parse_bin(str, 1);
      }

      if constexpr ((Format & Fmt::normal) == Fmt::normal) {
        // a decimal number
        return parse_dec(str);
      }

      return Error::invalid_value;
    }
  };

  template<traits::Character T, typename CharT>
  class Char {
  public:
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
      if (str.size() == 0)
        return Error::too_few_characters;

      CharT c = str[0];
      if (c == '\'') {
        if (str.size() < 3)
          return Error::too_few_characters;
        if (str[2] != '\'')
          return Error::invalid_character;
        return {static_cast<T>(str[1]), str.substr(3)};
      } else if (c >= '0' and c <= '9') {
        Int<T, CharT> parse;
        return parse(str);
      } else {
        return {static_cast<T>(c), str.substr(1)};
      }
    }
  };

  /**
   * A parser for stringviews.
   *
   * '' -> invalid
   * ' ' -> invalid
   * '""' -> ''
   * '\"' -> '\"'
   * '\"\"' -> '\"\"'
   * 'hello' -> 'hello'
   * 'hello world' -> 'hello', rest = ' world'
   * '"hello world"' -> 'hello world'
   * 'hello"world' -> 'hello"world'
   * 'hello\"world' -> 'hello\"world'
   * '"hello \"world\""' -> 'hello \"world\"'
   */
  template<traits::StringView T, typename CharT>
  class StringView {
  public:
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
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
        return Error::unescaped_string;
      } else {
        for (std::size_t i = 0; i < str.size(); ++i) {
          if (str[i] == ' ') {
            if (i == 0)
              return Error::invalid_character;
            const auto value = str.substr(0, i);
            return {T(value.data(), value.size()), str.substr(i)};
          }
        }
        return {T(str.data(), str.size())};
      }
    }
  };

  /**
   * @brief
   *
   * '' -> invalid
   * ' ' -> invalid
   * '""' -> ''
   * '\"' -> '"'
   * '\"\"' -> '""'
   * 'hello' -> 'hello'
   * 'hello world' -> 'hello', rest = ' world'
   * '"hello world"' -> 'hello world'
   * 'hello"world' -> 'hello"world'
   * 'hello\"world' -> 'hello"world'
   * '"hello \"world\""' -> 'hello "world"'
   * @tparam CharT
   * @param str
   * @return
   */
  template<traits::String T, typename CharT>
  class String {
  public:
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
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
        return Error::unescaped_string;
      } else {
        for (std::size_t i = 0; i < str.size(); ++i) {
          if (str[i] == ' ') {
            if (i == 0) {
              return cli::Error::invalid_character;
            }
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

  template<traits::FixPoint T,
           typename CharT,
           Fmt Format = Fmt::normal | Fmt::hex | Fmt::binary,
           CharT FixPointSeparator = '.'>
  class FixPoint : private Int<unsigned long long, CharT, Format> {
    using Base = Int<unsigned long long, CharT, Format>;
    using traits = traits::fixpoint_traits<T>;
    using unsigned_t = std::make_unsigned_t<typename traits::raw_value_type>;

    template<typename B, typename U>
    static constexpr B cxpow(B base, U exp) {
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
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
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
            return res.error;
        }
        if (str.find_first_of("xX#") == 0) {
          if (auto res = Base::parse_hex(str, 1))
            return {typename traits::raw_value_type(res.value), res.rest};
          else
            return res.error;
        }
      }
      if constexpr ((Format & Fmt::binary) == Fmt::binary) {
        // a binary number, starts with 0b, 0B, b, or B
        if (str.starts_with("0b") or str.starts_with("0B")) {
          if (auto res = Base::parse_bin(str, 2))
            return {typename traits::raw_value_type(res.value), res.rest};
          else
            return res.error;
        }
        if (str.find_first_of("bB") == 0) {
          {
            if (auto res = Base::parse_bin(str, 2))
              return {typename traits::raw_value_type(res.value), res.rest};
            else
              return res.error;
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
            return Error::invalid_character;
        }
        // decimal format
        const auto int_res = Base::parse_dec(str);
        if (not int_res)
          return int_res.error;

        if (int_res.rest.size() == 0 or int_res.rest[0] != FixPointSeparator)
          return {T(typename traits::raw_value_type(
                    int_res.value << traits::num_frac_digits)),
                  int_res.rest};

        if (int_res.rest.size() < 2)
          return Error::too_few_characters;

        str = int_res.rest.substr(1);

        const auto ch = str[0];
        if (ch < '0' or ch > '9')
          return Error::invalid_character;

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
          const auto ch = str[i];
          if (ch < '0' or ch > '9') {
            str = str.substr(i);
            break;
          }
          pow10 = pow10 * 10;
          value = value * 10 + (ch - '0');
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
      return Error::invalid_value;
    }
  };

  template<traits::Enum T, typename CharT, bool AllowNumbers>
  class Enum {
  public:
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
      if constexpr (traits::FlagEnum<T>) {
        T val{};
        bool read_one = true;
        while (str.size() != 0) {
          bool found_name = false;
          for (const auto &[e, name] : ctti::dtl::enum_name_map<T>) {
            if (not str.starts_with(name))
              continue;

            read_one = true;
            found_name = true;
            val = val | e;
            str = str.substr(name.size());
            auto s = skip_ws(str);
            if (s.size() == 0 or s[0] != '|')
              return {val, str};
            str = skip_ws(s.substr(1));
          }
          if (not found_name)
            break;
        }

        if (read_one)
          return {val, {}};
        else {
          if constexpr (not AllowNumbers)
            return error<CharT, T>(Error::invalid_value);
          else {
            auto res = Int<std::underlying_type_t<T>, CharT>{}(str);
            if (not res)
              return error<CharT, T>(Error::invalid_value);
            return {static_cast<T>(res.value), res.rest};
          }
        }
      } else {
        for (const auto &[e, name] : ctti::dtl::enum_name_map<T>) {
          if (str.starts_with(name)) {
            return {e, str.substr(name.size())};
          }
        }

        if constexpr (not AllowNumbers)
          return error<CharT, T>(Error::invalid_value);
        else {
          using Parser = Int<std::underlying_type_t<T>, CharT>;
          auto res = Parser{}(str);
          if (not res) {
            return error<CharT, T>(Error::invalid_value);
          }
          return {static_cast<T>(res.value), res.rest};
        }
      }
    }
  };

  /**
   * @brief
   *
   */
  template<traits::Sequence T,
           typename CharT,
           ParserOf<traits::sequence_value_type_t<T>, CharT> ElementParser,
           CharT Delimiter = ','>
  class Sequence {
  public:
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
      if (str.size() == 0)
        return Error::too_few_characters;

      if (str[0] != '[')
        return Error::invalid_character;

      str = skip_ws(str.substr(1));

      if (str.size() == 0)
        return Error::too_few_characters;

      if (str[0] == ']')
        return {T{}, str.substr(1)};

      T sequence;

      while (str.size() > 0) {

        ParseResult res = ElementParser{}(str);

        if (not res)
          return res.error;

        sequence.push_back(std::move(res.value));

        if (res.rest.size() == 0)
          return Error::too_few_characters;

        str = skip_ws(res.rest);

        if (str.size() == 0)
          return Error::too_few_characters;

        if (str[0] == ']')
          return {sequence, str.substr(1)};

        if (str[0] != Delimiter)
          return Error::invalid_character;
        else
          str = skip_ws(str.substr(1));
      }
      return Error::too_few_characters;
    }
  };

  /**
   * @brief
   * @tparam T
   * @tparam CharT
   * @tparam ElementParser
   * @tparam Delimiter
   */
  template<traits::FixedSizeSequence T,
           typename CharT,
           ParserOf<traits::sequence_value_type_t<T>, CharT> ElementParser,
           CharT Delimiter = ','>
  class FixedSizeSequence {
  public:
    constexpr ParseResult<T, CharT> operator()(View<const CharT> str) const {
      if (str.size() == 0)
        return Error::too_few_characters;

      if (str[0] != '[')
        return Error::invalid_character;

      str = skip_ws(str.substr(1));

      if (str.size() == 0)
        return Error::too_few_characters;

      T sequence;
      std::size_t size = 0;

      if (str[0] == ']') {
        if (size < sequence.size())
          return Error::too_few_sequence_values;
        return {T{}, str.substr(1)};
      }

      while (str.size() > 0) {

        ParseResult res = ElementParser{}(str);

        if (not res)
          return res.error;

        if (size >= sequence.size())
          return Error::too_many_sequence_values;

        sequence[size] = std::move(res.value);
        ++size;

        if (res.rest.size() == 0)
          return Error::too_few_characters;

        str = skip_ws(res.rest);

        if (str.size() == 0)
          return Error::too_few_characters;

        if (str[0] == ']') {
          if (size != sequence.size())
            return Error::too_few_sequence_values;
          return {sequence, str.substr(1)};
        }

        if (str[0] != Delimiter)
          return Error::invalid_character;
        else
          str = skip_ws(str.substr(1));
      }
      return Error::too_few_characters;
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
  struct DefaultParse {

    /**
     * @brief The call operator that is implemented in every specialization.
     *
     * @note the call operator doesn't have to be const.
     *
     * @param buf the buffer to parse from
     * @return ParseResult<T, CharT>
     */
    constexpr ParseResult<T, CharT> operator()(View<const CharT> buf) const {
      static_assert(always_false<T>, "No default parser for T available!");
    }
  };

  template<traits::Character T, typename CharT>
  struct DefaultParse<T, CharT> : public Char<T, CharT> {};

  template<traits::Integer T, typename CharT>
  struct DefaultParse<T, CharT> : public Int<T, CharT> {};

  template<traits::Float T, typename CharT>
  struct DefaultParse<T, CharT> : public Float<T> {};

  template<traits::FixPoint T, typename CharT>
  struct DefaultParse<T, CharT> : public FixPoint<T, CharT> {};

  template<traits::StringView T, typename CharT>
  struct DefaultParse<T, CharT> : public StringView<T, CharT> {};

  template<traits::String T, typename CharT>
  struct DefaultParse<T, CharT> : public String<T, CharT> {};

  template<traits::Enum T, typename CharT>
  struct DefaultParse<T, CharT> : Enum<T, CharT, false> {};

  template<typename CharT>
  struct DefaultParse<bool, CharT> {
    static constexpr View<const CharT> truthy[]{
      "true", "TRUE", "1", "yes", "y"};
    static constexpr View<const CharT> falsy[]{
      "false", "FALSE", "0", "no", "n"};

  public:
    constexpr ParseResult<bool, CharT> operator()(View<const CharT> sv) const {
      if (sv.size() == 0)
        return Error::too_few_characters;

      for (const auto &t : truthy)
        if (sv.starts_with(t))
          return {true, sv.substr(t.size())};

      for (const auto &f : falsy)
        if (sv.starts_with(f))
          return {false, sv.substr(f.size())};

      return Error::invalid_character;
    }
  };

  template<traits::Sequence T, typename CharT>
  struct DefaultParse<T, CharT>
    : public Sequence<T, CharT, DefaultParse<typename T::value_type, CharT>> {};

  template<traits::FixedSizeSequence T, typename CharT>
  struct DefaultParse<T, CharT>
    : public FixedSizeSequence<
        T,
        CharT,
        DefaultParse<traits::sequence_value_type_t<T>, CharT>> {};

  template<typename CharT,
           class Name,
           auto DefaultValue,
           class Parser =
             DefaultParse<std::remove_cvref_t<decltype(DefaultValue)>, CharT>>
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
           class Parser = DefaultParse<Type, CharT>>
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
    // std::tuple<Fields...> fields{};
    struct State {
      Error error = Error::none;
      std::size_t consumed = 0;
      bool initialized[sizeof...(Fields)]{};
      bool optional[sizeof...(Fields)]{};
      std::tuple<Fields...> fields{};
    };
    using Pair =
      std::pair<View<const CharT>, void (*)(State &, View<const CharT> &)>;

    static constexpr Pair parsers[]{
      Pair{View<const CharT>(typename Fields::name{}),
           +[](State &state, View<const CharT> &sv) {
             constexpr auto index =
               type_list::index_of_v<Fields, std::tuple<Fields...>>;
             auto res = std::get<index>(state.fields).parse(sv);
             if (not res) {
               state.error = res.error;
               return;
             }

             // parse success -> set value and remember that this field
             // has been initialized
             std::get<index>(state.fields).value = res.value;
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
    constexpr FieldGroup(Fs &&...fields)
      : s{.fields{std::forward<Fs>(fields)...}} {
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, std::tuple<Fields...>>;
          if constexpr (F::has_default) {
            /// self.initialized[index] = true;
            self.optional[index] = true;
          }
        },
        s.fields,
        s);
    }

    template<class... Fs>
    constexpr FieldGroup(std::tuple<Fs...> &&fields)
      : s{.fields = std::move(fields)} {
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, std::tuple<Fields...>>;
          if constexpr (F::has_default) {
            /// self.initialized[index] = true;
            self.optional[index] = true;
          }
        },
        s.fields,
        s);
    }

    template<class... Fs>
    constexpr FieldGroup(const std::tuple<Fs...> &fields)
      : s{.fields = fields} {
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, std::tuple<Fields...>>;
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

    constexpr ParseResult<std::tuple<Fields...>, CharT>
    operator()(View<const CharT> sv) {
      if (sv.size() == 0) {
        if constexpr ((Fields::has_default && ...))
          return s.fields;
        else
          return Error::too_few_characters;
      }

      if constexpr (Prefix == ' ')
        sv = skip_ws(sv);
      else {
        if (sv[0] == Prefix) {
          sv = skip_ws(sv.substr(1));
        } else {
          return Error::invalid_character;
        }
      }

      if (sv.size() == 0)
        return Error::too_few_characters;
      for_each(
        [](const auto &f, State &self) {
          using F = std::remove_cvref_t<decltype(f)>;
          constexpr auto index =
            type_list::index_of_v<F, std::tuple<Fields...>>;
          if constexpr (F::has_default) {
            /// self.initialized[index] = true;
            self.optional[index] = true;
          }
        },
        s.fields,
        s);

      std::size_t pos_index = 0;
      while (s.error == Error::none and s.consumed < sizeof...(Fields)) {
        std::size_t parser_index = 0;
        bool is_named_member = false;
        for (const auto &[name, parser] : parsers) {
          if (sv.starts_with(name)) {
            // stripping name
            auto str = skip_ws(sv.substr(name.size()));
            // now the assignment character is expected. If it is not present,
            // then this has to be a value
            if (str.size() == 0 or str[0] != Assignment) {
              // wasn't really the name -> try value
              break;
            }

            // consume the assignment character and trailing whitespace
            str = skip_ws(str.substr(1));
            if (str.size() == 0) {
              reset_state();
              return Error::too_few_characters;
            }

            sv = str;
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
          return err;
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
              return Error::too_few_characters;
            }
            if (sv[0] != Postfix) {
              reset_state();
              return Error::invalid_character;
            } else {
              auto fields = s.fields;
              reset_state();
              return {std::move(fields), sv.substr(1)};
            }
          }
        } else {
          if (sv.size() == 0) {
            reset_state();
            return Error ::too_few_characters;
          }

          if (sv[0] != MemberSeparator) {
            reset_state();
            return Error::invalid_character;
          }

          sv = skip_ws(sv.substr(1));
        }
      }
      auto err = s.error;
      auto fields = s.fields;
      reset_state();
      if (err != Error::none)
        return err;
      else
        return fields;
    }
  };

  template<typename CttiField, typename CharT>
  struct to_parse_field {
    using name = typename CttiField::name;
    using type_ = typename CttiField::type;
    using parser = DefaultParse<type_, CharT>;
    using type = FieldWithOutDefault<CharT, name, type_, parser>;
  };

  template<traits::Struct T,
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

  template<traits::Struct T,
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
    constexpr ParseResult<T, CharT> operator()(View<const CharT> sv) {
      if (sv.size() == 0)
        return Error::too_few_characters;

      if constexpr (Name::string_size > 0) {
        constexpr StringLiteral name{Name{}};
        if (name.size() > 0 and sv.starts_with(name)) {
          // named struct
          sv = skip_ws(sv.substr(name.size()));

          if (sv.size() == 0)
            return Error::too_few_characters;

          if (sv[0] != Assignment)
            return Error::invalid_character;

          sv = skip_ws(sv.substr(1));

          if (sv.size() == 0)
            return Error::too_few_characters;

          if (sv[0] != Prefix)
            return Error::invalid_character;

          sv = skip_ws(sv.substr(1));
        }
      }

      if (auto res = static_cast<Base *>(this)->operator()(sv))
        return {ctti::from_tuple<T>(res.value), res.rest};
      else
        return res.error;
    }
  };

  template<traits::Struct T, typename CharT>
  struct DefaultParse<T, CharT> : public Struct<T, CharT> {};

  template<typename CharT>
  class NullParse {
  public:
    constexpr ParseResult<dummy, CharT>
    operator()(View<const CharT> str) const {
      return {dummy{}, str};
    }
  };

} // namespace cli::parse
#endif
