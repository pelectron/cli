#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"

#include "cli/parse.hpp"
#include "common.hpp"
#include <iostream>

using Seq1 = cli::FixedCapacityVector<int, 10>;
using Seq2 = std::vector<int>;
using Result = cli::parse::ParseResult<cli::CharView, char>;

template<class Seq>
struct SeqTestVector {
  cli::CharView input;
  cli::parse::ParseResult<Seq, char> output;
};

#define FAIL_TV(input, error, rest)                                            \
  SeqTestVector<Seq> {                                                         \
    cli::CharView{input}, cli::parse::ParseResult<Seq, char> {                 \
      cli::parse::from_error, error, rest                                      \
    }                                                                          \
  }

#define PASS_TV(...)                                                           \
  SeqTestVector<Seq> {                                                         \
    .input = "[" #__VA_ARGS__ "]", .output = { Seq{__VA_ARGS__}, {} }          \
  }

#define PASS_TV1(...)                                                          \
  SeqTestVector<Seq> {                                                         \
    .input = "[ " #__VA_ARGS__ "]", .output = { Seq{__VA_ARGS__}, {} }         \
  }

#define PASS_TV2(...)                                                          \
  SeqTestVector<Seq> {                                                         \
    .input = "[" #__VA_ARGS__ " ]", .output = { Seq{__VA_ARGS__}, {} }         \
  }

#define PASS_TV3(...)                                                          \
  SeqTestVector<Seq> {                                                         \
    .input = "[ " #__VA_ARGS__ " ]", .output = { Seq{__VA_ARGS__}, {} }        \
  }

#define PASS(...)                                                              \
  PASS_TV(__VA_ARGS__), PASS_TV1(__VA_ARGS__), PASS_TV2(__VA_ARGS__),          \
    PASS_TV3(__VA_ARGS__)

#define PASS_TVR(...)                                                          \
  SeqTestVector<Seq> {                                                         \
    .input = "[" #__VA_ARGS__ "]rest", .output = {Seq{__VA_ARGS__}, "rest"}    \
  }

#define PASS_TV1R(...)                                                         \
  SeqTestVector<Seq> {                                                         \
    .input = "[ " #__VA_ARGS__ "]rest", .output = {Seq{__VA_ARGS__}, "rest"}   \
  }

#define PASS_TV2R(...)                                                         \
  SeqTestVector<Seq> {                                                         \
    .input = "[" #__VA_ARGS__ " ]rest", .output = {Seq{__VA_ARGS__}, "rest"}   \
  }

#define PASS_TV3R(...)                                                         \
  SeqTestVector<Seq> {                                                         \
    .input = "[ " #__VA_ARGS__ " ]rest", .output = {Seq{__VA_ARGS__}, "rest"}  \
  }

#define PASSR(...)                                                             \
  PASS_TVR(__VA_ARGS__), PASS_TV1R(__VA_ARGS__), PASS_TV2R(__VA_ARGS__),       \
    PASS_TV3R(__VA_ARGS__)

#define str(x) std::string(x.data(), x.size())

TEMPLATE_TEST_CASE("parse::Sequence", "", Seq1, Seq2) {
  using Seq = TestType;
  using Parser = cli::parse::Parse<Seq, char>;

  Parser parse;
  SECTION("valid sequences") {
    // clang-format off
    SeqTestVector<Seq> vectors[]={
      PASS(),
      PASS(1,2,3,4),
      PASS(1 ,2,3,4),
      PASS(1 , 2,3,4),
      PASS(1,2,3, 4),
      PASS(1 ,2,3, 4),
      PASS(1 , 2,3, 4),
      PASS(1,2,3 ,4),
      PASS(1 ,2,3 ,4),
      PASS(1 , 2,3 ,4),
      PASS(1,2,3 , 4),
      PASS(1 ,2,3 , 4),
      PASS(1 , 2,3 , 4),
      PASS(1,2, 3,4),
      PASS(1 ,2, 3,4),
      PASS(1 , 2, 3,4),
      PASS(1,2, 3, 4),
      PASS(1 ,2, 3, 4),
      PASS(1 , 2 ,3, 4),
      PASS(1,2, 3 ,4),
      PASS(1 ,2, 3 ,4),
      PASS(1 , 2, 3 ,4),
      PASS(1,2, 3 , 4),
      PASS(1 ,2, 3 , 4),
      PASS(1 , 2, 3 , 4),
      PASSR(),
      PASSR(1,2,3,4),
      PASSR(1 ,2,3,4),
      PASSR(1 , 2,3,4),
      PASSR(1,2,3, 4),
      PASSR(1 ,2,3, 4),
      PASSR(1 , 2,3, 4),
      PASSR(1,2,3 ,4),
      PASSR(1 ,2,3 ,4),
      PASSR(1 , 2,3 ,4),
      PASSR(1,2,3 , 4),
      PASSR(1 ,2,3 , 4),
      PASSR(1 , 2,3 , 4),
      PASSR(1,2, 3,4),
      PASSR(1 ,2, 3,4),
      PASSR(1 , 2, 3,4),
      PASSR(1,2, 3, 4),
      PASSR(1 ,2, 3, 4),
      PASSR(1 , 2 ,3, 4),
      PASSR(1,2, 3 ,4),
      PASSR(1 ,2, 3 ,4),
      PASSR(1 , 2, 3 ,4),
      PASSR(1,2, 3 , 4),
      PASSR(1 ,2, 3 , 4),
      PASSR(1 , 2, 3 , 4)
    };
    // clang-format on

    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(str(res.rest) == str(tv.output.rest));
    }
  }

  SECTION("invalid sequences") {
    SeqTestVector<Seq> vectors[]{
      FAIL_TV("", cli::Error::too_few_characters, ""),
      FAIL_TV("[", cli::Error::expected_closing_bracket, "["),
      FAIL_TV("[ ", cli::Error::expected_closing_bracket, "[ "),
      FAIL_TV("]", cli::Error::expected_open_bracket, "]"),
      FAIL_TV("[1,2,3", cli::Error::expected_closing_bracket, "[1,2,3"),
      FAIL_TV("[1,2 3", cli::Error::expected_delimiter, "3"),
    };
    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      REQUIRE_FALSE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(str(res.rest) == str(tv.output.rest));
    }
  }
}
