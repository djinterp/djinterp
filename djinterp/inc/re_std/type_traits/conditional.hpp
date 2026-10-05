/*******************************************************************************
* djinterp [re_std]                                              conditional.hpp
*
* conditional trait header:
*   Compile-time type selector. Yields member typedef `type` as
* `IfTrue` when `Condition` is true, otherwise `IfFalse`.
*
*   USAGE:
*     typename conditional<sizeof(int) == 4, int, long>::type four_byte;
*
*
* path:      /inc/re_std/type_traits/conditional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_CONDITIONAL_HPP
#define RE_STD_TYPE_TRAITS_CONDITIONAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   CONDITIONAL
// =============================================================================

// conditional
//   trait: yields IfTrue when Condition is true (primary template).
template<bool      Condition,
         typename  IfTrue,
         typename  IfFalse>
struct conditional
{
    typedef IfTrue type;
};

// conditional<false, ...>
//   trait: specialization yielding IfFalse when Condition is false.
template<typename IfTrue,
         typename IfFalse>
struct conditional<false, IfTrue, IfFalse>
{
    typedef IfFalse type;
};


// =============================================================================
// II.  CONDITIONAL_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // conditional_t
    //   alias: convenience alias for
    // conditional<Condition, IfTrue, IfFalse>::type.
    template<bool      Condition,
             typename  IfTrue,
             typename  IfFalse>
    using conditional_t =
        typename conditional<Condition, IfTrue, IfFalse>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_CONDITIONAL_HPP
