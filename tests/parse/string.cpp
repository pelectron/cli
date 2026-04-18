#include "catch2/catch_test_macros.hpp"
#include "cli/enums.hpp"
#include "cli/parse.hpp"
#include <array>
#include <string>

using Result = cli::parse::ParseResult<cli::CharView, char>;

struct StringTestVector {
  cli::CharView input;
  Result output;
};

#define PASS_TV_R(value, rest)                                                 \
  StringTestVector {                                                           \
    .input = value rest, .output = { value, rest }                             \
  }

#define PASS_TV(value)                                                         \
  StringTestVector {                                                           \
    .input = "\"" value "\"", .output = { value, {} }                          \
  }

#define PASS_TV_R_Q(value, rest)                                               \
  StringTestVector {                                                           \
    .input = value rest, .output = { value, rest }                             \
  }

#define PASS_TV_Q(value)                                                       \
  StringTestVector {                                                           \
    .input = "\"" value "\"", .output = { value, {} }                          \
  }

#define FAIL_TV(value, error)                                                  \
  StringTestVector {                                                           \
    .input = value, .output = { error }                                        \
  }

#define str(x) std::string(x.data(), x.size())

constexpr auto all_vis_chars() {
  std::array<char, 0x7E - 0x23 + 3> ret{};
  for (char c = 0x23; c <= 0x7E; ++c) {
    ret[c - 0x23 + 1] = c;
  }
  ret[0] = '"';
  ret[93] = '"';
  return ret;
}

static constexpr std::array vis_chars = all_vis_chars();
static constexpr cli::CharView vis_chars_view{vis_chars.data() + 1,
                                              vis_chars.size() - 2};
static constexpr cli::CharView quoted_vis_chars_view{vis_chars.data(),
                                                     vis_chars.size()};

TEST_CASE("parse::String") {
  cli::parse::String<cli::CharView, char> parse;

  SECTION("valid strings") {
    SECTION("unqoted") {
      StringTestVector vectors[]{
        StringTestVector{vis_chars_view, {vis_chars_view, {}}},
        PASS_TV("hello"),
        PASS_TV("kw341§$%=)(?\\^°~+-_.:,;<>|"),
        PASS_TV("kw341§$%=)(?\\^°"),
        PASS_TV_R("hello", "\"world"),
        PASS_TV_R("hello", "\tworld"),
        PASS_TV_R("hello", "\x7Fworld")
      };

      for (const auto &tv : vectors) {
        auto res = parse(tv.input);
        CHECK(res.error == tv.output.error);
        CHECK(str(res.value) == str(tv.output.value));
        CHECK(str(res.rest) == str(tv.output.rest));
      }
    }

    SECTION("quoted") {
      StringTestVector vectors[]{
        StringTestVector{quoted_vis_chars_view, {vis_chars_view, {}}},
        PASS_TV_Q("hello"),
        PASS_TV_Q("kw341§$%=)(?\\^°~+-_.:,;<>|"),
        PASS_TV_Q("kw341§$%=)(?\\^°"),
        PASS_TV_Q("\\\"kw341§$%=)(?\\^°"),
        PASS_TV_Q("kw341§$%=\\\")(?\\^°"),
        PASS_TV_R_Q("hello", "\"world"),
        PASS_TV_R_Q("hello", "\tworld"),
        PASS_TV_R_Q("hello", "\x7Fworld")
      };

      for (const auto &tv : vectors) {
        auto res = parse(tv.input);
        CHECK(res.error == tv.output.error);
        CHECK(str(res.value) == str(tv.output.value));
        CHECK(str(res.rest) == str(tv.output.rest));
      }
    }
  }

  SECTION("invalid strings") {
    using enum cli::Error;
    StringTestVector vectors[]{FAIL_TV("\"hello", unescaped_string),
                               FAIL_TV("\"hel\x7Flo", unescaped_string)};
    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      REQUIRE_FALSE(res);
      CHECK(res.error == tv.output.error);
      CHECK(str(res.value) == str(tv.output.value));
      CHECK(str(res.rest) == str(tv.output.rest));
    }
  }
}
