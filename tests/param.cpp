#include "catch2/catch_test_macros.hpp"
#include "cli/cli.hpp"
#include "cli/enums.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"

#include <catch2/catch_all.hpp>

using cli::operator""_sc;
constexpr auto name = "name"_sc;
constexpr auto description = "description"_sc;
constexpr auto get = [](int &i) -> cli::Error { return cli::Error::none; };
constexpr auto set = [](const int &i) -> cli::Error {
  return cli::Error::none;
};

constexpr auto parse =
    [](cli::View<const char>) -> cli::parse::ParseResult<int, char> {
  return 0;
};

constexpr auto format = [](cli::View<char>, int) -> cli::format::FormatResult {
  return 0;
};

constexpr auto validate = [](int) -> bool { return true; };

static_assert(cli::format::Formatter<decltype(format)>);
static_assert(cli::parse::Parser<decltype(parse)>);
TEST_CASE("params without object") {
  // base form
  (void)cli::param<int>(name, description, get, set, parse, format, validate);

  // missing one parameter{
  // no validate
  (void)cli::param<int>(name, description, get, set, parse, format);
  // no format and parse
  (void)cli::param<int>(name, description, get, set, validate);
  // no get -> format not used
  (void)cli::param<int>(name, description, set, parse, validate);
  // no set -> parse not used
  (void)cli::param<int>(name, description, get, format, validate);
  //}

  // missing two parameters{
  // no validate and format and parse
  (void)cli::param<int>(name, description, get, set);
  // no validate and get -> format not used
  (void)cli::param<int>(name, description, set, parse);
  // no validate and set -> parse not used
  (void)cli::param<int>(name, description, get, format);

  // no format/parse and get
  (void)cli::param<int>(name, description, set, validate);
  // }

  // missing three parameters {
  // no validate, format/parse and set
  (void)cli::param<int>(name, description, get);
  // no validate, format/parse and get
  (void)cli::param<int>(name, description, set);
  // }
}

TEST_CASE("params with object") {
  int i;
  // base form
  (void)cli::param(name, description, i, get, set, parse, format, validate);

  // missing one parameter{
  // no validate
  (void)cli::param(name, description, i, get, set, parse, format);
  // no format and parse
  (void)cli::param(name, description, i, get, set, validate);
  // no get
  (void)cli::param(name, description, i, set, parse, format, validate);
  // no set
  (void)cli::param(name, description, i, get, parse, format, validate);
  //}

  // missing two parameters{
  // no validate and format/parse
  (void)cli::param(name, description, i, get, set);
  // no validate and get
  (void)cli::param(name, description, i, set, parse, format);
  // no validate and set
  (void)cli::param(name, description, i, get, parse, format);

  // no format/parse and get
  (void)cli::param(name, description, i, set, validate);
  // no format/parse and set
  (void)cli::param(name, description, i, get, validate);
  // no set and get
  (void)cli::param(name, description, i, parse, format, validate);
  // }

  // missing three parameters {
  // no validate, format/parse and set
  (void)cli::param(name, description, i, get);
  // no validate, format/parse and get
  (void)cli::param(name, description, i, set);
  // no get, set and validate
  (void)cli::param(name, description, i, parse, format);
  // no get, set and format/parse
  (void)cli::param(name, description, i, validate);
  // }
  //
  (void)cli::param(name, description, i);
}

TEST_CASE("params with const object") {

  const int i{5};

  // base form
  (void)cli::param(name, description, i, get, format);

  // missing one parameter
  //{
  //  no format
  (void)cli::param(name, description, i, get);
  // no get
  (void)cli::param(name, description, i, format);
  //}

  // missing two parameters
  (void)cli::param(name, description, i);
}

struct S {
  int foo;
  const int k{5};
};

TEST_CASE("member data commands") {
  S s;
  // base form
  (void)cli::param(
      name, description, s,
      cli::param("foo"_sc, "foo mode"_sc, &S::foo, parse, format, validate));

  // no validate
  (void)cli::param(name, description, s,
                   cli::param("foo"_sc, "foo mode"_sc, &S::foo, parse, format));

  // no parse/format
  (void)cli::param(name, description, s,
                   cli::param("foo"_sc, "foo mode"_sc, &S::foo, validate));
  // no parse/format and validate
  (void)cli::param(name, description, s,
                   cli::param("foo"_sc, "foo mode"_sc, &S::foo));
}

TEST_CASE("const member data commands") {
  const S s{};
  // base form
  (void)cli::param(name, description, s,
                   cli::param("foo"_sc, "foo mode"_sc, &S::foo, format));
  (void)cli::param(name, description, s,
                   cli::param("k"_sc, "k mode"_sc, &S::k, format));

  // no parse/format and validate
  (void)cli::param(name, description, s,
                   cli::param("foo"_sc, "foo mode"_sc, &S::foo));

  (void)cli::param(name, description, s,
                   cli::param("k"_sc, "k mode"_sc, &S::k));
}
