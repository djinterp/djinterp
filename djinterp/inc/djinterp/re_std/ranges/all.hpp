/******************************************************************************
* djinterp [re_std]                                                    all.hpp
*
* views::all + all_t header:
*   Provides the C++20 views::all customisation-point-object entry
* and its result-type alias all_t. views::all dispatches between
* three behaviours based on its argument:
*     - already a view              -> perfect-forwarded copy / move
*     - lvalue non-view range       -> ref_view
*     - rvalue non-view range       -> owning_view
*
*   PORTABILITY:
*   - C++11+. The dispatch is performed by a partial-specialisation-
*     based internal helper rather than a true C++20 CPO — the pipe
*     operator (r | views::all) is therefore NOT yet supported. Once
*     the ranges:: CPO machinery ships this entry will be promoted
*     to a CPO; user code that writes views::all(r) (function-call
*     form) is unaffected by that future migration.
*   - all_t<R> mirrors std::ranges::views::all_t<R> and resolves to
*     decay_t<R> when R is already a view, ref_view<remove_reference_t<R>>
*     when R is an lvalue non-view range, and owning_view<decay_t<R>>
*     when R is an rvalue non-view range.
*
*
* path:      /inc/djinterp/re_std/ranges/all.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_ALL_
#define DJINTERP_RE_STD_RANGES_ALL_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./view.hpp"
#include "./ref_view.hpp"
#include "./owning_view.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// 0.   INTERNAL: ALL_DISPATCH
// ===========================================================================

NS_INTERNAL

// all_dispatch
//   trait: routes views::all to one of three result types based on
// (a) whether _R after ref-stripping is itself a view, and (b)
// whether _R is an lvalue reference. The three partial spec'ns
// below cover the three cases.
template<typename _R,
         bool _IsView      = view<typename remove_reference<_R>::type>::value,
         bool _IsLvalueRef = is_lvalue_reference<_R>::value>
struct all_dispatch;

// case A: _R is already a view (lvalue or rvalue). Forward as a
// decay_t<_R> — copy from lvalue (requires copyable view), move
// from rvalue.
template<typename _R, bool _IsLvalueRef>
struct all_dispatch<_R, true, _IsLvalueRef>
{
    typedef typename decay<_R>::type type;

    static D_CONSTEXPR type
    call(_R&& _r)
    {
        return static_cast<_R&&>(_r);
    }
};

// case B: _R is an lvalue non-view range. Wrap in ref_view.
template<typename _R>
struct all_dispatch<_R, false, true>
{
    typedef ref_view<typename remove_reference<_R>::type> type;

    static D_CONSTEXPR type
    call(_R&& _r)
    {
        return type(_r);
    }
};

// case C: _R is an rvalue non-view range. Wrap in owning_view via
// move.
template<typename _R>
struct all_dispatch<_R, false, false>
{
    typedef owning_view<typename decay<_R>::type> type;

    static D_CONSTEXPR type
    call(_R&& _r)
    {
        return type(static_cast<_R&&>(_r));
    }
};

NS_END  // internal


// ===========================================================================
// I.   VIEWS::ALL
// ===========================================================================

namespace views
{
    // all_fn
    //   class: function-object form of views::all. Deriving from
    // range_adaptor_closure makes views::all itself pipe-able:
    //     r | views::all  ==  views::all(r)
    struct all_fn : range_adaptor_closure<all_fn>
    {
        template<typename _R>
        D_CONSTEXPR
        typename internal::all_dispatch<_R>::type
        operator()(
            _R&& _r
        ) const
        {
            return internal::all_dispatch<_R>::call(static_cast<_R&&>(_r));
        }
    };

    // views::all
    //   constant: closure instance. inline-constexpr on C++17+ for
    // proper external-linkage; static-constexpr on C++11/14 for
    // ODR-safe header inclusion (multiple TUs each get an internal
    // instance, equivalent since all_fn is stateless).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline D_CONSTEXPR all_fn all = all_fn();
#else
    static D_CONSTEXPR all_fn all = all_fn();
#endif
}  // namespace views


// ===========================================================================
// II.  ALL_T (alias)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

namespace views
{
    // views::all_t<_R>
    //   alias: the result type of views::all(_r) where _r has type
    // _R&&. Useful for declaring view-typed members or function
    // return types without forcing a particular wrapper category.
    template<typename _R>
    using all_t = typename internal::all_dispatch<_R>::type;
}  // namespace views

#endif  // alias templates


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_ALL_
