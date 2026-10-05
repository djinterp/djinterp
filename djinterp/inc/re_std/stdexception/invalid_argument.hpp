/*******************************************************************************
* djinterp [re_std]                                         invalid_argument.hpp
*
* invalid_argument:
*   <stdexcept> class derived from logic_error; reported for an argument value that is invalid for the operation. Runtime-provided,
* so re_std re-exports std::invalid_argument when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/re_std/stdexception/invalid_argument.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STDEXCEPTION_INVALID_ARGUMENT_HPP
#define RE_STD_STDEXCEPTION_INVALID_ARGUMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "logic_error.hpp"

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <stdexcept>

namespace re_std
{
    // invalid_argument
    //   class: using-declaration from std::invalid_argument.
    using std::invalid_argument;

}  // re_std
#else // freestanding fallback

namespace re_std
{
    // invalid_argument
    //   class: standalone fallback deriving from re_std::logic_error;
    //   forwards the const char* constructor, inherits what().
    class invalid_argument : public logic_error
    {
    public:
        explicit invalid_argument(const char* _what) RE_STD_NOEXCEPT
            : logic_error(_what)
        {}

        virtual ~invalid_argument() RE_STD_NOEXCEPT
        {}
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_STDEXCEPTION_INVALID_ARGUMENT_HPP
