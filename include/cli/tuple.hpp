#ifndef CLI_TUPLE_HPP
#define CLI_TUPLE_HPP

#include "cli/compiler.hpp"
#include "cli/type_list.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

#if defined(CLI_GCC) or defined(CLI_ARM_GCC)
#define CLI_DISABLE_MULTIPLE_INHERITANCE_WANRING_START
#define CLI_DISABLE_MULTIPLE_INHERITANCE_WANRING_END
#else
#define CLI_DISABLE_MULTIPLE_INHERITANCE_WANRING_START
#define CLI_DISABLE_MULTIPLE_INHERITANCE_WANRING_END
#endif

namespace cli {
  template<typename... Ts>
  class Tuple;

  template<std::size_t, typename... Ts>
  constexpr auto &get(Tuple<Ts...> &tuple);

  template<std::size_t, typename... Ts>
  constexpr const auto &get(const Tuple<Ts...> &tuple);

  namespace dtl {
    template<typename Indices, typename... Ts>
    class TupleImpl;

    template<std::size_t I, typename T>
    struct TupleElem {
      T value_;

      constexpr TupleElem()
        requires std::is_constructible_v<T>
        : value_{} {}

      constexpr TupleElem(const T &t)
        requires std::is_copy_constructible_v<T>
        : value_(t) {}

      constexpr TupleElem(T &&t)
        requires std::is_move_constructible_v<T>
        : value_(std::move(t)) {}

      template<typename T_>
      constexpr TupleElem(T_ &&t)
        requires std::is_constructible_v<T, T_ &&> and
                 (not std::same_as<T, std::remove_cvref_t<T_>>)
        : value_{t} {}

      constexpr TupleElem(const TupleElem &o)
        requires std::is_copy_constructible_v<T>
        : value_(o.value_) {}

      constexpr TupleElem(TupleElem &&o)
        requires std::is_move_constructible_v<T>
        : value_(std::move(o.value_)) {}

      constexpr TupleElem &operator=(const TupleElem &o)
        requires std::is_copy_assignable_v<T>
      {
        value_ = o.value_;
        return *this;
      }

      constexpr TupleElem &operator=(TupleElem &&o)
        requires std::is_move_assignable_v<T>
      {
        value_ = std::move(o.value_);
        return *this;
      }
    };

#if defined(CLI_GCC) or defined(CLI_ARM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultiple-inheritance"
#else
#endif
    template<std::size_t... Is,
             template<typename I, I...> typename List,
             typename... Ts>
    class TupleImpl<List<std::size_t, Is...>, Ts...>
      : public TupleElem<Is, Ts>... {
    public:
      constexpr TupleImpl()
        requires(std::is_constructible_v<Ts> and ...)
        : TupleElem<Is, Ts>{}... {}

      template<typename... T>
      constexpr TupleImpl(T &&...ts)
        requires(std::is_constructible_v<Ts, T &&> && ...) and
                (not std::same_as<T, Ts> and ...)
        : TupleElem<Is, Ts>(std::forward<T>(ts))... {}

      constexpr TupleImpl(const Ts &...ts)
        requires(std::is_copy_constructible_v<Ts> && ...)
        : TupleElem<Is, Ts>{ts}... {}

      constexpr TupleImpl(Ts &&...ts)
        requires(std::is_move_constructible_v<Ts> && ...)
        : TupleElem<Is, Ts>{std::move(ts)}... {}

      constexpr TupleImpl(const TupleImpl &o)
        requires(std::is_copy_constructible_v<Ts> && ...)
        : TupleElem<Is, Ts>{static_cast<const TupleElem<Is, Ts> &>(o)}... {}

      constexpr TupleImpl(TupleImpl &&o)
        requires(std::is_move_constructible_v<Ts> && ...)
        : TupleElem<Is, Ts>{static_cast<TupleElem<Is, Ts> &&>(o)}... {}

      constexpr TupleImpl &operator=(const TupleImpl &o)
        requires(std::is_copy_assignable_v<Ts> && ...)
      {
        ((static_cast<TupleElem<Is, Ts> &>(*this) =
            static_cast<const TupleElem<Is, Ts> &>(o)),
         ...);
        return *this;
      }

      constexpr TupleImpl &operator=(TupleImpl &&o)
        requires(std::is_move_assignable_v<Ts> && ...)
      {
        ((static_cast<TupleElem<Is, Ts> &>(*this) =
            static_cast<TupleElem<Is, Ts> &&>(o)),
         ...);
        return *this;
      }
    };

#if defined(CLI_GCC) or defined(CLI_ARM_GCC)
#pragma GCC diagnostic pop
#else
#endif
  } // namespace dtl

  /**
   * A simple std::tuple implementation. Used because arm-none-eabi-gcc seems to
   * have trouble with constinit initialization.
   *
   * @tparam Ts the tuple member types.
   */
  template<typename... Ts>
  class Tuple
    : private dtl::TupleImpl<std::make_index_sequence<sizeof...(Ts)>, Ts...> {
    using Impl = dtl::TupleImpl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  public:
    /// default constructs each member of the tuple.
    constexpr Tuple()
      requires(std::is_constructible_v<Ts> and ...)
      : Impl{} {}

    /// constructs tuple membrs from values
    /// @param ts the initialization values
    template<typename... T>
      requires(sizeof...(T) == sizeof...(Ts)) and
              (std::is_constructible_v<Ts, T> and ...)
    constexpr Tuple(T &&...ts)
      : Impl(std::forward<T>(ts)...) {}

    /// copy ctor
    constexpr Tuple(const Tuple &o)
      requires(std::is_copy_constructible_v<Ts> && ...)
      : Impl(static_cast<const Impl &>(o)) {}

    /// move ctor
    constexpr Tuple(Tuple &&o)
      requires(std::is_move_constructible_v<Ts> && ...)
      : Impl(static_cast<Impl &&>(o)) {}

    /// copy assignment
    constexpr Tuple &operator=(const Tuple &o)
      requires(std::is_copy_assignable_v<Ts> && ...)
    {
      static_cast<Impl &>(*this) = static_cast<const Impl &>(o);
      return *this;
    }

    /// move assignment
    constexpr Tuple &operator=(Tuple &&o)
      requires(std::is_move_assignable_v<Ts> && ...)
    {
      static_cast<Impl &>(*this) = static_cast<Impl &&>(o);
      return *this;
    }

    template<std::size_t I, typename... T>
    friend constexpr auto &get(Tuple<T...> &tuple);

    template<std::size_t I, typename... T>
    friend constexpr const auto &get(const Tuple<T...> &tuple);
  };

  template<typename... T>
  Tuple(T &&...) -> Tuple<std::decay_t<T>...>;

  /// empty tuple specialization
  template<>
  class Tuple<> {
  public:
    /// constructor
    constexpr Tuple() = default;
    /// copy ctor
    constexpr Tuple(const Tuple &) = default;
    /// move ctor
    constexpr Tuple(Tuple &&) = default;
    /// copy assignment
    constexpr Tuple &operator=(const Tuple &) = default;
    /// move assignment
    constexpr Tuple &operator=(Tuple &&) = default;
  };

  template<std::size_t I, typename... Ts>
  constexpr auto &get(Tuple<Ts...> &tuple) {
    return static_cast<
             dtl::TupleElem<I, type_list::type_at_t<I, Tuple<Ts...>>> &>(tuple)
      .value_;
  }

  template<std::size_t I, typename... Ts>
  constexpr const auto &get(const Tuple<Ts...> &tuple) {
    return static_cast<
             const dtl::TupleElem<I, type_list::type_at_t<I, Tuple<Ts...>>> &>(
             tuple)
      .value_;
  }
} // namespace cli

namespace std {
  template<class... Ts>
  struct tuple_size<cli::Tuple<Ts...>> {
    static constexpr std::size_t value = sizeof...(Ts);
  };
} // namespace std

#endif
