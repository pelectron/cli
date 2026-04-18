#ifndef CLI_COMMAND_TREE_HPP
#define CLI_COMMAND_TREE_HPP

#include "cli/command.hpp"
#include "cli/concepts.hpp"
#include "cli/help.hpp"

#include <tuple>
#include <utility>

namespace cli {

  /**
   * The CommandTree holds all the commands and sets up a tree of
   * CommandNodes.
   *
   * @tparam Cfg the cli configuration
   * @tparam Commands the commands
   */
  template<Config Cfg, Command... Commands>
  class CommandTree {
  public:
    using config = Cfg;
    using char_type = typename config::char_type;
    using command_node = CommandNode<char_type>;

    template<Config Cfg_, Command... Cmds>
    constexpr CommandTree(Cfg_ &&, Cmds &&...cmds)
      : commands_{create_help(cmds_[0]), std::forward<Cmds>(cmds)...} {
      init_commands();
    }

    template<Config Cfg_, Command... Cmds>
    constexpr CommandTree(Cfg_ &&, const std::tuple<Cmds...> &cmds)
      : commands_{init_tuple(std::move(cmds))} {
      init_commands();
    }

    template<Config Cfg_, Command... Cmds>
    constexpr CommandTree(Cfg_ &&, std::tuple<Cmds...> &&cmds)
      : commands_{init_tuple(cmds)} {
      init_commands();
    }

    /**
     * returns the root command
     */
    constexpr command_node *root() noexcept { return cmds_.data(); }

    /**
     * returns the root command
     */
    constexpr const command_node *root() const noexcept { return cmds_.data(); }

    /**
     * returns the command corresponding to cmd_path, or nullptr if the command
     * doesn't exist
     */
    constexpr const command_node *
    get_command(View<const char_type> cmd_path) const {
      if (cmd_path.size() == 0)
        return nullptr;

      const command_node *node = root();
      auto end = cmd_path.find_first_of(config::access_separator);
      while (end != View<const char_type>::npos) {
        auto s = cmd_path.substr(0, end);
        bool found = false;
        for (const auto &sub : *node) {
          if (sub.name == s) {
            node = &sub;
            cmd_path = cmd_path.substr(end + 1);
            end = cmd_path.find_last_of(config::access_separator);
            found = true;
            break;
          }
        }
        if (not found)
          return nullptr;
      }

      for (const auto &sub : *node) {
        if (sub.name == cmd_path) {
          return &sub;
        }
      }

      return nullptr;
    }

  private:
    using Help = HelpCommand<char_type, config::access_separator>;

    std::array<command_node, (num_cmds_v<Commands> + ...) + 2> cmds_{};
    std::tuple<Help, Commands...> commands_{};

    static constexpr Help create_help(const CommandNode<char_type> &root) {
      return create_help_cmd<char_type, config::access_separator>(root);
    }

    template<Command... Cmds>
    constexpr std::tuple<Help, Commands...>
    init_tuple(const std::tuple<Cmds...> &t) {
      return [&t, this]<std::size_t... Is>(
               std::index_sequence<Is...>) -> std::tuple<Help, Commands...> {
        return {create_help(cmds_[0]), std::get<Is>(t)...};
      }(std::make_index_sequence<sizeof...(Commands)>{});
    }

    template<Command... Cmds>
    constexpr std::tuple<Help, Commands...>
    init_tuple(std::tuple<Cmds...> &&t) {
      return [&t, this]<std::size_t... Is>(
               std::index_sequence<Is...>) -> std::tuple<Help, Commands...> {
        return {create_help(cmds_[0]), std::move(std::get<Is>(t))...};
      }(std::make_index_sequence<sizeof...(Commands)>{});
    }

    template<Command Cmd>
    constexpr void
    init_cmd(std::size_t &index, CommandNode<char_type> &parent, Cmd &cmd) {
      // initialize the node
      CommandNode<char> &node = cmds_[index];
      node.name = Cmd::name;
      node.description = Cmd::description;
      node.type = Cmd::type;
      node.this_ = &cmd;
      node.exec_ = +[](void *this_,
                       ExecType type,
                       View<const char_type> args,
                       View<char_type> &out) -> Error {
        return static_cast<Cmd *>(this_)->execute(type, args, out);
      };
      // add the node to the parent
      parent.add_sub(node);
      // initialize sub commands of cmd
      if constexpr (requires { cmd.subcommands; })
        for_each([this, &index, &node](
                   Command auto &c) { this->init_cmd(++index, node, c); },
                 cmd.subcommands);
    }

    constexpr void init_commands() {
      auto &root = cmds_[0];
      root.name = config::name;
      root.description = config::description;
      std::size_t index = 0;
      for_each([this, &index, &root](
                 auto &cmd) { this->init_cmd(++index, root, cmd); },
               commands_);
    }
  };

  template<Config Cfg, Command... Cmds>
  CommandTree(Cfg &&, Cmds &&...)
    -> CommandTree<std::remove_cvref_t<Cfg>, std::remove_cvref_t<Cmds>...>;

  template<Config Cfg, Command... Cmds>
  CommandTree(Cfg &&, std::tuple<Cmds...> &&)
    -> CommandTree<std::remove_cvref_t<Cfg>, std::remove_cvref_t<Cmds>...>;

  template<Config Cfg, Command... Cmds>
  CommandTree(Cfg &&, const std::tuple<Cmds...> &)
    -> CommandTree<std::remove_cvref_t<Cfg>, std::remove_cvref_t<Cmds>...>;
} // namespace cli

#endif
