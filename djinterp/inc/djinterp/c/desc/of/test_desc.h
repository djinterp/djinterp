/*******************************************************************************
* djinterp [c][desc]                                                 test_desc.h
*
*   desc's describers of DTest's C types (desc's decisions 34, 68 and 82): a
* run's summary (struct dj_test_summary) and options (dj_test_suite_options),
* and the specs a suite header provides: a module's, a block's and a test's
* (dj_test_module_spec, dj_test_block_spec, dj_test_case_spec), each a
* struct dj_desc_type, so d_desc_say writes it in prose and d_desc_facts as
* facts. They are desc's, in djinterp_c_desc_of: DTest includes nothing of
* desc, and a program that describes none of these links none of this.
*   Each describer reads the type's fields and calls none of DTest's
* functions, so the library needs DTest's C header and not its units. A
* suite is opaque and is not described; its summary is. A module spec's
* contents are its blocks, each a level keyed by its name, and a block's
* are its tests' names. A spec's strings are its own, static. None of these
* types holds a lock or a field another thread writes. The fields each
* describer reads are recorded beside it (section 2), and the layout rows
* check the records against the types and the library's build against the
* caller's (desc_of.h).
*
* HOW TO USE
* ----------
*   d_desc_say(D_DESC_OF(dj_test_summary), &summary, ring, layers, 0, put,
* context) writes a run's counts in prose: "a passing test summary with 30
* tests and 275 checks"; d_desc_facts(D_DESC_OF(dj_test_module_spec),
* spec, sink) gives a suite header's spec as facts, its blocks and tests a
* level down each.
*
* OPTIONS
* -------
*   desc's own: desc.ring, desc.layers and the rest; desc.test_summary.ring
* and the other types' names, once d_desc_declare has declared the type.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and D_CFG_DESC_OF_TEST (cfg_desc_of.h); desc's knobs.
*
*
* path:      /inc/djinterp/c/desc/of/test_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESCRIPTIONS
    ------------
    1.  d_desc_type_dj_test_summary
    2.  d_desc_type_dj_test_suite_options
    3.  d_desc_type_dj_test_module_spec
    4.  d_desc_type_dj_test_block_spec
    5.  d_desc_type_dj_test_case_spec
2.  RECORDS
    -------
    1.  dj_test_desc_*_record
3.  LAYOUT
    ------
    1.  d_desc_of_test_layout_here
    2.  d_desc_of_test_layout
    3.  d_desc_of_test_agrees
*/

#ifndef DJINTERP_C_DESC_OF_TEST_DESC_H
#define DJINTERP_C_DESC_OF_TEST_DESC_H 1

// djinterp
#include "./desc_of.h"                       // the layout row, the knobs
#include "../../modules/test/test_suite.h"   // DTest's C types


D_EXTERN_C_BEGIN


#if D_INTERNAL_DESC_OF_TEST

//==============================================================================
// 1.  DESCRIPTIONS
//==============================================================================

// 1.1
// d_desc_type_dj_test_summary
//   constant: a run's summary: "a passing test summary with 30 tests and
// 275 checks", its failures and skips in the sentence. Its facts: passed,
// tests, tests_passed, tests_failed, tests_skipped, checks, checks_failed
// and modules.
extern const struct dj_desc_type d_desc_type_dj_test_summary;

// 1.2
// d_desc_type_dj_test_suite_options
//   constant: a run's options: what it lists, its table, its filter and
// where it writes. Its facts: show, table, color, filter and stream, one
// of "stdout", "stderr" and "other".
extern const struct dj_desc_type d_desc_type_dj_test_suite_options;

// 1.3
// d_desc_type_dj_test_module_spec
//   constant: a suite header's spec: "a test module named dtime with 1
// block". Its facts: name, descriptor, block_count and test_count; its
// contents, each block, keyed by its name.
extern const struct dj_desc_type d_desc_type_dj_test_module_spec;

// 1.4
// d_desc_type_dj_test_block_spec
//   constant: a block's spec: "a test block named register with 7 tests".
// Its facts: name, descriptor and test_count; its contents, each test's
// name.
extern const struct dj_desc_type d_desc_type_dj_test_block_spec;

// 1.5
// d_desc_type_dj_test_case_spec
//   constant: a test's spec: "a test named d_tests_dtime_clocks", or "an
// unbound test" without its function. Its facts: name, descriptor and
// runnable.
extern const struct dj_desc_type d_desc_type_dj_test_case_spec;

#endif  // D_INTERNAL_DESC_OF_TEST


#if D_INTERNAL_DESC_OF_TEST

//==============================================================================
// 2.  RECORDS
//==============================================================================
//   Each described type as its describer knows it, as memory's are
// (mem_desc.h). A pointer the describer does not follow is a void*, and
// a function pointer is fn_test_case, the type's own.

struct dj_test_desc_summary_record
{
    size_t modules;
    size_t tests;
    size_t tests_failed;
    size_t tests_skipped;
    size_t checks;
    size_t checks_failed;
};

struct dj_test_desc_options_record
{
    int         show;
    bool        table;
    bool        color;
    void*       stream;
    const char* filter;
};

struct dj_test_desc_case_record
{
    const char*  name;
    const char*  descriptor;
    fn_test_case fn;
};

struct dj_test_desc_block_record
{
    const char* name;
    const char* descriptor;
    const void* tests;
    size_t      count;
};

struct dj_test_desc_module_record
{
    const char* name;
    const char* descriptor;
    const void* blocks;
    size_t      count;
};

#endif  // D_INTERNAL_DESC_OF_TEST


//==============================================================================
// 3.  LAYOUT
//==============================================================================

// a row of each record, the field named once
#define D_TEST_DESC_ROW(type_, record_, field_)                                \
    D_DESC_OF_LAYOUT_FIELD(type_, dj_test_desc_##record_##_record, field_)

/**
 * @brief DTest's layout rows as the caller is built: each described type's
 *        size, and the offset and width of each field its describer reads,
 *        as recorded and as the type has them.
 *
 * @param[out] _rows  set to the rows, static; null when DTest's describers
 *                    are not built. May be null.
 * @return the rows' count; 0 when DTest's describers are not built.
 */
D_INLINE size_t
d_desc_of_test_layout_here(
    const struct dj_desc_of_layout** _rows
)
{
#if D_INTERNAL_DESC_OF_TEST
    static const struct dj_desc_of_layout rows[] =
    {
        D_DESC_OF_LAYOUT_SIZE(dj_test_summary, dj_test_desc_summary_record),
        D_TEST_DESC_ROW(dj_test_summary, summary, modules),
        D_TEST_DESC_ROW(dj_test_summary, summary, tests),
        D_TEST_DESC_ROW(dj_test_summary, summary, tests_failed),
        D_TEST_DESC_ROW(dj_test_summary, summary, tests_skipped),
        D_TEST_DESC_ROW(dj_test_summary, summary, checks),
        D_TEST_DESC_ROW(dj_test_summary, summary, checks_failed),
        D_DESC_OF_LAYOUT_SIZE(dj_test_suite_options,
                              dj_test_desc_options_record),
        D_TEST_DESC_ROW(dj_test_suite_options, options, show),
        D_TEST_DESC_ROW(dj_test_suite_options, options, table),
        D_TEST_DESC_ROW(dj_test_suite_options, options, color),
        D_TEST_DESC_ROW(dj_test_suite_options, options, stream),
        D_TEST_DESC_ROW(dj_test_suite_options, options, filter),
        D_DESC_OF_LAYOUT_SIZE(dj_test_case_spec, dj_test_desc_case_record),
        D_TEST_DESC_ROW(dj_test_case_spec, case, name),
        D_TEST_DESC_ROW(dj_test_case_spec, case, descriptor),
        D_TEST_DESC_ROW(dj_test_case_spec, case, fn),
        D_DESC_OF_LAYOUT_SIZE(dj_test_block_spec, dj_test_desc_block_record),
        D_TEST_DESC_ROW(dj_test_block_spec, block, name),
        D_TEST_DESC_ROW(dj_test_block_spec, block, descriptor),
        D_TEST_DESC_ROW(dj_test_block_spec, block, tests),
        D_TEST_DESC_ROW(dj_test_block_spec, block, count),
        D_DESC_OF_LAYOUT_SIZE(dj_test_module_spec,
                              dj_test_desc_module_record),
        D_TEST_DESC_ROW(dj_test_module_spec, module, name),
        D_TEST_DESC_ROW(dj_test_module_spec, module, descriptor),
        D_TEST_DESC_ROW(dj_test_module_spec, module, blocks),
        D_TEST_DESC_ROW(dj_test_module_spec, module, count)
    };

    if (_rows != NULL)
    {
        *_rows = rows;
    }

    return sizeof(rows) / sizeof(rows[0]);
#else
    if (_rows != NULL)
    {
        *_rows = NULL;
    }

    return 0u;
#endif
}

#undef D_TEST_DESC_ROW

/**
 * @brief DTest's layout rows as the library was built.
 *
 * @param[out] _rows  as d_desc_of_test_layout_here.
 * @return as d_desc_of_test_layout_here.
 */
size_t d_desc_of_test_layout(const struct dj_desc_of_layout** _rows);

/**
 * @brief Whether the library lays DTest's described types out as the
 *        caller does, as far as its describers read.
 *
 * @param[out] _index  set to the first row the two differ in; may be null.
 * @return 1 when they agree, or neither has DTest's describers; else 0.
 */
D_INLINE int
d_desc_of_test_agrees(
    size_t* _index
)
{
    const struct dj_desc_of_layout* built = NULL;
    const struct dj_desc_of_layout* here  = NULL;
    size_t built_count = d_desc_of_test_layout(&built);
    size_t here_count  = d_desc_of_test_layout_here(&here);

    return d_desc_of_agree(built, built_count, here, here_count, _index);
}


D_EXTERN_C_END


#endif  // DJINTERP_C_DESC_OF_TEST_DESC_H
