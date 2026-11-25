/*******************************************************************************
* djinterp [c][db]                                                db_log_tests.c
*
*   The suite of db's codes for the log (db_log.h), in DTest's C face: both
* tables' rows against their enumerations and against the classes and grades
* ruled for them (the log's decisions 57, 75, 78 and 81), their templates,
* their registration, and the vendor's message as the knob allows it.
*
*
* path:      /tests/djinterp/c/modules/db/db_log_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./db_log_tests.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strcmp
// djinterp
#include "../../../../../inc/djinterp/c/modules/db/db_log.h"  // under test


//==============================================================================
// HARNESS
//==============================================================================

// D_TESTS_DB_LOG_CHECK
//   macro: records one check against the open test.
#define D_TESTS_DB_LOG_CHECK(_cond)  D_TEST_CHECK(g_suite, _cond)

// g_suite
//   state: the suite every check goes to, set by the section.
static struct dj_test_suite* g_suite = NULL;

// dj_tests_db_log_row
//   struct: what a row must be: its enumerator, its code, its class and
// grade as ruled.
struct dj_tests_db_log_row
{
    int32_t                   number;
    const struct dj_log_code* code;
    uint16_t                  class_id;
    uint16_t                  grade;
};

/*
d_tests_db_log_rows
  Every row against what it must be: numbered as its enumerator, of its
table, with a name, its class and its grade; and the table holds them all,
in their order.
*/
static
void d_tests_db_log_rows(
    const struct dj_log_module*        _module,
    const struct dj_tests_db_log_row* _rows,
    size_t                             _count
)
{
    D_TESTS_DB_LOG_CHECK(_module->count == _count);

    for (size_t i = 0u; i < _count; ++i)
    {
        const struct dj_log_code* code = _rows[i].code;

        D_TESTS_DB_LOG_CHECK( (code->number == _rows[i].number)     &&
                               (code->module == _module)             &&
                               (code->class_id == _rows[i].class_id) &&
                               (code->grade == _rows[i].grade) );
        D_TESTS_DB_LOG_CHECK( (i < _module->count) &&
                               (_module->codes[i] == code) );
#if D_CFG_LOG_CODES
        D_TESTS_DB_LOG_CHECK( (code->name != NULL) &&
                               (code->text != NULL) &&
                               (code->facts != NULL) );
#endif  // D_CFG_LOG_CODES
    }

    return;
}

/*
d_tests_db_log_templates
  Every template compiles against its facts.
*/
static
void d_tests_db_log_templates(
    const struct dj_log_module* _module
)
{
    const struct dj_log_code*       bad = NULL;
    struct dj_ostream_template_error error;

    D_TESTS_DB_LOG_CHECK(d_log_module_check(_module, &bad, &error) ==
                          D_OSTREAM_TEMPLATE_OK);
    D_TESTS_DB_LOG_CHECK(bad == NULL);

    return;
}


//==============================================================================
// TESTS
//==============================================================================

/*
d_tests_db_log_own
  db's own table.
  Tests the following:
  - six rows, numbered as dj_db_status numbers them, negative, in order, of
    the table "db"
  - each row's class and grade as ruled
  - every template compiles against its facts
*/
static
void d_tests_db_log_own(void)
{
    static const struct dj_tests_db_log_row rows[] =
    {
        { D_DB_UNSUPPORTED, &d_log_code_db_unsupported,
          D_LOG_CLASS_OPERATION_UNSUPPORTED, D_LOG_GRADE_REJECTED },
        { D_DB_EXISTS, &d_log_code_db_exists,
          D_LOG_CLASS_CONSTRAINT_VIOLATED, D_LOG_GRADE_FAILED },
        { D_DB_NOT_FOUND, &d_log_code_db_not_found,
          D_LOG_CLASS_LOOKUP_NOT_FOUND, D_LOG_GRADE_FAILED },
        { D_DB_INVALID_ARGUMENT, &d_log_code_db_invalid_argument,
          D_LOG_CLASS_ARGUMENT_INVALID, D_LOG_GRADE_REJECTED },
        { D_DB_MISMATCH, &d_log_code_db_mismatch,
          D_LOG_CLASS_TYPE_MISMATCH, D_LOG_GRADE_FAILED },
        { D_DB_WAIT, &d_log_code_db_wait,
          D_LOG_CLASS_IO_WOULD_BLOCK, D_LOG_GRADE_SOFT }
    };

    (void)d_test_suite_block(g_suite, "db_log");
    (void)d_test_suite_test(g_suite, "db's own");

    D_TESTS_DB_LOG_CHECK(strcmp(d_log_module_db.name, "db") == 0);
    d_tests_db_log_rows(&d_log_module_db,
                        rows,
                        sizeof(rows) / sizeof(rows[0]));
    d_tests_db_log_templates(&d_log_module_db);

    return;
}

/*
d_tests_db_log_vendor
  The kinds' table.
  Tests the following:
  - ten rows, one for each kind but none, numbered as dj_db_vendor_kind
    numbers them, in order, of the table "db.vendor"
  - each row's class and grade as ruled
  - every template compiles against its facts
*/
static
void d_tests_db_log_vendor(void)
{
    static const struct dj_tests_db_log_row rows[] =
    {
        { D_DB_VENDOR_KIND_CONNECTION, &d_log_code_db_vendor_connection,
          D_LOG_CLASS_IO_FAILED, D_LOG_GRADE_FAILED },
        { D_DB_VENDOR_KIND_AUTH, &d_log_code_db_vendor_auth,
          D_LOG_CLASS_ACCESS_UNAUTHENTICATED, D_LOG_GRADE_FAILED },
        { D_DB_VENDOR_KIND_CONSTRAINT, &d_log_code_db_vendor_constraint,
          D_LOG_CLASS_CONSTRAINT_VIOLATED, D_LOG_GRADE_FAILED },
        { D_DB_VENDOR_KIND_SYNTAX, &d_log_code_db_vendor_syntax,
          D_LOG_CLASS_SYNTAX_UNEXPECTED, D_LOG_GRADE_REJECTED },
        { D_DB_VENDOR_KIND_NOT_FOUND, &d_log_code_db_vendor_not_found,
          D_LOG_CLASS_LOOKUP_NOT_FOUND, D_LOG_GRADE_REJECTED },
        { D_DB_VENDOR_KIND_BUSY, &d_log_code_db_vendor_busy,
          D_LOG_CLASS_SYNC_BUSY, D_LOG_GRADE_SOFT },
        { D_DB_VENDOR_KIND_DATA, &d_log_code_db_vendor_data,
          D_LOG_CLASS_VALUE_RANGE, D_LOG_GRADE_REJECTED },
        { D_DB_VENDOR_KIND_UNSUPPORTED, &d_log_code_db_vendor_unsupported,
          D_LOG_CLASS_OPERATION_UNSUPPORTED, D_LOG_GRADE_REJECTED },
        { D_DB_VENDOR_KIND_INTERNAL, &d_log_code_db_vendor_internal,
          D_LOG_CLASS_GENERIC_CALLEE, D_LOG_GRADE_FAILED },
        { D_DB_VENDOR_KIND_DENIED, &d_log_code_db_vendor_denied,
          D_LOG_CLASS_ACCESS_DENIED, D_LOG_GRADE_FAILED }
    };

    (void)d_test_suite_test(g_suite, "the kinds");

    D_TESTS_DB_LOG_CHECK(strcmp(d_log_module_db_vendor.name, "db.vendor") ==
                         0);
    d_tests_db_log_rows(&d_log_module_db_vendor,
                        rows,
                        sizeof(rows) / sizeof(rows[0]));
    d_tests_db_log_templates(&d_log_module_db_vendor);

    return;
}

/*
d_tests_db_log_add
  Registering, and the vendor's message.
  Tests the following:
  - both tables register, and again without harm
  - then a code of each is found by its number, and none for success or
    for no kind
  - a vendor's message comes back as the knob allows: itself by default
*/
static
void d_tests_db_log_add(void)
{
    const char* const message = "duplicate key value";

    (void)d_test_suite_test(g_suite, "registering");

    D_TESTS_DB_LOG_CHECK(d_db_log_add() == 0);
    D_TESTS_DB_LOG_CHECK(d_db_log_add() == 0);
    D_TESTS_DB_LOG_CHECK(
        d_log_code_find("db", 2u, (int32_t)D_DB_NOT_FOUND) ==
        &d_log_code_db_not_found);
    D_TESTS_DB_LOG_CHECK(
        d_log_code_find("db", 2u, (int32_t)D_DB_OK) == NULL);
    D_TESTS_DB_LOG_CHECK(
        d_log_code_find("db.vendor",
                        9u,
                        (int32_t)D_DB_VENDOR_KIND_CONSTRAINT) ==
        &d_log_code_db_vendor_constraint);
    D_TESTS_DB_LOG_CHECK(
        d_log_code_find("db.vendor",
                        9u,
                        (int32_t)D_DB_VENDOR_KIND_NONE) == NULL);

    (void)d_test_suite_test(g_suite, "the vendor's message");

#if (D_INTERNAL_DB_LOG_VENDOR_MESSAGE == 1)
    D_TESTS_DB_LOG_CHECK(d_db_log_vendor_message(message) == message);
#else
    D_TESTS_DB_LOG_CHECK(d_db_log_vendor_message(message) == NULL);
#endif  // D_INTERNAL_DB_LOG_VENDOR_MESSAGE
    D_TESTS_DB_LOG_CHECK(d_db_log_vendor_message(NULL) == NULL);

    return;
}


//==============================================================================
// RUN
//==============================================================================

/*
d_tests_db_log
  The suite's section.
*/
void d_tests_db_log(
    struct dj_test_suite* _suite
)
{
    g_suite = _suite;

    (void)d_test_suite_module(_suite, "db_log");
    (void)d_test_suite_describe(_suite, "db's codes for the log");

    d_tests_db_log_own();
    d_tests_db_log_vendor();
    d_tests_db_log_add();

    g_suite = NULL;

    return;
}
