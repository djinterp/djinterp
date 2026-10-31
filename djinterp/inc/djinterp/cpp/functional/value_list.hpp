/*******************************************************************************
* djinterp [cpp]                                                  value_list.hpp
*
* DValueList - the NTTP sequence (value-domain counterpart of the type list):
*   dtuple.hpp treats std::tuple<typename...> as the framework's compile-time
* TYPE sequence and gives it type-level ops (tuple_join, DTupleTypeAt,
* tuple_apply_all, DTupleSplit, ...).  DValueList is the missing parallel for
* the VALUE domain: a heterogeneous pack of non-type template parameters
* (`DValueList<auto...>`) with the same ops expressed once over values.  It is
* the second half of the dual-domain substrate begun in carrier.hpp - where a
* carrier lifts a single type/value into the object domain, DValueList is the
* object-domain *sequence* a compile-time driver materializes into.
*
*   Design (mirrors §10.3 of the dual-domain roadmap):
*   - The list itself is an empty struct - the values live in the type, so a
*     DValueList is passed by value and ops are constexpr FREE FUNCTIONS that
*     deduce the pack from the argument (no class-type NTTP needed; C++17-clean).
*   - Growth/transform/fold take value carriers (val_t) and carrier-callable
*     leaves, exactly the leaf shape compose and the transducer spine use, so a
*     leaf written once works here and in the runtime/type domains.
*   - Element access and size additionally expose trait faces (value_list_at,
*     value_list_size) mirroring DTupleTypeAt / std::tuple_size.
*
* TIER
*   DValueList is inherently an auto-NTTP facility, so the whole module is gated
* to the C++17 lifted-functional floor; at the C++11 detection floor this header
* is empty (it introduces no new trait-floor surface).  The concept face is
* gated further to C++20.
*
* OPS PROVIDED  (all constexpr)
*   DValueList<auto...>          the sequence; `.size()` member.
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
* (core/functional/reduce.hpp, a later task) builds on for its DValueList
* overload; it lives here so DValueList is self-contained.
*
*
* path:      /inc/djinterp/cpp/functional/value_list.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_VALUE_LIST_HPP
#define DJINTERP_FUNCTIONAL_VALUE_LIST_HPP 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/trait_detect.hpp"   // D_TYPE_TRAIT_VALUE_BOOL
#include "../meta/carrier.hpp"        // val_t / val (value carriers)


NS_DJINTERP


//   DValueList is an auto-NTTP facility (C++17); the whole module is gated to
// the lifted-functional floor.  Under C++11/C++14 this header contributes no
// declarations (it adds nothing to the C++11 detection/trait floor).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// ===========================================================================
// I.   DValueList
// ===========================================================================

// DValueList
//   struct: a heterogeneous compile-time sequence of NTTP values - the
// value-domain counterpart to the std::tuple type sequence used by dtuple.hpp.
// Empty (no storage): the values live in the type, so an instance is an empty
// object passed by value into the constexpr free-function ops below.
template<auto... _Values>
struct DValueList
{
    // size
    //   function: the number of elements in the list.
    static constexpr
    std::size_t size() noexcept
    {
        return sizeof...(_Values);
    }
};


// ===========================================================================
// II.  detection  (is_value_list)
// ===========================================================================
//   Hand-rolled rather than via D_TYPE_TRAIT_IS_SPECIALIZATION_OF because that
// macro matches only typename-kind template parameters; DValueList's pack is
// non-type (auto...), so it needs a dedicated specialization.

NS_INTERNAL

    // is_value_list_raw
    //   trait: primary template (failure case); cv-ref is already stripped by
    // the public face below.
    template<typename _Type>
    struct is_value_list_raw : std::false_type
    {};

    // is_value_list_raw (success case)
    //   trait: succeeds for a DValueList<...> specialization.
    template<auto... _Values>
    struct is_value_list_raw<DValueList<_Values...>> : std::true_type
    {};

NS_END  // internal

// is_value_list
//   trait: detects whether _Type is a DValueList (after cv-ref stripping).
// Specialization-based, so detection is exact.
template<typename _Type>
struct is_value_list
    : internal::is_value_list_raw<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_value_list)


// ===========================================================================
// III. size  (value_list_size)
// ===========================================================================

// value_list_size
//   trait: the element count of a DValueList as an integral_constant - the
// value-domain analog of std::tuple_size.
template<typename _List>
struct value_list_size;

// value_list_size<DValueList<...>>
//   trait: the populated specialization yielding sizeof...(_Values).
template<auto... _Values>
struct value_list_size<DValueList<_Values...>>
    : std::integral_constant<std::size_t, sizeof...(_Values)>
{};

// value_list_size_v
//   value: convenience alias for value_list_size<_List>::value.
template<typename _List>
inline constexpr std::size_t value_list_size_v =
    value_list_size<_List>::value;


// ===========================================================================
// IV.  element access  (value_list_at / at)
// ===========================================================================

NS_INTERNAL

    // value_list_at_helper
    //   trait: primary template; peels one head per index step (mirrors
    // pack_element over an NTTP pack).
    template<std::size_t _Index,
             auto         _Head,
             auto...      _Tail>
    struct value_list_at_helper
    {
        static constexpr auto value =
            value_list_at_helper<_Index - 1, _Tail...>::value;
    };

    // value_list_at_helper<0, ...>
    //   trait: base case - index 0 yields the current head.
    template<auto    _Head,
             auto... _Tail>
    struct value_list_at_helper<0, _Head, _Tail...>
    {
        static constexpr auto value = _Head;
    };

NS_END  // internal

// value_list_at
//   trait: the _Index-th value of a DValueList (0-based), exposed as `::value`.
// Out-of-range _Index is a hard error guarded by a static_assert, mirroring
// DTupleTypeAt.
template<std::size_t _Index,
         typename     _List>
struct value_list_at;

// value_list_at<_Index, DValueList<...>>
//   trait: the populated specialization with bounds checking.
template<std::size_t _Index,
         auto...      _Values>
struct value_list_at<_Index, DValueList<_Values...>>
{
private:
    static_assert((_Index < sizeof...(_Values)),
                  "Non-type parameter `_Index` is out of range for the "
                  "value_list.");

public:
    static constexpr auto value =
        internal::value_list_at_helper<_Index, _Values...>::value;
};

// value_list_at_v
//   value: convenience alias for value_list_at<_Index, _List>::value.
template<std::size_t _Index,
         typename     _List>
inline constexpr auto value_list_at_v =
    value_list_at<_Index, _List>::value;

// at
//   function: the _Index-th element of a DValueList instance, returned as a
// value carrier (val_t) so it slots straight back into the carrier pipeline.
template<std::size_t _Index,
         auto...      _Values>
constexpr
auto at(
    DValueList<_Values...>
)
{
    return val<value_list_at_v<_Index, DValueList<_Values...>>>;
}


// ===========================================================================
// V.   growth  (append / prepend / concat)
// ===========================================================================

// append
//   function: the list with _Value appended at the end (the §10.3 collect
// step).  The new element is supplied as a value carrier.
template<auto... _Values,
         auto    _Value>
constexpr
DValueList<_Values..., _Value> append(
    DValueList<_Values...>,
    val_t<_Value>
)
{
    return {};
}

// prepend
//   function: the list with _Value inserted at the front.
template<auto    _Value,
         auto... _Values>
constexpr
DValueList<_Value, _Values...> prepend(
    val_t<_Value>,
    DValueList<_Values...>
)
{
    return {};
}

// concat
//   function: concatenation of nothing - the empty list.
constexpr
DValueList<> concat()
{
    return {};
}

// concat
//   function: a single list concatenates to itself (identity).
template<auto... _Values>
constexpr
DValueList<_Values...> concat(
    DValueList<_Values...> _list
)
{
    return _list;
}

// concat
//   function: concatenation of exactly two lists (the base case).
template<auto... _As,
         auto... _Bs>
constexpr
DValueList<_As..., _Bs...> concat(
    DValueList<_As...>,
    DValueList<_Bs...>
)
{
    return {};
}

// concat
//   function: concatenation of three or more lists; folds pairwise from the
// left.  The >=3 arity keeps it disjoint from the two-list base case above.
template<typename    _First,
         typename    _Second,
         typename    _Third,
         typename... _Rest>
constexpr
auto concat(
    _First   _first,
    _Second  _second,
    _Third   _third,
    _Rest... _rest
)
{
    return concat(concat(_first, _second), _third, _rest...);
}


// ===========================================================================
// VI.  transformation  (transform)
// ===========================================================================

// transform
//   function: applies a unary value-domain operation to every element,
// returning a new DValueList.  `_Op` is a carrier-callable leaf - invoking it
// on a value carrier yields a value carrier (val_t<V> -> val_t<f(V)>), the same
// leaf shape compose and the transducer spine use.  Only _Op's result type is
// needed, so the op is taken unnamed and probed in an unevaluated context; the
// value-domain analog of dtuple's tuple_apply_all.
template<auto... _Values,
         typename _Op>
constexpr
auto transform(
    DValueList<_Values...>,
    _Op
)
{
    return DValueList<
        decltype(std::declval<_Op&>()(val<_Values>))::value...
    >{};
}


// ===========================================================================
// VII. reduction  (fold)
// ===========================================================================

NS_INTERNAL

    // value_list_fold_impl
    //   function: base case - an empty list folds to the accumulator.
    template<typename _Acc,
             typename _Op>
    constexpr
    _Acc value_list_fold_impl(
        _Acc _acc,
        _Op,
        DValueList<>
    )
    {
        return _acc;
    }

    // value_list_fold_impl (recursive)
    //   function: fold the head into the accumulator, recurse on the tail.
    template<typename _Acc,
             typename _Op,
             auto     _Head,
             auto...  _Tail>
    constexpr
    auto value_list_fold_impl(
        _Acc _acc,
        _Op  _op,
        DValueList<_Head, _Tail...>
    )
    {
        return value_list_fold_impl(_op(_acc, val<_Head>),
                                    _op,
                                    DValueList<_Tail...>{});
    }

NS_END  // internal

// fold
//   function: left fold over a DValueList.  `_op` is a binary carrier-callable
// reducer (acc, val_t<V>) -> acc and `_seed` is the initial accumulator (any
// object, typically a carrier or another DValueList).  This is the value-domain
// driver reduce_ct (core/functional/reduce.hpp) builds on.
template<auto... _Values,
         typename _Acc,
         typename _Op>
constexpr
auto fold(
    DValueList<_Values...> _list,
    _Acc                   _seed,
    _Op                    _op
)
{
    return internal::value_list_fold_impl(_seed, _op, _list);
}


// ===========================================================================
// VIII. concept face
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // ValueList
    //   concept: satisfied by any DValueList specialization.  PascalCase
    // per the project's concept naming convention, paralleling
    // is_value_list_v.
    template<typename _Type>
    concept ValueList = is_value_list<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_VALUE_LIST_HPP
