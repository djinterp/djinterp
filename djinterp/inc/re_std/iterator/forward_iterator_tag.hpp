/*******************************************************************************
* djinterp [re_std]                                     forward_iterator_tag.hpp
*
* forward_iterator_tag class header:
* tag for forward iterators — multi-pass, single-direction iteration.
* Derives from input_iterator_tag, so any algorithm taking input
* iterators by tag dispatch will also accept forward iterators.
*
*
* path:      /inc/re_std/iterator/forward_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_FORWARD_ITERATOR_TAG_HPP
#define RE_STD_ITERATOR_FORWARD_ITERATOR_TAG_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/input_iterator_tag.hpp"


namespace re_std
{

struct forward_iterator_tag : public input_iterator_tag
{
};


}  // re_std
#endif  // RE_STD_ITERATOR_FORWARD_ITERATOR_TAG_HPP
