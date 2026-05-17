#include "cli/param.hpp"
#include "catch2/catch_test_macros.hpp"
#include "cli.hpp"
#include "cli/basic_format.hpp"
#include "cli/enums.hpp"
#include "cli/exec_result.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "cli/validator.hpp"

#include <catch2/catch_all.hpp>
#include <tuple>

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
    static constinit auto p = cli::param<int>(
      name, description, getter, setter, parser, formatter, validator);
    (void)p;
  }

  // missing one parameter{
  // no validate
  {
    static constinit auto p =
      cli::param<int>(name, description, getter, setter, parser, formatter);
    (void)p;
  }
  // no format and parse
  {
    static constinit auto p =
      cli::param<int>(name, description, getter, setter, validator);
    (void)p;
  }
  // no get -> format not used
  {
    static constinit auto p =
      cli::param<int>(name, description, setter, parser, validator);
    (void)p;
  }
  // no set -> parse not used
  {
    static constinit auto p =
      cli::param<int>(name, description, getter, formatter, validator);
    (void)p;
  }
  //}

  // missing two parameters{
  // no validate and format and parse
  {
    static constinit auto p =
      cli::param<int>(name, description, getter, setter);
    (void)p;
  }
  // no validate and get -> format not used
  {
    static constinit auto p =
      cli::param<int>(name, description, setter, parser);
    (void)p;
  }
  // no validate and set -> parse not used
  {
    static constinit auto p =
      cli::param<int>(name, description, getter, formatter);
    (void)p;
  }

  // no format/parse and get
  {
    static constinit auto p =
      cli::param<int>(name, description, setter, validator);
    (void)p;
  }
  // }

  // missing three parameters {
  // no validate, format/parse and set
  {
    static constinit auto p = cli::param<int>(name, description, getter);
    (void)p;
  }
  // no validate, format/parse and get
  {
    static constinit auto p = cli::param<int>(name, description, setter);
    (void)p;
  }
  // }
}

static constinit int global_i = 0;

TEST_CASE("params with object", "[param]") {
  // base form
  {
    static constinit auto p = cli::param(name,
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
    static constinit auto p = cli::param(
      name, description, global_i, getter, setter, parser, formatter);
    (void)p;
  }
  // no format and parse
  {
    static constinit auto p =
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
    static constinit auto p = cli::param(
      name, description, global_i, getter, parser, formatter, validator);
    (void)p;
  }
  //}

  // missing two parameters{
  // no validate and format/parse
  {
    static constinit auto p =
      cli::param(name, description, global_i, getter, setter);
    (void)p;
  }
  // no validate and get
  {
    static constinit auto p =
      cli::param(name, description, global_i, setter, parser, formatter);
    (void)p;
  }
  // no validate and set
  {
    static constinit auto p =
      cli::param(name, description, global_i, getter, parser, formatter);
    (void)p;
  }

  // no format/parse and get
  {
    static constinit auto p =
      cli::param(name, description, global_i, setter, validator);
    (void)p;
  }
  // no format/parse and set
  {
    static constinit auto p =
      cli::param(name, description, global_i, getter, validator);
    (void)p;
  }
  // no set and get
  {
    static constinit auto p =
      cli::param(name, description, global_i, parser, formatter, validator);
    (void)p;
  }
  // }

  // missing three parameters {
  // no validate, format/parse and set
  {
    static constinit auto p = cli::param(name, description, global_i, getter);
    (void)p;
  }
  // no validate, format/parse and get
  {
    static constinit auto p = cli::param(name, description, global_i, setter);
    (void)p;
  }
  // no get, set and validate
  {
    static constinit auto p =
      cli::param(name, description, global_i, parser, formatter);
    (void)p;
  }
  // no get, set and format/parse
  {
    static constinit auto p =
      cli::param(name, description, global_i, validator);
    (void)p;
  }
  // }
  //
  {
    static constinit auto p = cli::param(name, description, global_i);
    (void)p;
  }
}

TEST_CASE("params with const object", "[param]") {

  static const int i{5};

  // base form
  {
    static constinit auto p =
      cli::param(name, description, i, getter, formatter);
    (void)p;
  }

  // missing one parameter
  //{
  //  no format
  {
    static constinit auto p = cli::param(name, description, i, getter);
    (void)p;
  }
  // no get
  {
    static constinit auto p = cli::param(name, description, i, formatter);
    (void)p;
  }
  //}

  // missing two parameters
  {
    static constinit auto p = cli::param(name, description, i);
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
    static constinit auto p = cli::param(
      name,
      description,
      s,
      cli::param(
        "foo"_sc, "foo mode"_sc, &S::foo, parser, formatter, validator));
    (void)p;
  }

  // no validate
  {
    static constinit auto p = cli::param(
      name,
      description,
      s,
      cli::param("foo"_sc, "foo mode"_sc, &S::foo, parser, formatter));
    (void)p;
  }

  // no parse/format
  {
    static constinit auto p =
      cli::param(name,
                 description,
                 s,
                 cli::param("foo"_sc, "foo mode"_sc, &S::foo, validator));
    (void)p;
  }
  // no parse/format and validate
  {
    static constinit auto p = cli::param(
      name, description, s, cli::param("foo"_sc, "foo mode"_sc, &S::foo));
    (void)p;
  }
}

TEST_CASE("const member data commands", "[param]") {
  static constexpr S s{};
  // base form
  {
    static constinit auto p =
      cli::param(name,
                 description,
                 s,
                 cli::param("foo"_sc, "foo mode"_sc, &S::foo, formatter));
    (void)p;
  }
  {
    static constinit auto p = cli::param(
      name, description, s, cli::param("k"_sc, "k mode"_sc, &S::k, formatter));
    (void)p;
  }

  // no parse/format and validate
  {
    static constinit auto p = cli::param(
      name, description, s, cli::param("foo"_sc, "foo mode"_sc, &S::foo));
    (void)p;
  }

  {
    static constinit auto p =
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

  auto invalid_get_var = [](int &) {
    return cli::Error::invalid_sequence_value;
  };

  static_assert(cli::params::Getter<decltype(get_var)>);
  auto set_var = [](int i_) {
    var = i_;
    return cli::Error::none;
  };

  auto invalid_set_var = [](int) { return cli::Error::invalid_sequence_value; };

  static_assert(cli::params::Setter<decltype(set_var)>);

  auto validate_var = [](int i) { return i >= 0 and i <= 42; };

  static_assert(cli::validate::Validator<decltype(validate_var)>);

  auto bogus_format = [](cli::View<char>, int) -> cli::format::FormatResult {
    return cli::Error::buffer_overflow;
  };

  static_assert(cli::format::Formatter<decltype(bogus_format)>);

  auto bogus_parse =
    [](cli::View<const char>) -> cli::parse::ParseResult<int, char> {
    return cli::Error::unknown;
  };

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

  exec_result = p.execute("=", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_value);

  exec_result = p.execute("k", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_assignment);

  exec_result = p.execute("=100 k", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::unexpected_characters);

  auto const_p = cli::param<int>("i"_sc, "i desc"_sc, get_var);

  exec_result = const_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);

  exec_result = const_p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "21");

  auto write_only_p =
    cli::param<int>("i"_sc, "i desc"_sc, set_var, validate_var);

  exec_result = write_only_p.execute({}, {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::cant_read_param);
  exec_result = write_only_p.execute("=10", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(var == 10);

  auto inv_get_var_p = cli::param<int>("i"_sc, "i desc"_sc, invalid_get_var);
  exec_result = inv_get_var_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  auto inv_set_var_p = cli::param<int>("i"_sc, "i desc"_sc, invalid_set_var);
  exec_result = inv_set_var_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  auto bogus_format_p =
    cli::param<int>("i"_sc, "i desc"_sc, get_var, bogus_format);
  exec_result = bogus_format_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::format_error);
  REQUIRE(exec_result.error() == cli::Error::buffer_overflow);

  auto bogus_parse_p =
    cli::param<int>("i"_sc, "i desc"_sc, set_var, bogus_parse);
  exec_result = bogus_parse_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::unknown);
}

TEST_CASE("DefaultGet") {
  static constexpr int i = 5;
  constexpr cli::params::dtl::DefaultGet<int> get{i};
  int out = 0;
  REQUIRE(get(out) == cli::Error::none);
  REQUIRE(out == 5);
}

TEST_CASE("DefaultSet") {
  static int i = 5;
  cli::params::dtl::DefaultSet<int> set{i};
  int val = 10;
  REQUIRE(set(val) == cli::Error::none);
  REQUIRE(i == val);
}

struct SubSubSettings {
  char a = 'x';
};
struct SubSettings {
  int i = 1;
  SubSubSettings subsubsettings;
};
struct Settings {
  int var = 5;
  char c = 'x';
  SubSettings subsettings{};
};

TEST_CASE("recursive param with callback and validate") {
  constexpr auto validate = [](const Settings &s) -> bool {
    return (s.var > 0 and s.var <= 5) and
           (s.c == 'x' or s.c == 'y' or s.c == 'z') and
           (s.subsettings.i > 0) and (s.subsettings.subsubsettings.a == 'x');
  };

  static constinit bool callback_called = false;
  constexpr auto callback = [](const Settings &) { callback_called = true; };

  static_assert(cli::params::SetCallback<decltype(callback), Settings>);
  static_assert(cli::validate::ValidatorOf<decltype(validate), Settings>);

  static constinit Settings settings;
  auto p = cli::param("settings"_sc,
                      "description"_sc,
                      settings,
                      callback,
                      validate,
                      cli::recursive);
  using cli::get;
  auto &var = get<0>(p);
  auto &c = get<1>(p);
  auto &subsettings = get<2>(p);
  static_assert(subsettings.name == "subsettings"_sc);
  auto &i = get<0>(subsettings);
  static_assert(i.name == "i"_sc);
  auto &subsubsettings = get<1>(subsettings);
  static_assert(subsubsettings.name == "subsubsettings"_sc);
  auto &a = get<0>(subsubsettings);
  static_assert(a.name == "a"_sc);

  var.execute("=1", {});
  REQUIRE(settings.var == 1);
  REQUIRE(callback_called);
  callback_called = false;

  i.execute("=10", {});
  REQUIRE(settings.subsettings.i == 10);
  REQUIRE(callback_called);
  callback_called = false;

  a.execute("=z", {});
  REQUIRE(settings.subsettings.subsubsettings.a == 'x');
  REQUIRE_FALSE(callback_called);
}
