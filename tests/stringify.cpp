#include "stringify.hpp"
#include "cli/ctti.hpp"

#include <string_view>

namespace cli {
  std::ostream &operator<<(std::ostream &os, const cli::Error &e) {
    return os << std::string_view{cli::ctti::enum_name(e).data()};
  }

  std::ostream &operator<<(std::ostream &os, const cli::View<const char> &str) {
    if (str.size() == 0)
      return os;
    return os << std::string_view{str.data(), str.size()};
  }

  std::ostream &operator<<(std::ostream &os, const Control &ctrl) {
    return os << ctti::enum_name(ctrl);
  }

  std::ostream &operator<<(std::ostream &os, const cli::Event<char> &ev) {
    if (ev.type() == cli::Control::character) {
      return os << "{char: " << ev.as_char() << "}";
    } else {
      return os << "{ctrl: " << ctti::enum_name(ev.type())
                << ",param: " << static_cast<unsigned>(ev.param()) << "}";
    }
  }
} // namespace cli
