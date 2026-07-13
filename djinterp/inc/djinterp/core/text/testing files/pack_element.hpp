/******************************************************************************
* djinterp [meta]                                              pack_element.hpp
*
*   Direct positional access into a typename parameter pack, without routing
* through std::tuple.  This is the primitive that pack-walking utilities
* (binary search, sorted lookup, get-like access) build on, so the "recurse
* over the index, peel one head per step" pattern lives in exactly one place.
*
*   Factored into its own leaf header so consumers that need only positional
* access - most notably bsearch.hpp - depend on this rather than dragging in
* the whole of type_traits.hpp for one alias.  Mirrors the role lookup_sentinels.hpp
* plays for the lookup family: a tiny, dependency-light component shared by two
* or more modules.  Nothing else belongs here.
*
*   Relationship to tuple_type_at (dtuple.hpp): tuple_type_at is the TUPLE-facing
* accessor - it normalizes its input via to_tuple_t (accepting either a pack or a
* single std::tuple) and additionally exposes a runtime `value(tuple)` getter.
* pack_element is the leaner PACK-facing core: no tuple normalization, no runtime
* getter, just the type at an index.  tuple_type_at may be expressed in terms of
* pack_element; pack_element never depends on tuple_type_at.
*
*
* path:      /inc/djinterp/core/meta/pack_element.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_META_PACK_ELEMENT_
#define DJINTERP_META_PACK_ELEMENT_ 1

// std
#include <cstddef>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// pack_element
//   trait: the _Index-th type in the pack (0-based).  Out-of-range _Index is a
// hard error (the recursion runs off the end of the pack and fails to match the
// base case) - callers that may pass an invalid index should guard it (see
// pack_element_or in bsearch.hpp, or precede the access with a bounds check).
template<std::size_t _Index,
         typename    _Head,
         typename... _Tail>
struct pack_element
{
    using type = typename pack_element<_Index - 1, _Tail...>::type;
};

// pack_element<0, ...>
//   trait: base case - index 0 yields the current head.
template<typename    _Head,
         typename... _Tail>
struct pack_element<0, _Head, _Tail...>
{
    using type = _Head;
};

// pack_element_t
//   type: convenience alias for pack_element<...>::type.
template<std::size_t _Index,
         typename... _Pack>
using pack_element_t = typename pack_element<_Index, _Pack...>::type;


NS_END  // djinterp


#endif  // DJINTERP_META_PACK_ELEMENT_
