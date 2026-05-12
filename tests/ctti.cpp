#include "cli/ctti.hpp"
#include "common.hpp"

#include <catch2/catch_test_macros.hpp>

struct S1 {};

template<char c>
struct S2 {};

template<int a>
struct S3 {};

template<typename T>
struct S4 {};

struct F1 {
  int a;
};

namespace ns {
  struct F3 {};
} // namespace ns

struct F2 {
  int i;
  char c;
  F1 f1_struct;
  void apply() {}
  void apply_c() const {}
  void apply_e() noexcept {}
  void apply_ce() const noexcept {}
  void f(ns::F3) {}
};

using F1Info = cli::ctti::StructInfo<F1>;

using F2Info = cli::ctti::StructInfo<F2>;

using cli::operator""_sc;

TEST_CASE("ctti::name", "[ctti]") {
  CHECK(cli::ctti::name<F1>() == "F1"_sc);
  CHECK(cli::ctti::name<F2>() == "F2"_sc);
  CHECK(cli::ctti::name<S4<int>>() == "S4<int>"_sc);
  CHECK(cli::ctti::name<cli::Error>() == "cli::Error"_sc);
  CHECK(cli::ctti::name<ns::F3>() == "ns::F3"_sc);
}

TEST_CASE("ctti::dtl::member_name", "[ctti]") {
  REQUIRE(cli::ctti::dtl::member_name<F2, 0>() == "i"_sc);
  REQUIRE(cli::ctti::dtl::member_name<F2, 1>() == "c"_sc);
  REQUIRE(cli::ctti::dtl::member_name<F2, 2>() == "f1_struct"_sc);
}

TEST_CASE("ctti::to_tuple", "[ctti]") {
  F2 f2{.i = 5, .c = 'k', .f1_struct = {.a = 10}};
  auto t = cli::ctti::to_tuple(f2);
  REQUIRE(cli::get<0>(t).value == 5);
  REQUIRE(cli::get<1>(t).value == 'k');
  REQUIRE(cli::get<2>(t).value.a == 10);
}

TEST_CASE("ctti::enum_name", "[ctti]") {
  REQUIRE(cli::ctti::enum_name(cli::Error::none) == "none");
  REQUIRE(cli::ctti::enum_name(static_cast<cli::Error>(300)) == "<unknown>");
  REQUIRE(cli::ctti::enum_name(F::A) == "A");
  REQUIRE(cli::ctti::enum_name(F::A | F::B) == "<unknown>");
}

TEST_CASE("ctti::value_name", "[ctti]") {
  REQUIRE(cli::ctti::value_name<123>() == "123"_sc);
  REQUIRE(cli::ctti::value_name<0xFFFF>() == "65535"_sc);
  REQUIRE(cli::ctti::value_name<&F2::apply>() == "apply"_sc);
  REQUIRE(cli::ctti::value_name<&F2::apply_c>() == "apply_c"_sc);
  REQUIRE(cli::ctti::value_name<&F2::apply_ce>() == "apply_ce"_sc);
  REQUIRE(cli::ctti::value_name<&F2::apply_e>() == "apply_e"_sc);
  REQUIRE(cli::ctti::value_name<&F2::f>() == "f"_sc);
}

// template<class Field, class... Fields>
// void print_fields(cli::TypeList<Field, Fields...>, std::size_t indent = 0) {
//   if constexpr (sizeof...(Fields) == 0) {
//     std::cout << std::string(2 * indent, ' ')
//               << cli::CharView(typename Field::name{}).data() << ": "
//               << cli::CharView(cli::ctti::name<typename Field::type>()) <<
//               "\n";
//   } else {
//     std::cout << std::string(2 * indent, ' ')
//               << cli::CharView(typename Field::name{}).data() << ": "
//               << cli::CharView(cli::ctti::name<typename
//               Field::type>()).data()
//               << "\n";
//     print_fields(cli::TypeList<Fields...>{}, indent);
//   }
// }
//
// template<class T>
// void print_struct(std::size_t indent = 0) {
//   using Info = cli::ctti::StructInfo<T>;
//   std::cout << std::string(2 * indent, ' ')
//             << cli::CharView(cli::ctti::name<T>()).data() << ":\n";
//   // print_fields(typename Info::fields{}, ++indent);
//   std::cout << std::endl;
// }
//
// void free_func(int i) {}
//
// constexpr cli::CharView names[]{
//   cli::ctti::name<int>(),
//   cli::ctti::name<S1>(),
//   cli::ctti::name<S2<'c'>>(),
//   cli::ctti::name<S3<1234>>(),
//   cli::ctti::name<decltype(free_func)>(),
// };
//
// using int_t = cli::ctti::TypeInfo<int>;
// using void_t = cli::ctti::TypeInfo<void>;
// using F1_t = cli::ctti::TypeInfo<F1>;
// using F2_t = cli::ctti::TypeInfo<F2>;
// namespace n {
//   struct Type {};
// } // namespace n
// using cli::operator""_sc;
//
// struct S {
//   int a;
//   void apply(int i) {}
// };
// static constexpr auto s = cli::ctti::dtl::name_impl<int>();
//
// static constexpr auto s2 = cli::ctti::dtl::member_name_impl<&S::apply>();
// static_assert(F1_t::name{} == "F1"_sc);
// static_assert(F2_t::name{} == "F2"_sc);
// static_assert(int_t::name{} == "int"_sc);
// static_assert(void_t::name{} == "void"_sc);
// static_assert(cli::ctti::TypeInfo<n::Type>::name{} == "n::Type"_sc);
//
// int main() {
//   std::cout << cli::ctti::dtl::name_impl<int>() << std::endl;
//   std::cout << cli::ctti::dtl::name_impl<void>() << std::endl;
//   std::cout << cli::ctti::dtl::name_impl<F2>() << std::endl;
//   std::cout << s2 << std::endl;
//   print_struct<F1>();
//   print_struct<F2>();
//
//   for (const auto &name : names) {
//     std::cout << name << std ::endl;
//   }
//
//   std::cout << "\nto_tuple(F2{.i=10,.c='c'}):\n";
//   cli::for_each(
//     [](const auto &field) {
//       std::cout << typename
//       std::remove_cvref_t<decltype(field)>::name{}.data()
//                 << ": " << field.value << std::endl;
//     },
//     cli::ctti::to_tuple(F2{.i = 10, .c = 'c'}));
//
//   F2 f2{.i = 10, .c = 'x'};
//   auto tup = cli::ctti::to_tuple(f2);
//   F2 f3 = cli::ctti::from_tuple<F2>(tup);
//   std::cout << "\nto_tuple(f2):\n";
//   cli::for_each(
//     [](const auto &field) {
//       std::cout << typename
//       std::remove_cvref_t<decltype(field)>::name{}.data()
//                 << ": " << field.value << std::endl;
//     },
//     tup);
//   std::cout << "\nfrom_tuple(to_tuple(f2)):\n";
//   std::cout << "i: " << f3.i << "\nc: " << f3.c << std::endl;
// }
