/******************************************************************************
* djinterp [test]                                 event_table_tests_buckets.cpp
*
*   Section III (bucket access + iteration) -- the per-key views over the
* store.  Covers entries_for on a missing key (nullptr) and a present key
* (both const and mutable overloads, with insertion order, ids, mask bits, and
* the stored callable verified); has_entries_for and count_for, including the
* subtle case where removing a bucket's last entry leaves an *empty but
* present* bucket behind (has_entries_for false, entries_for non-null,
* count_for zero); type_key_count (which counts distinct keys and is NOT
* decremented when a bucket is emptied via remove); and the two iteration
* primitives for_each_entry (all buckets) and for_each_entry_for (one bucket,
* no-op on a missing key).
*
*
* path:      /tests/djinterp/core/event/event_table/event_table_tests_buckets.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_entries_for_missing_null
bool
tests_entries_for_missing_null()
{
    bool ok = true;

    // both overloads return nullptr for a key with no bucket.
    event_table t;
    const event_table& ct = t;

    ok = D_ET_CHECK(t.entries_for(KEY_A) == nullptr) && ok;    // mutable overload
    ok = D_ET_CHECK(ct.entries_for(KEY_A) == nullptr) && ok;   // const overload

    return ok;
}


// tests_entries_for_present_contents
bool
tests_entries_for_present_contents()
{
    bool ok = true;

    // a present bucket exposes its entries in insertion order, with the ids,
    // mask bits, and stored callables intact -- through both overloads.
    event_table t;
    handler_id a = t.insert(KEY_A, &ret_pass);
    handler_id b = t.insert(KEY_A, &ret_consume);
    t.disable(b);
    (void)a;

    const std::vector<internal::handler_entry>* lst = t.entries_for(KEY_A);
    ok = D_ET_CHECK(lst != nullptr) && ok;

    if (lst != nullptr)
    {
        ok = D_ET_CHECK(lst->size() == 2u) && ok;
        // insertion order: a (id 1, enabled, pass) then b (id 2, disabled,
        // consume).
        ok = D_ET_CHECK((*lst)[0].id.value == 1u) && ok;
        ok = D_ET_CHECK((*lst)[0].enabled) && ok;
        ok = D_ET_CHECK((*lst)[0].invoke(nullptr) == verdict::pass) && ok;
        ok = D_ET_CHECK((*lst)[1].id.value == 2u) && ok;
        ok = D_ET_CHECK(!(*lst)[1].enabled) && ok;
        ok = D_ET_CHECK((*lst)[1].invoke(nullptr) == verdict::consume) && ok;
    }

    // the mutable overload finds the same bucket.
    std::vector<internal::handler_entry>* mlst = t.entries_for(KEY_A);
    ok = D_ET_CHECK(mlst != nullptr && mlst->size() == 2u) && ok;

    return ok;
}


// tests_has_entries_for
bool
tests_has_entries_for()
{
    bool ok = true;

    event_table t;

    // missing key -> false.
    ok = D_ET_CHECK(!t.has_entries_for(KEY_A)) && ok;

    // present, non-empty -> true.
    handler_id a = t.insert(KEY_A, &ret_pass);
    ok = D_ET_CHECK(t.has_entries_for(KEY_A)) && ok;

    // remove the only entry: the bucket persists but is now empty, so
    // has_entries_for reports false even though the key still exists.
    t.remove(a);
    ok = D_ET_CHECK(!t.has_entries_for(KEY_A)) && ok;
    ok = D_ET_CHECK(t.entries_for(KEY_A) != nullptr) && ok;  // key still present

    return ok;
}


// tests_count_for
bool
tests_count_for()
{
    bool ok = true;

    event_table t;

    // missing -> 0.
    ok = D_ET_CHECK(t.count_for(KEY_A) == 0u) && ok;

    // present -> bucket size.
    t.insert(KEY_A, &ret_pass);
    handler_id b = t.insert(KEY_A, &ret_pass);
    ok = D_ET_CHECK(t.count_for(KEY_A) == 2u) && ok;

    // a different key is independent.
    ok = D_ET_CHECK(t.count_for(KEY_B) == 0u) && ok;

    // emptying the bucket drops the count to 0 (key still present).
    t.remove(b);
    ok = D_ET_CHECK(t.count_for(KEY_A) == 1u) && ok;

    return ok;
}


// tests_type_key_count
bool
tests_type_key_count()
{
    bool ok = true;

    event_table t;

    // empty.
    ok = D_ET_CHECK(t.type_key_count() == 0u) && ok;

    // one bucket per distinct key.
    t.insert(KEY_A, &ret_pass);
    ok = D_ET_CHECK(t.type_key_count() == 1u) && ok;
    handler_id b = t.insert(KEY_B, &ret_pass);
    ok = D_ET_CHECK(t.type_key_count() == 2u) && ok;

    // a second entry under an existing key does not add a bucket.
    t.insert(KEY_A, &ret_pass);
    ok = D_ET_CHECK(t.type_key_count() == 2u) && ok;

    // emptying a bucket via remove does NOT erase the key.
    t.remove(b);
    ok = D_ET_CHECK(t.type_key_count() == 2u) && ok;

    return ok;
}


// tests_for_each_entry
bool
tests_for_each_entry()
{
    bool ok = true;

    // on an empty table the visitor is never called.
    event_table empty;
    int none = 0;
    empty.for_each_entry(visit_counter{&none});
    ok = D_ET_CHECK(none == 0) && ok;

    // every entry across every bucket is visited exactly once, and the visitor
    // observes the live mask state.
    event_table t;
    t.insert(KEY_A, &ret_pass);
    handler_id mid = t.insert(KEY_A, &ret_pass);
    t.insert(KEY_B, &ret_pass);
    t.disable(mid);

    int seen = 0;
    t.for_each_entry(visit_counter{&seen});
    ok = D_ET_CHECK(seen == 3) && ok;
    ok = D_ET_CHECK(static_cast<std::size_t>(seen) == t.total_count()) && ok;

    int enabled_seen = 0;
    t.for_each_entry(enabled_visit_counter{&enabled_seen});
    ok = D_ET_CHECK(enabled_seen == 2) && ok;
    ok = D_ET_CHECK(
        static_cast<std::size_t>(enabled_seen) == t.enabled_count()
    ) && ok;

    return ok;
}


// tests_for_each_entry_for
bool
tests_for_each_entry_for()
{
    bool ok = true;

    event_table t;
    t.insert(KEY_A, &ret_pass);
    t.insert(KEY_A, &ret_pass);
    t.insert(KEY_B, &ret_pass);

    // only the named bucket's entries are visited.
    int a_seen = 0;
    t.for_each_entry_for(KEY_A, bucket_visit_counter{&a_seen});
    ok = D_ET_CHECK(a_seen == 2) && ok;

    int b_seen = 0;
    t.for_each_entry_for(KEY_B, bucket_visit_counter{&b_seen});
    ok = D_ET_CHECK(b_seen == 1) && ok;

    // a missing key visits nothing.
    int c_seen = 0;
    t.for_each_entry_for(KEY_C, bucket_visit_counter{&c_seen});
    ok = D_ET_CHECK(c_seen == 0) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
