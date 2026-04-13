#include "cli/tracker.hpp"
#include "cli/cli.hpp"
#include "cli/command.hpp"
#include "cli/config.hpp"
#include "cli/string.hpp"

#include <catch2/catch_all.hpp>
#include <type_traits>

using cli::operator""_sc;

int cmd1_i;
constexpr auto cmd1 = cli::param("comd1"_sc, cmd1_i);

int cmd2_i;
int cmd3_i;
constexpr auto cmd2 =
    cli::param("cmd2"_sc, cmd2_i, cli::param("cmd3"_sc, cmd3_i));

static cli::CommandNode<char> cmds[5];
static std::tuple commands{cmd1, cmd2};

using View = cli::View<const char>;

template <cli::Command Cmd>
void init_cmd(std::size_t &index, cli::CommandNode<char> &parent, Cmd &cmd) {
  // initialize the node
  cli::CommandNode<char> &node = cmds[index];
  node.name = Cmd::name;
  node.description = Cmd::description;
  node.type = Cmd::type;
  node.this_ = &cmd;
  node.exec_ = +[](void *this_, cli::ExecType type, cli::View<const char> args,
                   cli::View<char> &out) -> cli::Error {
    return static_cast<Cmd *>(this_)->execute(type, args, out);
  };
  // add the node to the parent
  parent.add_sub(node);
  // initialize sub commands of cmd
  cli::for_each(
      [&index, &node](cli::Command auto &c) { init_cmd(++index, node, c); },
      cmd);
}

void init_commands() {
  auto &root = cmds[0];
  root.name = "root";
  root.description = "root";

  std::size_t index = 1;
  cli::for_each([&index, &root](auto &cmd) { init_cmd(++index, root, cmd); },
                commands);
}

TEST_CASE("Tracker") {
  cli::Tracker<cli::default_config, std::remove_cvref_t<decltype(cmd1)>,
               std::remove_cvref_t<decltype(cmd2)>>
      tracker{cmds[0]};
  init_commands();
  tracker.on_char('c');
  tracker.on_char('m');
  REQUIRE(tracker.on_autocomplete() == View{"d2"});
  REQUIRE(tracker.cmd() == cmds + 3);
  tracker.on_char('.');
  REQUIRE(tracker.on_autocomplete() == View{""});
  tracker.on_char('c');
  REQUIRE(tracker.on_autocomplete() == View{"md3"});
  // REQUIRE(tracker.cmd() == cmds + 3);
}
