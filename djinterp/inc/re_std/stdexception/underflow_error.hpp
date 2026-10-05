/*******************************************************************************
* djinterp [re_std]                                          underflow_error.hpp
*
* underflow_error:
*   <stdexcept> class derived from runtime_error; reported on arithmetic underflow. Runtime-provided,
* so re_std re-exports std::underflow_error when <stdexcept> is available (type
* identity preserved) and degrades to a standalone class deriving from
* re_std::runtime_error otherwise, forwarding the const char* constructor and
* inheriting what() from the base.
*
*
* path:      /inc/re_std/stdexception/underflow_error.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STDEXCEPTION_UNDERFLOW_ERROR_HPP
#define RE_STD_STDEXCEPTION_UNDERFLOW_ERROR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "runtime_error.hpp"

#if RE_STD_HAS_EXCEPTIONS

    // std
    #include <stdexcept>

namespace re_std
{
    // underflow_error
    //   class: using-declaration from std::underflow_error.
    using std::underflow_error;

}  // re_std
#else // freestanding fallback

namespace re_std
{
    // underflow_error
    //   class: standalone fallback deriving from re_std::runtime_error;
    //   forwards the const char* constructor, inherits what().
    class underflow_error : public runtime_error
    {
    public:
        explicit underflow_error(const char* _what) RE_STD_NOEXCEPT
            : runtime_error(_what)
        {}

        virtual ~underflow_error() RE_STD_NOEXCEPT
        {}
    };

}  // re_std
#endif // RE_STD_HAS_EXCEPTIONS

#endif  // RE_STD_STDEXCEPTION_UNDERFLOW_ERROR_HPP
