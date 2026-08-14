/******************************************************************************
* djinterp [restd]                                         take_while_view.hpp
*
* take_while_view header:
*   Provides the C++20 predicate-prefix adaptor. take_while_view<V, Pred>
* presents the elements of V from the start up to (but not including)
* the first element for which Pred returns false.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom sentinel (no custom
*     iterator).
*   - The iterator type is iterator_t<V> directly; the predicate is
*     checked inside the iterator-vs-sentinel comparison. Sentinel
*     holds a back-pointer to the parent take_while_view to invoke
*     the predicate during comparison.
*   - As a consequence take_while_view is NOT a common_range — its
*     end() returns the custom sentinel, not iterator_t<V>. Use
*     common_view to coerce for algorithms requiring iterator-pair
*     interfaces (common_view itself is deferred in restd; for now
*     materialise via subrange or copy into a container).
*
*   COLOCATED:
*   restd::views::take_while(r, pred).
*
*
* path:      /inc/djinterp/re_std/ranges/take_while_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_TAKE_WHILE_VIEW_
#define DJINTERP_RESTD_RANGES_TAKE_WHILE_VIEW_ 1

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
// I.   TAKE_WHILE_VIEW
// ===========================================================================

// take_while_view<_View, _Pred>
//   class: stops at the first element of _View where _Pred returns
// false. Begin = restd::begin(base); end = custom sentinel.
template<typename _View,
         typename _Pred>
class take_while_view : public view_interface<take_while_view<_View, _Pred> >
{
public:
    typedef _View   base_view;
    typedef _Pred   predicate_type;


private:
    _View                           m_base;
    internal::movable_box<_Pred>    m_pred;


public:
    // =======================================================
    // I.A   NESTED SENTINEL
    // =======================================================

    // sentinel
    //   class: holds the underlying end sentinel and a back-pointer
    // to the parent take_while_view. Compares equal to iterator_t<_View>
    // when either (a) the iterator has reached the underlying end,
    // or (b) the predicate returns false on *iter.
    class sentinel
    {
    private:
        sentinel_t<_View>          m_end;
        take_while_view const*     m_parent;


    public:
        D_CONSTEXPR
        sentinel()
            : m_end(),
              m_parent(D_NULLPTR)
        {}

        D_CONSTEXPR
        sentinel(
            sentinel_t<_View>          _e,
            take_while_view const*     _p
        )
            : m_end(_e),
              m_parent(_p)
        {}


        D_CONSTEXPR sentinel_t<_View>
        base() const
        {
            return m_end;
        }


        // iterator-vs-sentinel comparisons.
        //   note: the (_it == _s.m_end) check MUST come first —
        // dereferencing the end iterator is undefined behaviour.
        // Short-circuit evaluation guards the predicate invocation.
        friend D_CONSTEXPR bool
        operator==(
            iterator_t<_View> const&  _it,
            sentinel const&           _s
        )
        {
            return ( (_it == _s.m_end)
                  || !((*(_s.m_parent->m_pred))(*_it)) );
        }

        friend D_CONSTEXPR bool
        operator!=(
            iterator_t<_View> const&  _it,
            sentinel const&           _s
        )
        {
            return !(_it == _s);
        }

        friend D_CONSTEXPR bool
        operator==(
            sentinel const&           _s,
            iterator_t<_View> const&  _it
        )
        {
            return (_it == _s);
        }

        friend D_CONSTEXPR bool
        operator!=(
            sentinel const&           _s,
            iterator_t<_View> const&  _it
        )
        {
            return !(_it == _s);
        }
    };


public:
    // default ctor
    D_CONSTEXPR
    take_while_view()
        : m_base(),
          m_pred()
    {}

    // value ctor
    D_CONSTEXPR
    take_while_view(
        _View  _base,
        _Pred  _pred
    )
        : m_base(static_cast<_View&&>(_base)),
          m_pred(static_cast<_Pred&&>(_pred))
    {}


    // base
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }

    // pred
    //   function: const access to the stored predicate.
    D_CONSTEXPR _Pred const&
    pred() const
    D_NOEXCEPT
    {
        return *m_pred;
    }


    // begin
    //   function: returns iterator_t<_View> directly. No wrapping.
    D_CONSTEXPR iterator_t<_View>
    begin()
    {
        return restd::begin(m_base);
    }

    D_CONSTEXPR
    auto
    begin() const
        -> decltype(restd::begin(m_base))
    {
        return restd::begin(m_base);
    }


    // end
    //   function: returns the custom sentinel wrapping the
    // underlying end and a back-pointer to this view.
    D_CONSTEXPR sentinel
    end()
    {
        return sentinel(restd::end(m_base), this);
    }

    D_CONSTEXPR sentinel
    end() const
    {
        return sentinel(restd::end(m_base), this);
    }
};


// ===========================================================================
// II.  TAKE_WHILE_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _Pred>
struct take_while_closure : range_adaptor_closure<take_while_closure<_Pred> >
{
    _Pred pred;

    D_CONSTEXPR
    take_while_closure()
        : pred()
    {}

    D_CONSTEXPR explicit
    take_while_closure(
        _Pred _p
    )
        : pred(static_cast<_Pred&&>(_p))
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    take_while_view<typename internal::all_dispatch<_R>::type, _Pred>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return take_while_view<view_type, _Pred>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            pred
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::TAKE_WHILE
// ===========================================================================

namespace views
{
    // views::take_while(_r, _pred)  [direct form]
    template<typename _R,
             typename _Pred>
    D_CONSTEXPR_INLINE
    take_while_view<typename internal::all_dispatch<_R>::type,
                    typename decay<_Pred>::type>
    take_while(
        _R&&    _r,
        _Pred&& _pred
    )
    {
        typedef typename internal::all_dispatch<_R>::type  view_type;
        typedef typename decay<_Pred>::type                pred_type;
        return take_while_view<view_type, pred_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<_Pred&&>(_pred)
        );
    }

    // views::take_while(_pred)  [bound form]
    template<typename _Pred>
    D_CONSTEXPR_INLINE
    internal::take_while_closure<typename decay<_Pred>::type>
    take_while(
        _Pred&& _pred
    )
    {
        return internal::take_while_closure<typename decay<_Pred>::type>(
            static_cast<_Pred&&>(_pred)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_TAKE_WHILE_VIEW_
