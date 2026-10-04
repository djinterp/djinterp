/***********************************************************************
* re_std                                            range_error.hpp
*
* range_error:
*   <stdexcept> class derived from runtime_error; reported when a computed result is outside the representable range. Runtime-provided,
* so re_std re-exports std::range_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::runtime_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/re_std/stdexception/range_error.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_STDEXCEPT_RANGE_ERROR_
#define DJINTERP_RE_STD_STDEXCEPT_RANGE_ERROR_ 1

#include "../../core/djinterp.hpp"
#include "runtime_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace re_std
{
    // range_error
    //   class: using-declaration from std::range_error.
    using std::range_error;

} // namespace re_std

#else // freestanding fallback

namespace re_std
{
    // range_error
    //   class: standalone fallback deriving from re_std::runtime_error;
    //   forwards the const char* constructor, inherits what().
    class range_error : public runtime_error
    {
    public:
        explicit range_error(const char* _what) D_NOEXCEPT
            : runtime_error(_what)
        {}

        virtual ~range_error() D_NOEXCEPT
        {}
    };

} // namespace re_std

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif  // DJINTERP_RE_STD_STDEXCEPT_RANGE_ERROR_
