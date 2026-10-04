/***********************************************************************
* re_std                                                runtime_error.hpp
*
* runtime_error:
*   base of the "errors detectable only as the program runs" branch of
* the <stdexcept> hierarchy (derives from exception). Runtime-provided,
* so re_std re-exports std::runtime_error when <stdexcept> is available
* (type identity preserved) and degrades to a standalone class deriving
* from re_std::exception otherwise, reusing the non-allocating message
* holder defined alongside logic_error.
*
*
* path:      /inc/djinterp/re_std/stdexception/runtime_error.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_STDEXCEPT_RUNTIME_ERROR_
#define DJINTERP_RE_STD_STDEXCEPT_RUNTIME_ERROR_ 1

#include "../../core/djinterp.hpp"
#include "../exception/exception.hpp"
#include "logic_error.hpp" // for re_std::internal::fixed_message in the fallback

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace re_std
{
    // runtime_error
    //   class: using-declaration from std::runtime_error.
    using std::runtime_error;

} // namespace re_std

#else // freestanding fallback

namespace re_std
{
    // runtime_error
    //   class: standalone fallback deriving from re_std::exception.
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

} // namespace re_std

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif  // DJINTERP_RE_STD_STDEXCEPT_RUNTIME_ERROR_
