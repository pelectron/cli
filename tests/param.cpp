#include "cli.hpp"
#include "cli/enums.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"

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
TEST_CASE("params without object") {
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

TEST_CASE("params with object") {
  static int i;
  // base form
  {
    static constinit auto p = cli::param(
      name, description, i, getter, setter, parser, formatter, validator);
    (void)p;
  }

  // missing one parameter{
  // no validate
  {
    static constinit auto p =
      cli::param(name, description, i, getter, setter, parser, formatter);
    (void)p;
  }
  // no format and parse
  {
    static constinit auto p =
      cli::param(name, description, i, getter, setter, validator);
    (void)p;
  }
  // no get
  {
    static constinit auto p =
      cli::param(name, description, i, setter, parser, formatter, validator);
    (void)p;
  }
  // no set
  {
    static constinit auto p =
      cli::param(name, description, i, getter, parser, formatter, validator);
    (void)p;
  }
  //}

  // missing two parameters{
  // no validate and format/parse
  {
    static constinit auto p = cli::param(name, description, i, getter, setter);
    (void)p;
  }
  // no validate and get
  {
    static constinit auto p =
      cli::param(name, description, i, setter, parser, formatter);
    (void)p;
  }
  // no validate and set
  {
    static constinit auto p =
      cli::param(name, description, i, getter, parser, formatter);
    (void)p;
  }

  // no format/parse and get
  {
    static constinit auto p =
      cli::param(name, description, i, setter, validator);
    (void)p;
  }
  // no format/parse and set
  {
    static constinit auto p =
      cli::param(name, description, i, getter, validator);
    (void)p;
  }
  // no set and get
  {
    static constinit auto p =
      cli::param(name, description, i, parser, formatter, validator);
    (void)p;
  }
  // }

  // missing three parameters {
  // no validate, format/parse and set
  {
    static constinit auto p = cli::param(name, description, i, getter);
    (void)p;
  }
  // no validate, format/parse and get
  {
    static constinit auto p = cli::param(name, description, i, setter);
    (void)p;
  }
  // no get, set and validate
  {
    static constinit auto p =
      cli::param(name, description, i, parser, formatter);
    (void)p;
  }
  // no get, set and format/parse
  {
    static constinit auto p = cli::param(name, description, i, validator);
    (void)p;
  }
  // }
  //
  {
    static constinit auto p = cli::param(name, description, i);
    (void)p;
  }
}

TEST_CASE("params with const object") {

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

TEST_CASE("member data commands") {
  static S s;
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

TEST_CASE("const member data commands") {
  static const S s{};
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
