/*******************************************************************************
* djinterp [re_std]                                              ranges_swap.hpp
*
* ranges_swap swap specialization header:
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
*   COSTS NOTHING BELOW C++20.
*   Every dependency include sits INSIDE the language gate, so on a pre-C++20
* compiler this header pulls in config.hpp to read the tier and then expands
* to nothing at all - no transitive includes, no parse cost, and no way for a
* dependency that is not C++98-clean to break a translation unit that never
* wanted concepts in the first place.
*
*   NOTE - THESE ARE NOT IN re_std::concepts.
*   std puts same_as, integral and the rest directly in std, so re_std puts
* them directly in re_std - mirroring std is the rule.
*
*
* path:      /inc/re_std/concepts/ranges_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_RANGES_SWAP_HPP
#define RE_STD_CONCEPTS_RANGES_SWAP_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "./move_constructible.hpp"
#include "./assignable_from.hpp"

namespace re_std
{

namespace ranges
{

namespace internal
{
namespace swap_cpo
{

    // swap
    //   function: the poison pill.  Deleted, and deliberately visible to the
    // unqualified call below so that only an ADL-found swap can win.
    template<typename Type>
    void swap(Type&, Type&) = delete;

    // adl_swappable
    //   concept: an ADL-found swap exists for these operands.  The
    // class-or-enum guard reflects that ADL only has anywhere to look for
    // those; without it, unqualified lookup on a scalar would reach the
    // poison pill and hard-error instead of falling through.
    template<typename TypeA, typename TypeB>
    concept adl_swappable
        =  (   is_class<typename remove_reference<TypeA>::type>::value
            || is_enum<typename remove_reference<TypeA>::type>::value
            || is_class<typename remove_reference<TypeB>::type>::value
            || is_enum<typename remove_reference<TypeB>::type>::value)
        && requires(TypeA&& a, TypeB&& b)
           {
               swap(static_cast<TypeA&&>(a), static_cast<TypeB&&>(b));
           };

    // fn
    //   struct: the callable behind the ranges::swap object.
    struct fn
    {
        // (1) ADL-found swap
        template<typename TypeA, typename TypeB>
            requires adl_swappable<TypeA, TypeB>
        RE_STD_CONSTEXPR void operator()(TypeA&& a, TypeB&& b) const
            RE_STD_NOEXCEPT_IF(noexcept(swap(static_cast<TypeA&&>(a),
                                        static_cast<TypeB&&>(b))))
        {
            swap(static_cast<TypeA&&>(a), static_cast<TypeB&&>(b));
            return;
        }

        // (2) two arrays of equal extent — swap element-wise, recursing so
        // that arrays of arrays work and so each element re-enters the CPO
        //   fn is incomplete here, so the constraint and the noexcept
        // operand name it only through a reference, as std's own ranges::swap
        // does; constructing fn() would need a complete type.
        template<typename TypeA, typename TypeB, size_t Size>
            requires (!adl_swappable<TypeA (&)[Size], TypeB (&)[Size]>)
                  && requires(const fn& self, TypeA& a, TypeB& b)
                     {
                         self(a, b);
                     }
        RE_STD_CONSTEXPR void operator()(TypeA (&a)[Size],
                                    TypeB (&b)[Size]) const
            RE_STD_NOEXCEPT_IF(noexcept(declval<const fn&>()(*a, *b)))
        {
            for (size_t i = 0; i < Size; ++i)
            {
                fn()(a[i], b[i]);
            }
            return;
        }

        // (3) the three-move exchange fallback
        template<typename Type>
            requires (!adl_swappable<Type&, Type&>)
                  && move_constructible<Type>
                  && assignable_from<Type&, Type>
        RE_STD_CONSTEXPR void operator()(Type& a, Type& b) const
            RE_STD_NOEXCEPT_IF(   is_nothrow_move_constructible<Type>::value
                          && is_nothrow_move_assignable<Type>::value)
        {
            Type tmp = static_cast<Type&&>(a);
            a         = static_cast<Type&&>(b);
            b         = static_cast<Type&&>(tmp);
            return;
        }
    };

}  // swap_cpo
}  // internal

    // swap
    //   variable: the customisation point.  Declared in an inline namespace in
    // std so that a user cannot introduce a conflicting `swap` at namespace
    // scope; re_std keeps it a plain inline constexpr object, which has the
    // same practical effect here because the name is never re-opened.
    RE_STD_INLINE_VAR RE_STD_CONSTEXPR internal::swap_cpo::fn swap = {};

}  // ranges

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_RANGES_SWAP_HPP
