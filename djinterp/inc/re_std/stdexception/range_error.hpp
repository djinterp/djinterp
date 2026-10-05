/*******************************************************************************
* djinterp [re_std]                                              range_error.hpp
*
* range_error:
*   <stdexcept> class derived from runtime_error; reported when a computed result is outside the representable range. Runtime-provided,
* so re_std re-exports std::range_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::runtime_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/re_std/stdexception/range_error.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STDEXCEPTION_RANGE_ERROR_HPP
#define RE_STD_STDEXCEPTION_RANGE_ERROR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "runtime_error.hpp"

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <stdexcept>

namespace re_std
{
    // range_error
    //   class: using-declaration from std::range_error.
    using std::range_error;

}  // re_std
#else // freestanding fallback

namespace re_std
{
    // range_error
    //   class: standalone fallback deriving from re_std::runtime_error;
    //   forwards the const char* constructor, inherits what().
    class range_error : public runtime_error
    {
    public:
        explicit range_error(const char* _what) RE_STD_NOEXCEPT
            : runtime_error(_what)
        {}

        virtual ~range_error() RE_STD_NOEXCEPT
        {}
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_STDEXCEPTION_RANGE_ERROR_HPP
