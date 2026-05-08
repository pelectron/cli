#include "cli/input.hpp"
#include "catch2/catch_template_test_macros.hpp"
#include "cli/event.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>

struct non_volatile_cfg {
  using char_type = char;
  static constexpr cli::View<const char_type> name = "cli";
  static constexpr cli::View<const char_type> description =
    "a command line interface";
  static constexpr char_type access_separator = '.';
  static constexpr bool use_autocomplete = true;
  static constexpr bool use_cursor = true;
  static constexpr bool use_history = false;
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::lf;
  static constexpr std::size_t input_size = 8;
  static constexpr std::size_t max_line_length = 32;
  static constexpr bool use_volatile_input_buffer = false;
};

struct volatile_cfg {
  using char_type = char;
  static constexpr cli::View<const char_type> name = "cli";
  static constexpr cli::View<const char_type> description =
    "a command line interface";
  static constexpr char_type access_separator = '.';
  static constexpr bool use_autocomplete = true;
  static constexpr bool use_cursor = true;
  static constexpr bool use_history = false;
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::lf;
  static constexpr std::size_t input_size = 8;
  static constexpr std::size_t max_line_length = 32;
  static constexpr bool use_volatile_input_buffer = false;
};

struct cr_config : non_volatile_cfg {
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::cr;
};

struct crlf_config : non_volatile_cfg {
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::crlf;
};

TEMPLATE_TEST_CASE("Input::on_char", "", non_volatile_cfg, volatile_cfg) {
  using Input = cli::Input<TestType>;

  Input input{};

  SECTION("a full input can't accept more than input_size") {
    for (std::size_t i = 0; i < cli::config::input_size_v<TestType>; ++i) {
      REQUIRE(input.on_char((uint8_t)('a' + i)) == cli::Error::none);
    }
    REQUIRE(input.on_char('A') == cli::Error::buffer_overflow);
  }
}

TEMPLATE_TEST_CASE("Input::on_control", "", non_volatile_cfg, volatile_cfg) {
  using Input = cli::Input<TestType>;
  using Event = typename Input::event_type;

  Input input;
  input.on_control(cli::Control::backspace, 5);
  Event ev;
  input.pop_event(ev);
  REQUIRE(ev.type() == cli::Control::backspace);
  REQUIRE(ev.param() == 5);
}

TEMPLATE_TEST_CASE("Input::pop_event", "", non_volatile_cfg, volatile_cfg) {
  using Input = cli::Input<TestType>;
  using Event = typename Input::event_type;

  Input input{};

  SECTION("an empty input can't be popped from") {
    Event ev;
    REQUIRE_FALSE(input.pop_event(ev));
  }

  SECTION("events are popped in FIFO order") {
    for (std::size_t i = 0; i < cli::config::input_size_v<TestType>; ++i) {
      input.on_char('a' + i);
    }

    Event ev;

    for (std::size_t i = 0; i < cli::config::input_size_v<TestType>; ++i) {
      REQUIRE(input.pop_event(ev));
      REQUIRE(ev.type() == cli::Control::character);
      REQUIRE(ev.as_char() == 'a' + i);
    }
  }
}

TEMPLATE_TEST_CASE("Input escape sequences",
                   "",
                   non_volatile_cfg,
                   volatile_cfg) {
  using Input = cli::Input<TestType>;
  using Event = typename Input::event_type;

  Input input{};

  const std::map<cli::View<const char>, cli::Event<char>> esc_sequences{
    {"\x07",      {cli::Control::bell, 1}               },
    {"\x08",      {cli::Control::backspace, 1}          },
    {"\t",        {cli::Control::autocomplete, 1}       },
    {"\t",        {cli::Control::autocomplete, 1}       },
    {"\n",        {cli::Control::enter, 1}              },
    {"\x7F",      {cli::Control::delete_char, 1}        },
    {"\x1B[A",    {cli::Control::cursor_up, 1}          },
    {"\x1B[123A", {cli::Control::cursor_up, 123_u8}     },
    {"\x1B[B",    {cli::Control::cursor_down, 1}        },
    {"\x1B[123B", {cli::Control::cursor_down, 123_u8}   },
    {"\x1B[C",    {cli::Control::cursor_right, 1}       },
    {"\x1B[123C", {cli::Control::cursor_right, 123_u8}  },
    {"\x1B[D",    {cli::Control::cursor_left, 1}        },
    {"\x1B[123D", {cli::Control::cursor_left, 123_u8}   },
    {"\x1B[K",    {cli::Control::clear_line_to_end, 1}  },
    {"\x1B[0K",   {cli::Control::clear_line_to_end, 1}  },
    {"\x1B[1K",   {cli::Control::clear_line_to_begin, 1}},
    {"\x1B[2K",   {cli::Control::clear_line, 1}         },
    {"\x1B[2J",   {cli::Control::clear_screen, 1}       },
  };

  for (const auto &[seq, ctrl] : esc_sequences) {
    for (const char &ch : seq) {
      input.on_char(ch);
    }

    input.on_char('Z');

    Event ev;
    input.pop_event(ev);
    REQUIRE(ev.type() == ctrl.type());
    REQUIRE(ev.param() == ctrl.param());
    REQUIRE(input.pop_event(ev));
    REQUIRE(ev.type() == cli::Control::character);
    REQUIRE(ev.as_char() == 'Z');
  }
}

TEMPLATE_TEST_CASE("Input unrecginized sequences are printed as is",
                   "",
                   non_volatile_cfg,
                   volatile_cfg) {
  using Input = cli::Input<TestType>;
  using Event = typename Input::event_type;

  Input input{};

  const std::vector<cli::View<const char>> esc_sequences{
    "\x1B[X",
    "\x1B[256A",
    "\r",
  };

  for (const auto &seq : esc_sequences) {
    for (const char &ch : seq)
      input.on_char(ch);

    char buffer[8]{};
    std::size_t i = 0;
    Event ev;
    while (input.pop_event(ev)) {
      REQUIRE(ev.type() == cli::Control::character);
      buffer[i++] = ev.as_char();
    }

    CHECK(i == seq.size());
    for (std::size_t j = 0; j < i; ++j) {
      CHECK(seq[j] == buffer[j]);
    }
  }
}

TEST_CASE("Input crlf config") {
  using Input = cli::Input<crlf_config>;
  using Event = typename Input::event_type;

  Input input{};

  input.on_char('\n');
  input.on_char('\r');
  input.on_char('\n');

  Event ev{};

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\n');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::enter);
  REQUIRE(ev.param() == 1);

  REQUIRE_FALSE(input.pop_event(ev));
}

TEST_CASE("Input cr config") {
  using Input = cli::Input<cr_config>;
  using Event = typename Input::event_type;

  Input input{};

  input.on_char('\n');
  input.on_char('\r');
  input.on_char('\n');
  Event ev{};

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\n');

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::enter);
  REQUIRE(ev.param() == 1);

  REQUIRE(input.pop_event(ev));
  REQUIRE(ev.type() == cli::Control::character);
  REQUIRE(ev.as_char() == '\n');

  REQUIRE_FALSE(input.pop_event(ev));
}
