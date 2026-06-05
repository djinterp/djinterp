/***********************************************************************
* restd                                            length_error.hpp
*
* length_error:
*   <stdexcept> class derived from logic_error; reported when an object is asked to exceed its maximum permitted size. Runtime-provided,
* so restd re-exports std::length_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* restd::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/restd/stdexcept/length_error.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_LENGTH_ERROR_
#define RESTD_STDEXCEPT_LENGTH_ERROR_ 1

#include "../djinterp.hpp"
#include "logic_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // length_error
    //   class: using-declaration from std::length_error.
    using std::length_error;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // length_error
    //   class: standalone fallback deriving from restd::logic_error;
    //   forwards the const char* constructor, inherits what().
    class length_error : public logic_error
    {
    public:
        explicit length_error(const char* _what) D_NOEXCEPT
            : logic_error(_what)
        {}

        virtual ~length_error() D_NOEXCEPT
        {}
    };

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_LENGTH_ERROR_
