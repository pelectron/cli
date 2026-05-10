/**
 * @file cli/enums.hpp
 * This file defines core enumerations of cli
 *
 * @defgroup enumerations Enumerations
 * This group contains all enumerations used by cli.
 */

#ifndef CLI_ENUMS_HPP
#define CLI_ENUMS_HPP

#include <cstdint>

namespace cli {

  /**
   * This enumerates every possible error occuring in cli
   * @ingroup enumerations
   */
  enum class Error : std::uint32_t {
    none,                 //< no error
    unimplemented,        //< functionality is not implemented
    implementation_error, //< error in the implementation
    io_error,             //< error during IO
    invalid_argument,     //< invalid argument
    cant_set_param,       //< cant set a parameter
    cant_read_param,      //< cant read a parameter
    invalid_cmd,          //< an invalid command has been entered
    buffer_overflow,      //< a buffer would overflow
    too_few_arguments,    //< too few arguments have been provided
    expected_value,       //< expected a value but none has been given

    // parse errors
    too_few_characters,    //< expected more characters
    unexpected_characters, //< unexpected or too many characters
    invalid_character,     //< encountered invalid character
    too_many_sequence_values,
    too_few_sequence_values,
    invalid_sequence_value,
    expected_group_opening,
    expected_group_closing,
    expected_assignment,
    expected_delimiter,
    expected_endquote,
    expected_rparen, //< expected closing parentheses
    expected_lparen,
    expected_lbrace,
    expected_rbrace,
    expected_lbracket,
    expected_rbracket,
    expected_another_field,
    expected_another_arg,
    expected_field,
    expected_args,
    unexpected_characters_after_closing_paren,

    // validation errors
    invalid_value, //< encountered an invalid value
    unknown        //< unkown error
  };

  /**
   * This enumerates the possible delimiters of the input system
   * @ingroup enumerations
   */
  enum class Delimiter {
    lf,  //< line feed, i.e. '\n'
    cr,  //< carriage return, i.e. '\r'
    crlf //< carriage return and linefeed, i.e. "\r\n"
  };

  /**
   * This enumerates the possible options for formatting and parsing integers
   * and characters. Can be or'ed for multiple options.
   * @ingroup enumerations
   */
  enum class Fmt {
    normal = 1 << 0, //< default format
    hex = 1 << 1,    //< hex format, i.e. 0x or 0X
    binary = 1 << 2  //< binary format, i.e. 0b or 0B
  };

  /**
   * combines two format options
   *
   * @ingroup enumerations
   * @param a a format option
   * @param b a format option
   * @return the combination of a and b
   */
  constexpr Fmt operator|(Fmt a, Fmt b) noexcept {
    return static_cast<Fmt>(static_cast<unsigned>(a) |
                            static_cast<unsigned>(b));
  }

  /**
   * intersects two format options
   *
   * @ingroup enumerations
   * @param a a format option
   * @param b a format option
   * @return the intersection of a and b
   */
  constexpr Fmt operator&(Fmt a, Fmt b) noexcept {
    return static_cast<Fmt>(static_cast<unsigned>(a) &
                            static_cast<unsigned>(b));
  }

} // namespace cli

#endif
