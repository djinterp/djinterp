/*******************************************************************************
* djinterp [re_std]                                         default_sentinel.hpp
*
* default_sentinel support header:
*   default_sentinel_t and default_sentinel.
*
*   An empty type that means "the end is wherever the iterator says it is".
* Iterators that already know their own bound - counted_iterator knows its
* remaining count, an istream iterator knows the stream failed - compare
* against it instead of against a second iterator, which is what lets a range
* have a sentinel that is not the same type as its iterator.
*
*   CATALOGUE NOTE: this symbol was not on the <iterator> data sheet before
* 2026-08-13. It is a real part of the C++20 header and counted_iterator
* cannot be specified without it, so it is added here rather than left as a
* silent dependency.
*
*   STD IS C++20; re_std IS C++98 - it is an empty struct and a constant.
*
*
* path:      /inc/re_std/iterator/default_sentinel.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_DEFAULT_SENTINEL_HPP
#define RE_STD_ITERATOR_DEFAULT_SENTINEL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

namespace re_std
{

// default_sentinel_t
//   struct: the "ask the iterator" sentinel type.
struct default_sentinel_t {};

// default_sentinel
//   constant: the default_sentinel_t instance.
RE_STD_INLINE_VAR RE_STD_CONSTEXPR default_sentinel_t default_sentinel = default_sentinel_t();

}

#endif  // RE_STD_ITERATOR_DEFAULT_SENTINEL_HPP
