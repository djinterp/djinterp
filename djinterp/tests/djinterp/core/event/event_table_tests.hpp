/*******************************************************************************
* djinterp [test]                                          event_table_tests.hpp
*
*   Declarations for the unit-test suite covering event_table.hpp.  Each free
* function exercises one entity of the erased-store header and returns true if
* every check inside it passed, false otherwise.  Tests are grouped into
* translation units by the section of event_table.hpp they cover:
*
*   - event_table_tests_storage.cpp     -> I.   type_key + handler_entry
*   - event_table_tests_core.cpp        -> III. insert / remove / mask / lookup
*   - event_table_tests_buckets.cpp     -> III. bucket access + iteration
*   - event_table_tests_merge_clear.cpp -> III. merge + clear + clear_for
*   - event_table_tests_stats.cpp       -> II + III. event_table_stats / stats
*   - event_table_tests_traits.cpp      -> IV + V. detection traits + facade
*   - event_table_tests_concepts.cpp    -> VI.  concept constraints (C++20+)
*
*   The lone shared check helper, event_table_check, reports a failing check
* (with its stringized expression and source location) and forwards the
* boolean so the calling test can fold it into a running result.  The
* D_ET_CHECK macro is the intended call site.
*
*   IMPORTANT (include order): event_table.hpp enforces that the djinterp
* framework header is included first (it #errors otherwise), so this test
* header includes djinterp/core/djinterp.hpp before event_table.hpp.  The
* section TUs include only this header and inherit that ordering.
*
*   The erased handlers stored in the table are plain free functions of type
* verdict(void*) (no generic lambdas, keeping the suite C++11-clean); bucket
* keys are modelled by a small unscoped enum that converts to std::size_t.
* The structural fixtures (core_table, almost_table, wrong_insert_table,
* not_a_table) drive the detection-trait and concept tests.
*
*   NOTE: the entities under test live in djinterp (and djinterp::internal);
* the tests themselves live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/core/event/event_table_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_EVENT_TABLE_TESTS_
#define DJINTERP_EVENT_TABLE_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <functional>
#include <vector>
// djinterp
// djinterp  -- framework header FIRST (event_table.hpp requires it), then the
// header under test.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/core/event/event_table.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: enriched authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


// =========================================================================
//  shared check helper
// =========================================================================

// event_table_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
event_table_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// =========================================================================
//  shared fixtures -- bucket keys and erased handlers
// =========================================================================

// table_key
//   fixture: distinct bucket keys.  Unscoped so each enumerator converts
// implicitly to the std::size_t the table API expects.
enum table_key : std::size_t
{
    KEY_A = 0xA1,
    KEY_B = 0xB2,
    KEY_C = 0xC3
};

// ret_pass / ret_consume
//   fixture: distinguishable erased handlers.  The table never invokes these
// itself; the tests call them through stored entries to confirm the callable
// was stored (and copied, after merge) faithfully.  The void* payload pointer
// is unused by these handlers.
inline verdict
ret_pass(void*)
{
    return verdict::pass;
}

inline verdict
ret_consume(void*)
{
    return verdict::consume;
}


// =========================================================================
//  shared fixtures -- types for the erasure key (kappa)
// =========================================================================

// tk_a / tk_b
//   fixture: two distinct types used to confirm type_key is stable per type
// and distinct across types.
struct tk_a
{
};

struct tk_b
{
};


// =========================================================================
//  shared fixtures -- iteration visitors
// =========================================================================

// visit_counter
//   fixture: counts every (type_key, entry) pair for_each_entry yields.
struct visit_counter
{
    int* n;

    void operator()(std::size_t, const internal::handler_entry&) const
    {
        if (n)
        {
            ++*n;
        }
    }
};

// enabled_visit_counter
//   fixture: counts only the enabled entries for_each_entry yields.
struct enabled_visit_counter
{
    int* n;

    void operator()(std::size_t, const internal::handler_entry& _e) const
    {
        if (n && _e.enabled)
        {
            ++*n;
        }
    }
};

// bucket_visit_counter
//   fixture: counts every entry for_each_entry_for yields for one bucket.
struct bucket_visit_counter
{
    int* n;

    void operator()(const internal::handler_entry&) const
    {
        if (n)
        {
            ++*n;
        }
    }
};


// =========================================================================
//  shared fixtures -- structural fakes for detection / concepts
// =========================================================================

// not_a_table
//   fixture: a type providing none of the table interface.
struct not_a_table
{
};

// core_table
//   fixture: a distinct type providing the full required core interface (the
// eleven operations is_event_table demands) but NONE of the optional
// extensions (type_key_count / get_stats / clear_for / merge).  Confirms the
// detection is structural (duck-typed, not nominal) and separates the core
// concept from the optional-feature concepts.
struct core_table
{
    handler_id insert(std::size_t, std::function<verdict(void*)>)
    {
        return handler_id::null();
    }

    bool remove(handler_id)             { return false; }
    bool enable(handler_id)             { return false; }
    bool disable(handler_id)            { return false; }
    bool is_enabled(handler_id) const   { return false; }
    bool contains(handler_id) const     { return false; }
    std::size_t count_for(std::size_t) const      { return 0; }
    bool has_entries_for(std::size_t) const       { return false; }
    std::size_t total_count() const     { return 0; }
    std::size_t enabled_count() const   { return 0; }
    void clear()                        {}
};

// almost_table
//   fixture: the core interface minus clear(), so is_event_table is false --
// confirming the composite requires every core operation.
struct almost_table
{
    handler_id insert(std::size_t, std::function<verdict(void*)>)
    {
        return handler_id::null();
    }

    bool remove(handler_id)             { return false; }
    bool enable(handler_id)             { return false; }
    bool disable(handler_id)            { return false; }
    bool is_enabled(handler_id) const   { return false; }
    bool contains(handler_id) const     { return false; }
    std::size_t count_for(std::size_t) const      { return 0; }
    bool has_entries_for(std::size_t) const       { return false; }
    std::size_t total_count() const     { return 0; }
    std::size_t enabled_count() const   { return 0; }
};

// wrong_insert_table
//   fixture: insert is present but returns the wrong type, so detection must
// reject it on the return-type (is_same) gate, not merely on existence.
struct wrong_insert_table
{
    int insert(std::size_t, std::function<verdict(void*)>)
    {
        return 0;
    }
};


// =========================================================================
//  test declarations
// =========================================================================

#endif  // !DTEST_SPEC_MODE (fixtures + check helper)

// I.   INTERNAL STORAGE TYPES (type_key, handler_entry)
bool tests_type_key_stable_unique();
bool tests_handler_entry_fields();

// III. EVENT TABLE -- core: insert / remove / mask / lookup
bool tests_table_default_empty();
bool tests_insert_ids_increasing();
bool tests_insert_counts_enabled_contains();
bool tests_remove_basic();
bool tests_remove_disabled_bookkeeping();
bool tests_id_non_reuse();
bool tests_enable_disable_flips_and_counts();
bool tests_enable_disable_idempotent_and_missing();
bool tests_is_enabled_states();
bool tests_contains_states();

// III. EVENT TABLE -- bucket access + iteration
bool tests_entries_for_missing_null();
bool tests_entries_for_present_contents();
bool tests_has_entries_for();
bool tests_count_for();
bool tests_type_key_count();
bool tests_for_each_entry();
bool tests_for_each_entry_for();

// III. EVENT TABLE -- merge + clear + clear_for
bool tests_merge_into_empty();
bool tests_merge_order_and_fresh_ids();
bool tests_merge_preserves_enabled();
bool tests_merge_identity_and_src_untouched();
bool tests_clear();
bool tests_clear_for();
bool tests_clear_for_missing_noop();
bool tests_clear_for_mixed_enabled();

// II + III. event_table_stats + get_stats
bool tests_stats_struct_aggregate();
bool tests_stats_empty_table();
bool tests_stats_populated();
bool tests_stats_after_clear();

// IV + V. structural detection traits + event_table_traits facade
bool tests_detection_core_mutation();
bool tests_detection_const_query();
bool tests_detection_extended();
bool tests_traits_facade_real();
bool tests_traits_facade_structural();
bool tests_traits_facade_negative();
bool tests_traits_facade_cvref();

// VI.  CONCEPT CONSTRAINTS (C++20+; vacuous pass where concepts are absent)
bool tests_concept_is_event_table_type();
bool tests_concept_readable_and_non();
bool tests_concept_clearable_counting();
bool tests_concept_optional_features();
bool tests_concept_extended();
bool tests_concept_derived_false_on_non_table();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Mirrors this suite as data - each unit test paired with its name and a
//  one-line descriptor - so run_module / run_suite can lower it into the
//  six-kind tree and drive the report.  References only the test_fn_ptr
//  declarations above; no fixtures required, which is what lets the
//  aggregate runner include every suite header in one TU.
// =========================================================================

namespace dt = ::djinterp::test;

inline dt::module_spec
event_table_spec()
{
    return dt::module_spec{
        "event_table",
        "Type-erased handler storage: keys, entries, mask, iteration, "
        "merge, statistics, structural traits, and concepts.",
        {
            dt::block_spec{
                "storage",
                "The type key and the handler_entry record.",
                {
                    { "type_key_stable_unique",
                      "type_key is stable per type and unique across types",
                      &tests_type_key_stable_unique },
                    { "handler_entry_fields",
                      "a handler_entry carries its id, callable, and enabled flag",
                      &tests_handler_entry_fields },
                }
            },
            dt::block_spec{
                "core",
                "The mutating core: insert, remove, enable/disable, ids.",
                {
                    { "table_default_empty",
                      "a fresh table is empty with zero entries",
                      &tests_table_default_empty },
                    { "insert_ids_increasing",
                      "insert hands out strictly increasing ids",
                      &tests_insert_ids_increasing },
                    { "insert_counts_enabled_contains",
                      "insert updates count, enabled count, and contains",
                      &tests_insert_counts_enabled_contains },
                    { "remove_basic",
                      "remove deletes an entry by id",
                      &tests_remove_basic },
                    { "remove_disabled_bookkeeping",
                      "removing a disabled entry keeps the counts consistent",
                      &tests_remove_disabled_bookkeeping },
                    { "id_non_reuse",
                      "removed ids are never reused",
                      &tests_id_non_reuse },
                    { "enable_disable_flips_and_counts",
                      "enable/disable flips the flag and the enabled count",
                      &tests_enable_disable_flips_and_counts },
                    { "enable_disable_idempotent_and_missing",
                      "enable/disable is idempotent and safe on missing ids",
                      &tests_enable_disable_idempotent_and_missing },
                    { "is_enabled_states",
                      "is_enabled reports the entry's current flag",
                      &tests_is_enabled_states },
                    { "contains_states",
                      "contains reflects presence across insert / remove",
                      &tests_contains_states },
                }
            },
            dt::block_spec{
                "buckets",
                "Per-type buckets: lookup, counts, and iteration.",
                {
                    { "type_key_count",
                      "type_key_count counts the distinct event types held",
                      &tests_type_key_count },
                    { "entries_for_missing_null",
                      "entries_for returns null for an absent type",
                      &tests_entries_for_missing_null },
                    { "entries_for_present_contents",
                      "entries_for returns the bucket for a present type",
                      &tests_entries_for_present_contents },
                    { "has_entries_for",
                      "has_entries_for reflects whether a type has a bucket",
                      &tests_has_entries_for },
                    { "count_for",
                      "count_for counts the entries under one type",
                      &tests_count_for },
                    { "for_each_entry",
                      "for_each_entry visits every entry once",
                      &tests_for_each_entry },
                    { "for_each_entry_for",
                      "for_each_entry_for visits one type's entries",
                      &tests_for_each_entry_for },
                }
            },
            dt::block_spec{
                "merge_clear",
                "Merging tables and clearing all or one type.",
                {
                    { "merge_into_empty",
                      "merging into an empty table copies the source entries",
                      &tests_merge_into_empty },
                    { "merge_order_and_fresh_ids",
                      "merge preserves order and assigns fresh ids",
                      &tests_merge_order_and_fresh_ids },
                    { "merge_preserves_enabled",
                      "merge carries each entry's enabled state",
                      &tests_merge_preserves_enabled },
                    { "merge_identity_and_src_untouched",
                      "merge leaves the source table unchanged",
                      &tests_merge_identity_and_src_untouched },
                    { "clear",
                      "clear empties the whole table",
                      &tests_clear },
                    { "clear_for",
                      "clear_for removes one type's entries",
                      &tests_clear_for },
                    { "clear_for_missing_noop",
                      "clear_for on an absent type is a no-op",
                      &tests_clear_for_missing_noop },
                    { "clear_for_mixed_enabled",
                      "clear_for updates counts across mixed enabled states",
                      &tests_clear_for_mixed_enabled },
                }
            },
            dt::block_spec{
                "stats",
                "The aggregate statistics snapshot.",
                {
                    { "stats_struct_aggregate",
                      "the stats struct is a plain aggregate of counts",
                      &tests_stats_struct_aggregate },
                    { "stats_empty_table",
                      "stats of an empty table are all zero",
                      &tests_stats_empty_table },
                    { "stats_populated",
                      "stats report totals, enabled, and type counts",
                      &tests_stats_populated },
                    { "stats_after_clear",
                      "stats return to zero after clear",
                      &tests_stats_after_clear },
                }
            },
            dt::block_spec{
                "traits",
                "Capability detection and the event_table_traits facade.",
                {
                    { "detection_core_mutation",
                      "detects the core mutating surface of a table",
                      &tests_detection_core_mutation },
                    { "detection_const_query",
                      "detects the const query surface of a table",
                      &tests_detection_const_query },
                    { "detection_extended",
                      "detects the optional extended surface",
                      &tests_detection_extended },
                    { "traits_facade_real",
                      "event_table_traits reports a real table's capabilities",
                      &tests_traits_facade_real },
                    { "traits_facade_structural",
                      "the traits facade accepts a structural duck-typed table",
                      &tests_traits_facade_structural },
                    { "traits_facade_negative",
                      "the traits facade rejects a non-table",
                      &tests_traits_facade_negative },
                    { "traits_facade_cvref",
                      "the traits facade strips cv/ref before inspecting",
                      &tests_traits_facade_cvref },
                }
            },
            dt::block_spec{
                "concepts",
                "The C++20 event_table concept layer.",
                {
                    { "concept_is_event_table_type",
                      "is_event_table_type holds for tables",
                      &tests_concept_is_event_table_type },
                    { "concept_readable_and_non",
                      "separates readable tables from non-tables",
                      &tests_concept_readable_and_non },
                    { "concept_clearable_counting",
                      "the clearable / counting concepts match those surfaces",
                      &tests_concept_clearable_counting },
                    { "concept_optional_features",
                      "optional-feature concepts detect extended tables",
                      &tests_concept_optional_features },
                    { "concept_extended",
                      "the extended-table concept matches the full surface",
                      &tests_concept_extended },
                    { "concept_derived_false_on_non_table",
                      "table concepts are false for non-tables",
                      &tests_concept_derived_false_on_non_table },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_ET_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through event_table_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas (e.g.
// event_table_traits<T>::is_event_table inside a larger expression, or a
// concept-id with multiple arguments) need no defensive parentheses.
#define D_ET_CHECK(...)                                                       \
    ::djinterp::testing::event_table_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_EVENT_TABLE_TESTS_
