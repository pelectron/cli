#include "catch2/catch_test_macros.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>
#include <cstdint>

enum class E {
  A,
  B,
  C,
  D
};

using FormatE = cli::format::DefaultFormat<E, char>;
using ParseE = cli::parse::DefaultParse<E, char>;

using FormatF = cli::format::DefaultFormat<F, char>;
using ParseF = cli::parse::DefaultParse<F, char>;

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
  const std::pair<F, std::string> values[]{
    {F::A,               "A"    },
    {F::B,               "B"    },
    {F::C,               "C"    },
    {F::D,               "D"    },
    {F::A | F::B,        "A|B"  },
    {F::A | F::C,        "A|C"  },
    {F::A | F::B | F::C, "A|B|C"},
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
