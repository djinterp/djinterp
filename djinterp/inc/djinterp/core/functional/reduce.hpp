/******************************************************************************
* djinterp [functional]                                            reduce.hpp
*
* The drivers - the iteration half of the step/driver split:
*   The dual-domain design separates a pure step (a reducer (acc, x) -> acc,
* written once, domain-agnostic) from a driver that performs the iteration.
* This header is the driver layer.  A single reducer - whether it folds runtime
* values, NTTP value carriers, or type carriers - is run by one of two drivers:
*
*   reduce_rt   a runtime loop over an iterable (constexpr, so it also folds at
*               compile time over a constexpr range).  Lazy/large/infinite
*               sources are fine: it pulls until the range ends.
*   reduce_ct   a compile-time recursion.  Two overloads cover the two pack
*               kinds that remain irreducibly kinded (the roadmap's "irreducible
*               residue"): value_list<auto...> for the NTTP domain and
*               std::tuple<typename...> - the dtuple sequence type - for the
*               type domain.  The value_list overload delegates to value_list's
*               own fold; the tuple overload recurses over the element types and
*               feeds each as a type_c carrier.
*
*   The same reducer body therefore serves all three domains; only the driver
* (loop vs. recursion) and the leaf differ.  This is the substrate the
* transducer spine (transducer.hpp) and the stream modules hang off.
*
* TYPE-DOMAIN ENTRY POINTS
*   reduce_ct over a tuple is offered two ways:
*     reduce_ct(rf, acc, type_c<std::tuple<Ts...>>)   - preferred; the sequence
*         is carried as a type, so element types need NOT be default-
*         constructible.
*     reduce_ct(rf, acc, std::tuple<Ts...>{})         - convenience matching the
*         §10.3 value-passing style; requires default-constructible elements.
*
* TIER
*   reduce_ct and the value carriers are C++17 facilities, so the module is gated
* to the C++17 lifted-functional floor; under C++11 this header is empty.  The
* drivers are deliberately left unconstrained so one body serves every domain;
* a C++20 caller may layer the Reducer / Transducer concepts (structural_traits.hpp)
* at the call site.
*
* path:      /inc/djinterp/core/functional/reduce.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.05
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_REDUCE_
#define DJINTERP_FUNCTIONAL_REDUCE_ 1

// std
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/carrier.hpp"      // type_c / val (carriers)
#include "../meta/value_list.hpp"   // value_list + fold (the NTTP driver)


NS_DJINTERP


//   The drivers are an auto-NTTP / carrier facility (C++17); the module is gated
// to the lifted-functional floor and contributes nothing at the C++11 floor.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// ================================================================
//  reduce_rt  (runtime driver)
// ================================================================

// reduce_rt
//   function: left fold over an iterator range [_first, _last).  constexpr, so
// it also folds at compile time over a constexpr range.  `_rf` is a reducer
// (acc, *it) -> acc and `_acc` is the seed; the accumulator type is fixed
// across the fold.
template<typename _Rf,
         typename _Acc,
         typename _It>
constexpr _Acc
reduce_rt(_Rf _rf, _Acc _acc, _It _first, _It _last)
{
    // pull each element through the reducer until the range is exhausted
    for (; _first != _last; ++_first)
    {
        _acc = _rf(_acc, *_first);
    }

    return _acc;
}

// reduce_rt
//   function: left fold over an iterable (anything with begin()/end()).
// Convenience wrapper around the iterator-range form above.
template<typename _Rf,
         typename _Acc,
         typename _Iterable>
constexpr _Acc
reduce_rt(_Rf _rf, _Acc _acc, const _Iterable& _iterable)
{
    using std::begin;
    using std::end;

    // pull each element through the reducer until the range is exhausted
    for (auto _it = begin(_iterable); _it != end(_iterable); ++_it)
    {
        _acc = _rf(_acc, *_it);
    }

    return _acc;
}


// ================================================================
//  reduce_ct  (compile-time driver)
// ================================================================

// --- value domain (value_list) -------------------------------------------

// reduce_ct
//   function: compile-time left fold over a value_list.  Delegates to
// value_list's own fold - the value-domain recursion lives there - so this is
// purely the unified entry point.  `_rf` is a reducer (acc, val_t<V>) -> acc.
template<typename _Rf,
         typename _Acc,
         auto...  _Values>
constexpr auto
reduce_ct(_Rf _rf, _Acc _acc, value_list<_Values...> _list)
{
    return fold(_list, _acc, _rf);
}

// --- type domain (std::tuple) ---------------------------------------------

NS_INTERNAL

    // reduce_ct_tuple_impl
    //   trait: primary template (declared); specialized on the tuple's element
    // pack so the recursion walks element TYPES (no tuple value is built).
    template<typename _Rf,
             typename _Acc,
             typename _Tuple>
    struct reduce_ct_tuple_impl;

    // reduce_ct_tuple_impl<..., std::tuple<>>
    //   trait: base case - the empty tuple folds to the accumulator.
    template<typename _Rf,
             typename _Acc>
    struct reduce_ct_tuple_impl<_Rf, _Acc, std::tuple<>>
    {
        static constexpr _Acc
        apply(_Rf, _Acc _acc)
        {
            return _acc;
        }
    };

    // reduce_ct_tuple_impl<..., std::tuple<_T0, _Ts...>>
    //   trait: recursive case - fold the head type (as a type_c carrier) into
    // the accumulator, recurse on the tail.
    template<typename    _Rf,
             typename    _Acc,
             typename    _T0,
             typename... _Ts>
    struct reduce_ct_tuple_impl<_Rf, _Acc, std::tuple<_T0, _Ts...>>
    {
        static constexpr auto
        apply(_Rf _rf, _Acc _acc)
        {
            return reduce_ct_tuple_impl<
                       _Rf,
                       decltype(_rf(_acc, type_c<_T0>)),
                       std::tuple<_Ts...>
                   >::apply(_rf, _rf(_acc, type_c<_T0>));
        }
    };

NS_END  // internal

// reduce_ct
//   function: compile-time left fold over a tuple's element TYPES, carried as
// type_c<std::tuple<Ts...>>.  Preferred type-domain entry: the sequence is a
// type, so element types need not be default-constructible.  `_rf` is a reducer
// (acc, type_c<T>) -> acc.
template<typename    _Rf,
         typename    _Acc,
         typename... _Ts>
constexpr auto
reduce_ct(_Rf _rf, _Acc _acc, type_t<std::tuple<_Ts...>>)
{
    return internal::reduce_ct_tuple_impl<_Rf, _Acc,
                                          std::tuple<_Ts...>>::apply(_rf, _acc);
}

// reduce_ct
//   function: convenience overload taking a std::tuple VALUE directly (the
// §10.3 value-passing style).  Requires default-constructible element types;
// otherwise prefer the type_c<std::tuple<...>> form above.
template<typename    _Rf,
         typename    _Acc,
         typename... _Ts>
constexpr auto
reduce_ct(_Rf _rf, _Acc _acc, std::tuple<_Ts...>)
{
    return internal::reduce_ct_tuple_impl<_Rf, _Acc,
                                          std::tuple<_Ts...>>::apply(_rf, _acc);
}


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_REDUCE_
