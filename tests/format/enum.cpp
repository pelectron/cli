#include "cli/format.hpp"
#include "common.hpp"

#include <catch2/catch_all.hpp>
#include <string>

template<class Enum>
struct EnumTestVector {
  Enum input;
  std::string str;
  std::string buffer = std::string(255, 0);
};

template<class Enum>
void test(EnumTestVector<Enum> &tv) {
  auto res = cli::format::Format<Enum, char>{}(
    cli::View<char>(tv.buffer.data(), tv.buffer.size()), tv.input);
  REQUIRE(res);
  REQUIRE(res.size_written == tv.str.size());
  tv.buffer.resize(res.size_written);
  REQUIRE(tv.buffer == tv.str);
}

#define TV1(value)                                                             \
  EnumTestVector<std::remove_cvref_t<decltype(value)>>(                        \
    value, std::string(#value, sizeof(#value) - 1))

enum WeakEnum {
  WeakEnum_1,
  WeakEnum_2,
  WeakEnum_3,
  WeakEnum_4,
  WeakEnum_5,
  WeakEnum_6,
};

namespace cli::traits {
  template<>
  struct enum_traits<WeakEnum> {
    static constexpr int min = WeakEnum_1;
    static constexpr int max = WeakEnum_6;
    static constexpr bool is_flag = false;
  };
} // namespace cli::traits

TEST_CASE("format::Enum") {
  SECTION("strong enums") {
    using enum cli::Error;

    EnumTestVector<cli::Error> vectors[]{
      TV1(none),
      TV1(unimplemented),
      TV1(implementation_error),
      TV1(cant_set_param),
      TV1(cant_read_param),
      TV1(invalid_cmd),
      TV1(too_many_splits),
      TV1(dual_separators),
      TV1(buffer_overflow),
      TV1(buffer_underflow),
      TV1(incorrect_num_params),
      TV1(too_many_argments),
      TV1(too_few_arguments),
      TV1(invalid_esc_seq),
      TV1(invalid_state),
      TV1(expected_value),
      TV1(unexpected_characters_after_closing_paren),
      TV1(expected_rparen),
      TV1(too_few_characters),
      TV1(invalid_character),
      TV1(unescaped_string),
      TV1(invalid_value)};
    for (auto &tv : vectors) {
      test(tv);
    }
  }
  SECTION("weak enums") {
    EnumTestVector<WeakEnum> vectors[]{
      TV1(WeakEnum_1),
      TV1(WeakEnum_2),
      TV1(WeakEnum_3),
      TV1(WeakEnum_4),
      TV1(WeakEnum_5),
      TV1(WeakEnum_6),
    };
    for (auto &tv : vectors) {
      test(tv);
    }
  }
}

TEST_CASE("format::FlagEnum") {
  using enum F;
  // clang-format off
  EnumTestVector<F> vectors[]{
    TV1(A),
    TV1(B),
    TV1(C),
    TV1(D),
    TV1(A|B),
    TV1(A|C),
    TV1(A|B|C),
    TV1(A|B|C|D),
  };
  // clang-format on
  for (auto &tv : vectors) {
    test(tv);
  }
}
