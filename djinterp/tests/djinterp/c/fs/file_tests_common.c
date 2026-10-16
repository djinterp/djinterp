/*******************************************************************************
* djinterp [test]                                            file_tests_common.c
*
*   I. FOUNDATION (file_common) -- the naming helpers and the notification
* system that the rest of the subframework reports through. d_backend_name and
* d_file_notify_level_name are pure and checked against their whole documented
* range, including the out-of-range "unknown". The handler machinery is
* exercised end to end where notifications are compiled in
* (D_FILE_NOTIFY_IS_ACTIVE): a counter handler is installed, a real ERROR
* notice is provoked by a parameter-validation failure (which is exactly the
* path that raises one), the counter is checked, and the original handler is
* always put back so no later section inherits ours.
*
*
* path:      /tests/djinterp/c/fs/file_tests_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * INTERNAL: a counting notification handler
 *****************************************************************************/

// g_notify_count
//   variable: bumped once per notice delivered to d_fs_notify_counter.
static int g_notify_count = 0;

/*
d_fs_name_is
  NUL-safe name comparison. These accessors are documented never to return
NULL, but a check records its result and CONTINUES rather than aborting, so an
unguarded strcmp would turn a regression into a crash that takes the whole run
-- and every later section's result -- with it. Failing loudly beats that.

Parameter(s):
  _got:  the returned name, possibly NULL.
  _want: the expected name.
Return:
  true iff _got is non-NULL and equal to _want.
*/
static bool
d_fs_name_is
(
    const char* _got,
    const char* _want
)
{
    return ( (_got != NULL) &&
             (strcmp(_got, _want) == 0) );
}

/*
d_fs_notify_counter
  A handler that counts notices through its context. It touches nothing else,
so it cannot clobber errno or re-enter the module that raised the notice.

Parameter(s):
  _notice:  the notice (unused).
  _context: pointer to an int counter, or NULL.
Return:
  none.
*/
static void
d_fs_notify_counter
(
    const struct d_file_notice* _notice,
    void*                       _context
)
{
    (void)_notice;

    if (_context)
    {
        *((int*)_context) += 1;
    }

    return;
}


/******************************************************************************
 * I.a  d_file_backend_name
 *****************************************************************************/

static void
d_fs_tests_common_backend_name
(
    dtest_suite* _suite
)
{
    const char* name;

    dtest_block(_suite, "d_file_backend_name");

    dtest_test(_suite, "names one of the known backends");
    name = d_file_backend_name();
    D_FS_CHECK(_suite, name != NULL);
    D_FS_CHECK(_suite,
               d_fs_name_is(name, "native") ||
               d_fs_name_is(name, "posix")  ||
               d_fs_name_is(name, "stdc"));
}


/******************************************************************************
 * I.b  d_file_notify_level_name
 *****************************************************************************/

static void
d_fs_tests_common_level_name
(
    dtest_suite* _suite
)
{
    dtest_block(_suite, "d_file_notify_level_name");

    dtest_test(_suite, "each severity has its documented name");
    D_FS_CHECK(_suite,
               d_fs_name_is(d_file_notify_level_name(D_FILE_NOTIFY_NONE),
                            "none"));
    D_FS_CHECK(_suite,
               d_fs_name_is(d_file_notify_level_name(D_FILE_NOTIFY_ERROR),
                            "error"));
    // WARN's name is "warning", not "warn"
    D_FS_CHECK(_suite,
               d_fs_name_is(d_file_notify_level_name(D_FILE_NOTIFY_WARN),
                            "warning"));
    D_FS_CHECK(_suite,
               d_fs_name_is(d_file_notify_level_name(D_FILE_NOTIFY_INFO),
                            "info"));
    D_FS_CHECK(_suite,
               d_fs_name_is(d_file_notify_level_name(D_FILE_NOTIFY_TRACE),
                            "trace"));

    dtest_test(_suite, "a value outside the enum -> \"unknown\"");
    D_FS_CHECK(_suite,
               d_fs_name_is(d_file_notify_level_name(99), "unknown"));
    D_FS_CHECK(_suite,
               d_fs_name_is(d_file_notify_level_name(-1), "unknown"));
}


/******************************************************************************
 * I.c  handler management and delivery
 *****************************************************************************/

static void
d_fs_tests_common_notify
(
    dtest_suite* _suite
)
{
    fn_file_notify saved_handler;
    void*          saved_context;
    fn_file_notify got_handler;
    void*          got_context;

    dtest_block(_suite, "notification handler");

    // remember whatever handler is installed so we can restore it
    saved_context = NULL;
    saved_handler = d_file_notify_get_handler(&saved_context);

    //   Asserted only where there is a handler slot to store into. Compiled
    // out, set_handler is documented to be ACCEPTED and do nothing, and
    // get_handler to yield NULL and clear the context -- so asserting the round
    // trip in the default build asserted behaviour this build openly declines
    // to have. The #else asserts the contract it DOES have rather than skipping
    // the case: "accepted and inert" is a promise too, and an inert setter that
    // quietly kept the pointer would be as wrong as one that dropped it.
#if D_FILE_NOTIFY_IS_ACTIVE
    dtest_test(_suite, "set then get returns the same handler and context");
    d_file_notify_set_handler(d_fs_notify_counter, &g_notify_count);
    got_context = NULL;
    got_handler = d_file_notify_get_handler(&got_context);
    D_FS_CHECK(_suite, got_handler == d_fs_notify_counter);
    D_FS_CHECK(_suite, got_context == &g_notify_count);
#else
    dtest_test(_suite, "compiled out, set is accepted and get stays empty");
    d_file_notify_set_handler(d_fs_notify_counter, &g_notify_count);
    got_context = &g_notify_count;
    got_handler = d_file_notify_get_handler(&got_context);
    D_FS_CHECK(_suite, got_handler == NULL);
    D_FS_CHECK(_suite, got_context == NULL);
#endif

#if D_FILE_NOTIFY_IS_ACTIVE
#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "a provoked error notice reaches the handler");
    g_notify_count = 0;
    // a NULL argument is a validation failure, which is exactly the path that
    // raises an ERROR notice -- and ERROR is compiled in whenever notices are
    {
        FILE* file;

        file = d_file_open_stream(NULL, "rb");

        // never expected to open; guard anyway so a wrong result cannot leak
        if (file)
        {
            d_file_close_stream(file);
        }
    }
    D_FS_CHECK(_suite, g_notify_count > 0);
#endif
#endif

    // restore the original handler no matter what ran above
    d_file_notify_set_handler(saved_handler, saved_context);
}


/******************************************************************************
 * I.d  d_file_notify_default_handler
 *****************************************************************************/

static void
d_fs_tests_common_default_handler
(
    dtest_suite* _suite
)
{
    //   Declared inside the gate with the only cases that use it: every
    // case in this function is a validation case, so with checking off
    // there is nothing left here to declare it for.
    #if D_FILE_VALIDATE_IS_ACTIVE
    struct d_file_notice notice;
    #endif

    dtest_block(_suite, "d_file_notify_default_handler");

    // NOTE: this handler's job is to write a line to the configured stream,
    // so these cases deliberately produce output. What is asserted is the
    // contract around that write, not the text of it.

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "a NULL notice is ignored, not dereferenced");
    d_file_notify_default_handler(NULL, NULL);
    // reaching here at all is the result being checked
    D_FS_CHECK(_suite, true);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "errno is preserved across the call");
    notice.level    = D_FILE_NOTIFY_ERROR;
    notice.error    = EACCES;
    notice.function = "d_fs_tests_common";
    notice.path     = "/some/path";
    notice.message  = "synthetic notice";
    // a handler runs while the failing call's errno is still the value the
    // caller is about to read, so it must not clobber it
    errno = EDOM;
    d_file_notify_default_handler(&notice, NULL);
    D_FS_CHECK(_suite, errno == EDOM);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "absent optional fields are tolerated");
    notice.level    = D_FILE_NOTIFY_WARN;
    notice.error    = 0;
    notice.function = NULL;
    notice.path     = NULL;
    notice.message  = NULL;
    errno = EDOM;
    d_file_notify_default_handler(&notice, NULL);
    D_FS_CHECK(_suite, errno == EDOM);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "an unknown level is still reported, not rejected");
    notice.level    = 12345;
    notice.error    = 0;
    notice.function = "d_fs_tests_common";
    notice.path     = NULL;
    notice.message  = "level outside the enum";
    errno = EDOM;
    d_file_notify_default_handler(&notice, NULL);
    D_FS_CHECK(_suite, errno == EDOM);
    #endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_common
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "I. file_common");

    d_fs_tests_common_backend_name(_suite);
    d_fs_tests_common_level_name(_suite);
    d_fs_tests_common_default_handler(_suite);
    d_fs_tests_common_notify(_suite);

    return;
}
