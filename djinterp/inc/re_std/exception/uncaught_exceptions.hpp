/*******************************************************************************
* djinterp [re_std]                                      uncaught_exceptions.hpp
*
* in-flight-exception queries:
*   uncaught_exception() (singular, C++98; deprecated C++17; removed
* C++20) and uncaught_exceptions() (plural, C++17). The plural form is
* the useful one — it returns the *count* of in-flight exceptions, which
* lets a destructor tell "unwinding because of MY throw" apart from
* "unwinding past me". re_std:
*   - re-exports the std symbols where std has them;
*   - back-ports uncaught_exceptions() below C++17, but only to the
*     *boolean precision* that is portably linkable everywhere:
*     (uncaught_exception() ? 1 : 0). The exact in-flight COUNT is not
*     recovered pre-C++17 because the underlying counter is exposed
*     inconsistently across runtimes (libc++abi ships the extern "C"
*     __cxa_uncaught_exceptions; libstdc++ ships only the C++-mangled
*     std::uncaught_exceptions, gated behind C++17 headers), and re_std
*     will not emit a reference that may fail to link. Correct for the
*     common "is any exception in flight?" use; lossy for nested depth.
*   - retains uncaught_exception() past its C++20 removal as a thin shim
*     over uncaught_exceptions() > 0, honouring re_std's backwards-
*     compatibility goal. At C++17 the same shim is used so that re_std
*     never routes through the deprecated std::uncaught_exception (which
*     would surface -Wdeprecated-declarations at the call site).
*
*
* path:      /inc/re_std/exception/uncaught_exceptions.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXCEPTION_UNCAUGHT_EXCEPTIONS_HPP
#define RE_STD_EXCEPTION_UNCAUGHT_EXCEPTIONS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <exception>

namespace re_std
{

    // ---- uncaught_exceptions (plural) -----------------------------------
    #if RE_STD_LANG_IS_CPP17_OR_HIGHER

        // uncaught_exceptions
        //   function: using-declaration from std::uncaught_exceptions.
        using std::uncaught_exceptions;

    #else // pre-C++17: boolean-precision back-port (always linkable)

        // uncaught_exceptions
        //   function: degraded back-port — collapses the count to 0/1.
        //   Correct for "is any exception in flight?"; cannot recover
        //   nested unwinding depth pre-C++17. RE_STD AHEAD OF STD: the
        //   spelling is surfaced before std's C++17.
        inline int uncaught_exceptions() RE_STD_NOEXCEPT
        {
            return std::uncaught_exception() ? 1 : 0;
        }

    #endif // RE_STD_LANG_IS_CPP17_OR_HIGHER

    // ---- uncaught_exception (singular) ----------------------------------
    #if RE_STD_LANG_IS_CPP17_OR_HIGHER

        // uncaught_exception
        //   function: shim over the plural form. Used from C++17 onward so
        //   re_std never routes through std::uncaught_exception (deprecated
        //   in C++17, removed in C++20); also keeps the spelling alive for
        //   pre-C++20 source compatibility.
        inline bool uncaught_exception() RE_STD_NOEXCEPT
        {
            return uncaught_exceptions() > 0;
        }

    #else // C++98/11/14: std::uncaught_exception is present and not deprecated

        // uncaught_exception
        //   function: using-declaration from std::uncaught_exception.
        using std::uncaught_exception;

    #endif // RE_STD_LANG_IS_CPP17_OR_HIGHER

}  // re_std
#else // freestanding: no exception machinery to query

namespace re_std
{
    // uncaught_exceptions
    //   function: degraded — no in-flight tracking available.
    inline int uncaught_exceptions() RE_STD_NOEXCEPT
    {
        return 0;
    }

    // uncaught_exception
    //   function: degraded — always reports "none in flight".
    inline bool uncaught_exception() RE_STD_NOEXCEPT
    {
        return false;
    }

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_EXCEPTION_UNCAUGHT_EXCEPTIONS_HPP
