#ifndef CLI_CONCEPTS_HPP
#define CLI_CONCEPTS_HPP

#include "cli/enums.hpp"
#include "cli/event.hpp"
#include "cli/string.hpp"
#include "cli/traits.hpp"

#include <concepts>
#include <type_traits>

namespace cli {

  template<typename CharT>
  class ExecResult;

  /**
   * @brief This concept is true if T is a cli::string_constant
   *
   * @tparam T
   */
  template<class T>
  concept SC = cli::dtl::is_string_constant_v<std::remove_cvref_t<T>>;

  namespace concepts {

    template<class C>
    concept Command =
      requires(std::remove_cvref_t<C> &c,
               View<const typename std::remove_cvref_t<C>::char_type> args,
               View<typename std::remove_cvref_t<C>::char_type> out) {
        { typename std::remove_cvref_t<C>::sub_command_list{} };
        { std::remove_cvref_t<C>::name } -> SC;
        { std::remove_cvref_t<C>::description } -> SC;
        {
          c.execute(args, out)
        }
        -> std::same_as<ExecResult<typename std::remove_cvref_t<C>::char_type>>;
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
     * @ingroup Display
     * @tparam D
     * @tparam CharT
     */
    template<typename D, typename CharT>
    concept DisplayWithoutCursor = requires(
      D d, CharT character, cli::View<const CharT> string, std::size_t n) {
      { d.write(character) } -> std::same_as<void>;
      { d.write(string) } -> std::same_as<void>;
      { d.backspace(n) } -> std::same_as<void>;
      { d.clear_line() } -> std::same_as<void>;
      { d.clear_screen() } -> std::same_as<void>;
      { d.newline() } -> std::same_as<void>;
    };

    /**
     * @brief
     *
     * @ingroup Display
     * @tparam D
     * @tparam CharT
     */
    template<typename D, typename CharT>
    concept DisplayWithCursor =
      DisplayWithoutCursor<D, CharT> and
      requires(D d, CharT character, View<const CharT> string, std::size_t n) {
        { d.cursor_left(n) } -> std::same_as<void>;
        { d.cursor_right(n) } -> std::same_as<void>;
      };

    /**
     * @brief A Display is used by cli::Cli to output/display characters.
     *
     * This can be a display with cursor or a display without cursor.
     *
     * @ingroup Display
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
     * @ingroup Display
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
     * @ingroup Display
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
     * @ingroup Display
     * @tparam S the Output type
     * @tparam char_type the character type
     */
    template<class O, typename CharT>
    concept BasicOutput = CharOutput<O, CharT> or StringOutput<O, CharT>;

    /**
     * The Output concept denotes a StringOutput or a CharOutput. It is
     * used by AnsiDisplay to write characters.
     *
     * @ingroup Display
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
     * For a detailed explanation, see [here](docs.md#config).
     *
     * @sa cli::default_config
     *
     * @ingroup config
     * @tparam T the type to check
     */
    template<typename T>
    concept Config = requires(typename T::char_type) {
      /** the character that separates commands, for example '.' */
      {
        T::access_separator
      } -> std::convertible_to<typename std::remove_cvref_t<T>::char_type>;

      /** if true, the cli uses autocomplete */
      { T::use_autocomplete } -> std::convertible_to<bool>;

      /** if true, the cli recognizes cursor movement. If true, your display
       * must support this. */
      { T::use_cursor } -> std::convertible_to<bool>;

      /** if true, then a history of commands is available through the up and
       * down cursors */
      { T::use_history } -> std::convertible_to<bool>;

      { T::max_line_length } -> std::convertible_to<std::size_t>;

      {
        T::name
      } -> std::convertible_to<
        View<const typename std::remove_cvref_t<T>::char_type>>;

      {
        T::description
      } -> std::convertible_to<
        View<const typename std::remove_cvref_t<T>::char_type>>;
    };

    /**
     * The character concept. It is satified if T is one of char, unsigned char,
     * signed char, char8_t, char16_t, or char32_t.
     *
     * @tparam T the type to check
     */
    template<typename T>
    concept Character =
      cli::traits::is_char<T>::value or std::same_as<T, char> or
      std::same_as<T, unsigned char> or std::same_as<T, signed char> or
      std::same_as<T, char8_t> or std::same_as<T, char16_t> or
      std::same_as<T, char32_t>;

    /**
     * @brief The integer concept is true is the is_integer traits is true and T
     * is not a Character and T is not bool.
     *
     * @tparam T
     */
    template<class T>
    concept Integer =
      cli::traits::is_integer<T>::value or
      ((not Character<T>) and std::integral<T> and not std::same_as<T, bool>);

    /**
     * @brief The fixpoint concept.
     *
     * A fixpoint number type T must:
     * - have an inner typedef called raw_value_type, which is the underlying
     *   representation of the fixpoint type. The raw_value_type must be
     * integral.
     * - be constructible from the raw_value_type
     * - have static constants num_int_digits: the number of integer digits
     * - have static constants num_frac_digits: the number of fractional digits
     * - have the method fraction(): returns the fractional part of T
     * - have the method integer(): returns the integer part of T
     * - have the method value(): which returns the value of T as a
     * raw_value_type
     * - be less than comparable
     * - be negatable
     * - be assignable
     *
     * In addition, cli::traits::is_fixpoint must be specialized for T to
     * satisfy the FixPoint concept.
     *
     * @tparam T the type to check
     */
    template<class T>
    concept FixPoint =
      cli::traits::is_fixpoint<T>::value and
      std::constructible_from<T, typename T::raw_value_type> and
      std::integral<typename T::raw_value_type> and requires(T a, T b) {
        { T::num_int_digits } -> std::convertible_to<std::size_t>;
        { T::num_frac_digits } -> std::convertible_to<std::size_t>;
        { a.integer() } -> std::convertible_to<typename T::raw_value_type>;
        { a.fraction() } -> std::convertible_to<typename T::raw_value_type>;
        { a.value() } -> std::convertible_to<typename T::raw_value_type>;
        { a < b };
        { -a };
        { a = b };
      };

    /**
     * @brief The floating point concept.
     *
     * @tparam T the type to check
     */
    template<class T>
    concept Float = std::floating_point<T> or cli::traits::is_float<T>::value;

    /**
     * @brief This concept denotes a string view, i.e. a non owning string.
     *
     * To "enable" this concept, you must specialize cli::traits::is_string_view
     * and inherit that specialization from std::true_type.
     *
     * @tparam T the type to check
     */
    template<class T>
    concept StringView =
      cli::traits::is_string_view<T>::value and
      std::integral<typename T::value_type> and
      std::
        constructible_from<T, const typename T::value_type *, std::size_t> and
      requires(T &&t, std::size_t i) {
        { T{} };
        { t.size() } -> std::convertible_to<std::size_t>;
        { t.begin() } -> std::forward_iterator;
        { t.end() } -> std::forward_iterator;
        { t[i] } -> std::convertible_to<const typename T::value_type &>;
      };

    /**
     * @brief This concept denotes a string.
     *
     * To "enable" this concept, you must specialize cli::traits::is_string and
     * inherit that specialization from std::true_type.
     *
     * @tparam T
     */
    template<class T>
    concept String = cli::traits::is_string<T>::value and
                     std::integral<typename T::value_type> and
                     std::constructible_from<T,
                                             const typename T::value_type *,
                                             std::size_t> and
                     requires(T &&t, typename T::value_type c, std::size_t i) {
                       { T{} };
                       { t.size() } -> std::convertible_to<std::size_t>;
                       { t.begin() } -> std::forward_iterator;
                       { t.end() } -> std::forward_iterator;
                       {
                         t[i]
                       } -> std::convertible_to<const typename T::value_type &>;
                       { t.push_back(c) };
                     };

    /**
     * @brief This concept denotes a list of values.
     *
     * T contains a variable amount of elements of type T::value_type, for
     * example cli::FixedCapacityVector. To "enable"" this concept, you must
     * specialize cli::traits::is_sequence and inherit that specialization from
     * std::true_type.
     *
     * @tparam T
     */
    template<class T>
    concept Sequence =
      cli::traits::is_sequence<T>::value and std::copy_constructible<T> and
      requires(T a, typename T::value_type value) {
        { T{} };
        { a.begin() };
        { a.end() };
        { a.max_size() } -> std::convertible_to<std::size_t>;
        { a.push_back(value) };
      };

    /**
     * @brief This concept denotes a list of values. T contains a fixed amount
     * of elements of type T::value_type, for example std::array. To "enable""
     * this concept, you must specialize cli::traits::is_fixed_size_sequence and
     * inherit that specialization from std::true_type.
     *
     * @tparam T
     */
    template<class T>
    concept FixedSizeSequence =
      cli::traits::is_fixed_size_sequence<T>::value and
      std::copy_constructible<T> and
      requires(T a, std::size_t i, typename T::value_type value) {
        { T{} };
        { a.begin() };
        { a.end() };
        { a.size() } -> std::convertible_to<std::size_t>;
        { a[i] = value };
      };

    /**
     * @brief This concept denotes a "struct".
     *
     * It would be more appropriate to say that this represents an aggregate,
     * i.e. something that can be decomposed into structuerd bindings.
     *
     * @tparam T
     */
    template<class T>
    concept Struct = cli::traits::is_struct<T>::value and
                     not(Sequence<T> or FixedSizeSequence<T>);

    /**
     * @brief The enum concept
     *
     * @tparam T
     */
    template<class T>
    concept Enum = cli::traits::is_enum<T>::value;
  } // namespace concepts
} // namespace cli
#endif
