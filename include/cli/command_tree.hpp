#ifndef CLI_COMMAND_TREE_HPP
#define CLI_COMMAND_TREE_HPP

#include "cli/command.hpp"
#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "cli/help.hpp"
#include "cli/string.hpp"

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
      requires config::use_help_v<config_type>
    constexpr CommandTree(Engine &e, Cmds &&...cmds) noexcept
      : commands_{create_help_cmd(e), std::forward<Cmds>(cmds)...} {
      init_commands();
    }

    template<concepts::Command... Cmds>
      requires(not config::use_help_v<config_type>)
    constexpr CommandTree(Engine &, Cmds &&...cmds) noexcept
      : commands_{std::forward<Cmds>(cmds)...} {
      init_commands();
    }

    template<concepts::Command... Cmds>
    constexpr CommandTree(Engine &e, const std::tuple<Cmds...> &cmds) noexcept
      : commands_{init_tuple(e, std::move(cmds))} {
      init_commands();
    }

    template<concepts::Command... Cmds>
    constexpr CommandTree(Engine &e, std::tuple<Cmds...> &&cmds) noexcept
      : commands_{init_tuple(e, cmds)} {
      init_commands();
    }

    /**
     * returns the root command
     */
    constexpr const command_node *root() const noexcept { return cmds_.data(); }

    /**
     * returns the command corresponding to cmd_path, or nullptr if the command
     * doesn't exist
     */
    constexpr const command_node *
    get_command(View<const char_type> cmd_path) const noexcept {
      return cli::get_command(
        cmd_path, cmds_.data(), config_type::access_separator);
    }

  private:
    using Help = HelpCommand<Engine>;
    using CommandTuple = std::conditional_t<config::use_help_v<config_type>,
                                            std::tuple<Help, Commands...>,
                                            std::tuple<Commands...>>;
    using CommanNodeArray =
      std::array<command_node,
                 (num_cmds_v<Commands> + ...) +
                   (config::use_help_v<config_type> ? 2 : 1)>;

    CommanNodeArray cmds_{};
    CommandTuple commands_{};

    template<concepts::Command... Cmds>
    constexpr CommandTuple init_tuple(Engine &e,
                                      const std::tuple<Cmds...> &t) noexcept {
      return [&t, &e]<std::size_t... Is>(
               std::index_sequence<Is...>) -> CommandTuple {
        if constexpr (config::use_help_v<config_type>)
          return {create_help_cmd(e), std::get<Is>(t)...};
        else
          return {std::get<Is>(t)...};
      }(std::make_index_sequence<sizeof...(Commands)>{});
    }

    template<concepts::Command... Cmds>
    constexpr CommandTuple init_tuple(Engine &e,
                                      std::tuple<Cmds...> &&t) noexcept {
      return [&t, &e]<std::size_t... Is>(
               std::index_sequence<Is...>) -> CommandTuple {
        if constexpr (config::use_help_v<config_type>)
          return {create_help_cmd(e), std::move(std::get<Is>(t))...};
        else
          return {std::move(std::get<Is>(t))...};
      }(std::make_index_sequence<sizeof...(Commands)>{});
    }

    template<typename Cmd>
    static constexpr bool has_help_context =
      requires(const Cmd &cmd, cli::View<const char_type> arg) {
        { cmd.help_context(arg) } -> std::same_as<cli::View<const char_type>>;
      };

    template<concepts::Command Cmd>
    constexpr void init_cmd(std::size_t &index,
                            CommandNode<char_type> &parent,
                            Cmd &cmd) noexcept {
      // initialize the node
      CommandNode<char> &node = cmds_[index];
      node.name = Cmd::name;
      node.description = Cmd::description;
      node.type = Cmd::type;
      node.this_ = &cmd;
      node.exec_ = +[](void *this_,
                       View<const char_type> args,
                       View<char_type> out) -> ExecResult<char_type> {
        return static_cast<Cmd *>(this_)->execute(args, out);
      };

      if constexpr (has_help_context<Cmd>) {
        node.help_context_ = +[](const void *this_, View<const char_type> arg) {
          return static_cast<const Cmd *>(this_)->help_context(arg);
        };
      }

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

    constexpr void init_commands() noexcept {
      CommandNode<char_type> &root = cmds_[0];
      root.name = config_type::name;
      root.description = config_type::description;
      root.type = string_constant<char_type, 'r', 'o', 'o', 't'>{};
      std::size_t index = 0;
      for_each([this, &index, &root](
                 auto &cmd) { this->init_cmd(++index, root, cmd); },
               commands_);
    }
  };

} // namespace cli

#endif
