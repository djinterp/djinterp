/***********************************************************************
* restd                                            domain_error.hpp
*
* domain_error:
*   <stdexcept> class derived from logic_error; reported when an argument is outside the domain on which an operation is defined. Runtime-provided,
* so restd re-exports std::domain_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* restd::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/re_std/stdexception/domain_error.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_DOMAIN_ERROR_
#define RESTD_STDEXCEPT_DOMAIN_ERROR_ 1

#include "../djinterp.hpp"
#include "logic_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // domain_error
    //   class: using-declaration from std::domain_error.
    using std::domain_error;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // domain_error
    //   class: standalone fallback deriving from restd::logic_error;
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

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_DOMAIN_ERROR_
