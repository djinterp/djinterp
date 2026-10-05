/*******************************************************************************
* djinterp [re_std]                                             out_of_range.hpp
*
* out_of_range:
*   <stdexcept> class derived from logic_error; reported for an argument outside the valid range (e.g. at() bounds checks). Runtime-provided,
* so re_std re-exports std::out_of_range when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/re_std/stdexception/out_of_range.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STDEXCEPTION_OUT_OF_RANGE_HPP
#define RE_STD_STDEXCEPTION_OUT_OF_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "logic_error.hpp"

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <stdexcept>

namespace re_std
{
    // out_of_range
    //   class: using-declaration from std::out_of_range.
    using std::out_of_range;

}  // re_std
#else // freestanding fallback

namespace re_std
{
    // out_of_range
    //   class: standalone fallback deriving from re_std::logic_error;
    //   forwards the const char* constructor, inherits what().
    class out_of_range : public logic_error
    {
    public:
        explicit out_of_range(const char* _what) RE_STD_NOEXCEPT
            : logic_error(_what)
        {}

        virtual ~out_of_range() RE_STD_NOEXCEPT
        {}
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_STDEXCEPTION_OUT_OF_RANGE_HPP
