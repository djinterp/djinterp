/*******************************************************************************
* djinterp [test]                                   event_table_tests_traits.cpp
*
*   Sections IV + V (structural detection) -- the fifteen internal detection
* traits and the event_table_traits facade built on them.  For every trait the
* true branch is exercised on the real DEventTable and the false branch on a
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
* path:      /tests/djinterp/cpp/event/event_table_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_detection_core_mutation
bool tests_detection_core_mutation()
{
    bool ok = true;

    // insert / remove / enable / disable -> true on the real table.
    ok = D_ET_CHECK(internal::has_table_insert<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_remove<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_enable<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_disable<DEventTable>::value) && ok;

    // ...and false on a type with none of them.
    ok = D_ET_CHECK(!internal::has_table_insert<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_remove<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_enable<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_disable<DNotATable>::value) && ok;

    // insert present but wrong return type -> rejected by the is_same gate.
    ok = D_ET_CHECK(!internal::has_table_insert<DWrongInsertTable>::value)
         && ok;

    return ok;
}


// tests_detection_const_query
bool tests_detection_const_query()
{
    bool ok = true;

    // const query + count + clear detections -> true on the real table.
    ok = D_ET_CHECK(internal::has_table_is_enabled<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_contains<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_count_for<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_has_entries_for<DEventTable>::value)
         && ok;
    ok = D_ET_CHECK(internal::has_table_total_count<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_enabled_count<DEventTable>::value)
         && ok;
    ok = D_ET_CHECK(internal::has_table_clear<DEventTable>::value) && ok;

    // ...and false on a bare struct.
    ok = D_ET_CHECK(!internal::has_table_is_enabled<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_contains<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_count_for<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_has_entries_for<DNotATable>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_total_count<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_enabled_count<DNotATable>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_clear<DNotATable>::value) && ok;

    return ok;
}


// tests_detection_extended
bool tests_detection_extended()
{
    bool ok = true;

    // optional detections -> true on the real table.
    ok = D_ET_CHECK(internal::has_table_type_key_count<DEventTable>::value)
         && ok;
    ok = D_ET_CHECK(internal::has_table_get_stats<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_clear_for<DEventTable>::value) && ok;
    ok = D_ET_CHECK(internal::has_table_merge<DEventTable>::value) && ok;

    // ...false on a bare struct...
    ok = D_ET_CHECK(!internal::has_table_type_key_count<DNotATable>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_get_stats<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_clear_for<DNotATable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_merge<DNotATable>::value) && ok;

    // ...and false on the core-only type (which has the eleven core ops but
    // none of these four).
    ok = D_ET_CHECK(!internal::has_table_type_key_count<DCoreTable>::value)
         && ok;
    ok = D_ET_CHECK(!internal::has_table_get_stats<DCoreTable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_clear_for<DCoreTable>::value) && ok;
    ok = D_ET_CHECK(!internal::has_table_merge<DCoreTable>::value) && ok;

    return ok;
}


// tests_traits_facade_real
bool tests_traits_facade_real()
{
    bool ok = true;

    typedef event_table_traits<DEventTable> T;

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
bool tests_traits_facade_structural()
{
    bool ok = true;

    // a distinct type providing exactly the eleven core operations IS
    // recognized as an event table (structural, not nominal)...
    typedef event_table_traits<DCoreTable> T;
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
bool tests_traits_facade_negative()
{
    bool ok = true;

    // a bare struct satisfies nothing.
    typedef event_table_traits<DNotATable> N;
    ok = D_ET_CHECK(!N::has_insert) && ok;
    ok = D_ET_CHECK(!N::has_remove) && ok;
    ok = D_ET_CHECK(!N::has_clear) && ok;
    ok = D_ET_CHECK(!N::is_event_table) && ok;
    ok = D_ET_CHECK(!N::has_merge) && ok;

    // the core interface minus clear() is NOT a complete event table, proving
    // is_event_table requires every core operation.
    typedef event_table_traits<DAlmostTable> A;
    ok = D_ET_CHECK(A::has_insert) && ok;          // present
    ok = D_ET_CHECK(A::has_total_count) && ok;     // present
    ok = D_ET_CHECK(!A::has_clear) && ok;          // the missing one
    ok = D_ET_CHECK(!A::is_event_table) && ok;     // ...so composite is false

    return ok;
}


// tests_traits_facade_cvref
bool tests_traits_facade_cvref()
{
    bool ok = true;

    // the facade applies clean_t, so cv- and reference-qualified spellings of
    // the table type detect identically to the bare type.
    ok = D_ET_CHECK(event_table_traits<DEventTable>::is_event_table) && ok;
    ok = D_ET_CHECK(event_table_traits<const DEventTable&>::is_event_table)
         && ok;
    ok = D_ET_CHECK(event_table_traits<DEventTable&>::is_event_table) && ok;
    ok = D_ET_CHECK(
        event_table_traits<const volatile DEventTable&>::is_event_table
    ) && ok;

    // a qualified non-table is still not a table.
    ok = D_ET_CHECK(!event_table_traits<const DNotATable&>::is_event_table)
         && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
