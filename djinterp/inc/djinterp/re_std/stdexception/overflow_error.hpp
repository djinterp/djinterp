/***********************************************************************
* restd                                          overflow_error.hpp
*
* overflow_error:
*   <stdexcept> class derived from runtime_error; reported on arithmetic overflow. Runtime-provided,
* so restd re-exports std::overflow_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* restd::runtime_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/re_std/stdexception/overflow_error.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_OVERFLOW_ERROR_
#define RESTD_STDEXCEPT_OVERFLOW_ERROR_ 1

#include "../djinterp.hpp"
#include "runtime_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // overflow_error
    //   class: using-declaration from std::overflow_error.
    using std::overflow_error;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // overflow_error
    //   class: standalone fallback deriving from restd::runtime_error;
    //   forwards the const char* constructor, inherits what().
    class overflow_error : public runtime_error
    {
    public:
        explicit overflow_error(const char* _what) D_NOEXCEPT
            : runtime_error(_what)
        {}

        virtual ~overflow_error() D_NOEXCEPT
        {}
    };

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_OVERFLOW_ERROR_
