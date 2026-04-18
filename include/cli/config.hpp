#ifndef CLI_CONFIG_HPP
#define CLI_CONFIG_HPP

#include "cli/enums.hpp"
#include "cli/string.hpp"

#include <type_traits>

namespace cli {

  /**
   * The Config is used by cli to configure its relevant parts. It is a type
   * trait like structure.
   *
   * @sa cli::default_config
   *
   * @tparam T
   */
  template<typename T>
  concept Config = requires(typename std::remove_cvref_t<T>::char_type) {
    /** the character that separates commands, for example '.' */
    {
      std::remove_cvref_t<T>::access_separator
    } -> std::convertible_to<typename std::remove_cvref_t<T>::char_type>;

    /** if true, commands must start with the command separator */
    {
      std::remove_cvref_t<T>::commands_start_with_separators
    } -> std::convertible_to<bool>;

    /** if true, the cli uses autocomplete */
    { std::remove_cvref_t<T>::use_autocomplete } -> std::convertible_to<bool>;

    /** if true, the cli uses a volatile input buffer. This should be true if
     * Cli::on_char is called in an ISR
     */
    {
      std::remove_cvref_t<T>::use_volatile_input_buffer
    } -> std::convertible_to<bool>;

    /** the output buffer size */
    { std::remove_cvref_t<T>::tx_size } -> std::convertible_to<std::size_t>;

    /** the event buffer size */
    { std::remove_cvref_t<T>::rx_size } -> std::convertible_to<std::size_t>;

    /** the comand input delimiter, i.e. the "enter key" */
    {
      std::remove_cvref_t<T>::input_delimiter
    } -> std::convertible_to<Delimiter>;

    /** the comand output delimiter, i.e. the newline */
    {
      std::remove_cvref_t<T>::output_delimiter
    } -> std::convertible_to<Delimiter>;

    /** if true, then a history of commands is available through the up and down
     * cursors */
    { std::remove_cvref_t<T>::use_history } -> std::convertible_to<bool>;

    /** if use_history is true, this will specify how many commands can be
     * recalled in the history */
    {
      std::remove_cvref_t<T>::history_depth
    } -> std::convertible_to<std::size_t>;

    {
      std::remove_cvref_t<T>::max_line_length
    } -> std::convertible_to<std::size_t>;

    {
      std::remove_cvref_t<T>::name
    } -> std::convertible_to<
      View<const typename std::remove_cvref_t<T>::char_type>>;
    {
      std::remove_cvref_t<T>::description
    } -> std::convertible_to<
      View<const typename std::remove_cvref_t<T>::char_type>>;
  };

  struct default_config {
    using char_type = char;
    static constexpr View<const char> name = "cli";
    static constexpr View<const char> description = "a command line interface";
    static constexpr char_type access_separator = '.';
    static constexpr auto command_terminator{"\n"_sc};
    static constexpr bool commands_start_with_separators = false;
    static constexpr std::size_t tx_size = 128;
    static constexpr std::size_t rx_size = 32;
    static constexpr bool use_autocomplete = true;
    static constexpr bool use_volatile_input_buffer = false;
    static constexpr bool use_history = true;
    static constexpr std::size_t history_depth = 16;
    static constexpr std::size_t max_line_length = 80;
    static constexpr Delimiter input_delimiter = Delimiter::lf;
    static constexpr Delimiter output_delimiter = Delimiter::lf;
  };
} // namespace cli

#endif
