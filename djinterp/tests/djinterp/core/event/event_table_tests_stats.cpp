/******************************************************************************
* djinterp [test]                                   event_table_tests_stats.cpp
*
*   Sections II + III (statistics) -- the event_table_stats aggregate and the
* get_stats snapshot that fills it.  The deterministic fields are asserted
* exactly: total_entries, enabled_entries (which tracks the mask),
* event_type_count (distinct keys), max_entries_per_type (the largest bucket),
* and average_entries_per_type (total / keys, compared within a small epsilon).
* The unordered_map-implementation-dependent fields (total_buckets,
* used_buckets, load_factor) are checked only against their invariants, since
* their exact values are not part of the contract.  Empty-table and
* after-clear snapshots are verified to read all-zero.
*
*
* path:      /tests/djinterp/core/event/event_table/event_table_tests_stats.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_stats_struct_aggregate
bool
tests_stats_struct_aggregate()
{
    bool ok = true;

    // event_table_stats is a plain aggregate: every field is independently
    // assignable and reads back what was written.
    event_table_stats s;
    s.total_buckets             = 16;
    s.used_buckets              = 3;
    s.total_entries             = 9;
    s.enabled_entries           = 7;
    s.event_type_count          = 4;
    s.max_entries_per_type      = 5;
    s.average_entries_per_type  = 2.25;
    s.load_factor               = 0.5;

    ok = D_ET_CHECK(s.total_buckets == 16u) && ok;
    ok = D_ET_CHECK(s.used_buckets == 3u) && ok;
    ok = D_ET_CHECK(s.total_entries == 9u) && ok;
    ok = D_ET_CHECK(s.enabled_entries == 7u) && ok;
    ok = D_ET_CHECK(s.event_type_count == 4u) && ok;
    ok = D_ET_CHECK(s.max_entries_per_type == 5u) && ok;
    ok = D_ET_CHECK(s.average_entries_per_type > 2.249 &&
                    s.average_entries_per_type < 2.251) && ok;
    ok = D_ET_CHECK(s.load_factor > 0.499 && s.load_factor < 0.501) && ok;

    return ok;
}


// tests_stats_empty_table
bool
tests_stats_empty_table()
{
    bool ok = true;

    // an empty table reports zeroed entry metrics and a zero average; the
    // bucket-related fields, while implementation-defined, must satisfy their
    // invariants.
    event_table t;
    event_table_stats s = t.get_stats();

    ok = D_ET_CHECK(s.total_entries == 0u) && ok;
    ok = D_ET_CHECK(s.enabled_entries == 0u) && ok;
    ok = D_ET_CHECK(s.event_type_count == 0u) && ok;
    ok = D_ET_CHECK(s.max_entries_per_type == 0u) && ok;
    ok = D_ET_CHECK(s.average_entries_per_type == 0.0) && ok;
    ok = D_ET_CHECK(s.used_buckets == 0u) && ok;           // no map entries
    ok = D_ET_CHECK(s.used_buckets <= s.total_buckets) && ok;
    ok = D_ET_CHECK(s.load_factor >= 0.0) && ok;

    return ok;
}


// tests_stats_populated
bool
tests_stats_populated()
{
    bool ok = true;

    // KEY_A: 3 entries (one disabled); KEY_B: 1 entry. Totals: 4 entries,
    // 3 enabled, 2 keys, max bucket 3, average 4/2 = 2.0.
    event_table t;
    t.insert(KEY_A, &ret_pass);
    handler_id mid = t.insert(KEY_A, &ret_pass);
    t.insert(KEY_A, &ret_pass);
    t.disable(mid);
    t.insert(KEY_B, &ret_pass);

    event_table_stats s = t.get_stats();

    // deterministic fields -- asserted exactly.
    ok = D_ET_CHECK(s.total_entries == 4u) && ok;
    ok = D_ET_CHECK(s.enabled_entries == 3u) && ok;
    ok = D_ET_CHECK(s.event_type_count == 2u) && ok;
    ok = D_ET_CHECK(s.max_entries_per_type == 3u) && ok;
    ok = D_ET_CHECK(s.average_entries_per_type > 1.999 &&
                    s.average_entries_per_type < 2.001) && ok;

    // these mirror the table's own accessors.
    ok = D_ET_CHECK(s.total_entries == t.total_count()) && ok;
    ok = D_ET_CHECK(s.enabled_entries == t.enabled_count()) && ok;
    ok = D_ET_CHECK(s.event_type_count == t.type_key_count()) && ok;

    // implementation-dependent fields -- invariants only.
    ok = D_ET_CHECK(s.used_buckets >= 1u) && ok;
    ok = D_ET_CHECK(s.used_buckets <= s.event_type_count) && ok;
    ok = D_ET_CHECK(s.total_buckets >= s.used_buckets) && ok;
    ok = D_ET_CHECK(s.load_factor >= 0.0) && ok;

    return ok;
}


// tests_stats_after_clear
bool
tests_stats_after_clear()
{
    bool ok = true;

    // after clearing, the snapshot reads empty again.
    event_table t;
    t.insert(KEY_A, &ret_pass);
    t.insert(KEY_B, &ret_pass);
    t.clear();

    event_table_stats s = t.get_stats();
    ok = D_ET_CHECK(s.total_entries == 0u) && ok;
    ok = D_ET_CHECK(s.enabled_entries == 0u) && ok;
    ok = D_ET_CHECK(s.event_type_count == 0u) && ok;
    ok = D_ET_CHECK(s.max_entries_per_type == 0u) && ok;
    ok = D_ET_CHECK(s.average_entries_per_type == 0.0) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
