#ifndef CLI_SIM_HPP
#define CLI_SIM_HPP

#include "cli/cli.hpp"

#include "cpp-terminal/exception.hpp"
#include "cpp-terminal/input.hpp"
#include "cpp-terminal/iostream.hpp"
#include "cpp-terminal/key.hpp"
#include "cpp-terminal/options.hpp"
#include "cpp-terminal/terminal.hpp"
#include "cpp-terminal/tty.hpp"

#include <string_view>

namespace cli::sim {

  cli::Error write(cli::View<const char> s) {
    Term::cout << std::string_view(s.data(), s.size()) << std::flush;
    return cli::Error::none;
  }

  inline bool init() {
    // check if the terminal is capable of handling input
    Term::terminal.setOptions(Term::Option::NoClearScreen,
                              Term::Option::NoSignalKeys,
                              Term::Option::Cursor,
                              Term::Option::Raw);
    if (!Term::is_stdin_a_tty()) {
      Term::cerr << "The terminal is not attached to a TTY and "
                    "therefore can't catch user input. Exiting..."
                 << std::flush;
      return false;
    }
    return true;
  }

  template<typename Cli>
  bool get_input_and_process(Cli &cli) {
    try {
      // 1. gather input and call on_char().
      Term::Event event = Term::read_event();
      switch (event.type()) {
        case Term::Event::Type::Key: {
          Term::Key key(event);
          if (key == Term::Key::Ctrl_C)
            exit(0);
          else if (key == Term::Key::Enter)
            cli.on_char('\n');
          else if (key == Term::Key::ArrowDown)
            for (auto c : "\x1b[B")
              cli.on_char(c);
          else if (key == Term::Key::ArrowUp)
            for (auto c : "\x1b[A")
              cli.on_char(c);
          else
            cli.on_char(key.value);
          break;
        }
        default:
          break;
      }

      // call process()
      cli::Error err = cli.process();

      // ignore errors
      (void)err;
      return true;
    } catch (const Term::Exception &re) {
      Term::cerr << "cpp-terminal error: " << re.what() << std::endl;
      return false;
    } catch (...) {
      Term::cerr << "Unknown error." << std::endl;
      return false;
    }
  }

  template<cli::Config Config, cli::Command... Commands>
  constexpr auto create_cli(Config config,
                            Commands &&...commands) /* -> cli::Cli */ {
    static_assert(std::is_same_v<char, typename Config::char_type>,
                  "char_type must be char. Others are unsupported for now.");
    return cli::Cli{
      Config{},
      cli::AnsiOutput{Config{}, &cli::sim::write},
      std::forward<Commands>(commands)...
    };
  }

} // namespace cli::sim

#endif
