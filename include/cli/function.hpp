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

template <class A>
concept FuncArg = requires(A &&arg) {
  { typename std::remove_cvref_t<A>::name{} } -> SC;
  { typename std::remove_cvref_t<A>::description{} } -> SC;
  { arg.parse } -> parse::Parser<typename A::char_type>;
  { arg.validate } -> validate::Validator;
  // {
  //   std::remove_cvref_t<A>::default_value
  // } /* -> std::same_as<typename std::remove_cvref_t<A>::type> */;
};

inline constexpr struct Deduced {
  constexpr Deduced() = default;
} deduced{};

template <SC ArgName, SC Description, class T, auto DefaultValue,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
struct FunctionArg {
  using char_type = typename ArgName::char_type;
  using name = ArgName;
  using description = Description;
  using type = std::remove_cvref_t<decltype(DefaultValue)>;
  using parser = Parse;
  using validator = Validate;

  template <parse::Parser<char_type> P, validate::Validator V>
  constexpr FunctionArg(ArgName, Description, identity<T>,
                        constant<DefaultValue>, P &&parse, V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

  template <parse::Parser<char_type> P, validate::Validator V>
  constexpr FunctionArg(ArgName, Description, constant<DefaultValue>, P &&parse,
                        V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}
  template <parse::Parser<char_type> P, validate::Validator V>
  constexpr FunctionArg(ArgName, Description, P &&parse, V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

  CLI_NO_UNIQUE_ADDRESS Parse parse{};
  CLI_NO_UNIQUE_ADDRESS Validate validate{};
};

template <SC ArgName, SC Description, class T, auto DefaultValue,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
FunctionArg(ArgName, Description, identity<T>, constant<DefaultValue>, Parse &&,
            Validate &&)
    -> FunctionArg<ArgName, Description, T, DefaultValue,
                   std::remove_cvref_t<Parse>, std::remove_cvref_t<Validate>>;

template <SC ArgName, SC Description, auto DefaultValue,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
FunctionArg(ArgName, Description, constant<DefaultValue>, Parse &&, Validate &&)
    -> FunctionArg<ArgName, Description,
                   parse::value_type_t<typename ArgName::char_type, Parse>,
                   DefaultValue, std::remove_cvref_t<Parse>,
                   std::remove_cvref_t<Validate>>;

template <SC ArgName, SC Description,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
FunctionArg(ArgName, Description, Parse &&, Validate &&)
    -> FunctionArg<ArgName, Description,
                   parse::value_type_t<typename ArgName::char_type, Parse>,
                   parse::value_type_t<typename ArgName::char_type, Parse>{},
                   std::remove_cvref_t<Parse>, std::remove_cvref_t<Validate>>;

template <SC ArgName, SC Description, typename T,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
struct FunctionArgWithoutDefault {
  using char_type = typename ArgName::char_type;
  using name = ArgName;
  using description = Description;
  using type = T;
  using parser = Parse;
  using validator = Validate;

  template <parse::Parser<char_type> P, validate::Validator V>
  constexpr FunctionArgWithoutDefault(ArgName, Description, identity<T>,
                                      P &&parse, V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

  template <parse::Parser<char_type> P, validate::Validator V>
  constexpr FunctionArgWithoutDefault(ArgName, Description, P &&parse,
                                      V &&validate)
      : parse(std::forward<P>(parse)), validate(std::forward<V>(validate)) {}

  CLI_NO_UNIQUE_ADDRESS Parse parse{};
  CLI_NO_UNIQUE_ADDRESS Validate validate{};
};

template <SC ArgName, SC Description, typename T,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
FunctionArgWithoutDefault(ArgName, Description, identity<T>, Parse &&,
                          Validate &&)
    -> FunctionArgWithoutDefault<ArgName, Description, T,
                                 std::remove_cvref_t<Parse>,
                                 std::remove_cvref_t<Validate>>;

template <SC ArgName, SC Description,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
FunctionArgWithoutDefault(ArgName, Description, Parse &&, Validate &&)
    -> FunctionArgWithoutDefault<
        ArgName, Description,
        parse::value_type_t<typename ArgName::char_type, Parse>,
        std::remove_cvref_t<Parse>, std::remove_cvref_t<Validate>>;

template <SC ArgName, SC Description> struct UndeducedArg {
  using char_type = typename ArgName::char_type;
  using name = ArgName;
  using description = Description;
  using type = Deduced;
  using parser = parse::DefaultParse<Deduced, char_type>;
  using validator = validate::DefaultValidate<Deduced>;
};

namespace dtl {
template <Callable F, std::size_t I, SC N, SC D, class T,
          parse::Parser<typename N::char_type> P, validate::Validator V>
constexpr auto deduce_arg(const FunctionArgWithoutDefault<N, D, T, P, V> &arg) {
  using args = typename function_traits<F>::arguments;
  using arg_type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
  static_assert(parse::ParserOf<P, T, typename N::char_type>);
  static_assert(validate::ValidatorOf<V, T>);
  static_assert(
      std::same_as<arg_type, T>,
      "the I-th arg's explicitly set type does not match F's I-th argument");
  return arg;
}

template <Callable F, std::size_t I, SC N, SC D, class T, auto DV,
          parse::Parser<typename N::char_type> P, validate::Validator V>
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

template <Callable F, std::size_t I, SC N, SC D>
constexpr auto deduce_arg(const UndeducedArg<N, D> &arg) {
  using args = typename function_traits<F>::arguments;
  using type = std::remove_cvref_t<type_list::type_at_t<I, args>>;
  if constexpr (std::is_same_v<D, string_constant<typename N::char_type>> or
                std::is_same_v<D, NoDescription<typename N::char_type>>)
    return FunctionArgWithoutDefault{
        N{}, cli::ctti::name<type>(), identity<type>{},
        parse::DefaultParse<type, typename N::char_type>{},
        validate::DefaultValidate<type>{}};
  else
    return FunctionArgWithoutDefault{
        N{}, D{}, identity<type>{},
        parse::DefaultParse<type, typename N::char_type>{},
        validate::DefaultValidate<type>{}};
}

template <Callable F, class... Args>
constexpr auto deduce_args(const Args &...args) {
  return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    return std::tuple(deduce_arg<F, Is>(args)...);
  }(std::make_index_sequence<sizeof...(Args)>());
}

template <SC ArgName, SC Description, class T,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
constexpr auto
pretty_arg_name(const FunctionArgWithoutDefault<ArgName, Description, T, Parse,
                                                Validate> &) {
  return ArgName{} + ": "_sc + ctti::name<T>();
}

template <SC ArgName, SC Description, class T, auto DefaultValue,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
constexpr auto
pretty_arg_name(const FunctionArg<ArgName, Description, T, DefaultValue, Parse,
                                  Validate> &) {
  return ArgName{} + ": "_sc + ctti::name<T>() + "?"_sc;
}

template <FuncArg A, FuncArg... As>
constexpr auto make_pretty_signature_name(const A &arg, const As &...args) {
  if constexpr (sizeof...(As) == 0)
    return pretty_arg_name(arg);
  else
    return pretty_arg_name(arg) + ", "_sc + make_pretty_signature_name(args...);
}

template <Callable F, FuncArg... Args>
constexpr auto pretty_signature_name(const Args &...args) {
  return "("_sc + make_pretty_signature_name(args...) + ")->"_sc +
         ctti::name<typename function_traits<F>::return_type>();
}

template <Callable F, FuncArg... Args>
constexpr auto pretty_signature_name(const std::tuple<Args...> &args) {
  return []<std::size_t... Is>(std::index_sequence<Is...>,
                               const std::tuple<Args...> &args) {
    return "("_sc + make_pretty_signature_name(std::get<Is>(args)...) +
           ")->"_sc + ctti::name<typename function_traits<F>::return_type>();
  }(std::make_index_sequence<sizeof...(Args)>(), args);
}

template <Callable F> constexpr auto pretty_signature_name() {
  return "()->"_sc + ctti::name<typename function_traits<F>::return_type>();
}

template <SC ArgName, SC Description, class T, auto DefaultValue,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
constexpr auto
parse_field_from_arg(const FunctionArg<ArgName, Description, T, DefaultValue,
                                       Parse, Validate> &arg) {
  return parse::Field<typename ArgName::char_type, ArgName, DefaultValue,
                      Parse>{DefaultValue, arg.parse};
}

template <SC ArgName, SC Description, typename T,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
constexpr auto
parse_field_from_arg(const FunctionArgWithoutDefault<ArgName, Description, T,
                                                     Parse, Validate> &arg) {
  return parse::FieldWithOutDefault<typename ArgName::char_type, ArgName, T,
                                    Parse>{T{}, arg.parse};
}

template <class... Args>
constexpr auto parse_field_from_args(const std::tuple<Args...> &args) {
  return [&args]<std::size_t... Is>(std::index_sequence<Is...>) {
    return std::tuple(parse_field_from_arg(std::get<Is>(args))...);
  }(std::make_index_sequence<sizeof...(Args)>{});
}
} // namespace dtl

// clang-format off
/**
 * @addtogroup Arguments
 * @{
 *
 * Arguments are the elements that describe c++ function arguments.
 * These argument specifications are then used by Functions to parse the char
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
 *            SC ArgName, // either specified or deduced 
 *            SC Description, // either specified or left out 
 *            parse::Parser Parse, // either specified or deduced
 *            validate::Validator Validate // either specified or deduced
 *            >
 *  constexpr auto arg( ArgName name, // the name
 *                      Description description, // the description
 *                      Parse &&parse, // the parser
 *                      Validate &&validate // the validator
 *                      )
 * ```
 * 
 *
 */

/**
 * @addtogroup optional-args Optional Arguments
 * @{
 */

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
template <auto DefaultValue, SC ArgName, SC Description,
          parse::Parser<typename ArgName::char_type> Parse,
          validate::Validator Validate>
constexpr auto arg(ArgName name, Description description, Parse &&parse,
                   Validate &&validate) {
  (void)name;
  (void)description;
  return FunctionArg{ArgName{}, Description{}, constant<DefaultValue>{},
                     std::forward<Parse>(parse),
                     std::forward<Validate>(validate)};
}

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
template <class T, auto DefaultValue, SC ArgName, SC Description,
          parse::ParserOf<T, typename ArgName::char_type> Parse,
          validate::ValidatorOf<T> Validate>
constexpr auto arg(ArgName name, Description description, Parse &&parse,
                   Validate &&validate) {
  (void)name;
  (void)description;
  return FunctionArg{ArgName{},
                     Description{},
                     identity<T>{},
                     constant<DefaultValue>{},
                     std::forward<Parse>(parse),
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
template <class T, auto DefaultValue, SC ArgName, SC Description>
constexpr auto arg(ArgName name, Description description) {
  (void)name;
  (void)description;
  return FunctionArg{ArgName{},
                     Description{},
                     identity<T>{},
                     constant<DefaultValue>{},
                     parse::DefaultParse<T, typename ArgName::char_type>{},
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
template <class T, auto DefaultValue, SC ArgName>
constexpr auto arg(ArgName name) {
  (void)name;
  return FunctionArg{ArgName{},
                     NoDescription<typename ArgName::char_type>{},
                     identity<T>{},
                     constant<DefaultValue>{},
                     parse::DefaultParse<T, typename ArgName::char_type>{},
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
template <class T, SC ArgName, SC Description,
          parse::ParserOf<T, typename ArgName::char_type> Parse,
          validate::ValidatorOf<T> Validate>
constexpr auto arg(ArgName name, Description description, Parse &&parse,
                   Validate &&validate) {
  (void)name;
  (void)description;
  return FunctionArgWithoutDefault{ArgName{}, Description{}, identity<T>{},
                                   std::forward<Parse>(parse),
                                   std::forward<Validate>(validate)};
}

/**
 * creates a required argument. Its type, parser, and validator will be
 * deduced.
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
template <SC ArgName, SC Description>
constexpr auto arg(ArgName name, Description description) {
  (void)name;
  (void)description;
  return UndeducedArg<ArgName, Description>{};
}

/**
 * creates a required argument. Its type, parser, and validator will be
 * deduced.
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
template <SC ArgName> constexpr auto arg(ArgName name) {
  (void)name;
  return UndeducedArg<ArgName, NoDescription<typename ArgName::char_type>>{};
}

/**
 * the literal operator for arguments.
 *
 * This is equivalent to ``arg(Name)``
 *
 * @return arg(Name)
 */
template <cli::StringLiteral Name> constexpr auto operator""_arg() {
  return arg([&]<std::size_t... Is>(std::index_sequence<Is...>) {
    return string_constant<typename decltype(Name)::char_type, Name.s[Is]...>{};
  }(std::make_index_sequence<Name.size()>()));
}

/**
 * @}
 */

/**
 * @}
 */

template <SC FuncName, SC Description, SC Type, Callable F, FuncArg... Args>
class Function
    : public CommandBase<Function<FuncName, Description, Type, F, Args...>,
                         FuncName, Description, Type> {
  using Base = CommandBase<Function<FuncName, Description, Type, F, Args...>,
                           FuncName, Description, Type>;
  using traits = function_traits<F>;
  using arguments = typename traits::arguments;

  template <class... Fields>
  using PartialParser = parse::FieldGroup<typename FuncName::char_type, '=',
                                          ',', ' ', ' ', Fields...>;
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

  template <Callable Func, FuncArg... A>
  constexpr Function(FuncName, Description, Type, Func &&function, A &&...args)
      : func_(std::forward<Func>(function)), args_(std::forward<A>(args)...) {}

  template <Callable Func>
  constexpr Function(FuncName, Description, Type, Func &&function,
                     const std::tuple<Args...> &args)
      : func_(std::forward<Func>(function)), args_(args) {}

  template <Callable Func>
  constexpr Function(FuncName, Description, Type, Func &&function,
                     std::tuple<Args...> &&args)
      : func_(std::forward<Func>(function)), args_(std::move(args)) {}

  Error execute(ExecType type, View<const char_type> args,
                [[maybe_unused]] View<char_type> &out) {
    using Ret = typename traits::return_type;

    if (type != ExecType::call)
      return Error::invalid_cmd;

    Parser parse{dtl::parse_field_from_args(this->args_)};

    auto res = parse(args);
    if (not res)
      return res.error;

    return [&tuple = res.value, &out,
            this]<std::size_t... Is>(std::index_sequence<Is...>) {
      const Error err = validate(tuple, std::index_sequence<Is...>{});
      if (err != Error::none)
        return err;

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

  template <std::size_t I, std::size_t... Is>
  static constexpr cli::Error validate(const auto &tuple,
                                       std::index_sequence<I, Is...>) {
    auto err = typename type_list::type_at_t<I, TypeList<Args...>>::validator{}(
        std::get<I>(tuple).value);
    if constexpr (sizeof...(Is) == 0)
      return err;
    else {
      if (err != Error::none)
        return err;
      return validate(tuple, std::index_sequence<Is...>{});
    }
  }
  CharView get_help() {}

private:
  F func_{};
  std::tuple<Args...> args_;
};

template <SC FuncName, SC Description, SC Type, Callable F>
class Function<FuncName, Description, Type, F>
    : public CommandBase<Function<FuncName, Description, Type, F>, FuncName,
                         Description, Type> {
  using Base = CommandBase<Function<FuncName, Description, Type, F>, FuncName,
                           Description, Type>;
  using traits = function_traits<F>;
  using arguments = typename traits::arguments;

public:
  using char_type = typename Base::char_type;
  using Base::description;
  using Base::name;
  using Base::type;
  using sub_command_list = TypeList<>;

  using signature = typename traits::signature_type;

  template <Callable Func>
  constexpr Function(FuncName, Description, Type, Func &&function)
      : func_(std::forward<Func>(function)) {}

  Error execute(ExecType type, View<const char_type> args,
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
  CharView get_help() {}

private:
  F func_{};
};

// template <SC FuncName, SC Description, SC Help, Callable F>
// Function(FuncName, Description, Help, F &&)
//     -> Function<FuncName, Description, Help, std::remove_cvref_t<F>>;

template <SC FuncName, SC Description, SC Type, Callable F, FuncArg... Args>
Function(FuncName, Description, Type, F &&, Args &&...)
    -> Function<FuncName, Description, Type, std::remove_cvref_t<F>,
                std::remove_cvref_t<Args>...>;

template <SC FuncName, SC Description, SC Type, Callable F, FuncArg... Args>
Function(FuncName, Description, Type, F &&, const std::tuple<Args...> &)
    -> Function<FuncName, Description, Type, std::remove_cvref_t<F>, Args...>;

template <SC FuncName, SC Description, SC Type, Callable F, FuncArg... Args>
Function(FuncName, Description, Type, F &&, std::tuple<Args...> &&)
    -> Function<FuncName, Description, Type, std::remove_cvref_t<F>, Args...>;
/**
 * @addtogroup Functions
 * @{
 */

/**
 * @brief
 *
 * @param f
 * @param args
 * @return
 */
template <SC FuncName, SC Description, SC Help, Callable F, class... Args>
constexpr auto func(FuncName, Description, Help, F &&f, Args &&...args) {
  return Function<FuncName, Description, Help, std::decay_t<F>,
                  std::decay_t<Args>...>{
      FuncName{}, Description{}, Help{}, std::forward<F>(f),
      dtl::deduce_args<F>(std::forward<Args>(args)...)};
}

template <SC FuncName, SC Description, Callable F, class... Args>
constexpr auto func(FuncName, Description, F &&f, Args &&...args) {
  static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
          sizeof...(Args),
      "All arguments of F must be named");
  if constexpr (sizeof...(Args) > 0) {
    auto deduced = dtl::deduce_args<F>(std::forward<Args>(args)...);
    return Function{FuncName{}, Description{},
                    dtl::pretty_signature_name<F>(deduced), std::forward<F>(f),
                    deduced};
  } else {
    return Function{FuncName{}, Description{}, dtl::pretty_signature_name<F>(),
                    std::forward<F>(f)};
  }
}

template <SC FuncName, Callable F, class... Args>
constexpr auto func(FuncName, F &&f, Args &&...args) {
  static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
          sizeof...(Args),
      "All arguments of F must be named");
  if constexpr (sizeof...(Args) > 0) {
    auto deduced = dtl::deduce_args<F>(std::forward<Args>(args)...);
    return Function{FuncName{}, NoDescription<typename FuncName::char_type>{},
                    dtl::pretty_signature_name<F>(deduced), std::forward<F>(f),
                    deduced};
  } else {
    return Function{FuncName{}, NoDescription<typename FuncName::char_type>{},
                    dtl::pretty_signature_name<F>(), std::forward<F>(f)};
  }
}

template <Callable F, class... Args>
  requires(not std::is_pointer_v<std::decay_t<F>>)
constexpr auto func(F &&f, Args &&...args) {
  static_assert(
      type_list::list_size_v<typename function_traits<F>::arguments> ==
          sizeof...(Args),
      "All arguments of F must be named");
  if constexpr (sizeof...(Args) > 0) {
    auto deduced = dtl::deduce_args<F>(std::forward<Args>(args)...);
    return Function{
        to_lower(ctti::name<std::remove_cvref_t<F>>()), NoDescription<char>{},
        dtl::pretty_signature_name<F>(deduced), std::forward<F>(f), deduced};
  } else {
    return Function{to_lower(ctti::name<std::remove_cvref_t<F>>()),
                    NoDescription<char>{}, dtl::pretty_signature_name<F>(),
                    std::forward<F>(f)};
  }
}

template <SC FuncName, SC Description, class T, class MemberFunctionPointer,
          class... Args>
  requires std::is_member_function_pointer_v<MemberFunctionPointer>
constexpr auto func(FuncName name, Description description, T &t,
                    MemberFunctionPointer mem_fun, Args &&...args) {
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
    return Function{FuncName{}, Description{},
                    dtl::pretty_signature_name<MemberFunctionPointer>(deduced),
                    Binder{t, mem_fun}, deduced};
  } else {
    return Function{FuncName{}, Description{},
                    dtl::pretty_signature_name<MemberFunctionPointer>(),
                    Binder{t, mem_fun}};
  }
}

template <SC FuncName, SC Description, class T, class MemberFunctionPointer,
          class... Args>
  requires std::is_member_function_pointer_v<MemberFunctionPointer>
constexpr auto func(FuncName name, Description description, const T &t,
                    MemberFunctionPointer mem_fun, Args &&...args) {
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
    return Function{FuncName{}, Description{},
                    dtl::pretty_signature_name<MemberFunctionPointer>(deduced),
                    Binder{t, mem_fun}, deduced};
  } else {
    return Function{FuncName{}, Description{},
                    dtl::pretty_signature_name<MemberFunctionPointer>(),
                    Binder{t, mem_fun}};
  }
}

template <SC FuncName, class T, class MemberFunctionPointer, class... Args>
  requires std::is_member_function_pointer_v<MemberFunctionPointer>
constexpr auto func(FuncName name, T &t, MemberFunctionPointer mem_fun,
                    Args &&...args) {
  return func(name, NoDescription<typename FuncName::char_type>{}, t, mem_fun,
              std::forward<Args>(args)...);
}

template <SC FuncName, class T, class MemberFunctionPointer, class... Args>
  requires std::is_member_function_pointer_v<MemberFunctionPointer>
constexpr auto func(FuncName name, const T &t, MemberFunctionPointer mem_fun,
                    Args &&...args) {
  return func(name, NoDescription<typename FuncName::char_type>{}, t, mem_fun,
              std::forward<Args>(args)...);
}

/**
 * @}
 */

template <SC FuncName, SC Description, SC Help, class Function, class... Args>
struct MemberFunction {
  using arguments = TypeList<Args...>;
  static_assert(std::is_member_function_pointer_v<Function>,
                "A MemberFunctions Function template argument must be a "
                "pointer to member function");
  Function f;
  std::tuple<Args...> args;

  constexpr MemberFunction(FuncName, Description, Help, Function mem_fun_ptr,
                           const Args &...args) noexcept
      : f(mem_fun_ptr), args(args...) {}

  constexpr MemberFunction(FuncName, Description, Help, Function mem_fun_ptr,
                           const std::tuple<Args...> &args) noexcept
      : f(mem_fun_ptr), args(args) {}
};

template <SC FuncName, SC Description, SC Help, class Function>
struct MemberFunction<FuncName, Description, Help, Function> {
  using arguments = TypeList<>;
  static_assert(std::is_member_function_pointer_v<Function>,
                "A MemberFunctions Function template argument must be a "
                "pointer to member function");
  Function f;

  constexpr MemberFunction(FuncName, Description, Help,
                           Function mem_fun_ptr) noexcept
      : f(mem_fun_ptr) {}
};

template <SC FuncName, SC Description, SC Help, class Function, class... Args>
MemberFunction(FuncName &&, Description &&, Help &&, Function &&, Args &&...)
    -> MemberFunction<std::remove_cvref_t<FuncName>,
                      std::remove_cvref_t<Description>,
                      std::remove_cvref_t<Help>, std::remove_cvref_t<Function>,
                      std::remove_cvref_t<Args>...>;
template <SC FuncName, SC Description, SC Help, class Function, FuncArg... Args>
MemberFunction(FuncName &&, Description &&, Help &&, Function &&,
               const std::tuple<Args...> &)
    -> MemberFunction<std::remove_cvref_t<FuncName>,
                      std::remove_cvref_t<Description>,
                      std::remove_cvref_t<Help>, std::remove_cvref_t<Function>,
                      std::remove_cvref_t<Args>...>;

template <SC FuncName, SC Description, SC Help, class Function>
MemberFunction(FuncName &&, Description &&, Help &&, Function &&)
    -> MemberFunction<std::remove_cvref_t<FuncName>,
                      std::remove_cvref_t<Description>,
                      std::remove_cvref_t<Help>, std::remove_cvref_t<Function>>;

/**
 * @addtogroup Functions
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
 * @param args the arguments, i.e. something created with funcs::arg
 * @return a partial Command
 */
template <SC FuncName, class MemberFunctionPointer, class... Args>
  requires std::is_member_function_pointer_v<MemberFunctionPointer>
constexpr auto mem_fun(FuncName name, MemberFunctionPointer mem_fun,
                       Args &&...args) {
  (void)name;
  if constexpr (sizeof...(Args) > 0) {
    constexpr auto deduced_args =
        dtl::deduce_args<decltype(mem_fun)>(std::forward<Args>(args)...);
    return MemberFunction{
        FuncName{}, NoDescription<typename FuncName::char_type>{},
        dtl::pretty_signature_name<decltype(mem_fun)>(deduced_args), mem_fun,
        deduced_args};
  } else {
    return MemberFunction{
        FuncName{}, NoDescription<typename FuncName::char_type>{},
        dtl::pretty_signature_name<decltype(mem_fun)>(), mem_fun};
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
 *  cli::Cli my_cli(...,
 *                  cli::param("foo"_sc, foo, // <- foo must be direct parent
 *                                            // of bar and baz
 *                             cli::mem_fun<&Foo::bar>("param"_arg), // name is
 *                                                                   // deduced
 *                             cli::mem_fun<&Foo::baz>(), // name is deduced
 *                             ... ),
 *                  ...);
 * ```
 *
 * This is used to avoid repeating the object, i.e. ``foo``, for every
 * member function added, as would be the case when using cli::func().
 *
 * Additionally, the name of the function is automatically deduced.
 *
 * @param name the name of the member function
 * @param mem_fun pointer to the member function
 * @param args the arguments, i.e. something created with funcs::arg
 * @return a partial Command
 */
template <auto MemberFunctionPointer, class... Args>
  requires std::is_member_function_pointer_v<decltype(MemberFunctionPointer)>
constexpr auto mem_fun(Args &&...args) {
  if constexpr (sizeof...(Args) > 0) {
    constexpr auto deduced_args =
        dtl::deduce_args<decltype(MemberFunctionPointer)>(
            std::forward<Args>(args)...);
    return MemberFunction{
        ctti::value_name<MemberFunctionPointer>(), NoDescription<char>{},
        dtl::pretty_signature_name<decltype(MemberFunctionPointer)>(
            deduced_args),
        MemberFunctionPointer, deduced_args};
  } else {
    return MemberFunction{
        ctti::value_name<MemberFunctionPointer>(), NoDescription<char>{},
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
template <class T, SC CmdName, SC Description, SC Help, class F, class... Args>
constexpr auto
to_cmd(T &obj, const MemberFunction<CmdName, Description, Help, F, Args...>
                   &member_function) {
  if constexpr (sizeof...(Args) == 0)
    return Function(CmdName{}, Description{}, Help{},
                    MemFunBinder(obj, member_function.f));
  else
    return Function(CmdName{}, Description{}, Help{},
                    MemFunBinder(obj, member_function.f), member_function.args);
}

/**
 * create a Function from an object referenece and a MemberFunction
 */
template <class T, SC CmdName, SC Description, SC Help, class F, class... Args>
constexpr auto to_cmd(const T &obj,
                      const MemberFunction<CmdName, Description, Help, F,
                                           Args...> &member_function) {
  if constexpr (sizeof...(Args) == 0)
    return Function(CmdName{}, Description{}, Help{},
                    MemFunBinder(obj, member_function.f));
  else
    return Function(CmdName{}, Description{}, Help{},
                    MemFunBinder(obj, member_function.f), member_function.args);
}
} // namespace dtl
} // namespace cli::funcs
#endif
