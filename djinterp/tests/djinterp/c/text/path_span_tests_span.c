/*******************************************************************************
* djinterp [c][text]                                      path_span_tests_span.c
*
* The path_span suite: every block. Four grammars, written here as a caller
* would write them, and helpers that render a span or a join and compare the
* text and the length returned in one check.
*
*
* path:      /tests/djinterp/c/text/path_span_tests_span.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./path_span_tests.h"  // corresponding header

// std
#include <string.h>  // strcmp, strlen, strncmp


// D_TESTS_PATH_SPAN_ROOM
//   constant: a rendering's buffer, larger than any path here.
#define D_TESTS_PATH_SPAN_ROOM 256u


/*
d_tests_path_span_is_drive
  Whether _c is an ASCII letter, as a drive is named.
*/
static bool
d_tests_path_span_is_drive(
    char _c
)
{
    const char lower = (char)(_c | 0x20);

    return ( (lower >= 'a') && (lower <= 'z') );
}

/*
d_tests_path_span_file_anchor
  A Windows filesystem's anchor: a drive and a separator ("C:\"), or the
leading separators.
*/
static size_t
d_tests_path_span_file_anchor(
    const char*                   _text,
    size_t                        _length,
    const struct dj_path_grammar* _grammar
)
{
    size_t anchor = 0u;

    (void)_grammar;

    if ( (_length >= 3u)                                  &&
         (d_tests_path_span_is_drive(_text[0]))           &&
         (_text[1] == ':')                                &&
         ( (_text[2] == '/') || (_text[2] == '\\') ) )
    {
        return 3u;
    }

    while ( (anchor < _length) &&
            ( (_text[anchor] == '/') || (_text[anchor] == '\\') ) )
    {
        ++anchor;
    }

    return anchor;
}

/*
d_tests_path_span_url_anchor
  A URL's anchor: a scheme, "://" and the authority up to the path's first
"/"; or a leading "/" alone.
*/
static size_t
d_tests_path_span_url_anchor(
    const char*                   _text,
    size_t                        _length,
    const struct dj_path_grammar* _grammar
)
{
    size_t i = 0u;

    (void)_grammar;

    // the scheme runs to "://", unless a "/" comes first
    while ( (i + 2u < _length) &&
            (_text[i] != '/')  &&
            !( (_text[i] == ':')      &&
               (_text[i + 1u] == '/') &&
               (_text[i + 2u] == '/') ) )
    {
        ++i;
    }

    if ( (i + 2u < _length)      &&
         (_text[i] == ':')       &&
         (_text[i + 1u] == '/')  &&
         (_text[i + 2u] == '/') )
    {
        for (i += 3u; (i < _length) && (_text[i] != '/'); ++i)
        {
        }

        return i;
    }

    return ( (_length > 0u) && (_text[0] == '/') ) ? 1u : 0u;
}

// the four grammars, as a caller would write them
static const struct dj_path_grammar d_tests_path_span_posix =
    { "/", '/', NULL, NULL, D_PATH_GRAMMAR_DOT_LABELS };
static const struct dj_path_grammar d_tests_path_span_windows_grammar =
    { "\\/", '\\', NULL, &d_tests_path_span_file_anchor,
      D_PATH_GRAMMAR_DOT_LABELS | D_PATH_GRAMMAR_FOLD_CASE };
static const struct dj_path_grammar d_tests_path_span_url_grammar =
    { "/", '/', "?#", &d_tests_path_span_url_anchor,
      D_PATH_GRAMMAR_DOT_LABELS };
static const struct dj_path_grammar d_tests_path_span_tree_grammar =
    { ".", '.', NULL, NULL, 0u };

/*
d_tests_path_span_of
  The span of _text, read to its terminator through _grammar.
*/
static struct dj_path_span
d_tests_path_span_of(
    const char*                   _text,
    const struct dj_path_grammar* _grammar
)
{
    return d_path_span_of(_text, D_PATH_SPAN_TERMINATED, _grammar);
}

/*
d_tests_path_span_renders
  Records whether _span renders as _expected, with that length returned.
*/
static bool
d_tests_path_span_renders(
    const struct dj_path_span* _span,
    const char*                _expected
)
{
    char         text[D_TESTS_PATH_SPAN_ROOM];
    const size_t length = d_path_span_render(_span, text, sizeof(text));

    return d_test_suite_check_text_at(
               NULL,
               _expected,
               d_test_text_buffer(text,
                                  (length < sizeof(text)) ? length : 0u),
               d_test_text_cstr(_expected),
               __FILE__,
               __LINE__);
}

/*
d_tests_path_span_joins
  Records whether _base joined with _reference renders as _expected, with
that length returned.
*/
static bool
d_tests_path_span_joins(
    const struct dj_path_span* _base,
    const struct dj_path_span* _reference,
    const char*                _expected
)
{
    char         text[D_TESTS_PATH_SPAN_ROOM];
    const size_t length = d_path_span_render_join(_base,
                                                  _reference,
                                                  text,
                                                  sizeof(text));

    return d_test_suite_check_text_at(
               NULL,
               _expected,
               d_test_text_buffer(text,
                                  (length < sizeof(text)) ? length : 0u),
               d_test_text_cstr(_expected),
               __FILE__,
               __LINE__);
}

/*
d_tests_path_span_label_is
  Whether _label is the text _expected.
*/
static bool
d_tests_path_span_label_is(
    struct dj_path_label _label,
    const char*          _expected
)
{
    return ( (_label.length == strlen(_expected)) &&
             (strncmp(_label.text, _expected, _label.length) == 0) );
}


/*
d_tests_path_span_read
  Reading a POSIX path.
  Tests the following:
  - "/" anchors it, and it has six labels, the dot labels among them
  - next gives the labels in order; a run of separators frames one boundary
  - the last label
*/
bool
d_tests_path_span_read(void)
{
    struct dj_path_span  p      = d_tests_path_span_of(
                                      "/usr//local/./lib/../bin",
                                      &d_tests_path_span_posix);
    struct dj_path_label label;
    size_t               cursor = 0u;
    bool                 ok     = true;

    ok = D_TEST_EXPECT(d_path_span_is_anchored(&p)) && ok;
    ok = D_TEST_EXPECT(d_tests_path_span_label_is(d_path_span_anchor(&p),
                                                  "/")) && ok;
    ok = D_TEST_EXPECT(d_path_span_level(&p) == 6u) && ok;
    ok = D_TEST_EXPECT(d_path_span_next(&p, &cursor, &label) &&
                       d_tests_path_span_label_is(label, "usr")) && ok;
    ok = D_TEST_EXPECT(d_path_span_next(&p, &cursor, &label) &&
                       d_tests_path_span_label_is(label, "local")) && ok;
    ok = D_TEST_EXPECT(d_tests_path_span_label_is(d_path_span_last(&p),
                                                  "bin")) && ok;

    return ok;
}

/*
d_tests_path_span_fold
  Folding dot labels.
  Tests the following:
  - a rendering folds "." and "..", and the parent drops the last label
  - an explicit length reads only that much of the text
  - a relative path keeps a ".." it cannot fold
  - an anchored path cannot ascend above its anchor
*/
bool
d_tests_path_span_fold(void)
{
    struct dj_path_span p = d_tests_path_span_of("/usr//local/./lib/../bin",
                                                 &d_tests_path_span_posix);
    struct dj_path_span q = d_path_span_parent(&p);
    bool                ok = true;

    ok = d_tests_path_span_renders(&p, "/usr/local/bin") && ok;
    ok = d_tests_path_span_renders(&q, "/usr/local") && ok;

    q  = d_path_span_of("/a/b/c", 4u, &d_tests_path_span_posix);
    ok = D_TEST_EXPECT(d_path_span_level(&q) == 2u) && ok;
    ok = d_tests_path_span_renders(&q, "/a/b") && ok;

    q  = d_tests_path_span_of("a/../../b", &d_tests_path_span_posix);
    ok = d_tests_path_span_renders(&q, "../b") && ok;

    q  = d_tests_path_span_of("/a/../../b", &d_tests_path_span_posix);
    ok = d_tests_path_span_renders(&q, "/b") && ok;

    return ok;
}

/*
d_tests_path_span_order
  Order.
  Tests the following:
  - the meet of two paths is how many labels they share
  - an anchored path and a relative one have no meet
  - a prefix counts labels, not bytes
*/
bool
d_tests_path_span_order(void)
{
    struct dj_path_span p = d_tests_path_span_of("/usr//local/./lib/../bin",
                                                 &d_tests_path_span_posix);
    struct dj_path_span q = d_tests_path_span_of("/usr/local/lib",
                                                 &d_tests_path_span_posix);
    struct dj_path_span r = d_tests_path_span_of("usr/local",
                                                 &d_tests_path_span_posix);
    bool                ok = true;

    ok = D_TEST_EXPECT(d_path_span_meet(&p, &q) == 2u) && ok;
    ok = D_TEST_EXPECT(d_path_span_meet(&q, &r) == D_PATH_SPAN_NO_MEET) &&
         ok;

    r  = d_tests_path_span_of("/usr/local", &d_tests_path_span_posix);
    ok = D_TEST_EXPECT(d_path_span_starts_with(&q, &r)) && ok;

    r  = d_tests_path_span_of("/usr/loc", &d_tests_path_span_posix);
    ok = D_TEST_EXPECT(!d_path_span_starts_with(&q, &r)) && ok;

    return ok;
}

/*
d_tests_path_span_join
  Joins.
  Tests the following:
  - a ".." in the reference folds a label of the base
  - an anchored reference stands alone
  - two relative paths join with the grammar's separator
*/
bool
d_tests_path_span_join(void)
{
    struct dj_path_span base = d_tests_path_span_of("/a/b",
                                                    &d_tests_path_span_posix);
    struct dj_path_span ref  = d_tests_path_span_of("../c",
                                                    &d_tests_path_span_posix);
    bool                ok   = true;

    ok = d_tests_path_span_joins(&base, &ref, "/a/c") && ok;

    ref = d_tests_path_span_of("/x", &d_tests_path_span_posix);
    ok  = d_tests_path_span_joins(&base, &ref, "/x") && ok;

    base = d_tests_path_span_of("a", &d_tests_path_span_posix);
    ref  = d_tests_path_span_of("b", &d_tests_path_span_posix);
    ok   = d_tests_path_span_joins(&base, &ref, "a/b") && ok;

    return ok;
}

/*
d_tests_path_span_render
  Rendering into a buffer, as snprintf does.
  Tests the following:
  - a buffer too short gets what fits, terminated, and the length needed
  - no buffer measures only
*/
bool
d_tests_path_span_render(void)
{
    struct dj_path_span p = d_tests_path_span_of("/usr//local/./lib/../bin",
                                                 &d_tests_path_span_posix);
    char                eight[8];
    bool                ok = true;

    ok = D_TEST_EXPECT(d_path_span_render(&p, eight, sizeof(eight)) ==
                       14u) && ok;
    ok = D_TEST_EXPECT(strcmp(eight, "/usr/lo") == 0) && ok;
    ok = D_TEST_EXPECT(d_path_span_render(&p, NULL, 0u) == 14u) && ok;

    return ok;
}

/*
d_tests_path_span_windows
  A Windows filesystem's grammar.
  Tests the following:
  - a drive and a separator anchor the path
  - case and either separator are folded, the anchors' too, in a meet
  - a rendering keeps the anchor as read and frames the labels with the
    grammar's own separator
*/
bool
d_tests_path_span_windows(void)
{
    struct dj_path_span q = d_tests_path_span_of(
                                "C:\\Users\\Me\\x",
                                &d_tests_path_span_windows_grammar);
    struct dj_path_span r = d_tests_path_span_of(
                                "c:/users/me/y",
                                &d_tests_path_span_windows_grammar);
    bool                ok = true;

    ok = D_TEST_EXPECT(d_tests_path_span_label_is(d_path_span_anchor(&q),
                                                  "C:\\")) && ok;
    ok = D_TEST_EXPECT(d_path_span_level(&q) == 3u) && ok;
    ok = D_TEST_EXPECT(d_path_span_meet(&q, &r) == 2u) && ok;
    ok = d_tests_path_span_renders(&r, "c:/users\\me\\y") && ok;

    return ok;
}

/*
d_tests_path_span_url
  A URL's grammar.
  Tests the following:
  - the scheme and the authority anchor it
  - the labels run to the first terminator, and the rest is after it
  - a rendering folds the dot labels after the authority
  - a reference is taken from a base, and the authority alone is an ancestor
*/
bool
d_tests_path_span_url(void)
{
    struct dj_path_span q = d_tests_path_span_of(
                                "https://example.com/a/b/../c?q=1#f",
                                &d_tests_path_span_url_grammar);
    struct dj_path_span r = d_tests_path_span_of(
                                "../d", &d_tests_path_span_url_grammar);
    bool                ok = true;

    ok = D_TEST_EXPECT(d_tests_path_span_label_is(d_path_span_anchor(&q),
                                                  "https://example.com")) &&
         ok;
    ok = D_TEST_EXPECT(d_path_span_level(&q) == 4u) && ok;
    ok = D_TEST_EXPECT(d_tests_path_span_label_is(d_path_span_last(&q),
                                                  "c")) && ok;
    ok = D_TEST_EXPECT(d_tests_path_span_label_is(d_path_span_rest(&q),
                                                  "?q=1#f")) && ok;
    ok = d_tests_path_span_renders(&q, "https://example.com/a/c") && ok;
    ok = d_tests_path_span_joins(&q, &r, "https://example.com/a/d") && ok;

    r  = d_tests_path_span_of("https://example.com",
                              &d_tests_path_span_url_grammar);
    ok = D_TEST_EXPECT(d_path_span_level(&r) == 0u) && ok;
    ok = D_TEST_EXPECT(d_path_span_starts_with(&q, &r)) && ok;

    return ok;
}

/*
d_tests_path_span_tree
  An n-ary tree's grammar, its labels child indices.
  Tests the following:
  - three steps, the last of them "1"
  - siblings meet at their parent, and the parent renders
  - a rooted path and a relative one have no meet
*/
bool
d_tests_path_span_tree(void)
{
    struct dj_path_span q = d_tests_path_span_of(
                                "0.3.1", &d_tests_path_span_tree_grammar);
    struct dj_path_span r = d_tests_path_span_of(
                                "0.3.2", &d_tests_path_span_tree_grammar);
    struct dj_path_span parent;
    bool                ok = true;

    ok = D_TEST_EXPECT(d_path_span_level(&q) == 3u) && ok;
    ok = D_TEST_EXPECT(d_tests_path_span_label_is(d_path_span_last(&q),
                                                  "1")) && ok;
    ok = D_TEST_EXPECT(d_path_span_meet(&q, &r) == 2u) && ok;

    parent = d_path_span_parent(&q);
    ok     = d_tests_path_span_renders(&parent, "0.3") && ok;

    r  = d_tests_path_span_of(".0.3", &d_tests_path_span_tree_grammar);
    ok = D_TEST_EXPECT(d_path_span_meet(&q, &r) == D_PATH_SPAN_NO_MEET) &&
         ok;

    return ok;
}

/*
d_tests_path_span_empty
  Nothing.
  Tests the following:
  - NULL text: no labels, no anchor, and an empty rendering
  - a NULL span: level 0, and a rendering that writes only the terminator
*/
bool
d_tests_path_span_empty(void)
{
    struct dj_path_span e = d_path_span_of(NULL, 0u,
                                           &d_tests_path_span_posix);
    char                text[8] = { 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x' };
    bool                ok      = true;

    ok = D_TEST_EXPECT(d_path_span_level(&e) == 0u) && ok;
    ok = D_TEST_EXPECT(!d_path_span_is_anchored(&e)) && ok;
    ok = d_tests_path_span_renders(&e, "") && ok;
    ok = D_TEST_EXPECT(d_path_span_level(NULL) == 0u) && ok;
    ok = D_TEST_EXPECT(d_path_span_render(NULL, text, sizeof(text)) == 0u) &&
         ok;
    ok = D_TEST_EXPECT(text[0] == '\0') && ok;

    return ok;
}
