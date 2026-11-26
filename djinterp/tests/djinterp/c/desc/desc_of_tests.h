/*******************************************************************************
* djinterp [c][desc]                                             desc_of_tests.h
*
*   What the suites of desc's describers of other modules' types share, in
* DTest's C face: what a build's prose is expected to be, a machine that
* keeps each fact a sink wanted and the finders that read them, a put into
* a buffer, and each module's layout check. desc_of_tests_common.c holds
* the functions; each module's tests (desc_of_mem_tests.c and its kin)
* include this header.
*
*
* path:      /tests/djinterp/c/desc/desc_of_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DESC_OF_TESTS_H
#define DJINTERP_TESTS_C_DESC_OF_TESTS_H 1

// std
#include <string.h>  // strcmp, for D_TESTS_OF_SAYS
// djinterp
#include "./desc_tests.h"                                // the sections
#include "../../../../inc/djinterp/c/desc/desc_units.h"  // the engine
#include "../../../../inc/djinterp/c/desc/of/desc_of.h"  // the layout row


//==============================================================================
// EXPECTATIONS
//==============================================================================

// what the suite expects of a build's prose: the default build's, with or
// without articles
#define D_TESTS_OF_FULL_BUILD                                                  \
    ( D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS && (D_CFG_DESC_RING_MAX == 3) && \
      (D_CFG_DESC_LAYERS_BUILT == 0x1FF) )

#if D_CFG_DESC_ARTICLES
    #define D_TESTS_OF_A   "a "
    #define D_TESTS_OF_AN  "an "
#else
    #define D_TESTS_OF_A   ""
    #define D_TESTS_OF_AN  ""
#endif

// whether facts are built: where they are not, d_desc_facts says -1, and a
// check of a fact holds for want of one
#define D_TESTS_OF_FACTS  ( D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS )
#define D_TESTS_OF_FACT(expr_)  ( (!D_TESTS_OF_FACTS) || (expr_) )

// D_TESTS_FACT
//   macro: D_TEST_CHECK of a fact, which holds where facts are not built.
#define D_TESTS_FACT(_suite, _expr)                                            \
    D_TEST_CHECK((_suite), D_TESTS_OF_FACT(_expr))

// a size as the suite prints it
#define D_TESTS_OF_N(value_)  ((unsigned long long)(value_))

// D_TESTS_OF_SAYS
//   macro: a check that _text is _expected, which prints the text it got.
#define D_TESTS_OF_SAYS(_suite, _name, _text, _expected)                       \
    D_TEST_CHECK_MSG((_suite), (_name), strcmp((_text), (_expected)) == 0,     \
                     (_text))


//==============================================================================
// THE KEEPER
//==============================================================================
//   A machine that keeps each fact the sink wanted.

#define D_TESTS_OF_KEEP_MAX  64u

struct dj_tests_of_keep
{
    struct dj_desc_fact facts[D_TESTS_OF_KEEP_MAX];
    size_t              count;
    size_t              enters;
};

/*
d_tests_of_facts
  An object's facts, every layer and level, into a fresh keeper.
*/
int d_tests_of_facts(const struct dj_desc_type* _type,
                     const void*                _object,
                     struct dj_tests_of_keep*   _keep);

/*
d_tests_of_find
  The first kept fact of a key at a depth, or null.
*/
const struct dj_desc_fact* d_tests_of_find(
    const struct dj_tests_of_keep* _keep,
    const char*                    _key,
    uint32_t                       _depth);

/*
d_tests_of_is, d_tests_of_flag, d_tests_of_text
  1 when the kept fact of a key is the whole number (at a depth), the truth
or the text (at depth 0) given; a null text asks for none.
*/
int d_tests_of_is(const struct dj_tests_of_keep* _keep,
                  const char*                    _key,
                  uint32_t                       _depth,
                  unsigned long long             _value);
int d_tests_of_flag(const struct dj_tests_of_keep* _keep,
                    const char*                    _key,
                    uint32_t                       _value);
int d_tests_of_text(const struct dj_tests_of_keep* _keep,
                    const char*                    _key,
                    const char*                    _value);

/*
d_tests_of_say
  An object's prose at a ring, every layer, into _text.
*/
int d_tests_of_say(const struct dj_desc_type* _type,
                   const void*                _object,
                   uint32_t                   _ring,
                   char*                      _text,
                   size_t                     _capacity);


//==============================================================================
// THE LAYOUT
//==============================================================================

/*
d_tests_of_layout
  A module's layout check, as a test of its own: _built says whether its
describers are; _rows and _count are its rows as this program is built;
_agree and _index are its d_desc_of_<module>_agrees's answer.
*/
void d_tests_of_layout(struct dj_test_suite*           _suite,
                       const char*                     _module,
                       int                             _built,
                       const struct dj_desc_of_layout* _rows,
                       size_t                          _count,
                       int                             _agree,
                       size_t                          _index);


#endif  // DJINTERP_TESTS_C_DESC_OF_TESTS_H
