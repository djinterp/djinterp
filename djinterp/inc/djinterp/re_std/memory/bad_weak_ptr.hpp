/***********************************************************************
* restd                                                   bad_weak_ptr.hpp
*
* exception type thrown by the shared_ptr(weak_ptr) constructor when
* the weak_ptr has already expired:
*   restd::shared_ptr<T> p(my_weak_ptr);  // throws bad_weak_ptr
*                                          // if my_weak_ptr is expired
*
* tiered implementation, mirroring restd::bad_any_cast and
* restd::bad_optional_access:
*
*   D_ENV_CPP98_HAS_EXCEPTION = 1   inherits std::exception, what()
*                                   is virtual + override + noexcept.
*   neither header available         standalone class with non-virtual
*                                   what(). Throwable and catchable by
*                                   type, but not via
*                                   `catch (std::exception&)`.
*
* what() returns "bad_weak_ptr" on every tier.
*
*
* path:      /inc/djinterp/re_std/memory/bad_weak_ptr.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_BAD_WEAK_PTR_
#define RESTD_MEMORY_BAD_WEAK_PTR_ 1

#include "djinterp.hpp"


#if D_ENV_CPP98_HAS_EXCEPTION
    #include <exception>
#endif


namespace restd
{

// =============================================================================
// bad_weak_ptr
// =============================================================================

#if D_ENV_CPP98_HAS_EXCEPTION

    // bad_weak_ptr
    //   class: thrown by shared_ptr(weak_ptr) when the weak_ptr is
    //          expired. Inherits std::exception.
    class bad_weak_ptr : public std::exception
    {
    public:
        bad_weak_ptr() D_NOEXCEPT
        {
        }

        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            const char* what() const D_NOEXCEPT D_OVERRIDE
            {
                return "bad_weak_ptr";
            }
        #else
            const char* what() const D_NOEXCEPT
            {
                return "bad_weak_ptr";
            }
        #endif
    };

#else  // !D_ENV_CPP98_HAS_EXCEPTION

    // bad_weak_ptr
    //   class: standalone fallback. Catchable by type only.
    class bad_weak_ptr
    {
    public:
        bad_weak_ptr() D_NOEXCEPT
        {
        }

        const char* what() const D_NOEXCEPT
        {
            return "bad_weak_ptr";
        }
    };

#endif  // D_ENV_CPP98_HAS_EXCEPTION


}  // namespace restd

#endif  // RESTD_MEMORY_BAD_WEAK_PTR_
