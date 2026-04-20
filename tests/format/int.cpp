#include "cli/enums.hpp"
#include "cli/format.hpp"

#include <catch2/catch_all.hpp>
#include <format>
#include <limits>
#include <string>
#include <type_traits>

// TODO: test integer and hex formatting

template<class Int>
struct FmtIntTestVector {
  Int input;
  std::string expected_output;
  std::string buffer = std::string(255, 0);
};

template<class Int>
void test1(FmtIntTestVector<Int> &tv) {
  constexpr cli::format::Int<Int, char, cli::Fmt::normal, false> format;
  auto res = format({tv.buffer.data(), tv.buffer.size()}, tv.input);
  REQUIRE(res);
  CHECK(res.size_written == tv.expected_output.size());
  tv.buffer.resize(res.size_written);
  REQUIRE(tv.expected_output == tv.buffer);
}

template<class Int>
void test2(FmtIntTestVector<Int> &tv) {
  constexpr cli::format::Int<Int, char, cli::Fmt::normal, true> format;
  auto res = format({tv.buffer.data(), tv.buffer.size()}, tv.input);
  REQUIRE(res);
  CHECK(res.size_written == tv.expected_output.size());
  tv.buffer.resize(res.size_written);
  REQUIRE(tv.expected_output == tv.buffer);
}

template<class Int>
void test_hex(FmtIntTestVector<Int> &tv) {
  constexpr cli::format::Int<Int, char, cli::Fmt::hex, false> format;
  auto res = format({tv.buffer.data(), tv.buffer.size()}, tv.input);
  REQUIRE(res);
  CHECK(res.size_written == tv.expected_output.size());
  tv.buffer.resize(res.size_written);
  REQUIRE(tv.expected_output == tv.buffer);
}

template<class Int>
void test_bin(FmtIntTestVector<Int> &tv) {
  constexpr cli::format::Int<Int, char, cli::Fmt::binary, false> format;
  auto res = format({tv.buffer.data(), tv.buffer.size()}, tv.input);
  REQUIRE(res);
  CHECK(res.size_written == tv.expected_output.size());
  tv.buffer.resize(res.size_written);
  REQUIRE(tv.expected_output == tv.buffer);
}

template<class T>
std::string max_string(cli::Fmt fmt = cli::Fmt::normal) {
  switch (fmt) {
    case cli::Fmt::normal:
      return std::format("{}", std::numeric_limits<T>::max());
    case cli::Fmt::hex:
      return "0x" + std::format("{:X}", std::numeric_limits<T>::max());
    case cli::Fmt::binary:
      return std::format("{:#b}", std::numeric_limits<T>::max());
  }
}

template<class T>
std::string min_string(cli::Fmt fmt = cli::Fmt::normal) {
  switch (fmt) {
    case cli::Fmt::normal:
      return std::format("{}", std::numeric_limits<T>::min());
    case cli::Fmt::hex:
      return "0x" + std::format("{:X}",
                                static_cast<std::make_unsigned_t<T>>(
                                  std::numeric_limits<T>::min()));
    case cli::Fmt::binary:
      return std::format(
        "{:#b}",
        static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::min()));
  }
  return std::to_string(std::numeric_limits<T>::min());
}

#define TV1(value)                                                             \
  FmtIntTestVector<TestType> { static_cast<TestType>(value), #value }

TEMPLATE_TEST_CASE(
  "format::Int<TestType, Fmt::normal, UseSignForPositive=false>",
  "[format]",
  uint32_t,
  uint16_t,
  uint8_t,
  int32_t,
  int16_t,
  int8_t) {
  if constexpr (std::is_signed_v<TestType>) {
    SECTION("positive values") {
      FmtIntTestVector<TestType> vectors[]{
        TV1(0),
        TV1(1),
        TV1(2),
        TV1(3),
        TV1(4),
        TV1(5),
        TV1(6),
        TV1(7),
        TV1(8),
        TV1(9),
        TV1(10),
        TV1(11),
        TV1(12),
        TV1(13),
        TV1(14),
        TV1(15),
        TV1(16),
        TV1(17),
        TV1(18),
        TV1(19),
        TV1(20),
        {std::numeric_limits<TestType>::max(), max_string<TestType>()},
        {std::numeric_limits<TestType>::min(), min_string<TestType>()}
      };
      for (auto &tv : vectors) {
        test1(tv);
      }
    }
    SECTION("negative values") {
      FmtIntTestVector<TestType> vectors[]{
        TV1(-1),
        TV1(-2),
        TV1(-3),
        TV1(-4),
        TV1(-5),
        TV1(-6),
        TV1(-7),
        TV1(-8),
        TV1(-9),
        TV1(-10),
        TV1(-11),
        TV1(-12),
        TV1(-13),
        TV1(-14),
        TV1(-15),
        TV1(-16),
        TV1(-17),
        TV1(-18),
        TV1(-19),
        TV1(-20),
        {std::numeric_limits<TestType>::min(), min_string<TestType>()},
        {std::numeric_limits<TestType>::min(), min_string<TestType>()}
      };
      for (auto &tv : vectors) {
        test1(tv);
      }
    }
  } else {
    SECTION("positive values") {
      FmtIntTestVector<TestType> vectors[]{
        TV1(0),
        TV1(1),
        TV1(2),
        TV1(3),
        TV1(4),
        TV1(5),
        TV1(6),
        TV1(7),
        TV1(8),
        TV1(9),
        TV1(10),
        TV1(11),
        TV1(12),
        TV1(13),
        TV1(14),
        TV1(15),
        TV1(16),
        TV1(17),
        TV1(18),
        TV1(19),
        TV1(20),
        {std::numeric_limits<TestType>::max(), max_string<TestType>()},
        {std::numeric_limits<TestType>::min(), min_string<TestType>()}
      };
      for (auto &tv : vectors) {
        test1(tv);
      }
    }
  }
}

TEMPLATE_TEST_CASE(
  "format::Int<TestType, Fmt::normal, UseSignForPositive=true>",
  "[format]",
  uint32_t,
  uint16_t,
  uint8_t,
  int32_t,
  int16_t,
  int8_t) {
  if constexpr (std::is_signed_v<TestType>) {
    SECTION("positive values") {
      FmtIntTestVector<TestType> vectors[]{
        TV1(+0),
        TV1(+1),
        TV1(+2),
        TV1(+3),
        TV1(+4),
        TV1(+5),
        TV1(+6),
        TV1(+7),
        TV1(+8),
        TV1(+9),
        TV1(+10),
        TV1(+11),
        TV1(+12),
        TV1(+13),
        TV1(+14),
        TV1(+15),
        TV1(+16),
        TV1(+17),
        TV1(+18),
        TV1(+19),
        TV1(+20),
        {std::numeric_limits<TestType>::max(), "+" + max_string<TestType>()},
      };
      for (auto &tv : vectors) {
        test2(tv);
      }
    }
    SECTION("negative values") {
      FmtIntTestVector<TestType> vectors[]{
        TV1(-1),
        TV1(-2),
        TV1(-3),
        TV1(-4),
        TV1(-5),
        TV1(-6),
        TV1(-7),
        TV1(-8),
        TV1(-9),
        TV1(-10),
        TV1(-11),
        TV1(-12),
        TV1(-13),
        TV1(-14),
        TV1(-15),
        TV1(-16),
        TV1(-17),
        TV1(-18),
        TV1(-19),
        TV1(-20),
        {std::numeric_limits<TestType>::min(), min_string<TestType>()}
      };
      for (auto &tv : vectors) {
        test2(tv);
      }
    }
  } else {
    SECTION("positive values") {
      FmtIntTestVector<TestType> vectors[]{
        TV1(+0),
        TV1(+1),
        TV1(+2),
        TV1(+3),
        TV1(+4),
        TV1(+5),
        TV1(+6),
        TV1(+7),
        TV1(+8),
        TV1(+9),
        TV1(+10),
        TV1(+11),
        TV1(+12),
        TV1(+13),
        TV1(+14),
        TV1(+15),
        TV1(+16),
        TV1(+17),
        TV1(+18),
        TV1(+19),
        TV1(+20),
        {std::numeric_limits<TestType>::max(), "+" + max_string<TestType>()}
      };
      for (auto &tv : vectors) {
        test2(tv);
      }
    }
  }
}

TEMPLATE_TEST_CASE("format::Int<TestType, Fmt::hex, UseSignForPositive=false>",
                   "[format]",
                   uint32_t,
                   uint16_t,
                   uint8_t,
                   int32_t,
                   int16_t,
                   int8_t) {
  FmtIntTestVector<TestType> vectors[]{
    TV1(0x0),
    TV1(0x1),
    TV1(0x2),
    TV1(0x3),
    TV1(0x4),
    TV1(0x5),
    TV1(0x6),
    TV1(0x7),
    TV1(0x8),
    TV1(0x9),
    TV1(0x10),
    TV1(0x11),
    TV1(0x12),
    TV1(0x13),
    TV1(0x14),
    TV1(0x15),
    TV1(0x16),
    TV1(0x17),
    TV1(0x18),
    TV1(0x19),
    TV1(0x20),
    TV1(0xAB),
    {std::numeric_limits<TestType>::max(), max_string<TestType>(cli::Fmt::hex)},
    {std::numeric_limits<TestType>::min(),
            min_string<TestType>(cli::Fmt::hex)                                      }
  };
  for (auto &tv : vectors) {
    test_hex(tv);
  }
}

TEMPLATE_TEST_CASE(
  "format::Int<TestType, Fmt::binary, UseSignForPositive=false>",
  "[format]",
  uint32_t,
  uint16_t,
  uint8_t,
  int32_t,
  int16_t,
  int8_t) {
  FmtIntTestVector<TestType> vectors[]{
    TV1(0b0),
    TV1(0b1),
    TV1(0b1010),
    TV1(0b1011),
    TV1(0b10001011),
    {std::numeric_limits<TestType>::max(),
                  max_string<TestType>(cli::Fmt::binary)},
    {std::numeric_limits<TestType>::min(),
                  min_string<TestType>(cli::Fmt::binary)}
  };
  for (auto &tv : vectors) {
    test_bin(tv);
  }
}
