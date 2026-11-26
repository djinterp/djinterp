/*******************************************************************************
* djinterp [c][text]                                    number_to_string_tests.h
*
* The suite for c/text's numbers in words, number_to_string_en.h to
* number_to_string_pt.h, in DTest's C spec form: a block per language, each
* with its cardinals, ordinals and abbreviated ordinals as tables, and a block
* that holds every function to snprintf's way with a buffer.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  ROWS
    ----
    1.  A row
    2.  The rows' check
2.  TESTS
    -----
    1.  English
    2.  Spanish
    3.  French
    4.  Italian
    5.  Latin
    6.  Portuguese
    7.  The buffer
3.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_C_TEXT_NUMBER_TO_STRING_TESTS_H
#define DJINTERP_TESTS_C_TEXT_NUMBER_TO_STRING_TESTS_H 1
// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../inc/djinterp/c/text/number_to_string_en.h"  // nts_*
#include "../../../../inc/djinterp/c/text/number_to_string_es.h"  // nts_es_*
#include "../../../../inc/djinterp/c/text/number_to_string_fr.h"  // nts_fr_*
#include "../../../../inc/djinterp/c/text/number_to_string_it.h"  // nts_it_*
#include "../../../../inc/djinterp/c/text/number_to_string_la.h"  // nts_la_*
#include "../../../../inc/djinterp/c/text/number_to_string_pt.h"  // nts_pt_*
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ROWS
//==============================================================================


// 1.1    A row
//------------------------------------------------------------------------------
// D_TESTS_NUMBER_TO_STRING_CAPACITY
//   constant: the buffer a row is written into; more than the largest of the
// six headers' *_I64_BUFFER_SIZE.
#define D_TESTS_NUMBER_TO_STRING_CAPACITY 1024u

// dj_tests_number_to_string_fn
//   type: the shape all eighteen functions share.
typedef size_t (*dj_tests_number_to_string_fn)(char*    _dst,
                                                size_t   _dst_cap,
                                                int64_t  _value,
                                                unsigned _flags);

// dj_tests_number_to_string_row
//   struct: one value, its flags, and the text the function must write.
struct dj_tests_number_to_string_row
{
    int64_t     value;     // the number
    unsigned    flags;     // the language's flags
    const char* expected;  // the text, in UTF-8
};

// 1.2    The rows' check
//------------------------------------------------------------------------------
bool d_tests_number_to_string_rows(
    dj_tests_number_to_string_fn                _fn,
    const struct dj_tests_number_to_string_row* _rows,
    size_t                                      _count);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    English
//------------------------------------------------------------------------------
bool d_tests_number_to_string_en_cardinal(void);
bool d_tests_number_to_string_en_ordinal(void);
bool d_tests_number_to_string_en_abbrev(void);

// 2.2    Spanish
//------------------------------------------------------------------------------
bool d_tests_number_to_string_es_cardinal(void);
bool d_tests_number_to_string_es_ordinal(void);
bool d_tests_number_to_string_es_abbrev(void);

// 2.3    French
//------------------------------------------------------------------------------
bool d_tests_number_to_string_fr_cardinal(void);
bool d_tests_number_to_string_fr_ordinal(void);
bool d_tests_number_to_string_fr_abbrev(void);

// 2.4    Italian
//------------------------------------------------------------------------------
bool d_tests_number_to_string_it_cardinal(void);
bool d_tests_number_to_string_it_ordinal(void);
bool d_tests_number_to_string_it_abbrev(void);

// 2.5    Latin
//------------------------------------------------------------------------------
bool d_tests_number_to_string_la_cardinal(void);
bool d_tests_number_to_string_la_ordinal(void);
bool d_tests_number_to_string_la_abbrev(void);

// 2.6    Portuguese
//------------------------------------------------------------------------------
bool d_tests_number_to_string_pt_cardinal(void);
bool d_tests_number_to_string_pt_ordinal(void);
bool d_tests_number_to_string_pt_abbrev(void);

// 2.7    The buffer
//------------------------------------------------------------------------------
bool d_tests_number_to_string_measure(void);
bool d_tests_number_to_string_cut(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_number_to_string_en_rows
//   constant: the English block's rows.
static const struct dj_test_case_spec d_tests_number_to_string_en_rows[] =
{
    D_TEST_CASE(d_tests_number_to_string_en_cardinal,
                "zero to INT64_MIN; hyphens, commas and \"and\" by flag"),
    D_TEST_CASE(d_tests_number_to_string_en_ordinal,
                "the last word made ordinal: twenty-first, millionth"),
    D_TEST_CASE(d_tests_number_to_string_en_abbrev,
                "1st, 21st, 101st and 1,234th; the sign kept")
};

// d_tests_number_to_string_es_rows
//   constant: the Spanish block's rows.
static const struct dj_test_case_spec d_tests_number_to_string_es_rows[] =
{
    D_TEST_CASE(d_tests_number_to_string_es_cardinal,
                "veintiuno, veintiún mil, dos mil millones: the long scale"),
    D_TEST_CASE(d_tests_number_to_string_es_ordinal,
                "undécimo, vigésimo primero, dosmilésimo, millonésimo"),
    D_TEST_CASE(d_tests_number_to_string_es_abbrev,
                "1.º, and 1.234.º grouped by dots")
};

// d_tests_number_to_string_fr_rows
//   constant: the French block's rows.
static const struct dj_test_case_spec d_tests_number_to_string_fr_rows[] =
{
    D_TEST_CASE(d_tests_number_to_string_fr_cardinal,
                "et un, quatre-vingts, deux cents; the long scale"),
    D_TEST_CASE(d_tests_number_to_string_fr_ordinal,
                "premier, vingt et unième, quatre-vingtième, millionième"),
    D_TEST_CASE(d_tests_number_to_string_fr_abbrev,
                "1er and 2e; 1,234e with commas")
};

// d_tests_number_to_string_it_rows
//   constant: the Italian block's rows.
static const struct dj_test_case_spec d_tests_number_to_string_it_rows[] =
{
    D_TEST_CASE(d_tests_number_to_string_it_cardinal,
                "ventuno, ventotto, centottanta; un milione uno"),
    D_TEST_CASE(d_tests_number_to_string_it_ordinal,
                "primo, ventitreesimo, duemillesimo, un milione primo"),
    D_TEST_CASE(d_tests_number_to_string_it_abbrev,
                "1°, and 1.234° grouped by dots; meno for a minus")
};

// d_tests_number_to_string_la_rows
//   constant: the Latin block's rows.
static const struct dj_test_case_spec d_tests_number_to_string_la_rows[] =
{
    D_TEST_CASE(d_tests_number_to_string_la_cardinal,
                "nihil; viginti (et) unus by flag; thousands as milia"),
    D_TEST_CASE(d_tests_number_to_string_la_ordinal,
                "primus, vicesimus primus, bis millesimus"),
    D_TEST_CASE(d_tests_number_to_string_la_abbrev,
                "Roman numerals to MMMCMXCIX.; digits past it, and for 0")
};

// d_tests_number_to_string_pt_rows
//   constant: the Portuguese block's rows.
static const struct dj_test_case_spec d_tests_number_to_string_pt_rows[] =
{
    D_TEST_CASE(d_tests_number_to_string_pt_cardinal,
                "cem, cento e um, mil e cem, um milhão e mil, um bilhão"),
    D_TEST_CASE(d_tests_number_to_string_pt_ordinal,
                "décimo primeiro, segundo milésimo trecentésimo"),
    D_TEST_CASE(d_tests_number_to_string_pt_abbrev,
                "1.º, and 1.234.º grouped by dots; menos for a minus")
};

// d_tests_number_to_string_buffer_rows
//   constant: the buffer block's rows.
static const struct dj_test_case_spec d_tests_number_to_string_buffer_rows[] =
{
    D_TEST_CASE(d_tests_number_to_string_measure,
                "every function, given no room, writes nothing and gives "
                "the length"),
    D_TEST_CASE(d_tests_number_to_string_cut,
                "a short buffer gets the first bytes, terminated, and the "
                "whole length")
};

// D_TESTS_NUMBER_TO_STRING_BLOCK
//   macro: a block's spec from its name, description and rows.
#define D_TESTS_NUMBER_TO_STRING_BLOCK(_name, _description, _rows)           \
    { (_name), (_description), (_rows), sizeof(_rows) / sizeof((_rows)[0]) }

// d_tests_number_to_string_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_number_to_string_blocks[] =
{
    D_TESTS_NUMBER_TO_STRING_BLOCK("en", "English",
                                   d_tests_number_to_string_en_rows),
    D_TESTS_NUMBER_TO_STRING_BLOCK("es", "Spanish",
                                   d_tests_number_to_string_es_rows),
    D_TESTS_NUMBER_TO_STRING_BLOCK("fr", "French",
                                   d_tests_number_to_string_fr_rows),
    D_TESTS_NUMBER_TO_STRING_BLOCK("it", "Italian",
                                   d_tests_number_to_string_it_rows),
    D_TESTS_NUMBER_TO_STRING_BLOCK("la", "Latin",
                                   d_tests_number_to_string_la_rows),
    D_TESTS_NUMBER_TO_STRING_BLOCK("pt", "Portuguese",
                                   d_tests_number_to_string_pt_rows),
    D_TESTS_NUMBER_TO_STRING_BLOCK("buffer",
                                   "every function, as snprintf takes a "
                                   "buffer",
                                   d_tests_number_to_string_buffer_rows)
};

// d_tests_number_to_string_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_number_to_string_module =
{
    "number_to_string",
    "c/text: numbers in words, in six languages",
    d_tests_number_to_string_blocks,
    sizeof(d_tests_number_to_string_blocks) /
        sizeof(d_tests_number_to_string_blocks[0])
};

// d_tests_number_to_string_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_number_to_string_spec(void)
{
    return &d_tests_number_to_string_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_NUMBER_TO_STRING_TESTS_H
