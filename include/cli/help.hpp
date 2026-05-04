#ifndef CLI_HELP_HPP
#define CLI_HELP_HPP

#include "cli/command.hpp"
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

    using error_message = string_constant<char_type,
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
                                          'd'>;

    /**
     * returns the description of cmd
     *
     * @param cmd the command
     */
    constexpr View<const char_type>
    operator()(View<const char_type> cmd) const {
      if (cmd.size() == 0) {
        if constexpr (not config::empty_help_prints_commands_v<config_type>) {
          return error_message{};
        } else {
          engine.print();
          return {};
        }
      }

      const CommandNode<char_type> *node = engine.root();
      auto end = cmd.find_first_of(config_type::access_separator);
      while (end != CharView::npos) {
        auto s = cmd.substr(0, end);
        bool found = false;
        for (const auto &sub : *node) {
          if (sub.name == s) {
            node = &sub;
            cmd = cmd.substr(end + 1);
            end = cmd.find_last_of(config_type::access_separator);
            found = true;
            break;
          }
        }
        if (not found)
          return error_message{};
      }

      for (const auto &sub : *node) {
        if (sub.name == cmd) {
          node = &sub;
          return sub.description;
          break;
        }
      }

      return error_message{};
    }
    Engine &engine;
  };

  template<class Engine>
  constexpr auto create_help_cmd(Engine &engine) {
    return funcs::func(
      string_constant<typename Engine::char_type, 'h', 'e', 'l', 'p'>{},
      cli::Help<Engine>{engine},
      funcs::arg<cli::View<const typename Engine::char_type>,
                 string_constant<typename Engine::char_type>{}>(
        string_constant<typename Engine::char_type,
                        'c',
                        'o',
                        'm',
                        'm',
                        'a',
                        'n',
                        'd'>{}));
  }

  template<class Engine>
  using HelpCommand =
    decltype(create_help_cmd<Engine>(std::declval<Engine &>()));
} // namespace cli
#endif
