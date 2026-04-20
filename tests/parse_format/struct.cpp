#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "common.hpp"

#include <catch2/catch_test_macros.hpp>

using IntList = cli::FixedCapacityVector<int, 10>;

struct S {
  IntList ints;
  uint16_t index;
  int16_t special;
  char character;
  bool enable;
  constexpr bool operator==(const S &s) const {
    return ints == s.ints and index == s.index and special == s.special and
           character == s.character;
  }
};

struct StructTestVector {
  cli::CharView input;
  S value;
};

using Parse = cli::parse::DefaultParse<S, char>;
using Format = cli::format::DefaultFormat<S, char>;

TEST_CASE("parse-format Struct") {
  Parse parse;
  Format format;
  char buffer[256]{};
  const S s{
    {1, 2, 3, 4},
    12, -20, 'a', true
  };

  auto fmt_res = format({buffer, 256}, s);

  REQUIRE(fmt_res);
  REQUIRE(std::string(buffer, fmt_res.size_written) ==
          "{ ints = [1, 2, 3, 4], index = 12, special = -20, character = 'a', "
          "enable = true}");
  auto parse_res =
    parse("{ ints = [1,2,3,4], index = 12, special = -20, character = 'a', "
          "enable = true}");
  REQUIRE(parse_res);
  REQUIRE(parse_res.value == s);
}
