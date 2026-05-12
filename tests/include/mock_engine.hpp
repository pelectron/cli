#ifndef CLI_TESTS_MOCK_ENGINE_HPP
#define CLI_TESTS_MOCK_ENGINE_HPP
#include "cli/command_tree.hpp"
#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "test_display.hpp"

template<cli::concepts::Command... Commands>
struct MockEngine {
  using config_type = cli::default_config;
  using char_type = typename config_type::char_type;
  using display_type = MultilineDisplay;

  constexpr MockEngine(Commands... commands)
    : tree(*this, commands...) {}

  constexpr MockEngine(cli::Tuple<Commands...> &&commands)
    : tree(*this, std::move(commands)) {}

  constexpr MockEngine(const cli::Tuple<Commands...> &commands)
    : tree(*this, commands) {}

  constexpr void print() { print_called = true; }

  constexpr const cli::CommandNode<char_type> *root() const {
    return tree.root();
  }

  bool print_called = false;
  cli::CommandTree<MockEngine<Commands...>, Commands...> tree;
  MultilineDisplay display_{};
};
#endif
