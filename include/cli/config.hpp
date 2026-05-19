/**
 * @defgroup config Config
 *
 * The Config is used by the engine.
 *
 * See [here](docs.md#config) for a more detailed explanation.
 */

#ifndef CLI_CONFIG_HPP
#define CLI_CONFIG_HPP

#include "cli/concepts.hpp"
#include "cli/enums.hpp"
#include "cli/string.hpp"

#include <type_traits>

namespace cli {

  template<concepts::Config Cfg>
  class Input;

  namespace config {
    namespace dtl {
      template<concepts::Config C, typename = void>
      struct use_volatile_input_buffer : std::false_type {};

      template<concepts::Config C>
      struct use_volatile_input_buffer<C,
                                       std::enable_if_t<std::is_convertible_v<
                                         decltype(C::use_volatile_input_buffer),
                                         bool>>> {
        static constexpr bool value = C::use_volatile_input_buffer;
      };

      template<concepts::Config C, typename = void>
      struct input_delimiter {
        static constexpr cli::Delimiter value = cli::Delimiter::lf;
      };

      template<concepts::Config C>
      struct input_delimiter<
        C,
        std::enable_if_t<
          std::convertible_to<decltype(C::input_delimiter), cli::Delimiter>>> {
        static constexpr cli::Delimiter value = C::input_delimiter;
      };

      template<concepts::Config C, typename = void>
      struct input_size {
        static constexpr std::size_t value = 16;
      };

      template<concepts::Config C>
      struct input_size<
        C,
        std::enable_if_t<
          std::convertible_to<decltype(C::input_size), std::size_t>>> {
        static constexpr std::size_t value = C::input_size;
      };

      template<concepts::Config C, typename = void>
      struct output_size {
        static constexpr std::size_t value = C::max_line_length;
      };

      template<concepts::Config C>
      struct output_size<
        C,
        std::enable_if_t<
          std::convertible_to<decltype(C::output_size), std::size_t>>> {
        static constexpr std::size_t value = C::output_size;
      };

      template<typename C>
      concept has_input_template = requires {
        { typename C::template input_type<C>{} };
      };

      template<typename C>
      concept has_input_type = requires {
        { typename C::input_type{} };
      };

      template<concepts::Config C>
      struct input_type {
        using type = cli::Input<C>;
      };

      template<concepts::Config C>
        requires has_input_type<C>
      struct input_type<C> {
        using type = typename C::input_type;
      };

      template<concepts::Config C>
        requires has_input_template<C>
      struct input_type<C> {
        using type = typename C::template input_type<C>;
      };

      template<concepts::Config C, typename = void>
      struct char_type {
        using type = char;
      };

      template<concepts::Config C>
      struct char_type<C, std::void_t<typename C::char_type>> {
        using type = typename C::char_type;
      };

      template<concepts::Config C, typename = void>
      struct access_separator {
        static constexpr typename char_type<C>::type value = '.';
      };

      template<concepts::Config C>
      struct access_separator<
        C,
        std::enable_if_t<std::is_convertible_v<decltype(C::access_separator),
                                               typename char_type<C>::type>>> {
        static constexpr typename char_type<C>::type value =
          C::access_separator;
      };

      template<concepts::Config C, typename = void>
      struct empty_help_prints_commands : std::false_type {};

      template<concepts::Config C>
      struct empty_help_prints_commands<
        C,
        std::enable_if_t<
          std::is_convertible_v<decltype(C::empty_help_prints_commands),
                                bool>>> {
        static constexpr bool value = C::empty_help_prints_commands;
      };

      template<concepts::Config C, typename = void>
      struct use_autocomplete : std::false_type {};

      template<concepts::Config C>
      struct use_autocomplete<
        C,
        std::enable_if_t<
          std::is_convertible_v<decltype(C::use_autocomplete), bool>>> {
        static constexpr bool value = C::use_autocomplete;
      };

      template<concepts::Config C, typename = void>
      struct use_cursor : std::false_type {};

      template<concepts::Config C>
      struct use_cursor<
        C,
        std::enable_if_t<
          std::is_convertible_v<decltype(C::use_cursor), bool>>> {
        static constexpr bool value = C::use_cursor;
      };

      template<concepts::Config C, typename = void>
      struct use_history : std::false_type {};

      template<concepts::Config C>
      struct use_history<
        C,
        std::enable_if_t<
          std::is_convertible_v<decltype(C::use_history), bool>>> {
        static constexpr bool value = C::use_history;
      };

      template<concepts::Config C, typename = void>
      struct history_depth {
        static constexpr std::size_t value = 16;
      };

      template<concepts::Config C>
      struct history_depth<
        C,
        std::enable_if_t<
          std::is_convertible_v<decltype(C::history_depth), std::size_t>>> {
        static constexpr std::size_t value = C::history_depth;
      };

      template<concepts::Config C, typename = void>
      struct use_help : std::false_type {};

      template<concepts::Config C>
      struct use_help<
        C,
        std::enable_if_t<std::is_convertible_v<decltype(C::use_help), bool>>> {
        static constexpr bool value = C::use_help;
      };

      template<concepts::Config C, typename = void>
      struct use_detailed_error_messages : std::false_type {};

      template<concepts::Config C>
      struct use_detailed_error_messages<
        C,
        std::enable_if_t<
          std::is_convertible_v<decltype(C::use_detailed_error_messages),
                                bool>>> {
        static constexpr bool value = C::use_detailed_error_messages;
      };

      template<concepts::Config C, typename Display>
      struct display_fits_config : std::false_type {};

      template<concepts::Config C, typename Display>
        requires concepts::Display<Display, typename C::char_type>
      struct display_fits_config<C, Display> {
        static constexpr bool value =
          C::use_cursor
            ? concepts::DisplayWithCursor<Display, typename C::char_type>
            : concepts::DisplayWithoutCursor<Display, typename C::char_type>;
      };
    } // namespace dtl

    /**
     * evaluates to true if C specifies the use of a volatile input buffer.
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr bool use_volatile_input_buffer_v =
      dtl::use_volatile_input_buffer<C>::value;

    /**
     * evaluates to the delimier specified by C
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr Delimiter input_delimiter_v =
      dtl::input_delimiter<C>::value;

    /**
     * evaluates to the number of Events that the input should be able to store.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr std::size_t input_size_v = dtl::input_size<C>::value;

    /**
     * evaluates to the output buffer size.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr std::size_t output_size_v = dtl::output_size<C>::value;

    /**
     * evaluates to the input type specified by C.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    using input_type_t = typename dtl::input_type<C>::type;

    /**
     * evaluates to the char_type specified by C.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    using char_type_t = typename dtl::char_type<C>::type;

    /**
     * is true if C has a valid history_depth entry.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<typename C>
    concept has_history_depth = requires {
      { C::history_depth } -> std::convertible_to<std::size_t>;
    } and (C::history_depth > 0);

    /**
     * is true if C specifies that an empty help string prints the whole
     * command structure.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr char_type_t<C> access_separator_v =
      dtl::access_separator<C>::value;

    /**
     * is true if C specifies that an empty help string prints the whole
     * command structure.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr bool empty_help_prints_commands_v =
      dtl::empty_help_prints_commands<C>::value;

    /**
     * is true if C specifies that autocomplete should be used.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr bool use_autocomplete_v = dtl::use_autocomplete<C>::value;

    /**
     * is true if C specifies that cursor movement should be used.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr bool use_cursor_v = dtl::use_cursor<C>::value;

    /**
     * is true if C specifies that history should be used.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr bool use_history_v = dtl::use_history<C>::value;

    template<concepts::Config C>
    inline constexpr std::size_t history_depth_v = dtl::history_depth<C>::value;

    /**
     * is true if C specifies that help functionality should be used.
     *
     * @ingroup config
     * @tparam C the configuration
     */
    template<concepts::Config C>
    inline constexpr bool use_help_v = dtl::use_help<C>::value;

    template<concepts::Config C>
    inline constexpr bool use_detailed_error_messages_v =
      dtl::use_detailed_error_messages<C>::value;

    /**
     * checks if the config C and the Display fit together.
     *
     * @ingroup config
     * @tparam C the configuration
     * @tparam Display the display
     */
    template<concepts::Config C, typename Display>
    inline constexpr bool display_fits_config_v =
      dtl::display_fits_config<C, Display>::value;
  } // namespace config

  struct default_config {
    using char_type = char;
    static constexpr View<const char_type> name = "cli";
    static constexpr View<const char_type> description =
      "a command line interface";
    static constexpr char_type access_separator = '.';
    static constexpr bool use_autocomplete = true;
    static constexpr bool use_cursor = true;
    static constexpr bool use_help = true;
    static constexpr bool use_history = true;
    static constexpr std::size_t history_depth = 16;
    static constexpr std::size_t max_line_length = 32;
    static constexpr Delimiter input_delimiter = Delimiter::lf;
    static constexpr std::size_t input_size = 16;
    static constexpr bool use_volatile_input_buffer = false;
    static constexpr std::size_t output_size = 32;
    static constexpr bool empty_help_prints_commands = true;
  };

} // namespace cli

#endif
