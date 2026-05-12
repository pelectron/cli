#ifndef CLI_TEST_COMMON_HPP
#define CLI_TEST_COMMON_HPP
#include "cli/concepts.hpp"
#include "cli/traits.hpp"

#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

namespace cli::traits {

  template<class T, class A>
  struct is_sequence<std::vector<T, A>> : std::true_type {};
  static_assert(cli::concepts::Sequence<std::vector<int>>);

  template<>
  struct is_string<std::string> : std::true_type {};
  static_assert(cli::concepts::String<std::string>);

  /*
  template <std::size_t I, std::size_t F>
  struct is_fixpoint<psm::unsigned_fixed<I, F>> : std::true_type {};
  template <std::size_t I, std::size_t F>
  struct is_fixpoint<psm::signed_fixed<I, F>> : std::true_type {};

  template <std::size_t I, std::size_t F>
  struct fixpoint_traits<psm::unsigned_fixed<I, F>> {
    using type = psm::unsigned_fixed<I, F>;
    using raw_value_type = typename psm::unsigned_fixed<I, F>::raw_value_type;
    static constexpr bool is_signed = false;
    static constexpr std::size_t num_int_digits = I;
    static constexpr std::size_t num_frac_digits = F;
  };
  template <std::size_t I, std::size_t F>
  struct fixpoint_traits<psm::signed_fixed<I, F>> {
    using type = psm::signed_fixed<I, F>;
    using raw_value_type = typename psm::signed_fixed<I, F>::raw_value_type;
    static constexpr bool is_signed = true;
    static constexpr std::size_t num_int_digits = I;
    static constexpr std::size_t num_frac_digits = F;
  };
  */
} // namespace cli::traits

enum class Flag : uint32_t {
  A = 1 << 0,
  B = 1 << 1,
  C = 1 << 2,
  D = 1 << 3
};

constexpr Flag operator|(Flag f1, Flag f2) {
  return static_cast<Flag>(static_cast<uint32_t>(f1) |
                           static_cast<uint32_t>(f2));
}

#endif
