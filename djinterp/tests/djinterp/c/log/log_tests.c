/*******************************************************************************
* djinterp [c][log]                                                  log_tests.c
*
*   The suite of c/log, in DTest's C face. A toy module, "t.net", declares
* three codes; records go to a fixed stream, at a fixed time, so each test
* compares what a writer wrote byte for byte. Expectations follow the knobs a
* build has, so the suite runs under each.
*
*
* path:      /tests/djinterp/c/log/log_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "./log_tests.h"  // corresponding header

// std
#include <stdio.h>   // snprintf, fread, remove
#include <stdlib.h>  // setenv, unsetenv
#include <string.h>  // strcmp, strstr, strlen, strchr
#include <time.h>    // tzset
// djinterp
#include "../../../../inc/djinterp/c/log/log.h"             // the log
#include "../../../../inc/djinterp/c/event/event_common.h"  // keys
#include "../../../../inc/djinterp/c/filesys/file_open.h"   // the file
#include "../../../../inc/djinterp/c/option/option_env.h"   // the reader

// the process id the resource names, as log_writers.c reads it
#if D_ENV_COMPILER_MSVC_FAMILY
    #include <process.h>  // _getpid
    #define D_TESTS_LOG_PID() ((long)_getpid())
#elif defined(__unix__) || defined(__APPLE__)
    #include <unistd.h>  // getpid
    #define D_TESTS_LOG_PID() ((long)getpid())
#endif


//==============================================================================
// THE MODULE
//==============================================================================

// D_TESTS_NET_LOG_CODES
//   table: the toy module's codes.
#define D_TESTS_NET_LOG_CODES(X, m)                                           \
    X(m, 207, connect_timeout, RESOURCE_BUDGET, FAILED,                       \
      "connecting to {host} took over {timeout_ms} ms", (host, timeout_ms))  \
    X(m, 3, not_found, LOOKUP_NOT_FOUND, SOFT, "no route to {host}", (host))  \
    X(m, 4, bad_port, ARGUMENT_INVALID, REJECTED,                             \
      "port {port} is not one{?why}: {why}{/}", (port, why))

D_LOG_CODES_DECLARE(tnet, D_TESTS_NET_LOG_CODES)
D_LOG_CODES_DEFINE(tnet, "t.net", D_TESTS_NET_LOG_CODES)

// D_TESTS_DUP_LOG_CODES, D_TESTS_TWIN_LOG_CODES
//   tables: one with a number twice; one whose full names are t.net's.
#define D_TESTS_DUP_LOG_CODES(X, m)                                           \
    X(m, 1, first, NONE, OK, "first", ())                                    \
    X(m, 1, second, NONE, OK, "second", ())
#define D_TESTS_TWIN_LOG_CODES(X, m)                                          \
    X(m, 9, connect_timeout, NONE, OK, "twin", ())

D_LOG_CODES_DECLARE(tdup, D_TESTS_DUP_LOG_CODES)
D_LOG_CODES_DEFINE(tdup, "t.dup", D_TESTS_DUP_LOG_CODES)
D_LOG_CODES_DECLARE(ttwin, D_TESTS_TWIN_LOG_CODES)
D_LOG_CODES_DEFINE(ttwin, "t.net", D_TESTS_TWIN_LOG_CODES)

// D_TESTS_BAD_LOG_CODES, D_TESTS_OPEN_LOG_CODES
//   tables: one naming a fact it does not list; one with a block unclosed.
#define D_TESTS_BAD_LOG_CODES(X, m)                                           \
    X(m, 1, fine, NONE, OK, "{host} is fine", (host))                        \
    X(m, 2, unknown, NONE, OK, "{nope} is not a fact", (host))
#define D_TESTS_OPEN_LOG_CODES(X, m)                                          \
    X(m, 1, unclosed, NONE, OK, "{?host}{host} is never closed", (host))

D_LOG_CODES_DECLARE(tbad, D_TESTS_BAD_LOG_CODES)
D_LOG_CODES_DEFINE(tbad, "t.bad", D_TESTS_BAD_LOG_CODES)
D_LOG_CODES_DECLARE(topen, D_TESTS_OPEN_LOG_CODES)
D_LOG_CODES_DEFINE(topen, "t.open", D_TESTS_OPEN_LOG_CODES)

// D_TESTS_DB_LOG_CODES
//   macro: codes numbered below zero, as a module's enum gives them.
#define D_TESTS_DB_LOG_CODES(X, m)                                            \
    X(m, -6, wait, SYNC_BUSY, FAILED, "waited on {table}", (table))           \
    X(m, -1, closed, LIFECYCLE_FROZEN, REJECTED, "the pool is frozen", ())

D_LOG_CODES_DECLARE(tdb, D_TESTS_DB_LOG_CODES)
D_LOG_CODES_DEFINE(tdb, "t.db", D_TESTS_DB_LOG_CODES)

// D_TESTS_TIME
//   constant: the fixed clock's time: 2025-11-13T15:04:05.123456789Z.
#define D_TESTS_TIME  ((uint64_t)1763046245u * 1000000000u + 123456789u)

/*
d_tests_log_clock
  The fixed clock.
*/
static uint64_t
d_tests_log_clock(
    void* _context
)
{
    (void)_context;

    return D_TESTS_TIME;
}

/*
d_tests_log_step
  A clock a second later at each reading; its context counts the readings.
*/
static uint64_t
d_tests_log_step(
    void* _context
)
{
    unsigned* const readings = (unsigned*)_context;

    ++*readings;

    return D_TESTS_TIME + (uint64_t)*readings * 1000000000u;
}

// dj_tests_log_out
//   struct: the log's destination in a test: a fixed stream over a buffer.
struct dj_tests_log_out
{
    char                    text[4096];
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
};

static struct dj_tests_log_out d_tests_log_out;

/*
d_tests_log_reset
  The log at its defaults: every setting unbound, the fixed clock, and an
empty destination.
*/
static void
d_tests_log_reset(void)
{
    if (d_log_djin() != NULL)
    {
        d_option_env_clear(d_log_djin());
    }

    (void)d_ostream_init_fixed(&d_tests_log_out.os, &d_tests_log_out.fixed,
                               d_tests_log_out.text,
                               sizeof(d_tests_log_out.text), 1);
    d_log_set_stream(&d_tests_log_out.os);
    d_log_set_writer(NULL, NULL);
    d_log_set_clock(d_tests_log_clock, NULL);

    return;
}

/*
d_tests_log_put
  Binds a setting of the log's djin; 1 when bound.
*/
static int
d_tests_log_put(
    const char* _statement
)
{
    return ( (d_log_djin() != NULL) &&
             (d_option_env_put(d_log_djin(), _statement).is_ok) ) ? 1 : 0;
}

/*
d_tests_log_is
  Whether what the log wrote is the text expected; prints both when not.
*/
static int
d_tests_log_is(
    const char* _expected
)
{
    (void)d_ostream_flush(&d_tests_log_out.os);

    if (strcmp(d_tests_log_out.text, _expected) == 0)
    {
        return 1;
    }

    printf("      wrote:    [%s]\n      expected: [%s]\n",
           d_tests_log_out.text, _expected);

    return 0;
}

/*
d_tests_log_nested
  A describer of one level: a peer, its address and its round trip.
*/
static int
d_tests_log_nested(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    (void)_object;

    if (d_desc_enter(_sink, "peer", "peer") == 0)
    {
        return 0;
    }

    (void)d_desc_emit(_sink, "addr", NULL, d_ostream_value_text("10.0.0.1"),
                      D_DESC_LAYER_STATE, 0u, D_DESC_TRUTH_OBSERVED);
    (void)d_desc_emit(_sink, "rtt", NULL, d_ostream_value_real(1.5),
                      D_DESC_LAYER_STATE, 0u, D_DESC_TRUTH_OBSERVED);
    d_desc_leave(_sink);

    return 0;
}

/*
d_tests_log_peer_record
  A record of connect_timeout with a host, a count, and the nested peer.
*/
static void
d_tests_log_peer_record(void)
{
    struct dj_log_record record;

    if (d_log_open(&record, &d_log_code_tnet_connect_timeout, 0u) != 0)
    {
        (void)d_desc_emit(d_log_sink(&record), "host", NULL,
                          d_ostream_value_text("h"), D_DESC_LAYER_STATE, 0u,
                          D_DESC_TRUTH_OBSERVED);
        (void)d_desc_emit(d_log_sink(&record), "timeout_ms", NULL,
                          d_ostream_value_long(500L), D_DESC_LAYER_STATE, 0u,
                          D_DESC_TRUTH_OBSERVED);
        (void)d_tests_log_nested(NULL, d_log_sink(&record));
        (void)d_log_close(&record);
    }

    return;
}

// the default build: every format and feature, names and levels built in
#define D_TESTS_LOG_DEFAULTS                                                  \
    ( D_CFG_LOG_ENABLED && (D_CFG_LOG_LEVELS == 1) && D_CFG_LOG_CODES &&      \
      (D_CFG_LOG_FORMATS == 0x7) && (D_CFG_LOG_FORMAT == 0) &&                \
      (D_CFG_LOG_LEVEL == 9) && (D_CFG_LOG_LEVEL_MIN == 0) &&                 \
      (D_CFG_LOG_TIME == 1) && (D_CFG_LOG_CODE_STYLE == 0) &&                 \
      D_CFG_LOG_SCOPE && D_CFG_LOG_OPTIONS && (D_CFG_LOG_FACTS_MAX >= 8) &&   \
      (D_CFG_LOG_DEPTH_MAX >= 2) )

// the default text record of connect_timeout
#define D_TESTS_LOG_TEXT                                                      \
    "2025-11-13T15:04:05.123Z error t.net.connect_timeout: connecting to "    \
    "db 1 took over 500 ms host=\"db 1\" timeout_ms=500\n"


//==============================================================================
// THE TESTS
//==============================================================================

/*
d_tests_log_any_build
  Checks the following, under any knobs:
  - a record is wanted exactly when its grade's level reaches the threshold
    and D_CFG_LOG_LEVEL_MIN, or always without levels, never without a log
  - a record wanted is written, and one not wanted is not
*/
static void
d_tests_log_any_build(
    struct dj_test_suite* _suite
)
{
    const int level = D_CFG_LOG_GRADE_LEVEL(D_LOG_GRADE_FAILED);
    const int wanted = (D_CFG_LOG_ENABLED) &&
                       ( (D_CFG_LOG_LEVELS == 0) ||
                         ( (level >= D_CFG_LOG_LEVEL) &&
                           (level >= D_CFG_LOG_LEVEL_MIN) ) );

    (void)d_test_suite_test(_suite, "a record wanted is written, any build");
    d_tests_log_reset();
    (void)d_log_module_add(&d_log_module_tnet);
    D_TEST_CHECK(_suite, d_log_wants(&d_log_code_tnet_connect_timeout, 0u) ==
                         wanted);
    d_tests_log_peer_record();
    (void)d_ostream_flush(&d_tests_log_out.os);
    D_TEST_CHECK(_suite, (d_tests_log_out.text[0] != '\0') == (wanted != 0));
    D_TEST_CHECK(_suite, (d_log_djin() != NULL) == (D_CFG_LOG_OPTIONS != 0));

    if ( (wanted == 0) || (d_log_djin() == NULL) )
    {
        (void)d_tests_log_is;
        (void)d_tests_log_put;
    }

    return;
}

/*
d_tests_log_levels
  Checks the following:
  - a level by its name, any case, or by its severity's number
  - the name a severity is written with: its level's, or the nearest below
  - adding a level at run time, where the registry is built
*/
static void
d_tests_log_levels(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "levels");

    D_TEST_CHECK(_suite, (d_log_level_find("WARNING", 7u) == D_LOG_WARNING) &&
                         (d_log_level_find("info", 4u) == D_LOG_INFO)       &&
                         (d_log_level_find("14", 2u) == 14u)                &&
                         (d_log_level_find("25", 2u) == 0u)                 &&
                         (d_log_level_find("nope", 4u) == 0u)               &&
                         (d_log_level_find(NULL, 0u) == 0u));
    D_TEST_CHECK(_suite, (strcmp(d_log_level_name(13u), "warning") == 0) &&
                         (strcmp(d_log_level_name(24u), "fatal") == 0)   &&
                         (strcmp(d_log_level_name(0u), "unset") == 0));
    // 14 is written as warning, or as a level a build added at 14
    D_TEST_CHECK(_suite,
        (d_log_level_find(d_log_level_name(14u),
                          strlen(d_log_level_name(14u))) == 13u) ||
        ( (D_CFG_LOG_LEVELS >= 2) &&
          (d_log_level_find(d_log_level_name(14u),
                            strlen(d_log_level_name(14u))) == 14u) ));
#if D_CFG_LOG_LEVELS >= 3
    D_TEST_CHECK(_suite, (d_log_level_add("audit", 14u) == 0) &&
                         (d_log_level_add("audit", 14u) == 0) &&
                         (d_log_level_add("audit", 15u) == -1) &&
                         (d_log_level_add("9x", 3u) == -1) &&
                         (d_log_level_add("x y", 3u) == -1) &&
                         (d_log_level_add("big", 25u) == -1));
    D_TEST_CHECK(_suite, (d_log_level_find("Audit", 5u) == 14u) &&
                         (strcmp(d_log_level_name(14u), "audit") == 0));
#else
    D_TEST_CHECK(_suite, d_log_level_add("audit", 14u) == -1);
#endif

    return;
}

/*
d_tests_log_codes
  Checks the following:
  - a module registers once; a table with a number twice is refused, and,
    in a debug build, one whose full names another module has
  - a code by its module's name and its number; its key is the event
    system's key of its full name
  - the vocabulary's names
*/
static void
d_tests_log_codes(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "codes, their keys, and the vocabulary");

    D_TEST_CHECK(_suite, (d_log_module_add(&d_log_module_tnet) == 0) &&
                         (d_log_module_add(&d_log_module_tnet) == 0));
    D_TEST_CHECK(_suite, (d_log_module_add(&d_log_module_tdup) == -1) &&
                         (d_log_module_add(NULL) == -1));
#if D_CFG_LOG_CODES &&                                                        \
    ( (D_CFG_EVENT_KEY_CHECK == 2) ||                                         \
      ( (D_CFG_EVENT_KEY_CHECK == 1) && !defined(NDEBUG) ) )
    D_TEST_CHECK(_suite, d_log_module_add(&d_log_module_ttwin) == -1);
#else
    (void)d_log_module_ttwin;
#endif
    D_TEST_CHECK(_suite, (d_log_code_find("t.net", 5u, 207) ==
                          &d_log_code_tnet_connect_timeout) &&
                         (d_log_code_find("t.net", 5u, 99) == NULL) &&
                         (d_log_code_find("t.ne", 4u, 207) == NULL));
#if D_CFG_LOG_CODES
    D_TEST_CHECK(_suite, d_log_code_key(&d_log_code_tnet_connect_timeout) ==
                         d_event_key_of_name("t.net.connect_timeout"));
#else
    D_TEST_CHECK(_suite, d_log_code_key(&d_log_code_tnet_connect_timeout) ==
                         d_event_key_of_name("t.net:207"));
#endif
    D_TEST_CHECK(_suite,
        (strcmp(d_log_class_name(D_LOG_CLASS_POSITION_INDEX),
                "position.index") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_SYNTAX_ARITY),
                "syntax.arity") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_COUNT), "?") == 0) &&
        (strcmp(d_log_grade_name(D_LOG_GRADE_REJECTED), "rejected") == 0) &&
        (strcmp(d_log_grade_name(D_LOG_GRADE_COUNT), "?") == 0));

    return;
}

/*
d_tests_log_threshold
  Checks the following:
  - a record is wanted at its grade's level against the threshold: soft is
    debug, below info; failed is error
  - log.level lowers the threshold; log.<module>.level sets one module's
  - log.grade_<grade> changes a grade's level
  - a record not wanted is not written
*/
static void
d_tests_log_threshold(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "the threshold and the grades");
    d_tests_log_reset();
    (void)d_log_module_add(&d_log_module_tnet);

#if !D_TESTS_LOG_DEFAULTS
    (void)d_test_suite_skip(_suite, "its output is the default knobs'");
#else
    D_TEST_CHECK(_suite, (d_log_wants(&d_log_code_tnet_not_found, 0u) == 0) &&
                         (d_log_wants(&d_log_code_tnet_connect_timeout, 0u)
                          == 1) &&
                         (d_log_wants(&d_log_code_tnet_not_found,
                                      D_LOG_ERROR) == 1) &&
                         (d_log_wants(NULL, 0u) == 0));
    D_LOG(tnet, not_found, D_LOG_FACT_TEXT("host", "x"));
    D_TEST_CHECK(_suite, d_tests_log_is(""));

    D_TEST_CHECK(_suite, d_tests_log_put("t_net.level=debug"));
    D_TEST_CHECK(_suite, d_log_wants(&d_log_code_tnet_not_found, 0u) == 1);
    D_TEST_CHECK(_suite, d_tests_log_put("-t_net.level"));
    D_TEST_CHECK(_suite, d_tests_log_put("level=5"));
    D_TEST_CHECK(_suite, d_log_wants(&d_log_code_tnet_not_found, 0u) == 1);
    D_TEST_CHECK(_suite, d_tests_log_put("level=error"));
    D_TEST_CHECK(_suite, (d_log_wants(&d_log_code_tnet_bad_port, 0u) == 0) &&
                         d_tests_log_put("grade_rejected=fatal") &&
                         (d_log_wants(&d_log_code_tnet_bad_port, 0u) == 1));
    D_TEST_CHECK(_suite, !d_tests_log_put("level=loud") &&
                         !d_tests_log_put("level=25"));
#endif

    return;
}

/*
d_tests_log_text
  Checks the following:
  - the text writer: time, level, code, body, then the facts as key=value
  - a template's conditional, and a fact it names that is missing
  - a nested fact dotted; the code by number and both
*/
static void
d_tests_log_text(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "the text writer");
    d_tests_log_reset();

#if !D_TESTS_LOG_DEFAULTS
    (void)d_test_suite_skip(_suite, "its output is the default knobs'");
#else
    D_LOG(tnet, connect_timeout, D_LOG_FACT_TEXT("host", "db 1"),
                                 D_LOG_FACT_INT("timeout_ms", 500));
    D_TEST_CHECK(_suite, d_tests_log_is(D_TESTS_LOG_TEXT));

    d_tests_log_reset();
    D_LOG(tnet, bad_port, D_LOG_FACT_INT("port", 70000),
                          D_LOG_FACT_TEXT("why", "too big"));
    D_LOG(tnet, bad_port, D_LOG_FACT_INT("port", 0));
    D_TEST_CHECK(_suite, d_tests_log_is(
        "2025-11-13T15:04:05.123Z warning t.net.bad_port: port 70000 is not "
        "one: too big port=70000 why=\"too big\"\n"
        "2025-11-13T15:04:05.123Z warning t.net.bad_port: port 0 is not one "
        "port=0\n"));

    d_tests_log_reset();
    d_tests_log_peer_record();
    (void)d_tests_log_put("code_style=both");
    D_LOG(tnet, connect_timeout);
    (void)d_tests_log_put("code_style=number");
    D_LOG(tnet, connect_timeout);
    D_TEST_CHECK(_suite, d_tests_log_is(
        "2025-11-13T15:04:05.123Z error t.net.connect_timeout: connecting to "
        "h took over 500 ms host=h timeout_ms=500 peer.addr=10.0.0.1 "
        "peer.rtt=1.5\n"
        "2025-11-13T15:04:05.123Z error t.net.connect_timeout(207): "
        "connecting to  took over  ms\n"
        "2025-11-13T15:04:05.123Z error t.net:207: connecting to  took over "
        " ms\n"));
#endif

    return;
}

/*
d_tests_log_classes
  Checks the following:
  - the four classes of the log's decision 81 by their names
  - every class has a name of its own, dotted but for "none", and a
    number past the vocabulary is written "?"
*/
static void
d_tests_log_classes(
    struct dj_test_suite* _suite
)
{
    uint32_t    i;
    uint32_t    j;
    bool        named  = true;
    bool        unique = true;
    const char* name;

    (void)d_test_suite_test(_suite, "the class vocabulary");

    D_TEST_CHECK(_suite,
        (strcmp(d_log_class_name(D_LOG_CLASS_IO_INTERRUPTED),
                "io.interrupted") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_RESOURCE_IN_USE),
                "resource.in_use") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_ACCESS_DENIED),
                "access.denied") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_ACCESS_UNAUTHENTICATED),
                "access.unauthenticated") == 0));

    for (i = 0u; i < (uint32_t)D_LOG_CLASS_COUNT; ++i)
    {
        name = d_log_class_name(i);

        if ( (name == NULL) ||
             ( (i != (uint32_t)D_LOG_CLASS_NONE) &&
               (strchr(name, '.') == NULL) ) )
        {
            named = false;
        }

        for (j = 0u; (name != NULL) && (j < i); ++j)
        {
            if (strcmp(name, d_log_class_name(j)) == 0)
            {
                unique = false;
            }
        }
    }

    D_TEST_CHECK(_suite, named);
    D_TEST_CHECK(_suite, unique);
    D_TEST_CHECK(_suite,
        strcmp(d_log_class_name((uint32_t)D_LOG_CLASS_COUNT), "?") == 0);

    return;
}

/*
d_tests_log_signed
  Checks the following:
  - a code's number is its enum's, below zero too: found by it, and written
    with its sign by number ("t.db:-6")
  - the classes ruled on 11-17 by their names
  - log.real_digits rounds a real fact before every writer, and leaves NaN
*/
static void
d_tests_log_signed(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "signed numbers, new classes, real digits");
    d_tests_log_reset();

    D_TEST_CHECK(_suite, d_log_module_add(&d_log_module_tdb) == 0);
    D_TEST_CHECK(_suite, (d_log_code_find("t.db", 4u, -6) ==
                          &d_log_code_tdb_wait) &&
                         (d_log_code_find("t.db", 4u, -1) ==
                          &d_log_code_tdb_closed) &&
                         (d_log_code_find("t.db", 4u, 6) == NULL) &&
                         (d_log_code_tdb_wait.number == -6));
    D_TEST_CHECK(_suite,
        (strcmp(d_log_class_name(D_LOG_CLASS_LIFECYCLE_FROZEN),
                "lifecycle.frozen") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_CONSTRAINT_VIOLATED),
                "constraint.violated") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_IO_TIMED_OUT),
                "io.timed_out") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_IO_WOULD_BLOCK),
                "io.would_block") == 0) &&
        (strcmp(d_log_class_name(D_LOG_CLASS_SYNC_WOULD_BLOCK),
                "sync.would_block") == 0));

#if !D_TESTS_LOG_DEFAULTS
    (void)d_test_suite_skip(_suite, "its output is the default knobs'");
#else
    (void)d_tests_log_put("code_style=number");
    D_LOG(tdb, wait, D_LOG_FACT_TEXT("table", "jobs"));
    (void)d_ostream_flush(&d_tests_log_out.os);
    D_TEST_CHECK(_suite, strstr(d_tests_log_out.text, "t.db:-6") != NULL);

    d_tests_log_reset();
    (void)d_tests_log_put("format=json");
    D_TEST_CHECK(_suite, d_tests_log_put("real_digits=3"));
    D_TEST_CHECK(_suite, !d_tests_log_put("real_digits=18"));
    {
        const double zero = 0.0;

        D_LOG(tnet, bad_port, D_LOG_FACT_INT("port", 1),
                              D_LOG_FACT_REAL("load", 12.4996),
                              D_LOG_FACT_REAL("spare", zero / zero));
    }
    D_TEST_CHECK(_suite, d_tests_log_is(
        "{\"timestamp\":\"2025-11-13T15:04:05.123Z\",\"severityText\":"
        "\"warning\",\"severityNumber\":13,\"body\":\"port 1 is not one\","
        "\"attributes\":{\"code\":\"t.net.bad_port\",\"module\":"
        "\"t.net\",\"port\":1,\"load\":12.5,\"spare\":null},"
        "\"scope\":{\"name\":\"t.net\"}}\n"));
#endif

    return;
}

/*
d_tests_log_json
  Checks the following:
  - JSON lines with OpenTelemetry's data model's names; the code and its
    module first among the attributes; a quote escaped by the stream
  - nested facts as an object, or dotted (log.nesting)
  - integers as strings (log.json_integers), non-finite reals as strings
    (log.json_nonfinite), the resource (log.resource)
*/
static void
d_tests_log_json(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "the JSON lines writer");
    d_tests_log_reset();

#if !D_TESTS_LOG_DEFAULTS
    (void)d_test_suite_skip(_suite, "its output is the default knobs'");
#else
    D_TEST_CHECK(_suite, d_tests_log_put("format=json"));
    D_LOG(tnet, connect_timeout, D_LOG_FACT_TEXT("host", "db\"1"),
                                 D_LOG_FACT_INT("timeout_ms", 500));
    d_tests_log_peer_record();
    D_TEST_CHECK(_suite, d_tests_log_is(
        "{\"timestamp\":\"2025-11-13T15:04:05.123Z\",\"severityText\":"
        "\"error\",\"severityNumber\":17,\"body\":\"connecting to db\\\"1 "
        "took over 500 ms\",\"attributes\":{\"code\":"
        "\"t.net.connect_timeout\",\"module\":\"t.net\",\"host\":\"db\\\"1\","
        "\"timeout_ms\":500},\"scope\":{\"name\":\"t.net\"}}\n"
        "{\"timestamp\":\"2025-11-13T15:04:05.123Z\",\"severityText\":"
        "\"error\",\"severityNumber\":17,\"body\":\"connecting to h took "
        "over 500 ms\",\"attributes\":{\"code\":\"t.net.connect_timeout\","
        "\"module\":\"t.net\",\"host\":\"h\",\"timeout_ms\":500,\"peer\":"
        "{\"addr\":\"10.0.0.1\",\"rtt\":1.5}},\"scope\":{\"name\":"
        "\"t.net\"}}\n"));

    d_tests_log_reset();
    (void)d_tests_log_put("format=json");
    (void)d_tests_log_put("nesting=dotted");
    (void)d_tests_log_put("json_integers=string");
    (void)d_tests_log_put("json_nonfinite=string");
    (void)d_tests_log_put("scope=false");
    (void)d_tests_log_put("severity_number=false");
    {
        const double zero = 0.0;

        D_LOG(tnet, bad_port, D_LOG_FACT_INT("port", 1),
                              D_LOG_FACT_REAL("load", zero / zero));
    }
    d_tests_log_peer_record();
    D_TEST_CHECK(_suite, d_tests_log_is(
        "{\"timestamp\":\"2025-11-13T15:04:05.123Z\",\"severityText\":"
        "\"warning\",\"body\":\"port 1 is not one\",\"attributes\":{\"code\":"
        "\"t.net.bad_port\",\"module\":\"t.net\",\"port\":\"1\",\"load\":"
        "\"NaN\"}}\n"
        "{\"timestamp\":\"2025-11-13T15:04:05.123Z\",\"severityText\":"
        "\"error\",\"body\":\"connecting to h took over 500 ms\","
        "\"attributes\":{\"code\":\"t.net.connect_timeout\",\"module\":"
        "\"t.net\",\"host\":\"h\",\"timeout_ms\":\"500\",\"peer.addr\":"
        "\"10.0.0.1\",\"peer.rtt\":1.5}}\n"));

    // the resource: the service, and the process where there is one
    d_tests_log_reset();
    (void)d_tests_log_put("format=json");
    (void)d_tests_log_put("resource=true");
    (void)d_tests_log_put("service=probe");
    D_LOG(tnet, connect_timeout);
    (void)d_ostream_flush(&d_tests_log_out.os);
    D_TEST_CHECK(_suite, strstr(d_tests_log_out.text,
                                ",\"resource\":{\"service.name\":\"probe\"")
                         != NULL);
#endif

    return;
}

/*
d_tests_log_otlp
  Checks the following:
  - one ExportLogsServiceRequest a line: the resource, the module's scope,
    the record; times and integers as decimal strings; a nested fact as a
    kvlistValue; a double as doubleValue
*/
static void
d_tests_log_otlp(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "the OTLP/JSON writer");
    d_tests_log_reset();

#if !D_TESTS_LOG_DEFAULTS || !(defined(__unix__) || defined(__APPLE__))
    (void)d_test_suite_skip(_suite, "its output is the default knobs', with "
                                    "a process id");
#else
    {
        char expected[2048];

        (void)snprintf(expected, sizeof(expected),
            "{\"resourceLogs\":[{\"resource\":{\"attributes\":[{\"key\":"
            "\"service.name\",\"value\":{\"stringValue\":\"probe\"}},{\"key\":"
            "\"process.pid\",\"value\":{\"intValue\":\"%ld\"}}]},\"scopeLogs\":"
            "[{\"scope\":{\"name\":\"t.net\"},\"logRecords\":[{"
            "\"timeUnixNano\":\"1763046245123456789\",\"severityNumber\":17,"
            "\"severityText\":\"error\",\"body\":{\"stringValue\":"
            "\"connecting to h took over 500 ms\"},\"attributes\":[{\"key\":"
            "\"code\",\"value\":{\"stringValue\":\"t.net.connect_timeout\"}},"
            "{\"key\":\"module\",\"value\":{\"stringValue\":\"t.net\"}},"
            "{\"key\":\"host\",\"value\":{\"stringValue\":\"h\"}},{\"key\":"
            "\"timeout_ms\",\"value\":{\"intValue\":\"500\"}},{\"key\":"
            "\"peer\",\"value\":{\"kvlistValue\":{\"values\":[{\"key\":"
            "\"addr\",\"value\":{\"stringValue\":\"10.0.0.1\"}},{\"key\":"
            "\"rtt\",\"value\":{\"doubleValue\":1.5}}]}}}]}]}]}]}\n",
            D_TESTS_LOG_PID());
        (void)d_tests_log_put("format=otlp");
        (void)d_tests_log_put("service=probe");
        d_tests_log_peer_record();
        D_TEST_CHECK(_suite, d_tests_log_is(expected));
    }
#endif

    return;
}

/*
d_tests_log_caps
  Checks the following:
  - past log.facts_max facts are counted, and written as the dropped count
  - past log.depth_max a describer's levels are refused
  - a body past D_CFG_LOG_BODY_MAX is cut where a character starts
*/
static void
d_tests_log_caps(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "the caps");
    d_tests_log_reset();

#if !D_TESTS_LOG_DEFAULTS
    (void)d_test_suite_skip(_suite, "its output is the default knobs'");
#else
    (void)d_tests_log_put("facts_max=1");
    D_LOG(tnet, connect_timeout, D_LOG_FACT_TEXT("host", "a"),
                                 D_LOG_FACT_INT("timeout_ms", 1),
                                 D_LOG_FACT_BOOL("retry", 1));
    (void)d_tests_log_put("facts_max=8");
    (void)d_tests_log_put("depth_max=0");
    d_tests_log_peer_record();
    D_TEST_CHECK(_suite, d_tests_log_is(
        "2025-11-13T15:04:05.123Z error t.net.connect_timeout: connecting to "
        "a took over  ms host=a truncated=2\n"
        "2025-11-13T15:04:05.123Z error t.net.connect_timeout: connecting to "
        "h took over 500 ms host=h timeout_ms=500\n"));
    D_TEST_CHECK(_suite, !d_tests_log_put("facts_max=100000"));

    // a body cut inside a two-byte character keeps no half of it
    d_tests_log_reset();
    {
        char host[D_CFG_LOG_BODY_MAX + 8];
        size_t at;

        for (at = 0u; at + 2u < sizeof(host); at += 2u)
        {
            host[at]      = (char)0xC3;
            host[at + 1u] = (char)0xA9;
        }

        host[at] = '\0';
        D_LOG(tnet, not_found, D_LOG_FACT_TEXT("host", host));
        (void)d_tests_log_put("level=debug");
        D_LOG(tnet, not_found, D_LOG_FACT_TEXT("host", host));
        (void)d_ostream_flush(&d_tests_log_out.os);
        D_TEST_CHECK(_suite,
            (strstr(d_tests_log_out.text, "no route to \xC3\xA9") != NULL) &&
            (strstr(d_tests_log_out.text, "\xC3\xA9 host=") != NULL));
    }
#endif

    return;
}

// dj_tests_log_seen
//   struct: what a program's writer saw.
struct dj_tests_log_seen
{
    unsigned  records;
    uint32_t  severity;
    char      body[64];
    size_t    facts;
    uint64_t  time;
    uint64_t  observed;
};

/*
d_tests_log_writer_record
  A program's writer: it keeps what it is given.
*/
static int
d_tests_log_writer_record(
    void*                      _context,
    struct dj_ostream*         _os,
    const struct dj_log_entry* _entry
)
{
    struct dj_tests_log_seen* const seen = (struct dj_tests_log_seen*)_context;

    (void)_os;
    ++seen->records;
    seen->severity = _entry->severity;
    seen->facts    = _entry->facts->count;
    seen->time     = _entry->time;
    seen->observed = _entry->observed;
    (void)snprintf(seen->body, sizeof(seen->body), "%.*s",
                   (int)_entry->body_size, _entry->body);

    return 0;
}

/*
d_tests_log_programs
  Checks the following:
  - a program's writer replaces log.format's, and is given the record
  - a record of a code found at run time, at a level given
  - log.destination=none writes nothing, and fails nothing
  - a file: records appended, whole lines
  - a stream that failed counts the record as a failure
*/
static void
d_tests_log_programs(
    struct dj_test_suite* _suite
)
{
    static const struct dj_log_writer WRITER =
        { NULL, d_tests_log_writer_record, NULL, NULL };
    struct dj_tests_log_seen seen;
    const struct dj_desc_fact facts[] =
    {
        D_LOG_FACT_TEXT("host", "found"),
        D_LOG_FACT_UINT("timeout_ms", 9u)
    };

    (void)d_test_suite_test(_suite, "programs' writers and destinations");
    d_tests_log_reset();
    memset(&seen, 0, sizeof(seen));
    (void)d_log_module_add(&d_log_module_tnet);

    d_log_set_writer(&WRITER, &seen);
    D_TEST_CHECK(_suite, d_log_emit(d_log_code_find("t.net", 5u, 207u),
                                    D_LOG_FATAL, facts, 2u) == 0);
#if D_CFG_LOG_ENABLED && (D_CFG_LOG_LEVEL_MIN <= 21)
    D_TEST_CHECK(_suite, (seen.records == 1u) &&
                         (seen.severity == D_LOG_FATAL) &&
                         (seen.facts == ( (D_CFG_LOG_FACTS_MAX >= 2) ? 2u
                                          : (size_t)D_CFG_LOG_FACTS_MAX )));
#if D_CFG_LOG_CODES && (D_CFG_LOG_FACTS_MAX >= 2)
    D_TEST_CHECK(_suite, strcmp(seen.body,
                                "connecting to found took over 9 ms") == 0);
#endif
#else
    D_TEST_CHECK(_suite, seen.records == 0u);
#endif
    d_log_set_writer(NULL, NULL);

#if D_TESTS_LOG_DEFAULTS
    // none, then a file
    {
        const char* const path = "log_tests_destination.log";
        char              text[512];
        FILE*             file;
        size_t            got = 0u;

        d_log_set_stream(NULL);
        (void)remove(path);
        D_TEST_CHECK(_suite, d_tests_log_put("destination=none") &&
                             (d_log_init() == 0));
        D_LOG(tnet, connect_timeout);
        D_TEST_CHECK(_suite, d_tests_log_put("destination="
                                             "log_tests_destination.log") &&
                             (d_log_init() == 0));
        D_LOG(tnet, connect_timeout, D_LOG_FACT_TEXT("host", "db 1"),
                                     D_LOG_FACT_INT("timeout_ms", 500));
        D_LOG(tnet, connect_timeout, D_LOG_FACT_TEXT("host", "db 1"),
                                     D_LOG_FACT_INT("timeout_ms", 500));
        d_log_shutdown();
        file = d_file_open_stream(path, "rb");

        if (file != NULL)
        {
            got = fread(text, 1u, sizeof(text) - 1u, file);
            (void)d_file_close_stream(file);
        }

        text[got] = '\0';
        D_TEST_CHECK(_suite, strcmp(text, D_TESTS_LOG_TEXT D_TESTS_LOG_TEXT)
                             == 0);
        (void)remove(path);
        d_tests_log_reset();
    }

    // a stream that failed
    {
        const uint64_t before = d_log_failures();

        (void)d_ostream_close(&d_tests_log_out.os);
        D_LOG(tnet, connect_timeout);
        D_TEST_CHECK(_suite, d_log_failures() == before + 1u);
        d_tests_log_reset();
    }
#endif

    return;
}

/*
d_tests_log_times
  Checks the following:
  - RFC 3339 in UTC at each precision, from the epoch, across a leap day,
    and at a year's last second
  - local time with its offset, in a zone of a known offset
*/
static void
d_tests_log_times(
    struct dj_test_suite* _suite
)
{
    char                    text[64];
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    const uint64_t          leap = (uint64_t)1709208000u * 1000000000u + 5u;

    (void)d_test_suite_test(_suite, "times");

#define D_TESTS_TIME_IS(time, how, expected)                                  \
    ( (void)d_ostream_init_fixed(&os, &fixed, text, sizeof(text), 1),         \
      d_log_write_time(&os, (time), (how)),                                   \
      (void)d_ostream_close(&os),                                             \
      (strcmp(text, (expected)) == 0) )

    D_TEST_CHECK(_suite, D_TESTS_TIME_IS(0u, 0u, "1970-01-01T00:00:00Z"));
    D_TEST_CHECK(_suite, D_TESTS_TIME_IS(D_TESTS_TIME, 1u,
                                         "2025-11-13T15:04:05.123Z"));
    D_TEST_CHECK(_suite, D_TESTS_TIME_IS(D_TESTS_TIME, 2u,
                                         "2025-11-13T15:04:05.123456Z"));
    D_TEST_CHECK(_suite, D_TESTS_TIME_IS(D_TESTS_TIME, 3u,
                                         "2025-11-13T15:04:05.123456789Z"));
    D_TEST_CHECK(_suite, D_TESTS_TIME_IS(leap, 3u,
                                         "2024-02-29T12:00:00.000000005Z"));
    D_TEST_CHECK(_suite, D_TESTS_TIME_IS((uint64_t)4102444799u *
                                         1000000000u, 0u,
                                         "2099-12-31T23:59:59Z"));
#if defined(__unix__) || defined(__APPLE__)
    {
        // the zone as it was, read through option_env's own reader; a
        // zone too long to keep whole is left as it is, and these checks
        // skipped
        char          kept[64];
        size_t        length = 0u;
        const int32_t zone   = d_option_env_getenv_c(NULL, "TZ", kept,
                                                     sizeof(kept),
                                                     &length);

        (void)length;

        if (zone >= 0)
        {
            (void)setenv("TZ", "UTC+5", 1);
            tzset();
            D_TEST_CHECK(_suite, D_TESTS_TIME_IS(D_TESTS_TIME, 5u,
                             "2025-11-13T10:04:05.123-05:00"));
            (void)setenv("TZ", "UTC-5:30", 1);
            tzset();
            D_TEST_CHECK(_suite, D_TESTS_TIME_IS(D_TESTS_TIME, 4u,
                             "2025-11-13T20:34:05+05:30"));

            if (zone == 1)
            {
                (void)setenv("TZ", kept, 1);
            }
            else
            {
                (void)unsetenv("TZ");
            }

            tzset();
        }
    }
#endif
#undef D_TESTS_TIME_IS

    return;
}


/*
d_tests_log_held
  Checks the following:
  - a record held open while another is written: both written, no lock held
  - a held record's time is when it opened; its observed time, when it was
    written, where log.observed_time is on
*/
static void
d_tests_log_held(
    struct dj_test_suite* _suite
)
{
    static const struct dj_log_writer WRITER =
        { NULL, d_tests_log_writer_record, NULL, NULL };
    struct dj_tests_log_seen seen;
    struct dj_log_record     held;
    struct dj_log_settings   settings;
    unsigned                 readings = 0u;
    int                      opened;

    (void)d_test_suite_test(_suite, "a record held open: its times");
    d_tests_log_reset();
    (void)d_tests_log_put("observed_time=1");
    memset(&seen, 0, sizeof(seen));
    (void)d_log_module_add(&d_log_module_tnet);
    d_log_set_writer(&WRITER, &seen);
    d_log_set_clock(d_tests_log_step, &readings);
    d_log_settings(&settings, &d_log_module_tnet);

    // opened at the first reading; another record, readings 2 and 3
    opened = d_log_open(&held, &d_log_code_tnet_connect_timeout, D_LOG_FATAL);
    (void)d_log_emit(&d_log_code_tnet_bad_port, D_LOG_FATAL, NULL, 0u);
    D_TEST_CHECK(_suite, seen.records == ( (opened != 0) ? 1u : 0u ));

    if (opened != 0)
    {
        (void)d_log_close(&held);
        D_TEST_CHECK(_suite, seen.records == 2u);
        D_TEST_CHECK(_suite, seen.time == D_TESTS_TIME + 1000000000u);
        D_TEST_CHECK(_suite, seen.observed ==
                             ( (settings.observed_time != 0u)
                                   ? D_TESTS_TIME + 4000000000u
                                   : seen.time ));
    }

    d_log_set_writer(NULL, NULL);
    d_log_set_clock(d_tests_log_clock, NULL);

    return;
}

/*
d_tests_log_checked
  Checks the following:
  - a module whose templates compile passes d_log_module_check
  - a template naming a fact its code does not list fails, named
  - a template with a block unclosed fails, named
  - with templates not built, every module passes
*/
static void
d_tests_log_checked(
    struct dj_test_suite* _suite
)
{
    const struct dj_log_code*        bad = NULL;
    struct dj_ostream_template_error error;

    (void)d_test_suite_test(_suite, "templates checked: d_log_module_check");
    D_TEST_CHECK(_suite, d_log_module_check(&d_log_module_tnet, &bad, NULL) ==
                         (int32_t)D_OSTREAM_TEMPLATE_OK);
    D_TEST_CHECK(_suite, bad == NULL);
    D_TEST_CHECK(_suite, d_log_module_check(NULL, &bad, NULL) ==
                         (int32_t)D_OSTREAM_TEMPLATE_INVALID);
#if D_CFG_LOG_CODES
    D_TEST_CHECK(_suite, d_log_module_check(&d_log_module_tbad, &bad,
                                            &error) ==
                         (int32_t)D_OSTREAM_TEMPLATE_UNKNOWN_FIELD);
    D_TEST_CHECK(_suite, bad == &d_log_code_tbad_unknown);
    D_TEST_CHECK(_suite, d_log_module_check(&d_log_module_topen, &bad,
                                            NULL) ==
                         (int32_t)D_OSTREAM_TEMPLATE_NESTING);
    D_TEST_CHECK(_suite, bad == &d_log_code_topen_unclosed);
#else
    D_TEST_CHECK(_suite, d_log_module_check(&d_log_module_tbad, &bad,
                                            &error) ==
                         (int32_t)D_OSTREAM_TEMPLATE_OK);
    D_TEST_CHECK(_suite, d_log_module_check(&d_log_module_topen, &bad,
                                            NULL) ==
                         (int32_t)D_OSTREAM_TEMPLATE_OK);
#endif

    return;
}


//==============================================================================
// THE SECTION
//==============================================================================

void
d_tests_log(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_module(_suite, "log");
    (void)d_test_suite_block(_suite, "log");

    d_tests_log_any_build(_suite);
    d_tests_log_levels(_suite);
    d_tests_log_codes(_suite);
    d_tests_log_signed(_suite);
    d_tests_log_classes(_suite);
    d_tests_log_threshold(_suite);
    d_tests_log_text(_suite);
    d_tests_log_json(_suite);
    d_tests_log_otlp(_suite);
    d_tests_log_caps(_suite);
    d_tests_log_programs(_suite);
    d_tests_log_times(_suite);
    d_tests_log_held(_suite);
    d_tests_log_checked(_suite);

    d_log_set_stream(NULL);
    d_log_set_clock(NULL, NULL);

    return;
}
