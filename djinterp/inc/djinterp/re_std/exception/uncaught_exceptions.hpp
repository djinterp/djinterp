/***********************************************************************
* restd                                          uncaught_exceptions.hpp
*
* in-flight-exception queries:
*   uncaught_exception() (singular, C++98; deprecated C++17; removed
* C++20) and uncaught_exceptions() (plural, C++17). The plural form is
* the useful one — it returns the *count* of in-flight exceptions, which
* lets a destructor tell "unwinding because of MY throw" apart from
* "unwinding past me". restd:
*   - re-exports the std symbols where std has them;
*   - back-ports uncaught_exceptions() below C++17, but only to the
*     *boolean precision* that is portably linkable everywhere:
*     (uncaught_exception() ? 1 : 0). The exact in-flight COUNT is not
*     recovered pre-C++17 because the underlying counter is exposed
*     inconsistently across runtimes (libc++abi ships the extern "C"
*     __cxa_uncaught_exceptions; libstdc++ ships only the C++-mangled
*     std::uncaught_exceptions, gated behind C++17 headers), and restd
*     will not emit a reference that may fail to link. Correct for the
*     common "is any exception in flight?" use; lossy for nested depth.
*   - retains uncaught_exception() past its C++20 removal as a thin shim
*     over uncaught_exceptions() > 0, honouring restd's backwards-
*     compatibility goal. At C++17 the same shim is used so that restd
*     never routes through the deprecated std::uncaught_exception (which
*     would surface -Wdeprecated-declarations at the call site).
*
*
* path:      /inc/djinterp/restd/exception/uncaught_exceptions.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_EXCEPTION_UNCAUGHT_EXCEPTIONS_
#define RESTD_EXCEPTION_UNCAUGHT_EXCEPTIONS_ 1

#include "../djinterp.hpp"

#if D_ENV_CPP98_HAS_EXCEPTION

    #include <exception>

namespace restd
{

    // ---- uncaught_exceptions (plural) -----------------------------------
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER

        // uncaught_exceptions
        //   function: using-declaration from std::uncaught_exceptions.
        using std::uncaught_exceptions;

    #else // pre-C++17: boolean-precision back-port (always linkable)

        // uncaught_exceptions
        //   function: degraded back-port — collapses the count to 0/1.
        //   Correct for "is any exception in flight?"; cannot recover
        //   nested unwinding depth pre-C++17. RESTD AHEAD OF STD: the
        //   spelling is surfaced before std's C++17.
        inline int uncaught_exceptions() D_NOEXCEPT
        {
            return std::uncaught_exception() ? 1 : 0;
        }

    #endif // D_ENV_LANG_IS_CPP17_OR_HIGHER

    // ---- uncaught_exception (singular) ----------------------------------
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER

        // uncaught_exception
        //   function: shim over the plural form. Used from C++17 onward so
        //   restd never routes through std::uncaught_exception (deprecated
        //   in C++17, removed in C++20); also keeps the spelling alive for
        //   pre-C++20 source compatibility.
        inline bool uncaught_exception() D_NOEXCEPT
        {
            return uncaught_exceptions() > 0;
        }

    #else // C++98/11/14: std::uncaught_exception is present and not deprecated

        // uncaught_exception
        //   function: using-declaration from std::uncaught_exception.
        using std::uncaught_exception;

    #endif // D_ENV_LANG_IS_CPP17_OR_HIGHER

} // namespace restd

#else // freestanding: no exception machinery to query

namespace restd
{
    // uncaught_exceptions
    //   function: degraded — no in-flight tracking available.
    inline int uncaught_exceptions() D_NOEXCEPT
    {
        return 0;
    }

    // uncaught_exception
    //   function: degraded — always reports "none in flight".
    inline bool uncaught_exception() D_NOEXCEPT
    {
        return false;
    }

} // namespace restd

#endif // D_ENV_CPP98_HAS_EXCEPTION

#endif // RESTD_EXCEPTION_UNCAUGHT_EXCEPTIONS_
