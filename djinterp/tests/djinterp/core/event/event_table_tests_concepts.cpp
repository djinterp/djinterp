/******************************************************************************
* djinterp [test]                               event_table_tests_concepts.cpp
*
*   Section VI (concept constraints, C++20+) -- the ten concepts layered over
* event_table_traits, each exercised as a boolean.  The core concept
* (is_event_table_type and its readable alias event_table_type, plus the
* negation non_event_table_type) is checked on the real table, a structurally
* conforming type, and a non-table.  The feature concepts (clearable, counting,
* type_key_counting, stats, selectively_clearable, mergeable, extended) are
* checked true on the real table and -- crucially -- false on the core-only
* type, which is an event table yet exposes none of the optional features; and
* all derived concepts are confirmed to short-circuit to false on a non-table.
*
*   The whole suite is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS; where concepts
* are unavailable (pre-C++20) each test is a vacuous pass, mirroring the
* header's own gating so the runner's call sites stay identical across
* standards.
*
*
* path:      /tests/djinterp/core/event/event_table/event_table_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_concept_is_event_table_type
bool
tests_concept_is_event_table_type()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // satisfied by the real table and by a structurally conforming type;
    // not satisfied by a bare struct.
    ok = D_ET_CHECK(is_event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(is_event_table_type<core_table>) && ok;
    ok = D_ET_CHECK(!is_event_table_type<not_a_table>) && ok;

    return ok;
#else
    return true;
#endif
}


// tests_concept_readable_and_non
bool
tests_concept_readable_and_non()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // event_table_type is the readable spelling of is_event_table_type.
    ok = D_ET_CHECK(event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(!event_table_type<not_a_table>) && ok;

    // non_event_table_type is its exact negation.
    ok = D_ET_CHECK(non_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!non_event_table_type<event_table>) && ok;

    return ok;
#else
    return true;
#endif
}


// tests_concept_clearable_counting
bool
tests_concept_clearable_counting()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // both refine event_table_type with detections the core type also has, so
    // both the real table and the core-only type satisfy them.
    ok = D_ET_CHECK(clearable_event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(clearable_event_table_type<core_table>) && ok;

    ok = D_ET_CHECK(counting_event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(counting_event_table_type<core_table>) && ok;

    // neither is satisfied by a non-table.
    ok = D_ET_CHECK(!clearable_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!counting_event_table_type<not_a_table>) && ok;

    return ok;
#else
    return true;
#endif
}


// tests_concept_optional_features
bool
tests_concept_optional_features()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // the real table exposes every optional feature.
    ok = D_ET_CHECK(type_key_counting_event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(stats_event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(selectively_clearable_event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(mergeable_event_table_type<event_table>) && ok;

    // the core-only type is a valid event table but exposes NONE of them, so
    // each optional-feature concept rejects it even though event_table_type
    // accepts it.
    ok = D_ET_CHECK(!type_key_counting_event_table_type<core_table>) && ok;
    ok = D_ET_CHECK(!stats_event_table_type<core_table>) && ok;
    ok = D_ET_CHECK(!selectively_clearable_event_table_type<core_table>) && ok;
    ok = D_ET_CHECK(!mergeable_event_table_type<core_table>) && ok;

    return ok;
#else
    return true;
#endif
}


// tests_concept_extended
bool
tests_concept_extended()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // extended requires all four optional features at once: the real table
    // qualifies, the core-only type does not.
    ok = D_ET_CHECK(extended_event_table_type<event_table>) && ok;
    ok = D_ET_CHECK(!extended_event_table_type<core_table>) && ok;
    ok = D_ET_CHECK(!extended_event_table_type<not_a_table>) && ok;

    return ok;
#else
    return true;
#endif
}


// tests_concept_derived_false_on_non_table
bool
tests_concept_derived_false_on_non_table()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    bool ok = true;

    // every derived concept leads with event_table_type, so all of them
    // short-circuit to false on a type that is not an event table at all.
    ok = D_ET_CHECK(!clearable_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!counting_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!type_key_counting_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!stats_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!selectively_clearable_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!mergeable_event_table_type<not_a_table>) && ok;
    ok = D_ET_CHECK(!extended_event_table_type<not_a_table>) && ok;

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
