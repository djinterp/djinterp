/*******************************************************************************
* djinterp [re_std]                                                type_info.hpp
*
* run-time type information header:
*   Surfaces re_std::type_info as a using-declaration for std::type_info.
* type_info is the object the typeid operator yields; its storage layout,
* construction, and the name-interning behind operator== are all
* compiler-and-runtime-provided and cannot be reimplemented portably, so
* re_std re-exports the std type rather than defining its own. The
* using-declaration preserves type identity: re_std::type_info IS
* std::type_info, so a reference obtained from typeid binds to either
* spelling and comparisons interoperate.
*
*   WHY RE-EXPORT (NOT REIMPLEMENT):
*   The whole point of re_std is a namespace-consistent surface, so re_std
* modules that traffic in type identity (any, the bad_*_access exception
* chains, the future typeindex) can name re_std::type_info and stay
* in-namespace. There is nothing to back-port — type_info has existed
* since C++98. hash_code() rides along from C++11, and the C++23
* constant-evaluation support for the comparison/hash members rides along
* too; both come straight from the std type with no re_std involvement.
*
*   PORTABILITY:
*   Gated on RE_STD_HAS_RTTI (RTTI / <typeinfo> availability). On
* a build with RTTI disabled (e.g. -fno-rtti) the symbol is not surfaced,
* matching re_std's policy of not exposing RTTI types when RTTI is off.
*
*
* path:      /inc/re_std/typeinfo/type_info.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPEINFO_TYPE_INFO_HPP
#define RE_STD_TYPEINFO_TYPE_INFO_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_HAS_RTTI


// std (runtime-provided RTTI types)
// std
#include <typeinfo>


namespace re_std
{

// type_info
//   class: re-export of std::type_info. typeid yields a const reference
// to one of these; members name(), before(), operator==/!=, and
// hash_code() (C++11) come from the std type. Not copyable / not
// assignable, exactly as std specifies.
using ::std::type_info;

}  // re_std


#endif  // RE_STD_HAS_RTTI


#endif  // RE_STD_TYPEINFO_TYPE_INFO_HPP
