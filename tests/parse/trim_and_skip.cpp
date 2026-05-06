#include "cli/parse.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("parse::skip_ws") {
  REQUIRE(cli::parse::skip_ws(cli::View{"hello"}) == "hello");
  REQUIRE(cli::parse::skip_ws(cli::View{" hello"}) == "hello");
  REQUIRE(cli::parse::skip_ws(cli::View{"    hello"}) == "hello");
  REQUIRE(cli::parse::skip_ws(cli::View{"\thello"}) == "hello");
  REQUIRE(cli::parse::skip_ws(cli::View{"\rhello"}) == "hello");
  REQUIRE(cli::parse::skip_ws(cli::View{"\nhello"}) == "hello");
  REQUIRE(cli::parse::skip_ws(cli::View{"\vhello"}) == "hello");
  REQUIRE(cli::parse::skip_ws(cli::View{"\fhello"}) == "hello");
}

TEST_CASE("parse::trim_ws") {
  REQUIRE(cli::parse::trim_ws(cli::View{"hello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{" hello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"    hello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\thello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\rhello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\nhello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\vhello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\fhello"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"hello  "}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{" hello "}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"    hello  "}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\thello\t"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\rhello\r"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\nhello\n"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\vhello\v"}) == "hello");
  REQUIRE(cli::parse::trim_ws(cli::View{"\fhello\f"}) == "hello");
}
