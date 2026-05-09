#include "catch2/catch_test_macros.hpp"
#include "cli/enums.hpp"
#include "cli/format.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("format::Format<bool>") {
  constexpr cli::format::Format<bool, char> format;
  char buffer[32]{};
  SECTION("true value") {
    auto res = format(cli::View<char>{buffer, 32}, true);
    REQUIRE(res);
    REQUIRE(res.size_written == 4);
    REQUIRE(std::string((const char *)buffer) == "true");
  }
  SECTION("false value") {
    auto res = format(cli::View<char>{buffer, 32}, false);
    REQUIRE(res);
    REQUIRE(res.size_written == 5);
    REQUIRE(std::string((const char *)buffer) == "false");
  }
  SECTION("not enough space for true") {
    auto res = format({buffer, 3}, true);
    REQUIRE_FALSE(res);
    REQUIRE(res.error == cli::Error::buffer_overflow);
  }
  SECTION("not enough space for false") {
    auto res = format({buffer, 4}, false);
    REQUIRE_FALSE(res);
    REQUIRE(res.error == cli::Error::buffer_overflow);
  }
}
