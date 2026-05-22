/**
 * @file "cli/param.hpp"
 *
 * This file contains the utilities to create parameters.
 */

#ifndef CLI_PARAM_HPP
#define CLI_PARAM_HPP

#include "cli/basic_format.hpp"
#include "cli/command.hpp"
#include "cli/concepts.hpp"
#include "cli/ctti.hpp"
#include "cli/enums.hpp"
#include "cli/function.hpp"
#include "cli/parse.hpp"
#include "cli/string.hpp"
#include "cli/tuple.hpp"
#include "cli/type_list.hpp"
#include "cli/util.hpp"
#include "cli/validator.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

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
    (std::is_lvalue_reference_v<T> and
     std::is_const_v<std::remove_reference_t<T>>) or
    std::is_same_v<T, std::remove_reference_t<T>>;

  /**
   * concept for a Getter with value type V.
   *
   * See [here](docs.md#getters) for more details.
   *
   * @ingroup Parameters
   * @tparam G the getter type
   * @tparam V the value type
   */
  template<class G, class V>
  concept GetterOf =
    Callable<G> && requires(G &&getter, std::remove_cvref_t<V> &value) {
      { getter(value) } -> std::same_as<Error>;
    } && is_non_const_lvalue_ref<first_arg_t<G>>;

  /**
   * concept for a Setter with value type V.
   *
   * See [here](docs.md#setters) for more details.
   *
   * @ingroup Parameters
   * @tparam S the setter type
   * @tparam V the value type
   */
  template<typename S, typename V>
  concept SetterOf = Callable<S> && requires(S &&setter, const V &value) {
    { setter(value) } -> std::same_as<Error>;
  } && not is_non_const_lvalue_ref<V>;

  /**
   * A Getter G retrieves the value of a parameter. An instance of G must
   * be callable with an non-const lvalue reference and return a cli::Error. The
   * reference denotes the place where the getter should store its value. If G
   * cannot produce a value, it should return the error that occurred.
   *
   * See [here](docs.md#getters) for more details.
   *
   * @ingroup Parameters
   * @tparam G the type to test
   */
  template<class G>
  concept Getter =
    Callable<G> and requires(G &&getter, getter_value_type_t<G> &value) {
      { getter(value) } -> std::same_as<Error>;
    };

  /**
   * A Setter S sets the value of a parameter. An instance of S must be
   * callable with a const l value reference and return a cli::Error.
   *
   * See [here](docs.md#setters) for more details.
   *
   * @ingroup Parameters
   * @tparam S the type to test
   */
  template<class S>
  concept Setter =
    Callable<S> and requires(S &&setter, const setter_value_type_t<S> &value) {
      { setter(value) } -> std::same_as<Error>;
    };

  template<SC Str>
  using get_char_t = typename Str::char_type;

  namespace dtl {

    struct invalid_tag_t {};

    template<Id Name,
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

    public:
      using char_type = typename Base::char_type;
      using Base::description;
      using Base::name;
      using sub_command_list = typename Base::sub_command_list;
      using Base::type;

      using value_type = getter_value_type_t<Get>;

      static_assert(std::is_same_v<value_type, setter_value_type_t<Set>>,
                    "Get and Set must get/set a value of the same type");

      static_assert(
        std::is_same_v<value_type, parse::value_type_t<char_type, Parse>>,
        "Parse and Get/Set must have the same value type");

      static_assert(
        std::is_same_v<value_type, format::formatter_value_type_t<Format>>,
        "Format and Get/Set must have the same value type");

      static_assert(
        std::is_same_v<value_type, validate::value_type_t<Validate>>,
        "Validate, Parse and Get/Set must have the same value type");

      static_assert(
        all_same_char_type_v<Name, Description, Type, SubCommands...>,
        "The name, description, and the subcommands must all use "
        "the same character type.");

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
                      SubCommands_ &&...cmds) noexcept
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
                      Validate_ &&validate) noexcept
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
                      cli::Tuple<SubCommands...> &&cmds) noexcept
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
                      const cli::Tuple<SubCommands...> &cmds) noexcept
        : Base(cmds),
          get_(std::forward<Get_>(get)),
          set_(std::forward<Set_>(set)),
          parse_(std::forward<Parse_>(parse)),
          format_(std::forward<Format_>(format)),
          validate_(std::forward<Validate_>(validate)) {}

      constexpr ExecResult<char_type> execute(View<const char_type> args,
                                              View<char_type> out) noexcept {
        args = parse::trim_ws(args);
        if (args.size() == 0) {
          return get_value(out);
        }

        if (args[0] == '=') {
          args = args.substr(1);
          args = parse::skip_ws(args);
        } else {
          return ExecResult<char_type>::make_parse_error(
            Error::expected_assignment, args.begin());
        }

        if (args.size() == 0)
          return ExecResult<char_type>::make_parse_error(Error::expected_value,
                                                         nullptr);

        return set_value(args);
      }

    private:
      constexpr ExecResult<char_type>
      set_value(View<const char_type> args) noexcept {
        if constexpr (requires {
                        {
                          Set::invalid_tag
                        } -> std::convertible_to<dtl::invalid_tag_t>;
                      }) {
          return ExecResult<char_type>::make_set_error(Error::cant_set_param);
        } else {
          parse::ParseResult parse_result = parse_(args);
          if (not parse_result)
            return ExecResult<char_type>::make_parse_error(
              parse_result.error, parse_result.rest.data());

          if (parse_result.rest.size() != 0)
            return ExecResult<char_type>::make_parse_error(
              Error::unexpected_characters, parse_result.rest.data());

          if (not validate_(parse_result.value))
            return ExecResult<char_type>::make_set_error(Error::invalid_value);

          Error e = set_(parse_result.value);
          if (e != Error::none)
            return ExecResult<char_type>::make_set_error(e);
          else
            return ExecResult<char_type>::make_success();
        }
      }

      constexpr ExecResult<char_type> get_value(View<char_type> out) noexcept {
        if constexpr (requires {
                        {
                          Get::invalid_tag
                        } -> std::convertible_to<dtl::invalid_tag_t>;
                      }) {
          return ExecResult<char_type>::make_get_error(Error::cant_read_param);
        } else {
          value_type t{};
          if (Error err = get_(t); err != Error::none)
            return ExecResult<char_type>::make_get_error(err);

          format::FormatResult res = format_(out, t);
          if (res.error != Error::none)
            return ExecResult<char_type>::make_format_error(res.error);
          else
            return ExecResult<char_type>::make_success(
              out.substr(0, res.size_written));
        }
      }

      CLI_NO_UNIQUE_ADDRESS Get get_;
      CLI_NO_UNIQUE_ADDRESS Set set_;
      CLI_NO_UNIQUE_ADDRESS Parse parse_;
      CLI_NO_UNIQUE_ADDRESS Format format_;
      CLI_NO_UNIQUE_ADDRESS Validate validate_;
    };

    template<Id Name,
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

    template<Id Name,
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

    template<Id Name,
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
          cli::Tuple<SubCommands...> &&cmds)
      -> Param<std::decay_t<Name>,
               std::decay_t<Description>,
               std::decay_t<Type>,
               std::decay_t<Get>,
               std::decay_t<Set>,
               std::decay_t<Parse>,
               std::decay_t<Format>,
               std::decay_t<Validate>,
               std::decay_t<SubCommands>...>;

    template<Id Name,
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
          const cli::Tuple<SubCommands...> &cmds)
      -> Param<std::decay_t<Name>,
               std::decay_t<Description>,
               std::decay_t<Type>,
               std::decay_t<Get>,
               std::decay_t<Set>,
               std::decay_t<Parse>,
               std::decay_t<Format>,
               std::decay_t<Validate>,
               std::decay_t<SubCommands>...>;

    template<Id Name,
             SC Description,
             SC Type,
             class MemberPointer,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    struct MemberData {
      using char_type = get_char_t<Name>;
      MemberPointer member;
      cli::Tuple<SubCommands...> subcommands;
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
                           MemberPointer mem_ptr,
                           Parse_ &&p,
                           Format_ &&fmt,
                           Validate_ &&v,
                           SubCommands_ &&...cmds) noexcept
        : member(mem_ptr),
          subcommands(std::forward<SubCommands>(cmds)...),
          parse(std::forward<Parse_>(p)),
          format(std::forward<Format_>(fmt)),
          validate(std::forward<Validate_>(v)) {}
    };

    // template<Id Name,
    //          SC Description,
    //          SC Help,
    //          class MemberPointer,
    //          parse::Parser Parse,
    //          format::Formatter Format,
    //          validate::Validator Validate>
    // struct MemberData<Name,
    //                   Description,
    //                   Help,
    //                   MemberPointer,
    //                   Parse,
    //                   Format,
    //                   Validate> {
    //   using char_type = get_char_t<Name>;
    //   MemberPointer member;
    //   CLI_NO_UNIQUE_ADDRESS Parse parse;
    //   CLI_NO_UNIQUE_ADDRESS Format format;
    //   CLI_NO_UNIQUE_ADDRESS Validate validate;
    //
    //   template<parse::Parser Parse_,
    //            format::Formatter Format_,
    //            validate::Validator Validate_>
    //   constexpr MemberData(Name,
    //                        Description,
    //                        Help,
    //                        MemberPointer mem_ptr,
    //                        Parse_ &&p,
    //                        Format_ &&fmt,
    //                        Validate_ &&v) noexcept
    //     : member(mem_ptr),
    //       parse(std::forward<Parse_>(p)),
    //       format(std::forward<Format_>(fmt)),
    //       validate(std::forward<Validate_>(v)) {}
    // };

    template<Id Name,
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
      constexpr Error operator()(dummy &) noexcept { return Error::none; }
    };

    struct NullSet {
      constexpr NullSet() = default;
      constexpr NullSet(const NullSet &) = default;
      constexpr NullSet(NullSet &&) = default;
      constexpr NullSet &operator=(const NullSet &) = default;
      constexpr NullSet &operator=(NullSet &&) = default;
      constexpr Error operator()(const dummy &) noexcept { return Error::none; }
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

      constexpr Error operator()(T &t) const noexcept {
        CLI_ASSERT(value_);
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

      constexpr Error operator()(const T &t) noexcept {
        CLI_ASSERT(value_);
        *value_ = t;
        return Error::none;
      }
    };

    template<auto &Object>
    struct ObjectSet {
      constexpr Error operator()(
        const std::remove_reference_t<decltype(Object)> &t) const noexcept {
        Object = t;
        return Error::none;
      }
    };

    template<const auto &Object>
    struct ObjectGet {
      constexpr Error
      operator()(std::remove_cvref_t<decltype(Object)> &t) const noexcept {
        t = Object;
        return Error::none;
      }
    };

    template<typename T, typename MemberPtr>
    struct MemDataGet {
      const T &value_;
      MemberPtr member;
      constexpr Error operator()(mem_data_type<MemberPtr> &t) noexcept {
        t = (value_.*member);
        return Error::none;
      }
    };

    template<typename T, typename MemberPtr>
    struct MemDataSet {
      T &value_;
      MemberPtr member;
      constexpr Error operator()(const mem_data_type<MemberPtr> &t) noexcept {
        value_.*member = t;
        return Error::none;
      }
    };

    template<typename T, typename MemberPtr>
    struct MemDataSet<const T, MemberPtr> {
      constexpr Error operator()(const mem_data_type<MemberPtr> &) noexcept {
        return Error::cant_set_param;
      }
    };

    template<typename T>
    struct InvalidGet {
      static constexpr invalid_tag_t invalid_tag{};
      cli::Error operator()(T &) const noexcept {
        return cli::Error::cant_read_param;
      }
    };

    template<typename T>
    struct InvalidSet {
      static constexpr invalid_tag_t invalid_tag{};
      cli::Error operator()(const T &) const noexcept {
        return cli::Error::cant_set_param;
      }
    };

    template<class T,
             Id Name,
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
                                     SubCommands...> member_data) noexcept {
      if constexpr (sizeof...(SubCommands) > 0)
        return Param{
          Name{},
          Description{},
          Help{},
          MemDataGet<T, MemberPointer>{obj, member_data.member},
          MemDataSet<T, MemberPointer>{obj, member_data.member},
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
          MemDataGet<T, MemberPointer>{obj, member_data.member},
          MemDataSet<T, MemberPointer>{obj, member_data.member},
          std::move(member_data.parse),
          std::move(member_data.format),
          std::move(member_data.validate)
        };
    }

    template<class T,
             Id Name,
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
                                     SubCommands...> member_data) noexcept {
      if constexpr (sizeof...(SubCommands) > 0)
        return Param{
          Name{},
          Description{},
          Help{},
          MemDataGet<T, MemberPointer>{obj, member_data.member},
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
          MemDataGet<T, MemberPointer>{obj, member_data.member},
          MemDataSet<const T, MemberPointer>{},
          std::move(member_data.parse),
          std::move(member_data.format),
          std::move(member_data.validate)
        };
    }

    template<class T, class CommandOrMemberDataOrMemberFunction>
    constexpr auto
    transform(T &obj, CommandOrMemberDataOrMemberFunction &&mem) noexcept {
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
  constexpr auto set_cb(T &t, Callback callback) noexcept {
    static_assert(std::is_invocable_v<Callback, T>,
                  "The callback must take a T as its argument");
    return SetWithCallback{dtl::DefaultSet<T>{&t},
                           std::forward<Callback>(callback)};
  }

  /**
   * @defgroup Parameters Parameters
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
   * Parameters can be created with the cli::param overload set. See
   * [here](docs.md#parameters) for more details.
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
   * ``auto p = param("cfg"_sc,
   *                  "configuration"_sc,
   *                  param("app"_sc, ...),
   *                  param("dbg"_sc, ...))``
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param description the description of the parameter, used by the help
   * functionality. Must be a cli::string_constant.
   * @param cmds the sub commands.
   * @return a Command
   */
  template<Id Name, SC Description, concepts::Command... SubCommands>
    requires(sizeof...(SubCommands) > 0)
  [[nodiscard]] constexpr auto
  param(Name name, Description description, SubCommands &&...cmds) noexcept {
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
  template<Id Name, concepts::Command... SubCommands>
    requires(sizeof...(SubCommands) > 0)
  [[nodiscard]] constexpr auto param(Name name,
                                     SubCommands &&...cmds) noexcept {
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
   *
   * There is additional set of overloads like above, just without description.
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
           Id Name,
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
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    return dtl::Param{Name{},
                      Description{},
                      ctti::name<T, get_char_t<Name>>(),
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
           Id Name,
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
                                     SubCommands &&...cmds) noexcept {
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
           Id Name,
           SC Description,
           GetterOf<T> Get,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        Get &&get,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
           Id Name,
           SC Description,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        Set &&set,
        Parse &&parse,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
           Id Name,
           SC Description,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        Get &&get,
        Format &&format,
        SubCommands &&...cmds) noexcept {
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
   * creates a parameter command from its individual parts. The default
   * parser, formatter and validator are used
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
           Id Name,
           SC Description,
           GetterOf<T> Get,
           SetterOf<T> Set,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        Get &&get,
        Set &&set,
        SubCommands &&...cmds) noexcept {
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
           Id Name,
           SC Description,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        Set &&set,
        Parse &&parse,
        SubCommands &&...cmds) noexcept {
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
           Id Name,
           SC Description,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     Set &&set,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
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
           Id Name,
           SC Description,
           GetterOf<T> Get,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        Get &&get,
        SubCommands &&...cmds) noexcept {
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
           Id Name,
           SC Description,
           SetterOf<T> Set,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        Set &&set,
        SubCommands &&...cmds) noexcept {
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
   *                                    get_i,
   *                                    set_i,
   *                                    parse_i,
   *                                    format_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
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
           Id Name,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Get &&get,
                                     Set &&set,
                                     Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
                    std::forward<Get>(get),
                    std::forward<Set>(set),
                    std::forward<Parse>(parse),
                    std::forward<Format>(format),
                    std::forward<Validate>(validate),
                    std::forward<SubCommands>(cmds)...);
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
   *                                    get_i,
   *                                    set_i,
   *                                    parse_i,
   *                                    format_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
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
           Id Name,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Get &&get,
                                     Set &&set,
                                     Parse &&parse,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    get_i,
   *                                    set_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           GetterOf<T> Get,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Get &&get,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    set_i,
   *                                    parse_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Set &&set,
        Parse &&parse,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    get_i,
   *                                    format_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Get &&get,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
                    std::forward<Get>(get),
                    dtl::InvalidSet<T>{},
                    parse::NoParse<T, get_char_t<Name>>{},
                    std::forward<Format>(format),
                    std::forward<Validate>(validate),
                    std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default
   * parser, formatter and validator are used
   *
   * Usage:
   * ```
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   *
   * auto par = cli::params::param<int>("i"_sc,
   *                                    get_i,
   *                                    set_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           GetterOf<T> Get,
           SetterOf<T> Set,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Get &&get, Set &&set, SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    set_i,
   *                                    parse_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Set &&set, Parse &&parse, SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    get_i,
   *                                    format_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Get &&get, Format &&format, SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    set_i,
   *                                    validate_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Set &&set,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    get_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           GetterOf<T> Get,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Get &&get, SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   *                                    set_i);
   * ```
   * @tparam T the parameter's type
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<typename T,
           Id Name,
           SetterOf<T> Set,
           concepts::Command... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, Set &&set, SubCommands &&...cmds) noexcept {
    (void)name;
    return param<T>(Name{},
                    NoDescription<get_char_t<Name>>{},
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
   * - validate: a Validator for a T. It validates parsed values before they
   * are set. See also cli::validate::Validator.
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
  template<Id Name,
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
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    return dtl::Param{Name{},
                      Description{},
                      ctti::name<T, get_char_t<Name>>(),
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        Set &&set,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Set &&set,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Get &&get,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           SetterOf<T> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Set &&set,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        T &t,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto param(
    Name name, Description description, T &t, SubCommands &&...cmds) noexcept {
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
   *                               i,
   *                               get_i,
   *                               set_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
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
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 t,
                 std::forward<Get>(get),
                 std::forward<Set>(set),
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
   * cli::Error get_i(int& ret){...}
   * cli::Error set_i(int i){...}
   * cli::parse::ParseResult<int,char> parse_i(cli::View<const char> s){...}
   * cli::FormatResult format_i(int i, cli::View<char>& out){}
   * static int i;
   * auto par = cli::params::param<int>("i"_sc,
   *                                    i,
   *                                    get_i,
   *                                    set_i,
   *                                    parse_i,
   *                                    format_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
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
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used to validate a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Get &&get,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               set_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
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
  template<Id Name,
           typename T,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               get_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
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
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Get &&get,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               get_i,
   *                               set_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           SetterOf<T> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, T &t, Get &&get, Set &&set, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               set_i,
   *                               parse_i,
   *                               format_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           SetterOf<T> Set,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Set &&set,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               get_i,
   *                               parse_i,
   *                               format_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Get &&get,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               set_i,
   *                               validate_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           SetterOf<T> Set,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Get &&get,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               get_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, T &t, Get &&get, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               get_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           SetterOf<T> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, T &t, Set &&set, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               set_i,
   *                               parse_i,
   *                               format_i,
   *                               validate_i);
   * ```
   *
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           parse::ParserOf<T, get_char_t<Name>> Parse,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        T &t,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   *                               i,
   *                               validate_i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name,
           typename T,
           validate::ValidatorOf<T> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, T &t, Validate &&validate, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
   * auto par = cli::params::param("i"_sc, i);
   * ```
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name, typename T, CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>> and
             not std::is_const_v<T>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, T &t, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 t,
                 dtl::DefaultGet<T>{t},
                 dtl::DefaultSet<T>{t},
                 parse::Parse<T, get_char_t<Name>>{},
                 format::Format<T, get_char_t<Name>>{},
                 validate::DefaultValidate<T>{},
                 std::forward<SubCommands>(cmds)...);
  }

  template<auto &Object>
  using object_type = std::remove_reference_t<decltype(Object)>;

  template<auto &Object>
  using const_object_type = std::remove_cvref_t<decltype(Object)>;
  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
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
  template<
    auto &Object,
    SC Description,
    GetterOf<object_type<Object>> Get,
    SetterOf<object_type<Object>> Set,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    validate::ValidatorOf<object_type<Object>> Validate,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return dtl::Param{
      ctti::object_name<Object, get_char_t<Description>>(),
      Description{},
      ctti::name<std::remove_const_t<object_type<Object>>,
                 get_char_t<Description>>(),
      std::forward<Get>(get),
      std::forward<Set>(set),
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      std::forward<Validate>(validate),
      dtl::transform(Object, std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
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
  template<
    auto &Object,
    SC Description,
    GetterOf<object_type<Object>> Get,
    SetterOf<object_type<Object>> Set,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         std::forward<Get>(get),
                         std::forward<Set>(set),
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default parser
   * and formatter are used.
   *
   * @tparam Object the parameter value
   * @param description the parameter description, used by the help
   * functionality. Must be a cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used to validate a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           GetterOf<object_type<Object>> Get,
           SetterOf<object_type<Object>> Set,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      std::forward<Get>(get),
      std::forward<Set>(set),
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default getter
   * is used.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<
    auto &Object,
    SC Description,
    SetterOf<object_type<Object>> Set,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    validate::ValidatorOf<object_type<Object>> Validate,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         dtl::ObjectGet<Object>{},
                         std::forward<Set>(set),
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default setter
   * is used.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<
    auto &Object,
    SC Description,
    GetterOf<object_type<Object>> Get,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    validate::ValidatorOf<object_type<Object>> Validate,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         std::forward<Get>(get),
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           GetterOf<object_type<Object>> Get,
           SetterOf<object_type<Object>> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Set &&set,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      std::forward<Get>(get),
      std::forward<Set>(set),
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      validate::DefaultValidate<object_type<Object>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
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
  template<
    auto &Object,
    SC Description,
    SetterOf<object_type<Object>> Set,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Set &&set,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         dtl::ObjectGet<Object>{},
                         std::forward<Set>(set),
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
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
  template<
    auto &Object,
    SC Description,
    GetterOf<object_type<Object>> Get,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         std::forward<Get>(get),
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom setter and validator.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           SetterOf<object_type<Object>> Set,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Set &&set,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      dtl::ObjectGet<Object>{},
      std::forward<Set>(set),
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           GetterOf<object_type<Object>> Get,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      std::forward<Get>(get),
      dtl::ObjectSet<Object>{},
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
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
  template<
    auto &Object,
    SC Description,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    validate::ValidatorOf<object_type<Object>> Validate,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         dtl::ObjectGet<Object>{},
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           GetterOf<object_type<Object>> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires((not std::is_member_pointer_v<
               std::remove_cvref_t<object_type<Object>>>) and
             not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description, Get &&get, SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      std::forward<Get>(get),
      dtl::ObjectSet<Object>{},
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      validate::DefaultValidate<object_type<Object>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           SetterOf<object_type<Object>> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description, Set &&set, SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      dtl::ObjectGet<Object>{},
      std::forward<Set>(set),
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      validate::DefaultValidate<object_type<Object>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom parser and formatter.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<
    auto &Object,
    SC Description,
    parse::ParserOf<object_type<Object>, get_char_t<Description>> Parse,
    format::FormatterOf<object_type<Object>, get_char_t<Description>> Format,
    CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         dtl::ObjectGet<Object>{},
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom validator.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      dtl::ObjectGet<Object>{},
      dtl::ObjectSet<Object>{},
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object, SC Description, CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description, SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      dtl::ObjectGet<Object>{},
      dtl::ObjectSet<Object>{},
      parse::Parse<object_type<Object>, get_char_t<Description>>{},
      format::Format<object_type<Object>, get_char_t<Description>>{},
      validate::DefaultValidate<object_type<Object>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
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
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           SetterOf<object_type<Object>> Set,
           parse::Parser Parse,
           format::Formatter Format,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         std::forward<Get>(get),
                         std::forward<Set>(set),
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
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
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           SetterOf<object_type<Object>> Set,
           parse::Parser Parse,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get,
        Set &&set,
        Parse &&parse,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         std::forward<Get>(get),
                         std::forward<Set>(set),
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<ObjectType>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default parser
   * and formatter are used.
   *
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used to validate a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           SetterOf<object_type<Object>> Set,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto param(
    Get &&get, Set &&set, Validate &&validate, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         std::forward<Get>(get),
                         std::forward<Set>(set),
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default getter
   * is used.
   *
   * @tparam Object the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SetterOf<object_type<Object>> Set,
           parse::Parser Parse,
           format::Formatter Format,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Set &&set,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         dtl::DefaultGet<ObjectType>{Object},
                         std::forward<Set>(set),
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts. The default setter
   * is used.
   *
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           parse::Parser Parse,
           format::Formatter Format,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get,
        Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         std::forward<Get>(get),
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   * ```
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           SetterOf<object_type<Object>> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get, Set &&set, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         std::forward<Get>(get),
                         std::forward<Set>(set),
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SetterOf<object_type<Object>> Set,
           parse::Parser Parse,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto param(
    Set &&set, Parse &&parse, Format &&format, SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         dtl::ObjectGet<Object>{},
                         std::forward<Set>(set),
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           parse::Parser Parse,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto param(
    Get &&get, Parse &&parse, Format &&format, SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         std::forward<Get>(get),
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom setter and validator.
   *
   * @tparam Object the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SetterOf<object_type<Object>> Set,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Set &&set, Validate &&validate, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         dtl::ObjectGet<Object>{},
                         std::forward<Set>(set),
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get, Validate &&validate, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         std::forward<Get>(get),
                         dtl::ObjectSet<Object>{},
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           parse::Parser Parse,
           format::Formatter Format,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Parse &&parse,
        Format &&format,
        Validate &&validate,
        SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         dtl::ObjectGet<Object>{},
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           GetterOf<object_type<Object>> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         std::forward<Get>(get),
                         dtl::ObjectSet<Object>{},
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SetterOf<object_type<Object>> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Set &&set, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         dtl::ObjectGet<Object>{},
                         std::forward<Set>(set),
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom parser and formatter.
   *
   * @tparam Object the parameter value
   * @param parse the parser used to parse a T
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           parse::Parser Parse,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Parse &&parse, Format &&format, SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using ObjectType = object_type<Object>;

    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, ObjectType>,
      "format must be able to format the Object");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, ObjectType>,
      "parse must be able to parse values of the Object's type");

    return param<Object>(NoDescription<parse_char_type>{},
                         dtl::ObjectGet<Object>{},
                         dtl::ObjectSet<Object>{},
                         std::forward<Parse>(parse),
                         std::forward<Format>(format),
                         validate::DefaultValidate<object_type<Object>>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command with custom validator.
   *
   * @tparam Object the parameter value
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Validate &&validate, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         dtl::ObjectGet<Object>{},
                         dtl::ObjectSet<Object>{},
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         std::forward<Validate>(validate),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a parameter command from its individual parts.
   *
   * @tparam Object the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object, CmdOrMemDataOrMemFun... SubCommands>
    requires(
      not std::is_member_pointer_v<std::remove_cvref_t<object_type<Object>>> and
      not std::is_const_v<object_type<Object>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         dtl::ObjectGet<Object>{},
                         dtl::ObjectSet<Object>{},
                         parse::Parse<object_type<Object>, char>{},
                         format::Format<object_type<Object>, char>{},
                         validate::DefaultValidate<object_type<Object>>{},
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        const T &t,
        Get &&get,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    return dtl::Param{Name{},
                      Description{},
                      ctti::name<T, get_char_t<Name>>(),
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
  template<Id Name,
           SC Description,
           typename T,
           GetterOf<T> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        const T &t,
        Get &&get,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        const T &t,
        Format &&format,
        SubCommands &&...cmds) noexcept {
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
  template<Id Name,
           SC Description,
           typename T,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        Description description,
        const T &t,
        SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    return param(Name{},
                 Description{},
                 t,
                 dtl::DefaultGet<T>{t},
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<SubCommands>(cmds)...);
  }

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
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name,
        const T &t,
        Get &&get,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)name;
    return dtl::Param{Name{},
                      NoDescription<get_char_t<Name>>{},
                      ctti::name<T, get_char_t<Name>>(),
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
  template<Id Name,
           typename T,
           GetterOf<T> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, const T &t, Get &&get, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
  template<Id Name,
           typename T,
           format::FormatterOf<T, get_char_t<Name>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto param(
    Name name, const T &t, Format &&format, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
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
  template<Id Name, typename T, CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Name name, const T &t, SubCommands &&...cmds) noexcept {
    (void)name;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 t,
                 dtl::DefaultGet<T>{t},
                 format::Format<T, get_char_t<Name>>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with a custom getter and formatter.
   *
   * @tparam Object the parameter value
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
  template<const auto &Object,
           SC Description,
           GetterOf<const_object_type<Object>> Get,
           format::FormatterOf<const_object_type<Object>,
                               get_char_t<Description>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)description;
    return dtl::Param{
      ctti::object_name<Object, get_char_t<Description>>(),
      Description{},
      ctti::name<const_object_type<Object>, get_char_t<Description>>(),
      std::forward<Get>(get),
      dtl::InvalidSet<const_object_type<Object>>{},
      parse::NoParse<const_object_type<Object>, get_char_t<Description>>{},
      std::forward<Format>(format),
      validate::DefaultValidate<const_object_type<Object>>{},
      dtl::transform(Object, std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a read-only parameter command with default formatter.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<const auto &Object,
           SC Description,
           GetterOf<const_object_type<Object>> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description, Get &&get, SubCommands &&...cmds) noexcept {
    (void)description;
    return dtl::Param{
      ctti::object_name<Object, get_char_t<Description>>(),
      Description{},
      ctti::name<const_object_type<Object>, get_char_t<Description>>(),
      std::forward<Get>(get),
      dtl::InvalidSet<const_object_type<Object>>{},
      parse::NoParse<const_object_type<Object>, get_char_t<Description>>{},
      format::Format<const_object_type<Object>, get_char_t<Description>>{},
      validate::DefaultValidate<const_object_type<Object>>{},
      dtl::transform(Object, std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a read-only parameter command with custom formatter.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<const auto &Object,
           SC Description,
           format::FormatterOf<const_object_type<Object>,
                               get_char_t<Description>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto param(
    Description description, Format &&format, SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(Description{},
                         dtl::DefaultGet<const_object_type<Object>>{Object},
                         std::forward<Format>(format),
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with default formatter and getter.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param t the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<const auto &Object,
           SC Description,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description, SubCommands &&...cmds) noexcept {
    (void)description;
    return param<Object>(
      Description{},
      dtl::DefaultGet<const_object_type<Object>>{Object},
      format::Format<const_object_type<Object>, get_char_t<Description>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with a custom getter and formatter.
   *
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<const auto &Object,
           GetterOf<const_object_type<Object>> Get,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get, Format &&format, SubCommands &&...cmds) noexcept {
    using value_type = format::formatter_value_type_t<Format>;
    using char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;

    static_assert(std::is_same_v<value_type, const_object_type<Object>>,
                  "format must be able to format the Object");

    return dtl::Param{
      ctti::object_name<Object, char_type>(),
      NoDescription<char_type>{},
      ctti::name<const_object_type<Object>, char_type>(),
      std::forward<Get>(get),
      dtl::InvalidSet<const_object_type<Object>>{},
      parse::NoParse<const_object_type<Object>, char_type>{},
      std::forward<Format>(format),
      validate::DefaultValidate<const_object_type<Object>>{},
      dtl::transform(Object, std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a read-only parameter command with default formatter.
   *
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<const auto &Object,
           GetterOf<const_object_type<Object>> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get, SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         std::forward<Get>(get),
                         format::Format<const_object_type<Object>, char>{},
                         std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a read-only parameter command with custom formatter.
   *
   * @tparam Object the parameter value
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<const auto &Object,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(Format &&format, SubCommands &&...cmds) noexcept {
    using value_type = format::formatter_value_type_t<Format>;
    using char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;

    static_assert(std::is_same_v<value_type, const_object_type<Object>>,
                  "format must be able to format the Object");

    return param<Object>(NoDescription<char_type>{},
                         dtl::DefaultGet<const_object_type<Object>>{Object},
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
  template<const auto &Object, CmdOrMemDataOrMemFun... SubCommands>
    requires(not std::is_member_pointer_v<
             std::remove_cvref_t<const_object_type<Object>>>)
  [[nodiscard]] constexpr concepts::Command auto
  param(SubCommands &&...cmds) noexcept {
    return param<Object>(NoDescription<char>{},
                         dtl::DefaultGet<const_object_type<Object>>{Object},
                         format::Format<const_object_type<Object>, char>{},
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
  // template <Id Name, typename T, GetterOf<T> Get,
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
  // template <Id Name, typename T, SetterOf<T> Set,
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
  // template <Id Name, typename T, SetterOf<T> Set,
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
  // template <Id Name, SC Description, typename T, GetterOf<T> Get,
  // SetterOf<T> Set,
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
  // template <Id Name, typename T,
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
  // template <Id Name, SC Description, class T, concepts::Command...
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
  // template <Id Name, typename T, GetterOf<T> Get, SetterOf<T> Set,
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
  // template <Id Name, typename T, SetterOf<T> Set, concepts::Command...
  // SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Set &&set,
  // SubCommands
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
  // template <Id Name, typename T, GetterOf<T> Get, concepts::Command...
  // SubCommands>
  //   requires(not std::is_member_pointer_v<std::remove_cvref_t<T>>)
  // [[nodiscard]] constexpr auto param(Name name, T &t, Get &&get,
  // SubCommands
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
  // // template <Id Name, class T, concepts::Command... SubCommands>
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
  //  * @param format the formatter of the parameter. See
  //  cli::format::Formatter for
  //  * additional info.
  //  * @param validate the validator of the parameter. See
  //  cli::validate::Validator
  //  * for additional info.
  //  * @param cmds additional optional subcommands
  //  * @return a Command
  //  */
  // template <Id Name, SC Description, SC Type, Getter Get, Setter Set,
  //           parse::Parser Parse, format::Formatter Format,
  //           validate::Validator Validate, concepts::Command... SubCommands>
  // [[nodiscard]] constexpr auto param(Name name, Description description,
  // Type type, Get
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
  //  * functions in addition to sub commands. Uses the default getter,
  //  setter,
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
  // template <Id Name, class T, class... CommandOrMemberDataOrMemberFunction>
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
  //  * functions in addition to sub commands. Uses the default getter,
  //  setter,
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
  // template <Id Name, SC Description, class T,
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
  //  * then be retrieved by its name, which is deduced. This overload can
  //  take
  //  * member data and member functions in addition to sub commands. Uses the
  //  * default getter, setter, parsing, formatting, and validation
  //  facilities.
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
  //  * then be retrieved by its name, which is deduced. This overload can
  //  take
  //  * member data and member functions in addition to sub commands. Uses the
  //  * default getter, setter, parsing, formatting, and validation
  //  facilities.
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
   * To make the settings and its members foo and baz available to cli, you
   * can use the following functions to easily setup this structure.
   *
   * ```
   *  param("settings"_sc, "core settings", settings,
   *             param("foo"_sc, "foo mode"_sc, &Settings::foo),
   *          param("baz"_sc, "baz setting"_sc, &Settings::baz));
   * ```
   *
   * Then ``settings``, ``settings.foo`` and ``settings.baz`` can be used as
   * parameter commands.
   *
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
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    Id Name,
    SC Description,
    class MemberPointer,
    parse::ParserOf<mem_data_type<MemberPointer>, get_char_t<Name>> Parse,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    validate::ValidatorOf<mem_data_type<MemberPointer>> Validate,
    concepts::Command... SubCommands>
    requires(std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             not std::is_const_v<mem_data_type<MemberPointer>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer member,
                                     Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    using namespace dtl;
    return MemberData{
      Name{},
      Description{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      member,
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    Id Name,
    SC Description,
    class MemberPointer,
    parse::ParserOf<mem_data_type<MemberPointer>, get_char_t<Name>> Parse,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    concepts::Command... SubCommands>
    requires(std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             not std::is_const_v<mem_data_type<MemberPointer>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer member,
                                     Parse &&parse,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    using namespace dtl;
    return param(Name{},
                 Description{},
                 member,
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 validate::DefaultValidate<mem_data_type<MemberPointer>>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<Id Name,
           SC Description,
           class MemberPointer,
           validate::ValidatorOf<mem_data_type<MemberPointer>> Validate,
           concepts::Command... SubCommands>
    requires(std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             not std::is_const_v<mem_data_type<MemberPointer>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer member,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    using namespace dtl;
    return param(
      Name{},
      Description{},
      member,
      cli::parse::Parse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      cli::format::Format<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    Id Name,
    class MemberPointer,
    parse::ParserOf<mem_data_type<MemberPointer>, get_char_t<Name>> Parse,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    validate::ValidatorOf<mem_data_type<MemberPointer>> Validate,
    concepts::Command... SubCommands>
    requires(std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             not std::is_const_v<mem_data_type<MemberPointer>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     MemberPointer member,
                                     Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    using namespace dtl;
    return MemberData{
      Name{},
      NoDescription<get_char_t<Name>>{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      member,
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    Id Name,
    class MemberPointer,
    parse::ParserOf<mem_data_type<MemberPointer>, get_char_t<Name>> Parse,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    concepts::Command... SubCommands>
    requires(std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             not std::is_const_v<mem_data_type<MemberPointer>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     MemberPointer member,
                                     Parse &&parse,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    using namespace dtl;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 member,
                 std::forward<Parse>(parse),
                 std::forward<Format>(format),
                 validate::DefaultValidate<mem_data_type<MemberPointer>>{},
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param member pointer to the member variable
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<Id Name,
           class MemberPointer,
           validate::ValidatorOf<mem_data_type<MemberPointer>> Validate,
           concepts::Command... SubCommands>
    requires(std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             not std::is_const_v<mem_data_type<MemberPointer>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     MemberPointer member,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    using namespace dtl;
    return param(
      Name{},
      NoDescription<get_char_t<Name>>{},
      member,
      cli::parse::Parse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      cli::format::Format<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
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
   *  auto cmd = param("s"_sc,
   *                   s,
   *                   mem_data("a"_sc,
   *                            "a description"_sc,
   *                             &S::a));
   * ```
   *
   * @param name the name of f. Must be a cli::string_constant.
   * @param member member data pointer
   * @param description the description of MemberPointer. Must be a
   * cli::string_constant.
   * @param cmds the subcommands
   */
  template<Id Name,
           SC Description,
           class MemberPointer,
           concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer member,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    using namespace dtl;
    using T = mem_data_type<MemberPointer>;
    if constexpr (std::is_const_v<T>) {
      return MemberData{
        Name{},
        Description{},
        cli::ctti::name<std::remove_const_t<T>, get_char_t<Name>>(),
        member,
        parse::NoParse<T, get_char_t<Name>>{},
        format::Format<T, get_char_t<Name>>{},
        validate::DefaultValidate<mem_data_type<MemberPointer>>{},
        std::forward<SubCommands>(cmds)...};
    } else
      return MemberData{
        Name{},
        Description{},
        cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
        member,
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
   * @param member member data pointer
   * @param cmds the subcommands
   */
  template<Id Name, class MemberPointer, concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto
  param(Name name, MemberPointer member, SubCommands &&...cmds) noexcept {
    (void)name;
    using namespace dtl;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 member,
                 std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    Id Name,
    SC Description,
    class MemberPointer,
    parse::ParserOf<mem_data_type<MemberPointer>, get_char_t<Name>> Parse,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    concepts::Command... SubCommands>
    requires(std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>> and
             std::is_const_v<mem_data_type<MemberPointer>>)
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer member,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    using namespace dtl;
    return param(
      Name{},
      Description{},
      member,
      parse::NoParse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      std::forward<Format>(format),
      validate::DefaultValidate<mem_data_type<MemberPointer>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    auto MemberPointer,
    SC Description,
    parse::ParserOf<mem_data_type<decltype(MemberPointer)>,
                    get_char_t<Description>> Parse,
    format::FormatterOf<mem_data_type<decltype(MemberPointer)>,
                        get_char_t<Description>> Format,
    validate::ValidatorOf<mem_data_type<decltype(MemberPointer)>> Validate,
    concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>> and
      not std::is_const_v<mem_data_type<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto param(Description description,
                                     Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    return MemberData{
      ctti::value_name<MemberPointer, get_char_t<Description>>(),
      Description{},
      cli::ctti::name<mem_data_type<decltype(MemberPointer)>,
                      get_char_t<Description>>(),
      MemberPointer,
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<auto MemberPointer,
           SC Description,
           parse::ParserOf<mem_data_type<decltype(MemberPointer)>,
                           get_char_t<Description>> Parse,
           format::FormatterOf<mem_data_type<decltype(MemberPointer)>,
                               get_char_t<Description>> Format,
           concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>> and
      not std::is_const_v<mem_data_type<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto param(Description description,
                                     Parse &&parse,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    return param<MemberPointer>(
      Description{},
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      validate::DefaultValidate<mem_data_type<decltype(MemberPointer)>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    auto MemberPointer,
    SC Description,
    validate::ValidatorOf<mem_data_type<decltype(MemberPointer)>> Validate,
    concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>> and
      not std::is_const_v<mem_data_type<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto param(Description description,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    return param<MemberPointer>(
      Description{},
      cli::parse::Parse<mem_data_type<decltype(MemberPointer)>,
                        get_char_t<Description>>{},
      cli::format::Format<mem_data_type<decltype(MemberPointer)>,
                          get_char_t<Description>>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    auto MemberPointer,
    parse::Parser Parse,
    format::Formatter Format,
    validate::ValidatorOf<mem_data_type<decltype(MemberPointer)>> Validate,
    concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>> and
      not std::is_const_v<mem_data_type<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto param(Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using value_type = mem_data_type<decltype(MemberPointer)>;
    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, value_type>,
      "format must be able to format the type that MemberPointer points to");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, value_type>,
      "parse must be able to parse the type that MemberPointer points to");
    using namespace dtl;
    return MemberData{ctti::value_name<MemberPointer, parse_char_type>(),
                      NoDescription<parse_char_type>{},
                      cli::ctti::name<mem_data_type<decltype(MemberPointer)>,
                                      parse_char_type>(),
                      MemberPointer,
                      std::forward<Parse>(parse),
                      std::forward<Format>(format),
                      std::forward<Validate>(validate),
                      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<auto MemberPointer,
           parse::Parser Parse,
           format::Formatter Format,
           concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>> and
      not std::is_const_v<mem_data_type<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto
  param(Parse &&parse, Format &&format, SubCommands &&...cmds) noexcept {
    using parse_char_type = typename parse::result_type_t<Parse>::char_type;
    using format_char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;
    using value_type = mem_data_type<decltype(MemberPointer)>;
    static_assert(std::is_same_v<parse_char_type, format_char_type>,
                  "parse and format must use the same char type");
    static_assert(
      std::is_same_v<format::formatter_value_type_t<Format>, value_type>,
      "format must be able to format the type that MemberPointer points to");

    static_assert(
      std::is_same_v<parse::value_type_t<parse_char_type, Parse>, value_type>,
      "parse must be able to parse the type that MemberPointer points to");
    using namespace dtl;
    return param<MemberPointer>(
      NoDescription<parse_char_type>{},
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      validate::DefaultValidate<mem_data_type<decltype(MemberPointer)>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param member pointer to the member variable
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    auto MemberPointer,
    validate::ValidatorOf<mem_data_type<decltype(MemberPointer)>> Validate,
    concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>> and
      not std::is_const_v<mem_data_type<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto param(Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    using namespace dtl;
    return param<MemberPointer>(
      NoDescription<char>{},
      cli::parse::Parse<mem_data_type<decltype(MemberPointer)>, char>{},
      cli::format::Format<mem_data_type<decltype(MemberPointer)>, char>{},
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
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
   *  auto cmd = param("s"_sc,
   *                   s,
   *                   mem_data("a"_sc,
   *                            "a description"_sc,
   *                             &S::a));
   * ```
   *
   * @param name the name of f. Must be a cli::string_constant.
   * @param member member data pointer
   * @param description the description of MemberPointer. Must be a
   * cli::string_constant.
   * @param cmds the subcommands
   */
  template<auto MemberPointer, SC Description, concepts::Command... SubCommands>
    requires std::is_member_pointer_v<
      std::remove_cvref_t<decltype(MemberPointer)>>
  [[nodiscard]] constexpr auto param(Description description,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    using T = mem_data_type<decltype(MemberPointer)>;
    if constexpr (std::is_const_v<T>) {
      return MemberData{
        ctti::value_name<MemberPointer, get_char_t<Description>>(),
        Description{},
        cli::ctti::name<std::remove_const_t<T>, get_char_t<Description>>(),
        MemberPointer,
        parse::NoParse<std::remove_const_t<T>, get_char_t<Description>>{},
        format::Format<std::remove_const_t<T>, get_char_t<Description>>{},
        validate::DefaultValidate<std::remove_const_t<T>>{},
        std::forward<SubCommands>(cmds)...};
    } else
      return MemberData{
        ctti::value_name<MemberPointer, get_char_t<Description>>(),
        Description{},
        cli::ctti::name<T, get_char_t<Description>>(),
        MemberPointer,
        parse::Parse<T, get_char_t<Description>>{},
        format::Format<T, get_char_t<Description>>{},
        validate::DefaultValidate<T>{},
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
   * @param member member data pointer
   * @param cmds the subcommands
   */
  template<auto MemberPointer, concepts::Command... SubCommands>
    requires std::is_member_pointer_v<
      std::remove_cvref_t<decltype(MemberPointer)>>
  [[nodiscard]] constexpr auto param(SubCommands &&...cmds) noexcept {
    using namespace dtl;
    return param<MemberPointer>(NoDescription<char>{},
                                std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<auto MemberPointer,
           SC Description,
           format::FormatterOf<
             std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
             get_char_t<Description>> Format,
           concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto param(Description description,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    return MemberData{
      ctti::value_name<MemberPointer, get_char_t<Description>>(),
      Description{},
      cli::ctti::name<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        get_char_t<Description>>(),
      MemberPointer,
      parse::NoParse<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        get_char_t<Description>>{},
      std::forward<Format>(format),
      validate::DefaultValidate<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>>{},
      std::forward<SubCommands>(cmds)...};
  }
  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @param name the name, must be a string_constant.
   * @param description the description. Must be a string_constant.
   * @param member pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<auto MemberPointer,
           format::FormatterOf<
             std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
             char> Format,
           concepts::Command... SubCommands>
    requires(
      std::is_member_pointer_v<std::remove_cvref_t<decltype(MemberPointer)>>)
  [[nodiscard]] constexpr auto param(Format &&format,
                                     SubCommands &&...cmds) noexcept {
    using namespace dtl;
    return MemberData{
      ctti::value_name<MemberPointer, char>(),
      NoDescription<char>{},
      cli::ctti::name<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        char>(),
      MemberPointer,
      parse::NoParse<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        char>{},
      std::forward<Format>(format),
      validate::DefaultValidate<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>>{},
      std::forward<SubCommands>(cmds)...};
  }

  //   /** creates a member data subcommand. Must be used together with a parent
  //    * command. The name is deduced by cli.
  //    *
  //    * Example: ``` struct S{ int a; }; static S s; auto cmd = param("s"_sc,
  //    s,
  //    * mem_data<&S::a>("a number"_sc)); ``` @tparam MemberPointer member data
  //    * pointer @param description the description of MemberPointer. Must be a
  //    * cli::string_constant. @param cmds the subcommands
  //    */
  // template<auto MemberPointer, SC Description, concepts::Command...
  // SubCommands> requires std::is_member_pointer_v<
  // std::remove_cvref_t<decltype(MemberPointer)>> [[nodiscard]] constexpr auto
  // param(Description description, SubCommands &&...cmds) noexcept {
  //   (void)description; using namespace dtl; return param(
  //     ctti::value_name<MemberPointer, typename Description::char_type>(),
  //     Description{}, MemberPointer, std::forward<SubCommands>(cmds)...); }
  //
  // /** creates a member data subcommand. Must be used together with a parent
  //  * command. The name is deduced by cli.
  //    *
  //    * Example: ``` struct S{ int a; }; static S s; auto cmd = param("s"_sc,
  //    s,
  //    * mem_data<&S::a>()); ``` @tparam MemberPointer member data pointer
  //    @param
  //    * cmds the subcommands
  //    */
  // template<auto MemberPointer, concepts::Command... SubCommands> requires
  // std::is_member_pointer_v< std::remove_cvref_t<decltype(MemberPointer)>>
  // [[nodiscard]] constexpr auto param(SubCommands &&...cmds) noexcept { using
  //   namespace dtl; return param<MemberPointer>(NoDescription<char>{},
  //                                              std::forward<SubCommands>(cmds)...);
  // }

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
   * To make the settings and its members foo and baz available to cli, you
   * can use the following functions to easily setup this structure.
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
   * @brief cretes a member data parameter. Must be used as a subcommand.
   *
   * @param name the paramter name. Must be a string_constant.
   * @param description the parameter description. Must be a string_constant.
   * @param member the member data pointer
   * @param format a foramtter for te type pointed to by member.
   * @param cmds any additional subocmmands.
   * @return a partial command.
   */
  template<
    Id Name,
    SC Description,
    class MemberPointer,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer member,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    using namespace dtl;
    return MemberData{
      Name{},
      Description{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      member,
      parse::NoParse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      std::forward<Format>(format),
      validate::DefaultValidate<mem_data_type<MemberPointer>>{},
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief cretes a member data parameter. Must be used as a subcommand.
   *
   * @param name the paramter name. Must be a string_constant.
   * @param member the member data pointer
   * @param format a foramtter for te type pointed to by member.
   * @param cmds any additional subocmmands.
   * @return a partial command.
   */
  template<
    Id Name,
    class MemberPointer,
    format::FormatterOf<mem_data_type<MemberPointer>, get_char_t<Name>> Format,
    concepts::Command... SubCommands>
    requires std::is_member_pointer_v<std::remove_cvref_t<MemberPointer>>
  [[nodiscard]] constexpr auto param(Name name,
                                     MemberPointer member,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    using namespace dtl;
    return MemberData{
      Name{},
      NoDescription<get_char_t<Name>>{},
      cli::ctti::name<mem_data_type<MemberPointer>, get_char_t<Name>>(),
      member,
      parse::NoParse<mem_data_type<MemberPointer>, get_char_t<Name>>{},
      std::forward<Format>(format),
      validate::DefaultValidate<mem_data_type<MemberPointer>>{},
      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @}
   */

  inline constexpr struct recursive_t {
  } recursive;

  template<typename C, typename T>
  concept SetCallback = requires(std::remove_cvref_t<C> callback, const T &t) {
    { callback(t) } -> std::same_as<void>;
  };

  /**
   * @defgroup recursive-params Recursive Parameters
   * @ingroup Parameters
   *
   * Recursive parameters are created by any of the following param overloads:
   *
   * ```
   *  cli::param(name, description, t, set_callback, validate, cli::recursive)
   *  cli::param(name, t,              set_callback, validate, cli::recursive)
   *  cli::param(name, description, t,               validate, cli::recursive)
   *  cli::param(name,              t,               validate, cli::recursive)
   *  cli::param(name, description, t, set_callback,           cli::recursive)
   *  cli::param(name,              t, set_callback,           cli::recursive)
   *  cli::param(name, description, t,                         cli::recursive)
   *  cli::param(name,              t,                         cli::recursive)
   * ```
   *
   * where:
   * - **name** and **description** are string_constants,
   * - **t** is the object of the parameter
   * - **set_callback** is a callable the takes a T and returns void. Is is
   *   called when t or any of its subparameters are set.
   * - **validate**:  is a validator for a T.
   *
   * A call to these overloads will recursively build up subcommands of all the
   * members of t.
   *
   * For example, given the structs and variable:
   *
   * ```
   *  struct SubSettings{
   *    int i = 0;
   *  };
   *
   *  struct Settings{
   *    char c = 'x';
   *    SubSettings subsettings{};
   *  }
   *
   *  static constinit Settings settings;
   * ```
   *
   *  and this call to cli::param
   *
   * ```
   *  cli::param("settings"_sc, settings, cli::recursive);
   * ```
   *
   * will generate the following command structure:
   *
   *  - settings [Settings]
   *    - c [char]
   *    - subsettings [SubSettings]
   *      - i [int]
   *
   *
   * There are also overloads for a const **t** available:
   *
   * ```
   *  cli::param(name, description, t, cli::recursive)
   *  cli::param(name,              t, cli::recursive)
   * ```
   * @{
   */

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * void on_settings_update(const Settings& s);
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            "a description"_sc,
   *            settings,
   *            &on_settings_update,
   *            [](const Settings& s)-> bool{
   *              return s.c >= 'a' and s.c <= 'z' and s.subsettings.a >= 100;
   *            },
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param description the parameter description
   * @param t the parameter object
   * @param set_callback called when the parameter or any subparameter is set.
   * @param validate A validator of T. Validates the object before this
   *        parameters or any subparameters are set.
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name,
           SC Description,
           class T,
           SetCallback<T> Callback,
           validate::ValidatorOf<T> Validate>
    requires(std::is_copy_constructible_v<T> and not std::is_const_v<T>)
  constexpr concepts::Command auto param(Name name,
                                         Description description,
                                         T &t,
                                         Callback set_callback,
                                         Validate validate,
                                         recursive_t r) {
    (void)name;
    (void)description;
    (void)r;
    if constexpr (ctti::dtl::num_members<T>() == 1 and
                  not concepts::Struct<T>) {
      return param(
        Name{},
        Description{},
        t,
        // setter
        [&t, cb = set_callback](const T &t_set) -> Error {
          t = t_set;
          cb(t);
          return Error::none;
        },
        // validator
        validate);
    } else if constexpr (ctti::dtl::num_members<T>() == 1 and
                         concepts::Struct<T>) {
      using CharT = get_char_t<Name>;
      return param(
        Name{},
        Description{},
        t,
        // setter
        [&t, cb = set_callback](const T &t_set) -> Error {
          t = t_set;
          cb(t);
          return Error::none;
        },
        // validator
        validate,
        param(
          ctti::dtl::member_name<T, 0, CharT>(),
          NoDescription<CharT>{},
          ctti::dtl::get_ref<0>(t),
          // set callback
          [&t, cb = set_callback](
            const ctti::dtl::member_type_t<T, 0> &) -> void { cb(t); },
          // validator
          [&t, validate](const ctti::dtl::member_type_t<T, 0> &val) -> bool {
            T t_ = t;
            ctti::dtl::get_ref<0>(t_) = val;
            return validate(t_);
          },
          recursive));
    } else {
      using CharT = get_char_t<Name>;
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return param(
          Name{},
          Description{},
          t,
          // setter
          [&t, cb = set_callback](const T &t_set) -> Error {
            t = t_set;
            cb(t);
            return Error::none;
          },
          validate,
          param(
            ctti::dtl::member_name<T, Is, CharT>(),
            NoDescription<CharT>{},
            ctti::dtl::get_ref<Is>(t),
            // set callback
            [&t, cb = set_callback](
              const ctti::dtl::member_type_t<T, Is> &) -> void { cb(t); },
            // validator
            [&t, validate](const ctti::dtl::member_type_t<T, Is> &val) -> bool {
              T t_ = t;
              ctti::dtl::get_ref<Is>(t_) = val;
              return validate(t_);
            },
            recursive)...);
      }(std::make_index_sequence<ctti::dtl::num_members<T>()>{});
    }
  }

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * void on_settings_update(const Settings& s);
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            settings,
   *            &on_settings_update,
   *            [](const Settings& s)-> bool{
   *              return s.c >= 'a' and s.c <= 'z' and s.subsettings.a >= 100;
   *            },
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param t the parameter object
   * @param set_callback called when the parameter or any subparameter is set.
   * @param validate A validator of T. Validates the object before this
   *        parameters or any subparameters are set.
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name,
           class T,
           SetCallback<T> Callback,
           validate::ValidatorOf<T> Validate>
    requires(std::is_copy_constructible_v<T> and not std::is_const_v<T>)
  constexpr concepts::Command auto param(
    Name name, T &t, Callback set_callback, Validate validate, recursive_t r) {
    (void)name;
    (void)r;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 t,
                 std::move(set_callback),
                 std::move(validate),
                 recursive);
  }

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * void on_settings_update(const Settings& s);
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            "a description"_sc,
   *            settings,
   *            &on_settings_update,
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param description the parameter description
   * @param t the parameter object
   * @param set_callback called when the parameter or any subparameter is set.
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name,
           SC Description,
           class T,
           SetCallback<T> Callback,
           validate::ValidatorOf<T> Validate>
    requires(std::is_copy_constructible_v<T> and not std::is_const_v<T>)
  constexpr concepts::Command auto param(Name name,
                                         Description description,
                                         T &t,
                                         Callback set_callback,
                                         recursive_t r) {
    (void)name;
    (void)description;
    (void)r;
    if constexpr (ctti::dtl::num_members<T>() == 1 and
                  not concepts::Struct<T>) {
      return param(Name{},
                   Description{},
                   t,
                   // setter
                   [&t, cb = set_callback](const T &t_set) -> Error {
                     t = t_set;
                     cb(t);
                     return Error::none;
                   });
    } else if constexpr (ctti::dtl::num_members<T>() == 1 and
                         concepts::Struct<T>) {
      using CharT = get_char_t<Name>;
      return param(
        Name{},
        Description{},
        t,
        // setter
        [&t, cb = set_callback](const T &t_set) -> Error {
          t = t_set;
          cb(t);
          return Error::none;
        },
        // validator
        param(
          ctti::dtl::member_name<T, 0, CharT>(),
          NoDescription<CharT>{},
          ctti::dtl::get_ref<0>(t),
          // set callback
          [&t, cb = set_callback](
            const ctti::dtl::member_type_t<T, 0> &) -> void { cb(t); },
          recursive));
    } else {
      using CharT = get_char_t<Name>;
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return param(
          Name{},
          Description{},
          t,
          // setter
          [&t, cb = set_callback](const T &t_set) -> Error {
            t = t_set;
            cb(t);
            return Error::none;
          },
          param(
            ctti::dtl::member_name<T, Is, CharT>(),
            NoDescription<CharT>{},
            ctti::dtl::get_ref<Is>(t),
            // set callback
            [&t, cb = set_callback](
              const ctti::dtl::member_type_t<T, Is> &) -> void { cb(t); },
            recursive)...);
      }(std::make_index_sequence<ctti::dtl::num_members<T>()>{});
    }
  }

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * void on_settings_update(const Settings& s);
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            settings,
   *            &on_settings_update,
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param t the parameter object
   * @param set_callback called when the parameter or any subparameter is set.
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name,
           class T,
           SetCallback<T> Callback,
           validate::ValidatorOf<T> Validate>
    requires(std::is_copy_constructible_v<T> and not std::is_const_v<T>)
  constexpr concepts::Command auto
  param(Name name, T &t, Callback set_callback, recursive_t r) {
    (void)name;
    (void)r;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 t,
                 std::move(set_callback),
                 recursive);
  }

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            "a description"_sc,
   *            settings,
   *            [](const Settings& s)-> bool{
   *              return s.c >= 'a' and s.c <= 'z' and s.subsettings.a >= 100;
   *            },
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param description the parameter description
   * @param t the parameter object
   * @param validate A validator of T. Validates the object before this
   *        parameters or any subparameters are set.
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name, SC Description, class T, validate::ValidatorOf<T> Validate>
    requires(std::is_copy_constructible_v<T> and not std::is_const_v<T>)
  constexpr concepts::Command auto param(Name name,
                                         Description description,
                                         T &t,
                                         Validate validate,
                                         recursive_t r) {
    (void)name;
    (void)description;
    (void)r;
    if constexpr (ctti::dtl::num_members<T>() == 1 and
                  not concepts::Struct<T>) {
      return param(Name{}, Description{}, t);
    } else if constexpr (ctti::dtl::num_members<T>() == 1) {
      using CharT = get_char_t<Name>;
      return param(
        Name{},
        Description{},
        t,
        param(
          ctti::dtl::member_name<T, 0, CharT>(),
          NoDescription<CharT>{},
          ctti::dtl::get_ref<0>(t),
          [&t, validate](const ctti::dtl::member_type_t<T, 0> &val) -> bool {
            T t_ = t;
            ctti::dtl::get_ref<0>(t_) = val;
            return validate(t_);
          },
          recursive));
    } else {
      using CharT = get_char_t<Name>;
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return param(
          Name{},
          Description{},
          t,
          validate,
          param(
            ctti::dtl::member_name<T, Is, CharT>(),
            NoDescription<CharT>{},
            ctti::dtl::get_ref<Is>(t),
            [&t, validate](const ctti::dtl::member_type_t<T, Is> &val) -> bool {
              T t_ = t;
              ctti::dtl::get_ref<Is>(t_) = val;
              return validate(t_);
            },
            recursive)...);
      }(std::make_index_sequence<ctti::dtl::num_members<T>()>{});
    }
  }

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            settings,
   *            [](const Settings& s)-> bool{
   *              return s.c >= 'a' and s.c <= 'z' and s.subsettings.a >= 100;
   *            },
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param t the parameter object
   * @param validate A validator of T. Validates the object before this
   *        parameters or any subparameters are set.
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name, class T, validate::ValidatorOf<T> Validate>
    requires(std::is_copy_constructible_v<T> and not std::is_const_v<T>)
  constexpr concepts::Command auto
  param(Name name, T &t, Validate validate, recursive_t r) {
    (void)name;
    (void)r;
    return param(Name{},
                 NoDescription<get_char_t<Name>>{},
                 t,
                 std::move(validate),
                 recursive);
  }

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            "a description"_sc,
   *            settings,
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param description the parameter description
   * @param t the parameter object
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name, SC Description, class T>
  constexpr concepts::Command auto
  param(Name name, Description description, T &t, recursive_t r) {
    (void)name;
    (void)description;
    (void)r;
    using T_ = std::remove_cvref_t<T>;
    if constexpr (ctti::dtl::num_members<T_>() == 1 and
                  not concepts::Struct<T_>) {
      return param(Name{}, Description{}, t);
    } else if constexpr (ctti::dtl::num_members<T_>() == 1) {
      using CharT = get_char_t<Name>;
      return param(Name{},
                   Description{},
                   t,
                   param(ctti::dtl::member_name<T_, 0, CharT>(),
                         NoDescription<CharT>{},
                         ctti::dtl::get_ref<0>(t),
                         recursive));
    } else {
      using CharT = get_char_t<Name>;
      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return param(Name{},
                     Description{},
                     t,
                     param(ctti::dtl::member_name<T_, Is, CharT>(),
                           NoDescription<CharT>{},
                           ctti::dtl::get_ref<Is>(t),
                           recursive)...);
      }(std::make_index_sequence<ctti::dtl::num_members<T_>()>{});
    }
  }

  /**
   * construct a parameter command for t and adds all members of t as
   * subparameters/subcommands.
   *
   * Example:
   * ```
   * struct SubSettings{
   *  int a = 100;
   * };
   *
   * struct Settings{
   *  char c = 'a';
   *  SubSettings subsettings;
   * };
   *
   * static Settings settings;
   *
   *
   * cli::param("settings"_sc,
   *            settings,
   *            cli::recursive);
   * ```
   *
   * @param name the parameter name
   * @param t the parameter object
   * @param r must be cli::recursive.
   * @return parameter command
   */
  template<Id Name, class T>
  constexpr concepts::Command auto param(Name name, T &t, recursive_t r) {
    (void)name;
    (void)r;
    return param(Name{}, NoDescription<get_char_t<Name>>{}, t, recursive);
  }

  /// @}

} // namespace cli::params
#endif
