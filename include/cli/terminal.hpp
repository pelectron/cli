#ifndef CLI_TERMINAL_HPP
#define CLI_TERMINAL_HPP

#include "cli/util.hpp"
namespace cli {
/**
 * The terminal represents the output device, i.e. the actual screen the cli is
 * run on.
 */
class Terminal {
public:
  /// write a character to the terminal. This may be buffered.
  Error write(uint8_t c);
  // write a string to the terminal. This may be buffered.
  Error write(ByteView s);
  /// flush the terminal, i.e. write the internal buffer.
  Error flush();
  /// delete the last n characters
  Error backspace(unsigned n);
  Error cursor_up(unsigned n);
  Error cursor_down(unsigned n);
  Error cursor_left(unsigned n);
  Error cursor_right(unsigned n);
};
} // namespace cli
#endif
