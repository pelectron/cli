#include <catch2/catch_test_macros.hpp>

#include "cli/format.hpp"
#include "cli/parse.hpp"

using FormatE = cli::format::DefaultFormat<bool, char>;
using ParseE = cli::parse::DefaultParse<bool, char>;

TEST_CASE("parse-format bool") {
  ParseE parse;
  FormatE format;
  SECTION("true value") {
    char buffer[32]{};
    SECTION("format -> parse") {
      auto fmt_res = format({buffer, 32}, true);
      REQUIRE(fmt_res);
      auto parse_res = parse({buffer, 32});
      REQUIRE(parse_res);
      REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
      REQUIRE(parse_res.value == true);
    }
    SECTION("parse -> format -> parse") {
      auto parse_res = parse("true");
      REQUIRE(parse_res);
      REQUIRE(parse_res.value == true);
      auto fmt_res = format({buffer, 32}, parse_res.value);
      REQUIRE(fmt_res);
      parse_res = parse({buffer, 32});
      REQUIRE(parse_res);
      REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
      REQUIRE(parse_res.value == true);
    }
  }
  SECTION("false value") {
    char buffer[32]{};
    SECTION("format -> parse") {
      auto fmt_res = format({buffer, 32}, false);
      REQUIRE(fmt_res);
      auto parse_res = parse({buffer, 32});
      REQUIRE(parse_res);
      REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
      REQUIRE(parse_res.value == false);
    }
    SECTION("parse -> format -> parse") {
      auto parse_res = parse("false");
      REQUIRE(parse_res);
      REQUIRE(parse_res.value == false);
      auto fmt_res = format({buffer, 32}, parse_res.value);
      REQUIRE(fmt_res);
      parse_res = parse({buffer, 32});
      REQUIRE(parse_res);
      REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
      REQUIRE(parse_res.value == false);
    }
  }
}
