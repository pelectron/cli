#include "catch2/catch_test_macros.hpp"
#include "cli/format.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("format::StringView") {
  std::string buffer(255, 0);
  using Formatter = cli::format::StringView<cli::CharView, char>;
  Formatter format;
  SECTION("empty string") {
    auto res = format({buffer.data(), buffer.size()}, {});
    REQUIRE(res.size_written == 2);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\"\""));
  }
  SECTION("empty string with quotes") {
    auto res = format({buffer.data(), buffer.size()}, "\"\"");
    REQUIRE(res.size_written == 2);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\"\""));
  }
  SECTION("single quote") {
    auto res = format({buffer.data(), buffer.size()}, "\"");
    REQUIRE(res.size_written == 2);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\\\""));
  }
  SECTION("single space") {
    auto res = format({buffer.data(), buffer.size()}, " ");
    REQUIRE(res.size_written == 3);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\" \""));
  }
  SECTION("no spaces") {
    auto res = format({buffer.data(), buffer.size()}, "hello");
    REQUIRE(res.size_written == 5);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("hello"));
  }
  SECTION("no spaces with '\"'") {
    auto res = format({buffer.data(), buffer.size()}, "hello\"world");
    REQUIRE(res.size_written == 11);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("hello\"world"));
  }
  SECTION("with spaces") {
    auto res = format({buffer.data(), buffer.size()}, "hello world");
    REQUIRE(res.size_written == 13);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\"hello world\""));
  }
  SECTION("with spaces and '\"'") {
    SECTION("single quote") {
      auto res = format({buffer.data(), buffer.size()}, "hello \"world");
      REQUIRE(res.size_written == 15);
      buffer.resize(res.size_written);
      REQUIRE(buffer == std::string("\"hello \\\"world\""));
    }
    SECTION("double quote") {
      auto res = format({buffer.data(), buffer.size()}, "hello \"world\"");
      REQUIRE(res.size_written == 17);
      buffer.resize(res.size_written);
      REQUIRE(buffer == std::string("\"hello \\\"world\\\"\""));
    }
  }
}

TEST_CASE("format::String") {
  std::string buffer(255, 0);
  using Formatter = cli::format::String<std::string, char>;
  Formatter format;
  SECTION("empty string") {
    auto res = format({buffer.data(), buffer.size()}, {});
    REQUIRE(res.size_written == 2);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\"\""));
  }
  SECTION("empty string with quotes") {
    auto res = format({buffer.data(), buffer.size()}, "\"\"");
    REQUIRE(res.size_written == 2);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\"\""));
  }
  SECTION("single quote") {
    auto res = format({buffer.data(), buffer.size()}, "\"");
    REQUIRE(res.size_written == 2);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\\\""));
  }
  SECTION("single space") {
    auto res = format({buffer.data(), buffer.size()}, " ");
    REQUIRE(res.size_written == 3);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\" \""));
  }
  SECTION("no spaces") {
    auto res = format({buffer.data(), buffer.size()}, "hello");
    REQUIRE(res.size_written == 5);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("hello"));
  }
  SECTION("no spaces with '\"'") {
    auto res = format({buffer.data(), buffer.size()}, "hello\"world");
    REQUIRE(res.size_written == 11);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("hello\"world"));
  }
  SECTION("with spaces") {
    auto res = format({buffer.data(), buffer.size()}, "hello world");
    REQUIRE(res.size_written == 13);
    buffer.resize(res.size_written);
    REQUIRE(buffer == std::string("\"hello world\""));
  }
  SECTION("with spaces and '\"'") {
    SECTION("single quote") {
      auto res = format({buffer.data(), buffer.size()}, "hello \"world");
      REQUIRE(res.size_written == 15);
      buffer.resize(res.size_written);
      REQUIRE(buffer == std::string("\"hello \\\"world\""));
    }
    SECTION("double quote") {
      auto res = format({buffer.data(), buffer.size()}, "hello \"world\"");
      REQUIRE(res.size_written == 17);
      buffer.resize(res.size_written);
      REQUIRE(buffer == std::string("\"hello \\\"world\\\"\""));
    }
  }
}

TEST_CASE("format View<const CharT>") {
  char buffer[32]{};
  cli::format::Format<cli::View<const char>, char> format;
  cli::format::FormatResult res = format({buffer, 32}, "hello");
  REQUIRE(res);
  REQUIRE(res.size_written == 5);
  REQUIRE(cli::View{buffer, 5} == cli::View{"hello"});
  REQUIRE(buffer[6] == 0);

  SECTION("buffer too small") {
    res = format({buffer, 4}, "hello");
    REQUIRE_FALSE(res);
  }
}

TEST_CASE("format View<CharT>") {
  char buffer[32]{};
  char input[]{"hello"};
  cli::format::Format<cli::View<const char>, char> format;
  cli::format::FormatResult res = format({buffer, 32}, {input, 5});
  REQUIRE(res);
  REQUIRE(res.size_written == 5);
  REQUIRE(cli::View{buffer, 5} == cli::View{"hello"});
  REQUIRE(buffer[6] == 0);

  SECTION("buffer too small") {
    res = format({buffer, 4}, {input, 5});
    REQUIRE_FALSE(res);
  }
}
