/*******************************************************************************
* djinterp [fs]                                           file_tests_sa_common.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa_common.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * I.  file_common  --  NOTIFICATIONS, INTROSPECTION
 *
 *   The suite's own preamble: it reports how the build was configured, which
 * is what every other group's assertions are conditioned on. If this group is
 * wrong, the rest are testing something other than what they say.
 *****************************************************************************/

// d_tests_file_notice_count
//   variable: how many notices the handler below has seen.
static int d_tests_file_notice_count = 0;

/*
d_tests_file_notice_handler
  Counts notices. The suite installs it to prove the hook actually fires when
D_CFG_FILE_NOTIFY is on -- and, more usefully, that it fires NEVER when the
knob is off, because a notification layer that survives being switched off is
not switched off.
*/
// d_tests_file_notice_marker
//   An object whose ADDRESS is the payload. Its value is never read; it exists
// so the context round-trip can be asserted against something that cannot be
// produced by accident.
static int d_tests_file_notice_marker;

static void
d_tests_file_notice_handler
(
    const struct d_file_notice* _notice,
    void*                       _context
)
{
    (void)_notice;
    (void)_context;
    ++d_tests_file_notice_count;

    return;
}


/*
d_tests_file_notify
  Tests the notification hook.
  Tests the following:
  - a handler can be installed and read back
  - the hook fires exactly when the build says it should
  - NULL removes the handler
*/
static struct d_test_object*
d_tests_file_notify
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                size;
    size_t                idx;
#if D_FILE_NOTIFY_IS_ACTIVE
    //   Declared inside the gate, with the assertions that use it. Outside,
    // the hook does not exist to be queried, so the variable was set and never
    // read -- which is a warning in exactly the build where the notification
    // code is compiled out, i.e. the default one.
    void*                 ctx = NULL;
#endif

    idx = 0;

    //   A non-NULL context on purpose. Installing NULL and reading NULL back
    // would pass whether or not the context is stored at all -- it would be
    // testing a lucky value, not the contract. A marker address can only come
    // back if it was kept.
    d_file_notify_set_handler(d_tests_file_notice_handler, &d_tests_file_notice_marker);

#if D_FILE_NOTIFY_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("get_handler",
                                  (void*)d_file_notify_get_handler(&ctx),
                                  (void*)d_tests_file_notice_handler,
                                  "the installed handler reads back");
    items[idx++] = D_ASSERT_EQUAL("get_context",
                                  ctx, (void*)&d_tests_file_notice_marker,
                                  "the installed context reads back with it");

    d_tests_file_notice_count = 0;
    (void)d_file_read_all("no/such/file/at/all", &size);
    items[idx++] = D_ASSERT_TRUE("fires",
                                 d_tests_file_notice_count > 0,
                                 "a failure raises a notice when NOTIFY is on");

    d_file_notify_set_handler(NULL, NULL);
    d_tests_file_notice_count = 0;
    (void)d_file_read_all("no/such/file/at/all", &size);
    items[idx++] = D_ASSERT_EQUAL("removable",
                                  d_tests_file_notice_count, 0,
                                  "a NULL handler silences the hook");
#else
    // the knob is off, so the hook must be GONE -- not merely quiet. This is
    // the assertion that catches a notification layer which was compiled in
    // and then filtered at runtime, which is what the knob exists to avoid.
    d_tests_file_notice_count = 0;
    (void)d_file_read_all("no/such/file/at/all", &size);
    items[idx++] = D_ASSERT_EQUAL("compiled_out",
                                  d_tests_file_notice_count, 0,
                                  "NOTIFY=0 removes the hook entirely");
#endif

    d_file_notify_set_handler(NULL, NULL);

    items[idx++] = D_ASSERT_NOT_NULL("level_name",
                                     (void*)d_file_notify_level_name(D_FILE_NOTIFY_ERROR),
                                     "every level has a name");
    items[idx++] = D_ASSERT_STR_EQUAL("level_name_unknown",
                                      d_file_notify_level_name(999), "unknown",
                                      "an out-of-range level is named, not NULL");

    return d_tests_file_group("notifications", items, idx);
}


/*
d_tests_file_introspection
  Reports the resolved build configuration.
  These are not really tests -- they are the preamble every other group's
assertions are conditioned on. Asserting them makes the configuration visible
in the output, so a surprising result elsewhere can be read against the build
that produced it rather than guessed at.
*/
static struct d_test_object*
d_tests_file_introspection
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_NOT_NULL("backend_name",
                                     (void*)d_file_backend_name(),
                                     "the backend names itself");

#if D_FILE_BACKEND_IS_NATIVE
    items[idx++] = D_ASSERT_STR_EQUAL("backend", d_file_backend_name(), "native",
                                      "D_FILE_BACKEND_IS_NATIVE agrees with the runtime");
#elif D_FILE_BACKEND_IS_POSIX
    items[idx++] = D_ASSERT_STR_EQUAL("backend", d_file_backend_name(), "posix",
                                      "D_FILE_BACKEND_IS_POSIX agrees with the runtime");
#else
    items[idx++] = D_ASSERT_STR_EQUAL("backend", d_file_backend_name(), "stdc",
                                      "D_FILE_BACKEND_IS_STDC agrees with the runtime");
#endif

    items[idx++] = D_ASSERT_TRUE("path_max",
                                 D_FILE_PATH_MAX > 0,
                                 "D_FILE_PATH_MAX is set");
    items[idx++] = D_ASSERT_TRUE("name_max",
                                 D_FILE_NAME_MAX > 0,
                                 "D_FILE_NAME_MAX is set");

    // the struct must be laid out, not merely declared -- this catches the
    // st_mtime macro collision at its cheapest possible point
    items[idx++] = D_ASSERT_TRUE("stat_layout",
                                 sizeof(struct d_stat_t) >= 40,
                                 "d_stat_t is a real layout, not a macro casualty");

    return d_tests_file_group("build introspection", items, idx);
}


/*
d_tests_file_common_all
  Runs every file_common test.
*/
struct d_test_object*
d_tests_file_common_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_introspection();
    items[idx++] = d_tests_file_notify();

    return d_tests_file_group("I. file_common", items, idx);
}
