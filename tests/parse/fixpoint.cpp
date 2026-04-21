#include "fixpoint.hpp"
#include "catch2/catch_test_macros.hpp"
#include "cli/parse.hpp"
#include "cli/traits.hpp"
#include "cli/util.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>
using FP1 = psm::signed_fixed<16, 16>;
using FP2 = psm::signed_fixed<16, 16>;
using FP3 = psm::unsigned_fixed<15, 16>;
using FP4 = psm::unsigned_fixed<16, 12>;

template<class F>
struct FixpointTestVector {
  F value;
  std::string input;
  std::string rest;
};

#define TV1(value)                                                             \
  FixpointTestVector<TestType> {                                               \
    value, #value, {}                                                          \
  }

TEST_CASE("parse::FixPoint fixed<1,1>", "[parse][fixpoint]") {
  SECTION("signed") {
    REQUIRE(cli::parse::Parse<psm::signed_fixed<1, 1>>{}("0").value.value() ==
            0);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<1, 1>>{}("0.0").value.value() ==
            0);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<1, 1>>{}("-0.5").value.value() ==
      0b11);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<1, 1>>{}("0.5").value.value() ==
            0b01);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<1, 1>>{}("-1.0").value.value() ==
      0b10);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<1, 1>>{}("-0.50000").value.value() ==
      0b11);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<1, 1>>{}("0.500000000000")
              .value.value() == 0b01);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<1, 1>>{}("-00001.0").value.value() ==
      0b10);
  }

  SECTION("unsigned") {
    REQUIRE(cli::parse::Parse<psm::unsigned_fixed<1, 1>>{}("0").value.value() ==
            0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<1, 1>>{}("0.0").value.value() == 0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<1, 1>>{}("0.5").value.value() == 1);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<1, 1>>{}("1.0").value.value() ==
      0b10);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<1, 1>>{}("1.5").value.value() ==
      0b11);
  }
}

TEST_CASE("parse::FixPoint fixed<16,8>", "[parse][fixpoint]") {
  SECTION("signed") {
    REQUIRE(cli::parse::Parse<psm::signed_fixed<16, 8>>{}("0").value.value() ==
            0);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<16, 8>>{}("0.0").value.value() == 0);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<16, 8>>{}("32767.99609375")
              .value.value() == 0x7FFFFFu);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<16, 8>>{}("-32768.0").value.value() ==
      0x800000u);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<16, 8>>{}("-1.0").value.value() ==
      0xFFFF00u);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<16, 8>>{}("-0.00390625")
              .value.value() == 0xFFFFFFu);
  }

  SECTION("unsigned") {
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<16, 8>>{}("0").value.value() == 0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<16, 8>>{}("0.0").value.value() ==
      0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<16, 8>>{}("1.0").value.value() ==
      0x100u);
    REQUIRE(cli::parse::Parse<psm::unsigned_fixed<16, 8>>{}("65535.99609375")
              .value.value() == 0xFFFFFFu);
    REQUIRE(cli::parse::Parse<psm::unsigned_fixed<16, 8>>{}("32767.9980469")
              .value.value() == 0x7FFFFFu);
  }
}

TEST_CASE("parse::FixPoint fixed<16,16>", "[parse][fixpoint]") {
  SECTION("signed") {
    REQUIRE(cli::parse::Parse<psm::signed_fixed<16, 16>>{}("0").value.value() ==
            0);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<16, 16>>{}("0.0").value.value() == 0);
    CHECK(cli::parse::Parse<psm::signed_fixed<16, 16>>{}("32767.99998474121")
            .value.value() == 0x7fffffffu);
    CHECK(
      cli::parse::Parse<psm::signed_fixed<16, 16>>{}("32768.0").value.value() ==
      0x7fffffffu);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<16, 16>>{}("-32768.0")
              .value.value() == 0x80000000u);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<16, 16>>{}("-1.0").value.value() ==
      0xFFFF0000u);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<16, 16>>{}("-0.0000152587890625")
        .value.value() == 0xFFFFFFFFu);
  }

  SECTION("unsigned") {
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<16, 16>>{}("0").value.value() == 0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<16, 16>>{}("0.0").value.value() ==
      0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<16, 16>>{}("1.0").value.value() ==
      0x10000u);
    REQUIRE(cli::parse::Parse<psm::unsigned_fixed<16, 16>>{}("32768.0")
              .value.value() == 0x80000000u);
    CHECK(cli::parse::Parse<psm::unsigned_fixed<16, 16>>{}("32767.99998474121")
            .value.value() == 0x7FFFFFFFu);
    CHECK(cli::parse::Parse<psm::unsigned_fixed<16, 16>>{}("65535.99998474121")
            .value.value() == 0xFFFFFFFFu);
  }
}

TEST_CASE("parse::FixPoint fixed<3,21>", "[parse][fixpoint]") {
  SECTION("signed") {
    REQUIRE(cli::parse::Parse<psm::signed_fixed<3, 21>>{}("0").value.value() ==
            0);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<3, 21>>{}("0.0").value.value() == 0);
    CHECK(cli::parse::Parse<psm::signed_fixed<3, 21>>{}("3.999999523162842")
            .value.value() == 0x7fffffu);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<3, 21>>{}("-4").value.value() ==
            0x800000u);
    REQUIRE(
      cli::parse::Parse<psm::signed_fixed<3, 21>>{}("-1.0").value.value() ==
      0xe00000u);
    REQUIRE(cli::parse::Parse<psm::signed_fixed<3, 21>>{}("0.0001220703125")
              .value.value() == 0x100u);
  }

  SECTION("unsigned") {
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<3, 21>>{}("0").value.value() == 0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<3, 21>>{}("0.0").value.value() ==
      0);
    REQUIRE(
      cli::parse::Parse<psm::unsigned_fixed<3, 21>>{}("1.0").value.value() ==
      0x200000u);
    CHECK(cli::parse::Parse<psm::unsigned_fixed<3, 21>>{}("7.999999523162842")
            .value.value() == 0xffffffu);
    CHECK(cli::parse::Parse<psm::unsigned_fixed<3, 21>>{}("3.999999523162842")
            .value.value() == 0x7FFFFFu);
  }
}
