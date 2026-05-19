#include "cli/display.hpp"

#include <catch2/catch_all.hpp>

struct SingleLineDisplay {};
struct SingleLineDisplay2 {
  static constexpr std::size_t number_of_lines = 10;
};

struct MultiLineDisplay {
  static constexpr bool is_multiline = true;
};

template<std::size_t N, bool isML>
struct DisplayN {
  static constexpr bool is_multiline = isML;
  static constexpr std::size_t number_of_lines = N;
};

TEST_CASE("display::is_multiline_v") {
  REQUIRE_FALSE(cli::display::is_multiline_v<SingleLineDisplay>);
  REQUIRE_FALSE(cli::display::is_multiline_v<SingleLineDisplay2>);
  REQUIRE(cli::display::is_multiline_v<MultiLineDisplay>);
  REQUIRE(cli::display::is_multiline_v<DisplayN<5, true>>);
  REQUIRE_FALSE(cli::display::is_multiline_v<DisplayN<5, false>>);
}

TEST_CASE("display::number_of_lines_v") {
  REQUIRE(cli::display::number_of_lines_v<SingleLineDisplay> == 1);
  REQUIRE(cli::display::number_of_lines_v<SingleLineDisplay2> == 1);
  REQUIRE(cli::display::number_of_lines_v<MultiLineDisplay> ==
          std::numeric_limits<std::size_t>::max());
  REQUIRE(cli::display::number_of_lines_v<DisplayN<5, false>> == 1);
  REQUIRE(cli::display::number_of_lines_v<DisplayN<5, true>> == 5);
}
