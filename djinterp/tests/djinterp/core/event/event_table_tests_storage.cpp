/******************************************************************************
* djinterp [test]                                 event_table_tests_storage.cpp
*
*   Section I -- INTERNAL STORAGE TYPES.  Covers the erasure key kappa
* (internal::type_key: a per-type token that is stable across calls for the
* same type and distinct across types) and the type-erased slot
* (internal::handler_entry: an id, an erased verdict(void*) callable, and the
* enabled mask bit -- each field stored and retrievable).
*
*
* path:      /tests/djinterp/core/event/event_table/event_table_tests_storage.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_table_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_type_key_stable_unique
bool
tests_type_key_stable_unique()
{
    bool ok = true;

    // the key is stable: repeated calls for the same type agree.
    ok = D_ET_CHECK(internal::type_key<tk_a>() == internal::type_key<tk_a>())
         && ok;
    ok = D_ET_CHECK(internal::type_key<tk_b>() == internal::type_key<tk_b>())
         && ok;

    // the key is injective enough for use as a map key: distinct types yield
    // distinct keys.
    ok = D_ET_CHECK(internal::type_key<tk_a>() != internal::type_key<tk_b>())
         && ok;

    return ok;
}


// tests_handler_entry_fields
bool
tests_handler_entry_fields()
{
    bool ok = true;

    // each field of the erased slot is independently stored and retrievable,
    // including the erased callable (invoked here through the entry).
    internal::handler_entry e;
    e.id.value = 7;
    e.invoke   = &ret_consume;
    e.enabled  = true;

    ok = D_ET_CHECK(e.id.value == 7u) && ok;
    ok = D_ET_CHECK(e.enabled) && ok;
    ok = D_ET_CHECK(static_cast<bool>(e.invoke)) && ok;
    ok = D_ET_CHECK(e.invoke(nullptr) == verdict::consume) && ok;

    // the mask bit is plain data and can be cleared.
    e.enabled = false;
    ok = D_ET_CHECK(!e.enabled) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
