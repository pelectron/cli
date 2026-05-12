#include "catch2/catch_test_macros.hpp"
#include "cli/enums.hpp"
#include "cli/parse.hpp"
#include "common.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>

using enum cli::Error;

template<class Enum>
struct EnumTestVector {
  cli::View<const char> input;
  cli::parse::ParseResult<Enum, char> output;
};

#define PASS_TV(value)                                                         \
  EnumTestVector<decltype(value)> {                                            \
    #value, {                                                                  \
      cli::parse::from_value, value, {}                                        \
    }                                                                          \
  }

TEST_CASE("parse::Enum") {
  SECTION("no number allowed") {
    EnumTestVector<cli::Error> vectors[]{
      PASS_TV(none),
      PASS_TV(unimplemented),
      PASS_TV(cant_set_param),
      PASS_TV(cant_read_param),
      PASS_TV(invalid_cmd),
      PASS_TV(buffer_overflow),
      PASS_TV(too_few_arguments),
      PASS_TV(expected_value),
      PASS_TV(unexpected_characters_after_closing_paren),
      PASS_TV(expected_rparen),
      PASS_TV(too_few_characters),
      PASS_TV(invalid_character),
      PASS_TV(invalid_value),
      PASS_TV(unknown),
      {"0",        {cli::parse::from_error, cli::Error::invalid_value, "0"}    },
      {"0rest",    {cli::parse::from_error, cli::Error::invalid_value, "0rest"}},
      {"0none",    {cli::parse::from_error, cli::Error::invalid_value, "0none"}},
      {"noneRest", {cli::parse::from_value, cli::Error::none, "Rest"}          },
      {"abcd",     {cli::parse::from_error, cli::Error::invalid_value, "abcd"} }
    };

    constexpr cli::parse::Enum<cli::Error, char, false> parse;

    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      CHECK(bool(res) == bool(tv.output));
      CHECK(res.error == tv.output.error);
      CHECK(res.value == tv.output.value);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("numbers allowed") {
    EnumTestVector<cli::Error> vectors[]{
      PASS_TV(none),
      PASS_TV(unimplemented),
      PASS_TV(cant_set_param),
      PASS_TV(cant_read_param),
      PASS_TV(invalid_cmd),
      PASS_TV(buffer_overflow),
      PASS_TV(too_few_arguments),
      PASS_TV(expected_value),
      PASS_TV(unexpected_characters_after_closing_paren),
      PASS_TV(expected_rparen),
      PASS_TV(too_few_characters),
      PASS_TV(invalid_character),
      PASS_TV(invalid_value),
      {"0",          {cli::parse::from_value, cli::Error::none}                 },
      {"0rest",      {cli::parse::from_value, cli::Error::none, "rest"}         },
      {"0none",      {cli::parse::from_value, cli::Error::none, "none"}         },
      {"noneRest",   {cli::parse::from_value, cli::Error::none, "Rest"}         },
      {"abcd",       {cli::parse::from_error, cli::Error::invalid_value, "abcd"}},
      {"0xFFFFFFFF",
                       {cli::parse::from_error, cli::Error::invalid_value, "0xFFFFFFFF"}        },
    };

    constexpr cli::parse::Enum<cli::Error, char, true> parse;

    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      CHECK(bool(res) == bool(tv.output));
      CHECK(res.error == tv.output.error);
      CHECK(res.value == tv.output.value);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("invalid enum value (no number allowed)") {
    constexpr cli::parse::Enum<cli::Error, char, false> parse;
    cli::parse::ParseResult res = parse("abcd");
    REQUIRE_FALSE(res);
    REQUIRE(res.rest == "abcd");
  }
}

TEST_CASE("parse::FlagEnum") {
  using enum Flag;
  SECTION("valid values (no numbers allowed)") {
    // clang-format off
  EnumTestVector<Flag> vectors[]{
    PASS_TV(A),
    PASS_TV(B),
    PASS_TV(C),
    PASS_TV(D),
    PASS_TV(A|B),
    PASS_TV(A|B|C),
    PASS_TV(B|C|A),
    {"A | B | C", A|B|C},
    {"A | B | Crest", {A|B|C,"rest"}},
    {"A | B | C rest", {A|B|C,"rest"}},
  };
    // clang-format on

    constexpr cli::parse::Enum<Flag, char, false> parse;
    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      CHECK(res);
      CHECK(res.error == tv.output.error);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("invalid values (no number allowed)") {
    EnumTestVector<Flag> vectors[]{
      {"A | B | C | K", {cli::Error::invalid_value, "A | B | C | K"}                },
      {"A | K | C",     {cli::Error::invalid_value, "A | K | C"}                    },
      {"1",             {cli::parse::from_error, cli::Error::invalid_value, "1"}    },
      {"1rest",         {cli::parse::from_error, cli::Error::invalid_value, "1rest"}},
    };

    constexpr cli::parse::Enum<Flag, char, false> parse;

    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      CHECK_FALSE(res);
      CHECK(res.error == tv.output.error);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("valid values (numbers allowed)") {
    // clang-format off
  EnumTestVector<Flag> vectors[]{
    PASS_TV(A),
    PASS_TV(B),
    PASS_TV(C),
    PASS_TV(D),
    PASS_TV(A|B),
    PASS_TV(A|B|C),
    PASS_TV(B|C|A),
    {"A | B | C", A|B|C},
    {"A | B | Crest", {A|B|C,"rest"}},
    {"A | B | C rest", {A|B|C,"rest"}},
    {"1", {A}},
    {"2", {B}},
    {"3", {A|B}},
    {"1rest", {A,"rest"}},
    {"2 rest", {B," rest"}},
    {"3rest", {A|B,"rest"}},
  };
    // clang-format on

    constexpr cli::parse::Enum<Flag, char, true> parse;
    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      CHECK(bool(res) == bool(tv.output));
      CHECK(res.error == tv.output.error);
      CHECK(res.value == tv.output.value);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("invalid values (number allowed)") {
    EnumTestVector<Flag> vectors[]{
      {"A | B | C | K", {cli::Error::invalid_value, "A | B | C | K"}             },
      {"A | K | C",     {cli::Error::invalid_value, "A | K | C"}                 },
      {"10",            {cli::parse::from_error, cli::Error::invalid_value, "10"}},
    };

    constexpr cli::parse::Enum<Flag, char, false> parse;

    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      CHECK_FALSE(res);
      CHECK(res.error == tv.output.error);
      CHECK(res.rest == tv.output.rest);
    }
  }
}
