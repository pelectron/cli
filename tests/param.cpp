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
  [](cli::View<const char>) -> cli::parse::ParseResult<int, char> { return 0; };

constexpr auto format = [](cli::View<char>, int) -> cli::format::FormatResult {
  return 0;
};

constexpr auto validate = [](int) -> bool { return true; };

static_assert(cli::format::Formatter<decltype(format)>);
static_assert(cli::parse::Parser<decltype(parse)>);
TEST_CASE("params without object") {
  // base form
  {
    static constinit auto p =
      cli::param<int>(name, description, get, set, parse, format, validate);
    (void)p;
  }

  // missing one parameter{
  // no validate
  {
    static constinit auto p =
      cli::param<int>(name, description, get, set, parse, format);
    (void)p;
  }
  // no format and parse
  {
    static constinit auto p =
      cli::param<int>(name, description, get, set, validate);
    (void)p;
  }
  // no get -> format not used
  {
    static constinit auto p =
      cli::param<int>(name, description, set, parse, validate);
    (void)p;
  }
  // no set -> parse not used
  {
    static constinit auto p =
      cli::param<int>(name, description, get, format, validate);
    (void)p;
  }
  //}

  // missing two parameters{
  // no validate and format and parse
  {
    static constinit auto p = cli::param<int>(name, description, get, set);
    (void)p;
  }
  // no validate and get -> format not used
  {
    static constinit auto p = cli::param<int>(name, description, set, parse);
    (void)p;
  }
  // no validate and set -> parse not used
  {
    static constinit auto p = cli::param<int>(name, description, get, format);
    (void)p;
  }

  // no format/parse and get
  {
    static constinit auto p = cli::param<int>(name, description, set, validate);
    (void)p;
  }
  // }

  // missing three parameters {
  // no validate, format/parse and set
  {
    static constinit auto p = cli::param<int>(name, description, get);
    (void)p;
  }
  // no validate, format/parse and get
  {
    static constinit auto p = cli::param<int>(name, description, set);
    (void)p;
  }
  // }
}

TEST_CASE("params with object") {
  static int i;
  // base form
  {
    static constinit auto p =
      cli::param(name, description, i, get, set, parse, format, validate);
    (void)p;
  }

  // missing one parameter{
  // no validate
  {
    static constinit auto p =
      cli::param(name, description, i, get, set, parse, format);
    (void)p;
  }
  // no format and parse
  {
    static constinit auto p =
      cli::param(name, description, i, get, set, validate);
    (void)p;
  }
  // no get
  {
    static constinit auto p =
      cli::param(name, description, i, set, parse, format, validate);
    (void)p;
  }
  // no set
  {
    static constinit auto p =
      cli::param(name, description, i, get, parse, format, validate);
    (void)p;
  }
  //}

  // missing two parameters{
  // no validate and format/parse
  {
    static constinit auto p = cli::param(name, description, i, get, set);
    (void)p;
  }
  // no validate and get
  {
    static constinit auto p =
      cli::param(name, description, i, set, parse, format);
    (void)p;
  }
  // no validate and set
  {
    static constinit auto p =
      cli::param(name, description, i, get, parse, format);
    (void)p;
  }

  // no format/parse and get
  {
    static constinit auto p = cli::param(name, description, i, set, validate);
    (void)p;
  }
  // no format/parse and set
  {
    static constinit auto p = cli::param(name, description, i, get, validate);
    (void)p;
  }
  // no set and get
  {
    static constinit auto p =
      cli::param(name, description, i, parse, format, validate);
    (void)p;
  }
  // }

  // missing three parameters {
  // no validate, format/parse and set
  {
    static constinit auto p = cli::param(name, description, i, get);
    (void)p;
  }
  // no validate, format/parse and get
  {
    static constinit auto p = cli::param(name, description, i, set);
    (void)p;
  }
  // no get, set and validate
  {
    static constinit auto p = cli::param(name, description, i, parse, format);
    (void)p;
  }
  // no get, set and format/parse
  {
    static constinit auto p = cli::param(name, description, i, validate);
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
    static constinit auto p = cli::param(name, description, i, get, format);
    (void)p;
  }

  // missing one parameter
  //{
  //  no format
  {
    static constinit auto p = cli::param(name, description, i, get);
    (void)p;
  }
  // no get
  {
    static constinit auto p = cli::param(name, description, i, format);
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
  int foo;
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
      cli::param("foo"_sc, "foo mode"_sc, &S::foo, parse, format, validate));
    (void)p;
  }

  // no validate
  {
    static constinit auto p =
      cli::param(name,
                 description,
                 s,
                 cli::param("foo"_sc, "foo mode"_sc, &S::foo, parse, format));
    (void)p;
  }

  // no parse/format
  {
    static constinit auto p =
      cli::param(name,
                 description,
                 s,
                 cli::param("foo"_sc, "foo mode"_sc, &S::foo, validate));
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
                 cli::param("foo"_sc, "foo mode"_sc, &S::foo, format));
    (void)p;
  }
  {
    static constinit auto p = cli::param(
      name, description, s, cli::param("k"_sc, "k mode"_sc, &S::k, format));
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
