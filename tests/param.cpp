#include "cli/param.hpp"
#include "catch2/catch_test_macros.hpp"
#include "cli.hpp"
#include "cli/enums.hpp"
#include "cli/exec_result.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "cli/validator.hpp"

#include <catch2/catch_all.hpp>

using cli::operator""_sc;
constexpr auto name = "name"_sc;
constexpr auto description = "description"_sc;
constexpr auto getter = [](int &) -> cli::Error { return cli::Error::none; };
constexpr auto setter = [](const int &) -> cli::Error {
  return cli::Error::none;
};

constexpr auto parser =
  [](cli::View<const char>) -> cli::parse::ParseResult<int, char> { return 0; };

constexpr auto formatter = [](cli::View<char>,
                              int) -> cli::format::FormatResult { return 0; };

constexpr auto validator = [](int) -> bool { return true; };

static_assert(cli::format::Formatter<decltype(formatter)>);
static_assert(cli::parse::Parser<decltype(parser)>);
TEST_CASE("params without object", "[param]") {
  // base form
  {
    static CLI_CONSTINIT auto p = cli::param<int>(
      name, description, getter, setter, parser, formatter, validator);
    (void)p;
  }

  // missing one parameter{
  // no validate
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, getter, setter, parser, formatter);
    (void)p;
  }
  // no format and parse
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, getter, setter, validator);
    (void)p;
  }
  // no get -> format not used
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, setter, parser, validator);
    (void)p;
  }
  // no set -> parse not used
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, getter, formatter, validator);
    (void)p;
  }
  //}

  // missing two parameters{
  // no validate and format and parse
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, getter, setter);
    (void)p;
  }
  // no validate and get -> format not used
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, setter, parser);
    (void)p;
  }
  // no validate and set -> parse not used
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, getter, formatter);
    (void)p;
  }

  // no format/parse and get
  {
    static CLI_CONSTINIT auto p =
      cli::param<int>(name, description, setter, validator);
    (void)p;
  }
  // }

  // missing three parameters {
  // no validate, format/parse and set
  {
    static CLI_CONSTINIT auto p = cli::param<int>(name, description, getter);
    (void)p;
  }
  // no validate, format/parse and get
  {
    static CLI_CONSTINIT auto p = cli::param<int>(name, description, setter);
    (void)p;
  }
  // }
}

static constinit int global_i = 0;

TEST_CASE("params with object", "[param]") {
  // base form
  {
    static CLI_CONSTINIT auto p = cli::param(name,
                                             description,
                                             global_i,
                                             getter,
                                             setter,
                                             parser,
                                             formatter,
                                             validator);
    (void)p;
  }

  // missing one parameter{
  // no validate
  {
    static CLI_CONSTINIT auto p = cli::param(
      name, description, global_i, getter, setter, parser, formatter);
    (void)p;
  }
  // no format and parse
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, getter, setter, validator);
    (void)p;
  }
  // no get
  {
    static constexpr auto p = cli::param(
      name, description, global_i, setter, parser, formatter, validator);
    (void)p;
  }
  // no set
  {
    static CLI_CONSTINIT auto p = cli::param(
      name, description, global_i, getter, parser, formatter, validator);
    (void)p;
  }
  //}

  // missing two parameters{
  // no validate and format/parse
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, getter, setter);
    (void)p;
  }
  // no validate and get
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, setter, parser, formatter);
    (void)p;
  }
  // no validate and set
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, getter, parser, formatter);
    (void)p;
  }

  // no format/parse and get
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, setter, validator);
    (void)p;
  }
  // no format/parse and set
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, getter, validator);
    (void)p;
  }
  // no set and get
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, parser, formatter, validator);
    (void)p;
  }
  // }

  // missing three parameters {
  // no validate, format/parse and set
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, getter);
    (void)p;
  }
  // no validate, format/parse and get
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, setter);
    (void)p;
  }
  // no get, set and validate
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, parser, formatter);
    (void)p;
  }
  // no get, set and format/parse
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, global_i, validator);
    (void)p;
  }
  // }
  //
  {
    static CLI_CONSTINIT auto p = cli::param(name, description, global_i);
    (void)p;
  }
}

TEST_CASE("params with const object", "[param]") {

  static const int i{5};

  // base form
  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, i, getter, formatter);
    (void)p;
  }

  // missing one parameter
  //{
  //  no format
  {
    static CLI_CONSTINIT auto p = cli::param(name, description, i, getter);
    (void)p;
  }
  // no get
  {
    static CLI_CONSTINIT auto p = cli::param(name, description, i, formatter);
    (void)p;
  }
  //}

  // missing two parameters
  {
    static CLI_CONSTINIT auto p = cli::param(name, description, i);
    (void)p;
  }
}

struct S {
  int foo{0};
  const int k{5};
};

TEST_CASE("member data commands", "[param]") {
  static constinit S s;
  // base form
  {
    static CLI_CONSTINIT auto p = cli::param(
      name,
      description,
      s,
      cli::param(
        "foo"_sc, "foo mode"_sc, &S::foo, parser, formatter, validator));
    (void)p;
  }

  // no validate
  {
    static CLI_CONSTINIT auto p = cli::param(
      name,
      description,
      s,
      cli::param("foo"_sc, "foo mode"_sc, &S::foo, parser, formatter));
    (void)p;
  }

  // no parse/format
  {
    static CLI_CONSTINIT auto p =
      cli::param(name,
                 description,
                 s,
                 cli::param("foo"_sc, "foo mode"_sc, &S::foo, validator));
    (void)p;
  }
  // no parse/format and validate
  {
    static CLI_CONSTINIT auto p = cli::param(
      name, description, s, cli::param("foo"_sc, "foo mode"_sc, &S::foo));
    (void)p;
  }
}

TEST_CASE("const member data commands", "[param]") {
  static constexpr S s{};
  // base form
  {
    static CLI_CONSTINIT auto p =
      cli::param(name,
                 description,
                 s,
                 cli::param("foo"_sc, "foo mode"_sc, &S::foo, formatter));
    (void)p;
  }
  {
    static CLI_CONSTINIT auto p = cli::param(
      name, description, s, cli::param("k"_sc, "k mode"_sc, &S::k, formatter));
    (void)p;
  }

  // no parse/format and validate
  {
    static CLI_CONSTINIT auto p = cli::param(
      name, description, s, cli::param("foo"_sc, "foo mode"_sc, &S::foo));
    (void)p;
  }

  {
    static CLI_CONSTINIT auto p =
      cli::param(name, description, s, cli::param("k"_sc, "k mode"_sc, &S::k));
    (void)p;
  }
}

TEST_CASE("param", "[param]") {
  static constinit int var = 42;

  char buffer[10]{};
  auto get_var = [](int &i_) {
    i_ = var;
    return cli::Error::none;
  };

  static_assert(cli::params::Getter<decltype(get_var)>);
  auto set_var = [](int i_) {
    var = i_;
    return cli::Error::none;
  };

  static_assert(cli::params::Setter<decltype(set_var)>);

  auto validate_var = [](int i) { return i >= 0 and i <= 42; };

  static_assert(cli::validate::Validator<decltype(validate_var)>);

  auto p = cli::param<int>("i"_sc, "i desc"_sc, get_var, set_var, validate_var);
  REQUIRE(p.name == "i"_sc);
  REQUIRE(p.description == "i desc"_sc);
  REQUIRE(p.type == "int"_sc);
  auto exec_result = p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "42");

  exec_result = p.execute("=21", {buffer, 10});

  REQUIRE(exec_result);
  REQUIRE(var == 21);

  exec_result = p.execute("=100", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_value);

  auto const_p = cli::param<int>("i"_sc, "i desc"_sc, get_var);

  exec_result = const_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);

  exec_result = const_p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "21");

  auto write_only_p =
    cli::param<int>("i"_sc, "i desc"_sc, set_var, validate_var);

  exec_result = write_only_p.execute({}, {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  exec_result = write_only_p.execute("=10", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(var == 10);
}
