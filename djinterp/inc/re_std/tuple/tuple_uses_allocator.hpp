/*******************************************************************************
* djinterp [re_std]                                     tuple_uses_allocator.hpp
*
* tuple_uses_allocator support header:
*   uses_allocator<tuple<Ts...>, Alloc> specialisation.
*
*   UNCONDITIONALLY TRUE, and that is std's rule rather than an approximation.
* tuple is allocator-aware by construction: it accepts the
* allocator_arg_t-tagged constructors and passes the allocator down to whichever
* elements want it, so the answer does not depend on whether any individual
* element is itself allocator-aware. A tuple of ints still reports true; it
* simply has nothing to forward the allocator to.
*
*   The primary template asks whether T has a nested allocator_type convertible
* from Alloc, which tuple does not - so without this specialisation
* uses_allocator<tuple<...>, A> would answer false and uses-allocator
* construction would silently skip the tagged constructors.
*
*   NOTE: the allocator-extended tuple CONSTRUCTORS are still outstanding
* (roadmap 11). This specialisation is the trait half only; it is shipped now
* because it is additive and because the trait answering correctly is what lets
* generic allocator-aware code compile against re_std::tuple at all.
*
*
* path:      /inc/re_std/tuple/tuple_uses_allocator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_USES_ALLOCATOR_HPP
#define RE_STD_TUPLE_TUPLE_USES_ALLOCATOR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../memory/uses_allocator.hpp"
#include "./tuple.hpp"

namespace re_std
{

// uses_allocator<tuple<Types...>, Alloc>
//   trait: tuple is always allocator-aware.
template<typename... Types, typename Alloc>
struct uses_allocator<tuple<Types...>, Alloc> : true_type
{};

}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TUPLE_TUPLE_USES_ALLOCATOR_HPP
