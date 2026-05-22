/**
 * @file
 * @brief This file contains the utilities to format values into strings.
 *
 * @defgroup Formatting Formatting
 *
 * Formatting is performed by Formatters.
 *
 * A Formatter takes a cli::View<CharT> as its first argument and a ``T`` as its
 * second argument, and returns a cli::format::FormatResult. The first argument
 * specifies the buffer into which the second argument should be formatted into.
 *
 * An example of formatters:
 * ```
 * auto format_char(cli::View<char> output, char c)
 *   -> cli::format::FormatResult{
 *   if(output.size() == 0)
 *     return cli::Error::buffer_overflow;
 *   output[0] = c;
 *   return 1;
 * }
 *
 * auto format_quoted_char(cli::View<char> output, char c)
 *   -> cli::format::FormatResult {
 *   if(output.size() < 3) return cli::Error::buffer_overflow;
 *   output[0] = '\'';
 *   output[1] = c;
 *   output[2] = '\'';
 *   return 3;
 * }
 *
 * auto format_bool(cli::View<char> output, bool b)
 *   -> cli::format::FormatResult {
 *   if(b){
 *     if(output.size() < 4)
 *       return cli::Error::buffer_overflow;
 *     output[0] = 't';
 *     output[1] = 'r';
 *     output[2] = 'u';
 *     output[3] = 'e';
 *     return 4;
 *   }else{
 *     if(output.size() < 5)
 *       return cli::Error::buffer_overflow;
 *     output[0] = 'f';
 *     output[1] = 'a';
 *     output[2] = 'l';
 *     output[3] = 's';
 *     output[4] = 'e';
 *     return 5;
 *   }
 * }
 * ```
 *
 * See [here](docs.md#formatting) for more details.
 */

#ifndef CLI_FORMAT_HPP
#define CLI_FORMAT_HPP

#include "cli/basic_format.hpp"
#include "cli/ctti.hpp"
#include "cli/tuple.hpp"

namespace cli::format {

  /**
   * @brief The default formatter for enumerations.
   * If your enum is signed and has values outside the range of [-128, 127], or
   * if your enum is unsigned and has values outsde the range of [0, 255], you
   * must adjust cli::traits::enum_traits.
   * @tparam Enum the enumeration type
   * @tparam CharT the buffer's character type
   */
  template<concepts::Enum Enum, typename CharT>
  struct Format<Enum, CharT> {
    constexpr FormatResult operator()(View<CharT> buf,
                                      Enum value) const noexcept {
      if constexpr (traits::enum_traits<Enum>::is_flag) {
        View<const CharT> name{};
        std::size_t written = 0;
        bool first = true;
        for (std::size_t i = 0; i < sizeof(Enum) * 8; ++i) {
          if ((static_cast<std::underlying_type_t<Enum>>(1u << i) &
               static_cast<std::underlying_type_t<Enum>>(value)) == 0)
            continue;

          name = cli::ctti::enum_name<Enum, CharT>(static_cast<Enum>(1u << i));
          if ((first and buf.size() < name.size()) or
              (buf.size() < (name.size() + 1))) {
            return Error::buffer_overflow;
          }
          if (not first) {
            buf[0] = '|';
            ++written;
            buf = buf.substr(1);
          } else {
            first = true;
          }
          for (std::size_t j = 0; j < name.size(); ++j, ++written) {
            buf[j] = name[j];
          }
          buf = buf.substr(name.size());
          first = false;
        }

        if (written == 0) {
          auto name_ = string_constant<CharT, '<'>{} +
                       ctti::name<Enum, CharT>() +
                       string_constant<CharT,
                                       ':',
                                       ':',
                                       'u',
                                       'n',
                                       'k',
                                       'n',
                                       'o',
                                       'w',
                                       'n',
                                       '>'>{};
          name = name_;
          if (name.size() > buf.size())
            return Error::buffer_overflow;

          for (std::size_t i = 0; i < name.size(); ++i) {
            buf[i] = name[i];
          }
          return name.size();
        }

        return written;
      } else {
        const View<const CharT> name = cli::ctti::enum_name<Enum, CharT>(value);
        if (buf.size() < name.size()) {
          return Error::buffer_overflow;
        }
        for (std::size_t i = 0; i < name.size(); ++i) {
          buf[i] = name[i];
        }
        return name.size();
      }
    }
  };
  template<typename CharT,
           CharT Assignment = '=',
           CharT MemberSeparator = ',',
           CharT Prefix = '{',
           CharT Postfix = '}',
           bool UseNames = true,
           class... Fields>
  class FieldGroup {
  public:
    constexpr FormatResult
    operator()(View<CharT> buf,
               const cli::Tuple<Fields...> &fields) const noexcept {
      if (buf.size() == 0)
        return Error::buffer_overflow;

      std::size_t written = 0;
      if constexpr (Prefix != ' ') {
        if (buf.size() < 2)
          return Error::buffer_overflow;
        buf[written++] = Prefix;
        buf[written++] = ' ';
        buf = buf.substr(2);
      }
      Error error = Error::none;
      bool first = true;
      for_each(
        [&error, &written, &first, &buf](const auto &field) {
          if (error != Error::none)
            return;

          if (first) {
            first = false;
          } else {
            if (buf.size() <= 2) {
              error = Error::buffer_overflow;
              return;
            }
            buf[0] = MemberSeparator;
            buf[1] = ' ';
            buf = buf.substr(2);
            written += 2;
          }

          using Field = std::remove_cvref_t<decltype(field)>;
          using Formatter =
            Format<std::remove_cvref_t<typename Field::type>, CharT>;

          if constexpr (UseNames) {
            constexpr StringLiteral name{typename Field::name{}};
            if (buf.size() <= (name.size() + 3))
              error = Error::buffer_overflow;
            std::size_t i = 0;
            for (; i < name.size(); ++i) {
              buf[i] = name[i];
            }
            buf[i++] = ' ';
            buf[i++] = Assignment;
            buf[i++] = ' ';
            buf = buf.substr(i);
            written += i;
          }

          auto res = Formatter{}(buf, field.value);
          if (not res) {
            error = res.error;
            return;
          }
          buf = buf.substr(res.size_written);
          written += res.size_written;
        },
        fields);

      if (error != Error::none)
        return error;

      if constexpr (Postfix != ' ') {
        if (buf.size() < 2)
          return Error::buffer_overflow;

        buf[0] = ' ';
        buf[1] = Postfix;
        written += 2;
      }

      return written;
    }
  };

  template<concepts::Struct T,
           typename CharT,
           CharT Assignment = '=',
           CharT MemberSeparator = ',',
           CharT Prefix = '{',
           CharT Postfix = '}',
           bool UseNames = true>
  struct field_formatter_for {
    template<class... Fields>
    using type_ = FieldGroup<CharT,
                             Assignment,
                             MemberSeparator,
                             Prefix,
                             Postfix,
                             UseNames,
                             Fields...>;
    using fields = decltype(ctti::to_tuple(std::declval<const T>()));
    using type = type_list::apply_t<type_, fields>;
  };

  /**
   * @brief A foramtter fro struct aggregates.
   *
   * @tparam T the struct type
   * @tparam CharT
   */
  template<concepts::Struct T,
           typename CharT,
           class Name = string_constant<CharT>,
           CharT Assignment = '=',
           CharT MemberSeparator = ',',
           CharT Prefix = '{',
           CharT Postfix = '}',
           bool UseNames = true>
  class Struct : field_formatter_for<T,
                                     CharT,
                                     Assignment,
                                     MemberSeparator,
                                     Prefix,
                                     Postfix,
                                     UseNames>::type {
    using Base = typename field_formatter_for<T,
                                              CharT,
                                              Assignment,
                                              MemberSeparator,
                                              Prefix,
                                              Postfix,
                                              UseNames>::type;

  public:
    constexpr FormatResult operator()(View<CharT> buf,
                                      const T &t) const noexcept {
      if (buf.size() == 0)
        return Error::buffer_overflow;

      std::size_t written = 0;
      if constexpr (Name::string_size > 0) {
        constexpr StringLiteral name{Name{}};
        if (buf.size() <= (name.size() + 3))
          return Error::buffer_overflow;
        std::size_t i = 0;
        for (; i < name.size(); ++i) {
          buf[i] = name[i];
        }
        buf[i++] = ' ';
        buf[i++] = Assignment;
        buf[i++] = ' ';
        buf = buf.substr(i);
        written += i;
      }

      if (auto res = static_cast<const Base *>(this)->operator()(
            buf, ctti::to_tuple(t))) {
        return res.size_written + written;
      } else
        return res.error;
    }
  };

  /**
   * @brief The default formatter for struct aggregates.
   *
   * @tparam T the struct type
   * @tparam CharT
   */
  template<concepts::Struct T, typename CharT>
  struct Format<T, CharT> : public Struct<T, CharT> {};

} // namespace cli::format

#endif
