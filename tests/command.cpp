#include "cli/command.hpp"
#include "cli/string.hpp"

#include <catch2/catch_all.hpp>
#include <vector>

TEST_CASE("CommandNode::add_sub ") {
  cli::CommandNode<char> root;
  cli::CommandNode<char> abcd{.name = "abcd"};
  cli::CommandNode<char> abcde{.name = "abcde"};
  cli::CommandNode<char> abcdef{.name = "abcdef"};
  cli::CommandNode<char> abcdefg{.name = "abcdefg"};

  SECTION("add to empty") {
    root.add_sub(abcd);
    REQUIRE(root.subcommand == &abcd);
  }

  SECTION("add at the end") {
    root.add_sub(abcd);
    root.add_sub(abcde);
    REQUIRE(abcd.next == &abcde);
    root.add_sub(abcdef);
    REQUIRE(abcde.next == &abcdef);
  }

  SECTION("add in the beginning") {
    root.add_sub(abcde);
    root.add_sub(abcd);
    REQUIRE(root.subcommand == &abcd);
    REQUIRE(abcd.next == &abcde);
  }

  SECTION("add in the middle") {
    root.add_sub(abcd);
    root.add_sub(abcdefg);
    REQUIRE(root.subcommand == &abcd);
    root.add_sub(abcde);
    REQUIRE(root.subcommand == &abcd);
    REQUIRE(abcd.next == &abcde);
    REQUIRE(abcde.next == &abcdefg);
    root.add_sub(abcdef);
    REQUIRE(root.subcommand == &abcd);
    REQUIRE(abcd.next == &abcde);
    REQUIRE(abcde.next == &abcdef);
    REQUIRE(abcdef.next == &abcdefg);
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
      cli::View<char> out) -> cli::ExecResult<char> {                          \
    REQUIRE(i_ptr);                                                            \
    REQUIRE(*reinterpret_cast<int *>(i_ptr) == 5);                             \
    REQUIRE(args == "args");                                                   \
    REQUIRE(out.size() == 16);                                                 \
    *reinterpret_cast<int *>(i_ptr) = 0;                                       \
    return cli::ExecResult<char>::make_success();                              \
  }

TEST_CASE("CommandNode::execute") {
  cli::CommandNode<char> root;
  int i = 5;
  char buffer[16]{};

  root.this_ = &i;
  root.exec_ = TEST_LAMBDA();
  cli::View buf{buffer, 16};
  cli::ExecResult r = root.execute("args", buf);
  REQUIRE(r);
  REQUIRE(r.type() == cli::ExecResult<char>::success);
  REQUIRE(r.error() == cli::Error::none);
  REQUIRE(i == 0);
}

TEST_CASE("get_command") {
  cli::CommandNode<char> root;
  cli::CommandNode<char> abcd{.name = "abcd"};
  cli::CommandNode<char> bcd{.name = "bcd"};
  cli::CommandNode<char> cd{.name = "cd"};
  cli::CommandNode<char> abcde{.name = "abcde"};
  cli::CommandNode<char> bcde{.name = "bcde"};
  cli::CommandNode<char> cde{.name = "cde"};

  root.add_sub(abcd);
  abcd.add_sub(bcd);
  abcd.add_sub(cd);

  root.add_sub(abcde);
  abcde.add_sub(bcde);
  abcde.add_sub(cde);

  SECTION("invalid root") {
    REQUIRE(cli::get_command<char>(cli::CharView{}, nullptr, '.') == nullptr);
    REQUIRE(cli::get_command<char>(cli::CharView{"hello"}, nullptr, '.') ==
            nullptr);
  }
  SECTION("plain invalid commands") {
    REQUIRE(cli::get_command(cli::CharView{}, &root, '.') == nullptr);
    REQUIRE(cli::get_command(cli::CharView{" "}, &root, '.') == nullptr);
    REQUIRE(cli::get_command(cli::CharView{"("}, &root, '.') == nullptr);
    REQUIRE(cli::get_command(cli::CharView{"="}, &root, '.') == nullptr);
    REQUIRE(cli::get_command(cli::CharView{"."}, &root, '.') == nullptr);
    REQUIRE(cli::get_command(cli::CharView{".abcd"}, &root, '.') == nullptr);
    REQUIRE(cli::get_command(cli::CharView{"abcd."}, &root, '.') == nullptr);
  }
  SECTION("invalid commands first level") {
    REQUIRE(cli::get_command(cli::CharView{"abc"}, &root, '.') == nullptr);
  }
  SECTION("invalid commands second level") {
    REQUIRE(cli::get_command(cli::CharView{"abcd.abc"}, &root, '.') == nullptr);
    REQUIRE(cli::get_command(cli::CharView{"abcde.abc"}, &root, '.') ==
            nullptr);
  }
  SECTION("valid commands") {
    REQUIRE(cli::get_command(cli::CharView{"abcd"}, &root, '.') == &abcd);
    REQUIRE(cli::get_command(cli::CharView{"abcde"}, &root, '.') == &abcde);
    REQUIRE(cli::get_command(cli::CharView{"abcd.bcd"}, &root, '.') == &bcd);
    REQUIRE(cli::get_command(cli::CharView{"abcd.cd"}, &root, '.') == &cd);
    REQUIRE(cli::get_command(cli::CharView{"abcde"}, &root, '.') == &abcde);
    REQUIRE(cli::get_command(cli::CharView{"abcde.bcde"}, &root, '.') == &bcde);
    REQUIRE(cli::get_command(cli::CharView{"abcde.cde"}, &root, '.') == &cde);
  }
}

TEST_CASE("split line") {
  cli::CommandNode<char> root;
  cli::CommandNode<char> abcd{.name = "abcd"};
  cli::CommandNode<char> bcd{.name = "bcd"};
  cli::CommandNode<char> cd{.name = "cd"};
  cli::CommandNode<char> abcde{.name = "abcde"};
  cli::CommandNode<char> bcde{.name = "bcde"};
  cli::CommandNode<char> cde{.name = "cde"};

  root.add_sub(abcd);
  abcd.add_sub(bcd);
  abcd.add_sub(cd);

  root.add_sub(abcde);
  abcde.add_sub(bcde);
  abcde.add_sub(cde);

  SECTION("empty line") {
    cli::SplitResult r = cli::split_line(cli::CharView{}, &root, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});
  }

  SECTION("empty root") {
    cli::SplitResult r = cli::split_line<char>(cli::CharView{}, nullptr, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});

    r = cli::split_line<char>(cli::CharView{"hello"}, nullptr, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});
  }

  SECTION("plain invalid line") {
    cli::SplitResult r = cli::split_line(cli::CharView{" "}, &root, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});

    r = cli::split_line(cli::CharView{"("}, &root, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});

    r = cli::split_line(cli::CharView{"="}, &root, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});

    r = cli::split_line(cli::CharView{"."}, &root, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});

    r = cli::split_line(cli::CharView{"abcd."}, &root, '.');
    REQUIRE(r.command == nullptr);
    REQUIRE(r.args == cli::CharView{});
  }

  SECTION("valid commands") {

    cli::SplitResult r =
      cli::split_line(cli::CharView{"abcd args"}, &root, '.');
    REQUIRE(r.command == &abcd);
    REQUIRE(r.args == " args");

    r = cli::split_line(cli::CharView{"abcd.bcd args"}, &root, '.');
    REQUIRE(r.command == &bcd);
    REQUIRE(r.args == " args");

    r = cli::split_line(cli::CharView{"abcd.cd args"}, &root, '.');
    REQUIRE(r.command == &cd);
    REQUIRE(r.args == " args");

    r = cli::split_line(cli::CharView{"abcde args"}, &root, '.');
    REQUIRE(r.command == &abcde);
    REQUIRE(r.args == " args");

    r = cli::split_line(cli::CharView{"abcde.bcde args"}, &root, '.');
    REQUIRE(r.command == &bcde);
    REQUIRE(r.args == " args");

    r = cli::split_line(cli::CharView{"abcde.cde args"}, &root, '.');
    REQUIRE(r.command == &cde);
    REQUIRE(r.args == " args");
  }
}
