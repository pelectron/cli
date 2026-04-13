#include "cli/ctti.hpp"
#include "cli/util.hpp"
#include <iostream>
#include <ostream>
#include <string>

struct S1 {};

template <char c> struct S2 {};

template <int a> struct S3 {};

struct F1 {
  int a;
};

struct F2 {
  int i;
  char c;
};

using F1Info = cli::ctti::StructInfo<F1>;

using F2Info = cli::ctti::StructInfo<F2>;

std::ostream &operator<<(std::ostream &s, cli::CharView str) {
  for (char ch : str)
    s << ch;
  return s;
}

template <class Field, class... Fields>
void print_fields(cli::TypeList<Field, Fields...>, std::size_t indent = 0) {
  if constexpr (sizeof...(Fields) == 0) {
    std::cout << std::string(2 * indent, ' ')
              << cli::CharView(typename Field::name{}).data() << ": "
              << cli::CharView(cli::ctti::name<typename Field::type>()) << "\n";
  } else {
    std::cout << std::string(2 * indent, ' ')
              << cli::CharView(typename Field::name{}).data() << ": "
              << cli::CharView(cli::ctti::name<typename Field::type>()).data()
              << "\n";
    print_fields(cli::TypeList<Fields...>{}, indent);
  }
}

template <class T> void print_struct(std::size_t indent = 0) {
  using Info = cli::ctti::StructInfo<T>;
  std::cout << std::string(2 * indent, ' ')
            << cli::CharView(cli::ctti::name<T>()).data() << ":\n";
  // print_fields(typename Info::fields{}, ++indent);
  std::cout << std::endl;
}

void free_func(int i) {}

constexpr cli::CharView names[]{
    cli::ctti::name<int>(),
    cli::ctti::name<S1>(),
    cli::ctti::name<S2<'c'>>(),
    cli::ctti::name<S3<1234>>(),
    cli::ctti::name<decltype(free_func)>(),
};

using int_t = cli::ctti::TypeInfo<int>;
using void_t = cli::ctti::TypeInfo<void>;
using F1_t = cli::ctti::TypeInfo<F1>;
using F2_t = cli::ctti::TypeInfo<F2>;
namespace n {
struct Type {};
} // namespace n
using cli::operator""_sc;

struct S {
  int a;
  void apply(int i) {}
};
static constexpr auto s = cli::ctti::dtl::name_impl<int>();

static constexpr auto s2 = cli::ctti::dtl::member_name_impl<&S::apply>();
static_assert(F1_t::name{} == "F1"_sc);
static_assert(F2_t::name{} == "F2"_sc);
static_assert(int_t::name{} == "int"_sc);
static_assert(void_t::name{} == "void"_sc);
static_assert(cli::ctti::TypeInfo<n::Type>::name{} == "n::Type"_sc);

int main() {
  std::cout << cli::ctti::dtl::name_impl<int>() << std::endl;
  std::cout << cli::ctti::dtl::name_impl<void>() << std::endl;
  std::cout << cli::ctti::dtl::name_impl<F2>() << std::endl;
  std::cout << s2 << std::endl;
  print_struct<F1>();
  print_struct<F2>();

  for (const auto &name : names) {
    std::cout << name << std ::endl;
  }

  std::cout << "\nto_tuple(F2{.i=10,.c='c'}):\n";
  cli::for_each(
      [](const auto &field) {
        std::cout <<
            typename std::remove_cvref_t<decltype(field)>::name{}.data() << ": "
                  << field.value << std::endl;
      },
      cli::ctti::to_tuple(F2{.i = 10, .c = 'c'}));

  F2 f2{.i = 10, .c = 'x'};
  auto tup = cli::ctti::to_tuple(f2);
  F2 f3 = cli::ctti::from_tuple<F2>(tup);
  std::cout << "\nto_tuple(f2):\n";
  cli::for_each(
      [](const auto &field) {
        std::cout <<
            typename std::remove_cvref_t<decltype(field)>::name{}.data() << ": "
                  << field.value << std::endl;
      },
      tup);
  std::cout << "\nfrom_tuple(to_tuple(f2)):\n";
  std::cout << "i: " << f3.i << "\nc: " << f3.c << std::endl;
}
