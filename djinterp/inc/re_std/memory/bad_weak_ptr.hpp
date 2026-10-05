/*******************************************************************************
* djinterp [re_std]                                             bad_weak_ptr.hpp
*
* exception type thrown by the shared_ptr(weak_ptr) constructor when
* the weak_ptr has already expired:
*   re_std::shared_ptr<T> p(my_weak_ptr);  // throws bad_weak_ptr
*                                          // if my_weak_ptr is expired
*
* tiered implementation, mirroring re_std::bad_any_cast and
* re_std::bad_optional_access:
*
*   RE_STD_HAS_EXCEPTIONS = 1   inherits std::exception, what()
*                                   is virtual + override + noexcept.
*   neither header available         standalone class with non-virtual
*                                   what(). Throwable and catchable by
*                                   type, but not via
*                                   `catch (std::exception&)`.
*
* what() returns "bad_weak_ptr" on every tier.
*
*
* path:      /inc/re_std/memory/bad_weak_ptr.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_BAD_WEAK_PTR_HPP
#define RE_STD_MEMORY_BAD_WEAK_PTR_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std


#if RE_STD_HAS_EXCEPTIONS
    // std
    #include <exception>
#endif


namespace re_std
{

// =============================================================================
// bad_weak_ptr
// =============================================================================

#if RE_STD_HAS_EXCEPTIONS

    // bad_weak_ptr
    //   class: thrown by shared_ptr(weak_ptr) when the weak_ptr is
    //          expired. Inherits std::exception.
    class bad_weak_ptr : public std::exception
    {
    public:
        bad_weak_ptr() RE_STD_NOEXCEPT
        {
        }

        #if RE_STD_LANG_IS_CPP11_OR_HIGHER
            const char* what() const RE_STD_NOEXCEPT RE_STD_OVERRIDE
            {
                return "bad_weak_ptr";
            }
        #else
            const char* what() const RE_STD_NOEXCEPT
            {
                return "bad_weak_ptr";
            }
        #endif
    };

#else  // !RE_STD_HAS_EXCEPTIONS

    // bad_weak_ptr
    //   class: standalone fallback. Catchable by type only.
    class bad_weak_ptr
    {
    public:
        bad_weak_ptr() RE_STD_NOEXCEPT
        {
        }

        const char* what() const RE_STD_NOEXCEPT
        {
            return "bad_weak_ptr";
        }
    };

#endif  // RE_STD_HAS_EXCEPTIONS


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_MEMORY_BAD_WEAK_PTR_HPP
