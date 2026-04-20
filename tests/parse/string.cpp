#include "cli/string.hpp"
#include "catch2/catch_test_macros.hpp"
#include "cli/enums.hpp"
#include "cli/parse.hpp"
#include "common.hpp"
#include <array>
#include <string>

TEST_CASE("parse::String") {
  cli::parse::String<std::string, char> parse;

  SECTION("empty strings") {
    SECTION("invalid empty string") {
      auto res = parse("");
      REQUIRE_FALSE(res);
      REQUIRE(res.error == cli::Error::too_few_characters);
    }
    SECTION("valid empty string") {
      auto res = parse("\"\"");
      REQUIRE(res);
      REQUIRE(res.rest.size() == 0);
      REQUIRE(res.value == "");
    }
  }
  SECTION("single space") {
    auto res = parse(" ");
    REQUIRE_FALSE(res);
    REQUIRE(res.error == cli::Error::invalid_character);
  }
  SECTION("escaped quote") {
    auto res = parse("\\\"");
    REQUIRE(res);
    REQUIRE(res.rest.size() == 0);
    REQUIRE(res.value == "\"");

    res = parse("\\\" bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\"");

    res = parse("\\\"bc bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\"bc");
    res = parse("\\\"\\\" bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\"\"");
  }
  SECTION("no spaces") {
    auto res = parse("hello");
    REQUIRE(res);
    REQUIRE(res.rest.size() == 0);
    REQUIRE(res.value == "hello");

    res = parse("hello world");
    REQUIRE(res);
    REQUIRE(res.rest == " world");
    REQUIRE(res.value == "hello");
  }
  SECTION("no spaces with quotes") {
    auto res = parse("hello\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "hello\"world");

    res = parse("hello\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "hello\"world");

    res = parse("\\\"hello\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\"hello\"world");

    res = parse("\\\"hello\\\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\"hello\"world");
  }
  SECTION("quoted") {
    auto res = parse("\"hello world\"bc");
    REQUIRE(res);
    REQUIRE(res.rest == "bc");
    REQUIRE(res.value == "hello world");

    res = parse("\"hello \\\"world\"bc");
    REQUIRE(res);
    REQUIRE(res.rest == "bc");
    REQUIRE(res.value == "hello \"world");

    res = parse("\"hello \\\"world\"bc");
    REQUIRE(res);
    REQUIRE(res.rest == "bc");
    REQUIRE(res.value == "hello \"world");
  }
}

TEST_CASE("parse::StringView") {
  cli::parse::StringView<cli::CharView, char> parse;
  SECTION("empty strings") {
    SECTION("invalid empty string") {
      auto res = parse("");
      REQUIRE_FALSE(res);
      REQUIRE(res.error == cli::Error::too_few_characters);
    }
    SECTION("valid empty string") {
      auto res = parse("\"\"");
      REQUIRE(res);
      REQUIRE(res.rest.size() == 0);
      REQUIRE(res.value == "");
    }
  }
  SECTION("single space") {
    auto res = parse(" ");
    REQUIRE_FALSE(res);
    REQUIRE(res.error == cli::Error::invalid_character);
  }
  SECTION("escaped quote") {
    auto res = parse("\\\"");
    REQUIRE(res);
    REQUIRE(res.rest.size() == 0);
    REQUIRE(res.value == "\\\"");

    res = parse("\\\" bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\\\"");

    res = parse("\\\"bc bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\\\"bc");

    res = parse("\\\"\\\" bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\\\"\\\"");
  }
  SECTION("no spaces") {
    auto res = parse("hello");
    REQUIRE(res);
    REQUIRE(res.rest.size() == 0);
    REQUIRE(res.value == "hello");

    res = parse("hello world");
    REQUIRE(res);
    REQUIRE(res.rest == " world");
    REQUIRE(res.value == "hello");
  }
  SECTION("no spaces with quotes") {
    auto res = parse("hello\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "hello\"world");

    res = parse("hello\\\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "hello\\\"world");

    res = parse("hello\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "hello\"world");

    res = parse("\\\"hello\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\\\"hello\"world");

    res = parse("\\\"hello\\\"world bc");
    REQUIRE(res);
    REQUIRE(res.rest == " bc");
    REQUIRE(res.value == "\\\"hello\\\"world");
  }
  SECTION("quoted") {
    auto res = parse("\"hello world\"bc");
    REQUIRE(res);
    REQUIRE(res.rest == "bc");
    REQUIRE(res.value == "hello world");

    res = parse("\"hello \\\"world\"bc");
    REQUIRE(res);
    REQUIRE(res.rest == "bc");
    REQUIRE(res.value == "hello \\\"world");
  }
}
