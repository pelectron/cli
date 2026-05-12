#ifndef CLI_TUPLE_HPP
#define CLI_TUPLE_HPP

#include "cli/type_list.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace cli {
  template<typename... Ts>
  class Tuple;

  template<typename Indices, typename... Ts>
  class TupleImpl;

  template<std::size_t I, typename T>
  class TupleElem {
    T value_;

  public:
    constexpr TupleElem()
      requires std::is_constructible_v<T>
      : value_{} {}
    constexpr TupleElem(const T &t)
      requires std::is_copy_constructible_v<T>
      : value_(t) {}

    constexpr TupleElem(T &&t)
      requires std::is_move_constructible_v<T>
      : value_(std::move(t)) {}

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

    template<std::size_t, typename... Ts>
    friend constexpr auto &get(Tuple<Ts...> &tuple);

    template<std::size_t, typename... Ts>
    friend constexpr const auto &get(const Tuple<Ts...> &tuple);
  };

  template<std::size_t... Is,
           template<typename I, I...> typename List,
           typename... Ts>
  class TupleImpl<List<std::size_t, Is...>, Ts...>
    : public TupleElem<Is, Ts>... {
  public:
    constexpr TupleImpl()
      requires(std::is_constructible_v<Ts> and ...)
    {}

    template<typename... T>
    constexpr TupleImpl(T &&...ts)
      : TupleElem<Is, Ts>(std::forward<T>(ts))... {}

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

  template<typename... Ts>
  class Tuple
    : public TupleImpl<std::make_index_sequence<sizeof...(Ts)>, Ts...> {
    using Impl = TupleImpl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

  public:
    constexpr Tuple()
      requires(std::is_constructible_v<Ts> and ...)
    {}

    template<typename... T>
      requires(sizeof...(T) == sizeof...(Ts))
    constexpr Tuple(T &&...ts)
      : Impl(std::forward<T>(ts)...) {}

    constexpr Tuple(const Tuple &o)
      requires(std::is_copy_constructible_v<Ts> && ...)
      : Impl(static_cast<const Impl &>(o)) {}

    constexpr Tuple(Tuple &&o)
      requires(std::is_move_constructible_v<Ts> && ...)
      : Impl(static_cast<Impl &&>(o)) {}

    constexpr Tuple &operator=(const Tuple &o)
      requires(std::is_copy_assignable_v<Ts> && ...)
    {
      static_cast<Impl &>(*this) = static_cast<const Impl &>(o);
      return *this;
    }

    constexpr Tuple &operator=(Tuple &&o)
      requires(std::is_move_assignable_v<Ts> && ...)
    {
      static_cast<Impl &>(*this) = static_cast<Impl &&>(o);
      return *this;
    }
  };

  template<typename... T>
  Tuple(T &&...) -> Tuple<std::decay_t<T>...>;

  template<std::size_t I, typename... Ts>
  constexpr auto &get(Tuple<Ts...> &tuple) {
    return static_cast<TupleElem<I, type_list::type_at_t<I, Tuple<Ts...>>> &>(
             tuple)
      .value_;
  }

  template<std::size_t I, typename... Ts>
  constexpr const auto &get(const Tuple<Ts...> &tuple) {
    return static_cast<
             const TupleElem<I, type_list::type_at_t<I, Tuple<Ts...>>> &>(tuple)
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
