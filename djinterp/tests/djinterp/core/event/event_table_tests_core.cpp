/******************************************************************************
* djinterp [test]                                    event_table_tests_core.cpp
*
*   Section III (core) -- the event_table lifecycle operations.  Covers the
* empty initial state; insert (ids handed out from 1 and strictly increasing,
* new entries enabled, total/enabled counts advanced, contains true); remove
* (found -> true and counts decremented; the disabled-then-removed path that
* must not double-decrement the enabled count; missing/double remove -> false);
* id non-reuse across removal; the enable/disable mask (flips and count
* effects, idempotence, and failure on unknown ids); and the is_enabled /
* contains lookups across present, disabled, and absent ids.
*
*
* path:      /tests/djinterp/core/event/event_table/event_table_tests_core.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_table_default_empty
bool
tests_table_default_empty()
{
    bool ok = true;

    // a fresh table holds nothing.
    event_table t;
    ok = D_ET_CHECK(t.total_count() == 0u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 0u) && ok;
    ok = D_ET_CHECK(t.type_key_count() == 0u) && ok;

    return ok;
}


// tests_insert_ids_increasing
bool
tests_insert_ids_increasing()
{
    bool ok = true;

    // ids are assigned from 1 and strictly increase, regardless of bucket.
    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);
    handler_id b = t.insert(KEY_A, &ret_pass);
    handler_id c = t.insert(KEY_B, &ret_pass);

    ok = D_ET_CHECK(a.value == 1u) && ok;
    ok = D_ET_CHECK(b.value == 2u) && ok;
    ok = D_ET_CHECK(c.value == 3u) && ok;
    ok = D_ET_CHECK(a.is_valid() && b.is_valid() && c.is_valid()) && ok;

    return ok;
}


// tests_insert_counts_enabled_contains
bool
tests_insert_counts_enabled_contains()
{
    bool ok = true;

    // each insert advances both counts, the entry starts enabled, and the
    // table reports it present.
    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 1u) && ok;
    ok = D_ET_CHECK(t.contains(a)) && ok;
    ok = D_ET_CHECK(t.is_enabled(a)) && ok;

    handler_id b = t.insert(KEY_B, &ret_pass);
    ok = D_ET_CHECK(t.total_count() == 2u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 2u) && ok;
    ok = D_ET_CHECK(t.contains(b)) && ok;

    // two distinct buckets now exist.
    ok = D_ET_CHECK(t.type_key_count() == 2u) && ok;

    return ok;
}


// tests_remove_basic
bool
tests_remove_basic()
{
    bool ok = true;

    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);
    handler_id b = t.insert(KEY_A, &ret_pass);

    // removing a present, enabled entry returns true and decrements counts.
    ok = D_ET_CHECK(t.remove(a)) && ok;
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 1u) && ok;
    ok = D_ET_CHECK(!t.contains(a)) && ok;
    ok = D_ET_CHECK(t.contains(b)) && ok;

    // removing the same id again returns false and leaves counts alone.
    ok = D_ET_CHECK(!t.remove(a)) && ok;
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;

    // removing an id that never existed returns false.
    handler_id bogus;
    bogus.value = 999;
    ok = D_ET_CHECK(!t.remove(bogus)) && ok;
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;

    return ok;
}


// tests_remove_disabled_bookkeeping
bool
tests_remove_disabled_bookkeeping()
{
    bool ok = true;

    // removing a *disabled* entry must decrement only the total count -- the
    // enabled count was already decremented at disable time and must not be
    // decremented again.
    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);   // total 1, enabled 1
    t.disable(a);                                // total 1, enabled 0
    ok = D_ET_CHECK(t.enabled_count() == 0u) && ok;

    ok = D_ET_CHECK(t.remove(a)) && ok;
    ok = D_ET_CHECK(t.total_count() == 0u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 0u) && ok;

    return ok;
}


// tests_id_non_reuse
bool
tests_id_non_reuse()
{
    bool ok = true;

    // ids are drawn from a monotonic counter and are never recycled, even
    // after the holding entry is removed.
    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);
    t.remove(a);
    handler_id b = t.insert(KEY_A, &ret_pass);

    ok = D_ET_CHECK(b.value != a.value) && ok;
    ok = D_ET_CHECK(b.value == 2u) && ok;

    return ok;
}


// tests_enable_disable_flips_and_counts
bool
tests_enable_disable_flips_and_counts()
{
    bool ok = true;

    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);

    // disabling an enabled entry flips the mask and drops the enabled count.
    ok = D_ET_CHECK(t.disable(a)) && ok;
    ok = D_ET_CHECK(!t.is_enabled(a)) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 0u) && ok;
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;   // still stored

    // enabling a disabled entry flips it back and restores the count.
    ok = D_ET_CHECK(t.enable(a)) && ok;
    ok = D_ET_CHECK(t.is_enabled(a)) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 1u) && ok;

    return ok;
}


// tests_enable_disable_idempotent_and_missing
bool
tests_enable_disable_idempotent_and_missing()
{
    bool ok = true;

    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);

    // enabling an already-enabled entry is a no-op reported as false.
    ok = D_ET_CHECK(!t.enable(a)) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 1u) && ok;

    // first disable succeeds; a second disable is a no-op reported as false.
    ok = D_ET_CHECK(t.disable(a)) && ok;
    ok = D_ET_CHECK(!t.disable(a)) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 0u) && ok;

    // enable/disable of an unknown id both return false and touch nothing.
    handler_id bogus;
    bogus.value = 12345;
    ok = D_ET_CHECK(!t.enable(bogus)) && ok;
    ok = D_ET_CHECK(!t.disable(bogus)) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 0u) && ok;
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;

    return ok;
}


// tests_is_enabled_states
bool
tests_is_enabled_states()
{
    bool ok = true;

    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);

    // enabled entry -> true.
    ok = D_ET_CHECK(t.is_enabled(a)) && ok;

    // disabled entry -> false.
    t.disable(a);
    ok = D_ET_CHECK(!t.is_enabled(a)) && ok;

    // unknown id -> false.
    handler_id bogus;
    bogus.value = 4;
    ok = D_ET_CHECK(!t.is_enabled(bogus)) && ok;

    return ok;
}


// tests_contains_states
bool
tests_contains_states()
{
    bool ok = true;

    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);

    // present -> true; unknown -> false; after removal -> false. Disabled
    // entries are still contained.
    ok = D_ET_CHECK(t.contains(a)) && ok;

    handler_id bogus;
    bogus.value = 77;
    ok = D_ET_CHECK(!t.contains(bogus)) && ok;

    t.disable(a);
    ok = D_ET_CHECK(t.contains(a)) && ok;

    t.remove(a);
    ok = D_ET_CHECK(!t.contains(a)) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
