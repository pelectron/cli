#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "common.hpp"

#include <catch2/catch_test_macros.hpp>

using Parse = cli::parse::Parse<std::vector<int>, char>;
using Format = cli::format::Format<std::vector<int>, char>;

TEST_CASE("parse-format sequence") {
  Format format;
  Parse parse;
  char buffer[32]{};
  std::vector<int> arr{1, 2, 3, 4};
  auto fmt_res = format({buffer, 32}, arr);
  REQUIRE(fmt_res);
  REQUIRE(std::string(buffer, fmt_res.size_written) == "[1, 2, 3, 4]");
  auto parse_res = parse({buffer, fmt_res.size_written});
  REQUIRE(parse_res);
  REQUIRE(parse_res.value == arr);
}
