/**
 * @file cli/validator.hpp
 *
 * This file contains the Validator concept and the default
 * implementation.
 */

#ifndef CLI_VALIDATE_HPP
#define CLI_VALIDATE_HPP

#include "cli/util.hpp"

#include <concepts>
#include <type_traits>

namespace cli::validate {

/**
 * Extracts a validators value type
 */
template <Callable V> struct value_type {
  using type = std::remove_cvref_t<type_list::type_at_t<
      0, typename function_traits<std::remove_cvref_t<V>>::arguments>>;
};

template <Callable V> using value_type_t = typename value_type<V>::type;

/**
 * The validator concept
 *
 * A validator is a callable that takes one argument and returns a cli::Error.
 * @tparam V the type to check
 */
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

/**
 * The default validator, i.e. always validates as true.
 *
 * @tparam T the value type
 */
template <class T> struct DefaultValidate {
  constexpr cli::Error operator()(const T &) const { return Error::none; }
};

struct NullValidate {
  constexpr cli::Error operator()(const dummy &) const { return Error::none; }
};
} // namespace cli::validate
#endif
