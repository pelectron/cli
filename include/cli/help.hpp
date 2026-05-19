#ifndef CLI_HELP_HPP
#define CLI_HELP_HPP

#include "cli/command.hpp"
#include "cli/config.hpp"
#include "cli/function.hpp"
#include "cli/string.hpp"

namespace cli {

  /**
   * The help function
   */
  template<typename Engine>
  struct Help {
    using config_type = typename Engine::config_type;
    using char_type = config::char_type_t<config_type>;

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

    constexpr void write(View<const char_type> s) noexcept {
      engine->display_.write(s);
    }

    /**
     * executes the help command.
     *
     * @param cmd the command name
     * @param arg if cmd is a function command, arg can be a function argument
     *        to get the description of.
     */
    constexpr void operator()(View<const char_type> cmd,
                              View<const char_type> arg) noexcept {
      engine->display_.newline();
      if (cmd.size() == 0) {
        if (arg.size() != 0)
          return write(cmd_not_found);

        if constexpr (config::empty_help_prints_commands_v<config_type>)
          return engine->print();
        else
          return write(cmd_not_found);
      }

      const CommandNode<char_type> *cmd_node = get_command(
        cmd, engine->root(), config::access_separator_v<config_type>);

      if (cmd_node == nullptr)
        return write(cmd_not_found);

      if (arg.size() == 0) {
        engine->display_.write('[');
        engine->display_.write(cmd_node->type);
        engine->display_.write(
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

    Engine *engine;
  };

  template<class Engine>
  constexpr auto create_help_cmd() noexcept {
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
      cli::Help<Engine>{nullptr},
      funcs::arg<cli::View<const typename Engine::char_type>,
                 string_constant<typename Engine::char_type>{}>(
        string_constant<typename Engine::char_type, 'c', 'm', 'd'>{},
        string_constant<typename Engine::char_type,
                        't',
                        'h',
                        'e',
                        ' ',
                        'c',
                        'o',
                        'm',
                        'm',
                        'a',
                        'n',
                        'd'>{}),
      funcs::arg<cli::View<const typename Engine::char_type>,
                 string_constant<typename Engine::char_type>{}>(
        string_constant<typename Engine::char_type, 'a', 'r', 'g'>{},
        string_constant<typename Engine::char_type,
                        'f',
                        'u',
                        'n',
                        'c',
                        't',
                        'i',
                        'o',
                        'n',
                        ' ',
                        'a',
                        'r',
                        'g',
                        ' ',
                        'n',
                        'a',
                        'm',
                        'e'>{}));
  }

  template<class Engine>
  using HelpCommand = decltype(create_help_cmd<Engine>());
} // namespace cli
#endif
