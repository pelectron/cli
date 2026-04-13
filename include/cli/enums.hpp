#ifndef CLI_ENUMS_HPP
#define CLI_ENUMS_HPP

#include <cstdint>

namespace cli {

enum class Error : std::uint32_t {
  none,
  unimplemented,
  implementation_error,
  io_error,
  invalid_argument,
  cant_set_param,
  cant_read_param,
  invalid_cmd,
  too_many_splits,
  dual_separators,
  buffer_overflow,
  buffer_underflow,
  incorrect_num_params,
  too_many_argments,
  too_few_arguments,
  invalid_esc_seq,
  invalid_state,
  expected_value,
  unexpected_characters_after_closing_paren,
  expected_rparen,
  // parse errors
  too_few_characters,
  invalid_character,
  unescaped_string,
  invalid_value,

  unknown
};

enum class ExecType : std::uint8_t { none, get, set, call };

enum class Delimiter {
  lf,  //< line feed, i.e. '\n'
  cr,  //< carriage return, i.e. '\r'
  crlf //< carriage return and linefeed, i.e. "\r\n"
};

enum class Control {
  bell,
  backspace,
  autocomplete,
  cursor_up,
  cursor_down,
  cursor_left,
  cursor_right,
  delete_char,
  clear,
  enter
};

namespace ansi {
enum ASCII_Codes : std::uint8_t {
  BEL = 0x07,
  BS = 0x08,
  HT = 0x09,
  LF = 0x0A,
  VT = 0x0B,
  FF = 0x0C,
  CR = 0x0D,
  ESC = 0x1B,
  DEL = 0x7F
};
/**
 * Wikipedia: If the ESC is followed by a byte in the range 0x40 to 0x5F, the
 * escape sequence is of type Fe. Its interpretation is delegated to the
 * applicable C1 control code standard
 */
enum C1 {
  SS2 = 0x8E,
  SS3 = 0x8F,
  DCS = 0x90,
  CSI = 0x9B,
  ST = 0x9C,
  OSC = 0x9D,
  SOS = 0x98,
  PM = 0x9E,
  APC = 0x9F
};
} // namespace ansi

enum class Fmt { normal = 1 << 0, hex = 1 << 1, binary = 1 << 2 };

constexpr Fmt operator|(Fmt a, Fmt b) noexcept {
  return static_cast<Fmt>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr Fmt operator&(Fmt a, Fmt b) noexcept {
  return static_cast<Fmt>(static_cast<unsigned>(a) & static_cast<unsigned>(b));
}

} // namespace cli

#endif
