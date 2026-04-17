#ifndef CLI_COMMAND_HPP
#define CLI_COMMAND_HPP

#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "cli/string.hpp"
#include "cli/type_list.hpp"

#include <tuple>

namespace cli {

/**
 * the elements of the command tree
 *
 * @tparam CharT the character type
 */
template <typename CharT> struct CommandNode {
  /// holds the command's name
  View<const CharT> name{};
  /// the command's description
  View<const CharT> description{};
  /// the commands type as a string, i.e. the value type for parameters and the
  /// function signatures for functions.
  View<const CharT> type{};
  /// points to the actual command
  void *this_ = nullptr;
  /// executes the command
  Error (*exec_)(void *, ExecType, View<const CharT>, View<CharT> &) = nullptr;
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
    constexpr iterator(CommandNode *node) noexcept : node(node) {}

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
   * @param exec_type the expected type to execute
   * @param args the arguments
   * @param out where to put the results
   * @return the error
   */
  constexpr Error execute(ExecType exec_type, View<const CharT> args,
                          View<CharT> &out) const {
    if (this_ and exec_)
      return (*exec_)(this_, exec_type, args, out);
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
template <class Derived, SC Name, SC Description, SC Type,
          Command... SubCommands>
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

  template <Command... SubCommands_>
  constexpr CommandBase(SubCommands_ &&...cmds)
      : subcommands{std::forward<SubCommands_>(cmds)...} {}

  constexpr CommandBase(std::tuple<SubCommands...> &&cmds)
      : subcommands{std::move(cmds)} {}

  constexpr CommandBase(const std::tuple<SubCommands...> &cmds)
      : subcommands{cmds} {}

  /**
   * executes the command
   *
   * @param type the expected execution type
   * @param args the arguments
   * @param out where to put the results
   * @return the error
   */
  constexpr Error execute(ExecType type, View<const char_type> args,
                          View<char_type> &out) {
    return static_cast<Derived *>(this)->execute(type, args, out);
  }

protected:
  template <class D, SC C, SC Desc, SC H, Command... SubC>
  constexpr auto count_cmds(const CommandBase<D, C, Desc, H, SubC...> &c);
  template <class F, class D, SC C, SC Desc, SC H, Command... SubC,
            class... Args>
  friend constexpr void for_each(F &&f, CommandBase<D, C, Desc, H, SubC...> &t,
                                 Args &&...args);
  template <class F, class D, SC C, SC Desc, SC H, Command... SubC,
            class... Args>
  friend constexpr void
  for_each(F &&f, const CommandBase<D, C, Desc, H, SubC...> &t, Args &&...args);
  template <Config, Command...> friend class CommandTree;

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
template <class Derived, SC CmdName, SC Description, SC Type>
class CommandBase<Derived, CmdName, Description, Type> {
  template <Config, Command...> friend class CommandTree;

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
   * @param type the expected execution type
   * @param args the arguments
   * @param out where to put the results
   * @return the error
   */
  constexpr Error execute(ExecType type, View<const char_type> args,
                          View<char_type> &out) const {
    return static_cast<Derived *>(this)->execute(type, args, out);
  }
};

} // namespace cli
#endif
