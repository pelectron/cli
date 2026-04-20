#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "common.hpp"

#include <catch2/catch_test_macros.hpp>

using Parse = cli::parse::Int<int, char, cli::Fmt::normal>;
using Format = cli::format::Int<int, char, cli::Fmt::normal>;
using ParseHex = cli::parse::Int<int, char, cli::Fmt::hex>;
using FormatHex = cli::format::Int<int, char, cli::Fmt::hex>;
using ParseBin = cli::parse::Int<int, char, cli::Fmt::binary>;
using FormatBin = cli::format::Int<int, char, cli::Fmt::binary>;

TEST_CASE("parse-format Int") {
  char buffer[32]{};
  const int val = 10;
  SECTION("normal format") {
    Format format;
    Parse parse;
    auto fmt_res = format({buffer, 32}, val);
    REQUIRE(fmt_res);
    REQUIRE(std::string(buffer, fmt_res.size_written) == "10");
    auto parse_res = parse({buffer, fmt_res.size_written});
    REQUIRE(parse_res);
    REQUIRE(parse_res.value == val);
  }
  SECTION("hex format") {
    FormatHex format;
    ParseHex parse;
    auto fmt_res = format({buffer, 32}, val);
    REQUIRE(fmt_res);
    REQUIRE(std::string(buffer, fmt_res.size_written) == "0xA");
    auto parse_res = parse({buffer, fmt_res.size_written});
    REQUIRE(parse_res);
    REQUIRE(parse_res.value == val);
  }
  SECTION("binary format") {
    FormatBin format;
    ParseBin parse;
    auto fmt_res = format({buffer, 32}, val);
    REQUIRE(fmt_res);
    REQUIRE(std::string(buffer, fmt_res.size_written) == "0b1010");
    auto parse_res = parse({buffer, fmt_res.size_written});
    REQUIRE(parse_res);
    REQUIRE(parse_res.value == val);
  }
}
