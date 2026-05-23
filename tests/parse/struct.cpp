#include "catch2/catch_test_macros.hpp"
#include "cli/parse.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>

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

struct S2 {
  cli::View<const char> string{};
  bool b{false};
};

struct StructTestVector {
  cli::CharView input;
  cli::parse::ParseResult<S, char> output;
};

TEST_CASE("Struct", "[parse][Struct]") {
  using cli::parse::ok;
  using Parser = cli::parse::Parse<S, char>;

  SECTION("unnamed members") {
    StructTestVector vectors[] = {
      {"{[1,2,3,4], 12, -20, 0x41, true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{[1,2,3,4], 12, -20, 0x41, true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{ [1,2,3,4], 12, -20, 0x41, true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{ [1,2,3,4], 12, -20, 0x41, true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{[1,2,3,4],12,-20,0x41,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{[1,2,3,4], 12 , -20, 0x41 , true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{ [1,2,3,4], 12 , -20, 0x41 , true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{ [1,2,3,4], 12, -20 , 0x41, true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true}) },
      {"{ [1,2,3,4], 12, -20, 0x41, true} rest",
       {S{{1, 2, 3, 4}, 12, -20, 0x41, true}, " rest"}},
      {"{ [1,2,3,4], 12, -20, 0x41, true }xasdc",
       {S{{1, 2, 3, 4}, 12, -20, 0x41, true}, "xasdc"}}
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      CHECK(res);
      CHECK(res.error == tv.output.error);
      CHECK(res.value == tv.output.value);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("named members in order") {

    StructTestVector vectors[] = {
      {"{ints=[1,2,3,4], index =12, special = -20,character = "
       "'a',enable=true}",                                                ok<char>(S{{1, 2, 3, 4}, 12, -20, 'a', true}) },
      {"{ints=[1,2,3,4], index =12, special = -20,character = "
       "0x41,enable=true}",                                               ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{ints=[1,2,3,4], index =12, special = -20,character = "
       "0x41,enable=true}",                                               ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{ ints=[1,2,3,4], index=12,special=-20, character=  0x41, enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})                                                                          },
      {"{ints =[1,2,3,4], index=12, special=  -20, character  "
       "=0x41,enable=true }",                                             ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{ ints =[1,2,3,4], index=12, special= -20, character= "
       "0x41,enable=true}",                                               ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{ints=[1,2,3,4], index=12 , special=   -20, character "
       "=0x41,enable=true }",                                             ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{ ints=[1,2,3,4], index=12, special=  -20     , character = "
       "0x41,enable=true }",                                              ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{ints =[1,2,3,4], index=12, special=-20,character=0x41, enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})                                                                          },
      {"{ints=[1,2,3,4],index=12,special=-20,character=0x41,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})                                                                          }
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(res.rest == tv.output.rest);
    }
  }

  SECTION("unnamed and named members in order") {

    StructTestVector vectors[] = {
      {"{[1,2,3,4], 12, special = -20 ,character =  0x41, enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{[1,2,3,4],index= 12, special = -20 ,character =  0x41, enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(res.rest == tv.output.rest);
    }
  }

  SECTION("named members out of order") {

    StructTestVector vectors[] = {
      {"{ints=[1,2,3,4],character=0x41,special=-20,index=12,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{index=12,character=0x41,special=-20,enable=true,ints=[1,2,3,4]}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      CHECK(res);
      if (not res) {
        INFO("Failed struct parse");
        INFO(tv.input);
      }
      CHECK(res.error == tv.output.error);
      CHECK(res.value == tv.output.value);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("unnamed and named members, named out of order") {

    StructTestVector vectors[] = {
      {"{[1,2,3,4],  12, character=0x41, special=-20,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      CHECK(res);
      CHECK(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      CHECK(res.rest == tv.output.rest);
    }
  }

  SECTION("unnamed member after name encountered") {
    cli::View<const char> input =
      "{[1,2,3,4],  12, character=0x41, -20 ,enable=true}";
    auto res = Parser{}(input);
    REQUIRE_FALSE(res);
    REQUIRE(res.error == cli::Error::expected_field_name);
  }

  SECTION("invalid field name") {
    cli::View<const char> input =
      "{[1,2,3,4],  12, character=0x41, spec=-20 ,enable=true}";
    auto res = Parser{}(input);
    REQUIRE_FALSE(res);
    REQUIRE(res.error == cli::Error::invalid_field_name);
  }

  SECTION("field name fits value") {
    cli::View<const char> input = "{b, true}";
    cli::parse::Parse<S2, char> parse{};

    auto res = parse(input);
    REQUIRE(res.error == cli::Error::none);
    REQUIRE(res.value.string == "b");
    REQUIRE(res.value.b);

    input = "{string, true}";
    res = parse(input);
    REQUIRE(res.error == cli::Error::none);
    REQUIRE(res.value.string == "string");
    REQUIRE(res.value.b);

    input = "{b, b=true}";
    res = parse(input);
    REQUIRE(res.error == cli::Error::none);
    REQUIRE(res.value.string == "b");
    REQUIRE(res.value.b);

    input = "{string, b=true}";
    res = parse(input);
    REQUIRE(res.error == cli::Error::none);
    REQUIRE(res.value.string == "string");
    REQUIRE(res.value.b);

    input = "{string=hello, b=true}";
    res = parse(input);
    REQUIRE(res.error == cli::Error::none);
    REQUIRE(res.value.string == "hello");
    REQUIRE(res.value.b);
  }
}
