/*******************************************************************************
* djinterp [re_std]                                      output_iterator_tag.hpp
*
* output_iterator_tag class header:
* tag for output iterators — single-pass, write-only iteration.
* Standalone in the hierarchy: output_iterator_tag does not derive
* from input_iterator_tag, and forward_iterator_tag does not derive
* from output_iterator_tag.
*
* a forward (or stronger) iterator can act as an output iterator
* through usage, but the TAG hierarchy does not encode that — code
* that needs "this iterator can write" should check both branches
* of the hierarchy independently when necessary.
*
*
* path:      /inc/re_std/iterator/output_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_OUTPUT_ITERATOR_TAG_HPP
#define RE_STD_ITERATOR_OUTPUT_ITERATOR_TAG_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{

struct output_iterator_tag
{
};


}  // re_std
#endif  // RE_STD_ITERATOR_OUTPUT_ITERATOR_TAG_HPP
