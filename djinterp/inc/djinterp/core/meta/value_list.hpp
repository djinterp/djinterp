/*******************************************************************************
* djinterp [core]                                                 value_list.hpp
*
* value_list - the NTTP sequence (value-domain counterpart of the type list):
*   dtuple.hpp treats std::tuple<typename...> as the framework's compile-time
* TYPE sequence and gives it type-level ops (tuple_join, tuple_type_at,
* tuple_apply_all, tuple_split, ...).  value_list is the missing parallel for
* the VALUE domain: a heterogeneous pack of non-type template parameters
* (`value_list<auto...>`) with the same ops expressed once over values.  It is
* the second half of the dual-domain substrate begun in carrier.hpp - where a
* carrier lifts a single type/value into the object domain, value_list is the
* object-domain *sequence* a compile-time driver materializes into.
*
*   Design (mirrors §10.3 of the dual-domain roadmap):
*   - The list itself is an empty struct - the values live in the type, so a
*     value_list is passed by value and ops are constexpr FREE FUNCTIONS that
*     deduce the pack from the argument (no class-type NTTP needed; C++17-clean).
*   - Growth/transform/fold take value carriers (val_t) and carrier-callable
*     leaves, exactly the leaf shape compose and the transducer spine use, so a
*     leaf written once works here and in the runtime/type domains.
*   - Element access and size additionally expose trait faces (value_list_at,
*     value_list_size) mirroring tuple_type_at / std::tuple_size.
*
* TIER
*   value_list is inherently an auto-NTTP facility, so the whole module is gated
* to the C++17 lifted-functional floor; at the C++11 detection floor this header
* is empty (it introduces no new trait-floor surface).  The concept face is
* gated further to C++20.
*
* OPS PROVIDED  (all constexpr)
*   value_list<auto...>          the sequence; `.size()` member.
*   is_value_list<T> (+_v,+ValueList)   structural detector (+ concept face).
*   value_list_size<L> (+_v)     element count as integral_constant.
*   value_list_at<I,L> (+_v)     the I-th value (trait face).
*   at<I>(list)                  the I-th value as a val_t carrier.
*   append(list, val_t<W>)       list with W appended.
*   prepend(val_t<W>, list)      list with W prepended.
*   concat(a, b, ...)            concatenation of any number of lists.
*   transform(list, op)          map a unary carrier-callable over the list.
*   fold(list, seed, op)         left fold with a binary carrier-callable.
*
* RELATION TO reduce.hpp
*   `fold` is the value-domain driver that the unified reduce_ct
* (core/functional/reduce.hpp, a later task) builds on for its value_list
* overload; it lives here so value_list is self-contained.
*
*
* path:      /inc/djinterp/core/meta/value_list.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_VALUE_LIST_HPP
#define DJINTERP_META_VALUE_LIST_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./type_utility.hpp"  // clean_t
#include "./trait_detect.hpp"   // D_TYPE_TRAIT_VALUE_BOOL
#include "./carrier.hpp"        // val_t / val (value carriers)


NS_DJINTERP


//   value_list is an auto-NTTP facility (C++17); the whole module is gated to
// the lifted-functional floor.  Under C++11/C++14 this header contributes no
// declarations (it adds nothing to the C++11 detection/trait floor).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// ===========================================================================
// I.   value_list
// ===========================================================================

// value_list
//   struct: a heterogeneous compile-time sequence of NTTP values - the
// value-domain counterpart to the std::tuple type sequence used by dtuple.hpp.
// Empty (no storage): the values live in the type, so an instance is an empty
// object passed by value into the constexpr free-function ops below.
template<auto... Values>
struct value_list
{
    // size
    //   function: the number of elements in the list.
    static constexpr std::size_t
    size() noexcept
    {
        return sizeof...(Values);
    }
};


// ===========================================================================
// II.  detection  (is_value_list)
// ===========================================================================
//   Hand-rolled rather than via D_TYPE_TRAIT_IS_SPECIALIZATION_OF because that
// macro matches only typename-kind template parameters; value_list's pack is
// non-type (auto...), so it needs a dedicated specialization.

NS_INTERNAL

    // is_value_list_raw
    //   trait: primary template (failure case); cv-ref is already stripped by
    // the public face below.
    template<typename Type>
    struct is_value_list_raw : std::false_type
    {};

    // is_value_list_raw (success case)
    //   trait: succeeds for a value_list<...> specialization.
    template<auto... Values>
    struct is_value_list_raw<value_list<Values...>> : std::true_type
    {};

NS_END  // internal

// is_value_list
//   trait: detects whether Type is a value_list (after cv-ref stripping).
// Specialization-based, so detection is exact.
template<typename Type>
struct is_value_list
    : internal::is_value_list_raw<clean_t<Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_value_list)


// ===========================================================================
// III. size  (value_list_size)
// ===========================================================================

// value_list_size
//   trait: the element count of a value_list as an integral_constant - the
// value-domain analog of std::tuple_size.
template<typename List>
struct value_list_size;

// value_list_size<value_list<...>>
//   trait: the populated specialization yielding sizeof...(Values).
template<auto... Values>
struct value_list_size<value_list<Values...>>
    : std::integral_constant<std::size_t, sizeof...(Values)>
{};

// value_list_size_v
//   value: convenience alias for value_list_size<List>::value.
template<typename List>
inline constexpr std::size_t value_list_size_v =
    value_list_size<List>::value;


// ===========================================================================
// IV.  element access  (value_list_at / at)
// ===========================================================================

NS_INTERNAL

    // value_list_at_helper
    //   trait: primary template; peels one head per index step (mirrors
    // pack_element over an NTTP pack).
    template<std::size_t Index,
             auto         Head,
             auto...      Tail>
    struct value_list_at_helper
    {
        static constexpr auto value =
            value_list_at_helper<Index - 1, Tail...>::value;
    };

    // value_list_at_helper<0, ...>
    //   trait: base case - index 0 yields the current head.
    template<auto    Head,
             auto... Tail>
    struct value_list_at_helper<0, Head, Tail...>
    {
        static constexpr auto value = Head;
    };

NS_END  // internal

// value_list_at
//   trait: the Index-th value of a value_list (0-based), exposed as `::value`.
// Out-of-range Index is a hard error guarded by a static_assert, mirroring
// tuple_type_at.
template<std::size_t Index,
         typename     List>
struct value_list_at;

// value_list_at<Index, value_list<...>>
//   trait: the populated specialization with bounds checking.
template<std::size_t Index,
         auto...      Values>
struct value_list_at<Index, value_list<Values...>>
{
private:
    static_assert((Index < sizeof...(Values)),
                  "Non-type parameter `Index` is out of range for the "
                  "value_list.");

public:
    static constexpr auto value =
        internal::value_list_at_helper<Index, Values...>::value;
};

// value_list_at_v
//   value: convenience alias for value_list_at<Index, List>::value.
template<std::size_t Index,
         typename     List>
inline constexpr auto value_list_at_v =
    value_list_at<Index, List>::value;

// at
//   function: the Index-th element of a value_list instance, returned as a
// value carrier (val_t) so it slots straight back into the carrier pipeline.
template<std::size_t Index,
         auto...      Values>
constexpr auto
at(value_list<Values...>)
{
    return val<value_list_at_v<Index, value_list<Values...>>>;
}


// ===========================================================================
// V.   growth  (append / prepend / concat)
// ===========================================================================

// append
//   function: the list with Value appended at the end (the §10.3 collect
// step).  The new element is supplied as a value carrier.
template<auto... Values,
         auto    Value>
constexpr value_list<Values..., Value>
append(value_list<Values...>, val_t<Value>)
{
    return {};
}

// prepend
//   function: the list with Value inserted at the front.
template<auto    Value,
         auto... Values>
constexpr value_list<Value, Values...>
prepend(val_t<Value>, value_list<Values...>)
{
    return {};
}

// concat
//   function: concatenation of nothing - the empty list.
constexpr value_list<>
concat()
{
    return {};
}

// concat
//   function: a single list concatenates to itself (identity).
template<auto... Values>
constexpr value_list<Values...>
concat(value_list<Values...> _list)
{
    return _list;
}

// concat
//   function: concatenation of exactly two lists (the base case).
template<auto... As,
         auto... Bs>
constexpr value_list<As..., Bs...>
concat(value_list<As...>, value_list<Bs...>)
{
    return {};
}

// concat
//   function: concatenation of three or more lists; folds pairwise from the
// left.  The >=3 arity keeps it disjoint from the two-list base case above.
template<typename    First,
         typename    Second,
         typename    Third,
         typename... Rest>
constexpr auto
concat(First _first, Second _second, Third _third, Rest... _rest)
{
    return concat(concat(_first, _second), _third, _rest...);
}


// ===========================================================================
// VI.  transformation  (transform)
// ===========================================================================

// transform
//   function: applies a unary value-domain operation to every element,
// returning a new value_list.  `Op` is a carrier-callable leaf - invoking it
// on a value carrier yields a value carrier (val_t<V> -> val_t<f(V)>), the same
// leaf shape compose and the transducer spine use.  Only Op's result type is
// needed, so the op is taken unnamed and probed in an unevaluated context; the
// value-domain analog of dtuple's tuple_apply_all.
template<auto... Values,
         typename Op>
constexpr auto
transform(value_list<Values...>, Op)
{
    return value_list<
        decltype(std::declval<Op&>()(val<Values>))::value...
    >{};
}


// ===========================================================================
// VII. reduction  (fold)
// ===========================================================================

NS_INTERNAL

    // value_list_fold_impl
    //   function: base case - an empty list folds to the accumulator.
    template<typename Acc,
             typename Op>
    constexpr Acc
    value_list_fold_impl(Acc _acc, Op, value_list<>)
    {
        return _acc;
    }

    // value_list_fold_impl (recursive)
    //   function: fold the head into the accumulator, recurse on the tail.
    template<typename Acc,
             typename Op,
             auto     Head,
             auto...  Tail>
    constexpr auto
    value_list_fold_impl(Acc _acc, Op _op, value_list<Head, Tail...>)
    {
        return value_list_fold_impl(_op(_acc, val<Head>),
                                    _op,
                                    value_list<Tail...>{});
    }

NS_END  // internal

// fold
//   function: left fold over a value_list.  `_op` is a binary carrier-callable
// reducer (acc, val_t<V>) -> acc and `_seed` is the initial accumulator (any
// object, typically a carrier or another value_list).  This is the value-domain
// driver reduce_ct (core/functional/reduce.hpp) builds on.
template<auto... Values,
         typename Acc,
         typename Op>
constexpr auto
fold(value_list<Values...> _list, Acc _seed, Op _op)
{
    return internal::value_list_fold_impl(_seed, _op, _list);
}


// ===========================================================================
// VIII. concept face
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // ValueList
    //   concept: satisfied by any value_list specialization.  PascalCase
    // per the project's concept naming convention, paralleling
    // is_value_list_v.
    template<typename Type>
    concept ValueList = is_value_list<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_META_VALUE_LIST_HPP
