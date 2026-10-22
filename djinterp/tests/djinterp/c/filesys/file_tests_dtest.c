/*******************************************************************************
* djinterp [test]                                             file_tests_dtest.c
*
* The suite-local DTest vocabulary declared in file_tests_dtest.h.
*   Every test is a record of its module, block and name, its check count and
* its first failing check. A check evaluates where the suite makes it; the
* run only reports.
*
* path:      /tests/djinterp/c/filesys/file_tests_dtest.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./file_tests_dtest.h"  // corresponding header
// std
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // strlen, memcpy


// d_internal_dtest_record
//   struct: one test and what its checks found.
struct d_internal_dtest_record
{
    const char* module;
    const char* block;
    const char* name;
    size_t      checks;
    size_t      failed;
    const char* expr;  // the first failing check
    const char* file;
    int         line;
};

// dtest_suite
//   struct: the records, the open module and block, and the report mode.
struct dtest_suite
{
    struct d_internal_dtest_record* tests;
    size_t                          count;
    size_t                          capacity;
    const char*                     module;
    const char*                     block;
    bool                            open;
    bool                            lost;  // a record or a name not kept
    int                             show;
};

/**
 * @brief A copy of a name the suite keeps until it is destroyed.
 *
 * @param[in,out] _suite  the suite, marked lost when the copy fails.
 * @param[in]     _name   the name; `NULL` reads as empty.
 * @return the copy, or "?" when it could not be made.
 */
static const char*
d_internal_dtest_keep(
    dtest_suite* _suite,
    const char*  _name
)
{
    const char*  text = (_name) ? _name : "";
    const size_t size = strlen(text) + 1;
    char* const  copy = malloc(size);

    // a name that cannot be kept is reported as lost, not dropped silently
    if (!copy)
    {
        _suite->lost = true;

        return "?";
    }

    memcpy(copy,
           text,
           size);

    return copy;
}

dtest_suite*
dtest_create(
    void
)
{
    dtest_suite* const suite = calloc(1,
                                      sizeof(*suite));

    // the module and block a test opens beneath, before any is named
    if (suite)
    {
        suite->module = "";
        suite->block  = "";
        suite->show   = DTEST_SHOW_FAILURES_ONLY;
    }

    return suite;
}

void
dtest_destroy(
    dtest_suite* _suite
)
{
    if (!_suite)
    {
        return;
    }

    // every kept name is freed once: a test's own, and each module and block
    // the first time a test names it
    for (size_t i = 0; i < _suite->count; ++i)
    {
        const struct d_internal_dtest_record* const r = &_suite->tests[i];

        free((void*)r->name);

        if ( (i == 0) ||
             (r->module != _suite->tests[i - 1].module) )
        {
            free((void*)r->module);
        }

        if ( (i == 0) ||
             (r->block != _suite->tests[i - 1].block) )
        {
            free((void*)r->block);
        }
    }

    free(_suite->tests);
    free(_suite);

    return;
}

void
dtest_use_console(
    dtest_suite* _suite,
    int          _show
)
{
    if (_suite)
    {
        _suite->show = _show;
    }

    return;
}

void
dtest_module(
    dtest_suite* _suite,
    const char*  _name
)
{
    if (_suite)
    {
        _suite->module = d_internal_dtest_keep(_suite, _name);
        _suite->block  = "";
        _suite->open   = false;
    }

    return;
}

void
dtest_block(
    dtest_suite* _suite,
    const char*  _name
)
{
    if (_suite)
    {
        _suite->block = d_internal_dtest_keep(_suite, _name);
        _suite->open  = false;
    }

    return;
}

void
dtest_test(
    dtest_suite* _suite,
    const char*  _name
)
{
    if (!_suite)
    {
        return;
    }

    // room for one more record
    if (_suite->count == _suite->capacity)
    {
        const size_t                    grown = (_suite->capacity)
                                                    ? (_suite->capacity * 2)
                                                    : 64;
        struct d_internal_dtest_record* more  =
            realloc(_suite->tests,
                    grown * sizeof(*more));

        if (!more)
        {
            _suite->lost = true;
            _suite->open = false;

            return;
        }

        _suite->tests    = more;
        _suite->capacity = grown;
    }

    struct d_internal_dtest_record* const r = &_suite->tests[_suite->count++];

    memset(r,
           0,
           sizeof(*r));
    r->module     = _suite->module;
    r->block      = _suite->block;
    r->name       = d_internal_dtest_keep(_suite, _name);
    _suite->open  = true;

    return;
}

void
dtest_assert_expr(
    dtest_suite* _suite,
    bool         _result,
    const char*  _expr,
    const char*  _file,
    int          _line
)
{
    // a check with no open test counts nowhere
    if ( (!_suite) ||
         (!_suite->open) )
    {
        return;
    }

    struct d_internal_dtest_record* const r = &_suite->tests[_suite->count - 1];

    r->checks++;

    // the first failure is the one reported
    if (!_result)
    {
        if (r->failed == 0)
        {
            r->expr = _expr;
            r->file = _file;
            r->line = _line;
        }

        r->failed++;
    }

    return;
}

int
dtest_run(
    dtest_suite*   _suite,
    dtest_summary* _summary
)
{
    dtest_summary sum = {0, 0, 0, 0};

    if (!_suite)
    {
        return 2;
    }

    for (size_t i = 0; i < _suite->count; ++i)
    {
        const struct d_internal_dtest_record* const r    = &_suite->tests[i];
        const bool                                  pass = ( (r->checks > 0) &&
                                                             (r->failed == 0) );

        sum.tests++;
        sum.checks        += r->checks;
        sum.checks_failed += r->failed;
        sum.tests_failed  += (pass) ? 0 : 1;

        // a passing test prints only when asked to
        if ( (!pass) ||
             (_suite->show == DTEST_SHOW_ALL) )
        {
            printf("%-4s %s / %s / %s",
                   (pass) ? "ok" : "FAIL",
                   r->module,
                   r->block,
                   r->name);

            if (r->failed > 0)
            {
                printf("  -- %s (%s:%d)",
                       r->expr,
                       r->file,
                       r->line);
            }
            else if (r->checks == 0)
            {
                printf("  -- no check made");
            }

            printf("\n");
        }
    }

    printf("%s  %lu/%lu tests, %lu/%lu checks%s\n",
           (sum.tests_failed == 0) ? "PASS" : "FAIL",
           (unsigned long)(sum.tests - sum.tests_failed),
           (unsigned long)sum.tests,
           (unsigned long)(sum.checks - sum.checks_failed),
           (unsigned long)sum.checks,
           (_suite->lost) ? "; some records were lost" : "");

    if (_summary)
    {
        *_summary = sum;
    }

    return (_suite->lost) ? 2 : (sum.tests_failed > 0) ? 1 : 0;
}
