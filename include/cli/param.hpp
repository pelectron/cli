/**
 * @file "cli/param.hpp"
 *
 * This file contains the utilities to create parameters.
 */

#ifndef CLI_PARAM_HPP
#define CLI_PARAM_HPP

#include "cli/command.hpp"
#include "cli/concepts.hpp"
#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/format.hpp"
#include "cli/function.hpp"
#include "cli/parse.hpp"
#include "cli/string.hpp"
#include "cli/traits.hpp"
#include "cli/type_list.hpp"
#include "cli/util.hpp"
#include "cli/validator.hpp"
#include <concepts>
#include <type_traits>

namespace cli::params {

  template<class T>
  struct getter_value_type {
    using type = std::remove_cvref_t<
      type_list::type_at_t<0, typename function_traits<T>::arguments>>;
  };

  template<class T>
  struct setter_value_type {
    using type = std::remove_cvref_t<
      type_list::type_at_t<0, typename function_traits<T>::arguments>>;
  };

  template<class T>
  using getter_value_type_t =
    typename getter_value_type<std::remove_cvref_t<T>>::type;

  template<class T>
  using setter_value_type_t =
    typename setter_value_type<std::remove_cvref_t<T>>::type;

  template<class T>
  using first_arg_t = type_list::
    type_at_t<0, typename function_traits<std::remove_cvref_t<T>>::arguments>;

  template<typename T>
  inline constexpr bool is_non_const_lvalue_ref =
    std::is_lvalue_reference_v<T> and
    not std::is_const_v<std::remove_reference_t<T>>;

  template<typename T>
  inline constexpr bool is_const_lvalue_ref_or_unqualified =
    not is_non_const_lvalue_ref<T>;

  /**
   * concept for a Getter with value type V.
   *
   * @ingroup Parameters
   * @tparam G the getter type
   * @tparam V the value type
   */
  template<class G, class V>
  concept GetterOf = requires(G &&getter, std::remove_cvref_t<V> &value) {
    { getter(value) } -> std::same_as<Error>;
  } and is_non_const_lvalue_ref<first_arg_t<G>>;

  /**
   * concept for a Setter with value type V.
   *
   * @ingroup Parameters
   * @tparam S the setter type
   * @tparam V the value type
   */
  template<typename S, typename V>
  concept SetterOf = requires(S &&setter, const V &value) {
    { setter(value) } -> std::same_as<Error>;
  } and not is_non_const_lvalue_ref<V>;

  /**
   * A Getter G retrieves the value of a parameter. An instance of G must
   * be callable with an non-const lvalue reference and return a cli::Error. The
   * reference denotes the place where the getter should store its value. If G
   * cannot produce a value, it should return the error that occurred.
   *
   * @ingroup Parameters
   * @tparam G the type to test
   */
  template<class G>
  concept Getter = requires(G &&getter, getter_value_type_t<G> &value) {
    { getter(value) } -> std::same_as<Error>;
  };

  /**
   * A Setter S sets the value of a parameter. An instance of S must be
   * callable with a const l value reference and return a cli::Error.
   *
   * @ingroup Parameters
   * @tparam S the type to test
   */
  template<class S>
  concept Setter = requires(S &&setter, const setter_value_type_t<S> &value) {
    { setter(value) } -> std::same_as<Error>;
  };

  template<SC Str>
  using get_char_t = typename Str::char_type;

  namespace dtl {
    template<SC Name,
             SC Description,
             SC Type,
             Getter Get,
             Setter Set,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    class Param : public CommandBase<Param<Name,
                                           Description,
                                           Type,
                                           Get,
                                           Set,
                                           Parse,
                                           Format,
                                           Validate,
                                           SubCommands...>,
                                     Name,
                                     Description,
                                     Type,
                                     SubCommands...> {
      using Base = CommandBase<Param<Name,
                                     Description,
                                     Type,
                                     Get,
                                     Set,
                                     Parse,
                                     Format,
                                     Validate,
                                     SubCommands...>,
                               Name,
                               Description,
                               Type,
                               SubCommands...>;
      using value_type = typename getter_value_type<Get>::type;
      static_assert(
        std::is_same_v<value_type, typename setter_value_type<Set>::type>,
        "Get and Set must get/set a value of the same type");
      static_assert(
        std::is_same_v<value_type,
                       parse::value_type_t<get_char_t<Name>, Parse>>,
        "Parse and Get/Set must have the same value type");
      static_assert(
        std::is_same_v<value_type,
                       typename format::formatter_value_type<Format>::type>,
        "Format and Get/Set must have the same value type");
      static_assert(
        std::is_same_v<value_type, validate::value_type_t<Validate>>,
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

      template<Getter Get_,
               Setter Set_,
               parse::Parser Parse_,
               format::Formatter Format_,
               validate::Validator Validate_,
               concepts::Command... SubCommands_>
      constexpr Param(Name,
                      Description,
                      Type,
                      Get_ &&get,
                      Set_ &&set,
                      Parse_ &&parse,
                      Format_ &&format,
                      Validate_ &&validate,
                      SubCommands_ &&...cmds)
        : Base(std::forward<SubCommands_>(cmds)...),
          get_(std::forward<Get_>(get)),
          set_(std::forward<Set_>(set)),
          parse_(std::forward<Parse_>(parse)),
          format_(std::forward<Format_>(format)),
          validate_(std::forward<Validate_>(validate)) {}

      template<Getter Get_,
               Setter Set_,
               parse::Parser Parse_,
               format::Formatter Format_,
               validate::Validator Validate_>
        requires(sizeof...(SubCommands) == 0)
      constexpr Param(Name,
                      Description,
                      Type,
                      Get_ &&get,
                      Set_ &&set,
                      Parse_ &&parse,
                      Format_ &&format,
                      Validate_ &&validate)
        : Base(),
          get_(std::forward<Get_>(get)),
          set_(std::forward<Set_>(set)),
          parse_(std::forward<Parse_>(parse)),
          format_(std::forward<Format_>(format)),
          validate_(std::forward<Validate_>(validate)) {}

      template<Getter Get_,
               Setter Set_,
               parse::Parser Parse_,
               format::Formatter Format_,
               validate::Validator Validate_>
        requires(sizeof...(SubCommands) > 0)
      constexpr Param(Name,
                      Description,
                      Type,
                      Get_ &&get,
                      Set_ &&set,
                      Parse_ &&parse,
                      Format_ &&format,
                      Validate_ &&validate,
                      std::tuple<SubCommands...> &&cmds)
        : Base(std::move(cmds)),
          get_(std::forward<Get_>(get)),
          set_(std::forward<Set_>(set)),
          parse_(std::forward<Parse_>(parse)),
          format_(std::forward<Format_>(format)),
          validate_(std::forward<Validate_>(validate)) {}

      template<Getter Get_,
               Setter Set_,
               parse::Parser Parse_,
               format::Formatter Format_,
               validate::Validator Validate_>
        requires(sizeof...(SubCommands) > 0)
      constexpr Param(Name,
                      Description,
                      Type,
                      Get_ &&get,
                      Set_ &&set,
                      Parse_ &&parse,
                      Format_ &&format,
                      Validate_ &&validate,
                      const std::tuple<SubCommands...> &cmds)
        : Base(cmds),
          get_(std::forward<Get_>(get)),
          set_(std::forward<Set_>(set)),
          parse_(std::forward<Parse_>(parse)),
          format_(std::forward<Format_>(format)),
          validate_(std::forward<Validate_>(validate)) {}

      // Param(SC /*name*/, const T &value, Get getter, Set setter);

      constexpr Error execute(View<const char_type> args,
                              View<char_type> &out,
                              bool &should_print_newline) {
        args = parse::trim_ws(args);
        if (args.size() == 0) {
          should_print_newline = true;
          return get_value(out);
        }

        if (args[0] == '=') {
          args = args.substr(1);
          args = parse::skip_ws(args);
        }

        if (args.size() == 0)
          return Error::expected_value;

        out = {};
        should_print_newline = false;
        return set_value(args);
      }

    private:
      constexpr Error set_value(View<const char_type> args) {
        if (args.size() == 0)
          return Error::too_few_arguments;

        auto parse_result = parse_(args);
        if (not parse_result)
          return parse_result.error;

        if (parse_result.rest.size() != 0)
          return Error::unexpected_characters;

        if (not validate_(parse_result.value))
          return Error::invalid_value;

        return set_(parse_result.value);
      }

      constexpr Error get_value(View<char_type> &out) {
        value_type t{};
        if (auto err = get_(t); err != Error::none)
          return err;

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

    template<SC Name,
             SC Description,
             SC Type,
             Getter Get,
             Setter Set,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    Param(Name,
          Description,
          Type,
          Get &&get,
          Set &&set,
          Parse &&parse,
          Format &&format,
          Validate &&validate,
          SubCommands &&...cmds) -> Param<std::decay_t<Name>,
                                          std::decay_t<Description>,
                                          std::decay_t<Type>,
                                          std::decay_t<Get>,
                                          std::decay_t<Set>,
                                          std::decay_t<Parse>,
                                          std::decay_t<Format>,
                                          std::decay_t<Validate>,
                                          std::decay_t<SubCommands>...>;

    template<SC Name,
             SC Description,
             SC Type,
             Getter Get,
             Setter Set,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate>
    Param(Name,
          Description,
          Type,
          Get &&get,
          Set &&set,
          Parse &&parse,
          Format &&format,
          Validate &&validate) -> Param<std::decay_t<Name>,
                                        std::decay_t<Description>,
                                        std::decay_t<Type>,
                                        std::decay_t<Get>,
                                        std::decay_t<Set>,
                                        std::decay_t<Parse>,
                                        std::decay_t<Format>,
                                        std::decay_t<Validate>>;

    template<SC Name,
             SC Description,
             SC Type,
             Getter Get,
             Setter Set,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    Param(Name,
          Description,
          Type,
          Get &&get,
          Set &&set,
          Parse &&parse,
          Format &&format,
          Validate &&validate,
          std::tuple<SubCommands...> &&cmds)
      -> Param<std::decay_t<Name>,
               std::decay_t<Description>,
               std::decay_t<Type>,
               std::decay_t<Get>,
               std::decay_t<Set>,
               std::decay_t<Parse>,
               std::decay_t<Format>,
               std::decay_t<Validate>,
               std::decay_t<SubCommands>...>;

    template<SC Name,
             SC Description,
             SC Type,
             Getter Get,
             Setter Set,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    Param(Name,
          Description,
          Type,
          Get &&get,
          Set &&set,
          Parse &&parse,
          Format &&format,
          Validate &&validate,
          const std::tuple<SubCommands...> &cmds)
      -> Param<std::decay_t<Name>,
               std::decay_t<Description>,
               std::decay_t<Type>,
               std::decay_t<Get>,
               std::decay_t<Set>,
               std::decay_t<Parse>,
               std::decay_t<Format>,
               std::decay_t<Validate>,
               std::decay_t<SubCommands>...>;

    template<SC Name,
             SC Description,
             SC Type,
             class MemberPointer,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    struct MemberData {
      using char_type = get_char_t<Name>;
      MemberPointer f;
      std::tuple<SubCommands...> subcommands;
      CLI_NO_UNIQUE_ADDRESS Parse parse;
      CLI_NO_UNIQUE_ADDRESS Format format;
      CLI_NO_UNIQUE_ADDRESS Validate validate;

      template<parse::Parser Parse_,
               format::Formatter Format_,
               validate::Validator Validate_,
               concepts::Command... SubCommands_>
      constexpr MemberData(Name,
                           Description,
                           Type,
                           MemberPointer f,
                           Parse_ &&parse,
                           Format_ &&format,
                           Validate_ &&validate,
                           SubCommands_ &&...cmds)
        : f(f),
          subcommands(std::forward<SubCommands>(cmds)...),
          parse(std::forward<Parse_>(parse)),
          format(std::forward<Format_>(format)),
          validate(std::forward<Validate_>(validate)) {}
    };

    template<SC Name,
             SC Description,
             SC Help,
             class MemberPointer,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate>
    struct MemberData<Name,
                      Description,
                      Help,
                      MemberPointer,
                      Parse,
                      Format,
                      Validate> {
      using char_type = get_char_t<Name>;
      MemberPointer f;
      CLI_NO_UNIQUE_ADDRESS Parse parse;
      CLI_NO_UNIQUE_ADDRESS Format format;
      CLI_NO_UNIQUE_ADDRESS Validate validate;

      template<parse::Parser Parse_,
               format::Formatter Format_,
               validate::Validator Validate_>
      constexpr MemberData(Name,
                           Description,
                           Help,
                           MemberPointer f,
                           Parse_ &&parse,
                           Format_ &&format,
                           Validate_ &&validate)
        : f(f),
          parse(std::forward<Parse_>(parse)),
          format(std::forward<Format_>(format)),
          validate(std::forward<Validate_>(validate)) {}
    };

    template<SC Name,
             SC Description,
             SC Help,
             class MemberPointer,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    MemberData(Name,
               Description,
               Help,
               MemberPointer,
               Parse &&,
               Format &&,
               Validate &&,
               SubCommands &&...) -> MemberData<std::decay_t<Name>,
                                                std::decay_t<Description>,
                                                std::decay_t<Help>,
                                                MemberPointer,
                                                std::decay_t<Parse>,
                                                std::decay_t<Format>,
                                                std::decay_t<Validate>,
                                                std::decay_t<SubCommands>...>;

    template<typename T>
    inline constexpr bool is_member_data_v = false;
    template<class Name,
             class Description,
             class Help,
             class MemberPointer,
             class Parse,
             class Format,
             class Validate,
             class... SubCommands>
    inline constexpr bool is_member_data_v<MemberData<Name,
                                                      Description,
                                                      Help,
                                                      MemberPointer,
                                                      Parse,
                                                      Format,
                                                      Validate,
                                                      SubCommands...>> = true;

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

    template<typename T>
    struct DefaultGet {
      const T *value_;
      constexpr DefaultGet(const T &v)
        : value_(&v) {}
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

    template<typename T>
    struct DefaultSet {
      T *value_;
      constexpr DefaultSet(T &v)
        : value_(&v) {}
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

    template<typename T, typename MemberPtr>
    struct MemDataGet {
      const T &value_;
      MemberPtr member;
      constexpr Error operator()(mem_data_type<MemberPtr> &t) {
        t = (value_.*member);
        return Error::none;
      }
    };

    template<typename T, typename MemberPtr>
    struct MemDataSet {
      T &value_;
      MemberPtr member;
      constexpr Error operator()(const mem_data_type<MemberPtr> &t) {
        value_.*member = t;
        return Error::none;
      }
    };

    template<typename T, typename MemberPtr>
    struct MemDataSet<const T, MemberPtr> {
      constexpr Error operator()(const mem_data_type<MemberPtr> &) {
        return Error::cant_set_param;
      }
    };

    template<typename T>
    struct InvalidGet {
      cli::Error operator()(T &) const { return cli::Error::cant_read_param; }
    };

    template<typename T>
    struct InvalidSet {
      cli::Error operator()(const T &) const {
        return cli::Error::cant_set_param;
      }
    };

    template<class T,
             SC Name,
             SC Description,
             SC Help,
             class MemberPointer,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    constexpr auto to_cmd(T &obj,
                          MemberData<Name,
                                     Description,
                                     Help,
                                     MemberPointer,
                                     Parse,
                                     Format,
                                     Validate,
                                     SubCommands...> member_data) {
      if constexpr (sizeof...(SubCommands) > 0)
        return Param{
          Name{},
          Description{},
          Help{},
          MemDataGet<T, MemberPointer>{obj, member_data.f},
          MemDataSet<T, MemberPointer>{obj, member_data.f},
          std::move(member_data.parse),
          std::move(member_data.format),
          std::move(member_data.validate),
          std::move(member_data.subcommands)
        };
      else
        return Param{
          Name{},
          Description{},
          Help{},
          MemDataGet<T, MemberPointer>{obj, member_data.f},
          MemDataSet<T, MemberPointer>{obj, member_data.f},
          std::move(member_data.parse),
          std::move(member_data.format),
          std::move(member_data.validate)
        };
    }

    template<class T,
             SC Name,
             SC Description,
             SC Help,
             class MemberPointer,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    constexpr auto to_cmd(const T &obj,
                          MemberData<Name,
                                     Description,
                                     Help,
                                     MemberPointer,
                                     Parse,
                                     Format,
                                     Validate,
                                     SubCommands...> member_data) {
      if constexpr (sizeof...(SubCommands) > 0)
        return Param{
          Name{},
          Description{},
          Help{},
          MemDataGet<T, MemberPointer>{obj, member_data.f},
          MemDataSet<const T, MemberPointer>{},
          std::move(member_data.parse),
          std::move(member_data.format),
          std::move(member_data.validate),
          std::move(member_data.subcommands)
        };
      else
        return Param{
          Name{},
          Description{},
          Help{},
          MemDataGet<T, MemberPointer>{obj, member_data.f},
          MemDataSet<const T, MemberPointer>{},
          std::move(member_data.parse),
          std::move(member_data.format),
          std::move(member_data.validate)
        };
    }

    template<class T, class CommandOrMemberDataOrMemberFunction>
    constexpr auto transform(T &obj,
                             CommandOrMemberDataOrMemberFunction &&mem) {
      if constexpr (concepts::Command<std::remove_cvref_t<
                      CommandOrMemberDataOrMemberFunction>>) {
        return mem;
      } else {
        using dtl::to_cmd;
        using funcs::dtl::to_cmd;
        return to_cmd(obj,
                      std::forward<CommandOrMemberDataOrMemberFunction>(mem));
      }
    }
  } // namespace dtl

  /**
   * @brief This concept is satisfied if T is a Command or a member data
   * command, or a member fucntion command.
   * @tparam T
   */
  template<typename T>
  concept CmdOrMemDataOrMemFun =
    concepts::Command<std::remove_cvref_t<T>> or
    dtl::is_member_data_v<std::remove_cvref_t<T>> or
    funcs::is_member_function_v<std::remove_cvref_t<T>>;

  /**
   * @brief This class can be used to set a parameter with a callback. See
   * set_cb for an example.
   *
   * @tparam Setter the original setter
   * @tparam Callback
   */
  template<typename Setter, Callable Callback>
  struct SetWithCallback {
    using value_type = setter_value_type_t<Setter>;
    Setter setter;
    Callback callback;

    static_assert(std::is_invocable_v<Callback, value_type>,
                  "The callback must be callable with the setter's value type");

    cli::Error operator()(const value_type &v) {
      if (auto err = setter(v); err != Error::none)
        return err;
      callback(v);
      return Error::none;
    }
  };

  /**
   * @brief creates a default setter with callback. The callback must take a T
   * as its first and only argument.
   *
   * @tparam T the paramter's type
   * @param t the object
   * @param callback the callback
   */
  template<typename T, Callable Callback>
  constexpr auto set_cb(T &t, Callback callback) {
    static_assert(std::is_invocable_v<Callback, T>,
                  "The callback must take a T as its argument");
    return SetWithCallback{dtl::DefaultSet<T>{&t},
                           std::forward<Callback>(callback)};
  }

  /**
   * @defgroup Parameters
   * @ingroup Commands
   *
   * Parameters are commands that represent a value.
   *
   * They can be set with:
   *
   * ```bash
   * parameter = value
   * ```
   *
   * and read with:
   *
   * ```bash
   * parameter
   * ```
   *
   * A Parameter is fully defined by a:
   * - name: the parameter name. Must be a cli::string_constant.
   * - description: the parameter description. Must be a cli::string_constant.
   * - set: a Setter, i.e. a callback the sets the parameter value.
   * - get: a Getter, i.e. callback that gets the parameter value.
   * - parse: a callable that parses the value from a string. See @ref Parsing.
   * - format: a callable that formats the value into a string. See @ref
   *   Formatting.
   * - validate: a callable that checks wether the parsed value is considered
   *   valid. See @ref Validation.
   * - subcommands: optional subcommands, which may be parameters or functions.
   *
   * There are four kinds of parameter categories:
   * 1. Parameters without object/variable declarations. These kinds of
   * parameters don't store their value in a variable, as far as CLI is
   * concerned. They give complete control regarding read and write access
   * of the values and is the most flexible. The drawback is that using these
   * requires more boilerplate. See @ref params-without-object.
   * 2. Parameters with object/variable declarations. These parameters store
   * their value in a variable. See @ref params-with-object and @ref
   * params-with-const-object.
   * 3. Member data parameters. These are subcommands of a parent parameter. See
   * @ref memdata and @ref const-memdata.
   * 4. Virtual parameters. These can't be read or written to, but act as a
   * grouping for sub commands. See @ref virtual-params.
   *
   * The second and third category reduce the boilerplate required of the first
   * category and provide sensible defaults.
   */

  /**
   * @defgroup virtual-params Virtual Parameters
   * @ingroup Parameters
   * @{
   */

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
  template<SC Name, SC Description, concepts::Command... SubCommands>
    requires(sizeof...(SubCommands) > 0)
  [[nodiscard]] constexpr auto
  param(Name name, Description description, SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return dtl::Param{Name{},
                      Description{},
                      string_constant<get_char_t<Name>>{},
                      dtl::NullGet{},
                      dtl::NullSet{},
                      parse::NullParse<get_char_t<Name>>{},
                      format::NullFormat<get_char_t<Name>>{},
                      validate::NullValidate{},
                      std::forward<SubCommands>(cmds)...};
  }

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
  template<SC Name, concepts::Command... SubCommands>
    requires(sizeof...(SubCommands) > 0)
  [[nodiscard]] constexpr auto param(Name name, SubCommands &&...cmds) {
    (void)name;
    return dtl::Param{Name{},
                      NoDescription<get_char_t<Name>>{},
                      string_constant<get_char_t<Name>>{},
                      dtl::NullGet{},
                      dtl::NullSet{},
                      parse::NullParse<get_char_t<Name>>{},
                      format::NullFormat<get_char_t<Name>>{},
                      validate::NullValidate{},
                      std::forward<SubCommands>(cmds)...};
  }
  /// @}

  // clang-format off
  /**
   * @defgroup params-without-object Paramters Without Object/Variable Declarations 
   * @ingroup Parameters
   *
   * Parameter commands without an object/variable declaration can be setup
   * with the following functions.
   *
   * The basic form is:
   *
   * ```
   * param<T>(name, description, get, set, parse, format, validate, subcommands...);
   * ```
   *
   * The parts have the following functions:
   * - T: the parameter's type
   * - name: a string_constant that makes up the command name
   * - description: a string_constant that describes the command
   * - get: a Getter for a T. It retrieves the value associated with the parameter.
   * - set: a Setter for a T. It sets the value associated with the parameter.
   * - parse: a Parser for a T. It parses a T from a string. 
   *   See also @ref Parsing, cli::parse::Parser and cli::parse::ParserOf.
   * - format: a Formatter for a T. It formats a T to a string. 
   *   See also @ref Formatting, cli::format::Formatter and cli::format::FormatterOf.
   * - validate: a Validator of T. See @ref Validation.
   *
   * There are a multitide of overloads so that certain parts can be left out,
   * if you wish to use the defaults provided by cli.
   *
   * The available overloads are:
   *
   * ```
   * // the basic form
   * param<T>(name, description, get, set, parse, format, validate);
   *
   * // a parameter with default validator
   * param<T>(name, description, get, set, parse, format);
   *
   * // a parameter with default parser and formatter
   * param<T>(name, description, get, set, validate);
   *
   * // a write-only parameter with custom parser and validator
   * param<T>(name, description, set, parse, validate);
   *
   * // default parser, formatter and validator are used
   * param<T>(name, description, get, set);
   *
   * // a write-only parameter with custom parser
   * param<T>(name, description, set, parse);
   *
   * // a read-only parameter with custom formatter
   * param<T>(name, description, get, format);
   *
   * // a write-only parameter with custom validator
   * param<T>(name, description, set, validate);
   *
   * // a read-only parameter
   * param<T>(name, description, get);
   *
   * // a write-only parameter
   * param<T>(name, description, set);
   * ```
   * @{
   */
  // clang-format on

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   * bool validate_i(int i){}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    get_i,
   *                                    set_i,
   *                                    parse_i,
   *                                    format_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     Get &&get,
                                     Set &&set,
                                     Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return dtl::Param{Name{},
                      Description{},
                      ctti::name<T, typename Name::char_type>(),
                      std::forward<Get>(get),
                      std::forward<Set>(set),
                      std::forward<Parse>(parse),
                      std::forward<Format>(format),
                      std::forward<Validate>(validate),
                      std::forward<SubCommands>(cmds)...};
  }

  /**
   * creates a parameter command from its individual parts. The default
   * validator is used.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    get_i,
   *                                    set_i,
   *                                    parse_i,
   *                                    format_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     Get &&get,
                                     Set &&set,
                                     Parse &&parse,
                                     Format &&format,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    std::forward<Get>(get),
                    std::forward<Set>(set),
                    std::forward<Parse>(parse),
                    std::forward<Format>(format),
                    validate::DefaultValidate<T>{},
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with a cusotm validator.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * bool validate_i(int i){}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    get_i,
   *                                    set_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           GetterOf<T> Get,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       Get &&get,
                                                       Set &&set,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    std::forward<Get>(get),
                    std::forward<Set>(set),
                    parse::Parse<T, get_char_t<Name>>{},
                    format::Format<T, get_char_t<Name>>{},
                    std::forward<Validate>(validate),
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a write-only parameter command with custom parser and validator.
   *
   * Usage:
   * ```
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * bool validate_i(int i){}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    set_i,
   *                                    parse_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       Set &&set,
                                                       Parse &&parse,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    dtl::InvalidGet<T>{},
                    std::forward<Set>(set),
                    std::forward<Parse>(parse),
                    format::NoFormat<T, get_char_t<Name>>{},
                    std::forward<Validate>(validate),
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with custom formatter.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   * bool validate_i(int i){}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    get_i,
   *                                    format_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       Get &&get,
                                                       Format &&format,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    std::forward<Get>(get),
                    dtl::InvalidSet<T>{},
                    parse::NoParse<T, get_char_t<Name>>{},
                    std::forward<Format>(format),
                    std::forward<Validate>(validate),
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default parser,
   * formatter and validator are used
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    get_i,
   *                                    set_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           GetterOf<T> Get,
           SetterOf<T> Set,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       Get &&get,
                                                       Set &&set,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    std::forward<Get>(get),
                    std::forward<Set>(set),
                    parse::Parse<T, get_char_t<Name>>{},
                    format::Format<T, get_char_t<Name>>{},
                    validate::DefaultValidate<T>{},
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a write-only parameter command with custom parser.
   *
   * Usage:
   * ```
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    set_i,
   *                                    parse_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       Set &&set,
                                                       Parse &&parse,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    dtl::InvalidGet<T>{},
                    std::forward<Set>(set),
                    std::forward<Parse>(parse),
                    format::NoFormat<T, get_char_t<Name>>{},
                    validate::DefaultValidate<T>{},
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with custom formatter.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    get_i,
   *                                    format_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       Get &&get,
                                                       Format &&format,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    std::forward<Get>(get),
                    dtl::InvalidSet<T>{},
                    parse::NoParse<T, get_char_t<Name>>{},
                    std::forward<Format>(format),
                    validate::DefaultValidate<T>{},
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a write-only parameter command with a default parser and custom
   * validator.
   *
   * Usage:
   * ```
   * cli::Error set_i(int i){...}
   * bool validate_i(int i){}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    set_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     Set &&set,
                                     Validate &&validate,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    dtl::InvalidGet<T>{},
                    std::forward<Set>(set),
                    parse::Parse<T, get_char_t<Name>>{},
                    format::NoFormat<T, get_char_t<Name>>{},
                    std::forward<Validate>(validate),
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    get_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           GetterOf<T> Get,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Description description, Get &&get, SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    std::forward<Get>(get),
                    dtl::InvalidSet<T>{},
                    parse::NoParse<T, get_char_t<Name>>{},
                    format::Format<T, get_char_t<Name>>{},
                    validate::DefaultValidate<T>{},
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error set_i(int i){...}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    set_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           SC Name,
           SC Description,
           SetterOf<T> Set,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Description description, Set &&set, SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param<T>(Name{},
                    Description{},
                    dtl::InvalidGet<T>{},
                    std::forward<Set>(set),
                    parse::Parse<T, get_char_t<Name>>{},
                    format::NoFormat<T, get_char_t<Name>>{},
                    validate::DefaultValidate<T>{},
                    std::forward<SubCommands>(cmds)...);
  }
  /// @}

  //clang-format off
  /**
   * @defgroup params-with-object Parameters With Object/Variable Declarations
   * @ingroup Parameters
   *
   * The following functions can be used to setup parameters with
   * object/variable declarations.
   *
   * The basic form is:
   *
   * ```
   * param(name, description, t, get, set, parse, format, validate,
   * subcommands...);
   * ```
   *
   * The parts have the following functions:
   * - name: a string_constant that makes up the command name
   * - description: a string_constant that describes the command
   * - t: the variable
   * - get: a Getter for a T. It retrieves the value associated with the
   * parameter.
   * - set: a Setter for a T. It sets the value associated with the parameter.
   * - parse: a Parser for a T. It parses a T from a string.
   *   See also @ref Parsing, cli::parse::Parser and cli::parse::ParserOf.
   * - format: a Formatter for a T. It formats a T to a string.
   *   See also @ref Formatting, cli::format::Formatter and
   * cli::format::FormatterOf. cli::format::Formatter and
   * cli::format::FormatterOf.
   * - validate: a Validator for a T. It validates parsed values before they are
   * set. See also cli::validate::Validator.
   *
   * There are a multitide of overloads so that certain parts can be left out,
   * if you wish to use the defaults provided by cli.
   *
   * The parts that can be left out are:
   * - get: in that case, cli::param::DefaultGet is used.
   * - set: in that case, cli::param::DefaultSet is used.
   * - validate: in that case, cli::validate::DefaultValidate is used.
   * - parse and format: in that case, cli::parse::Parse and
   *   cli::format::Format are used.
   * @{
   */
  // clang-format on

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   * bool validate_i(int i){}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i,
   *                               set_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       Set &&set,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return dtl::Param{Name{},
                      Description{},
                      ctti::name<T>(),
                      std::forward<Get>(get),
                      std::forward<Set>(set),
                      std::forward<Parse>(parse),
                      std::forward<Format>(format),
                      std::forward<Validate>(validate),
                      dtl::transform(t, std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   * static int i;
   * auto par = cli::params::param<int>("i"_sc,
   *                                    "a description"_sc,
   *                                    i,
   *                                    get_i,
   *                                    set_i,
   *                                    parse_i,
   *                                    format_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       Set &&set,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 std::forward<Set>(set),
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default parser
   * and formatter are used.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * bool validate_i(int i){}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i,
   *                               set_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used to validate a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       Set &&set,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 std::forward<Set>(set),
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<Validate>(validate),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default getter
   * is used. Usage:
   * ```
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){...}
   * bool validate_i(int i){...}
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               set_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Set &&set,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 std::forward<Set>(set),
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 std::forward<Validate>(validate),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default setter
   * is used. Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){...}
   * bool validate_i(int i){...}
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 dtl::DefaultSet<T>{t},
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 std::forward<Validate>(validate),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                                    "a description"_sc,
   *                                    i,
   *                                    get_i,
   *                                    set_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       Set &&set,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 std::forward<Set>(set),
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   * ```
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               set_i,
   *                               parse_i,
   *                               format_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Set &&set,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 std::forward<Set>(set),
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i,
   *                               parse_i,
   *                               format_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 dtl::DefaultSet<T>{t},
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom setter and validator.
   *
   * Usage:
   * ```
   * cli::Error set_i(int i){...}
   * bool validate_i(int i){...}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               set_i,
   *                               validate_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Set &&set,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 std::forward<Set>(set),
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<Validate>(validate),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * bool validate_i(int i){}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i,
   *                               validate_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 dtl::DefaultSet<T>{t},
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<Validate>(validate),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   * bool validate_i(int i){}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                                    "a description"_sc,
   *                                    i,
   *                                    parse_i,
   *                                    format_i,
   *                                    validate_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 dtl::DefaultSet<T>{t},
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 std::forward<Validate>(validate),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& i){...}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Get &&get,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 dtl::DefaultSet<T>{t},
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& i){...}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Set &&set,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 std::forward<Set>(set),
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom parser and formatter.
   * Usage:
   * ```
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){...}
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               set_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Parse &&parse,
                                                       Format &&format,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 dtl::DefaultSet<T>{t},
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom validator.
   *
   * Usage:
   * ```
   * bool validate_i(int i){}
   *
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                                    "a description"_sc,
   *                                    i,
   *                                    validate_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       T &t,
                                                       Validate &&validate,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 dtl::DefaultSet<T>{t},
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<Validate>(validate),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * Usage:
   * ```
   * static int i;
   * auto par = cli::params::param("i"_sc,
   *                                    "a description"_sc,
   *                                    i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Description description, T &t, SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 dtl::DefaultSet<T>{t},
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }
  /// @}

  // clang-format off
  /**
   * @defgroup params-with-const-object Parameters With const Object/Variable Declarations 
   * @ingroup Parameters
   * Read-only parameters for const objects can be defined with the
   * following functions.
   *
   * The base form is:
   * ```
   * param(name, description, t, get, format)
   * ```
   *
   * In total there are four overloads, where get, or format, or both, can be left
   * out. In that case, a default getter or default formatter are used.
   * @{
  */
  // clang-format on

  /**
   * creates a read-only parameter command with a custom getter and formatter.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   *
   * static const int i = 5;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i,
   *                               format_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       const T &t,
                                                       Get &&get,
                                                       Format &&format,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return dtl::Param{Name{},
                      Description{},
                      ctti::name<T>(),
                      std::forward<Get>(get),
                      dtl::InvalidSet<T>{},
                      parse::NoParse<T, get_char_t<Name>>{},
                      std::forward<Format>(format),
                      validate::DefaultValidate<T>{},
                      dtl::transform(t, std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a read-only parameter command with default formatter.
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   *
   * static const int i = 5;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               get_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       const T &t,
                                                       Get &&get,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 std::forward<Get>(get),
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with custom formatter.
   *
   * Usage:
   * ```
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   *
   * static const int i = 5;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i,
   *                               format_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(Name name,
                                                       Description description,
                                                       const T &t,
                                                       Format &&format,
                                                       SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 std::forward<Format>(format),
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with default formatter and getter.
   *
   * Usage:
   * ```
   * static const int i = 5;
   * auto par = cli::params::param("i"_sc,
   *                               "a description"_sc,
   *                               i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<SC Name,
           SC Description,
           typename T,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Description description, const T &t, SubCommands &&...cmds) {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<SubCommands>(cmds)...);
  }
  /// @}

  //
  // /**
  //  * creates a parameter command from its individual parts. The default
  //  validator
  //  * is used.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param get the getter of the parameter. See cli::params::Getter for
  //  * additional info.
  //  * @param parse the parser used to parse a T
  //  * @param format the Formatter. Used to format a T. See also
  //  cli::format::Formatter.
  //  * @param validate the validator used when parsing a T
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, typename T, GetterOf<T> Get,
  //           parse::ParserOf<T, get_char_t<Name>> Parse,
  //           format::FormatterOf<T, get_char_t<Name>> Format,
  //           concepts::Command... SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Get &&get, Parse
  // &&parse, Format
  // &&format,
  //                      SubCommands &&...cmds) {
  //   (void)name;
  //   using namespace dtl;
  //   return Param{Name{},
  //                string_constant<get_char_t<Name>>{},
  //                ctti::name<T>(),
  //                std::forward<Get>(get),
  //                DefaultSet<T>{t},
  //                std::forward<Parse>(parse),
  //                std::forward<Format>(format),
  //                validate::DefaultValidate<T>{},
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command from its individual parts.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param set the setter of the parameter. See cli::params::Setter for
  //  * additional info.
  //  * @param parse the parser used to parse a T
  //  * @param format the Formatter. Used to format a T. See also
  //  cli::format::Formatter.
  //  * @param validate the validator used when parsing a T
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, typename T, SetterOf<T> Set,
  //           parse::ParserOf<T, get_char_t<Name>> Parse,
  //           format::FormatterOf<T, get_char_t<Name>> Format,
  //           validate::ValidatorOf<T> Validate, concepts::Command...
  //           SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Set &&set, Parse
  // &&parse, Format
  // &&format,
  //                      Validate &&validate, SubCommands &&...cmds) {
  //   (void)name;
  //   using namespace dtl;
  //   return Param{Name{},
  //                string_constant<get_char_t<Name>>{},
  //                ctti::name<T>(),
  //                DefaultGet<T>{t},
  //                std::forward<Set>(set),
  //                std::forward<Parse>(parse),
  //                std::forward<Format>(format),
  //                std::forward<Validate>(validate),
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command from its individual parts. The default
  //  validator
  //  * is used.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param set the setter of the parameter. See cli::params::Setter for
  //  * additional info.
  //  * @param parse the parser used to parse a T
  //  * @param format the Formatter. Used to format a T. See also
  //  cli::format::Formatter.
  //  * @param validate the validator used when parsing a T
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, typename T, SetterOf<T> Set,
  //           parse::ParserOf<T, get_char_t<Name>> Parse,
  //           format::FormatterOf<T, get_char_t<Name>> Format,
  //           concepts::Command... SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Set &&set, Parse
  // &&parse, Format
  // &&format,
  //                      SubCommands &&...cmds) {
  //   (void)name;
  //   using namespace dtl;
  //   return Param{Name{},
  //                string_constant<get_char_t<Name>>{},
  //                ctti::name<T>(),
  //                DefaultGet<T>{t},
  //                std::forward<Set>(set),
  //                std::forward<Parse>(parse),
  //                std::forward<Format>(format),
  //                validate::DefaultValidate<T>{},
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command from its individual parts. The default
  //  parser and
  //  * formatter are used.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param description the parameter description, used by the help
  //  functionality.
  //  * Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param set the setter of the parameter. See cli::params::Setter for
  //  * additional info.
  //  * @param validate the validator used to validate a T
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, SC Description, typename T, GetterOf<T> Get, SetterOf<T>
  // Set,
  //           validate::ValidatorOf<T> Validate, concepts::Command...
  //           SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, Description description, T
  // &t, Set &&set,
  //                      Validate &&validate, SubCommands &&...cmds) {
  //   (void)name;
  //   (void)description;
  //   using namespace dtl;
  //   using Char = get_char_t<Name>;
  //   return Param{Name{},
  //                Description{},
  //                ctti::name<T>(),
  //                DefaultGet<T>{},
  //                std::forward<Set>(set),
  //                parse::Parse<T, Char>{},
  //                format::Format<T, Char>{},
  //                std::forward<Validate>(validate),
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command from its individual parts.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param get the getter of the parameter. See cli::params::Getter for
  //  * additional info.
  //  * @param parse the parser used to parse a T
  //  * @param format the Formatter. Used to format a T. See also
  //  cli::format::Formatter.
  //  * @param validate the validator used when parsing a T
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, typename T,
  //           parse::ParserOf<T, get_char_t<Name>> Parse,
  //           format::FormatterOf<T, get_char_t<Name>> Format,
  //           concepts::Command... SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Parse &&parse, Format
  // &&format,
  //                      SubCommands &&...cmds) {
  //   (void)name;
  //   using namespace dtl;
  //   return Param{Name{},
  //                string_constant<get_char_t<Name>>{},
  //                ctti::name<T>(),
  //                DefaultGet<T>{t},
  //                DefaultSet<T>{t},
  //                std::forward<Parse>(parse),
  //                std::forward<Format>(format),
  //                validate::DefaultValidate<T>{},
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command. The value of the parameter, i.e. t, can
  //  * then be retrieved by its name. This uses the default getter, setter,
  //  parsing,
  //  * formatting, and validation facilities.
  //  *
  //  * Example:
  //  *
  //  * ```
  //  * extern T my_var;
  //  *
  //  * auto p = param("my-var"_sc, "how much to foo"_sc, my_var, ...);
  //  * ```
  //  *
  //  * @tparam T the parameters type
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param description the description of the parameter. Must be a
  //  * cli::string_constant.
  //  * @param t the parameter value
  //  * @param cmds optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, SC Description, class T, concepts::Command...
  // SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, Description description, T
  // &t,
  //                      SubCommands &&...cmds) {
  //   (void)name;
  //   (void)description;
  //   using namespace dtl;
  //   return Param{
  //       Name{},
  //       Description{},
  //       cli::ctti::name<std::remove_cvref_t<T>, get_char_t<Name>>(),
  //       DefaultGet<std::remove_cvref_t<T>>{t},
  //       DefaultSet<std::remove_cvref_t<T>>{t},
  //       parse::Parse<std::remove_cvref_t<T>, typename
  //       Name::char_type>(), format::Format<std::remove_cvref_t<T>,
  //       get_char_t<Name>>(),
  //       validate::DefaultValidate<std::remove_cvref_t<T>>{},
  //       std::forward<SubCommands>(cmds)...};
  // }
  //
  // /// @}
  //
  // /**
  //  * creates a parameter command from its individual parts.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param get the getter of the parameter. See cli::params::Getter for
  //  * additional info.
  //  * @param set the setter of the parameter. See cli::params::Setter for
  //  * additional info.
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, typename T, GetterOf<T> Get, SetterOf<T> Set,
  //           concepts::Command... SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Get &&get, Set &&set,
  //                      SubCommands &&...cmds) {
  //   (void)name;
  //   using namespace dtl;
  //   using Char = get_char_t<Name>;
  //   return Param{Name{},
  //                string_constant<Char>{},
  //                ctti::name<T>(),
  //                std::forward<Get>(get),
  //                std::forward<Set>(set),
  //                parse::Parse<T, Char>{},
  //                format::Format<T, Char>{},
  //                validate::DefaultValidate<T>{},
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command from its individual parts.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param description the parameter description, used by the help
  //  functionality.
  //  * Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param set the setter of the parameter. See cli::params::Setter for
  //  * additional info.
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, typename T, SetterOf<T> Set, concepts::Command...
  // SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Set &&set, SubCommands
  // &&...cmds) {
  //   (void)name;
  //   using namespace dtl;
  //   using Char = get_char_t<Name>;
  //   return Param{Name{},
  //                string_constant<Char>{},
  //                ctti::name<T>(),
  //                DefaultGet<T>{t},
  //                std::forward<Set>(set),
  //                parse::Parse<T, Char>{},
  //                format::Format<T, Char>{},
  //                validate::DefaultValidate<T>{},
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command from its individual parts.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param description the parameter description, used by the help
  //  functionality.
  //  * Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param get the getter of the parameter. See cli::params::Getter for
  //  * additional info.
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, typename T, GetterOf<T> Get, concepts::Command...
  // SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Get &&get, SubCommands
  // &&...cmds) {
  //   (void)name;
  //   using namespace dtl;
  //   using Char = get_char_t<Name>;
  //   return Param{Name{},
  //                string_constant<Char>{},
  //                ctti::name<T>(),
  //                std::forward<Get>(get),
  //                DefaultSet<T>(t),
  //                parse::Parse<T, Char>{},
  //                format::Format<T, Char>{},
  //                validate::DefaultValidate<T>{},
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command. The value of the parameter, i.e. t, can
  //  * then be retrieved by its name. This uses the default getter, setter,
  //  parsing,
  //  * formatting, and validation facilities.
  //  *
  //  * Example:
  //  *
  //  * ```
  //  * extern T my_var;
  //  *
  //  * auto p = param("my-var"_sc, my_var, ...);
  //  * ```
  //  *
  //  * @tparam T the parameters type
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param t the parameter value
  //  * @param cmds optional subcommands
  //  * @return a Command
  //  */
  // // template <SC Name, class T, concepts::Command... SubCommands>
  // //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // // [[nodiscard]] constexpr auto param(Name name, T &t, SubCommands
  // &&...cmds) {
  // //   (void)name;
  // //   using namespace dtl;
  // //   return Param{
  // //       Name{},
  // //       NoDescription<get_char_t<Name>>{},
  // //       cli::ctti::name<std::remove_cvref_t<T>, get_char_t<Name>>(),
  // //       DefaultGet<std::remove_cvref_t<T>>{t},
  // //       DefaultSet<std::remove_cvref_t<T>>{t},
  // //       parse::Parse<std::remove_cvref_t<T>, typename
  // //       Name::char_type>(), format::Format<std::remove_cvref_t<T>,
  // //       get_char_t<Name>>(),
  // //       validate::DefaultValidate<std::remove_cvref_t<T>>{},
  // //       std::forward<SubCommands>(cmds)...};
  // // }
  //
  // /**
  //  * creates a parameter command from its individual parts.
  //  *
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param description the parameter description, used by the help
  //  functionality.
  //  * Must be a cli::string_constant.
  //  * @param type the parameter type as a string, used by the help
  //  functionality.
  //  * Must be a cli::string_constant.
  //  * @param get the getter of the parameter. See cli::params::Getter for
  //  * additional info.
  //  * @param set the setter of the parameter. See cli::params::Setter for
  //  * additional info.
  //  * @param parse the parser of the parameter. See cli::parse::Parser for
  //  * additional info.
  //  * @param format the formatter of the parameter. See cli::format::Formatter
  //  for
  //  * additional info.
  //  * @param validate the validator of the parameter. See
  //  cli::validate::Validator
  //  * for additional info.
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <SC Name, SC Description, SC Type, Getter Get, Setter Set,
  //           parse::Parser Parse, format::Formatter Format,
  //           validate::Validator Validate, concepts::Command... SubCommands>
  // [[nodiscard]] constexpr auto param(Name name, Description description, Type
  // type, Get
  // &&get,
  //                      Set &&set, Parse &&parse, Format &&format,
  //                      Validate &&validate, SubCommands &&...cmds) {
  //   (void)name;
  //   (void)description;
  //   (void)type;
  //   using namespace dtl;
  //   return Param{Name{},
  //                Description{},
  //                Type{},
  //                std::forward<Get>(get),
  //                std::forward<Set>(set),
  //                std::forward<Parse>(parse),
  //                std::forward<Format>(format),
  //                std::forward<Validate>(validate),
  //                std::forward<SubCommands>(cmds)...};
  // }
  //
  // /**
  //  * creates a parameter command. The value of the parameter, i.e. t, can
  //  * then be retrieved by its name. This opverload can take member data and
  //  member
  //  * functions in addition to sub commands. Uses the default getter, setter,
  //  * parsing, formatting, and validation facilities.
  //  *
  //  * @tparam T the parameters type
  //  * @tparam CommandOrMemberDataOrMemberFunction
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param obj the parameter value and the object that member data and
  //  functions
  //  * are called on.
  //  * @param m an assortment of sub commands, member functions, and member
  //  data
  //  * @return
  //  */
  // template <SC Name, class T, class... CommandOrMemberDataOrMemberFunction>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &obj,
  //                      CommandOrMemberDataOrMemberFunction &&...m) {
  //   (void)name;
  //   using namespace dtl;
  //   return Param{
  //       Name{},
  //       NoDescription<get_char_t<Name>>{},
  //       cli::ctti::name<std::remove_cvref_t<T>, get_char_t<Name>>(),
  //       DefaultGet<T>{obj},
  //       DefaultSet<T>{obj},
  //       parse::Parse<T, get_char_t<Name>>{},
  //       format::Format<T, get_char_t<Name>>{},
  //       validate::DefaultValidate<std::remove_cvref_t<T>>{},
  //       dtl::transform(obj,
  //                      std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
  // }
  // /**
  //  * creates a parameter command. The value of the parameter, i.e. t, can
  //  * then be retrieved by its name. This overload can take member data and
  //  member
  //  * functions in addition to sub commands. Uses the default getter, setter,
  //  * parsing, formatting, and validation facilities.
  //  *
  //  * @tparam T the parameters type
  //  * @tparam CommandOrMemberDataOrMemberFunction
  //  * @param name the name of the parameter. Must be a cli::string_constant.
  //  * @param description the description of the parameter. Must be a
  //  * cli::string_constant.
  //  * @param obj the parameter value and the object that member data and
  //  functions
  //  * are called on.
  //  * @param m an assortment of sub commands, member functions, and member
  //  data
  //  * @return
  //  */
  // template <SC Name, SC Description, class T,
  //           class... CommandOrMemberDataOrMemberFunction>
  // [[nodiscard]] constexpr auto param(Name name, Description description, T
  // &obj,
  //                      CommandOrMemberDataOrMemberFunction &&...m) {
  //   (void)name;
  //   (void)description;
  //   using namespace dtl;
  //   return Param{
  //       Name{},
  //       Description{},
  //       cli::ctti::name<std::remove_cvref_t<T>, get_char_t<Name>>(),
  //       DefaultGet<T>{obj},
  //       DefaultSet<T>{obj},
  //       parse::Parse<T, get_char_t<Name>>{},
  //       format::Format<T, get_char_t<Name>>{},
  //       validate::DefaultValidate<std::remove_cvref_t<T>>{},
  //       dtl::transform(obj,
  //                      std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
  // }
  //
  // /**
  //  * creates a parameter command. The value of the parameter, i.e. Obj, can
  //  * then be retrieved by its name, which is deduced. This overload can take
  //  * member data and member functions in addition to sub commands. Uses the
  //  * default getter, setter, parsing, formatting, and validation facilities.
  //  *
  //  * Example:
  //  * ```
  //  * struct Settings{
  //  *   int k;
  //  *   ...
  //  * };
  //  * static Settings settings{...};
  //  * // p has the name "settings"
  //  * auto p = param<settings>(...);
  //  * ```
  //  * @tparam Obj a reference to an aggrate.
  //  * @tparam CommandOrMemberDataOrMemberFunction
  //  * @param m an assortment of sub commands, member functions, and member
  //  data
  //  * @return a Command
  //  */
  // template <auto &Obj, class... CommandOrMemberDataOrMemberFunction>
  // [[nodiscard]] constexpr auto param(CommandOrMemberDataOrMemberFunction
  // &&...m) {
  //   using T = std::remove_cvref_t<decltype(Obj)>;
  //   return dtl::Param{
  //       ctti::object_name<Obj>(),
  //       NoDescription<char>{},
  //       cli::ctti::name<T>(),
  //       dtl::DefaultGet<T>{Obj},
  //       dtl::DefaultSet<T>{Obj},
  //       parse::Parse<T, char>{},
  //       format::Format<T, char>{},
  //       validate::DefaultValidate<std::remove_cvref_t<T>>{},
  //       dtl::transform(Obj,
  //                      std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
  // }
  // /**
  //  * creates a parameter command. The value of the parameter, i.e. Obj, can
  //  * then be retrieved by its name, which is deduced. This overload can take
  //  * member data and member functions in addition to sub commands. Uses the
  //  * default getter, setter, parsing, formatting, and validation facilities.
  //  *
  //  * Example:
  //  * ```
  //  * struct Settings{
  //  *   int k;
  //  *   ...
  //  * };
  //  * static Settings settings{...};
  //  * // p has the name "settings"
  //  * auto p = param<settings>("global app settings"_sc, ...);
  //  * ```
  //  * @tparam Obj a reference to an aggrate.
  //  * @tparam CommandOrMemberDataOrMemberFunction
  //  * @param description the description of the parameter. Must be a
  //  * cli::string_constant.
  //  * @param m an assortment of sub commands, member functions, and member
  //  data
  //  * @return a Command
  //  */
  // template <auto &Obj, SC Description,
  //           class... CommandOrMemberDataOrMemberFunction>
  // [[nodiscard]] constexpr auto param(Description description,
  //                      CommandOrMemberDataOrMemberFunction &&...m) {
  //   (void)description;
  //   using namespace dtl;
  //   using T = std::remove_cvref_t<decltype(Obj)>;
  //   return Param{
  //       ctti::object_name<Obj, typename Description::char_type>(),
  //       Description{},
  //       cli::ctti::name<T, typename Description::char_type>(),
  //       DefaultGet<T>{Obj},
  //       DefaultSet<T>{Obj},
  //       parse::Parse<T, typename Description::char_type>{},
  //       format::Format<T, typename Description::char_type>{},
  //       validate::DefaultValidate<std::remove_cvref_t<T>>{},
  //       dtl::transform(Obj,
  //                      std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
  // }
  //
  // /**
  //  * @brief
  //  *
  //  * @tparam T
  //  * @tparam CommandOrMemberDataOrMemberFunction
  //  * @param obj
  //  * @param m
  //  * @return
  //  */
  // template <class T, CmdOrMemDataOrMemFun...
  // CommandOrMemberDataOrMemberFunction>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(T &obj,
  // CommandOrMemberDataOrMemberFunction &&...m) {
  //   using namespace dtl;
  //   return Param{
  //       cli::to_lower(cli::ctti::name<std::remove_cvref_t<T>>()),
  //       NoDescription<char>{},
  //       cli::ctti::name<std::remove_cvref_t<T>>(),
  //       DefaultGet<T>{obj},
  //       DefaultSet<T>{obj},
  //       parse::Parse<T, char>{},
  //       format::Format<T, char>{},
  //       validate::DefaultValidate<std::remove_cvref_t<T>>{},
  //       dtl::transform(obj,
  //                      std::forward<CommandOrMemberDataOrMemberFunction>(m))...};
  // }

  /**
   * @defgroup memdata Member Data
   * @ingroup Parameters
   *
   * Member data commands are used to easily setup subcommands for parameters
   * with subobjects.
   *
   * This is easiest explainable by example.
   * Take this struct and its variabe definition:
   *
   * ```
   *  struct Settings{
   *    int foo;
   *    char baz;
   *  };
   *
   *  static Settings settings;
   * ```
   *
   * To make the settings and its members foo and baz available to cli, you can
   * use the following functions to easily setup this structure.
   *
   * ```
   *  param("settings"_sc, "core Settings", settings,
   *          param("foo"_sc, "foo mode"_sc, &Settings::foo),
   *          param("baz"_sc, "baz setting"_sc, &Settings::baz));
   * ```
   *
   * Then ``settings``, ``settings.foo`` and ``settings.baz`` can be used as
   * parameter commands.
   *
   * The full list of member data parameter functions is:
   * ```
   *  param(name, description, ptr_to_member, parse, format, validate);
   *  param(name, description, ptr_to_member, parse, format);
   *  param(name, description, ptr_to_member, validate);
   *  param(name, description, ptr_to_member);
   * ```
   * @{
   */

  /**
   * @brief
   *
   * @tparam MemberPointer
   * @param name
   * @param description
   * @param f
   * @param parse
   * @param format
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds
   * @return
   */
  template<
    SC Name,
    SC Description,
    class MemberPointer,
    parse::ParserOf<mem_data_type<MemberPointer>, get_char_t<Name>> Parse,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    validate::ValidatorOf<mem_data_type<MemberPointer>> Validate,
    concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer f,
                                     Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    using namespace dtl;
    return MemberData{
      Name{},
      Description{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      f,
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief
   *
   * @tparam MemberPointer
   * @param name
   * @param description
   * @param f
   * @param parse
   * @param format
   * @param cmds
   * @return
   */
  template<
    SC Name,
    SC Description,
    class MemberPointer,
    parse::ParserOf<mem_data_type<MemberPointer>, get_char_t<Name>> Parse,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer f,
                                     Parse &&parse,
                                     Format &&format,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    using namespace dtl;
    return MemberData{
      Name{},
      Description{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      f,
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      validate::DefaultValidate<mem_data_type<MemberPointer>>{},
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief
   *
   * @tparam MemberPointer
   * @param name
   * @param description
   * @param f
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds
   * @return
   */
  template<SC Name,
           SC Description,
           class MemberPointer,
           validate::ValidatorOf<mem_data_type<MemberPointer>> Validate,
           concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             validate::ValidatorOf<Validate, mem_data_type<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer f,
                                     Validate &&validate,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    using namespace dtl;
    return MemberData{
      Name{},
      Description{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      f,
      cli::parse::Parse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      cli::format::Format<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * creates a member data subcommand. Must be used together with a parent
   * command.
   *
   * Example:
   * ```{cpp}
   *  struct S{
   *    int a;
   *  };
   *  static S s;
   *  auto cmd = param("s"_sc, s, mem_data("a"_sc, "a description"_sc, &S::a));
   * ```
   *
   * @param name the name of f. Must be a cli::string_constant.
   * @param f member data pointer
   * @param description the description of MemberPointer. Must be a
   * cli::string_constant.
   * @param cmds the subcommands
   */
  template<SC Name,
           SC Description,
           class MemberPointer,
           concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer f,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    using namespace dtl;
    using T = mem_data_type<MemberPointer>;
    if constexpr (std::is_const_v<T>) {
      return MemberData{
        Name{},
        Description{},
        cli::ctti::name<std::remove_const_t<T>, get_char_t<Name>>(),
        f,
        parse::NoParse<T, get_char_t<Name>>{},
        format::Format<T, get_char_t<Name>>{},
        validate::DefaultValidate<mem_data_type<MemberPointer>>{},
        std::forward<SubCommands>(cmds)...};
    } else
      return MemberData{
        Name{},
        Description{},
        cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
        f,
        parse::Parse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
        format::Format<mem_data_type<MemberPointer>, get_char_t<Name>>{},
        validate::DefaultValidate<mem_data_type<MemberPointer>>{},
        std::forward<SubCommands>(cmds)...};
  }

  /**
   * creates a member data subcommand. Must be used together with a parent
   * command.
   *
   * Example:
   * ```
   *  struct S{
   *    int a;
   *  };
   *  static S s;
   *  auto cmd = param("s"_sc, s, mem_data("a"_sc, &S::a));
   * ```
   *
   * @param name the name of f. Must be a cli::string_constant.
   * @param f member data pointer
   * @param cmds the subcommands
   */
  template<SC Name, class MemberPointer, concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto
  param(Name name, MemberPointer f, SubCommands &&...cmds) {
    (void)name;
    using namespace dtl;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 f,
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a member data subcommand. Must be used together with a parent
   * command. The name is deduced by cli.
   *
   * Example:
   * ```
   *  struct S{
   *    int a;
   *  };
   *  static S s;
   *  auto cmd = param("s"_sc, s, mem_data<&S::a>("a number"_sc));
   * ```
   * @tparam MemberPointer member data pointer
   * @param description the description of MemberPointer. Must be a
   * cli::string_constant.
   * @param cmds the subcommands
   */
  template<auto MemberPointer, SC Description, concepts::Command... SubCommands>
    requires std::is_member_pointer_v<
      std::remove_cvref_t<decltype(MemberPointer)>>
  [[nodiscard]] constexpr auto param(Description description,
                                     SubCommands &&...cmds) {
    (void)description;
    using namespace dtl;
    return param(
      ctti::value_name<MemberPointer, typename Description::char_type>(),
      Description{},
      MemberPointer,
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a member data subcommand. Must be used together with a parent
   * command. The name is deduced by cli.
   *
   * Example:
   * ```
   *  struct S{
   *    int a;
   *  };
   *  static S s;
   *  auto cmd = param("s"_sc, s, mem_data<&S::a>());
   * ```
   * @tparam MemberPointer member data pointer
   * @param cmds the subcommands
   */
  template<auto MemberPointer, concepts::Command... SubCommands>
    requires std::is_member_pointer_v<
      std::remove_cvref_t<decltype(MemberPointer)>>
  [[nodiscard]] constexpr auto param(SubCommands &&...cmds) {
    using namespace dtl;
    return param<MemberPointer>(NoDescription<char>{},
                                std::forward<SubCommands>(cmds)...);
  }

  /**
   * @}
   */

  /**
   * @defgroup const-memdata Const Member Data
   * @ingroup Parameters
   * Const member data commands are used to easily setup read-only subcommands
   * for parameters with objects.
   *
   * This is easiest explainable by example.
   * Take this struct and its variabe definition:
   *
   * ```
   *  struct Settings{
   *    int foo;
   *    char baz;
   *  };
   *
   *  static const Settings settings;
   * ```
   *
   * To make the settings and its members foo and baz available to cli, you can
   * use the following functions to easily setup this structure.
   *
   * ```
   *  param("settings"_sc, "core Settings", settings,
   *          param("foo"_sc, "foo mode"_sc, &Settings::foo),
   *          param("baz"_sc, "baz setting"_sc, &Settings::baz));
   * ```
   *
   * Then ``settings``, ``settings.foo`` and ``settings.baz`` can be used as
   * parameter commands.
   *
   * The full list of member data parameter functions is:
   * ```
   *  param(name, description, ptr_to_member, format);
   *  param(name, description, ptr_to_member);
   * ```
   * @{
   */

  /**
   * @brief
   *
   * @tparam MemberPointer
   * @param name
   * @param description
   * @param f
   * @param format
   * @param cmds
   * @return
   */
  template<
    SC Name,
    SC Description,
    class MemberPointer,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer f,
                                     Format &&format,
                                     SubCommands &&...cmds) {
    (void)name;
    (void)description;
    using namespace dtl;
    return MemberData{
      Name{},
      Description{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      f,
      parse::NoParse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      std::forward<Format>(format),
      validate::DefaultValidate<mem_data_type<MemberPointer>>{},
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @}
   */

} // namespace cli::params
#endif
