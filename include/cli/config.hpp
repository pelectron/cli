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
    template<concepts::Config C, typename = void>
    struct use_volatile_input_buffer : std::false_type {};

    template<concepts::Config C>
    struct use_volatile_input_buffer<
      C,
      std::enable_if_t<
        std::is_convertible_v<decltype(C::use_volatile_input_buffer), bool>>> {
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
      static constexpr std::size_t value = 32;
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

    template<concepts::Config C, typename = void>
    struct input_type {
      using type = cli::Input<C>;
    };

    template<concepts::Config C>
    struct input_type<C, std::void_t<typename C::input_type>> {
      using type = typename C::input_type;
    };

    template<concepts::Config C>
    inline constexpr bool use_volatile_input_buffer_v =
      use_volatile_input_buffer<C, void>::value;

    template<concepts::Config C>
    inline constexpr Delimiter input_delimiter_v =
      input_delimiter<C, void>::value;

    template<concepts::Config C>
    inline constexpr std::size_t input_size_v = input_size<C, void>::value;

    template<concepts::Config C>
    inline constexpr std::size_t output_size_v = output_size<C, void>::value;

    template<concepts::Config C>
    using input_type_t = typename input_type<C, void>::type;

    template<typename C>
    concept has_history_depth = requires {
      { C::history_depth } -> std::convertible_to<std::size_t>;
    } and (C::history_depth > 0);

  } // namespace config

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
    static constexpr bool use_cursor = true;
    static constexpr bool use_volatile_input_buffer = false;
    static constexpr bool use_history = true;
    static constexpr std::size_t history_depth = 16;
    static constexpr std::size_t max_line_length = 80;
    static constexpr Delimiter input_delimiter = Delimiter::lf;
    static constexpr Delimiter output_delimiter = Delimiter::lf;
  };
} // namespace cli

#endif
