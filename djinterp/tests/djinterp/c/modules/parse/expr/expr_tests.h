/*******************************************************************************
* djinterp [c/parse]                                                expr_tests.h
*
* Expression core v2's C suite in DTest's C face (c/modules/test/
* test_suite.h): the term, the signature, the parser, the renderer, folds
* and walks, the scanner and the scratch, each through the C API alone.
*   Two faces, chosen by DTEST_SPEC_MODE as in every C suite: the section
* translation units see the core and the fixtures; the runner, which
* defines DTEST_SPEC_MODE first, sees the tests and the spec provider,
* d_tests_parse_expr_spec().
*   Four languages are built on the core in the fixtures, each a table and
* nothing more: math's, the command language's, an awk-like one's and
* dawk's (lane H, 11-05). Every case of the design's tables is a case here,
* with the term it must give.
*
*
* TABLE OF CONTENTS
* =================
* 1.  FIXTURES
*     --------
*     1.  Token classes and operators
*     2.  Lexer flags
*     3.  Records
*     4.  The fixtures' functions
* 2.  TESTS
*     -----
* 3.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_PARSE_EXPR_TESTS_H
#define DJINTERP_TESTS_C_PARSE_EXPR_TESTS_H 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <stdbool.h>  // bool, true, false
    #include <stddef.h>   // size_t, NULL
    #include <stdio.h>    // snprintf
    #include <string.h>   // memset, strcmp, strlen, strncmp
    // djinterp
    #include "../../../../../../inc/djinterp/c/modules/parse/expr/expr.h"  // the core
    // re_std
    #include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURES
//==============================================================================
// The fixtures are the section units' alone: the runner sees only the
// tests and the spec.
#ifndef DTEST_SPEC_MODE

// 1.1    Token classes and operators
//------------------------------------------------------------------------------
enum dj_tests_expr_class
{
    C_NUM = 1, C_NAME, C_PLUS, C_MINUS, C_STAR, C_SLASH, C_CARET, C_BANG,
    C_QUOTE, C_EQ, C_NE, C_LT, C_LE, C_GT, C_GE, C_AND, C_OR, C_NOT, C_MOD,
    C_LP, C_RP, C_LB, C_RB, C_BAR, C_COMMA, C_SEMI, C_EQEQ, C_PIPE, C_OPTION,
    C_STRING, C_QMARK, C_COLON, C_IF, C_THEN, C_ELSE, C_INC, C_TILDE,
    C_DOLLAR, C_ANDAND, C_OROR, C_NOMATCH, C_MATCH, C_DEC, C_PERCENT, C_IN,
    C_GETLINE, C_APPEND, C_DIVEQ
};

enum dj_tests_expr_op
{
    O_ADD = 1, O_SUB, O_MUL, O_DIV, O_MOD, O_POW, O_NEG, O_POS, O_FACT,
    O_TRANS, O_EQ, O_NE, O_LT, O_LE, O_GT, O_GE, O_AND, O_OR, O_NOT, O_CALL,
    O_INDEX, O_VECTOR, O_MATRIX, O_ROW, O_GROUP, O_ABS, O_SEQ, O_PIPE,
    O_EQEQ, O_CMD, O_CONCAT, O_COND, O_IF, O_PREINC, O_POSTINC, O_MATCH,
    O_ASSIGN, O_IN, O_NOMATCH, O_PIPEGET, O_GETLINE, O_FIELD, O_PREDEC,
    O_POSTDEC, O_GETFILE, O_COUNT
};

// 1.2    Lexer flags
//------------------------------------------------------------------------------
// Lexer flags
//   constant: OPTIONS: `-l` and `--all` are options, the command language's;
// DAWK: dawk's punctuation is read too.
#define D_TESTS_EXPR_LEX_OPTIONS    0x01u
#define D_TESTS_EXPR_LEX_DAWK       0x02u

// 1.3    Records
//------------------------------------------------------------------------------
// dj_tests_expr_text
//   struct: a buffer an s-expression is written into.
struct dj_tests_expr_text
{
    char   out[512];
    size_t length;
};

// dj_tests_expr_run
//   struct: one parse and what it found, over storage of its own.
struct dj_tests_expr_run
{
    struct dj_parse_expr_token   tokens[4096];
    struct dj_parse_diagnostic   items[32];
    char                         text[4096];
    struct dj_parse_diag_sink    sink;
    struct dj_parse_expr_term    term;
    struct dj_parse_expr_result  result;
    struct dj_parse_expr_options options;
    int                          status;
};

// dj_tests_expr_case
//   struct: a text and the term it must read as.
struct dj_tests_expr_case
{
    const char* text;
    const char* term;
};

// dj_tests_expr_error
//   struct: a text, the error it must give, where, and how many errors.
struct dj_tests_expr_error
{
    const char* text;
    uint16_t    code;
    uint32_t    offset;
    uint32_t    length;
    uint32_t    errors;
};

// 1.4    The fixtures' functions
//------------------------------------------------------------------------------
// Defined in expr_tests_fixtures.c; each is described there.
bool
d_tests_expr_expect(
    bool        _held,
    const char* _what,
    int         _line
);
uint32_t
d_tests_expr_atom_value(
    const char* _text,
    size_t      _length
);
int
d_tests_expr_lex(
    const char*                 _text,
    uint32_t                    _lexer,
    struct dj_parse_expr_token* _out,
    int                         _max
);
const char*
d_tests_expr_sexpr(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root,
    struct dj_tests_expr_text*       _b
);
int
d_tests_expr_parse(
    struct dj_tests_expr_run*             _run,
    const struct dj_parse_expr_signature* _sig,
    const char*                           _text,
    uint32_t                              _lexer
);
struct dj_parse_expr_mixfix
d_tests_expr_mixfix_of(
    uint8_t     _lead,
    uint32_t    _open,
    const char* _open_spelling,
    uint32_t    _close,
    const char* _close_spelling
);
void
d_tests_expr_list_of(
    struct dj_parse_expr_mixfix* _m,
    uint16_t                     _min,
    uint16_t                     _max
);
bool
d_tests_expr_math(
    struct dj_parse_expr_signature* _s,
    bool                            _implied
);
bool
d_tests_expr_cli(
    struct dj_parse_expr_signature* _s
);
bool
d_tests_expr_awk(
    struct dj_parse_expr_signature* _s
);
bool
d_tests_expr_dawk_list(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    struct dj_parse_diag_sink*       _sink
);
bool
d_tests_expr_dawk_custom(
    struct dj_parse_expr_signature* _s,
    uint32_t                        _class,
    uint8_t                         _position,
    uint16_t                        _flags,
    uint32_t                        _op,
    dj_parse_expr_custom_fn         _hook
);
bool
d_tests_expr_dawk(
    struct dj_parse_expr_signature* _s
);
bool
d_tests_expr_cases(
    const struct dj_parse_expr_signature* _sig,
    const struct dj_tests_expr_case*      _cases,
    size_t                                _count,
    uint32_t                              _lexer,
    uint32_t                              _mode
);
bool
d_tests_expr_errors(
    const struct dj_parse_expr_signature* _sig,
    const struct dj_tests_expr_error*     _cases,
    size_t                                _count,
    uint8_t                               _order
);
bool
d_tests_expr_atom_name(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    dj_parse_expr_write_fn           _write,
    void*                            _write_ctx
);

#endif  // DTEST_SPEC_MODE


//==============================================================================
// 2.  TESTS
//==============================================================================
// One test per section of the old suite, each in its block's unit.
bool d_tests_expr_term(void);
bool d_tests_expr_signature(void);
bool d_tests_expr_math_cases(void);
bool d_tests_expr_cli_cases(void);
bool d_tests_expr_awk_cases(void);
bool d_tests_expr_dawk_cases(void);
bool d_tests_expr_stop(void);
bool d_tests_expr_subparse(void);
bool d_tests_expr_hooks(void);
bool d_tests_expr_diagnostics(void);
bool d_tests_expr_limits(void);
bool d_tests_expr_roundtrip(void);
bool d_tests_expr_fold_walk(void);
bool d_tests_expr_locate(void);
bool d_tests_expr_scan(void);
bool d_tests_expr_scratch(void);


//==============================================================================
// 3.  SPEC
//==============================================================================

// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_parse_expr_term_rows
//   constant: the term block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_term_rows[] =
{
    { "d_tests_expr_term",
      "the arena: leaves, applications, holes, copies, verify",
      d_tests_expr_term },
    { "d_tests_expr_signature",
      "signatures sealed, and each refusal",
      d_tests_expr_signature }
};

// d_tests_parse_expr_languages_rows
//   constant: the languages block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_languages_rows[] =
{
    { "d_tests_expr_math_cases",
      "math's table: calls, indexing, bars, rows, chains",
      d_tests_expr_math_cases },
    { "d_tests_expr_cli_cases",
      "the command language's: comparisons, pipelines",
      d_tests_expr_cli_cases },
    { "d_tests_expr_awk_cases",
      "an awk-like language's: concatenation, modes",
      d_tests_expr_awk_cases },
    { "d_tests_expr_dawk_cases",
      "dawk's, from lane H's review of 11-05",
      d_tests_expr_dawk_cases }
};

// d_tests_parse_expr_parsing_rows
//   constant: the parsing block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_parsing_rows[] =
{
    { "d_tests_expr_stop",
      "the stop option and the result's next (F17)",
      d_tests_expr_stop },
    { "d_tests_expr_subparse",
      "the sub-parse and the modes of mixfix holes (F18)",
      d_tests_expr_subparse },
    { "d_tests_expr_hooks",
      "custom roles: peek and take in a mode (F19)",
      d_tests_expr_hooks },
    { "d_tests_expr_diagnostics",
      "each error, its span, its note, its order",
      d_tests_expr_diagnostics },
    { "d_tests_expr_limits",
      "the guards, the allowance, the check hook",
      d_tests_expr_limits }
};

// d_tests_parse_expr_output_rows
//   constant: the output block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_output_rows[] =
{
    { "d_tests_expr_roundtrip",
      "random terms rendered and read back",
      d_tests_expr_roundtrip },
    { "d_tests_expr_fold_walk",
      "folds, and walks with skip and stop",
      d_tests_expr_fold_walk },
    { "d_tests_expr_locate",
      "a byte offset as a line and a column",
      d_tests_expr_locate }
};

// d_tests_parse_expr_sources_rows
//   constant: the sources block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_sources_rows[] =
{
    { "d_tests_expr_scan",
      "the default scanner, and parses through it",
      d_tests_expr_scan },
    { "d_tests_expr_scratch",
      "the caller's scratch for the parser's stacks",
      d_tests_expr_scratch }
};

// d_tests_parse_expr_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_parse_expr_blocks[] =
{
    { "term",
      "the term and the signature",
      d_tests_parse_expr_term_rows,
      sizeof(d_tests_parse_expr_term_rows) /
          sizeof(d_tests_parse_expr_term_rows[0]) },
    { "languages",
      "four languages built on the core, each a table",
      d_tests_parse_expr_languages_rows,
      sizeof(d_tests_parse_expr_languages_rows) /
          sizeof(d_tests_parse_expr_languages_rows[0]) },
    { "parsing",
      "the parser: stopping, sub-parses, hooks, errors, limits",
      d_tests_parse_expr_parsing_rows,
      sizeof(d_tests_parse_expr_parsing_rows) /
          sizeof(d_tests_parse_expr_parsing_rows[0]) },
    { "output",
      "rendering, folds, walks and places",
      d_tests_parse_expr_output_rows,
      sizeof(d_tests_parse_expr_output_rows) /
          sizeof(d_tests_parse_expr_output_rows[0]) },
    { "sources",
      "the default scanner and the caller's scratch",
      d_tests_parse_expr_sources_rows,
      sizeof(d_tests_parse_expr_sources_rows) /
          sizeof(d_tests_parse_expr_sources_rows[0]) }
};

// d_tests_parse_expr_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_parse_expr_module =
{
    "parse_expr",
    "parse/expr: expression core v2's C core",
    d_tests_parse_expr_blocks,
    sizeof(d_tests_parse_expr_blocks) / sizeof(d_tests_parse_expr_blocks[0])
};

// d_tests_parse_expr_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_parse_expr_spec(void)
{
    return &d_tests_parse_expr_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_PARSE_EXPR_TESTS_H
