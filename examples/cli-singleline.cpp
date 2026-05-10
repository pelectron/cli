/**
 * @file cli.cpp
 * @example examples/cli.cpp
 * @brief This file contains the main example for cli. It can be compiled on the
 * PC and played around with.
 */

#include "cli/config.hpp"
#include "cli/display.hpp"
#include "cli/sim.hpp"
#include "cli/util.hpp"

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
  cli::Error free2(int /*x*/) { return {}; }
} s_;

struct MyFunctor {
  cli::Error operator()(int /*x*/, char /*c*/) { return {}; }
};

struct MyFunctor2 {
  cli::Error operator()(int /*f*/) { return {}; }
};

static constinit struct Settings {
  int a{};
  int b{};
  char c{};
  int a_long_param{6};
  void apply() {}
} settings{};

// using declarations for literal operators
using cli::operator""_sc;
using cli::operator""_arg;

// using declarations to not repeat cli:: unnecessarily
using cli::arg;
using cli::func;
using cli::param;

struct Config : cli::default_config {};

// clang-format off
// the cli object itself
static cli::Engine cli_ = cli::sim::create(
  Config{},
  cli::constant<1>{},
  param<int>("foo"_sc, 
            "foo description"_sc, 
            &foo_getter, 
            &foo_setter, 
            &validate_foo),
  // functions
  // @{
  // free functions
  func("free1"_sc, &free1, cli::arg("param"_sc,"a parameter"_sc)),
  // lambdas without templated call operator
  func("lambda"_sc, 
      [](int /*i*/, char /*arg*/='k') {},
      "i"_arg, 
      cli::arg<'k'>("k"_sc,"k desc"_sc)),
  // and any other functor without templated call operator
  func("functor"_sc, MyFunctor{} ,"x"_arg,"c"_arg),
  func(MyFunctor2{}, "f"_arg),
  // member functions
  func("free2"_sc, s_, &S::free2, "x"_arg),
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
        param<&Settings::a_long_param>(),
        param("c"_sc, &Settings::c),
        func<&Settings::apply>())
);
// clang-format on

int main() {
  if (not cli::sim::init())
    return -1;

  // cli_.print();

  while (cli::sim::get_input_and_process(cli_)) {
  }

  return 0;
}
