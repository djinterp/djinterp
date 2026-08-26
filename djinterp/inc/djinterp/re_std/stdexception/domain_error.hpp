/***********************************************************************
* re_std                                           domain_error.hpp
*
* domain_error:
*   <stdexcept> class derived from logic_error; reported when an argument is outside the domain on which an operation is defined. Runtime-provided,
* so re_std re-exports std::domain_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/re_std/stdexception/domain_error.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_STDEXCEPT_DOMAIN_ERROR_
#define DJINTERP_RE_STD_STDEXCEPT_DOMAIN_ERROR_ 1

#include "../../core/djinterp.hpp"
#include "logic_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace re_std
{
    // domain_error
    //   class: using-declaration from std::domain_error.
    using std::domain_error;

} // namespace re_std

#else // freestanding fallback

namespace re_std
{
    // domain_error
    //   class: standalone fallback deriving from re_std::logic_error;
    //   forwards the const char* constructor, inherits what().
    class domain_error : public logic_error
    {
    public:
        explicit domain_error(const char* _what) D_NOEXCEPT
            : logic_error(_what)
        {}

        virtual ~domain_error() D_NOEXCEPT
        {}
    };

} // namespace re_std

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif  // DJINTERP_RE_STD_STDEXCEPT_DOMAIN_ERROR_
