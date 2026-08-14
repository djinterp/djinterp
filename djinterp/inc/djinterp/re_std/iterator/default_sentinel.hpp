/******************************************************************************
* re_std [iterator]                                        default_sentinel.hpp
*
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
* path:      /inc/djinterp/re_std/iterator/default_sentinel.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_ITERATOR_DEFAULT_SENTINEL_
#define RESTD_ITERATOR_DEFAULT_SENTINEL_ 1

#include "../../djinterp.hpp"

NS_DJINTERP
NS_RESTD

// default_sentinel_t
//   struct: the "ask the iterator" sentinel type.
struct default_sentinel_t {};

// default_sentinel
//   constant: the default_sentinel_t instance.
D_INLINE_VAR D_CONSTEXPR default_sentinel_t default_sentinel = default_sentinel_t();

NS_END
NS_END

#endif  // RESTD_ITERATOR_DEFAULT_SENTINEL_
