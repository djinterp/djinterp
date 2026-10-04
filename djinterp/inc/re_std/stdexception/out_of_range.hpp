/***********************************************************************
* re_std                                           out_of_range.hpp
*
* out_of_range:
*   <stdexcept> class derived from logic_error; reported for an argument outside the valid range (e.g. at() bounds checks). Runtime-provided,
* so re_std re-exports std::out_of_range when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/re_std/stdexception/out_of_range.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_STDEXCEPT_OUT_OF_RANGE_
#define DJINTERP_RE_STD_STDEXCEPT_OUT_OF_RANGE_ 1

#include "../../core/djinterp.hpp"
#include "logic_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace re_std
{
    // out_of_range
    //   class: using-declaration from std::out_of_range.
    using std::out_of_range;

} // namespace re_std

#else // freestanding fallback

namespace re_std
{
    // out_of_range
    //   class: standalone fallback deriving from re_std::logic_error;
    //   forwards the const char* constructor, inherits what().
    class out_of_range : public logic_error
    {
    public:
        explicit out_of_range(const char* _what) D_NOEXCEPT
            : logic_error(_what)
        {}

        virtual ~out_of_range() D_NOEXCEPT
        {}
    };

} // namespace re_std

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif  // DJINTERP_RE_STD_STDEXCEPT_OUT_OF_RANGE_
