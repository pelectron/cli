/**
 * @file cli.cpp
 * @brief This file contains the main example for cli. It can be compiled on the
 * PC and played around with.
 */

#include "cli/sim.hpp"

// the parameters and functions used in this example
constinit static bool enable = false;
constinit static int virtual_ = 0;
constinit static bool enable_virtual = false;
constinit static int enable_opts = 0xFF;

constexpr int free1(int param) { return param; }
void free3() { return; }
void free4(int) { return; }
int free5(void) { return -5; }

constinit static int foo = 100;

void set_foo(int i) { foo = i; }

int get_foo() { return foo; }

cli::Error foo_getter(int &i) {
  i = get_foo();
  return cli::Error::none;
}

cli::Error foo_setter(int i) {
  set_foo(i);
  return cli::Error::none;
}

bool validate_foo(int i) { return i >= 0 and i <= 200; }

static constinit struct S {
  cli::Error free2(int x) { return {}; }
} s;

struct MyFunctor {
  cli::Error operator()(int x, char c) { return {}; }
};

struct MyFunctor2 {
  cli::Error operator()(int f) { return {}; }
};

static constinit struct Settings {
  int a{};
  int b{};
  char c{};
  void apply() {}
} settings{};

// using declarations for literal operators
using cli::operator""_sc;
using cli::operator""_arg;

// using declarations to not repeat cli:: unnecessarily
using cli::arg;
using cli::func;
using cli::mem_fun;
using cli::param;

int main() {
  // clang-format off
  // the cli object itself
  cli::Cli my_cli = cli::sim::create_cli(
      cli::default_config{},
      param<int>("foo"_sc, 
                "foo description"_sc, 
                &foo_getter, 
                &foo_setter, 
                &validate_foo),
      // functions
      // @{
      // free functions
      func("free1"_sc, &free1, cli::arg("param"_sc)),
      // lambdas without templated call operator
      func("lambda"_sc, 
          [](int i, char c) {},
          "i"_arg, 
          "c"_arg),
      // and any other functor without templated call operator
      func("functor"_sc, MyFunctor{} ,"x"_arg,"c"_arg),
      func(MyFunctor2{}, "f"_arg),
      // member functions
      func("free2"_sc, s, &S::free2, "x"_arg),
    // @}
    // global objects
      param("enable"_sc,"enables stuff"_sc, enable),
      // virtual hierarchies
      param("virtual"_sc,
            "virtual group"_sc, 
            virtual_,
            param("enable"_sc,
                  "virtual enable"_sc, 
                  enable_virtual,
            param("opts"_sc, 
                  "enable options"_sc, 
                  enable_opts))),
      param("settings"_sc,
            "core settings"_sc, 
            settings,
            param("b"_sc, &Settings::b),
            param<&Settings::a>(),
            param("c"_sc, &Settings::c),
            mem_fun<&Settings::apply>())
  );
  // clang-format on

  if (not cli::sim::init())
    return -1;

  my_cli.print();

  while (cli::sim::get_input_and_process(my_cli)) {
  }

  return 0;
}
