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
#include "cli/exec_result.hpp"
#include "cli/string.hpp"
#include "cli/tuple.hpp"
#include "cli/type_list.hpp"

#include <cstddef>

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
    ExecResult<CharT> (*exec_)(void *this_,
                               View<const CharT> args,
                               View<CharT> out) = nullptr;
    /// gets extra help, used for function commands
    cli::View<const CharT> (*help_context_)(const void *this_,
                                            cli::View<const CharT>) = nullptr;
    // the parent command
    CommandNode *parent = nullptr;
    /// the next sibling command
    CommandNode *next = nullptr;
    /// pointers to the first sub command
    CommandNode *subcommand = nullptr;

    class iterator {
      friend struct CommandNode;
      CommandNode *node = nullptr;
      constexpr iterator(CommandNode *n) noexcept
        : node(n) {}

    public:
      constexpr iterator(const iterator &) noexcept = default;
      constexpr iterator(iterator &&) noexcept = default;
      constexpr iterator &operator=(const iterator &) noexcept = default;
      constexpr iterator &operator=(iterator &&) noexcept = default;

      constexpr iterator &operator++() noexcept {
        node = node->next;
        return *this;
      }

      constexpr iterator operator++(int) noexcept {
        iterator ret{node};
        node = node->next;
        return ret;
      }

      constexpr CommandNode *operator->() noexcept { return node; }

      constexpr const CommandNode *operator->() const noexcept { return node; }

      constexpr CommandNode &operator*() noexcept { return *node; }

      constexpr const CommandNode &operator*() const noexcept { return *node; }

      constexpr auto operator<=>(const iterator &) const noexcept = default;
    };

    constexpr iterator begin() noexcept { return subcommand; }
    constexpr iterator end() const noexcept { return nullptr; }
    constexpr const iterator begin() const noexcept { return subcommand; }

    /**
     * executes the command
     *
     * @param args the arguments
     * @param out where to put the results
     * @return the error
     */
    constexpr ExecResult<CharT> execute(View<const CharT> args,
                                        View<CharT> out) const noexcept {
      return (*exec_)(this_, args, out);
    }

    constexpr cli::View<const CharT>
    help_context(cli::View<const CharT> arg) const noexcept {
      if (help_context_)
        return (*help_context_)(this, arg);
      else
        return {};
    }

    /**
     * adds a subcommand to this
     *
     * @param c the command to add
     */
    constexpr void add_sub(CommandNode &c) noexcept {
      c.parent = this;
      c.next = nullptr;
      c.subcommand = nullptr;
      CommandNode *sub = subcommand;
      if (sub == nullptr) {
        // empty
        subcommand = &c;
      } else if (c.name < sub->name) {
        // c should be inserted as first
        subcommand = &c;
        c.next = sub;
      } else {
        // c should be inserted somewhere in the middle
        CommandNode *last_sub = sub;
        sub = sub->next;
        while (sub != nullptr) {
          if (c.name < sub->name) {
            last_sub->next = &c;
            c.next = sub;
            return;
          }
          last_sub = sub;
          sub = sub->next;
        }
        last_sub->next = &c;
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
    static_assert(Id<Name>,
                  "Name must be a valid identifier. Name can't contain "
                  "whitespace, or any of the characters (){},='\"");
    using char_type = typename Name::char_type;
    using sub_command_list = TypeList<SubCommands...>;
    using name_type = Name;
    static constexpr Name name{};
    static constexpr Description description{};
    static constexpr Type type{};

    constexpr CommandBase(const CommandBase &) = default;
    constexpr CommandBase(CommandBase &&) = default;
    constexpr CommandBase &operator=(const CommandBase &) = default;
    constexpr CommandBase &operator=(CommandBase &&) = default;

    constexpr CommandBase()
      requires(sizeof...(SubCommands) > 0)
    {}

    template<concepts::Command... SubCommands_>
    constexpr CommandBase(SubCommands_ &&...cmds) noexcept
      : subcommands{std::forward<SubCommands_>(cmds)...} {}

    constexpr CommandBase(cli::Tuple<SubCommands...> &&cmds) noexcept
      : subcommands{std::move(cmds)} {}

    constexpr CommandBase(const cli::Tuple<SubCommands...> &cmds) noexcept
      : subcommands{cmds} {}

    /**
     * executes the command
     *
     * @param args the arguments
     * @param out where to put the results
     * @return the error
     */
    constexpr ExecResult<char_type> execute(View<const char_type> args,
                                            View<char_type> out) noexcept {
      return static_cast<Derived *>(this)->execute(args, out);
    }

    template<std::size_t I>
    constexpr friend auto &get(CommandBase &cmd) {
      return get<I>(cmd.subcommands);
    }

    template<std::size_t I>
    constexpr friend const auto &get(const CommandBase &cmd) {
      return get<I>(cmd.subcommands);
    }

  private:
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

    cli::Tuple<SubCommands...> subcommands{};
  };

  template<typename CharT>
  struct SplitResult {
    const CommandNode<CharT> *command{nullptr};
    View<const CharT> args{};
  };

  template<typename CharT>
  constexpr const CommandNode<CharT> *
  get_command(View<const CharT> command,
              const CommandNode<CharT> *root,
              CharT access_separator) noexcept {

    if (command.size() == 0 or root == nullptr)
      return nullptr;

    std::size_t pos = command.find_first_of(
      View<const CharT>{string_constant<CharT, ' ', '(', '=', ')'>{}});

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
                                          CharT access_separator) noexcept {
    if (line.size() == 0)
      return {};

    std::size_t pos = line.find_first_of(
      View<const CharT>{string_constant<CharT, ' ', '(', '=', ')'>{}});

    if (pos == 0)
      return {};

    const View<const CharT> name = line.substr(0, pos);
    const View<const CharT> args = line.substr(pos);
    const CommandNode<CharT> *cmd = get_command(name, root, access_separator);
    return {cmd, args};
  }

} // namespace cli
#endif
