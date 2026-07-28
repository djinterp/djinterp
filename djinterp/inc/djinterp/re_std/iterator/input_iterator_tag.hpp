/***********************************************************************
* restd                                                  input_iterator_tag.hpp
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
* path:      /inc/restd/iterator/input_iterator_tag.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_INPUT_ITERATOR_TAG_
#define RESTD_ITERATOR_INPUT_ITERATOR_TAG_ 1

#include "djinterp.hpp"


namespace restd
{

struct input_iterator_tag
{
};


}  // namespace restd

#endif  // RESTD_ITERATOR_INPUT_ITERATOR_TAG_
