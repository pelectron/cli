#include "cli/parse.hpp"
#include "stringify.hpp"

#include <catch2/catch_all.hpp>

std::string to_string(cli::View<const char> s) { return {s.data(), s.size()}; }

TEST_CASE("parse::Parse<bool>", "[parse][Bool]") {
  constexpr cli::parse::Parse<bool, char> parse;

  SECTION("true") {
    SECTION("no rest") {
      const auto res = parse("true");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("true rest");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("false") {
    SECTION("no rest") {
      const auto res = parse("false");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("false rest");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("TRUE") {
    SECTION("no rest") {
      const auto res = parse("TRUE");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("TRUE rest");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("FALSE") {
    SECTION("no rest") {
      const auto res = parse("FALSE");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("FALSE rest");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("1") {
    SECTION("no rest") {
      const auto res = parse("1");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("1 rest");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("0") {
    SECTION("no rest") {
      const auto res = parse("0");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("0 rest");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("yes") {
    SECTION("no rest") {
      const auto res = parse("yes");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("yes rest");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("no") {
    SECTION("no rest") {
      const auto res = parse("no");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("no rest");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("y") {
    SECTION("no rest") {
      const auto res = parse("y");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("y rest");
      REQUIRE(res);
      REQUIRE(res.value == true);
      REQUIRE(res.rest == " rest");
    }
  }

  SECTION("n") {
    SECTION("n rest") {
      const auto res = parse("n");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest.size() == 0);
    }
    SECTION("rest") {
      const auto res = parse("n rest");
      REQUIRE(res);
      REQUIRE(res.value == false);
      REQUIRE(res.rest == " rest");
    }
  }
}
