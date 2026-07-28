/***********************************************************************
* restd                                                            advance.hpp
*
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
* added in std C++98; constexpr in C++17. restd back-ports the
* constexpr to all tiers via the D_CONSTEXPR macro.
*
*
* path:      /inc/restd/iterator/advance.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_ADVANCE_
#define RESTD_ITERATOR_ADVANCE_ 1

#include "djinterp.hpp"

#include "restd/iterator/iterator_traits.hpp"
#include "restd/iterator/input_iterator_tag.hpp"
#include "restd/iterator/bidirectional_iterator_tag.hpp"
#include "restd/iterator/random_access_iterator_tag.hpp"


namespace restd
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

    template<typename _It, typename _Distance>
    D_CONSTEXPR void advance_impl
    (
        _It&        _it,
        _Distance   _n,
        random_access_iterator_tag
    )
    {
        _it += _n;
    }

    template<typename _It, typename _Distance>
    D_CONSTEXPR void advance_impl
    (
        _It&        _it,
        _Distance   _n,
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

    template<typename _It, typename _Distance>
    D_CONSTEXPR void advance_impl
    (
        _It&        _it,
        _Distance   _n,
        input_iterator_tag
    )
    {
        // _n is required to be non-negative for input/forward iterators;
        // we don't enforce, just assume.
        for (; _n > 0; --_n) ++_it;
    }

}  // namespace internal


template<typename _It, typename _Distance>
D_CONSTEXPR void advance(_It& _it, _Distance _n)
{
    internal::advance_impl
    (
        _it,
        typename iterator_traits<_It>::difference_type(_n),
        typename iterator_traits<_It>::iterator_category()
    );
}


}  // namespace restd

#endif  // RESTD_ITERATOR_ADVANCE_
