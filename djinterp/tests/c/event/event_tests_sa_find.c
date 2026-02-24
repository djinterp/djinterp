#include "./event_tests_sa.h"


// d_tests_sa_event_find_dummy_cb
//   helper: minimal callback for listener construction in search tests.
static void
d_tests_sa_event_find_dummy_cb(void* _args)
{
    (void)_args;

    return;
}


/*
d_tests_sa_event_find_null
  Tests d_event_listener_find_index_of with NULL inputs.
  Tests the following:
  - NULL array returns -1 and sets count to 0
  - NULL count pointer does not crash
*/
bool
d_tests_sa_event_find_null
(
    struct d_test_counter* _counter
)
{
    bool    result;
    ssize_t idx;
    size_t  count;

    result = true;

    idx = d_event_listener_find_index_of(NULL, 5, 1, &count);

    result = d_assert_standalone(
        idx == -1 && count == 0,
        "find_null_array",
        "NULL array should return -1 and count=0",
        _counter) && result;

    idx = d_event_listener_find_index_of(NULL, 5, 1, NULL);

    result = d_assert_standalone(
        idx == -1,
        "find_null_count_ptr",
        "NULL count pointer should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_find_zero_count
  Tests d_event_listener_find_index_of with zero count.
  Tests the following:
  - Zero listeners_count returns -1 and count=0
*/
bool
d_tests_sa_event_find_zero_count
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener        dummy;
    const struct d_event_listener* arr[1];
    ssize_t                        idx;
    size_t                         count;

    result       = true;
    dummy.id     = 1;
    dummy.fn     = (fn_callback)d_tests_sa_event_find_dummy_cb;
    dummy.enabled = true;
    arr[0]       = &dummy;

    idx = d_event_listener_find_index_of(arr, 0, 1, &count);

    result = d_assert_standalone(
        idx == -1 && count == 0,
        "find_zero_count",
        "Zero count should return -1 and count=0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_find_single_match
  Tests finding a single matching listener.
  Tests the following:
  - Returns index 0 and count 1
*/
bool
d_tests_sa_event_find_single_match
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener*       l;
    const struct d_event_listener* arr[1];
    ssize_t                        idx;
    size_t                         count;

    result = true;
    l      = d_event_listener_new(
                 42,
                 (fn_callback)d_tests_sa_event_find_dummy_cb,
                 true);

    if (l)
    {
        arr[0] = l;
        idx    = d_event_listener_find_index_of(arr, 1, 42, &count);

        result = d_assert_standalone(
            idx == 0 && count == 1,
            "find_single_match",
            "Single match should return index=0, count=1",
            _counter) && result;

        d_event_listener_free(l);
    }

    return result;
}

/*
d_tests_sa_event_find_single_no_match
  Tests searching with no matching listener.
  Tests the following:
  - Returns -1 and count 0
*/
bool
d_tests_sa_event_find_single_no_match
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener*       l;
    const struct d_event_listener* arr[1];
    ssize_t                        idx;
    size_t                         count;

    result = true;
    l      = d_event_listener_new(
                 42,
                 (fn_callback)d_tests_sa_event_find_dummy_cb,
                 true);

    if (l)
    {
        arr[0] = l;
        idx    = d_event_listener_find_index_of(arr, 1, 99, &count);

        result = d_assert_standalone(
            idx == -1 && count == 0,
            "find_single_no_match",
            "No match should return -1, count=0",
            _counter) && result;

        d_event_listener_free(l);
    }

    return result;
}

/*
d_tests_sa_event_find_multi_one_match
  Tests finding one match among multiple listeners.
  Tests the following:
  - Returns correct index for match at known position
*/
bool
d_tests_sa_event_find_multi_one_match
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener*       l1;
    struct d_event_listener*       l2;
    struct d_event_listener*       l3;
    const struct d_event_listener* arr[3];
    ssize_t                        idx;
    size_t                         count;

    result = true;
    l1     = d_event_listener_new(10, (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l2     = d_event_listener_new(20, (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l3     = d_event_listener_new(30, (fn_callback)d_tests_sa_event_find_dummy_cb, true);

    if ( (l1) && (l2) && (l3) )
    {
        arr[0] = l1;
        arr[1] = l2;
        arr[2] = l3;

        idx = d_event_listener_find_index_of(arr, 3, 20, &count);

        result = d_assert_standalone(
            idx == 1 && count == 1,
            "find_multi_one_match",
            "Single match at index 1 should return 1, count=1",
            _counter) && result;

        idx = d_event_listener_find_index_of(arr, 3, 30, &count);

        result = d_assert_standalone(
            idx == 2 && count == 1,
            "find_multi_last_match",
            "Match at last position should return 2, count=1",
            _counter) && result;
    }

    if (l1)
    {
        d_event_listener_free(l1);
    }

    if (l2)
    {
        d_event_listener_free(l2);
    }

    if (l3)
    {
        d_event_listener_free(l3);
    }

    return result;
}

/*
d_tests_sa_event_find_multi_many_matches
  Tests finding multiple matches and occurrence counting.
  Tests the following:
  - Returns first matching index
  - Count reflects total number of matches
*/
bool
d_tests_sa_event_find_multi_many_matches
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener*       l1;
    struct d_event_listener*       l2;
    struct d_event_listener*       l3;
    struct d_event_listener*       l4;
    const struct d_event_listener* arr[4];
    ssize_t                        idx;
    size_t                         count;

    result = true;
    l1     = d_event_listener_new(5,  (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l2     = d_event_listener_new(10, (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l3     = d_event_listener_new(5,  (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l4     = d_event_listener_new(5,  (fn_callback)d_tests_sa_event_find_dummy_cb, true);

    if ( (l1) && (l2) && (l3) && (l4) )
    {
        arr[0] = l1;
        arr[1] = l2;
        arr[2] = l3;
        arr[3] = l4;

        idx = d_event_listener_find_index_of(arr, 4, 5, &count);

        result = d_assert_standalone(
            idx >= 0,
            "find_multi_many_found",
            "Multiple matches should return a valid index",
            _counter) && result;

        result = d_assert_standalone(
            count == 3,
            "find_multi_many_count",
            "Three matches should set count to 3",
            _counter) && result;
    }

    if (l1)
    {
        d_event_listener_free(l1);
    }

    if (l2)
    {
        d_event_listener_free(l2);
    }

    if (l3)
    {
        d_event_listener_free(l3);
    }

    if (l4)
    {
        d_event_listener_free(l4);
    }

    return result;
}

/*
d_tests_sa_event_find_no_matches
  Tests searching when no listeners match.
  Tests the following:
  - Returns -1 and count 0
*/
bool
d_tests_sa_event_find_no_matches
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener*       l1;
    struct d_event_listener*       l2;
    const struct d_event_listener* arr[2];
    ssize_t                        idx;
    size_t                         count;

    result = true;
    l1     = d_event_listener_new(10, (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l2     = d_event_listener_new(20, (fn_callback)d_tests_sa_event_find_dummy_cb, true);

    if ( (l1) && (l2) )
    {
        arr[0] = l1;
        arr[1] = l2;

        idx = d_event_listener_find_index_of(arr, 2, 99, &count);

        result = d_assert_standalone(
            idx == -1 && count == 0,
            "find_no_matches",
            "No matches should return -1, count=0",
            _counter) && result;
    }

    if (l1)
    {
        d_event_listener_free(l1);
    }

    if (l2)
    {
        d_event_listener_free(l2);
    }

    return result;
}

/*
d_tests_sa_event_find_null_entries
  Tests that NULL entries in the array are skipped safely.
  Tests the following:
  - NULL entries do not crash
  - Matches around NULL entries are counted correctly
*/
bool
d_tests_sa_event_find_null_entries
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener*       l1;
    struct d_event_listener*       l3;
    const struct d_event_listener* arr[3];
    ssize_t                        idx;
    size_t                         count;

    result = true;
    l1     = d_event_listener_new(7, (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l3     = d_event_listener_new(7, (fn_callback)d_tests_sa_event_find_dummy_cb, true);

    if ( (l1) && (l3) )
    {
        arr[0] = l1;
        arr[1] = NULL;
        arr[2] = l3;

        idx = d_event_listener_find_index_of(arr, 3, 7, &count);

        result = d_assert_standalone(
            idx >= 0 && count == 2,
            "find_null_entries",
            "Should find 2 matches, skipping NULL entry",
            _counter) && result;
    }

    if (l1)
    {
        d_event_listener_free(l1);
    }

    if (l3)
    {
        d_event_listener_free(l3);
    }

    return result;
}

/*
d_tests_sa_event_find_all_match
  Tests when all listeners match the search id.
  Tests the following:
  - Returns valid index and count equals array size
*/
bool
d_tests_sa_event_find_all_match
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_event_listener*       l1;
    struct d_event_listener*       l2;
    struct d_event_listener*       l3;
    const struct d_event_listener* arr[3];
    ssize_t                        idx;
    size_t                         count;

    result = true;
    l1     = d_event_listener_new(99, (fn_callback)d_tests_sa_event_find_dummy_cb, true);
    l2     = d_event_listener_new(99, (fn_callback)d_tests_sa_event_find_dummy_cb, false);
    l3     = d_event_listener_new(99, (fn_callback)d_tests_sa_event_find_dummy_cb, true);

    if ( (l1) && (l2) && (l3) )
    {
        arr[0] = l1;
        arr[1] = l2;
        arr[2] = l3;

        idx = d_event_listener_find_index_of(arr, 3, 99, &count);

        result = d_assert_standalone(
            idx >= 0 && count == 3,
            "find_all_match",
            "All matching should return valid index and count=3",
            _counter) && result;
    }

    if (l1)
    {
        d_event_listener_free(l1);
    }

    if (l2)
    {
        d_event_listener_free(l2);
    }

    if (l3)
    {
        d_event_listener_free(l3);
    }

    return result;
}

/*
d_tests_sa_event_find_all
  Runs all search function tests.
*/
bool
d_tests_sa_event_find_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Search Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_event_find_null(_counter)              && result;
    result = d_tests_sa_event_find_zero_count(_counter)        && result;
    result = d_tests_sa_event_find_single_match(_counter)      && result;
    result = d_tests_sa_event_find_single_no_match(_counter)   && result;
    result = d_tests_sa_event_find_multi_one_match(_counter)   && result;
    result = d_tests_sa_event_find_multi_many_matches(_counter) && result;
    result = d_tests_sa_event_find_no_matches(_counter)        && result;
    result = d_tests_sa_event_find_null_entries(_counter)      && result;
    result = d_tests_sa_event_find_all_match(_counter)         && result;

    return result;
}
