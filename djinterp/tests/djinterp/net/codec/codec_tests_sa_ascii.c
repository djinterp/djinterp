/*******************************************************************************
* djinterp [net]                                          codec_tests_sa_ascii.c
*
* Standalone tests of codec_ascii.h: every character's classes and folding,
* checked against the ASCII table itself, and comparison and trimming of
* spans, refusals included.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa_ascii.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./codec_tests_sa.h"  // the suite
// std
#include <string.h>  // memchr, memcmp, strlen
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text


/*
d_tests_codec_text
  A span over a NUL-terminated string.
*/
static struct d_pack_text
d_tests_codec_text(
    const char* _text
)
{
    const struct d_pack_text text = { _text,
                                      strlen(_text) };

    return text;
}

/*
d_tests_codec_class_holds
  Whether the classes and the folding of the character with code `_code`, 0
to 255, are what the ASCII table says: each class decided here by listing,
not by range.
*/
static bool
d_tests_codec_class_holds(
    int _code
)
{
    static const char DIGITS[] = "0123456789";
    static const char UPPER[]  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const char LOWER[]  = "abcdefghijklmnopqrstuvwxyz";
    const char        c        = (char)(unsigned char)_code;
    const bool        digit    = ( (_code != 0) &&
                                   (memchr(DIGITS,
                                           _code,
                                           10) != NULL) );
    const char*       capital  = (_code != 0) ? memchr(UPPER,
                                                       _code,
                                                       26)
                                              : NULL;
    const bool        small    = ( (_code != 0) &&
                                   (memchr(LOWER,
                                           _code,
                                           26) != NULL) );
    const bool        blank    = ( (_code == ' ') ||
                                   (_code == '\t') );
    const char        folded   = (capital != NULL)
                                 ? LOWER[capital - UPPER]
                                 : c;

    return ( (d_net_ascii_is_digit(c) == digit)               &&
             (d_net_ascii_is_alpha(c) == ( (capital != NULL) ||
                                           (small) ))         &&
             (d_net_ascii_is_alnum(c) == ( (digit)           ||
                                           (capital != NULL) ||
                                           (small) ))         &&
             (d_net_ascii_is_blank(c) == blank)               &&
             (d_net_ascii_is_space(c) == ( (blank)         ||
                                           (_code == '\r') ||
                                           (_code == '\n') )) &&
             (d_net_ascii_lower(c) == folded) );
}

/*
d_tests_sa_codec_ascii_classes
  Tests the following:
  - for every octet, 0 to 255: digit, letter, letter-or-digit, blank and
    space as the ASCII table has them, and folding only of the 26 capitals;
    octets past 0x7F in no class, whatever the sign of char
*/
bool
d_tests_sa_codec_ascii_classes(
    struct d_test_counter* _counter
)
{
    const char* const TEST = "ASCII classes";
    bool              all  = true;

    // every octet
    for (int code = 0; code < 256; ++code)
    {
        all = d_tests_codec_class_holds(code) && all;
    }

    return d_tests_net_assert(all,
                              TEST,
                              "every octet's classes and folding",
                              _counter);
}

/*
d_tests_sa_codec_ascii_compare
  Tests the following:
  - spans compare equal ignoring ASCII case, and only ASCII case
  - a span equals a string only when both end together; a NUL inside the
    span never matches
  - a span begins with a prefix when every character matches before it ends
  - a span NULL but not empty, or a NULL string, matches nothing
*/
bool
d_tests_sa_codec_ascii_compare(
    struct d_test_counter* _counter
)
{
    const char* const        TEST     = "ASCII comparison";
    const struct d_pack_text mixed    = d_tests_codec_text("Content-Type");
    const struct d_pack_text broken   = { NULL,
                                          3 };
    const struct d_pack_text empty    = { NULL,
                                          0 };
    const struct d_pack_text with_nul = { "AB\0",
                                          3 };
    bool                     result   = true;

    result = d_tests_net_assert(
                 ( (d_net_ascii_ieq(mixed,
                                    d_tests_codec_text("cONTENT-tYPE")))  &&
                   (!d_net_ascii_ieq(mixed,
                                     d_tests_codec_text("Content_Type"))) &&
                   (!d_net_ascii_ieq(d_tests_codec_text("\xc0"),
                                     d_tests_codec_text("\xe0")))         &&
                   (d_net_ascii_ieq(empty,
                                    d_tests_codec_text(""))) ),
                 TEST,
                 "spans: ASCII case, and only it, is ignored",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_net_ascii_ieq_cstr(mixed,
                                         "CONTENT-TYPE"))   &&
                   (!d_net_ascii_ieq_cstr(mixed,
                                          "Content-Typ"))   &&
                   (!d_net_ascii_ieq_cstr(mixed,
                                          "Content-Types")) &&
                   (!d_net_ascii_ieq_cstr(with_nul,
                                          "ab"))            &&
                   (d_net_ascii_ieq_cstr(empty,
                                         "")) ),
                 TEST,
                 "a span and a string end together",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_net_ascii_istarts(mixed,
                                        "content-"))       &&
                   (d_net_ascii_istarts(mixed,
                                        ""))               &&
                   (!d_net_ascii_istarts(mixed,
                                         "Content-Type-")) &&
                   (!d_net_ascii_istarts(mixed,
                                         "Context")) ),
                 TEST,
                 "a prefix matches before the span ends",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (!d_net_ascii_ieq(broken,
                                     broken))     &&
                   (!d_net_ascii_ieq_cstr(broken,
                                          "abc")) &&
                   (!d_net_ascii_ieq_cstr(mixed,
                                          NULL))  &&
                   (!d_net_ascii_istarts(broken,
                                         ""))     &&
                   (!d_net_ascii_istarts(mixed,
                                         NULL)) ),
                 TEST,
                 "a broken span or a NULL string matches nothing",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_trims_to
  Whether `_trim` narrows `_text` to exactly `_expected`, as a part of it.
*/
static bool
d_tests_codec_trims_to(
    struct d_pack_text (*_trim)(struct d_pack_text _text),
    const char*        _text,
    const char*        _expected
)
{
    const struct d_pack_text text    = d_tests_codec_text(_text);
    const struct d_pack_text trimmed = _trim(text);

    return ( (trimmed.length == strlen(_expected)) &&
             (trimmed.data >= text.data)           &&
             (trimmed.data + trimmed.length <=
              text.data + text.length)             &&
             (memcmp(trimmed.data,
                     _expected,
                     trimmed.length) == 0) );
}

/*
d_tests_sa_codec_ascii_trim
  Tests the following:
  - trimming drops blanks from both ends and keeps the inner ones
  - trimming spaces drops line breaks too; neither drops a vertical tab
  - an all-blank span narrows to nothing, and a NULL one comes back as it went
*/
bool
d_tests_sa_codec_ascii_trim(
    struct d_test_counter* _counter
)
{
    const char* const        TEST   = "ASCII trimming";
    const struct d_pack_text none   = { NULL,
                                        0 };
    bool                     result = true;

    result = d_tests_net_assert(
                 ( (d_tests_codec_trims_to(d_net_ascii_trim,
                                           " \tgzip, br \t",
                                           "gzip, br"))      &&
                   (d_tests_codec_trims_to(d_net_ascii_trim,
                                           "\r\nvalue\r\n",
                                           "\r\nvalue\r\n")) &&
                   (d_tests_codec_trims_to(d_net_ascii_trim,
                                           " \t ",
                                           "")) ),
                 TEST,
                 "blanks go from both ends",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_trims_to(d_net_ascii_trim_space,
                                           "\r\n AB:CD \t\r\n",
                                           "AB:CD"))     &&
                   (d_tests_codec_trims_to(d_net_ascii_trim_space,
                                           "\vAB\f",
                                           "\vAB\f"))    &&
                   (d_net_ascii_trim(none).data == NULL) &&
                   (d_net_ascii_trim_space(none).length == 0) ),
                 TEST,
                 "spaces, line breaks among them, go too",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_trims_to_end
  Whether `_trim` narrows `_length` characters at `_text`, every one of them
trimmed, to the empty span at their end.
*/
static bool
d_tests_codec_trims_to_end(
    struct d_pack_text (*_trim)(struct d_pack_text _text),
    const char*        _text,
    size_t             _length
)
{
    const struct d_pack_text text    = { _text,
                                         _length };
    const struct d_pack_text trimmed = _trim(text);

    return ( (trimmed.length == 0) &&
             (trimmed.data == _text + _length) );
}

/*
d_tests_sa_codec_ascii_edges
  Tests the following:
  - a broken span matches nothing on either side of a comparison
  - the first character counts, in each comparison
  - a span ends where its length says, though a NUL is further on
  - a span of nothing but trimmed characters narrows to the empty span at its
    end, even when more of them follow it
*/
bool
d_tests_sa_codec_ascii_edges(
    struct d_test_counter* _counter
)
{
    const char* const        TEST       = "ASCII edges";
    const struct d_pack_text abc        = d_tests_codec_text("abc");
    const struct d_pack_text xbc        = d_tests_codec_text("xbc");
    const struct d_pack_text broken     = { NULL,
                                            3 };
    const struct d_pack_text short_span = { "abcd",
                                            3 };
    bool                     result     = true;

    result = d_tests_net_assert(
                 ( (!d_net_ascii_ieq(broken,
                                     abc))        &&
                   (!d_net_ascii_ieq(abc,
                                     broken))     &&
                   (!d_net_ascii_ieq(abc,
                                     xbc))        &&
                   (!d_net_ascii_ieq_cstr(abc,
                                          "xbc")) &&
                   (!d_net_ascii_istarts(abc,
                                         "xb")) ),
                 TEST,
                 "either side broken, and the first character",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (!d_net_ascii_istarts(short_span,
                                         "abcd")) &&
                   (d_net_ascii_istarts(short_span,
                                        "ABC"))   &&
                   (!d_net_ascii_ieq_cstr(short_span,
                                          "abcd")) ),
                 TEST,
                 "a span ends at its length",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_trims_to_end(d_net_ascii_trim,
                                               " \t ",
                                               3)) &&
                   (d_tests_codec_trims_to_end(d_net_ascii_trim,
                                               "   ",
                                               2)) &&
                   (d_tests_codec_trims_to_end(d_net_ascii_trim_space,
                                               "\r\n \r\n",
                                               4)) ),
                 TEST,
                 "all trimmed: the empty span at the end",
                 _counter) && result;

    return result;
}
