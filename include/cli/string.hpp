#ifndef CLI_STRING_CONSTANT_HPP
#define CLI_STRING_CONSTANT_HPP

#include "cli/type_list.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>

namespace cli {

  namespace dtl {
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

    template<typename CharT>
    constexpr std::size_t strlen(const CharT *s) noexcept {
      if (not s)
        return 0;

      std::size_t i = 0;
      while (s[i] != 0) {
        ++i;
      }
      return i;
    }

    template<typename CharT>
    constexpr bool op_equal(const CharT *str,
                            std::size_t size,
                            const CharT *other_str,
                            std::size_t other_size) noexcept {
      if (other_size != size)
        return false;
      if (str == nullptr and other_str == nullptr)
        return true;
      for (std::size_t i = 0; i < size; ++i)
        if (str[i] != other_str[i])
          return false;
      return true;
    }

    template<typename CharT>
    constexpr bool op_less_than(const CharT *str,
                                std::size_t size,
                                const CharT *other_str,
                                std::size_t other_size) noexcept {

      const auto s = size > other_size ? other_size : size;
      for (std::size_t i = 0; i < s; ++i)
        if (str[i] == other_str[i])
          continue;
        else if (str[i] < other_str[i])
          return true;
        else
          return false;

      // substrings are equal-> if this is shorter than other, this is "less
      // than" other
      if (size < other_size)
        return true;

      return false;
    }

    template<typename CharT>
    constexpr bool op_greater_than(const CharT *str,
                                   std::size_t size,
                                   const CharT *other_str,
                                   std::size_t other_size) noexcept {
      const auto s = std::min(size, other_size);
      for (std::size_t i = 0; i < s; ++i)
        if (str[i] == other_str[i])
          continue;
        else if (str[i] > other_str[i])
          return true;
        else
          return false;

      // substrings are equal-> if this is longer than other, this is "greater
      // than" other
      if (size > other_size)
        return true;

      return false;
    }

    template<typename CharT>
    constexpr bool starts_with(const CharT *str,
                               std::size_t size,
                               const CharT *other_str,
                               std::size_t other_size) noexcept {
      if (size < other_size or other_size == 0)
        return false;
      for (std::size_t i = 0; i < other_size; ++i)
        if (other_str[i] != str[i])
          return false;
      return true;
    }

    template<typename CharT>
    constexpr std::size_t find_first_of(const CharT *str,
                                        std::size_t size,
                                        const CharT *other_str,
                                        std::size_t other_size,
                                        std::size_t pos) noexcept {
      for (std::size_t i = pos; i < size; ++i)
        for (std::size_t j = 0; j < other_size; ++j)
          if (other_str[j] == str[i])
            return i;
      return npos;
    }

    template<typename CharT>
    constexpr std::size_t find_first_not_of(const CharT *str,
                                            std::size_t size,
                                            const CharT *other_str,
                                            std::size_t other_size,
                                            std::size_t pos) noexcept {
      for (std::size_t i = pos; i < size; ++i) {
        std::size_t cnt{0};
        for (std::size_t j = 0; j < other_size; ++j) {
          if (other_str[j] != str[i]) {
            cnt++;
          } else {
            break;
          }
          if (cnt == other_size)
            return i;
        }
      }
      return npos;
    }

    template<typename CharT>
    constexpr std::size_t find_last_of(const CharT *str,
                                       std::size_t size,
                                       const CharT *other_str,
                                       std::size_t other_size,
                                       std::size_t pos) noexcept {
      if (size == 0)
        return npos;

      const std::size_t init = (pos < size - 1) ? pos : size - 1;
      for (std::size_t i = init; i < size; --i)
        for (std::size_t j = 0; j < other_size; ++j)
          if (other_str[j] == str[i])
            return i;
      return npos;
    }

    template<typename CharT>
    constexpr std::size_t find(const CharT *str,
                               std::size_t size,
                               CharT c,
                               std::size_t pos) noexcept {
      for (std::size_t i = pos; i < size; ++i)
        if (c == str[i])
          return i;
      return npos;
    }

    template<typename CharT>
    constexpr std::size_t find(const CharT *str,
                               std::size_t size,
                               const CharT *other_str,
                               std::size_t other_size,
                               std::size_t pos) noexcept {
      for (std::size_t i = pos; i < size; ++i)
        for (std::size_t j = 0; (i + other_size) <= size and j < other_size;
             ++j)
          if (str[i + j] != other_str[j])
            break;
          else if (j + 1 == other_size)
            return i;
      return npos;
    }

    template<typename CharT>
    constexpr std::size_t find_last_not_of(const CharT *str,
                                           std::size_t size,
                                           const CharT *other_str,
                                           std::size_t other_size,
                                           std::size_t pos) noexcept {
      if (size == 0)
        return npos;

      const std::size_t init = (pos < size - 1) ? pos : size - 1;
      for (std::size_t i = init; i < size; --i)
        if (find<CharT>(other_str, other_size, str[i], 0) == npos)
          return i;
      return npos;
    }

    template<typename CharT>
    constexpr std::size_t find_last_not_of(const CharT *str,
                                           std::size_t size,
                                           CharT c,
                                           std::size_t pos) noexcept {
      if (size == 0)
        return npos;

      const std::size_t init = (pos < size - 1) ? pos : size - 1;
      for (std::size_t i = init; i < size; --i)
        if (c != str[i])
          return i;
      return npos;
    }
  } // namespace dtl
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

    constexpr View(CharType *begin, std::nullptr_t) = delete;

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
      return dtl::op_equal<value_type>(str_, size_, other.str_, other.size_);
    }

    constexpr bool operator<(const View &other) const noexcept {
      return dtl::op_less_than<value_type>(
        str_, size_, other.str_, other.size_);
    }

    constexpr bool operator>(const View &other) const noexcept {
      return dtl::op_greater_than<value_type>(
        str_, size_, other.str_, other.size_);
    }

    constexpr bool starts_with(const value_type *s) const noexcept {
      return dtl::starts_with(str_, size_, s, dtl::strlen(s));
    }

    template<typename Ch>
    constexpr bool starts_with(View<Ch> str) const noexcept {
      return dtl::starts_with(str_, size_, str.str_, str.size_);
    }

    constexpr std::size_t find_first_of(value_type c,
                                        std::size_t pos = 0) const noexcept {
      return dtl::find_first_of<value_type>(str_, size_, &c, 1, pos);
    }

    constexpr std::size_t find_first_of(const value_type *str,
                                        std::size_t pos = 0) const noexcept {
      return dtl::find_first_of<value_type>(
        str_, size_, str, dtl::strlen(str), pos);
    }

    template<typename Ch>
    constexpr std::size_t find_first_of(View<Ch> str,
                                        std::size_t pos = 0) const noexcept {
      return dtl::find_first_of<value_type>(
        str_, size_, str.str_, str.size_, pos);
    }

    constexpr std::size_t
    find_first_not_of(value_type c, std::size_t pos = 0) const noexcept {
      return dtl::find_first_not_of<value_type>(str_, size_, &c, 1, pos);
    }

    constexpr std::size_t find_first_not_of(const value_type *s,
                                            std::size_t pos = 0) noexcept {
      return dtl::find_first_not_of<value_type>(
        str_, size_, s, dtl::strlen(s), pos);
    }

    template<typename Ch>
    constexpr std::size_t
    find_first_not_of(View<Ch> str, std::size_t pos = 0) const noexcept {
      return dtl::find_first_not_of<value_type>(
        str_, size_, str.str_, str.size_, pos);
    }

    constexpr std::size_t find_last_of(value_type c,
                                       std::size_t pos = npos) const noexcept {
      return dtl::find_last_of<value_type>(str_, size_, &c, 1, pos);
    }

    constexpr std::size_t find_last_of(const value_type *str,
                                       std::size_t pos = npos) const noexcept {
      return dtl::find_last_of<value_type>(
        str_, size_, str, dtl::strlen(str), pos);
    }

    template<typename Ch>
    constexpr std::size_t find_last_of(View<Ch> str,
                                       std::size_t pos = npos) const noexcept {
      return dtl::find_last_of<value_type>(
        str_, size_, str.str_, str.size_, pos);
    }

    constexpr std::size_t
    find_last_not_of(value_type c, std::size_t pos = npos) const noexcept {
      return dtl::find_last_not_of<value_type>(str_, size_, c, pos);
    }

    constexpr std::size_t
    find_last_not_of(const value_type *str,
                     std::size_t pos = npos) const noexcept {
      return dtl::find_last_not_of<value_type>(
        str_, size_, str, dtl::strlen(str), pos);
    }

    template<typename Ch>
    constexpr std::size_t
    find_last_not_of(View<Ch> str, std::size_t pos = npos) const noexcept {
      return dtl::find_last_not_of<value_type>(
        str_, size_, str.str_, str.size_, pos);
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

    constexpr std::size_t find(const value_type *str,
                               std::size_t pos = 0) const noexcept {
      return dtl::find<value_type>(str_, size_, str, dtl::strlen(str), pos);
    }

    template<typename Ch>
    constexpr std::size_t find(View<Ch> str,
                               std::size_t pos = 0) const noexcept {
      return dtl::find<value_type>(str_, size_, str.str_, str.size_, pos);
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
                            const string_constant<CharT, C2...> &) noexcept {
    return false;
  }

  template<typename CharT, CharT... Cs>
  constexpr bool operator==(const string_constant<CharT, Cs...> &,
                            const string_constant<CharT, Cs...> &) noexcept {
    return true;
  }

  template<typename CharT, CharT... C1, CharT... C2>
  constexpr bool operator!=(const string_constant<CharT, C1...> &,
                            const string_constant<CharT, C2...> &) noexcept {
    return true;
  }

  template<typename CharT, CharT... Cs>
  constexpr bool operator!=(const string_constant<CharT, Cs...> &,
                            const string_constant<CharT, Cs...> &) noexcept {
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
    constexpr StringLiteral(CharT const (&p)[N]) noexcept {
      for (std::size_t i = 0; i < N; ++i) {
        s[i] = p[i];
      }
    }

    constexpr StringLiteral(const auto... cs) noexcept
      requires(std::same_as<decltype(cs), CharT> && ...)
      : s{cs..., 0} {}

    template<CharT... Cs>
    constexpr StringLiteral(string_constant<CharT, Cs...>) noexcept
      : s{Cs..., 0} {}

    constexpr StringLiteral(View<CharT> str) noexcept {
      CLI_ASSERT(str.size() < N);
      for (std::size_t i = 0; i < str.size(); ++i)
        s[i] = str[i];
    }

    template<class T>
    constexpr operator T() const noexcept {
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
    constexpr void clear() noexcept {
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

    template<typename T>
    inline constexpr bool is_identifier_v = false;

    template<typename CharT, CharT... Cs>
    inline constexpr bool is_identifier_v<string_constant<CharT, Cs...>> =
      ((Cs != ' ' and Cs != '\n' and Cs != '\r' and Cs != '\t' and
        Cs != '\v' and Cs != '\f' and Cs != '(' and Cs != ')' and Cs != '{' and
        Cs != '}' and Cs != ',' and Cs != '=' and Cs != '\'' and Cs != '"') and
       ...);

  } // namespace dtl
} // namespace cli
#endif
