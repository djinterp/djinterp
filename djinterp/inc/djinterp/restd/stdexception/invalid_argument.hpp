/***********************************************************************
* restd                                        invalid_argument.hpp
*
* invalid_argument:
*   <stdexcept> class derived from logic_error; reported for an argument value that is invalid for the operation. Runtime-provided,
* so restd re-exports std::invalid_argument when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* restd::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/restd/stdexcept/invalid_argument.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_INVALID_ARGUMENT_
#define RESTD_STDEXCEPT_INVALID_ARGUMENT_ 1

#include "../djinterp.hpp"
#include "logic_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // invalid_argument
    //   class: using-declaration from std::invalid_argument.
    using std::invalid_argument;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // invalid_argument
    //   class: standalone fallback deriving from restd::logic_error;
    //   forwards the const char* constructor, inherits what().
    class invalid_argument : public logic_error
    {
    public:
        explicit invalid_argument(const char* _what) D_NOEXCEPT
            : logic_error(_what)
        {}

        virtual ~invalid_argument() D_NOEXCEPT
        {}
    };

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_INVALID_ARGUMENT_
