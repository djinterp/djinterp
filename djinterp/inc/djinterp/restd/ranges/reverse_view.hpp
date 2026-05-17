/******************************************************************************
* djinterp [restd]                                            reverse_view.hpp
*
* reverse_view header:
*   Provides the C++20 reversal adaptor. reverse_view<V> presents
* the elements of an underlying bidirectional, common_range V in
* reverse order. Implemented as a thin wrapper around restd::
* reverse_iterator (shipped <iterator> Phase 7b).
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + reverse_iterator.
*   - Requires V to be a common_range (iterator_t<V> == sentinel_t<V>).
*     The C++20 spec accepts non-common ranges by internally caching
*     ranges::next(begin(base), end(base)) to obtain an iterator at
*     the end position, then wrapping it; this path requires
*     common_iterator (deferred in <iterator> Phase 7c), so restd's
*     reverse_view does NOT support non-common ranges. For those,
*     pipe through common_view (also deferred) once it ships, or
*     materialise to a subrange<iterator_t<V>, iterator_t<V>> with
*     a hand-advanced end iterator.
*   - Requires V to be bidirectional. Operator-- on the underlying
*     iterator is invoked during forward iteration of the reverse.
*   - enable_borrowed_range<reverse_view<V>> inherits from
*     enable_borrowed_range<V>: the reverse_iterators are valid
*     exactly as long as V's underlying iterators are valid.
*
*   COLOCATED:
*   restd::views::reverse(r).
*
*
* path:      /inc/djinterp/restd/ranges/reverse_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_REVERSE_VIEW_
#define DJINTERP_RESTD_RANGES_REVERSE_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/reverse_iterator.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   REVERSE_VIEW
// ===========================================================================

// reverse_view<_View>
//   class: presents _View in reverse order via reverse_iterator.
// Requires _View to be a bidirectional common_range.
template<typename _View>
class reverse_view : public view_interface<reverse_view<_View> >
{
public:
    typedef _View                                       base_view;
    typedef reverse_iterator<iterator_t<_View> >        iterator;
    typedef iterator                                    sentinel;


private:
    _View  m_base;


public:
    // default ctor
    D_CONSTEXPR
    reverse_view()
        : m_base()
    {}

    // value ctor
    D_CONSTEXPR
    reverse_view(
        _View  _base
    )
        : m_base(static_cast<_View&&>(_base))
    {}


    // base
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }


    // begin
    //   function: reverse_iterator(end(base)). Requires base to be
    // a common_range so end returns an iterator_t<_View>.
    D_CONSTEXPR iterator
    begin()
    {
        return iterator(restd::end(m_base));
    }

    D_CONSTEXPR iterator
    begin() const
    {
        return iterator(restd::end(m_base));
    }


    // end
    //   function: reverse_iterator(begin(base)).
    D_CONSTEXPR iterator
    end()
    {
        return iterator(restd::begin(m_base));
    }

    D_CONSTEXPR iterator
    end() const
    {
        return iterator(restd::begin(m_base));
    }


    // size
    //   function: forwards to the underlying view when sized.
    D_CONSTEXPR
    auto
    size()
        -> decltype(restd::size(m_base))
    {
        return restd::size(m_base);
    }

    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_base))
    {
        return restd::size(m_base);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<reverse_view<V>>
//   trait: borrowed iff the underlying _View is itself borrowed.
// The reverse_iterators wrap V's underlying iterators, so their
// validity exactly tracks V's.
template<typename _View>
struct enable_borrowed_range<reverse_view<_View> >
    : enable_borrowed_range<_View>
{};


// ===========================================================================
// III. VIEWS::REVERSE
// ===========================================================================

namespace views
{
    // reverse_fn
    //   class: closure-fn for reverse. Pipe-able via the
    // range_adaptor_closure base.
    struct reverse_fn : range_adaptor_closure<reverse_fn>
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        reverse_view<typename internal::all_dispatch<_R>::type>
        operator()(
            _R&&  _r
        ) const
        {
            typedef typename internal::all_dispatch<_R>::type  view_type;
            return reverse_view<view_type>(
                internal::all_dispatch<_R>::call(static_cast<_R&&>(_r))
            );
        }
    };

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline D_CONSTEXPR reverse_fn reverse = reverse_fn();
#else
    static D_CONSTEXPR reverse_fn reverse = reverse_fn();
#endif
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_REVERSE_VIEW_
