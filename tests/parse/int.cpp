#include "cli/enums.hpp"
#include "cli/parse.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>
#include <limits>

using namespace cli;
using namespace cli::parse;

template<class Int>
struct TestVec {
  cli::parse::ParseResult<Int, char> res;
  cli::View<const char> str;

  constexpr TestVec(TestVec &&) = default;
  constexpr TestVec(const TestVec &) = default;
  constexpr TestVec(Int i,
                    cli::View<const char> str,
                    cli::View<const char> rest = {})
    : res(cli::parse::from_value, i, rest), str(str) {}
  constexpr TestVec(cli::Error e,
                    cli::View<const char> str,
                    cli::View<const char> rest)
    : res(cli::parse::from_error, e, rest), str(str) {}
  constexpr TestVec(cli::Error e, cli::View<const char> str)
    : res(cli::parse::from_error, e, str), str(str) {}

  // template<class I>
  // constexpr int_str(int_str<I> other)
  //   : i(other.i), str(other.str) {}
};

template<class I>
TestVec(I, cli::View<const char>, cli::View<const char>) -> TestVec<I>;

template<class I>
TestVec(I i, cli::View<const char> str) -> TestVec<I>;

#define TV(number) {number, #number}

#define HEX_TV(number)                                                         \
  {0x##number, "0x" #number}, { 0x##number, "0X" #number }

#define REST_TV(number, rest) {number, #number rest, rest}

#define FAIL_TV(number)                                                        \
  TestVec<decltype(number)> {                                                  \
    cli::Error::invalid_character, "a" #number, "a" #number                    \
  }

#define FAIL_HEX_TV(number)                                                    \
  {cli::Error::invalid_character, "0xh" #number},                              \
    {cli::Error::invalid_character, "0Xi" #number},                            \
    {cli::Error::invalid_character, "xj" #number},                             \
    {cli::Error::invalid_character, "Xk" #number}, {                           \
    cli::Error::invalid_character, "#l" #number                                \
  }

TEST_CASE("parse::Int<int>") {
  Int<int, char> parser;
  SECTION("well formed strings") {
    static_assert(std::numeric_limits<int>::min() == -2147483648);
    TestVec<int> passing_strings[]{
      TV(0),
      TV(+0),
      TV(-0),
      TV(1),
      TV(+1),
      TV(-1),
      TV(42),
      TV(-42),
      TV(2147483647),
      TV(-2147483647),
      {std::numeric_limits<int>::min(), "-2147483648"},
      HEX_TV(0),
      HEX_TV(1),
      HEX_TV(42),
      HEX_TV(deadbee),
      HEX_TV(042),
      HEX_TV(0deadbee),
      {-1, "0xFFFFFFFF"},
      {-1, "0XFFFFFFFF"},
      {std::numeric_limits<int>::min(), "0x80000000"},
      {std::numeric_limits<int>::min(), "0X80000000"},
      {std::numeric_limits<int>::max(), "0x7FFFFFFF"},
      {std::numeric_limits<int>::max(), "0X7FFFFFFF"},
      REST_TV(10, "x"),
      REST_TV(11, ","),
      REST_TV(14, " "),
      REST_TV(15, " "),
      {0, "0x0k", "k"},
      {1, "0x1k", "k"}
    };

    for (const auto &istr : passing_strings) {
      auto res = parser(istr.str);

      CHECK(res);
      if (not res)
        INFO(istr.str);
      CHECK(res.value == istr.res.value);
      CHECK(res.rest == istr.res.rest);
    }
  }
  SECTION("empty string") { CHECK_FALSE(parser(cli::CharView{})); }

  SECTION("malformed strings") {
    static_assert(std::numeric_limits<int>::min() == -2147483648);
    TestVec<int> fail_strings[]{
      FAIL_TV(0),
      FAIL_TV(+0),
      FAIL_TV(-0),
      FAIL_TV(1),
      FAIL_TV(+1),
      FAIL_TV(-1),
      FAIL_TV(42),
      FAIL_TV(-42),
      FAIL_TV(2147483647),
      FAIL_TV(-2147483647),
      {std::numeric_limits<int>::min(), "-a2147483648", "-a2147483648"},
      FAIL_HEX_TV(0),
      FAIL_HEX_TV(1),
      FAIL_HEX_TV(42),
      FAIL_HEX_TV(deadbee),
      FAIL_HEX_TV(042),
      FAIL_HEX_TV(0deadbee),
      {Error::invalid_character, "0xhFFFFFFFF"},
      {Error::invalid_character, "0XiFFFFFFFF"},
      {Error::invalid_character, "#jFFFFFFFF"},
      {Error::invalid_character, "0xk80000000"},
      {Error::invalid_character, "0Xl80000000"},
      {Error::invalid_character, "#m80000000"},
      {Error::invalid_character, "0xn7FFFFFFF"},
      {Error::invalid_character, "0Xo7FFFFFFF"},
      {Error::invalid_character, "#p7FFFFFFF"},
      {Error::too_few_characters, "-"},
      {Error::too_few_characters, "+"},
    };

    for (const auto &istr : fail_strings) {
      auto res = parser(istr.str);
      CHECK_FALSE(res);
      if (res)
        INFO(istr.str);
    }
  }
}

TEST_CASE("parse::Int<unsigned>") {
  Int<unsigned, const char> parser;
  SECTION("well formed strings") {
    // clang-format off
    TestVec<unsigned> passing_strings[]{
      {0u,                                   "0"         },
      {0u,                                   "+0"        },
      {1u,                                   "1"         },
      {1u,                                   "+1"        },
      {42,                                   "42"        },
      {2147483647,                           "2147483647"},
      {std::numeric_limits<unsigned>::min(), "0"         },
      {std::numeric_limits<unsigned>::max(), "4294967295"},
      {0,                                    "0x0"       },
      {1,                                    "0x1"       },
      {0x42,                                 "0x42"      },
      {0xdeadbee,                            "0xdeadbee" },
      {0x042,                                "0x042"     },
      {0x0deadbee,                           "0x0deadbee"},
      {0x0deadbee,                           "0x0deadbeee", "e"},
      {std::numeric_limits<unsigned>::max(), "42949672951", "1"},
      {std::numeric_limits<unsigned>::max(), "0xFFFFFFFF"},
      {std::numeric_limits<unsigned>::max(), "0XFFFFFFFF"},
      {0x80000000,                           "0x80000000"},
      {0x80000000,                           "0X80000000"},
      {0x7FFFFFFF,                           "0x7FFFFFFF"},
      {0x7FFFFFFF,                           "0X7FFFFFFF"},
      {10,                                   "10x",            "x"   },
      {0,                                    "0x",             "x"   },
      {0x0deadbee,                           "0x0deadbeeRest", "Rest"},
      {std::numeric_limits<unsigned>::max(), "0xFFFFFFFFRest", "Rest"},
      {std::numeric_limits<unsigned>::max(), "0XFFFFFFFFRest", "Rest"},
      {0x80000000,                           "0x80000000Rest", "Rest"},
      {0x80000000,                           "0X80000000Rest", "Rest"},
      {0x7FFFFFFF,                           "0x7FFFFFFFRest", "Rest"},
      {0x7FFFFFFF,                           "0X7FFFFFFFRest", "Rest"},
      {0, "0x0k", "k"},
      {1, "0x1k", "k"}
    };
    // clang-format on

    for (const auto &istr : passing_strings) {
      auto res = parser(istr.str);

      CHECK(res);
      if (not res)
        INFO(istr.str);
      CHECK(res.value == istr.res.value);
      CHECK((res.rest == istr.res.rest));
    }
  }

  SECTION("empty string") {
    auto res = parser(cli::CharView{});
    CHECK_FALSE(res);
    CHECK(res.error == cli::Error::too_few_characters);
  }

  SECTION("malformed strings") {
    TestVec<unsigned> fail_strings[]{
      {Error::invalid_character,  "a0"         },
      {Error::invalid_character,  "a+0"        },
      {Error::invalid_character,  "a-0"        },
      {Error::invalid_character,  "a2147483648"},
      {Error::invalid_character,  "-0"         },
      {Error::invalid_character,  "0xk"        },
      {Error::invalid_character,  "0Xk"        },
      {Error::invalid_character,  "0xk80000000"},
      {Error::invalid_character,  "0Xk80000000"},
      {Error::invalid_character,  "0bk"        },
      {Error::invalid_character,  "0Bk"        },
      {Error::invalid_character,  "0bk80000000"},
      {Error::invalid_character,  "0Bk80000000"},
      {Error::invalid_character,  "0b2"        },
      {Error::invalid_character,  "0B2"        },
      {Error::invalid_character,  "-"          },
      {Error::too_few_characters, "+"          },
    };

    for (const auto &istr : fail_strings) {
      auto res = parser(istr.str);
      CHECK_FALSE(res);
      CHECK(res.rest == istr.res.rest);
      CHECK(res.error == istr.res.error);
      CHECK(res.rest == istr.res.rest);
    }
  }
}
