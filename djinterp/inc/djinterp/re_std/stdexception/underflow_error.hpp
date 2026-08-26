/***********************************************************************
* re_std                                        underflow_error.hpp
*
* underflow_error:
*   <stdexcept> class derived from runtime_error; reported on arithmetic underflow. Runtime-provided,
* so re_std re-exports std::underflow_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::runtime_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/djinterp/re_std/stdexception/underflow_error.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_STDEXCEPT_UNDERFLOW_ERROR_
#define DJINTERP_RE_STD_STDEXCEPT_UNDERFLOW_ERROR_ 1

#include "../../core/djinterp.hpp"
#include "runtime_error.hpp"

#if D_ENV_CPP98_HAS_STDEXCEPT

    #include <stdexcept>

namespace re_std
{
    // underflow_error
    //   class: using-declaration from std::underflow_error.
    using std::underflow_error;

} // namespace re_std

#else // freestanding fallback

namespace re_std
{
    // underflow_error
    //   class: standalone fallback deriving from re_std::runtime_error;
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

} // namespace re_std

#endif // D_ENV_CPP98_HAS_STDEXCEPT

#endif  // DJINTERP_RE_STD_STDEXCEPT_UNDERFLOW_ERROR_
