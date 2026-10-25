/*******************************************************************************
* djinterp [tools]                                             test_dcmake_doc.c
*
* Exercises `dcmake_doc.h`: every change a CMake file takes, the layout each
* takes from the file, what each refuses, and saving.
*   With --fuzz, it makes changes at random to every CMake file under a
* directory instead -- arguments set, inserted and removed, commands renamed,
* replaced, inserted and removed, and raw splices -- and proves of each that
* a change kept left the file readable and every byte outside it as it was,
* and that a change refused left the file as it was.
*
*   test_dcmake_doc [work directory]          the unit checks
*   test_dcmake_doc --fuzz <dir> [seed] [n]   n changes to every file
*
* path:      /tests/djinterp/tools/dawk/ext/cmake/test_dcmake_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dcmake_doc.h"  // corresponding header
// std
#include <stdio.h>   // printf, snprintf, fopen, fwrite, fclose, remove
#include <stdlib.h>  // malloc, free, strtoul
#include <string.h>  // memcmp, memset, strcmp, strlen, strstr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"      // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"  // d_text_read


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
d_tests_doc
  A document of a text that must read.
*/
static struct d_cmake_doc*
d_tests_doc(
    const char* _text
)
{
    char                      why[256];
    struct d_cmake_doc* const doc = d_cmake_doc_make("t.cmake", _text,
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
    const struct d_cmake_doc* _doc,
    const char*               _expected
)
{
    size_t            length = 0;
    const char* const text   = d_cmake_doc_text(_doc, &length);
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
d_tests_refused
  Whether a change was refused, the document unchanged and saying why.
*/
static bool
d_tests_refused(
    const struct d_cmake_doc* _doc,
    bool                      _kept,
    const char*               _before,
    const char*               _why
)
{
    const bool said = (strstr(d_cmake_doc_problem(_doc), _why) != NULL);

    // a refusal that says something else is printed
    if (!said)
    {
        printf("    why: %s\n", d_cmake_doc_problem(_doc));
    }

    return ( (!_kept) &&
             (said) &&
             (d_tests_text_is(_doc, _before)) );
}


/*
d_tests_documents
  Tests the following:
  - a document made of a text, and of a text CMake would refuse
  - its path, its text, its changes, and where the last change was
  - a raw splice kept, and one refused for what it does outside it
*/
static void
d_tests_documents(void)
{
    char                      why[256];
    struct d_cmake_doc* const bad = d_cmake_doc_make("b.cmake", "set(a",
                                                     5u, why, sizeof(why));
    size_t                    start   = 0;
    size_t                    old_end = 0;
    size_t                    new_end = 0;

    d_tests_check(!bad, "a text CMake would refuse makes no document");
    d_tests_check(strstr(why, "b.cmake") != NULL,
                  "...and the refusal names the file");

    struct d_cmake_doc* const doc = d_tests_doc("set(a b)\nset(c)\n");

    // nothing more is checked of a document not made
    if (!doc)
    {
        return;
    }

    d_tests_check(strcmp(d_cmake_doc_path(doc), "t.cmake") == 0,
                  "a document keeps its path");
    d_tests_check( (d_cmake_doc_changes(doc) == 0u) &&
                   (!d_cmake_doc_changed(doc, &start, &old_end, &new_end)),
                  "...and has taken no change");
    d_tests_check(d_cmake_doc_splice(doc, 6u, 7u, "bb", 2u),
                  "a splice inside an argument is kept");
    d_tests_check( (d_tests_text_is(doc, "set(a bb)\nset(c)\n")) &&
                   (d_cmake_doc_changes(doc) == 1u) &&
                   (d_cmake_doc_changed(doc, &start, &old_end, &new_end)) &&
                   (start == 6u) && (old_end == 7u) && (new_end == 8u),
                  "...and counted, where it was");
    d_tests_check(d_tests_refused(doc,
                                  d_cmake_doc_splice(doc, 4u, 4u, "\"", 1u),
                                  "set(a bb)\nset(c)\n", "unreadable"),
                  "a splice that leaves the file unreadable is refused");
    d_tests_check(d_tests_refused(doc,
                                  d_cmake_doc_splice(doc, 17u, 17u,
                                                     "if(X)\n", 6u),
                                  "set(a bb)\nset(c)\n", "unpaired"),
                  "...as is one that leaves a block unpaired");
    d_tests_check(d_tests_refused(doc,
                                  d_cmake_doc_splice(doc, 10u, 10u,
                                                     "if(X)\n", 6u),
                                  "set(a bb)\nset(c)\n", "line 2, outside"),
                  "...and one that changes a command outside it, by its "
                  "line");

    struct d_cmake_doc* const block = d_tests_doc("if(A)\nset(a)\nendif()\n");

    // a splice that would change the pairing of the commands outside it
    if (block)
    {
        d_tests_check(d_tests_refused(block,
                                      d_cmake_doc_splice(block, 6u, 6u,
                                                         "endif()\n", 8u),
                                      "if(A)\nset(a)\nendif()\n",
                                      "outside it"),
                      "...and one that changes a command outside it");
        d_cmake_doc_free(block);
    }

    d_cmake_doc_free(doc);

    return;
}


/*
d_tests_set_argument
  Tests the following:
  - an argument set in place, quoted and bracket ones among them
  - a text set apart from a neighbour written against it
  - a text with a line feed written with the file's CR LF
  - a text that is not one argument, a parenthesis, and an argument not
    there, each refused
*/
static void
d_tests_set_argument(void)
{
    struct d_cmake_doc* const doc =
        d_tests_doc("set(a \"b\" [[c]])\nset(y \"a\"\"b\")\nif((A) OR B)\n"
                    "endif()\n");

    // nothing is checked of a document not made
    if (!doc)
    {
        return;
    }

    d_tests_check( (d_cmake_doc_set_argument(doc, 0u, 1u, "\"B C\"")) &&
                   (d_cmake_doc_set_argument(doc, 0u, 2u, "[=[d]]e]=]")) &&
                   (d_cmake_doc_set_argument(doc, 0u, 0u, "${x}")),
                  "arguments are set in place");
    d_tests_check(d_cmake_doc_set_argument(doc, 1u, 1u, "c"),
                  "an argument written against another is set");
    d_tests_check(d_tests_text_is(doc, "set(${x} \"B C\" [=[d]]e]=])\n"
                                       "set(y c \"b\")\nif((A) OR B)\n"
                                       "endif()\n"),
                  "...set apart from it, and the rest as it was");
    d_tests_check(d_tests_refused(doc,
                                  d_cmake_doc_set_argument(doc, 0u, 0u,
                                                           "a b"),
                                  "set(${x} \"B C\" [=[d]]e]=])\n"
                                  "set(y c \"b\")\nif((A) OR B)\nendif()\n",
                                  "not one argument: a b"),
                  "two arguments for one are refused");
    d_tests_check( (!d_cmake_doc_set_argument(doc, 0u, 0u, "# c")) &&
                   (!d_cmake_doc_set_argument(doc, 0u, 0u, "(")) &&
                   (!d_cmake_doc_set_argument(doc, 0u, 0u, "")) &&
                   (!d_cmake_doc_set_argument(doc, 0u, 0u, "\"open")),
                  "...as are a comment, a parenthesis, nothing, and a "
                  "quote left open");
    d_tests_check( (!d_cmake_doc_set_argument(doc, 2u, 0u, "x")) &&
                   (strstr(d_cmake_doc_problem(doc), "parenthesis") != NULL),
                  "a parenthesis is not set alone");
    d_tests_check( (!d_cmake_doc_set_argument(doc, 0u, 9u, "x")) &&
                   (!d_cmake_doc_set_argument(doc, 9u, 0u, "x")) &&
                   (strstr(d_cmake_doc_problem(doc), "command 10 has no "
                                                     "argument 1") != NULL),
                  "an argument not there is refused, by its number from 1");
    d_cmake_doc_free(doc);

    struct d_cmake_doc* const crlf = d_tests_doc("set(a\r\n    b)\r\n");

    // a CR LF file's text takes CR LF
    if (crlf)
    {
        d_tests_check( (d_cmake_doc_set_argument(crlf, 0u, 1u,
                                                 "\"x\ny\"")) &&
                       (d_tests_text_is(crlf, "set(a\r\n    \"x\r\ny\")\r\n")),
                      "a line feed is written as the file ends its lines");
        d_cmake_doc_free(crlf);
    }

    return;
}


/*
d_tests_insert_argument
  Tests the following:
  - before an argument and after one, on a line of its own or beside it
  - after an argument whose line a comment ends, past the comment
  - appended after the last, and into an empty list
  - beside a parenthesis, and a place not there refused
*/
static void
d_tests_insert_argument(void)
{
    struct d_cmake_doc* const doc =
        d_tests_doc("add_library(lib STATIC\n    \"a.c\"\n    # a comment\n"
                    "    \"c.c\" # trailing\n    LINK x)\nset()\n"
                    "set(a b c)\nif((A) OR B)\nendif()\n");

    // nothing is checked of a document not made
    if (!doc)
    {
        return;
    }

    d_tests_check( (d_cmake_doc_insert_argument(doc, 0u, 2u, "\"b.c\"",
                                                true)) &&
                   (d_cmake_doc_insert_argument(doc, 0u, 4u, "\"d.c\"",
                                                true)) &&
                   (d_cmake_doc_insert_argument(doc, 1u, 0u, "x", false)) &&
                   (d_cmake_doc_insert_argument(doc, 2u, 0u, "first",
                                                false)) &&
                   (d_cmake_doc_insert_argument(doc, 2u, 4u, "last",
                                                false)) &&
                   (d_cmake_doc_insert_argument(doc, 3u, 2u, "AND", true)) &&
                   (d_cmake_doc_insert_argument(doc, 3u, 1u, "NOT", false)),
                  "arguments are inserted at every kind of place");
    d_tests_check(d_tests_text_is(doc,
                                  "add_library(lib STATIC\n    \"a.c\"\n"
                                  "    \"b.c\"\n    # a comment\n"
                                  "    \"c.c\" # trailing\n    \"d.c\"\n"
                                  "    LINK x)\nset(x)\n"
                                  "set(first a b c last)\n"
                                  "if((NOT A) AND OR B)\nendif()\n"),
                  "...each in its neighbour's layout");
    d_tests_check( (!d_cmake_doc_insert_argument(doc, 1u, 1u, "y", true)) &&
                   (!d_cmake_doc_insert_argument(doc, 1u, 2u, "y", false)) &&
                   (!d_cmake_doc_insert_argument(doc, 4u, 0u, "y", true)),
                  "a place the command does not have is refused");
    d_tests_check(!d_cmake_doc_insert_argument(doc, 1u, 0u, "a b", false),
                  "...and two arguments for one");
    d_cmake_doc_free(doc);

    return;
}


/*
d_tests_remove_argument
  Tests the following:
  - an argument alone on its line, with its line and the comment ending it
  - one after another on its line, and one first on a line it shares
  - the last, before the `)`, which closes up to the argument before
  - a parenthesis refused
*/
static void
d_tests_remove_argument(void)
{
    struct d_cmake_doc* const doc =
        d_tests_doc("set(a\n    b # why\n    c d\n)\nset(x y z)\n"
                    "set(p\n    q)\nset(\n    only)\nif((A) OR B)\n"
                    "endif()\n");

    // nothing is checked of a document not made
    if (!doc)
    {
        return;
    }

    d_tests_check( (d_cmake_doc_remove_argument(doc, 0u, 1u)) &&
                   (d_cmake_doc_remove_argument(doc, 0u, 2u)) &&
                   (d_cmake_doc_remove_argument(doc, 1u, 0u)) &&
                   (d_cmake_doc_remove_argument(doc, 1u, 1u)) &&
                   (d_cmake_doc_remove_argument(doc, 2u, 1u)) &&
                   (d_cmake_doc_remove_argument(doc, 3u, 0u)),
                  "arguments are removed from every kind of place");
    d_tests_check(d_tests_text_is(doc, "set(a\n    c\n)\nset(y)\nset(p)\n"
                                       "set()\nif((A) OR B)\nendif()\n"),
                  "...each with the space about it");
    d_tests_check( (!d_cmake_doc_remove_argument(doc, 4u, 0u)) &&
                   (strstr(d_cmake_doc_problem(doc), "parenthesis") != NULL),
                  "a parenthesis is not removed alone");
    d_cmake_doc_free(doc);

    return;
}


/*
d_tests_commands
  Tests the following:
  - a command renamed, its role kept, and one refused that would change it
  - a command replaced, and one replaced by two refused
  - a run of commands removed, a whole block among them, and an opener
    alone refused
*/
static void
d_tests_commands(void)
{
    struct d_cmake_doc* const doc =
        d_tests_doc("set(a)\nif(A)\n    message(x) # why\nendif()\n"
                    "set(b)\n");

    // nothing is checked of a document not made
    if (!doc)
    {
        return;
    }

    d_tests_check( (d_cmake_doc_set_name(doc, 0u, "SET")) &&
                   (d_cmake_doc_set_name(doc, 3u, "ENDIF")),
                  "commands are renamed");
    d_tests_check( (!d_cmake_doc_set_name(doc, 2u, "else")) &&
                   (strstr(d_cmake_doc_problem(doc), "part the command "
                                                     "takes") != NULL),
                  "a rename that would change its part in a block is "
                  "refused");
    d_tests_check( (!d_cmake_doc_set_name(doc, 1u, "while")) &&
                   (!d_cmake_doc_set_name(doc, 0u, "1x")) &&
                   (!d_cmake_doc_set_name(doc, 0u, "a b")),
                  "...as are another opener, and a name no command has");
    d_tests_check(d_cmake_doc_set_command(doc, 2u, "message(STATUS \"y\")"),
                  "a command is replaced");
    d_tests_check( (!d_cmake_doc_set_command(doc, 2u, "a() b()")) &&
                   (!d_cmake_doc_set_command(doc, 2u, "a() # c")) &&
                   (!d_cmake_doc_set_command(doc, 2u, "endif()")) &&
                   (!d_cmake_doc_set_command(doc, 2u, "a(")),
                  "...but not by two, a comment, another part, or nothing "
                  "CMake reads");
    d_tests_check(d_tests_text_is(doc, "SET(a)\nif(A)\n"
                                       "    message(STATUS \"y\") # why\n"
                                       "ENDIF()\nset(b)\n"),
                  "...and the rest as it was");
    d_tests_check( (!d_cmake_doc_remove_commands(doc, 1u, 1u)) &&
                   (!d_cmake_doc_remove_commands(doc, 2u, 1u)) &&
                   (!d_cmake_doc_remove_commands(doc, 4u, 5u)),
                  "an opener alone, and a run not there, are not removed");
    d_tests_check( (d_cmake_doc_remove_commands(doc, 1u, 3u)) &&
                   (d_tests_text_is(doc, "SET(a)\nset(b)\n")),
                  "a whole block is removed, its lines with it");
    d_tests_check( (d_cmake_doc_remove_commands(doc, 1u, 1u)) &&
                   (d_tests_text_is(doc, "SET(a)\n")),
                  "...and the last command");
    d_cmake_doc_free(doc);

    return;
}


/*
d_tests_insert_commands
  Tests the following:
  - before a command, above the comments that lead into it
  - in a block, indented as its commands; in an empty block, a step past
    its opener, the step the file's own
  - at the end of a file, of one with no last line ending, and of an empty
    one
  - text of its own indentation taken off, and a multi-line argument left
    as it is
  - a block left unpaired refused
*/
static void
d_tests_insert_commands(void)
{
    struct d_cmake_doc* const doc =
        d_tests_doc("project(p)\n\n# the flags\n# for all\nset(f)\n"
                    "if(A)\n  set(x)\nendif()\nwhile(B)\nendwhile()");

    // nothing is checked of a document not made
    if (!doc)
    {
        return;
    }

    d_tests_check(d_cmake_doc_insert_commands(doc, 1u, "set(e)"),
                  "commands are inserted before a command");
    d_tests_check(d_cmake_doc_insert_commands(doc, 5u,
                                              "  message(\"a\n  b\")\n"
                                              "  set(y)\n"),
                  "...and at the end of a block");
    d_tests_check(d_cmake_doc_insert_commands(doc, 9u, "set(w)"),
                  "...and into an empty block");
    d_tests_check(d_cmake_doc_insert_commands(doc, 11u, "set(z)\n"),
                  "...and after the last");
    d_tests_check(d_tests_text_is(doc, "project(p)\n\nset(e)\n# the flags\n"
                                       "# for all\nset(f)\nif(A)\n"
                                       "  set(x)\n  message(\"a\n  b\")\n"
                                       "  set(y)\nendif()\nwhile(B)\n"
                                       "  set(w)\nendwhile()\nset(z)\n"),
                  "...each on lines of its own, indented as its block's");
    d_tests_check( (!d_cmake_doc_insert_commands(doc, 12u, "if(C)")) &&
                   (strstr(d_cmake_doc_problem(doc), "unpaired") != NULL),
                  "a block left open is refused");
    d_tests_check( (!d_cmake_doc_insert_commands(doc, 0u, "endif()")) &&
                   (!d_cmake_doc_insert_commands(doc, 0u, "# only")) &&
                   (!d_cmake_doc_insert_commands(doc, 0u, "a(")) &&
                   (!d_cmake_doc_insert_commands(doc, 99u, "a()")),
                  "...as are a block closed twice, no command, nothing CMake "
                  "reads, and a place not there");
    d_cmake_doc_free(doc);

    struct d_cmake_doc* const empty = d_tests_doc("");

    // an empty file takes its first command
    if (empty)
    {
        d_tests_check( (d_cmake_doc_insert_commands(empty, 0u,
                                                    "if(A)\n  set(a)\n"
                                                    "endif()")) &&
                       (d_tests_text_is(empty, "if(A)\n  set(a)\nendif()\n")),
                      "an empty file takes a block");
        d_cmake_doc_free(empty);
    }

    struct d_cmake_doc* const crlf = d_tests_doc("set(a)\r\n");

    // a CR LF file's lines end so
    if (crlf)
    {
        d_tests_check( (d_cmake_doc_insert_commands(crlf, 1u, "set(b)\n")) &&
                       (d_tests_text_is(crlf, "set(a)\r\nset(b)\r\n")),
                      "a CR LF file's new lines end in CR LF");
        d_cmake_doc_free(crlf);
    }

    return;
}


/*
d_tests_indent
  Tests the following:
  - the indentation of a slot, from the neighbours about it
*/
static void
d_tests_indent(void)
{
    struct d_cmake_doc* const doc =
        d_tests_doc("function(f)\n\tif(A)\n\tendif()\nendfunction()\n");
    char                      indent[16];

    // nothing is checked of a document not made
    if (!doc)
    {
        return;
    }

    d_tests_check( (d_cmake_doc_indent(doc, 0u, indent, sizeof(indent)) ==
                    0u) &&
                   (d_cmake_doc_indent(doc, 1u, indent, sizeof(indent)) ==
                    1u) &&
                   (indent[0] == '\t') &&
                   (d_cmake_doc_indent(doc, 2u, indent, sizeof(indent)) ==
                    2u) &&
                   (strcmp(indent, "\t\t") == 0) &&
                   (d_cmake_doc_indent(doc, 3u, indent, sizeof(indent)) ==
                    1u) &&
                   (d_cmake_doc_indent(doc, 4u, indent, sizeof(indent)) ==
                    0u),
                  "a slot is indented as its block, a tab the file's step");
    d_cmake_doc_free(doc);

    return;
}


/*
d_tests_saving
  Tests the following:
  - a file written, then left as it is when it holds the text already
  - a file written to another path, its directories made
  - a document read back from what was written
*/
static void
d_tests_saving(
    const char* _work
)
{
    char path[512];
    char other[512];
    char why[256];

    (void)snprintf(path, sizeof(path), "%s/doc/saved.cmake", _work);
    (void)snprintf(other, sizeof(other), "%s/doc/deep/er/other.cmake",
                   _work);

    // what an earlier run saved would be left as it is, and not written
    (void)remove(path);
    (void)remove(other);

    struct d_cmake_doc* const doc = d_cmake_doc_make(path, "set(a)\n", 7u,
                                                     why, sizeof(why));

    // nothing is checked of a document not made
    if (!doc)
    {
        d_tests_check(false, "a document is made to save");

        return;
    }

    d_tests_check(d_cmake_doc_save(doc, NULL, why, sizeof(why)) ==
                      D_CMAKE_SAVE_WRITTEN,
                  "a document is written, its directory made");
    d_tests_check(d_cmake_doc_save(doc, "", why, sizeof(why)) ==
                      D_CMAKE_SAVE_UNCHANGED,
                  "...and left as it is when the file holds it");
    d_tests_check( (d_cmake_doc_set_argument(doc, 0u, 0u, "b")) &&
                   (d_cmake_doc_save(doc, other, why, sizeof(why)) ==
                        D_CMAKE_SAVE_WRITTEN),
                  "a document is written to another path");

    struct d_cmake_doc* const back = d_cmake_doc_read(other, why,
                                                      sizeof(why));

    d_tests_check( (back) &&
                   (d_tests_text_is(back, "set(b)\n")),
                  "...and read back as it was written");
    d_cmake_doc_free(back);
    d_cmake_doc_free(doc);

    struct d_cmake_doc* const none = d_cmake_doc_make("", "", 0u, why,
                                                      sizeof(why));

    d_tests_check( (none) &&
                   (d_cmake_doc_save(none, NULL, why, sizeof(why)) ==
                        D_CMAKE_SAVE_FAILED),
                  "a document with no path is not saved");
    d_cmake_doc_free(none);

    return;
}


//==============================================================================
// THE FUZZ
//==============================================================================


// d_tests_fuzz
//   struct: a run's random state and what it counted.
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
  A number below `_limit`, from a linear congruential generator; 0 for a
limit of 0.
*/
static size_t
d_tests_random(
    struct d_tests_fuzz* _fuzz,
    size_t               _limit
)
{
    _fuzz->state = (_fuzz->state * 1103515245ul) + 12345ul;

    return (_limit > 0u)
               ? (size_t)((_fuzz->state >> 16) % (unsigned long)_limit)
               : 0u;
}


/*
d_tests_one_change
  Makes one change at random, from a pool of texts that hold every shape
a change may be given, good and bad.
  Long as it is a table of the changes, each a call.
*/
static bool
d_tests_one_change(
    struct d_cmake_doc*  _doc,
    struct d_tests_fuzz* _fuzz
)
{
    static const char* const arguments[] =
    {
        "x", "\"a b\"", "[[br]]", "${V}", "\"q\\\"x\"", "(", "a b", "#c",
        "\"open", "[=[x]=]", "\"\"", "a;b", "\"l\nm\"", ")", "\\;"
    };
    static const char* const names[] =
    {
        "set", "if", "endif", "foo_bar", "1bad", "else", "function", "SET"
    };
    static const char* const commands[] =
    {
        "set(a b)", "if(X)\nendif()", "message(\"hi\")", "endif()",
        "foo(\n  a\n  b)", "# only a comment", "x() y()", "bad(",
        "  indented(x)\n", "if(A)\nelse()\nendif()", "else()"
    };
    static const char* const raw[] =
    {
        ")", "(", "\"", " ", "\n", "#", "set(z)\n", "]]", "", "x"
    };
    const struct d_cmake* const cmake = d_cmake_doc_records(_doc);
    const size_t                count = cmake->command_count;
    const size_t                n     = d_tests_random(_fuzz, count + 1u);
    const size_t                args  = (n < count)
                                            ? cmake->commands[n].argument_count
                                            : 0u;
    const size_t                k     = d_tests_random(_fuzz, args + 1u);
    const char* const           arg   =
        arguments[d_tests_random(_fuzz, sizeof(arguments) /
                                        sizeof(arguments[0]))];
    size_t                      length = 0;

    (void)d_cmake_doc_text(_doc, &length);

    const size_t at = d_tests_random(_fuzz, length + 1u);

    // a table of the changes
    switch (d_tests_random(_fuzz, 8u))
    {
        case 0:
            return d_cmake_doc_set_argument(_doc, n, k, arg);

        case 1:
            return d_cmake_doc_insert_argument(_doc, n, k, arg,
                                               (d_tests_random(_fuzz, 2u) ==
                                                1u));

        case 2:
            return d_cmake_doc_remove_argument(_doc, n, k);

        case 3:
            return d_cmake_doc_set_name(_doc, n,
                                        names[d_tests_random(
                                                  _fuzz,
                                                  sizeof(names) /
                                                  sizeof(names[0]))]);

        case 4:
            return d_cmake_doc_set_command(_doc, n,
                                           commands[d_tests_random(
                                                        _fuzz,
                                                        sizeof(commands) /
                                                        sizeof(commands[0]))]);

        case 5:
            return d_cmake_doc_insert_commands(_doc, n,
                                               commands[d_tests_random(
                                                   _fuzz,
                                                   sizeof(commands) /
                                                   sizeof(commands[0]))]);

        case 6:
            return d_cmake_doc_remove_commands(_doc, n,
                                               n + d_tests_random(_fuzz, 3u));

        default:
            break;
    }

    const char* const text = raw[d_tests_random(_fuzz, sizeof(raw) /
                                                       sizeof(raw[0]))];
    const size_t      end  = at + d_tests_random(_fuzz, 4u);

    return d_cmake_doc_splice(_doc, at, (end <= length) ? end : length, text,
                              strlen(text));
}


/*
d_tests_same_outside
  Whether two readings agree on the commands outside a change, found here
afresh: as many wholly before `_start` in each, and wholly after its end in
each, each the same bytes, at the same depth, taking the same part in its
block, and paired alike -- with one outside at the same place, or with one
the change cut.
  Long as the places are found afresh, apart from the document's own way of
finding them.
*/
static bool
d_tests_same_outside(
    const struct d_cmake* _old,
    const struct d_cmake* _new,
    size_t                _start,
    size_t                _old_end,
    size_t                _new_end
)
{
    size_t before = 0;
    size_t old_at = 0;
    size_t new_at = 0;

    // those before in each
    while ( (before < _old->command_count) &&
            (_old->commands[before].end <= _start) )
    {
        before++;
    }

    old_at = before;
    new_at = before;

    // those cut, in each
    while ( (old_at < _old->command_count) &&
            (_old->commands[old_at].start < _old_end) )
    {
        old_at++;
    }

    // ...and in the new
    while ( (new_at < _new->command_count) &&
            (_new->commands[new_at].start < _new_end) )
    {
        new_at++;
    }

    // as many after in each, and the new as many before
    if ( ((_old->command_count - old_at) != (_new->command_count - new_at)) ||
         (before > _new->command_count) ||
         ( (before > 0u) &&
           (_new->commands[before - 1u].end > _start) ) )
    {
        return false;
    }

    // each outside, against its own
    for (size_t at = 0; at < _old->command_count; ++at)
    {
        const bool inside = ( (at >= before) && (at < old_at) );
        const size_t other = (at < before) ? at : (at - old_at + new_at);

        // one the change cut is the change's to answer for
        if (inside)
        {
            continue;
        }

        const struct d_cmake_command* const a = &_old->commands[at];
        const struct d_cmake_command* const b = &_new->commands[other];
        const size_t                        p = a->partner;
        const size_t                        q =
            (p == D_CMAKE_NONE) ? D_CMAKE_NONE
          : (p < before)        ? p
          : (p >= old_at)       ? (p - old_at + new_at)
          :                       (size_t)-2;

        // the same bytes, depth, part, and pairing
        if ( ((a->end - a->start) != (b->end - b->start))            ||
             (memcmp(&_old->text[a->start], &_new->text[b->start],
                     a->end - a->start) != 0)                        ||
             (a->depth != b->depth)                                  ||
             (a->role != b->role)                                    ||
             (a->unpaired != b->unpaired)                            ||
             ( (q != (size_t)-2) && (q != b->partner) )              ||
             ( (q == (size_t)-2) &&
               ( (b->partner < before) || (b->partner >= new_at) ) ) )
        {
            return false;
        }
    }

    return true;
}


/*
d_tests_fuzz_holds
  Whether a change did what it says: kept, the text reads, the bytes
before and after where it was are the text's own, and so are the commands
outside it; refused, the text as it was.
*/
static bool
d_tests_fuzz_holds(
    const struct d_cmake_doc* _doc,
    bool                      _kept,
    const char*               _before,
    size_t                    _before_length
)
{
    size_t            length  = 0;
    const char* const text    = d_cmake_doc_text(_doc, &length);
    size_t            start   = 0;
    size_t            old_end = 0;
    size_t            new_end = 0;

    // refused: the text as it was
    if (!_kept)
    {
        return ( (length == _before_length) &&
                 (memcmp(text, _before, length) == 0) );
    }

    struct d_cmake old;
    struct d_cmake now;

    // each released whether or not it was read
    memset(&old, 0, sizeof(old));
    memset(&now, 0, sizeof(now));

    const bool     read = (d_cmake_read(&now, text, length, NULL, 0u)) &&
                          (d_cmake_read(&old, _before, _before_length, NULL,
                                        0u));
    const bool     held = (read) &&
                          (d_cmake_doc_changed(_doc, &start, &old_end,
                                               &new_end)) &&
                          (start <= old_end) &&
                          (old_end <= _before_length) &&
                          (new_end <= length) &&
                          ((length - new_end) ==
                               (_before_length - old_end)) &&
                          (memcmp(text, _before, start) == 0) &&
                          (memcmp(&text[new_end], &_before[old_end],
                                  length - new_end) == 0) &&
                          (d_tests_same_outside(&old, &now, start, old_end,
                                                new_end));

    d_cmake_release(&old);
    d_cmake_release(&now);

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
    struct d_cmake_doc* const doc = d_cmake_doc_read(_path, why, sizeof(why));

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
        const char* const now    = d_cmake_doc_text(doc, &length);
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

    d_cmake_doc_free(doc);

    return;
}


/*
d_tests_is_cmake
  Whether a file name is CMake's: CMakeLists.txt, *.cmake or *.cmake.in.
*/
static bool
d_tests_is_cmake(
    const char* _name
)
{
    const size_t span = strlen(_name);

    return ( (strcmp(_name, "CMakeLists.txt") == 0)         ||
             ( (span > 6u) &&
               (strcmp(&_name[span - 6u], ".cmake") == 0) ) ||
             ( (span > 9u) &&
               (strcmp(&_name[span - 9u], ".cmake.in") == 0) ) );
}


/*
d_tests_fuzz_walk
  Every CMake file under a directory, in name order.
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

    // each name, a directory walked and a CMake file changed
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
        else if (d_tests_is_cmake(names[at]))
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
process is in; with --fuzz, changes at random to every CMake file under a
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

    d_tests_documents();
    d_tests_set_argument();
    d_tests_insert_argument();
    d_tests_remove_argument();
    d_tests_commands();
    d_tests_insert_commands();
    d_tests_indent();
    d_tests_saving((argc > 1) ? argv[1] : ".");

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
