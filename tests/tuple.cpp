#include "cli/tuple.hpp"
#include "elem.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("Tuple::Tuple()") {
  cli::Tuple<Elem, Elem, Elem> t{};
  CHECK(cli::get<0>(t).default_constructed());
  CHECK(cli::get<1>(t).default_constructed());
  CHECK(cli::get<2>(t).default_constructed());
  CHECK_FALSE(cli::get<0>(t).moved());
  CHECK_FALSE(cli::get<1>(t).moved());
  CHECK_FALSE(cli::get<2>(t).moved());
  CHECK_FALSE(cli::get<0>(t).copied());
  CHECK_FALSE(cli::get<1>(t).copied());
  CHECK_FALSE(cli::get<2>(t).copied());
}

TEST_CASE("Tuple::Tuple(Ts...)") {
  cli::Tuple<Elem, Elem, Elem> t{1, 2, 3};
  CHECK_FALSE(cli::get<0>(t).default_constructed());
  CHECK_FALSE(cli::get<1>(t).default_constructed());
  CHECK_FALSE(cli::get<2>(t).default_constructed());
  CHECK_FALSE(cli::get<0>(t).moved());
  CHECK_FALSE(cli::get<1>(t).moved());
  CHECK_FALSE(cli::get<2>(t).moved());
  CHECK_FALSE(cli::get<0>(t).copied());
  CHECK_FALSE(cli::get<1>(t).copied());
  CHECK_FALSE(cli::get<2>(t).copied());
  CHECK(cli::get<0>(t).value() == 1);
  CHECK(cli::get<1>(t).value() == 2);
  CHECK(cli::get<2>(t).value() == 3);
}

TEST_CASE("Tuple::Tuple(const Tuple&)") {
  const cli::Tuple<Elem, Elem, Elem> t{1, 2, 3};
  const cli::Tuple t2{t};
  REQUIRE_FALSE(cli::get<0>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<1>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<2>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<0>(t2).moved());
  REQUIRE_FALSE(cli::get<1>(t2).moved());
  REQUIRE_FALSE(cli::get<2>(t2).moved());
  REQUIRE(cli::get<0>(t2).copied());
  REQUIRE(cli::get<1>(t2).copied());
  REQUIRE(cli::get<2>(t2).copied());
  REQUIRE(cli::get<0>(t2).value() == 1);
  REQUIRE(cli::get<1>(t2).value() == 2);
  REQUIRE(cli::get<2>(t2).value() == 3);
}

TEST_CASE("Tuple::Tuple(Tuple&&)") {
  cli::Tuple<Elem, Elem, Elem> t{1, 2, 3};
  const cli::Tuple t2{std::move(t)};
  REQUIRE_FALSE(cli::get<0>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<1>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<2>(t2).default_constructed());
  REQUIRE(cli::get<0>(t2).moved());
  REQUIRE(cli::get<1>(t2).moved());
  REQUIRE(cli::get<2>(t2).moved());
  REQUIRE_FALSE(cli::get<0>(t2).copied());
  REQUIRE_FALSE(cli::get<1>(t2).copied());
  REQUIRE_FALSE(cli::get<2>(t2).copied());
  REQUIRE(cli::get<0>(t2).value() == 1);
  REQUIRE(cli::get<1>(t2).value() == 2);
  REQUIRE(cli::get<2>(t2).value() == 3);
}

TEST_CASE("Tuple::operator=(const Tuple&)") {
  const cli::Tuple<Elem, Elem, Elem> t{1, 2, 3};
  cli::Tuple<Elem, Elem, Elem> t2{};
  t2 = t;
  REQUIRE_FALSE(cli::get<0>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<1>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<2>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<0>(t2).moved());
  REQUIRE_FALSE(cli::get<1>(t2).moved());
  REQUIRE_FALSE(cli::get<2>(t2).moved());
  REQUIRE(cli::get<0>(t2).copied());
  REQUIRE(cli::get<1>(t2).copied());
  REQUIRE(cli::get<2>(t2).copied());
  REQUIRE(cli::get<0>(t2).value() == 1);
  REQUIRE(cli::get<1>(t2).value() == 2);
  REQUIRE(cli::get<2>(t2).value() == 3);
}

TEST_CASE("Tuple::operator=(Tuple&&)") {
  cli::Tuple<Elem, Elem, Elem> t{1, 2, 3};
  cli::Tuple<Elem, Elem, Elem> t2{};
  t2 = std::move(t);
  REQUIRE_FALSE(cli::get<0>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<1>(t2).default_constructed());
  REQUIRE_FALSE(cli::get<2>(t2).default_constructed());
  REQUIRE(cli::get<0>(t2).moved());
  REQUIRE(cli::get<1>(t2).moved());
  REQUIRE(cli::get<2>(t2).moved());
  REQUIRE_FALSE(cli::get<0>(t2).copied());
  REQUIRE_FALSE(cli::get<1>(t2).copied());
  REQUIRE_FALSE(cli::get<2>(t2).copied());
  REQUIRE(cli::get<0>(t2).value() == 1);
  REQUIRE(cli::get<1>(t2).value() == 2);
  REQUIRE(cli::get<2>(t2).value() == 3);
}
