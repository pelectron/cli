#include "cli/validator.hpp"

#include <catch2/catch_test_macros.hpp>

struct ValidateTest {};

TEST_CASE("validate::DefaultValidate") {
  ValidateTest s;
  cli::validate::DefaultValidate<ValidateTest> validate;
  REQUIRE(validate(s));
}

TEST_CASE("validate::NullValidate") {
  cli::validate::NullValidate validate;
  REQUIRE(validate(cli::dummy{}));
}
