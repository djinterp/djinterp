/*******************************************************************************
* djinterp [re_std]                                                      all.hpp
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
* path:      /inc/re_std/ranges/all.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_ALL_HPP
#define RE_STD_RANGES_ALL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./view.hpp"
#include "./ref_view.hpp"
#include "./owning_view.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL: ALL_DISPATCH
// ===========================================================================

namespace internal
{

// all_dispatch
//   trait: routes views::all to one of three result types based on
// (a) whether R after ref-stripping is itself a view, and (b)
// whether R is an lvalue reference. The three partial spec'ns
// below cover the three cases.
template<typename R,
         bool IsView      = view<typename remove_reference<R>::type>::value,
         bool IsLvalueRef = is_lvalue_reference<R>::value>
struct all_dispatch;

// case A: R is already a view (lvalue or rvalue). Forward as a
// decay_t<R> — copy from lvalue (requires copyable view), move
// from rvalue.
template<typename R, bool IsLvalueRef>
struct all_dispatch<R, true, IsLvalueRef>
{
    typedef typename decay<R>::type type;

    static RE_STD_CONSTEXPR type
    call(R&& _r)
    {
        return static_cast<R&&>(_r);
    }
};

// case B: R is an lvalue non-view range. Wrap in ref_view.
template<typename R>
struct all_dispatch<R, false, true>
{
    typedef ref_view<typename remove_reference<R>::type> type;

    static RE_STD_CONSTEXPR type
    call(R&& _r)
    {
        return type(_r);
    }
};

// case C: R is an rvalue non-view range. Wrap in owning_view via
// move.
template<typename R>
struct all_dispatch<R, false, false>
{
    typedef owning_view<typename decay<R>::type> type;

    static RE_STD_CONSTEXPR type
    call(R&& _r)
    {
        return type(static_cast<R&&>(_r));
    }
};

}  // internal


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
        template<typename R>
        RE_STD_CONSTEXPR
        typename internal::all_dispatch<R>::type
        operator()(
            R&& _r
        ) const
        {
            return internal::all_dispatch<R>::call(static_cast<R&&>(_r));
        }
    };

    // views::all
    //   constant: closure instance. inline-constexpr on C++17+ for
    // proper external-linkage; static-constexpr on C++11/14 for
    // ODR-safe header inclusion (multiple TUs each get an internal
    // instance, equivalent since all_fn is stateless).
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline RE_STD_CONSTEXPR all_fn all = all_fn();
#else
    static RE_STD_CONSTEXPR all_fn all = all_fn();
#endif
}  // namespace views


// ===========================================================================
// II.  ALL_T (alias)
// ===========================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

namespace views
{
    // views::all_t<R>
    //   alias: the result type of views::all(_r) where _r has type
    // R&&. Useful for declaring view-typed members or function
    // return types without forcing a particular wrapper category.
    template<typename R>
    using all_t = typename internal::all_dispatch<R>::type;
}  // namespace views

#endif  // alias templates


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_ALL_HPP
