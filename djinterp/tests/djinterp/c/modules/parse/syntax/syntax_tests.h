/*******************************************************************************
* djinterp [c/parse]                                              syntax_tests.h
*
* The syntax items' suite in DTest's C face (c/modules/test/test_suite.h):
* what dawk's checker, dcheck, reads beside each token (lane H): `pp_of`, the
* preprocessor conditionals open around a token (F20, F21), and the items of
* C++ heads, template arguments and heads, class heads with base lists,
* constructors' initializer lists, a macro's call in a template head, and
* the namespace macros a caller names (F22, F23). dawk's fixtures are the
* acceptance tests; these are parse's own.
*   Each text is lexed as C or C++, grouped and recognized, and the token or
* item at a named spelling held to what it must be.
*   Two faces, chosen by DTEST_SPEC_MODE: the section translation units see
* the units under test and the fixtures; the runner, which defines
* DTEST_SPEC_MODE first, sees the tests and the spec provider,
* d_tests_parse_syntax_spec().
*
*
* TABLE OF CONTENTS
* =================
* 1.  FIXTURES
*     --------
*     1.  Constants
*     2.  Records
*     3.  The fixtures' functions
* 2.  TESTS
*     -----
* 3.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/syntax_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_PARSE_SYNTAX_TESTS_H
#define DJINTERP_TESTS_C_PARSE_SYNTAX_TESTS_H 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <stdbool.h>  // bool, true, false
    #include <stddef.h>   // size_t, NULL
    #include <string.h>   // memset, strlen, strncmp
    // djinterp
    #include "../../../../../../inc/djinterp/c/modules/parse/lang/c/lex_c.h"          // d_lex_c_create
    #include "../../../../../../inc/djinterp/c/modules/parse/lang/cpp/lex_cpp.h"      // d_lex_cpp_create
    #include "../../../../../../inc/djinterp/c/modules/parse/lex/lex_scan.h"          // d_lex_tokenize
    #include "../../../../../../inc/djinterp/c/modules/parse/source/source_reader.h"  // d_source_init
    #include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_group.h"   // d_syntax_group
    #include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_item.h"    // d_syntax_items_build_with
    // re_std
    #include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t, uint16_t, uint8_t
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURES
//==============================================================================
// The fixtures are the section units' alone: the runner sees only the tests
// and the spec.
#ifndef DTEST_SPEC_MODE

// 1.1    Constants
//------------------------------------------------------------------------------
// D_TESTS_SYNTAX_TOKENS
//   constant: the most tokens a text here lexes into.
#define D_TESTS_SYNTAX_TOKENS   512u

// D_TESTS_SYNTAX_ANY
//   constant: a parameter count that is not checked.
#define D_TESTS_SYNTAX_ANY      (-1)

// D_TESTS_SYNTAX_PP_ANY
//   constant: an item's `pp_total` that is not checked.
#define D_TESTS_SYNTAX_PP_ANY   0xFFu

// D_TESTS_SYNTAX_LEX
//   constant: the lexer's options for heads: comments kept and keywords
// resolved, as dawk lexes.
#define D_TESTS_SYNTAX_LEX      (D_LEX_OPT_EMIT_COMMENTS | D_LEX_OPT_KEYWORDS)

// 1.2    Records
//------------------------------------------------------------------------------
// dj_tests_syntax_run
//   struct: one text's recognition.
struct dj_tests_syntax_run
{
    const char*             text;
    struct dj_token         tokens[D_TESTS_SYNTAX_TOKENS];
    size_t                  count;
    struct dj_syntax_groups groups;
    struct dj_syntax_items  items;
};

// dj_tests_syntax_expect
//   struct: the item that begins at the `nth` token (from 0) spelled
// `first`, and what it must be: its kind and statement, the spelling of its
// name or NULL for none, how many parameters its group holds or
// D_TESTS_SYNTAX_ANY, the flags it must have and must not, its level, and
// the spelling of a token it must reach to (its first) or NULL.
struct dj_tests_syntax_expect
{
    const char* first;
    uint32_t    nth;
    uint8_t     kind;
    uint8_t     statement;
    const char* name;
    int         params;
    uint16_t    set;
    uint16_t    clear;
    uint8_t     level;
    const char* reaches;
};

// dj_tests_syntax_pp_expect
//   struct: a token, by its spelling and which of that spelling it is, the
// count of conditionals it must carry, and the `pp_total` of the item it is
// in, or D_TESTS_SYNTAX_PP_ANY.
struct dj_tests_syntax_pp_expect
{
    const char* spelling;
    uint32_t    nth;
    uint8_t     pp;
    uint8_t     total;
};

// 1.3    The fixtures' functions
//------------------------------------------------------------------------------
// Defined in syntax_tests_fixtures.c; each is described there.
size_t
d_tests_syntax_find(
    const struct dj_tests_syntax_run* _run,
    const char*                       _spelling,
    uint32_t                          _nth
);
bool
d_tests_syntax_read(
    struct dj_tests_syntax_run*     _run,
    const char*                     _text,
    bool                            _cpp,
    unsigned                        _lex,
    const struct dj_syntax_options* _options
);
void
d_tests_syntax_release(
    struct dj_tests_syntax_run* _run
);
bool
d_tests_syntax_text(
    const char*                          _text,
    bool                                 _cpp,
    const struct dj_syntax_options*      _options,
    const struct dj_tests_syntax_expect* _expect,
    size_t                               _expected
);
bool
d_tests_syntax_pp_text(
    const char*                             _text,
    const struct dj_tests_syntax_pp_expect* _expect,
    size_t                                  _expected
);

#endif  // DTEST_SPEC_MODE


//==============================================================================
// 2.  TESTS
//==============================================================================
// One test per section of the old suites, each in its block's unit.
bool d_tests_syntax_pp_guarded(void);
bool d_tests_syntax_pp_unguarded(void);
bool d_tests_syntax_pp_directives(void);
bool d_tests_syntax_pp_not_guards(void);
bool d_tests_syntax_cpp_template_args(void);
bool d_tests_syntax_cpp_template_heads(void);
bool d_tests_syntax_cpp_class_heads(void);
bool d_tests_syntax_cpp_initializers(void);
bool d_tests_syntax_cpp_head_macros(void);
bool d_tests_syntax_cpp_namespaces(void);
bool d_tests_syntax_cpp_c_unchanged(void);


//==============================================================================
// 3.  SPEC
//==============================================================================

// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_parse_syntax_pp_rows
//   constant: the pp block's rows.
static const struct dj_test_case_spec d_tests_parse_syntax_pp_rows[] =
{
    { "d_tests_syntax_pp_guarded",
      "inside an include guard, the guard not counted",
      d_tests_syntax_pp_guarded },
    { "d_tests_syntax_pp_unguarded",
      "no guard: every conditional counted",
      d_tests_syntax_pp_unguarded },
    { "d_tests_syntax_pp_directives",
      "a guard of directives alone, an #if after an #include",
      d_tests_syntax_pp_directives },
    { "d_tests_syntax_pp_not_guards",
      "an #ifndef that is no guard",
      d_tests_syntax_pp_not_guards }
};

// d_tests_parse_syntax_cpp_rows
//   constant: the cpp block's rows.
static const struct dj_test_case_spec d_tests_parse_syntax_cpp_rows[] =
{
    { "d_tests_syntax_cpp_template_args",
      "commas, = and parentheses among template arguments",
      d_tests_syntax_cpp_template_args },
    { "d_tests_syntax_cpp_template_heads",
      "template heads with defaults",
      d_tests_syntax_cpp_template_heads },
    { "d_tests_syntax_cpp_class_heads",
      "class heads with arguments and base lists",
      d_tests_syntax_cpp_class_heads },
    { "d_tests_syntax_cpp_initializers",
      "constructors' initializer lists",
      d_tests_syntax_cpp_initializers },
    { "d_tests_syntax_cpp_head_macros",
      "a macro's call after a template head is the head's",
      d_tests_syntax_cpp_head_macros },
    { "d_tests_syntax_cpp_namespaces",
      "the namespace macros a caller names",
      d_tests_syntax_cpp_namespaces },
    { "d_tests_syntax_cpp_c_unchanged",
      "C, where a < after a name compares",
      d_tests_syntax_cpp_c_unchanged }
};

// d_tests_parse_syntax_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_parse_syntax_blocks[] =
{
    { "pp",
      "pp_of: the conditionals open around a token",
      d_tests_parse_syntax_pp_rows,
      sizeof(d_tests_parse_syntax_pp_rows) /
          sizeof(d_tests_parse_syntax_pp_rows[0]) },
    { "cpp",
      "the items of C++ heads, and C's unchanged",
      d_tests_parse_syntax_cpp_rows,
      sizeof(d_tests_parse_syntax_cpp_rows) /
          sizeof(d_tests_parse_syntax_cpp_rows[0]) }
};

// d_tests_parse_syntax_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_parse_syntax_module =
{
    "parse_syntax",
    "parse/syntax: the items dawk's checker reads",
    d_tests_parse_syntax_blocks,
    sizeof(d_tests_parse_syntax_blocks) /
        sizeof(d_tests_parse_syntax_blocks[0])
};

// d_tests_parse_syntax_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_parse_syntax_spec(void)
{
    return &d_tests_parse_syntax_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_PARSE_SYNTAX_TESTS_H
