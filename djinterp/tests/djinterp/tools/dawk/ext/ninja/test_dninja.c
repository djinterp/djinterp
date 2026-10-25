/*******************************************************************************
* djinterp [tools]                                                 test_dninja.c
*
* Exercises the Ninja extension's three parts: `dninja.h`, a file read as
* ninja's parser reads it, and the writers; `dninja_eval.h`, a build loaded
* as ninja loads one, and what ninja refuses of one, in its words; and
* `dninja_doc.h`, every change a file takes, the layout each takes from the
* file, what each refuses, and saving.
*   With --fuzz, it makes changes at random to every Ninja file under a
* directory instead -- values, names, bindings and paths set, inserted and
* removed, statements inserted, replaced and removed, and raw splices -- and
* proves of each that a change kept left the file readable and every byte
* and statement outside it as it was, and that a change refused left the
* file as it was.
*
*   test_dninja [work directory]          the unit checks
*   test_dninja --fuzz <dir> [seed] [n]   n changes to every file
*
* path:      /tests/djinterp/tools/dawk/ext/ninja/test_dninja.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja.h"       // corresponding header
// std
#include <stdio.h>   // printf, snprintf, remove
#include <stdlib.h>  // malloc, free, strtoul
#include <string.h>  // memcmp, memcpy, memset, strcmp, strlen, strstr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                    // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"                // d_text_read, d_text_write
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja_doc.h"   // d_ninja_doc
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja_eval.h"  // d_ninja_graph


static int g_checks = 0;
static int g_failed = 0;


//==============================================================================
// 1.  HELPERS
//==============================================================================


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
d_tests_is
  Whether a text is the text expected; a difference is printed.
*/
static bool
d_tests_is(
    const char* _got,
    const char* _expected
)
{
    const bool same = ( (_got) &&
                        (strcmp(_got, _expected) == 0) );

    // what it holds, beside what it should
    if (!same)
    {
        printf("    want: [%s]\n    got:  [%s]\n", _expected,
               (_got) ? _got : "(null)");
    }

    return same;
}


/*
d_tests_value
  A value of a reading, unparsed, in a buffer the next call writes over;
"" for none.
*/
static const char*
d_tests_value(
    const struct d_ninja* _ninja,
    size_t                _value
)
{
    static char text[4096];

    // no value is ""
    if (_value == D_NINJA_NONE)
    {
        text[0] = '\0';

        return text;
    }

    const struct d_ninja_value* const value = &_ninja->values[_value];

    // a value too long for the buffer is not one a test writes
    if ((((value->end - value->start) * 2u) + 1u) > sizeof(text))
    {
        return "(too long)";
    }

    (void)d_ninja_unparse(_ninja, value, text);

    return text;
}


/*
d_tests_span_is
  Whether a span of a reading's text is a text.
*/
static bool
d_tests_span_is(
    const struct d_ninja* _ninja,
    size_t                _start,
    size_t                _end,
    const char*           _expected
)
{
    return ( ((_end - _start) == strlen(_expected)) &&
             (memcmp(&_ninja->text[_start], _expected, _end - _start) == 0) );
}


/*
d_tests_path_is
  Whether path `_k` of statement `_n` is in a section and unparses as a
text.
*/
static bool
d_tests_path_is(
    const struct d_ninja* _ninja,
    size_t                _n,
    size_t                _k,
    enum d_ninja_section  _section,
    const char*           _expected
)
{
    const struct d_ninja_statement* const statement = &_ninja->statements[_n];

    // a path the statement has
    if (_k >= statement->path_count)
    {
        return false;
    }

    const struct d_ninja_path* const path =
        &_ninja->paths[statement->first_path + _k];

    return ( (path->section == _section) &&
             (d_tests_is(d_tests_value(_ninja, path->value), _expected)) );
}


/*
d_tests_binding_is
  Whether binding `_k` of statement `_n` has a name and unparses as a text.
*/
static bool
d_tests_binding_is(
    const struct d_ninja* _ninja,
    size_t                _n,
    size_t                _k,
    const char*           _name,
    const char*           _expected
)
{
    const struct d_ninja_statement* const statement = &_ninja->statements[_n];

    // a binding the statement has
    if (_k >= statement->binding_count)
    {
        return false;
    }

    const struct d_ninja_binding* const binding =
        &_ninja->bindings[statement->first_binding + _k];

    return ( (d_tests_span_is(_ninja, binding->name_start, binding->name_end,
                              _name)) &&
             (d_tests_is(d_tests_value(_ninja, binding->value), _expected)) );
}


/*
d_tests_doc
  A document of a text that must read.
*/
static struct d_ninja_doc*
d_tests_doc(
    const char* _text
)
{
    char                      why[256];
    struct d_ninja_doc* const doc = d_ninja_doc_make("t.ninja", _text,
                                                     strlen(_text), why,
                                                     sizeof(why));

    // a text that did not read is a failure of the test itself
    if (!doc)
    {
        printf("  FAIL  the text did not read: %s\n", why);
        g_failed++;
    }

    return doc;
}


/*
d_tests_text_is
  Whether a document's text is the text expected; a difference is printed.
*/
static bool
d_tests_text_is(
    const struct d_ninja_doc* _doc,
    const char*               _expected
)
{
    size_t            length = 0;
    const char* const text   = d_ninja_doc_text(_doc, &length);
    const bool        same   = ( (length == strlen(_expected)) &&
                                 (memcmp(text, _expected, length) == 0) );

    // what it holds, beside what it should
    if (!same)
    {
        printf("    want: [%s]\n    got:  [%.*s]\n", _expected, (int)length,
               text);
    }

    return same;
}


/*
d_tests_kept
  Whether a change was kept, and the document holds the text expected; why
it was refused is printed.
*/
static bool
d_tests_kept(
    const struct d_ninja_doc* _doc,
    bool                      _kept,
    const char*               _expected
)
{
    // a refusal says why
    if (!_kept)
    {
        printf("    refused: %s\n", d_ninja_doc_problem(_doc));
    }

    return ( (_kept) &&
             (d_tests_text_is(_doc, _expected)) );
}


/*
d_tests_refused
  Whether a change was refused, the document unchanged and saying why.
*/
static bool
d_tests_refused(
    const struct d_ninja_doc* _doc,
    bool                      _kept,
    const char*               _before,
    const char*               _why
)
{
    const bool said = (strstr(d_ninja_doc_problem(_doc), _why) != NULL);

    // a refusal that says something else is printed
    if (!said)
    {
        printf("    why: %s\n", d_ninja_doc_problem(_doc));
    }

    return ( (!_kept) &&
             (said) &&
             (d_tests_text_is(_doc, _before)) );
}


/*
d_tests_load
  A build of a manifest's text, acted on as a version of ninja, its
includes read from a directory; NULL when it would not load, `_problem`
saying why.
*/
static struct d_ninja_graph*
d_tests_load(
    const char* _text,
    const char* _version,
    const char* _directory,
    char*       _problem,
    size_t      _capacity
)
{
    struct d_ninja_options options;

    options.directory = _directory;
    options.version   = _version;
    options.win32     = false;
    _problem[0]       = '\0';

    return d_ninja_load_text("build.ninja", _text, strlen(_text), &options,
                             _problem, _capacity);
}


/*
d_tests_nodes_are
  Whether a list of nodes is the paths expected, a space between each.
*/
static bool
d_tests_nodes_are(
    const struct d_ninja_graph* _graph,
    const size_t*               _nodes,
    size_t                      _count,
    const char*                 _expected
)
{
    char   text[1024];
    size_t used = 0;

    text[0] = '\0';

    // each node's path, a space before all but the first
    for (size_t at = 0; at < _count; ++at)
    {
        const int wrote = snprintf(&text[used], sizeof(text) - used, "%s%s",
                                   (at > 0u) ? " " : "",
                                   d_ninja_node_path(_graph, _nodes[at]));

        used += ( (wrote > 0) &&
                  ((size_t)wrote < (sizeof(text) - used)) ) ? (size_t)wrote
                                                            : 0u;
    }

    return d_tests_is(text, _expected);
}


/*
d_tests_section_is
  Whether a section of an edge holds the paths expected.
*/
static bool
d_tests_section_is(
    const struct d_ninja_graph* _graph,
    size_t                      _edge,
    enum d_ninja_section        _section,
    const char*                 _expected
)
{
    const size_t* nodes = NULL;
    const size_t  count = d_ninja_edge_nodes(_graph, _edge, _section, &nodes);

    return d_tests_nodes_are(_graph, nodes, count, _expected);
}


/*
d_tests_binding_of
  Whether a binding of an edge evaluates as expected; the text is freed.
*/
static bool
d_tests_binding_of(
    const struct d_ninja_graph* _graph,
    size_t                      _edge,
    const char*                 _name,
    bool                        _escape,
    const char*                 _expected
)
{
    char        why[256];
    char* const value = d_ninja_edge_binding(_graph, _edge, _name, _escape,
                                             why, sizeof(why));
    const bool  same  = d_tests_is(value, _expected);

    free(value);

    return same;
}


//==============================================================================
// 2.  READING
//==============================================================================


/*
d_tests_reading
  Tests the following:
  - each kind of statement, its name, its value unparsed, and its lines
  - a build's paths in each of its six sections, escapes decoded
  - a rule's, a build's and a pool's bindings
  - comments, at the top and among a statement's bindings
  - the version a file requires, a continued line, and CR LF endings
*/
static void
d_tests_reading(void)
{
    static const char text[] =
        "# top\n"
        "ninja_required_version = 1.10\n"
        "cflags = -O2 $\n"
        "    -g\n"
        "rule cc\n"
        "  command = cc ${cflags} $in -o $out\n"
        "  # within\n"
        "  description = CC $out\n"
        "build a.o b$ c.o | a.d: cc a.c | h.h || gen |@ check\n"
        "  cflags = $cflags -Wall\n"
        "pool link\n"
        "  depth = 4\n"
        "default a.o\n"
        "include rules.ninja\n"
        "subninja sub/build.ninja\n";
    struct d_ninja ninja;
    const bool     read = d_ninja_read(&ninja, text, sizeof(text) - 1u, NULL,
                                       0u);

    d_tests_check(read, "a file of every kind of statement reads");
    d_tests_check(ninja.statement_count == 8u, "  eight statements");

    // what follows needs the statements
    if ( (!read) ||
         (ninja.statement_count != 8u) )
    {
        d_ninja_release(&ninja);

        return;
    }

    const struct d_ninja_statement* const s = ninja.statements;

    d_tests_check( (s[0].kind == D_NINJA_VARIABLE) &&
                   (ninja.major == 1) && (ninja.minor == 10),
                   "  the version it requires is 1.10");
    d_tests_check( (s[1].kind == D_NINJA_VARIABLE) &&
                   (d_tests_span_is(&ninja, s[1].name_start, s[1].name_end,
                                    "cflags")) &&
                   (d_tests_is(d_tests_value(&ninja, s[1].value), "-O2 -g")),
                   "  a continued value: its continuation dropped");
    d_tests_check( (s[1].line == 3u) && (s[1].end_line == 4u),
                   "  ...and its lines, 3 to 4");
    d_tests_check( (s[2].kind == D_NINJA_RULE) &&
                   (d_tests_span_is(&ninja, s[2].name_start, s[2].name_end,
                                    "cc")) &&
                   (s[2].binding_count == 2u),
                   "  a rule, its name and two bindings");
    d_tests_check(d_tests_binding_is(&ninja, 2u, 0u, "command",
                                     "cc ${cflags} ${in} -o ${out}"),
                  "  ...its command, each reference written ${name}");
    d_tests_check(d_tests_binding_is(&ninja, 2u, 1u, "description",
                                     "CC ${out}"),
                  "  ...its description, past a comment among them");
    d_tests_check( (s[3].kind == D_NINJA_BUILD) &&
                   (d_tests_span_is(&ninja, s[3].name_start, s[3].name_end,
                                    "cc")) &&
                   (s[3].path_count == 7u),
                   "  a build, its rule and seven paths");
    d_tests_check( (d_tests_path_is(&ninja, 3u, 0u, D_NINJA_OUTPUT, "a.o")) &&
                   (d_tests_path_is(&ninja, 3u, 1u, D_NINJA_OUTPUT,
                                    "b c.o")) &&
                   (d_tests_path_is(&ninja, 3u, 2u, D_NINJA_IMPLICIT_OUTPUT,
                                    "a.d")),
                   "  ...its outputs, an escaped space decoded, and its "
                   "implicit output");
    d_tests_check( (d_tests_path_is(&ninja, 3u, 3u, D_NINJA_INPUT, "a.c")) &&
                   (d_tests_path_is(&ninja, 3u, 4u, D_NINJA_IMPLICIT,
                                    "h.h")) &&
                   (d_tests_path_is(&ninja, 3u, 5u, D_NINJA_ORDER_ONLY,
                                    "gen")) &&
                   (d_tests_path_is(&ninja, 3u, 6u, D_NINJA_VALIDATION,
                                    "check")),
                   "  ...its inputs, implicit, order-only and validation");
    d_tests_check(d_tests_binding_is(&ninja, 3u, 0u, "cflags",
                                     "${cflags} -Wall"),
                  "  ...and its binding");
    d_tests_check( (s[4].kind == D_NINJA_POOL) &&
                   (d_tests_binding_is(&ninja, 4u, 0u, "depth", "4")),
                   "  a pool and its depth");
    d_tests_check( (s[5].kind == D_NINJA_DEFAULT) &&
                   (s[5].name_start == s[5].name_end) &&
                   (d_tests_path_is(&ninja, 5u, 0u, D_NINJA_TARGET, "a.o")),
                   "  a default, no name, and its target");
    d_tests_check( (s[6].kind == D_NINJA_INCLUDE) &&
                   (d_tests_path_is(&ninja, 6u, 0u, D_NINJA_FILE,
                                    "rules.ninja")) &&
                   (s[7].kind == D_NINJA_SUBNINJA) &&
                   (d_tests_path_is(&ninja, 7u, 0u, D_NINJA_FILE,
                                    "sub/build.ninja")),
                   "  an include and a subninja, and their files");
    d_tests_check( (ninja.comment_count == 2u) &&
                   (ninja.comments[0].line == 1u) &&
                   (ninja.comments[0].within == D_NINJA_NONE) &&
                   (ninja.comments[1].line == 7u) &&
                   (ninja.comments[1].within == 2u),
                   "  two comments: one at the top, one the rule's");
    d_tests_check( (!ninja.crlf) &&
                   (ninja.line_count == 15u) &&
                   (d_ninja_line_of(&ninja, s[3].start) == 9u),
                   "  LF endings, fifteen lines, and a place's line");
    d_ninja_release(&ninja);

    static const char crlf[] = "a = 1\r\nbuild x: phony\r\n  y = 2\r\n";
    const bool        both   = d_ninja_read(&ninja, crlf, sizeof(crlf) - 1u,
                                            NULL, 0u);

    d_tests_check( (both) &&
                   (ninja.crlf) &&
                   (ninja.statement_count == 2u) &&
                   (d_tests_is(d_tests_value(&ninja, ninja.statements[0].value),
                               "1")) &&
                   (d_tests_binding_is(&ninja, 1u, 0u, "y", "2")),
                   "a CR LF file reads, its CRs no part of its values");
    d_ninja_release(&ninja);

    return;
}


/*
d_tests_reader_refusals
  Tests the following:
  - what ninja's parser refuses of a file alone is refused, on the line
    ninja names, in its words
*/
static void
d_tests_reader_refusals(void)
{
    static const struct
    {
        const char* text;
        size_t      line;
        const char* error;
    } cases[] =
    {
        { "rule cc\n  command = c\n  bogus = 1\n", 3u,
          "unexpected variable 'bogus'" },
        { "rule cc\n  description = d\n", 3u,
          "expected 'command =' line" },
        { "pool p\n", 2u, "expected 'depth =' line" },
        { "build a: cc $!\n", 1u,
          "bad $-escape (literal $ must be written as $$)" },
        { "build : cc b\n", 1u, "expected path" },
        { "default\n", 1u, "expected target name" },
        { "build a:\n", 1u, "expected build command name" },
        { "build a cc b\n", 1u,
          "expected ':', got newline ($ also escapes ':')" },
        { "rule cc\n  command = c\n  rspfile = r\n", 4u,
          "rspfile and rspfile_content need to be both specified" },
        { "x\n", 1u, "expected '=', got newline" },
        { "  x = 1\n", 1u, "unexpected indent" },
        { "rule\n", 1u, "expected rule name" },
        { "x = 1 $\n", 2u, "unexpected EOF" },
        { "a = x$^y\n", 1u,
          "using $^ escape requires specifying 'ninja_required_version' "
          "with version greater or equal 1.14" }
    };

    // each case refused where and as ninja refuses it
    for (size_t at = 0; at < (sizeof(cases) / sizeof(cases[0])); ++at)
    {
        struct d_ninja ninja;
        char           what[160];
        const bool     read = d_ninja_read(&ninja, cases[at].text,
                                           strlen(cases[at].text), NULL, 0u);

        (void)snprintf(what, sizeof(what), "refused at line %zu: %s",
                       cases[at].line, cases[at].error);
        d_tests_check( (!read) &&
                       (ninja.error_line == cases[at].line) &&
                       (d_tests_is(ninja.error, cases[at].error)),
                       what);
        d_ninja_release(&ninja);
    }

    struct d_ninja ninja;
    char           why[192];
    const bool     read = d_ninja_read(&ninja, "x\n", 2u, why, sizeof(why));

    d_tests_check( (!read) &&
                   (d_tests_is(why, "line 1: expected '=', got newline")),
                   "the problem names its line");
    d_ninja_release(&ninja);

    return;
}


/*
d_tests_writers
  Tests the following:
  - a path and a value escaped so they read back as themselves, and what
    cannot be written so refused
  - a path made canonical, and written into a command as ninja writes it
  - names, and the names a rule may bind
*/
static void
d_tests_writers(void)
{
    char*  out    = NULL;
    size_t length = 0;
    char   canonical[64];

    out = d_ninja_escape("a b:c$", 6u, true, false, &length);
    d_tests_check( (d_tests_is(out, "a$ b$:c$$")) && (length == 9u),
                   "a path's space, colon and dollar escaped");
    free(out);
    out = d_ninja_escape(" a b:c", 6u, false, false, NULL);
    d_tests_check(d_tests_is(out, "$ a b:c"),
                  "a value's leading space escaped, and only that");
    free(out);
    out = d_ninja_escape("a\nb", 3u, false, true, NULL);
    d_tests_check(d_tests_is(out, "a$^b"), "a newline written $^ when let");
    free(out);
    d_tests_check( (!d_ninja_escape("a\nb", 3u, false, false, NULL)) &&
                   (!d_ninja_escape("a|b", 3u, true, false, NULL)) &&
                   (!d_ninja_escape("a\rb", 3u, false, true, NULL)),
                   "a newline not let, a pipe in a path, a CR: refused");

    length = d_ninja_canonical("./a//b/../c/.", 13u, false, canonical);
    d_tests_check( (d_tests_is(canonical, "a/c")) && (length == 3u),
                   "a path made canonical");
    (void)d_ninja_canonical("../x/../../y", 12u, false, canonical);
    d_tests_check(d_tests_is(canonical, "../../y"),
                  "  ..: kept where there is nothing to take back");
    (void)d_ninja_canonical("./", 2u, false, canonical);
    d_tests_check(d_tests_is(canonical, "."), "  nothing is .");
    (void)d_ninja_canonical("a\\.\\b", 5u, true, canonical);
    d_tests_check(d_tests_is(canonical, "a/b"),
                  "  on Windows, \\ separates too, and is written /");

    out = d_ninja_shell_escape("a/b.c", 5u, false, NULL);
    d_tests_check(d_tests_is(out, "a/b.c"), "a plain path as it stands");
    free(out);
    out = d_ninja_shell_escape("it's", 4u, false, NULL);
    d_tests_check(d_tests_is(out, "'it'\\''s'"), "a quote quoted, POSIX");
    free(out);
    out = d_ninja_shell_escape("a b", 3u, true, NULL);
    d_tests_check(d_tests_is(out, "\"a b\""), "a space quoted, Windows");
    free(out);

    d_tests_check( (d_ninja_is_name("a.b-c_9", 7u)) &&
                   (!d_ninja_is_name("a b", 3u)) &&
                   (!d_ninja_is_name("", 0u)),
                   "names: [A-Za-z0-9_.-]+");
    d_tests_check( (d_ninja_is_reserved("command", 7u)) &&
                   (d_ninja_is_reserved("msvc_deps_prefix", 16u)) &&
                   (!d_ninja_is_reserved("cflags", 6u)),
                   "a rule may bind command and msvc_deps_prefix, not "
                   "cflags");
    d_tests_check( (strcmp(d_ninja_kind_name(D_NINJA_SUBNINJA),
                           "subninja") == 0) &&
                   (strcmp(d_ninja_section_name(D_NINJA_ORDER_ONLY),
                           "order_only") == 0),
                   "kinds and sections by name");

    return;
}


//==============================================================================
// 3.  LOADING
//==============================================================================


/*
d_tests_loading
  Tests the following:
  - edges in the manifest's order, each section's nodes, canonical
  - a binding evaluated as ninja's EdgeEnv does, $in and $out escaped or
    not, the edge's bindings before the rule's
  - nodes: found by any spelling of their path, their edges in and out,
    and what they validate
  - defaults, roots, rules, pools, variables and files
*/
static void
d_tests_loading(void)
{
    static const char text[] =
        "cflags = -g\n"
        "dir = out\n"
        "rule cc\n"
        "  command = cc $cflags $in -o $out\n"
        "  description = CC $out\n"
        "  depfile = $out.d\n"
        "build $dir/a.o: cc a.c b c$ d | h.h || gen |@ check\n"
        "  cflags = -O2 $cflags\n"
        "build $dir/b.o $dir/c.o | $dir/b.d: cc ./x/../b.c\n"
        "build all: phony $dir/a.o $dir/b.o\n"
        "build check: phony\n"
        "build gen: phony\n"
        "default all\n";
    char                        why[512];
    struct d_ninja_graph* const graph = d_tests_load(text, NULL, NULL, why,
                                                     sizeof(why));
    const size_t*               nodes = NULL;

    d_tests_check(graph != NULL, "a manifest loads");

    // what follows needs the build
    if (!graph)
    {
        printf("    %s\n", why);

        return;
    }

    d_tests_check( (d_ninja_edge_count(graph) == 5u) &&
                   (strcmp(d_ninja_edge_rule(graph, 0u), "cc") == 0) &&
                   (d_ninja_edge_phony(graph, 2u)) &&
                   (!d_ninja_edge_phony(graph, 0u)),
                   "five edges, two of cc and three phony");
    d_tests_check( (d_tests_section_is(graph, 0u, D_NINJA_OUTPUT,
                                       "out/a.o")) &&
                   (d_tests_section_is(graph, 0u, D_NINJA_INPUT,
                                       "a.c b c d")) &&
                   (d_tests_section_is(graph, 0u, D_NINJA_IMPLICIT, "h.h")) &&
                   (d_tests_section_is(graph, 0u, D_NINJA_ORDER_ONLY,
                                       "gen")) &&
                   (d_tests_section_is(graph, 0u, D_NINJA_VALIDATION,
                                       "check")),
                   "  the first's sections, its outputs evaluated");
    d_tests_check( (d_tests_section_is(graph, 1u, D_NINJA_OUTPUT,
                                       "out/b.o out/c.o")) &&
                   (d_tests_section_is(graph, 1u, D_NINJA_IMPLICIT_OUTPUT,
                                       "out/b.d")) &&
                   (d_tests_section_is(graph, 1u, D_NINJA_INPUT, "b.c")),
                   "  the second's, its input made canonical");
    d_tests_check(d_tests_binding_of(graph, 0u, "command", true,
                                     "cc -O2 -g a.c b 'c d' -o out/a.o"),
                  "a command: the edge's cflags, $in shell-escaped");
    d_tests_check(d_tests_binding_of(graph, 0u, "command", false,
                                     "cc -O2 -g a.c b c d -o out/a.o"),
                  "  ...and raw");
    d_tests_check( (d_tests_binding_of(graph, 1u, "command", true,
                                       "cc -g b.c -o out/b.o out/c.o")) &&
                   (d_tests_binding_of(graph, 0u, "depfile", false,
                                       "out/a.o.d")) &&
                   (d_tests_binding_of(graph, 2u, "command", true, "")),
                   "  the file's cflags; a depfile; a phony's command");
    d_tests_check( (strcmp(d_ninja_edge_pool(graph, 0u), "") == 0) &&
                   (strcmp(d_ninja_edge_file(graph, 1u), "build.ninja") ==
                    0) &&
                   (d_ninja_edge_line(graph, 1u) == 9u),
                   "an edge's pool, file and line");

    const size_t a = d_ninja_node_find(graph, "./out//a.o", 10u);

    d_tests_check( (a != D_NINJA_NONE) &&
                   (strcmp(d_ninja_node_path(graph, a), "out/a.o") == 0) &&
                   (d_ninja_node_in_edge(graph, a) == 0u),
                   "a node found by another spelling, and its in-edge");
    d_tests_check( (a != D_NINJA_NONE) &&
                   (d_ninja_node_out_edges(graph, a, &nodes) == 1u) &&
                   (nodes[0] == 2u),
                   "  ...and its out-edge, all's");

    const size_t check = d_ninja_node_find(graph, "check", 5u);

    d_tests_check( (check != D_NINJA_NONE) &&
                   (d_ninja_node_validation_edges(graph, check, &nodes) ==
                    1u) &&
                   (nodes[0] == 0u) &&
                   (d_ninja_node_out_edges(graph, check, &nodes) == 0u),
                   "a validation: the edge it validates, no out-edge");
    d_tests_check( (d_ninja_node_find(graph, "nothere", 7u) == D_NINJA_NONE) &&
                   (d_ninja_node_in_edge(graph,
                                         d_ninja_node_find(graph, "a.c",
                                                           3u)) ==
                    D_NINJA_NONE),
                   "no node for a path not named; a source has no in-edge");
    d_tests_check(d_tests_nodes_are(graph, nodes,
                                    d_ninja_defaults(graph, &nodes), "all"),
                  "the defaults");
    d_tests_check(d_tests_nodes_are(graph, nodes,
                                    d_ninja_roots(graph, &nodes),
                                    "out/c.o out/b.d all check"),
                  "the roots, as ninja's RootNodes finds them");
    d_tests_check( (d_ninja_rule_count(graph) == 2u) &&
                   (strcmp(d_ninja_rule_name(graph, 0u), "cc") == 0) &&
                   (strcmp(d_ninja_rule_name(graph, 1u), "phony") == 0),
                   "the rules, sorted, phony among them");

    char* const command = d_ninja_rule_binding(graph, 0u, "command");

    d_tests_check( (d_tests_is(command, "cc ${cflags} ${in} -o ${out}")) &&
                   (!d_ninja_rule_binding(graph, 0u, "pool")),
                   "  a rule's binding unparsed; one it has not, NULL");
    free(command);
    d_tests_check( (d_ninja_pool_count(graph) == 2u) &&
                   (strcmp(d_ninja_pool_name(graph, 1u), "console") == 0) &&
                   (d_ninja_pool_depth(graph, 1u) == 1),
                   "the pools ninja defines: the default and console");

    char* const dir     = d_ninja_variable(graph, "dir");
    char* const nothing = d_ninja_variable(graph, "nothing");

    d_tests_check( (d_tests_is(dir, "out")) && (d_tests_is(nothing, "")),
                   "a variable of the top scope, and one not there");
    free(dir);
    free(nothing);
    d_tests_check( (d_ninja_file_count(graph) == 1u) &&
                   (d_ninja_warning_count(graph) == 0u),
                   "one file read, and no warning");
    d_ninja_graph_free(graph);

    return;
}


/*
d_tests_load_refusals
  Tests the following:
  - what ninja refuses as it loads a build, refused in its words, with the
    file and the line it names
  - what a version of ninja reads differently from another
  - a warning for a file that asks for an older ninja
*/
static void
d_tests_load_refusals(void)
{
    static const struct
    {
        const char* text;
        const char* version;
        const char* problem;
    } cases[] =
    {
        { "build a: cc b\n", NULL,
          "build.ninja:1: unknown build rule 'cc'" },
        { "build a: cc $!\n", NULL,
          "build.ninja:1: unknown build rule 'cc'" },
        { "rule cc\n  command = c\nbuild a: cc b\nbuild a: cc c\n", NULL,
          "build.ninja:5: multiple rules generate a" },
        { "rule cc\n  command = c\nbuild a: cc b\n  pool = p\n", NULL,
          "build.ninja:5: unknown pool name 'p'" },
        { "rule cc\n  command = c\nrule cc\n  command = d\n", NULL,
          "build.ninja:3: duplicate rule 'cc'" },
        { "pool p\n  depth = 1\npool p\n  depth = 2\n", NULL,
          "build.ninja:3: duplicate pool 'p'" },
        { "ninja_required_version = 9.0\n", NULL,
          "ninja version (1.14.0) incompatible with build file "
          "ninja_required_version version (9.0)." },
        { "ninja_required_version = 1.12\n", "1.11.1",
          "ninja version (1.11.1) incompatible with build file "
          "ninja_required_version version (1.12)." },
        { "default nothere\n", NULL,
          "build.ninja:1: unknown target 'nothere'" },
        { "rule cc\n  command = c\nbuild a: cc b\n  dyndep = d\n", NULL,
          "build.ninja:5: dyndep 'd' is not an input" },
        { "rule cc\n  command = c\nbuild $\n: cc b\n", NULL,
          "build.ninja:4: expected path" },
        { "pool p\n  depth = x\n", NULL,
          "build.ninja:2: invalid pool depth" },
        { "rule cc\n  command = c\nbuild a a: cc b\n", NULL,
          "build.ninja:4: a is defined as an output multiple times" },
        { "rule cc\n  command = c\nbuild a a: cc b\n", "1.11.1",
          "build.ninja:4: multiple rules generate a" },
        { "ninja_required_version = 1.14\na = x$^y\n", "1.13.1",
          "ninja version (1.13.1) incompatible with build file "
          "ninja_required_version version (1.14)." },
        { "a = x$^y\n", "1.11.1",
          "build.ninja:1: bad $-escape (literal $ must be written as $$)" }
    };

    // each case refused as ninja refuses it
    for (size_t at = 0; at < (sizeof(cases) / sizeof(cases[0])); ++at)
    {
        char                        why[512];
        struct d_ninja_graph* const graph = d_tests_load(cases[at].text,
                                                         cases[at].version,
                                                         NULL, why,
                                                         sizeof(why));

        d_tests_check( (!graph) &&
                       (d_tests_is(why, cases[at].problem)),
                       cases[at].problem);
        d_ninja_graph_free(graph);
    }

    char                  why[512];
    struct d_ninja_graph* graph = d_tests_load("pool p\n  depth = x\n",
                                               "1.11.1", NULL, why,
                                               sizeof(why));

    d_tests_check( (graph) &&
                   (d_ninja_pool_depth(graph, 2u) == 0),
                   "before 1.14, a depth that is no number is 0");
    d_ninja_graph_free(graph);
    graph = d_tests_load("ninja_required_version = 0.9\n", NULL, NULL, why,
                         sizeof(why));
    d_tests_check( (graph) &&
                   (d_ninja_warning_count(graph) == 1u) &&
                   (d_tests_is(d_ninja_warning(graph, 0u),
                               "ninja executable version (1.14.0) greater "
                               "than build file ninja_required_version "
                               "(0.9); versions may be incompatible.")),
                   "a file that asks for an older major version warns");
    d_ninja_graph_free(graph);
    graph = d_tests_load("rule cc\n"
                         "  command = x $description\n"
                         "  description = $command\n"
                         "build x: cc y\n", NULL, NULL, why, sizeof(why));

    char* const value = (graph) ? d_ninja_edge_binding(graph, 0u, "command",
                                                       true, why, sizeof(why))
                                : NULL;

    d_tests_check( (graph) &&
                   (!value) &&
                   (d_tests_is(why, "cycle in rule variables: description "
                                    "-> command -> description")),
                   "a cycle among a rule's bindings, as ninja words it");
    free(value);
    d_ninja_graph_free(graph);

    return;
}


/*
d_tests_put
  Writes a file under a directory, its own directories made.
*/
static void
d_tests_put(
    const char* _directory,
    const char* _name,
    const char* _text
)
{
    char path[1024];
    char why[256];

    (void)snprintf(path, sizeof(path), "%s/%s", _directory, _name);

    // a file that cannot be written fails what reads it
    if ( (!d_fs_make_parent(path)) ||
         (!d_text_write(path, _text, strlen(_text), why, sizeof(why))) )
    {
        printf("    cannot write %s\n", path);
    }

    return;
}


/*
d_tests_scopes
  Tests the following:
  - an include read into the scope of the file that names it, and a
    subninja into a scope of its own, the rules above it seen from it
  - every file read from the directory ninja runs in, and listed
  - a manifest loaded from disk, and one that is not there
  Long as the three files are written before the checks.
*/
static void
d_tests_scopes(
    const char* _work
)
{
    char directory[1024];
    char why[512];

    (void)snprintf(directory, sizeof(directory), "%s/ninja/scope", _work);
    d_tests_put(directory, "build.ninja",
                "v = top\n"
                "rule echo\n"
                "  command = echo $v $out\n"
                "include inc.ninja\n"
                "subninja sub/build.ninja\n"
                "build top.txt: echo\n");
    d_tests_put(directory, "inc.ninja", "w = inc\nv = changed\n");
    d_tests_put(directory, "sub/build.ninja", "v = sub\nbuild sub.txt: echo\n");

    struct d_ninja_options options;

    options.directory = directory;
    options.version   = NULL;
    options.win32     = false;

    struct d_ninja_graph* const graph = d_ninja_load("build.ninja", &options,
                                                     why, sizeof(why));

    d_tests_check(graph != NULL, "a manifest with an include and a subninja");

    // what follows needs the build
    if (!graph)
    {
        printf("    %s\n", why);

        return;
    }

    char* const w = d_ninja_variable(graph, "w");

    d_tests_check( (d_ninja_edge_count(graph) == 2u) &&
                   (d_tests_binding_of(graph, 0u, "command", true,
                                       "echo sub sub.txt")) &&
                   (d_tests_binding_of(graph, 1u, "command", true,
                                       "echo changed top.txt")),
                   "the subninja's v its own; the include's the file's");
    d_tests_check(d_tests_is(w, "inc"), "  ...and its w the top scope's");
    free(w);
    d_tests_check( (d_ninja_file_count(graph) == 3u) &&
                   (strcmp(d_ninja_file_path(graph, 1u), "inc.ninja") == 0) &&
                   (strcmp(d_ninja_file_path(graph, 2u), "sub/build.ninja") ==
                    0) &&
                   (strcmp(d_ninja_edge_file(graph, 0u), "sub/build.ninja") ==
                    0),
                   "three files read, in order, and an edge's file");
    d_ninja_graph_free(graph);

    struct d_ninja_graph* const none = d_ninja_load("nothere.ninja",
                                                    &options, why,
                                                    sizeof(why));

    d_tests_check( (!none) &&
                   (d_tests_is(why, "loading 'nothere.ninja': No such file "
                                    "or directory")),
                   "a manifest that is not there, as ninja says it");
    d_tests_put(directory, "bad.ninja", "include gone.ninja\n");

    struct d_ninja_graph* const bad = d_ninja_load("bad.ninja", &options, why,
                                                   sizeof(why));

    d_tests_check( (!bad) &&
                   (d_tests_is(why, "bad.ninja:1: loading 'gone.ninja': No "
                                    "such file or directory")),
                   "an include that is not there, with its line");

    return;
}


//==============================================================================
// 4.  DOCUMENTS
//==============================================================================


/*
d_tests_documents
  Tests the following:
  - a document made of a text, and of a text ninja would refuse
  - its path, its text, its changes, and where the last change was
  - a raw splice kept, and ones refused for what they do outside them
*/
static void
d_tests_documents(void)
{
    static const char before[] = "a = 1\nbuild x: phony\ny = 2\n";
    char              why[256];
    size_t            start   = 0;
    size_t            old_end = 0;
    size_t            new_end = 0;

    d_tests_check( (!d_ninja_doc_make("t.ninja", "x\n", 2u, why,
                                      sizeof(why))) &&
                   (d_tests_is(why, "t.ninja: line 1: expected '=', got "
                                    "newline")),
                   "a text ninja refuses makes no document, and says why");

    struct d_ninja_doc* const doc = d_tests_doc(before);

    // what follows needs the document
    if (!doc)
    {
        return;
    }

    d_tests_check( (strcmp(d_ninja_doc_path(doc), "t.ninja") == 0) &&
                   (d_ninja_doc_changes(doc) == 0u) &&
                   (!d_ninja_doc_changed(doc, &start, &old_end, &new_end)) &&
                   (d_ninja_doc_records(doc)->statement_count == 3u),
                   "a document: its path, no changes, three statements");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_splice(doc, 21u, 21u,
                                                          "  ", 2u),
                                  before, "outside it"),
                  "a splice that makes y x's binding is refused");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_splice(doc, 6u, 6u, "#",
                                                          1u),
                                  before, "outside it"),
                  "  ...and one that makes a statement a comment");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_splice(doc, 4u, 5u, "$!",
                                                          2u),
                                  before, "unreadable"),
                  "  ...and one that leaves the file unreadable");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_splice(doc, 40u, 40u, "",
                                                          0u),
                                  before, "not within the file"),
                  "  ...and one past the file's end");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_splice(doc, 4u, 5u, "22",
                                                       2u),
                               "a = 22\nbuild x: phony\ny = 2\n"),
                  "a splice within a value is kept");
    d_tests_check( (d_ninja_doc_changes(doc) == 1u) &&
                   (d_ninja_doc_changed(doc, &start, &old_end, &new_end)) &&
                   (start == 4u) && (old_end == 5u) && (new_end == 6u),
                   "  ...counted, and where it was");
    d_ninja_doc_free(doc);

    return;
}


/*
d_tests_values
  Tests the following:
  - a variable's value set, a space after a bare `=`
  - a variable's, a rule's and a pool's name set, and a build's rule
  - each refused where the statement has none, or the text is no name,
    or reads as more than was asked
*/
static void
d_tests_values(void)
{
    static const char before[] =
        "cflags = -O2\n"
        "empty =\n"
        "rule cc\n"
        "  command = cc $cflags -c $in -o $out\n"
        "build a.o: cc a.c\n"
        "default a.o\n";
    struct d_ninja_doc* const doc = d_tests_doc(before);

    // what follows needs the document
    if (!doc)
    {
        return;
    }

    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_value(doc, 0u,
                                                          "-O1 -g"),
                               "cflags = -O1 -g\n"
                               "empty =\n"
                               "rule cc\n"
                               "  command = cc $cflags -c $in -o $out\n"
                               "build a.o: cc a.c\n"
                               "default a.o\n"),
                  "a variable's value set");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_value(doc, 1u, "x"),
                               "cflags = -O1 -g\n"
                               "empty = x\n"
                               "rule cc\n"
                               "  command = cc $cflags -c $in -o $out\n"
                               "build a.o: cc a.c\n"
                               "default a.o\n"),
                  "  an empty one, a space after its =");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_name(doc, 2u, "cxx"),
                               "cflags = -O1 -g\n"
                               "empty = x\n"
                               "rule cxx\n"
                               "  command = cc $cflags -c $in -o $out\n"
                               "build a.o: cc a.c\n"
                               "default a.o\n"),
                  "a rule's name set");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_name(doc, 3u, "cxx"),
                               "cflags = -O1 -g\n"
                               "empty = x\n"
                               "rule cxx\n"
                               "  command = cc $cflags -c $in -o $out\n"
                               "build a.o: cxx a.c\n"
                               "default a.o\n"),
                  "  ...and the rule a build uses");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_name(doc, 0u, "flags"),
                               "flags = -O1 -g\n"
                               "empty = x\n"
                               "rule cxx\n"
                               "  command = cc $cflags -c $in -o $out\n"
                               "build a.o: cxx a.c\n"
                               "default a.o\n"),
                  "  ...and a variable's");

    size_t            length = 0;
    const char* const now    = d_ninja_doc_text(doc, &length);
    char              text[512];

    memcpy(text, now, length + 1u);
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_value(doc, 2u, "x"),
                                  text, "no variable"),
                  "a value set on a rule is refused");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_value(doc, 0u, "$!"),
                                  text, "unreadable"),
                  "  ...and a value ninja would not read");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_value(doc, 0u,
                                                             "a\nb = 2"),
                                  text, "does not read back"),
                  "  ...and one that reads as two statements");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_name(doc, 4u, "x"),
                                  text, "no name"),
                  "a name set on a default is refused");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_name(doc, 2u, "a b"),
                                  text, "not a name"),
                  "  ...and a name that is none");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_name(doc, 9u, "x"),
                                  text, "no statement 10"),
                  "  ...and a statement the file has not");
    d_ninja_doc_free(doc);

    return;
}


/*
d_tests_bindings
  Tests the following:
  - a binding set, and one added, indented as the statement's are, or by
    two spaces, after its last
  - one added to a CR LF file, with its endings
  - every binding of a name removed
  - each refused where ninja would refuse it
*/
static void
d_tests_bindings(void)
{
    static const char before[] =
        "rule cc\n"
        "    command = cc $cflags -c $in -o $out\n"
        "build a.o: cc a.c\n"
        "build b.o: cc b.c\n"
        "  cflags = -O0\n"
        "  x = 1\n"
        "  cflags = -O1\n"
        "n = 1\n";
    struct d_ninja_doc* doc = d_tests_doc(before);

    // what follows needs the document
    if (!doc)
    {
        return;
    }

    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_binding(doc, 2u, "cflags",
                                                            "-O3"),
                               "rule cc\n"
                               "    command = cc $cflags -c $in -o $out\n"
                               "build a.o: cc a.c\n"
                               "build b.o: cc b.c\n"
                               "  cflags = -O0\n"
                               "  x = 1\n"
                               "  cflags = -O3\n"
                               "n = 1\n"),
                  "the last binding of a name set");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_binding(doc, 1u, "cflags",
                                                            "-O1"),
                               "rule cc\n"
                               "    command = cc $cflags -c $in -o $out\n"
                               "build a.o: cc a.c\n"
                               "  cflags = -O1\n"
                               "build b.o: cc b.c\n"
                               "  cflags = -O0\n"
                               "  x = 1\n"
                               "  cflags = -O3\n"
                               "n = 1\n"),
                  "a binding added, two spaces in");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_binding(doc, 0u,
                                                            "description",
                                                            "CC $out"),
                               "rule cc\n"
                               "    command = cc $cflags -c $in -o $out\n"
                               "    description = CC $out\n"
                               "build a.o: cc a.c\n"
                               "  cflags = -O1\n"
                               "build b.o: cc b.c\n"
                               "  cflags = -O0\n"
                               "  x = 1\n"
                               "  cflags = -O3\n"
                               "n = 1\n"),
                  "  ...and one indented as the rule's are");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_remove_binding(doc, 2u,
                                                               "cflags"),
                               "rule cc\n"
                               "    command = cc $cflags -c $in -o $out\n"
                               "    description = CC $out\n"
                               "build a.o: cc a.c\n"
                               "  cflags = -O1\n"
                               "build b.o: cc b.c\n"
                               "  x = 1\n"
                               "n = 1\n"),
                  "every binding of a name removed");

    size_t            length = 0;
    const char* const now    = d_ninja_doc_text(doc, &length);
    char              text[512];

    memcpy(text, now, length + 1u);
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_binding(doc, 0u,
                                                               "bogus", "1"),
                                  text, "unreadable"),
                  "a binding no rule may have is refused");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_remove_binding(doc, 0u,
                                                                  "command"),
                                  text, "unreadable"),
                  "  ...and a rule left without its command");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_remove_binding(doc, 1u,
                                                                  "nothere"),
                                  text, "no binding nothere"),
                  "  ...and a binding not there removed");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_binding(doc, 3u, "a",
                                                               "1"),
                                  text, "takes no bindings"),
                  "  ...and a binding set on a variable");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_binding(doc, 1u, "a b",
                                                               "1"),
                                  text, "not a binding's name"),
                  "  ...and a name that is none");
    d_ninja_doc_free(doc);

    doc = d_tests_doc("rule r\r\n  command = c\r\nbuild a: r\r\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_set_binding(doc, 0u,
                                                              "description",
                                                              "D"),
                                 "rule r\r\n  command = c\r\n"
                                 "  description = D\r\nbuild a: r\r\n")),
                   "  ...and to a CR LF file, with its endings");
    d_ninja_doc_free(doc);
    doc = d_tests_doc("build b.o: cc b.c\n  cflags = -O0\nbuild c: phony\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_binding(doc, 0u,
                                                                 "cflags"),
                                 "build b.o: cc b.c\nbuild c: phony\n")),
                   "a statement's last binding removed, the next after it");
    d_ninja_doc_free(doc);
    doc = d_tests_doc("build b.o: cc b.c\n  cflags = -O0\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_binding(doc, 0u,
                                                                 "cflags"),
                                 "build b.o: cc b.c\n")),
                   "  ...and the file's last line");
    d_ninja_doc_free(doc);

    return;
}


/*
d_tests_paths
  Tests the following:
  - a path set, and inserted into each section of a build, the section's
    pipe written when it has none
  - a path removed, its pipe with it when it was its section's last, and
    the line it continued onto
  - each refused where the statement has no such place, or ninja would
    read the text as other paths, or a build or a default is left empty
*/
static void
d_tests_paths(void)
{
    static const struct
    {
        enum d_ninja_section section;
        size_t               place;
        const char*          text;
        const char*          after;
    } inserts[] =
    {
        { D_NINJA_OUTPUT,          1u, "b.o", "build a.o b.o: cc a.c\n"     },
        { D_NINJA_OUTPUT,          0u, "z.o", "build z.o a.o: cc a.c\n"     },
        { D_NINJA_IMPLICIT_OUTPUT, 0u, "a.d", "build a.o | a.d: cc a.c\n"   },
        { D_NINJA_INPUT,           0u, "z.c", "build a.o: cc z.c a.c\n"     },
        { D_NINJA_INPUT,           1u, "b.c", "build a.o: cc a.c b.c\n"     },
        { D_NINJA_IMPLICIT,        0u, "h.h", "build a.o: cc a.c | h.h\n"   },
        { D_NINJA_ORDER_ONLY,      0u, "gen", "build a.o: cc a.c || gen\n"  },
        { D_NINJA_VALIDATION,      0u, "chk", "build a.o: cc a.c |@ chk\n"  },
        { D_NINJA_INPUT,           0u, "a$ b", "build a.o: cc a$ b a.c\n"   }
    };

    // each insertion, into a build of its own
    for (size_t at = 0; at < (sizeof(inserts) / sizeof(inserts[0])); ++at)
    {
        struct d_ninja_doc* const doc = d_tests_doc("build a.o: cc a.c\n");
        char                      what[128];

        (void)snprintf(what, sizeof(what), "a path inserted: %s",
                       inserts[at].after);
        d_tests_check( (doc) &&
                       (d_tests_kept(doc,
                                     d_ninja_doc_insert_path(
                                         doc, 0u, inserts[at].section,
                                         inserts[at].place, inserts[at].text),
                                     inserts[at].after)),
                       what);
        d_ninja_doc_free(doc);
    }

    struct d_ninja_doc* doc = d_tests_doc("build a.o: phony\n");

    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_insert_path(
                                          doc, 0u, D_NINJA_ORDER_ONLY, 0u,
                                          "gen"),
                                 "build a.o: phony || gen\n")),
                   "  an order-only one into a build with no inputs");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_insert_path(
                                          doc, 0u, D_NINJA_INPUT, 0u, "x"),
                                 "build a.o: phony x || gen\n")),
                   "  ...then an input, after the rule");
    d_tests_check( (doc) &&
                   (d_tests_refused(doc, d_ninja_doc_insert_path(
                                             doc, 0u, D_NINJA_INPUT, 3u,
                                             "y"),
                                    "build a.o: phony x || gen\n",
                                    "no such place")),
                   "a place past the section's end is refused");
    d_tests_check( (doc) &&
                   (d_tests_refused(doc, d_ninja_doc_insert_path(
                                             doc, 0u, D_NINJA_TARGET, 0u,
                                             "y"),
                                    "build a.o: phony x || gen\n",
                                    "no such place")),
                   "  ...and a section a build has not");
    d_tests_check( (doc) &&
                   (d_tests_refused(doc, d_ninja_doc_insert_path(
                                             doc, 0u, D_NINJA_INPUT, 0u,
                                             "y z"),
                                    "build a.o: phony x || gen\n",
                                    "does not read back")),
                   "  ...and a text that reads as two paths");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_set_path(doc, 0u, 1u,
                                                           "x2"),
                                 "build a.o: phony x2 || gen\n")),
                   "a path set");
    d_ninja_doc_free(doc);

    doc = d_tests_doc("build a.o: cc a.c | h.h || gen |@ chk\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 2u),
                                 "build a.o: cc a.c || gen |@ chk\n")) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 2u),
                                 "build a.o: cc a.c |@ chk\n")) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 2u),
                                 "build a.o: cc a.c\n")),
                   "a section's last path removed, and its pipe");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 1u),
                                 "build a.o: cc\n")),
                   "  ...and the only input");
    d_tests_check( (doc) &&
                   (d_tests_refused(doc, d_ninja_doc_remove_path(doc, 0u, 0u),
                                    "build a.o: cc\n", "unreadable")),
                   "a build left with no output is refused");
    d_ninja_doc_free(doc);

    doc = d_tests_doc("build a b | c: cc\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 2u),
                                 "build a b: cc\n")) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 0u),
                                 "build b: cc\n")),
                   "outputs removed, the first and an implicit one");
    d_ninja_doc_free(doc);
    doc = d_tests_doc("build a: cc b $\n    c\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 2u),
                                 "build a: cc b\n")),
                   "a path removed with the line it continued onto");
    d_ninja_doc_free(doc);
    doc = d_tests_doc("default a b\ninclude x.ninja\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_insert_path(
                                          doc, 0u, D_NINJA_TARGET, 2u, "c"),
                                 "default a b c\ninclude x.ninja\n")) &&
                   (d_tests_kept(doc, d_ninja_doc_remove_path(doc, 0u, 0u),
                                 "default b c\ninclude x.ninja\n")),
                   "a default's target added and removed");
    d_tests_check( (doc) &&
                   (d_tests_refused(doc, d_ninja_doc_remove_path(doc, 1u, 0u),
                                    "default b c\ninclude x.ninja\n",
                                    "no such path to remove")),
                   "an include's file is not removed");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_set_path(doc, 1u, 0u,
                                                           "y.ninja"),
                                 "default b c\ninclude y.ninja\n")),
                   "  ...but set");
    d_ninja_doc_free(doc);

    return;
}


/*
d_tests_statements
  Tests the following:
  - statements inserted above the comments that lead into the one they go
    before, and after the last, a line ending given the text and the file
  - a statement replaced by one of its kind, and statements removed
  - each refused where the text is no statements, or the kind not the one
    replaced
*/
static void
d_tests_statements(void)
{
    static const char before[] = "a = 1\n\n# about b\n# more\nb = 2\n";
    struct d_ninja_doc* doc    = d_tests_doc(before);

    // what follows needs the document
    if (!doc)
    {
        return;
    }

    d_tests_check(d_tests_kept(doc, d_ninja_doc_insert_statements(doc, 1u,
                                                                  "c = 3\n"),
                               "a = 1\n\nc = 3\n# about b\n# more\nb = 2\n"),
                  "a statement inserted above b's comments");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_insert_statements(doc, 3u,
                                                                  "d = 4"),
                               "a = 1\n\nc = 3\n# about b\n# more\nb = 2\n"
                               "d = 4\n"),
                  "  ...one after the last, its line ended");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_set_statement(doc, 1u,
                                                              "c = 33\n"),
                               "a = 1\n\nc = 33\n# about b\n# more\nb = 2\n"
                               "d = 4\n"),
                  "a statement replaced");
    d_tests_check(d_tests_kept(doc, d_ninja_doc_remove_statements(doc, 0u,
                                                                  1u),
                               "# about b\n# more\nb = 2\nd = 4\n"),
                  "two statements removed, the comments after them kept");

    size_t            length = 0;
    const char* const now    = d_ninja_doc_text(doc, &length);
    char              text[256];

    memcpy(text, now, length + 1u);
    d_tests_check(d_tests_refused(doc, d_ninja_doc_set_statement(
                                           doc, 0u,
                                           "rule r\n  command = c\n"),
                                  text, "not one variable"),
                  "a statement replaced by one of another kind is refused");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_insert_statements(
                                           doc, 0u, "  x = 1\n"),
                                  text, "not statements"),
                  "  ...and a text that does not read alone");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_insert_statements(
                                           doc, 0u, "# only\n"),
                                  text, "has none"),
                  "  ...and one with no statement");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_remove_statements(doc, 1u,
                                                                     0u),
                                  text, "no statements 2 to 1"),
                  "  ...and a run backwards");
    d_tests_check(d_tests_refused(doc, d_ninja_doc_insert_statements(
                                           doc, 5u, "x = 1\n"),
                                  text, "no statement 6"),
                  "  ...and a place the file has not");
    d_ninja_doc_free(doc);

    doc = d_tests_doc("");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_insert_statements(
                                          doc, 0u, "b = 2"),
                                 "b = 2\n")),
                   "a statement inserted into an empty file");
    d_ninja_doc_free(doc);
    doc = d_tests_doc("build x: phony\n  y = 1\nz = 2\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_set_statement(
                                          doc, 0u, "build w: phony x\n"),
                                 "build w: phony x\nz = 2\n")),
                   "a build replaced, its bindings with it");
    d_ninja_doc_free(doc);
    doc = d_tests_doc("a = 1\r\nb = 2\r\n");
    d_tests_check( (doc) &&
                   (d_tests_kept(doc, d_ninja_doc_insert_statements(
                                          doc, 1u,
                                          "rule r\n  command = c\n"),
                                 "a = 1\r\nrule r\r\n  command = c\r\n"
                                 "b = 2\r\n")),
                   "statements inserted into a CR LF file, with its endings");
    d_ninja_doc_free(doc);

    return;
}


/*
d_tests_saving
  Tests the following:
  - a document saved, its directories made; not written again unchanged
  - saved after a change, and to another path; read back as it was saved
*/
static void
d_tests_saving(
    const char* _work
)
{
    char                path[1024];
    char                other[1024];
    char                why[256];
    struct d_ninja_doc* doc = NULL;

    (void)snprintf(path, sizeof(path), "%s/ninja/saved/deep/build.ninja",
                   _work);
    (void)snprintf(other, sizeof(other), "%s/ninja/saved/other.ninja", _work);
    (void)remove(path);
    (void)remove(other);
    doc = d_ninja_doc_make(path, "a = 1\n", 6u, why, sizeof(why));

    // what follows needs the document
    if (!doc)
    {
        d_tests_check(false, "a document to save");

        return;
    }

    d_tests_check(d_ninja_doc_save(doc, NULL, why, sizeof(why)) ==
                  D_NINJA_SAVE_WRITTEN,
                  "a document saved, its directories made");
    d_tests_check(d_ninja_doc_save(doc, "", why, sizeof(why)) ==
                  D_NINJA_SAVE_UNCHANGED,
                  "  ...and not written again unchanged");
    d_tests_check( (d_ninja_doc_set_value(doc, 0u, "2")) &&
                   (d_ninja_doc_save(doc, NULL, why, sizeof(why)) ==
                    D_NINJA_SAVE_WRITTEN) &&
                   (d_ninja_doc_save(doc, other, why, sizeof(why)) ==
                    D_NINJA_SAVE_WRITTEN),
                   "  ...written after a change, and to another path");

    struct d_ninja_doc* const back = d_ninja_doc_read(other, why,
                                                      sizeof(why));

    d_tests_check( (back) &&
                   (d_tests_text_is(back, "a = 2\n")),
                   "a file saved reads back as it was saved");
    d_ninja_doc_free(back);
    d_tests_check( (!d_ninja_doc_read("/nonexistent/x.ninja", why,
                                      sizeof(why))) &&
                   (strstr(why, "cannot read") != NULL),
                   "a file that is not there makes no document");
    d_ninja_doc_free(doc);

    return;
}


//==============================================================================
// 5.  FUZZING
//==============================================================================


// d_tests_fuzz
//   struct: a fuzz run: its random state, its rounds, and what it found.
struct d_tests_fuzz
{
    unsigned long state;
    unsigned long rounds;
    size_t        files;
    size_t        kept;
    size_t        refused;
    size_t        failed;
};


/*
d_tests_random
  A number below a bound, from the run's state.
*/
static size_t
d_tests_random(
    struct d_tests_fuzz* _fuzz,
    size_t               _bound
)
{
    _fuzz->state = (_fuzz->state * 1103515245ul + 12345ul) & 0x7fffffffUL;

    return (_bound > 0u) ? ((size_t)(_fuzz->state >> 8) % _bound) : 0u;
}


/*
d_tests_pick
  One of a list of texts, at random.
*/
static const char*
d_tests_pick(
    struct d_tests_fuzz* _fuzz,
    const char* const*   _texts,
    size_t               _count
)
{
    return _texts[d_tests_random(_fuzz, _count)];
}


/*
d_tests_one_change
  One change at random, of each kind the document takes.
  Long as it is a table of the changes, each a call.
*/
static bool
d_tests_one_change(
    struct d_ninja_doc*  _doc,
    struct d_tests_fuzz* _fuzz
)
{
    static const char* const texts[] =
    {
        "x", "", "$in", "a$ b", "${v} w", "$", " lead", "a\nb = 1", "a:b",
        "a | b", "$$", "-O2 $\n    -g"
    };
    static const char* const names[] =
    {
        "cc", "x", "command", "description", "a b", "", "depth", "phony",
        "pool", "cflags"
    };
    static const char* const statements[] =
    {
        "x = 1\n", "rule r\n  command = c\n", "build o: phony i\n",
        "default o\n", "  y = 2\n", "pool p\n  depth = 2\n", "# c\n",
        "build o: r\n  x = 1\n", "include i.ninja\n"
    };
    static const char* const raw[] =
    {
        "", " ", "\n", "  ", "#", "$", "$\n", "|", ":", "x", "\n  x = 1\n",
        "build"
    };
    const size_t count  = d_ninja_doc_records(_doc)->statement_count;
    const size_t n      = d_tests_random(_fuzz, count + 2u);
    const size_t k      = d_tests_random(_fuzz, 6u);
    const char*  text   = d_tests_pick(_fuzz, texts, 12u);
    const char*  name   = d_tests_pick(_fuzz, names, 10u);
    size_t       length = 0;

    (void)d_ninja_doc_text(_doc, &length);

    // a table of the changes
    switch (d_tests_random(_fuzz, 12u))
    {
        case 0:
            return d_ninja_doc_set_value(_doc, n, text);

        case 1:
            return d_ninja_doc_set_name(_doc, n, name);

        case 2:
            return d_ninja_doc_set_binding(_doc, n, name, text);

        case 3:
            return d_ninja_doc_remove_binding(_doc, n, name);

        case 4:
            return d_ninja_doc_set_path(_doc, n, k, text);

        case 5:
            return d_ninja_doc_insert_path(
                       _doc, n, (enum d_ninja_section)d_tests_random(_fuzz,
                                                                     8u),
                       d_tests_random(_fuzz, 3u), text);

        case 6:
            return d_ninja_doc_remove_path(_doc, n, k);

        case 7:
            return d_ninja_doc_insert_statements(
                       _doc, n, d_tests_pick(_fuzz, statements, 9u));

        case 8:
            return d_ninja_doc_set_statement(
                       _doc, n, d_tests_pick(_fuzz, statements, 9u));

        case 9:
            return d_ninja_doc_remove_statements(_doc, n,
                                                 n + d_tests_random(_fuzz,
                                                                    3u));

        default:
            break;
    }

    const size_t at    = d_tests_random(_fuzz, length + 1u);
    const size_t end   = at + d_tests_random(_fuzz, 4u);
    const char*  piece = d_tests_pick(_fuzz, raw, 12u);

    return d_ninja_doc_splice(_doc, at, (end <= length) ? end : length, piece,
                              strlen(piece));
}


/*
d_tests_same_statement
  Whether two statements, each of its own reading, are the same bytes, of
the same kind, with as many paths and bindings, each path in the same
section.
*/
static bool
d_tests_same_statement(
    const struct d_ninja* _old,
    size_t                _a,
    const struct d_ninja* _new,
    size_t                _b
)
{
    const struct d_ninja_statement* const a = &_old->statements[_a];
    const struct d_ninja_statement* const b = &_new->statements[_b];

    // the same bytes, kind and counts
    if ( ((a->end - a->start) != (b->end - b->start)) ||
         (memcmp(&_old->text[a->start], &_new->text[b->start],
                 a->end - a->start) != 0) ||
         (a->kind != b->kind) ||
         (a->path_count != b->path_count) ||
         (a->binding_count != b->binding_count) )
    {
        return false;
    }

    // each path in its section
    for (size_t at = 0; at < a->path_count; ++at)
    {
        // the same section
        if (_old->paths[a->first_path + at].section !=
            _new->paths[b->first_path + at].section)
        {
            return false;
        }
    }

    return true;
}


/*
d_tests_same_outside
  Whether two readings agree on the statements and the comments outside a
change, found here afresh: as many wholly before `_start` in each, and
wholly after its end in each, each the same.
  Long as the statements and then the comments are placed and compared.
*/
static bool
d_tests_same_outside(
    const struct d_ninja* _old,
    const struct d_ninja* _new,
    size_t                _start,
    size_t                _old_end,
    size_t                _new_end
)
{
    size_t before = 0;
    size_t old_at = 0;
    size_t new_at = 0;

    // the statements before, in the old
    while ( (before < _old->statement_count) &&
            (_old->statements[before].end <= _start) )
    {
        before++;
    }

    old_at = before;
    new_at = before;

    // past those cut, in the old
    while ( (old_at < _old->statement_count) &&
            (_old->statements[old_at].start < _old_end) )
    {
        old_at++;
    }

    // ...and in the new
    while ( (new_at < _new->statement_count) &&
            (_new->statements[new_at].start < _new_end) )
    {
        new_at++;
    }

    // as many after in each, and the new at least as many before
    if ( ((_old->statement_count - old_at) !=
          (_new->statement_count - new_at)) ||
         (before > _new->statement_count) )
    {
        return false;
    }

    // each before, then each after, the same
    for (size_t at = 0; at < _old->statement_count; ++at)
    {
        const bool   inside = ( (at >= before) && (at < old_at) );
        const size_t other  = (at < before) ? at : (at - old_at + new_at);

        // one the change cut is the change's to answer for
        if ( (!inside) &&
             (!d_tests_same_statement(_old, at, _new, other)) )
        {
            return false;
        }
    }

    size_t first = 0;
    size_t old_c = 0;
    size_t new_c = 0;

    // the comments before, in each
    while ( (first < _old->comment_count) &&
            (_old->comments[first].end <= _start) )
    {
        first++;
    }

    old_c = first;
    new_c = first;

    // past those cut, in each
    while ( (old_c < _old->comment_count) &&
            (_old->comments[old_c].start < _old_end) )
    {
        old_c++;
    }

    // ...and in the new
    while ( (new_c < _new->comment_count) &&
            (_new->comments[new_c].start < _new_end) )
    {
        new_c++;
    }

    // as many after in each, and the new at least as many before
    if ( (first > _new->comment_count) ||
         ((_old->comment_count - old_c) != (_new->comment_count - new_c)) )
    {
        return false;
    }

    // each before, then each after, the same bytes
    for (size_t at = 0; at < _old->comment_count; ++at)
    {
        const bool   inside = ( (at >= first) && (at < old_c) );
        const size_t other  = (at < first) ? at : (at - old_c + new_c);
        const struct d_ninja_comment* const a = &_old->comments[at];
        const struct d_ninja_comment* const b = &_new->comments[other];

        // one the change cut is the change's to answer for
        if ( (!inside) &&
             ( ((a->end - a->start) != (b->end - b->start)) ||
               (memcmp(&_old->text[a->start], &_new->text[b->start],
                       a->end - a->start) != 0) ) )
        {
            return false;
        }
    }

    return true;
}


/*
d_tests_fuzz_holds
  Whether a change did what it says: kept, the text reads, the bytes
before and after where it was are the text's own, and so are the
statements outside it; refused, the text as it was.
*/
static bool
d_tests_fuzz_holds(
    const struct d_ninja_doc* _doc,
    bool                      _kept,
    const char*               _before,
    size_t                    _before_length
)
{
    size_t            length  = 0;
    const char* const text    = d_ninja_doc_text(_doc, &length);
    size_t            start   = 0;
    size_t            old_end = 0;
    size_t            new_end = 0;

    // refused: the text as it was
    if (!_kept)
    {
        return ( (length == _before_length) &&
                 (memcmp(text, _before, length) == 0) );
    }

    struct d_ninja old;
    struct d_ninja now;

    // each released whether or not it was read
    memset(&old, 0, sizeof(old));
    memset(&now, 0, sizeof(now));

    const bool     read_now = d_ninja_read(&now, text, length, NULL, 0u);
    const bool     read_old = d_ninja_read(&old, _before, _before_length,
                                           NULL, 0u);
    const bool     held     = (read_now) &&
                              (read_old) &&
                              (d_ninja_doc_changed(_doc, &start, &old_end,
                                                   &new_end)) &&
                              (start <= old_end) &&
                              (old_end <= _before_length) &&
                              (new_end <= length) &&
                              ((length - new_end) ==
                                   (_before_length - old_end)) &&
                              (memcmp(text, _before, start) == 0) &&
                              (memcmp(&text[new_end], &_before[old_end],
                                      length - new_end) == 0) &&
                              (d_tests_same_outside(&old, &now, start,
                                                    old_end, new_end));

    d_ninja_release(&old);
    d_ninja_release(&now);

    return held;
}


/*
d_tests_fuzz_file
  Makes the run's changes to one file.
*/
static void
d_tests_fuzz_file(
    const char*          _path,
    struct d_tests_fuzz* _fuzz
)
{
    char                      why[256];
    struct d_ninja_doc* const doc = d_ninja_doc_read(_path, why, sizeof(why));

    // a file that does not read is the reader's to answer for
    if (!doc)
    {
        return;
    }

    _fuzz->files++;

    // each round, a change and its proof
    for (unsigned long round = 0; round < _fuzz->rounds; ++round)
    {
        size_t            length = 0;
        const char* const now    = d_ninja_doc_text(doc, &length);
        char* const       before = malloc(length + 1u);

        // the text before, to compare with
        if (!before)
        {
            break;
        }

        memcpy(before, now, length + 1u);

        const bool kept = d_tests_one_change(doc, _fuzz);

        _fuzz->kept    += (kept) ? 1u : 0u;
        _fuzz->refused += (kept) ? 0u : 1u;

        // a change that did not do what it says
        if (!d_tests_fuzz_holds(doc, kept, before, length))
        {
            _fuzz->failed++;
            printf("  FAIL  %s: round %lu, %s\n", _path, round,
                   (kept) ? "kept" : "refused");
        }

        free(before);
    }

    d_ninja_doc_free(doc);

    return;
}


/*
d_tests_is_ninja
  Whether a file name is a Ninja file's: *.ninja.
*/
static bool
d_tests_is_ninja(
    const char* _name
)
{
    const size_t span = strlen(_name);

    return ( (span > 6u) &&
             (strcmp(&_name[span - 6u], ".ninja") == 0) );
}


/*
d_tests_fuzz_walk
  Every Ninja file under a directory, in name order.
*/
static void
d_tests_fuzz_walk(
    const char*          _directory,
    struct d_tests_fuzz* _fuzz
)
{
    char** names = NULL;
    size_t count = 0;

    // a directory that cannot be listed holds nothing to change
    if (!d_fs_names(_directory, &names, &count))
    {
        return;
    }

    // each name, a directory walked and a Ninja file changed
    for (size_t at = 0; at < count; ++at)
    {
        const size_t span = strlen(_directory) + strlen(names[at]) + 2u;
        char* const  path = malloc(span);

        // a path that cannot be made is passed over
        if (!path)
        {
            continue;
        }

        (void)snprintf(path, span, "%s/%s", _directory, names[at]);

        // a table of what the name is
        if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
        {
            d_tests_fuzz_walk(path, _fuzz);
        }
        else if (d_tests_is_ninja(names[at]))
        {
            d_tests_fuzz_file(path, _fuzz);
        }

        free(path);
    }

    d_fs_names_free(names, count);

    return;
}


/*
main
  The unit checks, writing into the work directory given or the one the
process is in; with --fuzz, changes at random to every Ninja file under a
directory.
*/
int
main(
    int    argc,
    char** argv
)
{
    // a fuzz run changes files in memory and reports them, and nothing else
    if ( (argc > 2) &&
         (strcmp(argv[1], "--fuzz") == 0) )
    {
        struct d_tests_fuzz fuzz;

        memset(&fuzz, 0, sizeof(fuzz));
        fuzz.state  = (argc > 3) ? strtoul(argv[3], NULL, 10) : 1ul;
        fuzz.rounds = (argc > 4) ? strtoul(argv[4], NULL, 10) : 200ul;
        d_tests_fuzz_walk(argv[2], &fuzz);
        printf("\n%lu files, %lu changes kept, %lu refused: %lu failed\n",
               (unsigned long)fuzz.files, (unsigned long)fuzz.kept,
               (unsigned long)fuzz.refused, (unsigned long)fuzz.failed);

        return ( (fuzz.failed == 0u) && (fuzz.files > 0u) ) ? 0 : 1;
    }

    const char* const work = (argc > 1) ? argv[1] : ".";

    d_tests_reading();
    d_tests_reader_refusals();
    d_tests_writers();
    d_tests_loading();
    d_tests_load_refusals();
    d_tests_scopes(work);
    d_tests_documents();
    d_tests_values();
    d_tests_bindings();
    d_tests_paths();
    d_tests_statements();
    d_tests_saving(work);

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
