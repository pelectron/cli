#include <cli/util.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("util dtl::min") {
  SECTION("unsigned") { REQUIRE(cli::dtl::min(1u, 5u) == 1u); }
  SECTION("signed") {
    REQUIRE(cli::dtl::min(1, 5) == 1);
    REQUIRE(cli::dtl::min(-1, -5) == -5);
  }
  SECTION("mixed") {
    REQUIRE(cli::dtl::min(-1, 5u) == -1);
    REQUIRE(cli::dtl::min(1, 5u) == 1);
    REQUIRE(cli::dtl::min(10, 5u) == 5);
    REQUIRE(cli::dtl::min(5u, -1) == -1);
    REQUIRE(cli::dtl::min(5u, 1) == 1);
    REQUIRE(cli::dtl::min(5u, 10) == 5);
  }
}

TEST_CASE("util dtl::max") {
  REQUIRE(cli::dtl::max(1) == 1);
  REQUIRE(cli::dtl::max(1, 2) == 2);
  REQUIRE(cli::dtl::max(1, 2, 3) == 3);
  REQUIRE(cli::dtl::max(1, 3, 2) == 3);
  REQUIRE(cli::dtl::max(3, 1, 2) == 3);
  REQUIRE(cli::dtl::max(3, 2, 1) == 3);
  REQUIRE(cli::dtl::max(2, 3, 1) == 3);
  REQUIRE(cli::dtl::max(2, 1, 3) == 3);
}

struct T {
  int mem_fun(int i) { return i; }
  int mem_fun_ne(int i) noexcept { return i; }
  int mem_fun_c(int i) const { return i; }
  int mem_fun_c_ne(int i) const noexcept { return i; }
};

TEST_CASE("util MemFunBinder") {
  SECTION("const") {
    const T t;
    cli::MemFunBinder b{t, &T::mem_fun_c};
    REQUIRE(b(5) == 5);
  }
  SECTION("const noexcept") {
    const T t;
    cli::MemFunBinder b{t, &T::mem_fun_c_ne};
    REQUIRE(b(5) == 5);
  }
  SECTION("non const") {
    T t;
    cli::MemFunBinder b{t, &T::mem_fun};
    REQUIRE(b(5) == 5);
  }
  SECTION("non const noexcept") {
    T t;
    cli::MemFunBinder b{t, &T::mem_fun_ne};
    REQUIRE(b(5) == 5);
  }
}

TEST_CASE("util for_each") {
  std::vector<int> vec;
  auto f = [&vec](int i, int k) { vec.push_back(i + k); };
  cli::Tuple t{1, 2, 3};
  int k = 5;

  cli::for_each(f, t, k);

  REQUIRE(vec.size() == 3);
  REQUIRE(vec[0] == 6);
  REQUIRE(vec[1] == 7);
  REQUIRE(vec[2] == 8);
}

TEST_CASE("util apply") {
  auto f = [](int i, int j, int k) -> int { return i + j + k; };
  cli::Tuple t{1, 2, 3};
  int ret = cli::apply(t, f);
  REQUIRE(ret == 6);
}
