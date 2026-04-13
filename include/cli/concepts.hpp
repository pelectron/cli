#ifndef CLI_CONCEPTS_HPP
#define CLI_CONCEPTS_HPP

#include "cli/enums.hpp"
#include "cli/string.hpp"
#include <concepts>
#include <type_traits>

namespace cli {

template <class T>
concept SC = is_string_constant_v<std::remove_cvref_t<T>>;

template <class C>
concept Command =
    requires(std::remove_cvref_t<C> &c, ExecType type,
             View<const typename std::remove_cvref_t<C>::char_type> args,
             View<typename std::remove_cvref_t<C>::char_type> &out) {
      { typename std::remove_cvref_t<C>::sub_command_list{} };
      { std::remove_cvref_t<C>::name } -> SC;
      { std::remove_cvref_t<C>::description } -> SC;
      { c.execute(type, args, out) } -> std::same_as<Error>;
    };

template <typename T>
concept Printer = requires(T t) {
  { t.print() };
};

} // namespace cli
#endif
