#ifndef CLI_HPP
#define CLI_HPP

#include "cli/config.hpp"
#include "cli/display.hpp"
#include "cli/engine.hpp"
#include "cli/function.hpp"
#include "cli/param.hpp"

namespace cli {
  using funcs::arg;
  using funcs::operator""_arg;
  using funcs::func;
  using params::param;
} // namespace cli

#endif
