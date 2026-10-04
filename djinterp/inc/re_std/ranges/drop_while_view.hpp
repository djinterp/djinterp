/******************************************************************************
* djinterp [re_std]                                        drop_while_view.hpp
*
* drop_while_view header:
*   Provides the C++20 predicate-suffix adaptor. drop_while_view<V, Pred>
* skips the prefix of V for which Pred returns true and presents the
* remainder. begin() returns iterator_t<V> directly (lazy-cached at
* the first element where Pred fails); end() returns sentinel_t<V>.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface.
*   - Same lazy-cache pattern as drop_view, with a predicate stopping
*     condition instead of a count. mutable cache + init-flag pair
*     since re_std::optional is not yet shipped.
*   - Note that drop_while_view, unlike take_while_view, IS naturally
*     a common-range candidate when V is itself common — begin and
*     end both return iterator/sentinel types directly from V.
*
*   COLOCATED:
*   re_std::views::drop_while(r, pred).
*
*
* path:      /inc/djinterp/re_std/ranges/drop_while_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_DROP_WHILE_VIEW_
#define DJINTERP_RE_STD_RANGES_DROP_WHILE_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./movable_box.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   DROP_WHILE_VIEW
// ===========================================================================

// drop_while_view<_View, _Pred>
//   class: skips elements at the front of _View as long as _Pred
// returns true; the first false-result element is the new begin().
template<typename _View,
         typename _Pred>
class drop_while_view : public view_interface<drop_while_view<_View, _Pred> >
{
public:
    typedef _View   base_view;
    typedef _Pred   predicate_type;


private:
    _View                           m_base;
    internal::movable_box<_Pred>    m_pred;
    mutable bool                m_cache_init;
    mutable iterator_t<_View>   m_cache;


    // find_first_false
    //   function: scans forward from begin(base) for the first
    // element NOT satisfying the predicate; populates the cache.
    void
    find_first_false() const
    {
        if (m_cache_init)
        {
            return;
        }
        iterator_t<_View> it = re_std::begin(m_base);
        sentinel_t<_View> e  = re_std::end(m_base);
        while (it != e && (*m_pred)(*it))
        {
            ++it;
        }
        m_cache      = it;
        m_cache_init = true;
    }


public:
    // default ctor
    D_CONSTEXPR
    drop_while_view()
        : m_base(),
          m_pred(),
          m_cache_init(false),
          m_cache()
    {}

    // value ctor
    D_CONSTEXPR
    drop_while_view(
        _View  _base,
        _Pred  _pred
    )
        : m_base(static_cast<_View&&>(_base)),
          m_pred(static_cast<_Pred&&>(_pred)),
          m_cache_init(false),
          m_cache()
    {}


    // base
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }

    // pred
    D_CONSTEXPR _Pred const&
    pred() const
    D_NOEXCEPT
    {
        return *m_pred;
    }


    // begin
    //   function: lazy-cached. The first call scans forward for the
    // first failing element; subsequent calls return the cached
    // result.
    iterator_t<_View>
    begin() const
    {
        find_first_false();
        return m_cache;
    }


    // end
    //   function: forwarded directly from the underlying view.
    D_CONSTEXPR
    auto
    end() const
        -> decltype(re_std::end(m_base))
    {
        return re_std::end(m_base);
    }
};


// ===========================================================================
// II.  DROP_WHILE_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _Pred>
struct drop_while_closure : range_adaptor_closure<drop_while_closure<_Pred> >
{
    _Pred pred;

    D_CONSTEXPR
    drop_while_closure()
        : pred()
    {}

    D_CONSTEXPR explicit
    drop_while_closure(
        _Pred _p
    )
        : pred(static_cast<_Pred&&>(_p))
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    drop_while_view<typename internal::all_dispatch<_R>::type, _Pred>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return drop_while_view<view_type, _Pred>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            pred
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::DROP_WHILE
// ===========================================================================

namespace views
{
    // views::drop_while(_r, _pred)  [direct form]
    template<typename _R,
             typename _Pred>
    D_CONSTEXPR_INLINE
    drop_while_view<typename internal::all_dispatch<_R>::type,
                    typename decay<_Pred>::type>
    drop_while(
        _R&&    _r,
        _Pred&& _pred
    )
    {
        typedef typename internal::all_dispatch<_R>::type  view_type;
        typedef typename decay<_Pred>::type                pred_type;
        return drop_while_view<view_type, pred_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<_Pred&&>(_pred)
        );
    }

    // views::drop_while(_pred)  [bound form]
    template<typename _Pred>
    D_CONSTEXPR_INLINE
    internal::drop_while_closure<typename decay<_Pred>::type>
    drop_while(
        _Pred&& _pred
    )
    {
        return internal::drop_while_closure<typename decay<_Pred>::type>(
            static_cast<_Pred&&>(_pred)
        );
    }
}  // namespace views


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_DROP_WHILE_VIEW_
