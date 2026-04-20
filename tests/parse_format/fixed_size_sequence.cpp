#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "common.hpp"

#include <catch2/catch_test_macros.hpp>

using Parse = cli::parse::DefaultParse<std::array<int, 4>, char>;
using Format = cli::format::DefaultFormat<std::array<int, 4>, char>;

TEST_CASE("parse-format fixed size sequence") {
  Format format;
  Parse parse;
  char buffer[32]{};
  std::array<int, 4> arr{1, 2, 3, 4};
  auto fmt_res = format({buffer, 32}, arr);
  REQUIRE(fmt_res);
  REQUIRE(std::string(buffer, fmt_res.size_written) == "[1, 2, 3, 4]");
  auto parse_res = parse({buffer, fmt_res.size_written});
  REQUIRE(parse_res);
  REQUIRE(parse_res.value == arr);
}
