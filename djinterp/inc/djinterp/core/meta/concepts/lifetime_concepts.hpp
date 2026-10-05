/*******************************************************************************
* djinterp [core]                                          lifetime_concepts.hpp
*
*   C++20 concepts for the LIFETIME vocabulary -- the `requires`-facing view of
* {traits}.
*
*   THE CONCEPTS ADD NO POLICY.  Each is exactly its trait, spelled so it can
* constrain a template instead of gating one through enable_if.  The trait stays
* the single source of truth.
*
*   NAMES.  meta/concepts.hpp already owns the general type-level concepts (the
* `_c` family), and constexpr_iterator_concepts.hpp the constexpr-iteration
* ones; neither is duplicated here.  Where an obvious name is otherwise taken,
* the concept takes a form that cannot collide -- a concept and a class of one
* name in one namespace is a hard redeclaration.
*
*   PORTABILITY:
*   Gated on C++20 + concepts.  Below that the header is empty and callers use
* the `::value` / `_v` forms directly.
*
*
* path:      /inc/djinterp/core/meta/concepts/lifetime_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_CONCEPTS_LIFETIME_CONCEPTS_HPP
#define DJINTERP_META_CONCEPTS_LIFETIME_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../type_utility.hpp"  // clean_t
#include "../lifetime.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHEN A VALUE IS FIXED
// ==========================================================================


// ConstexprLifetimeTyped
// concept: constexpr-capable -- its lifetime includes the compile-time stage.
// The meta-level predicate; ConstexprContainer is the container view.
template<typename Type>
concept ConstexprLifetimeTyped =
    is_constexpr_lifetime_v<clean_t<Type>>;


// RuntimeOnlyLifetimeTyped
//   concept: the runtime stage EXCLUSIVELY -- not constant-evaluable.
template<typename Type>
concept RuntimeOnlyLifetimeTyped =
    is_runtime_only_lifetime_v<clean_t<Type>>;


// DualLifetimeTyped
// concept: spans BOTH stages -- the literal-type case, constexpr-capable and
// usable at runtime. A fortiori: anything fixed at compile time is available at
// runtime, so this is the top of the lattice, not a third independent option.
template<typename Type>
concept DualLifetimeTyped =
    is_dual_lifetime_v<clean_t<Type>>;


// ==========================================================================
//  SIGNALS
// ==========================================================================


// LiteralTyped
// concept: a literal type by the portable probe -- the general structural
// signal of constexpr-capability, beneath any opt-in.
template<typename Type>
concept LiteralTyped =
    is_literal_type_v<clean_t<Type>>;


// DeclaresLifetimeCategory
//   concept: carries the static `lifetime_category` member -- the opt-in that
// outranks the structural probe.
template<typename Type>
concept DeclaresLifetimeCategory =
    has_lifetime_category_v<clean_t<Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now


#endif  // DJINTERP_META_CONCEPTS_LIFETIME_CONCEPTS_HPP
