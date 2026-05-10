#ifndef CLI_EVENT_HPP
#define CLI_EVENT_HPP

#include "cli/concepts.hpp"
#include "cli/util.hpp"

#include <cstdint>

namespace cli {

  enum class Control : std::uint8_t {
    character,
    bell,
    backspace,
    autocomplete,
    cursor_up,
    cursor_down,
    cursor_left,
    cursor_right,
    delete_char,
    clear_screen,
    clear_line,
    clear_line_to_end,
    clear_line_to_begin,
    enter
  };

  /**
   * @brief Event represents either a character or control sequence.
   *
   * A @ref Input produces a list of events when processing the character input
   * stream.
   *
   * @tparam CharT the character type
   */
  template<typename CharT>
  class Event {
  public:
    constexpr Event() {}

    constexpr Event(CharT c) noexcept
      : type_(Control::character), payload_{c} {}

    constexpr Event(Control type) noexcept
      : type_{type}, payload_{0} {}

    constexpr Event(Control type, std::uint8_t param) noexcept
      : type_{type}, payload_{static_cast<CharT>(param)} {}

    constexpr Event(const Event &o) noexcept
      : type_(o.type_), payload_(o.payload_) {}

    // Event(const volatile Event &o)
    //   : type_(o.type_), payload_(o.payload_) {}

    constexpr Event &operator=(const Event &o) noexcept {
      type_ = o.type_;
      payload_ = o.payload_;
      return *this;
    }

    // constexpr Event &operator=(Event &&o) {
    //   type_ = o.type_;
    //   payload_ = o.payload_;
    //   return *this;
    // }

    volatile Event &operator=(const volatile Event &o) volatile noexcept {
      type_ = o.type_;
      payload_ = o.payload_;
      return *this;
    }

    constexpr Control type() const noexcept { return type_; }

    constexpr CharT as_char() const noexcept {
      CLI_ASSERT(type_ == Control::character);
      return payload_;
    }

    constexpr std::uint8_t param() const noexcept { return payload_; }

    Control type() const volatile noexcept { return type_; }

    CharT as_char() const volatile noexcept {
      CLI_ASSERT(type_ == Control::character);
      return payload_;
    }

    std::uint8_t param() const volatile noexcept { return payload_; }

  private:
    Control type_{};
    CharT payload_{};
  };
} // namespace cli
#endif
