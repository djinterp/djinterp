/******************************************************************************
* re_std [concepts]                                             ranges_swap.hpp
*
*   the re_std::ranges::swap customisation point object.
*
*   Despite living in namespace ranges, this CPO is specified in <concepts>
* ([concepts.swappable]), not <ranges> - so it ships here, with the swappable
* and swappable_with concepts that are defined in terms of it.
*
*   WHY A CPO AND NOT A FUNCTION.
*   Plain `using re_std::swap; swap(a, b);` is the old two-step dance, and it
* forces every caller to remember the using-declaration.  A CPO is an OBJECT,
* so the name is found without ADL at the call site while its implementation
* still performs ADL internally.  `ranges::swap(a, b)` is therefore correct in
* any context, including inside a requires-expression, which is exactly what
* the swappable concepts need.
*
*   THE POISON PILL.
*   The `void swap(_T&, _T&) = delete;` declaration below is not dead code and
* not a mistake.  It is deliberately visible to the ADL call inside this
* namespace so that unqualified `swap(a, b)` does NOT silently find
* re_std::swap (or std::swap) by ordinary unqualified lookup and report every
* type as ADL-swappable.  With the pill in scope, only a swap found by ARGUMENT
* DEPENDENT lookup - a hidden friend, or a free function in the type's own
* namespace - beats it.  Everything else falls through to the exchange
* fallback, which is the intended behaviour.
*
*   RESOLUTION ORDER, highest first:
*     1. an ADL-found swap for the two operand types
*     2. element-wise swap for two arrays of equal extent
*     3. the three-move exchange, for identical lvalue types that are
*        move_constructible and assignable_from
*   If none applies the call is ill-formed, which is what makes swappable<T>
* correctly report false rather than failing later inside a body.
*
*   NOTE - NO NS_RANGES MACRO EXISTS.
*   djinterp.hpp defines NS_INTERNAL, NS_CONCEPTS and friends but not
* NS_RANGES, so this file opens the namespace with the D_NAMESPACE primitive.
* Adding NS_RANGES alongside the others would be the consistent fix.
*
*   COSTS NOTHING BELOW C++20.
*   Every dependency include sits INSIDE the language gate, so on a pre-C++20
* compiler this header pulls in djinterp.hpp to read the tier and then expands
* to nothing at all - no transitive includes, no parse cost, and no way for a
* dependency that is not C++98-clean to break a translation unit that never
* wanted concepts in the first place.
*
*   ALSO NOTE - THESE ARE NOT IN re_std::concepts.
*   djinterp has an NS_CONCEPTS macro, but it is for djinterp's own concept
* layers over its trait surfaces.  std puts same_as, integral and the rest
* directly in std, so re_std puts them directly in re_std - mirroring std is
* the rule, and NS_CONCEPTS is deliberately not used by this module.
*
*
* path:      /inc/djinterp/re_std/concepts/ranges_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_RANGES_SWAP_
#define DJINTERP_RE_STD_CONCEPTS_RANGES_SWAP_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "./move_constructible.hpp"
#include "./assignable_from.hpp"

NS_RESTD

D_NAMESPACE(ranges)      // no NS_RANGES macro in djinterp.hpp — see header note

NS_INTERNAL
D_NAMESPACE(swap_cpo)

    // swap
    //   function: the poison pill.  Deleted, and deliberately visible to the
    // unqualified call below so that only an ADL-found swap can win.
    template<typename _Type>
    void swap(_Type&, _Type&) = delete;

    // adl_swappable
    //   concept: an ADL-found swap exists for these operands.  The
    // class-or-enum guard reflects that ADL only has anywhere to look for
    // those; without it, unqualified lookup on a scalar would reach the
    // poison pill and hard-error instead of falling through.
    template<typename _TypeA, typename _TypeB>
    concept adl_swappable
        =  (   is_class<typename remove_reference<_TypeA>::type>::value
            || is_enum<typename remove_reference<_TypeA>::type>::value
            || is_class<typename remove_reference<_TypeB>::type>::value
            || is_enum<typename remove_reference<_TypeB>::type>::value)
        && requires(_TypeA&& a, _TypeB&& b)
           {
               swap(static_cast<_TypeA&&>(a), static_cast<_TypeB&&>(b));
           };

    // fn
    //   struct: the callable behind the ranges::swap object.
    struct fn
    {
        // (1) ADL-found swap
        template<typename _TypeA, typename _TypeB>
            requires adl_swappable<_TypeA, _TypeB>
        D_CONSTEXPR void operator()(_TypeA&& a, _TypeB&& b) const
            D_NOEXCEPT_IF(noexcept(swap(static_cast<_TypeA&&>(a),
                                        static_cast<_TypeB&&>(b))))
        {
            swap(static_cast<_TypeA&&>(a), static_cast<_TypeB&&>(b));
            return;
        }

        // (2) two arrays of equal extent — swap element-wise, recursing so
        // that arrays of arrays work and so each element re-enters the CPO
        template<typename _TypeA, typename _TypeB, size_t _Size>
            requires (!adl_swappable<_TypeA (&)[_Size], _TypeB (&)[_Size]>)
                  && requires(_TypeA& a, _TypeB& b) { fn()(a, b); }
        D_CONSTEXPR void operator()(_TypeA (&a)[_Size],
                                    _TypeB (&b)[_Size]) const
            D_NOEXCEPT_IF(noexcept(fn()(*a, *b)))
        {
            for (size_t i = 0; i < _Size; ++i)
            {
                fn()(a[i], b[i]);
            }
            return;
        }

        // (3) the three-move exchange fallback
        template<typename _Type>
            requires (!adl_swappable<_Type&, _Type&>)
                  && move_constructible<_Type>
                  && assignable_from<_Type&, _Type>
        D_CONSTEXPR void operator()(_Type& a, _Type& b) const
            D_NOEXCEPT_IF(   is_nothrow_move_constructible<_Type>::value
                          && is_nothrow_move_assignable<_Type>::value)
        {
            _Type tmp = static_cast<_Type&&>(a);
            a         = static_cast<_Type&&>(b);
            b         = static_cast<_Type&&>(tmp);
            return;
        }
    };

NS_END  // swap_cpo
NS_END  // internal

    // swap
    //   object: the customisation point.  Declared in an inline namespace in
    // std so that a user cannot introduce a conflicting `swap` at namespace
    // scope; re_std keeps it a plain inline constexpr object, which has the
    // same practical effect here because the name is never re-opened.
    D_INLINE_VAR D_CONSTEXPR internal::swap_cpo::fn swap = {};

NS_END  // ranges

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_RANGES_SWAP_
