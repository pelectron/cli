#ifndef CLI_UTIL_HPP
#define CLI_UTIL_HPP
#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include "cli/concepts.hpp"
#include "cli/ring_buffer.hpp"
#include "cli/string.hpp"
#include "cli/type_list.hpp"

#ifdef _MSC_VER
#define CLI_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define CLI_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

namespace cli {
  template<class>
  inline constexpr bool always_false = false;
  template<auto V>
  using smallest_type_for_value_t = std::conditional_t<
    V <= std::numeric_limits<uint8_t>::max(),
    uint8_t,
    std::conditional_t<
      V <= std::numeric_limits<uint16_t>::max(),
      uint16_t,
      std::conditional_t<V <= std::numeric_limits<uint32_t>::max(),
                         uint32_t,
                         uint64_t>>>;

  template<auto V>
  struct constant {
    using type = std::remove_cvref_t<decltype(V)>;
    static constexpr type value{V};
  };

  template<auto V>
  inline constexpr auto constant_v = constant<V>{};

  template<class T>
  struct identity {
    using type = T;
  };

  template<typename CharT>
  using NoDescription = string_constant<CharT>;

  struct NoHelp {
    constexpr operator CharView() const noexcept { return ""; }
  };

  template<typename T, typename C>
  concept StringOf = std::constructible_from<const C *, const C *> or
                     std::constructible_from<const C *, std::size_t>;

  namespace dtl {} // namespace dtl

  namespace dtl {

    template<typename F, class Tuple, class... Args, std::size_t... Is>
    constexpr void for_each_impl(std::index_sequence<Is...>,
                                 F &&f,
                                 Tuple &&t,
                                 Args &&...args) {
      (f(std::get<Is>(std::forward<Tuple>(t)), std::forward<Args>(args)...),
       ...);
    }

    template<class Tuple, class F, std::size_t... Is>
    constexpr decltype(auto)
    apply_impl(Tuple &&t, F &&f, std::index_sequence<Is...>) {
      return std::forward<F>(f)(std::get<Is>(std::forward<Tuple>(t))...);
    }
  } // namespace dtl

  template<class Tuple, class F>
  constexpr decltype(auto) apply(Tuple &&t, F &&f) {
    return dtl::apply_impl(std::forward<Tuple>(t),
                           std::forward<F>(f),
                           std::make_index_sequence<
                             std::tuple_size_v<std::remove_cvref_t<Tuple>>>());
  }

  using ArgVector = CharView;

  using OutputIterator = RingBufView<uint8_t>::write_iterator;

  template<class F, class Tuple, class... Args>
  constexpr void for_each(F &&f, Tuple &&t, Args &&...args) {
    dtl::for_each_impl(
      std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>>>{},
      std::forward<F>(f),
      std::forward<Tuple>(t),
      std::forward<Args>(args)...);
  }

  template<class T, class L>
  struct num_cmds;
  template<class T, template<class...> class L, class... SubCmds>
  struct num_cmds<T, L<SubCmds...>> {
    static constexpr std::size_t value =
      1 + (num_cmds<SubCmds, typename SubCmds::sub_command_list>::value + ...);
  };
  template<class T, template<class...> class L>
  struct num_cmds<T, L<>> {
    static constexpr std::size_t value = 1;
  };

  template<concepts::Command C>
  inline constexpr std::size_t num_cmds_v =
    num_cmds<C, typename C::sub_command_list>::value;

  template<class T, class L>
  struct num_levels;
  template<class T, template<class...> class L, class... SubCmds>
  struct num_levels<T, L<SubCmds...>> {
    static constexpr std::size_t value =
      1 +
      std::max(
        {num_levels<SubCmds, typename SubCmds::sub_command_list>::value...});
  };
  template<class T, template<class...> class L>
  struct num_levels<T, L<>> {
    static constexpr std::size_t value = 0;
  };

  template<concepts::Command C>
  inline constexpr std::size_t num_levels_v =
    num_cmds<C, typename C::sub_command_list>::value;

  template<class T, class L>
  struct max_name_length;
  template<class T, template<class...> class L, class... SubCmds>
  struct max_name_length<T, L<SubCmds...>> {
    static constexpr std::size_t value = std::max(
      {T::name.size(),
       max_name_length<SubCmds, typename SubCmds::sub_command_list>::value...});
  };
  template<class T, template<class...> class L>
  struct max_name_length<T, L<>> {
    static constexpr std::size_t value = T::name.size();
  };

  template<concepts::Command C>
  inline constexpr std::size_t max_name_length_v =
    max_name_length<C, typename C::sub_command_list>::value;

  // template <class D, SC C, SC Desc, SC H, Command... SubC>
  // constexpr auto count_cmds(const CommandBase<D, C, Desc, H, SubC...> &c) {
  //   if constexpr (sizeof...(SubC) > 0) {
  //     1 + [&c]<std::size_t... Is>(std::index_sequence<Is...>) {
  //       return (count_cmds(std::get<Is>(c.subcommands)) + ...);
  //     }(std::make_index_sequence<sizeof...(SubC)>());
  //   } else
  //     return 1;
  // }
  //
  // template <class D, SC C, SC Desc, SC H, Command... SubC>
  // constexpr auto count_level(const CommandBase<D, C, Desc, H, SubC...> &c) {
  //   if constexpr (sizeof...(SubC) > 0) {
  //     1 + [&c]<std::size_t... Is>(std::index_sequence<Is...>) {
  //       return std::max(count_level(std::get<Is>(c.subcommands))...);
  //     }(std::make_index_sequence<sizeof...(SubC)>());
  //   } else
  //     return 1;
  // }

  /**
   * extracts the signature from a callable F, respecting const, volatile and
   * noexcept qualifications. The extracted signature can be accessed by its
   * inner typedef ``type``.
   *
   * F is either
   * - a signature type, i.e. ``Ret(Args...)[volatile][const][noexcept]``,
   * - a function pointer type, i.e. ``Ret(*)(Args...)[noexcept]``,
   * - a member function pointer type, i.e.
   * ``Ret(T::*)(Args...)[volatile][const][noexcept]``,
   * - or types with an unambigous call operator, i.e. a special case of the
   * member function pointer.
   *
   * for signature types, the signature itself is returned.
   * for function pointer types, the signature is formed by removing the pointer
   * for member function pointers and callables
   * @{
   */
  template<typename F, typename = void>
  struct extract_signature;
  template<typename R, typename... A>
  struct extract_signature<R(A...)> {
    using type = R(A...);
  };
  template<typename R, typename... A>
  struct extract_signature<R(A...) noexcept> {
    using type = R(A...) noexcept;
  };
  template<typename R, typename... A>
  struct extract_signature<R(A...) const> {
    using type = R(A...) const;
  };
  template<typename R, typename... A>
  struct extract_signature<R(A...) const noexcept> {
    using type = R(A...) const noexcept;
  };
  template<typename R, typename... A>
  struct extract_signature<R (*)(A...)> {
    using type = R(A...);
  };
  template<typename R, typename... A>
  struct extract_signature<R (*)(A...) noexcept> {
    using type = R(A...) noexcept;
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...)> {
    using type = R(A...);
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...) noexcept> {
    using type = R(A...) noexcept;
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...) const> {
    using type = R(A...) const;
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...) const noexcept> {
    using type = R(A...) const noexcept;
  };
  template<typename R, typename... A>
  struct extract_signature<R(A...) volatile> {
    using type = R(A...) volatile;
  };
  template<typename R, typename... A>
  struct extract_signature<R(A...) volatile noexcept> {
    using type = R(A...) volatile noexcept;
  };
  template<typename R, typename... A>
  struct extract_signature<R(A...) volatile const> {
    using type = R(A...) volatile const;
  };
  template<typename R, typename... A>
  struct extract_signature<R(A...) volatile const noexcept> {
    using type = R(A...) volatile const noexcept;
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...) volatile> {
    using type = R(A...) volatile;
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...) volatile noexcept> {
    using type = R(A...) volatile noexcept;
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...) volatile const> {
    using type = R(A...) volatile const;
  };
  template<typename T, typename R, typename... A>
  struct extract_signature<R (T::*)(A...) volatile const noexcept> {
    using type = R(A...) volatile const noexcept;
  };
  template<typename F>
  struct extract_signature<F, std::void_t<decltype(&F::operator())>> {
    using type = typename extract_signature<decltype(&F::operator())>::type;
  };

  template<typename F>
  using extract_signature_t = typename extract_signature<F>::type;
  /// @}

  template<typename Signature>
  struct signature_traits;

  template<typename R, class... A>
  struct signature_traits<R(A...)> {
    using signature_type = R(A...);
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...);
    using invoke_ptr_type = R (*)(void *, A...);
    using invoke_obj_type = void *;
    using mutable_obj_type = void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    template<typename F>
    static constexpr bool matches = std::is_invocable_r_v<R, F, A...>;
  };
  template<typename R, class... A>
  struct signature_traits<R(A...) noexcept> {
    using signature_type = R(A...) noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...) noexcept;
    using invoke_ptr_type = R (*)(void *, A...) noexcept;
    using invoke_obj_type = void *;
    using mutable_obj_type = void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    template<typename F>
    static constexpr bool matches = std::is_nothrow_invocable_r_v<R, F, A...>;
  };
  template<typename R, class... A>
  struct signature_traits<R(A...) const> {
    using signature_type = R(A...) const;
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...);
    using invoke_ptr_type = R (*)(const void *, A...);
    using invoke_obj_type = const void *;
    using mutable_obj_type = void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    template<typename F>
    static constexpr bool matches = std::is_invocable_r_v<R, const F, A...>;
  };
  template<typename R, class... A>
  struct signature_traits<R(A...) const noexcept> {
    using signature_type = R(A...) const noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...) noexcept;
    using invoke_ptr_type = R (*)(const void *, A...) noexcept;
    using invoke_obj_type = const void *;
    using mutable_obj_type = void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    template<typename F>
    static constexpr bool matches =
      std::is_nothrow_invocable_r_v<R, const F, A...>;
  };

  template<typename R, class... A>
  struct signature_traits<R(A...) volatile> {
    using signature_type = R(A...) volatile;
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...);
    using invoke_ptr_type = R (*)(volatile void *, A...);
    using invoke_obj_type = volatile void *;
    using mutable_obj_type = volatile void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = true;
    template<typename F>
    static constexpr bool matches = std::is_invocable_r_v<R, volatile F, A...>;
  };
  template<typename R, class... A>
  struct signature_traits<R(A...) volatile noexcept> {
    using signature_type = R(A...) volatile noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...) noexcept;
    using invoke_ptr_type = R (*)(volatile void *, A...) noexcept;
    using invoke_obj_type = volatile void *;
    using mutable_obj_type = volatile void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = true;
    template<typename F>
    static constexpr bool matches =
      std::is_nothrow_invocable_r_v<R, volatile F, A...>;
  };
  template<typename R, class... A>
  struct signature_traits<R(A...) volatile const> {
    using signature_type = R(A...) const;
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...);
    using invoke_ptr_type = R (*)(volatile const void *, A...);
    using invoke_obj_type = volatile const void *;
    using mutable_obj_type = volatile void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    template<typename F>
    static constexpr bool matches =
      std::is_invocable_r_v<R, volatile const F, A...>;
  };
  template<typename R, class... A>
  struct signature_traits<R(A...) volatile const noexcept> {
    using signature_type = R(A...) const noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (*)(A...) noexcept;
    using invoke_ptr_type = R (*)(volatile const void *, A...) noexcept;
    using invoke_obj_type = volatile const void *;
    using mutable_obj_type = volatile void *;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    template<typename F>
    static constexpr bool matches =
      std::is_nothrow_invocable_r_v<R, volatile const F, A...>;
  };

  template<typename F>
  struct function_traits : signature_traits<extract_signature_t<F>> {};

  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...)> {
    using object_type = T;
    using signature_type = R(A...);
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...);
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
  };
  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...) noexcept> {
    using object_type = T;
    using signature_type = R(A...) noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...) noexcept;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
  };
  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...) const> {
    using object_type = T;
    using signature_type = R(A...) const;
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...) const;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
  };
  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...) const noexcept> {
    using object_type = T;
    using signature_type = R(A...) const noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...) const noexcept;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
  };

  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...) volatile> {
    using object_type = T;
    using signature_type = R(A...) volatile;
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...) volatile;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = true;
  };
  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...) volatile noexcept> {
    using object_type = T;
    using signature_type = R(A...) volatile noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...) volatile noexcept;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = true;
  };
  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...) volatile const> {
    using object_type = T;
    using signature_type = R(A...) volatile const;
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...) volatile const;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
  };
  template<typename R, class T, class... A>
  struct function_traits<R (T::*)(A...) volatile const noexcept> {
    using object_type = T;
    using signature_type = R(A...) volatile const noexcept;
    using normalized_signature_type = R(A...);
    using ptr_type = R (T::*)(A...) volatile const noexcept;
    using return_type = R;
    using arguments = TypeList<A...>;
    static constexpr bool is_noexcept = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
  };

  template<class MemberPtr>
  struct member_traits;

  template<class Type, class Obj>
  struct member_traits<Type Obj::*> {
    using type = Type;
    using object_type = Obj;
  };
  template<class Type, class Obj>
  struct member_traits<const Type Obj::*> {
    using type = const Type;
    using object_type = Obj;
  };
  template<class Type, class Obj>
  struct member_traits<volatile Type Obj::*> {
    using type = volatile Type;
    using object_type = Obj;
  };
  template<class Type, class Obj>
  struct member_traits<const volatile Type Obj::*> {
    using type = const volatile Type;
    using object_type = Obj;
  };

  template<class MemberPtr>
  using mem_data_type = typename member_traits<MemberPtr>::type;

  template<class F, typename = void>
  struct is_callable : std::false_type {};

  template<class F>
  struct is_callable<F, std::void_t<function_traits<F>>> : std::true_type {};

  template<class F>
  inline constexpr bool is_callable_v = is_callable<F>::value;

  template<class F>
  concept Callable = is_callable_v<F>;

  template<class T, class MemFunPtr>
  struct MemFunBinder;

  template<class T, typename Ret, typename... MemFunArgs>
  struct MemFunBinder<T, Ret (T::*)(MemFunArgs...)> {
    T *t;
    Ret (T::*mem_fun)(MemFunArgs...);
    constexpr MemFunBinder(T &t, Ret (T::*mem_fun)(MemFunArgs...))
      : t(&t), mem_fun(mem_fun) {}
    constexpr MemFunBinder(const MemFunBinder &) = default;
    constexpr MemFunBinder(MemFunBinder &&) = default;
    constexpr MemFunBinder &operator=(const MemFunBinder &) = default;
    constexpr MemFunBinder &operator=(MemFunBinder &&) = default;

    Ret operator()(MemFunArgs... args) { return (t->*(mem_fun))(args...); }
  };

  template<class T, typename Ret, class... MemFunArgs>
  struct MemFunBinder<T, Ret (T::*)(MemFunArgs...) noexcept> {
    T *t;
    Ret (T::*mem_fun)(MemFunArgs...) noexcept;

    constexpr MemFunBinder(T &t, Ret (T::*mem_fun)(MemFunArgs...) noexcept)
      : t(&t), mem_fun(mem_fun) {}
    constexpr MemFunBinder(const MemFunBinder &) = default;
    constexpr MemFunBinder(MemFunBinder &&) = default;
    constexpr MemFunBinder &operator=(const MemFunBinder &) = default;
    constexpr MemFunBinder &operator=(MemFunBinder &&) = default;

    Ret operator()(MemFunArgs... args) noexcept {
      return (t->*(mem_fun))(args...);
    }
  };

  template<class T, typename Ret, class... MemFunArgs>
  struct MemFunBinder<T, Ret (T::*)(MemFunArgs...) const> {
    const T &t;
    Ret (T::*mem_fun)(MemFunArgs...) const;

    constexpr MemFunBinder(const T &t, Ret (T::*mem_fun)(MemFunArgs...) const)
      : t(t), mem_fun(mem_fun) {}
    constexpr MemFunBinder(const MemFunBinder &) = default;
    constexpr MemFunBinder(MemFunBinder &&) = default;
    constexpr MemFunBinder &operator=(const MemFunBinder &) = default;
    constexpr MemFunBinder &operator=(MemFunBinder &&) = default;
    Ret operator()(MemFunArgs... args) const { return (t.*(mem_fun))(args...); }
  };

  template<class T, typename Ret, class... MemFunArgs>
  struct MemFunBinder<T, Ret (T::*)(MemFunArgs...) const noexcept> {
    const T &t;
    Ret (T::*mem_fun)(MemFunArgs...) const noexcept;

    constexpr MemFunBinder(const T &t,
                           Ret (T::*mem_fun)(MemFunArgs...) const noexcept)
      : t(&t), mem_fun(mem_fun) {}
    constexpr MemFunBinder(const MemFunBinder &) = default;
    constexpr MemFunBinder(MemFunBinder &&) = default;
    constexpr MemFunBinder &operator=(const MemFunBinder &) = default;
    constexpr MemFunBinder &operator=(MemFunBinder &&) = default;
    Ret operator()(MemFunArgs... args) const noexcept {
      return (t->*(mem_fun))(args...);
    }
  };

  template<class T, class MemFunPtr>
  MemFunBinder(T &&, MemFunPtr)
    -> MemFunBinder<std::remove_cvref_t<T>, MemFunPtr>;

  struct dummy {};
} // namespace cli
#endif
