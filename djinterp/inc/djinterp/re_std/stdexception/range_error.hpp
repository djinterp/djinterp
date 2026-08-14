/***********************************************************************
* restd                                             range_error.hpp
*
* range_error:
*   <stdexcept> class derived from runtime_error; reported when a computed result is outside the representable range. Runtime-provided,
* so restd re-exports std::range_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* restd::runtime_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/re_std/stdexception/range_error.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_RANGE_ERROR_
#define RESTD_STDEXCEPT_RANGE_ERROR_ 1

#include "../djinterp.hpp"
#include "runtime_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // range_error
    //   class: using-declaration from std::range_error.
    using std::range_error;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // range_error
    //   class: standalone fallback deriving from restd::runtime_error;
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

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_RANGE_ERROR_
