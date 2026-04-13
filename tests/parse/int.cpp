#include "catch2/catch_test_macros.hpp"
#include "cli/parse.hpp"

#include <limits>

using namespace cli;
using namespace cli::parse;

template <class Int> struct int_str {
  Int i;
  cli::View<const char> str;
  constexpr int_str(int_str &&) = default;
  constexpr int_str(const int_str &) = default;
  template <class I>
  constexpr int_str(I i, cli::View<const char> str) : i(i), str(str) {}
  template <class I>
  constexpr int_str(int_str<I> other) : i(other.i), str(other.str) {}
};

template <class I> int_str(I i, cli::View<const char> str) -> int_str<I>;

#define TV(number)                                                             \
  int_str { number, #number }
#define HEX_TV(number)                                                         \
  int_str{0x##number, "0x" #number}, int_str{0x##number, "0X" #number},        \
      int_str{0x##number, "x" #number}, int_str{0x##number, "X" #number},      \
      int_str {                                                                \
    0x##number, "#" #number                                                    \
  }
#define REST_TV(number, rest)                                                  \
  int_str { number, #number rest }

#define FAIL_TV(number)                                                        \
  int_str { number, "a" #number }

#define FAIL_HEX_TV(number)                                                    \
  int_str{0x##number, "0xh" #number}, int_str{0x##number, "0Xi" #number},      \
      int_str{0x##number, "xj" #number}, int_str{0x##number, "Xk" #number},    \
      int_str {                                                                \
    0x##number, "#l" #number                                                   \
  }

TEST_CASE("parse::Int<int>") {
  Int<int, char> parser;
  SECTION("well formed strings") {
    static_assert(std::numeric_limits<int>::min() == -2147483648);
    int_str<int> passing_strings[]{
        TV(0),
        TV(+0),
        TV(-0),
        TV(1),
        TV(+1),
        TV(-1),
        TV(42),
        TV(-42),
        TV(2147483647),
        TV(-2147483647),
        int_str<int>{std::numeric_limits<int>::min(), "-2147483648"}};

    for (const auto &istr : passing_strings) {
      auto res = parser(istr.str);
      REQUIRE(res);
      REQUIRE(res.value == istr.i);
      REQUIRE(res.rest.size() == 0);
    }

    int_str<int> passing_hex_strings[]{
        HEX_TV(0),
        HEX_TV(1),
        HEX_TV(42),
        HEX_TV(deadbee),
        HEX_TV(042),
        HEX_TV(0deadbee),
        int_str<int>{-1, "0xFFFFFFFF"},
        int_str<int>{-1, "0XFFFFFFFF"},
        int_str<int>{-1, "#FFFFFFFF"},
        int_str<int>{std::numeric_limits<int>::min(), "0x80000000"},
        int_str<int>{std::numeric_limits<int>::min(), "0X80000000"},
        int_str<int>{std::numeric_limits<int>::min(), "#80000000"},
        int_str<int>{std::numeric_limits<int>::max(), "0x7FFFFFFF"},
        int_str<int>{std::numeric_limits<int>::max(), "0X7FFFFFFF"},
        int_str<int>{std::numeric_limits<int>::max(), "#7FFFFFFF"}};
    for (const auto &istr : passing_hex_strings) {
      auto res = parser(istr.str);
      REQUIRE(res);
      REQUIRE(res.value == istr.i);
      REQUIRE(res.rest.size() == 0);
    }

    int_str<int> passing_with_rest_strings[]{
        REST_TV(10, "x"), REST_TV(11, ","), REST_TV(14, " "), REST_TV(15, " ")};
    for (const auto &istr : passing_with_rest_strings) {
      auto res = parser(istr.str);
      REQUIRE(res);
      REQUIRE(res.value == istr.i);
      REQUIRE(res.rest.size() == 1);
    }
  }
  SECTION("empty string") { REQUIRE_FALSE(parser(cli::CharView{})); }

  SECTION("malformed strings") {
    static_assert(std::numeric_limits<int>::min() == -2147483648);
    int_str<int> fail_strings[]{
        FAIL_TV(0),
        FAIL_TV(+0),
        FAIL_TV(-0),
        FAIL_TV(1),
        FAIL_TV(+1),
        FAIL_TV(-1),
        FAIL_TV(42),
        FAIL_TV(-42),
        FAIL_TV(2147483647),
        FAIL_TV(-2147483647),
        int_str<int>{std::numeric_limits<int>::min(), "-a2147483648"}};

    for (const auto &istr : fail_strings) {
      auto res = parser(istr.str);
      REQUIRE_FALSE(res);
    }

    int_str<int> fail_hex_strings[]{
        FAIL_HEX_TV(0),
        FAIL_HEX_TV(1),
        FAIL_HEX_TV(42),
        FAIL_HEX_TV(deadbee),
        FAIL_HEX_TV(042),
        FAIL_HEX_TV(0deadbee),
        int_str<int>{-1, "0xhFFFFFFFF"},
        int_str<int>{-1, "0XiFFFFFFFF"},
        int_str<int>{-1, "#jFFFFFFFF"},
        int_str<int>{std::numeric_limits<int>::min(), "0xk80000000"},
        int_str<int>{std::numeric_limits<int>::min(), "0Xl80000000"},
        int_str<int>{std::numeric_limits<int>::min(), "#m80000000"},
        int_str<int>{std::numeric_limits<int>::max(), "0xn7FFFFFFF"},
        int_str<int>{std::numeric_limits<int>::max(), "0Xo7FFFFFFF"},
        int_str<int>{std::numeric_limits<int>::max(), "#p7FFFFFFF"}};
    for (const auto &istr : fail_hex_strings) {
      auto res = parser(istr.str);
      REQUIRE_FALSE(res);
    }
  }
}

TEST_CASE("parse::Int<unsigned>") {
  Int<unsigned, const char> parser;
  SECTION("well formed strings") {
    static_assert(std::numeric_limits<int>::min() == -2147483648);
    int_str<unsigned> passing_strings[]{
        TV(0),
        TV(+0),
        TV(1),
        TV(+1),
        TV(42),
        TV(2147483647),
        int_str<unsigned>{std::numeric_limits<unsigned>::min(), "0"},
        int_str<unsigned>{std::numeric_limits<unsigned>::max(), "4294967295"}};

    for (const auto &istr : passing_strings) {
      auto res = parser(istr.str);
      REQUIRE(res);
      REQUIRE(res.value == istr.i);
      REQUIRE((res.rest.size() == 0));
    }

    int_str<unsigned> passing_hex_strings[]{
        HEX_TV(0),
        HEX_TV(1),
        HEX_TV(42),
        HEX_TV(deadbee),
        HEX_TV(042),
        HEX_TV(0deadbee),
        int_str<unsigned>{std::numeric_limits<unsigned>::max(), "0xFFFFFFFF"},
        int_str<unsigned>{std::numeric_limits<unsigned>::max(), "0XFFFFFFFF"},
        int_str<unsigned>{std::numeric_limits<unsigned>::max(), "#FFFFFFFF"},
        int_str<unsigned>{0x80000000, "0x80000000"},
        int_str<unsigned>{0x80000000, "0X80000000"},
        int_str<unsigned>{0x80000000, "#80000000"},
        int_str<unsigned>{0x7FFFFFFF, "0x7FFFFFFF"},
        int_str<unsigned>{0x7FFFFFFF, "0X7FFFFFFF"},
        int_str<unsigned>{0x7FFFFFFF, "#7FFFFFFF"}};
    for (const auto &istr : passing_hex_strings) {
      auto res = parser(istr.str);
      REQUIRE(res);
      REQUIRE(res.value == istr.i);
      REQUIRE(res.rest.size() == 0);
    }

    int_str<unsigned> passing_with_rest_strings[]{
        REST_TV(10, "x"), REST_TV(11, ","), REST_TV(14, " "), REST_TV(15, " ")};
    for (const auto &istr : passing_with_rest_strings) {
      auto res = parser(istr.str);
      REQUIRE(res);
      REQUIRE(res.value == istr.i);
      REQUIRE(res.rest.size() == 1);
    }
  }
  SECTION("empty string") { REQUIRE_FALSE(parser(cli::CharView{})); }

  SECTION("malformed strings") {
    int_str<unsigned> fail_strings[]{
        FAIL_TV(0),
        FAIL_TV(+0),
        TV(-0),
        FAIL_TV(1),
        FAIL_TV(+1),
        TV(-1),
        FAIL_TV(42),
        TV(-42),
        FAIL_TV(2147483647),
        TV(-2147483647),
        FAIL_TV(4294967295),
        int_str<unsigned>{std::numeric_limits<unsigned>::min(), "a2147483648"}};

    for (const auto &istr : fail_strings) {
      auto res = parser(istr.str);
      REQUIRE_FALSE(res);
    }

    int_str<int> fail_hex_strings[]{
        FAIL_HEX_TV(0),
        FAIL_HEX_TV(1),
        FAIL_HEX_TV(42),
        FAIL_HEX_TV(deadbee),
        FAIL_HEX_TV(042),
        FAIL_HEX_TV(0deadbee),
        int_str<unsigned>{-1, "0xhFFFFFFFF"},
        int_str<unsigned>{-1, "0XiFFFFFFFF"},
        int_str<unsigned>{-1, "#jFFFFFFFF"},
        int_str<unsigned>{std::numeric_limits<unsigned>::min(), "0xk80000000"},
        int_str<unsigned>{std::numeric_limits<unsigned>::min(), "0Xl80000000"},
        int_str<unsigned>{std::numeric_limits<unsigned>::min(), "#m80000000"},
        int_str<unsigned>{std::numeric_limits<unsigned>::max(), "0xn7FFFFFFF"},
        int_str<unsigned>{std::numeric_limits<unsigned>::max(), "0Xo7FFFFFFF"},
        int_str<unsigned>{std::numeric_limits<unsigned>::max(), "#p7FFFFFFF"}};
    for (const auto &istr : fail_hex_strings) {
      auto res = parser(istr.str);
      REQUIRE_FALSE(res);
    }
  }
}
