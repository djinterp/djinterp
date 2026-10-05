/*******************************************************************************
* djinterp [re_std]                                             length_error.hpp
*
* length_error:
*   <stdexcept> class derived from logic_error; reported when an object is asked to exceed its maximum permitted size. Runtime-provided,
* so re_std re-exports std::length_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/re_std/stdexception/length_error.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STDEXCEPTION_LENGTH_ERROR_HPP
#define RE_STD_STDEXCEPTION_LENGTH_ERROR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "logic_error.hpp"

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <stdexcept>

namespace re_std
{
    // length_error
    //   class: using-declaration from std::length_error.
    using std::length_error;

}  // re_std
#else // freestanding fallback

namespace re_std
{
    // length_error
    //   class: standalone fallback deriving from re_std::logic_error;
    //   forwards the const char* constructor, inherits what().
    class length_error : public logic_error
    {
    public:
        explicit length_error(const char* _what) RE_STD_NOEXCEPT
            : logic_error(_what)
        {}

        virtual ~length_error() RE_STD_NOEXCEPT
        {}
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_STDEXCEPTION_LENGTH_ERROR_HPP
