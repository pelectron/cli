#include "catch2/catch_test_macros.hpp"
#include "cli/format.hpp"

#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("format::String") {
  SECTION("unquoted strings") {
    std::string buffer(255, 0);
    using Formatter = cli::format::String<cli::CharView, char, false>;
    auto res = Formatter{}({buffer.data(), buffer.size()}, "hello");
    REQUIRE(res.size_written == 5);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("hello"));
  }
  SECTION("quoted strings") {
    std::string buffer(255, 0);
    using Formatter = cli::format::String<cli::CharView, char, true>;
    auto res = Formatter{}({buffer.data(), buffer.size()}, "hello");
    REQUIRE(res.size_written == 7);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\"hello\""));
  }
}
