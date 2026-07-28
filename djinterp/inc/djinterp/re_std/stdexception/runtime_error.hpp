/***********************************************************************
* restd                                                 runtime_error.hpp
*
* runtime_error:
*   base of the "errors detectable only as the program runs" branch of
* the <stdexcept> hierarchy (derives from exception). Runtime-provided,
* so restd re-exports std::runtime_error when <stdexcept> is available
* (type identity preserved) and degrades to a standalone class deriving
* from restd::exception otherwise, reusing the non-allocating message
* holder defined alongside logic_error.
*
*
* path:      /inc/djinterp/restd/stdexcept/runtime_error.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_RUNTIME_ERROR_
#define RESTD_STDEXCEPT_RUNTIME_ERROR_ 1

#include "../djinterp.hpp"
#include "../exception/exception.hpp"
#include "logic_error.hpp" // for restd::internal::fixed_message in the fallback

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // runtime_error
    //   class: using-declaration from std::runtime_error.
    using std::runtime_error;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // runtime_error
    //   class: standalone fallback deriving from restd::exception.
    //   Exposes the const char* constructor only (no <string> dependency).
    class runtime_error : public exception
    {
    public:
        explicit runtime_error(const char* _what) D_NOEXCEPT
            : m_msg(_what)
        {}

        virtual ~runtime_error() D_NOEXCEPT
        {}

        virtual const char* what() const D_NOEXCEPT
        {
            return m_msg.c_str();
        }

    private:
        internal::fixed_message m_msg;
    };

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_RUNTIME_ERROR_
