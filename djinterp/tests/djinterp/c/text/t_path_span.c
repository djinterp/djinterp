/* t_path_span.c -- the generic path (c/text/path_span.h), read by three
   grammars a caller might write: a filesystem's, a URL's and an n-ary tree's.
   Self-contained: cc -Iinc tests/djinterp/c/text/t_path_span.c
   src/djinterp/c/text/path_span.c */
#include "../../../../inc/djinterp/c/text/path_span.h"
#include <stdio.h>
#include <string.h>

static int g_checks = 0, g_failed = 0;
#define CHECK(c, what) do { g_checks++; if (!(c)) { g_failed++; printf("FAIL %s (line %d)\n", what, __LINE__); } } while (0)

/* Three grammars a caller might write -- none ships with the type. */
static size_t file_anchor(const char* t, size_t n, const struct d_path_grammar* g)
{
    size_t a = 0;
    (void)g;
    if ((n >= 3) && (((t[0] | 0x20) >= 'a') && ((t[0] | 0x20) <= 'z')) && (t[1] == ':') &&
        ((t[2] == '/') || (t[2] == '\\')))
        return 3;
    while ((a < n) && ((t[a] == '/') || (t[a] == '\\'))) a++;
    return a;
}
static size_t url_anchor(const char* t, size_t n, const struct d_path_grammar* g)
{
    size_t i = 0;
    (void)g;
    while ((i + 2 < n) && !((t[i] == ':') && (t[i + 1] == '/') && (t[i + 2] == '/'))) { if (t[i] == '/') break; i++; }
    if ((i + 2 < n) && (t[i] == ':') && (t[i + 1] == '/') && (t[i + 2] == '/'))
    {
        i += 3;
        while ((i < n) && (t[i] != '/')) i++;
        return i;
    }
    return ((n > 0) && (t[0] == '/')) ? 1 : 0;
}
static const struct d_path_grammar posix_g = { "/", '/', NULL, NULL, D_PATH_GRAMMAR_DOT_LABELS };
static const struct d_path_grammar win_g   = { "\\/", '\\', NULL, file_anchor,
                                               D_PATH_GRAMMAR_DOT_LABELS | D_PATH_GRAMMAR_FOLD_CASE };
static const struct d_path_grammar url_g   = { "/", '/', "?#", url_anchor, D_PATH_GRAMMAR_DOT_LABELS };
static const struct d_path_grammar tree_g  = { ".", '.', NULL, NULL, 0u };

static int renders(const struct d_path_span* s, const char* want)
{
    char buf[256];
    return (d_path_span_render(s, buf, sizeof(buf)) == strlen(want)) && (strcmp(buf, want) == 0);
}
static int joins(const struct d_path_span* a, const struct d_path_span* b, const char* want)
{
    char buf[256];
    return (d_path_span_render_join(a, b, buf, sizeof(buf)) == strlen(want)) && (strcmp(buf, want) == 0);
}
static int label_is(struct d_path_label l, const char* want)
{
    return (l.length == strlen(want)) && (strncmp(l.text, want, l.length) == 0);
}

int main(void)
{
    char buf[8];
    size_t cursor = 0;
    struct d_path_label l;
    struct d_path_span p = d_path_span_of("/usr//local/./lib/../bin", D_PATH_SPAN_TERMINATED, &posix_g);
    struct d_path_span q, r, e;

    /* reading: labels, anchor, level */
    CHECK(d_path_span_is_anchored(&p) && label_is(d_path_span_anchor(&p), "/"), "posix: anchored by /");
    CHECK(d_path_span_level(&p) == 6, "posix: six labels, the dot labels among them");
    CHECK(d_path_span_next(&p, &cursor, &l) && label_is(l, "usr") &&
          d_path_span_next(&p, &cursor, &l) && label_is(l, "local"), "posix: a run of separators frames one boundary");
    CHECK(label_is(d_path_span_last(&p), "bin"), "posix: the last label");
    CHECK(renders(&p, "/usr/local/bin"), "posix: rendered, the dot labels folded");
    q = d_path_span_parent(&p);
    CHECK(renders(&q, "/usr/local"), "posix: the parent");
    q = d_path_span_of("/a/b/c", 4, &posix_g);
    CHECK(d_path_span_level(&q) == 2 && renders(&q, "/a/b"), "posix: an explicit length");
    q = d_path_span_of("a/../../b", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(renders(&q, "../b"), "posix: a relative path keeps a .. it cannot fold");
    q = d_path_span_of("/a/../../b", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(renders(&q, "/b"), "posix: an anchored path cannot ascend above its anchor");

    /* order */
    q = d_path_span_of("/usr/local/lib", D_PATH_SPAN_TERMINATED, &posix_g);
    r = d_path_span_of("usr/local", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(d_path_span_meet(&p, &q) == 2, "posix: the meet of two paths");
    CHECK(d_path_span_meet(&q, &r) == D_PATH_SPAN_NO_MEET, "posix: anchored and relative have no meet");
    r = d_path_span_of("/usr/local", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(d_path_span_starts_with(&q, &r), "posix: a prefix");
    r = d_path_span_of("/usr/loc", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(!d_path_span_starts_with(&q, &r), "posix: labels, not bytes, are the prefix order");

    /* joins */
    q = d_path_span_of("/a/b", D_PATH_SPAN_TERMINATED, &posix_g);
    r = d_path_span_of("../c", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(joins(&q, &r, "/a/c"), "join: a .. folds a label of the base");
    r = d_path_span_of("/x", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(joins(&q, &r, "/x"), "join: an anchored path stands alone");
    q = d_path_span_of("a", D_PATH_SPAN_TERMINATED, &posix_g);
    r = d_path_span_of("b", D_PATH_SPAN_TERMINATED, &posix_g);
    CHECK(joins(&q, &r, "a/b"), "join: two relative paths");

    /* snprintf's way */
    CHECK(d_path_span_render(&p, buf, sizeof(buf)) == 14 && strcmp(buf, "/usr/lo") == 0, "render: cut short, the length it needed");
    CHECK(d_path_span_render(&p, NULL, 0) == 14, "render: measured only");

    /* a Windows-like grammar: drive anchors, either separator, case folded */
    q = d_path_span_of("C:\\Users\\Me\\x", D_PATH_SPAN_TERMINATED, &win_g);
    r = d_path_span_of("c:/users/me/y", D_PATH_SPAN_TERMINATED, &win_g);
    CHECK(label_is(d_path_span_anchor(&q), "C:\\") && d_path_span_level(&q) == 3, "win: a drive anchors");
    CHECK(d_path_span_meet(&q, &r) == 2, "win: case and separator folded, anchors too");
    CHECK(renders(&r, "c:/users\\me\\y"), "win: the anchor as read, labels framed by the grammar's separator");

    /* a URL's: the path proper ends at ? or #, and the authority anchors */
    q = d_path_span_of("https://example.com/a/b/../c?q=1#f", D_PATH_SPAN_TERMINATED, &url_g);
    CHECK(label_is(d_path_span_anchor(&q), "https://example.com"), "url: scheme and authority anchor");
    CHECK(d_path_span_level(&q) == 4 && label_is(d_path_span_last(&q), "c"), "url: labels to the terminator");
    CHECK(label_is(d_path_span_rest(&q), "?q=1#f"), "url: the rest after the path");
    CHECK(renders(&q, "https://example.com/a/c"), "url: rendered, framed after the authority");
    r = d_path_span_of("../d", D_PATH_SPAN_TERMINATED, &url_g);
    CHECK(joins(&q, &r, "https://example.com/a/d"), "url: a reference taken from a base");
    r = d_path_span_of("https://example.com", D_PATH_SPAN_TERMINATED, &url_g);
    CHECK(d_path_span_level(&r) == 0 && d_path_span_starts_with(&q, &r), "url: the authority alone, an ancestor");

    /* an n-ary tree's: labels are child indices */
    q = d_path_span_of("0.3.1", D_PATH_SPAN_TERMINATED, &tree_g);
    r = d_path_span_of("0.3.2", D_PATH_SPAN_TERMINATED, &tree_g);
    CHECK(d_path_span_level(&q) == 3 && label_is(d_path_span_last(&q), "1"), "tree: three steps");
    CHECK(d_path_span_meet(&q, &r) == 2, "tree: siblings meet at their parent");
    e = d_path_span_parent(&q);
    CHECK(renders(&e, "0.3"), "tree: the parent");
    r = d_path_span_of(".0.3", D_PATH_SPAN_TERMINATED, &tree_g);
    CHECK(d_path_span_meet(&q, &r) == D_PATH_SPAN_NO_MEET, "tree: rooted and relative have no meet");

    /* nothing */
    e = d_path_span_of(NULL, 0, &posix_g);
    CHECK(d_path_span_level(&e) == 0 && !d_path_span_is_anchored(&e) && renders(&e, ""), "empty: NULL text");
    CHECK(d_path_span_level(NULL) == 0 && d_path_span_render(NULL, buf, sizeof(buf)) == 0 && buf[0] == '\0', "empty: NULL span");

    printf("t_path_span: %d checks, %d failed\n", g_checks, g_failed);
    return (g_failed != 0);
}
