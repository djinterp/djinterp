/******************************************************************************
* djinterp [test]                             event_table_tests_merge_clear.cpp
*
*   Section III (merge + clear) -- whole-table composition and teardown.
* merge realizes the pointwise concatenation rho (+) rho': it appends _other's
* entries key by key, assigns FRESH ids from this table's id space (the
* source's ids are not preserved), keeps this table's existing entries first
* within each key, preserves each entry's enabled state, returns the number of
* entries merged, and leaves the source untouched; the empty table is the
* identity.  clear empties every bucket and zeroes the counts.  clear_for
* erases a single key's bucket and adjusts the counts by exactly that bucket's
* total/enabled contribution (a no-op on a missing key), including a bucket
* mixing enabled and disabled entries.
*
*
* path:      /tests/djinterp/core/event/event_table/event_table_tests_merge_clear.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_into_empty
bool
tests_merge_into_empty()
{
    bool ok = true;

    // merging a populated source into an empty table copies every entry, with
    // counts and the returned tally reflecting the source.
    event_table src;
    src.insert(KEY_A, &ret_pass);
    src.insert(KEY_A, &ret_consume);
    src.insert(KEY_B, &ret_pass);

    event_table dst;
    std::size_t merged = dst.merge(src);

    ok = D_ET_CHECK(merged == 3u) && ok;
    ok = D_ET_CHECK(dst.total_count() == 3u) && ok;
    ok = D_ET_CHECK(dst.enabled_count() == 3u) && ok;
    ok = D_ET_CHECK(dst.type_key_count() == 2u) && ok;
    ok = D_ET_CHECK(dst.count_for(KEY_A) == 2u) && ok;
    ok = D_ET_CHECK(dst.count_for(KEY_B) == 1u) && ok;

    // the copied callables are intact and in source order within the key.
    const std::vector<internal::handler_entry>* la = dst.entries_for(KEY_A);
    ok = D_ET_CHECK(la != nullptr && la->size() == 2u) && ok;
    if (la != nullptr && la->size() == 2u)
    {
        ok = D_ET_CHECK((*la)[0].invoke(nullptr) == verdict::pass) && ok;
        ok = D_ET_CHECK((*la)[1].invoke(nullptr) == verdict::consume) && ok;
    }

    return ok;
}


// tests_merge_order_and_fresh_ids
bool
tests_merge_order_and_fresh_ids()
{
    bool ok = true;

    // dst already holds an entry under KEY_A (id 1). src holds an entry under
    // KEY_A whose own id also happens to be 1.
    event_table dst;
    dst.insert(KEY_A, &ret_pass);        // dst id 1

    event_table src;
    src.insert(KEY_A, &ret_consume);     // src id 1

    std::size_t merged = dst.merge(src);
    ok = D_ET_CHECK(merged == 1u) && ok;
    ok = D_ET_CHECK(dst.count_for(KEY_A) == 2u) && ok;

    const std::vector<internal::handler_entry>* la = dst.entries_for(KEY_A);
    ok = D_ET_CHECK(la != nullptr && la->size() == 2u) && ok;
    if (la != nullptr && la->size() == 2u)
    {
        // this table's existing entry stays first...
        ok = D_ET_CHECK((*la)[0].id.value == 1u) && ok;
        ok = D_ET_CHECK((*la)[0].invoke(nullptr) == verdict::pass) && ok;

        // ...and the merged copy receives a FRESH id from dst's space (2),
        // not the source's id (1).
        ok = D_ET_CHECK((*la)[1].id.value == 2u) && ok;
        ok = D_ET_CHECK((*la)[1].invoke(nullptr) == verdict::consume) && ok;
    }

    // a subsequent insert continues from the advanced counter.
    handler_id after = dst.insert(KEY_A, &ret_pass);
    ok = D_ET_CHECK(after.value == 3u) && ok;

    return ok;
}


// tests_merge_preserves_enabled
bool
tests_merge_preserves_enabled()
{
    bool ok = true;

    // a disabled source entry stays disabled after merging, and contributes to
    // the total but not the enabled count.
    event_table src;
    handler_id s1 = src.insert(KEY_A, &ret_pass);    // enabled
    handler_id s2 = src.insert(KEY_A, &ret_pass);    // will disable
    src.disable(s2);
    (void)s1;

    event_table dst;
    std::size_t merged = dst.merge(src);

    ok = D_ET_CHECK(merged == 2u) && ok;
    ok = D_ET_CHECK(dst.total_count() == 2u) && ok;
    ok = D_ET_CHECK(dst.enabled_count() == 1u) && ok;

    const std::vector<internal::handler_entry>* la = dst.entries_for(KEY_A);
    ok = D_ET_CHECK(la != nullptr && la->size() == 2u) && ok;
    if (la != nullptr && la->size() == 2u)
    {
        ok = D_ET_CHECK((*la)[0].enabled) && ok;
        ok = D_ET_CHECK(!(*la)[1].enabled) && ok;
    }

    return ok;
}


// tests_merge_identity_and_src_untouched
bool
tests_merge_identity_and_src_untouched()
{
    bool ok = true;

    // X (+) empty leaves X unchanged and merges nothing.
    event_table x;
    x.insert(KEY_A, &ret_pass);
    event_table empty;

    std::size_t merged_empty = x.merge(empty);
    ok = D_ET_CHECK(merged_empty == 0u) && ok;
    ok = D_ET_CHECK(x.total_count() == 1u) && ok;
    ok = D_ET_CHECK(x.type_key_count() == 1u) && ok;

    // merging X into a fresh table leaves the source X untouched.
    event_table src;
    src.insert(KEY_A, &ret_pass);
    src.insert(KEY_B, &ret_pass);

    event_table dst;
    dst.merge(src);

    ok = D_ET_CHECK(src.total_count() == 2u) && ok;
    ok = D_ET_CHECK(src.enabled_count() == 2u) && ok;
    ok = D_ET_CHECK(src.type_key_count() == 2u) && ok;

    return ok;
}


// tests_clear
bool
tests_clear()
{
    bool ok = true;

    event_table t;
    t.insert(KEY_A, &ret_pass);
    t.insert(KEY_A, &ret_pass);
    t.insert(KEY_B, &ret_pass);

    // clear empties every bucket and zeroes the counts.
    t.clear();
    ok = D_ET_CHECK(t.total_count() == 0u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 0u) && ok;
    ok = D_ET_CHECK(t.type_key_count() == 0u) && ok;
    ok = D_ET_CHECK(t.entries_for(KEY_A) == nullptr) && ok;
    ok = D_ET_CHECK(!t.has_entries_for(KEY_B)) && ok;

    // clearing an already-empty table is a safe no-op.
    t.clear();
    ok = D_ET_CHECK(t.total_count() == 0u) && ok;

    // the table is reusable after clear.
    handler_id a = t.insert(KEY_A, &ret_pass);
    ok = D_ET_CHECK(t.total_count() == 1u && t.contains(a)) && ok;

    return ok;
}


// tests_clear_for
bool
tests_clear_for()
{
    bool ok = true;

    event_table t;
    t.insert(KEY_A, &ret_pass);
    t.insert(KEY_A, &ret_pass);    // KEY_A: 2 entries
    t.insert(KEY_B, &ret_pass);    // KEY_B: 1 entry

    // clearing one key erases its bucket and subtracts exactly that bucket's
    // contribution from the counts, leaving other keys intact.
    t.clear_for(KEY_A);
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 1u) && ok;
    ok = D_ET_CHECK(t.type_key_count() == 1u) && ok;
    ok = D_ET_CHECK(t.entries_for(KEY_A) == nullptr) && ok;   // key erased
    ok = D_ET_CHECK(t.count_for(KEY_B) == 1u) && ok;          // other key kept

    return ok;
}


// tests_clear_for_missing_noop
bool
tests_clear_for_missing_noop()
{
    bool ok = true;

    event_table t;
    t.insert(KEY_A, &ret_pass);

    // clearing a key with no bucket changes nothing.
    t.clear_for(KEY_C);
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 1u) && ok;
    ok = D_ET_CHECK(t.type_key_count() == 1u) && ok;

    return ok;
}


// tests_clear_for_mixed_enabled
bool
tests_clear_for_mixed_enabled()
{
    bool ok = true;

    // a bucket mixing enabled and disabled entries adjusts both counts
    // correctly: total drops by the bucket size, enabled drops by only the
    // enabled members.
    event_table t;
    t.insert(KEY_A, &ret_pass);                  // enabled
    handler_id a2 = t.insert(KEY_A, &ret_pass);  // disabled below
    t.insert(KEY_A, &ret_pass);                  // enabled
    t.disable(a2);
    t.insert(KEY_B, &ret_pass);                  // enabled, other key

    // totals now: 4 entries, 3 enabled.
    ok = D_ET_CHECK(t.total_count() == 4u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 3u) && ok;

    t.clear_for(KEY_A);   // removes 3 entries, 2 of them enabled
    ok = D_ET_CHECK(t.total_count() == 1u) && ok;
    ok = D_ET_CHECK(t.enabled_count() == 1u) && ok;
    ok = D_ET_CHECK(t.type_key_count() == 1u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
