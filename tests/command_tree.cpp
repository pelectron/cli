#include "catch2/catch_test_macros.hpp"
#include "cli.hpp"
#include "cli/config.hpp"
#include "mock_engine.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>

using cli::operator""_sc;

static constinit int global_i{0};

TEST_CASE("CommandTree") {
  MockEngine engine{cli::param("c1"_sc, "c1 desc"_sc, global_i),
                    cli::param("c"_sc, "c desc"_sc, global_i)};

  const auto *root = engine.tree.root();
  REQUIRE(root->subcommand->name == "c");
  REQUIRE(root->subcommand->next->name == "c1");
  REQUIRE(root->subcommand->next->next->name == "help");
}

TEST_CASE("CommandTree rvalue tuple") {
  MockEngine engine{
    cli::Tuple{cli::param("c1"_sc, "c1 desc"_sc, global_i),
               cli::param("c"_sc, "c desc"_sc, global_i)}
  };

  const auto *root = engine.tree.root();
  REQUIRE(root->subcommand->name == "c");
  REQUIRE(root->subcommand->next->name == "c1");
  REQUIRE(root->subcommand->next->next->name == "help");
}

TEST_CASE("CommandTree lvalue tuple") {
  const cli::Tuple params{cli::param("c1"_sc, "c1 desc"_sc, global_i),
                          cli::param("c"_sc, "c desc"_sc, global_i)};

  MockEngine engine{params};

  const auto *root = engine.tree.root();
  REQUIRE(root->subcommand->name == "c");
  REQUIRE(root->subcommand->next->name == "c1");
  REQUIRE(root->subcommand->next->next->name == "help");
}
