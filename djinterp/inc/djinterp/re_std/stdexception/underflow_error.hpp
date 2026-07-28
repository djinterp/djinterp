/***********************************************************************
* restd                                         underflow_error.hpp
*
* underflow_error:
*   <stdexcept> class derived from runtime_error; reported on arithmetic underflow. Runtime-provided,
* so restd re-exports std::underflow_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* restd::runtime_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/restd/stdexcept/underflow_error.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_STDEXCEPT_UNDERFLOW_ERROR_
#define RESTD_STDEXCEPT_UNDERFLOW_ERROR_ 1

#include "../djinterp.hpp"
#include "runtime_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace restd
{
    // underflow_error
    //   class: using-declaration from std::underflow_error.
    using std::underflow_error;

} // namespace restd

#else // freestanding fallback

namespace restd
{
    // underflow_error
    //   class: standalone fallback deriving from restd::runtime_error;
    //   forwards the const char* constructor, inherits what().
    class underflow_error : public runtime_error
    {
    public:
        explicit underflow_error(const char* _what) D_NOEXCEPT
            : runtime_error(_what)
        {}

        virtual ~underflow_error() D_NOEXCEPT
        {}
    };

} // namespace restd

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif // RESTD_STDEXCEPT_UNDERFLOW_ERROR_
