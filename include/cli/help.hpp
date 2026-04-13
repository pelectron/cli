#ifndef CLI_HELP_HPP
#define CLI_HELP_HPP

#include "cli/command.hpp"
#include "cli/function.hpp"
#include "cli/string.hpp"
#include <utility>

namespace cli {

template <class CharT, CharT AccessSeparator> struct Help {
  using char_type = CharT;
  using error_message =
      string_constant<char_type, 'n', 'o', ' ', 's', 'u', 'c', 'h', ' ', 'c',
                      'o', 'm', 'm', 'a', 'n', 'd'>;
  constexpr View<const char_type> operator()(View<const char_type> cmd) const {
    if (cmd.size() == 0) {
      return error_message{};
    }

    const CommandNode<char_type> *node = &root;
    auto end = cmd.find_first_of(AccessSeparator);
    while (end != CharView::npos) {
      auto s = cmd.substr(0, end);
      bool found = false;
      for (const auto &sub : *node) {
        if (sub.name == s) {
          node = &sub;
          cmd = cmd.substr(end);
          end = cmd.find_last_of(AccessSeparator);
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

  const CommandNode<char_type> &root;
};

template <class CharT, CharT AccessSeparator>
constexpr auto create_help_cmd(const CommandNode<CharT> &root) {
  return funcs::func(
      string_constant<CharT, 'h', 'e', 'l', 'p'>{},
      cli::Help<CharT, AccessSeparator>{root},
      funcs::arg(string_constant<CharT, 'c', 'o', 'm', 'm', 'a', 'n', 'd'>{}));
}

template <class CharT, CharT AccessSeparator>
using HelpCommand = decltype(create_help_cmd<CharT, AccessSeparator>(
    std::declval<const CommandNode<CharT> &>()));

} // namespace cli
#endif
