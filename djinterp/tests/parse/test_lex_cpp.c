/*******************************************************************************
* djinterp [test]                                                 test_lex_cpp.c
*
* Unit checks for the C++ lexer, and the cross-language invariants.
*
*
* path:      /tests/parse/test_lex_cpp.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// unit checks for the C++ lexer, and the cross-language invariants
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
static void check(int ok, const char* what)
{ if (!ok) { printf("  FAIL %s\n", what); ++failures; } }

static void expect(const char* src, unsigned level, const int* kinds, size_t n)
{
    struct d_source s; d_source_init(&s, src, strlen(src), "<t>");
    struct d_lexer* lx = d_lex_cpp_create(&s, level, D_LEX_OPT_KEYWORDS);
    struct d_token t; size_t i = 0;
    while (d_lex_next(lx, &t)) {
        if (i < n && t.kind != kinds[i]) {
            printf("  FAIL %-26s tok %zu: got %s want %s\n", src, i,
                   d_token_kind_name(t.kind), d_token_kind_name(kinds[i]));
            ++failures; }
        ++i; }
    if (i != n) { printf("  FAIL %-26s count %zu want %zu\n", src, i, n); ++failures; }
    d_lex_destroy(lx);
}

int main(void)
{
    const char* reason = NULL;
    check(d_lex_dialect_validate(d_lex_cpp_dialect(D_LEX_CPP23), &reason),
          "c++23 validates");
    if (reason) printf("  reason: %s\n", reason);

    // C++-only punctuators
    { const int k[] = {D_TOKEN_CPP_IDENTIFIER, D_TOKEN_CPP_SCOPE,
                       D_TOKEN_CPP_IDENTIFIER};
      expect("std::cout", D_LEX_CPP98, k, 3); }
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_ARROW_STAR, D_TOKEN_IDENTIFIER};
      expect("p->*m", D_LEX_CPP98, k, 3); }

    // <=> is C++20; before it, maximal munch gives <= then >
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_COMPARE, D_TOKEN_IDENTIFIER};
      expect("a<=>b", D_LEX_CPP20, k, 3); }
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_LESS_EQUAL,
                       D_TOKEN_GREATER, D_TOKEN_IDENTIFIER};
      expect("a<=>b", D_LEX_CPP17, k, 4); }

    // alternative tokens are punctuator kinds, flagged as alternative spellings
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_AND, D_TOKEN_IDENTIFIER};
      expect("a and b", D_LEX_CPP23, k, 3); }
    { struct d_source s; d_source_init(&s, "a and b", 7, "<t>");
      struct d_lexer* lx = d_lex_cpp_create(&s, D_LEX_CPP23, D_LEX_OPT_KEYWORDS);
      struct d_token t; d_lex_next(lx,&t); d_lex_next(lx,&t);
      check(t.kind == D_TOKEN_AND, "`and` is `&&`");
      check((t.flags & D_TOKEN_FLAG_DIGRAPH) != 0, "`and` flagged alternative");
      d_lex_destroy(lx); }
    // without keyword resolution `and` is an identifier, as phase 3 requires
    { struct d_source s; d_source_init(&s, "a and b", 7, "<t>");
      struct d_lexer* lx = d_lex_cpp_create(&s, D_LEX_CPP23, D_LEX_OPT_NONE);
      struct d_token t; d_lex_next(lx,&t); d_lex_next(lx,&t);
      check(t.kind == D_TOKEN_IDENTIFIER, "`and` is an identifier in phase 3");
      d_lex_destroy(lx); }

    // raw strings
    { const int k[] = {D_TOKEN_RAW_STRING}; expect("R\"(a\"b)\"", D_LEX_CPP11, k, 1); }
    { const int k[] = {D_TOKEN_RAW_STRING};
      expect("R\"xy(a)xz(b)xy\"", D_LEX_CPP11, k, 1); }
    { const int k[] = {D_TOKEN_RAW_STRING}; expect("u8R\"(hi)\"", D_LEX_CPP11, k, 1); }
    // a raw string spans lines, and a backslash-newline inside it is text
    { const char* src = "R\"(a\\\nb)\"";
      struct d_source s; d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_cpp_create(&s, D_LEX_CPP11, D_LEX_OPT_KEYWORDS);
      struct d_token t; d_lex_next(lx, &t);
      check(t.kind == D_TOKEN_RAW_STRING, "raw string spans a line");
      check(t.span.length == strlen(src), "raw string covers the backslash-newline");
      d_lex_destroy(lx); }
    // before C++11 there are no raw strings: `R` is an identifier
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_STRING};
      expect("R\"(a)\"", D_LEX_CPP98, k, 2); }

    // the <:: rule
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_LESS, D_TOKEN_SCOPE,
                       D_TOKEN_IDENTIFIER, D_TOKEN_GREATER};
      expect("A<::B>", D_LEX_CPP23, k, 5); }
    // but `<:` is still a bracket when a `:` or `>` follows the `::`
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_BRACKET_OPEN, D_TOKEN_SCOPE,
                       D_TOKEN_GREATER};
      expect("A<:::>", D_LEX_CPP23, k, 4); }

    // digit separators are C++14
    { const int k[] = {D_TOKEN_NUMBER}; expect("1'000'000", D_LEX_CPP14, k, 1); }

    // user-defined literal suffix rides along on the literal
    { const int k[] = {D_TOKEN_STRING}; expect("\"s\"sv", D_LEX_CPP11, k, 1); }
    { const int k[] = {D_TOKEN_NUMBER};  expect("1_km", D_LEX_CPP11, k, 1); }

    // cross-language: shared kinds are one value, private kinds are disjoint
    check(D_TOKEN_C_IDENTIFIER == D_TOKEN_CPP_IDENTIFIER, "shared kinds agree");
    check(D_TOKEN_C_KW_RESTRICT != D_TOKEN_CPP_KW_CLASS, "private kinds differ");
    check(d_token_is_cpp_only(D_TOKEN_CPP_KW_CLASS), "class is C++ only");
    check(d_token_is_c_only(D_TOKEN_C_KW_RESTRICT),  "restrict is C only");
    check(d_token_is_shared(D_TOKEN_KW_INT),         "int is shared");

    // the same text, both languages: `class` is a keyword in one only
    { const int k[] = {D_TOKEN_CPP_KW_CLASS, D_TOKEN_IDENTIFIER, D_TOKEN_SEMICOLON};
      expect("class T;", D_LEX_CPP23, k, 3); }
    { struct d_source s; d_source_init(&s, "class T;", 8, "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23, D_LEX_OPT_KEYWORDS);
      struct d_token t; d_lex_next(lx, &t);
      check(t.kind == D_TOKEN_IDENTIFIER, "`class` is an identifier in C");
      d_lex_destroy(lx); }

    printf(failures ? "\n%d check(s) failed\n" : "\nall checks passed\n", failures);
    return failures ? 1 : 0;
}
