#ifndef CLI_TRAITS_HPP
#define CLI_TRAITS_HPP
#include "cli/string.hpp"
#include "cli/util.hpp"
#include "cli/vector.hpp"

#include <concepts>
#include <limits>
#include <type_traits>

namespace cli::traits {

  namespace dtl {
    template<typename T, typename = void>
    struct sequence_value_type {
      static_assert(
        always_false<T>,
        "T is not a sequence. T must have an inner typedef called value_type.");
    };

    template<typename T>
    struct sequence_value_type<T, std::void_t<typename T::value_type>> {
      using type = typename T::value_type;
    };
  } // namespace dtl

  template<typename T>
  using sequence_value_type_t =
    typename dtl::sequence_value_type<std::remove_cvref_t<T>>::type;

  enum class Kind {
    Integer,
    FixPoint,
    Float,
    String,
    Struct,
    Sequence
  };

  template<class T>
  struct kind;

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

  template<typename T>
  concept Character =
    is_char<T>::value or std::same_as<T, char> or
    std::same_as<T, unsigned char> or std::same_as<T, signed char> or
    std::same_as<T, char8_t> or std::same_as<T, char16_t> or
    std::same_as<T, char32_t> or std::same_as<T, wchar_t>;

  template<class T>
  concept Integer =
    is_integer<T>::value or
    ((not Character<T>) and std::integral<T> and not std::same_as<T, bool>);

  template<class T>
  concept FixPoint =
    is_fixpoint<T>::value and
    std::constructible_from<T, typename T::raw_value_type> and
    std::integral<typename T::raw_value_type> and requires(T a, T b) {
      { T::num_int_digits } -> std::convertible_to<std::size_t>;
      { T::num_frac_digits } -> std::convertible_to<std::size_t>;
      { a.integer() } -> std::convertible_to<typename T::raw_value_type>;
      { a.fraction() } -> std::convertible_to<typename T::raw_value_type>;
      { a.value() } -> std::convertible_to<typename T::raw_value_type>;
      { a < b };
      { -a };
      { a = b };
    };

  template<class T>
  concept Float = std::floating_point<T> or is_float<T>::value;

  template<class T>
  concept StringView =
    is_string_view<T>::value and std::integral<typename T::value_type> and
    std::constructible_from<T, const typename T::value_type *, std::size_t> and
    requires(T &&t, std::size_t i) {
      { T{} };
      { t.size() } -> std::convertible_to<std::size_t>;
      { t.begin() } -> std::forward_iterator;
      { t.end() } -> std::forward_iterator;
      { t[i] } -> std::convertible_to<const typename T::value_type &>;
    };

  template<class T>
  concept String =
    is_string<T>::value and std::integral<typename T::value_type> and
    std::constructible_from<T, const typename T::value_type *, std::size_t> and
    requires(T &&t, typename T::value_type c, std::size_t i) {
      { T{} };
      { t.size() } -> std::convertible_to<std::size_t>;
      { t.begin() } -> std::forward_iterator;
      { t.end() } -> std::forward_iterator;
      { t[i] } -> std::convertible_to<const typename T::value_type &>;
      { t.push_back(c) };
    };

  template<class T>
  concept Sequence = is_sequence<T>::value and std::copy_constructible<T> and
                     requires(T a, sequence_value_type_t<T> value) {
                       { T{} };
                       { a.begin() };
                       { a.end() };
                       { a.max_size() } -> std::convertible_to<std::size_t>;
                       { a.push_back(value) };
                     };

  template<class T>
  concept FixedSizeSequence =
    is_fixed_size_sequence<T>::value and std::copy_constructible<T> and
    requires(T a, std::size_t i, sequence_value_type_t<T> value) {
      { T{} };
      { a.begin() };
      { a.end() };
      { a.size() } -> std::convertible_to<std::size_t>;
      { a[i] = value };
    };

  template<class T>
  concept Struct =
    is_struct<T>::value and not(Sequence<T> or FixedSizeSequence<T>);

  template<class T>
  concept Enum = is_enum<T>::value;

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

  template<FixPoint T>
  struct fixpoint_traits {
    using type = T;
    using raw_value_type = typename T::raw_value_type;
    static constexpr bool is_signed = std::is_signed_v<raw_value_type>;
    static constexpr std::size_t num_int_digits = T::num_int_digits;
    static constexpr std::size_t num_frac_digits = T::num_frac_digits;
  };

  template<class T>
  struct string_traits : std::false_type {};

  template<class T>
  struct sequence_traits : std::false_type {};

  template<class T>
  concept FlagEnum = Enum<T> and requires(T a, T b) {
    { a | b } -> std::same_as<T>;
  };

  template<Enum T>
  struct enum_traits;

  template<Enum T>
    requires std::is_signed_v<std::underlying_type_t<T>> and (not FlagEnum<T>)
  struct enum_traits<T> {
    static constexpr std::underlying_type_t<T> min = -128;
    static constexpr std::underlying_type_t<T> max = 127;
    static constexpr bool is_flag = false;
  };

  template<Enum T>
    requires std::is_unsigned_v<std::underlying_type_t<T>> and (not FlagEnum<T>)
  struct enum_traits<T> {
    static constexpr std::underlying_type_t<T> min = 0;
    static constexpr std::underlying_type_t<T> max = 255;
    static constexpr bool is_flag = false;
  };

  template<FlagEnum T>
  struct enum_traits<T> {
    static constexpr std::underlying_type_t<T> min = 0u;
    static constexpr std::underlying_type_t<T> max = sizeof(T) * 8u - 1u;
    static constexpr bool is_flag = true;
  };

} // namespace cli::traits
#endif
