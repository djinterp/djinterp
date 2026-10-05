/*******************************************************************************
* djinterp [re_std]                                                exception.hpp
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
* path:      /inc/re_std/exception/exception.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXCEPTION_EXCEPTION_HPP
#define RE_STD_EXCEPTION_EXCEPTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <exception>

namespace re_std
{
    // exception
    //   class: using-declaration from std::exception. Type identity is
    //   preserved across the std/re_std boundary.
    using std::exception;

}  // re_std
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
        exception() RE_STD_NOEXCEPT
        {}

        virtual ~exception() RE_STD_NOEXCEPT
        {}

        virtual const char* what() const RE_STD_NOEXCEPT
        {
            return "unknown exception";
        }
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_EXCEPTION_EXCEPTION_HPP
