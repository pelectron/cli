#include <cli/command.hpp>

#include <catch2/catch_all.hpp>
#include <vector>

TEST_CASE("CommandNode::add_sub ") {
  cli::CommandNode<char> root;
  cli::CommandNode<char> abcd{.name = "abcd"};
  cli::CommandNode<char> abcde{.name = "abcde"};
  cli::CommandNode<char> abcdef{.name = "abcdef"};

  SECTION("add to empty") {
    root.add_sub(abcd);
    REQUIRE(root.subcommand == &abcd);
    REQUIRE(root.last_subcommand == &abcd);
  }

  SECTION("add at the end") {
    root.add_sub(abcd);
    root.add_sub(abcde);
    REQUIRE(abcd.next == &abcde);
    REQUIRE(root.last_subcommand == &abcde);
    root.add_sub(abcdef);
    REQUIRE(abcde.next == &abcdef);
    REQUIRE(root.last_subcommand == &abcdef);
  }

  SECTION("add in the beginning") {
    root.add_sub(abcde);
    root.add_sub(abcd);
    REQUIRE(root.subcommand == &abcd);
    REQUIRE(abcd.next == &abcde);
    REQUIRE(root.last_subcommand == &abcde);
  }

  SECTION("add in the middle") {
    root.add_sub(abcd);
    root.add_sub(abcdef);
    REQUIRE(root.subcommand == &abcd);
    REQUIRE(root.last_subcommand == &abcdef);
    root.add_sub(abcde);
    REQUIRE(root.subcommand == &abcd);
    REQUIRE(root.last_subcommand == &abcdef);
    REQUIRE(abcd.next == &abcde);
    REQUIRE(abcde.next == &abcdef);
  }
}

TEST_CASE("CommandNode::iteration") {
  std::vector<cli::View<const char>> names;
  std::vector<cli::View<const char>> expected{"abcd", "abcde", "abcdef"};
  cli::CommandNode<char> root;
  cli::CommandNode<char> abcd{.name = "abcd"};
  cli::CommandNode<char> abcde{.name = "abcde"};
  cli::CommandNode<char> abcdef{.name = "abcdef"};
  root.add_sub(abcd);
  root.add_sub(abcde);
  root.add_sub(abcdef);

  for (const cli::CommandNode<char> &child : root) {
    names.push_back(child.name);
  }

  REQUIRE(names == expected);
}

#define TEST_LAMBDA()                                                          \
  +[](void *i_ptr,                                                             \
      cli::View<const char> args,                                              \
      cli::View<char> &out,                                                    \
      bool &should_print_newline) -> cli::Error {                              \
    REQUIRE(*reinterpret_cast<int *>(i_ptr) == 5);                             \
    REQUIRE(args == "args");                                                   \
    REQUIRE(out.size() == 16);                                                 \
    *reinterpret_cast<int *>(i_ptr) = 0;                                       \
    out = {};                                                                  \
    should_print_newline = true;                                               \
    return cli::Error::dual_separators;                                        \
  }

TEST_CASE("CommandNode::execute") {
  cli::CommandNode<char> root;
  int i = 5;
  char buffer[16]{};
  bool should_print_newline = false;

  SECTION("execute without this and without execute") {
    cli::View buf{buffer, 16};
    REQUIRE(root.execute("args", buf, should_print_newline) ==
            cli::Error::invalid_cmd);
    REQUIRE(i == 5);
    REQUIRE(buf.size() == 16);
    REQUIRE_FALSE(should_print_newline);
  }

  SECTION("execute with this and without execute") {
    root.this_ = &i;
    cli::View buf{buffer, 16};
    REQUIRE(root.execute("args", buf, should_print_newline) ==
            cli::Error::invalid_cmd);
    REQUIRE(i == 5);
    REQUIRE(buf.size() == 16);
    REQUIRE_FALSE(should_print_newline);
  }

  SECTION("execute without this and with execute") {
    root.exec_ = TEST_LAMBDA();
    cli::View buf{buffer, 16};
    REQUIRE(root.execute("args", buf, should_print_newline) ==
            cli::Error::invalid_cmd);
    REQUIRE(i == 5);
    REQUIRE(buf.size() == 16);
    REQUIRE_FALSE(should_print_newline);
  }

  SECTION("execute with this and with execute") {
    root.this_ = &i;
    root.exec_ = TEST_LAMBDA();
    cli::View buf{buffer, 16};
    REQUIRE(root.execute("args", buf, should_print_newline) ==
            cli::Error::dual_separators);
    REQUIRE(i == 0);
    REQUIRE(buf.size() == 0);
    REQUIRE(should_print_newline);
  }
}
