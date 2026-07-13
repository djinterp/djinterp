/******************************************************************************
* djinterp [meta]                                                 bsearch.hpp
*
*   A single compile-time binary-search ENGINE over a typename pack,
* agnostic to what the entries are and how they are ordered.  The
* caller injects the ordering as two template-template PREDICATES with
* the search target ("needle") baked in:
*
*     _Below<Entry>  : true iff Entry sorts BEFORE the needle
*     _Above<Entry>  : true iff Entry sorts AFTER  the needle
*
* and a hit is simply (!_Below && !_Above).  Because the needle never
* appears as an engine parameter - only inside the predicates the
* caller forms - one engine serves an NTTP needle and a type needle
* equally.  The predicate shape matches find_by_pred (lookup.hpp): a
* template<typename> class exposing `static constexpr bool ::value`.
*
*   This header is deliberately key-CONVENTION-agnostic: it knows
* nothing about ::key or ::key_type.  The convention-specific
* shorthands (find_by_key_bsearch / find_by_type_key_bsearch) live in
* lookup.hpp, which owns those conventions; they are thin adapters
* that form the predicates and call bsearch_by here.
*
*   Precondition for any search: the pack is sorted ascending under the
* SAME order the predicates encode.  Misses yield lookup_npos /
* lookup_not_found (both from lookup_sentinels.hpp, shared with
* lookup.hpp so the engine and the search families agree on miss
* values without a cyclic include).
*
*   Complexity: O(log N) recursion DEPTH.  As with any compile-time
* search the instantiation COUNT is not strictly log N (each index
* probe is itself an O(index) pack walk via pack_element); the win is
* bounded depth, which matters mainly for large N against
* -ftemplate-depth.  For small N a linear find_by_* is usually cheaper
* to compile.
*
*
* path:      /inc/djinterp/core/meta/bsearch.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.03
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    pack_element_or          (bounds-guarded indexed access)
II.   bsearch_by               (the engine)
*/

#ifndef DJINTERP_BSEARCH_
#define DJINTERP_BSEARCH_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./pack_element.hpp"                   // pack_element_t
#include "../util/lookup/lookup_sentinels.hpp"  // lookup_not_found, lookup_npos


NS_DJINTERP


// ===========================================================================
// I.   pack_element_or
// ===========================================================================

// pack_element_or
//   trait: pack_element_t<_Index, _Pack...> when _Ok is true, else
// _Fallback.  The point is INSTANTIATION DEFERRAL: a plain
// std::conditional_t would form BOTH branch types, instantiating
// pack_element_t with a possibly-invalid index (e.g. lookup_npos) and
// hard-erroring.  Routing the real access through a partial
// specialization means pack_element_t is touched ONLY when _Ok holds.
//
//   Lives here rather than in type_traits because its sole purpose is
// to guard the miss path of a search; it is the search engine's
// companion, not a general type utility.
template<bool         _Ok,
         std::size_t  _Index,
         typename     _Fallback,
         typename...  _Pack>
struct pack_element_or
{
    using type = _Fallback;
};

template<std::size_t  _Index,
         typename      _Fallback,
         typename...   _Pack>
struct pack_element_or<true, _Index, _Fallback, _Pack...>
{
    using type = pack_element_t<_Index, _Pack...>;
};

// pack_element_or_t
//   alias: shorthand for pack_element_or<...>::type.
template<bool         _Ok,
         std::size_t  _Index,
         typename     _Fallback,
         typename...  _Pack>
using pack_element_or_t =
    typename pack_element_or<_Ok, _Index, _Fallback, _Pack...>::type;


// ===========================================================================
// II.  bsearch_by
// ===========================================================================

NS_INTERNAL

    // bsearch_by_step / _range
    //   helper: half-open [_Lo, _Hi) search driven by the two needle
    // predicates.  The empty-range terminator is a SEPARATE
    // specialization (not a ternary), because a ternary in the
    // initializer would still instantiate bsearch_by_range on an empty
    // range and probe pack_element out of bounds - template
    // instantiation is not lazy across ?:.  A hit short-circuits: the
    // matched index is carried in _Found and reported immediately
    // rather than by collapsing the range (which would not converge).
    //
    //   Branch directions (the easy thing to get backwards):
    //     _Below<mid> -> mid sorts before needle -> go RIGHT
    //     _Above<mid> -> mid sorts after  needle -> go LEFT
    template<bool                      _Done,
             std::size_t               _Found,
             template<typename> class  _Below,
             template<typename> class  _Above,
             std::size_t               _Lo,
             std::size_t               _Hi,
             typename...               _Entries>
    struct bsearch_by_step;

    template<std::size_t               _Found,
             template<typename> class  _Below,
             template<typename> class  _Above,
             std::size_t               _Lo,
             std::size_t               _Hi,
             typename...               _Entries>
    struct bsearch_by_range
    {
    private:
        static constexpr std::size_t mid = _Lo + (_Hi - _Lo) / 2;

        using mid_entry = pack_element_t<mid, _Entries...>;

        static constexpr bool go_right = _Below<mid_entry>::value;
        static constexpr bool go_left  = _Above<mid_entry>::value;
        static constexpr bool hit      = (!go_left && !go_right);

        static constexpr std::size_t next_lo = go_right ? (mid + 1) : _Lo;
        static constexpr std::size_t next_hi = go_left  ? mid       : _Hi;

    public:
        static constexpr std::size_t value =
            bsearch_by_step<hit, mid, _Below, _Above, next_lo, next_hi,
                            _Entries...>::value;
    };

    // still searching, non-empty range
    template<std::size_t               _Found,
             template<typename> class  _Below,
             template<typename> class  _Above,
             std::size_t               _Lo,
             std::size_t               _Hi,
             typename...               _Entries>
    struct bsearch_by_step<false, _Found, _Below, _Above, _Lo, _Hi, _Entries...>
    {
        static constexpr std::size_t value =
            bsearch_by_range<_Found, _Below, _Above, _Lo, _Hi,
                             _Entries...>::value;
    };

    // still searching, EMPTY range -> miss (more specialized; wins)
    template<std::size_t               _Found,
             template<typename> class  _Below,
             template<typename> class  _Above,
             std::size_t               _LoHi,
             typename...               _Entries>
    struct bsearch_by_step<false, _Found, _Below, _Above, _LoHi, _LoHi, _Entries...>
    {
        static constexpr std::size_t value = lookup_npos;
    };

    // hit -> report index, stop
    template<std::size_t               _Found,
             template<typename> class  _Below,
             template<typename> class  _Above,
             std::size_t               _Lo,
             std::size_t               _Hi,
             typename...               _Entries>
    struct bsearch_by_step<true, _Found, _Below, _Above, _Lo, _Hi, _Entries...>
    {
        static constexpr std::size_t value = _Found;
    };

NS_END  // internal

// bsearch_by
//   trait: THE engine.  Binary-search _Entries (sorted ascending under
// the order encoded by _Below / _Above) for the entry the predicates
// identify as equal to the implicit needle.
//
//   Result members mirror lookup.hpp's find_* family:
//     ::type   = matching entry | lookup_not_found
//     ::found  = bool
//     ::index  = position        | lookup_npos
//
//   Most callers want a key-convention shorthand from lookup.hpp
// (find_by_key_bsearch / find_by_type_key_bsearch).  Use bsearch_by
// directly when neither convention fits: a custom projection, a
// reversed order, or a heterogeneous key.
template<template<typename> class _Below,
         template<typename> class _Above,
         typename...              _Entries>
struct bsearch_by
{
private:
    static constexpr std::size_t idx =
        internal::bsearch_by_step<false, lookup_npos, _Below, _Above,
                                  0, sizeof...(_Entries), _Entries...>::value;

public:
    using type =
        pack_element_or_t<(idx != lookup_npos), idx,
                          lookup_not_found, _Entries...>;

    static D_CONSTEXPR bool        found = (idx != lookup_npos);
    static D_CONSTEXPR std::size_t index = idx;
};

// bsearch_by_t
//   type: convenience alias for bsearch_by<...>::type.
template<template<typename> class _Below,
         template<typename> class _Above,
         typename...              _Entries>
using bsearch_by_t = typename bsearch_by<_Below, _Above, _Entries...>::type;


NS_END  // djinterp


#endif  // DJINTERP_BSEARCH_