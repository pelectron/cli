#ifndef CLI_STRING_CONSTANT_HPP
#define CLI_STRING_CONSTANT_HPP

#include "cli/type_list.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>

namespace cli {

  /**
   * A view of characters, kinda like std::span.
   *
   * @tparam CharType the character type.
   */
  template<typename CharType>
  class View {
  public:
    using value_type = std::remove_const_t<CharType>;
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

    constexpr View() noexcept {}

    constexpr View(const View &o) noexcept
      : str_(o.str_), size_(o.size_) {}

    constexpr View(View &&o) noexcept
      : str_(o.str_), size_(o.size_) {}

    template<typename Ch>
      requires std::convertible_to<Ch *, CharType *>
    constexpr View(View<Ch> other)
      : str_(other.str_), size_(other.size_) {}

    constexpr View(CharType *str, std::size_t size) noexcept
      : str_(str), size_(size) {}

    constexpr View(CharType *str) noexcept
      : str_(str), size_(0) {
      while (str_[size_] != 0)
        ++size_;
    }

    constexpr View(CharType *begin, CharType *end) noexcept
      : str_(begin), size_(end - begin) {}

    constexpr View &operator=(const View &o) noexcept {
      str_ = o.str_;
      size_ = o.size_;
      return *this;
    }

    constexpr View &operator=(View &&o) noexcept {
      str_ = o.str_;
      size_ = o.size_;
      return *this;
    }

    constexpr CharType *data() noexcept { return str_; }
    constexpr const CharType *data() const noexcept { return str_; }
    constexpr std::size_t size() const noexcept { return size_; }

    constexpr CharType *begin() noexcept { return str_; }
    constexpr const CharType *begin() const noexcept { return str_; }
    constexpr CharType *end() noexcept { return str_ + size_; }
    constexpr const CharType *end() const noexcept { return str_ + size_; }

    constexpr CharType &operator[](std::size_t i) noexcept { return str_[i]; }
    constexpr const CharType &operator[](std::size_t i) const noexcept {
      return str_[i];
    }

    constexpr bool operator==(const View &other) const noexcept {
      if (other.size_ != size_)
        return false;
      if (str_ == nullptr and other.str_ == nullptr)
        return true;
      for (std::size_t i = 0; i < size_; ++i)
        if (str_[i] != other.str_[i])
          return false;
      return true;
    }

    // template <typename C>
    // constexpr bool operator==(const View<C> &other) const noexcept {
    //   if (other.size_ != size_)
    //     return false;
    //   if (str_ == nullptr and other.str_ == nullptr)
    //     return true;
    //   for (std::size_t i = 0; i < size_; ++i)
    //     if (str_[i] != other.str_[i])
    //       return false;
    //   return true;
    // }
    //
    constexpr bool operator<(const View &other) const noexcept {
      const auto s = std::min(size_, other.size_);
      for (std::size_t i = 0; i < s; ++i)
        if (str_[i] == other.str_[i])
          continue;
        else if (str_[i] < other.str_[i])
          return true;
        else
          return false;

      // substrings are equal-> if this is shorter than other, this is "less
      // than" other
      if (size_ < other.size_)
        return true;

      return false;
    }

    constexpr bool operator>(const View &other) const noexcept {
      const auto s = std::min(size_, other.size_);
      for (std::size_t i = 0; i < s; ++i)
        if (str_[i] == other.str_[i])
          continue;
        else if (str_[i] > other.str_[i])
          return true;
        else
          return false;

      // substrings are equal-> if this is longer than other, this is "greater
      // than" other
      if (size_ > other.size_)
        return true;

      return false;
    }

    constexpr bool starts_with(const value_type *s) const noexcept {
      return starts_with(View{s});
    }

    template<typename Ch>
    constexpr bool starts_with(View<Ch> s) const noexcept {
      if (size_ < s.size_ or s.size_ == 0)
        return false;
      for (std::size_t i = 0; i < s.size_; ++i)
        if (s.str_[i] != str_[i])
          return false;
      return true;
    }

    constexpr std::size_t find_first_of(value_type c,
                                        std::size_t pos = 0) const noexcept {
      for (std::size_t i = pos; i < size_; ++i)
        if (c == str_[i])
          return i;
      return npos;
    }

    constexpr std::size_t find_first_of(const value_type *s,
                                        std::size_t pos = 0) const noexcept {
      return find_first_of(View{s}, pos);
    }

    template<typename Ch>
    constexpr std::size_t find_first_of(View<Ch> s,
                                        std::size_t pos = 0) const noexcept {
      for (std::size_t i = pos; i < size_; ++i)
        for (std::size_t j = 0; j < s.size_; ++j)
          if (s.str_[j] == str_[i])
            return i;
      return npos;
    }

    constexpr std::size_t
    find_first_not_of(value_type c, std::size_t pos = 0) const noexcept {
      for (std::size_t i = pos; i < size_; ++i)
        if (c != str_[i])
          return i;
      return npos;
    }

    constexpr std::size_t find_first_not_of(const value_type *c,
                                            std::size_t pos = 0) noexcept {
      return find_first_not_of(View{c}, pos);
    }

    template<typename Ch>
    constexpr std::size_t
    find_first_not_of(View<Ch> s, std::size_t pos = 0) const noexcept {
      for (std::size_t i = pos; i < size_; ++i) {
        std::size_t cnt{0};
        for (std::size_t j = 0; j < s.size_; ++j) {
          if (s.str_[j] != str_[i]) {
            cnt++;
          } else {
            break;
          }
          if (cnt == s.size_)
            return i;
        }
      }
      return npos;
    }

    constexpr std::size_t find_last_of(value_type c,
                                       std::size_t pos = npos) const noexcept {
      if (size_ == 0)
        return npos;

      for (std::size_t i = std::min(size_ - 1, pos); i < size_; --i)
        if (c == str_[i])
          return i;
      return npos;
    }

    constexpr std::size_t find_last_of(const value_type *c,
                                       std::size_t pos = npos) const noexcept {
      return find_last_of(View{c}, pos);
    }

    template<typename Ch>
    constexpr std::size_t find_last_of(View<Ch> s,
                                       std::size_t pos = npos) const noexcept {
      if (size_ == 0)
        return npos;

      for (std::size_t i = std::min(size_ - 1, pos); i < size_; --i)
        for (std::size_t j = 0; j < s.size_; ++j)
          if (s.str_[j] == str_[i])
            return i;
      return npos;
    }

    constexpr std::size_t
    find_last_not_of(value_type c, std::size_t pos = npos) const noexcept {
      if (size_ == 0)
        return npos;

      for (std::size_t i = std::min(size_ - 1, pos); i < size_; --i)
        if (c != str_[i])
          return i;
      return npos;
    }

    constexpr std::size_t
    find_last_not_of(const value_type *c,
                     std::size_t pos = npos) const noexcept {
      return find_last_not_of(View{c}, pos);
    }

    template<typename Ch>
    constexpr std::size_t
    find_last_not_of(View<Ch> s, std::size_t pos = npos) const noexcept {
      if (size_ == 0)
        return npos;

      for (std::size_t i = std::min(size_ - 1, pos); i < size_; --i)
        if (s.find(str_[i]) == npos)
          return i;
      return npos;
    }

    constexpr View substr(std::size_t offset,
                          std::size_t size = npos) const noexcept {
      if (offset >= size_)
        return {};

      return {str_ + offset, std::min(size, size_ - offset)};
    }

    constexpr std::size_t find(value_type c,
                               std::size_t pos = 0) const noexcept {
      for (std::size_t i = pos; i < size_; ++i)
        if (c == str_[i])
          return i;
      return npos;
    }

    constexpr std::size_t find(const value_type *s,
                               std::size_t pos = 0) const noexcept {
      return find(View{s}, pos);
    }

    template<typename Ch>
    constexpr std::size_t find(View<Ch> s, std::size_t pos = 0) const noexcept {
      for (std::size_t i = pos; i < size_; ++i)
        for (std::size_t j = 0; (i + s.size_) <= size_ and j < s.size_; ++j)
          if (str_[i + j] != s[j])
            break;
          else if (j + 1 == s.size_)
            return i;
      return npos;
    }

    constexpr CharType &back() noexcept { return str_[size_ - 1]; }

    constexpr const CharType &back() const noexcept { return str_[size_ - 1]; }

  private:
    template<typename>
    friend class View;
    CharType *str_{nullptr};
    std::size_t size_{0};
  };

  using CharView = View<const char>;

  template<typename CharT, std::size_t N>
  struct StringLiteral;

  /**
   * A string_constant is a compile time string.
   *
   * To create a string constant, its easiest to use the literal operator _sc.
   *
   * Example:
   * ```
   *  using cli::operator""_sc;
   *  constexpr auto s1 = "hi"_sc; // type = string_constant<char, 'h', 'i'>
   *  constexpr auto s2 = u"hi"_sc; // type = string_constant<char16_t, 'h',
   * 'i'> constexpr auto s2 = U"hi"_sc; // type = string_constant<char32_t, 'h',
   * 'i'>
   * ```
   * @tparam CharT the character type
   * @tparam Cs the characters
   */
  template<typename CharT, CharT... Cs>
  struct string_constant {
    using char_type = CharT;

    enum {
      string_size = sizeof...(Cs)
    };

    static constexpr CharT value[]{Cs..., 0};

    constexpr operator View<const CharT>() const noexcept {
      return {value, sizeof...(Cs)};
    }

    constexpr
    operator StringLiteral<CharT, sizeof...(Cs) + 1>() const noexcept {
      return {Cs...};
    }

    constexpr const CharT *data() const noexcept { return value; }
    constexpr std::size_t size() const noexcept { return sizeof...(Cs); }
  };

  template<typename CharT, CharT... C1, CharT... C2>
  constexpr bool operator==(const string_constant<CharT, C1...> &,
                            const string_constant<CharT, C2...> &) {
    return false;
  }

  template<typename CharT, CharT... Cs>
  constexpr bool operator==(const string_constant<CharT, Cs...> &,
                            const string_constant<CharT, Cs...> &) {
    return true;
  }

  template<typename CharT, CharT... C1, CharT... C2>
  constexpr bool operator!=(const string_constant<CharT, C1...> &,
                            const string_constant<CharT, C2...> &) {
    return true;
  }

  template<typename CharT, CharT... Cs>
  constexpr bool operator!=(const string_constant<CharT, Cs...> &,
                            const string_constant<CharT, Cs...> &) {
    return false;
  }

  /**
   * A class for storing an array of characters. Used for passing strings
   * as template parameters.
   *
   * @tparam CharT the character type
   * @param N string size
   */
  template<typename CharT, std::size_t N>
  struct StringLiteral {
    using char_type = CharT;
    CharT s[N]{0};
    constexpr StringLiteral(CharT const (&p)[N]) {
      for (std::size_t i = 0; i < N; ++i) {
        s[i] = p[i];
      }
    }

    constexpr StringLiteral(const auto... cs)
      requires(std::same_as<decltype(cs), CharT> && ...)
      : s{cs..., 0} {}

    template<CharT... Cs>
    constexpr StringLiteral(string_constant<CharT, Cs...>)
      : s{Cs..., 0} {}

    constexpr StringLiteral(View<CharT> str) {
      CLI_ASSERT(str.size() < N);
      for (std::size_t i = 0; i < str.size(); ++i)
        s[i] = str[i];
    }

    template<class T>
    constexpr operator T() const {
      if constexpr (N == 1)
        return T();
      else
        return T{s, N - 1};
    }

    [[nodiscard]] constexpr auto
    operator<=>(const StringLiteral &) const = default;
    [[nodiscard]] constexpr operator View<CharT>() const { return {s, N - 1}; }
    [[nodiscard]] constexpr auto size() const -> std::size_t { return N - 1; }
    [[nodiscard]] constexpr const CharT *data() const noexcept { return s; }
    [[nodiscard]] constexpr CharT &operator[](std::size_t i) noexcept {
      return s[i];
    }
    [[nodiscard]] constexpr const CharT &
    operator[](std::size_t i) const noexcept {
      return s[i];
    }
    constexpr void clear() {
      for (auto &ch : s)
        ch = 0;
    }
  };

  template<typename CharT, std::size_t N>
  StringLiteral(CharT const (&p)[N]) -> StringLiteral<CharT, N>;

  template<typename... CharT>
  StringLiteral(const CharT... cs)
    -> StringLiteral<type_list::type_at<0, type_list::TypeList<CharT...>>,
                     sizeof...(cs) + 1>;

  template<typename CharT, CharT... Cs>
  StringLiteral(string_constant<CharT, Cs...>)
    -> StringLiteral<CharT, sizeof...(Cs) + 1>;

  template<typename CharT, CharT... Cs>
  constexpr auto to_lower(const string_constant<CharT, Cs...> &) {
    return string_constant<CharT, [](CharT c) {
      return c >= 'A' and c <= 'Z' ? c + ('a' - 'A') : c;
    }(Cs)...>{};
  }

  template<typename CharT, CharT... C1, CharT... C2>
  constexpr string_constant<CharT, C1..., C2...>
  operator+(const string_constant<CharT, C1...> &,
            const string_constant<CharT, C2...> &) {
    return {};
  }

  template<typename CharT, CharT... C1, CharT... C2>
  constexpr string_constant<CharT, C1..., C2...>
  operator+(const string_constant<CharT, C1..., 0> &,
            const string_constant<CharT, C2..., 0> &) {
    return {};
  }

  template<typename CharT, CharT... C1, CharT... C2>
  constexpr string_constant<CharT, C1..., C2...>
  operator+(const string_constant<CharT, C1..., 0> &,
            const string_constant<CharT, C2...> &) {
    return {};
  }

  template<typename CharT, CharT... C1, CharT... C2>
  constexpr string_constant<CharT, C1..., C2...>
  operator+(const string_constant<CharT, C1...> &,
            const string_constant<CharT, C2..., 0> &) {
    return {};
  }

  /**
   * creates a string_constant. Combine it with the u and U prefix for different
   * kinds of character types.
   *
   * Example:
   * ```
   *  using cli::operator""_sc;
   *  constexpr auto s1 = "hi"_sc; // type = string_constant<char, 'h', 'i'>
   *  constexpr auto s2 = u"hi"_sc; // type = string_constant<char16_t, 'h',
   * 'i'> constexpr auto s2 = U"hi"_sc; // type = string_constant<char32_t, 'h',
   * 'i'>
   * ```
   * @return string_constant containing S
   */
  template<StringLiteral S>
  constexpr auto operator""_sc() {
    return []<std::size_t... Is>(std::index_sequence<Is...>) {
      return string_constant<typename decltype(S)::char_type, S.s[Is]...>{};
    }(std::make_index_sequence<S.size()>());
  }

  namespace dtl {
    template<typename T>
    inline constexpr bool is_view_v = false;

    template<typename CharT>
    inline constexpr bool is_view_v<View<CharT>> = true;

    template<typename T>
    inline constexpr bool is_const_view_v = false;

    template<typename CharT>
    inline constexpr bool is_const_view_v<View<CharT>> = false;

    template<typename CharT>
    inline constexpr bool is_const_view_v<View<const CharT>> = true;

    template<typename T>
    inline constexpr bool is_non_const_view_v = false;

    template<typename CharT>
    inline constexpr bool is_non_const_view_v<View<const CharT>> = false;

    template<typename CharT>
    inline constexpr bool is_non_const_view_v<View<CharT>> = true;

    template<typename T>
    inline constexpr bool is_string_constant_v = false;

    template<typename CharT, CharT... Cs>
    inline constexpr bool
      is_string_constant_v<cli::string_constant<CharT, Cs...>> = true;
  } // namespace dtl
} // namespace cli
#endif
