// clang-format off
/**
 * @file type_list/type_list.hpp
 * @version 1.0
 * @copyright Boost Software License - Version 1.0
 * A single header C++ library for type lists.
 */

#ifndef TYPE_LIST_HPP
#define TYPE_LIST_HPP

#include <cstddef>
#include <limits>
#include <type_traits>

#if __cplusplus >= 201703L
#define TYPELIST_INLINE inline
#else
#define TYPELIST_INLINE static
#endif

namespace cli::type_list {

/// constant for indicating an invalid index.
TYPELIST_INLINE constexpr size_t npos = std::numeric_limits<size_t>::max();

/**
 * a concrete type list type.
 * @tparam Ts the elements of the type list
 */
template <class... Ts> struct TypeList {};

template <class... Ts>
constexpr bool operator==(TypeList<Ts...>, TypeList<Ts...>) {
  return true;
}

template <class... Ts, class... Us>
constexpr bool operator==(TypeList<Ts...>, TypeList<Us...>) {
  return false;
}

template <class... Ts>
constexpr bool operator!=(TypeList<Ts...>, TypeList<Ts...>) {
  return false;
}

template <class... Ts, class... Us>
constexpr bool operator!=(TypeList<Ts...>, TypeList<Us...>) {
  return true;
}

namespace {

template <size_t N, class T, class... Ts> struct index_of_impl;

template <size_t N, class T, class T1, class... Ts>
struct index_of_impl<N, T, T1, Ts...> : index_of_impl<N + 1, T, Ts...> {};

template <size_t N, class T, class... Ts> struct index_of_impl<N, T, T, Ts...> {
  static constexpr size_t value = N;
};

template <size_t N, class T> struct index_of_impl<N, T> {
  static constexpr size_t value = npos;
};

template <size_t N, class T, class... Ts>
struct type_at_impl : type_at_impl<N - 1, Ts...> {};

template <class T, class... Ts> struct type_at_impl<0, T, Ts...> {
  using type = T;
};

template<bool B,class...Ts>
struct disjunction_helper;

template<>
struct disjunction_helper<false> : std::false_type{};

template<typename...Ts>
struct disjunction_helper<true,Ts...> : std::true_type{};

template<class T,class...Ts>
struct disjunction_helper<false, T, Ts...> : disjunction_helper<T::value, Ts...>{};


template<class...Ts> struct disjunction : disjunction_helper<false, Ts...>{};

template <class FilteredList, class ToFilterList,
          template <class> class Predicate>
struct filter_impl;

template <template <class...> class List1, template <class...> class List2,
          class... Filtered, class ToFilter, class... ToBeFiltered,
          template <class> class Predicate>
struct filter_impl<List1<Filtered...>, List2<ToFilter, ToBeFiltered...>,
                   Predicate> {
  using type = typename std::conditional<
      Predicate<ToFilter>::value,
      typename filter_impl<List2<Filtered..., ToFilter>, List2<ToBeFiltered...>,
                           Predicate>::type,
      typename filter_impl<List2<Filtered...>, List2<ToBeFiltered...>,
                           Predicate>::type>::type;
};

template <template <class...> class List1, template <class...> class List2,
          class... Filtered, template <class> class Predicate>
struct filter_impl<List1<Filtered...>, List2<>, Predicate> {
  using type = List2<Filtered...>;
};
} // namespace

/**
 * get the first element of List.
 * @tparam List a type list
 */
template <class List> struct head;

template <class T, template <class...> class List, class... Ts>
struct head<List<T, Ts...> > {
  using type = T;
};

/**
 * get the first element of List.
 * @tparam List a type list
 */
template <class List> using head_t = typename head<List>::type;

/**
 * returns List with the first element removed.
 * @tparam List a type list
 */
template <class List> struct tail;

template <template <class...> class List> struct tail<List<> > {
  using type = List<>;
};

template <class T, template <class...> class List, class... Ts>
struct tail<List<T, Ts...> > {
  using type = List<Ts...>;
};

/**
 * returns List with the first element removed.
 * @tparam List a type list
 */
template <class List> using tail_t = typename tail<List>::type;

/**
 * get the number of elements in a type list.
 * @tparam List a type list
 */
template <class List> struct list_size;

template <template <class...> class List, class... Ts>
struct list_size<List<Ts...> > {
  static constexpr size_t value = sizeof...(Ts);
};

/**
 * get the index of the first occurrence T in List, or
 * npos if T is not in the List.
 *
 * @tparam T the type to search for
 * @tparam List a type list
 */
template <class T, class List> struct index_of;

template <class T, template <class...> class List, class... Ts>
struct index_of<T, List<Ts...> > : index_of_impl<0, T, Ts...> {};

/**
 * return List with T added to the front.
 * @tparam T the type to add
 * @tparam List a type list
 */
template <class T, class List> struct push_front;

template <class T, template <class...> class List, class... Ts>
struct push_front<T, List<Ts...> > {
  using type = TypeList<T, Ts...>;
};

/**
 * return List with T added to the front.
 * @tparam T the type to add
 * @tparam List a type list
 */
template <class T, class List>
using push_front_t = typename push_front<T, List>::type;

/**
 * returns List with the first element removed. If List is empty, 
 * an empty List is returned.
 * @tparam List a type list
 */
template <class List> struct pop_front;

template <class T, template <class...> class List, class... Ts>
struct pop_front<List<T, Ts...> > {
  using type = List<Ts...>;
};

template <template <class...> class List> struct pop_front<List<> > {
  using type = List<>;
};

/**
 * returns List with the first element removed. If List is empty, 
 * an empty List is returned.
 * @tparam List a type list
 */
template <class List> using pop_front_t = typename pop_front<List>::type;

/**
 * returns N-th type in type_list List.
 * @tparam N integer index
 * @tparam List a type list
 */
template <size_t N, class List> struct type_at;

template <size_t N, template <class...> class List, class... Ts>
struct type_at<N, List<Ts...> > : type_at_impl<N, Ts...> {};

/**
 * returns N-th type in type_list List.
 * @tparam N integer index
 * @tparam List a type list
 */
template <size_t N, class List>
using type_at_t = typename type_at<N, List>::type;

/**
 * returns the first element of List
 * @tparam List a type list
 */
template <class List> using first = head<List>;

/**
 * returns the first element of List
 * @tparam List a type list
 */
template <class List> using first_t = head_t<List>;

/**
 * returns the last element of List
 * @tparam List a type list
 */
template <class List>
using last = type_at<list_size<List>::value - 1, List>;

/**
 * returns the last element of List
 * @tparam List a type list
 */
template <class List>
using last_t = typename last<List>::type;

/**
 * return List with T added at the end.
 * @tparam T the element
 * @tparam List a type list
 */
template <class T, class List> struct push_back;

template <class T, template <class...> class List, class... Ts>
struct push_back<T, List<Ts...> > {
  using type = List<Ts..., T>;
};

/**
 * return List with T added at the end.
 * @tparam T the element
 * @tparam List a type list
 */
template <class T, class List>
using push_back_t = typename push_back<T, List>::type;

namespace {
template <class List, size_t I> struct pop_back_impl;

template <class T, template <class...> class List, class... Ts, size_t I>
struct pop_back_impl<List<T, Ts...>, I> {
  using type =
      push_front_t<T, typename pop_back_impl<List<Ts...>, I - 1>::type>;
};

template <template <class...> class List, class T>
struct pop_back_impl<List<T>, 1> {
  using type = List<>;
};

template <template <class...> class List> struct pop_back_impl<List<>, 0> {
  using type = List<>;
};
} // namespace

/**
 * returns List with the last element removed.
 * @tparam List a type list
 */

template <class List>
struct pop_back : pop_back_impl<List, list_size<List>::value> {};

/**
 * returns List with the last element removed.
 * @tparam List a type list
 */
template <class List> using pop_back_t = typename pop_back<List>::type;

/**
 * applies List to F, i.e. returns F<Ts...> where Ts are the elements
 * of List.
 * @tparam F the type to apply to
 * @tparam List a type list
 */
template <template <class...> class F, class List> struct apply;

template <template <class...> class F, template <class...> class List,
          class... Ts>
struct apply<F, List<Ts...> > {
  using type = F<Ts...>;
};

/**
 * applies List to F, i.e. returns F<Ts...> where Ts are the elements
 * of List.
 * @tparam F the type to apply to
 * @tparam List a type list
 */
template <template <class...> class F, class List>
using apply_t = typename apply<F, List>::type;

/**
 * returns the template parameters of Class as a TypeList. If Class does
 * not have template parameters, an empty list is returned.
 * @note This does not work with non type template parameters.
 */
template <class Class> struct extract;

template <template <class...> class Class, class... Ts>
struct extract<Class<Ts...> > {
  using type = TypeList<Ts...>;
};

/**
 * returns the template parameters of Class as a TypeList. If Class does
 * not have template parameters, an empty list is returned.
 * @note This does not work with non type template parameters.
 * @tparam Class any type
 */
template <class Class> using extract_t = typename extract<Class>::type;

/**
 * replaces T with F<T> for each T in List.
 * @tparam F a meta function returning true or false
 * @tparam List a type list
 */
template <template <class> class F, class List> struct for_each;

template <template <class> class F, template <class...> class List, class... Ts>
struct for_each<F, List<Ts...> > {
  using type = List<F<Ts>...>;
};

/**
 * replaces T with F<T> for each T in List.
 * @tparam F a meta function returning true or false
 * @tparam List a type list
 */
template <template <class> class F, class List>
using for_each_t = typename for_each<F, List>::type;

/**
 * replaces T with F<T>::type for each T in List.
 * @tparam F a meta function returning true or false
 * @tparam List a type list
 */
template <template <class> class F, class List> struct transform;

template <template <class> class F, template <class...> class List, class... Ts>
struct transform<F, List<Ts...> > {
  using type = List<typename F<Ts>::type...>;
};

template <template <class> class F, class List>
using transform_t = typename transform<F, List>::type;

/**
 * merges Lists into a single type list.
 *
 * If sizeof...(Lists) is 0 then an TypeList<> is returned.
 * If a mixture of type list templates is used, e.g. mixing TypeList with
 * std::variant and std::tuple, then the result will use the underlying template of
 * the first element in Lists. 
 *
 * @tparam Lists a pack of type lists
 */
template <class... Lists> struct merge;

template <template <class...> class List1, template <class...> class List2,
          class... Ts, class... Us, class... Lists>
struct merge<List1<Ts...>, List2<Us...>, Lists...> {
  using type = typename merge<List1<Ts..., Us...>, Lists...>::type;
};

template <template <class...> class List1, template <class...> class List2,
          class... Ts, class... Us>
struct merge<List1<Ts...>, List2<Us...> > {
  using type = List1<Ts..., Us...>;
};

template <template <class...> class List, class... Ts>
struct merge<List<Ts...> > {
  using type = List<Ts...>;
};

template <> struct merge<> {
  using type = TypeList<>;
};

/**
 * merges Lists into a single type list.
 *
 * If sizeof...(Lists) is 0 then an TypeList<> is returned.
 * If a mixture of type list templates is used, e.g. mixing TypeList with
 * std::variant and std::tuple, then the result will use the template of
 * the first element in Lists. 
 *
 * @tparam Lists a pack of type lists
 */
template <class... Lists> using merge_t = typename merge<Lists...>::type;


/**
 * returns List filtered with Predicate.
 *
 * @tparam Predicate a meta function returning true or false
 * @tparam List a type list
 */
template <template <class> class Predicate, class List>
struct filter : filter_impl<TypeList<>, List, Predicate> {};

/**
 * returns List filtered with Predicate.
 *
 * @tparam Predicate a meta function returning true or false
 * @tparam List a type list
 */
template <template <class> class Predicate, class List>
using filter_t = typename filter<Predicate, List>::type;

/**
 * returns true if T is an element of List, else false.
 *
 * @tparam T the type to search for
 * @tparam List a type list
 */
template <class T, class List> struct contains;

template <class T, template <class...> class List, class... Ts>
struct contains<T, List<Ts...> > : disjunction<std::is_same<T, Ts>...> {};

namespace {
template <class FilteredList, class ToFilterList> struct remove_duplicates_impl;

template <template <class...> class List1, template <class...> class List2,
          class... Filtered, class ToFilter, class... ToBeFiltered>
struct remove_duplicates_impl<List1<Filtered...>,
                              List2<ToFilter, ToBeFiltered...> > {
  using type = typename std::conditional<
      contains<ToFilter, List1<Filtered...> >::value,
      typename remove_duplicates_impl<List2<Filtered...>,
                                      List2<ToBeFiltered...> >::type,
      typename remove_duplicates_impl<List2<Filtered..., ToFilter>,
                                      List2<ToBeFiltered...> >::type>::type;
};

template <template <class...> class List1, template <class...> class List2,
          class... Filtered>
struct remove_duplicates_impl<List1<Filtered...>, List2<> > {
  using type = List2<Filtered...>;
};
} // namespace

/**
 * returns List with duplicate elements removed.
 * @tparam List a type list
 */
template <class List>
struct remove_duplicates : remove_duplicates_impl<TypeList<>, List> {};

/**
 * returns List with duplicate elements removed.
 * @tparam List a type list
 */
template <class List>
using remove_duplicates_t = typename remove_duplicates<List>::type;

#if __cplusplus >= 201402L
template <class List>
TYPELIST_INLINE constexpr size_t list_size_v = list_size<List>::value;

template <class T, class List>
TYPELIST_INLINE constexpr size_t index_of_v = index_of<T, List>::value;

template <class T, class List>
TYPELIST_INLINE constexpr bool contains_v = contains<T, List>::value;
#endif
} // namespace type_list

namespace cli{

template<class...Ts>
using TypeList = type_list::TypeList<Ts...>;

}
#endif
// clang-format on
