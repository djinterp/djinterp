/***********************************************************************
* restd                                                 bad_exception.hpp
*
* the bad_exception type:
*   restd::bad_exception is thrown by the runtime when exception
* handling itself fails (e.g. a dynamic-exception-specification
* violation, or an exception escaping during unwinding pre-C++17). It
* is runtime/ABI-defined, so restd re-exports std::bad_exception when
* available and degrades to a standalone class deriving from
* restd::exception otherwise.
*
*
* path:      /inc/djinterp/re_std/exception/bad_exception.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_EXCEPTION_BAD_EXCEPTION_
#define RESTD_EXCEPTION_BAD_EXCEPTION_ 1

#include "../djinterp.hpp"
#include "exception.hpp"

#if D_ENV_CPP98_HAS_EXCEPTION

    #include <exception>

namespace restd
{
    // bad_exception
    //   class: using-declaration from std::bad_exception.
    using std::bad_exception;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // bad_exception
    //   class: standalone fallback deriving from restd::exception.
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

} // namespace restd

#endif // D_ENV_CPP98_HAS_EXCEPTION

#endif // RESTD_EXCEPTION_BAD_EXCEPTION_
