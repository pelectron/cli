#include "catch2/catch_test_macros.hpp"
#include "cli/format.hpp"
#include <array>
#include <catch2/catch_all.hpp>

TEST_CASE("format::FixedSizeSequence") {
  using Format = cli::format::DefaultFormat<std::array<int, 4>, char>;
  Format format;
  std::string buffer(256, 0);
  auto res = format({buffer.data(), buffer.size()}, {1, 2, 3, 4});
  REQUIRE(res);
  buffer.resize(res.size_written);
  REQUIRE(buffer == "[1, 2, 3, 4]");
}
