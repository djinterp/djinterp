/*******************************************************************************
* djinterp [re_std]                                             reverse_view.hpp
*
* reverse_view header:
*   Provides the C++20 reversal adaptor. reverse_view<V> presents
* the elements of an underlying bidirectional, common_range V in
* reverse order. Implemented as a thin wrapper around re_std::
* reverse_iterator (shipped <iterator> Phase 7b).
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + reverse_iterator.
*   - Requires V to be a common_range (iterator_t<V> == sentinel_t<V>).
*     The C++20 spec accepts non-common ranges by internally caching
*     ranges::next(begin(base), end(base)) to obtain an iterator at
*     the end position, then wrapping it; this path requires
*     common_iterator (deferred in <iterator> Phase 7c), so re_std's
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
*   re_std::views::reverse(r).
*
*
* path:      /inc/re_std/ranges/reverse_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_REVERSE_VIEW_HPP
#define RE_STD_RANGES_REVERSE_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/reverse_iterator.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   REVERSE_VIEW
// ===========================================================================

// reverse_view<View>
//   class: presents View in reverse order via reverse_iterator.
// Requires View to be a bidirectional common_range.
template<typename View>
class reverse_view : public view_interface<reverse_view<View> >
{
public:
    typedef View                                       base_view;
    typedef reverse_iterator<iterator_t<View> >        iterator;
    typedef iterator                                    sentinel;


private:
    View  m_base;


public:
    // default ctor
    RE_STD_CONSTEXPR
    reverse_view()
        : m_base()
    {}

    // value ctor
    RE_STD_CONSTEXPR
    reverse_view(
        View  _base
    )
        : m_base(static_cast<View&&>(_base))
    {}


    // base
    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }


    // begin
    //   function: reverse_iterator(end(base)). Requires base to be
    // a common_range so end returns an iterator_t<View>.
    RE_STD_CONSTEXPR_CPP14 iterator
    begin()
    {
        return iterator(re_std::end(m_base));
    }

    RE_STD_CONSTEXPR iterator
    begin() const
    {
        return iterator(re_std::end(m_base));
    }


    // end
    //   function: reverse_iterator(begin(base)).
    RE_STD_CONSTEXPR_CPP14 iterator
    end()
    {
        return iterator(re_std::begin(m_base));
    }

    RE_STD_CONSTEXPR iterator
    end() const
    {
        return iterator(re_std::begin(m_base));
    }


    // size
    //   function: forwards to the underlying view when sized.
    RE_STD_CONSTEXPR_CPP14
    auto
    size()
        -> decltype(re_std::size(m_base))
    {
        return re_std::size(m_base);
    }

    RE_STD_CONSTEXPR
    auto
    size() const
        -> decltype(re_std::size(m_base))
    {
        return re_std::size(m_base);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<reverse_view<V>>
//   trait: borrowed iff the underlying View is itself borrowed.
// The reverse_iterators wrap V's underlying iterators, so their
// validity exactly tracks V's.
template<typename View>
struct enable_borrowed_range<reverse_view<View> >
    : enable_borrowed_range<View>
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
        template<typename R>
        RE_STD_CONSTEXPR_INLINE
        reverse_view<typename internal::all_dispatch<R>::type>
        operator()(
            R&&  _r
        ) const
        {
            typedef typename internal::all_dispatch<R>::type  view_type;
            return reverse_view<view_type>(
                internal::all_dispatch<R>::call(static_cast<R&&>(_r))
            );
        }
    };

#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline RE_STD_CONSTEXPR reverse_fn reverse = reverse_fn();
#else
    static RE_STD_CONSTEXPR reverse_fn reverse = reverse_fn();
#endif
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_REVERSE_VIEW_HPP
