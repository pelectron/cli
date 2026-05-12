#include "cli/line.hpp"
#include "cli/concepts.hpp"
#include "cli/enums.hpp"
#include "cli/string.hpp"
#include "stringify.hpp"
#include "test_display.hpp"

#include <catch2/catch_all.hpp>
#include <string>
#include <vector>

TEST_CASE("Display") {
  Display d;
  d.data = "hello world";
  SECTION("cursor left") {
    d.cursor_left(1);
    REQUIRE(d.cursor == 0);
    d.cursor = 5;
    d.cursor_left(4);
    REQUIRE(d.cursor == 1);
  }
  SECTION("cursor right") {
    d.cursor_right(1);
    REQUIRE(d.cursor == 1);
    d.cursor = d.data.size() + 5;
    d.cursor_right(d.data.size() + 5);
    REQUIRE(d.cursor == d.data.size());
  }
  SECTION("delete char") {
    d.cursor = 5;
    d.delete_char();
    REQUIRE(d.data == "helloworld");
    REQUIRE(d.cursor == 5);
  }
  SECTION("clear") {
    d.clear_line();
    REQUIRE(d.data == "");
    REQUIRE(d.cursor == 0);
  }
  SECTION("backspace") {
    d.cursor = 5;
    d.backspace(1);
    REQUIRE(d.data == "hell world");
    REQUIRE(d.cursor == 4);
    d.backspace(10);
    REQUIRE(d.cursor == 0);
    REQUIRE(d.data == " world");
  }
  SECTION("write") {
    d.cursor = d.data.size();
    d.write('1');
    REQUIRE(d.cursor == d.data.size());
    REQUIRE(d.data == "hello world1");

    d.cursor = 5;
    d.write('1');
    REQUIRE(d.cursor == 6);
    REQUIRE(d.data == "hello1world1");

    d.data = "hello world";
    d.cursor = d.data.size();
    d.write("12");
    REQUIRE(d.cursor == d.data.size());
    REQUIRE(d.data == "hello world12");

    d.cursor = 5;
    d.write("12");
    REQUIRE(d.cursor == 7);
    REQUIRE(d.data == "hello12orld12");
  }
}

struct NoCursor_NoAutocomplete {
  using char_type = char;
  static constexpr cli::View<const char_type> name = "cli";
  static constexpr cli::View<const char_type> description =
    "a command line interface";
  static constexpr bool use_history = false;
  static constexpr char access_separator = '.';
  static constexpr bool use_cursor = false;
  static constexpr bool use_autocomplete = false;
  static constexpr std::size_t max_line_length = 32;
  static constexpr bool use_detailed_error_messages = true;
};

struct NoCursor_Autocomplete {
  using char_type = char;
  static constexpr cli::View<const char_type> name = "cli";
  static constexpr cli::View<const char_type> description =
    "a command line interface";
  static constexpr bool use_history = false;
  static constexpr char access_separator = '.';
  static constexpr bool use_cursor = false;
  static constexpr bool use_autocomplete = true;
  static constexpr std::size_t max_line_length = 32;
  static constexpr bool use_detailed_error_messages = true;
};

struct Cursor_NoAutocomplete {
  using char_type = char;
  static constexpr cli::View<const char_type> name = "cli";
  static constexpr cli::View<const char_type> description =
    "a command line interface";
  static constexpr bool use_history = false;
  static constexpr char access_separator = '.';
  static constexpr bool use_cursor = true;
  static constexpr bool use_autocomplete = false;
  static constexpr std::size_t max_line_length = 32;
  static constexpr bool use_detailed_error_messages = true;
};

struct Cursor_Autocomplete {
  using char_type = char;
  static constexpr cli::View<const char_type> name = "cli";
  static constexpr cli::View<const char_type> description =
    "a command line interface";
  static constexpr bool use_history = false;
  static constexpr char access_separator = '.';
  static constexpr bool use_cursor = true;
  static constexpr bool use_autocomplete = true;
  static constexpr std::size_t max_line_length = 32;
  static constexpr bool use_detailed_error_messages = true;
};

static_assert(cli::concepts::Config<NoCursor_NoAutocomplete>);
static_assert(cli::concepts::Config<NoCursor_Autocomplete>);
static_assert(cli::concepts::Config<Cursor_NoAutocomplete>);
static_assert(cli::concepts::Config<Cursor_Autocomplete>);

static_assert(cli::concepts::DisplayWithoutCursor<Display, char>);
static_assert(cli::concepts::DisplayWithCursor<Display, char>);
static_assert(cli::concepts::Display<Display, char>);

#define DEFINE_COMMANDS()                                                      \
  cli::CommandNode<char> root;                                                 \
  cli::CommandNode<char> c1{.name = "c1",                                      \
                            .description = {},                                 \
                            .this_ = &root,                                    \
                            .exec_ =                                           \
                              +[](void *,                                      \
                                  cli::View<const char>,                       \
                                  cli::View<char>) -> cli::ExecResult<char> {  \
                              return cli::ExecResult<char>::make_success();    \
                            }};                                                \
  cli::CommandNode<char> c2{                                                   \
    .name = "c2",                                                              \
    .description = {},                                                         \
    .this_ = &root,                                                            \
    .exec_ = +[](void *,                                                       \
                 cli::View<const char>,                                        \
                 cli::View<char>) -> cli::ExecResult<char> {                   \
      return cli::ExecResult<char>::make_success("hello");                     \
    }};                                                                        \
  cli::CommandNode<char> c3{                                                   \
    .name = "c3",                                                              \
    .description = {},                                                         \
    .this_ = &root,                                                            \
    .exec_ = +[](void *,                                                       \
                 cli::View<const char> buf,                                    \
                 cli::View<char>) -> cli::ExecResult<char> {                   \
      return cli::ExecResult<char>::make_parse_error(cli::Error::invalid_cmd,  \
                                                     buf.data());              \
    }};                                                                        \
  cli::CommandNode<char> c4{.name = "c4long",                                  \
                            .description = {},                                 \
                            .this_ = &root,                                    \
                            .exec_ =                                           \
                              +[](void *,                                      \
                                  cli::View<const char>,                       \
                                  cli::View<char>) -> cli::ExecResult<char> {  \
                              return cli::ExecResult<char>::make_set_error(    \
                                cli::Error::expected_assignment);              \
                            }};                                                \
  cli::CommandNode<char> c5{.name = "c5long",                                  \
                            .description = {},                                 \
                            .this_ = &root,                                    \
                            .exec_ =                                           \
                              +[](void *,                                      \
                                  cli::View<const char>,                       \
                                  cli::View<char>) -> cli::ExecResult<char> {  \
                              return cli::ExecResult<char>::make_get_error(    \
                                cli::Error::expected_delimiter);               \
                            }};                                                \
  cli::CommandNode<char> z1{.name = "z1",                                      \
                            .description = {},                                 \
                            .this_ = &root,                                    \
                            .exec_ =                                           \
                              +[](void *,                                      \
                                  cli::View<const char>,                       \
                                  cli::View<char>) -> cli::ExecResult<char> {  \
                              return cli::ExecResult<char>::make_format_error( \
                                cli::Error::invalid_argument);                 \
                            }};                                                \
  cli::CommandNode<char> z2{                                                   \
    .name = "z2",                                                              \
    .description = {},                                                         \
    .this_ = &root,                                                            \
    .exec_ = +[](void *,                                                       \
                 cli::View<const char>,                                        \
                 cli::View<char>) -> cli::ExecResult<char> {                   \
      return cli::ExecResult<char>::make_validation_error(42);                 \
    }};                                                                        \
  root.add_sub(c1);                                                            \
  root.add_sub(c2);                                                            \
  c2.add_sub(c3);                                                              \
  root.add_sub(c4);                                                            \
  c4.add_sub(c5);                                                              \
  root.add_sub(z1);                                                            \
  root.add_sub(z2)

TEMPLATE_TEST_CASE("cli::Line::execute and on_char after execute",
                   "",
                   NoCursor_NoAutocomplete,
                   Cursor_NoAutocomplete,
                   NoCursor_Autocomplete,
                   Cursor_Autocomplete) {
  DEFINE_COMMANDS();
  SECTION("single line display") {
    Display d;
    cli::Line<TestType, Display> line(root, d);
    char buffer[16]{};
    cli::View<char> out{buffer, 16};

    SECTION("invalid command") {
      if constexpr (not std::is_same_v<TestType, NoCursor_Autocomplete>) {
        line.set_data("c5long");
        REQUIRE(line.execute(out) == cli::Error::invalid_cmd);
        REQUIRE(d.data == "invalid_cmd");
        REQUIRE(d.past == std::vector<std::string>{"c5long"});
      }
    }

    SECTION("no error from execute") {
      SECTION("empty result") {
        line.set_data("c1");
        REQUIRE(line.execute(out) == cli::Error::none);
        REQUIRE(d.data == "c1");
        REQUIRE(d.past.empty());
      }
      SECTION("with result") {
        line.set_data("c2");
        REQUIRE(line.execute(out) == cli::Error::none);
        REQUIRE(d.data == "hello");
        REQUIRE(d.past == std::vector<std::string>{"c2"});
      }
    }

    SECTION("parse_error from execute") {
      line.set_data("c2.c3 args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data == "parse error: invalid_cmd at 6");
      REQUIRE(d.past == std::vector<std::string>{"c2.c3 args"});
    }

    SECTION("set_error from execute") {
      line.set_data("c4long args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data == "can't set param: expected '='");
      REQUIRE(d.past == std::vector<std::string>{"c4long args"});
    }

    SECTION("get_error from execute") {
      line.set_data("c4long.c5long args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data == "can't get param: expected_delimiter");
      REQUIRE(d.past == std::vector<std::string>{"c4long.c5long args"});
    }

    SECTION("format_error from execute") {
      line.set_data("z1 args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data == "format error: invalid_argument");
      REQUIRE(d.past == std::vector<std::string>{"z1 args"});
    }

    SECTION("format_error from execute") {
      line.set_data("z2 args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data == "arg 42 is invalid");
      REQUIRE(d.past == std::vector<std::string>{"z2 args"});
    }

    SECTION("on_char after execute") {
      line.set_data("c1");
      char buf[16]{};
      cli::View<char> out_{buf, 16};
      line.execute(out_);
      line.on_char('c');
      REQUIRE(d.data == "c");
      REQUIRE(d.past == std::vector<std::string>{"c1"});
    }
  }

  SECTION("execute with multiline display") {
    MultilineDisplay d;
    cli::Line<TestType, MultilineDisplay> line(root, d);
    char buffer[16]{};
    cli::View<char> out{buffer, 16};

    SECTION("invalid command") {
      if constexpr (not std::is_same_v<TestType, NoCursor_Autocomplete>) {
        line.set_data("c5long");
        REQUIRE(line.execute(out) == cli::Error::invalid_cmd);
        REQUIRE(d.data.empty());
        REQUIRE(d.past == std::vector<std::string>{"c5long", "invalid_cmd"});
      }
    }

    SECTION("no error from execute") {
      SECTION("empty result") {
        line.set_data("c1");
        REQUIRE(line.execute(out) == cli::Error::none);
        REQUIRE(d.data.empty());
        REQUIRE(d.past == std::vector<std::string>{"c1"});
      }
      SECTION("with result") {
        line.set_data("c2");
        REQUIRE(line.execute(out) == cli::Error::none);
        REQUIRE(d.data.empty());
        REQUIRE(d.past == std::vector<std::string>{"c2", "hello"});
      }
    }

    SECTION("parse_error from execute") {
      line.set_data("c2.c3 args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data.empty());
      REQUIRE(d.past == std::vector<std::string>{
                          "c2.c3 args", "parse error: invalid_cmd at 6"});
    }

    SECTION("set_error from execute") {
      line.set_data("c4long args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data.empty());
      REQUIRE(d.past == std::vector<std::string>{
                          "c4long args", "can't set param: expected '='"});
    }

    SECTION("get_error from execute") {
      line.set_data("c4long.c5long args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data.empty());
      REQUIRE(d.past ==
              std::vector<std::string>{"c4long.c5long args",
                                       "can't get param: expected_delimiter"});
    }

    SECTION("format_error from execute") {
      line.set_data("z1 args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data.empty());
      REQUIRE(d.past == std::vector<std::string>{
                          "z1 args", "format error: invalid_argument"});
    }

    SECTION("format_error from execute") {
      line.set_data("z2 args");
      REQUIRE(line.execute(out) == cli::Error::none);
      REQUIRE(d.data.empty());
      REQUIRE(d.past ==
              std::vector<std::string>{"z2 args", "arg 42 is invalid"});
    }

    SECTION("on_char after execute") {
      MultilineDisplay display;
      cli::Line<NoCursor_Autocomplete, MultilineDisplay> line_(root, display);
      char buf[16]{};
      cli::View<char> out_{buf, 16};
      line_.set_data("c1");
      line_.execute(out_);
      line_.on_char('c');
      REQUIRE(display.data == "c");
      REQUIRE(display.past == std::vector<std::string>{"c1"});
    }
  }
}

TEST_CASE("cli::Line<NoCursor, NoAutocomplete>") {
  DEFINE_COMMANDS();
  Display d;
  cli::Line<NoCursor_NoAutocomplete, Display> line(root, d);
  REQUIRE(line.on_char('c') == cli::Error::none);
  REQUIRE(line.on_char('1') == cli::Error::none);
  REQUIRE(line.view() == "c1");
  REQUIRE(line.on_char('.') == cli::Error::none);
  REQUIRE(line.on_char('c') == cli::Error::none);
  REQUIRE(line.on_char('2') == cli::Error::none);
  REQUIRE(line.view() == "c1.c2");

  SECTION("on_char line is full") {
    line.set_data("01234567890123456789012345678901");
    REQUIRE(line.on_char('x') == cli::Error::buffer_overflow);
  }

  SECTION("on_char empty and access_separator") {
    line.clear();
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.view().size() == 0);
  }

  SECTION("backspace(1)") {
    line.on_backspace(1);
    REQUIRE(line.view() == "c1.c");
    line.on_backspace(4);
    REQUIRE(line.view().size() == 0);
  }

  SECTION("backspace(2)") {
    line.on_backspace(2);
    REQUIRE(line.view() == "c1.");
    line.on_backspace(3);
    REQUIRE(line.view().size() == 0);
  }

  SECTION("autocomplete") {
    line.set_data("c1.c2");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
  }

  SECTION("clear") {
    line.clear();
    REQUIRE(line.view().size() == 0);
  }
}

TEST_CASE("cli::Line<NoCursor, Autocomplete>") {
  DEFINE_COMMANDS();
  Display d;
  cli::Line<NoCursor_Autocomplete, Display> line(root, d);
  REQUIRE(line.on_char('a') == cli::Error::none);
  REQUIRE(line.view().size() == 0);
  REQUIRE(d.data.empty());

  SECTION("on_char no subcommands") {
    REQUIRE(line.on_char('c') == cli::Error::none);
    REQUIRE(line.on_char('1') == cli::Error::none);
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
  }

  SECTION("on_char empty and access_separator") {
    line.clear();
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.view().size() == 0);
  }

  SECTION("on_char in args") {
    line.set_data("c2.c3 args");
    REQUIRE(line.on_char('1') == cli::Error::none);
    REQUIRE(line.view() == "c2.c3 args1");
  }

  SECTION("on_char and full line") {
    line.set_data("c1 01234567890123456789012345678");
    REQUIRE(line.on_char('x') == cli::Error::buffer_overflow);
  }

  SECTION("autocomplete with empty line") {
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
  }

  SECTION("correct set_data without subcommands") {
    REQUIRE(line.set_data("c1") == cli::Error::none);
    REQUIRE(d.data == "c1");
    REQUIRE(line.set_data("c1 args") == cli::Error::none);
    REQUIRE(d.data == "c1 args");
  }

  SECTION("incorrect set_data without subcommands") {
    REQUIRE_FALSE(line.set_data("c1.") == cli::Error::none);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c1. args") == cli::Error::none);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c3") == cli::Error::none);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c3 args") == cli::Error::none);
    REQUIRE(d.data.empty());
  }

  SECTION("correct set_data with subcommands") {
    REQUIRE(line.set_data("c2") == cli::Error::none);
    REQUIRE(line.view() == "c2");
    REQUIRE(d.data == "c2");
    REQUIRE(line.set_data("c2 args") == cli::Error::none);
    REQUIRE(line.view() == "c2 args");
    REQUIRE(d.data == "c2 args");
    REQUIRE(line.set_data("c2.") == cli::Error::none);
    REQUIRE(line.view() == "c2.");
    REQUIRE(d.data == "c2.");
    REQUIRE(line.set_data("c2.c") == cli::Error::none);
    REQUIRE(line.view() == "c2.c");
    REQUIRE(d.data == "c2.c");
    REQUIRE(line.set_data("c2.c3") == cli::Error::none);
    REQUIRE(line.view() == "c2.c3");
    REQUIRE(d.data == "c2.c3");
    REQUIRE(line.set_data("c2.c3 args") == cli::Error::none);
    REQUIRE(line.view() == "c2.c3 args");
    REQUIRE(d.data == "c2.c3 args");
  }

  SECTION("incorrect set_data with subcommands") {
    REQUIRE_FALSE(line.set_data("c3") == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c2. args") == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c3 args") == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c2.c4") == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c2.c4 args") == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c2.c3.") == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE_FALSE(line.set_data("c2.c3. args") == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
  }

  SECTION("autocomplete without subcommands") {
    REQUIRE(line.set_data("c1") == cli::Error::none);
    REQUIRE(d.data == "c1");
    REQUIRE(d.cursor == 2);
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
    REQUIRE(d.cursor == 2);
  }

  SECTION("autocomplete with subcommands with starting character") {
    line.set_data("c2");
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.on_char('c') == cli::Error::none);
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3");
    REQUIRE(d.data == "c2.c3");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3");
    REQUIRE(d.data == "c2.c3");
  }

  SECTION("autocomplete with full name") {
    line.set_data("c2");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.");
    REQUIRE(d.data == "c2.");
  }

  SECTION("autocomplete with full name and separator") {
    line.set_data("c2.");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3");
    REQUIRE(d.data == "c2.c3");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3");
    REQUIRE(d.data == "c2.c3");
  }

  SECTION("autocomplete with full name and separator") {
    line.set_data("c2.c");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3");
    REQUIRE(d.data == "c2.c3");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3");
    REQUIRE(d.data == "c2.c3");
  }

  SECTION("autocomplete in args") {
    line.set_data("c2.c3 args");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3 args");
    REQUIRE(d.data == "c2.c3 args");
  }
}

TEST_CASE("cli::Line<Cursor, NoAutocomplete>") {
  DEFINE_COMMANDS();
  Display d;
  cli::Line<Cursor_NoAutocomplete, Display> line(root, d);
  SECTION("on_char cursor at the end") {
    REQUIRE(line.on_char('c') == cli::Error::none);
    REQUIRE(line.on_char('1') == cli::Error::none);
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
    REQUIRE(d.cursor == 2);
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.on_char('c') == cli::Error::none);
    REQUIRE(line.on_char('2') == cli::Error::none);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 5);
  }
  SECTION("on_char cursor in the middle") {
    line.set_data("c1.c2");
    REQUIRE(d.cursor == 5);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");

    REQUIRE(line.on_cursor_right(1) == cli::Error::none);
    REQUIRE(d.cursor == d.data.size());

    REQUIRE(line.on_cursor_left(1) == cli::Error::none);
    REQUIRE(d.cursor == 4);

    REQUIRE(line.on_char('b') == cli::Error::none);
    REQUIRE(line.view() == "c1.cb2");
    REQUIRE(d.data == "c1.cb2");
    REQUIRE(d.cursor == 5);

    REQUIRE(line.on_char('a') == cli::Error::none);
    REQUIRE(line.view() == "c1.cba2");
    REQUIRE(d.data == "c1.cba2");
    REQUIRE(line.on_cursor_left(6) == cli::Error::none);
    REQUIRE(line.on_char('x') == cli::Error::none);
    REQUIRE(line.view() == "xc1.cba2");
    REQUIRE(line.on_cursor_left(1) == cli::Error::none);
    REQUIRE(line.on_cursor_left(5) == cli::Error::none);
    REQUIRE(d.data == "xc1.cba2");
  }
  SECTION("on_char line is full") {
    line.set_data("01234567890123456789012345678901");
    REQUIRE(line.on_char('x') == cli::Error::buffer_overflow);
  }
  SECTION("on_char empty and access_separator") {
    line.clear();
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.view().size() == 0);
  }
  SECTION("backspace at the end") {
    line.set_data("c1.c2");
    line.on_backspace();
    REQUIRE(line.view() == "c1.c");
    REQUIRE(d.data == "c1.c");

    line.on_backspace(4);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());

    line.on_backspace(4);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
  }
  SECTION("backspace in the middle") {
    line.set_data("c1.c2");
    line.on_cursor_left(1);
    line.on_backspace();
    REQUIRE(d.cursor == 3);
    REQUIRE(line.view() == "c1.2");
    REQUIRE(d.data == "c1.2");

    line.on_backspace(3);
    REQUIRE(line.view() == "2");
    REQUIRE(d.data == "2");
  }
  SECTION("backspace at the beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    line.on_backspace();
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 0);
    line.on_cursor_right(1);
    line.on_backspace(2);
    REQUIRE(line.view() == "1.c2");
    REQUIRE(d.data == "1.c2");
    REQUIRE(d.cursor == 0);
  }
  SECTION("delete_char at the end") {
    line.set_data("c1.c2");
    line.on_delete_char();
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 5);
  }
  SECTION("delete_char in the middle") {
    line.set_data("c1.c2");
    REQUIRE(d.cursor == 5);
    line.on_cursor_left(1);
    line.on_delete_char();
    REQUIRE(line.view() == "c1.c");
    REQUIRE(d.data == "c1.c");
    REQUIRE(d.cursor == 4);
    line.on_cursor_left(2);
    line.on_delete_char();
    line.on_delete_char();
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
    REQUIRE(d.cursor == 2);
    line.set_data("c1.c2");
    line.on_cursor_left(3);
    line.on_delete_char();
    line.on_delete_char();
    REQUIRE(line.view() == "c12");
    REQUIRE(d.data == "c12");
  }
  SECTION("delete_char at the beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    line.on_delete_char();
    REQUIRE(line.view() == "1.c2");
    REQUIRE(d.data == "1.c2");
    REQUIRE(d.cursor == 0);
    for (auto i = 0; i < 10; ++i)
      line.on_delete_char();
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_end at end") {
    line.set_data("c1.c2");
    REQUIRE(line.on_clear_line_to_end() == cli::Error::none);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 5);
  }
  SECTION("clear_line_to_end in middle") {
    line.set_data("c1.c2");
    line.on_cursor_left(2);
    REQUIRE(line.on_clear_line_to_end() == cli::Error::none);
    // REQUIRE(line.view() == "c1.");
    REQUIRE(d.data == "c1.");
    REQUIRE(d.cursor == 3);
  }
  SECTION("clear_line_to_end in beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    REQUIRE(line.on_clear_line_to_end() == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_begin at the end") {
    line.set_data("c1.c2");
    REQUIRE(line.on_clear_line_to_begin() == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_begin in the middle") {
    line.set_data("c1.c2");
    line.on_cursor_left(2);
    REQUIRE(line.on_clear_line_to_begin() == cli::Error::none);
    REQUIRE(line.view() == "c2");
    REQUIRE(d.data == "c2");
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_begin at the beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    REQUIRE(line.on_clear_line_to_begin() == cli::Error::none);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 0);
  }
  SECTION("cursor left") {
    line.set_data("c2.c3");
    line.on_cursor_left(1);
    REQUIRE(d.cursor == 4);
    line.on_cursor_left(10);
    REQUIRE(d.cursor == 0);
  }
  SECTION("cursor right") {
    line.set_data("c2.c3");
    line.on_cursor_left(5);
    REQUIRE(d.cursor == 0);
    line.on_cursor_right(1);
    REQUIRE(d.cursor == 1);
    line.on_cursor_right(10);
    REQUIRE(d.cursor == 5);
  }
}

TEST_CASE("cli::Line<Cursor, Autocomplete>") {
  DEFINE_COMMANDS();
  Display d;
  cli::Line<Cursor_Autocomplete, Display> line(root, d);
  SECTION("on_char cursor at the end") {
    REQUIRE(line.on_char('c') == cli::Error::none);
    REQUIRE(line.on_char('1') == cli::Error::none);
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
    REQUIRE(d.cursor == 2);
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.on_char('c') == cli::Error::none);
    REQUIRE(line.on_char('2') == cli::Error::none);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 5);
  }
  SECTION("on_char cursor in the middle") {
    line.set_data("c1.c2");
    REQUIRE(d.cursor == 5);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");

    REQUIRE(line.on_cursor_right(1) == cli::Error::none);
    REQUIRE(d.cursor == d.data.size());

    REQUIRE(line.on_cursor_left(1) == cli::Error::none);
    REQUIRE(d.cursor == 4);

    REQUIRE(line.on_char('b') == cli::Error::none);
    REQUIRE(line.view() == "c1.cb2");
    REQUIRE(d.data == "c1.cb2");
    REQUIRE(d.cursor == 5);

    REQUIRE(line.on_char('a') == cli::Error::none);
    REQUIRE(line.view() == "c1.cba2");
    REQUIRE(d.data == "c1.cba2");
    REQUIRE(line.on_cursor_left(6) == cli::Error::none);
    REQUIRE(line.on_char('x') == cli::Error::none);
    REQUIRE(line.view() == "xc1.cba2");
    REQUIRE(line.on_cursor_left(1) == cli::Error::none);
    REQUIRE(line.on_cursor_left(5) == cli::Error::none);
    REQUIRE(d.data == "xc1.cba2");
  }
  SECTION("on_char line is full") {
    line.set_data("01234567890123456789012345678901");
    REQUIRE(line.on_char('x') == cli::Error::buffer_overflow);
  }
  SECTION("on_char empty and access_separator") {
    line.clear();
    REQUIRE(line.on_char('.') == cli::Error::none);
    REQUIRE(line.view().size() == 0);
  }
  SECTION("backspace at the end") {
    line.set_data("c1.c2");
    line.on_backspace();
    REQUIRE(line.view() == "c1.c");
    REQUIRE(d.data == "c1.c");

    line.on_backspace(4);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());

    line.on_backspace(4);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
  }
  SECTION("backspace in the middle") {
    line.set_data("c1.c2");
    line.on_cursor_left(1);
    line.on_backspace();
    REQUIRE(d.cursor == 3);
    REQUIRE(line.view() == "c1.2");
    REQUIRE(d.data == "c1.2");

    line.on_backspace(3);
    REQUIRE(line.view() == "2");
    REQUIRE(d.data == "2");
  }
  SECTION("backspace at the beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    line.on_backspace();
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 0);
    line.on_cursor_right(1);
    line.on_backspace(2);
    REQUIRE(line.view() == "1.c2");
    REQUIRE(d.data == "1.c2");
    REQUIRE(d.cursor == 0);
  }
  SECTION("delete_char at the end") {
    line.set_data("c1.c2");
    line.on_delete_char();
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 5);
  }
  SECTION("delete_char in the middle") {
    line.set_data("c1.c2");
    REQUIRE(d.cursor == 5);
    line.on_cursor_left(1);
    line.on_delete_char();
    REQUIRE(line.view() == "c1.c");
    REQUIRE(d.data == "c1.c");
    REQUIRE(d.cursor == 4);
    line.on_cursor_left(2);
    line.on_delete_char();
    line.on_delete_char();
    REQUIRE(line.view() == "c1");
    REQUIRE(d.data == "c1");
    REQUIRE(d.cursor == 2);
    line.set_data("c1.c2");
    line.on_cursor_left(3);
    line.on_delete_char();
    line.on_delete_char();
    REQUIRE(line.view() == "c12");
    REQUIRE(d.data == "c12");
  }
  SECTION("delete_char at the beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    line.on_delete_char();
    REQUIRE(line.view() == "1.c2");
    REQUIRE(d.data == "1.c2");
    REQUIRE(d.cursor == 0);
    for (auto i = 0; i < 10; ++i)
      line.on_delete_char();
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_end at end") {
    line.set_data("c1.c2");
    REQUIRE(line.on_clear_line_to_end() == cli::Error::none);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 5);
  }
  SECTION("clear_line_to_end in middle") {
    line.set_data("c1.c2");
    line.on_cursor_left(2);
    REQUIRE(line.on_clear_line_to_end() == cli::Error::none);
    REQUIRE(line.view() == "c1.");
    REQUIRE(d.data == "c1.");
    REQUIRE(d.cursor == 3);
  }
  SECTION("clear_line_to_end in beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    REQUIRE(line.on_clear_line_to_end() == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_begin at the end") {
    line.set_data("c1.c2");
    REQUIRE(line.on_clear_line_to_begin() == cli::Error::none);
    REQUIRE(line.view().size() == 0);
    REQUIRE(d.data.empty());
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_begin in the middle") {
    line.set_data("c1.c2");
    line.on_cursor_left(2);
    REQUIRE(line.on_clear_line_to_begin() == cli::Error::none);
    REQUIRE(line.view() == "c2");
    REQUIRE(d.data == "c2");
    REQUIRE(d.cursor == 0);
  }
  SECTION("clear_line_to_begin at the beginning") {
    line.set_data("c1.c2");
    line.on_cursor_left(5);
    REQUIRE(line.on_clear_line_to_begin() == cli::Error::none);
    REQUIRE(line.view() == "c1.c2");
    REQUIRE(d.data == "c1.c2");
    REQUIRE(d.cursor == 0);
  }
  SECTION("cursor left") {
    line.set_data("c2.c3");
    line.on_cursor_left(1);
    REQUIRE(d.cursor == 4);
    line.on_cursor_left(10);
    REQUIRE(d.cursor == 0);
  }
  SECTION("cursor right") {
    line.set_data("c2.c3");
    line.on_cursor_left(5);
    REQUIRE(d.cursor == 0);
    line.on_cursor_right(1);
    REQUIRE(d.cursor == 1);
    line.on_cursor_right(10);
    REQUIRE(d.cursor == 5);
  }
  SECTION("autocomplete at end") {
    SECTION("autocomplete with empty line") {
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c1");
      REQUIRE(d.data == "c1");
      REQUIRE(d.cursor == 2);
    }
    SECTION("autocomplete at end of command") {
      line.set_data("c1.c2");
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c1.c2");
      REQUIRE(d.data == "c1.c2");
      REQUIRE(d.cursor == d.data.size());

      line.set_data("c1.c2 args");
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c1.c2 args");
      REQUIRE(d.data == "c1.c2 args");
      REQUIRE(d.cursor == d.data.size());

      line.set_data("c4");
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c4long");
      REQUIRE(d.data == "c4long");
      REQUIRE(d.cursor == d.data.size());

      line.set_data("c4long");
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c4long.");
      REQUIRE(d.data == "c4long.");
      REQUIRE(d.cursor == d.data.size());

      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c4long.c5long");
      REQUIRE(d.data == "c4long.c5long");
      REQUIRE(d.cursor == d.data.size());

      line.set_data("c4 args");
      line.on_cursor_left(5);
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c4long args");
      REQUIRE(d.data == "c4long args");
      REQUIRE(d.cursor == 6);

      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c4long. args");
      REQUIRE(d.data == "c4long. args");
      REQUIRE(d.cursor == 7);

      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c4long.c5long args");
      REQUIRE(d.data == "c4long.c5long args");
      REQUIRE(d.cursor == 13);

      REQUIRE(line.set_data("c1") == cli::Error::none);
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c1");
      REQUIRE(d.data == "c1");
      REQUIRE(d.cursor == d.data.size());

      REQUIRE(line.set_data("c1 args") == cli::Error::none);
      line.on_cursor_left(5);
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "c1 args");
      REQUIRE(d.data == "c1 args");
      REQUIRE(d.cursor == 2);
    }
  }
  SECTION("autocomplete in the middle") {
    SECTION("no args") {
      SECTION("on separator with partial name") {
        line.set_data("c.c3");
        line.on_cursor_left(3);
        line.on_autocomplete();
        REQUIRE(line.view() == "c1.c3");
        REQUIRE(d.data == "c1.c3");
        REQUIRE(d.cursor == 2);

        line.set_data("c4l.");
        line.on_cursor_left(1);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.");
        REQUIRE(d.data == "c4long.");
        REQUIRE(d.cursor == d.data.size() - 1);

        line.set_data("c4long.c");
        line.on_cursor_left(2);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c");
        REQUIRE(d.data == "c4long.c");
        REQUIRE(d.cursor == d.data.size() - 1);
      }
      SECTION("on separator with full name") {
        line.set_data("c2.c3");
        line.on_cursor_left(3);
        REQUIRE(d.cursor == 2);

        line.on_autocomplete();
        REQUIRE(line.view() == "c2.c3");
        REQUIRE(d.data == "c2.c3");
        REQUIRE(d.cursor == 3);

        line.on_autocomplete();
        REQUIRE(line.view() == "c2.c3");
        REQUIRE(d.data == "c2.c3");
        REQUIRE(d.cursor == 5);

        line.set_data("c4long.c5long");
        line.on_cursor_left(7);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long");
        REQUIRE(d.data == "c4long.c5long");
        REQUIRE(d.cursor == 7);

        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long");
        REQUIRE(d.data == "c4long.c5long");
        REQUIRE(d.cursor == d.data.size());
      }
      SECTION("not on separator at beginning") {
        line.set_data("c4long.c5long");
        line.on_cursor_left(13);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long");
        REQUIRE(d.data == "c4long.c5long");
        REQUIRE(d.cursor == 6);
      }
      SECTION("not on separator in middle") {
        line.set_data("c4long.c5long");
        line.on_cursor_left(10);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long");
        REQUIRE(d.data == "c4long.c5long");
        REQUIRE(d.cursor == 6);

        line.set_data("c4.c5long");
        line.on_cursor_left(8);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long");
        REQUIRE(d.data == "c4long.c5long");
        REQUIRE(d.cursor == 6);

        line.set_data("c4.c5long");
        line.on_cursor_left(9);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long");
        REQUIRE(d.data == "c4long.c5long");
        REQUIRE(d.cursor == 6);

        line.set_data("c4l.c5long");
        line.on_cursor_left(9);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long");
        REQUIRE(d.data == "c4long.c5long");
        REQUIRE(d.cursor == 6);
      }
    }
    SECTION("with args") {
      SECTION("no subcommands") {
        line.set_data(" args");
        line.on_cursor_left(5);
        line.on_autocomplete();
        REQUIRE(line.view() == "c1 args");
        REQUIRE(d.data == "c1 args");
        REQUIRE(d.cursor == 2);
      }
      SECTION("on separator with partial name") {
        line.set_data("c.c3 args");
        line.on_cursor_left(8);
        line.on_autocomplete();
        REQUIRE(line.view() == "c1.c3 args");
        REQUIRE(d.data == "c1.c3 args");
        REQUIRE(d.cursor == 2);

        line.set_data("c4l. args");
        line.on_cursor_left(6);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long. args");
        REQUIRE(d.data == "c4long. args");
        REQUIRE(d.cursor == 6);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long. args");
        REQUIRE(d.data == "c4long. args");
        REQUIRE(d.cursor == 7);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long args");
        REQUIRE(d.data == "c4long.c5long args");
        REQUIRE(d.cursor == 13);
      }
      SECTION("on separator with full name") {
        line.set_data("c2.c3 args");
        line.on_cursor_left(8);
        REQUIRE(d.cursor == 2);

        line.on_autocomplete();
        REQUIRE(line.view() == "c2.c3 args");
        REQUIRE(d.data == "c2.c3 args");
        REQUIRE(d.cursor == 3);

        line.on_autocomplete();
        REQUIRE(line.view() == "c2.c3 args");
        REQUIRE(d.data == "c2.c3 args");
        REQUIRE(d.cursor == 5);

        line.set_data("c4long.c5long args");
        line.on_cursor_left(12);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long args");
        REQUIRE(d.data == "c4long.c5long args");
        REQUIRE(d.cursor == 7);
        line.on_autocomplete();
        REQUIRE(line.view() == "c4long.c5long args");
        REQUIRE(d.data == "c4long.c5long args");
        REQUIRE(d.cursor == 13);
      }
    }
  }
  SECTION("autocomplete in args") {
    line.set_data("c2.c3 args");
    REQUIRE(line.on_autocomplete() == cli::Error::none);
    REQUIRE(line.view() == "c2.c3 args");
    REQUIRE(d.data == "c2.c3 args");
  }
  SECTION("autocomplete bogus") {
    SECTION("at the end") {
      line.set_data("bogus");
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "bogus");
      REQUIRE(d.data == "bogus");
    }
    SECTION("at the end with args") {
      line.set_data("bogus args");
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "bogus args");
      REQUIRE(d.data == "bogus args");
    }
    SECTION("in the middle") {
      line.set_data("bogus");
      line.on_cursor_left(2);
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "bogus");
      REQUIRE(d.data == "bogus");
      REQUIRE(d.cursor == 3);
    }
    SECTION("in the middle with args") {
      line.set_data("bogus args");
      line.on_cursor_left(7);
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "bogus args");
      REQUIRE(d.data == "bogus args");
      REQUIRE(d.cursor == 3);
    }
    SECTION("at the beginning") {
      line.set_data("bogus");
      line.on_cursor_left(5);
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "bogus");
      REQUIRE(d.data == "bogus");
      REQUIRE(d.cursor == 0);
    }
    SECTION("at the beginning with args") {
      line.set_data("bogus args");
      line.on_cursor_left(10);
      REQUIRE(line.on_autocomplete() == cli::Error::none);
      REQUIRE(line.view() == "bogus args");
      REQUIRE(d.data == "bogus args");
      REQUIRE(d.cursor == 0);
    }
  }
}

TEST_CASE("cli::Line::set_data") {
  DEFINE_COMMANDS();
  Display d1;
  Display d2;
  Display d3;
  Display d4;
  cli::Line<NoCursor_NoAutocomplete, Display> l1(root, d1);
  cli::Line<NoCursor_Autocomplete, Display> l2(root, d2);
  cli::Line<Cursor_NoAutocomplete, Display> l3(root, d3);
  cli::Line<Cursor_Autocomplete, Display> l4(root, d4);
  SECTION("string is too_big") {
    cli::View<const char> too_big = "01234567890123445678901234567890123456789";
    SECTION("empty line") {
      REQUIRE(l1.set_data(too_big) == cli::Error::buffer_overflow);
      REQUIRE(d1.data.empty());
      REQUIRE(d1.cursor == 0);

      REQUIRE(l2.set_data(too_big) == cli::Error::buffer_overflow);
      REQUIRE(d2.data.empty());
      REQUIRE(d2.cursor == 0);

      REQUIRE(l3.set_data(too_big) == cli::Error::buffer_overflow);
      REQUIRE(d3.data.empty());
      REQUIRE(d3.cursor == 0);

      REQUIRE(l4.set_data(too_big) == cli::Error::buffer_overflow);
      REQUIRE(d4.data.empty());
      REQUIRE(d4.cursor == 0);
    }
    SECTION("aleady has data") {
      SECTION("no cursor and no autocomplete") {
        l1.set_data("c1.c2");
        REQUIRE(l1.set_data(too_big) == cli::Error::buffer_overflow);
        REQUIRE(l1.view() == "c1.c2");
        REQUIRE(d1.data == "c1.c2");
        REQUIRE(d1.data.size() == d1.cursor);
      }
      SECTION("no cursor and autocomplete") {
        l2.set_data("c2.c3");
        REQUIRE(l2.set_data(too_big) == cli::Error::buffer_overflow);
        REQUIRE(l2.view() == "c2.c3");
        REQUIRE(d2.data == "c2.c3");
        REQUIRE(d2.data.size() == d2.cursor);
      }
      SECTION("cursor and no autocomplete") {
        l3.set_data("c1.c2");
        REQUIRE(l3.set_data(too_big) == cli::Error::buffer_overflow);
        REQUIRE(l3.view() == "c1.c2");
        REQUIRE(d3.data == "c1.c2");
        REQUIRE(d3.data.size() == d3.cursor);
      }
      SECTION("cursor and autocomplete") {
        l4.set_data("c1.c2");
        REQUIRE(l4.set_data(too_big) == cli::Error::buffer_overflow);
        REQUIRE(l4.view() == "c1.c2");
        REQUIRE(d4.data == "c1.c2");
        REQUIRE(d4.data.size() == d4.cursor);
      }
    }
  }
  SECTION("string is empty") {
    cli::View<const char> empty;
    REQUIRE(l1.set_data(empty) == cli::Error::none);
    REQUIRE(l1.view().size() == 0);
    REQUIRE(d1.data.empty());
    REQUIRE(d1.cursor == 0);

    REQUIRE(l2.set_data(empty) == cli::Error::none);
    REQUIRE(l2.view().size() == 0);
    REQUIRE(d2.data.empty());
    REQUIRE(d2.cursor == 0);

    REQUIRE(l3.set_data(empty) == cli::Error::none);
    REQUIRE(l3.view().size() == 0);
    REQUIRE(d3.data.empty());
    REQUIRE(d3.cursor == 0);

    REQUIRE(l4.set_data(empty) == cli::Error::none);
    REQUIRE(l4.view().size() == 0);
    REQUIRE(d4.data.empty());
    REQUIRE(d4.cursor == 0);
  }

  SECTION("invalid command") {
    cli::View<const char> invalid = "c1.invalid";
    SECTION("no cursor and no autocomplete doesn't check validity") {
      REQUIRE(l1.set_data(invalid) == cli::Error::none);
      REQUIRE(d1.data == "c1.invalid");
      REQUIRE(d1.data.size() == d1.cursor);
    }
    SECTION("no cursor and autocomplete checks validity") {
      REQUIRE(l2.set_data(invalid) == cli::Error::invalid_cmd);
      REQUIRE(l2.view().size() == 0);
      REQUIRE(d2.data.empty());
      REQUIRE(d2.cursor == 0);
    }
    SECTION("cursor and no autocomplete doesn't check validity") {
      REQUIRE(l3.set_data(invalid) == cli::Error::none);
      REQUIRE(d3.data == "c1.invalid");
      REQUIRE(d3.data.size() == d3.cursor);
    }
    SECTION("cursor and autocomplete doesn't check validity") {
      REQUIRE(l4.set_data(invalid) == cli::Error::none);
      REQUIRE(d4.data == "c1.invalid");
      REQUIRE(d4.data.size() == d4.cursor);
    }
  }
}
