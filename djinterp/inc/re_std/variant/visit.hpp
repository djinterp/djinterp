/*******************************************************************************
* djinterp [re_std]                                                    visit.hpp
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
* path:      /inc/re_std/variant/visit.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_VARIANT_VISIT_HPP
#define RE_STD_VARIANT_VISIT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <utility>          // std::declval

#include "./variant.hpp"
#include "./variant_get.hpp"
#include "./bad_variant_access.hpp"


namespace re_std
{


namespace internal
{

    // ---- visit_result<Visitor, V> ----
    // Deduce the visitor's return type from its invocation on the
    // FIRST alternative of V. All alternatives must agree.
    template<typename Visitor, typename Variant>
    struct visit_result;

    template<typename Visitor, typename... Types>
    struct visit_result<Visitor, variant<Types...> >
    {
        typedef decltype(std::declval<Visitor>()(
                            std::declval<
                                typename va_type_at<0, Types...>::type&
                            >()
                        )) type;
    };


    // ---- visit_at_impl ----
    // Recursive dispatch. Works for non-void AND void return via
    // static_cast<Ret>(expr) — `static_cast<void>(any-expr)` is
    // well-formed for any expression. So a single template handles
    // both paths.

    template<std::size_t I, std::size_t N>
    struct visit_at_impl
    {
        template<typename Ret, typename Visitor, typename Variant>
        static Ret apply(Visitor&& _vis, Variant& _v)
        {
            if (_v.index() == I)
            {
                return static_cast<Ret>(
                    static_cast<Visitor&&>(_vis)(get<I>(_v)));
            }
            return visit_at_impl<I + 1, N>::template apply<Ret>(
                static_cast<Visitor&&>(_vis), _v);
        }
    };

    // Terminator: all indices exhausted. Only reachable for a
    // valueless variant — the public visit guards against that,
    // so this branch is dead in well-formed callers. Defensive
    // throw covers the dead path on builds with exceptions.
    template<std::size_t N>
    struct visit_at_impl<N, N>
    {
        template<typename Ret, typename Visitor, typename Variant>
        static Ret apply(Visitor&&, Variant&)
        {
#if RE_STD_HAS_EXCEPTIONS
            throw bad_variant_access();
#else
            // Exceptions disabled: dead path. Return a value-initialised
            // Ret — UB if Ret isn't default-constructible. Documented
            // limitation for -fno-exceptions builds.
            return Ret();
#endif
        }
    };

    // void-return terminator specialisation — no return value to
    // construct. Without this, the generic terminator would try to
    // `return Ret()` which is `return void()` — well-formed in
    // expression context but ill-formed as a return-statement value.
    // Provide an explicit specialisation.
    // (The recursive branch is fine for void: static_cast<void>(...)
    // and `return static_cast<void>(call)` both work because a void
    // expression can appear in a return statement of a void function.)

}  // internal


// ===========================================================================
// I.   VISIT — non-void return path
// ===========================================================================

template<typename Visitor,
         typename... Types>
typename internal::visit_result<Visitor, variant<Types...> >::type
visit(
    Visitor&&              _vis,
    variant<Types...>&     _v
)
{
    typedef typename internal::visit_result<Visitor, variant<Types...> >::type Ret;
    if (_v.valueless_by_exception())
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_variant_access();
#endif
    }
    return internal::visit_at_impl<0, sizeof...(Types)>::template apply<Ret>(
        static_cast<Visitor&&>(_vis), _v);
}

template<typename Visitor,
         typename... Types>
typename internal::visit_result<Visitor, variant<Types...> >::type
visit(
    Visitor&&                   _vis,
    variant<Types...> const&    _v
)
{
    typedef typename internal::visit_result<Visitor, variant<Types...> >::type Ret;
    if (_v.valueless_by_exception())
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_variant_access();
#endif
    }
    return internal::visit_at_impl<0, sizeof...(Types)>::template apply<Ret>(
        static_cast<Visitor&&>(_vis), _v);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VISIT_HPP
