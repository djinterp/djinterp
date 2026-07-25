/******************************************************************************
* djinterp [test]                               test_container_tests_probes.cpp
*
*   Section I of the test_container suite: the STRUCTURAL MEMBER PROBES.  One
* unit per probe (has_value_type, has_size_accessor, has_empty_method,
* has_begin_end, has_root_method, has_clear_method, has_append_child_method),
* each pinned true on a fixture carrying the member and false on one without;
* plus the cv/ref behavior that distinguishes has_value_type (normalized, it
* runs through clean_t) from the expression probes (which query the type
* directly), and a sweep confirming every `_v` companion agrees with ::value.
*
*   has_clear_method is wired to no contract predicate, so this is the only
* place it is exercised at all.
*
* path:      /tests/djinterp/test/test_container/test_container_tests_probes.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_container_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_probe_has_value_type
//   true iff has_value_type detects a nested value_type alias and only that.
bool
tests_probe_has_value_type()
{
    bool ok = true;

    // present -> true
    ok = D_TC_CHECK(dt::has_value_type<object_container_min>::value)   && ok;
    ok = D_TC_CHECK(dt::has_value_type<buildable_container_min>::value)&& ok;
    ok = D_TC_CHECK(dt::has_value_type<tc_vector_of_tests>::value)     && ok;
    ok = D_TC_CHECK(dt::has_value_type<non_evaluable_element>::value)  && ok;  // value_type=int still counts

    // absent -> false
    ok = D_TC_CHECK(!dt::has_value_type<missing_value_type>::value)    && ok;
    ok = D_TC_CHECK(!dt::has_value_type<plain_empty>::value)           && ok;
    ok = D_TC_CHECK(!dt::has_value_type<root_only>::value)             && ok;

    return ok;
}

// tests_probe_has_size_accessor
//   true iff has_size_accessor detects size() reachable on a const lvalue.
bool
tests_probe_has_size_accessor()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::has_size_accessor<object_container_min>::value)   && ok;
    ok = D_TC_CHECK(dt::has_size_accessor<tc_vector_of_tests>::value)     && ok;

    ok = D_TC_CHECK(!dt::has_size_accessor<missing_size>::value)          && ok;
    ok = D_TC_CHECK(!dt::has_size_accessor<plain_empty>::value)           && ok;

    return ok;
}

// tests_probe_has_empty_method
//   true iff has_empty_method detects empty() reachable on a const lvalue.
bool
tests_probe_has_empty_method()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::has_empty_method<object_container_min>::value)    && ok;
    ok = D_TC_CHECK(dt::has_empty_method<tc_vector_of_tests>::value)      && ok;

    ok = D_TC_CHECK(!dt::has_empty_method<missing_empty>::value)          && ok;
    ok = D_TC_CHECK(!dt::has_empty_method<plain_empty>::value)            && ok;

    return ok;
}

// tests_probe_has_begin_end
//   true iff has_begin_end detects begin() AND end() on a non-const lvalue.
// Also pins the documented behavior that this expression probe queries the
// type DIRECTLY: a const-qualified probe of non-const-only begin()/end()
// fails (it is the contract layer, section III, that normalizes cv/ref).
bool
tests_probe_has_begin_end()
{
    bool ok = true;

    // both present -> true
    ok = D_TC_CHECK(dt::has_begin_end<object_container_min>::value)       && ok;
    ok = D_TC_CHECK(dt::has_begin_end<buildable_container_min>::value)    && ok;
    ok = D_TC_CHECK(dt::has_begin_end<tc_vector_of_tests>::value)         && ok;

    // absent -> false
    ok = D_TC_CHECK(!dt::has_begin_end<missing_begin_end>::value)         && ok;
    ok = D_TC_CHECK(!dt::has_begin_end<plain_empty>::value)               && ok;

    // raw probe queries _Type directly: non-const-only begin/end, probed on a
    // const lvalue, is not found.
    ok = D_TC_CHECK(!dt::has_begin_end<const object_container_min&>::value) && ok;

    return ok;
}

// tests_probe_has_root_method
//   true iff has_root_method detects root() on a non-const lvalue.
bool
tests_probe_has_root_method()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::has_root_method<root_only>::value)                && ok;
    ok = D_TC_CHECK(dt::has_root_method<rooted_container_min>::value)     && ok;
    ok = D_TC_CHECK(dt::has_root_method<buildable_container_min>::value)  && ok;

    ok = D_TC_CHECK(!dt::has_root_method<object_container_min>::value)    && ok;
    ok = D_TC_CHECK(!dt::has_root_method<tc_vector_of_tests>::value)      && ok;
    ok = D_TC_CHECK(!dt::has_root_method<plain_empty>::value)            && ok;

    return ok;
}

// tests_probe_has_clear_method
//   true iff has_clear_method detects clear() on a non-const lvalue.  This
// probe feeds no contract predicate, so its correctness is only observable
// here.
bool
tests_probe_has_clear_method()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::has_clear_method<clear_only>::value)             && ok;
    ok = D_TC_CHECK(dt::has_clear_method<tc_vector_of_tests>::value)     && ok;  // std::vector has clear()

    ok = D_TC_CHECK(!dt::has_clear_method<object_container_min>::value)  && ok;
    ok = D_TC_CHECK(!dt::has_clear_method<plain_empty>::value)           && ok;

    return ok;
}

// tests_probe_has_append_child_method
//   true iff has_append_child_method detects append_child(node_type*,
// value_type) specifically - it needs BOTH the node_type and value_type
// aliases and the two-argument shape.
bool
tests_probe_has_append_child_method()
{
    bool ok = true;

    // exact shape present -> true
    ok = D_TC_CHECK(dt::has_append_child_method<buildable_container_min>::value) && ok;
    ok = D_TC_CHECK(dt::has_append_child_method<append_no_root>::value)          && ok;

    // wrong arity -> false
    ok = D_TC_CHECK(!dt::has_append_child_method<append_wrong_arity>::value)     && ok;
    // no node_type alias -> the probe names _Type::node_type, so SFINAE rejects
    ok = D_TC_CHECK(!dt::has_append_child_method<append_no_node_type>::value)    && ok;
    // no append_child at all -> false
    ok = D_TC_CHECK(!dt::has_append_child_method<object_container_min>::value)   && ok;
    ok = D_TC_CHECK(!dt::has_append_child_method<tc_vector_of_tests>::value)     && ok;  // vector: no node_type
    ok = D_TC_CHECK(!dt::has_append_child_method<plain_empty>::value)           && ok;

    return ok;
}

// tests_probe_value_type_cv_normalized
//   has_value_type is the one probe that strips cv/ref (it is built on
// clean_t), so a const / reference / cv-qualified container agrees with its
// bare form - in contrast to the expression probes exercised above.
bool
tests_probe_value_type_cv_normalized()
{
    bool ok = true;

    ok = D_TC_CHECK(dt::has_value_type<object_container_min>::value ==
                    dt::has_value_type<const object_container_min&>::value)  && ok;
    ok = D_TC_CHECK(dt::has_value_type<object_container_min&&>::value)       && ok;
    ok = D_TC_CHECK(dt::has_value_type<const volatile object_container_min>::value) && ok;

    // absence is likewise stable under cv/ref
    ok = D_TC_CHECK(!dt::has_value_type<const plain_empty&>::value)          && ok;

    return ok;
}

// tests_probe_v_companions
//   every probe's `_v` companion (C++14+) agrees with its ::value.  Below
// C++14 there are no variable templates, so the unit passes vacuously.
bool
tests_probe_v_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = D_TC_CHECK(dt::has_value_type_v<object_container_min> ==
                    dt::has_value_type<object_container_min>::value)             && ok;
    ok = D_TC_CHECK(dt::has_size_accessor_v<object_container_min> ==
                    dt::has_size_accessor<object_container_min>::value)          && ok;
    ok = D_TC_CHECK(dt::has_empty_method_v<object_container_min> ==
                    dt::has_empty_method<object_container_min>::value)           && ok;
    ok = D_TC_CHECK(dt::has_begin_end_v<object_container_min> ==
                    dt::has_begin_end<object_container_min>::value)              && ok;
    ok = D_TC_CHECK(dt::has_root_method_v<rooted_container_min> ==
                    dt::has_root_method<rooted_container_min>::value)            && ok;
    ok = D_TC_CHECK(dt::has_clear_method_v<clear_only> ==
                    dt::has_clear_method<clear_only>::value)                     && ok;
    ok = D_TC_CHECK(dt::has_append_child_method_v<buildable_container_min> ==
                    dt::has_append_child_method<buildable_container_min>::value) && ok;

    // a negative through the companion, to prove it is not stuck true
    ok = D_TC_CHECK(!dt::has_root_method_v<object_container_min>)                && ok;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
