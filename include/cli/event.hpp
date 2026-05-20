#ifndef CLI_EVENT_HPP
#define CLI_EVENT_HPP

#include "cli/concepts.hpp"
#include "cli/util.hpp"

#include <cstdint>

namespace cli {

  enum class Control : std::uint8_t {
    character,           //< a normal character
    bell,                //< bell
    backspace,           //< backspace: deletes the character before the cursor
    autocomplete,        //< autocomplete
    cursor_up,           //< cursor up / up arrow
    cursor_down,         //< cursor down / down arrow
    cursor_left,         //< cursor left / left arrow
    cursor_right,        //< cusor right / right arrow
    delete_char,         //< delete: deletes the character under the cursor.
    clear_screen,        //< clears the display
    clear_line,          // clears the current line
    clear_line_to_end,   //<  clears the line from the cursor to the end of the
                         // line
    clear_line_to_begin, //< clears the line from the cursor to the beginning of
                         // the line
    enter                //< the enter key
  };

  /**
   * Event represents either a character or control sequence.
   *
   * An @ref Input produces a sequence of events, which is then process by the
   * engine.
   *
   * See also [here](docs.md#event).
   *
   * @tparam CharT the character type
   */
  template<typename CharT>
  class Event {
  public:
    /// Constructs a character event with the character value 0.
    constexpr Event() {}

    /// Constructs a character event
    /// @param c the character
    constexpr explicit Event(CharT c) noexcept
      : type_(Control::character), payload_{c} {}

    /// Constructs an event with the type `type` and param value 0.
    /// @param type the control type
    constexpr explicit Event(Control type) noexcept
      : type_{type}, payload_{0} {}

    /// Constructs an event with the type `type` and param value `param`.
    /// @param type the control type
    /// @param param how many times the control should be executed
    constexpr Event(Control type, std::uint8_t param) noexcept
      : type_{type}, payload_{static_cast<CharT>(param)} {}

    constexpr Event(const Event &o) noexcept
      : type_(o.type_), payload_(o.payload_) {}

    constexpr Event &operator=(const Event &o) noexcept {
      type_ = o.type_;
      payload_ = o.payload_;
      return *this;
    }

    Event &operator=(const volatile Event &o) noexcept {
      type_ = o.type_;
      payload_ = o.payload_;
      return *this;
    }

    void operator=(const Event &o) volatile noexcept {
      type_ = o.type_;
      payload_ = o.payload_;
    }

    /// get the event's control type
    constexpr Control type() const noexcept { return type_; }

    /// Returns the events character. Should only be used when the control type
    /// is `cli::Control::character`
    constexpr CharT as_char() const noexcept {
      CLI_ASSERT(type_ == Control::character);
      return payload_;
    }

    /// get the control sequence parameter. Should only be used when the control
    /// type is not `cli::Control::character`.
    constexpr std::uint8_t param() const noexcept { return payload_; }

    /// get the event's control type
    Control type() const volatile noexcept { return type_; }

    /// Returns the events character. Should only be used when the control type
    /// is `cli::Control::character`
    CharT as_char() const volatile noexcept {
      CLI_ASSERT(type_ == Control::character);
      return payload_;
    }

    /// get the control sequence parameter. Should only be used when the control
    /// type is not `cli::Control::character`.
    std::uint8_t param() const volatile noexcept { return payload_; }

  private:
    Control type_{};
    CharT payload_{};
  };
} // namespace cli
#endif
