/******************************************************************************
* djinterp [test]                              test_container_tests_element.cpp
*
*   Section II of the test_container suite: the guarded ELEMENT PROTOCOL.  The
* internal container_element_evaluable trait has two arms - a PRIMARY (for a
* type with no value_type) that is unconditionally false, and a
* SPECIALIZATION (value_type present) that defers to is_test_evaluable on the
* element type.  Both arms are pinned here, together with the property the
* guard exists for: naming the element check on a type that lacks value_type
* must stay well-formed rather than hard-erroring - the fact that these units
* COMPILE is itself the proof.
*
*   The is_test_evaluable element protocol the specialization forwards to is
* anchored with a structural positive (mini_evaluable, not basic_test) and a
* clutch of negatives, so the specialization's verdict has a firm floor.
*
* path:      /tests/djinterp/test/test_container/test_container_tests_element.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_element_evaluable_positive
//   is_test_evaluable holds for the framework element (basic_test) and for a
// hand-rolled type that merely exposes the six protocol members - proving the
// element protocol the specialization forwards to is structural, not nominal.
bool
tests_element_evaluable_positive()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::is_test_evaluable<dt::basic_test>::value)   && ok;
    ok = D_TC_CHECK(dt::is_test_evaluable<mini_evaluable>::value)   && ok;

    return ok;
}

// tests_element_evaluable_negative
//   is_test_evaluable rejects types missing the protocol - a bare scalar and
// a plain struct alike.
bool
tests_element_evaluable_negative()
{
    bool ok = true;

    ok = D_TC_CHECK(!dt::is_test_evaluable<int>::value)            && ok;
    ok = D_TC_CHECK(!dt::is_test_evaluable<not_evaluable>::value)  && ok;
    ok = D_TC_CHECK(!dt::is_test_evaluable<plain_empty>::value)    && ok;

    return ok;
}

// tests_element_guard_primary_no_value_type
//   the PRIMARY arm (second parameter false) is unconditionally false.  It is
// the arm selected for a container lacking value_type.
bool
tests_element_guard_primary_no_value_type()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::internal::container_element_evaluable<
                        missing_value_type, false>::value == false)  && ok;
    ok = D_TC_CHECK(dt::internal::container_element_evaluable<
                        plain_empty, false>::value == false)         && ok;

    return ok;
}

// tests_element_guard_specialization_evaluable
//   the SPECIALIZATION arm (value_type present) defers to is_test_evaluable;
// for an evaluable element it is true.
bool
tests_element_guard_specialization_evaluable()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::internal::container_element_evaluable<
                        object_container_min, true>::value == true)  && ok;
    ok = D_TC_CHECK(dt::internal::container_element_evaluable<
                        tc_vector_of_tests, true>::value == true)    && ok;

    return ok;
}

// tests_element_guard_specialization_non_evaluable
//   the SPECIALIZATION arm is false when the element does not meet the
// protocol (here a std::vector<int> / an int-element container).
bool
tests_element_guard_specialization_non_evaluable()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::internal::container_element_evaluable<
                        non_evaluable_element, true>::value == false) && ok;
    ok = D_TC_CHECK(dt::internal::container_element_evaluable<
                        tc_vector_of_int, true>::value == false)      && ok;

    return ok;
}

// tests_element_guard_wellformed_without_value_type
//   the reason the guard exists: for a type with no value_type, naming the
// element check must not be a hard error.  These expressions COMPILING is the
// proof; their being false is the expected verdict.  The same well-formedness
// then flows up into is_test_object_container, which stays defined (and false)
// for such a type rather than failing to instantiate.
bool
tests_element_guard_wellformed_without_value_type()
{
    bool ok = true;

    // guard directly: primary is well-formed and false for a no-value_type type
    ok = D_TC_CHECK(!dt::internal::container_element_evaluable<
                        plain_empty, false>::value)                 && ok;

    // and the whole contract stays well-formed and false for it
    ok = D_TC_CHECK(!dt::is_test_object_container<plain_empty>::value)        && ok;
    ok = D_TC_CHECK(!dt::is_test_object_container<missing_value_type>::value) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
