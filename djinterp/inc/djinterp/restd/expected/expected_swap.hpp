/******************************************************************************
* djinterp [restd]                                             expected_swap.hpp
*
* expected swap specialization header:
*   Provides non-member swap overloads for restd::expected and
* restd::unexpected. ADL-friendly; delegate to the member swap on
* each.
*
*   CONSTEXPR:
*   constexpr from C++20 (matches std).
*
*
* path:      /inc/djinterp/restd/expected/expected_swap.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_EXPECTED_SWAP_
#define DJINTERP_RESTD_EXPECTED_SWAP_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "./expected.hpp"
#include "./unexpected.hpp"


#ifndef D_CONSTEXPR_CPP20
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_CONSTEXPR_CPP20   constexpr
    #else
        #define D_CONSTEXPR_CPP20
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   swap (expected)
// ===========================================================================

// swap (expected<T, E>)
template<typename _T,
         typename _E>
D_CONSTEXPR_CPP20 void
swap(
    expected<_T, _E>& _lhs,
    expected<_T, _E>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}

// swap (expected<void, E>)
template<typename _E>
D_CONSTEXPR_CPP20 void
swap(
    expected<void, _E>& _lhs,
    expected<void, _E>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}


// ===========================================================================
// II.  swap (unexpected)
// ===========================================================================

// swap (unexpected<E>)
template<typename _E>
D_CONSTEXPR_CPP20 void
swap(
    unexpected<_E>& _lhs,
    unexpected<_E>& _rhs
) D_NOEXCEPT
{
    _lhs.swap(_rhs);

    return;
}


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_EXPECTED_SWAP_
