#include "catch2/catch_test_macros.hpp"
#include "cli.hpp"
#include "cli/command.hpp"
#include "mock_engine.hpp"

#include <catch2/catch_all.hpp>

using cli::operator""_sc;
static int i;
TEST_CASE("help") {
  MockEngine engine{cli::param("c1"_sc, "c1 desc"_sc, i),
                    cli::param("c"_sc, "c desc"_sc, i),
                    cli::param("c2"_sc, ""_sc, i),
                    cli::func(
                      "func"_sc,
                      "func desc"_sc,
                      [](int /*arg*/, int /*arg2*/ = 1) {},
                      cli::arg("arg"_sc, "arg desc"_sc),
                      cli::arg<1>("arg2"_sc, "arg2 desc"_sc))};
  const cli::CommandNode<char> *help = engine.tree.get_command("help");

  [[maybe_unused]] cli::View<char> out;

  SECTION("parameter help") {
    help->execute("(c1)", out);
    REQUIRE(engine.display_.data == "[int]: c1 desc");

    help->execute("(c)", out);
    REQUIRE(engine.display_.data == "[int]: c desc");
  }

  SECTION("no description") {
    help->execute("(c2)", out);
    REQUIRE(engine.display_.data == "[int]: no description available");
  }

  SECTION("function help") {
    help->execute("(func)", out);
    REQUIRE(engine.display_.data ==
            "[(arg: int, arg2: int? = 1)->void]: func desc");

    help->execute("(func, arg)", out);
    REQUIRE(engine.display_.data == "[int]: arg desc");

    help->execute("(func, arg2)", out);
    REQUIRE(engine.display_.data == "[int? = 1]: arg2 desc");
  }

  SECTION("invalid command") {
    help->execute("(command)", out);
    REQUIRE(engine.display_.data == "no such command");
  }

  SECTION("invalid arg") {
    help->execute("(c2, arg)", out);
    REQUIRE(engine.display_.data == "no description available");

    help->execute("(arg=arg)", out);
    REQUIRE(engine.display_.data == "no such command");
  }

  SECTION("empty help") {
    help->execute("()", out);
    REQUIRE(engine.print_called);
  }
}
