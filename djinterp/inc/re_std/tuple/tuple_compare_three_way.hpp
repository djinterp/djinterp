/*******************************************************************************
* djinterp [re_std]                                  tuple_compare_three_way.hpp
*
* tuple operator<=> header:
*   Adds the C++20 three-way comparison operator to re_std::tuple.
* Returns the common_comparison_category over all element-wise <=>
* results; lexicographic semantics matching the existing classic
* operators in tuple_compare.hpp.
*
*     tuple<int, int>(1, 2) <=> tuple<int, int>(1, 3)
*       -> strong_ordering::less
*     tuple<int, double>(1, 1.0) <=> tuple<int, double>(1, 1.0)
*       -> partial_ordering::equivalent
*       (common category clamps to partial because of the double)
*
*   ARITY MISMATCH: the constraint sizeof...(A) == sizeof...(B)
* on the template restricts the overload to matching arities;
* a different-arity comparison won't find this overload and will
* fail to compile (clearer error than a template-instantiation
* failure mid-pack-expansion).
*
*   IMPLEMENTATION:
*   Recursive helper internal::tuple_3way_impl walks indices 0..N-1
* via `if constexpr`. At each index, computes get<I>(t) <=> get<I>(u)
* cast to the precomputed common result type; returns the first
* non-equal-to-0 result, or the identity (strong_ordering::equal
* cast to the result type) after the last index.
*
*   The synth-three-way machinery from the std spec (which falls
* back to weak_ordering from `<` and `==` when `<=>` is ill-formed)
* is NOT implemented in this back-port — element types without
* operator<=> will cause a compile error at the decltype site.
* Users needing the synth-fallback can apply compare_weak_order_fallback
* element-wise via a custom routine.
*
*   PORTABILITY:
*   Entire file gated on RE_STD_LANG_IS_CPP20_OR_HIGHER. On C++11-17
* the classic six comparison operators from tuple_compare.hpp
* remain the only comparison surface.
*
*
* path:      /inc/re_std/tuple/tuple_compare_three_way.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_COMPARE_THREE_WAY_HPP
#define RE_STD_TUPLE_TUPLE_COMPARE_THREE_WAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP20_OR_HIGHER


// std
#include <cstddef>
// re_std
#include "./tuple.hpp"
#include "./tuple_get.hpp"
#include "../compare/strong_ordering.hpp"
#include "../compare/common_comparison_category.hpp"
#include "../utility/declval.hpp"


namespace re_std
{


// =============================================================================
// I.   INTERNAL: RECURSIVE THREE-WAY HELPER
// =============================================================================

namespace internal
{

    // tuple_3way_impl
    //   function: recursive lexicographic three-way comparison.
    // Walks indices 0..N-1. At each index, computes the per-element
    // <=> (cast to the precomputed common result ResultT). Returns
    // the first non-equal result, else returns the identity after
    // the last index.
    //
    //   Note: the identity is always equivalent to strong_ordering::
    // equal cast to ResultT. For empty tuples (N == 0) the
    // function returns identity immediately.
    template<std::size_t I,
             std::size_t N,
             typename    ResultT,
             typename    TT,
             typename    UU>
    constexpr ResultT
    tuple_3way_impl(
        const TT&    _t,
        const UU&    _u,
        ResultT      _identity
    )
    {
        if constexpr (I == N)
        {
            return _identity;
        }
        else
        {
            ResultT _c = static_cast<ResultT>(get<I>(_t) <=> get<I>(_u));
            if (_c != 0)
            {
                return _c;
            }
            return tuple_3way_impl<I + 1, N, ResultT>(_t, _u, _identity);
        }
    }

}  // internal


// =============================================================================
// II.  OPERATOR<=>
// =============================================================================

// operator<=>
//   function: lexicographic three-way comparison of two equal-arity
// tuples. The return type is common_comparison_category over all
// element-wise <=> results. Requires-clause restricts the overload
// to matching arities so different-arity comparisons fail with a
// clear "no matching operator" error.
template<typename... A,
         typename... B>
    requires (sizeof...(A) == sizeof...(B))
constexpr common_comparison_category_t<
              decltype(re_std::declval<const A&>() <=> re_std::declval<const B&>())...>
operator<=>(
    const tuple<A...>& _lhs,
    const tuple<B...>& _rhs
)
{
    typedef common_comparison_category_t<
                decltype(re_std::declval<const A&>() <=> re_std::declval<const B&>())...
            > _R;
    return internal::tuple_3way_impl<0, sizeof...(A), _R>(
        _lhs,
        _rhs,
        static_cast<_R>(strong_ordering::equal));
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


#endif  // RE_STD_TUPLE_TUPLE_COMPARE_THREE_WAY_HPP
