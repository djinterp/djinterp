/*******************************************************************************
* djinterp [re_std]                                            bad_exception.hpp
*
* the bad_exception type:
*   re_std::bad_exception is thrown by the runtime when exception
* handling itself fails (e.g. a dynamic-exception-specification
* violation, or an exception escaping during unwinding pre-C++17). It
* is runtime/ABI-defined, so re_std re-exports std::bad_exception when
* available and degrades to a standalone class deriving from
* re_std::exception otherwise.
*
*
* path:      /inc/re_std/exception/bad_exception.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXCEPTION_BAD_EXCEPTION_HPP
#define RE_STD_EXCEPTION_BAD_EXCEPTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "exception.hpp"

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <exception>

namespace re_std
{
    // bad_exception
    //   class: using-declaration from std::bad_exception.
    using std::bad_exception;

}  // re_std
#else // freestanding fallback

namespace re_std
{
    // bad_exception
    //   class: standalone fallback deriving from re_std::exception.
    class bad_exception : public exception
    {
    public:
        bad_exception() RE_STD_NOEXCEPT
        {}

        virtual ~bad_exception() RE_STD_NOEXCEPT
        {}

        virtual const char* what() const RE_STD_NOEXCEPT
        {
            return "bad_exception";
        }
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_EXCEPTION_BAD_EXCEPTION_HPP
