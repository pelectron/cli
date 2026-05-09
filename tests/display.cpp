
#include "cli/display.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>
#include <cstddef>
#include <limits>

struct SingleLineDisplay {};
struct SingleLineDisplay2 {
  static constexpr std::size_t number_of_lines = 10;
};

struct MultiLineDisplay {
  static constexpr bool is_multiline_display = true;
};

template<std::size_t N, bool isML>
struct DisplayN {
  static constexpr bool is_multiline_display = isML;
  static constexpr std::size_t number_of_lines = N;
};

TEST_CASE("is_multiline_display_v") {
  REQUIRE_FALSE(cli::is_multiline_display_v<SingleLineDisplay>);
  REQUIRE_FALSE(cli::is_multiline_display_v<SingleLineDisplay2>);
  REQUIRE(cli::is_multiline_display_v<MultiLineDisplay>);
  REQUIRE(cli::is_multiline_display_v<DisplayN<5, true>>);
  REQUIRE_FALSE(cli::is_multiline_display_v<DisplayN<5, false>>);
}

TEST_CASE("number_of_lines_v") {
  REQUIRE(cli::number_of_lines_v<SingleLineDisplay> == 1);
  REQUIRE(cli::number_of_lines_v<SingleLineDisplay2> == 1);
  REQUIRE(cli::number_of_lines_v<MultiLineDisplay> ==
          std::numeric_limits<std::size_t>::max());
  REQUIRE(cli::number_of_lines_v<DisplayN<5, false>> == 1);
  REQUIRE(cli::number_of_lines_v<DisplayN<5, true>> == 5);
}
