/**
 * @file cli/sim.hpp
 * @brief This header contains the functions to simulate a cli on a PC terminal.
 * See also @ref Simulation.
 *
 * @defgroup Simulation Simulation
 *
 * clis can also be simulated on a PC.
 *
 * See [here](docs.md#simulation) for more info.
 */

#ifndef CLI_SIM_HPP
#define CLI_SIM_HPP

#include "cli.hpp"

#include <cpp-terminal/exception.hpp>
#include <cpp-terminal/input.hpp>
#include <cpp-terminal/iostream.hpp>
#include <cpp-terminal/key.hpp>
#include <cpp-terminal/options.hpp>
#include <cpp-terminal/terminal.hpp>
#include <cpp-terminal/tty.hpp>
#include <ostream>
#include <string_view>

namespace cli::sim {

  namespace dtl {
    inline void write(cli::View<const char> s) {
      Term::cout << std::string_view(s.data(), s.size()) << std::flush;
    }
  } // namespace dtl

  /**
   * @brief The init function for the sim.
   *
   * @ingroup Simulation
   * @return true if init succeded, else false.
   */
  inline bool init() {
    // check if the terminal is capable of handling input
    Term::terminal.setOptions(
      Term::Option::NoSignalKeys, Term::Option::Cursor, Term::Option::Raw);

    if (!Term::is_stdin_a_tty()) {
      Term::cerr << "The terminal is not attached to a TTY and "
                    "therefore can't catch user input. Exiting..."
                 << std::flush;
      return false;
    }
    return true;
  }

  /**
   * @brief gets an input and processes it on the cli. Returns false when ctrl-C
   * is pressed.
   *
   * @ingroup Simulation
   * @param engine the engine object
   */
  template<typename Engine>
  bool get_input_and_process(Engine &engine) {
    try {
      // 1. gather input and call on_char().
      Term::Event event = Term::read_event();
      switch (event.type()) {
        case Term::Event::Type::Key: {
          Term::Key key(event);

          switch (key.value) {
            case Term::Key::Ctrl_C:
              Term::cout << std::endl;
              exit(0);
            case Term::Key::Ctrl_J:
              // clear screen
              for (auto c : cli::View{"\x1b[2J"})
                engine.on_char(c);
              break;
            case Term::Key::Ctrl_K:
              // clear to end of line
              for (auto c : cli::View{"\x1b[0K"})
                engine.on_char(c);
              break;
            case Term::Key::Ctrl_L:
              // clear entire line
              for (auto c : cli::View{"\x1b[2K"})
                engine.on_char(c);
              break;
            case Term::Key::Ctrl_U:
              // clear to begin of line
              for (auto c : cli::View{"\x1b[1K"})
                engine.on_char(c);
              break;
            case Term::Key::Enter:
              switch (
                cli::config::input_delimiter_v<typename Engine::config_type>) {
                case cli::Delimiter::lf:
                  engine.on_char('\n');
                  break;
                case cli::Delimiter::cr:
                  engine.on_char('\r');
                  break;
                case cli::Delimiter::crlf:
                  engine.on_char('\r');
                  engine.on_char('\n');
                  break;
              }
              break;
            case Term::Key::ArrowDown:
              for (auto c : cli::View{"\x1b[B"})
                engine.on_char(c);
              break;
            case Term::Key::ArrowUp:
              for (auto c : cli::View{"\x1b[A"})
                engine.on_char(c);
              break;
            case Term::Key::ArrowRight:
              for (auto c : cli::View{"\x1b[C"})
                engine.on_char(c);
              break;
            case Term::Key::ArrowLeft:
              for (auto c : cli::View{"\x1b[D"})
                engine.on_char(c);
              break;
            default:
              engine.on_char(static_cast<char>(key.value));
          }
        } break;
        default:
          break;
      }

      // call process()
      cli::Error err = engine.process();

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

  /**
   * @brief creates and returns the engine object from the config and commands
   *
   * @ingroup Simulation
   * @param config the cli::Config
   * @param commands the commands
   * @return cli::Engine
   */
  template<cli::concepts::Config Config, cli::concepts::Command... Commands>
  constexpr auto create(Config, Commands &&...commands) /* -> cli::Engine */ {
    static_assert(std::is_same_v<char, typename Config::char_type>,
                  "char_type must be char. Others are unsupported for now.");

    return cli::Engine{Config{},
                       cli::AnsiDisplay{&::cli::sim::dtl::write},
                       std::forward<Commands>(commands)...};
  }

} // namespace cli::sim

#endif
