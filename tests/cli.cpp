#include "cli/cli.hpp"

#include <cstdio>
#include <ostream>
#include <string_view>

// #include <cstdio>
// #include <source_location>

// template <typename T> void info() {
//   const std::source_location &loc = std::source_location::current();
//   std::printf("%s", loc.function_name());
// }
/*
 * cpp-terminal
 * C++ library for writing multi-platform terminal applications.
 *
 * SPDX-FileCopyrightText: 2019-2025 cpp-terminal
 *
 * SPDX-License-Identifier: MIT
 */

#include "cli/parse.hpp"
#include "cli/string.hpp"
#include "cpp-terminal/exception.hpp"
#include "cpp-terminal/input.hpp"
#include "cpp-terminal/iostream.hpp"
#include "cpp-terminal/key.hpp"
#include "cpp-terminal/options.hpp"
#include "cpp-terminal/terminal.hpp"
#include "cpp-terminal/tty.hpp"
#include "cpp-terminal/version.hpp"

cli::Error stream(cli::View<const char> s) {
  Term::cout << std::string_view(s.data(), s.size()) << std::flush;
  return cli::Error::none;
}

using namespace cli;

using cfg = cli::default_config;
struct MyCfg : cfg {

  static cli::Error transmit(uint8_t c) {
    Term::cout << static_cast<char>(c) << std::flush;
    return {};
  }
};

static bool enable = false;

constexpr int free1(int param) { return param; }

constinit int virtual_ = 0;
static bool enable_virtual = false;
static constinit int enable_opts = 0xFF;

inline struct S {
  cli::Error free2(int x) { return {}; }
} s;

struct MyFunctor {
  cli::Error operator()(int x, char c) { return {}; }
};
struct MyFunctor2 {
  cli::Error operator()(int f) { return {}; }
};
void free3() { return; }
void free4(int) { return; }
int free5(void) { return -5; }

inline struct Settings {
  int a;
  int b;
  char c;
  void apply() {}
} settings;

using P = cli::parse::String<cli::View<const char>, char>;
static_assert(traits::String<cli::View<const char>>);
static_assert(cli::Output<
              decltype(cli::AnsiOutput{cli::default_config{}, &stream}), char>);
// clang-format off
static constinit cli::Cli my_cli(
    cli::default_config{},
    cli::AnsiOutput{cli::default_config{},&stream},
    // functions
    // @{
    // free functions
    func("free1"_sc, &free1, funcs::arg("param"_sc)) ,
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
    param("virtual"_sc,"virtual group"_sc, virtual_,
          param("enable"_sc,"virtual enable"_sc, enable_virtual,
          param("opts"_sc, "enable options"_sc, enable_opts))),
    param("settings"_sc, "core settings"_sc, settings,
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
                              Term::Option::NoSignalKeys, Term::Option::Cursor,
                              Term::Option::Raw);
    if (!Term::is_stdin_a_tty()) {
      throw Term::Exception("The terminal is not attached to a TTY and "
                            "therefore can't catch user input. Exiting...");
    }
    my_cli.print();
    while (1) {
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
      case Term::Event::Type::CopyPaste: {
        std::string key_str(event);
        if (!key_str.empty() && key_str[0] == '\033') {
          Term::cout << "You discovered a key combination not yet managed by "
                        "cpp-terminal (";
          for (std::size_t i = 0; i != key_str.size(); ++i) {
            Term::cout << static_cast<std::int32_t>(key_str[i]) << " ";
          }
          Term::cout << ").\nPlease report key combination pressed to "
                     << Term::homepage() << std::endl;
        }
      }
      default:
        break;
      }
      my_cli.process();
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
