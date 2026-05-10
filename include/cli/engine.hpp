#ifndef CLI_ENGINE_HPP
#define CLI_ENGINE_HPP

#include "cli/command.hpp"
#include "cli/command_tree.hpp"
#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "cli/display.hpp"
#include "cli/enums.hpp"
#include "cli/event.hpp"
#include "cli/history.hpp"
#include "cli/input.hpp"
#include "cli/line.hpp"
#include "cli/string.hpp"
#include "cli/util.hpp"

#include <cstdint>
#include <type_traits>

namespace cli {

  template<typename CharT, bool PrintNeedsIncrementalData>
  struct PrintData {
    const CommandNode<CharT> *current = nullptr;
    std::size_t indent{0};
  };

  template<typename CharT>
  struct PrintData<CharT, false> {};

  template<bool useHelp, typename CharT, typename Cmd>
  constexpr bool name_is_not_help() {
    if constexpr (useHelp)
      return Cmd::name != string_constant<CharT, 'h', 'e', 'l', 'p'>{};
    else
      return true;
  }

  /**
   * @brief The Engine is the interface for CLI. It contains the @ref Input,
   * @ref Display, and commands.
   *
   * See [here](docs.md#engine) for more info.
   *
   * @param Cfg the configuration
   * @param Display the display type
   * @param Commands the commands
   */
  template<concepts::Config Cfg,
           concepts::Display<typename Cfg::char_type> Display,
           concepts::Command... Commands>
  class Engine {
  public:
    using config_type = Cfg;
    using char_type = typename config_type::char_type;
    using input_type = config::input_type_t<config_type>;
    using event_type = Event<char_type>;
    using display_type = Display;

    static_assert(concepts::Input<input_type, char_type>,
                  "The input_type of your config does not satisfy the Input "
                  "concept. See cli::concepts::Input for the definition.");

    static_assert(
      config::display_fits_config_v<config_type, Display>,
      "The Display and config don't fit together. This is the case when "
      "'use_cursor' is true and the Display doesn't support a cursor. Either "
      "use a Display with cursor support or set 'use_cursor' to false.");

    static_assert(minimum_line_length_v<Commands...> <=
                    config_type::max_line_length,
                  "The maximum line length is smaller than the minimum length "
                  "required to input the largets command. Increase "
                  "'max_line_length' to fix this error.");

    static_assert(is_multiline_display_v<Display>
                    ? number_of_lines_v<Display> > 1
                    : true,
                  "A display can't be multiline and only have one line "
                  "available. Either set is_multiline_display to false or set "
                  "number_of_lines to a value greater than 1.");

    static_assert(
      not is_multiline_display_v<Display> ? number_of_lines_v<Display> == 1
                                          : true,
      "A display can't be single line and only have more than one line "
      "available. Either set is_multiline_display to true or set "
      "number_of_lines to 1.");

    static_assert(
      (name_is_not_help<config::use_help_v<config_type>,
                        char_type,
                        Commands>() and
       ...),
      "A command with the name 'help' is detected. This is not valid because "
      "'use_help' is true. Either remove said command or set 'use_help' to "
      "false.");

    template<concepts::Config C,
             concepts::Display<typename C::char_type> D,
             concepts::Command... Cmds>
    constexpr Engine(C config, D &&display, Cmds &&...commands)
      : commands_{*this, std::forward<Cmds>(commands)...},
        display_{std::forward<D>(display)},
        line_{*commands_.root(), display_} {
      (void)config;
    }

    /**
     * Notifes the ngine of a newly received character.
     *
     * This method passes c onto its @ref cli::concepts::Input "input".
     *
     * @param c the received character
     * @return Error::none on success, Error::buffer_overflow if the input can't
     * handle more events.
     */
    constexpr Error on_char(char_type c) { return input_.on_char(c); }

    /**
     * Notifies the engine of a newly received control.
     *
     * @param ctrl the Control
     * @return Error::none on success, Error::buffer_overflow if the input can't
     * handle more events.
     */
    constexpr Error on_control(Control ctrl, std::uint8_t param = 1) {
      return input_.on_control(ctrl, param);
    }

    /**
     * processes the characters/events of the input.
     */
    constexpr Error process() {
      Event<char_type> ev{};
      while (input_.pop_event(ev)) {
        Error e = process_event(ev);
        if (e != Error::none)
          return e;
      }
      return Error::none;
    }

    /**
     * resets the engine, i.e. clears the input and clears the display.
     */
    constexpr void reset() {
      input_.reset();
      line_.clear_screen();
    }

    /**
     * prints the command tree
     */
    constexpr void print() {
      if constexpr (needs_incremental_print) {
        print_data_.current = nullptr;
        print_data_.indent = 0;
        line_.reset();
        print_one();
      } else {
        print(*commands_.root(), 0);
        display_.newline();
      }
    }

  private:
    constexpr Error process_event(const event_type &ev) {
      if constexpr (needs_incremental_print) {
        if (print_data_.current) {
          // is printing
          // cursor left and right still work
          // cursor down and enter will progress the print operation
          // any other event will cancel print
          switch (ev.type()) {
            case Control::cursor_left:
              display_.cursor_left(ev.param());
              return Error::none;
            case Control::cursor_right:
              display_.cursor_right(ev.param());
              return Error::none;
            case Control::cursor_down:
              [[fallthrough]];
            case Control::enter:
              print_one();
              if (print_data_.current == nullptr) {
                // print finished
                display_.newline();
              }
              return Error::none;
            default:
              // print is cancelled
              print_data_.current = nullptr;
              print_data_.indent = 0;
              display_.newline();
          }
        }
      }

      switch (ev.type()) {
        case Control::character:
          return line_.on_char(ev.as_char());
        case Control::backspace:
          return line_.on_backspace(ev.param());
        case Control::autocomplete:
          return line_.on_autocomplete();
        case Control::cursor_down:
          return on_cursor_down(ev.param());
        case Control::cursor_up:
          return on_cursor_up(ev.param());
        case Control::cursor_left:
          return line_.on_cursor_left(ev.param());
        case Control::cursor_right:
          return line_.on_cursor_right(ev.param());
        case Control::delete_char:
          return line_.on_delete_char();
        case Control::clear_screen:
          return line_.on_clear_screen();
        case Control::clear_line:
          return line_.clear();
        case Control::clear_line_to_end:
          return line_.on_clear_line_to_end();
        case Control::clear_line_to_begin:
          return line_.on_clear_line_to_begin();
        case Control::enter:
          return on_enter();
        default:
          // TODO: Control::bell handling
          return Error::unimplemented;
      }
    }

    constexpr Error on_cursor_down(uint32_t n) {
      if constexpr (Cfg::use_history) {
        View str = history_.cursor_down(n);
        // if (line_.view().size() == 0)
        //   display_.newline();
        return line_.set_data(str);
      } else
        return Error::none;
    }

    constexpr Error on_cursor_up(uint32_t n) {
      if constexpr (Cfg::use_history) {
        View str = history_.cursor_up(n);
        // if (line_.view().size() == 0)
        //   display_.newline();
        return line_.set_data(str);
      } else
        return Error::none;
    }

    constexpr Error on_enter() {
      history_.push(line_.view());
      View out_buf = {buffer_, config::output_size_v<Cfg>};

      return line_.execute(out_buf);
    }

    constexpr void print_one() {
      if (print_data_.current == nullptr) {
        // print first lines
        print_data_.current = root();
        print_data_.indent = 0;
        for (std::size_t i = 0; i < cli::number_of_lines_v<Display>; ++i) {
          print_one();
        }
      } else {
        // print current
        display_.newline();
        for (std::size_t i = 0; i < 2 * print_data_.indent; ++i)
          display_.write(' ');
        display_.write(print_data_.current->name);
        display_.write(' ');
        display_.write('[');
        display_.write(print_data_.current->type);
        display_.write(']');
        display_.write(':');
        display_.write(' ');
        display_.write(print_data_.current->description);

        // get next
        if (print_data_.current->subcommand) {
          // go down one level
          ++print_data_.indent;
          print_data_.current = print_data_.current->subcommand;
        } else if (print_data_.current->next) {
          // stay on curent level
          print_data_.current = print_data_.current->next;
        } else {
          // go up one level
          print_data_.current = print_data_.current->parent->next;
          --print_data_.indent;
        }
      }
    }

    constexpr void print(const CommandNode<char_type> &c, std::size_t indent) {
      display_.newline();
      if (indent == 0) {
        display_.write(c.name);
        display_.write(':');
        display_.write(' ');
        display_.write(c.description);
      } else {
        for (std::size_t i = 0; i < 2 * indent; ++i)
          display_.write(' ');
        display_.write(c.name);
        display_.write(' ');
        display_.write('[');
        display_.write(c.type);
        display_.write(']');
        display_.write(':');
        display_.write(' ');
        display_.write(c.description);
      }
      if (c.subcommand == nullptr) {
        return;
      }
      ++indent;
      auto sub = c.subcommand;
      while (sub != nullptr) {
        print(*sub, indent);
        sub = sub->next;
      }
    }

    template<typename Engine>
    friend struct Help;

    const CommandNode<char_type> *root() const { return commands_.root(); }

    using CommandTree_t =
      CommandTree<Engine<Cfg, Display, Commands...>, Commands...>;

    static constexpr bool needs_incremental_print =
      (num_cmds_v<Commands> + ...) > number_of_lines_v<Display>;

    CLI_NO_UNIQUE_ADDRESS CommandTree_t commands_;
    CLI_NO_UNIQUE_ADDRESS Display display_;
    CLI_NO_UNIQUE_ADDRESS Line<Cfg, Display> line_;
    CLI_NO_UNIQUE_ADDRESS input_type input_{};
    CLI_NO_UNIQUE_ADDRESS History<Cfg> history_{};
    CLI_NO_UNIQUE_ADDRESS
    PrintData<char_type, needs_incremental_print> print_data_;
    char_type buffer_[config::output_size_v<Cfg>]{};
  };

  template<typename C, typename D, typename... Cmds>
  Engine(C, D &&, Cmds &&...)
    -> Engine<std::decay_t<C>, std::decay_t<D>, std::decay_t<Cmds>...>;

} // namespace cli

#endif
