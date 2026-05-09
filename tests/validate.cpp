#include "cli/util.hpp"
#include "cli/validator.hpp"

#include <catch2/catch_test_macros.hpp>

struct S {};

TEST_CASE("validate::DefaultValidate") {
  S s;
  cli::validate::DefaultValidate<S> validate;
  REQUIRE(validate(s));
}

TEST_CASE("validate::NullValidate") {
  cli::validate::NullValidate validate;
  REQUIRE(validate(cli::dummy{}));
}
