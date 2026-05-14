#include "cli/sim.hpp"

#include <cpp-terminal/exception.hpp>
#include <cpp-terminal/input.hpp>
#include <cpp-terminal/iostream.hpp>
#include <cpp-terminal/key.hpp>
#include <cpp-terminal/options.hpp>
#include <cpp-terminal/terminal.hpp>
#include <cpp-terminal/tty.hpp>
#include <iostream>
#include <string_view>

namespace cli::sim {

  Engine::~Engine() {
    if (engine_)
      delete engine_;
  }

  cli::Error Engine::error() const { return error_; }

  void Engine::write_view(cli::View<const char> s) {
    Term::cout << std::string_view(s.data(), s.size()) << std::flush;
  }

  bool Engine::get_input_and_process() {
    try {
      // 1. gather input and call on_char().
      Term::Event event = Term::read_event();
      switch (event.type()) {
        case Term::Event::Type::Key: {
          Term::Key key(event);

          switch (key.value) {
            case Term::Key::Ctrl_C:
              if (not engine_->has_multiline_display())
                Term::cout << std::endl;
              return false;
            case Term::Key::Ctrl_J:
              // clear screen
              for (auto c : cli::View{"\x1b[2J"})
                engine_->on_char(c);
              break;
            case Term::Key::Ctrl_K:
              // clear to end of line
              for (auto c : cli::View{"\x1b[0K"})
                engine_->on_char(c);
              break;
            case Term::Key::Ctrl_L:
              // clear entire line
              for (auto c : cli::View{"\x1b[2K"})
                engine_->on_char(c);
              break;
            case Term::Key::Ctrl_U:
              // clear to begin of line
              for (auto c : cli::View{"\x1b[1K"})
                engine_->on_char(c);
              break;
            case Term::Key::Enter:
              if (engine_->delimiter() == Delimiter::lf) {
                engine_->on_char('\n');
              } else if (engine_->delimiter() == Delimiter::cr) {
                engine_->on_char('\r');
              } else if (engine_->delimiter() == Delimiter::crlf) {
                engine_->on_char('\r');
                engine_->on_char('\n');
              }
              break;
            case Term::Key::ArrowDown:
              for (auto c : cli::View{"\x1b[B"})
                engine_->on_char(c);
              break;
            case Term::Key::ArrowUp:
              for (auto c : cli::View{"\x1b[A"})
                engine_->on_char(c);
              break;
            case Term::Key::ArrowRight:
              for (auto c : cli::View{"\x1b[C"})
                engine_->on_char(c);
              break;
            case Term::Key::ArrowLeft:
              for (auto c : cli::View{"\x1b[D"})
                engine_->on_char(c);
              break;
            default:
              engine_->on_char(static_cast<char>(key.value));
          }
        } break;
        case Term::Event::Type::Empty:
          [[fallthrough]];
        case Term::Event::Type::Cursor:
          [[fallthrough]];
        case Term::Event::Type::Screen:
          [[fallthrough]];
        case Term::Event::Type::Focus:
          [[fallthrough]];
        case Term::Event::Type::Mouse:
          [[fallthrough]];
        case Term::Event::Type::CopyPaste:
          [[fallthrough]];
        default:
          break;
      }

      // call process()
      error_ = engine_->process();

      if (error_ != Error::none)
        return false;

      return true;
    } catch (const Term::Exception &re) {
      Term::cerr << "cpp-terminal error: " << re.what() << std::endl;
      return false;
    } catch (...) {
      Term::cerr << "Unknown error." << std::endl;
      return false;
    }
  }

  cli::Error Engine::on_char(char c) { return error_ = engine_->on_char(c); }

  cli::Error Engine::on_control(cli::Control ctrl, std::uint8_t n) {
    return error_ = engine_->on_control(ctrl, n);
  }

  void Engine::print() { engine_->print(); }

  void Engine::reset() {
    error_ = cli::Error::none;
    return engine_->reset();
  }

  Engine::EngineInterface::~EngineInterface() {}

  bool init() {
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

} // namespace cli::sim
