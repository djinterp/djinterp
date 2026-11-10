/*******************************************************************************
* djinterp [test]                                 text_template_overflow_tests.c
*
*   Regression tests for the two allocation sizes the upgrade audit of
* 2026.11.01 found unchecked in c/text/text_template.c: the binding array a
* new template starts with, and the array d_internal_template_ensure_capacity
* grows to, where both the doubled capacity and its size in bytes could wrap.
* Each is now checked with d_ckd_mul (c/dckdint.h) before the call that uses
* it, and a size that does not fit fails as an allocation does.
*   The growth is reached only past 2^59 bindings, which no test can bind, so
* this suite includes text_template.c itself and calls the static function on
* a template whose counts say so; it never touches the bindings it claims.
* Built with -fsanitize=address it also proves nothing is written out of
* bounds. In DTest's C face (c/modules/test/test_suite.h): the module
* "text_template_overflow", whose one test records every check.
*
*
* path:      /tests/djinterp/c/text/text_template_overflow_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "text_template_overflow_tests.h"  // the entry point, DTest's C face
// std
#include <stdio.h>   // printf
// djinterp
#include "../../../../src/djinterp/c/text/text_template.c"  // the unit, whole


// D_TESTS_TT_OVERFLOW_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_TT_OVERFLOW_CHECK(expr)                                        \
    d_tests_tt_overflow_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_TT_OVERFLOW_MAX
//   constant: the largest size_t.
#define D_TESTS_TT_OVERFLOW_MAX    ((size_t)-1)

// d_tests_tt_overflow_suite
//   the suite being built, for the checks; set by d_tests_tt_overflow_all.
static struct dj_test_suite* d_tests_tt_overflow_suite;


/*
d_tests_tt_overflow_check
  Records one check against the open test.
*/
static void
d_tests_tt_overflow_check(
    int         _passed,
    const char* _expression,
    int         _line
)
{
    (void)d_test_suite_check(d_tests_tt_overflow_suite,
                             (_passed != 0),
                             _expression,
                             __FILE__,
                             _line);

    return;
}

/*
d_tests_tt_overflow_refused
  Whether growing a template whose binding array claims _capacity full slots
fails as an allocation does, leaving the template as it was. The array it
points at is the template's own; nothing reads past it, since the growth is
refused before the copy.
*/
static int
d_tests_tt_overflow_refused(
    struct dj_text_template* _template,
    size_t                   _capacity
)
{
    struct dj_text_template_binding* const bindings = _template->bindings;
    const size_t                           count    = _template->binding_count;
    const size_t                           capacity =
        _template->binding_capacity;

    _template->binding_count    = _capacity;
    _template->binding_capacity = _capacity;

    const enum dj_text_template_error err =
        d_internal_template_ensure_capacity(_template);
    const int unchanged = ( (_template->bindings == bindings)          &&
                            (_template->binding_capacity == _capacity) );

    // give the template back its true counts, so it frees what it owns
    _template->binding_count    = count;
    _template->binding_capacity = capacity;

    return ( (err == D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED) &&
             (unchanged) );
}

/*
d_tests_tt_overflow_growth
  Tests the following:
  - a capacity that cannot double is refused (SIZE_MAX / 2 + 1)
  - a capacity that doubles, but whose bytes do not fit, is refused
  - an ordinary template still grows, from 8 slots to 16, keeping its
    bindings
*/
static void
d_tests_tt_overflow_growth(void)
{
    struct dj_text_template* const tmpl = d_text_template_new();
    const size_t                   slot =
        sizeof(struct dj_text_template_binding);

    D_TESTS_TT_OVERFLOW_CHECK(tmpl != NULL);

    if (tmpl == NULL)
    {
        return;
    }

    D_TESTS_TT_OVERFLOW_CHECK(tmpl->binding_capacity ==
                              D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY);

    // the doubling wraps
    D_TESTS_TT_OVERFLOW_CHECK(
        d_tests_tt_overflow_refused(tmpl,
                                    (D_TESTS_TT_OVERFLOW_MAX / 2u) + 1u));
    D_TESTS_TT_OVERFLOW_CHECK(
        d_tests_tt_overflow_refused(tmpl, D_TESTS_TT_OVERFLOW_MAX));

    // the doubling fits, the bytes wrap
    const size_t wraps = (D_TESTS_TT_OVERFLOW_MAX / slot / 2u) + 1u;

    D_TESTS_TT_OVERFLOW_CHECK(d_tests_tt_overflow_refused(tmpl, wraps));

    // an ordinary growth: fill the eight slots, and bind a ninth
    const size_t count = D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY + 1u;

    for (size_t i = 0u; i < count; ++i)
    {
        char key[8];

        key[0] = 'k';
        key[1] = (char)('a' + (int)i);
        key[2] = '\0';

        D_TESTS_TT_OVERFLOW_CHECK(d_text_template_bind_string(tmpl, key, "v")
                                  == D_TEXT_TEMPLATE_SUCCESS);
    }

    D_TESTS_TT_OVERFLOW_CHECK(tmpl->binding_count ==
                              D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY + 1u);
    D_TESTS_TT_OVERFLOW_CHECK(tmpl->binding_capacity ==
                              2u * D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY);
    D_TESTS_TT_OVERFLOW_CHECK(d_text_template_has_binding(tmpl, "ka"));
    D_TESTS_TT_OVERFLOW_CHECK(d_text_template_has_binding(tmpl, "ki"));

    d_text_template_free(tmpl);

    return;
}


/*
d_tests_tt_overflow_all
  Opens the module, its block and its one test, and makes its checks.
*/
void
d_tests_tt_overflow_all(
    struct dj_test_suite* _suite
)
{
    d_tests_tt_overflow_suite = _suite;

    (void)d_test_suite_module(_suite, "text_template_overflow");
    (void)d_test_suite_describe(_suite,
                                "text_template.c's allocation sizes, checked "
                                "with d_ckd_mul");
    (void)d_test_suite_block(_suite, "text_template.c");
    (void)d_test_suite_test(_suite, "growth");
    (void)d_test_suite_describe(_suite,
                                "a size that cannot double, or whose bytes "
                                "wrap, is refused; an ordinary one grows");

    d_tests_tt_overflow_growth();

    d_tests_tt_overflow_suite = NULL;

    return;
}
