/*******************************************************************************
* djinterp [re_std]                                            runtime_error.hpp
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
* path:      /inc/re_std/stdexception/runtime_error.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STDEXCEPTION_RUNTIME_ERROR_HPP
#define RE_STD_STDEXCEPTION_RUNTIME_ERROR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "../exception/exception.hpp"
#include "logic_error.hpp" // for re_std::internal::fixed_message in the fallback

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <stdexcept>

namespace re_std
{
    // runtime_error
    //   class: using-declaration from std::runtime_error.
    using std::runtime_error;

}  // re_std
#else // freestanding fallback

namespace re_std
{
    // runtime_error
    //   class: standalone fallback deriving from re_std::exception.
    //   Exposes the const char* constructor only (no <string> dependency).
    class runtime_error : public exception
    {
    public:
        explicit runtime_error(const char* _what) RE_STD_NOEXCEPT
            : m_msg(_what)
        {}

        virtual ~runtime_error() RE_STD_NOEXCEPT
        {}

        virtual const char* what() const RE_STD_NOEXCEPT
        {
            return m_msg.c_str();
        }

    private:
        internal::fixed_message m_msg;
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_STDEXCEPTION_RUNTIME_ERROR_HPP
