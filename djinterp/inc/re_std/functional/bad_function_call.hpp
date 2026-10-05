/*******************************************************************************
* djinterp [re_std]                                        bad_function_call.hpp
*
* bad_function_call exception header:
* exception type thrown by `function::operator()` when the wrapper is
*   empty.
*   Adapts its inheritance hierarchy to whichever standard headers are
* available: when `<exception>` is reachable it derives from
* `std::exception` so it participates in `catch (std::exception&)`
* handlers; otherwise it is a standalone class. Mirrors the
* tiered-base-class pattern used by `bad_any_cast` and
* `bad_optional_access`.
*
*
* path:      /inc/re_std/functional/bad_function_call.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_BAD_FUNCTION_CALL_HPP
#define RE_STD_FUNCTIONAL_BAD_FUNCTION_CALL_HPP 1

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

#if RE_STD_HAS_EXCEPTIONS

// bad_function_call
//   class: thrown by an empty `function`'s call operator. Inherits
// `std::exception` when available.
class bad_function_call : public std::exception
{
public:
    bad_function_call()
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {}

    virtual ~bad_function_call()
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {}

    virtual const char*
    what() const
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {
        return "bad_function_call";
    }
};

#else // no <exception>

// bad_function_call
//   class: standalone fallback when `<exception>` is unavailable.
// Cannot be caught by a `std::exception&` handler.
class bad_function_call
{
public:
    bad_function_call()
    {}

    ~bad_function_call()
    {}

    const char*
    what() const
    {
        return "bad_function_call";
    }
};

#endif // RE_STD_HAS_EXCEPTIONS

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_FUNCTIONAL_BAD_FUNCTION_CALL_HPP
