/**
 * @file cli/validator.hpp
 *
 * @brief This file contains the Validator concept and the default
 * implementation.
 *
 * @defgroup Validation
 *
 * A validator is a callable that takes a value as its sole argument and
 * returns a bool.
 *
 * If the value is valid, true must be returned, else false.
 *
 * Example:
 * ```
 * // a foo must be in the range [0,100]
 * cli::Error validate_foo(int foo){
 *   return foo >= 0 and foo <= 100;
 * }
 * ```
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
  template<Callable V>
  struct value_type {
    using type = std::remove_cvref_t<type_list::type_at_t<
      0,
      typename function_traits<std::remove_cvref_t<V>>::arguments>>;
  };

  template<Callable V>
  using value_type_t = typename value_type<V>::type;

  /**
   * The validator concept
   *
   * A validator is a callable that takes a value as its sole argument and
   * returns a bool.
   *
   * If the value is valid, true must be returned, else false.
   *
   * Example:
   * ```
   * // a foo must be in the range [0,100]
   * cli::Error validate_foo(int foo){
   *   return foo >= 0 and foo <= 100;
   * }
   * ```
   *
   * @ingroup Validation
   * @tparam V the type to check
   */
  template<class V>
  concept Validator =
    requires(std::remove_cvref_t<V> v, const value_type_t<V> &value) {
      { v(value) } -> std::convertible_to<bool>;
    };

  /**
   * A validator is a callable that takes a value of type T as its sole argument
   * and returns a bool.
   *
   * If the value is valid, true must be returned, else false.
   *
   * Example:
   * ```
   * // a foo must be in the range [0,100]
   * cli::Error validate_foo(int foo){
   *   return foo >= 0 and foo <= 100;
   * }
   * ```
   *
   * @ingroup Validation
   * @tparam V the validator
   * @tparam T the value type
   */
  template<class V, class T>
  concept ValidatorOf = requires(std::remove_cvref_t<V> v, const T &value) {
    { v(value) } -> std::convertible_to<bool>;
  };

  /**
   * The default validator, i.e. always validates as true.
   *
   * @tparam T the value type
   */
  template<class T>
  struct DefaultValidate {
    constexpr bool operator()(const T &) const { return true; }
  };

  struct NullValidate {
    constexpr bool operator()(const dummy &) const { return true; }
  };
} // namespace cli::validate
#endif
