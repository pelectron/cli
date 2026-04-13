#ifndef CLI_VALIDATE_HPP
#define CLI_VALIDATE_HPP
#include "cli/util.hpp"
#include <concepts>
#include <string_view>
#include <type_traits>

namespace cli::validate {

template <Callable V> struct value_type {
  using type = std::remove_cvref_t<type_list::type_at_t<
      0, typename function_traits<std::remove_cvref_t<V>>::arguments>>;
};

template <Callable V> using value_type_t = typename value_type<V>::type;

template <class V>
concept Validator =
    requires(std::remove_cvref_t<V> v, const value_type_t<V> &value) {
      { v(value) } -> std::same_as<cli::Error>;
    };

template <class V, class T>
concept ValidatorOf = std::same_as<T, value_type_t<V>> and
                      requires(std::remove_cvref_t<V> v, const T &value) {
                        { v(value) } -> std::same_as<cli::Error>;
                      };

template <class T> struct DefaultValidate {
  constexpr cli::Error operator()(const T &) const { return Error::none; }
};

struct NullValidate {
  constexpr cli::Error operator()(const dummy &) const { return Error::none; }
};
} // namespace cli::validate
#endif
