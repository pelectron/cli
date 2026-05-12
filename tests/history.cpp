#include "cli/history.hpp"
#include "cli/config.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("History") {
  cli::History<cli::default_config> h;

  REQUIRE(h.cursor_up(1) == cli::View<const char>{});
  REQUIRE(h.cursor_down(1) == cli::View<const char>{});

  h.push("cmd1");
  REQUIRE(h.cursor_up(1) == "cmd1");
  REQUIRE(h.cursor_down(1) == "cmd1");

  h.push("cmd2");
  REQUIRE(h.cursor_down(1) == "cmd2");
  REQUIRE(h.cursor_up(1) == "cmd1");
  REQUIRE(h.cursor_up(1) == "cmd1");
  REQUIRE(h.cursor_down(1) == "cmd2");
  REQUIRE(h.cursor_down(1) == "cmd2");

  h.push("cmd3");
  REQUIRE(h.cursor_down(1) == "cmd3");
  REQUIRE(h.cursor_up(1) == "cmd2");
  REQUIRE(h.cursor_up(1) == "cmd1");
  REQUIRE(h.cursor_up(1) == "cmd1");
  REQUIRE(h.cursor_down(1) == "cmd2");
  REQUIRE(h.cursor_down(1) == "cmd3");
  REQUIRE(h.cursor_down(1) == "cmd3");

  for (std::size_t i = 4; i < 20; ++i) {
    auto s = std::format("cmd{}", i);
    h.push(s.data());
  }

  REQUIRE(h.cursor_down(1) == "cmd19");
  REQUIRE(h.cursor_down(1) == "cmd19");
  REQUIRE(h.cursor_up(1) == "cmd18");
  h.push("cmd20");
  REQUIRE(h.cursor_up(1) == "cmd20");
  REQUIRE(h.cursor_up(1) == "cmd19");
  h.cursor_up(13);
  REQUIRE(h.cursor_up(1) == "cmd5");
  REQUIRE(h.cursor_up(1) == "cmd5");
  REQUIRE(h.cursor_down(5) == "cmd10");

  SECTION("reset") {
    h.reset();
    REQUIRE(h.cursor_down(1) == "");
    REQUIRE(h.cursor_up(1) == "");
  }
}

TEST_CASE("Histroy push empty string") {
  cli::History<cli::default_config> h;
  h.push({});
  REQUIRE(h.cursor_down(1) == "");
  h.push({});
  REQUIRE(h.cursor_up(1) == "");
}

TEST_CASE("Histroy cursor up down with n = 0") {
  cli::History<cli::default_config> h;
  h.push("hello");
  REQUIRE(h.cursor_down(0) == "");
  h.push("hello");
  REQUIRE(h.cursor_up(0) == "");
  REQUIRE(h.cursor_down(0) == "");
  REQUIRE(h.cursor_down(0) == "");
}
