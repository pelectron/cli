#ifndef CLI_TEST_DISPLAY_HPP
#define CLI_TEST_DISPLAY_HPP

#include "cli/string.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct Display {
  std::size_t cursor = 0;
  std::string data{};
  std::vector<std::string> past{};

  void write(char c);
  void write(cli::View<const char> s);
  void backspace(std::size_t n);
  void clear_line();
  void clear_screen();
  void newline();
  void delete_char();
  void cursor_left(std::size_t n);
  void cursor_right(std::size_t n);
};

struct MultilineDisplay : Display {
  static constexpr bool is_multiline_display = true;
};

#endif
