#ifndef CLI_HELP_HPP
#define CLI_HELP_HPP

#include "cli/command.hpp"
#include "cli/config.hpp"
#include "cli/display.hpp"
#include "cli/function.hpp"
#include "cli/string.hpp"

#include <utility>

namespace cli {

  /**
   * The help function
   */
  template<typename Engine>
  struct Help {
    using config_type = typename Engine::config_type;
    using char_type = typename config_type::char_type;

    static constexpr View<const char_type> cmd_not_found{
      string_constant<char_type,
                      'n',
                      'o',
                      ' ',
                      's',
                      'u',
                      'c',
                      'h',
                      ' ',
                      'c',
                      'o',
                      'm',
                      'm',
                      'a',
                      'n',
                      'd'>{}};

    static constexpr View<const char_type> no_desc_available{
      string_constant<char_type,
                      'n',
                      'o',
                      ' ',
                      'd',
                      'e',
                      's',
                      'c',
                      'r',
                      'i',
                      'p',
                      't',
                      'i',
                      'o',
                      'n',
                      ' ',
                      'a',
                      'v',
                      'a',
                      'i',
                      'l',
                      'a',
                      'b',
                      'l',
                      'e'>{}};

    constexpr void write(View<const char_type> s) const {
      engine.display_.write(s);
    }

    /**
     * returns the description of cmd
     *
     * @param cmd the command
     */
    constexpr void operator()(View<const char_type> cmd,
                              View<const char_type> arg) const {
      engine.display_.newline();
      if (cmd.size() == 0) {
        if (arg.size() != 0)
          return write(cmd_not_found);

        if constexpr (config::empty_help_prints_commands_v<config_type>)
          return engine.print();
        else
          return write(cmd_not_found);
      }

      const CommandNode<char_type> *cmd_node =
        get_command(cmd, engine.root(), config_type::access_separator);

      if (cmd_node == nullptr)
        return write(cmd_not_found);

      if (arg.size() == 0) {
        engine.display_.write('[');
        engine.display_.write(cmd_node->type);
        engine.display_.write(
          View<const char_type>{string_constant<char_type, ']', ':', ' '>{}});

        if (cmd_node->description.size() == 0)
          return write(no_desc_available);
        else
          return write(cmd_node->description);
      }

      const View context_help = cmd_node->help_context(arg);
      if (context_help.size() == 0)
        write(no_desc_available);
      else
        write(context_help);
    }

    Engine &engine;
  };

  template<class Engine>
  constexpr auto create_help_cmd(Engine &engine) {
    return funcs::func(
      string_constant<typename Engine::char_type, 'h', 'e', 'l', 'p'>{},
      string_constant<typename Engine::char_type,
                      'p',
                      'r',
                      'i',
                      'n',
                      't',
                      's',
                      ' ',
                      'h',
                      'e',
                      'l',
                      'p'>{},
      cli::Help<Engine>{engine},
      funcs::arg<cli::View<const typename Engine::char_type>,
                 string_constant<typename Engine::char_type>{}>(
        string_constant<typename Engine::char_type, 'c', 'm', 'd'>{}),
      funcs::arg<cli::View<const typename Engine::char_type>,
                 string_constant<typename Engine::char_type>{}>(
        string_constant<typename Engine::char_type, 'a', 'r', 'g'>{}));
  }

  template<class Engine>
  using HelpCommand =
    decltype(create_help_cmd<Engine>(std::declval<Engine &>()));
} // namespace cli
#endif
