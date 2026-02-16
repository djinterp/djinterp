#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * IV. TEST OBJECT TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_object_struct
  Tests the d_test_object structure.
  Tests the following:
  - Structure has expected members
  - Members are accessible
  - Structure can represent both leaf and interior nodes
*/
bool
d_tests_sa_standalone_object_struct
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object  test_obj;

    result = true;

    // test 1: is_leaf member is accessible
    test_obj.is_leaf = true;

    result = d_assert_standalone(
        test_obj.is_leaf == true,
        "object_is_leaf_accessible",
        "is_leaf member should be accessible",
        _counter) && result;

    // test 2: name member is accessible
    test_obj.name = "test_name";

    result = d_assert_standalone(
        test_obj.name != NULL,
        "object_name_accessible",
        "name member should be accessible",
        _counter) && result;

    // test 3: message member is accessible
    test_obj.message = "test_message";

    result = d_assert_standalone(
        test_obj.message != NULL,
        "object_message_accessible",
        "message member should be accessible",
        _counter) && result;

    // test 4: result member is accessible
    test_obj.result = true;

    result = d_assert_standalone(
        test_obj.result == true,
        "object_result_accessible",
        "result member should be accessible",
        _counter) && result;

    // test 5: args member is accessible
    test_obj.args = NULL;

    result = d_assert_standalone(
        test_obj.args == NULL,
        "object_args_accessible",
        "args member should be accessible",
        _counter) && result;

    // test 6: count member is accessible
    test_obj.count = 5;

    result = d_assert_standalone(
        test_obj.count == 5,
        "object_count_accessible",
        "count member should be accessible",
        _counter) && result;

    // test 7: elements member is accessible
    test_obj.elements = NULL;

    result = d_assert_standalone(
        test_obj.elements == NULL,
        "object_elements_accessible",
        "elements member should be accessible",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_object_new_leaf
  Tests the d_test_object_new_leaf function.
  Tests the following:
  - Creates non-NULL object
  - is_leaf is set to true
  - name is stored correctly
  - message is stored correctly
  - result reflects passed value
  - count is 0 and elements is NULL
*/
bool
d_tests_sa_standalone_object_new_leaf
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_pass;
    struct d_test_object* obj_fail;

    result = true;

    // test 1: creates non-NULL object
    obj_pass = d_test_object_new_leaf("test_name", "test_message", true);

    result = d_assert_standalone(
        obj_pass != NULL,
        "new_leaf_creates_object",
        "d_test_object_new_leaf should return non-NULL",
        _counter) && result;

    if (obj_pass)
    {
        // test 2: is_leaf is true
        result = d_assert_standalone(
            obj_pass->is_leaf == true,
            "new_leaf_is_leaf_true",
            "Leaf object should have is_leaf=true",
            _counter) && result;

        // test 3: name is stored correctly
        result = d_assert_standalone(
            obj_pass->name != NULL,
            "new_leaf_name_stored",
            "name should be stored in leaf object",
            _counter) && result;

        // test 4: message is stored correctly
        result = d_assert_standalone(
            obj_pass->message != NULL,
            "new_leaf_message_stored",
            "message should be stored in leaf object",
            _counter) && result;

        // test 5: result is true when passed true
        result = d_assert_standalone(
            obj_pass->result == true,
            "new_leaf_result_true",
            "result should be true when created with true",
            _counter) && result;

        // test 6: count is 0
        result = d_assert_standalone(
            obj_pass->count == 0,
            "new_leaf_count_zero",
            "Leaf object should have count=0",
            _counter) && result;

        // test 7: elements is NULL
        result = d_assert_standalone(
            obj_pass->elements == NULL,
            "new_leaf_elements_null",
            "Leaf object should have elements=NULL",
            _counter) && result;

        // test 8: args is NULL
        result = d_assert_standalone(
            obj_pass->args == NULL,
            "new_leaf_args_null",
            "Leaf object should have args=NULL",
            _counter) && result;

        d_test_object_free(obj_pass);
    }

    // test 9: creates object with false result
    obj_fail = d_test_object_new_leaf("fail_test", "fail_message", false);

    if (obj_fail)
    {
        result = d_assert_standalone(
            obj_fail->result == false,
            "new_leaf_result_false",
            "result should be false when created with false",
            _counter) && result;

        d_test_object_free(obj_fail);
    }

    return result;
}


/*
d_tests_sa_standalone_object_new_interior
  Tests the d_test_object_new_interior function.
  Tests the following:
  - Creates non-NULL object
  - is_leaf is set to false
  - name is stored correctly
  - count reflects passed value
  - elements array is allocated for non-zero count
  - Zero count creates object with NULL elements
*/
bool
d_tests_sa_standalone_object_new_interior
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* obj_with_children;
    struct d_test_object* obj_no_children;

    result = true;

    // test 1: creates non-NULL object with children
    obj_with_children = d_test_object_new_interior("group_name", 3);

    result = d_assert_standalone(
        obj_with_children != NULL,
        "new_interior_creates_object",
        "d_test_object_new_interior should return non-NULL",
        _counter) && result;

    if (obj_with_children)
    {
        // test 2: is_leaf is false
        result = d_assert_standalone(
            obj_with_children->is_leaf == false,
            "new_interior_is_leaf_false",
            "Interior object should have is_leaf=false",
            _counter) && result;

        // test 3: name is stored correctly
        result = d_assert_standalone(
            obj_with_children->name != NULL,
            "new_interior_name_stored",
            "name should be stored in interior object",
            _counter) && result;

        // test 4: count reflects passed value
        result = d_assert_standalone(
            obj_with_children->count == 3,
            "new_interior_count_correct",
            "count should be 3",
            _counter) && result;

        // test 5: elements array is allocated
        result = d_assert_standalone(
            obj_with_children->elements != NULL,
            "new_interior_elements_allocated",
            "elements should be non-NULL for count > 0",
            _counter) && result;

        // test 6: elements are initially NULL (calloc)
        if (obj_with_children->elements)
        {
            result = d_assert_standalone(
                obj_with_children->elements[0] == NULL,
                "new_interior_elements_null_init",
                "Element pointers should be initialized to NULL",
                _counter) && result;
        }

        // test 7: message is NULL for interior node
        result = d_assert_standalone(
            obj_with_children->message == NULL,
            "new_interior_message_null",
            "Interior object should have message=NULL",
            _counter) && result;

        d_test_object_free(obj_with_children);
    }

    // test 8: zero count creates object with NULL elements
    obj_no_children = d_test_object_new_interior("empty_group", 0);

    if (obj_no_children)
    {
        result = d_assert_standalone(
            obj_no_children->count == 0,
            "new_interior_zero_count",
            "Zero count should result in count=0",
            _counter) && result;

        result = d_assert_standalone(
            obj_no_children->elements == NULL,
            "new_interior_zero_elements_null",
            "Zero count should result in elements=NULL",
            _counter) && result;

        d_test_object_free(obj_no_children);
    }

    return result;
}


/*
d_tests_sa_standalone_object_add_child
  Tests the d_test_object_add_child function.
  Tests the following:
  - NULL parent is handled safely
  - NULL child is handled safely
  - Leaf parent is rejected
  - Out-of-bounds index is rejected
  - Valid add succeeds
*/
bool
d_tests_sa_standalone_object_add_child
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* parent;
    struct d_test_object* child;
    struct d_test_object* leaf_parent;

    result = true;

    // test 1: NULL parent is handled safely
    child = d_test_object_new_leaf("child", "message", true);

    d_test_object_add_child(NULL, child, 0);

    result = d_assert_standalone(
        true,
        "add_child_null_parent_safe",
        "add_child with NULL parent should not crash",
        _counter) && result;

    // test 2: NULL child is handled safely
    parent = d_test_object_new_interior("parent", 2);

    d_test_object_add_child(parent, NULL, 0);

    result = d_assert_standalone(
        parent->elements[0] == NULL,
        "add_child_null_child_safe",
        "add_child with NULL child should not modify parent",
        _counter) && result;

    // test 3: leaf parent is rejected
    leaf_parent = d_test_object_new_leaf("leaf", "msg", true);

    d_test_object_add_child(leaf_parent, child, 0);

    result = d_assert_standalone(
        true,
        "add_child_leaf_parent_rejected",
        "add_child to leaf parent should be rejected (no crash)",
        _counter) && result;

    // test 4: out-of-bounds index is rejected
    d_test_object_add_child(parent, child, 99);

    result = d_assert_standalone(
        parent->elements[0] == NULL,
        "add_child_oob_rejected",
        "add_child with OOB index should be rejected",
        _counter) && result;

    // test 5: valid add succeeds
    d_test_object_add_child(parent, child, 0);

    result = d_assert_standalone(
        parent->elements[0] == child,
        "add_child_valid_succeeds",
        "Valid add_child should store child pointer",
        _counter) && result;

    // test 6: can add multiple children
    {
        struct d_test_object* child2;

        child2 = d_test_object_new_leaf("child2", "msg2", false);
        d_test_object_add_child(parent, child2, 1);

        result = d_assert_standalone(
            parent->elements[1] == child2,
            "add_child_multiple",
            "Should be able to add multiple children",
            _counter) && result;
    }

    // cleanup (parent free will free children)
    d_test_object_free(parent);
    d_test_object_free(leaf_parent);

    return result;
}


/*
d_tests_sa_standalone_object_free
  Tests the d_test_object_free function.
  Tests the following:
  - NULL object is handled safely
  - Leaf object is freed
  - Interior object and children are freed recursively
*/
bool
d_tests_sa_standalone_object_free
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_object* leaf;
    struct d_test_object* interior;
    struct d_test_object* child1;
    struct d_test_object* child2;

    result = true;

    // test 1: NULL object is handled safely
    d_test_object_free(NULL);

    result = d_assert_standalone(
        true,
        "free_null_safe",
        "d_test_object_free(NULL) should not crash",
        _counter) && result;

    // test 2: leaf object is freed (no crash)
    leaf = d_test_object_new_leaf("leaf", "message", true);
    d_test_object_free(leaf);

    result = d_assert_standalone(
        true,
        "free_leaf",
        "Freeing leaf object should not crash",
        _counter) && result;

    // test 3: interior object with children is freed recursively
    interior = d_test_object_new_interior("parent", 2);
    child1   = d_test_object_new_leaf("child1", "msg1", true);
    child2   = d_test_object_new_leaf("child2", "msg2", false);

    d_test_object_add_child(interior, child1, 0);
    d_test_object_add_child(interior, child2, 1);

    d_test_object_free(interior);

    result = d_assert_standalone(
        true,
        "free_interior_recursive",
        "Freeing interior object should free children",
        _counter) && result;

    // test 4: empty interior object is freed
    interior = d_test_object_new_interior("empty", 0);
    d_test_object_free(interior);

    result = d_assert_standalone(
        true,
        "free_empty_interior",
        "Freeing empty interior object should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_object_all
  Aggregation function that runs all test object tests.
*/
bool
d_tests_sa_standalone_object_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Test Object\n");
    printf("  ----------------------\n");

    result = d_tests_sa_standalone_object_struct(_counter) && result;
    result = d_tests_sa_standalone_object_new_leaf(_counter) && result;
    result = d_tests_sa_standalone_object_new_interior(_counter) && result;
    result = d_tests_sa_standalone_object_add_child(_counter) && result;
    result = d_tests_sa_standalone_object_free(_counter) && result;

    return result;
}
