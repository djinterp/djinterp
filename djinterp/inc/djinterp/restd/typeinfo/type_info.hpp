/******************************************************************************
* djinterp [restd]                                                type_info.hpp
*
* run-time type information header:
*   Surfaces restd::type_info as a using-declaration for std::type_info.
* type_info is the object the typeid operator yields; its storage layout,
* construction, and the name-interning behind operator== are all
* compiler-and-runtime-provided and cannot be reimplemented portably, so
* restd re-exports the std type rather than defining its own. The
* using-declaration preserves type identity: restd::type_info IS
* std::type_info, so a reference obtained from typeid binds to either
* spelling and comparisons interoperate.
*
*   WHY RE-EXPORT (NOT REIMPLEMENT):
*   The whole point of restd is a namespace-consistent surface, so restd
* modules that traffic in type identity (any, the bad_*_access exception
* chains, the future typeindex) can name restd::type_info and stay
* in-namespace. There is nothing to back-port — type_info has existed
* since C++98. hash_code() rides along from C++11, and the C++23
* constant-evaluation support for the comparison/hash members rides along
* too; both come straight from the std type with no restd involvement.
*
*   PORTABILITY:
*   Gated on D_ENV_CPP98_HAS_TYPEINFO (RTTI / <typeinfo> availability). On
* a build with RTTI disabled (e.g. -fno-rtti) the symbol is not surfaced,
* matching restd's policy of not exposing RTTI types when RTTI is off.
*
*
* path:      /inc/djinterp/restd/typeinfo/type_info.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPEINFO_TYPE_INFO_
#define DJINTERP_RESTD_TYPEINFO_TYPE_INFO_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_CPP98_HAS_TYPEINFO


// std (runtime-provided RTTI types)
#include <typeinfo>


NS_RESTD

// type_info
//   class: re-export of std::type_info. typeid yields a const reference
// to one of these; members name(), before(), operator==/!=, and
// hash_code() (C++11) come from the std type. Not copyable / not
// assignable, exactly as std specifies.
using ::std::type_info;

NS_END  // restd


#endif  // D_ENV_CPP98_HAS_TYPEINFO


#endif  // DJINTERP_RESTD_TYPEINFO_TYPE_INFO_
