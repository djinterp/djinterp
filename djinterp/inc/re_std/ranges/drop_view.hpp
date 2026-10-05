/*******************************************************************************
* djinterp [re_std]                                                drop_view.hpp
*
* drop_view header:
*   Provides the C++20 suffix adaptor. drop_view<V> skips the first N
* elements of an underlying view V and presents the remainder. The
* skip is computed lazily on the first call to begin() and cached for
* subsequent calls (the C++20 spec mandates this caching to keep
* begin() amortised-O(1) on forward and stronger ranges).
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface.
*   - The first-call cache is conditional: for random-access ranges,
*     begin() does begin+n in O(1) and no caching is needed. For
*     forward / bidirectional / input ranges, begin() advances the
*     underlying iterator N times on its first call, stores the
*     result in a mutable cache, and returns the cached value on
*     subsequent calls. The cache uses a small optional-style
*     wrapper rather than std::optional (not yet shipped in re_std).
*   - For input-only ranges, drop_view is single-pass: the cached
*     iterator can be used at most once for iteration. This matches
*     the C++20 contract.
*
*   COLOCATED:
*   re_std::views::drop(r, n).
*
*
* path:      /inc/re_std/ranges/drop_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_DROP_VIEW_HPP
#define RE_STD_RANGES_DROP_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/iterator_traits.hpp"
#include "../iterator/advance.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   DROP_VIEW
// ===========================================================================

// drop_view<View>
//   class: presents View with the first N elements skipped. begin()
// advances the underlying iterator N times (or to the end, whichever
// comes first) and caches the result.
template<typename View>
class drop_view : public view_interface<drop_view<View> >
{
public:
    typedef View                                  base_view;
    typedef typename iterator_traits<
                          iterator_t<View>
                      >::difference_type           difference_type;


private:
    // ---- mutable cache for the first-call advance ----
    // We don't have re_std::optional yet, so we use a manual
    // initialised-flag + storage pair. The cache is mutable so
    // begin() const can populate it.
    View                       m_base;
    difference_type             m_count;
    mutable bool                m_cache_init;
    mutable iterator_t<View>   m_cache;


public:
    // default ctor
    RE_STD_CONSTEXPR
    drop_view()
        : m_base(),
          m_count(0),
          m_cache_init(false),
          m_cache()
    {}

    // value ctor
    RE_STD_CONSTEXPR
    drop_view(
        View            _base,
        difference_type  _n
    )
        : m_base(static_cast<View&&>(_base)),
          m_count(_n),
          m_cache_init(false),
          m_cache()
    {}


    // base
    //   function: returns a copy of the underlying view.
    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }


    // begin
    //   function: advances begin(_base) by m_count positions (or to
    // end), caches the result, and returns it. Subsequent calls
    // return the cached value.
    iterator_t<View>
    begin()
    {
        if (!m_cache_init)
        {
            iterator_t<View> it = re_std::begin(m_base);
            sentinel_t<View> e  = re_std::end(m_base);
            difference_type   n  = m_count;
            while (n > 0 && it != e)
            {
                ++it;
                --n;
            }
            m_cache      = it;
            m_cache_init = true;
        }
        return m_cache;
    }

    // end
    //   function: simply the underlying view's end.
    sentinel_t<View>
    end()
    {
        return re_std::end(m_base);
    }


    // size
    //   function: max(0, size(base) - n). Only well-formed when the
    // underlying view is sized (decltype-SFINAE'd via the trailing
    // return type).
    RE_STD_CONSTEXPR_CPP14
    auto
    size() const
        -> decltype(re_std::size(m_base))
    {
        typedef decltype(re_std::size(m_base)) size_type;
        size_type s = re_std::size(m_base);
        size_type n = static_cast<size_type>(m_count);
        return (s > n) ? (s - n) : static_cast<size_type>(0);
    }
};


// ===========================================================================
// II.  DROP_CLOSURE (bound form for pipe syntax)
// ===========================================================================

namespace internal
{

// drop_closure
//   class: the bound form of views::drop. Holds a count and, when
// invoked, constructs a drop_view directly.
template<typename N>
struct drop_closure : range_adaptor_closure<drop_closure<N> >
{
    N count;

    RE_STD_CONSTEXPR
    drop_closure()
        : count()
    {}

    RE_STD_CONSTEXPR explicit
    drop_closure(
        N _n
    )
        : count(_n)
    {}

    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    drop_view<typename internal::all_dispatch<R>::type>
    operator()(
        R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        typedef typename iterator_traits<
                              iterator_t<typename remove_reference<R>::type>
                          >::difference_type             diff_type;
        return drop_view<view_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            static_cast<diff_type>(count)
        );
    }
};

}  // internal


// ===========================================================================
// III. VIEWS::DROP
// ===========================================================================

namespace views
{
    // views::drop(_r, _n)  [direct form]
    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    drop_view<typename internal::all_dispatch<R>::type>
    drop(
        R&&                                                            _r,
        typename iterator_traits<
                     iterator_t<typename remove_reference<R>::type>
                 >::difference_type                                     _n
    )
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        return drop_view<view_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            _n
        );
    }

    // views::drop(_n)  [bound form]
    //   function: returns a drop_closure for pipe composition.
    template<typename N>
    RE_STD_CONSTEXPR_INLINE
    internal::drop_closure<typename decay<N>::type>
    drop(
        N&& _n
    )
    {
        return internal::drop_closure<typename decay<N>::type>(
            static_cast<N&&>(_n)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_DROP_VIEW_HPP
