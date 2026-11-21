/*******************************************************************************
* djinterp [test]                                                net_log_tests.c
*
*   The suite of net's codes for the log (net_log.h), in DTest's C face: the
* table's rows against dj_net_error and against the classes and grades
* ruled for them (the log's decisions 78 and 81, net's 50), its templates,
* and its registration.
*
*
* path:      /tests/djinterp/c/modules/net/net_log_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./net_log_tests.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/net/net_log.h"  // under test


//==============================================================================
// HARNESS
//==============================================================================

// D_TESTS_NET_LOG_CHECK
//   macro: records one check against the open test.
#define D_TESTS_NET_LOG_CHECK(_cond)  D_TEST_CHECK(g_suite, _cond)

// g_suite
//   state: the suite every check goes to, set by the section.
static struct dj_test_suite* g_suite = NULL;

// dj_tests_net_log_row
//   struct: what a row must be: its enumerator, its code, its class and
// grade as ruled.
struct dj_tests_net_log_row
{
    int32_t                   number;
    const struct dj_log_code* code;
    uint16_t                  class_id;
    uint16_t                  grade;
};

/*
d_tests_net_log_rows
  Every row against what it must be: numbered as its enumerator, of its
table, with a name, its class and its grade; and the table holds them all,
in their order.
*/
static
void d_tests_net_log_rows(
    const struct dj_log_module*        _module,
    const struct dj_tests_net_log_row* _rows,
    size_t                             _count
)
{
    D_TESTS_NET_LOG_CHECK(_module->count == _count);

    for (size_t i = 0u; i < _count; ++i)
    {
        const struct dj_log_code* code = _rows[i].code;

        D_TESTS_NET_LOG_CHECK( (code->number == _rows[i].number)     &&
                               (code->module == _module)             &&
                               (code->class_id == _rows[i].class_id) &&
                               (code->grade == _rows[i].grade) );
        D_TESTS_NET_LOG_CHECK( (i < _module->count) &&
                               (_module->codes[i] == code) );
#if D_CFG_LOG_CODES
        D_TESTS_NET_LOG_CHECK( (code->name != NULL) &&
                               (code->text != NULL) &&
                               (code->facts != NULL) );
#endif  // D_CFG_LOG_CODES
    }

    return;
}

/*
d_tests_net_log_templates
  Every template compiles against its facts.
*/
static
void d_tests_net_log_templates(
    const struct dj_log_module* _module
)
{
    const struct dj_log_code*       bad = NULL;
    struct dj_ostream_template_error error;

    D_TESTS_NET_LOG_CHECK(d_log_module_check(_module, &bad, &error) ==
                          D_OSTREAM_TEMPLATE_OK);
    D_TESTS_NET_LOG_CHECK(bad == NULL);

    return;
}


//==============================================================================
// TESTS
//==============================================================================

/*
d_tests_net_log_table
  The table.
  Tests the following:
  - 21 rows, one for each code but D_NET_OK, numbered as dj_net_error
    numbers it, in order, of the table "net"
  - each row's class and grade as ruled
  - every template compiles against its facts
*/
static
void d_tests_net_log_table(void)
{
    static const struct dj_tests_net_log_row rows[] =
    {
        { D_NET_ERROR_CLOSED, &d_log_code_net_closed,
          D_LOG_CLASS_FORMAT_TRUNCATED, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_WOULD_BLOCK, &d_log_code_net_would_block,
          D_LOG_CLASS_IO_WOULD_BLOCK, D_LOG_GRADE_SOFT },
        { D_NET_ERROR_TIMED_OUT, &d_log_code_net_timed_out,
          D_LOG_CLASS_IO_TIMED_OUT, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_INTERRUPTED, &d_log_code_net_interrupted,
          D_LOG_CLASS_IO_INTERRUPTED, D_LOG_GRADE_SOFT },
        { D_NET_ERROR_CONNECTION_RESET, &d_log_code_net_connection_reset,
          D_LOG_CLASS_IO_RESET, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_CONNECTION_REFUSED, &d_log_code_net_connection_refused,
          D_LOG_CLASS_IO_REFUSED, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_CONNECTION_ABORTED, &d_log_code_net_connection_aborted,
          D_LOG_CLASS_IO_FAILED, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_ADDRESS_IN_USE, &d_log_code_net_address_in_use,
          D_LOG_CLASS_RESOURCE_IN_USE, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_ADDRESS_INVALID, &d_log_code_net_address_invalid,
          D_LOG_CLASS_FORMAT_MALFORMED, D_LOG_GRADE_REJECTED },
        { D_NET_ERROR_HOST_UNREACHABLE, &d_log_code_net_host_unreachable,
          D_LOG_CLASS_IO_UNREACHABLE, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_NETWORK_DOWN, &d_log_code_net_network_down,
          D_LOG_CLASS_IO_UNREACHABLE, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_ACCESS_DENIED, &d_log_code_net_access_denied,
          D_LOG_CLASS_ACCESS_DENIED, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_INVALID_ARGUMENT, &d_log_code_net_invalid_argument,
          D_LOG_CLASS_ARGUMENT_INVALID, D_LOG_GRADE_REJECTED },
        { D_NET_ERROR_MESSAGE_TOO_LARGE, &d_log_code_net_message_too_large,
          D_LOG_CLASS_QUANTITY_SIZE, D_LOG_GRADE_REJECTED },
        { D_NET_ERROR_TOO_MANY_OPEN_FILES, &d_log_code_net_too_many_open_files,
          D_LOG_CLASS_RESOURCE_BUDGET, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_OUT_OF_MEMORY, &d_log_code_net_out_of_memory,
          D_LOG_CLASS_RESOURCE_MEMORY, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_UNKNOWN, &d_log_code_net_unknown,
          D_LOG_CLASS_GENERIC_CALLEE, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_PROTOCOL, &d_log_code_net_protocol,
          D_LOG_CLASS_FORMAT_MALFORMED, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_UNSUPPORTED, &d_log_code_net_unsupported,
          D_LOG_CLASS_OPERATION_UNSUPPORTED, D_LOG_GRADE_REJECTED },
        { D_NET_ERROR_ADDRESS_UNAVAILABLE, &d_log_code_net_address_unavailable,
          D_LOG_CLASS_LOOKUP_NOT_FOUND, D_LOG_GRADE_FAILED },
        { D_NET_ERROR_HOST_NOT_FOUND, &d_log_code_net_host_not_found,
          D_LOG_CLASS_LOOKUP_NOT_FOUND, D_LOG_GRADE_FAILED }
    };

    (void)d_test_suite_block(g_suite, "net_log");
    (void)d_test_suite_test(g_suite, "the table");

    D_TESTS_NET_LOG_CHECK(strcmp(d_log_module_net.name, "net") == 0);
    d_tests_net_log_rows(&d_log_module_net,
                         rows,
                         sizeof(rows) / sizeof(rows[0]));
    d_tests_net_log_templates(&d_log_module_net);

    return;
}

/*
d_tests_net_log_add
  Registering.
  Tests the following:
  - the table registers, and again without harm
  - then a code is found by its number, and none for D_NET_OK
*/
static
void d_tests_net_log_add(void)
{
    (void)d_test_suite_test(g_suite, "registering");

    D_TESTS_NET_LOG_CHECK(d_net_log_add() == 0);
    D_TESTS_NET_LOG_CHECK(d_net_log_add() == 0);
    D_TESTS_NET_LOG_CHECK(
        d_log_code_find("net", 3u, (int32_t)D_NET_ERROR_HOST_NOT_FOUND) ==
        &d_log_code_net_host_not_found);
    D_TESTS_NET_LOG_CHECK(
        d_log_code_find("net", 3u, (int32_t)D_NET_ERROR_CLOSED) ==
        &d_log_code_net_closed);
    D_TESTS_NET_LOG_CHECK(
        d_log_code_find("net", 3u, (int32_t)D_NET_OK) == NULL);

    return;
}


//==============================================================================
// RUN
//==============================================================================

/*
d_tests_net_log
  The suite's section.
*/
void d_tests_net_log(
    struct dj_test_suite* _suite
)
{
    g_suite = _suite;

    (void)d_test_suite_module(_suite, "net_log");
    (void)d_test_suite_describe(_suite, "net's codes for the log");

    d_tests_net_log_table();
    d_tests_net_log_add();

    g_suite = NULL;

    return;
}
