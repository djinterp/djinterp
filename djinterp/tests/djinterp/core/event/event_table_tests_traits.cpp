/******************************************************************************
* djinterp [test]                                  event_table_tests_traits.cpp
*
*   Sections IV + V (structural detection) -- the fifteen internal detection
* traits and the event_table_traits facade built on them.  For every trait the
* true branch is exercised on the real event_table and the false branch on a
* type lacking the operation; the insert trait is additionally checked against
* a type whose insert returns the wrong type, confirming the is_same
* return-type gate (not mere name lookup).  The facade is verified four ways:
* on the real table (all detections true); on a distinct type providing exactly
* the eleven core operations (is_event_table true -- detection is structural,
* not nominal -- while every optional detection is false); on negative types
* (a bare struct: all false; a core-minus-clear struct: is_event_table false,
* proving the composite needs every core op); and through a cv/ref-qualified
* spelling, confirming clean_t is applied.
*
*
* path:      /tests/djinterp/core/event/event_table/event_table_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_detection_core_mutation
bool
tests_detection_core_mutation()
{
    bool ok = true;

    // insert / remove / enable / disable -> true on the real table.
    ok = D_ET_CHECK(internal::has_table_insert<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_remove<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_enable<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_disable<event_table>::value) && ok;

    // ...and false on a type with none of them.
    ok = D_ET_CHECK(!internal::has_table_insert<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_remove<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_enable<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_disable<not_a_table>::value) && ok;

    // insert present but wrong return type -> rejected by the is_same gate.
    ok = D_ET_CHECK(!internal::has_table_insert<wrong_insert_table>::value)
         && ok;

    return ok;
}


// tests_detection_const_query
bool
tests_detection_const_query()
{
    bool ok = true;

    // const query + count + clear detections -> true on the real table.
    ok = D_ET_CHECK(internal::has_table_is_enabled<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_contains<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_count_for<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_has_entries_for<event_table>::value)
         && ok;
    ok = D_ET_CHECK(internal::has_table_total_count<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_enabled_count<event_table>::value)
         && ok;
    ok = D_ET_CHECK(internal::has_table_clear<event_table>::value) && ok;

    // ...and false on a bare struct.
    ok = D_ET_CHECK(!internal::has_table_is_enabled<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_contains<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_count_for<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_has_entries_for<not_a_table>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_total_count<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_enabled_count<not_a_table>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_clear<not_a_table>::value) && ok;

    return ok;
}


// tests_detection_extended
bool
tests_detection_extended()
{
    bool ok = true;

    // optional detections -> true on the real table.
    ok = D_ET_CHECK(internal::has_table_type_key_count<event_table>::value)
         && ok;
    ok = D_ET_CHECK(internal::has_table_get_stats<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_clear_for<event_table>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_merge<event_table>::value) && ok;

    // ...false on a bare struct...
    ok = D_ET_CHECK(!internal::has_table_type_key_count<not_a_table>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_get_stats<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_clear_for<not_a_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_merge<not_a_table>::value) && ok;

    // ...and false on the core-only type (which has the eleven core ops but
    // none of these four).
    ok = D_ET_CHECK(!internal::has_table_type_key_count<core_table>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_get_stats<core_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_clear_for<core_table>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_merge<core_table>::value) && ok;

    return ok;
}


// tests_traits_facade_real
bool
tests_traits_facade_real()
{
    bool ok = true;

    typedef event_table_traits<event_table> T;

    // every core detection is true...
    ok = D_ET_CHECK(T::has_insert) && ok;
    ok = D_ET_CHECK(T::has_remove) && ok;
    ok = D_ET_CHECK(T::has_enable) && ok;
    ok = D_ET_CHECK(T::has_disable) && ok;
    ok = D_ET_CHECK(T::has_is_enabled) && ok;
    ok = D_ET_CHECK(T::has_contains) && ok;
    ok = D_ET_CHECK(T::has_count_for) && ok;
    ok = D_ET_CHECK(T::has_has_entries_for) && ok;
    ok = D_ET_CHECK(T::has_total_count) && ok;
    ok = D_ET_CHECK(T::has_enabled_count) && ok;
    ok = D_ET_CHECK(T::has_clear) && ok;

    // ...so the composite holds...
    ok = D_ET_CHECK(T::is_event_table) && ok;

    // ...as do all optional detections.
    ok = D_ET_CHECK(T::has_type_key_count) && ok;
    ok = D_ET_CHECK(T::has_stats) && ok;
    ok = D_ET_CHECK(T::has_clear_for) && ok;
    ok = D_ET_CHECK(T::has_merge) && ok;

    return ok;
}


// tests_traits_facade_structural
bool
tests_traits_facade_structural()
{
    bool ok = true;

    // a distinct type providing exactly the eleven core operations IS
    // recognized as an event table (structural, not nominal)...
    typedef event_table_traits<core_table> T;
    ok = D_ET_CHECK(T::is_event_table) && ok;
    ok = D_ET_CHECK(T::has_insert) && ok;
    ok = D_ET_CHECK(T::has_clear) && ok;
    ok = D_ET_CHECK(T::has_enabled_count) && ok;

    // ...while every optional detection is false for it.
    ok = D_ET_CHECK(!T::has_type_key_count) && ok;
    ok = D_ET_CHECK(!T::has_stats) && ok;
    ok = D_ET_CHECK(!T::has_clear_for) && ok;
    ok = D_ET_CHECK(!T::has_merge) && ok;

    return ok;
}


// tests_traits_facade_negative
bool
tests_traits_facade_negative()
{
    bool ok = true;

    // a bare struct satisfies nothing.
    typedef event_table_traits<not_a_table> N;
    ok = D_ET_CHECK(!N::has_insert) && ok;
    ok = D_ET_CHECK(!N::has_remove) && ok;
    ok = D_ET_CHECK(!N::has_clear) && ok;
    ok = D_ET_CHECK(!N::is_event_table) && ok;
    ok = D_ET_CHECK(!N::has_merge) && ok;

    // the core interface minus clear() is NOT a complete event table, proving
    // is_event_table requires every core operation.
    typedef event_table_traits<almost_table> A;
    ok = D_ET_CHECK(A::has_insert) && ok;          // present
    ok = D_ET_CHECK(A::has_total_count) && ok;     // present
    ok = D_ET_CHECK(!A::has_clear) && ok;          // the missing one
    ok = D_ET_CHECK(!A::is_event_table) && ok;     // ...so composite is false

    return ok;
}


// tests_traits_facade_cvref
bool
tests_traits_facade_cvref()
{
    bool ok = true;

    // the facade applies clean_t, so cv- and reference-qualified spellings of
    // the table type detect identically to the bare type.
    ok = D_ET_CHECK(event_table_traits<event_table>::is_event_table) && ok;
    ok = D_ET_CHECK(event_table_traits<const event_table&>::is_event_table)
         && ok;
    ok = D_ET_CHECK(event_table_traits<event_table&>::is_event_table) && ok;
    ok = D_ET_CHECK(
        event_table_traits<const volatile event_table&>::is_event_table
    ) && ok;

    // a qualified non-table is still not a table.
    ok = D_ET_CHECK(!event_table_traits<const not_a_table&>::is_event_table)
         && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
