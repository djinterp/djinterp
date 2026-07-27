/******************************************************************************
* djinterp [container]                                    lifetime_concepts.hpp
*
*   C++20 concepts for the LIFETIME vocabulary -- the `requires`-facing view of
* meta/lifetime.hpp.
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
* path:      /inc/djinterp/core/container/concepts/lifetime_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.14
******************************************************************************/

#ifndef DJINTERP_META_LIFETIME_CONCEPTS_
#define DJINTERP_META_LIFETIME_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../../meta/lifetime.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHEN A VALUE IS FIXED
// ==========================================================================


// ConstexprLifetimeTyped
// concept: constexpr-capable -- its lifetime includes the compile-time stage.
// The meta-level predicate; ConstexprContainer is the container view.
D_CONCEPT_FROM_TRAIT(ConstexprLifetimeTyped, is_constexpr_lifetime_v)


// RuntimeOnlyLifetimeTyped
//   concept: the runtime stage EXCLUSIVELY -- not constant-evaluable.
D_CONCEPT_FROM_TRAIT(RuntimeOnlyLifetimeTyped, is_runtime_only_lifetime_v)


// DualLifetimeTyped
// concept: spans BOTH stages -- the literal-type case, constexpr-capable and
// usable at runtime. A fortiori: anything fixed at compile time is available at
// runtime, so this is the top of the lattice, not a third independent option.
D_CONCEPT_FROM_TRAIT(DualLifetimeTyped, is_dual_lifetime_v)


// ==========================================================================
//  SIGNALS
// ==========================================================================


// LiteralTyped
// concept: a literal type by the portable probe -- the general structural
// signal of constexpr-capability, beneath any opt-in.
D_CONCEPT_FROM_TRAIT(LiteralTyped, is_literal_type_v)


// DeclaresLifetimeCategory
//   concept: carries the static `lifetime_category` member -- the opt-in that
// outranks the structural probe.
D_CONCEPT_FROM_TRAIT(DeclaresLifetimeCategory, has_lifetime_category_v)

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_META_LIFETIME_CONCEPTS_
