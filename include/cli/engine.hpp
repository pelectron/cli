#ifndef CLI_ENGINE_HPP
#define CLI_ENGINE_HPP

#include "cli/command_tree.hpp"
#include "cli/concepts.hpp"
#include "cli/enums.hpp"
#include "cli/history.hpp"
#include "cli/input.hpp"
#include "cli/line.hpp"
#include "cli/util.hpp"

#include <type_traits>

namespace cli {

  /**
   * @brief The Engine is the interface for CLI. It contains the @ref Input,
   * @ref Display, and commands.
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
    using input_type = get_input_type<config_type>;

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
     * This method passes c onto its @ref cli::concept::Input "input".
     * @param c the received character
     */
    constexpr Error on_char(char_type c) { return input_.on_char(c); }

    /**
     * processes the characters received by the input.
     */
    constexpr Error process() {
      Event<char_type> ev{};
      while (input_.pop_event(ev)) {
        Error e{};

        if (ev.is_char)
          e = process_char(ev.c);
        else
          e = process_control(ev.ctrl);

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

    constexpr void print() {
      print(*commands_.root(), 0);
      display_.newline();
    }

  private:
    constexpr Error process_char(char_type c) { return line_.on_char(c); }

    constexpr Error process_control(const Control &ctrl) {
      switch (ctrl.type) {
        case Control::backspace:
          return line_.on_backspace(ctrl.param);
        case Control::autocomplete:
          return line_.on_autocomplete();
        case Control::cursor_down:
          return on_cursor_down(ctrl.param);
        case Control::cursor_up:
          return on_cursor_up(ctrl.param);
        case Control::cursor_left:
          return line_.on_cursor_left(ctrl.param);
        case Control::cursor_right:
          return line_.on_cursor_right(ctrl.param);
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
      if constexpr (Cfg::use_cursor) {
        View str = history_.cursor_down(n);
        if (line_.view().size() == 0)
          display_.newline();
        return line_.set_data(str);
      } else
        return Error::none;
    }

    constexpr Error on_cursor_up(uint32_t n) {
      if constexpr (Cfg::use_cursor) {
        View str = history_.cursor_up(n);
        if (line_.view().size() == 0)
          display_.newline();
        return line_.set_data(str);
      } else
        return Error::none;
    }

    constexpr Error on_enter() {
      history_.push(line_.view());
      View out_buf = {buffer_, config::output_size_v<Cfg>};

      return line_.execute(out_buf);
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

    CLI_NO_UNIQUE_ADDRESS CommandTree_t commands_;
    CLI_NO_UNIQUE_ADDRESS Display display_;
    CLI_NO_UNIQUE_ADDRESS Line<Cfg, Display> line_;
    CLI_NO_UNIQUE_ADDRESS input_type input_{};
    CLI_NO_UNIQUE_ADDRESS History<Cfg> history_{};
    char_type buffer_[config::output_size_v<Cfg>]{};
  };

  template<typename C, typename D, typename... Cmds>
  Engine(C, D &&, Cmds &&...)
    -> Engine<std::decay_t<C>, std::decay_t<D>, std::decay_t<Cmds>...>;

} // namespace cli

#endif
