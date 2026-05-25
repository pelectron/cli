/**
 * @file cli/function.hpp
 *
 * This file defines the utilities to add functions and their arguments
 * to a cli in the namespace cli::funcs, namely:
 *
 * - the literal operator _arg
 * - various overloads of arg, see also @ref Arguments
 * - various overloads of func, see also @ref Functions
 * - various overloads of mem_fun, see also @ref Functions
 *
 * The operators and functions are also available in the namespace cli if the
 * header cli/cli.hpp is included.
 */

#ifndef CLI_FUNCTION_HPP
#define CLI_FUNCTION_HPP

#include "cli/command.hpp"
#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/exec_result.hpp"
#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "cli/string.hpp"
#include "cli/tuple.hpp"
#include "cli/type_list.hpp"
#include "cli/util.hpp"
#include "cli/validator.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace cli {
  template<class Engine, concepts::Command... Commands>
  class CommandTree;
} // namespace cli

namespace cli::funcs {

  template<class A>
  concept FuncArg = requires(A &&arg) {
    { typename std::remove_cvref_t<A>::char_type{} };
    { arg.name } -> SC;
    { arg.description } -> SC;
    { arg.parse } -> parse::Parser;
    { arg.validate } -> validate::Validator;
    // {
    //   std::remove_cvref_t<A>::default_value
    // } /* -> std::same_as<typename std::remove_cvref_t<A>::type> */;
  };

  inline constexpr struct Deduced {
    constexpr Deduced() = default;
  } deduced{};

  template<Id Name,
           SC Description,
           class T,
           auto DefaultValue,
           parse::Parser Parse,
           validate::Validator Validate>
  struct FunctionArg {
    using char_type = typename Name::char_type;
    using type = std::remove_cvref_t<decltype(DefaultValue)>;
    using validator = Validate;
    using parser = Parse;

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArg(Name,
                          Description,
                          identity<T>,
                          constant<DefaultValue>,
                          P &&parse_,
                          V &&validate_) noexcept
      : parse(std::forward<P>(parse_)), validate(std::forward<V>(validate_)) {}

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArg(Name,
                          Description,
                          constant<DefaultValue>,
                          P &&parse_,
                          V &&validate_) noexcept
      : parse(std::forward<P>(parse_)), validate(std::forward<V>(validate_)) {}

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArg(Name, Description, P &&parse_, V &&validate_) noexcept
      : parse(std::forward<P>(parse_)), validate(std::forward<V>(validate_)) {}

    CLI_NO_UNIQUE_ADDRESS Name name{};
    CLI_NO_UNIQUE_ADDRESS Description description{};
    CLI_NO_UNIQUE_ADDRESS Parse parse{};
    CLI_NO_UNIQUE_ADDRESS Validate validate{};
  };

  template<Id Name,
           SC Description,
           class T,
           auto DefaultValue,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArg(Name,
              Description,
              identity<T>,
              constant<DefaultValue>,
              Parse &&,
              Validate &&) -> FunctionArg<Name,
                                          Description,
                                          T,
                                          DefaultValue,
                                          std::decay_t<Parse>,
                                          std::decay_t<Validate>>;

  template<Id Name,
           SC Description,
           auto DefaultValue,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArg(Name, Description, constant<DefaultValue>, Parse &&, Validate &&)
    -> FunctionArg<Name,
                   Description,
                   parse::value_type_t<typename Name::char_type, Parse>,
                   DefaultValue,
                   std::decay_t<Parse>,
                   std::decay_t<Validate>>;

  template<Id Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArg(Name, Description, Parse &&, Validate &&)
    -> FunctionArg<Name,
                   Description,
                   parse::value_type_t<typename Name::char_type, Parse>,
                   parse::value_type_t<typename Name::char_type, Parse>{},
                   std::decay_t<Parse>,
                   std::decay_t<Validate>>;

  template<Id Name,
           SC Description,
           typename T,
           parse::Parser Parse,
           validate::Validator Validate>
  struct FunctionArgWithoutDefault {
    using char_type = typename Name::char_type;
    using type = T;
    using parser = Parse;
    using validator = Validate;

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArgWithoutDefault(
      Name, Description, identity<T>, P &&parse_, V &&validate_) noexcept
      : parse(std::forward<P>(parse_)), validate(std::forward<V>(validate_)) {}

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArgWithoutDefault(Name,
                                        Description,
                                        P &&parse_,
                                        V &&validate_) noexcept
      : parse(std::forward<P>(parse_)), validate(std::forward<V>(validate_)) {}

    CLI_NO_UNIQUE_ADDRESS Name name{};
    CLI_NO_UNIQUE_ADDRESS Description description{};
    CLI_NO_UNIQUE_ADDRESS Parse parse{};
    CLI_NO_UNIQUE_ADDRESS Validate validate{};
  };

  template<Id Name,
           SC Description,
           typename T,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArgWithoutDefault(
    Name, Description, identity<T>, Parse &&, Validate &&)
    -> FunctionArgWithoutDefault<Name,
                                 Description,
                                 T,
                                 std::decay_t<Parse>,
                                 std::decay_t<Validate>>;

  template<Id Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArgWithoutDefault(Name, Description, Parse &&, Validate &&)
    -> FunctionArgWithoutDefault<
      Name,
      Description,
      parse::value_type_t<typename Name::char_type, Parse>,
      std::decay_t<Parse>,
      std::decay_t<Validate>>;

  template<Id Name, SC Description>
  struct UndeducedArg {
    using char_type = typename Name::char_type;
    using type = Deduced;
    using parser = parse::Parse<Deduced, char_type>;
    using validator = validate::DefaultValidate<Deduced>;

    CLI_NO_UNIQUE_ADDRESS Name name{};
    CLI_NO_UNIQUE_ADDRESS Description description{};
  };

  namespace dtl {
    template<Callable F,
             std::size_t I,
             SC N,
             SC D,
             class T,
             parse::Parser P,
             validate::Validator V>
    constexpr auto
    deduce_arg(const FunctionArgWithoutDefault<N, D, T, P, V> &arg) noexcept {
      using args = typename function_traits<F>::arguments;
      using arg_type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
      static_assert(parse::ParserOf<P, T, typename N::char_type>);
      static_assert(validate::ValidatorOf<V, T>);
      static_assert(std::same_as<arg_type, T>,
                    "the I-th arg's explicitly set type does not match "
                    "function F's I-th argument");
      return arg;
    }

    template<Callable F,
             std::size_t I,
             SC N,
             SC D,
             class T,
             auto DV,
             parse::Parser P,
             validate::Validator V>
    constexpr auto
    deduce_arg(const FunctionArg<N, D, T, DV, P, V> &arg) noexcept {
      using args = typename function_traits<F>::arguments;
      using arg_type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
      static_assert(parse::ParserOf<P, T, typename N::char_type>);
      static_assert(validate::ValidatorOf<V, T>);
      static_assert(std::constructible_from<T, decltype(DV)>);
      static_assert(std::same_as<arg_type, T>,
                    "the I-th arg's explicitly set type does not match "
                    "function F's I-th argument");
      return arg;
    }

    template<Callable F, std::size_t I, SC N, SC D>
    constexpr auto deduce_arg(const UndeducedArg<N, D> &) noexcept {
      using args = typename function_traits<F>::arguments;
      using type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
      if constexpr (std::is_same_v<D, string_constant<typename N::char_type>> or
                    std::is_same_v<D, NoDescription<typename N::char_type>>)
        return FunctionArgWithoutDefault{
          N{},
          cli::ctti::name<type, typename N::char_type>(),
          identity<type>{},
          parse::Parse<type, typename N::char_type>{},
          validate::DefaultValidate<type>{}};
      else
        return FunctionArgWithoutDefault{
          N{},
          D{},
          identity<type>{},
          parse::Parse<type, typename N::char_type>{},
          validate::DefaultValidate<type>{}};
    }

    template<Callable F, class... Args>
    constexpr auto deduce_args(const Args &...args) noexcept {
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return cli::Tuple(deduce_arg<F, Is>(args)...);
      }(std::make_index_sequence<sizeof...(Args)>());
    }

    template<Id Name,
             SC Description,
             class T,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto pretty_arg_name(
      const FunctionArgWithoutDefault<Name, Description, T, Parse, Validate>
        &) noexcept {
      using CharT = typename Name::char_type;
      return Name{} + string_constant<CharT, ':', ' '>{} +
             ctti::name<T, CharT>();
    }

    template<Id Name,
             SC Description,
             class T,
             auto DefaultValue,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto pretty_arg_name(
      const FunctionArg<Name, Description, T, DefaultValue, Parse, Validate>
        &) noexcept {
      using CharT = typename Name::char_type;
      return Name{} + string_constant<CharT, ':', ' '>{} +
             ctti::name<T, CharT>() +
             string_constant<CharT, '?', ' ', '=', ' '>{} +
             ctti::dtl::value<T, CharT>::template get<DefaultValue>();
    }

    template<FuncArg A, FuncArg... As>
    constexpr auto make_pretty_signature_name(const A &arg,
                                              const As &...args) noexcept {
      using CharT = typename A::char_type;
      if constexpr (sizeof...(As) == 0)
        return pretty_arg_name(arg);
      else
        return pretty_arg_name(arg) + string_constant<CharT, ',', ' '>{} +
               make_pretty_signature_name(args...);
    }

    template<typename CharT, Callable F, FuncArg... Args>
    constexpr auto pretty_signature_name(const Args &...args) noexcept {
      return string_constant<CharT, '('>{} +
             make_pretty_signature_name(args...) +
             string_constant<CharT, ')', '-', '>'>{} +
             ctti::name<typename function_traits<F>::return_type, CharT>();
    }

    template<typename CharT, Callable F, FuncArg... Args>
    constexpr auto
    pretty_signature_name(const cli::Tuple<Args...> &args) noexcept {
      return []<std::size_t... Is>(std::index_sequence<Is...>,
                                   const cli::Tuple<Args...> &args_) {
        return string_constant<CharT, '('>{} +
               make_pretty_signature_name(get<Is>(args_)...) +
               string_constant<CharT, ')', '-', '>'>{} +
               ctti::name<typename function_traits<F>::return_type, CharT>();
      }(std::make_index_sequence<sizeof...(Args)>(), args);
    }

    template<typename CharT, Callable F>
    constexpr auto pretty_signature_name() {
      return cli::string_constant<CharT, '(', ')', '-', '>'>{} +
             ctti::name<typename function_traits<F>::return_type, CharT>();
    }

    template<Id Name,
             SC Description,
             class T,
             auto DefaultValue,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto parse_field_from_arg(
      const FunctionArg<Name, Description, T, DefaultValue, Parse, Validate>
        &arg) noexcept {
      return parse::Field<typename Name::char_type, Name, DefaultValue, Parse>{
        DefaultValue, arg.parse};
    }

    template<Id Name,
             SC Description,
             typename T,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto parse_field_from_arg(
      const FunctionArgWithoutDefault<Name, Description, T, Parse, Validate>
        &arg) noexcept {
      return parse::
        FieldWithOutDefault<typename Name::char_type, Name, T, Parse>{
          T{}, arg.parse};
    }

    template<class... Args>
    constexpr auto
    parse_field_from_args(const cli::Tuple<Args...> &args) noexcept {
      return [&args]<std::size_t... Is>(std::index_sequence<Is...>) {
        return cli::Tuple(parse_field_from_arg(get<Is>(args))...);
      }(std::make_index_sequence<sizeof...(Args)>{});
    }

    struct ValidateResult {
      bool valid{false};
      std::size_t index{0};
    };
  } // namespace dtl

  /**
   * @defgroup Arguments Arguments
   * @ingroup Functions
   *
   * Arguments are the elements that describe c++ function arguments.
   *
   * There are two kinds of function arguments:
   *
   * - required: these parameters must be specified, else it is an error.
   * - optional: these parameters can be left out because they have a default
   * value.
   *
   * To create arguments, use the cli::arg overload set.
   *
   * See [here](docs.md#arguments) for more details.
   */

  /**
   * @defgroup optional-args Optional Arguments
   * @ingroup Arguments
   *
   * Optional arguments do not have to be specified when calling function
   * commands because they have a default value.
   *
   * For more details, see [here](docs.md#optional-arguments).
   *
   * @{
   */

  // clang-format off
  /**
  * creates an optional argument of type T.
  *
  * Intended usage:
  * ```
  *  using cli::operator""_sc;
  *  constexpr int DefaultValue = 1;
  *  constexpr auto arg = 
  *    cli::funcs::arg<double, DefaultValue>("x"_sc, 
  *                                  "the target x position"_sc, 
  *                                  cli::parse::Parse<double>{}, 
  *                                  cli::validate::DefaultValidate<double>{});
  * ```

  * @tparam T the arguments type
  * @tparam DefaultValue the default value of this argument
  * @param name the humanreadable name of the argument as a string_constant
  * @param description a string_constant that is used by the help functionality
  * @param parse the parser for the argument
  * @param validate the validator for the argument
  * @return a FunctionArg
  */
  // clang-format on
  template<class T,
           auto DefaultValue,
           Id Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse,
           validate::ValidatorOf<T> Validate>
  constexpr auto arg(Name name,
                     Description description,
                     Parse &&parse,
                     Validate &&validate) noexcept {
    (void)name;
    (void)description;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       std::forward<Parse>(parse),
                       std::forward<Validate>(validate)};
  }

  // clang-format off
  /**
  * creates an optional argument of type T. The default validator is used.
  *
  * Intended usage:
  * ```
  *  using cli::operator""_sc;
  *  constexpr int DefaultValue = 1;
  *  constexpr auto arg = 
  *    cli::funcs::arg<double, DefaultValue>("x"_sc, 
  *                                  "the target x position"_sc, 
  *                                  cli::parse::Parse<double>{});
  * ```

  * @tparam T the arguments type
  * @tparam DefaultValue the default value of this argument
  * @param name the humanreadable name of the argument as a string_constant
  * @param description a string_constant that is used by the help functionality
  * @param parse the parser for the argument
  * @return a FunctionArg
  */
  // clang-format on
  template<class T,
           auto DefaultValue,
           Id Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse>
  constexpr auto
  arg(Name name, Description description, Parse &&parse) noexcept {
    (void)name;
    (void)description;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       std::forward<Parse>(parse),
                       validate::DefaultValidate<T>{}};
  }

  // clang-format off
  /**
  * creates an optional argument of type T. The default parser is used.
  *
  * Intended usage:
  * ```
  *  using cli::operator""_sc;
  *  constexpr int DefaultValue = 1;
  *  constexpr auto arg = 
  *    cli::funcs::arg<double, DefaultValue>("x"_sc, 
  *                                  "the target x position"_sc, 
  *                                  cli::validate::DefaultValidate<double>{});
  * ```

  * @tparam T the arguments type
  * @tparam DefaultValue the default value of this argument
  * @param name the humanreadable name of the argument as a string_constant
  * @param description a string_constant that is used by the help functionality
  * @param validate the validator for the argument
  * @return a FunctionArg
  */
  // clang-format on
  template<class T,
           auto DefaultValue,
           Id Name,
           SC Description,
           validate::ValidatorOf<T> Validate>
  constexpr auto
  arg(Name name, Description description, Validate &&validate) noexcept {
    (void)name;
    (void)description;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::Parse<T, typename Name::char_type>{},
                       std::forward<Validate>(validate)};
  }

  // clang-format off
  /**
  * creates an optional argument of type T. The default parser and
  * validator are used.
  *
  * Intended usage:
  * ```
  *  using cli::operator""_sc;
  *  constexpr int DefaultValue = 1;
  *  constexpr auto arg = 
  *    cli::funcs::arg<double, DefaultValue>("x"_sc, "the target x position"_sc);
  * ```

  * @tparam T the arguments type
  * @tparam DefaultValue the default value of this argument
  * @param name the humanreadable name of the argument as a string_constant
  * @param description a string_constant that is used by the help functionality
  * @return a FunctionArg
  */
  // clang-format on
  template<class T, auto DefaultValue, Id Name, SC Description>
  constexpr auto arg(Name name, Description description) noexcept {
    (void)name;
    (void)description;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::Parse<T, typename Name::char_type>{},
                       validate::DefaultValidate<T>{}};
  }

  // clang-format off
  /**
  * creates an optional argument of type T. The default parser and
  * validator are used.
  *
  * Intended usage:
  * ```
  *  using cli::operator""_sc;
  *  constexpr int DefaultValue = 1;
  *  constexpr auto arg = 
  *    cli::funcs::arg<double, DefaultValue>("x"_sc);
  * ```

  * @tparam T the arguments type
  * @tparam DefaultValue the default value of this argument
  * @param name the humanreadable name of the argument as a string_constant
  * @return a FunctionArg
  */
  // clang-format on
  template<class T, auto DefaultValue, Id Name>
  constexpr auto arg(Name name) noexcept {
    (void)name;
    return FunctionArg{Name{},
                       NoDescription<typename Name::char_type>{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::Parse<T, typename Name::char_type>{},
                       validate::DefaultValidate<T>{}};
  }

  /**
   * creates an optional argument. The value type of the argument is deduced
   * from the value type of parse.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *  constexpr int DefaultValue = 1;
   *  constexpr auto arg =
   *    cli::funcs::arg<DefaultValue>("x"_sc,
   *                                  "the target x position"_sc,
   *                                  cli::parse::Parse<int>{},
   *                                  cli::validate::DefaultValidate<int>{});
   * ```

   * @tparam DefaultValue the default value of this argument
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param parse the parser for the argument
   * @param validate the validator for the argument
   * @return a FunctionArg
   */
  // clang-format on
  template<auto DefaultValue,
           Id Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  constexpr auto arg(Name name,
                     Description description,
                     Parse &&parse,
                     Validate &&validate) noexcept {
    (void)name;
    (void)description;
    return FunctionArg{Name{},
                       Description{},
                       constant<DefaultValue>{},
                       std::forward<Parse>(parse),
                       std::forward<Validate>(validate)};
  }

  /**
   * creates an optional argument. The value type of the argument is deduced
   * from the value type of parse.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *  constexpr int DefaultValue = 1;
   *  constexpr auto arg =
   *    cli::funcs::arg<DefaultValue>("x"_sc,
   *                                  "the target x position"_sc,
   *                                  cli::parse::Parse<double>{});
   * ```

   * @tparam DefaultValue the default value of this argument
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param parse the parser for the argument
   * @return a FunctionArg
   */
  // clang-format on
  template<auto DefaultValue,
           Id Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  constexpr auto
  arg(Name name, Description description, Parse &&parse) noexcept {
    (void)name;
    (void)description;
    using T = parse::value_type_t<typename Name::char_type, Parse>;

    return FunctionArg{Name{},
                       Description{},
                       constant<DefaultValue>{},
                       std::forward<Parse>(parse),
                       validate::DefaultValidate<T>{}};
  }

  /**
   * creates an optional argument. The value type of the argument is deduced
   * from the value type of validate.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *  constexpr int DefaultValue = 1;
   *  constexpr auto arg =
   *    cli::funcs::arg<DefaultValue>("x"_sc,
   *                                  "the target x position"_sc,
   *                                  cli::validate::DefaultValidate<double>{});
   * ```

   * @tparam DefaultValue the default value of this argument
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param validate the validator for the argument
   * @return a FunctionArg
   */
  // clang-format on
  template<auto DefaultValue,
           Id Name,
           SC Description,
           validate::Validator Validate>
  constexpr auto
  arg(Name name, Description description, Validate &&validate) noexcept {
    (void)name;
    (void)description;
    using T = validate::value_type_t<Validate>;
    using CharT = typename Name::char_type;
    return FunctionArg{Name{},
                       Description{},
                       constant<DefaultValue>{},
                       parse::Parse<T, CharT>{},
                       std::forward<Validate>(validate)};
  }

  // clang-format off
/**
 * creates an optional argument of the DefaultValues type. The default parser and
 * validator are used.
 *
 * Intended usage:
 * ```
 *  using cli::operator""_sc;
 *  constexpr int DefaultValue = 1;
 *  constexpr auto arg = 
 *            cli::funcs::arg<DefaultValue>("x"_sc
 *                                          "the target x position"_sc);
 * ```

 * @tparam DefaultValue the default value of this argument
 * @param name the humanreadable name of the argument as a string_constant
 * @param description the argument description as a string_constant
 * @return a FunctionArg
 */
  // clang-format on
  template<auto DefaultValue, Id Name, SC Description>
  constexpr auto arg(Name name, Description description) noexcept {
    (void)name;
    (void)description;
    using T = std::decay_t<decltype(DefaultValue)>;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::Parse<T, typename Name::char_type>{},
                       validate::DefaultValidate<T>{}};
  }

  // clang-format off
/**
 * creates an optional argument of the DefaultValues type. The default parser and
 * validator are used.
 *
 * Intended usage:
 * ```
 *  using cli::operator""_sc;
 *  constexpr int DefaultValue = 1;
 *  constexpr auto arg = cli::funcs::arg<DefaultValue>("x"_sc);
 * ```

 * @tparam DefaultValue the default value of this argument
 * @param name the humanreadable name of the argument as a string_constant
 * @return a FunctionArg
 */
  // clang-format on
  template<auto DefaultValue, Id Name>
  constexpr auto arg(Name name) noexcept {
    (void)name;
    using T = std::decay_t<decltype(DefaultValue)>;
    return FunctionArg{Name{},
                       NoDescription<typename Name::char_type>{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::Parse<T, typename Name::char_type>{},
                       validate::DefaultValidate<T>{}};
  }
  /**
   * @}
   */

  /**
   * @defgroup required-args Required Arguments
   * @ingroup Arguments
   *
   * Required arguments have to be specified when calling function commands
   * because they don't have a default value.
   *
   * See [here](docs.md#required-arguments) for more details.
   *
   * @{
   */

  /**
   * creates a required argument of type T.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *    cli::funcs::arg<int>("x"_sc,
   *                         "the target x position"_sc,
   *                         cli::parse::Parse<int, char>{},
   *                         cli::validate::DefaultValidate<int>{});
   * ```
   * @tparam T the arguments type
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param parse the parser for the argument
   * @param validate the validator for the argument
   * @return a FunctionArg
   */
  template<class T,
           Id Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse,
           validate::ValidatorOf<T> Validate>
  constexpr auto arg(Name name,
                     Description description,
                     Parse &&parse,
                     Validate &&validate) noexcept {
    (void)name;
    (void)description;
    return FunctionArgWithoutDefault{Name{},
                                     Description{},
                                     identity<T>{},
                                     std::forward<Parse>(parse),
                                     std::forward<Validate>(validate)};
  }

  /**
   * creates a required argument of type T. The default validator is used.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *    cli::funcs::arg<int>("x"_sc,
   *                         "the target x position"_sc,
   *                         cli::parse::Parse<int, char>{});
   * ```
   * @tparam T the arguments type
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param parse the parser for the argument
   * @return a FunctionArg
   */
  template<class T,
           Id Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse>
  constexpr auto
  arg(Name name, Description description, Parse &&parse) noexcept {
    (void)name;
    (void)description;
    return FunctionArgWithoutDefault{Name{},
                                     Description{},
                                     identity<T>{},
                                     std::forward<Parse>(parse),
                                     validate::DefaultValidate<T>{}};
  }

  /**
   * creates a required argument of type T. The default parser is used.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *    cli::funcs::arg<int>("x"_sc,
   *                         "the target x position"_sc,
   *                         cli::validate::DefaultValidate<int>{});
   * ```
   * @tparam T the arguments type
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param validate the validator for the argument
   * @return a FunctionArg
   */
  template<class T, Id Name, SC Description, validate::ValidatorOf<T> Validate>
  constexpr auto
  arg(Name name, Description description, Validate &&validate) noexcept {
    (void)name;
    (void)description;
    return FunctionArgWithoutDefault{
      Name{},
      Description{},
      identity<T>{},
      parse::Parse<T, typename Name::char_type>{},
      std::forward<Validate>(validate)};
  }

  /**
   * creates a required argument of type T. The default parser and validator are
   * used.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *    cli::funcs::arg<int>("x"_sc,
   *                         "the target x position"_sc);
   * ```
   * @tparam T the arguments type
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @return a FunctionArg
   */
  template<class T, Id Name, SC Description>
  constexpr auto arg(Name name, Description description) noexcept {
    (void)name;
    (void)description;
    return FunctionArgWithoutDefault{
      Name{},
      Description{},
      identity<T>{},
      parse::Parse<T, typename Name::char_type>{},
      validate::DefaultValidate<T>{}};
  }

  /**
   * creates a required argument of type T. The default parser and validator are
   * used.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *  cli::funcs::arg<int>("x"_sc);
   * ```
   * @tparam T the arguments type
   * @param name the humanreadable name of the argument as a string_constant
   * @return a FunctionArg
   */
  template<class T, Id Name>
  constexpr auto arg(Name name) noexcept {
    (void)name;
    return FunctionArgWithoutDefault{
      Name{},
      NoDescription<typename Name::char_type>{},
      identity<T>{},
      parse::Parse<T, typename Name::char_type>{},
      validate::DefaultValidate<T>{}};
  }

  /// @}

  /**
   * @defgroup deduced-args Deduced Arguments
   * @ingroup required-args
   * Deduced arguments are required arguments that have their type deduced. The
   * default parser and validator are always used for these type of arguments.
   * The benefit of deduced arguments is the shorter notation.
   *
   * For more details, see [here](docs.md#deduced-arguments).
   *
   * @{
   */

  /**
   * creates a required argument. Its type will be deduced. the default parser
   * and validator are used.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *    cli::funcs::arg("x"_sc,
   *                    "the target x position"_sc);
   * ```
   *
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @return a FunctionArg
   */
  template<Id Name, SC Description>
  constexpr auto arg(Name name, Description description) noexcept {
    (void)name;
    (void)description;
    return UndeducedArg<Name, Description>{};
  }

  /**
   * creates a required argument. Its type, parser, and validator will be
   * deduced.
   *
   * Intended usage:
   * ```
   *  using cli::operator""_sc;
   *    cli::funcs::arg("x"_sc);
   * ```
   *
   * @param name the humanreadable name of the argument as a string_constant
   * @return a FunctionArg
   */
  template<Id Name>
  constexpr auto arg(Name name) noexcept {
    (void)name;
    return UndeducedArg<Name, NoDescription<typename Name::char_type>>{};
  }

  /**
   * the literal operator for arguments.
   *
   * This is equivalent to ``arg(Name)``
   *
   * Intended usage:
   * ```
   *  using cli::funcs::operator""_arg;
   *  constexpr auto x = "x"_arg;// equivalent: cli::funcs::arg("x"_sc)
   * ```
   *
   * @return arg(Name)
   */
  template<cli::StringLiteral Name>
  constexpr auto operator""_arg() noexcept {
    return arg([&]<std::size_t... Is>(std::index_sequence<Is...>) {
      return string_constant<typename decltype(Name)::char_type,
                             Name.s[Is]...>{};
    }(std::make_index_sequence<Name.size()>()));
  }

  /// @}

  struct FuncGetter {
    template<typename Function>
    constexpr auto &get(Function &f) {
      return f.func_;
    }
  };

  template<typename CharT>
  constexpr ExecResult<CharT> preparse_args(View<const CharT> &args) {
    View<const CharT> args_ = parse::trim_ws(args);

    if (args_.size() == 0) {
      return ExecResult<CharT>::make_parse_error(Error::expected_lparen,
                                                 args.begin());
    } else if (args_.size() == 1) {
      if (args_[0] == '(')
        return ExecResult<CharT>::make_parse_error(Error::expected_rparen,
                                                   nullptr);
      else
        return ExecResult<CharT>::make_parse_error(Error::expected_lparen,
                                                   nullptr);
    } else if (args_.size() == 2) {
      if (args_[0] != '(')
        return ExecResult<CharT>::make_parse_error(Error::expected_lparen,
                                                   args_.begin());
      if (args_[1] != ')') {
        return ExecResult<CharT>::make_parse_error(Error::expected_rparen,
                                                   args_.begin() + 1);
      }
    } else {
      if (args_[0] != '(')
        return ExecResult<CharT>::make_parse_error(Error::expected_lparen,
                                                   args_.begin());
    }
    args = args_;
    return ExecResult<CharT>::make_success();
  }

  template<Id Name, SC Description, SC Type, Callable F, FuncArg... Args>
  class Function
    : public CommandBase<Function<Name, Description, Type, F, Args...>,
                         Name,
                         Description,
                         Type> {
    using Base = CommandBase<Function<Name, Description, Type, F, Args...>,
                             Name,
                             Description,
                             Type>;
    using traits = function_traits<F>;
    using arguments = typename traits::arguments;

    template<class... Fields>
    using PartialParser = parse::
      FieldGroup<typename Name::char_type, '=', ',', '(', ')', Fields...>;
    using Parser = type_list::apply_t<PartialParser,
                                      decltype(dtl::parse_field_from_args(
                                        std::declval<cli::Tuple<Args...>>()))>;
    friend struct FuncGetter;

  public:
    using char_type = typename Base::char_type;
    using Base::description;
    using Base::name;
    using Base::type;
    using sub_command_list = TypeList<>;

    using signature = typename traits::signature_type;

    static_assert(all_same_char_type_v<Name, Description, Type, Args...>,
                  "name, description, and args must all use the same character "
                  "type. Make sure the name, the description and the "
                  "the names and descriptions of the arguments have the same "
                  "character type.");

    template<Callable Func, FuncArg... A>
    constexpr Function(
      Name, Description, Type, Func &&function, A &&...args) noexcept
      : func_(std::forward<Func>(function)), args_(std::forward<A>(args)...) {}

    template<Callable Func>
    constexpr Function(Name,
                       Description,
                       Type,
                       Func &&function,
                       const cli::Tuple<Args...> &args) noexcept
      : func_(std::forward<Func>(function)), args_(args) {}

    template<Callable Func>
    constexpr Function(Name,
                       Description,
                       Type,
                       Func &&function,
                       cli::Tuple<Args...> &&args) noexcept
      : func_(std::forward<Func>(function)), args_(std::move(args)) {}

    constexpr ExecResult<char_type>
    execute(View<const char_type> args,
            [[maybe_unused]] View<char_type> out) noexcept {
      using Ret = typename traits::return_type;

      ExecResult exec_res = preparse_args(args);
      if (not exec_res)
        return exec_res;

      Parser parse{dtl::parse_field_from_args(this->args_)};

      parse::ParseResult res = parse(args);

      if (not res)
        return ExecResult<char_type>::make_parse_error(res.error,
                                                       res.rest.data());

      if (res.rest.size() != 0)
        return ExecResult<char_type>::make_parse_error(
          Error::unexpected_characters_after_closing_paren, res.rest.data());

      return [&tuple = res.value, &out, this]<std::size_t... Is>(
               std::index_sequence<Is...>) -> ExecResult<char_type> {
        if (auto validate_res = validate(tuple, std::index_sequence<Is...>{});
            not validate_res.valid)
          return ExecResult<char_type>::make_validation_error(
            validate_res.index);

        if constexpr (std::is_same_v<void, Ret>) {
          (void)out;
          func_(get<Is>(tuple).value...);
          return ExecResult<char_type>::make_success();
        } else {
          Ret ret_val = func_(get<Is>(tuple).value...);
          format::Format<Ret, typename Base::char_type> format{};
          format::FormatResult fmt_result = format(out, ret_val);
          if (not fmt_result)
            return ExecResult<char_type>::make_format_error(fmt_result.error);
          else
            return ExecResult<char_type>::make_success(
              out.substr(0, fmt_result.size_written));
        }
      }(std::make_index_sequence<sizeof...(Args)>());
    }

    template<std::size_t I, std::size_t... Is>
    constexpr dtl::ValidateResult
    validate(const auto &tuple, std::index_sequence<I, Is...>) noexcept {
      auto valid = get<I>(args_).validate(get<I>(tuple).value);
      if constexpr (sizeof...(Is) == 0)
        return {.valid = valid, .index = type_list::list_size_v<arguments>};
      else {
        if (not valid)
          return {.valid = false, .index = I + 1};
        return validate(tuple, std::index_sequence<Is...>{});
      }
    }

    View<const char_type>
    help_context(View<const char_type> arg) const noexcept {
      return get_help(arg, std::make_index_sequence<sizeof...(Args)>{});
    }

  private:
    template<Id N,
             SC D,
             class T,
             auto DefaultValue,
             parse::Parser P,
             validate::Validator V>
    static constexpr auto
    pretty_arg_type(const FunctionArg<N, D, T, DefaultValue, P, V> &) noexcept {
      return ctti::name<T>() +
             string_constant<typename Name::char_type, '?', ' ', '=', ' '>{} +
             ctti::dtl::value<T>::template get<DefaultValue>();
    }

    template<Id N, SC D, class T, parse::Parser P, validate::Validator V>
    static constexpr auto
    pretty_arg_type(const FunctionArgWithoutDefault<N, D, T, P, V> &) noexcept {
      using CharT = typename Name::char_type;
      return ctti::name<T, CharT>();
    }

    template<std::size_t I, std::size_t... Is>
    View<const char_type>
    get_help(View<const char_type> arg,
             std::index_sequence<I, Is...>) const noexcept {
      if (arg == get<I>(args_).name) {
        return string_constant<char_type, '['>{} +
               pretty_arg_type(get<I>(args_)) +
               string_constant<char_type, ']', ':', ' '>{} +
               get<I>(args_).description;
      } else {
        if constexpr (sizeof...(Is) == 0)
          return {};
        else
          return get_help(arg, std::index_sequence<Is...>{});
      }
    }

    CLI_NO_UNIQUE_ADDRESS F func_{};
    CLI_NO_UNIQUE_ADDRESS cli::Tuple<Args...> args_{};
  };

  template<typename CharT>
  constexpr ExecResult<CharT> parse_void_args(View<const CharT> args) {
    args = parse::trim_ws(args);

    if (args.size() == 0) {
      return ExecResult<CharT>::make_parse_error(Error::expected_lparen,
                                                 nullptr);
    }

    if (args.size() == 1) {
      if (args[0] == '(')
        return ExecResult<CharT>::make_parse_error(Error::expected_rparen,
                                                   args.end());
      else
        return ExecResult<CharT>::make_parse_error(Error::expected_lparen,
                                                   args.end());
    }

    if (args.size() > 1) {
      if (args[0] == '(') {
        View rest = parse::skip_ws(args.substr(1));
        if (rest.size() == 0) {
          return ExecResult<CharT>::make_parse_error(Error::expected_rparen,
                                                     args.end());
        }

        if (rest[0] != ')') {
          return ExecResult<CharT>::make_parse_error(Error::invalid_argument,
                                                     args.begin());
        }
        View end = rest.substr(1);
        if (end.size() != 0) {
          return ExecResult<CharT>::make_parse_error(
            Error::unexpected_characters_after_closing_paren, end.begin());
        }
      } else {
        return ExecResult<CharT>::make_parse_error(Error::expected_lparen,
                                                   args.begin());
      }
    }
    return ExecResult<CharT>::make_success();
  }

  template<Id Name, SC Description, SC Type, Callable F>
  class Function<Name, Description, Type, F>
    : public CommandBase<Function<Name, Description, Type, F>,
                         Name,
                         Description,
                         Type> {
    using Base = CommandBase<Function<Name, Description, Type, F>,
                             Name,
                             Description,
                             Type>;
    using traits = function_traits<F>;
    using arguments = typename traits::arguments;

  public:
    using char_type = typename Base::char_type;
    using Base::description;
    using Base::name;
    using Base::type;
    using sub_command_list = TypeList<>;

    using signature = typename traits::signature_type;

    static_assert(all_same_char_type_v<Name, Description, Type>,
                  "name and description must use the same character "
                  "type. Make sure the name and the description use the same "
                  "character type.");

    template<Callable Func>
    constexpr Function(Name, Description, Type, Func &&function) noexcept
      : func_(std::forward<Func>(function)) {}

    template<Callable Func>
    constexpr Function(Name,
                       Description,
                       Type,
                       Func &&function,
                       [[maybe_unused]] Tuple<> args) noexcept
      : func_(std::forward<Func>(function)) {}

    constexpr ExecResult<char_type>
    execute(View<const char_type> args,
            [[maybe_unused]] View<char_type> out) noexcept {
      using Ret = typename traits::return_type;

      ExecResult<char_type> exec_res = parse_void_args(args);
      if (not exec_res)
        return exec_res;

      if constexpr (std::is_same_v<void, Ret>) {
        func_();
        return ExecResult<char_type>::make_success();
      } else {
        format::Format<Ret, typename Base::char_type> format;
        format::FormatResult res = format(out, func_());
        if (not res)
          return ExecResult<char_type>::make_format_error(res.error);
        else
          return ExecResult<char_type>::make_success(
            out.substr(0, res.size_written));
      }
    }

  private:
    CLI_NO_UNIQUE_ADDRESS F func_{};
  };

  // template <Id Name, SC Description, SC Help, Callable F>
  // Function(Name, Description, Help, F &&)
  //     -> Function<Name, Description, Help, std::remove_cvref_t<F>>;

  template<Id Name, SC Description, SC Type, Callable F, FuncArg... Args>
  Function(Name, Description, Type, F &&, Args &&...)
    -> Function<Name,
                Description,
                Type,
                std::decay_t<F>,
                std::decay_t<Args>...>;

  template<Id Name, SC Description, SC Type, Callable F, FuncArg... Args>
  Function(Name, Description, Type, F &&, const cli::Tuple<Args...> &)
    -> Function<Name, Description, Type, std::decay_t<F>, Args...>;

  template<Id Name, SC Description, SC Type, Callable F, FuncArg... Args>
  Function(Name, Description, Type, F &&, cli::Tuple<Args...> &&)
    -> Function<Name, Description, Type, std::decay_t<F>, Args...>;

  /**
   * @defgroup Functions Functions
   * @ingroup Commands
   *
   * Functions are commands that can be called with arguments.
   *
   * A function is fully defined by:
   * - name: the function's name. Must be a cli::string_constant.
   * - description: the function's description. Must be a cli::string_constant.
   * - a callable f: the C++ callable that actually performs the action. This
   * may be a free function, a functor/lambda, or a member function.
   * - arguments: Elements that describe the callable's arguments. See @ref
   * Arguments.
   *
   * Functions are created with the cli::func overload set.
   *
   * For more details and a nice overview of all available overloads, see
   * [here](docs.md#functions).
   * @{
   */

  /**
   * @brief create a function command
   *
   * Usage:
   * ```
   * using cli::operator""_sc;
   * using cli::funcs::operator""_arg;
   *
   * void f1(int i);
   * auto lambda = [](char c){...};
   * struct F{
   * int operator()()}{...}
   * };
   *
   * constexpr auto f1_func =
   *                cli::funcs::func("f1"_sc,
   *                                 "a description"_sc,
   *                                 f1,
   *                                 "i"_arg);
   * constexpr auto lambda_func =
   *                cli::funcs::func("lambda"_sc,
   *                                 "a description"_sc,
   *                                 lambda,
   *                                 "c"_arg);
   * constexpr auto F_func =
   *                cli::funcs::func("f"_sc,
   *                                 "a description"_sc,
   *                                 F{});
   * ```
   * @param name the function's name. Must be a cli::string_constant.
   * @param description the function's description. Must be a
   * cli::string_constant.
   * @param f the actual function to call
   * @param args the functions arguments. See cli::arg and @ref Arguments.
   * @return
   */
  template<Id Name, SC Description, Callable F, class... Args>
  [[nodiscard]] constexpr auto
  func(Name name, Description description, F &&f, Args &&...args) noexcept {
    (void)name;
    (void)description;
    static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
        sizeof...(Args),
      "All arguments of f must be named");

    static_assert(all_same_char_type_v<Name, Description, Args...>,
                  "name, description, and args must all use the same character "
                  "type. Make sure the name, the description and the "
                  "the names and descriptions of the arguments have the same "
                  "character type.");

    if constexpr (sizeof...(Args) > 0) {
      auto deduced_ = dtl::deduce_args<F>(std::forward<Args>(args)...);
      return Function{
        Name{},
        Description{},
        dtl::pretty_signature_name<typename Name::char_type, F>(deduced_),
        std::forward<F>(f),
        deduced_};
    } else {
      return Function{Name{},
                      Description{},
                      dtl::pretty_signature_name<typename Name::char_type, F>(),
                      std::forward<F>(f)};
    }
  }

  /**
   * @brief create a function command
   *
   * Usage:
   * ```
   * using cli::operator""_sc;
   * using cli::funcs::operator""_arg;
   *
   * void f1(int i);
   * auto lambda = [](char c){...};
   * struct F{
   * int operator()()}{...}
   * };
   *
   * constexpr auto f1_func =
   *                cli::funcs::func("f1"_sc,
   *                                 f1,
   *                                 "i"_arg);
   * constexpr auto lambda_func =
   *                cli::funcs::func("lambda"_sc,
   *                                 lambda,
   *                                 "c"_arg);
   * constexpr auto F_func =
   *                cli::funcs::func("f"_sc,
   *                                 F{});
   * ```
   * @param name the function's name. Must be a cli::string_constant.
   * @param f the actual function to call
   * @param args the functions arguments. See cli::arg and @ref Arguments
   * @return
   */
  template<Id Name, Callable F, class... Args>
  [[nodiscard]] constexpr auto func(Name name, F &&f, Args &&...args) noexcept {
    (void)name;
    return func(Name{},
                NoDescription<typename Name::char_type>{},
                std::forward<F>(f),
                std::forward<Args>(args)...);
  }

  /**
   * @brief creates a function command. The command name will be the name of F.
   *
   * Usage:
   * ```
   * using cli::operator""_sc;
   * using cli::funcs::operator""_arg;
   *
   * struct F{
   * int operator()(int i)}{...}
   * };
   *
   * // equvilanet to cli::func("F"_sc, "a description"_sc, F{}, "i"_arg)
   * constexpr auto F_func = cli::func(F{}, "a description"_sc, "i"_arg);
   *
   * ```
   * @param f the callable. Can't be a free function or function pointer.
   * @param description the callable's description. Must be a
   * cli::string_constant.
   * @param args the callable's arguments. See cli::arg and @ref Arguments.
   */
  template<Callable F, SC Description, class... Args>
    requires(not std::is_pointer_v<std::decay_t<F>>)
  [[nodiscard]] constexpr auto
  func(F &&f, Description description, Args &&...args) noexcept {
    (void)description;
    return func(ctti::name<std::decay_t<F>, typename Description::char_type>(),
                Description{},
                std::forward<F>(f),
                std::forward<Args>(args)...);
  }

  /**
   * @brief creates a function command. The command name will be the name of F.
   *
   * Usage:
   * ```
   * using cli::funcs::operator""_arg;
   *
   * struct F{
   * int operator()(int i)}{...}
   * };
   *
   * constexpr auto F_func = cli::funcs::func(F{}, "i"_arg);
   * ```
   * @param f the callable. Can't be a free function or function pointer.
   * @param args the callable's arguments. See cli::arg and @ref Arguments.
   */
  template<Callable F, class... Args>
    requires(not std::is_pointer_v<std::decay_t<F>>)
  [[nodiscard]] constexpr auto func(F &&f, Args &&...args) noexcept {
    if constexpr (sizeof...(Args) == 0) {
      return func(ctti::name<std::decay_t<F>>(),
                  NoDescription<char>{},
                  std::forward<F>(f));
    } else {
      using char_type = typename type_list::
        type_at_t<0, type_list::TypeList<std::decay_t<Args>...>>::char_type;
      return func(ctti::name<std::decay_t<F>, char_type>(),
                  NoDescription<char_type>{},
                  std::forward<F>(f),
                  std::forward<Args>(args)...);
    }
  }

  /**
   * @brief creates a Function from a T and its member function.
   *
   * Example:
   * ```
   *  struct S{
   *    void apply(int i);
   *  };
   *
   *  S s;
   *
   *  auto f = cli::func("apply"_sc,
   *                     "apply description"_sc,
   *                     s,
   *                     &S::apply,
   *                     "i"_arg);
   * ```
   * @param name the function name. Must be a cli::string_constant.
   * @param description the function description. Must be a
   * cli::string_constant.
   * @param t the object
   * @param mem_fun pointer to the member function
   * @param args the member function's arguments. See cli::arg and @ref
   * Arguments.
   */
  template<Id Name,
           SC Description,
           class T,
           class MemberFunctionPointer,
           class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer> and
             (not function_traits<MemberFunctionPointer>::is_const)
  [[nodiscard]] constexpr auto func(Name name,
                                    Description description,
                                    T &t,
                                    MemberFunctionPointer mem_fun,
                                    Args &&...args) noexcept {
    (void)name;
    (void)description;
    using Binder = MemFunBinder<T, MemberFunctionPointer>;
    return func(
      Name{}, Description{}, Binder{t, mem_fun}, std::forward<Args>(args)...);
  }

  /**
   * @brief creates a Function from a T and its const member function.
   *
   * Example:
   * ```
   *  struct S{
   *    void foo(int i) const;
   *  };
   *
   *  S s;
   *
   *  auto f = cli::func("foo"_sc,
   *                     "foo description"_sc,
   *                     s,
   *                     &S::foo,
   *                     "i"_arg);
   * ```
   * @param name the function name. Must be a cli::string_constant.
   * @param description the function description. Must be a
   * cli::string_constant.
   * @param t the object
   * @param mem_fun pointer to the member function
   * @param args the member function's arguments. See cli::arg and @ref
   * Arguments.
   */
  template<Id Name,
           SC Description,
           class T,
           class MemberFunctionPointer,
           class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer> and
             function_traits<MemberFunctionPointer>::is_const
  [[nodiscard]] constexpr auto func(Name name,
                                    Description description,
                                    const T &t,
                                    MemberFunctionPointer mem_fun,
                                    Args &&...args) noexcept {
    (void)name;
    (void)description;
    using Binder = MemFunBinder<const T, MemberFunctionPointer>;
    return func(
      Name{}, Description{}, Binder{t, mem_fun}, std::forward<Args>(args)...);
  }

  /**
   * @brief creates a Function from a T and its member function.
   *
   * Example:
   * ```
   *  struct S{
   *    void apply(int i);
   *  };
   *
   *  S s;
   *
   *  auto f = cli::func("apply"_sc,
   *                     s,
   *                     &S::apply,
   *                     "i"_arg);
   * ```
   * @param name the function name. Must be a cli::string_constant.
   * @param t the object
   * @param mem_fun pointer to the member function
   * @param args the member function's arguments. See cli::arg and @ref
   * Arguments.
   */
  template<Id Name, class T, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer> and
             (not function_traits<MemberFunctionPointer>::is_const)
  [[nodiscard]] constexpr auto func(Name name,
                                    T &t,
                                    MemberFunctionPointer mem_fun,
                                    Args &&...args) noexcept {
    return func(name,
                NoDescription<typename Name::char_type>{},
                t,
                mem_fun,
                std::forward<Args>(args)...);
  }

  /**
   * @brief creates a Function from a T and its const member function.
   *
   * Example:
   * ```
   *  struct S{
   *    void foo(int i) const;
   *  };
   *
   *  S s;
   *
   *  auto f = cli::func("foo"_sc,
   *                     s,
   *                     &S::foo,
   *                     "i"_arg);
   * ```
   * @param name the function name. Must be a cli::string_constant.
   * @param t the object
   * @param mem_fun pointer to the member function
   * @param args the member function's arguments. See cli::arg and @ref
   * Arguments.
   */
  template<Id Name, class T, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer> and
             function_traits<MemberFunctionPointer>::is_const
  [[nodiscard]] constexpr auto func(Name name,
                                    const T &t,
                                    MemberFunctionPointer mem_fun,
                                    Args &&...args) noexcept {
    return func(name,
                NoDescription<typename Name::char_type>{},
                t,
                mem_fun,
                std::forward<Args>(args)...);
  }

  /**
   * @}
   */

  template<Id Name, SC Description, SC Help, class Function, class... Args>
  struct MemberFunction {
    using arguments = TypeList<Args...>;
    static_assert(std::is_member_function_pointer_v<Function>,
                  "A MemberFunctions Function template argument must be a "
                  "pointer to member function");

    static_assert(all_same_char_type_v<Name, Description, Args...>,
                  "name, description, and args must all use the same character "
                  "type. Make sure the name, the description and the "
                  "the names and descriptions of the arguments have the same "
                  "character type.");

    Function func;
    cli::Tuple<Args...> args;

    constexpr MemberFunction(Name,
                             Description,
                             Help,
                             Function mem_fun_ptr,
                             const Args &...func_args) noexcept
      : func(mem_fun_ptr), args(func_args...) {}

    constexpr MemberFunction(Name,
                             Description,
                             Help,
                             Function mem_fun_ptr,
                             const cli::Tuple<Args...> &func_args) noexcept
      : func(mem_fun_ptr), args(func_args) {}
  };

  template<Id Name, SC Description, SC Help, class Function, class... Args>
  MemberFunction(Name &&, Description &&, Help &&, Function &&, Args &&...)
    -> MemberFunction<std::decay_t<Name>,
                      std::decay_t<Description>,
                      std::decay_t<Help>,
                      std::decay_t<Function>,
                      std::decay_t<Args>...>;
  template<Id Name, SC Description, SC Help, class Function, FuncArg... Args>
  MemberFunction(
    Name &&, Description &&, Help &&, Function &&, const cli::Tuple<Args...> &)
    -> MemberFunction<std::decay_t<Name>,
                      std::decay_t<Description>,
                      std::decay_t<Help>,
                      std::decay_t<Function>,
                      std::decay_t<Args>...>;

  template<Id Name, SC Description, SC Help, class Function>
  MemberFunction(Name &&, Description &&, Help &&, Function &&)
    -> MemberFunction<std::decay_t<Name>,
                      std::decay_t<Description>,
                      std::decay_t<Help>,
                      std::decay_t<Function>>;

  template<typename T>
  inline constexpr bool is_member_function_v = false;

  template<class Name,
           class Description,
           class Help,
           class Function,
           class... Args>
  inline constexpr bool is_member_function_v<
    MemberFunction<Name, Description, Help, Function, Args...>> = true;

  /**
   * @defgroup member-functions Member Functions
   * @ingroup Functions
   *
   * Member function commands are partial commands for member functions. They
   * are called partial because they can't be used standalone, e.g. their parent
   * command must be an object that the member function can be called on.
   *
   * See [here](docs.md#member-functions) for more details.
   * @{
   */

  /**
   * creates a member function command. This command cannot be used
   * standalone. Its parent command must be an object that this member function
   * can be called on.
   *
   *
   * Intended usage:
   *
   * ```
   *  struct Foo{
   *    void bar(int param);
   *    int baz();
   *  };
   *
   *  static Foo foo;
   *
   * using cli::operator""_sc;
   * using cli::funcs::operator""_arg;
   *
   *  cli::Cli my_cli(...,
   *                  cli::param("foo"_sc,
   *                             foo, // <- foo must be direct parent of bar and
   *                                  // baz
   *                             cli::func("bar"_sc,
   *                                          "bars the foo"_sc,
   *                                          &Foo::bar,
   *                                          "param"_arg),
   *                             cli::func("baz"_sc,
   *                                          "bazzes"_sc,
   *                                          &Foo::baz),
   *                             ... ),
   *                  ...);
   * ```
   *
   * This is used to avoid repeating the object, i.e. ``foo``, for every
   * member function added, as would be the case when using cli::func().
   *
   * @param name the name of the member function
   * @param description the description of the member function
   * @param mem_fun pointer to the member function
   * @param args the arguments. See cli::arg and @ref Arguments.
   * @return a partial Command
   */
  template<Id Name, SC Description, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  [[nodiscard]] constexpr auto func(Name name,
                                    Description description,
                                    MemberFunctionPointer mem_fun,
                                    Args &&...args) noexcept {
    static_assert(all_same_char_type_v<Name, Description, Args...>,
                  "name, description, and args must all use the same character "
                  "type. Make sure the name, the description and the "
                  "the names and descriptions of the arguments have the same "
                  "character type.");
    (void)name;
    (void)description;
    auto deduced_args =
      dtl::deduce_args<MemberFunctionPointer>(std::forward<Args>(args)...);
    return MemberFunction{
      Name{},
      Description{},
      dtl::pretty_signature_name<typename Name::char_type, decltype(mem_fun)>(
        deduced_args),
      mem_fun,
      deduced_args};
  }

  /**
   * creates a member function command. This command cannot be used
   * standalone. Its parent command must be an object that this member function
   * can be called on.
   *
   *
   * Intended usage:
   *
   * ```
   *  struct Foo{
   *    void bar(int param);
   *    int baz();
   *  };
   *
   *  static Foo foo;
   *
   *  using cli::funcs::operator""_arg;
   *
   *  cli::Cli my_cli(...,
   *                  cli::param("foo"_sc, foo, // <- foo must be direct parent
   *                                            // of bar and baz
   *                             cli::func("bar"_sc, &Foo::bar, "param"_arg),
   *                             cli::func("baz"_sc, &Foo::baz),
   *                             ... ),
   *                  ...);
   * ```
   *
   * This is used to avoid repeating the object, i.e. ``foo``, for every
   * member function added, as would be the case when using cli::func().
   *
   * @param name the name of the member function
   * @param mem_fun pointer to the member function
   * @param args the arguments. See cli::arg and @ref Arguments.
   * @return a partial Command
   */
  template<Id Name, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  [[nodiscard]] constexpr auto
  func(Name name, MemberFunctionPointer mem_fun, Args &&...args) noexcept {
    (void)name;
    return func(Name{},
                NoDescription<typename Name::char_type>{},
                mem_fun,
                std::forward<Args>(args)...);
  }

  /**
   * creates a member function command. This command cannot be used
   * standalone. Its parent command must be an object that this member function
   * can be called on.
   *
   *
   * Intended usage:
   *
   * ```
   *  struct Foo{
   *    void bar(int param);
   *    int baz();
   *  };
   *
   *  static Foo foo;
   *
   *  using cli::operator""_sc;
   *  using cli::funcs::operator""_arg;
   *
   *  cli::Cli my_cli(...,
   *    cli::param("foo"_sc, foo, // <- foo must be direct parent
   *                              // of bar and baz
   *               cli::func<&Foo::bar>("bars the foo"_sc,
   *                                    param"_arg), // name is deduced to
   *                                                 // "bar"
   *               cli::func<&Foo::baz>("bazzes"_sc), // name is deduced to
   *                                                  // "baz"
   *               ... ),
   *    ...);
   * ```
   *
   * This is used to avoid repeating the object, i.e. ``foo``, for every
   * member function added, as would be the case when using cli::func().
   *
   * Additionally, the name of the function is automatically deduced.
   *
   * @tparam MemberFunctionPointer the pointer to the member function
   * @param description the description of the member function
   * @param args the arguments. See cli::arg and @ref Arguments.
   * @return a partial Command
   */
  template<auto MemberFunctionPointer, SC Description, class... Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunctionPointer)>
  [[nodiscard]] constexpr auto func(Description description,
                                    Args &&...args) noexcept {
    (void)description;
    return func(ctti::value_name<MemberFunctionPointer,
                                 typename Description::char_type>(),
                Description{},
                MemberFunctionPointer,
                std::forward<Args>(args)...);
  }

  /**
   * creates a member function command. This command cannot be used
   * standalone. Its parent command must be an object that this member function
   * can be called on.
   *
   *
   * Intended usage:
   *
   * ```
   *  struct Foo{
   *    void bar(int param);
   *    int baz();
   *  };
   *
   *  static Foo foo;
   *
   *  using cli::operator""_sc;
   *  using cli::funcs::operator""_arg;
   *
   *  cli::Cli my_cli(...,
   *    cli::param("foo"_sc,
   *               foo, // <- foo must be direct parent of bar and baz
   *               cli::func<&Foo::bar>("param"_arg), // name is deduced to
   *                                                  // "bar"
   *               cli::func<&Foo::baz>(), // name is deduced to "baz"
   *               ... ),
   *    ...);
   * ```
   *
   * This is used to avoid repeating the object, i.e. ``foo``, for every
   * member function added, as would be the case when using cli::func().
   *
   * Additionally, the name of the function is automatically deduced.
   *
   * @tparam MemberFunctionPointer the pointer to the member function
   * @param args the arguments. See cli::arg and @ref Arguments.
   */
  template<auto MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunctionPointer)>
  [[nodiscard]] constexpr auto func(Args &&...args) noexcept {
    static_assert(all_same_char_type_v<Args...>,
                  "args must all use the same character type. Make sure all "
                  "the names and descriptions of the arguments have the same "
                  "character type.");
    if constexpr (sizeof...(Args) > 0) {
      using CharT = typename type_list::
        type_at_t<0, type_list::TypeList<Args...>>::name::char_type;
      constexpr auto deduced_args =
        dtl::deduce_args<decltype(MemberFunctionPointer)>(
          std::forward<Args>(args)...);
      return MemberFunction{
        ctti::value_name<MemberFunctionPointer, CharT>(),
        NoDescription<CharT>{},
        dtl::pretty_signature_name<CharT, decltype(MemberFunctionPointer)>(
          deduced_args),
        MemberFunctionPointer,
        deduced_args};
    } else {
      return MemberFunction{
        ctti::value_name<MemberFunctionPointer>(),
        NoDescription<char>{},
        dtl::pretty_signature_name<char, decltype(MemberFunctionPointer)>(),
        MemberFunctionPointer};
    }
  }

  /**
   * @}
   */

  namespace dtl {
    /**
     * create a Function from an object reference and a MemberFunction
     */
    template<class T, Id Name, SC Description, SC Help, class F, class... Args>
    constexpr auto
    to_cmd(T &obj,
           const MemberFunction<Name, Description, Help, F, Args...>
             &member_function) noexcept {
      if constexpr (sizeof...(Args) == 0)
        return Function(Name{},
                        Description{},
                        Help{},
                        MemFunBinder(obj, member_function.func));
      else
        return Function(Name{},
                        Description{},
                        Help{},
                        MemFunBinder(obj, member_function.func),
                        member_function.args);
    }

    /**
     * create a Function from an object referenece and a MemberFunction
     */
    template<class T, Id Name, SC Description, SC Help, class F, class... Args>
    constexpr auto
    to_cmd(const T &obj,
           const MemberFunction<Name, Description, Help, F, Args...>
             &member_function) noexcept {
      static_assert(
        function_traits<F>::is_const,
        "Can't bind non-const member functions to const parameters");
      if constexpr (sizeof...(Args) == 0)
        return Function(Name{},
                        Description{},
                        Help{},
                        MemFunBinder(obj, member_function.func));
      else
        return Function(Name{},
                        Description{},
                        Help{},
                        MemFunBinder(obj, member_function.func),
                        member_function.args);
    }
  } // namespace dtl
} // namespace cli::funcs
#endif
