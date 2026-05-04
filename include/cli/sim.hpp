/**
 * @file cli/sim.hpp
 * @brief This header contains the functions to simulate a cli on a PC terminal.
 * See also @ref Simulation.
 *
 * @defgroup Simulation
 * clis can also be simulated on a PC.
 *
 * Basic usage example:
 *
 * ```
 * #include "cli/sim.hpp"
 *
 * struct Config{...};
 *
 * int main(){
 *   if (not cli::sim::init())
 *     return -1;
 *
 *   cli::Cli my_cli = cli::sim::create_cli(Config{}, commands...);
 *
 *   my_cli.print();
 *
 *   while (cli::sim::get_input_and_process(my_cli)) {
 *   }
 *
 *   return 0;
 * }
 * ```
 *
 * The executable can then be run on a PC. Ctrl-C exits the cli.
 *
 * @note [cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal) is
 * required for the simulation to work.
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
#include <string_view>

namespace cli::sim {

  namespace dtl {
    inline cli::Error write(cli::View<const char> s) {
      Term::cout << std::string_view(s.data(), s.size()) << std::flush;
      return cli::Error::none;
    }
  } // namespace dtl

  /**
   * @brief The init function for the sim.
   *
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
   * @brief gets an input and processes it on the cli. Returns false when ctrl-c
   * is pressed.
   *
   * @param cli the cli object
   */
  template<typename Cli>
  bool get_input_and_process(Cli &cli) {
    try {
      // 1. gather input and call on_char().
      Term::Event event = Term::read_event();
      switch (event.type()) {
        case Term::Event::Type::Key: {
          Term::Key key(event);

          switch (key.value) {
            case Term::Key::Ctrl_C:
              exit(0);
            case Term::Key::Ctrl_J:
              // clear screen
              for (auto c : cli::View{"\x1b[2J"})
                cli.on_char(c);
              break;
            case Term::Key::Ctrl_K:
              // clear to end of line
              for (auto c : cli::View{"\x1b[0K"})
                cli.on_char(c);
              break;
            case Term::Key::Ctrl_L:
              // clear entire line
              for (auto c : cli::View{"\x1b[2K"})
                cli.on_char(c);
              break;
            case Term::Key::Ctrl_U:
              // clear to begin of line
              for (auto c : cli::View{"\x1b[1K"})
                cli.on_char(c);
              break;
            case Term::Key::Enter:
              cli.on_char('\n'); // TODO: respect Cli's delimiter
              break;
            case Term::Key::ArrowDown:
              for (auto c : cli::View{"\x1b[B"})
                cli.on_char(c);
              break;
            case Term::Key::ArrowUp:
              for (auto c : cli::View{"\x1b[A"})
                cli.on_char(c);
              break;
            case Term::Key::ArrowRight:
              for (auto c : cli::View{"\x1b[C"})
                cli.on_char(c);
              break;
            case Term::Key::ArrowLeft:
              for (auto c : cli::View{"\x1b[D"})
                cli.on_char(c);
              break;
            default:
              cli.on_char(key.value);
          }
        } break;
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

  /**
   * @brief creates the cli object from the config and commands
   *
   * @param config the cli::Config
   * @param commands the commands
   */
  template<cli::concepts::Config Config, cli::concepts::Command... Commands>
  constexpr auto create_cli(Config config,
                            Commands &&...commands) /* -> cli::Cli */ {
    static_assert(std::is_same_v<char, typename Config::char_type>,
                  "char_type must be char. Others are unsupported for now.");

    static_assert(is_multiline_display_v<decltype(cli::AnsiDisplay{
                    &::cli::sim::dtl::write})>);

    return cli::Engine{Config{},
                       cli::AnsiDisplay{&::cli::sim::dtl::write},
                       std::forward<Commands>(commands)...};
  }

} // namespace cli::sim

#endif
