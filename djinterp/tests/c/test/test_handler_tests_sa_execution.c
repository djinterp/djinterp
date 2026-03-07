/******************************************************************************
* djinterp [test]                              test_handler_tests_sa_execution.c
*
*   Unit tests for test_handler execution functions.
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"

bool d_tests_sa_handler_run_test(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing d_test_handler_run_test ---\n");
    ip = _test_info->tests_passed; ok = true;

    // NULL params
    if (!d_assert_standalone(
        d_test_handler_run_test(NULL, NULL, NULL) == false,
        "run_test NULL params", "NULL handler+test returns false", _test_info))
        ok = false;

    // NULL test on valid handler
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        if (!d_assert_standalone(d_test_handler_run_test(h, NULL, NULL) == false,
            "run_test NULL test", "NULL test returns false", _test_info))
            ok = false;
        d_test_handler_free(h); } }

    // Passing test updates stats
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test* t = helper_make_passing_test();
        if (t) {
            bool r = d_test_handler_run_test(h, t, NULL);
            struct d_test_result res = d_test_handler_get_results(h);
            if (!d_assert_standalone(
                (r == true && res.tests_run == 1 &&
                 res.tests_passed == 1 && res.tests_failed == 0),
                "passing test stats", "run=1,passed=1,failed=0", _test_info))
                ok = false;
            d_test_free(t); }
        d_test_handler_free(h); } }

    // Failing test updates stats
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test* t = helper_make_failing_test();
        if (t) {
            bool r = d_test_handler_run_test(h, t, NULL);
            struct d_test_result res = d_test_handler_get_results(h);
            if (!d_assert_standalone(
                (r == false && res.tests_run == 1 &&
                 res.tests_passed == 0 && res.tests_failed == 1),
                "failing test stats", "run=1,passed=0,failed=1", _test_info))
                ok = false;
            d_test_free(t); }
        d_test_handler_free(h); } }

    // Accumulation 3 pass + 2 fail
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) { int i;
        for (i = 0; i < 3; i++) {
            struct d_test* t = helper_make_passing_test();
            if (t) { d_test_handler_run_test(h, t, NULL); d_test_free(t); } }
        for (i = 0; i < 2; i++) {
            struct d_test* t = helper_make_failing_test();
            if (t) { d_test_handler_run_test(h, t, NULL); d_test_free(t); } }
        struct d_test_result res = d_test_handler_get_results(h);
        if (!d_assert_standalone(
            (res.tests_run == 5 && res.tests_passed == 3 && res.tests_failed == 2),
            "accumulation 3+2", "5 tests: 3 passed, 2 failed", _test_info))
            ok = false;
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] run_test\n", D_INDENT); }
    else printf("%s[FAIL] run_test\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_run_block(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing d_test_handler_run_block ---\n");
    ip = _test_info->tests_passed; ok = true;

    if (!d_assert_standalone(d_test_handler_run_block(NULL, NULL, NULL) == false,
        "run_block NULL", "NULL returns false", _test_info))
        ok = false;

    // Empty block succeeds
    { struct d_test_handler* h = d_test_handler_new(NULL);
      struct d_test_block* b = d_test_block_new(NULL, 0);
      if (h && b) {
        bool r = d_test_handler_run_block(h, b, NULL);
        struct d_test_result res = d_test_handler_get_results(h);
        if (!d_assert_standalone(
            (r == true && res.blocks_run == 1 && res.tests_run == 0),
            "empty block", "succeeds, blocks=1, tests=0", _test_info))
            ok = false;
        d_test_block_free(b); }
      if (h) d_test_handler_free(h); }

    // Block with 3 passing tests
    { struct d_test_handler* h = d_test_handler_new(NULL);
      struct d_test_block* b = d_test_block_new(NULL, 0);
      if (h && b) { int i;
        for (i = 0; i < 3; i++) helper_add_passing_test_to_block(b);
        bool r = d_test_handler_run_block(h, b, NULL);
        struct d_test_result res = d_test_handler_get_results(h);
        if (!d_assert_standalone(
            (r == true && res.tests_run == 3 && res.tests_passed == 3 && res.blocks_run == 1),
            "block 3 pass", "Block: 3 tests all pass", _test_info))
            ok = false;
        d_test_block_free(b); }
      if (h) d_test_handler_free(h); }

    // Block with mixed
    { struct d_test_handler* h = d_test_handler_new(NULL);
      struct d_test_block* b = d_test_block_new(NULL, 0);
      if (h && b) {
        helper_add_passing_test_to_block(b);
        helper_add_failing_test_to_block(b);
        bool r = d_test_handler_run_block(h, b, NULL);
        struct d_test_result res = d_test_handler_get_results(h);
        if (!d_assert_standalone(
            (r == false && res.tests_run == 2 && res.tests_passed == 1 && res.tests_failed == 1),
            "mixed block", "overall=false", _test_info))
            ok = false;
        d_test_block_free(b); }
      if (h) d_test_handler_free(h); }

    // Depth tracking
    { struct d_test_handler* h = d_test_handler_new(NULL);
      struct d_test_block* b = d_test_block_new(NULL, 0);
      if (h && b) {
        helper_add_passing_test_to_block(b);
        d_test_handler_run_block(h, b, NULL);
        struct d_test_result res = d_test_handler_get_results(h);
        if (!d_assert_standalone(
            (res.max_depth >= 1 && h->current_depth == 0),
            "block depth", "max_depth>=1, resets to 0", _test_info))
            ok = false;
        d_test_block_free(b); }
      if (h) d_test_handler_free(h); }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] run_block\n", D_INDENT); }
    else printf("%s[FAIL] run_block\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_run_test_type(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing d_test_handler_run_test_type ---\n");
    ip = _test_info->tests_passed; ok = true;

    if (!d_assert_standalone(
        d_test_handler_run_test_type(NULL, NULL, NULL) == false,
        "run_test_type NULL", "NULL returns false", _test_info))
        ok = false;

    // Wrapping a passing test
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test* t = helper_make_passing_test();
        struct d_test_type* ty = d_test_type_new(D_TEST_TYPE_TEST, t);
        if (ty) {
            bool r = d_test_handler_run_test_type(h, ty, NULL);
            if (!d_assert_standalone(r == true,
                "test_type wrapping test", "Dispatches correctly", _test_info))
                ok = false;
            d_test_type_free(ty); }
        d_test_handler_free(h); } }

    // Unknown type
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test_type dummy;
        d_memset(&dummy, 0, sizeof(dummy));
        dummy.type = D_TEST_TYPE_UNKNOWN;
        if (!d_assert_standalone(
            d_test_handler_run_test_type(h, &dummy, NULL) == false,
            "unknown type", "D_TEST_TYPE_UNKNOWN returns false", _test_info))
            ok = false;
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] run_test_type\n", D_INDENT); }
    else printf("%s[FAIL] run_test_type\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_nested_execution(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing nested execution ---\n");
    ip = _test_info->tests_passed; ok = true;

    // Two-level: outer -> inner -> test
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test_block* outer = d_test_block_new(NULL, 0);
        struct d_test_block* inner = d_test_block_new(NULL, 0);
        if (outer && inner) {
            helper_add_passing_test_to_block(inner);
            helper_add_block_child_to_block(outer, inner);
            d_test_handler_run_block(h, outer, NULL);
            struct d_test_result res = d_test_handler_get_results(h);
            if (!d_assert_standalone(
                (res.max_depth == 2 && res.blocks_run == 2 &&
                 res.tests_run == 1 && h->current_depth == 0),
                "two-level nesting",
                "depth=2, blocks=2, tests=1, current_depth=0", _test_info))
                ok = false;
            d_test_block_free(outer); }
        d_test_handler_free(h); } }

    // Three-level
    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test_block* l1 = d_test_block_new(NULL, 0);
        struct d_test_block* l2 = d_test_block_new(NULL, 0);
        struct d_test_block* l3 = d_test_block_new(NULL, 0);
        if (l1 && l2 && l3) {
            helper_add_passing_test_to_block(l3);
            helper_add_block_child_to_block(l2, l3);
            helper_add_block_child_to_block(l1, l2);
            d_test_handler_run_block(h, l1, NULL);
            struct d_test_result res = d_test_handler_get_results(h);
            if (!d_assert_standalone(
                (res.max_depth == 3 && res.blocks_run == 3),
                "three-level nesting", "depth=3, blocks=3", _test_info))
                ok = false;
            d_test_block_free(l1); }
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] nested_execution\n", D_INDENT); }
    else printf("%s[FAIL] nested_execution\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_config_override(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing config override ---\n");
    ip = _test_info->tests_passed; ok = true;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test* t = helper_make_passing_test();
        if (t) {
            if (!d_assert_standalone(
                d_test_handler_run_test(h, t, NULL) == true,
                "NULL config", "NULL config uses default", _test_info))
                ok = false;
            d_test_free(t); }
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] config_override\n", D_INDENT); }
    else printf("%s[FAIL] config_override\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_run_module(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing d_test_handler_run_module ---\n");
    ip = _test_info->tests_passed; ok = true;

    if (!d_assert_standalone(
        d_test_handler_run_module(NULL, NULL, NULL) == false,
        "run_module NULL", "NULL returns false", _test_info))
        ok = false;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        if (!d_assert_standalone(
            d_test_handler_run_module(h, NULL, NULL) == false,
            "run_module NULL module", "NULL module returns false", _test_info))
            ok = false;
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] run_module\n", D_INDENT); }
    else printf("%s[FAIL] run_module\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_run_tree_and_session(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing run_tree and run_session ---\n");
    ip = _test_info->tests_passed; ok = true;

    if (!d_assert_standalone(
        d_test_handler_run_tree(NULL, NULL, NULL) == false,
        "run_tree NULL", "NULL returns false", _test_info))
        ok = false;
    if (!d_assert_standalone(
        d_test_handler_run_session(NULL, NULL) == false,
        "run_session NULL", "NULL returns false", _test_info))
        ok = false;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        if (!d_assert_standalone(
            d_test_handler_run_tree(h, NULL, NULL) == false,
            "run_tree NULL tree", "NULL tree returns false", _test_info))
            ok = false;
        if (!d_assert_standalone(
            d_test_handler_run_session(h, NULL) == false,
            "run_session NULL session", "NULL session returns false", _test_info))
            ok = false;
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] run_tree_and_session\n", D_INDENT); }
    else printf("%s[FAIL] run_tree_and_session\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}
