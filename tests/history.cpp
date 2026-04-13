#include "cli/history.hpp"
#include "catch2/catch_test_macros.hpp"
#include "cli/config.hpp"
#include <catch2/catch_all.hpp>
#include <iostream>

TEST_CASE("History") {
  cli::History<cli::default_config> h;

  REQUIRE(h.cursor_up() == cli::View<const char>{});
  REQUIRE(h.cursor_down() == cli::View<const char>{});

  h.push("cmd1");
  REQUIRE(h.cursor_up() == "cmd1");
  REQUIRE(h.cursor_down() == "cmd1");

  h.push("cmd2");
  REQUIRE(h.cursor_down() == "cmd2");
  REQUIRE(h.cursor_up() == "cmd1");
  REQUIRE(h.cursor_up() == "cmd1");
  REQUIRE(h.cursor_down() == "cmd2");
  REQUIRE(h.cursor_down() == "cmd2");

  h.push("cmd3");
  REQUIRE(h.cursor_down() == "cmd3");
  REQUIRE(h.cursor_up() == "cmd2");
  REQUIRE(h.cursor_up() == "cmd1");
  REQUIRE(h.cursor_up() == "cmd1");
  REQUIRE(h.cursor_down() == "cmd2");
  REQUIRE(h.cursor_down() == "cmd3");
  REQUIRE(h.cursor_down() == "cmd3");

  for (std::size_t i = 4; i < 20; ++i) {
    auto s = std::format("cmd{}", i);
    h.push(s.data());
  }

  REQUIRE(h.cursor_down() == "cmd19");
  REQUIRE(h.cursor_down() == "cmd19");
  REQUIRE(h.cursor_up() == "cmd18");
  h.push("cmd20");
  REQUIRE(h.cursor_up() == "cmd20");
  REQUIRE(h.cursor_up() == "cmd19");
  for (auto i = 0; i < 13; ++i) {
    h.cursor_up();
  }
  REQUIRE(h.cursor_up() == "cmd5");
  REQUIRE(h.cursor_up() == "cmd5");
}
