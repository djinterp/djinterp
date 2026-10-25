/*******************************************************************************
* djinterp [tools]                                                   test_dxml.c
*
* Exercises `dxml.h`: the tokens of a slide part in the shape a .story holds,
* names with and without prefixes, attributes as written, the decoding of
* text, and the refusal of markup that is not well formed.
*
* path:      /tests/djinterp/tools/dawk/test_dxml.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dxml.h"  // corresponding header
// std
#include <stdio.h>   // printf
#include <string.h>  // memcmp, strlen


static int g_checks = 0;
static int g_failed = 0;


/*
d_tests_check
  Counts one check, and names it when it fails.
*/
static void
d_tests_check(
    bool        _condition,
    const char* _what
)
{
    g_checks++;

    // a failed check is named, so a green run says what it checked
    if (!_condition)
    {
        g_failed++;
        printf("  FAIL  %s\n", _what);
    }

    return;
}


/*
d_tests_same
  Whether a span holds exactly a string.
*/
static bool
d_tests_same(
    const char* _span,
    size_t      _length,
    const char* _expected
)
{
    const size_t span = strlen(_expected);

    return ( (_span != NULL)    &&
             (_length == span)  &&
             (memcmp(_span, _expected, span) == 0) );
}


/*
d_tests_scan
  Tests the following:
  - the tokens of a part come in document order, each of its kind
  - a token's bounds are its place in the buffer
  - comments, declarations and instructions are skipped as other
  - CDATA is text, taken as it stands
  - the end is reported, and keeps being reported
*/
static void
d_tests_scan(void)
{
    static const char part[] =
        "<?xml version=\"1.0\"?>\n"
        "<!-- made by hand -->"
        "<sld name=\"Intro &amp; Welcome\" xmlns:r='urn:r'>"
        "<media r:id=\"rId3\" md5Checksum=\"ab12\"/>"
        "<p:txt>caf&#233; &lt;b&gt;</p:txt>"
        "<![CDATA[a<b>&amp;]]>"
        "</sld>";

    struct d_xml       xml;
    struct d_xml_token token;
    const char*        value  = NULL;
    size_t             length = 0;

    printf("scanning\n");

    d_xml_begin(&xml, part, sizeof(part) - 1u);

    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_OTHER) &&
                  (token.start == 0u) && (token.end == 21u),
                  "the declaration is other, and spans itself");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_TEXT) &&
                  d_tests_same(token.text, token.text_length, "\n"),
                  "the line break after it is text");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_OTHER),
                  "a comment is other");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_START) &&
                  d_xml_is(&token, "sld") &&
                  d_tests_same(token.name, token.name_length, "sld"),
                  "the root is a start tag named sld");
    d_tests_check(d_xml_attribute(&token, "name", &value, &length) &&
                  d_tests_same(value, length, "Intro &amp; Welcome"),
                  "...whose name is given raw");
    d_tests_check(d_xml_attribute(&token, "xmlns:r", &value, &length) &&
                  d_tests_same(value, length, "urn:r"),
                  "...and a value in single quotes is read");
    d_tests_check(!d_xml_attribute(&token, "nam", &value, &length),
                  "...and a name's prefix is not the name");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_EMPTY) &&
                  d_xml_is(&token, "media"),
                  "an empty-element tag");
    d_tests_check(d_xml_attribute(&token, "r:id", &value, &length) &&
                  d_tests_same(value, length, "rId3"),
                  "...with a prefixed attribute");
    d_tests_check(!d_xml_attribute(&token, "id", &value, &length),
                  "...which its local name does not find");
    d_tests_check(d_xml_attribute(&token, "md5Checksum", &value, &length) &&
                  d_tests_same(value, length, "ab12") &&
                  (part[(size_t)(value - part) + length] == '"'),
                  "...and a value points into the buffer");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_START) &&
                  d_xml_is(&token, "txt") && !d_xml_is(&token, "p:txt") &&
                  !d_xml_is(&token, "tx"),
                  "a prefixed element matches its local name only");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_TEXT) &&
                  (!token.cdata) &&
                  d_tests_same(token.text, token.text_length,
                               "caf&#233; &lt;b&gt;"),
                  "text keeps its references");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_CLOSE) &&
                  d_xml_is(&token, "txt"),
                  "an end tag");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_TEXT) &&
                  (token.cdata) &&
                  d_tests_same(token.text, token.text_length, "a<b>&amp;"),
                  "CDATA is text as it stands");
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_CLOSE) &&
                  d_xml_is(&token, "sld") &&
                  (token.end == (sizeof(part) - 1u)),
                  "the root closes at the buffer's end");
    d_tests_check((!d_xml_next(&xml, &token)) && (token.kind == D_XML_END),
                  "the end is reported");
    d_tests_check((!d_xml_next(&xml, &token)) && (token.kind == D_XML_END),
                  "...and again");

    static const char quoted[] = "<a title=\"x > y\" b='\"'/>";

    d_xml_begin(&xml, quoted, sizeof(quoted) - 1u);
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_EMPTY) &&
                  d_xml_attribute(&token, "title", &value, &length) &&
                  d_tests_same(value, length, "x > y") &&
                  d_xml_attribute(&token, "b", &value, &length) &&
                  d_tests_same(value, length, "\""),
                  "a `>` within quotes does not close the tag");

    static const char bare[] = "<a/>";

    d_xml_begin(&xml, bare, sizeof(bare) - 1u);
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_EMPTY) &&
                  d_tests_same(token.name, token.name_length, "a") &&
                  (token.attributes_length == 0u),
                  "an empty element without attributes");

    return;
}


/*
d_tests_errors
  Tests the following:
  - markup that is not well formed is an error, and stays one
  - an attribute without a quoted value is not read, nor any after it
*/
static void
d_tests_errors(void)
{
    static const char* const broken[] =
    {
        "<",
        "<a",
        "<a b=\"c>",
        "<>",
        "</>",
        "<!-- never closed",
        "<![CDATA[ never closed",
        "<?pi never closed"
    };

    struct d_xml       xml;
    struct d_xml_token token;
    bool               all = true;

    printf("errors\n");

    for (size_t at = 0; at < (sizeof(broken) / sizeof(broken[0])); ++at)
    {
        d_xml_begin(&xml, broken[at], strlen(broken[at]));

        // each is an error at once, and the next call says so again
        all = all &&
              (!d_xml_next(&xml, &token)) && (token.kind == D_XML_ERROR) &&
              (!d_xml_next(&xml, &token)) && (token.kind == D_XML_ERROR);
    }

    d_tests_check(all, "every malformed tag is an error, and stays one");

    static const char text_first[] = "text<";

    d_xml_begin(&xml, text_first, sizeof(text_first) - 1u);
    d_tests_check(d_xml_next(&xml, &token) && (token.kind == D_XML_TEXT) &&
                  (!d_xml_next(&xml, &token)) && (token.kind == D_XML_ERROR),
                  "text before a broken tag is still read");

    static const char unquoted[] = "<a x=1 y=\"2\"/>";
    const char*       value      = NULL;
    size_t            length     = 0;

    d_xml_begin(&xml, unquoted, sizeof(unquoted) - 1u);
    d_tests_check(d_xml_next(&xml, &token) &&
                  (!d_xml_attribute(&token, "x", &value, &length)) &&
                  (!d_xml_attribute(&token, "y", &value, &length)),
                  "an unquoted value ends the walk");

    d_xml_begin(&xml, NULL, 10u);
    d_tests_check((!d_xml_next(&xml, &token)) && (token.kind == D_XML_END),
                  "no buffer is an empty one");

    return;
}


/*
d_tests_decode
  Tests the following:
  - the five predefined entities
  - decimal and hexadecimal references, written as UTF-8
  - what is no reference is copied as it stands
*/
static void
d_tests_decode(void)
{
    char   out[64];
    size_t length = 0;

    printf("decoding\n");

    static const char five[] = "&amp;&lt;&gt;&quot;&apos;";

    length = d_xml_decode(five, sizeof(five) - 1u, out);
    d_tests_check(d_tests_same(out, length, "&<>\"'"),
                  "the five predefined entities");

    static const char numeric[] = "&#65;&#x42;&#xe9;&#x20AC;&#x1F600;";

    length = d_xml_decode(numeric, sizeof(numeric) - 1u, out);
    d_tests_check(d_tests_same(out, length,
                               "AB\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80"),
                  "character references, as UTF-8");

    static const char loose[] =
        "&nbsp; & &#; &#x; &#12 &#xZZ; &#x110000; &#xD800;";

    length = d_xml_decode(loose, sizeof(loose) - 1u, out);
    d_tests_check(d_tests_same(out, length, loose),
                  "what is no reference is copied");

    static const char cut[] = "a&am";

    length = d_xml_decode(cut, sizeof(cut) - 1u, out);
    d_tests_check(d_tests_same(out, length, "a&am"),
                  "a reference cut short is copied");

    return;
}


/*
main
  Runs the checks.
*/
int
main(void)
{
    d_tests_scan();
    d_tests_errors();
    d_tests_decode();

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
