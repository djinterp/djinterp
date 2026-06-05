/******************************************************************************
* djinterp [restd]                                                new_handler.hpp
*
* new_handler facility header:
*   The new-handler is the function called by operator new when
* allocation fails (before it throws bad_alloc). User code installs
* a handler via set_new_handler; the handler typically tries to
* free memory, retries the allocation, or terminates.
*
*   STRATEGY:
*     restd::new_handler        - typedef for std::new_handler.
*     restd::set_new_handler    - pass-through to std::set_new_handler;
*                                 C++98+.
*     restd::get_new_handler    - C++11+ in std. Back-ported to C++98
*                                 via a wrapper that remembers the
*                                 last value passed to set_new_handler.
*
*   PORTABILITY NOTE FOR C++98 BACK-PORT:
*   The C++98 back-port of get_new_handler is not thread-safe — it
* uses a single global to remember the last set value. Pre-C++11 std
* itself isn't thread-aware so this matches the era's idioms; on
* C++11+ the std::get_new_handler is properly atomic and we defer
* to it.
*
*
* path:      /inc/djinterp/restd/new/new_handler.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_NEW_HANDLER_
#define DJINTERP_RESTD_NEW_HANDLER_ 1

#include "../../core/djinterp.hpp"
#include <new>


NS_RESTD


// ===========================================================================
// I.   NEW_HANDLER TYPEDEF
// ===========================================================================

// new_handler
//   typedef: pointer to a no-argument, no-return function.
typedef std::new_handler new_handler;


// ===========================================================================
// II.  C++98 STORAGE HELPER (back-port for get_new_handler)
// ===========================================================================

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    // Single global that mirrors the value last passed to set_new_handler.
    // Function-local static = zero-init at first use, no order-of-init
    // issues. NOT thread-safe — pre-C++11 std isn't thread-aware so this
    // matches the era's overall idioms.
    inline new_handler&
    _last_set_handler()
    {
        static new_handler _h = D_NULLPTR;
        return _h;
    }

NS_END  // internal

#endif


// ===========================================================================
// III. SET_NEW_HANDLER
// ===========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// C++11+: std::set_new_handler is noexcept. Pass through directly.
inline new_handler
set_new_handler(
    new_handler _h
) D_NOEXCEPT
{
    return std::set_new_handler(_h);
}

#else

// C++98: std::set_new_handler is throw(). Wrap to also update our
// storage shadow so get_new_handler can return the current value.
inline new_handler
set_new_handler(
    new_handler _h
) throw()
{
    new_handler _prev = std::set_new_handler(_h);
    internal::_last_set_handler() = _h;
    return _prev;
}

#endif


// ===========================================================================
// IV.  GET_NEW_HANDLER
// ===========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// C++11+: pass through to std.
inline new_handler
get_new_handler() D_NOEXCEPT
{
    return std::get_new_handler();
}

#else

// C++98 back-port: returns the last value passed to restd::set_new_handler.
// Caveat: if user code calls std::set_new_handler directly (instead of
// restd::set_new_handler), the back-port will not see the update.
inline new_handler
get_new_handler() throw()
{
    return internal::_last_set_handler();
}

#endif


NS_END  // restd


#endif  // DJINTERP_RESTD_NEW_HANDLER_
