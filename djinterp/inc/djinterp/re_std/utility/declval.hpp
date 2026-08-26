/***********************************************************************
* re_std                                                      declval.hpp
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
* re_std::declval on D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES.
*
*
* path:      /inc/djinterp/re_std/utility/declval.hpp
* link(s):   TBA
* author(s): re_std team                                date: 2026.04.30
***********************************************************************/

#ifndef DJINTERP_RE_STD_UTILITY_DECLVAL_
#define DJINTERP_RE_STD_UTILITY_DECLVAL_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "../type_traits/add_rvalue_reference.hpp"

NS_RESTD

// =============================================================================
// DECLVAL
// =============================================================================

// declval
//   function: declared-only -- never defined, never invokable. Used
//   inside unevaluated operands to obtain a value of type T without
//   requiring T to be default-constructible.
template<typename _Type>
typename add_rvalue_reference<_Type>::type declval() noexcept;

NS_END  // re_std

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // DJINTERP_RE_STD_UTILITY_DECLVAL_
