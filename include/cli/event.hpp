#ifndef CLI_EVENT_HPP
#define CLI_EVENT_HPP
#include <compare>
#include <cstdint>

namespace cli {

  /**
   * @class Control
   * @brief
   *
   */
  struct Control {
    enum Type {
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

    Type type{};
    uint32_t param{0};

    constexpr Control() {}

    constexpr Control(Type type, uint32_t param = 1)
      : type(type), param(param) {}

    constexpr Control(const Control &) = default;

    constexpr Control(Control &&) = default;

    Control(const volatile Control &o)
      : type(o.type), param(o.param) {}

    Control(const volatile Control &&o)
      : type(o.type), param(o.param) {}

    constexpr Control &operator=(const Control &o) {
      type = o.type;
      param = o.param;
      return *this;
    }

    constexpr Control &operator=(Control &&o) {
      type = o.type;
      param = o.param;
      return *this;
    }

    volatile Control &operator=(const volatile Control &o) volatile {
      type = o.type;
      param = o.param;
      return *this;
    }

    volatile Control &operator=(const volatile Control &&o) volatile {
      type = o.type;
      param = o.param;
      return *this;
    }

    constexpr auto operator<=>(const Control &) const = default;
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
  struct Event {
    bool is_char; //< if true, c is the active member of the union, else ctrl is
                  //< the active member.
    union {
      Control ctrl;
      CharT c;
    };

    constexpr Event()
      : is_char(true), c(0) {}

    constexpr explicit Event(CharT c)
      : is_char(true), c(c) {}

    constexpr explicit Event(Control ctrl)
      : is_char(false), ctrl(ctrl) {}

    constexpr Event(const Event &o)
      : is_char(o.is_char) {
      if (is_char)
        c = o.c;
      else
        ctrl = o.ctrl;
    }

    constexpr Event &operator=(const Event &o) {
      is_char = o.is_char;
      if (o.is_char) {
        c = o.c;
      } else {
        ctrl = o.ctrl;
      }
      return *this;
    }

    volatile Event &operator=(const volatile Event &o) volatile {
      is_char = o.is_char;
      if (o.is_char) {
        c = o.c;
      } else {
        ctrl = o.ctrl;
      }
      return *this;
    }
  };
} // namespace cli
#endif
