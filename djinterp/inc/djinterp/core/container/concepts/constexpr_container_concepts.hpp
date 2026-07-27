/******************************************************************************
* djinterp [container] constexpr_container_concepts.hpp C++20 concepts for the
* LIFETIME (compile-time end) axis -- the `requires`-facing view of
* constexpr_container_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth; if a
* classification is wrong, it is wrong in one place. That is the whole point of
* generating these rather than restating the detection logic in `requires`
* clauses. PORTABILITY: Gated on C++20 + concepts. Below that the header is
* empty and callers use the `::value` / `_v` forms directly -- which is why
* nothing else in the framework is allowed to depend on these. path:
* /inc/djinterp/core/container/concepts/constexpr_container_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_CONSTEXPR_CONTAINER_CONCEPTS_
#define DJINTERP_CONSTEXPR_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../traits/constexpr_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  LIFETIME
// ==========================================================================


// constexpr_container
// concept: usable in constant evaluation -- BOTH the size and every component
// value are fixed by the program text. That conjunction is the lattice MEET of
// the two lifetimes, not a coincidence of two checks.
template<typename _Type>
concept constexpr_container =
    is_constexpr_container_v<clean_t<_Type>>;


// runtime_container
// concept: the explicit negation -- a DYNAMIC lifetime. Spelled out so a pair
// of requires-clauses can be disjoint rather than one being `!`.
template<typename _Type>
concept runtime_container =
    is_not_constexpr_container_v<clean_t<_Type>>;


// ==========================================================================
//  SIGNALS
// ==========================================================================


// constexpr_tagged_container
// concept: carries the opt-in `is_constexpr_container` alias -- an explicit
// claim of static-lifetime capability, which outranks the structural probes.
template<typename _Type>
concept constexpr_tagged_container =
    has_constexpr_container_tag_v<clean_t<_Type>>;


// compile_time_sized_container
// concept: its SIZE is compile-time-expressible, whether or not its CONTENTS
// are. array<std::string, N> is this and is not constexpr_container -- which is
// exactly why the two are separate concepts.
template<typename _Type>
concept compile_time_sized_container =
    is_compile_time(size_lifetime<clean_t<_Type>>::value);

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_CONSTEXPR_CONTAINER_CONCEPTS_
