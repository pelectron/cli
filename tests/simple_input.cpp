#include "cli/event.hpp"
#include "cli/input.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

struct base_cfg {
  static constexpr cli::View<const char> name = "cli";
  static constexpr cli::View<const char> description =
    "a command line interface";
  static constexpr std::size_t max_line_length = 16;
};

struct volatile_cfg : base_cfg {
  static constexpr bool use_volatile_input_buffer = true;
};

struct autocomplete_cfg : base_cfg {
  static constexpr bool use_autocomplete = true;
};

struct history_cfg : base_cfg {
  static constexpr bool use_history = true;
};

struct cursor_cfg : base_cfg {
  static constexpr bool use_cursor = true;
};

struct cr_cfg : base_cfg {
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::cr;
};

struct crlf_cfg : base_cfg {
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::crlf;
};

TEMPLATE_TEST_CASE("SimpleInput::on_char", "[input]", base_cfg, volatile_cfg) {
  using Input = cli::SimpleInput<TestType>;

  Input input{};

  SECTION("a full input can't accept more than input_size") {
    for (std::size_t i = 0; i < cli::config::input_size_v<TestType>; ++i) {
      REQUIRE(input.on_char(static_cast<char>('a' + i)) == cli::Error::none);
    }
    REQUIRE(input.on_char('A') == cli::Error::buffer_overflow);
  }
  SECTION("backspace") {
    input.on_char('\b');
    cli::Event<char> ev;
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::backspace);
    REQUIRE(ev.param() == 1);
  }

  SECTION("bell") {
    input.on_char(0x07);
    cli::Event<char> ev;
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::bell);
    REQUIRE(ev.param() == 1);
  }
}

TEMPLATE_TEST_CASE("SimpleInput::on_control",
                   "[input]",
                   base_cfg,
                   volatile_cfg) {
  using Input = cli::SimpleInput<TestType>;
  using Event = typename Input::event_type;

  Input input;
  Event ev;

  input.on_control(cli::Control::backspace, 5);
  input.pop_event(ev);
  REQUIRE(ev.type() == cli::Control::backspace);
  REQUIRE(ev.param() == 5);

  input.on_control(cli::Control::character, 'a');
  input.pop_event(ev);
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == 'a');

  input.on_control(cli::Control::bell, 5);
  input.pop_event(ev);
  REQUIRE(ev.type() == cli::Control::bell);
  REQUIRE(ev.param() == 5);

  input.on_control(cli::Control::clear_screen, 5);
  input.pop_event(ev);
  REQUIRE(ev.type() == cli::Control::clear_screen);
  REQUIRE(ev.param() == 5);

  input.on_control(cli::Control::clear_line, 5);
  input.pop_event(ev);
  REQUIRE(ev.type() == cli::Control::clear_line);
  REQUIRE(ev.param() == 5);

  input.on_control(cli::Control::clear_line_to_begin, 5);
  input.pop_event(ev);
  REQUIRE(ev.type() == cli::Control::clear_line_to_begin);
  REQUIRE(ev.param() == 5);
}

TEST_CASE("SimpleInput crlf", "[input]") {
  cli::SimpleInput<crlf_cfg> input{};
  cli::Event<char> ev{};

  input.on_char('\n');
  input.on_char('\r');
  input.on_char('\n');
  input.on_char('\r');
  input.on_char('a');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\n');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::enter);
  REQUIRE(ev.param() == 1);

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\r');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == 'a');

  REQUIRE_FALSE(input.pop_event(ev));
}

TEST_CASE("SimpleInput lf", "input") {
  cli::SimpleInput<base_cfg> input{};
  cli::Event<char> ev{};

  input.on_char('\n');
  input.on_char('\r');
  input.on_char('\n');
  input.on_char('\r');
  input.on_char('a');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::enter);
  REQUIRE(ev.param() == 1);

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\r');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::enter);
  REQUIRE(ev.param() == 1);

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\r');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == 'a');

  REQUIRE_FALSE(input.pop_event(ev));
}

TEST_CASE("SimpleInput cr", "input") {
  cli::SimpleInput<cr_cfg> input{};
  cli::Event<char> ev{};

  input.on_char('\n');
  input.on_char('\r');
  input.on_char('\n');
  input.on_char('\r');
  input.on_char('a');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\n');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::enter);
  REQUIRE(ev.param() == 1);

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\n');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::enter);
  REQUIRE(ev.param() == 1);

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == 'a');

  REQUIRE_FALSE(input.pop_event(ev));
}
TEST_CASE("SimpleInput autocomplete", "[input]") {
  SECTION("no autocomplete") {
    cli::SimpleInput<base_cfg> input;
    cli::Event<char> ev{};

    input.on_char('\t');
    REQUIRE_FALSE(input.pop_event(ev));

    input.on_control(cli::Control::autocomplete);
    REQUIRE_FALSE(input.pop_event(ev));
  }

  SECTION("with autocomplete") {
    cli::SimpleInput<autocomplete_cfg> input;
    cli::Event<char> ev{};

    input.on_char('\t');
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::autocomplete);
    REQUIRE(ev.param() == 1);

    input.on_control(cli::Control::autocomplete);
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::autocomplete);
    REQUIRE(ev.param() == 1);
  }
}

TEST_CASE("SimpleInput history", "[input]") {
  SECTION("no history") {
    cli::SimpleInput<base_cfg> input;
    cli::Event<char> ev{};

    input.on_control(cli::Control::cursor_up);
    REQUIRE_FALSE(input.pop_event(ev));

    input.on_control(cli::Control::cursor_down);
    REQUIRE_FALSE(input.pop_event(ev));
  }

  SECTION("with history") {
    cli::SimpleInput<history_cfg> input;
    cli::Event<char> ev{};

    input.on_control(cli::Control::cursor_up);
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::cursor_up);
    REQUIRE(ev.param() == 1);

    input.on_control(cli::Control::cursor_down);
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::cursor_down);
    REQUIRE(ev.param() == 1);
  }
}

TEST_CASE("SimpleInput cursor", "[input]") {
  SECTION("no cursor") {
    cli::SimpleInput<base_cfg> input;
    cli::Event<char> ev{};

    input.on_control(cli::Control::cursor_left);
    REQUIRE_FALSE(input.pop_event(ev));

    input.on_control(cli::Control::cursor_right);
    REQUIRE_FALSE(input.pop_event(ev));

    input.on_control(cli::Control::delete_char);
    REQUIRE_FALSE(input.pop_event(ev));
    input.on_char(0x7F);
    REQUIRE_FALSE(input.pop_event(ev));

    input.on_control(cli::Control::clear_line_to_end);
    REQUIRE_FALSE(input.pop_event(ev));
  }

  SECTION("with cursor") {
    cli::SimpleInput<cursor_cfg> input;
    cli::Event<char> ev{};

    input.on_control(cli::Control::cursor_left);
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::cursor_left);
    REQUIRE(ev.param() == 1);

    input.on_control(cli::Control::cursor_right);
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::cursor_right);
    REQUIRE(ev.param() == 1);

    input.on_control(cli::Control::delete_char);
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::delete_char);
    REQUIRE(ev.param() == 1);

    input.on_control(cli::Control::clear_line_to_end);
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::clear_line_to_end);
    REQUIRE(ev.param() == 1);
  }
}
