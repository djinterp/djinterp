/*******************************************************************************
* djinterp [re_std]                                       input_iterator_tag.hpp
*
* iterator-category tag types are empty struct types used solely for
* tag dispatch in iterator-aware algorithms. The inheritance hierarchy
* mirrors the iterator concept hierarchy:
*
*   input_iterator_tag
*   forward_iterator_tag         : input_iterator_tag
*   bidirectional_iterator_tag   : forward_iterator_tag
*   random_access_iterator_tag   : bidirectional_iterator_tag
*   contiguous_iterator_tag      : random_access_iterator_tag   (C++20+)
*
*   output_iterator_tag          (standalone — no derivation)
*
* this means an algorithm overload constrained on
* bidirectional_iterator_tag will accept random_access_iterator_tag
* (and on C++20+ contiguous_iterator_tag) by ordinary derived-to-base
* conversion. Tag dispatch falls through to the most-derived viable
* overload via standard overload resolution.
*
*
* path:      /inc/re_std/iterator/input_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_INPUT_ITERATOR_TAG_HPP
#define RE_STD_ITERATOR_INPUT_ITERATOR_TAG_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{

struct input_iterator_tag
{
};


}  // re_std
#endif  // RE_STD_ITERATOR_INPUT_ITERATOR_TAG_HPP
