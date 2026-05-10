#include "catch2/catch_test_macros.hpp"
#include "cli.hpp"
#include "cli/config.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>

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
