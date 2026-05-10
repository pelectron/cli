#ifndef CLI_TRAITS_HPP
#define CLI_TRAITS_HPP
#include "cli/enums.hpp"
#include "cli/string.hpp"
#include "cli/vector.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace cli::traits {

  namespace dtl {
    template<class T>
    concept FlagEnum = std::is_enum_v<T> and requires(T a, T b) {
      { a | b } -> std::same_as<T>;
    };
  } // namespace dtl

  /**
   * @defgroup type-categories Type Categories
   * The basic categories of types the library can handle. These must be
   * overridden in order to opt in to the corresponding concept.
   *
   * Example:
   * ```
   * // custom_string.hpp
   * #include "cli/traits.hpp"
   * namespace abc{
   *   class CustomString{...};
   * }
   *
   * namespace cli::traits{
   * template<>
   *   struct is_string<abc::CustomString> : std::true_type{};
   * }
   * ```
   * @{
   */
  /// type category predicate for chars
  template<class T>
  struct is_char : std::false_type {};
  /// type category predicate for integers
  template<class T>
  struct is_integer : std::false_type {};
  /// type category predicate for floats
  template<class T>
  struct is_float : std::false_type {};
  /// type category predicate for fixpoint
  template<class T>
  struct is_fixpoint : std::false_type {};
  /// type category predicate for strings
  template<class T>
  struct is_string : std::false_type {};
  /// type category predicate for strings
  template<class T>
  struct is_string_view : std::false_type {};
  /// type category predicate for sequences
  template<class T>
  struct is_sequence : std::false_type {};
  /// type category predicate for fixed size sequences, for example arrays
  template<class T>
  struct is_fixed_size_sequence : std::false_type {};
  /// type category predicate for structs/aggregates
  template<class T>
  struct is_struct : std::is_aggregate<T> {};
  /// type category predicate for enums
  template<class T>
  struct is_enum : std::is_enum<T> {};
  /**
   * @}
   */

  template<typename CharType>
  struct is_string_view<View<CharType>> : std::true_type {};

  template<class T, std::size_t Cap>
  struct is_sequence<FixedCapacityVector<T, Cap>> : std::true_type {};

  template<class T, std::size_t Size>
  struct is_fixed_size_sequence<std::array<T, Size>> : std::true_type {};

  template<std::integral T>
  struct integer_traits {
    using type = T;
    using unsigned_type = std::make_unsigned_t<type>;
    static constexpr bool is_signed = std::is_signed_v<type>;
    static constexpr auto size = sizeof(type);
    static constexpr auto align = alignof(type);
    static constexpr T min = std::numeric_limits<T>::min();
    static constexpr T max = std::numeric_limits<T>::max();
  };

  template<std::floating_point T>
  struct float_traits {
    using type = T;
    static constexpr bool is_signed = std::is_signed_v<type>;
    static constexpr auto size = sizeof(type);
    static constexpr auto align = alignof(type);
  };

  template<typename T>
  struct fixpoint_traits {
    using type = T;
    using raw_value_type = typename T::raw_value_type;
    static constexpr bool is_signed = std::is_signed_v<raw_value_type>;
    static constexpr std::size_t num_int_digits = T::num_int_digits;
    static constexpr std::size_t num_frac_digits = T::num_frac_digits;
  };

  template<class E>
  struct enum_traits;

  template<class E>
    requires std::is_enum_v<E> and
             std::is_signed_v<std::underlying_type_t<E>> and
             (not cli::traits::dtl::FlagEnum<E>)
  struct enum_traits<E> {
    static constexpr std::underlying_type_t<E> min = -128;
    static constexpr std::underlying_type_t<E> max = 127;
    static constexpr bool is_flag = false;
  };

  template<class E>
    requires std::is_enum_v<E> and
             std::is_unsigned_v<std::underlying_type_t<E>> and
             (not cli::traits::dtl::FlagEnum<E>)
  struct enum_traits<E> {
    static constexpr std::underlying_type_t<E> min = 0;
    static constexpr std::underlying_type_t<E> max = 255;
    static constexpr bool is_flag = false;
  };

  template<cli::traits::dtl::FlagEnum E>
  struct enum_traits<E> {
    static constexpr std::underlying_type_t<E> min = 0u;
    static constexpr std::underlying_type_t<E> max = sizeof(E) * 8u - 1u;
    static constexpr bool is_flag = true;
  };

  template<>
  struct enum_traits<Error> {
    static constexpr uint32_t min = 0;
    static constexpr uint32_t max = static_cast<uint32_t>(Error::unknown);
    static constexpr bool is_flag = false;
  };
} // namespace cli::traits
#endif
