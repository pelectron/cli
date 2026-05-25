#include "cli/param.hpp"
#include "cli/enums.hpp"
#include "cli/exec_result.hpp"
#include "cli/util.hpp"

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using cli::operator""_sc;
using cli::params::param;

constexpr auto name = "name"_sc;
constexpr auto description = "description"_sc;
constexpr auto get = [](int &) -> cli::Error { return cli::Error::none; };
constexpr auto set = [](const int &) -> cli::Error { return cli::Error::none; };

constexpr auto parse =
  [](cli::View<const char>) -> cli::parse::ParseResult<int, char> { return 0; };

constexpr auto format = [](cli::View<char>, int) -> cli::format::FormatResult {
  return 0;
};

constexpr auto validate = [](int) -> bool { return true; };

static_assert(cli::format::Formatter<decltype(format)>);
static_assert(cli::parse::Parser<decltype(parse)>);

using T = int;

TEST_CASE("params without object", "[param]") {
  // clang-format off
  // the basic/full form
  (void)param<T>(name, description, get, set, parse, format, validate);
  (void)param<T>(name,              get, set, parse, format, validate);

  // a parameter with default parser and formatter
  (void)param<T>(name, description, get, set,                validate);
  (void)param<T>(name,              get, set,                validate);

  // a parameter with default validator
  (void)param<T>(name, description, get, set, parse, format);
  (void)param<T>(name,              get, set, parse, format);

  // default parser, formatter and validator are used
  (void)param<T>(name, description, get, set);
  (void)param<T>(name,              get, set);

  // a write-only parameter with custom parser and validator
  (void)param<T>(name, description,      set, parse,         validate);
  (void)param<T>(name,                   set, parse,         validate);

  // a write-only parameter with custom parser
  (void)param<T>(name, description,      set, parse);
  (void)param<T>(name,                   set, parse);

  // a write-only parameter with custom validator
  (void)param<T>(name, description,      set,                validate);
  (void)param<T>(name,                   set,                validate);

  // a write-only parameter
  (void)param<T>(name, description,      set);
  (void)param<T>(name,                   set);

  // a read-only parameter with custom formatter
  (void)param<T>(name, description, get,             format);
  (void)param<T>(name,              get,             format);

  // a read-only parameter
  (void)param<T>(name, description, get);
  (void)param<T>(name,              get);
  // clang-format on
}

static constinit int t = 0;
static constexpr int ct = 0;

TEST_CASE("params with object", "[param]") {
  // clang-format off
  // the basic/full form
  (void)param(name, description, t, get, set, parse, format, validate);
  (void)param(name,              t, get, set, parse, format, validate);

  // a parameter with default parser and formatter
  (void)param(name, description, t, get, set,                validate);
  (void)param(name,              t, get, set,                validate);

  // a parameter with default validator
  (void)param(name, description, t, get, set, parse, format);
  (void)param(name,              t, get, set, parse, format);

  // default parser, formatter and validator are used
  (void)param(name, description, t, get, set);
  (void)param(name,              t, get, set);

  // default getter is used
  (void)param(name, description, t,      set, parse, format, validate);
  (void)param(name,              t,      set, parse, format, validate);

  // default getter, parser and formatter are used
  (void)param(name, description, t,      set,                validate);
  (void)param(name,              t,      set,                validate);

  // default getter and validator are used
  (void)param(name, description, t,      set, parse, format);
  (void)param(name,              t,      set, parse, format);

  // default getter, parser, formatter and validator are used
  (void)param(name, description, t,      set);
  (void)param(name,              t,      set);

  // default setter is used
  (void)param(name, description, t, get,      parse, format, validate);
  (void)param(name,              t, get,      parse, format, validate);

  // default setter, parser, and formatter are used
  (void)param(name, description, t, get,                     validate);
  (void)param(name,              t, get,                     validate);

  // default setter and validator are used
  (void)param(name, description, t, get,      parse, format);
  (void)param(name,              t, get,      parse, format);

  // default setter, parser, formatter and validator are used
  (void)param(name, description, t, get);
  (void)param(name,              t, get);

  // default getter and setter are used
  (void)param(name, description, t,           parse, format, validate);
  (void)param(name,              t,           parse, format, validate);

  // default getter, setter, parser, and formatter are used
  (void)param(name, description, t,                          validate);
  (void)param(name,              t,                          validate);

  // default getter, setter and validator are used
  (void)param(name, description, t,           parse, format);
  (void)param(name,              t,           parse, format);

  // default setter, getter, parser, formatter and validator are used
  (void)param(name, description, t);
  (void)param(name,              t);


  // the basic/full form, where t is passed as a template parameter.
  (void)param<t>(description, get, set, parse, format, validate);
  (void)param<t>(             get, set, parse, format, validate);

  // a parameter with default parser and formatter
  (void)param<t>(description, get, set,                validate);
  (void)param<t>(             get, set,                validate);

  // a parameter with default validator
  (void)param<t>(description, get, set, parse, format);
  (void)param<t>(             get, set, parse, format);

  // default parser, formatter and validator are used
  (void)param<t>(description, get, set);
  (void)param<t>(             get, set);

  // default getter is used
  (void)param<t>(description,      set, parse, format, validate);
  (void)param<t>(                  set, parse, format, validate);

  // default getter, parser and formatter are used
  (void)param<t>(description,      set,                validate);
  (void)param<t>(                  set,                validate);

  // default getter, parser and formatter are used
  (void)param<t>(description,                          validate);
  (void)param<t>(                                      validate);

  // default getter and validator are used
  (void)param<t>(description,      set, parse, format);
  (void)param<t>(                  set, parse, format);

  // default getter, parser, formatter and validator are used
  (void)param<t>(description,      set);
  (void)param<t>(                  set);

  // default setter is used
  (void)param<t>(description, get,      parse, format, validate);
  (void)param<t>(             get,      parse, format, validate);

  // default setter, parser, and formatter are used
  (void)param<t>(description, get,                     validate);
  (void)param<t>(             get,                     validate);

  // default setter and validator are used
  (void)param<t>(description, get,      parse, format);
  (void)param<t>(             get,      parse, format);
  // clang-format on
}

TEST_CASE("params with const object", "[param]") {
  // clang-format off
  // base form with t as argument
  (void)param(name, description, t, get, format);
  (void)param(name,              t, get, format);

  // default getter is used
  (void)param(name, description, t,      format);
  (void)param(name,              t,      format);

  // default formatter is used
  (void)param(name, description, cli::as_const(t), get);
  (void)param(name,              cli::as_const(t), get);

  // default formatter and getter are used
  (void)param(name, description, cli::as_const(t));
  (void)param(name,              cli::as_const(t));

  // base form with t as template parameter
  (void)param<t>(description, get, format);
  (void)param<t>(             get, format);

  // default getter is used
  (void)param<t>(description,      format);
  (void)param<t>(                  format);

  #if !defined(_MSC_VER)
  // default formatter is used
  (void)param<ct>(description, get);
  (void)param<ct>(             get);

  // default formatter and getter are used
  (void)param<ct>(description);
  (void)param<ct>(           );
  #endif
  // clang-format on
}

struct S {
  int foo{0};
  const int k{5};
};

constexpr auto ptr_to_member = &S::foo;
constexpr auto const_ptr_to_member = &S::k;

TEST_CASE("member data commands", "[param]") {
  // clang-format off
  (void)param(name, description, ptr_to_member, parse, format, validate);
  (void)param(name,              ptr_to_member, parse, format, validate);
  (void)param(name, description, ptr_to_member,                validate);
  (void)param(name,              ptr_to_member,                validate);
  (void)param(name, description, ptr_to_member, parse, format);
  (void)param(name,              ptr_to_member, parse, format);
  (void)param(name, description, ptr_to_member);
  (void)param(name,              ptr_to_member);

  (void)param<ptr_to_member>(description, parse, format, validate);
  (void)param<ptr_to_member>(             parse, format, validate);
  (void)param<ptr_to_member>(description,                validate);
  (void)param<ptr_to_member>(                            validate);
  (void)param<ptr_to_member>(description, parse, format);
  (void)param<ptr_to_member>(             parse, format);
  (void)param<ptr_to_member>(description);
  (void)param<ptr_to_member>(           );
  // clang-format on
}

TEST_CASE("const member data commands", "[param]") {
  // clang-format off
  (void)param(name, description, const_ptr_to_member, format);
  (void)param(name,              const_ptr_to_member, format);
  (void)param(name, description, ptr_to_member,       format);
  (void)param(name,              ptr_to_member,       format);
  (void)param(name, description, const_ptr_to_member);
  (void)param(name,              const_ptr_to_member);
  (void)param(name, description, cli::as_const(ptr_to_member));
  (void)param(name,              cli::as_const(ptr_to_member));


  (void)param<const_ptr_to_member>(description, format);
  (void)param<const_ptr_to_member>(             format);
  (void)param<const_ptr_to_member>(description);
  (void)param<const_ptr_to_member>();

  (void)param<ptr_to_member>(description, format);
  (void)param<ptr_to_member>(             format);
  (void)param<cli::as_const(ptr_to_member)>(description);
  (void)param<cli::as_const(ptr_to_member)>();
  // clang-format on
}

TEST_CASE("param<T>", "[param]") {
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

  auto p = param<int>("i"_sc, "i desc"_sc, get_var, set_var, validate_var);
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

  auto const_p = param<int>("i"_sc, "i desc"_sc, get_var);

  exec_result = const_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);

  exec_result = const_p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "21");

  auto write_only_p = param<int>("i"_sc, "i desc"_sc, set_var, validate_var);

  exec_result = write_only_p.execute({}, {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::cant_read_param);
  exec_result = write_only_p.execute("=10", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(var == 10);

  auto inv_get_var_p = param<int>("i"_sc, "i desc"_sc, invalid_get_var);
  exec_result = inv_get_var_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  auto inv_set_var_p = param<int>("i"_sc, "i desc"_sc, invalid_set_var);
  exec_result = inv_set_var_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  auto bogus_format_p = param<int>("i"_sc, "i desc"_sc, get_var, bogus_format);
  exec_result = bogus_format_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::format_error);
  REQUIRE(exec_result.error() == cli::Error::buffer_overflow);

  auto bogus_parse_p = param<int>("i"_sc, "i desc"_sc, set_var, bogus_parse);
  exec_result = bogus_parse_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::unknown);
}

TEST_CASE("param(t)", "[param]") {
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

  auto p = param("i"_sc, "i desc"_sc, var, get_var, set_var, validate_var);
  REQUIRE(p.name == "i"_sc);
  REQUIRE(p.description == "i desc"_sc);
  REQUIRE(p.type == "int"_sc);
  auto exec_result = p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "42");

  exec_result = p.execute("=21", {buffer, 10});

  REQUIRE(exec_result);
  REQUIRE(var == 21);

  // validation failure
  exec_result = p.execute("=100", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_value);

  // no value given
  exec_result = p.execute("=", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_value);

  // parse error assignment misssing
  exec_result = p.execute("k", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_assignment);

  // parse error too many characters
  exec_result = p.execute("=100 k", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::unexpected_characters);

  auto const_p = param("i"_sc, "i desc"_sc, var, get_var, invalid_set_var);

  // cant set because of set error
  exec_result = const_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  // can get the result
  exec_result = const_p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "21");

  auto write_only_p =
    param("i"_sc, "i desc"_sc, var, invalid_get_var, set_var, validate_var);

  // cant get because of get error
  exec_result = write_only_p.execute({}, {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  // can set
  exec_result = write_only_p.execute("=10", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(var == 10);

  // cant get because of format error
  auto bogus_p = param("i"_sc, "i desc"_sc, var, bogus_parse, bogus_format);
  exec_result = bogus_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::format_error);
  REQUIRE(exec_result.error() == cli::Error::buffer_overflow);

  // cant set because of parse error
  exec_result = bogus_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::unknown);
}

TEST_CASE("param<t>()", "[param]") {
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

  auto p = param<var>("i desc"_sc, get_var, set_var, validate_var);
  REQUIRE(p.name == "var"_sc);
  REQUIRE(p.description == "i desc"_sc);
  REQUIRE(p.type == "int"_sc);
  auto exec_result = p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "42");

  exec_result = p.execute("=21", {buffer, 10});

  REQUIRE(exec_result);
  REQUIRE(var == 21);

  // validation failure
  exec_result = p.execute("=100", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_value);

  // no value given
  exec_result = p.execute("=", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_value);

  // parse error assignment misssing
  exec_result = p.execute("k", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::expected_assignment);

  // parse error too many characters
  exec_result = p.execute("=100 k", {buffer, 10});
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::unexpected_characters);

  auto const_p = param<var>("i desc"_sc, get_var, invalid_set_var);

  // cant set because of set error
  exec_result = const_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  // can get the result
  exec_result = const_p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "21");

  auto write_only_p =
    param<var>("i desc"_sc, invalid_get_var, set_var, validate_var);

  // cant get because of get error
  exec_result = write_only_p.execute({}, {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);

  // can set
  exec_result = write_only_p.execute("=10", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(var == 10);

  // cant get because of format error
  auto bogus_p = param<var>("i desc"_sc, bogus_parse, bogus_format);
  exec_result = bogus_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::format_error);
  REQUIRE(exec_result.error() == cli::Error::buffer_overflow);

  // cant set because of parse error
  exec_result = bogus_p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::parse_error);
  REQUIRE(exec_result.error() == cli::Error::unknown);
}

TEST_CASE("param(const t)", "[param]") {
  static constexpr int var = 42;

  char buffer[10]{};
  auto get_var = [](int &i_) {
    i_ = var;
    return cli::Error::none;
  };

  auto invalid_get_var = [](int &) {
    return cli::Error::invalid_sequence_value;
  };

  auto bogus_format = [](cli::View<char>, int) -> cli::format::FormatResult {
    return cli::Error::buffer_overflow;
  };

  auto p = param("i"_sc, "i desc"_sc, var, get_var);

  // cant set because of set error
  auto exec_result = p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);

  // can get the result
  exec_result = p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "42");

  // cant get because of format error
  auto bogus_p = param("i"_sc, "i desc"_sc, var, bogus_format);
  exec_result = bogus_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::format_error);
  REQUIRE(exec_result.error() == cli::Error::buffer_overflow);

  // cant get because of invalid get
  auto invalid_get_p = param("i"_sc, "i desc"_sc, var, invalid_get_var);
  exec_result = invalid_get_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);
}

#if !defined(_MSC_VER)
TEST_CASE("param<const t>()", "[param]") {
  static constexpr int var = 42;

  char buffer[10]{};
  auto get_var = [](int &i_) {
    i_ = var;
    return cli::Error::none;
  };

  auto invalid_get_var = [](int &) {
    return cli::Error::invalid_sequence_value;
  };

  auto bogus_format = [](cli::View<char>, int) -> cli::format::FormatResult {
    return cli::Error::buffer_overflow;
  };

  auto p = param<var>("i desc"_sc, get_var);

  // cant set because of set error
  auto exec_result = p.execute("=21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);

  // can get the result
  exec_result = p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "42");

  // cant get because of format error
  auto bogus_p = param<var>("i desc"_sc, bogus_format);
  exec_result = bogus_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::format_error);
  REQUIRE(exec_result.error() == cli::Error::buffer_overflow);

  // cant get because of invalid get
  auto invalid_get_p = param<var>("i desc"_sc, invalid_get_var);
  exec_result = invalid_get_p.execute("", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::get_error);
  REQUIRE(exec_result.error() == cli::Error::invalid_sequence_value);
}
#endif

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
  auto p = param("settings"_sc,
                 "description"_sc,
                 settings,
                 callback,
                 validate,
                 cli::params::recursive);
  using cli::get;
  auto &var = get<0>(p);
  auto &c = get<1>(p);
  auto &subsettings = get<2>(p);
  REQUIRE(subsettings.name == "subsettings"_sc);
  auto &i = get<0>(subsettings);
  REQUIRE(i.name == "i"_sc);
  auto &subsubsettings = get<1>(subsettings);
  REQUIRE(subsubsettings.name == "subsubsettings"_sc);
  auto &a = get<0>(subsubsettings);
  REQUIRE(a.name == "a"_sc);

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

TEST_CASE("const recursive param") {
  static constinit Settings settings;
  (void)param(
    name, description, cli::as_const(settings), cli::params::recursive);
  auto p = param(name, cli::as_const(settings), cli::params::recursive);
  auto exec_result = p.execute(" = 10", {});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);
}
