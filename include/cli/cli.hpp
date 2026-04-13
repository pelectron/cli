/**
 * This file provides a CLI enginge to build a remote interface upon a byte
 * stream.
 *
 * The basics:
 * - the cli is a hierarchical tree of commands
 * - a command has the following properties:
 *   - a name: a string to identify the command
 *   - a description: a string to display for help messages
 *   - the function to execute
 *   - and optionally, any amount of sub commands
 *   - the command sequence: this sequence specifies the command to execute
 *   - the arg sequence: this sequence specifies the command's arguments
 * - the root is owned by the Cli structure.
 * - commands with the root as parents are called root commands.
 *
 * c++ syntax:
 *  ``auto my_cli = cli::cli(config, commands...);``
 *  a command:
 *  ``cli::cmd("[any.path.]name", func)``: a "free function" command. This
 * cannot have subcommands.
 *  ``cli::cmd("[any.path.]name", obj_reference, sub_commands...)``: an "object"
 * command is added. This must have subcommands
 *  ``cli::cmd("[any.path.]name", const_obj_reference, get)``: a retrievable
 * parameter is added.
 *
 * syntax:
 * - "cmd [args...]": invoke the command "cmd" with optional args
 * - "cmd.sub [args...]"/"cmd sub [args...]": invoke the command "cmd" with
 * optional args
 * - "get obj.property": returns the obj's property's value
 * - "get.obj.property": returns the obj's property's value
 * - "set obj.property arg": set obj's property value to arg
 * - "set.obj.property arg": set obj's property value to arg
 */
#ifndef CLI_CLI_HPP
#define CLI_CLI_HPP

#include "cli/command_tree.hpp"
#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/function.hpp"
#include "cli/history.hpp"
#include "cli/input.hpp"
#include "cli/output.hpp"
#include "cli/param.hpp"
#include "cli/parse.hpp"
#include "cli/tracker.hpp"
#include "cli/util.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace cli {
using funcs::arg;
using funcs::operator""_arg;
using funcs::func;
using funcs::mem_fun;
using params::mem_data;
using params::param;

template <class Tuple> constexpr Error parse_args(Tuple &t, const ArgVector &v);

struct Deduced {};

template <template <class...> class L, class... Commands>
constexpr auto generate_names(L<Commands...>)
    -> std::array<CharView, sizeof...(Commands)> {
  return {Commands::name...};
}

struct ControlSequence {
  CharView introducer;
  CharView params;
  CharView intermediate;
  uint8_t final;
  constexpr operator CharView() const noexcept {
    return {introducer.data(),
            introducer.size() + params.size() + intermediate.size() + 1};
  }
  constexpr void reset() {}
};

enum class State {
  active, // the cli is receiving a sequence of events that make up a
          // command name.
  args_start,
  args,
  //   set_params_start, // a param set
  //   call_params_start,
  //   call_params,
  //   set_params,
};

/**
 * @brief
 *
 * @tparam Cfg
 * @tparam Out
 * @tparam Stream
 * @tparam Commands
 */
template <Config Cfg, cli::Output<typename Cfg::char_type> Out,
          Command... Commands>
class Cli {
  using This = Cli<Cfg, Out, Commands...>;

public:
  using config = Cfg;
  using char_type = typename Cfg::char_type;
  using input_type = cli::Input<Cfg>;
  using event_type = typename input_type::event_type;
  using output_type = Out;
  static_assert(Cfg::access_separator != ' ',
                "The access_separator cannot be the space character");
  using enum State;

  template <Config Cfg_, cli::Output<typename Cfg_::char_type> Out_,
            cli::Command... Cmds>
  constexpr Cli(Cfg_, Out_ &&out, Commands &&...cmds)
      : commands{Cfg_{}, std::forward<Commands>(cmds)...},
        out_(std::forward<Out_>(out)), tracker_(*commands.root()) {}

  template <Config Cfg_, cli::Output<typename Cfg_::char_type> Out_,
            cli::Command... Cmds>
  constexpr Cli(Cfg_, Out_ &&out, const std::tuple<Commands...> &cmds)
      : commands{Cfg_{}, cmds}, out_(std::forward<Out_>(out)),
        tracker_(*commands.root()) {}

  template <Config Cfg_, cli::Output<typename Cfg_::char_type> Out_,
            cli::Command... Cmds>
  constexpr Cli(Cfg_, Out_ &&out, std::tuple<Commands...> &&cmds)
      : commands{Cfg_{}, std::move(cmds)}, out_(std::forward<Out_>(out)),
        tracker_(*commands.root()) {}

  // template <Config Cfg_, Command... Cmds>
  // constexpr Cli(Cfg_ &&cfg, output_type &&stream, Cmds &&...cmds)
  //     : commands_{std::forward<Cmds>(cmds)...},
  //       out_(io::AnsiOutputStream{std::forward<S>(stream)}) {
  //   init_tree();
  // }

  // constexpr Cli(Cli &&other)
  //     : commands_{std::move(other.commands_)}, out_(std::move(other.out_)) {
  //   init_tree();
  // }
  //
  // constexpr Cli(const Cli &other)
  //     : commands_{other.commands_}, out_(other.out_) {
  //   init_tree();
  // }
  //
  // constexpr Cli &operator=(Cli &&other) {
  //   commands_ = std::move(other.commands_);
  //   out_ = std::move(other.out_);
  //   in_.reset();
  //   current_line_.clear();
  //   state_ = active;
  //   tracker_.clear();
  //   init_tree();
  //   return *this;
  // }
  //
  // constexpr Cli &operator=(const Cli &other) {
  //   commands_ = other.commands_;
  //   out_ = other.out_;
  //   in_.reset();
  //   current_line_.clear();
  //   state_ = active;
  //   tracker_.clear();
  //   init_tree();
  //   return *this;
  // }

  constexpr Error process() {
    while (1) {
      Error e = process_one();
      if (e == Error::none)
        continue;
      else if (e == Error::buffer_underflow)
        return Error::none;
      else
        return e;
    }
  }

  constexpr void print() { print(root(), 0); }

  constexpr Error on_char(char_type c) { return in_.on_char(c); }

  constexpr void reset() {
    history_.reset();
    in_.reset();
    out_.control(Control::clear);
    return_to_idle();
  }

private:
  constexpr Error write_char(char_type c) {
    if (not current_line_.push_back(c))
      return Error::buffer_overflow;
    if (Error err = out_.write(c); err != Error::none)
      return err;
    return Error::none;
  }

  constexpr View<const char_type> line_view() {
    return {current_line_.data(), current_line_.size()};
  }

  constexpr Error process_char(char_type c) {
    // active:
    //   - just track the command for autocomplete
    //   - if c is '(', '=' or ' ' -> args_start
    // args_start:
    //   - if c is ' ', keep state
    //   - if c is '(' -> call_params_start
    //   - if c is '=' -> set_params_start
    //   - else return invalid_character
    // call_params_start: keep state until enter
    // set_params_start: keep state until enter
    switch (state_) {
    case active:
      switch (c) {
      case ' ':
        cmd_size_ = current_line_.size();
        cmd_type_ = ExecType::get;
        if (auto err = write_char(c); err != Error::none)
          return err;
        state_ = args_start;
        return Error::none;
      case '(':
        cmd_size_ = current_line_.size();
        cmd_type_ = ExecType::call;
        if (auto err = write_char(c); err != Error::none)
          return err;
        start_of_args_ = current_line_.size();
        state_ = args;
        return Error::none;
      case '=':
        cmd_size_ = current_line_.size();
        cmd_type_ = ExecType::set;
        if (auto err = write_char(c); err != Error::none)
          return err;
        start_of_args_ = current_line_.size();
        state_ = args;
        return Error::none;
      default:
        if (auto err = tracker_.on_char(c); err != Error::none) {
          return err;
        }
        return write_char(c);
      }
    case args_start:
      switch (c) {
      case ' ':
        return write_char(' ');
      case '(':
        if (auto err = write_char(c); err != Error::none)
          return err;
        start_of_args_ = current_line_.size();
        cmd_type_ = ExecType::call;
        state_ = args;
        return Error::none;
      case '=':
        if (auto err = write_char(c); err != Error::none)
          return err;
        start_of_args_ = current_line_.size();
        cmd_type_ = ExecType::set;
        state_ = args;
        return Error::none;
      default:
        start_of_args_ = current_line_.size();
        if (auto err = write_char(c); err != Error::none)
          return err;
        state_ = args;
        return write_char(' ');
      }
    case args:
      return write_char(c);
    default:
      return Error::invalid_state;
    }
  }

  constexpr Error on_autocomplete() {
    const auto str = tracker_.on_autocomplete();
    out_.write(str);
    return Error::none;
  }

  constexpr Error on_backspace(uint8_t n = 1) {
    // active:
    //  - just track backspace
    // args_start:
    //  - if line size is smaller than start_of_args_ go to active, else keep
    //  state
    // call_params_start:
    //  - if line size is start_of_args_ go to active, else keep state
    // set_params_start:
    //  - if line size is start_of_args_ go to active, else keep state
    if (n >= current_line_.size()) {
      // delete the whole input
      tracker_.clear();
      current_line_.clear();
      state_ = active;
      start_of_args_ = 0;
      cmd_type_ = ExecType::none;
      for (std::size_t i = 0; i < n; ++i)
        if (auto err = out_.control(Control::backspace); err != Error::none)
          return err;
      return Error::none;
    }

    switch (state_) {
    case active:
      for (unsigned i = 0; i < n; ++i)
        tracker_.on_backspace();
      current_line_.remove_last(n);
      break;
    case args_start: {
      auto new_size = current_line_.size() - n;
      if (new_size <= cmd_size_) {
        // back to active
        state_ = active;
        cmd_type_ = ExecType::none;
        auto backtrack_size = cmd_size_ - new_size;
        for (unsigned i = 0; i < backtrack_size; ++i)
          tracker_.on_backspace();
      }
      current_line_.remove_last(n);
    } break;
    case args: {
      auto new_size = current_line_.size() - n;
      if (new_size <= cmd_size_) {
        // back to active
        state_ = active;
        auto backtrack_size = cmd_size_ - new_size;
        for (unsigned i = 0; i < backtrack_size; ++i)
          tracker_.on_backspace();
      } else if (new_size <= start_of_args_) {
        // back to args_start
        state_ = args_start;
      } else {
        // stay in args
      }
      current_line_.remove_last(n);
    } break;
    default:
      return Error::invalid_state;
    }
    for (std::size_t i = 0; i < n; ++i)
      if (auto err = out_.control(Control::backspace); err != Error::none)
        return err;
    return Error::none;
  }

  constexpr Error on_cursor_down() {
    if constexpr (Cfg::use_history) {
      auto str = history_.cursor_down();
      on_backspace(current_line_.size());
      for (const auto &ch : str)
        process_char(ch);
    }
    return Error::none;
  }

  constexpr Error on_cursor_up() {
    if constexpr (Cfg::use_history) {
      auto str = history_.cursor_up();
      on_backspace(current_line_.size());
      for (const auto &ch : str)
        process_char(ch);
    }
    return Error::none;
  }

  constexpr Error process_one() {
    event_type ev{};
    if (not in_.pop_event(ev))
      return Error::buffer_underflow;

    if (ev.is_char) {
      return process_char(ev.c);
    }

    // TODO: delete_char
    switch (ev.ctrl) {
    case Control::autocomplete:
      return on_autocomplete();
    case Control::backspace:
      return on_backspace();
    case Control::enter:
      return on_enter();
    case Control::cursor_down:
      return on_cursor_down();
    case Control::cursor_up:
      return on_cursor_up();
    default:
      return out_.control(ev.ctrl);
    }
    return Error::none;
  }

  constexpr void print(const CommandNode<char_type> &c, std::size_t indent) {
    if (indent == 0) {
      out_.write(c.name);
      out_.write(':');
      out_.write(' ');
      out_.write(c.description);
    } else {
      for (std::size_t i = 0; i < 2 * indent; ++i)
        out_.write(' ');
      out_.write(c.name);
      out_.write(' ');
      out_.write('[');
      out_.write(c.type);
      out_.write(']');
      out_.write(':');
      out_.write(' ');
      out_.write(c.description);
    }

    out_.control(Control::enter);
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

  constexpr Error on_enter() {
    if (current_line_.size() == 0) {
      print();
      return Error::none;
    }

    history_.push(line_view());
    switch (state_) {
    case active:
      [[fallthrough]];
    case args_start:
      // a valid command without trailing "=value" is a get command
      return process_get_param();
    case args: {
      auto line = line_view();
      auto arguments = line.substr(start_of_args_);
      switch (cmd_type_) {
      case ExecType::get:
        return process_get_param();
      case ExecType::set:
        return process_set_param(arguments);
      case ExecType::call: {
        const auto closing_bracket_pos = arguments.find_last_of(')');
        if (closing_bracket_pos == View<const char_type>::npos)
          return Error::expected_rparen;
        return process_call(arguments.substr(0, closing_bracket_pos));
      }
      default:
        return Error::implementation_error;
      }
    }
    }
  }

  constexpr CommandNode<char_type> &root() noexcept { return *commands.root(); }
  constexpr const CommandNode<char_type> &root() const noexcept {
    return *commands.root();
  }

  const CommandNode<char_type> *get_cmd() const {
    if constexpr (Cfg::use_autocomplete) {
      return tracker_.cmd();
    } else {
      const char_type chars[3]{' ', '(', '='};
      auto cmd_path = line_view().substr(0, line_view().find_first_of(chars));
      return commands.get_command(cmd_path);
    }
  }

  constexpr void return_to_idle() {
    current_line_.clear();
    tracker_.clear();
    state_ = active;
    cmd_type_ = ExecType::none;
    cmd_size_ = 0;
    start_of_args_ = 0;
  }

  constexpr Error return_to_idle(Error error) {
    return_to_idle();
    if (error != Error::none) {
      if (auto err = out_.write("Error: "); err != Error::none)
        return err;
      if (auto err = out_.write(ctti::enum_name<Error, char_type>(error));
          err != Error::none)
        return err;
    }
    if (auto err = out_.control(Control::enter); err != Error::none)
      return err;
    return error;
  }

  constexpr Error process_get_param() {
    View<char_type> out{output_line_.data(), output_line_.size()};
    auto cmd = get_cmd();

    if (auto err = out_.control(Control::enter); err != Error::none)
      return err;

    if (cmd == nullptr) {
      return return_to_idle(Error::invalid_cmd);
    }

    const auto error = cmd->execute(ExecType::get, {}, out);

    if (error != Error::none) {
      return return_to_idle(error);
    }

    if (auto err = out_.write(out); err != Error::none)
      return err;

    if (auto err = out_.control(Control::enter); err != Error::none)
      return err;

    return_to_idle();
    return error;
  }

  constexpr Error process_set_param(View<const char_type> args) {
    View<char_type> out{output_line_.data(), output_line_.size()};
    auto cmd = get_cmd();

    if (auto err = out_.control(Control::enter); err != Error::none)
      return err;

    if (cmd == nullptr) {
      return return_to_idle(Error::invalid_cmd);
    }

    const auto error = cmd->execute(ExecType::set, parse::skip_ws(args), out);

    return return_to_idle(error);
  }

  constexpr Error process_call(View<const char_type> args) {
    View<char_type> out{output_line_.data(), output_line_.size()};
    auto cmd = get_cmd();

    if (auto err = out_.control(Control::enter); err != Error::none)
      return err;

    if (cmd == nullptr) {
      return return_to_idle(Error::invalid_cmd);
    }

    const auto error = cmd->execute(ExecType::call, args, out);
    if (error != Error::none) {
      return return_to_idle(error);
    }

    if (auto err = out_.write(out); err != Error::none)
      return err;

    if (auto err = out_.control(Control::enter); err != Error::none)
      return err;

    return_to_idle();
    return Error::none;
  }

  CommandNode<char_type> *find_parent(CommandNode<char_type> *root,
                                      VecView<CharView>::iterator begin,
                                      VecView<CharView>::iterator end) {
    while (begin != end) {
      auto sub = root->subcommand;
      bool found = false;
      while (sub) {
        if (sub->name == *begin) {
          root = sub;
          found = true;
          break;
        }
        sub = sub->next;
      }
      if (not found)
        return nullptr;
      ++begin;
    }
    return root;
  }

  Error transmit_current_line() {
    // TODO: dont use command_terminator
    auto err = out_.write(Cfg::command_terminator);
    if (err != Error::none)
      return err;
    err = out_.write(
        View<const char_type>(output_line_.begin(), output_line_.end()));
    if (err != Error::none)
      return err;
    return out_.write(View<const char_type>(Cfg::command_terminator));
  }

  Error process_message() {
    // a whole message block is received, i.e. a complete command is
    // available.
    // 1. parse string and split it into the path and arguments
  }

  template <Config, Command...> friend class CommandTree;

  CommandTree<Cfg, Commands...> commands;
  cli::History<Cfg> history_;
  FixedSizeVector<char_type, Cfg::max_line_length> current_line_{};
  std::array<char_type, Cfg::max_line_length> output_line_{};
  input_type in_{};
  output_type out_;
  Tracker<Cfg, Commands...> tracker_{};
  std::size_t cmd_size_{0};
  std::size_t start_of_args_{0};
  State state_ = active;
  ExecType cmd_type_{ExecType::none};
};

template <Config Cfg, cli::Output<typename Cfg::char_type> Out,
          Command... Commands>
Cli(Cfg &&, Out &&, Commands &&...)
    -> Cli<std::remove_cvref_t<Cfg>, std::remove_cvref_t<Out>,
           std::remove_cvref_t<Commands>...>;

} // namespace cli
#endif
