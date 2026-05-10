#include "cli/exec_result.hpp"

#include "cli/enums.hpp"
#include "common.hpp"
#include <catch2/catch_test_macros.hpp>

using Result = cli::ExecResult<char>;

TEST_CASE("ExecResult::make_success") {
  Result r = Result::make_success();
  REQUIRE(r);
  REQUIRE(r.type() == Result::success);
  REQUIRE(r.error() == cli::Error::none);
  REQUIRE(r.result().size() == 0);
}

TEST_CASE("ExecResult::make_success(result)") {
  Result r = Result::make_success("hello world");
  REQUIRE(r);
  REQUIRE(r.type() == Result::success);
  REQUIRE(r.error() == cli::Error::none);
  REQUIRE(r.result() == "hello world");
}

TEST_CASE("ExecResult::make_parse_error") {
  cli::View<const char> s{"hello world"};
  Result r =
    Result::make_parse_error(cli::Error::invalid_argument, s.data() + 5);
  REQUIRE_FALSE(r);
  REQUIRE(r.type() == Result::parse_error);
  REQUIRE(r.error_location() == s.data() + 5);
}

TEST_CASE("ExecResult::make_set_error") {
  Result r = Result::make_set_error(cli::Error::implementation_error);
  REQUIRE_FALSE(r);
  REQUIRE(r.type() == Result::set_error);
  REQUIRE(r.error() == cli::Error::implementation_error);
}

TEST_CASE("ExecResult::make_get_error") {
  Result r = Result::make_get_error(cli::Error::implementation_error);
  REQUIRE_FALSE(r);
  REQUIRE(r.type() == Result::get_error);
  REQUIRE(r.error() == cli::Error::implementation_error);
}

TEST_CASE("ExecResult::make_format_error") {
  Result r = Result::make_format_error(cli::Error::implementation_error);
  REQUIRE_FALSE(r);
  REQUIRE(r.type() == Result::format_error);
  REQUIRE(r.error() == cli::Error::implementation_error);
}

TEST_CASE("ExecResult::make_validation_error") {
  Result r = Result::make_validation_error(5);
  REQUIRE_FALSE(r);
  REQUIRE(r.type() == Result::validation_error);
  REQUIRE(r.error() == cli::Error::invalid_value);
  REQUIRE(r.index() == 5);
}
