/***********************************************************************
* re_std                                           contiguous_iterator_tag.hpp
*
* tag for contiguous iterators — random-access PLUS the guarantee that
* logically adjacent elements are physically adjacent in memory
* (i.e. *(it + n) and it[n] refer to the same byte address). Derives
* from random_access_iterator_tag.
*
* introduced in std C++20. re_std back-ports the tag itself, but the
* full set of "this iterator carries the contiguous tag" facilities
* (e.g. iter_concept detection in iterator_traits) only activate on
* C++20+ since the standard library on earlier tiers has no contiguous
* iterator anywhere to detect.
*
* raw pointers automatically receive this tag through the raw-pointer
* specialisation of iterator_traits, on every tier from C++20+. Pre-
* C++20 they get random_access_iterator_tag (which is what std does
* on those tiers as well).
*
*
* path:      /inc/djinterp/re_std/iterator/contiguous_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_CONTIGUOUS_ITERATOR_TAG_
#define DJINTERP_RE_STD_ITERATOR_CONTIGUOUS_ITERATOR_TAG_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    #include "re_std/iterator/random_access_iterator_tag.hpp"


namespace re_std
{

struct contiguous_iterator_tag : public random_access_iterator_tag
{
};


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_CONTIGUOUS_ITERATOR_TAG_
