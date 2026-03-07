/******************************************************************************
* djinterp [test]                                 test_handler_tests_sa_stats.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"

bool d_tests_sa_handler_depth_tracking(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing depth tracking ---\n");
    ip = _test_info->tests_passed; ok = true;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        if (!d_assert_standalone(h->current_depth == 0,
            "fresh depth=0", "Fresh handler depth=0", _test_info)) ok = false;
        struct d_test_block* b = d_test_block_new(NULL, 0);
        if (b) {
            helper_add_passing_test_to_block(b);
            d_test_handler_run_block(h, b, NULL);
            if (!d_assert_standalone(h->current_depth == 0,
                "depth resets", "current_depth=0 after", _test_info)) ok = false;
            struct d_test_result r = d_test_handler_get_results(h);
            if (!d_assert_standalone(r.max_depth == 1,
                "max_depth=1", "Single block max_depth=1", _test_info)) ok = false;
            d_test_block_free(b); }
        d_test_handler_free(h); } }

    // Sequential blocks
    { struct d_test_handler* h = d_test_handler_new(NULL);
      int i; bool seq_ok = true;
      if (h) {
        for (i = 0; i < 3; i++) {
            struct d_test_block* b = d_test_block_new(NULL, 0);
            if (b) {
                helper_add_passing_test_to_block(b);
                d_test_handler_run_block(h, b, NULL);
                if (h->current_depth != 0) seq_ok = false;
                d_test_block_free(b); } }
        if (!d_assert_standalone(seq_ok,
            "sequential depth", "Depth correct across sequential blocks", _test_info))
            ok = false;
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] depth_tracking\n", D_INDENT); }
    else printf("%s[FAIL] depth_tracking\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_max_depth(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing max_depth ---\n");
    ip = _test_info->tests_passed; ok = true;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test_result r = d_test_handler_get_results(h);
        if (!d_assert_standalone(r.max_depth == 0,
            "fresh=0", "Fresh max_depth=0", _test_info)) ok = false;

        // Shallow block (depth 1)
        { struct d_test_block* sh = d_test_block_new(NULL, 0);
          if (sh) { helper_add_passing_test_to_block(sh);
            d_test_handler_run_block(h, sh, NULL); d_test_block_free(sh); } }

        // Nested block (depth 2)
        { struct d_test_block* o = d_test_block_new(NULL, 0);
          struct d_test_block* in = d_test_block_new(NULL, 0);
          if (o && in) {
            helper_add_passing_test_to_block(in);
            helper_add_block_child_to_block(o, in);
            d_test_handler_run_block(h, o, NULL); d_test_block_free(o); } }

        r = d_test_handler_get_results(h);
        if (!d_assert_standalone(r.max_depth == 2,
            "max tracks highest", "max_depth=2 after shallow+deep", _test_info))
            ok = false;

        d_test_handler_reset_results(h);
        r = d_test_handler_get_results(h);
        if (!d_assert_standalone(r.max_depth == 0,
            "reset clears", "max_depth=0 after reset", _test_info)) ok = false;
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] max_depth\n", D_INDENT); }
    else printf("%s[FAIL] max_depth\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_block_counting(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing block counting ---\n");
    ip = _test_info->tests_passed; ok = true;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test_result r = d_test_handler_get_results(h);
        if (!d_assert_standalone(r.blocks_run == 0,
            "fresh=0", "Fresh blocks=0", _test_info)) ok = false;

        // Empty block counts
        { struct d_test_block* e = d_test_block_new(NULL, 0);
          if (e) { d_test_handler_run_block(h, e, NULL);
            r = d_test_handler_get_results(h);
            if (!d_assert_standalone(r.blocks_run == 1,
                "empty counted", "Empty block increments", _test_info)) ok = false;
            d_test_block_free(e); } }

        // 5 sequential blocks
        d_test_handler_reset_results(h);
        { int i;
          for (i = 0; i < 5; i++) {
            struct d_test_block* b = d_test_block_new(NULL, 0);
            if (b) { helper_add_passing_test_to_block(b);
              d_test_handler_run_block(h, b, NULL); d_test_block_free(b); } }
          r = d_test_handler_get_results(h);
          if (!d_assert_standalone(r.blocks_run == 5,
              "5 sequential", "5 blocks counted", _test_info)) ok = false; }

        // Nested blocks count each level
        d_test_handler_reset_results(h);
        { struct d_test_block* o = d_test_block_new(NULL, 0);
          struct d_test_block* in = d_test_block_new(NULL, 0);
          if (o && in) {
            helper_add_passing_test_to_block(in);
            helper_add_block_child_to_block(o, in);
            d_test_handler_run_block(h, o, NULL);
            r = d_test_handler_get_results(h);
            if (!d_assert_standalone(r.blocks_run == 2,
                "nested=2", "Outer+inner=2", _test_info)) ok = false;
            d_test_block_free(o); } }

        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] block_counting\n", D_INDENT); }
    else printf("%s[FAIL] block_counting\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}
