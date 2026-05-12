#ifndef CLI_U64_HPP
#define CLI_U64_HPP
#include <cstddef>
#include <cstdint>

namespace cli {
#if !defined(UINT64_MAX)
  /**
   * @class u64
   *
   */
  struct u64 {
    std::uint32_t l;
    std::uint32_t h = 0;

    constexpr std::uint32_t low() const noexcept { return l; }

    constexpr friend u64 operator~(u64 a) { return {~a.l, ~a.h}; }

    constexpr friend u64 operator*(u64 a, u64 b) { return mul_alg(a, b); }

    constexpr friend u64 operator+(u64 a, u64 b) {
      u64 ret{};
      if (add_overflows(a.l, b.l, ret.l))
        ret.h = 1;
      ret.h += a.h + b.h;
      return ret;
    }

    constexpr friend u64 operator/(u64 a, u64 b) {
      div64_with_mod(a, b);
      return a;
    }
    /**
     * a = a/b and returns a % b
     *
     * @param a
     * @param b
     * @return
     */
    static constexpr u64 div64_with_mod(u64 &a, u64 b) {
      /*
      Q := 0                  -- Initialize quotient and remainder to zero
      R := 0
      for i := n − 1 .. 0 do  -- Where n is number of bits in N
        R := R << 1           -- Left-shift R by 1 bit
        R(0) := N(i)          -- Set the least-significant bit of R equal to bit
      -- i of the numerator

        if R ≥ D then
          R := R − D
          Q(i) := 1
        end
    */
      CLI_ASSERT(b.h or b.l);
      u64 Q{};
      u64 R{};
      for (int i = 63; i >= 0; --i) {
        R = R << 1;
        R.set_bit(0, a.get_bit(i));
        if (R >= b) {
          R = R - b;
          Q.set_bit(i, true);
        }
      }
      a = Q;
      return R;
    }
    constexpr void set_bit(std::size_t i, bool value) {
      if (value) {
        if (i > 31)
          h |= 1u << (i - 32);
        else
          l |= 1u << i;
      } else {
        if (i > 31)
          h &= ~(1u << (i - 32));
        else
          l &= ~(1u << i);
      }
    }
    constexpr uint32_t get_bit(std::size_t i) {
      if (i > 31)
        return h & (1u << (i - 32));
      else
        return l & (1u << i);
    }
    constexpr friend u64 operator/(u64 a, uint32_t b) { return a / u64{b}; }

    constexpr friend u64 operator-(u64 a, u64 b) { return a + ((~b) + 1); }

    constexpr friend bool operator>(u64 a, u64 b) {
      if (a.h == b.h)
        return a.l > b.l;
      return a.h > b.h;
    }
    constexpr friend bool operator>=(u64 a, u64 b) {
      if (a.h == b.h)
        return a.l >= b.l;
      return a.h >= b.h;
    }
    constexpr friend bool operator<(u64 a, u64 b) {
      if (a.h == b.h)
        return a.l < b.l;
      return a.h < b.h;
    }
    constexpr friend bool operator<=(u64 a, u64 b) {
      if (a.h == b.h)
        return a.l <= b.l;
      return a.h <= b.h;
    }
    constexpr friend bool operator==(u64 a, u64 b) {
      return a.h == b.h and a.l == b.l;
    }
    constexpr friend bool operator!=(u64 a, u64 b) {
      return a.h != b.h or a.l != b.l;
    }
    constexpr friend u64 operator*(u64 a, uint32_t b) {
      return mul_alg(a, u64{b});
    }

    constexpr friend u64 operator-(u64 a, uint32_t b) { return a - u64{b}; }

    constexpr friend u64 operator+(u64 a, uint32_t b) {
      u64 ret{};
      if (add_overflows(a.l, b, ret.l))
        ret.h = 1;
      ret.h += a.h;
      return ret;
    }
    constexpr friend bool operator>(u64 a, uint32_t b) {
      if (a.h == 0)
        return a.l > b;
      return true;
    }
    constexpr friend bool operator>=(u64 a, uint32_t b) {
      if (a.h == 0)
        return a.l >= b;
      return true;
    }
    constexpr friend bool operator<(u64 a, uint32_t b) {
      if (a.h == 0)
        return a.l < b;
      return false;
    }
    constexpr friend bool operator<=(u64 a, uint32_t b) {
      if (a.h == 0)
        return a.l <= b;
      return false;
    }
    constexpr friend bool operator==(u64 a, uint32_t b) {
      return a.h == 0 and a.l == b;
    }
    constexpr friend bool operator!=(u64 a, uint32_t b) { return not(a == b); }
    constexpr friend u64 operator*(uint32_t a, u64 b) {
      return mul_alg(u64{a}, b);
    }

    constexpr friend u64 operator+(uint32_t a, u64 b) {
      u64 ret{};
      if (add_overflows(b.l, a, ret.l))
        ret.h = 1;
      ret.h += b.h;
      return ret;
    }

    constexpr friend u64 operator-(uint32_t a, u64 b) { return u64{a} - b; }

    constexpr friend bool operator>(uint32_t a, u64 b) {
      if (b.h == 0)
        return a > b.l;
      return false;
    }
    constexpr friend bool operator>=(uint32_t a, u64 b) {
      if (b.h == 0)
        return a >= b.l;
      return false;
    }
    constexpr friend bool operator<(uint32_t a, u64 b) {
      if (b.h == 0)
        return a < b.l;
      return true;
    }
    constexpr friend bool operator<=(uint32_t a, u64 b) {
      if (b.h == 0)
        return a <= b.l;
      return true;
    }
    constexpr friend bool operator==(uint32_t a, u64 b) {
      return b.h == 0 and b.l == a;
    }
    constexpr friend bool operator!=(uint32_t a, u64 b) { return not(a == b); }
    constexpr friend u64 operator>>(u64 a, size_t i) {
      if (i == 0)
        return a;

      if (i > 31)
        return {a.h >> (i - 32u)};

      return {(a.l >> i) | (a.h << (32 - i)), a.h >> i};
    }
    constexpr friend u64 operator<<(u64 a, size_t i) {
      if (i == 0)
        return a;

      if (i > 31)
        return {0, a.l << (i - 32)};

      return {(a.l << i), (a.h << i) | (a.l >> (32 - i))};
    }

    /// calculates nom = nom/denom and returns nom % base
    // static constexpr uint32_t div_with_rem(u64 &nom, uint32_t denom) {
    //   return div64_32(nom, u64{denom});
    // }
    //
  private:
    static constexpr bool add_overflows(uint32_t a, uint32_t b, uint32_t &res) {
      res = a + b;
      return res < a or res < b;
    }

    static constexpr uint32_t mul32_with_overflow(
      uint16_t al, uint16_t ah, uint16_t bl, uint16_t bh, uint32_t &overflow) {
      const uint32_t l = uint32_t{al} * uint32_t{bl};
      const uint32_t m0 = uint32_t{ah} * uint32_t{bl};
      const uint32_t m1 = uint32_t{al} * uint32_t{bh};

      uint32_t m = 0;
      if (add_overflows(m0, m1, m)) {
        overflow = 1u << 16u;
      }
      const uint32_t mh = m >> 16u;
      const uint32_t ml = uint32_t{m << 16u};
      overflow += uint32_t{ah} * uint32_t{bh} + mh;

      uint32_t res = 0;
      if (add_overflows(l, ml, res)) {
        overflow += 1;
      }

      return res;
    }

    static constexpr uint32_t
    mul32_with_overflow(uint32_t a, uint32_t b, uint32_t &overflow) {
      return mul32_with_overflow(a, a >> 16, b, b >> 16, overflow);
    }

    static constexpr u64 mul_alg(u64 a, u64 b) {
      std::uint32_t overflow = 0;
      const std::uint32_t l = mul32_with_overflow(a.l, b.l, overflow);
      const std::uint32_t h = b.l * a.h + a.l * b.h + overflow;
      return {l, h};
    }
  };

  static_assert((u64{1} << 1).l == 2);
  static_assert((u64{1} << 1).h == 0);
  static_assert((u64{1} >> 1).l == 0);
  static_assert((u64{1} >> 1).h == 0);
  static_assert((u64{0, 1} >> 1).l == 0x80000000u);
  static_assert((u64{0, 1} >> 1).h == 0);
  static_assert((u64{0x80000000u} << 1).h == 1);
  static_assert((u64{0x80000000u} << 1).l == 0);
  static_assert((u64{5} / u64{1}) == u64{5});
  static_assert((u64{10} / u64{2}) == u64{5});
  static_assert((u64{0, 1} / u64{2}).l == u64{0x80000000u});

#else
  struct u64 {
    std::uint64_t v{};
    constexpr u64(std::uint32_t l, std::uint32_t h)
      : v(l | (static_cast<uint64_t>(h) << 32)) {}
    constexpr u64(std::uint64_t val)
      : v(val) {}
    constexpr u64() = default;
    constexpr std::uint32_t low() const noexcept {
      return static_cast<std::uint32_t>(v);
    }
    constexpr friend u64 operator~(u64 a) { return {~a.v}; }

    constexpr friend u64 operator*(u64 a, u64 b) { return {a.v * b.v}; }

    constexpr friend u64 operator+(u64 a, u64 b) { return {a.v + b.v}; }

    constexpr friend u64 operator-(u64 a, u64 b) { return {a.v - b.v}; }

    constexpr friend u64 operator/(u64 a, u64 b) { return {a.v / b.v}; }

    /**
     * a = a/b and returns a % b
     *
     * @param a
     * @param b
     * @return
     */
    static constexpr u64 div64_with_mod(u64 &a, u64 b) {
      auto n = a;
      a.v = a.v / b.v;
      return {n.v % b.v};
    }
    constexpr void set_bit(std::size_t i, bool value) {
      if (value) {
        v |= 1ull << i;
      } else {
        v |= ~(1ull << i);
      }
    }
    constexpr uint32_t get_bit(std::size_t i) {
      return static_cast<uint32_t>(v & (1ull << i));
    }
    constexpr friend u64 operator/(u64 a, uint32_t b) { return a / u64{b}; }

    constexpr friend bool operator>(u64 a, u64 b) { return a.v > b.v; }
    constexpr friend bool operator>=(u64 a, u64 b) { return a.v >= b.v; }
    constexpr friend bool operator<(u64 a, u64 b) { return a.v < b.v; }
    constexpr friend bool operator<=(u64 a, u64 b) { return a.v <= b.v; }
    constexpr friend bool operator==(u64 a, u64 b) { return a.v == b.v; }
    constexpr friend bool operator!=(u64 a, u64 b) { return a.v != b.v; }
    constexpr friend u64 operator*(u64 a, uint32_t b) { return a * u64{b}; }

    constexpr friend u64 operator-(u64 a, uint32_t b) { return a - u64{b}; }

    constexpr friend u64 operator+(u64 a, uint32_t b) { return a + u64{b}; }
    constexpr friend bool operator>(u64 a, uint32_t b) { return a > u64{b}; }
    constexpr friend bool operator>=(u64 a, uint32_t b) { return a >= u64{b}; }
    constexpr friend bool operator<(u64 a, uint32_t b) { return a < u64{b}; }
    constexpr friend bool operator<=(u64 a, uint32_t b) { return a <= u64{b}; }
    constexpr friend bool operator==(u64 a, uint32_t b) { return a == u64{b}; }
    constexpr friend bool operator!=(u64 a, uint32_t b) { return a != u64{b}; }
    constexpr friend u64 operator*(uint32_t a, u64 b) { return u64{a} * b; }

    constexpr friend u64 operator+(uint32_t a, u64 b) { return u64{a} + b; }

    constexpr friend u64 operator-(uint32_t a, u64 b) { return u64{a} - b; }

    constexpr friend bool operator>(uint32_t a, u64 b) { return u64{a} > b; }
    constexpr friend bool operator>=(uint32_t a, u64 b) { return u64{a} >= b; }
    constexpr friend bool operator<(uint32_t a, u64 b) { return u64{a} < b; }
    constexpr friend bool operator<=(uint32_t a, u64 b) { return u64{a} <= b; }
    constexpr friend bool operator==(uint32_t a, u64 b) { return u64{a} == b; }
    constexpr friend bool operator!=(uint32_t a, u64 b) { return u64{a} != b; }
    constexpr friend u64 operator>>(u64 a, size_t i) { return {a.v >> i}; }
    constexpr friend u64 operator<<(u64 a, size_t i) { return {a.v << i}; }
  };
#endif
} // namespace cli
#endif
