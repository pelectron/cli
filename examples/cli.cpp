/**
 * @file cli.cpp
 * @brief This file contains the main example for cli. It can be compiled on the
 * PC and played around with.
 */

#include "cli/cli.hpp"

#include "cpp-terminal/exception.hpp"
#include "cpp-terminal/input.hpp"
#include "cpp-terminal/iostream.hpp"
#include "cpp-terminal/key.hpp"
#include "cpp-terminal/options.hpp"
#include "cpp-terminal/terminal.hpp"
#include "cpp-terminal/tty.hpp"

// the cli output stream
cli::Error stream(cli::View<const char> s) {
  Term::cout << std::string_view(s.data(), s.size()) << std::flush;
  return cli::Error::none;
}

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

// clang-format off
// the cli object itself
static cli::Cli my_cli(
    cli::default_config{},
    cli::AnsiOutput{cli::default_config{},&stream},
    param<int>("foo"_sc, "foo description"_sc, &foo_getter, &foo_setter, &validate_foo),
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

int main() {
  try {
    // check if the terminal is capable of handling input
    Term::terminal.setOptions(Term::Option::NoClearScreen,
                              Term::Option::NoSignalKeys,
                              Term::Option::Cursor,
                              Term::Option::Raw);
    if (!Term::is_stdin_a_tty()) {
      throw Term::Exception("The terminal is not attached to a TTY and "
                            "therefore can't catch user input. Exiting...");
    }

    my_cli.print();

    while (1) {
      // 1. gather input and call on_char().
      Term::Event event = Term::read_event();
      switch (event.type()) {
        case Term::Event::Type::Key: {
          Term::Key key(event);
          if (key == Term::Key::Ctrl_C)
            exit(0);
          else if (key == Term::Key::Enter)
            my_cli.on_char('\n');
          else if (key == Term::Key::ArrowDown)
            for (auto c : "\x1b[B")
              my_cli.on_char(c);
          else if (key == Term::Key::ArrowUp)
            for (auto c : "\x1b[A")
              my_cli.on_char(c);
          else
            my_cli.on_char(key.value);
          break;
        }
        default:
          break;
      }

      // call process()
      cli::Error err = my_cli.process();

      // handle errors
      switch (err) {
        default:
          (void)err; // ignore error
      }
    }
  } catch (const Term::Exception &re) {
    Term::cerr << "cpp-terminal error: " << re.what() << std::endl;
    return 2;
  } catch (...) {
    Term::cerr << "Unknown error." << std::endl;
    return 1;
  }
  return 0;
}
