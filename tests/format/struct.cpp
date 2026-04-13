#include "catch2/catch_all.hpp"
#include "catch2/catch_test_macros.hpp"
#include "cli/format.hpp"
#include "cli/util.hpp"
using IntList = cli::FixedSizeVector<int, 10>;

struct S1 {
  uint16_t index;
  int16_t special;
  char character;
  bool enable;
};

struct FmtStructTestVector {
  S1 s;
  std::string input;
};

#define TV(index, special, character, enable)                                  \
  FmtStructTestVector {                                                        \
    S1{index, special, character, enable},                                     \
        std::string("{index = " #index ", special = " #special                 \
                    ", character = " #character ", enable = " #enable "}")     \
  }

TEST_CASE("fmt::Struct", "[format][Struct]") {
  const FmtStructTestVector vectors[]{
      TV(42, -42, 'c', true), TV(42, -42, 'c', false), TV(42, -42, 'x', false),
      TV(42, 42, 'x', true)};
  for (const auto &tv : vectors) {
    std::string buffer(256, 0);
    auto res = cli::format::DefaultFormat<S1, char>{}(
        {buffer.data(), buffer.size()}, tv.s);
    // REQUIRE(res);
    buffer.resize(res.size_written);
    // REQUIRE(buffer == tv.input);
  }
}
