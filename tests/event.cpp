#include "cli/event.hpp"
#include "common.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Event::Event()") {
  cli::Event<char> e;
  REQUIRE(e.type() == cli::Control::character);
  REQUIRE(e.as_char() == 0);
}

TEST_CASE("Event::Event(char)") {
  cli::Event<char> e = cli::Event<char>('k');
  REQUIRE(e.type() == cli::Control::character);
  REQUIRE(e.as_char() == 'k');
}

TEST_CASE("Event::Event(control)") {
  cli::Event<char> e = cli::Event<char>(cli::Control::backspace, 5);
  REQUIRE(e.type() == cli::Control::backspace);
  REQUIRE(e.param() == 5);
}

TEST_CASE("Event::Event(const Event&)") {
  const cli::Event<char> ce{cli::Control::backspace, 5};

  cli::Event<char> e{ce};
  REQUIRE(e.type() == cli::Control::backspace);
  REQUIRE(e.param() == 5);
}

TEST_CASE("Event::operator=(const Event&)") {
  const cli::Event<char> ce{cli::Control::backspace, 5};

  cli::Event<char> e{};
  e = ce;
  REQUIRE(e.type() == cli::Control::backspace);
  REQUIRE(e.param() == 5);

  const cli::Event<char> ce2{'k'};

  e = ce2;
  REQUIRE(e.type() == cli::Control::character);
  REQUIRE(e.as_char() == 'k');

  const volatile cli::Event<char> v_ce1{cli::Control::backspace, 5};

  volatile cli::Event<char> v_e{};
  v_e = v_ce1;
  REQUIRE(v_e.type() == cli::Control::backspace);
  REQUIRE(v_e.param() == 5);

  const volatile cli::Event<char> v_ce2{'k'};
  v_e = v_ce2;

  REQUIRE(v_e.type() == cli::Control::character);
  REQUIRE(v_e.as_char() == 'k');
}
