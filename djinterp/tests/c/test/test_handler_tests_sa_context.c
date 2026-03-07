/******************************************************************************
* djinterp [test]                                test_handler_tests_sa_context.c
*
*   Unit tests for test_handler context helpers:
*     d_test_context_new, d_test_context_init, d_test_context_free
*
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/

#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_context_new
  Tests d_test_context_new allocation and initialization.

  Tests:
  - Allocation succeeds with valid handler
  - Allocation succeeds with NULL handler
  - Returned context has handler pointer set
  - Returned context has result initialized to true
  - All other fields zero-initialized
*/
bool
d_tests_sa_handler_context_new
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_context_new ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Allocation with valid handler
    {
        struct d_test_handler* handler;
        struct d_test_context* context;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            context = d_test_context_new(handler);

            if (!d_assert_standalone(context != NULL,
                "context_new with valid handler",
                "d_test_context_new returns non-NULL",
                _test_info))
            {
                all_assertions_passed = false;
            }

            if (context)
            {
                if (!d_assert_standalone(
                    context->handler == handler,
                    "context handler pointer set",
                    "Context stores the handler pointer",
                    _test_info))
                {
                    all_assertions_passed = false;
                }

                if (!d_assert_standalone(
                    context->result == true,
                    "context result initialized true",
                    "Context result defaults to true",
                    _test_info))
                {
                    all_assertions_passed = false;
                }

                if (!d_assert_standalone(
                    (context->depth          == 0    &&
                     context->current_test   == NULL &&
                     context->current_block  == NULL),
                    "context fields zero-initialized",
                    "Other context fields start at zero/NULL",
                    _test_info))
                {
                    all_assertions_passed = false;
                }

                d_test_context_free(context);
            }

            d_test_handler_free(handler);
        }
    }

    // Test 2: Allocation with NULL handler
    {
        struct d_test_context* context;

        context = d_test_context_new(NULL);

        if (!d_assert_standalone(context != NULL,
            "context_new with NULL handler",
            "d_test_context_new succeeds with NULL handler",
            _test_info))
        {
            all_assertions_passed = false;
        }

        if (context)
        {
            if (!d_assert_standalone(
                context->handler == NULL,
                "context handler NULL when passed NULL",
                "Context handler is NULL when created with NULL",
                _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_context_free(context);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_context_new\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_context_new\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_context_init
  Tests d_test_context_init with various handler states.

  Tests:
  - Init with NULL context does not crash
  - Init sets handler pointer
  - Init sets result to true
  - Init zeroes all other fields
  - Init overwrites previously set fields
*/
bool
d_tests_sa_handler_context_init
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_context_init ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Init NULL context does not crash
    d_test_context_init(NULL, NULL);

    if (!d_assert_standalone(true,
        "init NULL context",
        "d_test_context_init with NULL does not crash",
        _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Init sets handler and result
    {
        struct d_test_handler* handler;
        struct d_test_context  context;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            // Dirty the context first
            d_memset(&context, 0xFF, sizeof(context));

            d_test_context_init(&context, handler);

            if (!d_assert_standalone(
                (context.handler == handler &&
                 context.result  == true),
                "init sets handler and result",
                "Context init sets handler and result=true",
                _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                (context.depth         == 0    &&
                 context.current_test  == NULL &&
                 context.current_block == NULL),
                "init zeroes other fields",
                "Context init zeroes depth, pointers",
                _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // Test 3: Init with NULL handler
    {
        struct d_test_context context;

        d_memset(&context, 0xFF, sizeof(context));
        d_test_context_init(&context, NULL);

        if (!d_assert_standalone(
            (context.handler == NULL &&
             context.result  == true),
            "init with NULL handler",
            "Context init with NULL handler sets handler=NULL",
            _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_context_init\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_context_init\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_context_free
  Tests d_test_context_free with valid and NULL contexts.

  Tests:
  - Free NULL context does not crash
  - Free valid context does not crash
  - Free context allocated by d_test_context_new
  - Multiple sequential frees of different contexts
*/
bool
d_tests_sa_handler_context_free
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_context_free ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Free NULL context
    d_test_context_free(NULL);

    if (!d_assert_standalone(true,
        "free NULL context",
        "d_test_context_free with NULL does not crash",
        _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Free context from d_test_context_new
    {
        struct d_test_context* context;

        context = d_test_context_new(NULL);

        if (context)
        {
            d_test_context_free(context);

            if (!d_assert_standalone(true,
                "free allocated context",
                "Freeing allocated context succeeds",
                _test_info))
            {
                all_assertions_passed = false;
            }
        }
    }

    // Test 3: Multiple sequential allocations and frees
    {
        int  i;
        bool all_ok;

        all_ok = true;

        for (i = 0; i < 10; i++)
        {
            struct d_test_context* ctx;

            ctx = d_test_context_new(NULL);

            if (!ctx)
            {
                all_ok = false;
                break;
            }

            d_test_context_free(ctx);
        }

        if (!d_assert_standalone(all_ok,
            "sequential alloc/free contexts",
            "10 sequential context alloc/free cycles succeed",
            _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_context_free\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_context_free\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
