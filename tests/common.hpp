#ifndef CLI_TEST_COMMON_HPP
#define CLI_TEST_COMMON_HPP
#include "cli/concepts.hpp"
#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/traits.hpp"
// #include "fixpoint.hpp"
#include <catch2/catch_tostring.hpp>
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

enum class F : uint32_t {
  A = 1 << 0,
  B = 1 << 1,
  C = 1 << 2,
  D = 1 << 3
};

constexpr F operator|(F f1, F f2) {
  return static_cast<F>(static_cast<uint32_t>(f1) | static_cast<uint32_t>(f2));
}

namespace cli {
  inline std::ostream &operator<<(std::ostream &os, const cli::Error &e) {
    return os << std::string_view{cli::ctti::enum_name(e).data()};
  }

  inline std::ostream &operator<<(std::ostream &os,
                                  const cli::View<const char> &str) {
    if (str.size() == 0)
      return os;
    return os << std::string_view{str.data(), str.size()};
  }
} // namespace cli

namespace Catch {
  template<>
  struct StringMaker<cli::Error> {
    static std::string convert(cli::Error const &value) {
      return cli::ctti::enum_name(value).data();
    }
  };
  template<>
  struct StringMaker<cli::View<const char>> {
    static std::string convert(cli::View<const char> const &value) {
      if (value.size() == 0)
        return {};
      return {value.data(), value.size()};
    }
  };
} // namespace Catch
#endif
