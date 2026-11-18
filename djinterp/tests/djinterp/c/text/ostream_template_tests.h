/*******************************************************************************
* djinterp [test]                                       ostream_template_tests.h
*
* The template engine's suite in DTest's C face (c/modules/test/test_suite.h).
*   One entry point builds the module "ostream_template" imperatively: a
* block for ostream_template.h, with a test per group of checks. The groups
* are in ostream_template_tests.c and ostream_template_tests_json.c, which
* share the helpers below.
*
*
* TABLE OF CONTENTS
* =================
* 1.  ENTRY POINT
* 2.  SHARED
*     a.  checks and helpers (ostream_template_tests.c)
*     b.  JSON's groups (ostream_template_tests_json.c)
*
*
* path:      /tests/djinterp/c/text/ostream_template_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_TESTS_H
#define DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_TESTS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
#include "../../../../inc/djinterp/c/text/ostream_template.h"    // values
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // uint64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ENTRY POINT
//==============================================================================

/**
 * @brief Builds and runs the module "ostream_template" in a suite.
 *
 * @param[in,out] _suite  the suite; its tests and checks are added to it.
 */
void d_tests_template_all(struct dj_test_suite* _suite);


//==============================================================================
// 2.  SHARED
//==============================================================================

// a.  checks and helpers (ostream_template_tests.c)

// D_TESTS_TEMPLATE_CHECK
//   macro: one check: counts it, and prints the expression, file and line
// on failure.
#define D_TESTS_TEMPLATE_CHECK(expr)                                           \
    d_tests_template_check(((expr) ? 1 : 0), #expr, __FILE__, __LINE__)

// d_tests_template_suite
//   the suite being built; set by d_tests_template_all.
extern struct dj_test_suite* d_tests_template_suite;

bool        d_tests_template_check(int         _ok,
                                   const char* _expression,
                                   const char* _file,
                                   int         _line);
void        d_tests_template_open(const char* _name, const char* _descriptor);
uint64_t    d_tests_template_next(void);
const char* d_tests_template_format(char*                   _buffer,
                                    size_t                  _size,
                                    struct dj_ostream_value _value,
                                    const char*             _spec);
int         d_tests_template_is(struct dj_ostream_value _value,
                                const char*             _spec,
                                const char*             _expected);

// b.  JSON's groups (ostream_template_tests_json.c)
void d_tests_template_json_reals(void);
void d_tests_template_json_random(void);
void d_tests_template_json_utf8(void);


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_OSTREAM_TEMPLATE_TESTS_H
