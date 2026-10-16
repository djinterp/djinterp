/*******************************************************************************
* djinterp [test]                                                   test_lex_c.c
*
* Smoke + differential driver for the C lexer.
*
*
* path:      /tests/parse/test_lex_c.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// smoke + differential driver for the C lexer
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;
static void check(int ok, const char* what)
{
    if (!ok) { printf("  FAIL %s\n", what); ++failures; }
}

static void expect(const char* src, unsigned level, const int* kinds, size_t n)
{
    struct d_source s;
    d_source_init(&s, src, strlen(src), "<test>");
    struct d_lexer* lx = d_lex_c_create(&s, level, D_LEX_OPT_KEYWORDS);
    struct d_token t; size_t i = 0;
    while (d_lex_next(lx, &t)) {
        if (i < n && t.kind != kinds[i]) {
            printf("  FAIL %-28s tok %zu: got %s want %s\n", src, i,
                   d_token_kind_name(t.kind), d_token_kind_name(kinds[i]));
            ++failures;
        }
        ++i;
    }
    if (i != n) { printf("  FAIL %-28s count: got %zu want %zu\n", src, i, n); ++failures; }
    d_lex_destroy(lx);
}

int main(void)
{
    const char* reason = NULL;
    check(d_lex_dialect_validate(d_lex_c_dialect(D_LEX_C23), &reason), "c23 validates");
    if (reason) printf("  reason: %s\n", reason);

    { const int k[] = {D_TOKEN_KW_INT, D_TOKEN_IDENTIFIER, D_TOKEN_ASSIGN,
                       D_TOKEN_NUMBER, D_TOKEN_SEMICOLON};
      expect("int x = 42;", D_LEX_C23, k, 5); }

    // restrict is a keyword at C99, an identifier at C89
    { const int k[] = {D_TOKEN_KW_RESTRICT}; expect("restrict", D_LEX_C99, k, 1); }
    { const int k[] = {D_TOKEN_IDENTIFIER};  expect("restrict", D_LEX_C89, k, 1); }

    // maximal munch
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_SHIFT_RIGHT_ASSIGN, D_TOKEN_NUMBER};
      expect("a>>=2", D_LEX_C23, k, 3); }
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_ELLIPSIS, D_TOKEN_IDENTIFIER};
      expect("a...b", D_LEX_C23, k, 3); }

    // pp-number swallows what is not yet a literal
    { const int k[] = {D_TOKEN_NUMBER}; expect("0x1e+2", D_LEX_C23, k, 1); }
    { const int k[] = {D_TOKEN_NUMBER}; expect("1'000'000", D_LEX_C23, k, 1); }
    // no digit separators before C23: `'000'` lexes as a character literal
    { const int k[] = {D_TOKEN_NUMBER, D_TOKEN_CHARACTER, D_TOKEN_NUMBER};
      expect("1'000'000", D_LEX_C11, k, 3); }

    // digraphs carry the kind they spell
    { const int k[] = {D_TOKEN_BRACE_OPEN, D_TOKEN_BRACE_CLOSE};
      expect("<%%>", D_LEX_C23, k, 2); }

    // literal prefixes
    { const int k[] = {D_TOKEN_STRING}; expect("u8\"hi\"", D_LEX_C23, k, 1); }
    { const int k[] = {D_TOKEN_IDENTIFIER}; expect("u8", D_LEX_C23, k, 1); }
    { const int k[] = {D_TOKEN_CHARACTER}; expect("L'x'", D_LEX_C89, k, 1); }

    // header names only inside an inclusion directive
    { const int k[] = {D_TOKEN_HASH, D_TOKEN_IDENTIFIER, D_TOKEN_HEADER_NAME};
      expect("#include <stdio.h>", D_LEX_C23, k, 3); }
    { const int k[] = {D_TOKEN_IDENTIFIER, D_TOKEN_LESS, D_TOKEN_IDENTIFIER,
                       D_TOKEN_GREATER, D_TOKEN_IDENTIFIER};
      expect("a < b > c", D_LEX_C23, k, 5); }

    // line splice is invisible and the token is flagged
    { struct d_source s; const char* src = "in\\\nt v;";
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23, D_LEX_OPT_KEYWORDS);
      struct d_token t; d_lex_next(lx, &t);
      check(t.kind == D_TOKEN_KW_INT, "spliced `int` is a keyword");
      check((t.flags & D_TOKEN_FLAG_SPLICED) != 0, "splice is flagged");
      char buf[16]; d_token_spell(lx, &t, buf, sizeof buf);
      check(strcmp(buf, "int") == 0, "spelling drops the splice");
      d_lex_destroy(lx); }

    // trigraph at C17, gone at C23
    { const int k[] = {D_TOKEN_HASH}; expect("?\?=", D_LEX_C17, k, 1); }
    { const int k[] = {D_TOKEN_QUESTION, D_TOKEN_QUESTION, D_TOKEN_ASSIGN};
      expect("?\?=", D_LEX_C23, k, 3); }

    // errors do not stop the scan
    { struct d_source s; const char* src = "\"open\nint y;";
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23, D_LEX_OPT_KEYWORDS);
      struct d_parse_diagnostic items[4];
      struct d_parse_diag_sink sink;
      d_parse_diag_sink_init(&sink, items, 4u, NULL, 0u);
      d_lex_set_diagnostics(lx, &sink);
      struct d_token t; int n = 0, kw = 0;
      while (d_lex_next(lx, &t)) { ++n; if (t.kind == D_TOKEN_KW_INT) kw = 1; }
      check(kw, "scanning resumes past an unterminated literal");
      check(sink.count == 1u && items[0].code == D_LEX_ERR_UNTERMINATED_LITERAL &&
            items[0].domain == D_PARSE_DIAG_DOMAIN_LEX &&
            items[0].span.offset == 0u && items[0].span.length == 5u,
            "error emitted into the sink, spanning the literal");
      d_lex_destroy(lx); }

    // directive names are never keywords, and are flagged
    { struct d_source s; const char* src = "#if X\n#else\n#endif\nif";
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23, D_LEX_OPT_KEYWORDS);
      struct d_token t; int names = 0, stray_kw = 0, bare_if = 0;
      while (d_lex_next(lx, &t)) {
          if (t.flags & D_TOKEN_FLAG_DIRECTIVE_NAME) {
              ++names; if (t.kind != D_TOKEN_IDENTIFIER) ++stray_kw; }
          if (!(t.flags & D_TOKEN_FLAG_IN_DIRECTIVE) && t.kind == D_TOKEN_KW_IF)
              bare_if = 1; }
      check(names == 3, "three directive names flagged");
      check(stray_kw == 0, "no directive name resolved to a keyword");
      check(bare_if, "`if` outside a directive is still a keyword");
      d_lex_destroy(lx); }

    // the macro a directive defines or tests is flagged, however spelled
    { struct d_source s;
      const char* src = "#define inline 1\n#if defined(A) && defined B\n#endif";
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23, D_LEX_OPT_KEYWORDS);
      struct d_token t; int macros = 0, kw = 0;
      while (d_lex_next(lx, &t)) {
          if (t.flags & D_TOKEN_FLAG_MACRO_NAME) {
              ++macros; if (t.kind != D_TOKEN_IDENTIFIER) ++kw; } }
      check(macros == 3, "`inline`, `A` and `B` flagged as macro names");
      check(kw == 0, "a macro named `inline` is not the keyword");
      d_lex_destroy(lx); }

    // comment kinds, and the banner rule is not documentation
    { struct d_source s;
      const char* src = "// a\n/// b\n//// c\n/* d */\n/** e */\n/*****/\n/**/";
      const uint32_t want[] = {
          D_TOKEN_FLAG_COMMENT_LINE,
          D_TOKEN_FLAG_COMMENT_LINE | D_TOKEN_FLAG_COMMENT_DOC,
          D_TOKEN_FLAG_COMMENT_LINE,
          0u, D_TOKEN_FLAG_COMMENT_DOC, 0u, 0u };
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23,
                                          D_LEX_OPT_EMIT_COMMENTS);
      struct d_token t; size_t i = 0; int ok = 1;
      const uint32_t mask = D_TOKEN_FLAG_COMMENT_LINE | D_TOKEN_FLAG_COMMENT_DOC;
      while (d_lex_next(lx, &t)) {
          if (i < 7 && (t.flags & mask) != want[i]) ok = 0;
          ++i; }
      check(ok && i == 7, "seven comments, each of the right kind");
      d_lex_destroy(lx); }

    // manual header mode hands over the header context and nothing else
    { struct d_source s; const char* src = "#include x";
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23,
                                          D_LEX_OPT_MANUAL_HEADER);
      struct d_token t; d_lex_next(lx, &t);
      check(d_lex_in_directive(lx), "manual mode still tracks directives");
      d_lex_destroy(lx); }

    // a comment before `#` does not stop it opening a directive, whether or
    // not comments are reported
    for (int emit = 0; emit < 2; ++emit)
    { struct d_source s; const char* src = "/* note */ #define X 1";
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23,
          D_LEX_OPT_KEYWORDS | (emit ? D_LEX_OPT_EMIT_COMMENTS : 0u));
      struct d_token t; int macro = 0;
      while (d_lex_next(lx, &t))
          if (t.flags & D_TOKEN_FLAG_MACRO_NAME) macro = 1;
      check(macro, emit ? "directive after a comment (comments emitted)"
                        : "directive after a comment (comments skipped)");
      d_lex_destroy(lx); }

    // a reported comment is positioned where it begins, not where it ends
    { struct d_source s; const char* src = "int a;\n/** doc */\n  // tail";
      d_source_init(&s, src, strlen(src), "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23,
                                          D_LEX_OPT_EMIT_COMMENTS);
      struct d_token t; int ok = 1, seen = 0;
      while (d_lex_next(lx, &t)) {
          if (t.kind != D_TOKEN_COMMENT) continue;
          ++seen;
          if (seen == 1 && !(t.span.line == 2 && t.span.column == 1)) ok = 0;
          if (seen == 2 && !(t.span.line == 3 && t.span.column == 3)) ok = 0; }
      check(ok && seen == 2, "comments positioned at their start");
      d_lex_destroy(lx); }

    printf(failures ? "\n%d check(s) failed\n" : "\nall checks passed\n", failures);
    return failures ? 1 : 0;
}
