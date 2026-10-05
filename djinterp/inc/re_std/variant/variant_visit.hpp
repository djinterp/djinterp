/*******************************************************************************
* djinterp [re_std]                                            variant_visit.hpp
*
* variant_visit support header:
*   visit<R>(vis, v...) and MULTI-VARIANT visit(vis, v1, v2, ...).
*
*   Single-variant visit already shipped; this is the general case and the
* explicit-return-type form.
*
*   HOW N-ARY DISPATCH IS DONE HERE, and why not the obvious way.
*   The textbook implementation builds an N-dimensional table of function
* pointers indexed by (i1, ..., iN).  That is O(product of sizes) function
* pointers materialised whether or not they are ever called, and for four
* four-alternative variants it is 256 instantiations before anything runs.
*
*   This peels ONE variant at a time instead.  Dispatching on v1's index picks
* an alternative, binds it into a small functor, and recurses on the remaining
* variants with that functor as the visitor.  The instantiation count is the
* same in the worst case, but the recursion is linear in the number of
* variants rather than exponential in the source text, and it needs no
* index_sequence gymnastics - which is what lets the whole thing stay at C++11
* rather than needing C++14 generic lambdas.
*
*   INDEX DISPATCH IS A LINEAR IF-CHAIN over the alternatives, not a jump
* table.  Compilers turn a dense chain of `if (idx == I)` on a constant range
* into a switch, so the generated code is the same; writing it as recursion
* keeps it expressible without a macro or a generated switch of fixed arity.
*
*   VALUELESS VARIANTS THROW bad_variant_access, checked BEFORE any dispatch.
* Doing it first matters: the index of a valueless variant is variant_npos,
* which would fall off the end of the if-chain and reach the unreachable base
* case.
*
*   STD IS C++17; re_std IS C++11 - a six-year back-port.
*
*
* path:      /inc/re_std/variant/variant_visit.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_VISIT_HPP
#define RE_STD_VARIANT_VARIANT_VISIT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../functional/invoke.hpp"
#include "./variant.hpp"
#include "./variant_get.hpp"
#include "./variant_size.hpp"
#include "./bad_variant_access.hpp"

namespace re_std
{

namespace internal
{

    // visit_bound
    //   struct: a visitor with its leading argument already chosen.  Holds
    // references only - it never outlives the dispatch that made it.
    template<typename Visitor, typename First>
    struct visit_bound
    {
        Visitor& m_visitor;
        First&   m_first;

        template<typename... Rest>
        auto operator()(Rest&&... rest) const
            -> decltype(re_std::invoke(m_visitor, m_first,
                                       static_cast<Rest&&>(rest)...))
        {
            return re_std::invoke(m_visitor, m_first,
                                  static_cast<Rest&&>(rest)...);
        }
    };

    // visit_multi
    //   function: forward declaration - the recursion below needs it.
    template<typename Result, typename Visitor, typename... Variants>
    Result visit_multi(Visitor&& visitor, Variants&&... variants);

    // visit_dispatch
    //   struct: linear index dispatch over one variant's alternatives.
    // Index counts up; the specialisation at variant_size is the
    // unreachable base.
    template<size_t Index, size_t Size>
    struct visit_dispatch
    {
        template<typename Result, typename Visitor,
                 typename Variant, typename... Rest>
        static Result apply(Visitor&& visitor, Variant&& variant,
                             Rest&&... rest)
        {
            if (variant.index() == Index)
            {
                //   Bind this alternative and recurse on what is left.
                visit_bound<
                    typename remove_reference<Visitor>::type,
                    typename remove_reference<
                        decltype(re_std::get<Index>(variant))>::type>
                    bound = { visitor, re_std::get<Index>(variant) };

                return visit_multi<Result>(bound,
                                            static_cast<Rest&&>(rest)...);
            }
            return visit_dispatch<Index + 1, Size>::template apply<Result>(
                static_cast<Visitor&&>(visitor),
                static_cast<Variant&&>(variant),
                static_cast<Rest&&>(rest)...);
        }
    };

    // visit_dispatch<Size, Size>
    //   struct: unreachable base.  Only a valueless variant could get here,
    // and that is rejected before dispatch begins - so this throwing arm
    // exists to keep the function well-formed on every path, not because it
    // is expected to run.
    template<size_t Size>
    struct visit_dispatch<Size, Size>
    {
        template<typename Result, typename Visitor,
                 typename Variant, typename... Rest>
        static Result apply(Visitor&&, Variant&&, Rest&&...)
        {
            internal::throw_bad_variant_access();
            return visit_dispatch_unreachable<Result>();
        }

        template<typename Result>
        static Result visit_dispatch_unreachable()
        {
            //   Never executed; throw_bad_variant_access does not return.
            internal::throw_bad_variant_access();
        }
    };

    // visit_multi
    //   function: base case - no variants left, so invoke.
    template<typename Result, typename Visitor>
    Result visit_multi(Visitor&& visitor)
    {
        return static_cast<Result>(re_std::invoke(visitor));
    }

    //   Recursive case: dispatch on the first variant, recurse on the rest.
    template<typename Result, typename Visitor,
             typename Variant, typename... Rest>
    Result visit_multi(Visitor&& visitor, Variant&& variant,
                        Rest&&... rest)
    {
        typedef typename remove_reference<Variant>::type _Bare;
        return visit_dispatch<0, variant_size<_Bare>::value>::
            template apply<Result>(static_cast<Visitor&&>(visitor),
                                    static_cast<Variant&&>(variant),
                                    static_cast<Rest&&>(rest)...);
    }

    // any_valueless
    //   function: fold over the pack; checked before dispatch, see the
    // header note on why the order matters.
    inline bool any_valueless() { return false; }

    template<typename Variant, typename... Rest>
    bool any_valueless(const Variant& variant, const Rest&... rest)
    {
        return variant.valueless_by_exception() || any_valueless(rest...);
    }

}  // internal


// visit
//   function: multi-variant visit, deducing the return type from the
// first alternative of each variant.
template<typename Visitor, typename... Variants>
auto visit(Visitor&& visitor, Variants&&... variants)
    -> decltype(re_std::invoke(
           visitor,
           re_std::get<0>(
               static_cast<typename remove_reference<Variants>::type&>(
                   variants))...))
{
    typedef decltype(re_std::invoke(
        visitor,
        re_std::get<0>(
            static_cast<typename remove_reference<Variants>::type&>(
                variants))...)) Result;

    if (internal::any_valueless(variants...))
    {
        internal::throw_bad_variant_access();
    }
    return internal::visit_multi<Result>(visitor,
                                          static_cast<Variants&&>(variants)...);
}

// visit<R>
//   function: as above with the result type fixed, so that alternatives whose
// natural results differ but share a common target type still compose.
template<typename Result, typename Visitor, typename... Variants>
Result visit(Visitor&& visitor, Variants&&... variants)
{
    if (internal::any_valueless(variants...))
    {
        internal::throw_bad_variant_access();
    }
    return internal::visit_multi<Result>(visitor,
                                          static_cast<Variants&&>(variants)...);
}

}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_VARIANT_VARIANT_VISIT_HPP
