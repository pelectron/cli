#include "catch2/catch_test_macros.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "cli/traits.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>

enum class E : int {
  A = 0,
  B = 1,
  C = 2,
  D = 3
};

namespace cli::traits {

  template<>
  struct enum_traits<E> {
    static constexpr int min = 0;
    static constexpr int max = 3;
    static constexpr bool is_flag = false;
  };
} // namespace cli::traits

using FormatE = cli::format::Format<E, char>;
using ParseE = cli::parse::Parse<E, char>;

using FormatF = cli::format::Format<F, char>;
using ParseF = cli::parse::Parse<F, char>;

TEST_CASE("parse-format enum") {
  FormatE format;
  ParseE parse;
  const std::pair<E, std::string> values[]{
    {E::A, "A"},
    {E::B, "B"},
    {E::C, "C"},
    {E::D, "D"}
  };
  char buffer[32]{};
  for (auto [e, s] : values) {
    auto fmt_res = format({buffer, 32}, e);
    REQUIRE(fmt_res);
    REQUIRE(std::string(buffer, fmt_res.size_written) == s);
    auto parse_res = parse({buffer, 32});
    REQUIRE(parse_res);
    REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
    REQUIRE(parse_res.value == e);
  }
}

TEST_CASE("parse-format flag enum") {
  FormatF format;
  ParseF parse;
  const std::pair<F, std::string> values[]{
    {F::A,               "A"    },
    {F::B,               "B"    },
    {F::C,               "C"    },
    {F::D,               "D"    },
    {F::A | F::B,        "A|B"  },
    {F::A | F::C,        "A|C"  },
    {F::A | F::B | F::C, "A|B|C"},
  };
  char buffer[32]{};
  for (auto [e, s] : values) {
    auto fmt_res = format({buffer, 32}, e);
    REQUIRE(fmt_res);
    REQUIRE(std::string(buffer, fmt_res.size_written) == s);
    auto parse_res = parse({buffer, 32});
    REQUIRE(parse_res);
    REQUIRE(parse_res.rest.size() == 32 - fmt_res.size_written);
    REQUIRE(parse_res.value == e);
  }
}
