/**
 * @file cli.cpp
 * @example examples/cli.cpp
 * @brief This file contains the main example for cli. It can be compiled on the
 * PC and played around with.
 */

#include "cli.hpp"
#include "cli/sim.hpp"

// the parameters and functions used in this example
constinit static bool enable = false;
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

struct FooSettings {
  int glorb = 5;
  char k = 'x';
};

struct BazSettings {
  enum class Mode {
    normal,
    fast,
    extreme
  };
  int num;
  Mode mode;
};

static constinit struct Settings2 {
  FooSettings foo{};
  BazSettings baz{};
} settings2;

static constinit BazSettings baz1{};
static constinit BazSettings baz2{};
static constinit BazSettings baz3{};
static constinit BazSettings baz4{};
static constinit BazSettings baz5{};
static constinit BazSettings baz6{};
static constinit BazSettings baz7{};
static constinit BazSettings baz8{};
static constinit BazSettings baz9{};
static constinit BazSettings baz10{};
static constinit BazSettings baz11{};

static constexpr BazSettings cbaz1{};
static constexpr BazSettings cbaz2{};
static constexpr BazSettings cbaz3{};
static constexpr BazSettings cbaz4{};
static constexpr BazSettings cbaz5{};
static constexpr BazSettings cbaz6{};
static constexpr BazSettings cbaz7{};
static constexpr BazSettings cbaz8{};
static constexpr BazSettings cbaz9{};

// using declarations for literal operators
using cli::operator""_sc;
using cli::operator""_arg;

// using declarations to not repeat cli:: unnecessarily
using cli::arg;
using cli::func;
using cli::param;

struct Config : cli::default_config {
  static constexpr std::size_t output_size = 256;
  static constexpr bool use_detailed_error_messages = true;
};

// clang-format off
// the cli object itself
static cli::sim::Engine engine{
  Config{},
  // parameter without object
  // param<int>("foo"_sc, 
  //           "foo description"_sc, 
  //           &foo_getter, 
  //           &foo_setter, 
  //           &validate_foo),
  // // free function
  // func("free1"_sc, &free1, cli::arg("param"_sc,"a parameter"_sc)),
  // // lambdas without templated call operator
  // func("lambda"_sc, 
  //     [](int /*i*/, char /*arg*/='k') {},
  //     "i"_arg, 
  //     cli::arg<'k'>("k"_sc,"k desc"_sc)),
  // // and any other functor without templated call operator
  // func("functor"_sc, MyFunctor{} ,"x"_arg,"c"_arg),
  func(MyFunctor2{}, "f"_arg),
  // member functions
  func("free2"_sc, s_, &S::free2, "x"_arg),
  // parameters with object declarations
  param("enable"_sc,"enables stuff"_sc, enable),
  // virtual hierarchies
  param("enable-group"_sc,
        "a group of enable and options"_sc, 
        param("enable"_sc,
              "virtual enable"_sc, 
              enable_virtual,
        param("opts"_sc, 
              "enable options"_sc, 
              enable_opts))),
  param("settings"_sc,
        "core settings"_sc, 
        settings,
        // member data parameters
        param("b"_sc, &Settings::b),
        param("a"_sc,&Settings::a),
        param("a_long_param"_sc, &Settings::a_long_param),
        param("c"_sc, &Settings::c),
        // member function command
        func<&Settings::apply>()),
  // recrusive parameter
  param("settings2"_sc,
        "settings 2 description"_sc,
        settings2,
        cli::recursive),
  param("csettings2"_sc,
        "settings 2 description"_sc,
        cli::as_const(settings2),
        cli::recursive),
  param<baz1>(),
  param<baz2>("baz description"_sc),
  param<baz3>("baz description"_sc, [](BazSettings&s){s=baz3;return cli::Error::none;}),
  param<baz4>("baz description"_sc, [](BazSettings&s){s=baz4;return cli::Error::none;}, cli::format::Format<BazSettings,char>{}),
  param<baz5>("baz description"_sc,                                                     cli::format::Format<BazSettings,char>{}),
  param<baz6>(                      [](BazSettings&s){s=baz6;return cli::Error::none;}),
  param<baz7>(                      [](BazSettings&s){s=baz7;return cli::Error::none;}, cli::format::Format<BazSettings,char>{}),
  param<baz8>(                                                                          cli::format::Format<BazSettings,char>{}),
  param<baz9>("baz description"_sc, [](BazSettings&s){s=baz9;return cli::Error::none;}, [](const BazSettings&s){baz9=s;return cli::Error::none;}),
  param<baz10>(                     [](BazSettings&s){s=baz9;return cli::Error::none;}, [](const BazSettings&s){baz9=s;return cli::Error::none;}),
  param<baz11>(param<&BazSettings::num>()),
  param<cbaz1>(),
  param<cbaz2>("baz description"_sc),
  param<cbaz3>("baz description"_sc, [](BazSettings&s){s=baz3;return cli::Error::none;}),
  param<cbaz4>("baz description"_sc, [](BazSettings&s){s=baz4;return cli::Error::none;}, cli::format::Format<BazSettings,char>{}),
  param<cbaz5>("baz description"_sc,                                                     cli::format::Format<BazSettings,char>{}),
  param<cbaz6>(                      [](BazSettings&s){s=baz6;return cli::Error::none;}),
  param<cbaz7>(                      [](BazSettings&s){s=baz7;return cli::Error::none;}, cli::format::Format<BazSettings,char>{}),
  param<cbaz8>(                                                                          cli::format::Format<BazSettings,char>{}),
  param<cbaz9>(param<&BazSettings::num>()),
};
// clang-format on

int main() {

  if (not cli::sim::init())
    return -1;

  engine.print();

  while (engine.get_input_and_process()) {
  }

  return 0;
}
