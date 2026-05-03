#include "cli/string.hpp"

#include "catch2/catch_test_macros.hpp"
#include <catch2/catch_all.hpp>

using namespace cli;

TEST_CASE("View::operator==") {
  REQUIRE(CharView{"hello"} == CharView{"hello"});
  REQUIRE(CharView{} == CharView{});
  REQUIRE(CharView{""} == CharView{""});
  REQUIRE_FALSE(CharView{"hi"} == CharView{"ho"});
}

TEST_CASE("View::operator<") {
  REQUIRE(CharView{"hello"} < CharView{"jello"});
  REQUIRE_FALSE(CharView{"hello"} < CharView{"hello"});
  REQUIRE_FALSE(CharView{"jello"} < CharView{"hello"});
  REQUIRE(CharView{"hella"} < CharView{"hello"});
  REQUIRE(CharView{"hello"} < CharView{"jello"});
  REQUIRE(CharView{"hello"} < CharView{"hello1"});
  REQUIRE_FALSE(CharView{"hello"} < CharView{"hel"});
  REQUIRE(CharView{"hel"} < CharView{"hello"});
  REQUIRE_FALSE(CharView{"hello1"} < CharView{"hello"});
  REQUIRE_FALSE(CharView{"hella"} < CharView{"hella"});
  REQUIRE_FALSE(CharView{"hello"} < CharView{"hella"});
}

TEST_CASE("View::operator>") {
  REQUIRE(CharView{"jello"} > CharView{"hello"});
  REQUIRE_FALSE(CharView{"hello"} > CharView{"hello"});
  REQUIRE_FALSE(CharView{"hello"} > CharView{"jello"});
  REQUIRE(CharView{"hello"} > CharView{"hella"});
  REQUIRE_FALSE(CharView{"hella"} > CharView{"hella"});
  REQUIRE_FALSE(CharView{"hella"} > CharView{"hello"});
  REQUIRE_FALSE(CharView{"hello"} > CharView{"hello1"});
  REQUIRE(CharView{"hello1"} > CharView{"hello"});
  REQUIRE(CharView{"hello"} > CharView{"hel"});
}

TEST_CASE("View::substr") {
  REQUIRE(CharView{"hello"}.substr(0) == "hello");
  REQUIRE(CharView{"hello"}.substr(0, 5) == "hello");
  REQUIRE(CharView{"hello"}.substr(1) == "ello");
  REQUIRE(CharView{"hello"}.substr(5) == CharView{});
}

TEST_CASE("View::starts_with") {
  REQUIRE(CharView{"hello"}.starts_with("h"));
  REQUIRE(CharView{"hello"}.starts_with("he"));
  REQUIRE(CharView{"hello"}.starts_with("hello"));
  REQUIRE_FALSE(CharView{"hello"}.starts_with("H"));
  REQUIRE_FALSE(CharView{"hello"}.starts_with(""));
  REQUIRE(CharView{"hello"}.starts_with(CharView{"h"}));
  REQUIRE(CharView{"hello"}.starts_with(CharView{"he"}));
  REQUIRE(CharView{"hello"}.starts_with(CharView{"hello"}));
  REQUIRE_FALSE(CharView{"hello"}.starts_with(CharView{"H"}));
  REQUIRE_FALSE(CharView{"hello"}.starts_with(CharView{""}));
}

TEST_CASE("View::find_first_of") {
  REQUIRE(CharView{"hello"}.find_first_of('h') == 0);
  REQUIRE(CharView{"hello"}.find_first_of('a') == CharView::npos);
  REQUIRE(CharView{"hello"}.find_first_of('l') == 2);
  REQUIRE(CharView{"hello"}.find_first_of('l', 3) == 3);

  REQUIRE(CharView{"hello"}.find_first_of("abch") == 0);
  REQUIRE(CharView{"hello"}.find_first_of("achb") == 0);
  REQUIRE(CharView{"hello"}.find_first_of("bacd") == CharView::npos);
  REQUIRE(CharView{"hello"}.find_first_of("abl") == 2);
  REQUIRE(CharView{"hello"}.find_first_of("abl", 3) == 3);
}

TEST_CASE("View::find_first_not_of") {
  REQUIRE(CharView{"hello"}.find_first_not_of('h') == 1);
  REQUIRE(CharView{" hello"}.find_first_not_of(" \n\r\t\v\f") == 1);
  REQUIRE(CharView{"hello"}.find_first_not_of('a') == 0);
  REQUIRE(CharView{"hello"}.find_first_not_of('l', 3) == 4);
  REQUIRE(CharView{"hello"}.find_first_not_of('a', 3) == 3);

  REQUIRE(CharView{"hello"}.find_first_not_of("abch") == 1);
  REQUIRE(CharView{"hello"}.find_first_not_of("acb") == 0);
  REQUIRE(CharView{"hello"}.find_first_not_of("abc", 3) == 3);
  REQUIRE(CharView{"hello"}.find_first_not_of("abe", 2) == 2);
}

TEST_CASE("View::find_last_of") {
  REQUIRE(CharView{"hello"}.find_last_of('l') == 3);
  REQUIRE(CharView{"hello"}.find_last_of('h') == 0);
  REQUIRE(CharView{"hello"}.find_last_of('l', 2) == 2);
  REQUIRE(CharView{"hello"}.find_last_of('a') == CharView::npos);
  REQUIRE(CharView{"hello"}.find_last_of('a', 2) == CharView::npos);
  REQUIRE(CharView{"hello"}.find_last_of("l") == 3);
  REQUIRE(CharView{"hello"}.find_last_of("l", 2) == 2);
  REQUIRE(CharView{"hello"}.find_last_of(' ') == CharView::npos);
}

TEST_CASE("View::find_last_not_of") {
  REQUIRE(CharView{"hello"}.find_last_not_of('l') == 4);
  REQUIRE(CharView{"hello"}.find_last_not_of('l', 2) == 1);
  REQUIRE(CharView{"hello"}.find_last_not_of('a') == 4);
  REQUIRE(CharView{"hello"}.find_last_not_of('a', 2) == 2);
  REQUIRE(CharView{"hello"}.find_last_not_of("o") == 3);
  REQUIRE(CharView{"hello"}.find_last_not_of("o", 2) == 2);
  REQUIRE(CharView{"hello"}.find_last_not_of("helo") == CharView::npos);
  REQUIRE(CharView{"BCDEF"}.find_last_not_of("DEF") == 1);
  REQUIRE(CharView{"BCDEFG"}.find_last_not_of("EFG", 3) == 2);
  REQUIRE(CharView{"ABBA"}.find_last_not_of('A') == 2);
  REQUIRE(CharView{"ABBA"}.find_last_not_of('A', 1) == 1);
}

TEST_CASE("View::find") {
  REQUIRE(CharView{"hello"}.find('h') == 0);
  REQUIRE(CharView{"hello"}.find('o') == 4);
  REQUIRE(CharView{"hello"}.find('l') == 2);
  REQUIRE(CharView{"hello"}.find('l', 3) == 3);
  REQUIRE(CharView{"hello"}.find('a') == CharView::npos);

  REQUIRE(CharView{"hello"}.find("he") == 0);
  REQUIRE(CharView{"hello"}.find("ell") == 1);
  REQUIRE(CharView{"hello"}.find("hello") == 0);
  REQUIRE(CharView{"hello"}.find("o") == 4);
  REQUIRE(CharView{"hello"}.find("lo") == 3);
}

TEST_CASE("operator _sc") {
  using cli::operator""_sc;
  SECTION("char") {
    STATIC_REQUIRE(
      std::is_same_v<decltype("hello"_sc),
                     string_constant<char, 'h', 'e', 'l', 'l', 'o'>>);
  }
  SECTION("char16") {
    STATIC_REQUIRE(
      std::is_same_v<decltype(u"hello"_sc),
                     string_constant<char16_t, 'h', 'e', 'l', 'l', 'o'>>);
  }
  SECTION("char32") {
    STATIC_REQUIRE(
      std::is_same_v<decltype(U"hello"_sc),
                     string_constant<char32_t, 'h', 'e', 'l', 'l', 'o'>>);
  }
}
