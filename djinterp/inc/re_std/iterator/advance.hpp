/*******************************************************************************
* djinterp [re_std]                                                  advance.hpp
*
* advance function header:
* advance(_it, _n) moves _it forward (or backward, if _n is negative
* and the iterator is bidirectional or stronger) by _n steps.
*
* complexity:
*   O(1) for random-access iterators (uses += directly).
*   O(|_n|) otherwise (loop with ++, or -- for bidirectional).
*
* selected by tag dispatch on iterator_traits<It>::iterator_category.
*
* preconditions:
*   _n must be reachable from _it. For input/forward iterators _n must
*   be non-negative — the standard does not define negative advance
*   for these. We don't enforce this at compile time.
*
* added in std C++98; constexpr in C++17. re_std back-ports the
* constexpr to C++14 via RE_STD_CONSTEXPR_CPP14: these are void functions with
* loops, which C++11's constexpr rules cannot hold.
*
*
* path:      /inc/re_std/iterator/advance.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_ADVANCE_HPP
#define RE_STD_ITERATOR_ADVANCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/iterator_traits.hpp"
#include "re_std/iterator/input_iterator_tag.hpp"
#include "re_std/iterator/bidirectional_iterator_tag.hpp"
#include "re_std/iterator/random_access_iterator_tag.hpp"


namespace re_std
{
namespace internal
{

    // ---- tag-dispatched implementations ----
    //
    // Most-specific tag first by overload viability:
    //   * random_access overload accepts random_access_iterator_tag and
    //     anything derived from it (e.g. contiguous_iterator_tag).
    //   * bidirectional accepts bidirectional and weaker.
    //   * input is the catch-all.

    template<typename It, typename Distance>
    RE_STD_CONSTEXPR_CPP14 void advance_impl
    (
        It&        _it,
        Distance   _n,
        random_access_iterator_tag
    )
    {
        _it += _n;
    }

    template<typename It, typename Distance>
    RE_STD_CONSTEXPR_CPP14 void advance_impl
    (
        It&        _it,
        Distance   _n,
        bidirectional_iterator_tag
    )
    {
        if (_n >= 0)
        {
            for (; _n > 0; --_n) ++_it;
        }
        else
        {
            for (; _n < 0; ++_n) --_it;
        }
    }

    template<typename It, typename Distance>
    RE_STD_CONSTEXPR_CPP14 void advance_impl
    (
        It&        _it,
        Distance   _n,
        input_iterator_tag
    )
    {
        // _n is required to be non-negative for input/forward iterators;
        // we don't enforce, just assume.
        for (; _n > 0; --_n) ++_it;
    }

}  // internal
template<typename It, typename Distance>
RE_STD_CONSTEXPR_CPP14 void advance(It& _it, Distance _n)
{
    internal::advance_impl
    (
        _it,
        typename iterator_traits<It>::difference_type(_n),
        typename iterator_traits<It>::iterator_category()
    );
}


}  // re_std
#endif  // RE_STD_ITERATOR_ADVANCE_HPP
