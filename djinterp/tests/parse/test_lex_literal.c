/*******************************************************************************
* djinterp [test]                                             test_lex_literal.c
*
* Unit tests: the literal decoder, especially what diff_literal.py cannot ask.
*
*
* path:      /tests/parse/test_lex_literal.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// unit tests: the literal decoder, especially what diff_literal.py cannot ask
// gcc 13 -- `wb`, decimal suffixes, `\N{}`, unknown escapes, ud-suffixes,
// misplaced separators, P1854 -- plus diagnostic placement and spliced tokens
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/lex/lex_literal.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;

struct result { struct d_lex_literal lit; int ok; int tokens; int code;
                int severity; uint32_t offset, length, column, count; };

static const struct d_lex_target* target = NULL;

static struct result decode(const char* src, int cpp, unsigned level)
{
    struct result r;
    memset(&r, 0, sizeof r);
    struct d_source s;
    d_source_init(&s, src, strlen(src), "<t>");
    struct d_lexer* lx = cpp ? d_lex_cpp_create(&s, level, 0)
                             : d_lex_c_create(&s, level, 0);
    struct d_parse_diagnostic items[8];
    struct d_parse_diag_sink sink;
    d_parse_diag_sink_init(&sink, items, 8u, NULL, 0u);
    struct d_token t, more;
    if (d_lex_next(lx, &t)) r.tokens = 1;
    if (d_lex_next(lx, &more)) ++r.tokens;
    d_lex_set_diagnostics(lx, &sink);
    r.ok = d_lex_literal_decode_token(lx, target, &t, &sink, &r.lit);
    r.count = sink.count;
    if (sink.count) {
        r.code = items[0].code; r.severity = items[0].severity;
        r.offset = items[0].span.offset; r.length = items[0].span.length;
        r.column = items[0].span.column;
    }
    d_lex_destroy(lx);
    return r;
}

static void ok(const char* src, int cpp, unsigned level, const char* what)
{
    struct result r = decode(src, cpp, level);
    if (!r.ok || r.tokens != 1 || r.count) {
        printf("  FAIL %-24s %s: ok %d tokens %d code %d\n", src, what, r.ok,
               r.tokens, r.code);
        ++failures;
    }
}

static void bad(const char* src, int cpp, unsigned level, int code, const char* what)
{
    struct result r = decode(src, cpp, level);
    if (r.code != code || (r.severity >= D_PARSE_SEVERITY_ERROR) == r.ok) {
        printf("  FAIL %-24s %s: ok %d code %d want %d\n", src, what, r.ok,
               r.code, code);
        ++failures;
    }
}

static void check(int cond, const char* what)
{
    if (!cond) { printf("  FAIL %s\n", what); ++failures; }
}

int main(void)
{
    const unsigned C11 = D_LEX_C11, C23 = D_LEX_C23;
    const unsigned P17 = D_LEX_CPP17, P20 = D_LEX_CPP20, P23 = D_LEX_CPP23;

    // numbers: forms, bases, separators
    ok("0b1010", 0, C23, "binary in C23");
    bad("0b1010", 0, C11, D_LEX_ERR_NUMBER_SUFFIX, "binary before C23");
    bad("0b102", 0, C23, D_LEX_ERR_NUMBER_DIGIT, "binary digit");
    bad("0789", 0, C23, D_LEX_ERR_NUMBER_DIGIT, "octal digit");
    ok("0789.5", 0, C23, "a leading zero does not make a float octal");
    ok("1'000'000", 0, C23, "separators in C23");
    ok("0x1'f", 1, P17, "separator between hex digits");
    { struct result r = decode("1'0'", 0, C11);
      check(r.tokens == 2, "no separators in C11: `1'0'` is a number and a character"); }
    bad("1e+", 0, C23, D_LEX_ERR_NUMBER_FORM, "exponent without digits");
    bad("0x1.8", 0, C23, D_LEX_ERR_NUMBER_FORM, "hex float needs p");
    ok("0x1.8p1", 1, P17, "hex float in C++17");
    bad("0x1.8p1", 1, D_LEX_CPP14, D_LEX_ERR_NUMBER_FORM, "no hex float in C++14");
    bad("0x", 0, C23, D_LEX_ERR_NUMBER_FORM, "prefix with no digits");

    // numbers: suffixes
    ok("1wb", 0, C23, "wb in C23");
    ok("1uwb", 0, C23, "uwb in C23");
    ok("123456789012345678901234567890wb", 0, C23, "wb takes wide values");
    bad("1wb", 0, C11, D_LEX_ERR_NUMBER_SUFFIX, "no wb before C23");
    ok("1.0df", 0, C23, "decimal float suffix");
    bad("1.0df", 1, P23, D_LEX_ERR_RESERVED_SUFFIX, "in C++ `df` is a reserved ud-suffix");
    ok("1.0f16", 1, P23, "f16 in C++23");
    ok("1.0bf16", 1, P23, "bf16 in C++23");
    bad("1.0bf16", 0, C23, D_LEX_ERR_NUMBER_SUFFIX, "no bf16 in C");
    ok("1.0f32x", 0, C23, "f32x in C23");
    ok("1uz", 1, P23, "uz in C++23");
    bad("1lL", 0, C23, D_LEX_ERR_NUMBER_SUFFIX, "lL is not a suffix");
    bad("1.0u", 0, C23, D_LEX_ERR_NUMBER_SUFFIX, "u on a float");
    bad("9223372036854775808", 0, C23, D_LEX_ERR_NUMBER_RANGE,
        "decimal without u past long long");
    ok("9223372036854775808u", 0, C23, "the same with u");
    ok("0x8000000000000000", 0, C23, "hex may be unsigned");
    bad("18446744073709551616u", 0, C23, D_LEX_ERR_NUMBER_RANGE, "past 64 bits");

    // user-defined suffixes
    ok("10_km", 1, P20, "ud-suffix with an underscore");
    ok("10s", 1, P20, "a library suffix");
    ok("\"x\"sv", 1, P20, "sv on a string");
    bad("10km", 1, P20, D_LEX_ERR_RESERVED_SUFFIX, "reserved ud-suffix");
    bad("10_km", 0, C23, D_LEX_ERR_NUMBER_SUFFIX, "no ud-suffix in C");

    // escapes
    ok("'\\n'", 0, C23, "simple escape");
    bad("'\\q'", 0, C23, D_LEX_ERR_UNKNOWN_ESCAPE, "unknown escape warns");
    bad("\"\\x\"", 0, C23, D_LEX_ERR_ESCAPE_FORM, "\\x with no digit");
    bad("'\\x100'", 0, C23, D_LEX_ERR_ESCAPE_RANGE, "hex escape too wide");
    ok("u'\\x100'", 0, C23, "fits a UTF-16 unit");
    ok("\"\\o{17}\\x{41}\\u{e9}\"", 1, P23, "delimited escapes in C++23");
    bad("\"\\x{41\"", 1, P23, D_LEX_ERR_ESCAPE_FORM, "unclosed delimited escape");
    ok("\"\\N{LATIN SMALL LETTER A}\"", 1, P23, "named escape, shape only");
    bad("\"\\N{}\"", 1, P23, D_LEX_ERR_UNIVERSAL_NAME, "empty named escape");
    bad("\"\\U0000D800\"", 1, P23, D_LEX_ERR_UNIVERSAL_NAME, "surrogate UCN");
    bad("\"\\u0041\"", 0, C23, D_LEX_ERR_UNIVERSAL_NAME, "C: UCN for a basic char");
    ok("\"\\u0041\"", 1, P20, "C++: UCN for a basic char in a literal");
    ok("\"\\u0024\"", 0, C23, "C: $ is allowed");

    // character literals
    bad("'ab'", 0, C23, D_LEX_ERR_MULTICHARACTER, "multicharacter warns");
    bad("u8'\\u00e9'", 1, P20, D_LEX_ERR_CHARACTER_WIDTH, "u8 needs one unit");
    bad("u'\\U0001F600'", 1, P20, D_LEX_ERR_CHARACTER_WIDTH, "C++ u needs one unit");
    bad("u'\\U0001F600'", 0, C23, D_LEX_ERR_WIDE_CHARACTER, "C u warns");
    bad("'\\u20ac'", 1, P20, D_LEX_ERR_MULTICHARACTER, "before C++23: warning");
    bad("'\\u20ac'", 1, P23, D_LEX_ERR_CHARACTER_WIDTH, "C++23: P1854");
    bad("L'ab'", 1, P23, D_LEX_ERR_CHARACTER_WIDTH, "C++23: P2362");

    // values
    { struct result r = decode("'ab'", 0, C23);
      check(r.lit.integer == 0x6162u && r.lit.characters == 2, "multichar value"); }
    { struct result r = decode("0x1'ffu", 1, P17);
      check(r.lit.integer == 0x1FFu && r.lit.base == 16 &&
            (r.lit.suffix & D_LEX_SUFFIX_UNSIGNED) &&
            (r.lit.flags & D_LEX_LITERAL_FLAG_SEPARATOR) &&
            r.lit.suffix_offset == 6 && r.lit.suffix_length == 1,
            "hex value, suffix and separator"); }
    { struct result r = decode("017", 0, C23);
      check(r.lit.integer == 15u && r.lit.base == 8, "octal value"); }
    { struct result r = decode("1.5e1f", 0, C23);
      check(r.lit.category == D_LEX_LITERAL_FLOATING && r.lit.floating == 15.0 &&
            (r.lit.suffix & D_LEX_SUFFIX_FLOAT), "float value"); }
    { struct result r = decode("u8R\"x(a\\b)x\"", 1, P20);
      check(r.lit.category == D_LEX_LITERAL_STRING && r.lit.characters == 3 &&
            r.lit.encoding == D_LEX_ENCODING_UTF8 &&
            (r.lit.flags & D_LEX_LITERAL_FLAG_RAW) && r.ok, "raw string body"); }
    { struct result r = decode("U'\\U0001F600'", 1, P23);
      check(r.ok && r.lit.integer == 0x1F600u, "UTF-32 value"); }

    // placement: on the character when the spelling is the source
    { struct result r = decode("\"ab\\qc\"", 0, C23);
      check(r.offset == 3 && r.length == 2 && r.column == 4, "diagnostic on the escape"); }
    { struct result r = decode("0\\\n9", 0, C23);
      check(r.code == D_LEX_ERR_NUMBER_DIGIT && r.offset == 0 && r.length == 4,
            "a spliced token is decoded from its spelling, reported whole"); }

    // the target: `wchar_t` is UTF-16 on Windows and UTF-32 elsewhere
    target = d_lex_target_preset(D_LEX_TARGET_WINDOWS);
    bad("L'\\U0001F600'", 0, C23, D_LEX_ERR_WIDE_CHARACTER, "Windows: C wide pair warns");
    bad("L'\\U0001F600'", 1, P23, D_LEX_ERR_CHARACTER_WIDTH, "Windows: C++23 wide pair");
    bad("L'\\x10000'", 0, C23, D_LEX_ERR_ESCAPE_RANGE, "Windows: wide escape past 16 bits");
    ok("L'\\xFFFF'", 0, C23, "Windows: wide escape within 16 bits");
    { struct result r = decode("L'\\U0001F600'", 0, C23);
      check(r.lit.integer == 0xDE00u, "Windows: wide pair keeps the low surrogate"); }
    target = d_lex_target_preset(D_LEX_TARGET_UNIX);
    ok("L'\\U0001F600'", 1, P23, "Unix: one wide unit");
    ok("L'\\x10000'", 0, C23, "Unix: wide escape within 32 bits");
    { struct result r = decode("L'\\U0001F600'", 0, C23);
      check(r.lit.integer == 0x1F600u, "Unix: wide value is the code point"); }
    { static const struct d_lex_target narrow = { 8u, 32u, 32u };
      target = &narrow;
      bad("2147483648", 0, C23, D_LEX_ERR_NUMBER_RANGE, "32-bit long long: signed limit");
      bad("4294967296u", 0, C23, D_LEX_ERR_NUMBER_RANGE, "32-bit long long: unsigned limit");
      ok("4294967295u", 0, C23, "32-bit long long: unsigned maximum"); }
    target = NULL;
    check(d_lex_target_preset(D_LEX_TARGET_HOST)->char_bits == 8 &&
          d_lex_target_preset(99) == NULL, "presets");

    printf(failures ? "%d failure(s)\n" : "all checks passed\n", failures);
    return failures ? 1 : 0;
}
