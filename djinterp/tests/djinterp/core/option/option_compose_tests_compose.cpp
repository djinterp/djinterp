/******************************************************************************
* djinterp [test]                             option_compose_tests_compose.cpp
*
*   Section IV of the option_compose.hpp suite: build a set from empty.
*
*     compose_options_as_t<_Policy, _Surfaces...> / compose_options_t<...>
*         - fold _Surfaces... into option_set<> under _Policy (default
*           override_replace) - the from-scratch counterpart to with_options.
*
*   Since compose is with_options anchored at option_set<>, the tests focus on
* that anchoring: building a set from several surfaces; the empty pack (which
* yields the empty set); a same-key collision within the pack (later wins); the
* explicit-policy form; and the header's worked example (three defopt surfaces
* composed into the three-option set), which also confirms defopt and compose
* interoperate.
*
*   (Whole file gated on C++20 concepts - see the suite header.)
*
*
* path:      /tests/djinterp/core/option/option_compose_tests_compose.cpp
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

// compose_build_from_empty
//   several surfaces compose into the corresponding set, built from empty.
bool
compose_build_from_empty()
{
    constexpr bool ok =
        std::is_same<compose_options_t<option<oc_key::a>, option<oc_key::b, oc_val<5>>>,
                     option_set<option<oc_key::a>, option<oc_key::b, oc_val<5>>>>::value;

    static_assert(ok, "compose_options: build a set from surfaces");
    return ok;
}

// compose_empty
//   composing nothing yields the empty set.
bool
compose_empty()
{
    constexpr bool ok =
        std::is_same<compose_options_t<>, option_set<>>::value;

    static_assert(ok, "compose_options<> == option_set<>");
    return ok;
}

// compose_collision_later_wins
//   a same-key collision within the pack resolves left-to-right (later wins).
bool
compose_collision_later_wins()
{
    constexpr bool ok =
        std::is_same<compose_options_t<option<oc_key::a, oc_val<1>>, option<oc_key::a, oc_val<2>>>,
                     option_set<option<oc_key::a, oc_val<2>>>>::value;

    static_assert(ok, "compose_options: same-key collision - last wins");
    return ok;
}

// compose_as_explicit_policy
//   the explicit-policy form with override_replace matches the default form.
bool
compose_as_explicit_policy()
{
    constexpr bool ok =
        std::is_same<compose_options_as_t<override_replace, option<oc_key::a>, option<oc_key::b>>,
                     option_set<option<oc_key::a>, option<oc_key::b>>>::value;

    static_assert(ok, "compose_options_as<override_replace>: matches the default form");
    return ok;
}

// compose_doc_example
//   the header's worked example: three defopt surfaces composed into the
// three-option set (defopt and compose interoperate).
bool
compose_doc_example()
{
    using result = compose_options_t<
        defopt<oc_key::a, oc_val<1>>,
        defopt<oc_key::b, oc_val<2>>,
        defopt<oc_key::c, oc_val<3>>>;

    constexpr bool ok =
        std::is_same<result,
            option_set<option<oc_key::a, oc_val<1>>,
                       option<oc_key::b, oc_val<2>>,
                       option<oc_key::c, oc_val<3>>>>::value;

    static_assert(ok, "compose_options: documented three-defopt example");
    return ok;
}

#endif  // C++20 concepts


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_compose_compose_block()
{
    ::djinterp::test::block_spec b;
    b.name = "IV. compose_options / compose_options_as";
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.descriptor = "build a set from empty (build, empty, collision, explicit policy, doc example)";
    b.tests = {
        { "compose_build_from_empty",     "build a set from surfaces",                &compose_build_from_empty },
        { "compose_empty",                "compose_options<> == option_set<>",        &compose_empty },
        { "compose_collision_later_wins", "same-key collision - last wins",           &compose_collision_later_wins },
        { "compose_as_explicit_policy",   "explicit override_replace matches default",&compose_as_explicit_policy },
        { "compose_doc_example",          "documented three-defopt example",          &compose_doc_example },
    };
#else
    b.descriptor = "skipped: option_compose.hpp needs C++20 concepts";
#endif
    return b;
}


NS_END  // testing
NS_END  // djinterp
