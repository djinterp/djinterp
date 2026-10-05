/*******************************************************************************
* djinterp [re_std]                                               bad_typeid.hpp
*
* bad_typeid exception header:
*   Surfaces re_std::bad_typeid as a using-declaration for
* std::bad_typeid — the exception thrown when typeid is applied to a
* dereferenced null pointer of polymorphic type. It is runtime-provided
* (the typeid machinery throws it), so re_std re-exports rather than
* reimplements. Type identity is preserved: re_std::bad_typeid IS
* std::bad_typeid, so a language-level typeid throw is caught by
* catch (const re_std::bad_typeid&) and vice versa.
*
*   PORTABILITY:
*   Gated on RE_STD_HAS_RTTI. C++98 baseline; nothing to
* back-port (std::bad_typeid has existed since C++98).
*
*
* path:      /inc/re_std/typeinfo/bad_typeid.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPEINFO_BAD_TYPEID_HPP
#define RE_STD_TYPEINFO_BAD_TYPEID_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_HAS_RTTI


// std (runtime-provided RTTI types)
// std
#include <typeinfo>


namespace re_std
{

// bad_typeid
//   class: re-export of std::bad_typeid (derives from std::exception).
// Thrown when typeid is applied to a null dereferenced glvalue of
// polymorphic type; what() returns an implementation-defined message.
using ::std::bad_typeid;

}  // re_std


#endif  // RE_STD_HAS_RTTI


#endif  // RE_STD_TYPEINFO_BAD_TYPEID_HPP
