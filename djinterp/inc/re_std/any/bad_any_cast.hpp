/*******************************************************************************
* djinterp [re_std]                                             bad_any_cast.hpp
*
* bad_any_cast exception header:
*   Provides the exception type thrown by any_cast when the requested type
* does not match the stored type. The base class is selected based on
* available headers:
*   - <typeinfo>  available -> inherits std::bad_cast (-> std::exception)
*   - <exception> available -> inherits std::exception
*   - neither               -> standalone class (no base, non-virtual what())
*
*
* path:      /inc/re_std/any/bad_any_cast.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.10
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ANY_BAD_ANY_CAST_HPP
#define RE_STD_ANY_BAD_ANY_CAST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   CONDITIONAL INCLUDES
// ===========================================================================

#if RE_STD_HAS_RTTI
    // std
    #include <typeinfo>
#elif RE_STD_HAS_EXCEPTIONS
    // std
    #include <exception>
#endif


namespace re_std
{


// ===========================================================================
// I.   BAD_ANY_CAST
// ===========================================================================

#if RE_STD_HAS_RTTI

// bad_any_cast
//   class: thrown by any_cast when the requested type does not
// match the type of the stored value.
// inherits: std::bad_cast -> std::exception.
class bad_any_cast : public std::bad_cast
{
public:
    const char*
    what() const
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        RE_STD_NOEXCEPT override
#else
        // C++98's std::exception::what() is throw(), and an override may not
        // have a looser exception specification -- a requirement of the
        // base, not a choice of spelling, so not RE_STD_NOEXCEPT's (decision 3.1)
        throw()
#endif
    {
        return "bad any_cast";
    }
};

#elif RE_STD_HAS_EXCEPTIONS

// bad_any_cast
//   class: thrown by any_cast when the requested type does not
// match the type of the stored value.
// note: <typeinfo> unavailable; inherits std::exception directly.
class bad_any_cast : public std::exception
{
public:
    const char*
    what() const
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        RE_STD_NOEXCEPT override
#else
        // C++98's std::exception::what() is throw(), and an override may not
        // have a looser exception specification -- a requirement of the
        // base, not a choice of spelling, so not RE_STD_NOEXCEPT's (decision 3.1)
        throw()
#endif
    {
        return "bad any_cast";
    }
};

#else

// bad_any_cast
//   class: thrown by any_cast when the requested type does not
// match the type of the stored value.
// note: exceptions disabled or unavailable; standalone class. what()
// is non-virtual. Throw and catch by type only.
class bad_any_cast
{
public:
    const char*
    what() const RE_STD_NOEXCEPT
    {
        return "bad any_cast";
    }
};

#endif  // RE_STD_HAS_RTTI / RE_STD_HAS_EXCEPTIONS


}  // re_std


#endif  // RE_STD_ANY_BAD_ANY_CAST_HPP
