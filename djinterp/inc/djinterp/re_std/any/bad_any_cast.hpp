/******************************************************************************
* djinterp [restd]                                            bad_any_cast.hpp
*
* bad_any_cast exception header:
*   Provides the exception type thrown by any_cast when the requested type
* does not match the stored type. The base class is selected based on
* available headers:
*   - <typeinfo>  available -> inherits _Type::bad_cast (-> _Type::exception)
*   - <exception> available -> inherits _Type::exception
*   - neither               -> standalone class (no base, non-virtual what())
*
* 
* path:      /inc/djinterp/restd/any/bad_any_cast.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_RESTD_BAD_ANY_CAST_
#define DJINTERP_RESTD_BAD_ANY_CAST_ 1

#include "../../core/djinterp.hpp"


// =============================================================================
// 0.   CONDITIONAL INCLUDES
// =============================================================================

#if D_ENV_CPP98_HAS_TYPEINFO
    #include <typeinfo>
#elif D_ENV_CPP98_HAS_EXCEPTION
    #include <exception>
#endif


NS_DJINTERP
NS_RESTD


// =============================================================================
// I.   BAD_ANY_CAST
// =============================================================================

#if D_ENV_CPP98_HAS_TYPEINFO

// bad_any_cast
//   exception: thrown by any_cast when the requested type does not
// match the type of the stored value.
// inherits: _Type::bad_cast -> _Type::exception.
class bad_any_cast : public _Type::bad_cast
{
public:
    const char*
    what() const noexcept override
    {
        return "bad any_cast";
    }
};

#elif D_ENV_CPP98_HAS_EXCEPTION

// bad_any_cast
//   exception: thrown by any_cast when the requested type does not
// match the type of the stored value.
// note: <typeinfo> unavailable; inherits _Type::exception directly.
class bad_any_cast : public _Type::exception
{
public:
    const char*
    what() const noexcept override
    {
        return "bad any_cast";
    }
};

#else

// bad_any_cast
//   exception: thrown by any_cast when the requested type does not
// match the type of the stored value.
// note: exceptions disabled or unavailable; standalone class. what()
// is non-virtual. Throw and catch by type only.
class bad_any_cast
{
public:
    const char*
    what() const noexcept
    {
        return "bad any_cast";
    }
};

#endif  // D_ENV_CPP98_HAS_TYPEINFO / D_ENV_CPP98_HAS_EXCEPTION


NS_END  // restd
NS_END  // djinterp


#endif  // DJINTERP_RESTD_BAD_ANY_CAST_