/*******************************************************************************
* djinterp [re_std]                                          take_while_view.hpp
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
*     interfaces (common_view itself is deferred in re_std; for now
*     materialise via subrange or copy into a container).
*
*   COLOCATED:
*   re_std::views::take_while(r, pred).
*
*
* path:      /inc/re_std/ranges/take_while_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_TAKE_WHILE_VIEW_HPP
#define RE_STD_RANGES_TAKE_WHILE_VIEW_HPP 1

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
// I.   TAKE_WHILE_VIEW
// ===========================================================================

// take_while_view<View, Pred>
//   class: stops at the first element of View where Pred returns
// false. Begin = re_std::begin(base); end = custom sentinel.
template<typename View,
         typename Pred>
class take_while_view : public view_interface<take_while_view<View, Pred> >
{
public:
    typedef View   base_view;
    typedef Pred   predicate_type;


private:
    View                           m_base;
    internal::movable_box<Pred>    m_pred;


public:
    // =======================================================
    // I.A   NESTED SENTINEL
    // =======================================================

    // sentinel
    //   class: holds the underlying end sentinel and a back-pointer
    // to the parent take_while_view. Compares equal to iterator_t<View>
    // when either (a) the iterator has reached the underlying end,
    // or (b) the predicate returns false on *iter.
    class sentinel
    {
    private:
        sentinel_t<View>          m_end;
        take_while_view const*     m_parent;


    public:
        RE_STD_CONSTEXPR
        sentinel()
            : m_end(),
              m_parent(RE_STD_NULLPTR)
        {}

        RE_STD_CONSTEXPR
        sentinel(
            sentinel_t<View>          _e,
            take_while_view const*     _p
        )
            : m_end(_e),
              m_parent(_p)
        {}


        RE_STD_CONSTEXPR sentinel_t<View>
        base() const
        {
            return m_end;
        }


        // iterator-vs-sentinel comparisons.
        //   note: the (_it == _s.m_end) check MUST come first —
        // dereferencing the end iterator is undefined behaviour.
        // Short-circuit evaluation guards the predicate invocation.
        friend RE_STD_CONSTEXPR bool
        operator==(
            iterator_t<View> const&  _it,
            sentinel const&           _s
        )
        {
            return ( (_it == _s.m_end)
                  || !((*(_s.m_parent->m_pred))(*_it)) );
        }

        friend RE_STD_CONSTEXPR bool
        operator!=(
            iterator_t<View> const&  _it,
            sentinel const&           _s
        )
        {
            return !(_it == _s);
        }

        friend RE_STD_CONSTEXPR bool
        operator==(
            sentinel const&           _s,
            iterator_t<View> const&  _it
        )
        {
            return (_it == _s);
        }

        friend RE_STD_CONSTEXPR bool
        operator!=(
            sentinel const&           _s,
            iterator_t<View> const&  _it
        )
        {
            return !(_it == _s);
        }
    };


public:
    // default ctor
    RE_STD_CONSTEXPR
    take_while_view()
        : m_base(),
          m_pred()
    {}

    // value ctor
    RE_STD_CONSTEXPR
    take_while_view(
        View  _base,
        Pred  _pred
    )
        : m_base(static_cast<View&&>(_base)),
          m_pred(static_cast<Pred&&>(_pred))
    {}


    // base
    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }

    // pred
    //   function: const access to the stored predicate.
    RE_STD_CONSTEXPR Pred const&
    pred() const
    RE_STD_NOEXCEPT
    {
        return *m_pred;
    }


    // begin
    //   function: returns iterator_t<View> directly. No wrapping.
    RE_STD_CONSTEXPR_CPP14 iterator_t<View>
    begin()
    {
        return re_std::begin(m_base);
    }

    RE_STD_CONSTEXPR
    auto
    begin() const
        -> decltype(re_std::begin(m_base))
    {
        return re_std::begin(m_base);
    }


    // end
    //   function: returns the custom sentinel wrapping the
    // underlying end and a back-pointer to this view.
    RE_STD_CONSTEXPR_CPP14 sentinel
    end()
    {
        return sentinel(re_std::end(m_base), this);
    }

    RE_STD_CONSTEXPR sentinel
    end() const
    {
        return sentinel(re_std::end(m_base), this);
    }
};


// ===========================================================================
// II.  TAKE_WHILE_CLOSURE (bound form for pipe syntax)
// ===========================================================================

namespace internal
{

template<typename Pred>
struct take_while_closure : range_adaptor_closure<take_while_closure<Pred> >
{
    Pred pred;

    RE_STD_CONSTEXPR
    take_while_closure()
        : pred()
    {}

    RE_STD_CONSTEXPR explicit
    take_while_closure(
        Pred _p
    )
        : pred(static_cast<Pred&&>(_p))
    {}

    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    take_while_view<typename internal::all_dispatch<R>::type, Pred>
    operator()(
        R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        return take_while_view<view_type, Pred>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            pred
        );
    }
};

}  // internal


// ===========================================================================
// III. VIEWS::TAKE_WHILE
// ===========================================================================

namespace views
{
    // views::take_while(_r, _pred)  [direct form]
    template<typename R,
             typename Pred>
    RE_STD_CONSTEXPR_INLINE
    take_while_view<typename internal::all_dispatch<R>::type,
                    typename decay<Pred>::type>
    take_while(
        R&&    _r,
        Pred&& _pred
    )
    {
        typedef typename internal::all_dispatch<R>::type  view_type;
        typedef typename decay<Pred>::type                pred_type;
        return take_while_view<view_type, pred_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            static_cast<Pred&&>(_pred)
        );
    }

    // views::take_while(_pred)  [bound form]
    template<typename Pred>
    RE_STD_CONSTEXPR_INLINE
    internal::take_while_closure<typename decay<Pred>::type>
    take_while(
        Pred&& _pred
    )
    {
        return internal::take_while_closure<typename decay<Pred>::type>(
            static_cast<Pred&&>(_pred)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_TAKE_WHILE_VIEW_HPP
