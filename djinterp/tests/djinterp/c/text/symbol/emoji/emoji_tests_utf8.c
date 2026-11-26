/*******************************************************************************
* djinterp [c][text]                                          emoji_tests_utf8.c
*
* The emoji suite: its one block. d_text_emoji_utf8 and d_text_emoji_utf8_seq
* write into a thread-local buffer each, so where the build has no
* thread-local storage they are not built, and their tests are skipped.
*
*
* path:      /tests/djinterp/c/text/symbol/emoji/emoji_tests_utf8.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./emoji_tests.h"  // corresponding header

// std
#include <string.h>  // memcmp, strcmp, strlen
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // UINT32_MAX


// dj_tests_emoji_case
//   struct: one code point and its UTF-8; a length of 0 means the encoder
// must refuse it.
struct dj_tests_emoji_case
{
    const char* name;    // the check's name
    uint32_t    cp;      // the code point
    const char* bytes;   // its encoding
    size_t      length;  // bytes in the encoding, or 0 when refused
};

// D_TESTS_EMOJI_NO_TLS
//   constant: why utf8 and seq are skipped without thread-local storage.
#define D_TESTS_EMOJI_NO_TLS                                                  \
    "the build has no thread-local storage, so d_text_emoji_utf8 and "        \
    "d_text_emoji_utf8_seq are not built"


/*
d_tests_emoji_encode
  d_text_emoji_utf8_encode.
  Tests the following:
  - the first and last code point of each UTF-8 length, and one emoji
  - surrogates at both ends of their range, U+110000 and UINT32_MAX, each
    refused with nothing written
  - a NULL output buffer, refused
*/
bool
d_tests_emoji_encode(void)
{
    static const struct dj_tests_emoji_case cases[] =
    {
        { "U+0000",       0x000000u, "\x00",             1u },
        { "U+007F",       0x00007Fu, "\x7F",             1u },
        { "U+0080",       0x000080u, "\xC2\x80",         2u },
        { "U+07FF",       0x0007FFu, "\xDF\xBF",         2u },
        { "U+0800",       0x000800u, "\xE0\xA0\x80",     3u },
        { "U+D7FF",       0x00D7FFu, "\xED\x9F\xBF",     3u },
        { "U+E000",       0x00E000u, "\xEE\x80\x80",     3u },
        { "U+FFFF",       0x00FFFFu, "\xEF\xBF\xBF",     3u },
        { "U+10000",      0x010000u, "\xF0\x90\x80\x80", 4u },
        { "U+1F600",      0x01F600u, "\xF0\x9F\x98\x80", 4u },
        { "U+10FFFF",     0x10FFFFu, "\xF4\x8F\xBF\xBF", 4u },
        { "U+D800",       0x00D800u, "",                 0u },
        { "U+DFFF",       0x00DFFFu, "",                 0u },
        { "U+110000",     0x110000u, "",                 0u },
        { "UINT32_MAX",   UINT32_MAX, "",                0u }
    };
    bool   ok = true;
    size_t i;

    for (i = 0u; i < (sizeof(cases) / sizeof(cases[0])); ++i)
    {
        // a buffer that shows any stray write
        char         out[4] = { 'x', 'x', 'x', 'x' };
        const size_t n      = d_text_emoji_utf8_encode(cases[i].cp, out);
        const bool   right  =
            (cases[i].length == 0u)
                ? ( (n == 0u) && (memcmp(out, "xxxx", 4u) == 0) )
                : ( (n == cases[i].length) &&
                    (memcmp(out, cases[i].bytes, n) == 0) );

        ok = d_test_suite_require(right,
                                  cases[i].name,
                                  __FILE__,
                                  __LINE__) && ok;
    }

    ok = D_TEST_EXPECT(d_text_emoji_utf8_encode(0x41u, NULL) == 0u) && ok;

    return ok;
}

/*
d_tests_emoji_utf8
  d_text_emoji_utf8 and D_EMOJI_UTF8.
  Tests the following:
  - an emoji matches the header's literal, through the function and the
    macro
  - U+0000 gives the empty string
  - a surrogate and U+110000 are refused, and the last result stays as it was
*/
bool
d_tests_emoji_utf8(void)
{
#if D_THREAD_LOCAL_AVAILABLE
    const char* grinning = d_text_emoji_utf8(0x1F600u);
    const char* nul;
    const char* kept;
    bool        ok = true;

    ok = D_TEST_EXPECT( (grinning != NULL) &&
                        (strcmp(grinning, D_EMOJI_FACE_GRINNING) == 0) ) &&
         ok;
    ok = D_TEST_EXPECT(strcmp(D_EMOJI_UTF8(0x1F4BB),
                              D_EMOJI_OBJECT_LAPTOP_COMPUTER) == 0) && ok;

    nul = d_text_emoji_utf8(0x0u);
    ok  = D_TEST_EXPECT( (nul != NULL) && (nul[0] == '\0') ) && ok;

    kept = d_text_emoji_utf8(0x1F600u);
    ok   = D_TEST_EXPECT(d_text_emoji_utf8(0xDC00u) == NULL) && ok;
    ok   = D_TEST_EXPECT(d_text_emoji_utf8(0x110000u) == NULL) && ok;
    ok   = D_TEST_EXPECT(strcmp(kept, D_EMOJI_FACE_GRINNING) == 0) && ok;

    return ok;
#else
    D_TEST_SKIP(D_TESTS_EMOJI_NO_TLS);
#endif  // D_THREAD_LOCAL_AVAILABLE
}

/*
d_tests_emoji_seq
  d_text_emoji_utf8_seq and D_EMOJI_SEQ.
  Tests the following:
  - two ZWJ sequences match the header's literals
  - the longest RGI sequence, 35 bytes, fits
  - an empty sequence gives "", with no array
  - a NULL array with a length, and a surrogate inside a sequence, are
    refused, and the last result stays as it was
*/
bool
d_tests_emoji_seq(void)
{
#if D_THREAD_LOCAL_AVAILABLE
    static const uint32_t bad[] = { 0x1F600u, 0xD83Du, 0x1F600u };
    const char*           kiss;
    const char*           empty;
    const char*           kept;
    bool                  ok = true;

    ok = D_TEST_EXPECT(strcmp(D_EMOJI_SEQ(0x1F9D1u, 0x200Du, 0x1F4BBu),
                              D_EMOJI_PROFESSION_TECHNOLOGIST) == 0) && ok;
    ok = D_TEST_EXPECT(strcmp(D_EMOJI_SEQ(0x1F9D1u, 0x200Du, 0x2695u,
                                          0xFE0Fu),
                              D_EMOJI_PROFESSION_HEALTH_WORKER) == 0) && ok;

    // kiss: woman, man, light and medium-light skin tones
    kiss = D_EMOJI_SEQ(0x1F469u, 0x1F3FBu, 0x200Du,  0x2764u, 0xFE0Fu,
                       0x200Du,  0x1F48Bu, 0x200Du,  0x1F468u, 0x1F3FCu);
    ok   = D_TEST_EXPECT( (kiss != NULL) && (strlen(kiss) == 35u) ) && ok;

    empty = d_text_emoji_utf8_seq(NULL, 0u);
    ok    = D_TEST_EXPECT( (empty != NULL) && (empty[0] == '\0') ) && ok;

    kept = D_EMOJI_SEQ(0x1F600u, 0x1F600u);
    ok   = D_TEST_EXPECT(d_text_emoji_utf8_seq(NULL, 1u) == NULL) && ok;
    ok   = D_TEST_EXPECT(d_text_emoji_utf8_seq(bad, 3u) == NULL) && ok;
    ok   = D_TEST_EXPECT(strcmp(kept,
                                "\xF0\x9F\x98\x80\xF0\x9F\x98\x80") == 0) &&
           ok;

    return ok;
#else
    D_TEST_SKIP(D_TESTS_EMOJI_NO_TLS);
#endif  // D_THREAD_LOCAL_AVAILABLE
}

/*
d_tests_emoji_capacity
  D_EMOJI_SEQ_CAPACITY, and the two buffers.
  Tests the following:
  - D_EMOJI_SEQ_CAPACITY - 1 one-byte code points fit, and one more is
    refused
  - d_text_emoji_utf8 and d_text_emoji_utf8_seq keep separate buffers
*/
bool
d_tests_emoji_capacity(void)
{
#if D_THREAD_LOCAL_AVAILABLE
    uint32_t    fill[D_EMOJI_SEQ_CAPACITY];
    const char* full;
    const char* single;
    const char* pair;
    size_t      i;
    bool        ok = true;

    for (i = 0u; i < D_EMOJI_SEQ_CAPACITY; ++i)
    {
        fill[i] = 0x41u;
    }

    full = d_text_emoji_utf8_seq(fill, D_EMOJI_SEQ_CAPACITY - 1u);
    ok   = D_TEST_EXPECT( (full != NULL) &&
                          (strlen(full) == D_EMOJI_SEQ_CAPACITY - 1u) ) &&
           ok;
    ok   = D_TEST_EXPECT(d_text_emoji_utf8_seq(fill,
                                               D_EMOJI_SEQ_CAPACITY) ==
                         NULL) && ok;

    single = d_text_emoji_utf8(0x1F600u);
    pair   = D_EMOJI_SEQ(0x41u, 0x42u);
    ok     = D_TEST_EXPECT(strcmp(single, D_EMOJI_FACE_GRINNING) == 0) && ok;
    ok     = D_TEST_EXPECT(strcmp(pair, "AB") == 0) && ok;

    return ok;
#else
    D_TEST_SKIP(D_TESTS_EMOJI_NO_TLS);
#endif  // D_THREAD_LOCAL_AVAILABLE
}
