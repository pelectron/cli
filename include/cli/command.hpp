/**
 * @file
 * @brief This file contains the CommandNode and CommandBase.
 *
 * @defgroup Commands Commands
 *
 * A command is functionality to execute on the CLI.
 *
 * There are two types of commands:
 * 1. [Functions](docs.md#functions)
 * 2. [parameters](docs.md#parameters)
 *
 * For a detailed explanation, see [here](docs.md#commands).
 */

#ifndef CLI_COMMAND_HPP
#define CLI_COMMAND_HPP

#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "cli/parse.hpp"
#include "cli/string.hpp"
#include "cli/type_list.hpp"

#include <tuple>

namespace cli {

  /**
   * the elements of the command tree
   *
   * @tparam CharT the character type
   */
  template<typename CharT>
  struct CommandNode {
    /// holds the command's name
    View<const CharT> name{};
    /// the command's description
    View<const CharT> description{};
    /// the commands type as a string, i.e. the value type for parameters and
    /// the function signatures for functions.
    View<const CharT> type{};
    /// points to the actual command
    void *this_ = nullptr;
    /// executes the command
    Error (*exec_)(void *, View<const CharT>, View<CharT> &, bool &) = nullptr;
    // the parent command
    CommandNode *parent = nullptr;
    /// the next sibling command
    CommandNode *next = nullptr;
    /// pointers to the first sub command
    CommandNode *subcommand = nullptr;
    /// pointers to the last sub command
    CommandNode *last_subcommand = nullptr;

    class iterator {
      friend struct CommandNode;
      CommandNode *node = nullptr;
      constexpr iterator(CommandNode *node) noexcept
        : node(node) {}

    public:
      constexpr iterator(const iterator &) noexcept = default;
      constexpr iterator(iterator &&) noexcept = default;
      constexpr iterator &operator=(const iterator &) noexcept = default;
      constexpr iterator &operator=(iterator &&) noexcept = default;

      constexpr iterator &operator++() {
        node = node->next;
        return *this;
      }

      constexpr iterator operator++(int) {
        iterator ret{node};
        node = node->next;
        return ret;
      }

      constexpr CommandNode *operator->() { return node; }

      constexpr const CommandNode *operator->() const { return node; }

      constexpr CommandNode &operator*() { return *node; }

      constexpr const CommandNode &operator*() const { return *node; }

      constexpr auto operator<=>(const iterator &) const noexcept = default;
    };

    constexpr iterator begin() { return subcommand; }
    constexpr iterator end() const { return nullptr; }
    constexpr const iterator begin() const { return subcommand; }

    /**
     * executes the command
     *
     * @param args the arguments
     * @param out where to put the results
     * @param should_print_newline will be set to true if a newline should be
     *        printed after execution.
     * @return the error
     */
    constexpr Error execute(View<const CharT> args,
                            View<CharT> &out,
                            bool &should_print_newline) const {
      if (this_ and exec_)
        return (*exec_)(this_, args, out, should_print_newline);
      return Error::invalid_cmd;
    }

    /**
     * adds a subcommand to this
     *
     * @param c the command to add
     */
    constexpr void add_sub(CommandNode &c) {
      c.parent = this;
      c.next = nullptr;
      c.subcommand = nullptr;
      c.last_subcommand = nullptr;
      CommandNode *sub = subcommand;
      if (sub == nullptr) {
        // empty
        subcommand = &c;
        last_subcommand = &c;
      } else if (c.name < sub->name) {
        // c should be inserted as first
        subcommand = &c;
        c.next = sub;
      } else if (c.name > last_subcommand->name) {
        // c should be inserted as last
        last_subcommand->next = &c;
        last_subcommand = &c;
      } else {
        // c should be inserted somewhere in the middle
        CommandNode *last_sub = sub;
        sub = sub->next;
        while (sub != nullptr) {
          if (c.name < sub->name) {
            last_sub->next = &c;
            c.next = sub;
            break;
          } else {
            last_sub = sub;
            sub = sub->next;
          }
        }
      }
    }
  };

  /**
   * The CRTP base class for commands
   *
   * @tparam Derived the derived class
   * @tparam Name the command name
   * @tparam Description the command description
   * @tparam Type the command type as a string, i.e. the parameters type or the
   * function signature
   * @tparam SubCommands the sub commands
   */
  template<class Derived,
           SC Name,
           SC Description,
           SC Type,
           concepts::Command... SubCommands>
  class CommandBase {
  public:
    using char_type = typename Name::char_type;
    using sub_command_list = TypeList<SubCommands...>;
    using name_type = Name;
    static constexpr Name name{};
    static constexpr Description description{};
    static constexpr Type type{};

    CommandBase() = delete;
    constexpr CommandBase(const CommandBase &) = default;
    constexpr CommandBase(CommandBase &&) = default;
    constexpr CommandBase &operator=(const CommandBase &) = default;
    constexpr CommandBase &operator=(CommandBase &&) = default;

    template<concepts::Command... SubCommands_>
    constexpr CommandBase(SubCommands_ &&...cmds)
      : subcommands{std::forward<SubCommands_>(cmds)...} {}

    constexpr CommandBase(std::tuple<SubCommands...> &&cmds)
      : subcommands{std::move(cmds)} {}

    constexpr CommandBase(const std::tuple<SubCommands...> &cmds)
      : subcommands{cmds} {}

    /**
     * executes the command
     *
     * @param args the arguments
     * @param out where to put the results
     * @param should_print_newline set to true if a newline should be printed
     *        after executing this command.
     * @return the error
     */
    constexpr Error execute(View<const char_type> args,
                            View<char_type> &out,
                            bool &should_print_newline) {
      return static_cast<Derived *>(this)->execute(
        args, out, should_print_newline);
    }

  protected:
    template<class D, SC C, SC Desc, SC H, concepts::Command... SubC>
    constexpr auto count_cmds(const CommandBase<D, C, Desc, H, SubC...> &c);
    template<class F,
             class D,
             SC C,
             SC Desc,
             SC H,
             concepts::Command... SubC,
             class... Args>
    friend constexpr void
    for_each(F &&f, CommandBase<D, C, Desc, H, SubC...> &t, Args &&...args);
    template<class F,
             class D,
             SC C,
             SC Desc,
             SC H,
             concepts::Command... SubC,
             class... Args>
    friend constexpr void for_each(F &&f,
                                   const CommandBase<D, C, Desc, H, SubC...> &t,
                                   Args &&...args);
    template<class Engine, concepts::Command...>
    friend class CommandTree;

    std::tuple<SubCommands...> subcommands{};
  };

  /**
   * The CRTP base class for commands without subcommands
   *
   * @tparam Derived the derived class
   * @tparam Name the command name
   * @tparam Description the command description
   * @tparam Type the command type as a string, i.e. the parameters type or the
   * function signature
   */
  template<class Derived, SC CmdName, SC Description, SC Type>
  class CommandBase<Derived, CmdName, Description, Type> {
    template<class Engine, concepts::Command...>
    friend class CommandTree;

  public:
    using char_type = typename CmdName::char_type;
    using sub_command_list = TypeList<>;
    static constexpr CmdName name{};
    static constexpr Description description{};
    static constexpr Type type{};

    constexpr CommandBase() = default;
    constexpr CommandBase(const CommandBase &) = default;
    constexpr CommandBase(CommandBase &&) = default;
    constexpr CommandBase &operator=(const CommandBase &) = default;
    constexpr CommandBase &operator=(CommandBase &&) = default;

    /**
     * executes the command
     *
     * @param args the arguments
     * @param out where to put the results
     * @param should_print_newline set to true if a newline should be printed
     *        after executing this command
     * @return the error
     */
    constexpr Error execute(View<const char_type> args,
                            View<char_type> &out,
                            bool &should_print_newline) {
      return static_cast<Derived *>(this)->execute(
        type, args, out, should_print_newline);
    }
  };

  template<typename CharT>
  struct SplitResult {
    const CommandNode<CharT> *command{nullptr};
    View<const CharT> args{};
  };

  template<typename CharT>
  constexpr SplitResult<CharT> split_line(View<const CharT> line,
                                          CharT access_separator) {
    if (line.size() == 0)
      return {nullptr};

    std::size_t pos =
      line.find_first_of(string_constant<CharT, ' ', '(', '='>{});
    const View<const CharT> rest = parse::skip_ws(line.substr(pos));
    return {nullptr, rest};
  }

  template<typename CharT>
  constexpr const CommandNode<CharT> *
  get_command(View<const CharT> command,
              const CommandNode<CharT> *root,
              CharT access_separator) {

    if (command.size() == 0)
      return nullptr;

    std::size_t pos = command.find_first_of(
      View<const CharT>{string_constant<CharT, ' ', '(', '='>{}});

    if (pos == 0)
      return {};

    // get the whole command
    View<const CharT> cmd_name = command;

    if (cmd_name[cmd_name.size() - 1] == access_separator)
      return nullptr;

    // find the parent node
    const CommandNode<CharT> *parent = root;
    std::size_t end = cmd_name.find_first_of(access_separator);
    while (end != View<const CharT>::npos) {
      View name = cmd_name.substr(0, end);
      bool found = false;
      for (const CommandNode<CharT> &child : *parent) {
        if (child.name == name) {
          parent = &child;
          cmd_name = cmd_name.substr(end + 1);
          end = cmd_name.find_last_of(access_separator);
          found = true;
          break;
        }
      }
      if (not found)
        return {nullptr};
    }

    // find the child
    for (const CommandNode<CharT> &child : *parent) {
      if (child.name == cmd_name) {
        return &child;
      }
    }

    return nullptr;
  }

  template<typename CharT>
  constexpr SplitResult<CharT> split_line(View<const CharT> line,
                                          const CommandNode<CharT> *root,
                                          CharT access_separator) {
    if (line.size() == 0)
      return {};

    std::size_t pos = line.find_first_of(
      View<const CharT>{string_constant<CharT, ' ', '(', '='>{}});

    if (pos == 0)
      return {};

    // get the whole command
    View<const CharT> cmd_name;
    if (pos == View<const CharT>::npos) {
      cmd_name = line;
    } else {
      cmd_name = line.substr(0, pos);
    }

    if (cmd_name[cmd_name.size() - 1] == access_separator) {
      return {};
    }

    // find the parent node
    const CommandNode<CharT> *parent = root;
    std::size_t end = cmd_name.find_first_of(access_separator);
    while (end != View<const CharT>::npos) {
      View name = cmd_name.substr(0, end);
      bool found = false;
      for (const CommandNode<CharT> &child : *parent) {
        if (child.name == name) {
          parent = &child;
          cmd_name = cmd_name.substr(end + 1);
          end = cmd_name.find_last_of(access_separator);
          found = true;
          break;
        }
      }
      if (not found)
        return {nullptr};
    }

    // find the child
    for (const CommandNode<CharT> &child : *parent) {
      if (child.name == cmd_name) {
        return {&child, line.substr(pos)};
      }
    }

    return {};
  }
} // namespace cli
#endif
