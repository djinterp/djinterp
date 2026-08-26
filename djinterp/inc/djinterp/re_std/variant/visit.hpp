/******************************************************************************
* djinterp [re_std]                                                     visit.hpp
*
* single-variant visit header:
*   Invokes a visitor with the variant's active alternative as its
* argument. Returns whatever the visitor returns.
*
*     variant<int, std::string> v(42);
*     visit([](auto& x) { std::cout << x; }, v);
*
*   IMPLEMENTATION:
*   Recursive index-dispatch. visit_at<I> tests `v.index() == I`; on
* match, returns `vis(get<I>(v))`; otherwise tail-calls visit_at<I+1>.
* On reaching I == sizeof...(Types), throws bad_variant_access (only
* reachable from a valueless variant — guarded by the public visit's
* up-front check).
*
*   Compared to the buffered-return / placement-new approach (used
* by some implementations), recursive return-by-value:
*     - Works naturally for non-default-constructible return types.
*     - Works for void return (separate overload below — no buffer).
*     - Compiler usually optimises the chain into a jump table at
*       -O2+ for shallow recursion (N typically <= 10).
*     - Imposes no constraint that std doesn't already impose.
*
*   RETURN TYPE:
*   Deduced from the visitor's invocation on the first alternative.
* All alternatives' invocations must return the SAME type (or a
* common one) — matches std. Use visit<R> (deferred) for explicit
* return types when needed.
*
*   NOT IMPLEMENTED (deferred):
*   - Multi-variant visit(vis, v1, v2, ...) — recursive expansion is
*     heavy; deferred to a follow-up phase.
*   - visit<R> (C++20 explicit return type)
*
*
* path:      /inc/djinterp/re_std/variant/visit.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RE_STD_VISIT_
#define DJINTERP_RE_STD_VISIT_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>
#include <utility>          // std::declval

#include "./variant.hpp"
#include "./variant_get.hpp"
#include "./bad_variant_access.hpp"


NS_RESTD


NS_INTERNAL

    // ---- visit_result<Visitor, V> ----
    // Deduce the visitor's return type from its invocation on the
    // FIRST alternative of V. All alternatives must agree.
    template<typename _Visitor, typename _Variant>
    struct visit_result;

    template<typename _Visitor, typename... _Types>
    struct visit_result<_Visitor, variant<_Types...> >
    {
        typedef decltype(std::declval<_Visitor>()(
                            std::declval<
                                typename va_type_at<0, _Types...>::type&
                            >()
                        )) type;
    };


    // ---- visit_at_impl ----
    // Recursive dispatch. Works for non-void AND void return via
    // static_cast<_Ret>(expr) — `static_cast<void>(any-expr)` is
    // well-formed for any expression. So a single template handles
    // both paths.

    template<std::size_t _I, std::size_t _N>
    struct visit_at_impl
    {
        template<typename _Ret, typename _Visitor, typename _Variant>
        static _Ret apply(_Visitor&& _vis, _Variant& _v)
        {
            if (_v.index() == _I)
            {
                return static_cast<_Ret>(
                    static_cast<_Visitor&&>(_vis)(get<_I>(_v)));
            }
            return visit_at_impl<_I + 1, _N>::template apply<_Ret>(
                static_cast<_Visitor&&>(_vis), _v);
        }
    };

    // Terminator: all indices exhausted. Only reachable for a
    // valueless variant — the public visit guards against that,
    // so this branch is dead in well-formed callers. Defensive
    // throw covers the dead path on builds with exceptions.
    template<std::size_t _N>
    struct visit_at_impl<_N, _N>
    {
        template<typename _Ret, typename _Visitor, typename _Variant>
        static _Ret apply(_Visitor&&, _Variant&)
        {
#if D_ENV_CPP98_HAS_EXCEPTION
            throw bad_variant_access();
#else
            // Exceptions disabled: dead path. Return a value-initialised
            // _Ret — UB if Ret isn't default-constructible. Documented
            // limitation for -fno-exceptions builds.
            return _Ret();
#endif
        }
    };

    // void-return terminator specialisation — no return value to
    // construct. Without this, the generic terminator would try to
    // `return _Ret()` which is `return void()` — well-formed in
    // expression context but ill-formed as a return-statement value.
    // Provide an explicit specialisation.
    // (The recursive branch is fine for void: static_cast<void>(...)
    // and `return static_cast<void>(call)` both work because a void
    // expression can appear in a return statement of a void function.)

NS_END  // internal


// ===========================================================================
// I.   VISIT — non-void return path
// ===========================================================================

template<typename _Visitor,
         typename... _Types>
typename internal::visit_result<_Visitor, variant<_Types...> >::type
visit(
    _Visitor&&              _vis,
    variant<_Types...>&     _v
)
{
    typedef typename internal::visit_result<_Visitor, variant<_Types...> >::type _Ret;
    if (_v.valueless_by_exception())
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_variant_access();
#endif
    }
    return internal::visit_at_impl<0, sizeof...(_Types)>::template apply<_Ret>(
        static_cast<_Visitor&&>(_vis), _v);
}

template<typename _Visitor,
         typename... _Types>
typename internal::visit_result<_Visitor, variant<_Types...> >::type
visit(
    _Visitor&&                   _vis,
    variant<_Types...> const&    _v
)
{
    typedef typename internal::visit_result<_Visitor, variant<_Types...> >::type _Ret;
    if (_v.valueless_by_exception())
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_variant_access();
#endif
    }
    return internal::visit_at_impl<0, sizeof...(_Types)>::template apply<_Ret>(
        static_cast<_Visitor&&>(_vis), _v);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_VISIT_
