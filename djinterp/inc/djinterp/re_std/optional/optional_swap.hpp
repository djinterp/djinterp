/******************************************************************************
* djinterp [restd]                                              optional_swap.hpp
*
* Non-member swap for optional:
*   Free-function overload that delegates to optional<T>::swap(other).
* Allows ADL-driven swap calls (the "using std::swap; swap(a, b);" idiom)
* to find the correct optional swap.
*
*   STANDARD STATUS:
*   Introduced in C++17 alongside std::optional. Constrained in the
* standard to participate in overload resolution iff T is move-
* constructible AND swappable; we omit the constraint here since
* optional<T>'s member swap will simply fail to compile if those
* conditions are not met -- failing inside the function body rather
* than via SFINAE gives a clearer diagnostic.
*
*   PORTABILITY:
*   Available on C++11 and later.
*
*
* path:      /inc/djinterp/restd/optional/optional_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_OPTIONAL_OPTIONAL_SWAP_
#define DJINTERP_RESTD_OPTIONAL_OPTIONAL_SWAP_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if    D_ENV_LANG_IS_CPP11_OR_HIGHER \
    && D_ENV_CPP98_HAS_NEW

// restd
#include "./optional.hpp"


NS_RESTD


    // swap(optional<T>&, optional<T>&)
    //   function: free-function ADL hook. Delegates to the member
    //             swap, which carries the noexcept specification
    //             (so this wrapper inherits it via noexcept(noexcept(...))).
    template<typename _T>
    void swap(optional<_T>& a, optional<_T>& b)
        noexcept(noexcept( a.swap(b) ))
    {
        a.swap(b);
    }


NS_END  // restd


#endif  // CPP11+ && HAS_NEW

#endif  // DJINTERP_RESTD_OPTIONAL_OPTIONAL_SWAP_
