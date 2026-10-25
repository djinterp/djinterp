/*******************************************************************************
* djinterp [tools]                                                 test_dcmake.c
*
* Exercises `dcmake.h`: every form an argument takes, comments in every place
* they may stand, values, line endings, the blocks and their pairing, every
* refusal, with where it is reported, and the writers, each value read back.
*   With --corpus, it reads every CMake file under a directory instead --
* CMakeLists.txt, *.cmake and *.cmake.in -- and proves each reading whole:
* every byte outside a record is space or a line ending, every record lies
* in order within its parent, and every line and column is the one its
* offset gives.
*
*   With --values, it prints the arguments of a file's `show(...)` commands
* as cmake_values.cmake prints them under CMake itself, so that what this
* reader makes of each argument can be compared with what CMake makes of it.
*
*   test_dcmake                    the unit checks
*   test_dcmake --corpus <dir>...  every CMake file under each directory
*   test_dcmake --values <file>    the values of the file's show(...)
*
* path:      /tests/djinterp/tools/dawk/ext/cmake/test_dcmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dcmake.h"  // corresponding header
// std
#include <stdio.h>   // printf, fopen, fread, fclose, snprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // strcmp, strlen, strstr, memcmp
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"  // d_fs_names, d_fs_kind_of


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
d_tests_read
  Reads a text that must read; a failure is reported as the check named.
*/
static bool
d_tests_read(
    struct d_cmake* _cmake,
    const char*     _text,
    const char*     _what
)
{
    char       why[256];
    const bool read = d_cmake_read(_cmake, _text, strlen(_text), why,
                                   sizeof(why));

    // the reason is what a reader of the failure needs
    if (!read)
    {
        printf("  FAIL  %s: %s\n", _what, why);
        g_failed++;
    }

    g_checks++;

    return read;
}


/*
d_tests_value_is
  Whether an argument's value is the text expected.
*/
static bool
d_tests_value_is(
    const struct d_cmake* _cmake,
    size_t                _argument,
    const char*           _expected
)
{
    // an argument that is not there has no value
    if (_argument >= _cmake->argument_count)
    {
        return false;
    }

    const struct d_cmake_argument* const argument =
        &_cmake->arguments[_argument];
    char* const  value  = malloc((argument->end - argument->start) + 1u);
    bool         same   = false;

    // a value that cannot be held cannot be compared
    if (value)
    {
        const size_t length = d_cmake_value(_cmake, argument, value);

        same = ( (length == strlen(_expected)) &&
                 (memcmp(value, _expected, length) == 0) );
        free(value);
    }

    return same;
}


/*
d_tests_raw_is
  Whether an argument is written as the text expected.
*/
static bool
d_tests_raw_is(
    const struct d_cmake* _cmake,
    size_t                _argument,
    const char*           _expected
)
{
    // an argument that is not there is written as nothing
    if (_argument >= _cmake->argument_count)
    {
        return false;
    }

    const struct d_cmake_argument* const argument =
        &_cmake->arguments[_argument];
    const size_t length = argument->end - argument->start;

    return ( (length == strlen(_expected)) &&
             (memcmp(&_cmake->text[argument->start], _expected,
                     length) == 0) );
}


/*
d_tests_arguments
  Tests the following:
  - a command's name, place and arguments
  - the three kinds of argument, and nested parentheses
  - the legacy forms, kept as one argument with their quotes
  - every escape, and what a value keeps of each
*/
static void
d_tests_arguments(void)
{
    struct d_cmake cmake;

    printf("arguments\n");

    if (d_tests_read(&cmake, "  Set (a \"b c\" [[d]])\n", "a command"))
    {
        const struct d_cmake_command* const command = &cmake.commands[0];

        d_tests_check( (cmake.command_count == 1u) &&
                       (command->argument_count == 3u) &&
                       (command->line == 1u) && (command->column == 3u) &&
                       (command->name_length == 3u) &&
                       (cmake.text[command->open] == '(') &&
                       (command->end == 21u),
                       "a command's name, place and span");
        d_tests_check(d_cmake_is(&cmake, command, "set") &&
                      d_cmake_is(&cmake, command, "SET") &&
                      !d_cmake_is(&cmake, command, "se"),
                      "names compare without case");
        d_tests_check( (cmake.arguments[0].kind == D_CMAKE_UNQUOTED) &&
                       (cmake.arguments[1].kind == D_CMAKE_QUOTED) &&
                       (cmake.arguments[2].kind == D_CMAKE_BRACKET) &&
                       (cmake.arguments[1].column == 10u),
                       "the three kinds of argument");
        d_tests_check(d_tests_raw_is(&cmake, 1u, "\"b c\"") &&
                      d_tests_value_is(&cmake, 1u, "b c") &&
                      d_tests_value_is(&cmake, 2u, "d"),
                      "an argument as written, and as read");
    }

    d_cmake_release(&cmake);

    if (d_tests_read(&cmake, "if((A OR B) AND NOT (C))\nendif()\n",
                     "nested parentheses"))
    {
        d_tests_check( (cmake.commands[0].argument_count == 10u) &&
                       (cmake.arguments[0].paren == '(') &&
                       (cmake.arguments[4].paren == ')') &&
                       (cmake.arguments[7].paren == '(') &&
                       (cmake.arguments[9].paren == ')') &&
                       (cmake.arguments[1].paren == '\0') &&
                       d_tests_raw_is(&cmake, 7u, "(") &&
                       d_tests_raw_is(&cmake, 8u, "C"),
                       "a nested parenthesis is an argument of its own");
    }

    d_cmake_release(&cmake);

    if (d_tests_read(&cmake,
                     "add_definitions(-Da=\"b c\" $(MAKE)x [x] a[[b]])\n",
                     "legacy forms"))
    {
        d_tests_check( (cmake.argument_count == 4u) &&
                       (cmake.arguments[0].legacy) &&
                       d_tests_raw_is(&cmake, 0u, "-Da=\"b c\"") &&
                       d_tests_value_is(&cmake, 0u, "-Da=\"b c\""),
                       "a quoted run keeps its quotes and its space");
        d_tests_check( (cmake.arguments[1].legacy) &&
                       d_tests_raw_is(&cmake, 1u, "$(MAKE)x"),
                       "a make variable keeps its parentheses");
        d_tests_check( (!cmake.arguments[2].legacy) &&
                       (cmake.arguments[2].kind == D_CMAKE_UNQUOTED) &&
                       d_tests_raw_is(&cmake, 3u, "a[[b]]"),
                       "a bracket opens only an argument's start");
    }

    d_cmake_release(&cmake);

    if (d_tests_read(&cmake,
                     "x(a\\;b \"\\t\\n\\r\\$\\\"\\\\\" \\d \"a\\\n"
                     "b\" [==[\nx]]y]=]z]==] [[\r\ny]] \"a\\;b\" \"p\r\nq\" "
                     "[[r\r\ns]])\n",
                     "escapes"))
    {
        d_tests_check(d_tests_value_is(&cmake, 0u, "a;b"),
                      "`\\;` is `;` in an unquoted argument");
        d_tests_check(d_tests_value_is(&cmake, 1u, "\t\n\r$\"\\"),
                      "the encoded and the identity escapes");
        d_tests_check(d_tests_value_is(&cmake, 2u, "\\d"),
                      "an escaped letter is kept as written");
        d_tests_check(d_tests_value_is(&cmake, 3u, "ab") &&
                      (cmake.arguments[3].end_line == 2u),
                      "a continuation joins the lines of a quoted argument");
        d_tests_check( (cmake.arguments[4].equals == 2u) &&
                       d_tests_value_is(&cmake, 4u, "x]]y]=]z"),
                       "a bracket closes only with its own `=` count, and "
                       "drops a first newline");
        d_tests_check(d_tests_value_is(&cmake, 5u, "y"),
                      "...a first CR LF too");
        d_tests_check(d_tests_value_is(&cmake, 6u, "a\\;b"),
                      "`\\;` stays as it is in a quoted argument");
        d_tests_check(d_tests_value_is(&cmake, 7u, "p\nq") &&
                      d_tests_value_is(&cmake, 8u, "r\ns"),
                      "a CR LF inside quotes or brackets is read as LF");
    }

    d_cmake_release(&cmake);

    return;
}


/*
d_tests_comments
  Tests the following:
  - line and bracket comments, at the top and among arguments
  - which comments trail something on their line
  - a `#` inside an unquoted argument ends it
*/
static void
d_tests_comments(void)
{
    struct d_cmake cmake;

    printf("comments\n");

    if (d_tests_read(&cmake,
                     "# top\n"
                     "    # indented\n"
                     "set(a # among\n"
                     "    #[[ bracket\n"
                     "    comment ]] b) # after\n"
                     "#[=[ alone ]=] # second\n"
                     "x(a#b\n"
                     ")\n",
                     "comments"))
    {
        d_tests_check(cmake.comment_count == 8u, "eight comments");
        d_tests_check( (cmake.comments[0].within == D_CMAKE_NONE) &&
                       (!cmake.comments[0].trailing) &&
                       (!cmake.comments[1].trailing) &&
                       (cmake.comments[1].column == 5u),
                       "a comment alone on its line, indented or not");
        d_tests_check( (cmake.comments[2].within == 0u) &&
                       (cmake.comments[2].trailing) &&
                       (cmake.comments[3].within == 0u) &&
                       (cmake.comments[3].bracket) &&
                       (!cmake.comments[3].trailing) &&
                       (cmake.comments[3].line == 4u) &&
                       (cmake.comments[3].end_line == 5u),
                       "comments among the arguments belong to the command");
        d_tests_check( (cmake.comments[4].within == D_CMAKE_NONE) &&
                       (cmake.comments[4].trailing) &&
                       (cmake.comments[5].bracket) &&
                       (!cmake.comments[5].trailing) &&
                       (cmake.comments[6].trailing) &&
                       (!cmake.comments[6].bracket),
                       "a comment after a command, or a comment, trails");
        d_tests_check( (cmake.command_count == 2u) &&
                       (cmake.commands[0].argument_count == 2u) &&
                       (cmake.commands[1].argument_count == 1u) &&
                       d_tests_raw_is(&cmake, 2u, "a") &&
                       (cmake.comments[7].within == 1u) &&
                       (cmake.comments[7].trailing),
                       "a `#` ends an unquoted argument and begins a "
                       "comment");
    }

    d_cmake_release(&cmake);

    return;
}


/*
d_tests_lines
  Tests the following:
  - LF and CR LF line endings, and a lone CR, which is space
  - the line count, with and without a last line ending
  - a byte-order mark is skipped, and columns count after it
*/
static void
d_tests_lines(void)
{
    struct d_cmake cmake;

    printf("lines\n");

    if (d_tests_read(&cmake, "a()\r\n\r\nb(x\r\n y)\r\n", "CR LF"))
    {
        d_tests_check( (cmake.crlf) && (cmake.line_count == 4u) &&
                       (cmake.commands[1].line == 3u) &&
                       (cmake.commands[1].end_line == 4u) &&
                       (cmake.arguments[1].line == 4u) &&
                       (cmake.arguments[1].column == 2u) &&
                       d_tests_raw_is(&cmake, 1u, "y"),
                       "CR LF ends a line, and a CR is not in an argument");
    }

    d_cmake_release(&cmake);

    if (d_tests_read(&cmake, "a(x\ry)", "a lone CR"))
    {
        d_tests_check( (!cmake.crlf) && (cmake.line_count == 1u) &&
                       (cmake.argument_count == 2u),
                       "a lone CR is space, and an unended line counts");
    }

    d_cmake_release(&cmake);

    d_tests_check(d_cmake_read(&cmake, "", 0u, NULL, 0u) &&
                  (cmake.line_count == 0u) && (cmake.command_count == 0u),
                  "an empty file has no lines");
    d_cmake_release(&cmake);

    if (d_tests_read(&cmake, "\xEF\xBB\xBFset(a)\n", "a byte-order mark"))
    {
        d_tests_check( (cmake.command_count == 1u) &&
                       (cmake.commands[0].column == 1u) &&
                       (cmake.commands[0].start == 3u),
                       "a byte-order mark is no element");
    }

    d_cmake_release(&cmake);

    return;
}


/*
d_tests_blocks
  Tests the following:
  - openers, branches and closers pair, and hold their bodies one deeper
  - a closer passing over a block it does not close leaves that block open
  - a closer, or a branch, with nothing to pair with is unpaired
*/
static void
d_tests_blocks(void)
{
    struct d_cmake cmake;

    printf("blocks\n");

    if (d_tests_read(&cmake,
                     "function(f)\n"         // 0
                     "  if(a)\n"             // 1
                     "    foreach(x y)\n"    // 2
                     "      message(x)\n"    // 3
                     "    endforeach()\n"    // 4
                     "  elseif(b)\n"         // 5
                     "  else()\n"            // 6
                     "  endif()\n"           // 7
                     "endfunction()\n"       // 8
                     "set(z)\n",             // 9
                     "nested blocks"))
    {
        const struct d_cmake_command* const c = cmake.commands;

        d_tests_check( (c[0].depth == 0u) && (c[1].depth == 1u) &&
                       (c[2].depth == 2u) && (c[3].depth == 3u) &&
                       (c[4].depth == 2u) && (c[5].depth == 1u) &&
                       (c[6].depth == 1u) && (c[7].depth == 1u) &&
                       (c[8].depth == 0u) && (c[9].depth == 0u),
                       "every command at its depth");
        d_tests_check( (c[0].partner == 8u) && (c[8].partner == 0u) &&
                       (c[1].partner == 7u) && (c[5].partner == 1u) &&
                       (c[6].partner == 1u) && (c[2].partner == 4u) &&
                       (c[9].partner == D_CMAKE_NONE),
                       "openers, branches and closers pair");
        d_tests_check( (c[0].block == D_CMAKE_BLOCK_FUNCTION) &&
                       (c[0].role == D_CMAKE_ROLE_OPEN) &&
                       (c[5].role == D_CMAKE_ROLE_BRANCH) &&
                       (c[7].role == D_CMAKE_ROLE_CLOSE) &&
                       (c[9].role == D_CMAKE_ROLE_NONE) &&
                       (strcmp(d_cmake_block_name(c[2].block),
                               "foreach") == 0),
                       "each command's block and role");

        bool none = true;

        for (size_t at = 0; at < cmake.command_count; ++at)
        {
            none = none && (!c[at].unpaired);
        }

        d_tests_check(none, "nothing is unpaired");
    }

    d_cmake_release(&cmake);

    if (d_tests_read(&cmake,
                     "endif()\n"            // 0
                     "else()\n"             // 1
                     "function(f)\n"        // 2
                     "  if(a)\n"            // 3
                     "endfunction()\n"      // 4
                     "macro(m)\n",          // 5
                     "broken blocks"))
    {
        const struct d_cmake_command* const c = cmake.commands;

        d_tests_check( (c[0].unpaired) && (c[0].partner == D_CMAKE_NONE) &&
                       (c[1].unpaired),
                       "a closer and a branch with no opener are unpaired");
        d_tests_check( (c[3].unpaired) && (c[3].partner == D_CMAKE_NONE) &&
                       (!c[2].unpaired) && (c[2].partner == 4u) &&
                       (c[4].depth == 0u),
                       "a closer passing over a block leaves it open");
        d_tests_check( (c[5].unpaired) && (c[5].depth == 0u),
                       "an opener the file never closes is unpaired");
    }

    d_cmake_release(&cmake);

    return;
}


/*
d_tests_refusals
  Tests the following:
  - each refusal, with the line and column it is reported at
  - the commands before a refusal are kept, and the broken one is not
*/
static void
d_tests_refusals(void)
{
    // d_tests_refusal
    //   struct: a text the reader must refuse, where, and in what words.
    struct d_tests_refusal
    {
        const char* text;
        size_t      line;
        size_t      column;
        const char* words;
    };

    static const struct d_tests_refusal refusals[] =
    {
        { "set a\n",                 1u, 5u, "expected `(`"           },
        { "set\n(a)\n",              1u, 4u, "expected `(`"           },
        { "a()\nb(x \"y\n",          2u, 5u, "quoted argument"        },
        { "a(\n  [=[x]]\n)\n",       2u, 3u, "bracket argument"       },
        { "a()\n#[[ open\n",         2u, 1u, "bracket comment"        },
        { "a()\nb(x\n  y\n",         2u, 1u, "never closed"           },
        { "a() b()\n",               1u, 5u, "must end its line"      },
        { ")\n",                     1u, 1u, "expected a command"     },
        { "a(\"x\")\n\"y\"\n",       2u, 1u, "expected a command"     },
        { "a(x\\\n)\n",              1u, 4u, "backslash"              },
        { "a(-D\"x\n)\n",            1u, 5u, "quoted run"             }
    };

    printf("refusals\n");

    for (size_t at = 0; at < (sizeof(refusals) / sizeof(refusals[0])); ++at)
    {
        struct d_cmake cmake;
        char           why[256];
        char           what[320];
        const bool     read = d_cmake_read(&cmake, refusals[at].text,
                                           strlen(refusals[at].text), why,
                                           sizeof(why));

        (void)snprintf(what, sizeof(what), "refused at %lu:%lu with \"%s\"",
                       (unsigned long)refusals[at].line,
                       (unsigned long)refusals[at].column,
                       refusals[at].words);
        d_tests_check( (!read) &&
                       (cmake.error_line == refusals[at].line) &&
                       (cmake.error_column == refusals[at].column) &&
                       (strstr(why, refusals[at].words) != NULL),
                       what);

        // what failed is named in the check's words, for a reader
        if ( (read) ||
             (cmake.error_line != refusals[at].line) ||
             (cmake.error_column != refusals[at].column) )
        {
            printf("        got %s\n", read ? "no refusal" : why);
        }

        d_cmake_release(&cmake);
    }

    struct d_cmake cmake;

    d_tests_check( (!d_cmake_read(&cmake, "a(x)\nb(y # z\n", 14u, NULL,
                                  0u)) &&
                   (cmake.command_count == 1u) &&
                   (cmake.argument_count == 1u) &&
                   (cmake.comment_count == 0u),
                   "the commands before a refusal are kept, the broken one "
                   "and its comments are not");
    d_cmake_release(&cmake);

    return;
}


/*
d_tests_generated
  Tests the following:
  - CMake's markers, on the first line only
  - the tree's marker, anywhere in the first 4 KiB
*/
static void
d_tests_generated(void)
{
    static const char install[] = "# Install script for directory: D:/x\n";
    static const char testfile[] = "# CMake generated Testfile for \n";
    static const char later[]   = "set(a)\n# Install script for directory:\n";
    static const char tree[]    = "# Auto-generated. Do not edit by hand.\n";

    printf("generated\n");

    d_tests_check(d_cmake_generated(install, sizeof(install) - 1u) &&
                  d_cmake_generated(testfile, sizeof(testfile) - 1u),
                  "CMake's markers");
    d_tests_check(!d_cmake_generated(later, sizeof(later) - 1u),
                  "...only on the first line");
    d_tests_check(d_cmake_generated(tree, sizeof(tree) - 1u),
                  "the tree's marker");
    d_tests_check(!d_cmake_generated("set(a)\n", 7u) &&
                  !d_cmake_generated(NULL, 0u),
                  "a hand-written file");

    return;
}


/*
d_tests_reads_back
  Whether a text written for a value reads as one argument of that value.
*/
static bool
d_tests_reads_back(
    const char* _text,
    const char* _value
)
{
    const size_t   span    = strlen(_text);
    char* const    wrapped = malloc(span + 4u);
    struct d_cmake cmake;
    bool           same    = false;

    // the text as a command's one argument
    if (wrapped)
    {
        wrapped[0] = 'x';
        wrapped[1] = '(';
        memcpy(&wrapped[2], _text, span);
        wrapped[span + 2u] = ')';
        wrapped[span + 3u] = '\0';
        same = ( (d_cmake_read(&cmake, wrapped, span + 3u, NULL, 0u)) &&
                 (cmake.argument_count == 1u)                         &&
                 (d_tests_value_is(&cmake, 0u, _value)) );
        d_cmake_release(&cmake);
        free(wrapped);
    }

    return same;
}


/*
d_tests_writing
  Tests the following:
  - every value written as each kind of argument it can be reads back as
    itself, and the plainest form is unquoted where it can be
  - what an unquoted or a bracket argument cannot hold is refused
  - a command's name, and what is not one
  - lists divided as CMake divides them, empty items kept or dropped
*/
static void
d_tests_writing(void)
{
    static const char* const values[] =
    {
        "plain", "a b", "a;b", "", "x\"y", "back\\slash", "[[x]]", "=a",
        "#hash", "tab\there", "new\nline", "a]==]b", "(p)", "c\rr", "\n",
        "${v}", "a\\;b", "]]", "x]=]y"
    };
    const size_t             count    = sizeof(values) / sizeof(values[0]);
    bool                     all      = true;
    size_t                   unquoted = 0;

    // each value, each kind
    for (size_t at = 0; at < count; ++at)
    {
        const size_t length = strlen(values[at]);

        // a table of the kinds, each read back when it was written
        for (int kind = 0; kind < 3; ++kind)
        {
            char* const text = d_cmake_write(values[at], length,
                                             (enum d_cmake_argument_kind)kind,
                                             NULL);

            all       = (all) &&
                        ( (!text) || (d_tests_reads_back(text, values[at])) );
            unquoted += ( (text) && (kind == 0) ) ? 1u : 0u;
            free(text);
        }

        char* const any = d_cmake_write_any(values[at], length, NULL);

        all = (all) && (any) && (d_tests_reads_back(any, values[at]));
        free(any);
    }

    d_tests_check(all, "every value written reads back as itself");
    d_tests_check(unquoted == 5u,
                  "...unquoted only where it needs no escape: plain, "
                  "a]==]b, ${v}, ]], x]=]y");

    char* const quoted  = d_cmake_write("a\"b", 3u, D_CMAKE_QUOTED, NULL);
    char* const fenced  = d_cmake_write("a]]b", 4u, D_CMAKE_BRACKET, NULL);
    char* const crlf    = d_cmake_write("a\r\nb", 4u, D_CMAKE_BRACKET,
                                        NULL);
    char* const spaced  = d_cmake_write("a b", 3u, D_CMAKE_UNQUOTED, NULL);
    char* const nothing = d_cmake_write("", 0u, D_CMAKE_UNQUOTED, NULL);

    d_tests_check( (quoted) && (strcmp(quoted, "\"a\\\"b\"") == 0) &&
                   (fenced) && (strcmp(fenced, "[=[a]]b]=]") == 0),
                  "a quote escaped, and a bracket fenced past its value");
    d_tests_check( (!crlf) && (!spaced) && (!nothing),
                  "a CR LF in brackets, and a space or nothing unquoted, "
                  "are refused");
    free(quoted);
    free(fenced);
    free(crlf);
    free(spaced);
    free(nothing);

    d_tests_check( (d_cmake_is_name("add_executable", 14u)) &&
                   (d_cmake_is_name("_x9", 3u)) &&
                   (!d_cmake_is_name("9x", 2u)) &&
                   (!d_cmake_is_name("a-b", 3u)) &&
                   (!d_cmake_is_name("", 0u)),
                  "a command's name, and what is not one");

    char         items[32];
    size_t       used  = 0;
    const size_t kept  = d_cmake_list_items("a;;b;", 5u, true, items, &used);
    const size_t drop  = d_cmake_list_items("a;;b;", 5u, false, items, &used);
    const size_t odd   = d_cmake_list_items("]a;b[;c\\;d", 10u, false,
                                            items, &used);

    d_tests_check( (kept == 4u) && (drop == 2u),
                  "empty items are kept, or dropped");
    d_tests_check( (odd == 2u) &&
                   (strcmp(items, "]a;b[") == 0) &&
                   (strcmp(&items[6], "c;d") == 0),
                  "a `]` before any `[` holds the list together until a `[`, "
                  "and `\\;` is a `;`");
    d_tests_check( (d_cmake_list_items("", 0u, true, items, &used) == 0u) &&
                   (d_cmake_list_items("a\\b", 3u, false, items, &used) ==
                    1u) &&
                   (strcmp(items, "a\\b") == 0),
                  "an empty list has no items, and one with no `;` is one "
                  "as it stands");

    return;
}


// d_tests_corpus
//   struct: what a corpus run counted.
struct d_tests_corpus
{
    size_t files;
    size_t generated;
    size_t commands;
    size_t arguments;
    size_t comments;
    size_t lines;
    size_t unpaired;
    size_t failed;
};


/*
d_tests_slurp
  Reads a whole file; the caller frees it.
*/
static char*
d_tests_slurp(
    const char* _path,
    size_t*     _length
)
{
    FILE* const handle = fopen(_path, "rb");

    // a file that cannot be opened has no text
    if (!handle)
    {
        return NULL;
    }

    size_t capacity = 4096u;
    size_t used     = 0;
    char*  text     = malloc(capacity);

    // a block at a time, the buffer doubled as it fills
    while (text)
    {
        const size_t got = fread(&text[used], 1u, capacity - used, handle);

        used += got;

        // a short read is the end of the file
        if (used < capacity)
        {
            break;
        }

        char* const grown = realloc(text, capacity * 2u);

        // memory that ran out loses what was read
        if (!grown)
        {
            free(text);
            text = NULL;
            break;
        }

        text      = grown;
        capacity *= 2u;
    }

    (void)fclose(handle);
    *_length = used;

    return text;
}


/*
d_tests_place_holds
  Whether a record's line and column are the ones its offset gives.
*/
static bool
d_tests_place_holds(
    const size_t* _line_starts,
    size_t        _line_count,
    size_t        _start,
    size_t        _line,
    size_t        _column
)
{
    // a line the file does not have is no place
    if ( (_line == 0u) ||
         (_line > _line_count) )
    {
        return false;
    }

    return ( (_start >= _line_starts[_line - 1u]) &&
             ( (_line == _line_count) ||
               (_start < _line_starts[_line]) ) &&
             ((_start - _line_starts[_line - 1u] + 1u) == _column) );
}


/*
d_tests_whole
  Proves a reading whole: everything outside the records is space or a line
ending, the records come in order, and each sits where its place says.
Returns the first offset that breaks it, or the text's length.
  Long as each kind of record is proved in turn, and then the bytes between.
*/
static size_t
d_tests_whole(
    const struct d_cmake* _cmake,
    const char**          _why
)
{
    const char* const text   = _cmake->text;
    const size_t      length = _cmake->length;
    char* const       taken  = calloc(length + 1u, 1u);
    size_t* const     starts = malloc((length + 2u) * sizeof(size_t));
    size_t            lines  = 0;
    size_t            broken = length;

    *_why = "out of memory";

    // the proof needs its own map of the text
    if ( (!taken) ||
         (!starts) )
    {
        free(taken);
        free(starts);

        return 0u;
    }

    // where every line starts, a byte-order mark aside
    starts[lines++] = ( (length >= 3u) &&
                        ((unsigned char)text[0] == 0xEFu) ) ? 3u : 0u;

    // each line's first byte
    for (size_t at = 0; at < length; ++at)
    {
        // a line feed starts the next line
        if ( (text[at] == '\n') &&
             ((at + 1u) < length) )
        {
            starts[lines++] = at + 1u;
        }
    }

    *_why = NULL;

    // each command: its name, its `(`, its arguments, its `)`
    for (size_t which = 0; (!*_why) && (which < _cmake->command_count);
         ++which)
    {
        const struct d_cmake_command* const command =
            &_cmake->commands[which];

        // a command starts past the one before it
        if ( (which > 0u) &&
             (command->start < _cmake->commands[which - 1u].end) )
        {
            *_why  = "commands out of order";
            broken = command->start;
        }
        else if (!d_tests_place_holds(starts, lines, command->start,
                                      command->line, command->column))
        {
            *_why  = "a command's place";
            broken = command->start;
        }
        else if ( (text[command->open] != '(') ||
                  (text[command->end - 1u] != ')') )
        {
            *_why  = "a command's parentheses";
            broken = command->open;
        }

        // its name holds no byte of another record
        for (size_t at = command->start;
             at < (command->start + command->name_length); ++at)
        {
            taken[at] = 1;
        }

        taken[command->open]       = 1;
        taken[command->end - 1u]   = 1;

        size_t previous = command->open + 1u;

        // each argument, in order, within its command
        for (size_t arg = command->first_argument;
             (!*_why) &&
             (arg < (command->first_argument + command->argument_count));
             ++arg)
        {
            const struct d_cmake_argument* const argument =
                &_cmake->arguments[arg];

            // within the command, past the one before, and not empty
            if ( (argument->start < previous) ||
                 (argument->end > (command->end - 1u)) ||
                 (argument->end <= argument->start) )
            {
                *_why  = "an argument out of order";
                broken = argument->start;
            }
            else if (!d_tests_place_holds(starts, lines, argument->start,
                                          argument->line, argument->column))
            {
                *_why  = "an argument's place";
                broken = argument->start;
            }

            // its bytes are its own
            for (size_t at = argument->start; at < argument->end; ++at)
            {
                taken[at] = 1;
            }

            previous = argument->end;
        }
    }

    // each comment, within its command when it has one
    for (size_t which = 0; (!*_why) && (which < _cmake->comment_count);
         ++which)
    {
        const struct d_cmake_comment* const comment =
            &_cmake->comments[which];

        // a comment opens with `#`, where its place says
        if ( (text[comment->start] != '#') ||
             (!d_tests_place_holds(starts, lines, comment->start,
                                   comment->line, comment->column)) )
        {
            *_why  = "a comment's place";
            broken = comment->start;
        }
        else if ( (comment->within != D_CMAKE_NONE) &&
                  ( (comment->start <
                     _cmake->commands[comment->within].open) ||
                    (comment->end >
                     _cmake->commands[comment->within].end) ) )
        {
            *_why  = "a comment outside its command";
            broken = comment->start;
        }

        // its bytes are its own
        for (size_t at = comment->start; at < comment->end; ++at)
        {
            // one already taken is another record's
            if (taken[at])
            {
                *_why  = "a comment over another record";
                broken = at;
                break;
            }

            taken[at] = 1;
        }
    }

    // and nothing else but space and line endings
    for (size_t at = starts[0]; (!*_why) && (at < length); ++at)
    {
        const char byte = text[at];

        // a byte outside every record is space or a line ending
        if ( (!taken[at]) &&
             (byte != ' ') && (byte != '\t') &&
             (byte != '\r') && (byte != '\n') )
        {
            *_why  = "a byte no record holds";
            broken = at;
        }
    }

    free(taken);
    free(starts);

    return broken;
}


/*
d_tests_corpus_file
  Reads one file and proves the reading whole.
  Long as the counts and the notes of an unpaired block are each a step.
*/
static void
d_tests_corpus_file(
    const char*            _path,
    struct d_tests_corpus* _counts
)
{
    size_t      length = 0;
    char* const text   = d_tests_slurp(_path, &length);
    char        why[256];

    _counts->files++;

    // a file that cannot be read is a failure of its own
    if (!text)
    {
        printf("  FAIL  %s: cannot be read\n", _path);
        _counts->failed++;

        return;
    }

    struct d_cmake cmake;
    const bool     read      = d_cmake_read(&cmake, text, length, why,
                                            sizeof(why));
    const bool     generated = d_cmake_generated(text, length) ||
                               (strstr(_path, "/CMakeFiles/") != NULL);
    const char*    problem   = NULL;
    const size_t   broken    = read ? d_tests_whole(&cmake, &problem) : 0u;

    _counts->generated += generated ? 1u : 0u;
    _counts->commands  += cmake.command_count;
    _counts->arguments += cmake.argument_count;
    _counts->comments  += cmake.comment_count;
    _counts->lines     += cmake.line_count;

    // a refusal, or a reading that is not whole, fails the file
    if (!read)
    {
        printf("  FAIL  %s: %s\n", _path, why);
        _counts->failed++;
    }
    else if (problem)
    {
        printf("  FAIL  %s: %s at byte %lu\n", _path, problem,
               (unsigned long)broken);
        _counts->failed++;
    }

    // each command, by whether it pairs
    for (size_t at = 0; (read) && (at < cmake.command_count); ++at)
    {
        // an unpaired block is counted, and named in a hand-written file
        if (cmake.commands[at].unpaired)
        {
            _counts->unpaired++;

            // a generated file's are its generator's to answer for
            if (!generated)
            {
                printf("  note  %s:%lu: %.*s is unpaired\n", _path,
                       (unsigned long)cmake.commands[at].line,
                       (int)cmake.commands[at].name_length,
                       &text[cmake.commands[at].start]);
            }
        }
    }

    d_cmake_release(&cmake);
    free(text);

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

    return ( (strcmp(_name, "CMakeLists.txt") == 0)                 ||
             ( (span > 6u) &&
               (strcmp(&_name[span - 6u], ".cmake") == 0) )         ||
             ( (span > 9u) &&
               (strcmp(&_name[span - 9u], ".cmake.in") == 0) ) );
}


/*
d_tests_corpus_walk
  Every CMake file under a directory, in name order.
*/
static void
d_tests_corpus_walk(
    const char*            _directory,
    struct d_tests_corpus* _counts
)
{
    char** names = NULL;
    size_t count = 0;

    // a directory that cannot be listed holds nothing to read
    if (!d_fs_names(_directory, &names, &count))
    {
        printf("  FAIL  %s: cannot be listed\n", _directory);
        _counts->failed++;

        return;
    }

    // each name, a directory walked and a CMake file read
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

        // a directory is walked, and a CMake file read
        if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
        {
            d_tests_corpus_walk(path, _counts);
        }
        else if (d_tests_is_cmake(names[at]))
        {
            d_tests_corpus_file(path, _counts);
        }

        free(path);
    }

    d_fs_names_free(names, count);

    return;
}


/*
d_tests_print_passed
  Prints the arguments CMake passes for one argument, as `show` prints
them, and counts them: a quoted or a bracket argument's value; an unquoted
one's text divided as a list, its empty items dropped.
*/
static size_t
d_tests_print_passed(
    const struct d_cmake*          _cmake,
    const struct d_cmake_argument* _argument,
    bool                           _print
)
{
    const size_t span   = _argument->end - _argument->start;
    char* const  passed = malloc(span + 1u);
    char* const  items  = malloc(span + 2u);
    size_t       count  = 0;

    // room for the text and its items
    if ( (passed) &&
         (items) )
    {
        const size_t length = d_cmake_passed(_cmake, _argument, passed);
        size_t       used   = 0;

        // an unquoted argument divided; another passed whole
        if (_argument->kind == D_CMAKE_UNQUOTED)
        {
            count = d_cmake_list_items(passed, length, false, items, &used);
        }
        else
        {
            memcpy(items, passed, length + 1u);
            count = 1u;
        }

        // each item, bracketed
        for (size_t at = 0, item = 0; (_print) && (item < count); ++item)
        {
            printf("|[%s]", &items[at]);
            at += strlen(&items[at]) + 1u;
        }
    }

    free(passed);
    free(items);

    return count;
}


/*
d_tests_values
  Prints the arguments of every `show(...)` in a file as CMake's `show`
function in cmake_values.cmake prints them: `argc=N`, then `|[value]` for
each argument CMake passes, so the two outputs can be compared byte for
byte.
  Long as the arguments are counted first and printed after, each a loop.
*/
static int
d_tests_values(
    const char* _path
)
{
    size_t      length = 0;
    char* const text   = d_tests_slurp(_path, &length);
    char        why[256];

    // a file that cannot be read has no values
    if (!text)
    {
        printf("cannot read %s\n", _path);

        return 1;
    }

    struct d_cmake cmake;
    const bool     read = d_cmake_read(&cmake, text, length, why,
                                       sizeof(why));

    // each show(...) in turn
    for (size_t at = 0; (read) && (at < cmake.command_count); ++at)
    {
        const struct d_cmake_command* const command = &cmake.commands[at];
        size_t                              count   = 0;

        // only `show` is the comparison's
        if (!d_cmake_is(&cmake, command, "show"))
        {
            continue;
        }

        // the count first, then the arguments
        for (size_t arg = 0; arg < command->argument_count; ++arg)
        {
            count += d_tests_print_passed(
                         &cmake,
                         &cmake.arguments[command->first_argument + arg],
                         false);
        }

        printf("argc=%lu", (unsigned long)count);

        // each argument, as CMake passes it
        for (size_t arg = 0; arg < command->argument_count; ++arg)
        {
            (void)d_tests_print_passed(
                      &cmake,
                      &cmake.arguments[command->first_argument + arg],
                      true);
        }

        printf("\n");
    }

    // a refusal is reported in place of the values
    if (!read)
    {
        printf("refused: %s\n", why);
    }

    d_cmake_release(&cmake);
    free(text);

    return read ? 0 : 1;
}


/*
main
  The unit checks; with --corpus every CMake file under the directories, and
with --values the arguments of a file's `show(...)` commands.
*/
int
main(
    int    argc,
    char** argv
)
{
    // a corpus run reads files and reports them, and nothing else
    if ( (argc > 2) &&
         (strcmp(argv[1], "--corpus") == 0) )
    {
        struct d_tests_corpus counts;

        memset(&counts, 0, sizeof(counts));

        // each directory given
        for (int at = 2; at < argc; ++at)
        {
            d_tests_corpus_walk(argv[at], &counts);
        }

        printf("\n%lu files (%lu generated), %lu lines, %lu commands, "
               "%lu arguments, %lu comments, %lu unpaired: %lu failed\n",
               (unsigned long)counts.files, (unsigned long)counts.generated,
               (unsigned long)counts.lines, (unsigned long)counts.commands,
               (unsigned long)counts.arguments,
               (unsigned long)counts.comments,
               (unsigned long)counts.unpaired,
               (unsigned long)counts.failed);

        return ( (counts.failed == 0u) && (counts.files > 0u) ) ? 0 : 1;
    }

    // a values run prints values, for comparing with CMake's
    if ( (argc == 3) &&
         (strcmp(argv[1], "--values") == 0) )
    {
        return d_tests_values(argv[2]);
    }

    d_tests_arguments();
    d_tests_comments();
    d_tests_lines();
    d_tests_blocks();
    d_tests_refusals();
    d_tests_generated();
    d_tests_writing();

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
