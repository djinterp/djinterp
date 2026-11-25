/*******************************************************************************
* djinterp [c][parse]                                         syntax_tests_lex.c
*
* Parse's syntax suite, block "lex": two readings of the scanner the items
* stand on, each as Clang's raw lexer reads the text: C23's `::`, and a
* comment that a line splice comes before.
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/syntax_tests_lex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./syntax_tests.h"  // corresponding header


// D_TESTS_SYNTAX_LEX_MOST
//   constant: the most tokens a text here lexes into.
#define D_TESTS_SYNTAX_LEX_MOST  32u

/*
d_tests_syntax_lex_kinds
  Lexes a text as C at a level, comments kept, into an array; the count.
*/
static size_t
d_tests_syntax_lex_kinds(
    const char*      _text,
    unsigned         _level,
    struct dj_token* _out
)
{
    struct dj_source source;
    size_t           count = 0u;

    d_source_init(&source, _text, strlen(_text), "t.c");

    struct dj_lexer* lexer = d_lex_c_create(&source, _level,
                                            D_LEX_OPT_EMIT_COMMENTS |
                                            D_LEX_OPT_KEYWORDS);

    count = (lexer) ? d_lex_tokenize(lexer, _out, D_TESTS_SYNTAX_LEX_MOST)
                    : 0u;
    d_lex_destroy(lexer);

    return count;
}

/*
d_tests_syntax_lex_scope
  C23's `::`, a punctuator for an attribute's prefix.
  Tests the following:
  - from C23, `[[gnu::unused]]` holds one `::`, the kind C++ reads it as
  - before C23, the same text holds two colons
  - `<::` before C23 is a digraph `<:` and a colon, and C23 keeps that
*/
bool
d_tests_syntax_lex_scope(void)
{
    struct dj_token tokens[D_TESTS_SYNTAX_LEX_MOST];
    size_t          n;

    n = d_tests_syntax_lex_kinds("[[gnu::unused]] int a;", D_LEX_C23,
                                 tokens);
    D_TEST_REQUIRE( (n == 10u) &&
                    (tokens[3].kind == D_TOKEN_SCOPE) &&
                    (tokens[3].span.offset == 5u) &&
                    (tokens[3].span.length == 2u) &&
                    (tokens[4].kind == D_TOKEN_IDENTIFIER) );

    n = d_tests_syntax_lex_kinds("[[gnu::unused]] int a;", D_LEX_C17,
                                 tokens);
    D_TEST_REQUIRE( (n == 11u) &&
                    (tokens[3].kind == D_TOKEN_COLON) &&
                    (tokens[4].kind == D_TOKEN_COLON) &&
                    (tokens[4].span.offset == 6u) );

    n = d_tests_syntax_lex_kinds("a<::b", D_LEX_C23, tokens);
    D_TEST_REQUIRE( (n == 4u) &&
                    (tokens[1].kind == D_TOKEN_BRACKET_OPEN) &&
                    (tokens[2].kind == D_TOKEN_COLON) );

    return true;
}

/*
d_tests_syntax_lex_splice
  A comment that a line splice comes before, or holds.
  Tests the following:
  - the comment begins where the splice does, as Clang's raw lexer reads it,
    and is flagged spliced, so a reader of its bytes takes the splice out
  - a comment with a splice inside it is flagged too; one with none is not
  - the token after such a comment begins at its own first character
*/
bool
d_tests_syntax_lex_splice(void)
{
    struct dj_token tokens[D_TESTS_SYNTAX_LEX_MOST];
    size_t          n;

    n = d_tests_syntax_lex_kinds("int a;\n  \\\n/* c */ int b;", 0u,
                                 tokens);
    D_TEST_REQUIRE( (n == 7u) &&
                    (tokens[3].kind == D_TOKEN_COMMENT) &&
                    (tokens[3].span.offset == 9u) &&
                    (tokens[3].span.line == 2u) &&
                    (tokens[3].span.column == 3u) &&
                    ((tokens[3].flags & D_TOKEN_FLAG_SPLICED) != 0u) &&
                    (tokens[4].kind == D_TOKEN_KW_INT) &&
                    (tokens[4].span.offset == 19u) &&
                    ((tokens[4].flags & D_TOKEN_FLAG_SPLICED) == 0u) );

    n = d_tests_syntax_lex_kinds("/* x \\\ny */ /* z */ \\\n// w\n", 0u,
                                 tokens);
    D_TEST_REQUIRE( (n == 3u) &&
                    ((tokens[0].flags & D_TOKEN_FLAG_SPLICED) != 0u) &&
                    ((tokens[1].flags & D_TOKEN_FLAG_SPLICED) == 0u) &&
                    (tokens[2].kind == D_TOKEN_COMMENT) &&
                    (tokens[2].span.offset == 20u) &&
                    ((tokens[2].flags & D_TOKEN_FLAG_SPLICED) != 0u) );

    return true;
}
