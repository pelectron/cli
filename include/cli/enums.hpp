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
    too_many_splits,
    dual_separators,
    buffer_overflow,      //< a buffer would overflow
    buffer_underflow,     //< a buffer would underflow
    incorrect_num_params, //< an incorrect number of parameters/arguments has
                          //< been provided
    too_many_argments,    //< too many arguments have been provided
    too_few_arguments,    //< too few arguments have been provided
    invalid_esc_seq,      //< an invalid escape sequence has been encountered
    invalid_state,        //< an invalid sate has been encountered
    expected_value,       //< expected a value but none has been given
    unexpected_characters_after_closing_paren, //< characters after closing
                                               //< parentheses
    expected_rparen,                           //< expected closing parentheses
    expected_lparen,
    //
    // parse errors
    too_few_characters,    //< expected more characters
    unexpected_characters, //< unexpected or too many characters
    invalid_character,     //< encountered invalid character
    too_many_sequence_values,
    too_few_sequence_values,
    invalid_sequence_value,
    expected_open_bracket,
    expected_closing_bracket,
    expected_delimiter,
    expected_group_opening,
    expected_group_closing,
    expected_another_field,
    expected_field,
    expected_struct_value,
    expected_assignment,
    expected_endquote,

    // validation errors
    invalid_value, //< encountered an invalid value
    out_of_range,
    value_too_small,
    value_too_large,
    unknown //< unkown error
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
