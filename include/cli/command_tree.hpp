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
  template<class Engine, concepts::Command... Commands>
  class CommandTree {
  public:
    using config_type = typename Engine::config_type;
    using char_type = typename config_type::char_type;
    using command_node = CommandNode<char_type>;

    template<concepts::Command... Cmds>
    constexpr CommandTree(Engine &e, Cmds &&...cmds)
      : commands_{create_help(e), std::forward<Cmds>(cmds)...} {
      init_commands();
    }

    template<concepts::Command... Cmds>
    constexpr CommandTree(Engine &e, const std::tuple<Cmds...> &cmds)
      : commands_{init_tuple(e, std::move(cmds))} {
      init_commands();
    }

    template<concepts::Command... Cmds>
    constexpr CommandTree(Engine &e, std::tuple<Cmds...> &&cmds)
      : commands_{init_tuple(e, cmds)} {
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
      auto end = cmd_path.find_first_of(config_type::access_separator);
      while (end != View<const char_type>::npos) {
        auto s = cmd_path.substr(0, end);
        bool found = false;
        for (const auto &sub : *node) {
          if (sub.name == s) {
            node = &sub;
            cmd_path = cmd_path.substr(end + 1);
            end = cmd_path.find_last_of(config_type::access_separator);
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
    using Help = HelpCommand<Engine>;

    std::array<command_node, (num_cmds_v<Commands> + ...) + 2> cmds_{};
    std::tuple<Help, Commands...> commands_{};

    static constexpr Help create_help(Engine &e) {
      return create_help_cmd<Engine>(e);
    }

    template<concepts::Command... Cmds>
    constexpr std::tuple<Help, Commands...>
    init_tuple(Engine &e, const std::tuple<Cmds...> &t) {
      return [&t, &e, this]<std::size_t... Is>(
               std::index_sequence<Is...>) -> std::tuple<Help, Commands...> {
        return {create_help(e), std::get<Is>(t)...};
      }(std::make_index_sequence<sizeof...(Commands)>{});
    }

    template<concepts::Command... Cmds>
    constexpr std::tuple<Help, Commands...>
    init_tuple(Engine &e, std::tuple<Cmds...> &&t) {
      return [&t, &e, this]<std::size_t... Is>(
               std::index_sequence<Is...>) -> std::tuple<Help, Commands...> {
        return {create_help(e), std::move(std::get<Is>(t))...};
      }(std::make_index_sequence<sizeof...(Commands)>{});
    }

    template<concepts::Command Cmd>
    constexpr void
    init_cmd(std::size_t &index, CommandNode<char_type> &parent, Cmd &cmd) {
      // initialize the node
      CommandNode<char> &node = cmds_[index];
      node.name = Cmd::name;
      node.description = Cmd::description;
      node.type = Cmd::type;
      node.this_ = &cmd;
      node.exec_ = +[](void *this_,
                       View<const char_type> args,
                       View<char_type> &out,
                       bool &newline) -> Error {
        return static_cast<Cmd *>(this_)->execute(args, out, newline);
      };
      // add the node to the parent
      parent.add_sub(node);
      // initialize sub commands of cmd
      if constexpr (requires { cmd.subcommands; })
        for_each(
          [this, &index, &node](concepts::Command auto &c) {
            this->init_cmd(++index, node, c);
          },
          cmd.subcommands);
    }

    constexpr void init_commands() {
      auto &root = cmds_[0];
      root.name = config_type::name;
      root.description = config_type::description;
      std::size_t index = 0;
      for_each([this, &index, &root](
                 auto &cmd) { this->init_cmd(++index, root, cmd); },
               commands_);
    }
  };

  template<concepts::Config Cfg, concepts::Command... Cmds>
  CommandTree(Cfg &&, Cmds &&...)
    -> CommandTree<std::remove_cvref_t<Cfg>, std::remove_cvref_t<Cmds>...>;

  template<concepts::Config Cfg, concepts::Command... Cmds>
  CommandTree(Cfg &&, std::tuple<Cmds...> &&)
    -> CommandTree<std::remove_cvref_t<Cfg>, std::remove_cvref_t<Cmds>...>;

  template<concepts::Config Cfg, concepts::Command... Cmds>
  CommandTree(Cfg &&, const std::tuple<Cmds...> &)
    -> CommandTree<std::remove_cvref_t<Cfg>, std::remove_cvref_t<Cmds>...>;
} // namespace cli

#endif
