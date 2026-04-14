/**
 * @file cli/param.hpp
 *
 * This file contains the utilities to create parameters, namely the
 * functions:
 * - param(..): creates a parameter
 * - obj(...): creates a parameter, and further also accepts member functions
 * and member data as subcommands. member functions are created with
 * cli::funcs::mem_fun().
 * - mem_data(...): Creates a member data command, inteded to be used with
 * obj(...).
 *
 * and the concepts:
 * - Getter/GetterOf: parameter getters/accessors
 * - Setter/SetterOf: parameter setters
 */

#ifndef CLI_PARAM_HPP
#define CLI_PARAM_HPP

#include "cli/command.hpp"
#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/format.hpp"
#include "cli/function.hpp"
#include "cli/parse.hpp"
#include "cli/type_list.hpp"
#include "cli/util.hpp"
#include "cli/validator.hpp"
#include <concepts>
#include <type_traits>

namespace cli::params {

template <class T> struct getter_value_type {
  using type = std::remove_cvref_t<
      type_list::type_at_t<0, typename function_traits<T>::arguments>>;
};

template <class T> struct setter_value_type {
  using type = std::remove_cvref_t<
      type_list::type_at_t<0, typename function_traits<T>::arguments>>;
};

template <class T>
using getter_value_type_t =
    typename getter_value_type<std::remove_cvref_t<T>>::type;

template <class T>
using setter_value_type_t =
    typename setter_value_type<std::remove_cvref_t<T>>::type;

/**
 * concept for a Getter with value type V
 *
 * @tparam G the getter type
 * @tparam V the value type
 */
template <class G, class V>
concept GetterOf = requires(G &&getter, V &value) {
  { getter(value) } -> std::same_as<Error>;
};

/**
 * concept for a Setter with value type V
 *
 * @tparam S the setter type
 * @tparam V the value type
 */
template <typename S, typename V>
concept SetterOf = requires(S &&setter, const V &value) {
  { setter(value) } -> std::same_as<Error>;
};

/**
 * A Getter G retrieves the value of a parameter. An instance of G must
 * be callable with an l value reference and return a cli::Error. The reference
 * denotes the place where the getter should store it value. If G cannot produce
 * a value, it should return the error that occurred.
 *
 * TODO: maybe a pointer return type would be better
 *
 * @tparam G the type to test
 */
template <class G>
concept Getter = requires(G &&getter, getter_value_type_t<G> &value) {
  { getter(value) } -> std::same_as<Error>;
};

/**
 * A Setter S sets the value of a parameter. An instance of S must be
 * callable with a const l value reference and return a cli::Error.
 *
 * @tparam S the type to test
 */
template <class S>
concept Setter = requires(S &&setter, const setter_value_type_t<S> &value) {
  { setter(value) } -> std::same_as<Error>;
};

namespace dtl {

/**
 * the thing returned by calls to cli::params::param
 *
 * TODO: forward subcommands of struct to struct parser
 *
 * @tparam Name the name of the parameter
 * @tparam Description the description
 * @tparam Type a string that represents the type of the parameter
 * @tparam Get the callable thats retrieves a T's value
 * @tparam Set the callable thats sets a T's value
 * @tparam Parse the callable that parses a T from a ByteView
 * @tparam Format the callable that formats a T into a ByteView
 * @tparam SubCommands further Param or Function sub commands
 */
template <SC Name, SC Description, SC Type, Getter Get, Setter Set,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
class Param : public CommandBase<Param<Name, Description, Type, Get, Set, Parse,
                                       Format, Validate, SubCommands...>,
                                 Name, Description, Type, SubCommands...> {
  using Base = CommandBase<Param<Name, Description, Type, Get, Set, Parse,
                                 Format, Validate, SubCommands...>,
                           Name, Description, Type, SubCommands...>;
  using value_type = typename getter_value_type<Get>::type;
  static_assert(
      std::is_same_v<value_type, typename setter_value_type<Set>::type>,
      "Get and Set must get/set a value of the same type");
  static_assert(
      std::is_same_v<value_type,
                     parse::value_type_t<typename Name::char_type, Parse>>,
      "Parse and Get/Set must have the same value type");
  static_assert(
      std::is_same_v<value_type,
                     typename format::formatter_value_type<Format>::type>,
      "Format and Get/Set must have the same value type");
  static_assert(std::is_same_v<value_type, validate::value_type_t<Validate>>,
                "Validate, Parse and Get/Set must have the same value type");

public:
  using char_type = typename Base::char_type;
  using Base::description;
  using Base::name;
  using sub_command_list = typename Base::sub_command_list;
  using Base::type;

  constexpr Param(const Param &) = default;
  constexpr Param(Param &&) = default;
  constexpr Param &operator=(const Param &) = default;
  constexpr Param &operator=(Param &&) = default;

  template <Getter Get_, Setter Set_,
            parse::Parser<typename Name::char_type> Parse_,
            format::Formatter Format_, validate::Validator Validate_,
            Command... SubCommands_>
  constexpr Param(Name, Description, Type, Get_ &&get, Set_ &&set,
                  Parse_ &&parse, Format_ &&format, Validate_ &&validate,
                  SubCommands_ &&...cmds)
      : Base(std::forward<SubCommands_>(cmds)...),
        get_(std::forward<Get_>(get)), set_(std::forward<Set_>(set)),
        parse_(std::forward<Parse_>(parse)),
        format_(std::forward<Format_>(format)),
        validate_(std::forward<Validate_>(validate)) {}

  template <Getter Get_, Setter Set_,
            parse::Parser<typename Name::char_type> Parse_,
            format::Formatter Format_, validate::Validator Validate_>
    requires(sizeof...(SubCommands) == 0)
  constexpr Param(Name, Description, Type, Get_ &&get, Set_ &&set,
                  Parse_ &&parse, Format_ &&format, Validate_ &&validate)
      : Base(), get_(std::forward<Get_>(get)), set_(std::forward<Set_>(set)),
        parse_(std::forward<Parse_>(parse)),
        format_(std::forward<Format_>(format)),
        validate_(std::forward<Validate_>(validate)) {}

  template <Getter Get_, Setter Set_,
            parse::Parser<typename Name::char_type> Parse_,
            format::Formatter Format_, validate::Validator Validate_>
    requires(sizeof...(SubCommands) > 0)
  constexpr Param(Name, Description, Type, Get_ &&get, Set_ &&set,
                  Parse_ &&parse, Format_ &&format, Validate_ &&validate,
                  std::tuple<SubCommands...> &&cmds)
      : Base(std::move(cmds)), get_(std::forward<Get_>(get)),
        set_(std::forward<Set_>(set)), parse_(std::forward<Parse_>(parse)),
        format_(std::forward<Format_>(format)),
        validate_(std::forward<Validate_>(validate)) {}

  template <Getter Get_, Setter Set_,
            parse::Parser<typename Name::char_type> Parse_,
            format::Formatter Format_, validate::Validator Validate_>
    requires(sizeof...(SubCommands) > 0)
  constexpr Param(Name, Description, Type, Get_ &&get, Set_ &&set,
                  Parse_ &&parse, Format_ &&format, Validate_ &&validate,
                  const std::tuple<SubCommands...> &cmds)
      : Base(cmds), get_(std::forward<Get_>(get)),
        set_(std::forward<Set_>(set)), parse_(std::forward<Parse_>(parse)),
        format_(std::forward<Format_>(format)),
        validate_(std::forward<Validate_>(validate)) {}

  // Param(SC /*name*/, const T &value, Get getter, Set setter);

  Error execute(ExecType type, View<const char_type> args,
                View<char_type> &out) {
    switch (type) {
    case ExecType::set:
      return set_value(args);
    case ExecType::get:
      if (args.size() != 0)
        return Error::too_many_argments;
      return get_value(out);
    default:
      return Error::invalid_cmd;
    }
    return Error::unimplemented;
  }

private:
  Error set_value(View<const char_type> args) {
    if (args.size() == 0)
      return Error::too_few_arguments;
    auto parse_result = parse_(args);
    if (not parse_result)
      return parse_result.error;
    return set_(parse_result.value);
  }

  Error get_value(View<char_type> &out) {
    value_type t{};
    get_(t);
    auto res = format_(out, t);
    if (res.error != Error::none)
      return res.error;
    out = out.substr(0, res.size_written);
    return Error::none;
  }

  CLI_NO_UNIQUE_ADDRESS Get get_;
  CLI_NO_UNIQUE_ADDRESS Set set_;
  CLI_NO_UNIQUE_ADDRESS Parse parse_;
  CLI_NO_UNIQUE_ADDRESS Format format_;
  CLI_NO_UNIQUE_ADDRESS Validate validate_;
};

template <SC Name, SC Description, SC Type, Getter Get, Setter Set,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
Param(Name, Description, Type, Get &&get, Set &&set, Parse &&parse,
      Format &&format, Validate &&validate, SubCommands &&...cmds)
    -> Param<std::remove_cvref_t<Name>, std::remove_cvref_t<Description>,
             std::remove_cvref_t<Type>, std::remove_cvref_t<Get>,
             std::remove_cvref_t<Set>, std::remove_cvref_t<Parse>,
             std::remove_cvref_t<Format>, std::remove_cvref_t<Validate>,
             std::remove_cvref_t<SubCommands>...>;

template <SC Name, SC Description, SC Type, Getter Get, Setter Set,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate>
Param(Name, Description, Type, Get &&get, Set &&set, Parse &&parse,
      Format &&format, Validate &&validate)
    -> Param<std::remove_cvref_t<Name>, std::remove_cvref_t<Description>,
             std::remove_cvref_t<Type>, std::remove_cvref_t<Get>,
             std::remove_cvref_t<Set>, std::remove_cvref_t<Parse>,
             std::remove_cvref_t<Format>, std::remove_cvref_t<Validate>>;

template <SC Name, SC Description, SC Type, Getter Get, Setter Set,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
Param(Name, Description, Type, Get &&get, Set &&set, Parse &&parse,
      Format &&format, Validate &&validate, std::tuple<SubCommands...> &&cmds)
    -> Param<std::remove_cvref_t<Name>, std::remove_cvref_t<Description>,
             std::remove_cvref_t<Type>, std::remove_cvref_t<Get>,
             std::remove_cvref_t<Set>, std::remove_cvref_t<Parse>,
             std::remove_cvref_t<Format>, std::remove_cvref_t<Validate>,
             std::remove_cvref_t<SubCommands>...>;

template <SC Name, SC Description, SC Type, Getter Get, Setter Set,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
Param(Name, Description, Type, Get &&get, Set &&set, Parse &&parse,
      Format &&format, Validate &&validate,
      const std::tuple<SubCommands...> &cmds)
    -> Param<std::remove_cvref_t<Name>, std::remove_cvref_t<Description>,
             std::remove_cvref_t<Type>, std::remove_cvref_t<Get>,
             std::remove_cvref_t<Set>, std::remove_cvref_t<Parse>,
             std::remove_cvref_t<Format>, std::remove_cvref_t<Validate>,
             std::remove_cvref_t<SubCommands>...>;

template <SC Name, SC Description, SC Type, class MemberPointer,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
struct MemberData {
  using char_type = typename Name::char_type;
  MemberPointer f;
  std::tuple<SubCommands...> subcommands;
  CLI_NO_UNIQUE_ADDRESS Parse parse;
  CLI_NO_UNIQUE_ADDRESS Format format;
  CLI_NO_UNIQUE_ADDRESS Validate validate;

  template <parse::Parser<typename Name::char_type> Parse_,
            format::Formatter Format_, validate::Validator Validate_,
            Command... SubCommands_>
  constexpr MemberData(Name, Description, Type, MemberPointer f, Parse_ &&parse,
                       Format_ &&format, Validate_ &&validate,
                       SubCommands_ &&...cmds)
      : f(f), subcommands(std::forward<SubCommands>(cmds)...),
        parse(std::forward<Parse_>(parse)),
        format(std::forward<Format>(format)),
        validate(std::forward<Validate_>(validate)) {}
};

template <SC Name, SC Description, SC Help, class MemberPointer,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate>
struct MemberData<Name, Description, Help, MemberPointer, Parse, Format,
                  Validate> {
  using char_type = typename Name::char_type;
  MemberPointer f;
  CLI_NO_UNIQUE_ADDRESS Parse parse;
  CLI_NO_UNIQUE_ADDRESS Format format;
  CLI_NO_UNIQUE_ADDRESS Validate validate;

  template <parse::Parser<typename Name::char_type> Parse_,
            format::Formatter Format_, validate::Validator Validate_>
  constexpr MemberData(Name, Description, Help, MemberPointer f, Parse_ &&parse,
                       Format_ &&format, Validate_ &&validate)
      : f(f), parse(std::forward<Parse_>(parse)),
        format(std::forward<Format>(format)),
        validate(std::forward<Validate_>(validate)) {}
};

template <SC Name, SC Description, SC Help, class MemberPointer,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
MemberData(Name, Description, Help, MemberPointer, Parse &&, Format &&,
           Validate &&, SubCommands &&...)
    -> MemberData<std::remove_cvref_t<Name>, std::remove_cvref_t<Description>,
                  std::remove_cvref_t<Help>, MemberPointer,
                  std::remove_cvref_t<Parse>, std::remove_cvref_t<Format>,
                  std::remove_cvref_t<Validate>,
                  std::remove_cvref_t<SubCommands>...>;

struct NullGet {
  constexpr NullGet() = default;
  constexpr NullGet(const NullGet &) = default;
  constexpr NullGet(NullGet &&) = default;
  constexpr NullGet &operator=(const NullGet &) = default;
  constexpr NullGet &operator=(NullGet &&) = default;
  constexpr Error operator()(dummy &) { return Error::none; }
};

struct NullSet {
  constexpr NullSet() = default;
  constexpr NullSet(const NullSet &) = default;
  constexpr NullSet(NullSet &&) = default;
  constexpr NullSet &operator=(const NullSet &) = default;
  constexpr NullSet &operator=(NullSet &&) = default;
  constexpr Error operator()(const dummy &) { return Error::none; }
};

template <typename T> struct DefaultGet {
  const T *value_;
  constexpr DefaultGet(const T &v) : value_(&v) {}
  constexpr DefaultGet(const DefaultGet &) = default;
  constexpr DefaultGet(DefaultGet &&) = default;
  constexpr DefaultGet &operator=(const DefaultGet &) = default;
  constexpr DefaultGet &operator=(DefaultGet &&) = default;

  constexpr Error operator()(T &t) {
    if (value_ == nullptr)
      return Error::cant_read_param;
    t = *value_;
    return Error::none;
  }
};

template <typename T> struct DefaultSet {
  T *value_;
  constexpr DefaultSet(T &v) : value_(&v) {}
  constexpr DefaultSet(const DefaultSet &) = default;
  constexpr DefaultSet(DefaultSet &&) = default;
  constexpr DefaultSet &operator=(const DefaultSet &) = default;
  constexpr DefaultSet &operator=(DefaultSet &&) = default;

  constexpr Error operator()(const T &t) {
    if (value_ == nullptr)
      return Error::cant_set_param;
    *value_ = t;
    return Error::none;
  }
};

template <typename T, typename MemberPtr> struct MemDataGet {
  const T &value_;
  MemberPtr member;
  constexpr Error operator()(mem_data_type<MemberPtr> &t) {
    t = (value_.*member);
    return Error::none;
  }
};

template <typename T, typename MemberPtr> struct MemDataSet {
  T &value_;
  MemberPtr member;
  constexpr Error operator()(const mem_data_type<MemberPtr> &t) {
    value_.*member = t;
    return Error::none;
  }
};

template <typename T, typename MemberPtr>
struct MemDataSet<const T, MemberPtr> {
  constexpr Error operator()(const mem_data_type<MemberPtr> &) {
    return Error::cant_set_param;
  }
};

template <class T, SC Name, SC Description, SC Help, class MemberPointer,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
constexpr auto to_cmd(T &obj,
                      MemberData<Name, Description, Help, MemberPointer, Parse,
                                 Format, Validate, SubCommands...>
                          member_data) {
  if constexpr (sizeof...(SubCommands) > 0)
    return Param{Name{},
                 Description{},
                 Help{},
                 MemDataGet<T, MemberPointer>{obj, member_data.f},
                 MemDataSet<T, MemberPointer>{obj, member_data.f},
                 std::move(member_data.parse),
                 std::move(member_data.format),
                 std::move(member_data.validate),
                 std::move(member_data.subcommands)};
  else
    return Param{Name{},
                 Description{},
                 Help{},
                 MemDataGet<T, MemberPointer>{obj, member_data.f},
                 MemDataSet<T, MemberPointer>{obj, member_data.f},
                 std::move(member_data.parse),
                 std::move(member_data.format),
                 std::move(member_data.validate)};
}

template <class T, SC Name, SC Description, SC Help, class MemberPointer,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
constexpr auto to_cmd(const T &obj,
                      MemberData<Name, Description, Help, MemberPointer, Parse,
                                 Format, Validate, SubCommands...>
                          member_data) {
  if constexpr (sizeof...(SubCommands) > 0)
    return Param{Name{},
                 Description{},
                 Help{},
                 MemDataGet<T, MemberPointer>{obj, member_data.f},
                 MemDataSet<T, MemberPointer>{obj, member_data.f},
                 std::move(member_data.parse),
                 std::move(member_data.format),
                 std::move(member_data.validate),
                 std::move(member_data.subcommands)};
  else
    return Param{Name{},
                 Description{},
                 Help{},
                 MemDataGet<T, MemberPointer>{obj, member_data.f},
                 MemDataSet<T, MemberPointer>{obj, member_data.f},
                 std::move(member_data.parse),
                 std::move(member_data.format),
                 std::move(member_data.validate)};
}

template <class T, class CommandOrMemberDataOrMemberFunction>
constexpr auto transform(T &obj, CommandOrMemberDataOrMemberFunction &&mem) {
  if constexpr (Command<CommandOrMemberDataOrMemberFunction>) {
    return mem;
  } else {
    using dtl::to_cmd;
    using funcs::dtl::to_cmd;
    return to_cmd(obj, std::forward<CommandOrMemberDataOrMemberFunction>(mem));
  }
}
} // namespace dtl

/**
 * @addtogroup parameters
 *
 *
 * @{
 */

/**
 * creates a "virtual" command, i.e. a parameter without a value to set
 * or get, but subcommands. Requires at least one sub command.
 *
 * Example:
 * ``auto p = param("my-cmd"_sc, param(...), func(...))``
 *
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param cmds the sub commands.
 * @return a Command
 */
template <SC Name, Command... SubCommands>
  requires(sizeof...(SubCommands) > 0)
constexpr auto param(Name name, SubCommands &&...cmds) {
  (void)name;
  using namespace dtl;
  return Param{Name{},
               NoDescription<typename Name::char_type>{},
               "virtual"_sc,
               NullGet{},
               NullSet{},
               parse::NullParse<typename Name::char_type>{},
               format::NullFormat<typename Name::char_type>{},
               validate::NullValidate{},
               std::forward<SubCommands>(cmds)...};
}

/**
 * creates a "virtual" command, i.e. a parameter without a value ot set
 * or get, but subcommands. Requires at least one sub command.
 *
 * Example:
 * ``auto p = param("cfg"_sc, "configuration"_sc,  param("app"_sc, ...),
 * param("dbg"_sc, ...))``
 *
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param description the description of the parameter, used by the help
 * functionality. Must be a cli::string_constant.
 * @param cmds the sub commands.
 * @return a Command
 */
template <SC Name, SC Description, Command... SubCommands>
  requires(sizeof...(SubCommands) > 0)
constexpr auto param(Name name, Description description,
                     SubCommands &&...cmds) {
  (void)name;
  (void)description;
  using namespace dtl;
  return Param{Name{},
               Description{},
               "virtual"_sc,
               NullGet{},
               NullSet{},
               parse::NullParse<typename Name::char_type>{},
               format::NullFormat<typename Name::char_type>{},
               validate::NullValidate{},
               std::forward<SubCommands>(cmds)...};
}

/**
 * creates a parameter command. The value of the parameter, i.e. t, can
 * then be retrieved by its name. This uses the default getter, setter, parsing,
 * formatting, and validation facilities.
 *
 * Example:
 *
 * ```
 * extern T my_var;
 *
 * auto p = param("my-var"_sc, my_var, ...);
 * ```
 *
 * @tparam T the parameters type
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param t the parameter value
 * @param cmds optional subcommands
 * @return a Command
 */
template <SC Name, class T, Command... SubCommands>
constexpr auto param(Name name, T &t, SubCommands &&...cmds) {
  (void)name;
  using namespace dtl;
  return Param{
      Name{},
      NoDescription<typename Name::char_type>{},
      cli::ctti::name<std::remove_cvref_t<T>, typename Name::char_type>(),
      DefaultGet<std::remove_cvref_t<T>>{t},
      DefaultSet<std::remove_cvref_t<T>>{t},
      parse::DefaultParse<std::remove_cvref_t<T>, typename Name::char_type>(),
      format::DefaultFormat<std::remove_cvref_t<T>, typename Name::char_type>(),
      validate::DefaultValidate<std::remove_cvref_t<T>>{},
      std::forward<SubCommands>(cmds)...};
}

/**
 * creates a parameter command. The value of the parameter, i.e. t, can
 * then be retrieved by its name. This uses the default getter, setter, parsing,
 * formatting, and validation facilities.
 *
 * Example:
 *
 * ```
 * extern T my_var;
 *
 * auto p = param("my-var"_sc, "how much to foo"_sc, my_var, ...);
 * ```
 *
 * @tparam T the parameters type
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param description the description of the parameter. Must be a
 * cli::string_constant.
 * @param t the parameter value
 * @param cmds optional subcommands
 * @return a Command
 */
template <SC Name, SC Description, class T, Command... SubCommands>
constexpr auto param(Name name, Description description, T &t,
                     SubCommands &&...cmds) {
  (void)name;
  (void)description;
  using namespace dtl;
  return Param{
      Name{},
      Description{},
      cli::ctti::name<std::remove_cvref_t<T>, typename Name::char_type>(),
      DefaultGet<std::remove_cvref_t<T>>{t},
      DefaultSet<std::remove_cvref_t<T>>{t},
      parse::DefaultParse<std::remove_cvref_t<T>, typename Name::char_type>(),
      format::DefaultFormat<std::remove_cvref_t<T>, typename Name::char_type>(),
      validate::DefaultValidate<std::remove_cvref_t<T>>{},
      std::forward<SubCommands>(cmds)...};
}

/**
 * creates a parameter command from its individual parts.
 *
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param description the parameter description, used by the help functionality.
 * Must be a cli::string_constant.
 * @param type the parameter type as a string, used by the help functionality.
 * Must be a cli::string_constant.
 * @param get the getter of the parameter. See cli::params::Getter for
 * additional info.
 * @param set the setter of the parameter. See cli::params::Setter for
 * additional info.
 * @param parse the parser of the parameter. See cli::parse::Parser for
 * additional info.
 * @param format the formatter of the parameter. See cli::format::Formatter for
 * additional info.
 * @param validate the validator of the parameter. See cli::validate::Validator
 * for additional info.
 * @param cmds additional optional subcommands
 * @return a Command
 */
template <SC Name, SC Description, SC Type, Getter Get, Setter Set,
          parse::Parser<typename Name::char_type> Parse,
          format::Formatter Format, validate::Validator Validate,
          Command... SubCommands>
constexpr auto param(Name name, Description description, Type type, Get &&get,
                     Set &&set, Parse &&parse, Format &&format,
                     Validate &&validate, SubCommands &&...cmds) {
  (void)name;
  (void)description;
  (void)type;
  using namespace dtl;
  return Param{Name{},
               Description{},
               Type{},
               std::forward<Get>(get),
               std::forward<Set>(set),
               std::forward<Parse>(parse),
               std::forward<Format>(format),
               std::forward<Validate>(validate),
               std::forward<SubCommands>(cmds)...};
}

/**
 * creates a parameter command from its individual parts.
 *
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param description the parameter description, used by the help functionality.
 * Must be a cli::string_constant.
 * @param type the parameter type as a string, used by the help functionality.
 * Must be a cli::string_constant.
 * @param get the getter of the parameter. See cli::params::Getter for
 * additional info.
 * @param set the setter of the parameter. See cli::params::Setter for
 * additional info.
 * @param cmds additional optional subcommands
 * @return a Command
 */
template <SC Name, SC Description, typename T, GetterOf<T> Get, SetterOf<T> Set,
          Command... SubCommands>
constexpr auto param(Name name, Description description, T &t, Get &&get,
                     Set &&set, SubCommands &&...cmds) {
  (void)name;
  (void)description;
  using namespace dtl;
  using Char = typename Name::char_type;
  return Param{Name{},
               Description{},
               ctti::name<T>(),
               std::forward<Get>(get),
               std::forward<Set>(set),
               parse::DefaultParse<T, Char>{},
               format::DefaultFormat<T, Char>{},
               validate::DefaultValidate<T>{},
               std::forward<SubCommands>(cmds)...};
}

/**
 * creates a parameter command from its individual parts.
 *
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param description the parameter description, used by the help functionality.
 * Must be a cli::string_constant.
 * @param type the parameter type as a string, used by the help functionality.
 * Must be a cli::string_constant.
 * @param get the getter of the parameter. See cli::params::Getter for
 * additional info.
 * @param set the setter of the parameter. See cli::params::Setter for
 * additional info.
 * @param cmds additional optional subcommands
 * @return a Command
 */
template <SC Name, SC Description, typename T, SetterOf<T> Set,
          Command... SubCommands>
constexpr auto param(Name name, Description description, T &t, Set &&set,
                     SubCommands &&...cmds) {
  (void)name;
  (void)description;
  using namespace dtl;
  using Char = typename Name::char_type;
  return Param{Name{},
               Description{},
               ctti::name<T>(),
               DefaultGet<T>{t},
               std::forward<Set>(set),
               parse::DefaultParse<T, Char>{},
               format::DefaultFormat<T, Char>{},
               validate::DefaultValidate<T>{},
               std::forward<SubCommands>(cmds)...};
}

/**
 * creates a parameter command. The value of the parameter, i.e. t, can
 * then be retrieved by its name. This opverload can take member data and member
 * functions in addition to sub commands. Uses the default getter, setter,
 * parsing, formatting, and validation facilities.
 *
 * @tparam T the parameters type
 * @tparam CommandOrMemberDataOrMemberFunction
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param obj the parameter value and the object that member data and functions
 * are called on.
 * @param m an assortment of sub commands, member functions, and member data
 * @return
 */
template <SC Name, class T, class... CommandOrMemberDataOrMemberFunction>
constexpr auto param(Name name, T &obj,
                     CommandOrMemberDataOrMemberFunction &&...m) {
  (void)name;
  using namespace dtl;
  return Param{
      Name{},
      NoDescription<typename Name::char_type>{},
      cli::ctti::name<std::remove_cvref_t<T>, typename Name::char_type>(),
      DefaultGet<T>{obj},
      DefaultSet<T>{obj},
      parse::DefaultParse<T, typename Name::char_type>{},
      format::DefaultFormat<T, typename Name::char_type>{},
      validate::DefaultValidate<std::remove_cvref_t<T>>{},
      dtl::transform(obj,
                     std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
}
/**
 * creates a parameter command. The value of the parameter, i.e. t, can
 * then be retrieved by its name. This overload can take member data and member
 * functions in addition to sub commands. Uses the default getter, setter,
 * parsing, formatting, and validation facilities.
 *
 * @tparam T the parameters type
 * @tparam CommandOrMemberDataOrMemberFunction
 * @param name the name of the parameter. Must be a cli::string_constant.
 * @param description the description of the parameter. Must be a
 * cli::string_constant.
 * @param obj the parameter value and the object that member data and functions
 * are called on.
 * @param m an assortment of sub commands, member functions, and member data
 * @return
 */
// template <SC Name, SC Description, class T,
//           class... CommandOrMemberDataOrMemberFunction>
// constexpr auto param(Name name, Description description, T &obj,
//                      CommandOrMemberDataOrMemberFunction &&...m) {
//   (void)name;
//   (void)description;
//   using namespace dtl;
//   return Param{
//       Name{},
//       Description{},
//       cli::ctti::name<std::remove_cvref_t<T>, typename Name::char_type>(),
//       DefaultGet<T>{obj},
//       DefaultSet<T>{obj},
//       parse::DefaultParse<T, typename Name::char_type>{},
//       format::DefaultFormat<T, typename Name::char_type>{},
//       validate::DefaultValidate<std::remove_cvref_t<T>>{},
//       dtl::transform(obj,
//                      std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
// }
//
/**
 * creates a parameter command. The value of the parameter, i.e. Obj, can
 * then be retrieved by its name, which is deduced. This overload can take
 * member data and member functions in addition to sub commands. Uses the
 * default getter, setter, parsing, formatting, and validation facilities.
 *
 * Example:
 * ```
 * struct Settings{
 *   int k;
 *   ...
 * };
 * static Settings settings{...};
 * // p has the name "settings"
 * auto p = param<settings>(...);
 * ```
 * @tparam Obj a reference to an aggrate.
 * @tparam CommandOrMemberDataOrMemberFunction
 * @param m an assortment of sub commands, member functions, and member data
 * @return a Command
 */
template <auto &Obj, class... CommandOrMemberDataOrMemberFunction>
constexpr auto param(CommandOrMemberDataOrMemberFunction &&...m) {
  using T = std::remove_cvref_t<decltype(Obj)>;
  return dtl::Param{
      ctti::object_name<Obj>(),
      NoDescription<char>{},
      cli::ctti::name<T>(),
      dtl::DefaultGet<T>{Obj},
      dtl::DefaultSet<T>{Obj},
      parse::DefaultParse<T, char>{},
      format::DefaultFormat<T, char>{},
      validate::DefaultValidate<std::remove_cvref_t<T>>{},
      dtl::transform(Obj,
                     std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
}
/**
 * creates a parameter command. The value of the parameter, i.e. Obj, can
 * then be retrieved by its name, which is deduced. This overload can take
 * member data and member functions in addition to sub commands. Uses the
 * default getter, setter, parsing, formatting, and validation facilities.
 *
 * Example:
 * ```
 * struct Settings{
 *   int k;
 *   ...
 * };
 * static Settings settings{...};
 * // p has the name "settings"
 * auto p = param<settings>("global app settings"_sc, ...);
 * ```
 * @tparam Obj a reference to an aggrate.
 * @tparam CommandOrMemberDataOrMemberFunction
 * @param description the description of the parameter. Must be a
 * cli::string_constant.
 * @param m an assortment of sub commands, member functions, and member data
 * @return a Command
 */
template <auto &Obj, SC Description,
          class... CommandOrMemberDataOrMemberFunction>
constexpr auto param(Description description,
                     CommandOrMemberDataOrMemberFunction &&...m) {
  (void)description;
  using namespace dtl;
  using T = std::remove_cvref_t<decltype(Obj)>;
  return Param{
      ctti::object_name<Obj, typename Description::char_type>(),
      Description{},
      cli::ctti::name<T, typename Description::char_type>(),
      DefaultGet<T>{Obj},
      DefaultSet<T>{Obj},
      parse::DefaultParse<T, typename Description::char_type>{},
      format::DefaultFormat<T, typename Description::char_type>{},
      validate::DefaultValidate<std::remove_cvref_t<T>>{},
      dtl::transform(Obj,
                     std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
}

/**
 * @brief
 *
 * @tparam T
 * @tparam CommandOrMemberDataOrMemberFunction
 * @param obj
 * @param m
 * @return
 */
template <class T, class... CommandOrMemberDataOrMemberFunction>
constexpr auto param(T &obj, CommandOrMemberDataOrMemberFunction &&...m) {
  using namespace dtl;
  return Param{
      cli::to_lower(cli::ctti::name<std::remove_cvref_t<T>>()),
      NoDescription<char>{},
      cli::ctti::name<std::remove_cvref_t<T>>(),
      DefaultGet<T>{obj},
      DefaultSet<T>{obj},
      parse::DefaultParse<T, char>{},
      format::DefaultFormat<T, char>{},
      validate::DefaultValidate<std::remove_cvref_t<T>>{},
      dtl::transform(obj,
                     std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
}

template <SC Name, SC Description, class MemberPointer, Command... SubCommands>
  requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
constexpr auto mem_data(Name, Description, MemberPointer f,
                        SubCommands &&...cmds) {
  using namespace dtl;
  return MemberData{
      Name{},
      Description{},
      cli::ctti::name<mem_data_type<MemberPointer>, typename Name::char_type>(),
      f,
      parse::DefaultParse<mem_data_type<MemberPointer>,
                          typename Name::char_type>{},
      format::DefaultFormat<mem_data_type<MemberPointer>,
                            typename Name::char_type>{},
      validate::DefaultValidate<mem_data_type<MemberPointer>>{},
      std::forward<SubCommands>(cmds)...};
}
template <SC Name, class MemberPointer, Command... SubCommands>
  requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
constexpr auto mem_data(Name, MemberPointer f, SubCommands &&...cmds) {
  using namespace dtl;
  return mem_data(Name{}, NoDescription<typename Name::char_type>{}, f,
                  std::forward<SubCommands>(cmds)...);
}

template <auto MemberPointer, SC Description, Command... SubCommands>
  requires std::is_member_pointer_v<
      std::remove_cvref_t<decltype(MemberPointer)>>
constexpr auto mem_data(Description, SubCommands &&...cmds) {
  using namespace dtl;
  return mem_data(
      ctti::value_name<MemberPointer, typename Description::char_type>(),
      Description{}, MemberPointer, std::forward<SubCommands>(cmds)...);
}

template <auto MemberPointer, Command... SubCommands>
  requires std::is_member_pointer_v<
      std::remove_cvref_t<decltype(MemberPointer)>>
constexpr auto mem_data(SubCommands &&...cmds) {
  using namespace dtl;
  return mem_data<MemberPointer>(NoDescription<char>{},
                                 std::forward<SubCommands>(cmds)...);
}

/**
 * @}
 */

} // namespace cli::params
#endif
