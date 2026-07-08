/******************************************************************************
* djinterp [test]                         option_compose_tests_with_option.cpp
*
*   Section II of the option_compose.hpp suite: define-and-add for one surface.
*
*     internal::as_set          - wrap a lone surface as a one-element set for
*                                 the merge engine.
*     with_option_as_t<_Policy, _Set, _Key, _Args...>
*                               - define option<_Key, _Args...> and fold it into
*                                 _Set as the DELTA under _Policy.
*     with_option_t<_Set, _Key, _Args...>
*                               - the same under the default override_replace.
*
*   Coverage: the internal as_set wrap; adding to the empty set; adding a fresh
* key to a populated set; a key COLLISION under the default policy (the new
* surface's args replace the old, since override_replace == keep_delta); the
* explicit-policy form; and a collision under override_strict at an EXISTING key
* (strict forbids introducing NEW keys - a documented hard error - but an
* existing key may still be overridden).  as_set is internal:: and is reached
* through a TU-local alias.
*
*   (Whole file gated on C++20 concepts - see the suite header.)
*
*
* path:      /tests/djinterp/core/option/option_compose_tests_with_option.cpp
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

// with_option_as_set_internal
//   as_set wraps a single surface as a one-element option_set.
bool
with_option_as_set_internal()
{
    constexpr bool ok =
        std::is_same<ic::as_set_t<option<oc_key::a>>,
                     option_set<option<oc_key::a>>>::value;

    static_assert(ok, "as_set: a surface -> a one-element option_set");
    return ok;
}

// with_option_add_to_empty
//   adding a surface to the empty set yields the one-option set.
bool
with_option_add_to_empty()
{
    constexpr bool ok =
        std::is_same<with_option_t<option_set<>, oc_key::a>,
                     option_set<option<oc_key::a>>>::value;

    static_assert(ok, "with_option: add to option_set<> -> option_set<option<K>>");
    return ok;
}

// with_option_add_new_key
//   adding a fresh key extends the set, preserving the existing option.
bool
with_option_add_new_key()
{
    constexpr bool ok =
        std::is_same<with_option_t<option_set<option<oc_key::a>>, oc_key::b, oc_val<5>>,
                     option_set<option<oc_key::a>, option<oc_key::b, oc_val<5>>>>::value;

    static_assert(ok, "with_option: a fresh key extends the set");
    return ok;
}

// with_option_collision_replaces
//   under the default policy a colliding key takes the new surface's args (the
// old args are dropped entirely).
bool
with_option_collision_replaces()
{
    constexpr bool ok =
        std::is_same<with_option_t<option_set<option<oc_key::a, oc_val<1>>>, oc_key::a, oc_val<2>>,
                     option_set<option<oc_key::a, oc_val<2>>>>::value;

    static_assert(ok, "with_option: colliding key -> new args replace old (override_replace)");
    return ok;
}

// with_option_as_explicit_policy
//   the explicit-policy form with override_replace matches the default form.
bool
with_option_as_explicit_policy()
{
    constexpr bool ok =
        std::is_same<with_option_as_t<override_replace, option_set<>, oc_key::a, oc_val<5>>,
                     option_set<option<oc_key::a, oc_val<5>>>>::value;

    static_assert(ok, "with_option_as<override_replace>: matches the default form");
    return ok;
}

// with_option_as_strict_same_key
//   override_strict forbids introducing a NEW key (a documented hard error),
// but an EXISTING key may still be overridden.
bool
with_option_as_strict_same_key()
{
    constexpr bool ok =
        std::is_same<with_option_as_t<override_strict,
                                      option_set<option<oc_key::a, oc_val<1>>>, oc_key::a, oc_val<2>>,
                     option_set<option<oc_key::a, oc_val<2>>>>::value;

    static_assert(ok, "with_option_as<override_strict>: an existing key may be overridden");
    return ok;
}

#endif  // C++20 concepts


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_compose_with_option_block()
{
    ::djinterp::test::block_spec b;
    b.name = "II. with_option / with_option_as";
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.descriptor = "define + add one surface (as_set, add / extend / collide, explicit + strict policy)";
    b.tests = {
        { "with_option_as_set_internal",     "as_set: surface -> one-element set",               &with_option_as_set_internal },
        { "with_option_add_to_empty",        "add to option_set<> -> one-option set",            &with_option_add_to_empty },
        { "with_option_add_new_key",         "a fresh key extends the set",                      &with_option_add_new_key },
        { "with_option_collision_replaces",  "colliding key -> new args replace (default)",      &with_option_collision_replaces },
        { "with_option_as_explicit_policy",  "explicit override_replace matches default",        &with_option_as_explicit_policy },
        { "with_option_as_strict_same_key",  "override_strict: existing key may be overridden",  &with_option_as_strict_same_key },
    };
#else
    b.descriptor = "skipped: option_compose.hpp needs C++20 concepts";
#endif
    return b;
}


NS_END  // testing
NS_END  // djinterp
