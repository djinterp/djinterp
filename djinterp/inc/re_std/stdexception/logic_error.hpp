/***********************************************************************
* re_std                                                  logic_error.hpp
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
* path:      /inc/djinterp/re_std/stdexception/logic_error.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_STDEXCEPT_LOGIC_ERROR_
#define DJINTERP_RE_STD_STDEXCEPT_LOGIC_ERROR_ 1

#include "../../core/djinterp.hpp"
#include "../exception/exception.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace re_std
{
    // logic_error
    //   class: using-declaration from std::logic_error.
    using std::logic_error;

} // namespace re_std

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
        explicit fixed_message(const char* _msg) D_NOEXCEPT
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

        const char* c_str() const D_NOEXCEPT
        {
            return m_buf;
        }

    private:
        char m_buf[256];
    };

} // namespace internal

    // logic_error
    //   class: standalone fallback deriving from re_std::exception.
    //   Exposes the const char* constructor only (no <string> dependency).
    class logic_error : public exception
    {
    public:
        explicit logic_error(const char* _what) D_NOEXCEPT
            : m_msg(_what)
        {}

        virtual ~logic_error() D_NOEXCEPT
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

#endif  // DJINTERP_RE_STD_STDEXCEPT_LOGIC_ERROR_
