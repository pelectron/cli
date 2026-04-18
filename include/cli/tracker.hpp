#ifndef CLI_TRACKER_HPP
#define CLI_TRACKER_HPP
#include "cli/command.hpp"
#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "cli/util.hpp"
#include "cli/vector.hpp"
#include <array>

namespace cli {

  /**
   * The Tracker is responsible for parsing the command name input into a
   * CommandNode. Optionally provides autocomplete functionality.
   * TODO: add differentiation between functions and params for autocomplete
   *
   * Thsi class only tracks command names, but not values (in case of params) or
   * arguments (in case of functions)
   *
   * @tparam Cfg the cli configuration
   * @tparam Commands the cli commands
   * @tparam Cfg::use_autocomplete if true, autocomplete is enabled, else it is
   * disabled
   *
   * @{
   */

  template<Config Cfg, typename... Commands>
  class Tracker {
    using CharT = typename Cfg::char_type;
    constexpr Tracker(CommandNode<CharT> &) {}
    constexpr Error on_char(uint8_t c) { return Error::none; }
    constexpr void on_backspace() {}
    constexpr View<CharT> on_autocomplete() { return {}; }
    constexpr CommandNode<CharT> *cmd() { return nullptr; }
    constexpr void clear() {}
  };

  template<Config Cfg, Command... Commands>
    requires Cfg::use_autocomplete
  class Tracker<Cfg, Commands...> {
    using CharT = typename Cfg::char_type;
    // the maximum depth of the command tree
    static constexpr std::size_t Depth =
      std::max({num_levels_v<Commands>...}) + 1;
    // the maximum command length
    static constexpr std::size_t MaxNameLength =
      std::max({max_name_length_v<Commands>...});

    const CommandNode<CharT> &root;
    const CommandNode<CharT> *command_{};
    FixedCapacityVector<CharT, MaxNameLength> buffer{};
    std::array<CommandNode<CharT> *, Depth> cmds{nullptr};
    smallest_type_for_value_t<Depth> size = 0;
    smallest_type_for_value_t<MaxNameLength + 1> cmd_size = 0;
    CharT last_char = 0;

    constexpr View<const CharT> buf_view() const {
      return {buffer.data(), buffer.size()};
    }

    constexpr CommandNode<CharT> *candidate() {
      return size < Depth ? cmds[size] : nullptr;
    }

  public:
    /**
     * create a tracker
     * @param root the command tree root node
     */
    constexpr Tracker(CommandNode<CharT> &root)
      : root(root), command_(&root) {}

    /**
     * needs to be called when a character is encountered
     *
     * @param c the character
     * @return the rror, if any occured
     */
    constexpr Error on_char(CharT c) {
      if (command_ == &root and buffer.size() == 0) {
        for (const auto &cmd : root) {
          if (cmd.name[0] == c) {
            command_ = &cmd;
            buffer.push_back(c);
            last_char = c;
            return Error::none;
          }
        }
        return Error::invalid_cmd;
      }

      if (buffer.size() == 0) {
        for (const auto &cmd : *command_) {
          if (cmd.name[0] == c) {
            // found next candidate
            command_ = &cmd;
            buffer.push_back(c);
            last_char = c;
            return Error::none;
          }
        }
        return Error::invalid_cmd;
      }

      // buffer.size() > 0 and command_ != root
      if (c == Cfg::access_separator) {
        if (command_->name != buf_view())
          return Error::invalid_cmd;
        last_char = Cfg::access_separator;
        buffer.clear();
        return Error::none;
      }

      buffer.push_back(c);
      auto buf = buf_view();
      auto *cmd = command_;
      while (cmd and not cmd->name.starts_with(buf)) {
        cmd = cmd->next;
      }

      if (cmd == nullptr) {
        buffer.remove_last(1);
        return Error::invalid_cmd;
      }

      command_ = cmd;
      last_char = c;
      return Error::none;
    }

    /**
     * needs to be called when a backspace is encountered
     */
    constexpr void on_backspace() {
      if (command_ == &root) {
        assert(buffer.size() == 0);
        return;
      }

      if (buffer.size() == 0) {
        last_char = command_->name[command_->name.size() - 1];
        for (const CharT &c : command_->name)
          buffer.push_back(c);
        return;
      }

      if (buffer.size() == 1) {
        command_ = command_->parent;
        if (command_ == &root)
          last_char = 0;
        else
          last_char = Cfg::access_separator;
        buffer.clear();
        return;
      }

      // buffer.size() > 1
      buffer.remove_last(1);
      const auto buf = buf_view();
      for (const auto &cmd : *command_->parent) {
        if (cmd.name.starts_with(buf)) {
          command_ = &cmd;
          return;
        }
      }
    }

    /**
     * returns the remaining string for autocomplete
     */
    constexpr View<const CharT> on_autocomplete() {
      if (buffer.size() == 0 or last_char == Cfg::access_separator)
        return {};

      auto ret = command_->name.substr(buffer.size());
      if (ret.size() == 0) {
        on_char(Cfg::access_separator);
        return {&last_char, 1};
      }
      for (const CharT &c : ret)
        buffer.push_back(c);
      last_char = ret.back();
      return ret;
    }

    /**
     * returns the command that the tracker resolves to.
     */
    constexpr const CommandNode<CharT> *cmd() const { return command_; }

    /**
     * clears the tracker, i.e. resets its state.
     */
    constexpr void clear() {
      buffer.clear();
      last_char = Cfg::access_separator;
      command_ = &root;
    }
  };

  /// @}

} // namespace cli

#endif
