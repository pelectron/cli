#include "cli/tracker.hpp"
#include "cli/cli.hpp"
#include "cli/command.hpp"
#include "cli/config.hpp"
#include "cli/string.hpp"

#include <catch2/catch_all.hpp>
#include <type_traits>

using cli::operator""_sc;

int cmd1_i;
constexpr auto cmd1 = cli::param("comd1"_sc, ""_sc, cmd1_i);

int cmd2_i;
int cmd3_i;
constexpr auto cmd2 =
    cli::param("cmd2"_sc, ""_sc, cmd2_i, cli::param("cmd3"_sc, ""_sc, cmd3_i));

using View = cli::View<const char>;

TEST_CASE("Tracker") {
  static constinit cli::CommandTree tree{cli::default_config{}, cmd1, cmd2};
  static constinit cli::Tracker<cli::default_config,
                                std::remove_cvref_t<decltype(cmd1)>,
                                std::remove_cvref_t<decltype(cmd2)>>
      tracker{*tree.root()};
  tracker.on_char('c');
  tracker.on_char('m');
  REQUIRE(tracker.on_autocomplete() == View{"d2"});
  REQUIRE(tracker.cmd() == tree.root() + 3);
  tracker.on_char('.');
  REQUIRE(tracker.on_autocomplete() == View{""});
  tracker.on_char('c');
  REQUIRE(tracker.on_autocomplete() == View{"md3"});
  // REQUIRE(tracker.cmd() == cmds + 3);
}
