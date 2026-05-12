#include "cli/enums.hpp"
#include "cli/parse.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("parse::Char") {
  cli::parse::Char<char, char> parse;
  cli::parse::ParseResult<char, char> res{cli::Error::none};

  SECTION("empty string") {
    res = parse("");
    REQUIRE_FALSE(res);
    REQUIRE(res.error == cli::Error::too_few_characters);
  }

  SECTION("unquoted char") {
    res = parse("c");
    REQUIRE(res);
    REQUIRE(res.value == 'c');
  }

  SECTION("quoted char") {

    SECTION("valid quoted char") {
      res = parse("'c'");
      REQUIRE(res);
      REQUIRE(res.value == 'c');
    }

    SECTION("no end quote") {
      res = parse("'c");
      REQUIRE_FALSE(res);
      REQUIRE(res.error == cli::Error::expected_endquote);
    }
  }

  SECTION("hex char") {

    SECTION("invalid hex char") {
      // "0x" will be parsed as 0' with rest "x"
      res = parse("0x");
      REQUIRE(res);
      REQUIRE(res.rest == "x");

      res = parse("0X");
      REQUIRE(res);
      REQUIRE(res.rest == "X");
    }

    SECTION("valid hex char") {
      res = parse("0x41");
      REQUIRE(res);
      REQUIRE(res.value == 0x41);
    }
  }
}
