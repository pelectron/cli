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

  template<typename T>
  concept IsMemberPointer =
    std::is_member_pointer_v<std::remove_reference_t<T>>;

  template<typename T>
  concept MutMemberPointer =
    IsMemberPointer<T> and not std::is_const_v<mem_data_type<T>>;

  template<auto Ptr>
  inline constexpr bool is_mut_member_pointer =
    MutMemberPointer<std::remove_cvref_t<decltype(Ptr)>>;

  template<auto Ptr>
  inline constexpr bool is_member_pointer =
    IsMemberPointer<std::remove_cvref_t<decltype(Ptr)>>;

  template<auto &Object>
  struct is_object_t : std::false_type {};

  template<auto &Object>
    requires(not std::is_member_pointer_v<
              std::remove_cvref_t<decltype(Object)>>) and
            (not std::is_pointer_v<
              std::remove_reference_t<decltype(Object)>>) and
            (not std::is_member_function_pointer_v<
              std::remove_reference_t<decltype(Object)>>)
  struct is_object_t<Object> : std::true_type {};

  template<auto &Object>
  inline constexpr bool is_object = is_object_t<Object>::value;

  /**
   * @brief
   *
   * @tparam T
   */
  template<typename T>
  concept ParamType =
    std::is_constructible_v<T> and std::is_copy_assignable_v<T> and
    (not SC<T>) and (not std::is_const_v<T>) and
    (not std::is_reference_v<T>) and
    (not std::is_member_pointer_v<std::remove_cvref_t<T>>) and
    (not std::is_pointer_v<std::remove_cvref_t<T>>) and
    (not std::is_member_function_pointer_v<std::remove_cvref_t<T>>);

  template<auto &Object>
  struct is_mut_object_t : std::false_type {};

  template<auto &Object>
    requires(not std::is_const_v<std::remove_reference_t<decltype(Object)>>) and
            (not std::is_member_pointer_v<
              std::remove_cvref_t<decltype(Object)>>) and
            (not std::is_pointer_v<
              std::remove_reference_t<decltype(Object)>>) and
            (not std::is_member_function_pointer_v<
              std::remove_reference_t<decltype(Object)>>)
  struct is_mut_object_t<Object> : std::true_type {};

  template<auto &Object>
  inline constexpr bool is_mut_object = is_mut_object_t<Object>::value;

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
        requires(sizeof...(SubCommands) == 0)
      constexpr Param(Name,
                      Description,
                      Type,
                      Get_ &&get,
                      Set_ &&set,
                      Parse_ &&parse,
                      Format_ &&format,
                      Validate_ &&validate,
                      Tuple<>) noexcept
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
             validate::Validator Validate>
    Param(Name,
          Description,
          Type,
          Get &&get,
          Set &&set,
          Parse &&parse,
          Format &&format,
          Validate &&validate,
          Tuple<>) -> Param<std::decay_t<Name>,
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

    template<auto MemberPointer,
             Id Name,
             SC Description,
             SC Type,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    struct MemberDataT {
      using char_type = get_char_t<Name>;
      cli::Tuple<SubCommands...> subcommands;
      CLI_NO_UNIQUE_ADDRESS Parse parse;
      CLI_NO_UNIQUE_ADDRESS Format format;
      CLI_NO_UNIQUE_ADDRESS Validate validate;
    };

    template<auto MemberPointer,
             Id Name,
             SC Description,
             SC Type,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    constexpr auto make_member_data(Name,
                                    Description,
                                    Type,
                                    Parse &&p,
                                    Format &&f,
                                    Validate &&v,
                                    SubCommands &&...s) {
      return MemberDataT<MemberPointer,
                         Name,
                         Description,
                         Type,
                         std::decay_t<Parse>,
                         std::decay_t<Format>,
                         std::decay_t<Validate>,
                         std::decay_t<SubCommands>...>{
        .subcommands{std::forward<SubCommands>(s)...},
        .parse{std::forward<Parse>(p)},
        .format{std::forward<Format>(f)},
        .validate{std::forward<Validate>(v)}};
    };

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

      constexpr MemberData(const MemberData &) = default;
      constexpr MemberData(MemberData &&) = default;
      constexpr MemberData &operator=(const MemberData &) = default;
      constexpr MemberData &operator=(MemberData &&) = default;

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

    template<typename T>
    inline constexpr bool is_member_data_t_v = false;
    template<auto MemberPointer,
             class Name,
             class Description,
             class Help,
             class Parse,
             class Format,
             class Validate,
             class... SubCommands>
    inline constexpr bool is_member_data_t_v<MemberDataT<MemberPointer,
                                                         Name,
                                                         Description,
                                                         Help,
                                                         Parse,
                                                         Format,
                                                         Validate,
                                                         SubCommands...>> =
      true;

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
      const T *value_;
      MemberPtr member;
      constexpr Error
      operator()(std::remove_const_t<mem_data_type<MemberPtr>> &t) noexcept {
        CLI_ASSERT(value_);
        t = value_->*member;
        return Error::none;
      }
    };

    template<typename T, typename MemberPtr>
    struct MemDataSet {
      T *value_;
      MemberPtr member;
      constexpr Error operator()(const mem_data_type<MemberPtr> &t) noexcept {
        CLI_ASSERT(value_);
        value_->*member = t;
        return Error::none;
      }
    };

    template<typename T, typename MemberPtr>
    struct MemDataSet<const T, MemberPtr> {
      constexpr Error operator()(const mem_data_type<MemberPtr> &) noexcept {
        return Error::cant_set_param;
      }
    };

    template<typename T, auto MemberPtr>
    struct MemberGet {
      const T *value_;
      constexpr Error operator()(
        std::remove_const_t<mem_data_type<decltype(MemberPtr)>> &t) noexcept {
        CLI_ASSERT(value_);
        t = value_->*MemberPtr;
        return Error::none;
      }
    };

    template<typename T, auto MemberPtr>
    struct MemberSet {
      T *value_;
      constexpr Error
      operator()(const mem_data_type<decltype(MemberPtr)> &t) noexcept {
        CLI_ASSERT(value_);
        value_->*MemberPtr = t;
        return Error::none;
      }
    };

    template<typename T, auto MemberPtr>
    struct MemberSet<const T, MemberPtr> {
      static constexpr invalid_tag_t invalid_tag{};
      constexpr Error
      operator()(const mem_data_type<decltype(MemberPtr)> &t) noexcept {
        return Error::cant_set_param;
      }
    };

    template<const auto &Object, auto MemberPtr>
    struct MemberGetObject {
      constexpr Error operator()(
        std::remove_const_t<mem_data_type<decltype(MemberPtr)>> &t) noexcept {
        t = Object.*MemberPtr;
        return Error::none;
      }
    };

    template<typename T>
    inline constexpr bool is_mut_ref = false;
    template<typename T>
    inline constexpr bool is_mut_ref<T &> = true;
    template<typename T>
    inline constexpr bool is_mut_ref<const T &> = false;
    template<typename T>
    inline constexpr bool is_const_ref = false;
    template<typename T>
    inline constexpr bool is_const_ref<const T &> = true;

    static constinit int i = 0;
    static constexpr int ci = 0;
    template<auto &obj>
    constexpr bool verify_mut() {
      return is_mut_ref<decltype(obj)>;
    }
    template<auto &obj>
    constexpr bool verify_const() {
      return is_const_ref<decltype(obj)>;
    }
    static_assert(verify_mut<i>());
    static_assert(not verify_mut<ci>());
    static_assert(not verify_const<i>());
    static_assert(verify_const<ci>());

    template<auto &Object, auto MemberPtr>
    struct MemberSetObject {
      constexpr Error
      operator()(const std::remove_const_t<mem_data_type<decltype(MemberPtr)>>
                   &t) noexcept {
        static_assert(is_const_ref<decltype(Object)>);
        return Error::cant_set_param;
      }
    };

    template<auto &Object, auto MemberPtr>
      requires is_mut_ref<decltype(Object)>
    struct MemberSetObject<Object, MemberPtr> {
      constexpr Error
      operator()(const std::remove_const_t<mem_data_type<decltype(MemberPtr)>>
                   &t) noexcept {
        Object.*MemberPtr = t;
        return Error::none;
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
      return Param{
        Name{},
        Description{},
        Help{},
        MemDataGet<T, MemberPointer>{&obj, member_data.member},
        MemDataSet<T, MemberPointer>{&obj, member_data.member},
        std::move(member_data.parse),
        std::move(member_data.format),
        std::move(member_data.validate),
        std::move(member_data.subcommands)
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
      return Param{
        Name{},
        Description{},
        Help{},
        MemDataGet<T, MemberPointer>{&obj, member_data.member},
        MemDataSet<const T, MemberPointer>{},
        std::move(member_data.parse),
        std::move(member_data.format),
        std::move(member_data.validate),
        std::move(member_data.subcommands)
      };
    }

    template<class T,
             auto MemberPointer,
             Id Name,
             SC Description,
             SC Help,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    constexpr auto to_cmd(const T &obj,
                          MemberDataT<MemberPointer,
                                      Name,
                                      Description,
                                      Help,
                                      Parse,
                                      Format,
                                      Validate,
                                      SubCommands...> member_data) noexcept {
      return Param{Name{},
                   Description{},
                   Help{},
                   MemberGet<T, MemberPointer>{&obj},
                   MemberSet<const T, MemberPointer>{},
                   std::move(member_data.parse),
                   std::move(member_data.format),
                   std::move(member_data.validate),
                   std::move(member_data.subcommands)};
    }

    template<class T,
             auto MemberPointer,
             Id Name,
             SC Description,
             SC Help,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    constexpr auto to_cmd(T &obj,
                          MemberDataT<MemberPointer,
                                      Name,
                                      Description,
                                      Help,
                                      Parse,
                                      Format,
                                      Validate,
                                      SubCommands...> member_data) noexcept {
      return Param{Name{},
                   Description{},
                   Help{},
                   MemberGet<T, MemberPointer>{&obj},
                   MemberSet<T, MemberPointer>{&obj},
                   std::move(member_data.parse),
                   std::move(member_data.format),
                   std::move(member_data.validate),
                   std::move(member_data.subcommands)};
    }

    template<auto &Object,
             bool Const,
             auto MemberPointer,
             Id Name,
             SC Description,
             SC Help,
             parse::Parser Parse,
             format::Formatter Format,
             validate::Validator Validate,
             concepts::Command... SubCommands>
    constexpr auto to_cmd(MemberDataT<MemberPointer,
                                      Name,
                                      Description,
                                      Help,
                                      Parse,
                                      Format,
                                      Validate,
                                      SubCommands...> member_data) noexcept {
      if constexpr (not Const) {
        return Param{Name{},
                     Description{},
                     Help{},
                     MemberGetObject<Object, MemberPointer>{},
                     MemberSetObject<Object, MemberPointer>{},
                     std::move(member_data.parse),
                     std::move(member_data.format),
                     std::move(member_data.validate),
                     std::move(member_data.subcommands)};
      } else
        return Param{
          Name{},
          Description{},
          Help{},
          MemberGetObject<Object, MemberPointer>{},
          InvalidSet<
            std::remove_cvref_t<mem_data_type<decltype(MemberPointer)>>>{},
          std::move(member_data.parse),
          std::move(member_data.format),
          std::move(member_data.validate),
          std::move(member_data.subcommands)};
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

    template<auto &Object,
             bool Const,
             class CommandOrMemberDataOrMemberFunction>
    constexpr auto
    transform(CommandOrMemberDataOrMemberFunction &&mem) noexcept {
      if constexpr (concepts::Command<std::remove_cvref_t<
                      CommandOrMemberDataOrMemberFunction>>) {
        return mem;
      } else if constexpr (is_member_data_t_v<std::remove_cvref_t<
                             CommandOrMemberDataOrMemberFunction>>) {
        return to_cmd<Object, Const>(
          std::forward<CommandOrMemberDataOrMemberFunction>(mem));
      } else {
        using dtl::to_cmd;
        using funcs::dtl::to_cmd;
        return to_cmd(Object,
                      std::forward<CommandOrMemberDataOrMemberFunction>(mem));
      }
    }
  } // namespace dtl

  /**
   * @brief This concept is satisfied if T is a Command or a member data
   * command, or a member function command.
   * @tparam T
   */
  template<typename T>
  concept CmdOrMemDataOrMemFun =
    concepts::Command<std::remove_cvref_t<T>> or
    dtl::is_member_data_v<std::remove_cvref_t<T>> or
    dtl::is_member_data_t_v<std::remove_cvref_t<T>> or
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
   *
   * Virtual parameters can be used to group commands.
   *
   * See [here](docs.md#virtual-parameters) for more details.
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
   * See [here](docs.md#parameters-without-object-declarations) for more details.
   * @{
   */
  // clang-format on

  /**
   * creates a parameter command from its individual parts.
   *
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * creates a parameter command with a custom validator and default parser and
   * formatter.
   *
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
    requires ParamType<T>
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
    requires ParamType<T>
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
           Id Name,
           SC Description,
           GetterOf<T> Get,
           format::FormatterOf<T, get_char_t<Name>> Format,
           concepts::Command... SubCommands>
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * creates a parameter command with a custom validator.
   *
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * creates a read-only parameter command  with default formatter.
   *
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
    requires ParamType<T>
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
   * creates a write-only parameter command with default parser and validator.
   *
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
    requires ParamType<T>
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
   * The basic forms are:
   *
   * ```
   * param(name, description, t, get, set, parse, format, validate,
   * subcommands...);
   *
   * param<t>(   description,    get, set, parse, format, validate,
   * subcommands...);
   * ```
   *
   * The parts have the following functions:
   * - name: a string_constant that makes up the command name
   * - description: a string_constant that describes the command
   * - t: the variable of type T.
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
   * See [here](docs.md#parameters-with-objectvariable-declarations) for more
   * details.
   * @{
   */
  // clang-format on

  /**
   * creates a parameter command from its individual parts.
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
    requires ParamType<T>
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
   * creates a parameter command from its individual parts. The default
   * validator is used.
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * is used.
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
    requires ParamType<T>
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
   * is used.
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * is used.
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
    requires ParamType<T>
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
   * is used.
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name, typename T, CmdOrMemDataOrMemFun... SubCommands>
    requires ParamType<T>
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
    requires is_mut_object<Object>
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
      dtl::transform<Object, false>(std::forward<SubCommands>(cmds))...};
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           SetterOf<object_type<Object>> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
   * @param validate the validator used when parsing a T. See @ref Validation.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           validate::ValidatorOf<object_type<Object>> Validate,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
   * @param set the setter of the parameter. See cli::params::Setter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SetterOf<object_type<Object>> Set,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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
    requires is_mut_object<Object>
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

  /// @}

  // clang-format off
  /**
   * @defgroup params-with-const-object Parameters With const Object/Variable Declarations 
   * @ingroup Parameters
   * Read-only parameters for const objects can be defined with the
   * following functions.
   *
   * The base forms are:
   * ```
   * param(name, description, t, get, format, subcommands...)
   * param<t>(   description,    get, format, subcommands...);
   * ```
   *
   * See [here](docs.md#parameters-with-const-objectvariable-declarations) for more details.
   * @{
  */
  // clang-format on

  /**
   * creates a read-only parameter command with a custom getter and formatter.
   *
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * @param name the name of the parameter. Must be a cli::string_constant.
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
    requires ParamType<T>
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
    requires ParamType<T>
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
   * @param name the name of the parameter. Must be a cli::string_constant.
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
    requires ParamType<T>
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
   * @param name the name of the parameter. Must be a cli::string_constant.
   * @param t the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<Id Name, typename T, CmdOrMemDataOrMemFun... SubCommands>
    requires ParamType<T>
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
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           GetterOf<const_object_type<Object>> Get,
           format::FormatterOf<const_object_type<Object>,
                               get_char_t<Description>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description,
        Get &&get,
        Format &&format,
        SubCommands &&...cmds) noexcept {
    (void)description;
    using T = std::remove_cvref_t<decltype(Object)>;
    using char_type = get_char_t<Description>;
    return dtl::Param{
      ctti::object_name<Object, char_type>(),
      Description{},
      ctti::name<T, char_type>(),
      std::forward<Get>(get),
      dtl::InvalidSet<T>{},
      parse::NoParse<T, char_type>{},
      std::forward<Format>(format),
      validate::DefaultValidate<T>{},
      dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a read-only parameter command with default formatter, If Object is
   * const. If Object is mutable, this will create a read-write parameter.
   *
   * @ingroup params-with-object
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           GetterOf<const_object_type<Object>> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description, Get &&get, SubCommands &&...cmds) noexcept {
    (void)description;
    using T = std::remove_cvref_t<decltype(Object)>;
    using char_type = get_char_t<Description>;
    if constexpr (not std::is_const_v<
                    std::remove_reference_t<decltype(Object)>>) {
      return dtl::Param{
        ctti::object_name<Object, char_type>(),
        Description{},
        ctti::name<T, char_type>(),
        std::forward<Get>(get),
        dtl::ObjectSet<Object>{},
        parse::Parse<T, char_type>{},
        format::Format<T, char_type>{},
        validate::DefaultValidate<T>{},
        dtl::transform<Object, false>(std::forward<SubCommands>(cmds))...};
    } else {
      return dtl::Param{
        ctti::object_name<Object, char_type>(),
        Description{},
        ctti::name<T, char_type>(),
        std::forward<Get>(get),
        dtl::InvalidSet<T>{},
        parse::NoParse<T, char_type>{},
        format::Format<T, char_type>{},
        validate::DefaultValidate<T>{},
        dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...};
    }
  }

  /**
   * creates a read-only parameter command with custom formatter.
   *
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param format the Formatter. Used to format a T. See also
   * cli::format::Formatter.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           SC Description,
           format::FormatterOf<const_object_type<Object>,
                               get_char_t<Description>> Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
  [[nodiscard]] constexpr concepts::Command auto param(
    Description description, Format &&format, SubCommands &&...cmds) noexcept {
    (void)description;
    using T = std::remove_cvref_t<decltype(Object)>;
    using char_type = get_char_t<Description>;
    return dtl::Param{
      ctti::object_name<Object, char_type>(),
      Description{},
      ctti::name<T, char_type>(),
      dtl::ObjectGet<Object>{},
      dtl::InvalidSet<T>{},
      parse::NoParse<T, char_type>{},
      format::Format<T, char_type>{},
      validate::DefaultValidate<T>{},
      dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a read-only parameter command with default formatter and getter if
   * Object is const. If Object is mutable, this will create a read-write
   * parameter.
   *
   * @ingroup params-with-object
   * @tparam Object the parameter value
   * @param description the description of the parameter. Must be a
   * cli::string_constant.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object, SC Description, CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
  [[nodiscard]] constexpr concepts::Command auto
  param(Description description, SubCommands &&...cmds) noexcept {
    (void)description;
    using T = std::remove_cvref_t<decltype(Object)>;
    using char_type = get_char_t<Description>;
    if constexpr (not std::is_const_v<
                    std::remove_reference_t<decltype(Object)>>) {
      return dtl::Param{
        ctti::object_name<Object, char_type>(),
        Description{},
        ctti::name<T, char_type>(),
        dtl::ObjectGet<Object>{},
        dtl::ObjectSet<Object>{},
        parse::Parse<T, char_type>{},
        format::Format<T, char_type>{},
        validate::DefaultValidate<T>{},
        dtl::transform<Object, false>(std::forward<SubCommands>(cmds))...};
    } else {
      return dtl::Param{
        ctti::object_name<Object, char_type>(),
        Description{},
        ctti::name<T, char_type>(),
        dtl::ObjectGet<Object>{},
        dtl::InvalidSet<T>{},
        parse::NoParse<T, char_type>{},
        format::Format<T, char_type>{},
        validate::DefaultValidate<T>{},
        dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...};
    }
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
  template<auto &Object,
           GetterOf<const_object_type<Object>> Get,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
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
      dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...};
  }

  /**
   * creates a read-only parameter command with default formatter if Object is
   * const. If Object is mutable, this will create a read-write parameter.
   *
   * @ingroup params-with-object
   * @tparam Object the parameter value
   * @param get the getter of the parameter. See cli::params::Getter for
   * additional info.
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object,
           GetterOf<const_object_type<Object>> Get,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
  [[nodiscard]] constexpr concepts::Command auto
  param(Get &&get, SubCommands &&...cmds) noexcept {
    using T = std::remove_cvref_t<decltype(Object)>;
    if constexpr (not std::is_const_v<
                    std::remove_reference_t<decltype(Object)>>)
      return dtl::Param{
        ctti::object_name<Object, char>(),
        NoDescription<char>{},
        ctti::name<T>(),
        std::forward<Get>(get),
        dtl::ObjectSet<Object>{},
        parse::Parse<T, char>{},
        format::Format<T, char>{},
        validate::DefaultValidate<T>{},
        dtl::transform<Object, false>(std::forward<SubCommands>(cmds))...};
    else
      return dtl::Param{
        ctti::object_name<Object, char>(),
        NoDescription<char>{},
        ctti::name<T>(),
        std::forward<Get>(get),
        dtl::InvalidSet<T>{},
        parse::NoParse<T, char>{},
        format::Format<T, char>{},
        validate::DefaultValidate<T>{},
        dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...};
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
  template<auto &Object,
           format::Formatter Format,
           CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
  [[nodiscard]] constexpr concepts::Command auto
  param(Format &&format, SubCommands &&...cmds) noexcept {
    using value_type = format::formatter_value_type_t<Format>;
    using char_type =
      typename format::formatter_buffer_type_t<Format>::value_type;

    static_assert(std::is_same_v<value_type, const_object_type<Object>>,
                  "format must be able to format the Object");

    return param<Object>(
      NoDescription<char_type>{},
      dtl::DefaultGet<const_object_type<Object>>{Object},
      std::forward<Format>(format),
      dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...);
  }

  /**
   * creates a read-only parameter command with default formatter and getter if
   * Object is const. Else this will create a read-write parameter.
   *
   * @ingroup params-with-object
   *
   * @tparam Object the parameter value
   * @param cmds additional optional subcommands
   * @return a Command
   */
  template<auto &Object, CmdOrMemDataOrMemFun... SubCommands>
    requires is_object<Object>
  [[nodiscard]] concepts::Command auto param(SubCommands &&...cmds) noexcept {
    using cT = std::remove_reference_t<decltype(Object)>;
    using T = std::remove_cvref_t<decltype(Object)>;
    if constexpr (not std::is_const_v<cT>) {
      return dtl::Param{
        ctti::object_name<Object, char>(),
        NoDescription<char>{},
        ctti::name<T, char>(),
        dtl::ObjectGet<Object>{},
        dtl::ObjectSet<Object>{},
        parse::Parse<T, char>{},
        format::Format<T, char>{},
        validate::DefaultValidate<T>{},
        dtl::transform(Object, std::forward<SubCommands>(cmds))...};
    } else {
      return dtl::Param{
        ctti::object_name<Object, char>(),
        NoDescription<char>{},
        ctti::name<T, char>(),
        dtl::ObjectGet<Object>{},
        dtl::InvalidSet<T>{},
        parse::NoParse<T, char>{},
        format::Format<T, char>{},
        validate::DefaultValidate<T>{},
        dtl::transform<Object, true>(std::forward<SubCommands>(cmds))...};
    }
  }
  /// @}

  /**
   * @defgroup memdata Member Data
   * @ingroup Parameters
   *
   * Member data commands are used to easily setup subcommands for parameters
   * with subobjects.
   *
   * There are two base forms:
   *
   * ```cpp
   * param(name, description, ptr_to_member, parse, format, validate,
   * subcommands...);
   *
   * param<ptr_to_member>(description, parse, format, validate,
   * subcommands...);
   * ```
   *
   * See [here](docs.md#member-data-parameters) for more details.
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
    requires MutMemberPointer<MemberPointer>
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
    requires MutMemberPointer<MemberPointer>
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
    requires MutMemberPointer<MemberPointer>
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
    requires MutMemberPointer<MemberPointer>
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
    requires MutMemberPointer<MemberPointer>
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
    requires MutMemberPointer<MemberPointer>
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
    requires IsMemberPointer<MemberPointer>
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
        parse::NoParse<std::remove_const_t<T>, get_char_t<Name>>{},
        format::Format<std::remove_const_t<T>, get_char_t<Name>>{},
        validate::DefaultValidate<std::remove_const_t<T>>{},
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
   * @param name the name of f. Must be a cli::string_constant.
   * @param member member data pointer
   * @param cmds the subcommands
   */
  template<Id Name, class MemberPointer, concepts::Command... SubCommands>
    requires IsMemberPointer<MemberPointer>
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
   * @tparam MemberPointer pointer to the member variable
   * @param description the description. Must be a string_constant.
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
    requires is_mut_member_pointer<MemberPointer>
  [[nodiscard]] constexpr auto param(Description description,
                                     Parse &&parse,
                                     Format &&format,
                                     Validate &&validate,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    return make_member_data<MemberPointer>(
      ctti::value_name<MemberPointer, get_char_t<Description>>(),
      Description{},
      cli::ctti::name<mem_data_type<decltype(MemberPointer)>,
                      get_char_t<Description>>(),
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @tparam MemberPointer pointer to the member variable
   * @param description the description. Must be a string_constant.
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
    requires is_mut_member_pointer<MemberPointer>
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
   * @tparam MemberPointer pointer to the member variable
   * @param description the description. Must be a string_constant.
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    auto MemberPointer,
    SC Description,
    validate::ValidatorOf<mem_data_type<decltype(MemberPointer)>> Validate,
    concepts::Command... SubCommands>
    requires is_mut_member_pointer<MemberPointer>
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
   * @tparam MemberPointer pointer to the member variable
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
    requires is_mut_member_pointer<MemberPointer>
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
    return make_member_data<MemberPointer>(
      ctti::value_name<MemberPointer, parse_char_type>(),
      NoDescription<parse_char_type>{},
      cli::ctti::name<mem_data_type<decltype(MemberPointer)>,
                      parse_char_type>(),
      std::forward<Parse>(parse),
      std::forward<Format>(format),
      std::forward<Validate>(validate),
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a member data parameter. Must be used together with a parent
   * command.
   *
   * @tparam MemberPointer pointer to the member variable
   * @param parse a parser of the type pointed to by member
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<auto MemberPointer,
           parse::Parser Parse,
           format::Formatter Format,
           concepts::Command... SubCommands>
    requires is_mut_member_pointer<MemberPointer>
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
   * @tparam MemberPointer pointer to the member variable
   * @param validate the validator used for validating the member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<
    auto MemberPointer,
    validate::ValidatorOf<mem_data_type<decltype(MemberPointer)>> Validate,
    concepts::Command... SubCommands>
    requires is_mut_member_pointer<MemberPointer>
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
   * command. If MemberPointer points to const data, the created parameter will
   * be read-only. Else the parameter will be read-write.
   *
   * @ingroup const-memdata
   * @tparam MemberPointer pointer to the member variable
   * @param description the description of MemberPointer. Must be a
   * cli::string_constant.
   * @param cmds the subcommands
   */
  template<auto MemberPointer, SC Description, concepts::Command... SubCommands>
    requires is_member_pointer<MemberPointer>
  [[nodiscard]] constexpr auto param(Description description,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    using T = mem_data_type<decltype(MemberPointer)>;
    if constexpr (std::is_const_v<T>) {
      return make_member_data<MemberPointer>(
        ctti::value_name<MemberPointer, get_char_t<Description>>(),
        Description{},
        cli::ctti::name<std::remove_const_t<T>, get_char_t<Description>>(),
        parse::NoParse<std::remove_const_t<T>, get_char_t<Description>>{},
        format::Format<std::remove_const_t<T>, get_char_t<Description>>{},
        validate::DefaultValidate<std::remove_const_t<T>>{},
        std::forward<SubCommands>(cmds)...);
    } else
      return dtl::make_member_data<MemberPointer>(
        ctti::value_name<MemberPointer, get_char_t<Description>>(),
        Description{},
        cli::ctti::name<T, get_char_t<Description>>(),
        parse::Parse<T, get_char_t<Description>>{},
        format::Format<T, get_char_t<Description>>{},
        validate::DefaultValidate<T>{},
        std::forward<SubCommands>(cmds)...);
  }

  /**
   * creates a member data subcommand. Must be used together with a parent
   * command. If MemberPointer points to const data, the created parameter will
   * be read-only. Else the parameter will be read-write.
   *
   * @ingroup const-memdata
   * @ingroup const-memdata
   * @tparam MemberPointer pointer to the member variable
   * @param cmds the subcommands
   */
  template<auto MemberPointer, concepts::Command... SubCommands>
    requires is_member_pointer<MemberPointer>
  [[nodiscard]] constexpr auto param(SubCommands &&...cmds) noexcept {
    using namespace dtl;
    return param<MemberPointer>(NoDescription<char>{},
                                std::forward<SubCommands>(cmds)...);
  }

  /// @}

  /**
   * @defgroup const-memdata Const Member Data
   * @ingroup Parameters
   *
   * Const member data commands are used to easily setup read-only subcommands
   * for parameters with objects.
   *
   * See [here](docs.md#member-data-parameters) fo rmore details.
   *
   * @{
   */

  /**
   * @brief creates a read-only member data parameter. Must be used as a
   * subcommand.
   *
   * @param name the name, must be a string_constant.
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
    requires IsMemberPointer<MemberPointer>
  [[nodiscard]] constexpr auto param(Name name,
                                     Description description,
                                     MemberPointer member,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    (void)description;
    using T = std::remove_cvref_t<mem_data_type<MemberPointer>>;
    using char_type = get_char_t<Name>;
    using namespace dtl;
    return MemberData{Name{},
                      Description{},
                      cli::ctti::name<T, char_type>(),
                      member,
                      parse::NoParse<T, char_type>{},
                      std::forward<Format>(format),
                      validate::DefaultValidate<T>{},
                      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief creates a read-only member data parameter. Must be used as a
   * subcommand.
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
    requires IsMemberPointer<MemberPointer>
  [[nodiscard]] constexpr auto param(Name name,
                                     MemberPointer member,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)name;
    using T = std::remove_cvref_t<mem_data_type<MemberPointer>>;
    using char_type = get_char_t<Name>;
    using namespace dtl;
    return MemberData{Name{},
                      NoDescription<char_type>{},
                      cli::ctti::name<T, char_type>(),
                      member,
                      parse::NoParse<T, char_type>{},
                      std::forward<Format>(format),
                      validate::DefaultValidate<T>{},
                      std::forward<SubCommands>(cmds)...};
  }

  /**
   * @brief creates a read-only member data parameter. Must be used together
   * with a parent command. The name of the parameter will be the name of the
   * member pointed to by MemberPointer.
   *
   * @ingroup const-memdata
   * @tparam MemberPointer pointer to the member variable
   * @param description the description. Must be a string_constant.
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
    requires is_member_pointer<MemberPointer>
  [[nodiscard]] constexpr auto param(Description description,
                                     Format &&format,
                                     SubCommands &&...cmds) noexcept {
    (void)description;
    using namespace dtl;
    return make_member_data<MemberPointer>(
      ctti::value_name<MemberPointer, get_char_t<Description>>(),
      Description{},
      cli::ctti::name<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        get_char_t<Description>>(),
      parse::NoParse<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        get_char_t<Description>>{},
      std::forward<Format>(format),
      validate::DefaultValidate<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>>{},
      std::forward<SubCommands>(cmds)...);
  }

  /**
   * @brief creates a read-only member data parameter. Must be used together
   * with a parent command. The name of the parameter will be the name of the
   * member pointed to by MemberPointer.
   *
   * @tparam MemberPointer to the member variable
   * @param format a formatter for the type pointed to by member
   * @param cmds additional subcommands
   * @return a partial command
   */
  template<auto MemberPointer,
           format::FormatterOf<
             std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
             char> Format,
           concepts::Command... SubCommands>
    requires is_member_pointer<MemberPointer>
  [[nodiscard]] constexpr auto param(Format &&format,
                                     SubCommands &&...cmds) noexcept {
    using namespace dtl;
    return make_member_data<MemberPointer>(
      ctti::value_name<MemberPointer, char>(),
      NoDescription<char>{},
      cli::ctti::name<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        char>(),
      parse::NoParse<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>,
        char>{},
      std::forward<Format>(format),
      validate::DefaultValidate<
        std::remove_const_t<mem_data_type<decltype(MemberPointer)>>>{},
      std::forward<SubCommands>(cmds)...);
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
   * See [here](docs.md#recursive-parameters) for more details.
   * @{
   */

  /**
   * construct a parameter command for t and adds all members of t (and their
   * members) as subparameters/subcommands recursively.
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
    requires ParamType<T>
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
