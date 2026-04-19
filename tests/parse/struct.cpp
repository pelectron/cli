#include "catch2/catch_test_macros.hpp"
#include "cli/parse.hpp"

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
  cli::parse::ParseResult<S, char> output;
};

#define str(x) std::string(x.data(), x.size())

TEST_CASE("Struct", "[parse][Struct]") {
  using cli::parse::ok;
  using Parser = cli::parse::DefaultParse<S, char>;

  SECTION("unnamed members") {

    StructTestVector vectors[] = {
      {"{[1,2,3,4], 12, -20, 65, true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{[1,2,3,4], 12, -20, 65, true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{ [1,2,3,4], 12, -20, 65, true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{ [1,2,3,4], 12, -20, 65, true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{[1,2,3,4],12,-20,65,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{[1,2,3,4], 12 , -20, 65 , true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{ [1,2,3,4], 12 , -20, 65 , true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{ [1,2,3,4], 12, -20 , 65, true }",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true}) },
      {"{ [1,2,3,4], 12, -20, 65, true} rest",
       {S{{1, 2, 3, 4}, 12, -20, 65, true}, " rest"}},
      {"{ [1,2,3,4], 12, -20, 65, true }xasdc",
       {S{{1, 2, 3, 4}, 12, -20, 65, true}, "xasdc"}}
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      CHECK(res);
      CHECK(res.error == tv.output.error);
      CHECK(res.value == tv.output.value);
      CHECK(str(res.rest) == str(tv.output.rest));
    }
  }

  SECTION("named members in order") {

    StructTestVector vectors[] = {
      {"{ints=[1,2,3,4], index =12, special = -20,character = "
       "'a',enable=true}",                                              ok<char>(S{{1, 2, 3, 4}, 12, -20, 'a', true}) },
      {"{ints=[1,2,3,4], index =12, special = -20,character = "
       "0x41,enable=true}",                                             ok<char>(S{{1, 2, 3, 4}, 12, -20, 0x41, true})},
      {"{ints=[1,2,3,4], index =12, special = -20,character = "
       "65,enable=true}",                                               ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})  },
      {"{ ints=[1,2,3,4], index=12,special=-20, character=  65, enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})                                                                          },
      {"{ints =[1,2,3,4], index=12, special=  -20, character  "
       "=65,enable=true }",                                             ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})  },
      {"{ ints =[1,2,3,4], index=12, special= -20, character= "
       "65,enable=true}",                                               ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})  },
      {"{ints=[1,2,3,4], index=12 , special=   -20, character "
       "=65,enable=true }",                                             ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})  },
      {"{ ints=[1,2,3,4], index=12, special=  -20     , character = "
       "65,enable=true }",                                              ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})  },
      {"{ints =[1,2,3,4], index=12, special=-20,character=65, enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})                                                                          },
      {"{ints=[1,2,3,4],index=12,special=-20,character=65,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})                                                                          }
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(str(res.rest) == str(tv.output.rest));
    }
  }

  SECTION("unnamed and named members in order") {

    StructTestVector vectors[] = {
      {"{ints=[1,2,3,4], index =12, -20 ,character =  65,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ints=[1,2,3,4], index =12, -20 , character =  65, true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ints=[1,2,3,4], index =12, -20,character =  65 ,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ints=[1,2,3,4], index =12, -20, character =  65 , true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{[1,2,3,4], index =12, -20 ,character =  65,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{[1,2,3,4], index =12, -20 , character =  65, true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ [1,2,3,4], index =12, -20,character =  65 ,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ [1,2,3,4],index=12,-20,character= 65,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(str(res.rest) == str(tv.output.rest));
    }
  }

  SECTION("named members out of order") {

    StructTestVector vectors[] = {
      {"{ints=[1,2,3,4],special=-20,index=12,character=65,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{enable=true,ints=[1,2,3,4],index=12,-20,65}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ints=[1,2,3,4],index=12,character=65,special=-20,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ints=[1,2,3,4],character=65,special=-20,index=12,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{special=-20,index=12,character=65,[1,2,3,4],true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{special=-20,index=12,character=65,enable=true,[1,2,3,4]}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{enable=true,ints=[1,2,3,4],special=-20,12,65}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{ints=[1,2,3,4],special=-20,index=12,character=65,enable=true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})}
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(str(res.rest) == str(tv.output.rest));
    }
  }

  SECTION("unnamed an named members, named out of order") {

    StructTestVector vectors[] = {
      {"{ints=[1,2,3,4],  12, character=65, -20,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{[1,2,3,4], 12,enable=true,-20,65}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{[1,2,3,4], 12,enable=true,-20,65}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
      {"{index=12,character= 65, [1,2,3,4],-20,true}",
       ok<char>(S{{1, 2, 3, 4}, 12, -20, 65, true})},
    };
    for (const auto &tv : vectors) {
      auto res = Parser{}(tv.input);
      CHECK(res);
      CHECK(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      CHECK(str(res.rest) == str(tv.output.rest));
    }
  }
}
