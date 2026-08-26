/***********************************************************************
* re_std                                                bad_exception.hpp
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
* path:      /inc/djinterp/re_std/exception/bad_exception.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_EXCEPTION_BAD_EXCEPTION_
#define DJINTERP_RE_STD_EXCEPTION_BAD_EXCEPTION_ 1

#include "../../core/djinterp.hpp"
#include "exception.hpp"

#if D_ENV_CPP98_HAS_EXCEPTION

    #include <exception>

namespace re_std
{
    // bad_exception
    //   class: using-declaration from std::bad_exception.
    using std::bad_exception;

} // namespace re_std

#else // freestanding fallback

namespace re_std
{
    // bad_exception
    //   class: standalone fallback deriving from re_std::exception.
    class bad_exception : public exception
    {
    public:
        bad_exception() D_NOEXCEPT
        {}

        virtual ~bad_exception() D_NOEXCEPT
        {}

        virtual const char* what() const D_NOEXCEPT
        {
            return "bad_exception";
        }
    };

} // namespace re_std

#endif // D_ENV_CPP98_HAS_EXCEPTION

#endif  // DJINTERP_RE_STD_EXCEPTION_BAD_EXCEPTION_
