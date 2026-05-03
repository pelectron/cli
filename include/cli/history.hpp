#ifndef CLI_HISTORY_HPP
#define CLI_HISTORY_HPP

#include "cli/config.hpp"
#include "cli/string.hpp"
#include "cli/util.hpp"

#include <array>
#include <cstddef>

namespace cli {

  /**
   * Handles command history with cursor up and down
   */
  template<concepts::Config Cfg>
  class History {
    static_assert(config::has_history_depth<Cfg>,
                  "Your configuration must have a static constexpr member of "
                  "type std::size_t called "
                  "history_depth, which must be grater than 0, because you "
                  "specified use_history as true.");

    using Str = View<const typename Cfg::char_type>;
    using Array = std::array<typename Cfg::char_type, Cfg::max_line_length>;
    struct Line {
      Array contents{};
      std::size_t size{};
      constexpr operator Str() const noexcept {
        return {contents.data(), size};
      }
    };
    using Index = smallest_type_for_value_t<Cfg::history_depth>;
    using Buffer = std::array<Line, Cfg::history_depth>;

    Buffer buffer{};
    Index current_{0}; // points to the curent history item in use
    Index head_{0};    // the latest addition to the history
    Index tail_{0};    // the first item added
    Index size_{0};    // the current size of the history
    bool last_action_was_push_{
      true}; // if true, the last action executed was push

    void increment_current() noexcept {
      if (size_ == 0)
        return;
      if (current_ < head_) {
        if (current_ + 1 != head_)
          ++current_;
      } else {
        // current_ >= head_
        if (current_ == Cfg::history_depth - 1 and head_ == 0)
          return;
        else if (current_ == Cfg::history_depth - 1)
          current_ = 0;
        else
          ++current_;
      }
    }

    void decrement_current() noexcept {
      if (size_ == 0 or current_ == tail_)
        return;
      else if (current_ == 0)
        current_ = Cfg::history_depth - 1;
      else
        --current_;
    }

    Index incr(Index &idx) const noexcept {
      ++idx;
      if (idx == Cfg::history_depth)
        return idx = 0;
      else
        return idx;
    }
    Index post_incr(Index &idx) const noexcept {
      auto ret = idx;
      ++idx;
      if (idx == Cfg::history_depth)
        idx = 0;
      else
        idx;
      return ret;
    }

    Index decr(Index &idx) const noexcept {
      if (idx == 0)
        return idx = Cfg::history_depth - 1;
      else
        return --idx;
    }

    Index decr_c(Index idx) const noexcept {
      if (idx == 0)
        return idx = Cfg::history_depth - 1;
      else
        return --idx;
    }

    void copy_into(Line &line, Str str) noexcept {
      line.size = 0;
      for (const auto &ch : str) {
        line.contents[line.size++] = ch;
      }
      line.contents[line.size] = 0;
    }

    constexpr Str cursor_up() {
      if (size_ == 0)
        return {};

      if (not last_action_was_push_)
        decrement_current();
      Str ret = buffer[current_];
      last_action_was_push_ = false;
      return ret;
    }

    constexpr Str cursor_down() {
      if (size_ == 0)
        return {};

      if (not last_action_was_push_)
        increment_current();
      Str ret = buffer[current_];
      last_action_was_push_ = false;
      return ret;
    }

  public:
    /**
     * adds a new command to the history
     *
     * @param cmd the command
     */
    constexpr void push(Str cmd) {
      copy_into(buffer[head_], cmd);
      current_ = head_;
      if (size_ == 0) {
        tail_ = head_;
        size_ = 1;
      } else if (size_ == Cfg::history_depth) {
        incr(tail_);
      } else {
        ++size_;
      }
      incr(head_);
      last_action_was_push_ = true;
    }

    /**
     * goes backward in history
     *
     * @return the command
     */
    constexpr Str cursor_up(std::size_t n) {
      if (n == 0)
        return {};
      for (std::size_t i = 0; i < n - 1; ++i)
        cursor_up();
      return cursor_up();
    }

    /**
     * goes forward in history
     *
     * @return the command
     */
    constexpr Str cursor_down(std::size_t n) {
      if (n == 0)
        return {};
      for (std::size_t i = 0; i < n - 1; ++i)
        cursor_down();
      return cursor_down();
    }

    /**
     * resets the history, i.e. clears it
     */
    constexpr void reset() {
      current_ = 0;
      head_ = 0;
      tail_ = 0;
      size_ = 0;
      last_action_was_push_ = true;
    }
  };

  template<concepts::Config Cfg>
    requires(not Cfg::use_history)
  class History<Cfg> {
  public:
    using Str = View<const typename Cfg::char_type>;
    constexpr void push_cmd(Str) {}
    constexpr Str cursor_up(std::size_t) {}
    constexpr Str cursor_down(std::size_t) {}
    constexpr void reset() {}
  };
} // namespace cli
#endif
