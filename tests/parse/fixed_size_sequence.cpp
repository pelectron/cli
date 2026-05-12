#include "catch2/catch_test_macros.hpp"
#include "stringify.hpp"

#include "cli/parse.hpp"

struct SeqTestVector {
  cli::CharView input;
  cli::parse::ParseResult<std::array<int, 10>, char> output;
};

#define PASS_TV(...)                                                           \
  SeqTestVector {                                                              \
    .input = "[" #__VA_ARGS__ "]", .output = { {__VA_ARGS__}, {} }             \
  }

#define PASS_TV1(...)                                                          \
  SeqTestVector {                                                              \
    .input = "[ " #__VA_ARGS__ "]", .output = { {__VA_ARGS__}, {} }            \
  }

#define PASS_TV2(...)                                                          \
  SeqTestVector {                                                              \
    .input = "[" #__VA_ARGS__ " ]", .output = { {__VA_ARGS__}, {} }            \
  }

#define PASS_TV3(...)                                                          \
  SeqTestVector {                                                              \
    .input = "[ " #__VA_ARGS__ " ]", .output = { {__VA_ARGS__}, {} }           \
  }

#define PASS(...)                                                              \
  PASS_TV(__VA_ARGS__), PASS_TV1(__VA_ARGS__), PASS_TV2(__VA_ARGS__),          \
    PASS_TV3(__VA_ARGS__)

#define PASS_TVR(...)                                                          \
  SeqTestVector {                                                              \
    .input = "[" #__VA_ARGS__ "]rest", .output = {{__VA_ARGS__}, "rest"}       \
  }

#define PASS_TV1R(...)                                                         \
  SeqTestVector {                                                              \
    .input = "[ " #__VA_ARGS__ "]rest", .output = {{__VA_ARGS__}, "rest"}      \
  }

#define PASS_TV2R(...)                                                         \
  SeqTestVector {                                                              \
    .input = "[" #__VA_ARGS__ " ]rest", .output = {{__VA_ARGS__}, "rest"}      \
  }

#define PASS_TV3R(...)                                                         \
  SeqTestVector {                                                              \
    .input = "[ " #__VA_ARGS__ " ]rest", .output = {{__VA_ARGS__}, "rest"}     \
  }

#define PASSR(...)                                                             \
  PASS_TVR(__VA_ARGS__), PASS_TV1R(__VA_ARGS__), PASS_TV2R(__VA_ARGS__),       \
    PASS_TV3R(__VA_ARGS__)

TEST_CASE("parse::FixedSizeSequence") {
  using Parser = cli::parse::Parse<std::array<int, 10>, char>;
  Parser parse{};

  SECTION("valid input") {
    // clang-format off
    SeqTestVector passing_vectors[]={
      PASS(1,2,3,4,5,6,7,8,9,10),
      PASS(1, 2,3 ,4, 5 ,6,7,8,9,10),
      PASS( 1 , 2,3 ,4, 5 ,6,7,8,9,10 ),
      PASSR(1,2,3,4,5,6,7,8,9,10),
      PASSR(1, 2,3 ,4, 5 ,6,7,8,9,10),
      PASSR( 1 , 2,3 ,4, 5 ,6,7,8,9,10 ),
    };
    // clang-format on
    for (const auto &tv : passing_vectors) {
      auto res = parse(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(res.rest == tv.output.rest);
    }
  }

  SECTION("invalid input") {
    SeqTestVector vector[]{
      {.input = {},                         .output{cli::Error::too_few_characters}     },
      {.input = "[",                        .output{cli::Error::too_few_sequence_values}},
      {.input = "a",                        .output{cli::Error::expected_lbracket}      },
      {.input = "[]",                       .output{cli::Error::too_few_sequence_values}},
      {.input = "[1,2,3]",                  .output{cli::Error::too_few_sequence_values}},
      {.input = "[1 2,3]",                  .output{cli::Error::expected_delimiter}     },
      {.input = "[0,1,2,3,4,5,6,7,8,9,10]",
       .output{cli::Error::too_many_sequence_values}                                    },
      {.input = "[0,1,2,3,4,5,6,7,8,b]",
       .output{cli::Error::invalid_sequence_value}                                      },
      {.input = "[0,1,2,3,4,5,6,7,8,9",     .output{cli::Error::expected_rbracket}      },
      {.input = "[1,2,3",                   .output{cli::Error::expected_delimiter}     },
    };
    for (const auto &tv : vector) {
      auto res = parse(tv.input);
      REQUIRE_FALSE(res);
      REQUIRE(res.error == tv.output.error);
    }
  }
}
