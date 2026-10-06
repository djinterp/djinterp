/*******************************************************************************
* djinterp [c]                                                         t_emoji.c
*
*   Conformance harness for emoji.h's UTF-8 encoders. It checks
* d_text_emoji_utf8_encode at both ends of every UTF-8 length and on each kind
* of value it must refuse; it checks d_text_emoji_utf8, d_text_emoji_utf8_seq
* and their macros against the header's own literals and at the edges of
* D_EMOJI_SEQ_CAPACITY, and that a refused call leaves the previous result
* intact and each function its own buffer.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                 \
*        src/djinterp/c/text/symbol/emoji/emoji.c                             \
*        tests/djinterp/c/text/symbol/emoji/t_emoji.c -o t_emoji
*
*
* path:      /tests/djinterp/c/text/symbol/emoji/t_emoji.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <string.h>   // memcmp, strcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/text/symbol/emoji/emoji.h"  // unit under test
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t,
                                                           // UINT32_MAX


// d_tests_emoji_case
//   struct: one code point and the UTF-8 it must encode to; a length of 0
// means the encoder must refuse it.
struct d_tests_emoji_case
{
    uint32_t    cp;      // the code point
    const char* bytes;   // its encoding
    size_t      length;  // bytes in the encoding, or 0 when refused
};

static int g_checks   = 0;
static int g_failures = 0;


/*
d_tests_emoji_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_emoji_expect(
    bool        _ok,
    const char* _what
)
{
    ++g_checks;

    // report a failure with what was being checked
    if (!_ok)
    {
        ++g_failures;
        printf("FAIL: %s\n", _what);
    }

    return;
}

/*
d_tests_emoji_encode
  d_text_emoji_utf8_encode
  Tests the following:
  - the first and last code point of each UTF-8 length, and one emoji
  - surrogates at both ends of their range, past U+10FFFF, and UINT32_MAX,
    each refused with nothing written
  - a NULL output buffer, refused
*/
static void
d_tests_emoji_encode(void)
{
    static const struct d_tests_emoji_case cases[] =
    {
        { 0x000000u, "\x00",             1 },
        { 0x00007Fu, "\x7F",             1 },
        { 0x000080u, "\xC2\x80",         2 },
        { 0x0007FFu, "\xDF\xBF",         2 },
        { 0x000800u, "\xE0\xA0\x80",     3 },
        { 0x00D7FFu, "\xED\x9F\xBF",     3 },
        { 0x00E000u, "\xEE\x80\x80",     3 },
        { 0x00FFFFu, "\xEF\xBF\xBF",     3 },
        { 0x010000u, "\xF0\x90\x80\x80", 4 },
        { 0x01F600u, "\xF0\x9F\x98\x80", 4 },
        { 0x10FFFFu, "\xF4\x8F\xBF\xBF", 4 },
        { 0x00D800u, "",                 0 },
        { 0x00DFFFu, "",                 0 },
        { 0x110000u, "",                 0 },
        { UINT32_MAX, "",                0 }
    };

    // encode each case into a buffer that shows any stray write
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        char         out[4] = { 'x', 'x', 'x', 'x' };
        const size_t n      = d_text_emoji_utf8_encode(cases[i].cp,
                                                       out);
        char         what[64];

        (void)snprintf(what,
                       sizeof(what),
                       "encode U+%04lX",
                       (unsigned long)cases[i].cp);

        // a refused value must leave the buffer untouched
        if (cases[i].length == 0)
        {
            d_tests_emoji_expect( ( (n == 0) &&
                                    (memcmp(out, "xxxx", 4) == 0) ),
                                  what);
        }
        else
        {
            d_tests_emoji_expect( ( (n == cases[i].length) &&
                                    (memcmp(out,
                                            cases[i].bytes,
                                            n) == 0) ),
                                  what);
        }
    }

    d_tests_emoji_expect(d_text_emoji_utf8_encode(0x41u, NULL) == 0,
                         "encode refuses a NULL buffer");

    return;
}

#if D_THREAD_LOCAL_AVAILABLE

/*
d_tests_emoji_utf8
  d_text_emoji_utf8 and D_EMOJI_UTF8
  Tests the following:
  - an emoji matches the header's literal, through the function and the macro
  - U+0000 gives the empty string
  - a refused value returns NULL and leaves the previous result intact
*/
static void
d_tests_emoji_utf8(void)
{
    const char* grinning = d_text_emoji_utf8(0x1F600u);

    d_tests_emoji_expect( ( (grinning != NULL) &&
                            (strcmp(grinning, D_EMOJI_FACE_GRINNING) == 0) ),
                          "utf8 U+1F600 matches D_EMOJI_FACE_GRINNING");
    d_tests_emoji_expect(strcmp(D_EMOJI_UTF8(0x1F4BB),
                                D_EMOJI_OBJECT_LAPTOP_COMPUTER) == 0,
                         "D_EMOJI_UTF8 gives D_EMOJI_OBJECT_LAPTOP_COMPUTER");

    const char* nul = d_text_emoji_utf8(0x0u);

    d_tests_emoji_expect( ( (nul != NULL) &&
                            (nul[0] == '\0') ),
                          "utf8 U+0000 gives \"\"");

    const char* kept = d_text_emoji_utf8(0x1F600u);

    d_tests_emoji_expect(d_text_emoji_utf8(0xDC00u) == NULL,
                         "utf8 refuses a surrogate");
    d_tests_emoji_expect(d_text_emoji_utf8(0x110000u) == NULL,
                         "utf8 refuses U+110000");
    d_tests_emoji_expect(strcmp(kept, D_EMOJI_FACE_GRINNING) == 0,
                         "a refused utf8 call leaves the last result intact");

    return;
}

/*
d_tests_emoji_seq
  d_text_emoji_utf8_seq and D_EMOJI_SEQ
  Tests the following:
  - ZWJ sequences match the header's literals
  - the longest RGI sequence, 35 bytes, fits
  - an empty sequence gives "", with or without an array
  - a NULL array with a nonzero length, and a surrogate inside a sequence,
    are refused and leave the previous result intact
  - exactly D_EMOJI_SEQ_CAPACITY - 1 bytes fit, and one more is refused
  - the two functions keep separate buffers
*/
static void
d_tests_emoji_seq(void)
{
    d_tests_emoji_expect(strcmp(D_EMOJI_SEQ(0x1F9D1u, 0x200Du, 0x1F4BBu),
                                D_EMOJI_PROFESSION_TECHNOLOGIST) == 0,
                         "seq matches D_EMOJI_PROFESSION_TECHNOLOGIST");
    d_tests_emoji_expect(strcmp(D_EMOJI_SEQ(0x1F9D1u,
                                            0x200Du,
                                            0x2695u,
                                            0xFE0Fu),
                                D_EMOJI_PROFESSION_HEALTH_WORKER) == 0,
                         "seq matches D_EMOJI_PROFESSION_HEALTH_WORKER");

    // kiss: woman, man, light and medium-light skin tones
    const char* kiss = D_EMOJI_SEQ(0x1F469u, 0x1F3FBu, 0x200Du, 0x2764u,
                                   0xFE0Fu,  0x200Du,  0x1F48Bu, 0x200Du,
                                   0x1F468u, 0x1F3FCu);

    d_tests_emoji_expect( ( (kiss != NULL) &&
                            (strlen(kiss) == 35) ),
                          "the 35-byte kiss sequence fits");

    const char* empty = d_text_emoji_utf8_seq(NULL, 0);

    d_tests_emoji_expect( ( (empty != NULL) &&
                            (empty[0] == '\0') ),
                          "an empty sequence gives \"\"");

    const uint32_t bad[] = { 0x1F600u, 0xD83Du, 0x1F600u };
    const char*    kept  = D_EMOJI_SEQ(0x1F600u, 0x1F600u);

    d_tests_emoji_expect(d_text_emoji_utf8_seq(NULL, 1) == NULL,
                         "seq refuses a NULL array with a length");
    d_tests_emoji_expect(d_text_emoji_utf8_seq(bad, 3) == NULL,
                         "seq refuses a surrogate inside a sequence");
    d_tests_emoji_expect(strcmp(kept, "\xF0\x9F\x98\x80\xF0\x9F\x98\x80") == 0,
                         "a refused seq call leaves the last result intact");

    uint32_t fill[D_EMOJI_SEQ_CAPACITY];

    // one-byte code points, to reach the capacity exactly
    for (size_t i = 0; i < D_EMOJI_SEQ_CAPACITY; ++i)
    {
        fill[i] = 0x41u;
    }

    const char* full = d_text_emoji_utf8_seq(fill,
                                             D_EMOJI_SEQ_CAPACITY - 1);

    d_tests_emoji_expect( ( (full != NULL) &&
                            (strlen(full) == D_EMOJI_SEQ_CAPACITY - 1) ),
                          "D_EMOJI_SEQ_CAPACITY - 1 bytes fit");
    d_tests_emoji_expect(d_text_emoji_utf8_seq(fill,
                                               D_EMOJI_SEQ_CAPACITY) == NULL,
                         "D_EMOJI_SEQ_CAPACITY bytes are refused");

    const char* single = d_text_emoji_utf8(0x1F600u);
    const char* pair   = D_EMOJI_SEQ(0x41u, 0x42u);

    d_tests_emoji_expect( ( (strcmp(single, D_EMOJI_FACE_GRINNING) == 0) &&
                            (strcmp(pair, "AB") == 0) ),
                          "utf8 and seq keep separate buffers");

    return;
}

#endif  // D_THREAD_LOCAL_AVAILABLE


int
main(void)
{
    d_tests_emoji_encode();
#if D_THREAD_LOCAL_AVAILABLE
    d_tests_emoji_utf8();
    d_tests_emoji_seq();
#else
    printf("no thread-local storage: utf8 and seq are not built\n");
#endif  // D_THREAD_LOCAL_AVAILABLE

    printf("t_emoji: %d checks, %d failed\n",
           g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
