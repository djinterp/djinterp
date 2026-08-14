/***********************************************************************
* restd                                            bad_function_call.hpp
*
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
* path:      /inc/djinterp/re_std/functional/bad_function_call.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_BAD_FUNCTION_CALL_
#define RESTD_FUNCTIONAL_BAD_FUNCTION_CALL_ 1

#include "djinterp.hpp"

#if D_ENV_CPP98_HAS_EXCEPTION
    #include <exception>
#endif

namespace restd
{

#if D_ENV_CPP98_HAS_EXCEPTION

// bad_function_call
//   class: thrown by an empty `function`'s call operator. Inherits
// `std::exception` when available.
class bad_function_call : public std::exception
{
public:
    bad_function_call()
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {}

    virtual ~bad_function_call()
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {}

    virtual const char*
    what() const
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
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

#endif // D_ENV_CPP98_HAS_EXCEPTION

} // namespace restd

#endif // RESTD_FUNCTIONAL_BAD_FUNCTION_CALL_
