/******************************************************************************
* djinterp [re_std]                                      bad_optional_access.hpp
*
* bad_optional_access exception class:
*   Thrown by optional<T>::value() when called on a disengaged optional.
* When exceptions are unavailable in the build environment (i.e. neither
* <typeinfo> nor <exception> is present per the env detection macros),
* the trait is still provided as a standalone class -- but optional's
* value() will not throw on disengaged access (behavior becomes UB,
* matching the policy used by any_cast).
*
*   STANDARD STATUS:
*   Introduced in C++17 alongside std::optional. re_std provides it on
* C++11+, since the rest of the optional module targets that floor.
*
*   TIERED IMPLEMENTATION:
*   Mirrors bad_any_cast's tier strategy:
*     - Tier 1: <exception> available -- inherits std::exception, what()
*       is virtual override noexcept.
*     - Tier 2: neither <typeinfo> nor <exception> available -- standalone
*       class with no base, non-virtual what(). Cannot be caught by
*       catch(std::exception&).
*   There is no <typeinfo>-only tier (unlike bad_any_cast's std::bad_cast
*   case): bad_optional_access has no logical typeinfo parent.
*
*   PORTABILITY:
*   Available on C++11 and later.
*
*
* path:      /inc/djinterp/re_std/optional/bad_optional_access.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_OPTIONAL_BAD_OPTIONAL_ACCESS_
#define DJINTERP_RE_STD_OPTIONAL_BAD_OPTIONAL_ACCESS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#if D_ENV_CPP98_HAS_EXCEPTION
    #include <exception>
#else
    // std (std::abort -- the only honest action with exceptions off)
    #include <cstdlib>
#endif


NS_RESTD


    // bad_optional_access
    //   class: exception type thrown by optional<T>::value() when the
    //          optional is disengaged. Inherits std::exception when
    //          available, otherwise is standalone.
    class bad_optional_access
    #if D_ENV_CPP98_HAS_EXCEPTION
        : public std::exception
    #endif
    {
    public:

        D_CONSTEXPR bad_optional_access() D_NOEXCEPT
        {}

        // what
        //   function: returns a static C-string describing the error.
        //             Marked `override` and `noexcept` to match the
        //             std::exception virtual contract on Tier 1; on
        //             Tier 2 (no base), it is a plain non-virtual
        //             member.
        const char* what() const D_NOEXCEPT
        #if D_ENV_CPP98_HAS_EXCEPTION
            override
        #endif
        {
            return "bad optional access";
        }

    };  // class bad_optional_access


NS_INTERNAL

    // throw_bad_optional_access
    //   function: raise bad_optional_access. Added 2026-08-25 -- optional.hpp
    // called internal::throw_bad_optional_access() in three places and no
    // definition existed anywhere in the corpus, so <optional> did not
    // compile.
    //
    //   WHY A FUNCTION AND NOT A BARE `throw` AT EACH CALL SITE:
    //   value() is constexpr and its body is a conditional expression --
    //     return engaged ? m_value : (throw_bad_optional_access(), m_value)
    //   -- and a throw-expression is not usable as an operand there before
    //   C++14. Routing through a function call keeps the expression valid
    //   at every tier and puts the exceptions-disabled handling in ONE
    //   place rather than three.
    //
    //   NOT MARKED D_CONSTEXPR: it never succeeds, so it can never appear
    // in a successful constant evaluation. Marking it constexpr would
    // suggest otherwise. The enclosing value() stays constexpr regardless
    // -- a constexpr function may contain a call that is only reachable on
    // a path the evaluation does not take.
    //
    //   WITH EXCEPTIONS DISABLED there is no way to report the error and
    // no value to return, so the only honest action is to terminate.
    // Returning a reference to the disengaged storage would hand the
    // caller an uninitialised object, which is strictly worse than a
    // clean abort.
    D_INLINE void throw_bad_optional_access()
    {
    #if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_optional_access();
    #else
        ::std::abort();
    #endif
    }

NS_END  // internal


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_OPTIONAL_BAD_OPTIONAL_ACCESS_
