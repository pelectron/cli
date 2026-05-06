#include "cli/ring_buffer.hpp"

#include "common.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("RingBuffer::RingBuffer()") {
  cli::RingBuffer<int, 10> buf;
  REQUIRE(buf.is_empty());
  REQUIRE_FALSE(buf.is_full());
  REQUIRE(buf.capacity() == 10);
  int i = 0;
  REQUIRE_FALSE(buf.pop(i));
}

TEMPLATE_TEST_CASE("RingBufView::RingBufView()",
                   "",
                   (cli::RingBufView<int>),
                   (cli::RingBufView<volatile int>)) {
  int arr[10]{};
  TestType buf(arr, 10);
  REQUIRE(buf.is_empty());
  REQUIRE_FALSE(buf.is_full());
  REQUIRE(buf.capacity() == 10);
  int i = 0;
  REQUIRE_FALSE(buf.pop(i));
}

TEMPLATE_TEST_CASE("RingBufView::push_back()",
                   "",
                   (cli::RingBufView<int>),
                   (cli::RingBufView<volatile int>)) {
  int arr[10]{};
  TestType buf(arr, 10);

  REQUIRE(buf.push_back(1));
  REQUIRE(arr[0] == 1);

  for (int i = 2; i <= 10; ++i)
    REQUIRE(buf.push_back(i));

  REQUIRE(buf.push_back(11) == false);
  REQUIRE(buf.is_full());

  for (int i = 0; i < 10; ++i)
    REQUIRE(arr[i] == i + 1);
}

TEMPLATE_TEST_CASE("RingBufView::pop()",
                   "",
                   (cli::RingBufView<int>),
                   (cli::RingBufView<volatile int>)) {
  int arr[10]{};
  TestType buf(arr, 10);

  int elem = 0;
  SECTION("pop empty") {
    REQUIRE(buf.is_empty());
    REQUIRE_FALSE(buf.pop(elem));
    REQUIRE(elem == 0);
  }

  SECTION("pop not full") {
    for (int i = 1; i <= 5; ++i) {
      buf.push_back(i);
    }

    for (int i = 1; i <= 5; ++i) {
      REQUIRE(buf.pop(elem));
      REQUIRE(elem == i);
    }

    REQUIRE(buf.is_empty());
  }

  SECTION("pop full") {
    for (int i = 1; i <= 10; ++i) {
      buf.push_back(i);
    }

    for (int i = 1; i <= 10; ++i) {
      REQUIRE(buf.pop(elem));
      REQUIRE(elem == i);
    }

    REQUIRE(buf.is_empty());
  }
}

TEMPLATE_TEST_CASE("RingBufView::size()",
                   "",
                   (cli::RingBufView<int>),
                   (cli::RingBufView<volatile int>)) {
  int arr[10]{};
  TestType buf(arr, 10);

  for (int i = 1; i <= 10; ++i) {
    REQUIRE(buf.push_back(i));
    REQUIRE(buf.size() == i);
  }
  buf.push_back(11);
  REQUIRE(buf.size() == 10);
}
