/*******************************************************************************
* djinterp [re_std]                                          drop_while_view.hpp
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
* path:      /inc/re_std/ranges/drop_while_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_DROP_WHILE_VIEW_HPP
#define RE_STD_RANGES_DROP_WHILE_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./movable_box.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   DROP_WHILE_VIEW
// ===========================================================================

// drop_while_view<View, Pred>
//   class: skips elements at the front of View as long as Pred
// returns true; the first false-result element is the new begin().
template<typename View,
         typename Pred>
class drop_while_view : public view_interface<drop_while_view<View, Pred> >
{
public:
    typedef View   base_view;
    typedef Pred   predicate_type;


private:
    View                           m_base;
    internal::movable_box<Pred>    m_pred;
    mutable bool                m_cache_init;
    mutable iterator_t<View>   m_cache;


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
        iterator_t<View> it = re_std::begin(m_base);
        sentinel_t<View> e  = re_std::end(m_base);
        while (it != e && (*m_pred)(*it))
        {
            ++it;
        }
        m_cache      = it;
        m_cache_init = true;
    }


public:
    // default ctor
    RE_STD_CONSTEXPR
    drop_while_view()
        : m_base(),
          m_pred(),
          m_cache_init(false),
          m_cache()
    {}

    // value ctor
    RE_STD_CONSTEXPR
    drop_while_view(
        View  _base,
        Pred  _pred
    )
        : m_base(static_cast<View&&>(_base)),
          m_pred(static_cast<Pred&&>(_pred)),
          m_cache_init(false),
          m_cache()
    {}


    // base
    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }

    // pred
    RE_STD_CONSTEXPR Pred const&
    pred() const
    RE_STD_NOEXCEPT
    {
        return *m_pred;
    }


    // begin
    //   function: lazy-cached. The first call scans forward for the
    // first failing element; subsequent calls return the cached
    // result.
    iterator_t<View>
    begin() const
    {
        find_first_false();
        return m_cache;
    }


    // end
    //   function: forwarded directly from the underlying view.
    RE_STD_CONSTEXPR
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

namespace internal
{

template<typename Pred>
struct drop_while_closure : range_adaptor_closure<drop_while_closure<Pred> >
{
    Pred pred;

    RE_STD_CONSTEXPR
    drop_while_closure()
        : pred()
    {}

    RE_STD_CONSTEXPR explicit
    drop_while_closure(
        Pred _p
    )
        : pred(static_cast<Pred&&>(_p))
    {}

    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    drop_while_view<typename internal::all_dispatch<R>::type, Pred>
    operator()(
        R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        return drop_while_view<view_type, Pred>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            pred
        );
    }
};

}  // internal


// ===========================================================================
// III. VIEWS::DROP_WHILE
// ===========================================================================

namespace views
{
    // views::drop_while(_r, _pred)  [direct form]
    template<typename R,
             typename Pred>
    RE_STD_CONSTEXPR_INLINE
    drop_while_view<typename internal::all_dispatch<R>::type,
                    typename decay<Pred>::type>
    drop_while(
        R&&    _r,
        Pred&& _pred
    )
    {
        typedef typename internal::all_dispatch<R>::type  view_type;
        typedef typename decay<Pred>::type                pred_type;
        return drop_while_view<view_type, pred_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            static_cast<Pred&&>(_pred)
        );
    }

    // views::drop_while(_pred)  [bound form]
    template<typename Pred>
    RE_STD_CONSTEXPR_INLINE
    internal::drop_while_closure<typename decay<Pred>::type>
    drop_while(
        Pred&& _pred
    )
    {
        return internal::drop_while_closure<typename decay<Pred>::type>(
            static_cast<Pred&&>(_pred)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_DROP_WHILE_VIEW_HPP
