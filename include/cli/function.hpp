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
#include "cli/format.hpp"
#include "cli/parse.hpp"
#include "cli/type_list.hpp"
#include "cli/util.hpp"
#include "cli/validator.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace cli::funcs {

  template<class A>
  concept FuncArg = requires(A &&arg) {
    { typename std::remove_cvref_t<A>::name{} } -> SC;
    { typename std::remove_cvref_t<A>::description{} } -> SC;
    { arg.parse } -> parse::Parser;
    { arg.validate } -> validate::Validator;
    // {
    //   std::remove_cvref_t<A>::default_value
    // } /* -> std::same_as<typename std::remove_cvref_t<A>::type> */;
  };

  inline constexpr struct Deduced {
    constexpr Deduced() = default;
  } deduced{};

  template<SC Name,
           SC Description,
           class T,
           auto DefaultValue,
           parse::Parser Parse,
           validate::Validator Validate>
  struct FunctionArg {
    using char_type = typename Name::char_type;
    using name = Name;
    using description = Description;
    using type = std::remove_cvref_t<decltype(DefaultValue)>;
    using parser = Parse;
    using validator = Validate;

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArg(Name,
                          Description,
                          identity<T>,
                          constant<DefaultValue>,
                          P &&parse,
                          V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArg(
      Name, Description, constant<DefaultValue>, P &&parse, V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}
    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArg(Name, Description, P &&parse, V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

    CLI_NO_UNIQUE_ADDRESS Parse parse{};
    CLI_NO_UNIQUE_ADDRESS Validate validate{};
  };

  template<SC Name,
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
                                          std::remove_cvref_t<Parse>,
                                          std::remove_cvref_t<Validate>>;

  template<SC Name,
           SC Description,
           auto DefaultValue,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArg(Name, Description, constant<DefaultValue>, Parse &&, Validate &&)
    -> FunctionArg<Name,
                   Description,
                   parse::value_type_t<typename Name::char_type, Parse>,
                   DefaultValue,
                   std::remove_cvref_t<Parse>,
                   std::remove_cvref_t<Validate>>;

  template<SC Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArg(Name, Description, Parse &&, Validate &&)
    -> FunctionArg<Name,
                   Description,
                   parse::value_type_t<typename Name::char_type, Parse>,
                   parse::value_type_t<typename Name::char_type, Parse>{},
                   std::remove_cvref_t<Parse>,
                   std::remove_cvref_t<Validate>>;

  template<SC Name,
           SC Description,
           typename T,
           parse::Parser Parse,
           validate::Validator Validate>
  struct FunctionArgWithoutDefault {
    using char_type = typename Name::char_type;
    using name = Name;
    using description = Description;
    using type = T;
    using parser = Parse;
    using validator = Validate;

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArgWithoutDefault(
      Name, Description, identity<T>, P &&parse, V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

    template<parse::ParserOf<T, char_type> P, validate::Validator V>
    constexpr FunctionArgWithoutDefault(Name,
                                        Description,
                                        P &&parse,
                                        V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

    CLI_NO_UNIQUE_ADDRESS Parse parse{};
    CLI_NO_UNIQUE_ADDRESS Validate validate{};
  };

  template<SC Name,
           SC Description,
           typename T,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArgWithoutDefault(
    Name, Description, identity<T>, Parse &&, Validate &&)
    -> FunctionArgWithoutDefault<Name,
                                 Description,
                                 T,
                                 std::remove_cvref_t<Parse>,
                                 std::remove_cvref_t<Validate>>;

  template<SC Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  FunctionArgWithoutDefault(Name, Description, Parse &&, Validate &&)
    -> FunctionArgWithoutDefault<
      Name,
      Description,
      parse::value_type_t<typename Name::char_type, Parse>,
      std::remove_cvref_t<Parse>,
      std::remove_cvref_t<Validate>>;

  template<SC Name, SC Description>
  struct UndeducedArg {
    using char_type = typename Name::char_type;
    using name = Name;
    using description = Description;
    using type = Deduced;
    using parser = parse::DefaultParse<Deduced, char_type>;
    using validator = validate::DefaultValidate<Deduced>;
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
    deduce_arg(const FunctionArgWithoutDefault<N, D, T, P, V> &arg) {
      using args = typename function_traits<F>::arguments;
      using arg_type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
      static_assert(parse::ParserOf<P, T, typename N::char_type>);
      static_assert(validate::ValidatorOf<V, T>);
      static_assert(
        std::same_as<arg_type, T>,
        "the I-th arg's explicitly set type does not match F's I-th argument");
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
    constexpr auto deduce_arg(const FunctionArg<N, D, T, DV, P, V> &arg) {
      using args = typename function_traits<F>::arguments;
      using arg_type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
      static_assert(parse::ParserOf<P, T, typename N::char_type>);
      static_assert(validate::ValidatorOf<V, T>);
      static_assert(std::constructible_from<T, decltype(DV)>);
      static_assert(
        std::same_as<arg_type, T>,
        "the I-th arg's explicitly set type does not match F's I-th argument");
      return arg;
    }

    template<Callable F, std::size_t I, SC N, SC D>
    constexpr auto deduce_arg(const UndeducedArg<N, D> &arg) {
      using args = typename function_traits<F>::arguments;
      using type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
      if constexpr (std::is_same_v<D, string_constant<typename N::char_type>> or
                    std::is_same_v<D, NoDescription<typename N::char_type>>)
        return FunctionArgWithoutDefault{
          N{},
          cli::ctti::name<type>(),
          identity<type>{},
          parse::DefaultParse<type, typename N::char_type>{},
          validate::DefaultValidate<type>{}};
      else
        return FunctionArgWithoutDefault{
          N{},
          D{},
          identity<type>{},
          parse::DefaultParse<type, typename N::char_type>{},
          validate::DefaultValidate<type>{}};
    }

    template<Callable F, class... Args>
    constexpr auto deduce_args(const Args &...args) {
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::tuple(deduce_arg<F, Is>(args)...);
      }(std::make_index_sequence<sizeof...(Args)>());
    }

    template<SC Name,
             SC Description,
             class T,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto pretty_arg_name(
      const FunctionArgWithoutDefault<Name, Description, T, Parse, Validate>
        &) {
      return Name{} + ": "_sc + ctti::name<T>();
    }

    template<SC Name,
             SC Description,
             class T,
             auto DefaultValue,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto pretty_arg_name(
      const FunctionArg<Name, Description, T, DefaultValue, Parse, Validate>
        &) {
      return Name{} + ": "_sc + ctti::name<T>() + "?"_sc;
    }

    template<FuncArg A, FuncArg... As>
    constexpr auto make_pretty_signature_name(const A &arg, const As &...args) {
      if constexpr (sizeof...(As) == 0)
        return pretty_arg_name(arg);
      else
        return pretty_arg_name(arg) + ", "_sc +
               make_pretty_signature_name(args...);
    }

    template<Callable F, FuncArg... Args>
    constexpr auto pretty_signature_name(const Args &...args) {
      return "("_sc + make_pretty_signature_name(args...) + ")->"_sc +
             ctti::name<typename function_traits<F>::return_type>();
    }

    template<Callable F, FuncArg... Args>
    constexpr auto pretty_signature_name(const std::tuple<Args...> &args) {
      return []<std::size_t... Is>(std::index_sequence<Is...>,
                                   const std::tuple<Args...> &args) {
        return "("_sc + make_pretty_signature_name(std::get<Is>(args)...) +
               ")->"_sc +
               ctti::name<typename function_traits<F>::return_type>();
      }(std::make_index_sequence<sizeof...(Args)>(), args);
    }

    template<Callable F>
    constexpr auto pretty_signature_name() {
      return "()->"_sc + ctti::name<typename function_traits<F>::return_type>();
    }

    template<SC Name,
             SC Description,
             class T,
             auto DefaultValue,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto parse_field_from_arg(
      const FunctionArg<Name, Description, T, DefaultValue, Parse, Validate>
        &arg) {
      return parse::Field<typename Name::char_type, Name, DefaultValue, Parse>{
        DefaultValue, arg.parse};
    }

    template<SC Name,
             SC Description,
             typename T,
             parse::Parser Parse,
             validate::Validator Validate>
    constexpr auto parse_field_from_arg(
      const FunctionArgWithoutDefault<Name, Description, T, Parse, Validate>
        &arg) {
      return parse::
        FieldWithOutDefault<typename Name::char_type, Name, T, Parse>{
          T{}, arg.parse};
    }

    template<class... Args>
    constexpr auto parse_field_from_args(const std::tuple<Args...> &args) {
      return [&args]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::tuple(parse_field_from_arg(std::get<Is>(args))...);
      }(std::make_index_sequence<sizeof...(Args)>{});
    }
  } // namespace dtl

  // clang-format off
/**
 * @addtogroup Arguments
 * @ingroup Functions
 * @{
 *
 * Arguments are the elements that describe c++ function arguments.
 * These argument specifications are then used by Functions to parse the
 * input into the specified values and validate them.
 *
 * There are two kinds of function arguments:
 *
 * - required: these parameters must be specified, else it is an error.
 * - optional: these parameters can be left out because they have a default
 * value.
 *
 * Arguments are fully specified by their:
 *
 * - name: the human readable name
 * - description: a string that is used by the help functionality
 * - type: the value type of the argument
 * - parser: used to parse a value of the arguments type
 * - validator: used to validate the parsed value
 * - optionally, a default value
 *
 * cli::funcs::arg is the templated overload set (sounds horrible, but bear with
 * it) to use for creating arguments. There are two main templates forms, one
 * for required and one for optional arguments.
 *
 * The base form for an optional arguments is written below:
 *
 * ```
 *  template <class T, // the arguments type, must be explicitly specified
 *            auto DefaultValue, // the default value, must be explicitly specified
 *            SC Name, // must be specified 
 *            SC Description, // either specified or left out 
 *            parse::Parser Parse, // either specified or deduced
 *            validate::Validator Validate // either specified or deduced
 *            >
 *  constexpr auto arg( Name name, // the name
 *                      Description description, // the description
 *                      Parse &&parse, // the parser
 *                      Validate &&validate // the validator
 *                      );
 * ```
 * 
 * The base form for a required arguments is:
 *
 * ```
 *  template <class T, // the arguments type, must be explicitly specified
 *            SC Name, //must be specified 
 *            SC Description, // either specified or left out 
 *            parse::Parser Parse, // either specified or deduced
 *            validate::Validator Validate // either specified or deduced
 *            >
 *  constexpr auto arg( Name name, // the name
 *                      Description description, // the description
 *                      Parse &&parse, // the parser
 *                      Validate &&validate // the validator
 *                      );
 * ```
 *
 */

/**
 * @addtogroup optional-args Optional Arguments
 * @ingroup Functions
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
 *                                  cli::parse::DefaultParse<double>{}, 
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
           SC Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse,
           validate::ValidatorOf<T> Validate>
  constexpr auto
  arg(Name name, Description description, Parse &&parse, Validate &&validate) {
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
 *                                  cli::parse::DefaultParse<double>{});
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
           SC Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse>
  constexpr auto arg(Name name, Description description, Parse &&parse) {
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
           SC Name,
           SC Description,
           validate::ValidatorOf<T> Validate>
  constexpr auto arg(Name name, Description description, Validate &&validate) {
    (void)name;
    (void)description;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::DefaultParse<T, typename Name::char_type>{},
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
  template<class T, auto DefaultValue, SC Name, SC Description>
  constexpr auto arg(Name name, Description description) {
    (void)name;
    (void)description;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::DefaultParse<T, typename Name::char_type>{},
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
  template<class T, auto DefaultValue, SC Name>
  constexpr auto arg(Name name) {
    (void)name;
    return FunctionArg{Name{},
                       NoDescription<typename Name::char_type>{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::DefaultParse<T, typename Name::char_type>{},
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
   *                                  cli::parse::DefaultParse<int>{},
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
           SC Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  constexpr auto
  arg(Name name, Description description, Parse &&parse, Validate &&validate) {
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
   *                                  cli::parse::DefaultParse<double>{});
   * ```

   * @tparam DefaultValue the default value of this argument
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param parse the parser for the argument
   * @return a FunctionArg
   */
  // clang-format on
  template<auto DefaultValue,
           SC Name,
           SC Description,
           parse::Parser Parse,
           validate::Validator Validate>
  constexpr auto arg(Name name, Description description, Parse &&parse) {
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
           SC Name,
           SC Description,
           validate::Validator Validate>
  constexpr auto arg(Name name, Description description, Validate &&validate) {
    (void)name;
    (void)description;
    using T = validate::value_type_t<Validate>;
    using CharT = typename Name::char_type;
    return FunctionArg{Name{},
                       Description{},
                       constant<DefaultValue>{},
                       parse::DefaultParse<T, CharT>{},
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
  template<auto DefaultValue, SC Name, SC Description>
  constexpr auto arg(Name name, Description description) {
    (void)name;
    (void)description;
    using T = std::remove_cvref_t<decltype(DefaultValue)>;
    return FunctionArg{Name{},
                       Description{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::DefaultParse<T, typename Name::char_type>{},
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
  template<auto DefaultValue, SC Name>
  constexpr auto arg(Name name) {
    (void)name;
    using T = std::remove_cvref_t<decltype(DefaultValue)>;
    return FunctionArg{Name{},
                       NoDescription<typename Name::char_type>{},
                       identity<T>{},
                       constant<DefaultValue>{},
                       parse::DefaultParse<T, typename Name::char_type>{},
                       validate::DefaultValidate<T>{}};
  }
  /**
   * @}
   */

  /**
   * @addtogroup required-args Required Arguments
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
   *                         cli::parse::DefaultParse<int, char>{},
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
           SC Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse,
           validate::ValidatorOf<T> Validate>
  constexpr auto
  arg(Name name, Description description, Parse &&parse, Validate &&validate) {
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
   *                         cli::parse::DefaultParse<int, char>{});
   * ```
   * @tparam T the arguments type
   * @param name the humanreadable name of the argument as a string_constant
   * @param description a string_constant that is used by the help functionality
   * @param parse the parser for the argument
   * @return a FunctionArg
   */
  template<class T,
           SC Name,
           SC Description,
           parse::ParserOf<T, typename Name::char_type> Parse>
  constexpr auto arg(Name name, Description description, Parse &&parse) {
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
  template<class T, SC Name, SC Description, validate::ValidatorOf<T> Validate>
  constexpr auto arg(Name name, Description description, Validate &&validate) {
    (void)name;
    (void)description;
    return FunctionArgWithoutDefault{
      Name{},
      Description{},
      identity<T>{},
      parse::DefaultParse<T, typename Name::char_type>{},
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
  template<class T, SC Name, SC Description>
  constexpr auto arg(Name name, Description description) {
    (void)name;
    (void)description;
    return FunctionArgWithoutDefault{
      Name{},
      Description{},
      identity<T>{},
      parse::DefaultParse<T, typename Name::char_type>{},
      validate::DefaultValidate<T>{}};
  }

  /**
   * @defgroup deduced-args Deduced Arguments
   * Deduced arguments are arguments that have their type deduced. The default
   * parser and validator are always used for these type of arguments. The
   * benefit of deduced arguments is the shorter notation.
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
  template<SC Name, SC Description>
  constexpr auto arg(Name name, Description description) {
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
  template<SC Name>
  constexpr auto arg(Name name) {
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
  constexpr auto operator""_arg() {
    return arg([&]<std::size_t... Is>(std::index_sequence<Is...>) {
      return string_constant<typename decltype(Name)::char_type,
                             Name.s[Is]...>{};
    }(std::make_index_sequence<Name.size()>()));
  }

  /**
   * @}
   * @}
   * @}
   */

  template<SC Name, SC Description, SC Type, Callable F, FuncArg... Args>
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
      FieldGroup<typename Name::char_type, '=', ',', ' ', ' ', Fields...>;
    using Parser = type_list::apply_t<PartialParser,
                                      decltype(dtl::parse_field_from_args(
                                        std::declval<std::tuple<Args...>>()))>;

  public:
    using char_type = typename Base::char_type;
    using Base::description;
    using Base::name;
    using Base::type;
    using sub_command_list = TypeList<>;

    using signature = typename traits::signature_type;

    template<Callable Func, FuncArg... A>
    constexpr Function(Name, Description, Type, Func &&function, A &&...args)
      : func_(std::forward<Func>(function)), args_(std::forward<A>(args)...) {}

    template<Callable Func>
    constexpr Function(
      Name, Description, Type, Func &&function, const std::tuple<Args...> &args)
      : func_(std::forward<Func>(function)), args_(args) {}

    template<Callable Func>
    constexpr Function(
      Name, Description, Type, Func &&function, std::tuple<Args...> &&args)
      : func_(std::forward<Func>(function)), args_(std::move(args)) {}

    Error execute(ExecType type,
                  View<const char_type> args,
                  [[maybe_unused]] View<char_type> &out) {
      using Ret = typename traits::return_type;

      if (type != ExecType::call)
        return Error::invalid_cmd;

      Parser parse{dtl::parse_field_from_args(this->args_)};

      auto res = parse(args);
      if (not res)
        return res.error;

      return [&tuple = res.value, &out, this]<std::size_t... Is>(
               std::index_sequence<Is...>) {
        if (not validate(tuple, std::index_sequence<Is...>{}))
          return Error::invalid_argument;

        if constexpr (std::is_same_v<void, Ret>) {
          static_cast<void>(out);
          func_(std::get<Is>(tuple).value...);
          return Error::none;
        } else {
          auto res = func_(std::get<Is>(tuple).value...);
          format::DefaultFormat<Ret, typename Base::char_type> format;
          auto fmt_result = format(out, res);
          out = out.substr(0, fmt_result.size_written);
          return fmt_result.error;
        }
      }(std::make_index_sequence<sizeof...(Args)>());
      return Error::unimplemented;
    }

    template<std::size_t I, std::size_t... Is>
    static constexpr bool validate(const auto &tuple,
                                   std::index_sequence<I, Is...>) {
      auto valid =
        typename type_list::type_at_t<I, TypeList<Args...>>::validator{}(
          std::get<I>(tuple).value);
      if constexpr (sizeof...(Is) == 0)
        return valid;
      else {
        if (not valid)
          return false;
        return validate(tuple, std::index_sequence<Is...>{});
      }
    }

  private:
    F func_{};
    std::tuple<Args...> args_{};
  };

  template<SC Name, SC Description, SC Type, Callable F>
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

    template<Callable Func>
    constexpr Function(Name, Description, Type, Func &&function)
      : func_(std::forward<Func>(function)) {}

    Error execute(ExecType type,
                  View<const char_type> args,
                  [[maybe_unused]] View<char_type> &out) {
      using Ret = typename traits::return_type;

      if (type != ExecType::call)
        return Error::invalid_cmd;

      if constexpr (std::is_same_v<void, Ret>) {
        func_();
        return Error::none;
      } else {
        auto res = format_(out, func_());
        return res.error;
      }
      return Error::unimplemented;
    }

  private:
    F func_{};
  };

  // template <SC Name, SC Description, SC Help, Callable F>
  // Function(Name, Description, Help, F &&)
  //     -> Function<Name, Description, Help, std::remove_cvref_t<F>>;

  template<SC Name, SC Description, SC Type, Callable F, FuncArg... Args>
  Function(Name, Description, Type, F &&, Args &&...)
    -> Function<Name,
                Description,
                Type,
                std::remove_cvref_t<F>,
                std::remove_cvref_t<Args>...>;

  template<SC Name, SC Description, SC Type, Callable F, FuncArg... Args>
  Function(Name, Description, Type, F &&, const std::tuple<Args...> &)
    -> Function<Name, Description, Type, std::remove_cvref_t<F>, Args...>;

  template<SC Name, SC Description, SC Type, Callable F, FuncArg... Args>
  Function(Name, Description, Type, F &&, std::tuple<Args...> &&)
    -> Function<Name, Description, Type, std::remove_cvref_t<F>, Args...>;
  /**
   * @addtogroup Functions
   * @{
   */

  // clang-format off
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
 * @param args the functions arguments. See cli::funcs::arg
 * @return
 */
  // clang-format on
  template<SC Name, SC Description, Callable F, class... Args>
  constexpr auto
  func(Name name, Description description, F &&f, Args &&...args) {
    (void)name;
    (void)description;
    static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
        sizeof...(Args),
      "All arguments of f must be named");
    if constexpr (sizeof...(Args) > 0) {
      auto deduced = dtl::deduce_args<F>(std::forward<Args>(args)...);
      return Function{Name{},
                      Description{},
                      dtl::pretty_signature_name<F>(deduced),
                      std::forward<F>(f),
                      deduced};
    } else {
      return Function{Name{},
                      Description{},
                      dtl::pretty_signature_name<F>(),
                      std::forward<F>(f)};
    }
  }

  // clang-format off
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
 * @param args the functions arguments. See cli::funcs::arg
 * @return
 */
  // clang-format on
  template<SC Name, Callable F, class... Args>
  constexpr auto func(Name name, F &&f, Args &&...args) {
    (void)name;
    static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
        sizeof...(Args),
      "All arguments of f must be named");
    if constexpr (sizeof...(Args) > 0) {
      auto deduced = dtl::deduce_args<F>(std::forward<Args>(args)...);
      return Function{Name{},
                      NoDescription<typename Name::char_type>{},
                      dtl::pretty_signature_name<F>(deduced),
                      std::forward<F>(f),
                      deduced};
    } else {
      return Function{Name{},
                      NoDescription<typename Name::char_type>{},
                      dtl::pretty_signature_name<F>(),
                      std::forward<F>(f)};
    }
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
   * constexpr auto F_func = cli::funcs::func(F{}, "a description"_sc, "i"_arg);
   * ```
   * @param f the callable. Can't be a free function or function pointer.
   * @param description the callable's description. Must be a
   * cli::string_constant.
   * @param args the callable's arguments. See cli::funcs::arg.
   */
  template<Callable F, SC Description, class... Args>
    requires(not std::is_pointer_v<std::decay_t<F>>)
  constexpr auto func(F &&f, Description description, Args &&...args) {
    (void)description;
    static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
        sizeof...(Args),
      "All arguments of f must be named");
    if constexpr (sizeof...(Args) > 0) {
      auto deduced = dtl::deduce_args<F>(std::forward<Args>(args)...);
      return Function{ctti::name<std::remove_cvref_t<F>>(),
                      Description{},
                      dtl::pretty_signature_name<F>(deduced),
                      std::forward<F>(f),
                      deduced};
    } else {
      return Function{ctti::name<std::remove_cvref_t<F>>(),
                      Description{},
                      dtl::pretty_signature_name<F>(),
                      std::forward<F>(f)};
    }
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
   * @param args the callable's arguments. See cli::funcs::arg.
   */
  template<Callable F, class... Args>
    requires(not std::is_pointer_v<std::decay_t<F>>)
  constexpr auto func(F &&f, Args &&...args) {
    // TODO: check that each args char_type if char, or extract the args
    // char_type
    static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
        sizeof...(Args),
      "All arguments of f must be named");
    if constexpr (sizeof...(Args) > 0) {
      auto deduced = dtl::deduce_args<F>(std::forward<Args>(args)...);
      return Function{ctti::name<std::remove_cvref_t<F>>(),
                      NoDescription<char>{},
                      dtl::pretty_signature_name<F>(deduced),
                      std::forward<F>(f),
                      deduced};
    } else {
      return Function{ctti::name<std::remove_cvref_t<F>>(),
                      NoDescription<char>{},
                      dtl::pretty_signature_name<F>(),
                      std::forward<F>(f)};
    }
  }

  template<SC Name,
           SC Description,
           class T,
           class MemberFunctionPointer,
           class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  constexpr auto func(Name name,
                      Description description,
                      T &t,
                      MemberFunctionPointer mem_fun,
                      Args &&...args) {
    (void)name;
    (void)description;
    static_assert(
      type_list::list_size_v<
        typename function_traits<MemberFunctionPointer>::arguments> ==
        sizeof...(Args),
      "All arguments of mem_fun must be named");

    using Binder = MemFunBinder<T, MemberFunctionPointer>;
    if constexpr (sizeof...(Args) > 0) {
      auto deduced =
        dtl::deduce_args<MemberFunctionPointer>(std::forward<Args>(args)...);
      return Function{
        Name{},
        Description{},
        dtl::pretty_signature_name<MemberFunctionPointer>(deduced),
        Binder{t, mem_fun},
        deduced
      };
    } else {
      return Function{
        Name{},
        Description{},
        dtl::pretty_signature_name<MemberFunctionPointer>(),
        Binder{t, mem_fun}
      };
    }
  }

  template<SC Name,
           SC Description,
           class T,
           class MemberFunctionPointer,
           class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  constexpr auto func(Name name,
                      Description description,
                      const T &t,
                      MemberFunctionPointer mem_fun,
                      Args &&...args) {
    (void)name;
    (void)description;
    static_assert(
      type_list::list_size_v<
        typename function_traits<MemberFunctionPointer>::arguments> ==
        sizeof...(Args),
      "All arguments of mem_fun must be named");

    using Binder = MemFunBinder<const T, MemberFunctionPointer>;
    if constexpr (sizeof...(Args) > 0) {
      auto deduced =
        dtl::deduce_args<MemberFunctionPointer>(std::forward<Args>(args)...);
      return Function{
        Name{},
        Description{},
        dtl::pretty_signature_name<MemberFunctionPointer>(deduced),
        Binder{t, mem_fun},
        deduced
      };
    } else {
      return Function{
        Name{},
        Description{},
        dtl::pretty_signature_name<MemberFunctionPointer>(),
        Binder{t, mem_fun}
      };
    }
  }

  template<SC Name, class T, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  constexpr auto
  func(Name name, T &t, MemberFunctionPointer mem_fun, Args &&...args) {
    return func(name,
                NoDescription<typename Name::char_type>{},
                t,
                mem_fun,
                std::forward<Args>(args)...);
  }

  template<SC Name, class T, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  constexpr auto
  func(Name name, const T &t, MemberFunctionPointer mem_fun, Args &&...args) {
    return func(name,
                NoDescription<typename Name::char_type>{},
                t,
                mem_fun,
                std::forward<Args>(args)...);
  }

  /**
   * @}
   */

  template<SC Name, SC Description, SC Help, class Function, class... Args>
  struct MemberFunction {
    using arguments = TypeList<Args...>;
    static_assert(std::is_member_function_pointer_v<Function>,
                  "A MemberFunctions Function template argument must be a "
                  "pointer to member function");
    Function f;
    std::tuple<Args...> args;

    constexpr MemberFunction(Name,
                             Description,
                             Help,
                             Function mem_fun_ptr,
                             const Args &...args) noexcept
      : f(mem_fun_ptr), args(args...) {}

    constexpr MemberFunction(Name,
                             Description,
                             Help,
                             Function mem_fun_ptr,
                             const std::tuple<Args...> &args) noexcept
      : f(mem_fun_ptr), args(args) {}
  };

  template<SC Name, SC Description, SC Help, class Function>
  struct MemberFunction<Name, Description, Help, Function> {
    using arguments = TypeList<>;
    static_assert(std::is_member_function_pointer_v<Function>,
                  "A MemberFunctions Function template argument must be a "
                  "pointer to member function");
    Function f;

    constexpr MemberFunction(Name,
                             Description,
                             Help,
                             Function mem_fun_ptr) noexcept
      : f(mem_fun_ptr) {}
  };

  template<SC Name, SC Description, SC Help, class Function, class... Args>
  MemberFunction(Name &&, Description &&, Help &&, Function &&, Args &&...)
    -> MemberFunction<std::remove_cvref_t<Name>,
                      std::remove_cvref_t<Description>,
                      std::remove_cvref_t<Help>,
                      std::remove_cvref_t<Function>,
                      std::remove_cvref_t<Args>...>;
  template<SC Name, SC Description, SC Help, class Function, FuncArg... Args>
  MemberFunction(
    Name &&, Description &&, Help &&, Function &&, const std::tuple<Args...> &)
    -> MemberFunction<std::remove_cvref_t<Name>,
                      std::remove_cvref_t<Description>,
                      std::remove_cvref_t<Help>,
                      std::remove_cvref_t<Function>,
                      std::remove_cvref_t<Args>...>;

  template<SC Name, SC Description, SC Help, class Function>
  MemberFunction(Name &&, Description &&, Help &&, Function &&)
    -> MemberFunction<std::remove_cvref_t<Name>,
                      std::remove_cvref_t<Description>,
                      std::remove_cvref_t<Help>,
                      std::remove_cvref_t<Function>>;

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
   * @{
   */

  // clang-format off
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
 *                  cli::param("foo"_sc, foo, // <- foo must be direct parent
 *                                            // of bar and baz
 *                             cli::mem_fun("bar"_sc, "bars the foo"_sc, &Foo::bar, "param"_arg),
 *                             cli::mem_fun("baz"_sc, "bazzes"_sc, &Foo::baz),
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
 * @param args the arguments. See cli::funcs::arg.
 * @return a partial Command
 */
  // clang-format on
  template<SC Name, SC Description, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  constexpr auto mem_fun(Name name,
                         Description description,
                         MemberFunctionPointer mem_fun,
                         Args &&...args) {
    (void)name;
    (void)description;
    if constexpr (sizeof...(Args) > 0) {
      constexpr auto deduced_args =
        dtl::deduce_args<decltype(mem_fun)>(std::forward<Args>(args)...);
      return MemberFunction{
        Name{},
        Description{},
        dtl::pretty_signature_name<decltype(mem_fun)>(deduced_args),
        mem_fun,
        deduced_args};
    } else {
      return MemberFunction{Name{},
                            Description{},
                            dtl::pretty_signature_name<decltype(mem_fun)>(),
                            mem_fun};
    }
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
   *                             cli::mem_fun("bar"_sc, &Foo::bar, "param"_arg),
   *                             cli::mem_fun("baz"_sc, &Foo::baz),
   *                             ... ),
   *                  ...);
   * ```
   *
   * This is used to avoid repeating the object, i.e. ``foo``, for every
   * member function added, as would be the case when using cli::func().
   *
   * @param name the name of the member function
   * @param mem_fun pointer to the member function
   * @param args the arguments. See cli::funcs::arg.
   * @return a partial Command
   */
  template<SC Name, class MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<MemberFunctionPointer>
  constexpr auto
  mem_fun(Name name, MemberFunctionPointer mem_fun, Args &&...args) {
    (void)name;
    if constexpr (sizeof...(Args) > 0) {
      constexpr auto deduced_args =
        dtl::deduce_args<decltype(mem_fun)>(std::forward<Args>(args)...);
      return MemberFunction{
        Name{},
        NoDescription<typename Name::char_type>{},
        dtl::pretty_signature_name<decltype(mem_fun)>(deduced_args),
        mem_fun,
        deduced_args};
    } else {
      return MemberFunction{Name{},
                            NoDescription<typename Name::char_type>{},
                            dtl::pretty_signature_name<decltype(mem_fun)>(),
                            mem_fun};
    }
  }

  // clang-format off
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
 *               cli::mem_fun<&Foo::bar>("bars the foo"_sc, param"_arg), // name is deduced to "bar"
 *               cli::mem_fun<&Foo::baz>("bazzes"_sc), // name is deduced to "baz"
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
 * @param args the arguments. See cli::funcs::arg.
 * @return a partial Command
 */
  // clang-format on
  template<auto MemberFunctionPointer, SC Description, class... Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunctionPointer)>
  constexpr auto mem_fun(Description description, Args &&...args) {
    if constexpr (sizeof...(Args) > 0) {
      constexpr auto deduced_args =
        dtl::deduce_args<decltype(MemberFunctionPointer)>(
          std::forward<Args>(args)...);
      return MemberFunction{
        ctti::value_name<MemberFunctionPointer>(),
        Description{},
        dtl::pretty_signature_name<decltype(MemberFunctionPointer)>(
          deduced_args),
        MemberFunctionPointer,
        deduced_args};
    } else {
      return MemberFunction{
        ctti::value_name<MemberFunctionPointer>(),
        Description{},
        dtl::pretty_signature_name<decltype(MemberFunctionPointer)>(),
        MemberFunctionPointer};
    }
  }
  // clang-format off
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
 *               cli::mem_fun<&Foo::bar>("param"_arg), // name is deduced to "bar"
 *               cli::mem_fun<&Foo::baz>(), // name is deduced to "baz"
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
 * @param args the arguments. See cli::funcs::arg.
 */
  // clang-format on
  template<auto MemberFunctionPointer, class... Args>
    requires std::is_member_function_pointer_v<decltype(MemberFunctionPointer)>
  constexpr auto mem_fun(Args &&...args) {
    if constexpr (sizeof...(Args) > 0) {
      constexpr auto deduced_args =
        dtl::deduce_args<decltype(MemberFunctionPointer)>(
          std::forward<Args>(args)...);
      return MemberFunction{
        ctti::value_name<MemberFunctionPointer>(),
        NoDescription<char>{},
        dtl::pretty_signature_name<decltype(MemberFunctionPointer)>(
          deduced_args),
        MemberFunctionPointer,
        deduced_args};
    } else {
      return MemberFunction{
        ctti::value_name<MemberFunctionPointer>(),
        NoDescription<char>{},
        dtl::pretty_signature_name<decltype(MemberFunctionPointer)>(),
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
    template<class T, SC Name, SC Description, SC Help, class F, class... Args>
    constexpr auto
    to_cmd(T &obj,
           const MemberFunction<Name, Description, Help, F, Args...>
             &member_function) {
      if constexpr (sizeof...(Args) == 0)
        return Function(
          Name{}, Description{}, Help{}, MemFunBinder(obj, member_function.f));
      else
        return Function(Name{},
                        Description{},
                        Help{},
                        MemFunBinder(obj, member_function.f),
                        member_function.args);
    }

    /**
     * create a Function from an object referenece and a MemberFunction
     */
    template<class T, SC Name, SC Description, SC Help, class F, class... Args>
    constexpr auto
    to_cmd(const T &obj,
           const MemberFunction<Name, Description, Help, F, Args...>
             &member_function) {
      if constexpr (sizeof...(Args) == 0)
        return Function(
          Name{}, Description{}, Help{}, MemFunBinder(obj, member_function.f));
      else
        return Function(Name{},
                        Description{},
                        Help{},
                        MemFunBinder(obj, member_function.f),
                        member_function.args);
    }
  } // namespace dtl
} // namespace cli::funcs
#endif
