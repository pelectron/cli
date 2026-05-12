#ifndef CLI_TEST_STRINGIFY_HPP
#define CLI_TEST_STRINGIFY_HPP

#include "cli/event.hpp"
#include <ostream>

namespace cli {
  std::ostream &operator<<(std::ostream &os, const cli::Error &e);

  std::ostream &operator<<(std::ostream &os, const cli::View<const char> &str);

  template<char... C>
  std::ostream &operator<<(std::ostream &os, string_constant<char, C...>) {
    ((os << C), ...);
    return os;
  }

  std::ostream &operator<<(std::ostream &os, const Control &ctrl);

  std::ostream &operator<<(std::ostream &os, const cli::Event<char> &ev);
} // namespace cli
#endif
