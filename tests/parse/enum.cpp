#include "catch2/catch_test_macros.hpp"
#include "cli/enums.hpp"
#include "cli/parse.hpp"
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
      value, {}                                                                \
    }                                                                          \
  }
#define str(x) std::string(x.data(), x.size())

TEST_CASE("parse::Enum") {
  SECTION("valid values without rest") {
    EnumTestVector<cli::Error> vectors[]{
      PASS_TV(none),
      PASS_TV(unimplemented),
      PASS_TV(cant_set_param),
      PASS_TV(cant_read_param),
      PASS_TV(invalid_cmd),
      PASS_TV(too_many_splits),
      PASS_TV(dual_separators),
      PASS_TV(buffer_overflow),
      PASS_TV(buffer_underflow),
      PASS_TV(incorrect_num_params),
      PASS_TV(too_many_argments),
      PASS_TV(too_few_arguments),
      PASS_TV(invalid_esc_seq),
      PASS_TV(invalid_state),
      PASS_TV(expected_value),
      PASS_TV(unexpected_characters_after_closing_paren),
      PASS_TV(expected_rparen),
      PASS_TV(too_few_characters),
      PASS_TV(invalid_character),
      PASS_TV(unescaped_string),
      PASS_TV(invalid_value)};
    constexpr cli::parse::Enum<cli::Error, char, false> parse;
    for (const auto &tv : vectors) {
      auto res = parse(tv.input);
      REQUIRE(res);
      REQUIRE(res.error == tv.output.error);
      REQUIRE(res.value == tv.output.value);
      REQUIRE(str(res.rest) == str(tv.output.rest));
    }
  }
}
