#ifndef CLI_BASIC_FORMAT_HPP
#define CLI_BASIC_FORMAT_HPP
#include "cli/enums.hpp"
#include "cli/string.hpp"
#include "cli/traits.hpp"
#include "cli/u64.hpp"
#include "cli/util.hpp"

#include <bit>
#include <cassert>
#include <cstdint>
#include <type_traits>
namespace cli::format {
  /**
   * This class represent the result of a formatting operation.
   *
   * It contains an error and the number of characters formatted. Although it
   * has both memebrs, it should be thought of as a variant/union. If the error
   * is Error::none, then size_written contains the number of characters used
   * for formatting. If the error is not Error::none, then size_written is
   * always 0.
   *
   * @ingroup Formatting
   */
  struct FormatResult {

    constexpr FormatResult(Error e) noexcept
      : error(e) {}

    constexpr FormatResult(std::size_t size) noexcept
      : size_written(size) {}

    /// returns true if error is Error::none, else false.
    constexpr operator bool() const noexcept { return error == Error::none; }

    Error error{cli::Error::none};
    std::size_t size_written{0};
  };

  template<class F>
  struct formatter_value_type {
    using type = std::remove_cvref_t<type_list::type_at_t<
      1,
      typename function_traits<std::decay_t<F>>::arguments>>;
  };

  template<class F>
  using formatter_value_type_t = typename formatter_value_type<F>::type;

  template<class F>
  struct formatter_buffer_type {
    using type = std::remove_cvref_t<type_list::type_at_t<
      0,
      typename function_traits<std::decay_t<F>>::arguments>>;
  };

  template<class F>
  using formatter_buffer_type_t = typename formatter_buffer_type<F>::type;

  /**
   * This concept denotes a formatter for T, i.e. a callable that turns a T
   * into a string.
   *
   * A Formatter takes a cli::View<CharT> as its first argument and a T as its
   * second argument, and returns a FormatResult. The first argument specifies
   * the buffer into which the second argument should be formatted into.
   *
   * See @ref Formatting for more info.
   *
   * @ingroup Formatting
   * @tparam F the formatter
   * @tparam T the type to format
   * @tparam CharT the character type of the buffer
   */
  template<typename F, typename T, typename CharT>
  concept FormatterOf = requires(F &&f, View<CharT> buf, const T &t) {
    { f(buf, t) } -> std::same_as<FormatResult>;
  } and not std::is_const_v<CharT>;

  /**
   * A Formatter takes a cli::View<CharT> as its first argument and a ``T`` as
   * its second argument, and returns a cli::format::FormatResult. The first
   * argument specifies the buffer into which the second argument should be
   * formatted into. See @ref Formatting for more info.
   *
   * @ingroup Formatting
   * @tparam F the formatter
   */
  template<class F>
  concept Formatter =
    cli::dtl::is_non_const_view_v<
      typename cli::format::formatter_buffer_type<F>::type> and
    (not std::same_as<void,
                      typename cli::format::formatter_value_type<F>::type>);

  /**
   * @brief This class, and its various specializations, are used to format
   * values.
   *
   * It is a hard error to use the unspecialized version.
   *
   * @ingroup Formatting
   * @tparam T the type to format
   * @tparam CharT the buffer's character type
   */
  template<class T, typename CharT>
  struct Format {

    /**
     * @brief The call operator that is implemented in every specialization.
     *
     * @note the call operator doesn't have to be const.
     *
     * @param buf the buffer to format into
     * @param t the value to format
     * @return FormatResult
     */
    constexpr FormatResult operator()(View<CharT>, const T &) const noexcept {
      static_assert(always_false<T>, "There is no Format defined for T!");
      return 0;
    }
  };

  template<typename CharT>
  struct NullFormat {
    constexpr FormatResult operator()(View<CharT>,
                                      const cli::dummy &) const noexcept {
      return 0;
    }
  };

  template<typename T, typename CharT>
  struct NoFormat {
    constexpr FormatResult operator()(View<CharT>, const T &) const noexcept {
      // TODO: add no_format_available to Error.
      return Error::unimplemented;
    }
  };

  /**
   * Formatter for void.
   * @tparam CharT the character type
   */
  template<typename CharT>
  struct Format<void, CharT> {
    constexpr FormatResult operator()(View<CharT>) const noexcept { return 0; }
  };

  template<typename CharT>
  struct Format<View<CharT>, CharT> {
    constexpr FormatResult operator()(View<CharT> buf,
                                      View<CharT> s) const noexcept {
      if (s.size() > buf.size())
        return Error::buffer_overflow;
      for (std::size_t i = 0; i < s.size(); ++i)
        buf[i] = s[i];
      return s.size();
    }
  };

  template<typename CharT>
  struct Format<View<const CharT>, CharT> {
    constexpr FormatResult operator()(View<CharT> buf,
                                      View<const CharT> s) const noexcept {
      if (s.size() > buf.size())
        return Error::buffer_overflow;
      for (std::size_t i = 0; i < s.size(); ++i)
        buf[i] = s[i];
      return s.size();
    }
  };

  /**
   * @brief The default formatter for bool
   *
   * @tparam CharT the character type
   */
  template<typename CharT>
  struct Format<bool, CharT> {
    constexpr FormatResult operator()(View<CharT> buf, bool b) const noexcept {
      if (b) {
        if (buf.size() < 4)
          return Error::buffer_overflow;
        std::size_t size = 0;
        for (const auto &ch : "true")
          buf[size++] = ch;
        return 4;
      } else {
        if (buf.size() < 5)
          return Error::buffer_overflow;
        std::size_t size = 0;
        for (const auto &ch : "false")
          buf[size++] = ch;
        return 5;
      }
    }
  };

  /**
   * An integer formatter.
   *
   * @tparam T the integer type
   * @tparam CharT the character type
   * @tparam Format the number format to used. Can be normal (i.e. decimal),
   * hex, or binary.
   * @tparam UseSignForPositive if true, a leading '+' character will be used
   * for positive values.
   */
  template<typename T,
           typename CharT,
           Fmt Format = Fmt::normal,
           bool UseSignForPositive = false>
  struct Int {
    static constexpr std::size_t num_dec_digits() {
      using traits = traits::integer_traits<T>;
      if constexpr (not traits::is_signed) {
        if constexpr (traits::max >= uint64_t{10'000'000'000'000'000'000u})
          return 21;
        else if constexpr (traits::max >= uint64_t{1'000'000'000'000'000'000u})
          return 20;
        else if constexpr (traits::max >= uint64_t{100'000'000'000'000'000u})
          return 19;
        else if constexpr (traits::max >= uint64_t{10'000'000'000'000'000u})
          return 18;
        else if constexpr (traits::max >= uint64_t{1'000'000'000'000'000u})
          return 17;
        else if constexpr (traits::max >= uint64_t{100'000'000'000'000u})
          return 16;
        else if constexpr (traits::max >= uint64_t{10'000'000'000'000u})
          return 15;
        else if constexpr (traits::max >= uint64_t{1'000'000'000'000u})
          return 14;
        else if constexpr (traits::max >= uint64_t{100'000'000'000u})
          return 13;
        else if constexpr (traits::max >= uint64_t{10'000'000'000u})
          return 12;
        else if constexpr (traits::max >= uint32_t{1'000'000'000u})
          return 11;
        else if constexpr (traits::max >= uint32_t{100'000'000u})
          return 10;
        else if constexpr (traits::max >= uint32_t{10'000'000u})
          return 9;
        else if constexpr (traits::max >= uint32_t{1'000'000u})
          return 8;
        else if constexpr (traits::max >= uint32_t{100'000u})
          return 7;
        else if constexpr (traits::max >= uint32_t{10'000u})
          return 6;
        else if constexpr (traits::max >= uint32_t{1'000u})
          return 5;
        else if constexpr (traits::max >= uint32_t{100u})
          return 4;
        else if constexpr (traits::max >= uint32_t{10u})
          return 3;
        else if constexpr (traits::max >= uint32_t{1u})
          return 2;
        else
          static_assert(always_false<T>, "Cannot use Format for T");
      } else {
        if constexpr (traits::max >= int64_t{1'000'000'000'000'000'000} or
                      traits::min <= int64_t{-1'000'000'000'000'000'000})
          return 20;
        else if constexpr (traits::max >= int64_t{100'000'000'000'000'000} or
                           traits::min <= int64_t{-100'000'000'000'000'000})
          return 19;
        else if constexpr (traits::max >= int64_t{10'000'000'000'000'000} or
                           traits::min <= int64_t{-10'000'000'000'000'000})
          return 18;
        else if constexpr (traits::max >= int64_t{1'000'000'000'000'000} or
                           traits::min <= int64_t{-1'000'000'000'000'000})
          return 17;
        else if constexpr (traits::max >= int64_t{100'000'000'000'000} or
                           traits::min <= int64_t{-100'000'000'000'000})
          return 16;
        else if constexpr (traits::max >= int64_t{10'000'000'000'000} or
                           traits::min <= int64_t{-10'000'000'000'000})
          return 15;
        else if constexpr (traits::max >= int64_t{1'000'000'000'000} or
                           traits::min <= int64_t{-1'000'000'000'000'000'000})
          return 14;
        else if constexpr (traits::max >= int64_t{100'000'000'000} or
                           traits::min <= int64_t{-100'000'000'000})
          return 13;
        else if constexpr (traits::max >= int64_t{10'000'000'000} or
                           traits::min <= int64_t{-10'000'000'000})
          return 12;
        else if constexpr (traits::max >= int64_t{1'000'000'000} or
                           traits::min <= int64_t{-1'000'000'000})
          return 11;
        else if constexpr (traits::max >= int64_t{100'000'000} or
                           traits::min <= int64_t{-1'000'000'000'000'000'000})
          return 10;
        else if constexpr (traits::max >= int64_t{10'000'000} or
                           traits::min <= int64_t{-10'000'000})
          return 9;
        else if constexpr (traits::max >= int64_t{1'000'000} or
                           traits::min <= int64_t{-1'000'000})
          return 8;
        else if constexpr (traits::max >= int64_t{100'000} or
                           traits::min <= int64_t{-100'000})
          return 7;
        else if constexpr (traits::max >= int64_t{10'000} or
                           traits::min <= int64_t{-10'000})
          return 6;
        else if constexpr (traits::max >= int64_t{1'000} or
                           traits::min <= int64_t{-1'000})
          return 5;
        else if constexpr (traits::max >= int64_t{100} or
                           traits::min <= int64_t{-100})
          return 4;
        else if constexpr (traits::max >= int64_t{10} or
                           traits::min <= int64_t{-10})
          return 3;
        else if constexpr (traits::max >= int64_t{1} or
                           traits::min <= int64_t{-1})
          return 2;
        else
          static_assert(always_false<T>, "Cannot use Format for T");
      }
    }

    constexpr FormatResult operator()(View<CharT> buf, T value) const noexcept {
      using traits = traits::integer_traits<T>;
      using UnsignedT = typename traits::unsigned_type;

      if constexpr (Format == Fmt::normal) {
        if (value == 0) {
          if constexpr (UseSignForPositive) {
            if (buf.size() < 2)
              return Error::buffer_overflow;
            buf[0] = '+';
            buf[1] = '0';
            return 2;
          } else {
            if (buf.size() < 1)
              return Error::buffer_overflow;
            buf[0] = '0';
            return 1;
          }
        }

        if constexpr (traits::is_signed) {
          constexpr T max_pow_10 = []() {
            T t{1};
            for (std::size_t i = 0; i < num_dec_digits() - 2 and
                                    (t < traits::max or -t > traits::min);
                 ++i)
              t = static_cast<T>(10 * t);
            return t;
          }();
          UnsignedT u_value = static_cast<UnsignedT>(value);
          const bool is_negative = value < 0;
          char buffer[num_dec_digits()]{};
          std::size_t size = 0;
          if constexpr (UseSignForPositive) {
            if (value > 0) {
              buffer[0] = '+';
              ++size;
            }
          }
          if (is_negative) {
            buffer[0] = '-';
            ++size;
            u_value = static_cast<UnsignedT>((~u_value) + 1u);
          }

          T pow10 = max_pow_10;
          for (; pow10 > 0; pow10 /= 10)
            if (u_value >= static_cast<UnsignedT>(pow10))
              break;
          for (; pow10 > 0; pow10 /= 10) {
            const auto digit = u_value / pow10;
            buffer[size++] = static_cast<CharT>(digit + '0');
            u_value = static_cast<UnsignedT>(u_value - digit * pow10);
          }

          if (buf.size() < size)
            return Error::buffer_overflow;

          for (std::size_t i = 0; i < size; ++i)
            buf[i] = buffer[i];
          return size;
        } else {
          constexpr T max_pow_10 = []() {
            T t{1};
            for (std::size_t i = 0;
                 i < num_dec_digits() - 2 and t < traits::max;
                 ++i)
              if (t > (10u * t))
                return t;
              else
                t = static_cast<T>(10u * t);
            return t;
          }();
          char buffer[num_dec_digits()]{};
          std::size_t size = 0;
          if constexpr (UseSignForPositive) {
            buffer[0] = '+';
            ++size;
          }

          T pow10 = max_pow_10;
          for (; pow10 > 0; pow10 /= 10)
            if (value >= pow10)
              break;

          for (; pow10 > 0; pow10 /= 10) {
            const auto digit = value / pow10;
            buffer[size++] = static_cast<CharT>(digit + u'0');
            value = static_cast<T>(value - digit * pow10);
          }

          if (buf.size() < size)
            return Error::buffer_overflow;

          for (std::size_t i = 0; i < size; ++i)
            buf[i] = buffer[i];

          return size;
        }
      } else if constexpr (Format == Fmt::hex) {
        if (value == 0) {
          if (buf.size() < 3)
            return Error::buffer_overflow;
          buf[0] = '0';
          buf[1] = 'x';
          buf[2] = '0';
          return 3;
        }

        UnsignedT u_value = static_cast<UnsignedT>(value);
        constexpr std::size_t max_size = 2 + sizeof(UnsignedT) * 2;
        char buffer[max_size]{'0', 'x', 0};
        std::size_t size = 2;
        const auto max_nibble = (sizeof(UnsignedT) * 2) - 1u;
        unsigned nibble = (sizeof(UnsignedT) * 2) - 1u;
        for (; nibble != 0; --nibble) {
          if (u_value < (1u << (nibble * 4u)))
            continue;
          else
            break;
        }
        for (; nibble <= max_nibble; --nibble) {
          const auto digit =
            static_cast<CharT>(0x0Fu & (u_value >> (nibble * 4u)));
          if (digit >= 0 and digit <= 9)
            buffer[size++] = static_cast<CharT>(digit + '0');
          else if (digit >= 10 and digit <= 15)
            buffer[size++] = static_cast<CharT>(digit - 10u + 'A');
          else
            assert(false);
          u_value =
            static_cast<UnsignedT>(u_value & ((1u << (nibble * 4u)) - 1u));
        }

        if (buf.size() < size)
          return Error::buffer_overflow;

        for (std::size_t i = 0; i < size; ++i)
          buf[i] = buffer[i];

        return size;
      } else if constexpr (Format == Fmt::binary) {
        if (value == 0) {
          if (buf.size() < 3)
            return Error::buffer_overflow;
          buf[0] = '0';
          buf[1] = 'b';
          buf[2] = '0';
          return 3;
        }
        const UnsignedT u_value = static_cast<UnsignedT>(value);
        constexpr std::size_t max_size = 2 + sizeof(UnsignedT) * 8;
        char buffer[max_size]{'0', 'b', 0};
        std::size_t size = 2;
        const auto max_bit = sizeof(UnsignedT) * 8 - 1;
        unsigned bit =
          static_cast<unsigned>(max_bit - std::countl_zero(u_value));
        for (; bit <= max_bit; --bit) {
          const auto digit = (u_value >> bit) & 1u;
          buffer[size++] = static_cast<CharT>(digit + '0');
        }

        if (buf.size() < size)
          return Error::buffer_overflow;

        for (std::size_t i = 0; i < size; ++i)
          buf[i] = buffer[i];

        return size;
      } else {
        static_assert(
          always_false<decltype(Format)>,
          "Invalid Format supplied to format::Int, must be one of Fmt::normal, "
          "Fmt::hex, or Fmt::binary. Mixed Fmt values are not allowed.");
      }
    }
  };

  /**
   * The default formatter for integers.
   *
   * @tparam T the integer type
   * @tparam CharT the character type
   */
  template<concepts::Integer T, typename CharT>
  struct Format<T, CharT> : public Int<T, CharT> {};

  /**
   * @brief A character formatter. This will format the characters quoted if
   * Format is Fmt::normal. Else it will format the values as the corresponding
   * hex/binary value.
   *
   * @tparam T the type of the character to format
   * @tparam CharT the buffer character type
   * @tparam Format how to format characters
   */
  template<concepts::Character T, typename CharT, Fmt Format = Fmt::normal>
  struct Char {
    constexpr FormatResult operator()(View<CharT> buf, T value) const noexcept {
      if constexpr (Format == Fmt::normal) {
        if (buf.size() < 3)
          return Error::buffer_overflow;

        buf[0] = '\'';
        buf[1] = value;
        buf[2] = '\'';
        return 3;
      } else {
        return Int<T, CharT, Format>{}(buf, value);
      }
    }
  };

  /**
   * @brief The default character formatter.
   *
   * @tparam T the type of the character to format
   * @tparam CharT the buffer character type
   */
  template<concepts::Character T, typename CharT>
  struct Format<T, CharT> : public Char<T, CharT> {};

  /**
   * @brief a formatter for fixpoint numbers.
   * To use this formatter, you must tell cli to enable use of T by implementing
   * cli::traits::is_fixpoint and implmenting or conforming to
   * cli::traits::fixpoint_traits. See the cli/traits.hpp header for more
   * information.
   *
   * @tparam CharT
   */
  template<concepts::FixPoint T,
           typename CharT,
           std::size_t Precision = 11,
           bool PrintTrailingZeros = false,
           Fmt Format = Fmt::normal,
           bool UseSignForPositive = false,
           CharT FixPointSeparator = '.'>
  class FixPoint {
    using traits = traits::fixpoint_traits<T>;
    using raw_value = typename traits::raw_value_type;
    using int_type =
      std::conditional_t<traits::is_signed, long long, unsigned long long>;

    static_assert(
      traits::num_frac_digits >= 1 and traits::num_frac_digits <= 27,
      "fix point number with more than 27 fractional digits are not supported");
    // frac is the raw integer value of the fractional part of value with F
    // bits. This needs to be converted to a decimal integer value for
    // formatting. The integer value needed can be calculated as follows: value
    // = round(frac / max_frac * 10^precision) = round(frac / 2^F *
    // 10^precision) = round(frac
    // * 2^-F * 10^precision). factor = 2^-F * 10^precision can be calculated at
    // compile time, with only integer divisions at run time. This will
    // unfortunately produce some rounding errors.
    static constexpr auto factor = []() {
      u64 u{10};
      for (std::size_t i = 0; i < Precision; ++i) {
        u = u * 10;
      }
      return u >> traits::num_frac_digits;
    }();
    static constexpr uint8_t max_frac_string_length{traits::num_frac_digits};
    static constexpr u64 bin_weight{[] {
      switch (traits::num_frac_digits) {
        case 1:
          return u64{5u, 0u};
        case 2:
          return u64{25u, 0u};
        case 3:
          return u64{125u, 0u};
        case 4:
          return u64{625u, 0u};
        case 5:
          return u64{3125u, 0u};
        case 6:
          return u64{15625u, 0u};
        case 7:
          return u64{78125u, 0u};
        case 8:
          return u64{390625u, 0u};
        case 9:
          return u64{1953125u, 0u};
        case 10:
          return u64{9765625u, 0u};
        case 11:
          return u64{48828125u, 0u};
        case 12:
          return u64{244140625u, 0u};
        case 13:
          return u64{1220703125u, 0u};
        case 14:
          return u64{1808548329u, 1u};
        case 15:
          return u64{452807053u, 7u};
        case 16:
          return u64{2264035265u, 35u};
        case 17:
          return u64{2730241733u, 177u};
        case 18:
          return u64{766306777u, 888u};
        case 19:
          return u64{3831533885u, 4440u};
        case 20:
          return u64{1977800241u, 22204u};
        case 21:
          return u64{1299066613u, 111022u};
        case 22:
          return u64{2200365769u, 555111u};
        case 23:
          return u64{2411894253u, 2775557u};
        case 24:
          return u64{3469536673u, 13877787u};
        case 25:
          return u64{167814181u, 69388939u};
        case 26:
          return u64{839070905u, 346944695u};
        case 27:
          return u64{4195354525u, 1734723475u};
        default:
          assert(false);
      };
    }()};
    static constexpr unsigned long long integer_mask =
      (1ull << traits::num_int_digits) - 1u;
    static constexpr unsigned long long fraction_mask =
      (1ull << traits::num_frac_digits) - 1u;
    static constexpr unsigned long long value_mask =
      (1ull << (traits::num_frac_digits + traits::num_int_digits)) - 1u;
    static constexpr unsigned long long sign_mask =
      1ull << (traits::num_frac_digits + traits::num_int_digits - 1u);

    static constexpr std::uint32_t len() {
      switch (traits::num_frac_digits) {
        case 1:
          return 1u;
        case 2:
          return 2u;
        case 3:
          return 3u;
        case 4:
          return 3u;
        case 5:
          return 4u;
        case 6:
          return 5u;
        case 7:
          return 5u;
        case 8:
          return 6u;
        case 9:
          return 7u;
        case 10:
          return 7u;
        case 11:
          return 8u;
        case 12:
          return 9u;
        case 13:
          return 10u;
        case 14:
          return 10u;
        case 15:
          return 11u;
        case 16:
          return 12u;
        case 17:
          return 12u;
        case 18:
          return 13u;
        case 19:
          return 14u;
        case 20:
          return 14u;
        case 21:
          return 15u;
        case 22:
          return 16u;
        case 23:
          return 17u;
        case 24:
          return 17u;
        case 25:
          return 18u;
        case 26:
          return 19u;
        case 27:
          return 19u;
        default:
          assert(false);
      }
    }
    static constexpr auto max_pow10_exp = [] {
      switch (traits::num_frac_digits) {
        case 0:
          return 1;
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

    static_assert(Precision < max_pow10_exp,
                  "The Precision is too high for the amount of bits to print.");
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

    static constexpr u64 precision_factor = [] {
      u64 r{1};
      for (std::size_t i = 0; i < max_pow10_exp - Precision; ++i)
        r = 10 * r;
      return r;
    }();
    static constexpr u64 pow10_precision = [] {
      u64 r{1};
      for (std::size_t i = 0; i < Precision; ++i)
        r = 10 * r;
      return r;
    }();
    static constexpr auto billion = 1'000'000'000;

  public:
    constexpr FormatResult operator()(View<CharT> buf, T fp) const noexcept {
      if constexpr (Format == Fmt::normal) {

        uint32_t int_val = fp.integer();
        uint32_t frac_val = fp.fraction();
        std::size_t written = 0;
        bool negative = false;

        if constexpr (traits::is_signed) {
          if (auto v = static_cast<unsigned long long>(fp.value());
              (v & sign_mask) == sign_mask) {
            // fp is negative -> convert to twos complement
            if (buf.size() < 2)
              return Error::buffer_overflow;
            negative = true;
            v = (~v + 1u) & value_mask;
            int_val = (v >> traits::num_frac_digits) & integer_mask;
            frac_val = v & fraction_mask;
          }
        }

        // v is the decimal representation of the fractional part, i.e. the
        // actual number to print in terms of max_pow10/10. Dividing v by
        // precision_factor give the number to print in terms of 10^Precision
        u64 v = ((frac_factor * u64{frac_val}) >> traits::num_frac_digits);
        // dividing by precision_factor to round
        // vnext = v / precision_factor, rest = v % precision_factor
        u64 vnext = v;
        u64 rest = u64::div64_with_mod(vnext, precision_factor);

        // std::cout << "v: " << v.v << std::endl;
        // std::cout << "rest: " << rest.v << std::endl;
        // std::cout << "precision_factor: " << precision_factor.v << std::endl;
        if (rest > (1 + (precision_factor >> 1))) {
          // round up because the rest is bigger than pow10_precision/2
          v = vnext + 1;
          if (v == pow10_precision) {
            int_val = int_val + 1;
            v = u64{0};
          }
        } else {
          v = vnext;
        }

        if constexpr (traits::is_signed) {
          if (negative and (int_val != 0 or v != 0)) {
            // add minus sign if the value to be printed is not 0
            buf[written++] = u'-';
            buf = buf.substr(1);
          }
        }

        // formatting the integer part
        auto res =
          Int<int_type, CharT, Fmt::normal, UseSignForPositive>{}(buf, int_val);
        if (not res)
          return res.error;

        buf = buf.substr(res.size_written);

        if (buf.size() < 2)
          return Error::buffer_overflow;

        // adding decimal separator
        buf[0] = FixPointSeparator;
        written += res.size_written + 1;

        buf = buf.substr(1);
        std::size_t buf_idx = 0;

        if (v == 0) {
          // fractional part is zero-> print single 0 and print trailing zeros
          // in case we should
          buf[buf_idx++] = '0';
          if constexpr (PrintTrailingZeros) {
            if (buf.size() < Precision + buf_idx)
              return Error::buffer_overflow;
            while (buf_idx < Precision)
              buf[buf_idx++] = '0';
          }
          return written + buf_idx;
        }

        auto pow10 = pow10_precision / 10;

        if (buf.size() < Precision)
          return Error::buffer_overflow;
        // printing leading zeros. These occur when pow10 is greater than the
        // value to format.
        while (pow10 > v and buf_idx < Precision) {
          buf[buf_idx++] = '0';
          pow10 = pow10 / 10;
        }

        // printing the fractional value
        while (pow10 > 0 and buf_idx < Precision) {
          if (buf.size() < buf_idx)
            return Error::buffer_overflow;

          // calculate the next digit to print and the rest. Done in this
          // roundabout way to support architectures which dont have uint64_t.
          //
          // next = v/pow10, rest = v % pow10
          u64 next = v;
          u64 r = u64::div64_with_mod(next, pow10);

          if (next >= 10)
            return Error::implementation_error;

          buf[buf_idx++] = next.low() + '0';
          if (r == 0)
            break;
          v = rest;
          pow10 = pow10 / 10;
        }

        if constexpr (PrintTrailingZeros) {
          if (buf_idx < Precision) {
            if (buf.size() < Precision - buf_idx)
              return Error::buffer_overflow;
            while (buf_idx < Precision)
              buf[buf_idx++] = '0';
          }
        }
        return written + buf_idx;
      } else if constexpr (Format == Fmt::hex) {
        return Int<raw_value, CharT, Fmt::hex>{}(buf, fp.value());
      } else if constexpr (Format == Fmt::binary) {
        return Int<raw_value, CharT, Fmt::binary>{}(buf, fp.value());
      } else {
        static_assert(
          always_false<decltype(Format)>,
          "Supplied invalid Fmt to Format::FixPoint. Must be one of "
          "Fmt::normal, Fmt::hex and Fmt::binary.");
      }
    }
  };

  template<concepts::FixPoint T, typename CharT>
  struct Format<T, CharT> : public FixPoint<T, CharT> {};

  template<concepts::Float T,
           typename CharT,
           Fmt Format = Fmt::normal,
           char FixPointSeparator = '.'>
  class Float {
    static_assert(always_false<T>, "Not implemented yet");
  };

  /**
   * @brief This is a formatter for sequences. To use this with your custom
   * type, cli::traits::is_sequence must be overridden.
   *
   * Example:
   * ```
   *  // my_vec.hpp
   *  #include "cli/traits.hpp"
   *  namespace abc{
   *    class Vector{
   *      public:
   *      using value_type = T;
   *      using iterator = T*;
   *
   *      Vector();
   *      void push_back(const T& t);
   *      iterator begin();
   *      iterator end();
   *    };
   *  }
   *
   *  namespace cli::traits{
   *    template<>
   *    struct is_sequence<abc::Vector> : std::true_type {};
   *  }
   * ```
   *
   * @tparam T the sequence
   * @tparam CharT the buffer's character type
   * @tparam ElementFormatter the formatter of the sequence's value_type
   * @tparam Delimiter the character used to seperate elements of the sequence
   */
  template<concepts::Sequence T,
           typename CharT,
           FormatterOf<typename T::value_type, CharT> ElementFormatter,
           CharT Delimiter = ','>
  struct Sequence {
    constexpr FormatResult operator()(View<CharT> buf,
                                      const T &seq) const noexcept {
      if (buf.size() < 2)
        return Error::buffer_overflow;
      bool first = true;
      std::size_t size = 0;
      buf[0] = '[';
      ++size;
      buf = buf.substr(1);
      for (const auto &v : seq) {
        if (first)
          first = false;
        else {
          if (buf.size() < 2)
            return Error::buffer_overflow;
          buf[0] = Delimiter;
          buf[1] = ' ';
          buf = buf.substr(2);
          size += 2;
        }

        auto res = ElementFormatter{}(buf, v);
        if (not res) {
          return res.error;
        }
        buf = buf.substr(res.size_written);
        size += res.size_written;
      }

      if (buf.size() == 0)
        return Error::buffer_overflow;

      buf[0] = ']';
      ++size;

      return size;
    }
  };

  /**
   * @brief This is a formatter for sequences. To use this with your custom
   * type, cli::traits::is_fixed_size_sequence must be overridden.
   *
   * Example:
   * ```
   *  // my_vec.hpp
   *  #include "cli/traits.hpp"
   *  namespace abc{
   *    class Array{
   *      public:
   *      using value_type = T;
   *      using iterator = T*;
   *
   *      Array();
   *      iterator begin();
   *      iterator end();
   *      T& operator[](std::size_t i);
   *      std::size_t size()const;
   *    };
   *  }
   *
   *  namespace cli::traits{
   *    template<>
   *    struct is_fixed_size_sequence<abc::Array> : std::true_type {};
   *  }
   * ```
   *
   * @tparam T the fixed size sequence
   * @tparam CharT the buffer's character type
   * @tparam ElementFormatter the formatter of the sequence's value_type
   * @tparam Delimiter the character used to seperate elements of the sequence
   */
  template<concepts::FixedSizeSequence T,
           typename CharT,
           FormatterOf<typename T::value_type, CharT> ElementFormatter,
           CharT Delimiter = ','>
  struct FixedSizeSequence {
    constexpr FormatResult operator()(View<CharT> buf,
                                      const T &seq) const noexcept {
      if (buf.size() < 2)
        return Error::buffer_overflow;
      bool first = true;
      std::size_t size = 0;
      buf[0] = '[';
      ++size;
      buf = buf.substr(1);
      for (const auto &v : seq) {
        if (first)
          first = false;
        else {
          if (buf.size() < 2)
            return Error::buffer_overflow;
          buf[0] = Delimiter;
          buf[1] = ' ';
          buf = buf.substr(2);
          size += 2;
        }

        auto res = ElementFormatter{}(buf, v);
        if (not res) {
          return res.error;
        }
        buf = buf.substr(res.size_written);
        size += res.size_written;
      }

      if (buf.size() == 0)
        return Error::buffer_overflow;

      buf[0] = ']';
      ++size;

      return size;
    }
  };

  /**
   * @brief The default formatter for sequences. This uses Format for the
   * sequence's value_type.
   *
   * @tparam T the sequence
   * @tparam CharT the buffer's character type
   */
  template<concepts::Sequence T, typename CharT>
  struct Format<T, CharT>
    : Sequence<T, CharT, Format<typename T::value_type, CharT>> {};

  template<concepts::FixedSizeSequence T, typename CharT>
  struct Format<T, CharT>
    : FixedSizeSequence<T, CharT, Format<typename T::value_type, CharT>> {};

  /**
   * @brief This is a formatter for stringviews.
   *
   * '' -> '""'
   * '"' ->'\"'
   * '""' -> '""'
   * ' ' -> '" "'
   * 'hello' -> 'hello'
   * 'hello world' -> '"hello world"'
   * 'hello"world' -> 'hello"world'
   *
   * @tparam CharT
   * @param buf
   * @param str
   * @return
   */
  template<concepts::StringView T, typename CharT>
  struct StringView {
    constexpr FormatResult operator()(View<CharT> buf,
                                      const T &str) const noexcept {
      static_assert(
        std::is_same_v<CharT, typename T::value_type>,
        "The value_type of the stringview T must be the same as CharT");

      if (str.size() == 0) {
        if (buf.size() < 2)
          return Error::buffer_overflow;
        buf[0] = '"';
        buf[1] = '"';
        return 2;
      }

      if (str.size() == 1) {
        if (str[0] == '"') {
          if (buf.size() < 2)
            return Error::buffer_overflow;
          buf[0] = '\\';
          buf[1] = '"';
          return 2;
        } else if (str[0] == ' ') {
          if (buf.size() < 3)
            return Error::buffer_overflow;
          buf[0] = '"';
          buf[1] = ' ';
          buf[2] = '"';
          return 3;
        } else {
          if (buf.size() < 1)
            return Error::buffer_overflow;
          buf[0] = str[0];
          return 1;
        }
      }

      std::size_t pos = 0;
      std::size_t quote_count = 0;
      bool has_space = false;
      CharT last_char = 0;

      for (const auto &v : str) {
        if (v == '"' and last_char != '\\') {
          ++quote_count;
        } else if (v == ' ') {
          has_space = true;
        }
        last_char = v;
      }

      const std::size_t format_size =
        str.size() + (has_space ? quote_count + 2 : 0);

      if (buf.size() < format_size)
        return Error::buffer_overflow;

      const bool needs_quote = has_space;

      if (needs_quote)
        buf[pos++] = '"';

      last_char = 0;
      for (const auto &ch : str) {
        if (ch == '"' and last_char != '\\' and needs_quote) {
          buf[pos++] = '\\';
          buf[pos++] = '"';
        } else {
          buf[pos++] = ch;
        }
        last_char = ch;
      }

      if (needs_quote)
        buf[pos++] = '"';

      return format_size;
    }
  };

  /**
   * @brief The default formatter for stringviews.
   *
   * @tparam T the string type
   * @tparam CharT
   */
  template<concepts::StringView T, typename CharT>
  struct Format<T, CharT> : public StringView<T, CharT> {};

  /**
   * @brief Formatter for strings.
   *
   * @tparam CharT
   */
  template<concepts::String T, typename CharT>
  struct String {
    constexpr FormatResult operator()(View<CharT> buf,
                                      const T &str) const noexcept {
      static_assert(
        std::is_same_v<CharT, typename T::value_type>,
        "The value_type of the stringview T must be the same as CharT");
      if (str.size() == 0) {
        if (buf.size() < 2)
          return Error::buffer_overflow;
        buf[0] = '"';
        buf[1] = '"';
        return 2;
      }

      if (str.size() == 1) {
        if (str[0] == '"') {
          if (buf.size() < 2)
            return Error::buffer_overflow;
          buf[0] = '\\';
          buf[1] = '"';
          return 2;
        } else if (str[0] == ' ') {
          if (buf.size() < 3)
            return Error::buffer_overflow;
          buf[0] = '"';
          buf[1] = ' ';
          buf[2] = '"';
          return 3;
        } else {
          if (buf.size() < 1)
            return Error::buffer_overflow;
          buf[0] = str[0];
          return 1;
        }
      }

      std::size_t pos = 0;
      std::size_t quote_count = 0;
      bool has_space = false;
      CharT last_char = 0;

      for (const auto &v : str) {
        if (v == '"' and last_char != '\\') {
          ++quote_count;
        } else if (v == ' ') {
          has_space = true;
        }
        last_char = v;
      }

      const std::size_t format_size =
        str.size() + (has_space ? quote_count + 2 : 0);

      if (buf.size() < format_size)
        return Error::buffer_overflow;

      const bool needs_quote = has_space;

      if (needs_quote)
        buf[pos++] = '"';

      last_char = 0;
      for (const auto &ch : str) {
        if (ch == '"' and last_char != '\\' and needs_quote) {
          buf[pos++] = '\\';
          buf[pos++] = '"';
        } else {
          buf[pos++] = ch;
        }
        last_char = ch;
      }

      if (needs_quote)
        buf[pos++] = '"';

      return format_size;
    }
  };

  template<concepts::String T, typename CharT>
  struct Format<T, CharT> : public String<T, CharT> {};
} // namespace cli::format

#endif
