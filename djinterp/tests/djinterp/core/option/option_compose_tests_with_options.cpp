/******************************************************************************
* djinterp [test]                        option_compose_tests_with_options.cpp
*
*   Section III of the option_compose.hpp suite: fold a whole pack of surfaces.
*
*     internal::as_delta_set    - normalize one fold input: a surface becomes a
*                                 one-element set; an option_set passes through.
*     internal::with_options_fold - the left fold of the deltas into an
*                                 accumulator under a policy.
*     with_options_as_t<_Policy, _Base, _Surfaces...>  / with_options_t<...>
*                               - fold a pack of surfaces (and/or sub-sets) into
*                                 _Base, left to right (default override_replace).
*
*   Coverage: the internal as_delta_set for both input kinds; folding several
* surfaces onto a base; a MIXED pack of a surface and an option_set sub-set
* (uniformly normalized); a collision against the base (later wins); two
* same-key surfaces WITHIN the pack (the last wins, confirming true left-to-right
* order); the empty pack (the base is returned unchanged - the fold's identity
* case); and the explicit-policy form.  Internal traits are reached through a
* TU-local alias.
*
*   (Whole file gated on C++20 concepts - see the suite header.)
*
*
* path:      /tests/djinterp/core/option/option_compose_tests_with_options.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_compose_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace  // internal-helper alias, local to this TU
{
    namespace ic = ::djinterp::internal;
}

// with_options_as_delta_set_internal
//   as_delta_set normalizes a surface to a one-element set and leaves an
// existing option_set untouched.
bool
with_options_as_delta_set_internal()
{
    constexpr bool ok =
        std::is_same<ic::as_delta_set_t<option<oc_key::a>>,
                     option_set<option<oc_key::a>>>::value                      &&
        std::is_same<ic::as_delta_set_t<option_set<option<oc_key::a>, option<oc_key::b>>>,
                     option_set<option<oc_key::a>, option<oc_key::b>>>::value;

    static_assert(ok, "as_delta_set: surface -> one-element set; option_set passes through");
    return ok;
}

// with_options_fold_surfaces
//   several surfaces fold onto a base in order.
bool
with_options_fold_surfaces()
{
    constexpr bool ok =
        std::is_same<with_options_t<option_set<>, option<oc_key::a>, option<oc_key::b, oc_val<5>>>,
                     option_set<option<oc_key::a>, option<oc_key::b, oc_val<5>>>>::value;

    static_assert(ok, "with_options: fold several surfaces onto a base");
    return ok;
}

// with_options_mixed_surface_and_subset
//   a pack mixing a surface with an option_set sub-set is normalized uniformly:
// the sub-set's options are merged in.
bool
with_options_mixed_surface_and_subset()
{
    constexpr bool ok =
        std::is_same<
            with_options_t<option_set<>, option<oc_key::a>,
                           option_set<option<oc_key::b>, option<oc_key::c>>>,
            option_set<option<oc_key::a>, option<oc_key::b>, option<oc_key::c>>>::value;

    static_assert(ok, "with_options: a mixed pack of a surface and a sub-set merges uniformly");
    return ok;
}

// with_options_collision_later_wins
//   a surface colliding with the base overrides it (later wins).
bool
with_options_collision_later_wins()
{
    constexpr bool ok =
        std::is_same<with_options_t<option_set<option<oc_key::a, oc_val<1>>>, option<oc_key::a, oc_val<2>>>,
                     option_set<option<oc_key::a, oc_val<2>>>>::value;

    static_assert(ok, "with_options: a colliding surface overrides the base");
    return ok;
}

// with_options_pack_same_key_last_wins
//   two same-key surfaces within the pack resolve left-to-right, so the last
// one wins - confirming the fold order.
bool
with_options_pack_same_key_last_wins()
{
    constexpr bool ok =
        std::is_same<with_options_t<option_set<>, option<oc_key::a, oc_val<1>>, option<oc_key::a, oc_val<2>>>,
                     option_set<option<oc_key::a, oc_val<2>>>>::value;

    static_assert(ok, "with_options: two same-key surfaces in the pack - last wins (left-to-right)");
    return ok;
}

// with_options_empty_pack_is_base
//   folding an empty pack returns the base unchanged (the fold identity).
bool
with_options_empty_pack_is_base()
{
    constexpr bool ok =
        std::is_same<with_options_t<option_set<option<oc_key::a>>>,
                     option_set<option<oc_key::a>>>::value;

    static_assert(ok, "with_options: an empty pack yields the base unchanged");
    return ok;
}

// with_options_as_explicit_policy
//   the explicit-policy form with override_replace matches the default form.
bool
with_options_as_explicit_policy()
{
    constexpr bool ok =
        std::is_same<
            with_options_as_t<override_replace, option_set<>, option<oc_key::a>, option<oc_key::b>>,
            option_set<option<oc_key::a>, option<oc_key::b>>>::value;

    static_assert(ok, "with_options_as<override_replace>: matches the default form");
    return ok;
}

#endif  // C++20 concepts


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_compose_with_options_block()
{
    ::djinterp::test::block_spec b;
    b.name = "III. with_options / with_options_as";
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.descriptor = "fold a pack of surfaces/sub-sets (as_delta_set, order, collisions, empty, policy)";
    b.tests = {
        { "with_options_as_delta_set_internal",   "as_delta_set: surface vs option_set",             &with_options_as_delta_set_internal },
        { "with_options_fold_surfaces",           "fold several surfaces onto a base",               &with_options_fold_surfaces },
        { "with_options_mixed_surface_and_subset","a mixed surface + sub-set pack merges uniformly", &with_options_mixed_surface_and_subset },
        { "with_options_collision_later_wins",    "a colliding surface overrides the base",          &with_options_collision_later_wins },
        { "with_options_pack_same_key_last_wins", "two same-key surfaces - last wins",               &with_options_pack_same_key_last_wins },
        { "with_options_empty_pack_is_base",      "an empty pack yields the base unchanged",         &with_options_empty_pack_is_base },
        { "with_options_as_explicit_policy",      "explicit override_replace matches default",       &with_options_as_explicit_policy },
    };
#else
    b.descriptor = "skipped: option_compose.hpp needs C++20 concepts";
#endif
    return b;
}


NS_END  // testing
NS_END  // djinterp
