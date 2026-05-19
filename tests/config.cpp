#include "cli/config.hpp"
#include "cli/enums.hpp"

#include <catch2/catch_all.hpp>

struct cfg {
  static constexpr cli::View<const char> name = "";
  static constexpr cli::View<const char> description = "";
  static constexpr std::size_t max_line_length = 16;
};

struct volatile_buffer_cfg : cfg {
  static constexpr bool use_volatile_input_buffer = true;
};

struct input_delimiter_cfg : cfg {
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::crlf;
};

struct input_size_cfg : cfg {
  static constexpr std::size_t input_size = 256;
};

struct output_size_cfg : cfg {
  static constexpr std::size_t output_size = 256;
};

struct input_type1_cfg : cfg {
  struct input_type {};
};

struct input_type2_cfg : cfg {
  template<typename C>
  struct input_type {};
};

struct char16_t_config {
  using char_type = char16_t;
  static constexpr cli::View<const char16_t> name = u"";
  static constexpr cli::View<const char16_t> description = u"";
  static constexpr std::size_t max_line_length = 16;
};

struct access_separator1_cfg : cfg {
  static constexpr char access_separator = 'x';
};

struct access_separator2_cfg : char16_t_config {
  static constexpr char16_t access_separator = 'x';
};

struct empty_help_prints_command_cfg : cfg {
  static constexpr bool empty_help_prints_commands = true;
};

struct autocomplete_cfg : cfg {
  static constexpr bool use_autocomplete = true;
};

struct cursor_cfg : cfg {
  static constexpr bool use_cursor = true;
};

struct history1_cfg : cfg {
  static constexpr bool use_history = true;
  static constexpr std::size_t history_depth = 32;
};

struct history2_cfg : cfg {
  static constexpr bool use_history = true;
};

struct help_cfg : cfg {
  static constexpr bool use_help = true;
};

struct detailed_error_messages_cfg : cfg {
  static constexpr bool use_detailed_error_messages = true;
};

TEST_CASE("config::use_volatile_input_buffer_v") {
  REQUIRE_FALSE(cli::config::use_volatile_input_buffer_v<cfg>);
  REQUIRE(cli::config::use_volatile_input_buffer_v<volatile_buffer_cfg>);
}

TEST_CASE("config::input_delimiter_v") {
  REQUIRE(cli::config::input_delimiter_v<cfg> == cli::Delimiter::lf);
  REQUIRE(cli::config::input_delimiter_v<input_delimiter_cfg> ==
          cli::Delimiter::crlf);
}

TEST_CASE("config::input_size_v") {
  REQUIRE(cli::config::input_size_v<cfg> == 16);
  REQUIRE(cli::config::input_size_v<input_size_cfg> == 256);
}

TEST_CASE("config::output_size_v") {
  REQUIRE(cli::config::output_size_v<cfg> == 16);
  REQUIRE(cli::config::output_size_v<output_size_cfg> == 256);
}

TEST_CASE("config::input_type_t") {
  REQUIRE(std::same_as<cli::config::input_type_t<cli::default_config>,
                       cli::Input<cli::default_config>>);

  REQUIRE(std::same_as<cli::config::input_type_t<input_type1_cfg>,
                       input_type1_cfg::input_type>);

  REQUIRE(std::same_as<cli::config::input_type_t<input_type2_cfg>,
                       input_type2_cfg::input_type<input_type2_cfg>>);
}

TEST_CASE("config::char_type_t") {
  REQUIRE(std::same_as<cli::config::char_type_t<cfg>, char>);
  REQUIRE(std::same_as<cli::config::char_type_t<char16_t_config>, char16_t>);
}

TEST_CASE("config::access_separator_v") {
  REQUIRE(cli::config::access_separator_v<cfg> == '.');
  REQUIRE(cli::config::access_separator_v<access_separator1_cfg> == 'x');
  REQUIRE(std::same_as<
          const char16_t,
          decltype(cli::config::access_separator_v<access_separator2_cfg>)>);
  REQUIRE(cli::config::access_separator_v<access_separator2_cfg> == 'x');
}

TEST_CASE("config::empty_help_prints_commands_v") {
  REQUIRE_FALSE(cli::config::empty_help_prints_commands_v<cfg>);
  REQUIRE(
    cli::config::empty_help_prints_commands_v<empty_help_prints_command_cfg>);
}

TEST_CASE("config::use_autocomplete_v") {
  REQUIRE_FALSE(cli::config::use_autocomplete_v<cfg>);
  REQUIRE(cli::config::use_autocomplete_v<autocomplete_cfg>);
}

TEST_CASE("config::use_cursor_v") {
  REQUIRE_FALSE(cli::config::use_cursor_v<cfg>);
  REQUIRE(cli::config::use_cursor_v<cursor_cfg>);
}

TEST_CASE("config::use_history_v") {
  REQUIRE_FALSE(cli::config::use_history_v<cfg>);
  REQUIRE(cli::config::use_history_v<history1_cfg>);
  REQUIRE(cli::config::use_history_v<history2_cfg>);
}

TEST_CASE("config::history_size_v") {
  REQUIRE(cli::config::history_depth_v<history1_cfg> == 32);
  REQUIRE(cli::config::history_depth_v<history2_cfg> == 16);
}

TEST_CASE("config::use_help_v") {
  REQUIRE_FALSE(cli::config::use_help_v<cfg>);
  REQUIRE(cli::config::use_help_v<help_cfg>);
}

TEST_CASE("config::use_detailed_error_messages_v") {
  REQUIRE_FALSE(cli::config::use_detailed_error_messages_v<cfg>);
  REQUIRE(
    cli::config::use_detailed_error_messages_v<detailed_error_messages_cfg>);
}
