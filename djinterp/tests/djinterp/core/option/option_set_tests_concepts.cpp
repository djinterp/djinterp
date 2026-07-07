/******************************************************************************
* djinterp [test]                                option_set_tests_concepts.cpp
*
*   Section VII of the option_set.hpp suite: the C++20 concept analogs of the
* query traits.
*
*     Keyed<T>                       - T exposes the bare keyed shape
*                                      (::key_type alias + ::key member).
*     OptionSet<T>                   - T is some option_set<...> (parallels
*                                      is_option_set_v).
*     OptionSetContains<Set, Key>    - Set is an option_set containing Key.
*     OptionSetFindable<Set, Key>    - Set is an option_set whose find reports
*                                      found for Key (find vocabulary).
*     OptionSetNonEmpty<Set>         - Set is a non-empty option_set.
*
*   The whole section is gated on C++20 concepts; below that it is emitted empty
* (the traits in section VI remain the portable path), and the block reports a
* skipped descriptor so the runner still links and tallies cleanly.
*
*   Concepts are boolean, so each facet is pinned directly with static_assert.
* Note the deliberate negatives: Keyed rejects option_set itself (it exposes no
* ::key / ::key_type), and OptionSet rejects a bare option (an option is not a
* set).
*
*
* path:      /tests/djinterp/core/option/option_set_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_set_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// concept_keyed
//   Keyed holds for an option (it has ::key_type and ::key), and fails for a
// bare built-in and for option_set itself (which exposes neither).
bool
concept_keyed()
{
    constexpr bool ok =
        Keyed<option<os_key::a>>                                                &&
        Keyed<option<os_key::b, int>>                                           &&
        !Keyed<int>                                                             &&
        !Keyed<option_set<option<os_key::a>>>;

    static_assert(ok, "Keyed: options yes; int and option_set no");
    return ok;
}

// concept_option_set
//   OptionSet holds exactly for an option_set<...> specialization; not for an
// unrelated type, and not for a bare option.
bool
concept_option_set()
{
    constexpr bool ok =
        OptionSet<option_set<option<os_key::a>>>                                &&
        OptionSet<option_set<>>                                                 &&
        !OptionSet<int>                                                         &&
        !OptionSet<option<os_key::a>>;

    static_assert(ok, "OptionSet: option_set<...> yes; int and bare option no");
    return ok;
}

// concept_option_set_contains
//   OptionSetContains holds for a set / declared-key pair and fails for an
// absent key.
bool
concept_option_set_contains()
{
    using set = option_set<option<os_key::a>, option<os_key::b>>;

    constexpr bool ok =
        OptionSetContains<set, os_key::a>                                       &&
        OptionSetContains<set, os_key::b>                                       &&
        !OptionSetContains<set, os_key::c>;

    static_assert(ok, "OptionSetContains: declared keys yes, absent key no");
    return ok;
}

// concept_option_set_findable
//   OptionSetFindable mirrors contains in find vocabulary: satisfied when find
// reports found for the key, not otherwise.
bool
concept_option_set_findable()
{
    using set = option_set<option<os_key::a>, option<os_key::b>>;

    constexpr bool ok =
        OptionSetFindable<set, os_key::a>                                       &&
        OptionSetFindable<set, os_key::b>                                       &&
        !OptionSetFindable<set, os_key::c>;

    static_assert(ok, "OptionSetFindable: found keys yes, missing key no");
    return ok;
}

// concept_option_set_non_empty
//   OptionSetNonEmpty holds for a set with at least one option, and fails for
// the empty set and for a non-set.
bool
concept_option_set_non_empty()
{
    constexpr bool ok =
        OptionSetNonEmpty<option_set<option<os_key::a>>>                        &&
        !OptionSetNonEmpty<option_set<>>                                        &&
        !OptionSetNonEmpty<int>;

    static_assert(ok, "OptionSetNonEmpty: non-empty set yes; empty set and non-set no");
    return ok;
}

#endif  // C++20 concepts


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_concepts_block()
{
    ::djinterp::test::block_spec b;
    b.name = "VII. concepts (C++20)";
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.descriptor = "Keyed / OptionSet / OptionSetContains / OptionSetFindable / OptionSetNonEmpty";
    b.tests = {
        { "concept_keyed",
          "Keyed: options yes; int and option_set no",
          &concept_keyed },
        { "concept_option_set",
          "OptionSet: option_set<...> yes; int and bare option no",
          &concept_option_set },
        { "concept_option_set_contains",
          "OptionSetContains: declared keys yes, absent key no",
          &concept_option_set_contains },
        { "concept_option_set_findable",
          "OptionSetFindable: found keys yes, missing key no",
          &concept_option_set_findable },
        { "concept_option_set_non_empty",
          "OptionSetNonEmpty: non-empty set yes; empty set and non-set no",
          &concept_option_set_non_empty },
    };
#else
    b.descriptor = "skipped: concepts need C++20 (traits in section VI are the portable path)";
#endif
    return b;
}


NS_END  // testing
NS_END  // djinterp
