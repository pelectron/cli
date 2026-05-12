#include "test_display.hpp"

void Display::write(char c) {
  if (cursor == data.size()) {
    data.push_back(c);
    ++cursor;
  } else {
    data.insert(data.begin() + cursor, c);
    ++cursor;
    data.erase(data.begin() + cursor);
  }
}

void Display::write(cli::View<const char> s) {
  for (const char &ch : s) {
    data.insert(data.begin() + cursor, ch);
    ++cursor;
  }
  if (cursor != data.size())
    data.erase(cursor, s.size());
}

void Display::backspace(std::size_t n) {
  data.erase(data.begin() + (n >= cursor ? 0 : cursor - n),
             data.begin() + cursor);
  if (n >= cursor)
    cursor = 0;
  else
    cursor -= n;
}

void Display::clear_line() {
  data.clear();
  cursor = 0;
}

void Display::clear_screen() {
  data.clear();
  cursor = 0;
}

void Display::newline() {
  past.push_back(std::move(data));
  cursor = 0;
  data.clear();
}

void Display::delete_char() { data.erase(data.begin() + cursor); }

void Display::cursor_left(std::size_t n) {
  if (n >= cursor)
    n = cursor;
  cursor -= n;
}

void Display::cursor_right(std::size_t n) {
  if (n + cursor > data.size())
    n = data.size() - cursor;
  cursor += n;
}
