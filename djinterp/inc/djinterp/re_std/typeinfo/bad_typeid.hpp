/******************************************************************************
* djinterp [restd]                                               bad_typeid.hpp
*
* bad_typeid exception header:
*   Surfaces restd::bad_typeid as a using-declaration for
* std::bad_typeid — the exception thrown when typeid is applied to a
* dereferenced null pointer of polymorphic type. It is runtime-provided
* (the typeid machinery throws it), so restd re-exports rather than
* reimplements. Type identity is preserved: restd::bad_typeid IS
* std::bad_typeid, so a language-level typeid throw is caught by
* catch (const restd::bad_typeid&) and vice versa.
*
*   PORTABILITY:
*   Gated on D_ENV_CPP98_HAS_TYPEINFO. C++98 baseline; nothing to
* back-port (std::bad_typeid has existed since C++98).
*
*
* path:      /inc/djinterp/restd/typeinfo/bad_typeid.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPEINFO_BAD_TYPEID_
#define DJINTERP_RESTD_TYPEINFO_BAD_TYPEID_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP98_HAS_TYPEINFO


// std (runtime-provided RTTI types)
#include <typeinfo>


NS_RESTD

// bad_typeid
//   class: re-export of std::bad_typeid (derives from std::exception).
// Thrown when typeid is applied to a null dereferenced glvalue of
// polymorphic type; what() returns an implementation-defined message.
using ::std::bad_typeid;

NS_END  // restd


#endif  // D_ENV_CPP98_HAS_TYPEINFO


#endif  // DJINTERP_RESTD_TYPEINFO_BAD_TYPEID_
