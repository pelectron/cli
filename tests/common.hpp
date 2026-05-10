#ifndef CLI_TEST_COMMON_HPP
#define CLI_TEST_COMMON_HPP
#include "cli/command_tree.hpp"
#include "cli/concepts.hpp"
#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/event.hpp"
#include "cli/string.hpp"
#include "cli/traits.hpp"
// #include "fixpoint.hpp"
#include <catch2/catch_tostring.hpp>
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

enum class F : uint32_t {
  A = 1 << 0,
  B = 1 << 1,
  C = 1 << 2,
  D = 1 << 3
};

constexpr F operator|(F f1, F f2) {
  return static_cast<F>(static_cast<uint32_t>(f1) | static_cast<uint32_t>(f2));
}

constexpr std::uint8_t operator""_u8(unsigned long long int i) {
  return static_cast<std::uint8_t>(i);
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

  template<char... C>
  std::ostream &operator<<(std::ostream &os, string_constant<char, C...>) {
    ((os << C), ...);
    return os;
  }

  inline std::ostream &operator<<(std::ostream &os, const Control &ctrl) {
    return os << ctti::enum_name(ctrl);
  }

  inline std::ostream &operator<<(std::ostream &os,
                                  const cli::Event<char> &ev) {
    if (ev.type() == cli::Control::character) {
      return os << std::format("{{char: {}}}", ev.as_char());
    } else {
      return os << "{ctrl: " << ctti::enum_name(ev.type())
                << ",param: " << static_cast<unsigned>(ev.param()) << "}";
    }
  }
} // namespace cli

struct Display {
  std::size_t cursor = 0;
  std::string data{};
  std::vector<std::string> past;

  void write(char c) {
    if (cursor == data.size()) {
      data.push_back(c);
      ++cursor;
    } else {
      data.insert(data.begin() + cursor, c);
      ++cursor;
      data.erase(data.begin() + cursor);
    }
  }

  void write(cli::View<const char> s) {
    for (const char &ch : s) {
      data.insert(data.begin() + cursor, ch);
      ++cursor;
    }
    if (cursor != data.size())
      data.erase(cursor, s.size());
  }

  void backspace(std::size_t n) {
    data.erase(data.begin() + (n >= cursor ? 0 : cursor - n),
               data.begin() + cursor);
    if (n >= cursor)
      cursor = 0;
    else
      cursor -= n;
  }

  void clear_line() {
    data.clear();
    cursor = 0;
  }

  void clear_screen() {
    data.clear();
    cursor = 0;
  }

  void newline() {
    past.push_back(std::move(data));
    cursor = 0;
    data.clear();
  }

  void delete_char() { data.erase(data.begin() + cursor); }

  void cursor_left(std::size_t n) {
    if (n >= cursor)
      n = cursor;
    cursor -= n;
  }

  void cursor_right(std::size_t n) {
    if (n + cursor > data.size())
      n = data.size() - cursor;
    cursor += n;
  }
};

struct MultilineDisplay : Display {
  static constexpr bool is_multiline_display = true;
};

template<cli::concepts::Command... Commands>
struct MockEngine {
  using config_type = cli::default_config;
  using char_type = typename config_type::char_type;
  using display_type = MultilineDisplay;

  constexpr MockEngine(Commands... commands)
    : tree(*this, commands...) {}

  constexpr MockEngine(std::tuple<Commands...> &&commands)
    : tree(*this, std::move(commands)) {}

  constexpr MockEngine(const std::tuple<Commands...> &commands)
    : tree(*this, commands) {}

  constexpr void print() { print_called = true; }

  constexpr const cli::CommandNode<char_type> *root() const {
    return tree.root();
  }

  bool print_called = false;
  cli::CommandTree<MockEngine<Commands...>, Commands...> tree;
  MultilineDisplay display_;
};

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
