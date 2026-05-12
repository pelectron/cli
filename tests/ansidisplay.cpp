#include "cli/display.hpp"

#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("Ansidisplay") {
  std::string output{};
  auto char_output = [&output](char c) { output.push_back(c); };

  auto string_output = [&output](cli::View<const char> str) {
    for (char c : str)
      output.push_back(c);
  };

  static_assert(cli::concepts::CharOutput<decltype(char_output), char>);
  static_assert(cli::concepts::StringOutput<decltype(string_output), char>);

  SECTION("write char") {
    cli::AnsiDisplay d1{char_output};
    d1.write('c');
    REQUIRE(output == "c");

    output.clear();
    cli::AnsiDisplay d2{string_output};
    d2.write('c');
    REQUIRE(output == "c");
  }

  SECTION("write string") {
    cli::AnsiDisplay d1{char_output};
    d1.write(cli::View{"hello"});
    REQUIRE(output == "hello");

    output.clear();
    cli::AnsiDisplay d2{string_output};
    d2.write(cli::View{"world"});
    REQUIRE(output == "world");
  }

  SECTION("backspace") {
    cli::AnsiDisplay d{char_output};
    d.backspace(2);
    REQUIRE(output == "\b \b\b \b");
  }

  SECTION("clear line") {
    cli::AnsiDisplay d{char_output};
    d.clear_line();
    REQUIRE(output == "\x1B[2K\x1B[1G");
  }

  SECTION("clear screen") {
    cli::AnsiDisplay d{char_output};
    d.clear_screen();
    REQUIRE(output == "\x1B[2J\x1B[H");
  }

  SECTION("newline") {
    cli::AnsiDisplay d{char_output};
    d.newline();
    REQUIRE(output == "\n");
  }

  SECTION("cursor_left") {
    cli::AnsiDisplay d{char_output};
    d.cursor_left(0);
    REQUIRE(output.empty());
    d.cursor_left(1);
    REQUIRE(output == "\x1B[1D");
  }

  SECTION("cursor_right") {
    cli::AnsiDisplay d{char_output};
    d.cursor_right(0);
    REQUIRE(output.empty());
    d.cursor_right(1);
    REQUIRE(output == "\x1B[1C");
  }
}
