/***********************************************************************
* restd                                                     exception.hpp
*
* the exception base class:
*   restd::exception is the root of the standard exception hierarchy.
* It is a runtime/ABI-defined type, so restd re-exports std::exception
* via a using-declaration whenever <exception> is available — type
* identity is preserved, so catch(std::exception&) catches restd
* exceptions and vice versa, and restd modules throw restd::exception-
* derived types to stay in-namespace. When <exception> is unavailable
* (freestanding), restd degrades to a minimal standalone base so that
* dependent code still compiles.
*
*
* path:      /inc/djinterp/re_std/exception/exception.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_EXCEPTION_EXCEPTION_
#define RESTD_EXCEPTION_EXCEPTION_ 1

#include "../djinterp.hpp"

#if D_ENV_CPP98_HAS_EXCEPTION

    #include <exception>

namespace restd
{
    // exception
    //   class: using-declaration from std::exception. Type identity is
    //   preserved across the std/restd boundary.
    using std::exception;

} // namespace restd

#else // freestanding: no <exception>

namespace restd
{
    // exception
    //   class: minimal standalone base used only when <exception> is
    //   unavailable. Does not participate in catch(std::exception&)
    //   because there is no std::exception to relate to.
    class exception
    {
    public:
        exception() D_NOEXCEPT
        {}

        virtual ~exception() D_NOEXCEPT
        {}

        virtual const char* what() const D_NOEXCEPT
        {
            return "unknown exception";
        }
    };

} // namespace restd

#endif // D_ENV_CPP98_HAS_EXCEPTION

#endif // RESTD_EXCEPTION_EXCEPTION_
