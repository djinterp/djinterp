/*******************************************************************************
* djinterp [re_std]                               bidirectional_iterator_tag.hpp
*
* bidirectional_iterator_tag class header:
* tag for bidirectional iterators — multi-pass, both-direction
* iteration via -- as well as ++. Derives from forward_iterator_tag.
*
*
* path:      /inc/re_std/iterator/bidirectional_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_HPP
#define RE_STD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/forward_iterator_tag.hpp"


namespace re_std
{

struct bidirectional_iterator_tag : public forward_iterator_tag
{
};


}  // re_std
#endif  // RE_STD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_HPP
