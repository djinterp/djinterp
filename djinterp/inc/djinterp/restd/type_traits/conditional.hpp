/******************************************************************************
* djinterp [restd]                                             conditional.hpp
*
* conditional trait header:
*   Compile-time type selector. Yields member typedef `type` as
* `_IfTrue` when `_Condition` is true, otherwise `_IfFalse`.
*
*   USAGE:
*     typename conditional<sizeof(int) == 4, int, long>::type four_byte;
*
*
* path:      /inc/djinterp/restd/type_traits/conditional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_CONDITIONAL_
#define DJINTERP_RESTD_TYPE_TRAITS_CONDITIONAL_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   CONDITIONAL
// =============================================================================

// conditional
//   trait: yields _IfTrue when _Condition is true (primary template).
template<bool      _Condition,
         typename  _IfTrue,
         typename  _IfFalse>
struct conditional
{
    typedef _IfTrue type;
};

// conditional<false, ...>
//   trait: specialization yielding _IfFalse when _Condition is false.
template<typename _IfTrue,
         typename _IfFalse>
struct conditional<false, _IfTrue, _IfFalse>
{
    typedef _IfFalse type;
};


// =============================================================================
// II.  CONDITIONAL_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // conditional_t
    //   alias: convenience alias for
    // conditional<_Condition, _IfTrue, _IfFalse>::type.
    template<bool      _Condition,
             typename  _IfTrue,
             typename  _IfFalse>
    using conditional_t =
        typename conditional<_Condition, _IfTrue, _IfFalse>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_CONDITIONAL_
