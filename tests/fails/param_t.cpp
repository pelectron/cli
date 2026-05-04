#include "cli.hpp"
#include "cli/string.hpp"
using cli::operator""_sc;
int main() { constexpr auto p = cli::param<int>("name"_sc, "description"_sc); }
