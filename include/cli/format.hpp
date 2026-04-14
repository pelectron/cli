#ifndef CLI_FORMAT_HPP
#define CLI_FORMAT_HPP

#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/string.hpp"
#include "cli/traits.hpp"
#include "cli/u64.hpp"
#include "cli/util.hpp"

#include <cassert>
#include <cstdint>
#include <gcem.hpp>
#include <type_traits>

namespace cli::format {

struct FormatResult {

  constexpr FormatResult(Error error) : error(error), size_written(0) {}

  constexpr FormatResult(std::size_t size_written)
      : error(Error::none), size_written(size_written) {}

  constexpr operator bool() const noexcept { return error == Error::none; }

  Error error;
  std::size_t size_written;
};

template <class F> struct formatter_value_type {
  using type = std::decay_t<type_list::type_at_t<
      1, typename function_traits<std::decay_t<F>>::arguments>>;
};

template <typename F, typename T, typename CharT>
concept FormatterOf = requires(F &&f, View<CharT> buf, const T &t) {
  { f(buf, t) } -> std::same_as<FormatResult>;
};

template <class F>
concept Formatter =
    not std::same_as<void, typename formatter_value_type<F>::type>;

template <class T, typename CharT> struct DefaultFormat {
  constexpr FormatResult operator()(View<CharT> buf, const T &t) const {
    return Error::unimplemented;
  }
};

template <typename CharT> struct NullFormat {
  constexpr FormatResult operator()(View<CharT> buf, const dummy &) const {
    return 0;
  }
};

template <typename CharT> struct DefaultFormat<void, CharT> {
  constexpr FormatResult operator()(View<CharT> buf) const { return 0; }
};

template <typename CharT> struct DefaultFormat<bool, CharT> {
  constexpr FormatResult operator()(View<CharT> buf, bool b) const {
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

template <typename T, typename CharT, Fmt Format = Fmt::normal,
          bool UseSignForPositive = false>
struct Int {
  static constexpr std::size_t num_dec_digits() {
    using traits = traits::integer_traits<T>;
    if constexpr (traits::is_signed) {
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
      else if constexpr (traits::max >= uint64_t{1'000'000'000u})
        return 11;
      else if constexpr (traits::max >= uint64_t{100'000'000u})
        return 10;
      else if constexpr (traits::max >= uint64_t{10'000'000u})
        return 9;
      else if constexpr (traits::max >= uint64_t{1'000'000u})
        return 8;
      else if constexpr (traits::max >= uint64_t{100'000u})
        return 7;
      else if constexpr (traits::max >= uint64_t{10'000u})
        return 6;
      else if constexpr (traits::max >= uint64_t{1'000u})
        return 5;
      else if constexpr (traits::max >= uint64_t{100u})
        return 4;
      else if constexpr (traits::max >= uint64_t{10u})
        return 3;
      else if constexpr (traits::max >= uint64_t{1u})
        return 2;
      else
        static_assert(always_false<T>, "Cannot use DefaultFormat for T");
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
        static_assert(always_false<T>, "Cannot use DefaultFormat for T");
    }
  }

  constexpr FormatResult operator()(View<CharT> buf, T value) const {
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
            t = 10 * t;
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
          u_value = (~u_value) + 1u;
        }

        T pow10 = max_pow_10;
        for (; pow10 > 0; pow10 /= 10)
          if (u_value >= pow10)
            break;
        for (; pow10 > 0; pow10 /= 10) {
          const auto digit = u_value / pow10;
          buffer[size++] = static_cast<char>(digit + '0');
          u_value = u_value - digit * pow10;
        }

        if (buf.size() < size)
          return Error::buffer_overflow;

        for (std::size_t i = 0; i < size; ++i)
          buf[i] = buffer[i];
        return size;
      } else {
        constexpr T max_pow_10 = []() {
          T t{1};
          for (std::size_t i = 0; i < num_dec_digits() - 2 and t < traits::max;
               ++i)
            if (t > (10u * t))
              return t;
            else
              t = 10u * t;
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
          buffer[size++] = static_cast<char>(digit + u'0');
          value = value - digit * pow10;
        }

        if (buf.size() < size)
          return Error::buffer_overflow;

        for (std::size_t i = 0; i < size; ++i)
          buf[i] = buffer[i];

        return size;
      }
    } else if constexpr (Format == Fmt::hex) {
      UnsignedT u_value = static_cast<UnsignedT>(value);
      constexpr std::size_t max_size = 2 + sizeof(UnsignedT) * 2;
      char buffer[max_size]{'0', 'x', 0};
      std::size_t size = 2;
      auto nibble = (sizeof(UnsignedT) * 2) - 1u;
      for (; nibble != 0; --nibble) {
        if (u_value < (1u << (nibble * 4u)))
          continue;
        else
          break;
      }
      for (; nibble != 0; --nibble) {
        const auto digit =
            static_cast<char>(0x0Fu & (u_value >> (nibble * 4u)));
        if (digit >= 0 and digit <= 9)
          buffer[size++] = static_cast<char>(digit + '0');
        else if (digit >= 10 and digit <= 15)
          buffer[size++] = static_cast<char>(digit + 'A');
        else
          assert(false);
        u_value &= (1u << (nibble * 4u)) - 1u;
      }

      if (buf.size() < size)
        return Error::buffer_overflow;

      for (std::size_t i = 0; i < size; ++i)
        buf[i] = buffer[i];

      return size;
    } else if constexpr (Format == Fmt::binary) {
      const UnsignedT u_value = static_cast<UnsignedT>(value);
      constexpr std::size_t max_size = 2 + sizeof(UnsignedT) * 8;
      char buffer[max_size]{'0', 'b', 0};
      std::size_t size = 2;
      auto bit = (sizeof(UnsignedT) * 2) - 1u;
      for (; bit != 0; --bit) {
        if (u_value < (1u << (bit * 8u)))
          continue;
        else
          break;
      }
      for (; bit != 0; --bit) {
        const auto digit = u_value >> (bit * 8u);
        buffer[size++] = static_cast<char>(digit + '0');
        u_value >>= 1u;
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

template <traits::Integer T, typename CharT>
struct DefaultFormat<T, CharT> : public Int<T, CharT> {};

template <traits::Character T, typename CharT, Fmt Format = Fmt::normal>
struct Char {
  constexpr FormatResult operator()(View<CharT> buf, T value) const {
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

template <traits::Character T, typename CharT>
struct DefaultFormat<T, CharT> : public Char<T, CharT> {};

template <traits::FixPoint T, typename CharT, std::size_t Precision = 11,
          bool PrintTrailingZeros = false, Fmt Format = Fmt::normal,
          bool UseSignForPositive = false, char FixPointSeparator = '.'>
class FixPoint {
  using traits = traits::fixpoint_traits<T>;
  using raw_value = typename traits::raw_value_type;
  using int_type =
      std::conditional_t<traits::is_signed, long long, unsigned long long>;

  static_assert(
      traits::num_frac_digits >= 1 and traits::num_frac_digits <= 27,
      "fix point number with more than 27 fractional digits are not supported");
  // frac is the raw integer value of the fractional part of value with F bits.
  // This needs to be converted to a decimal integer value for formatting.
  // The integer value needed can be calculated as follows:
  // value = round(frac / max_frac * 10^precision) = round(frac / 2^F *
  // 10^precision) = round(frac
  // * 2^-F * 10^precision). factor = 2^-F * 10^precision can be calculated at
  // compile time, with only integer divisions at run time. This will
  // unfortunately produce some rounding errors.
  static constexpr auto factor = []() {
    u64 i{10};
    for (std::size_t i = 0; i < Precision; ++i) {
      i = i * 10;
    }
    return i >> traits::num_frac_digits;
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
  static constexpr auto billion = gcem::pow(10u, 9u);

public:
  constexpr FormatResult operator()(View<CharT> buf, T fp) const {
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

      // v is the decimal representation of the fractional part, i.e. the actual
      // number to print in terms of max_pow10/10. Dividing v by
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
        // fractional part is zero-> print single 0 and print trailing zeros in
        // case we should
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
        u64 rest = u64::div64_with_mod(next, pow10);

        if (next >= 10)
          return Error::implementation_error;

        buf[buf_idx++] = next.low() + '0';
        if (rest == 0)
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
      static_assert(always_false<decltype(Format)>,
                    "Supplied invalid Fmt to Format::FixPoint. Must be one of "
                    "Fmt::normal, Fmt::hex and Fmt::binary.");
    }
  }
};

template <traits::FixPoint T, typename CharT>
struct DefaultFormat<T, CharT> : public FixPoint<T, CharT> {};

template <traits::Float T, typename CharT, Fmt Format = Fmt::normal,
          char FixPointSeparator = '.'>
class Float {
  static_assert(always_false<T>, "Not implemented yet");
};

template <traits::Sequence T, typename CharT,
          FormatterOf<typename T::value_type, CharT> ElementFormatter,
          CharT Delimiter = ','>
struct Sequence {
  constexpr FormatResult operator()(View<CharT> buf, const T &seq) const {
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

template <traits::Sequence T, typename CharT>
struct DefaultFormat<T, CharT>
    : Sequence<T, CharT, DefaultFormat<typename T::value_type, CharT>> {};

template <traits::Enum Enum, typename CharT> struct DefaultFormat<Enum, CharT> {
  constexpr FormatResult operator()(View<CharT> buf, Enum value) const {
    if constexpr (traits::FlagEnum<Enum>) {
      View<const CharT> name{};
      std::size_t written = 0;
      bool first = true;
      for (std::size_t i = 0; i < sizeof(Enum) * 8; ++i) {
        if ((static_cast<std::underlying_type_t<Enum>>(1u << i) &
             static_cast<std::underlying_type_t<Enum>>(value)) == 0)
          continue;

        name = cli::ctti::enum_name<Enum, CharT>(value);
        if ((first and buf.size() < name.size()) or
            (buf.size() < (name.size() + 1))) {
          return Error::buffer_overflow;
        }
        std::size_t offset = 0;
        if (not first) {
          buf[0] = '|';
          offset = 1;
          ++written;
        } else {
          first = true;
        }
        for (std::size_t i = 0; i < name.size(); ++i, ++written) {
          buf[i + offset] = name[i];
        }
        buf = buf.substr(name.size() + offset);
      }

      if (written == 0) {
        for (const auto &ch : "none")
          buf[written++] = ch;
      }

      return written;
    } else {
      const View<const CharT> name = cli::ctti::enum_name<Enum, CharT>(value);
      if (buf.size() < name.size()) {
        return Error::buffer_overflow;
      }
      for (std::size_t i = 0; i < name.size(); ++i) {
        buf[i] = name[i];
      }
      return name.size();
    }
  }
};

template <traits::String T, typename CharT, bool UseQuotes = false>
struct String {
  constexpr FormatResult operator()(View<CharT> buf, const T &str) const {
    using elem = typename T::value_type;
    constexpr std::size_t size = sizeof(elem);
    static_assert(size == 1,
                  "character types with sizeof > 1 are not supported for now");
    std::size_t pos = 0;

    if constexpr (UseQuotes) {
      if (buf.size() < (str.size() * size + 2))
        return Error::buffer_overflow;

      buf[pos++] = '"';
    } else {
      if (buf.size() < (str.size() * size))
        return Error::buffer_overflow;
    }

    for (const auto &ch : str) {
      buf[pos++] = ch;
    }

    if constexpr (UseQuotes) {
      buf[pos++] = '"';
    }

    return pos;
  }
};

template <traits::String T, typename CharT>
struct DefaultFormat<T, CharT> : public String<T, CharT> {};

template <typename CharT, CharT Assignment = '=', CharT MemberSeparator = ',',
          CharT Prefix = '{', CharT Postfix = '}', bool UseNames = true,
          class... Fields>
class FieldGroup {
public:
  constexpr FormatResult operator()(View<CharT> buf,
                                    const std::tuple<Fields...> &fields) const {
    if (buf.size() == 0)
      return Error::buffer_overflow;

    std::size_t written = 0;
    if constexpr (Prefix != ' ') {
      if (buf.size() < 2)
        return Error::buffer_overflow;
      buf[written++] = Prefix;
      buf[written++] = ' ';
      buf = buf.substr(2);
    }
    Error error = Error::none;
    bool first = true;
    for_each(
        [&error, &written, &first, &buf](const auto &field) {
          if (error != Error::none)
            return;

          if (first) {
            first = false;
          } else {
            if (buf.size() <= 2) {
              error = Error::buffer_overflow;
              return;
            }
            buf[0] = MemberSeparator;
            buf[1] = ' ';
            buf = buf.substr(2);
            written += 2;
          }

          using Field = std::remove_cvref_t<decltype(field)>;
          using Formatter =
              DefaultFormat<std::remove_cvref_t<typename Field::type>, CharT>;

          if constexpr (UseNames) {
            constexpr StringLiteral name{typename Field::name{}};
            if (buf.size() <= (name.size() + 3))
              error = Error::buffer_overflow;
            std::size_t i = 0;
            for (; i < name.size(); ++i) {
              buf[i] = name[i];
            }
            buf[i++] = ' ';
            buf[i++] = Assignment;
            buf[i++] = ' ';
            buf = buf.substr(i);
            written += i;
          }

          auto res = Formatter{}(buf, field.value);
          if (not res) {
            error = res.error;
            return;
          }
          buf = buf.substr(res.size_written);
          written += res.size_written;
        },
        fields);

    if (error != Error::none)
      return error;

    if constexpr (Postfix != ' ') {
      if (buf.size() == 0)
        return Error::buffer_overflow;

      buf[0] = Postfix;
      ++written;
    }

    return written;
  }
};

template <traits::Struct T, typename CharT, CharT Assignment = '=',
          CharT MemberSeparator = ',', CharT Prefix = '{', CharT Postfix = '}',
          bool UseNames = true>
struct field_formatter_for {
  template <class... Fields>
  using type_ = FieldGroup<CharT, Assignment, MemberSeparator, Prefix, Postfix,
                           UseNames, Fields...>;
  using fields = decltype(ctti::to_tuple(std::declval<const T>()));
  using type = type_list::apply_t<type_, fields>;
};

template <traits::Struct T, typename CharT, class Name = string_constant<CharT>,
          CharT Assignment = '=', CharT MemberSeparator = ',',
          CharT Prefix = '{', CharT Postfix = '}', bool UseNames = true>
class Struct : field_formatter_for<T, CharT, Assignment, MemberSeparator,
                                   Prefix, Postfix, UseNames>::type {
  using Base =
      typename field_formatter_for<T, CharT, Assignment, MemberSeparator,
                                   Prefix, Postfix, UseNames>::type;

public:
  constexpr FormatResult operator()(View<CharT> buf, const T &t) const {
    if (buf.size() == 0)
      return Error::too_few_characters;

    std::size_t written = 0;
    if constexpr (Name::string_size > 0) {
      constexpr StringLiteral name{Name{}};
      if (buf.size() <= (name.size() + 3))
        return Error::buffer_overflow;
      std::size_t i = 0;
      for (; i < name.size(); ++i) {
        buf[i] = name[i];
      }
      buf[i++] = ' ';
      buf[i++] = Assignment;
      buf[i++] = ' ';
      buf = buf.substr(i);
      written += i;
    }

    if (auto res = static_cast<const Base *>(this)->operator()(
            buf, ctti::to_tuple(t))) {
      return res.size_written + written;
    } else
      return res.error;
  }
};

template <traits::Struct T, typename CharT>
struct DefaultFormat<T, CharT> : public Struct<T, CharT> {};

template <typename T, typename CharT>
FormatResult format(View<CharT> buf, const T &t) {
  return DefaultFormat<T, CharT>{}(buf, t);
}
} // namespace cli::format

#endif
