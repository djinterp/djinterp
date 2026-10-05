/*******************************************************************************
* djinterp [re_std]                                              logic_error.hpp
*
* logic_error:
*   base of the "errors detectable before the program runs" branch of
* the <stdexcept> hierarchy (derives from exception). Runtime-provided
* — the what() string is stored with reference-counted ABI machinery —
* so re_std re-exports std::logic_error when <stdexcept> is available,
* preserving type identity (catch(std::logic_error&) catches re_std's,
* and both are catchable as re_std::exception / std::exception). When
* <stdexcept> is unavailable (freestanding), a minimal standalone class
* deriving from re_std::exception is provided, storing the message in a
* fixed internal buffer and exposing the const char* constructor only.
*
*
* path:      /inc/re_std/stdexception/logic_error.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STDEXCEPTION_LOGIC_ERROR_HPP
#define RE_STD_STDEXCEPTION_LOGIC_ERROR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "../exception/exception.hpp"

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <stdexcept>

namespace re_std
{
    // logic_error
    //   class: using-declaration from std::logic_error.
    using std::logic_error;

}  // re_std
#else // freestanding fallback (no <stdexcept>)

namespace re_std
{
namespace internal
{
    // fixed_message
    //   class: non-allocating message holder used by the freestanding
    //   <stdexcept> fallbacks. Copies up to capacity-1 chars; truncates.
    class fixed_message
    {
    public:
        explicit fixed_message(const char* _msg) RE_STD_NOEXCEPT
        {
            unsigned i = 0;
            if (_msg != 0)
            {
                for (; _msg[i] != '\0' && i + 1 < sizeof(m_buf); ++i)
                {
                    m_buf[i] = _msg[i];
                }
            }
            m_buf[i] = '\0';
        }

        const char* c_str() const RE_STD_NOEXCEPT
        {
            return m_buf;
        }

    private:
        char m_buf[256];
    };

}  // internal
    // logic_error
    //   class: standalone fallback deriving from re_std::exception.
    //   Exposes the const char* constructor only (no <string> dependency).
    class logic_error : public exception
    {
    public:
        explicit logic_error(const char* _what) RE_STD_NOEXCEPT
            : m_msg(_what)
        {}

        virtual ~logic_error() RE_STD_NOEXCEPT
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

#endif  // RE_STD_STDEXCEPTION_LOGIC_ERROR_HPP
