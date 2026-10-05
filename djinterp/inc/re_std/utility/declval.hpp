/*******************************************************************************
* djinterp [re_std]                                                  declval.hpp
*
* unevaluated-context value utility:
*   Provides re_std::declval<T>(), a declared-only function template
* that returns an "instance" of T usable inside unevaluated contexts
* (decltype, sizeof, noexcept). Never invoked at runtime; calling
* declval is ill-formed.
*
*   The return type is add_rvalue_reference<T>::type, which yields
* T&& for referenceable types and T (unchanged) for cv-qualified
* `void`. This means declval<void>() is well-formed and yields a
* prvalue of type void, matching the standard library.
*
*   Requires rvalue references (C++11+). On standards without rvalue
* references, no symbol is defined; callers must gate their use of
* re_std::declval on RE_STD_LANG_HAS_RVALUE_REFERENCES.
*
*
* path:      /inc/re_std/utility/declval.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_DECLVAL_HPP
#define RE_STD_UTILITY_DECLVAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "../type_traits/add_rvalue_reference.hpp"

namespace re_std
{

// =============================================================================
// DECLVAL
// =============================================================================

// declval
//   function: declared-only -- never defined, never invokable. Used
//   inside unevaluated operands to obtain a value of type T without
//   requiring T to be default-constructible.
template<typename Type>
typename add_rvalue_reference<Type>::type declval() noexcept;

}  // re_std

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_UTILITY_DECLVAL_HPP
