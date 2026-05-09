#include "catch2/catch_test_macros.hpp"
#include "cli.hpp"
#include "cli/config.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>

template<cli::concepts::Command... Commands>
struct MockEngine {
  using config_type = cli::default_config;
  using char_type = typename config_type::char_type;

  constexpr MockEngine(Commands... commands)
    : tree(*this, commands...) {}

  constexpr MockEngine(std::tuple<Commands...> &&commands)
    : tree(*this, std::move(commands)) {}

  constexpr MockEngine(const std::tuple<Commands...> &commands)
    : tree(*this, commands) {}

  constexpr void print() { print_called = true; }

  constexpr const cli::CommandNode<char_type> *root() const {
    return tree.root();
  }

  bool print_called = false;
  cli::CommandTree<MockEngine<Commands...>, Commands...> tree;
};

using cli::operator""_sc;

static constinit int i{0};

TEST_CASE("CommandTree") {
  MockEngine engine{cli::param("c1"_sc, "c1 desc"_sc, i),
                    cli::param("c"_sc, "c desc"_sc, i)};

  const auto *root = engine.tree.root();
  REQUIRE(root->subcommand->name == "c");
  REQUIRE(root->subcommand->next->name == "c1");
  REQUIRE(root->subcommand->next->next->name == "help");
}

TEST_CASE("CommandTree rvalue tuple") {
  MockEngine engine{
    std::tuple{cli::param("c1"_sc, "c1 desc"_sc, i),
               cli::param("c"_sc, "c desc"_sc, i)}
  };

  const auto *root = engine.tree.root();
  REQUIRE(root->subcommand->name == "c");
  REQUIRE(root->subcommand->next->name == "c1");
  REQUIRE(root->subcommand->next->next->name == "help");
}

TEST_CASE("CommandTree lvalue tuple") {
  const std::tuple params{cli::param("c1"_sc, "c1 desc"_sc, i),
                          cli::param("c"_sc, "c desc"_sc, i)};

  MockEngine engine{params};

  const auto *root = engine.tree.root();
  REQUIRE(root->subcommand->name == "c");
  REQUIRE(root->subcommand->next->name == "c1");
  REQUIRE(root->subcommand->next->next->name == "help");
}
