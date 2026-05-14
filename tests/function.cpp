#include "cli/function.hpp"
#include "catch2/catch_test_macros.hpp"
#include "cli/enums.hpp"
#include "cli/exec_result.hpp"
#include "cli/string.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>

using cli::funcs::arg;
using cli::funcs::func;
using cli::operator""_sc;
using cli::funcs::operator""_arg;

TEST_CASE("arg with default", "[function]") {

  constexpr auto parse_i =
    [](cli::View<const char>) -> cli::parse::ParseResult<int, char> {
    return cli::Error::none;
  };
  constexpr auto validate_i = [](int) -> bool { return true; };

  {
    static constinit auto a =
      arg<int, 5>("name"_sc, "description"_sc, parse_i, validate_i);
    (void)a;
  }
  {
    static constinit auto a = arg<int, 5>("name"_sc, "description"_sc, parse_i);
    (void)a;
  }
  {
    static constinit auto a =
      arg<int, 5>("name"_sc, "description"_sc, validate_i);
    (void)a;
  }
  {
    static constinit auto a = arg<int, 5>("name"_sc, "description"_sc);
    (void)a;
  }
  {
    static constinit auto a = arg<int, 5>("name"_sc);
    (void)a;
  }
  {
    static constinit auto a =
      arg<5>("name"_sc, "description"_sc, parse_i, validate_i);
    (void)a;
  }
  {
    static constinit auto a = arg<5>("name"_sc, "description"_sc, parse_i);
    (void)a;
  }
  {
    static constinit auto a = arg<5>("name"_sc, "description"_sc, validate_i);
    (void)a;
  }
  {
    static constinit auto a = arg<5>("name"_sc, "description"_sc);
    (void)a;
  }
  {
    static constinit auto a = arg<5>("name"_sc);
    (void)a;
  }
}

TEST_CASE("arg without default", "[function]") {
  constexpr auto parse_i =
    [](cli::View<const char>) -> cli::parse::ParseResult<int, char> {
    return cli::Error::none;
  };
  constexpr auto validate_i = [](int) -> bool { return true; };

  {
    static constinit auto a =
      arg<int>("name"_sc, "description"_sc, parse_i, validate_i);
    (void)a;
  }
  {
    static constinit auto a = arg<int>("name"_sc, "description"_sc, parse_i);
    (void)a;
  }
  {
    static constinit auto a = arg<int>("name"_sc, "description"_sc, validate_i);
    (void)a;
  }
  {
    static constinit auto a = arg<int>("name"_sc, "description"_sc);
    (void)a;
  }
  {
    static constinit auto a = arg<int>("name"_sc);
    (void)a;
  }
}

TEST_CASE("deduced args", "[function]") {
  {
    static constinit auto a = arg("name"_sc, "description"_sc);
    (void)a;
  }
  {
    static constinit auto a = arg("name"_sc);
    (void)a;
  }
  {
    static constinit auto a = "name"_arg;
    (void)a;
  }
}

TEST_CASE("function without arguments") {
  constexpr auto f = []() -> int { return 5; };
  static constinit bool called = false;
  constexpr auto f_void = []() -> void { called = true; };
  static constinit auto fn = cli::funcs::func("f"_sc, "description"_sc, f);
  static constinit auto void_fn =
    cli::funcs::func("f"_sc, "description"_sc, f_void);

  char buffer[10]{};
  auto exec_result = fn.execute("()", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "5");

  exec_result = fn.execute("", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_lparen);

  exec_result = fn.execute("(", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_rparen);

  exec_result = fn.execute("k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_lparen);

  exec_result = fn.execute("ka", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_lparen);

  exec_result = fn.execute("(k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_argument);

  exec_result = fn.execute("( k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_argument);

  exec_result = fn.execute("()k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() ==
          cli::Error::unexpected_characters_after_closing_paren);

  exec_result = void_fn.execute("()", {});
  REQUIRE(exec_result);
  REQUIRE(called);
}

TEST_CASE("function with args") {
  constexpr auto f = [](int i) -> int { return i; };
  constexpr auto no_validate_i = [](int) -> bool { return false; };
  static constinit auto fn =
    cli::funcs::func("f"_sc, "description"_sc, f, "i"_arg);

  char buffer[10]{};
  auto exec_result = fn.execute("(5)", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "5");

  exec_result = fn.execute("", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_lparen);

  exec_result = fn.execute("(", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_rparen);

  exec_result = fn.execute("k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_lparen);

  exec_result = fn.execute("ka", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_lparen);

  exec_result = fn.execute("(k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_rparen);

  exec_result = fn.execute("( k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_character);

  exec_result = fn.execute("(5)k", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() ==
          cli::Error::unexpected_characters_after_closing_paren);

  static constinit auto fn_non_validate =
    cli::funcs::func("f"_sc,
                     "description"_sc,
                     f,
                     arg<int>("i"_sc, "description"_sc, no_validate_i));
  exec_result = fn_non_validate.execute("(5)", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::validation_error);
  REQUIRE(exec_result.index() == 1);
}
