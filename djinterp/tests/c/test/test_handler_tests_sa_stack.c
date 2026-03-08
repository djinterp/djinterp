#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_result_stack
  Tests result stack push/pop operations.
  Tests the following:
  - push/pop on NULL handler return false
  - push NULL result returns false
  - push and pop a single result preserves data
  - multiple push/pop in LIFO order
  - pop from empty stack returns false
*/
bool
d_tests_sa_handler_result_stack
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler result stack ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // push/pop on NULL handler
    {
        struct d_test_result dummy;
        bool                 push_result;
        bool                 pop_result;

        d_memset(&dummy, 0, sizeof(dummy));
        push_result = d_test_handler_push_result(NULL, &dummy);
        pop_result  = d_test_handler_pop_result(NULL, &dummy);

        if (!d_assert_standalone(
                ( (push_result == false) &&
                  (pop_result == false) ),
                "push/pop result on NULL handler",
                "Both push and pop return false for NULL handler",
                _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // push NULL result on valid handler
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if (handler)
        {
            bool result;

            result = d_test_handler_push_result(handler, NULL);

            if (!d_assert_standalone(result == false,
                                     "push NULL result",
                                     "Pushing NULL result returns false",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // push and pop a single result
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if ( (handler) && (handler->result_stack) )
        {
            struct d_test_result push_data;
            struct d_test_result pop_data;
            bool                 push_ok;
            bool                 pop_ok;

            d_memset(&push_data, 0, sizeof(push_data));
            d_memset(&pop_data,  0, sizeof(pop_data));
            push_data.tests_run    = 42;
            push_data.tests_passed = 40;

            push_ok = d_test_handler_push_result(handler,
                                                  &push_data);
            pop_ok  = d_test_handler_pop_result(handler,
                                                 &pop_data);

            if (!d_assert_standalone(
                    ( (push_ok)                     &&
                      (pop_ok)                      &&
                      (pop_data.tests_run == 42)    &&
                      (pop_data.tests_passed == 40) ),
                    "push/pop single result",
                    "Pushed result is correctly popped",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // multiple push/pop in LIFO order
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if ( (handler) && (handler->result_stack) )
        {
            struct d_test_result r1;
            struct d_test_result r2;
            struct d_test_result r3;
            struct d_test_result out;
            bool                 lifo_correct;

            d_memset(&r1,  0, sizeof(r1));
            d_memset(&r2,  0, sizeof(r2));
            d_memset(&r3,  0, sizeof(r3));
            d_memset(&out, 0, sizeof(out));

            r1.tests_run = 1;
            r2.tests_run = 2;
            r3.tests_run = 3;

            d_test_handler_push_result(handler, &r1);
            d_test_handler_push_result(handler, &r2);
            d_test_handler_push_result(handler, &r3);

            lifo_correct = true;

            d_test_handler_pop_result(handler, &out);

            if (out.tests_run != 3)
            {
                lifo_correct = false;
            }

            d_test_handler_pop_result(handler, &out);

            if (out.tests_run != 2)
            {
                lifo_correct = false;
            }

            d_test_handler_pop_result(handler, &out);

            if (out.tests_run != 1)
            {
                lifo_correct = false;
            }

            if (!d_assert_standalone(lifo_correct,
                                     "LIFO order for result stack",
                                     "Results popped in LIFO order",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // pop from empty stack
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if ( (handler) && (handler->result_stack) )
        {
            struct d_test_result out;
            bool                 pop_result;

            d_memset(&out, 0, sizeof(out));
            pop_result = d_test_handler_pop_result(handler, &out);

            if (!d_assert_standalone(pop_result == false,
                                     "pop from empty result stack",
                                     "Popping empty result stack returns false",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] result_stack tests\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] result_stack tests\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_context_stack
  Tests context stack push/pop operations.
  Tests the following:
  - push/pop on NULL handler return false
  - push NULL context returns false
  - push and pop preserves context fields
  - pop from empty context stack returns false
  - context auto-pushed during run with TRACK_STACK flag
*/
bool
d_tests_sa_handler_context_stack
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler context stack ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // push/pop on NULL handler
    {
        struct d_test_context dummy;
        bool                  push_result;
        bool                  pop_result;

        d_memset(&dummy, 0, sizeof(dummy));
        push_result = d_test_handler_push_context(NULL, &dummy);
        pop_result  = d_test_handler_pop_context(NULL, &dummy);

        if (!d_assert_standalone(
                ( (push_result == false) &&
                  (pop_result == false) ),
                "push/pop context on NULL handler",
                "Both return false for NULL handler",
                _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // push NULL context
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if (handler)
        {
            bool result;

            result = d_test_handler_push_context(handler, NULL);

            if (!d_assert_standalone(result == false,
                                     "push NULL context",
                                     "Pushing NULL context returns false",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // push and pop preserves fields
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if ( (handler) && (handler->context_stack) )
        {
            struct d_test_context push_ctx;
            struct d_test_context pop_ctx;
            bool                  push_ok;
            bool                  pop_ok;

            d_memset(&push_ctx, 0, sizeof(push_ctx));
            d_memset(&pop_ctx,  0, sizeof(pop_ctx));

            push_ctx.handler = handler;
            push_ctx.depth   = 5;
            push_ctx.result  = true;

            push_ok = d_test_handler_push_context(handler,
                                                   &push_ctx);
            pop_ok  = d_test_handler_pop_context(handler,
                                                  &pop_ctx);

            if (!d_assert_standalone(
                    ( (push_ok)                      &&
                      (pop_ok)                       &&
                      (pop_ctx.handler == handler)   &&
                      (pop_ctx.depth == 5)           &&
                      (pop_ctx.result == true) ),
                    "push/pop context preserves fields",
                    "Context fields preserved through push/pop",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // pop from empty context stack
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if ( (handler) && (handler->context_stack) )
        {
            struct d_test_context out;
            bool                  pop_result;

            d_memset(&out, 0, sizeof(out));
            pop_result = d_test_handler_pop_context(handler, &out);

            if (!d_assert_standalone(pop_result == false,
                                     "pop from empty context stack",
                                     "Popping empty context stack returns false",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // auto-push during test execution with TRACK_STACK
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if ( (handler) && (handler->context_stack) )
        {
            struct d_test* test;

            test = d_test_new(handler_test_passing, NULL);

            if (test)
            {
                struct d_test_context popped;
                bool                  pop_ok;

                d_test_handler_run_test(handler, test, NULL);

                d_memset(&popped, 0, sizeof(popped));
                pop_ok = d_test_handler_pop_context(handler,
                                                     &popped);

                if (!d_assert_standalone(
                        ( (pop_ok) &&
                          (popped.result == true) ),
                        "auto-push context on TRACK_STACK",
                        "Context auto-pushed during run with TRACK_STACK",
                        _test_info))
                {
                    all_assertions_passed = false;
                }

                d_test_free(test);
            }

            d_test_handler_free(handler);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] context_stack tests\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] context_stack tests\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_stack_no_alloc
  Tests stack operations on handler created without stacks.
  Tests the following:
  - all stack push/pop operations return false
  - handler without stacks still runs tests correctly
*/
bool
d_tests_sa_handler_stack_no_alloc
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing stack ops on handler without stacks ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    {
        struct d_test_handler* handler;

        // create handler without TRACK_STACK flag and 0 stack capacity
        handler = d_test_handler_new(NULL);

        if (handler)
        {
            struct d_test_result  result_buf;
            struct d_test_context context_buf;
            bool                  push_result;
            bool                  pop_result;
            bool                  push_context;
            bool                  pop_context;

            d_memset(&result_buf,  0, sizeof(result_buf));
            d_memset(&context_buf, 0, sizeof(context_buf));

            push_result  = d_test_handler_push_result(handler,
                                                       &result_buf);
            pop_result   = d_test_handler_pop_result(handler,
                                                      &result_buf);
            push_context = d_test_handler_push_context(handler,
                                                        &context_buf);
            pop_context  = d_test_handler_pop_context(handler,
                                                       &context_buf);

            if (!d_assert_standalone(
                    ( (!push_result)  &&
                      (!pop_result)   &&
                      (!push_context) &&
                      (!pop_context) ),
                    "stack ops fail without alloc",
                    "All stack ops return false on handler without stacks",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            // handler should still work for normal execution
            {
                struct d_test* test;

                test = d_test_new(handler_test_passing, NULL);

                if (test)
                {
                    bool run_result;

                    run_result = d_test_handler_run_test(handler,
                                                         test,
                                                         NULL);

                    if (!d_assert_standalone(run_result == true,
                                             "handler without stacks runs tests",
                                             "Handler without stacks still runs tests",
                                             _test_info))
                    {
                        all_assertions_passed = false;
                    }

                    d_test_free(test);
                }
            }

            d_test_handler_free(handler);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] stack_no_alloc tests\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] stack_no_alloc tests\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
