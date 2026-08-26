/***********************************************************************
* re_std                                                    exception.hpp
*
* the exception base class:
*   re_std::exception is the root of the standard exception hierarchy.
* It is a runtime/ABI-defined type, so re_std re-exports std::exception
* via a using-declaration whenever <exception> is available — type
* identity is preserved, so catch(std::exception&) catches re_std
* exceptions and vice versa, and re_std modules throw re_std::exception-
* derived types to stay in-namespace. When <exception> is unavailable
* (freestanding), re_std degrades to a minimal standalone base so that
* dependent code still compiles.
*
*
* path:      /inc/djinterp/re_std/exception/exception.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_EXCEPTION_EXCEPTION_
#define DJINTERP_RE_STD_EXCEPTION_EXCEPTION_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_CPP98_HAS_EXCEPTION

    #include <exception>

namespace re_std
{
    // exception
    //   class: using-declaration from std::exception. Type identity is
    //   preserved across the std/re_std boundary.
    using std::exception;

} // namespace re_std

#else // freestanding: no <exception>

namespace re_std
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

} // namespace re_std

#endif // D_ENV_CPP98_HAS_EXCEPTION

#endif  // DJINTERP_RE_STD_EXCEPTION_EXCEPTION_
