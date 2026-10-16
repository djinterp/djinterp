/*******************************************************************************
* djinterp [test]                                             test_lex_unicode.c
*
* Unit tests: UTF-8 validation, identifier sets, UCNs, NFC, end positions,.
*
*
* path:      /tests/parse/test_lex_unicode.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// unit tests: UTF-8 validation, identifier sets, UCNs, NFC, end positions,
// code-point columns, and the scanner's diagnostic channel
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/lex/lex_unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;
static void check(int ok, const char* what)
{
    if (!ok) { printf("  FAIL %s\n", what); ++failures; }
}

// a reference decoder written straight from Table 3-7, to compare against
static size_t reference(const unsigned char* b, size_t n, uint32_t* cp)
{
    static const struct { unsigned char lo, hi, s1lo, s1hi; size_t len; } t[] = {
        {0xC2, 0xDF, 0x80, 0xBF, 2}, {0xE0, 0xE0, 0xA0, 0xBF, 3},
        {0xE1, 0xEC, 0x80, 0xBF, 3}, {0xED, 0xED, 0x80, 0x9F, 3},
        {0xEE, 0xEF, 0x80, 0xBF, 3}, {0xF0, 0xF0, 0x90, 0xBF, 4},
        {0xF1, 0xF3, 0x80, 0xBF, 4}, {0xF4, 0xF4, 0x80, 0x8F, 4}};
    if (n && b[0] < 0x80) { *cp = b[0]; return 1; }
    for (size_t i = 0; i < 8; ++i) {
        if (!n || b[0] < t[i].lo || b[0] > t[i].hi) continue;
        if (n < t[i].len || b[1] < t[i].s1lo || b[1] > t[i].s1hi) return 0;
        uint32_t v = b[0] & (t[i].len == 2 ? 0x1F : t[i].len == 3 ? 0x0F : 0x07);
        for (size_t k = 1; k < t[i].len; ++k) {
            if ((b[k] & 0xC0) != 0x80) return 0;
            v = (v << 6) | (b[k] & 0x3F);
        }
        *cp = v; return t[i].len;
    }
    return 0;
}

// scan a whole source into a sink; returns the scanner for inspection
struct run { struct d_source s; struct d_lexer* lx; struct d_parse_diag_sink sink;
             struct d_parse_diagnostic items[32]; char text[4096];
             struct d_token toks[64]; size_t n; };

static void scan(struct run* r, const char* src, size_t len, int cpp, unsigned level,
                 unsigned options)
{
    (void)len;  // every source here is NUL-free
    d_source_init(&r->s, src, strlen(src), "<t>");
    r->lx = cpp ? d_lex_cpp_create(&r->s, level, options)
                : d_lex_c_create(&r->s, level, options);
    d_parse_diag_sink_init(&r->sink, r->items, 32u, r->text, sizeof r->text);
    d_lex_set_diagnostics(r->lx, &r->sink);
    r->n = 0;
    while (r->n < 64 && d_lex_next(r->lx, &r->toks[r->n])) ++r->n;
    d_lex_destroy(r->lx);
}

static int has(const struct run* r, int code)
{
    for (uint32_t i = 0; i < r->sink.count; ++i)
        if (r->items[i].code == code && r->items[i].domain == D_PARSE_DIAG_DOMAIN_LEX)
            return 1;
    return 0;
}

int main(void)
{
    // 1. the decoder agrees with the reference on every 1-, 2- and 3-byte
    //    input and on every 4-byte input with a 4-byte lead
    { size_t bad = 0; unsigned char b[4]; uint32_t x = 0, y = 0;
      for (uint32_t v = 0; v < 0x1000000u; ++v) {
          b[0] = (unsigned char)(v >> 16); b[1] = (unsigned char)(v >> 8);
          b[2] = (unsigned char)v;
          size_t n = 3;
          size_t p = d_source_utf8_decode((const char*)b, n, &x);
          size_t q = reference(b, n, &y);
          if (p != q || (p && x != y)) ++bad;
      }
      for (uint32_t v = 0; v < 0x1000000u; v += 7u) {
          b[0] = 0xF0 | (unsigned char)(v >> 22 & 7); b[1] = (unsigned char)(v >> 14);
          b[2] = (unsigned char)(v >> 6); b[3] = (unsigned char)(0x80 | (v & 0x3F));
          size_t p = d_source_utf8_decode((const char*)b, 4, &x);
          size_t q = reference(b, 4, &y);
          if (p != q || (p && x != y)) ++bad;
      }
      check(bad == 0, "UTF-8 decoder matches Table 3-7 exhaustively"); }

    // 2. identifier sets
    check(d_lex_unicode_is_id_start(D_LEX_IDSET_XID, 0x00E9u), "e-acute is XID_Start");
    check(!d_lex_unicode_is_id_start(D_LEX_IDSET_XID, 0x0301u), "combining acute is not XID_Start");
    check(d_lex_unicode_is_id_continue(D_LEX_IDSET_XID, 0x0301u), "combining acute is XID_Continue");
    check(!d_lex_unicode_is_id_continue(D_LEX_IDSET_XID, 0x00D7u), "multiplication sign is not");
    check(!d_lex_unicode_is_id_continue(D_LEX_IDSET_XID, 0x1F600u), "emoji is not");
    check(d_lex_unicode_is_id_start(D_LEX_IDSET_C11, 0x1F600u), "C11 Annex D admits emoji");
    check(!d_lex_unicode_is_id_start(D_LEX_IDSET_C11, 0x0301u), "C11 D.2 not initially");
    check(d_lex_unicode_is_id_continue(D_LEX_IDSET_C11, 0x0301u), "C11 D.2 continues");
    check(!d_lex_unicode_is_id_start(D_LEX_IDSET_C99, 0x0660u), "C99 digit not initially");
    check(d_lex_unicode_is_id_continue(D_LEX_IDSET_C99, 0x0660u), "C99 digit continues");
    check(!d_lex_unicode_is_id_start(D_LEX_IDSET_XID, 'a'), "ASCII is the scanner's");

    // 3. NFC
    { const uint32_t pre[] = {'c', 'a', 'f', 0xE9};
      const uint32_t dec[] = {'c', 'a', 'f', 'e', 0x301};
      const uint32_t hang[] = {0x1100, 0x1161};
      const uint32_t ord[]  = {'a', 0x301, 0x323};
      check(d_lex_unicode_is_nfc(pre, 4, true), "precomposed is NFC");
      check(!d_lex_unicode_is_nfc(dec, 5, true), "decomposed is not NFC");
      check(d_lex_unicode_is_nfc(dec, 5, false), "partial check never invents");
      check(!d_lex_unicode_is_nfc(hang, 2, true), "conjoining jamo compose");
      check(!d_lex_unicode_is_nfc(ord, 3, true), "marks out of order"); }

    struct run r;

    // 4. identifiers in the scanner
    scan(&r, "int caf\xC3\xA9 = 1;", 15, 0, D_LEX_C23, D_LEX_OPT_KEYWORDS);
    check(r.n == 5 && r.toks[1].span.length == 5 && r.sink.count == 0,
          "UTF-8 identifier is one token, no diagnostic");
    check(r.toks[1].flags & D_TOKEN_FLAG_EXTENDED, "flagged extended");
    check(r.toks[2].span.column == 10, "column after it counts code points");

    scan(&r, "a\xC3\x97" "b", 4, 0, D_LEX_C23, 0);
    check(r.n == 3 && r.toks[1].kind == D_TOKEN_OTHER && r.toks[1].span.length == 2
          && has(&r, D_LEX_ERR_STRAY_CHARACTER),
          "non-XID character ends the name and is one stray token");

    scan(&r, "cafe\xCC\x81", 6, 1, D_LEX_CPP23, 0);
    check(r.n == 1 && has(&r, D_LEX_ERR_NOT_NFC), "non-NFC name reported in C++");
    scan(&r, "cafe\xCC\x81", 6, 0, D_LEX_C11, 0);
    check(r.n == 1 && !has(&r, D_LEX_ERR_NOT_NFC), "C11 does not require NFC");

    scan(&r, "a\\u00e9b \\u0301x", 16, 1, D_LEX_CPP20, 0);
    check(r.n == 2 && r.sink.count == 1 && has(&r, D_LEX_ERR_UNIVERSAL_NAME) &&
          r.items[0].span.offset == 9 && r.items[0].span.length == 6,
          "UCN that may not begin a name is reported on the UCN itself");
    scan(&r, "a\\u0041", 7, 0, D_LEX_C23, 0);
    check(r.n == 1 && has(&r, D_LEX_ERR_UNIVERSAL_NAME), "UCN naming a basic character");
    scan(&r, "\\u12x", 5, 0, D_LEX_C23, 0);
    check(r.n == 2 && r.toks[0].kind == D_TOKEN_OTHER && has(&r, D_LEX_ERR_UNIVERSAL_NAME),
          "malformed UCN: the backslash, reported as a UCN");
    scan(&r, "a\\u{e9}b", 8, 1, D_LEX_CPP23, 0);
    check(r.n == 1 && r.sink.count == 0, "C++23 delimited UCN in a name");
    scan(&r, "a\\u{e9}b", 8, 1, D_LEX_CPP20, 0);
    check(r.n > 1, "delimited UCN is C++23 only");
    scan(&r, "a\\N{LATIN SMALL LETTER E WITH ACUTE}", 36, 1, D_LEX_CPP23, 0);
    check(r.n == 1 && r.sink.count == 0, "named UCN in a name, shape only");

    // 5. encoding
    scan(&r, "/* \xFF */ x \"\xC3\" y", 17, 0, D_LEX_C23, 0);
    check(r.sink.count == 2 && r.items[0].code == D_LEX_ERR_ENCODING &&
          r.items[0].span.offset == 3 && r.items[1].span.offset == 11,
          "ill-formed UTF-8 reported in a comment and a literal, once each");
    scan(&r, "/* \xFF */ x", 10, 0, D_LEX_C23, D_LEX_OPT_RAW_BYTES);
    check(r.sink.count == 0, "RAW_BYTES turns the check off");
    scan(&r, "a\xEF\xBB\xBF" "b", 5, 0, D_LEX_C23, 0);
    check(r.n == 3 && has(&r, D_LEX_ERR_STRAY_CHARACTER), "a BOM mid-file is stray");

    // 6. end positions and multi-line tokens
    scan(&r, "x /* a\n b */ y", 14, 0, D_LEX_C23, D_LEX_OPT_EMIT_COMMENTS);
    check(r.n == 3 && r.toks[1].span.line == 1 && r.toks[1].end_line == 2 &&
          r.toks[1].end_column == 6, "block comment ends where it closes");
    check(r.toks[0].end_line == 1 && r.toks[0].end_column == 2, "end is half-open");
    scan(&r, "R\"d(\n\n)d\"", 10, 1, D_LEX_CPP23, 0);
    check(r.n == 1 && r.toks[0].end_line == 3 && r.toks[0].end_column == 4,
          "raw string end position");
    scan(&r, "in\\\nt", 5, 0, D_LEX_C23, D_LEX_OPT_KEYWORDS);
    check(r.n == 1 && r.toks[0].kind == D_TOKEN_KW_INT && r.toks[0].end_line == 2,
          "spliced keyword ends on the next line");

    // 7. the diagnostic channel
    scan(&r, "''", 2, 0, D_LEX_C23, 0);
    check(r.n == 1 && r.toks[0].kind == D_TOKEN_ERROR && has(&r, D_LEX_ERR_EMPTY_CHARACTER),
          "empty character literal");
    { struct d_source s; const char* src = "\"x";
      d_source_init(&s, src, 2, "<t>");
      struct d_lexer* lx = d_lex_c_create(&s, D_LEX_C23, 0);
      struct d_parse_diag_sink k; d_parse_diag_sink_init(&k, NULL, 0, NULL, 0);
      d_lex_set_diagnostics(lx, &k);
      struct d_token t;
      d_lex_peek(lx, 0, &t); d_lex_peek(lx, 0, &t);
      check(d_parse_diag_tally(&k, D_PARSE_SEVERITY_ERROR) == 0, "peek does not report");
      d_lex_next(lx, &t);
      check(d_parse_diag_tally(&k, D_PARSE_SEVERITY_ERROR) == 1 && d_parse_diag_failed(&k),
            "a storage-less sink counts");
      d_lex_destroy(lx); }
    check(!strcmp(d_lex_error_text(D_LEX_ERR_ENCODING), "source is not well-formed UTF-8")
          && !strcmp(d_lex_error_text(0), "unrecognized code"), "error texts");
    check(d_lex_error_severity(D_LEX_ERR_MULTICHARACTER) == D_PARSE_SEVERITY_WARNING &&
          d_lex_error_severity(D_LEX_ERR_NUMBER_DIGIT) == D_PARSE_SEVERITY_ERROR,
          "error severities");
    check(strcmp(d_lex_unicode_version(), "18.0.0") == 0, "tables are Unicode 18.0.0");

    printf(failures ? "%d failure(s)\n" : "all checks passed\n", failures);
    return failures ? 1 : 0;
}
