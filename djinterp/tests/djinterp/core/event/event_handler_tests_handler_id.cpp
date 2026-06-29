/******************************************************************************
* djinterp [test]                           event_handler_tests_handler_id.cpp
*
*   Section I -- HANDLER IDENTIFICATION.  Covers the handler_id handle: its
* relational operators, the validity query (non-zero is real, zero is the
* null sentinel), the null() factory, and value-copy semantics.
*
*
* path:      /tests/djinterp/core/event/event_handler/event_handler_tests_handler_id.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_handler_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_handler_id_relational
bool
tests_handler_id_relational()
{
    bool ok = true;

    handler_id a;
    handler_id b;
    handler_id c;
    a.value = 7;
    b.value = 7;
    c.value = 9;

    // equality compares the underlying value.
    ok = D_EH_CHECK(a == b) && ok;
    ok = D_EH_CHECK(!(a == c)) && ok;

    // inequality is the negation of equality.
    ok = D_EH_CHECK(a != c) && ok;
    ok = D_EH_CHECK(!(a != b)) && ok;

    return ok;
}


// tests_handler_id_validity
bool
tests_handler_id_validity()
{
    bool ok = true;

    handler_id real;
    real.value = 1;

    handler_id zero;
    zero.value = 0;

    // any non-zero id refers to a real handler; zero does not.
    ok = D_EH_CHECK(real.is_valid()) && ok;
    ok = D_EH_CHECK(!zero.is_valid()) && ok;

    return ok;
}


// tests_handler_id_null
bool
tests_handler_id_null()
{
    bool ok = true;

    handler_id n = handler_id::null();

    // the null sentinel carries value 0 and is not valid.
    ok = D_EH_CHECK(n.value == 0u) && ok;
    ok = D_EH_CHECK(!n.is_valid()) && ok;

    // two null sentinels compare equal.
    ok = D_EH_CHECK(handler_id::null() == handler_id::null()) && ok;

    return ok;
}


// tests_handler_id_value_semantics
bool
tests_handler_id_value_semantics()
{
    bool ok = true;

    handler_id a;
    a.value = 42;

    // copy-construction preserves the value...
    handler_id b(a);
    ok = D_EH_CHECK(b == a) && ok;
    ok = D_EH_CHECK(b.value == 42u) && ok;

    // ...as does copy-assignment.
    handler_id c;
    c.value = 1;
    c = a;
    ok = D_EH_CHECK(c == a) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
