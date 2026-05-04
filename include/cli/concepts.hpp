#ifndef CLI_CONCEPTS_HPP
#define CLI_CONCEPTS_HPP

#include "cli/enums.hpp"
#include "cli/event.hpp"
#include "cli/string.hpp"
#include <concepts>
#include <type_traits>

namespace cli {

  /**
   * @brief This concept denotes a cli::string_constant
   *
   * @tparam T
   */
  template<class T>
  concept SC = is_string_constant_v<std::remove_cvref_t<T>>;

  template<typename T>
  concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

  namespace concepts {

    template<class C>
    concept Command =
      requires(std::remove_cvref_t<C> &c,
               View<const typename std::remove_cvref_t<C>::char_type> args,
               View<typename std::remove_cvref_t<C>::char_type> &out,
               bool &should_print_newline) {
        { typename std::remove_cvref_t<C>::sub_command_list{} };
        { std::remove_cvref_t<C>::name } -> SC;
        { std::remove_cvref_t<C>::description } -> SC;
        { c.execute(args, out, should_print_newline) } -> std::same_as<Error>;
      };

    /**
     * @brief The Input concept. See cli::Input for an example.
     *
     * @ingroup Input
     * @tparam I the input type
     * @tparam CharT the character type of the input type
     */
    template<typename I, typename CharT>
    concept Input =
      std::is_constructible_v<I> and
      requires(I input, CharT character, cli::Event<CharT> &event) {
        /// on_char is called by the engine when a character is received.
        { input.on_char(character) } -> std::same_as<cli::Error>;

        /// pop_event is called by the engine to process an event.
        { input.pop_event(event) } -> std::convertible_to<bool>;

        /// reset resets the input to its default state.
        { input.reset() } -> std::same_as<void>;
      };

    /**
     * @brief This concept denotes a Display without cursor.
     *
     * @ingroup Output
     * @tparam D
     * @tparam CharT
     */
    template<typename D, typename CharT>
    concept DisplayWithoutCursor = requires(
      D d, CharT character, cli::View<const CharT> string, std::size_t n) {
      { d.write(character) } -> std::same_as<cli::Error>;
      { d.write(string) } -> std::same_as<cli::Error>;
      { d.backspace(n) } -> std::same_as<cli::Error>;
      { d.clear_line() } -> std::same_as<cli::Error>;
      { d.clear_screen() } -> std::same_as<cli::Error>;
      { d.newline() } -> std::same_as<cli::Error>;
    };

    /**
     * @brief
     *
     * @ingroup Output
     * @tparam D
     * @tparam CharT
     */
    template<typename D, typename CharT>
    concept DisplayWithCursor =
      DisplayWithoutCursor<D, CharT> and
      requires(D d, CharT character, View<const CharT> string, std::size_t n) {
        { d.cursor_left(n) } -> std::same_as<cli::Error>;
        { d.cursor_right(n) } -> std::same_as<cli::Error>;
      };

    /**
     * @brief A Display is used by cli::Cli to output/display characters.
     *
     * This can be a display with cursor or a display without cursor.
     *
     * @ingroup Output
     * @tparam D
     * @tparam CharT
     */
    template<typename D, typename CharT>
    concept Display =
      DisplayWithoutCursor<D, CharT> or DisplayWithCursor<D, CharT>;

    /**
     * A CharOutput is used to write a single character to an unbuffered
     * output Output.
     *
     * It is a callable that takes a character as input and returns a
     * cli::Error to indicate write success or failure.
     *
     * Example for an embedded target with UART:
     *
     * ```
     * cli::Error char_output(char c){
     *   HAL_Status status = HAL_UART_Transmit(c);
     *   return status_to_cli_err(status);
     * }
     *
     * static_assert(cli::CharOutput<decltype(&char_output), char>);
     * ```
     *
     * @ingroup Output
     * @tparam S the Output type
     * @tparam CharT the character type
     */
    template<class O, typename CharT>
    concept CharOutput = requires(std::decay_t<O> output, CharT c) {
      { output(c) } -> std::same_as<cli::Error>;
    };

    /**
     * A StringOutput is used to write a string of characters to an
     * unbuffered output Output.
     *
     * It is a callable that takes a cli::View<const CharT> as input
     * and returns a cli::Error to indicate write success or failure.
     *
     * Example for an embedded target with UART:
     *
     * ```
     * cli::Error my_string_output(cli::View<const char> string){
     *   for(const char& ch: string){
     *     HAL_Status status = HAL_UART_Transmit(ch);
     *     if(status != HAL_STAUS_OK)
     *       return status_to_cli_err(status);
     *   }
     *   return cli::Error::none;
     * }
     *
     * static_assert(cli::StringOutput<decltype(&my_string_output), char>);
     * ```
     *
     * @ingroup Output
     * @tparam S the Output type
     * @tparam CharT the character type of the Output
     */
    template<class O, typename CharT>
    concept StringOutput =
      requires(std::decay_t<O> output, cli::View<const CharT> s) {
        { output(s) } -> std::same_as<cli::Error>;
      };

    /**
     * A BasicOutput is used to write raw characters to an unbuffered
     * output stream. A BasicOutput must satisfy the CharOutput or the
     * StringOutput concept, or both.
     *
     * @ingroup Output
     * @tparam S the Output type
     * @tparam char_type the character type
     */
    template<class O, typename CharT>
    concept BasicOutput = CharOutput<O, CharT> or StringOutput<O, CharT>;

    /**
     * The Output concept denotes a StringOutput or a CharOutput. It is
     * used by AnsiDisplay to write characters.
     *
     * @ingroup Output
     * @tparam S
     */
    template<class O>
    concept Output = BasicOutput<O, char> or BasicOutput<O, unsigned char> or
                     BasicOutput<O, signed char> or BasicOutput<O, char8_t> or
                     BasicOutput<O, char16_t> or BasicOutput<O, char32_t>;

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

      /** if true, the cli uses autocomplete */
      { std::remove_cvref_t<T>::use_autocomplete } -> std::convertible_to<bool>;

      /** if true, the cli recognizes cursor movement. If true, your display
       * must support this. */
      { std::remove_cvref_t<T>::use_cursor } -> std::convertible_to<bool>;

      /** if true, the cli uses a volatile input buffer. This should be true if
       * Engine::on_char is called in an ISR
       */
      // {
      //   std::remove_cvref_t<T>::use_volatile_input_buffer
      // } -> std::convertible_to<bool>;

      /** if true, then a history of commands is available through the up and
       * down cursors */
      { std::remove_cvref_t<T>::use_history } -> std::convertible_to<bool>;

      /** if use_history is true, this will specify how many commands can be
       * recalled in the history */
      // {
      //   std::remove_cvref_t<T>::history_depth
      // } -> std::convertible_to<std::size_t>;

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

  } // namespace concepts
} // namespace cli
#endif
