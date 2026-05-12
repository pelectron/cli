#include "catch2/catch_test_macros.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "cli/traits.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>

enum class E : int {
  A = 0,
  B = 1,
  C = 2,
  D = 3
};

namespace cli::traits {

  template<>
  struct enum_traits<E> {
    static constexpr int min = 0;
    static constexpr int max = 3;
    static constexpr bool is_flag = false;
  };
} // namespace cli::traits

using FormatE = cli::format::Format<E, char>;
using ParseE = cli::parse::Parse<E, char>;

using FormatF = cli::format::Format<Flag, char>;
using ParseF = cli::parse::Parse<Flag, char>;

TEST_CASE("parse-format enum") {
  FormatE format;
  ParseE parse;
  const std::pair<E, std::string> values[]{
    {E::A, "A"},
    {E::B, "B"},
    {E::C, "C"},
    {E::D, "D"}
  };
  char buffer[32]{};
  for (auto [e, s] : values) {
    auto fmt_res = format({buffer, 32}, e);
    REQUIRE(fmt_res);
    REQUIRE(std::string(buffer, fmt_res.size_written) == s);
    auto parse_res = parse({buffer, 32});
    REQUIRE(parse_res);
    REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
    REQUIRE(parse_res.value == e);
  }
}

TEST_CASE("parse-format flag enum") {
  FormatF format;
  ParseF parse;
  const std::pair<Flag, std::string> values[]{
    {Flag::A,                     "A"    },
    {Flag::B,                     "B"    },
    {Flag::C,                     "C"    },
    {Flag::D,                     "D"    },
    {Flag::A | Flag::B,           "A|B"  },
    {Flag::A | Flag::C,           "A|C"  },
    {Flag::A | Flag::B | Flag::C, "A|B|C"},
  };
  char buffer[32]{};
  for (auto [e, s] : values) {
    auto fmt_res = format({buffer, 32}, e);
    REQUIRE(fmt_res);
    REQUIRE(std::string(buffer, fmt_res.size_written) == s);
    auto parse_res = parse({buffer, 32});
    REQUIRE(parse_res);
    REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
    REQUIRE(parse_res.value == e);
  }
}
