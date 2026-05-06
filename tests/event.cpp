#include "cli/event.hpp"
#include "common.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Control::Control()") {
  cli::Control c{};
  REQUIRE(c.type == cli::Control::bell);
  REQUIRE(c.param == 0);
}

TEST_CASE("Control::Control(type, param)") {
  cli::Control c{cli::Control::bell, 5};
  REQUIRE(c.type == cli::Control::bell);
  REQUIRE(c.param == 5);
}

TEST_CASE("Control::Control(const Control&)") {
  const cli::Control ctrl{cli::Control::bell, 5};
  cli::Control c(ctrl);
  REQUIRE(c.type == cli::Control::bell);
  REQUIRE(c.param == 5);

  const volatile cli::Control v_ctrl{cli::Control::bell, 5};
  cli::Control v_c(v_ctrl);
  REQUIRE(v_c.type == cli::Control::bell);
  REQUIRE(v_c.param == 5);
}

TEST_CASE("Control::Control(Control&&)") {
  cli::Control ctrl{cli::Control::bell, 5};
  cli::Control c(std::move(ctrl));
  REQUIRE(c.type == cli::Control::bell);
  REQUIRE(c.param == 5);

  volatile cli::Control v_ctrl{cli::Control::bell, 5};
  cli::Control v_c(std::move(v_ctrl));
  REQUIRE(v_c.type == cli::Control::bell);
  REQUIRE(v_c.param == 5);
}

TEST_CASE("Control::operator=(const Control&)") {
  const cli::Control ctrl{cli::Control::bell, 5};
  cli::Control c{cli::Control::backspace, 1};
  c = ctrl;
  REQUIRE(c.type == cli::Control::bell);
  REQUIRE(c.param == 5);

  const volatile cli::Control v_ctrl{cli::Control::bell, 5};
  cli::Control v_c{};
  v_c = v_ctrl;
  REQUIRE(v_c.type == cli::Control::bell);
  REQUIRE(v_c.param == 5);
}

TEST_CASE("Control::operator=(Control&&)") {
  cli::Control ctrl{cli::Control::bell, 5};
  cli::Control c{cli::Control::backspace, 1};
  c = std::move(ctrl);
  REQUIRE(c.type == cli::Control::bell);
  REQUIRE(c.param == 5);

  volatile cli::Control v_ctrl{cli::Control::bell, 5};
  cli::Control v_c(cli::Control::backspace, 1);
  v_c = std::move(v_ctrl);
  REQUIRE(v_c.type == cli::Control::bell);
  REQUIRE(v_c.param == 5);
}

TEST_CASE("Event::Event()") {
  cli::Event<char> e;
  REQUIRE(e.is_char);
  REQUIRE(e.c == 0);
}

TEST_CASE("Event::Event(char)") {
  cli::Event<char> e = cli::Event<char>('k');
  REQUIRE(e.is_char);
  REQUIRE(e.c == 'k');
}

TEST_CASE("Event::Event(control)") {
  cli::Event<char> e =
    cli::Event<char>(cli::Control{cli::Control::backspace, 5});
  REQUIRE_FALSE(e.is_char);
  REQUIRE(e.ctrl == cli::Control{cli::Control::backspace, 5});
}

TEST_CASE("Event::Event(const Event&)") {
  const cli::Event<char> ce{
    cli::Control{cli::Control::backspace, 5}
  };

  cli::Event<char> e{ce};
  REQUIRE_FALSE(e.is_char);
  REQUIRE(e.ctrl == cli::Control{cli::Control::backspace, 5});
}

TEST_CASE("Event::operator=(const Event&)") {
  const cli::Event<char> ce{
    cli::Control{cli::Control::backspace, 5}
  };

  cli::Event<char> e{};
  e = ce;
  REQUIRE_FALSE(e.is_char);
  REQUIRE(e.ctrl == cli::Control{cli::Control::backspace, 5});

  const cli::Event<char> ce2{'k'};

  e = ce2;
  REQUIRE(e.is_char);
  REQUIRE(e.c == 'k');
  const volatile cli::Event<char> v_ce1{
    cli::Control{cli::Control::backspace, 5}
  };

  volatile cli::Event<char> v_e{};
  v_e = v_ce1;
  REQUIRE_FALSE(v_e.is_char);
  REQUIRE(v_e.ctrl.type == cli::Control::backspace);
  REQUIRE(v_e.ctrl.param == 5);

  const volatile cli::Event<char> v_ce2{'k'};
  v_e = v_ce2;

  REQUIRE(v_e.is_char);
  REQUIRE(v_e.c == 'k');
}
