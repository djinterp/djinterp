/***********************************************************************
* restd                                            out_of_range.hpp
*
* out_of_range:
*   <stdexcept> class derived from logic_error; reported for an argument outside the valid range (e.g. at() bounds checks). Runtime-provided,
* so restd re-exports std::out_of_range when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* restd::logic_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/restd/stdexcept/out_of_range.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_OUT_OF_RANGE_
#define RESTD_STDEXCEPT_OUT_OF_RANGE_ 1

#include "../djinterp.hpp"
#include "logic_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // out_of_range
    //   class: using-declaration from std::out_of_range.
    using std::out_of_range;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // out_of_range
    //   class: standalone fallback deriving from restd::logic_error;
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

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_OUT_OF_RANGE_
