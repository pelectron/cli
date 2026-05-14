#include "cli.hpp"

#if !defined(CLI_PREFIX)
#define SC(str) str##_sc
#define ARG(str) str##_arg
#define OUTPUT [](cli::View<const char>) -> void {}
#elif CLI_PREFIX == u
#define SC(str) u##str##_sc
#define ARG(str) u##str##_arg
#define OUTPUT [](cli::View<char16_t>) -> void {}
#elif CLI_PREFIX == U
#define SC(str) U##str##_sc
#define ARG(str) U##str##_arg
#define OUTPUT [](cli::View<char32_t>) -> void {}
#endif

static_assert(cli::concepts::Output<decltype(OUTPUT)>);

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

struct Config : cli::default_config {
  static constexpr std::size_t output_size = 256;
  static constexpr bool use_detailed_error_messages = true;
};

// clang-format off
// the cli object itself
static cli::Engine engine{
  Config{},
  cli::AnsiDisplay{OUTPUT},
  param<int>(SC("foo"), 
            SC("foo description"), 
            &foo_getter, 
            &foo_setter, 
            &validate_foo),
  // functions
  // @{
  // free functions
  func(SC("free1"), 
       &free1, 
       cli::arg(SC("param"),
                SC("a parameter"))),
  // lambdas without templated call operator
  func(SC("lambda"), 
      [](int /*i*/, char /*arg*/='k') {},
      ARG("i"), 
      cli::arg<'k'>(SC("k"),
                    SC("k desc"))),
  // and any other functor without templated call operator
  func(SC("functor"), 
       MyFunctor{},
       ARG("x"),
       ARG("c")),
  func(MyFunctor2{}, ARG("f")),
  // member functions
  func(SC("free2"),
       s_, 
       &S::free2, 
       ARG("x")),
// @}
// global objects
  param(SC("enable"),
        SC("enables stuff"), 
        enable),
  // virtual hierarchies
  param(SC("settings"),
        SC("core settings"), 
        settings,
        param(SC("b"), &Settings::b),
        // param<&Settings::a>(),
       // param<&Settings::a_long_param>(),
        param(SC("c"), &Settings::c))
};
// clang-format on

int main() { return 0; }
