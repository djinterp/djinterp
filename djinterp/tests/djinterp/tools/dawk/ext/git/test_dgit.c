/*******************************************************************************
* djinterp [tools]                                                   test_dgit.c
*
* Exercises `dgit_text.h`, `dgit_doc.h` and `dgit_repo.h`: ignore files,
* attribute files and configurations read line by line as git reads them;
* wildmatch against vectors git's own matcher agreed with; ignoring and
* attributes over scopes; keys, values and quoting; the documents: every
* change a file takes, each refusal leaving it as it was, and saving; and a
* repository built here from nothing -- loose objects deflated by hand and
* by zlib, refs loose and packed, a history with a merge, an index and a
* work tree -- read for its objects, revisions, history, status, ignored
* files and attributes.
*   With --corpus, it reads every ignore, attribute and configuration file
* under the directories named, proving each reading sound, and every
* repository it finds there, reading each object its refs reach and proving
* its name. With --fuzz, it makes random changes to those files, and to
* texts of its own, proving of each that it was kept only if the text
* outside it reads as it did. With --oracle, it answers as git commands do,
* for git.sh to compare with git.
*
*   test_dgit [directory]                the unit checks; the repository
*                                        is built beneath the directory,
*                                        or here
*   test_dgit --corpus <dir>...          every file and repository under
*                                        each
*   test_dgit --fuzz <seed> [<dir>...]   random changes
*   test_dgit --oracle <path> <command> [argument]...
*                                        as git's command answers, of the
*                                        repository the path is in
*
* path:      /tests/djinterp/tools/dawk/ext/git/test_dgit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_repo.h"  // corresponding header
// std
#include <stdio.h>   // printf, fopen, fwrite, fclose, remove, snprintf
#include <stdlib.h>  // malloc, realloc, free, qsort, strtoul
#include <string.h>  // memcmp, memcpy, memset, strchr, strcmp, strlen
#include <time.h>    // clock
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"                // d_fs_names
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_doc.h"   // d_git_doc
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_text.h"  // d_git_text


static int g_checks = 0;
static int g_failed = 0;


//==============================================================================
// helpers
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
  Counts one check that a text is the one expected, and shows both when it
is not.
*/
static void
d_tests_is(
    const char* _got,
    const char* _want,
    const char* _what
)
{
    const char* const got = (_got) ? _got : "(none)";

    g_checks++;

    // both, between brackets, so a space at the end shows
    if (strcmp(got, _want) != 0)
    {
        g_failed++;
        printf("  FAIL  %s\n        want [%s]\n        got  [%s]\n", _what,
               _want, got);
    }

    return;
}


/*
d_tests_span_is
  Counts one check that a span of a text is the one expected.
*/
static void
d_tests_span_is(
    const char* _text,
    size_t      _start,
    size_t      _end,
    const char* _want,
    const char* _what
)
{
    char span[512];

    // a span not there is nothing
    if ( (_start == D_GIT_TEXT_NONE) ||
         (_end < _start) ||
         ( (_end - _start) >= sizeof(span) ) )
    {
        span[0] = '\0';
    }
    else
    {
        memcpy(span, &_text[_start], _end - _start);
        span[_end - _start] = '\0';
    }

    d_tests_is(span, _want, _what);

    return;
}


/*
d_tests_count_is
  Counts one check that a number is the one expected.
*/
static void
d_tests_count_is(
    size_t      _got,
    size_t      _want,
    const char* _what
)
{
    g_checks++;

    // both numbers, for the reader of the failure
    if (_got != _want)
    {
        g_failed++;
        printf("  FAIL  %s: want %lu, got %lu\n", _what,
               (unsigned long)_want, (unsigned long)_got);
    }

    return;
}


/*
d_tests_read
  Reads a text that must read; a failure is reported as the check named.
*/
static bool
d_tests_read(
    struct d_git_text*   _text,
    const char*          _source,
    enum d_git_text_kind _kind,
    const char*          _what
)
{
    char       why[256];
    const bool read = d_git_text_read(_text, _source, strlen(_source), _kind,
                                      why, sizeof(why));

    d_tests_check(read, _what);

    return read;
}


/*
d_tests_load
  A file's bytes, allocated and terminated; NULL when it cannot be read.
*/
static char*
d_tests_load(
    const char* _path,
    size_t*     _out_length
)
{
    FILE* const file = fopen(_path, "rb");
    char*       text = NULL;
    size_t      used = 0u;
    size_t      room = 0u;

    // a file that opens
    if (!file)
    {
        return NULL;
    }

    // to its end, a block at a time
    for (;;)
    {
        // room for a block more
        if ( (room - used) < 4097u )
        {
            char* const grown = realloc(text, room + 65536u);

            // memory that ran out
            if (!grown)
            {
                free(text);
                (void)fclose(file);

                return NULL;
            }

            text  = grown;
            room += 65536u;
        }

        const size_t got = fread(&text[used], 1u, 4096u, file);

        used += got;

        // the end
        if (got < 4096u)
        {
            break;
        }
    }

    (void)fclose(file);
    text[used]  = '\0';
    *_out_length = used;

    return text;
}


/*
d_tests_put
  Writes bytes to a file, its directories made first; false when it
cannot.
*/
static bool
d_tests_put(
    const char* _path,
    const void* _bytes,
    size_t      _length
)
{
    FILE* const file = ( (d_fs_make_parent(_path)) )
                       ? fopen(_path, "wb")
                       : NULL;
    bool        ok   = (file != NULL);

    // the bytes, all of them
    if (file)
    {
        ok = (fwrite(_bytes, 1u, _length, file) == _length);
        ok = ( (fclose(file) == 0) && (ok) );
    }

    return ok;
}


/*
d_tests_put_text
  Writes a text to a file beneath a directory.
*/
static bool
d_tests_put_text(
    const char* _directory,
    const char* _name,
    const char* _text
)
{
    char path[1024];

    (void)snprintf(path, sizeof(path), "%s/%s", _directory, _name);

    return d_tests_put(path, _text, strlen(_text));
}


//==============================================================================
// kinds
//==============================================================================


/*
d_tests_kinds
  Files' kinds by their names, and by the names a program gives.
*/
static void
d_tests_kinds(void)
{
    puts("kinds");
    d_tests_check(d_git_text_kind_of("a/b/.gitignore") == D_GIT_TEXT_IGNORE,
                  "a .gitignore is an ignore file");
    d_tests_check(d_git_text_kind_of(".git/info/exclude")
                      == D_GIT_TEXT_IGNORE,
                  "info/exclude is an ignore file");
    d_tests_check(d_git_text_kind_of("x/.gitattributes")
                      == D_GIT_TEXT_ATTRIBUTES,
                  "a .gitattributes is an attribute file");
    d_tests_check(d_git_text_kind_of(".git/config") == D_GIT_TEXT_CONFIG,
                  "config is a configuration");
    d_tests_check(d_git_text_kind_of(".gitmodules") == D_GIT_TEXT_CONFIG,
                  ".gitmodules is a configuration");
    d_tests_check(d_git_text_kind_of("config.worktree") == D_GIT_TEXT_CONFIG,
                  "config.worktree is a configuration");
    d_tests_check(d_git_text_kind_of("notes.txt") == D_GIT_TEXT_OTHER,
                  "a text is no kind git reads");
    d_tests_check(d_git_text_kind_named("ignore") == D_GIT_TEXT_IGNORE,
                  "ignore names ignore files");
    d_tests_check(d_git_text_kind_named("attributes")
                      == D_GIT_TEXT_ATTRIBUTES,
                  "attributes names attribute files");
    d_tests_check(d_git_text_kind_named("config") == D_GIT_TEXT_CONFIG,
                  "config names configurations");
    d_tests_check(d_git_text_kind_named("nonsense") == D_GIT_TEXT_OTHER,
                  "nonsense names no kind");
    d_tests_is(d_git_text_kind_name(D_GIT_TEXT_ATTRIBUTES), "attributes",
               "a kind's own name");

    return;
}


//==============================================================================
// ignore files
//==============================================================================


/*
d_tests_ignore_file
  An ignore file's lines and patterns: comments only in the first column,
`\#` and `\!` literal, `!` negating, a trailing `/` for directories,
trailing blanks trimmed unless escaped, leading blanks kept.
*/
static void
d_tests_ignore_file(void)
{
    static const char source[] =
        "# comment\n\\#hash\n!keep.o\n*.o\nbuild/\n/root.txt\ndoc/*.md\n"
        "trail\\ \nspaces   \n\\!bang\n  lead\n#\n\n!\n";
    struct d_git_text text;

    puts("ignore files");

    // the file read
    if (!d_tests_read(&text, source, D_GIT_TEXT_IGNORE, "an ignore file reads"))
    {
        return;
    }

    d_tests_count_is(text.line_count, 14u, "fourteen lines");
    d_tests_count_is(text.pattern_count, 11u, "eleven patterns");
    d_tests_check(text.lines[0].kind == D_GIT_TEXT_COMMENT, "# a comment");
    d_tests_check(text.lines[1].kind == D_GIT_TEXT_PATTERN,
                  "\\# a pattern");
    d_tests_check(text.lines[11].kind == D_GIT_TEXT_COMMENT,
                  "# alone a comment");
    d_tests_check(text.lines[12].kind == D_GIT_TEXT_BLANK, "a blank line");
    d_tests_check(text.patterns[1].negated, "!keep.o negated");
    d_tests_is(d_git_text_string(&text, text.patterns[1].text), "keep.o",
               "the ! off the pattern matched");
    d_tests_check( (text.patterns[3].directory) &&
                   (text.patterns[3].anywhere),
                   "build/ a directory's, at any depth");
    d_tests_is(d_git_text_string(&text, text.patterns[3].text), "build",
               "the / off the pattern matched");
    d_tests_check(!text.patterns[4].anywhere, "/root.txt anchored");
    d_tests_check(!text.patterns[5].anywhere, "doc/*.md anchored");
    d_tests_span_is(source, text.patterns[6].start, text.patterns[6].end,
                    "trail\\ ", "an escaped trailing blank kept");
    d_tests_span_is(source, text.patterns[7].start, text.patterns[7].end,
                    "spaces", "trailing blanks trimmed");
    d_tests_check(!text.patterns[8].negated, "\\!bang not negated");
    d_tests_span_is(source, text.patterns[9].start, text.patterns[9].end,
                    "  lead", "leading blanks kept");
    d_tests_check( (text.patterns[10].negated) &&
                   (text.patterns[10].length == 0u),
                   "! alone an empty negation");
    d_tests_count_is(text.patterns[2].line, 3u, "a pattern's line");
    d_tests_count_is(text.lines[3].pattern, 2u, "a line's pattern");
    d_git_text_release(&text);

    return;
}


//==============================================================================
// attribute files
//==============================================================================


/*
d_tests_attribute_file
  An attribute file's patterns and states, its macros, a quoted pattern,
and the lines git refuses: a negated pattern, an invalid name.
*/
static void
d_tests_attribute_file(void)
{
    static const char source[] =
        "# c\n*.c text diff=cpp -merge !eol\n[attr]bin -diff -merge -text\n"
        "*.png bin\n\"sp ace.txt\" eol=crlf\n!neg.txt text\n"
        "bad.txt 9bad\n*.md\n  \t*.h   text\n\"unterminated text\n"
        "dir/ text\n[attr]bad!name x\n";
    struct d_git_text text;

    puts("attribute files");

    // the file read
    if (!d_tests_read(&text, source, D_GIT_TEXT_ATTRIBUTES,
                      "an attribute file reads"))
    {
        return;
    }

    const struct d_git_text_pattern* const c = &text.patterns[0];
    const struct d_git_text_attribute*     states =
        &text.attributes[c->first_attribute];

    d_tests_count_is(text.pattern_count, 9u, "nine patterns and macros");
    d_tests_count_is(c->attribute_count, 4u, "*.c has four states");
    d_tests_check(states[0].state == D_GIT_TEXT_SET, "text set");
    d_tests_check(states[1].state == D_GIT_TEXT_VALUE, "diff=cpp a value");
    d_tests_span_is(source, states[1].value_start, states[1].value_end,
                    "cpp", "the value");
    d_tests_check(states[2].state == D_GIT_TEXT_UNSET, "-merge unset");
    d_tests_check(states[3].state == D_GIT_TEXT_UNSPECIFIED,
                  "!eol unspecified");
    d_tests_check(text.patterns[1].macro, "[attr]bin a macro");
    d_tests_is(d_git_text_string(&text, text.patterns[1].text), "bin",
               "the macro's name");
    d_tests_check(text.lines[2].kind == D_GIT_TEXT_MACRO, "a macro's line");
    d_tests_check(text.patterns[3].quoted, "a quoted pattern");
    d_tests_is(d_git_text_string(&text, text.patterns[3].text), "sp ace.txt",
               "the quoted pattern unquoted");
    d_tests_check(text.lines[5].kind == D_GIT_TEXT_REFUSED,
                  "a negated pattern refused");
    d_tests_check(text.lines[11].kind == D_GIT_TEXT_REFUSED,
                  "an invalid macro name refused");
    d_tests_count_is(text.problem_count, 2u, "two problems");
    d_tests_count_is(text.patterns[5].attribute_count, 0u,
                     "a pattern alone has no states");
    d_tests_is(d_git_text_string(&text, text.patterns[7].text),
               "\"unterminated", "a bad quote read as a plain pattern");
    d_tests_check(text.patterns[8].directory, "dir/ a directory's");
    d_git_text_release(&text);

    return;
}


//==============================================================================
// configurations
//==============================================================================


/*
d_tests_config_file
  A configuration's sections and entries: names in any case, subsections
quoted and escaped, the old [a.B] form, a name alone, quoted values,
comments after values, escapes, a continued value, and an entry after a
header on its line.
*/
static void
d_tests_config_file(void)
{
    static const char source[] =
        "; top\n[Core]\n\tBare = false\n\tEditor = \"vim -f\" ; c\n\tflag\n"
        "[remote \"Origin\"]\n\turl = x\\\n\ty\n[a.B]\n"
        "\tk = \"q\\\"uote\" tail  # x\n[e \"s\\\\l\\\"q\"] n = v\n"
        "\tesc = a\\tb\\nc\\\\d\n\tsp = \" lead\"   \n";
    static const char* const keys[] = {
        "core.bare", "core.editor", "core.flag", "remote.Origin.url",
        "a.b.k", "e.s\\l\"q.n", "e.s\\l\"q.esc", "e.s\\l\"q.sp" };
    static const char* const values[] = {
        "false", "vim -f", NULL, "x y", "q\"uote tail", "v", "a\tb\nc\\d",
        " lead" };
    struct d_git_text text;
    char              what[128];

    puts("configurations");

    // the file read
    if (!d_tests_read(&text, source, D_GIT_TEXT_CONFIG,
                      "a configuration reads"))
    {
        return;
    }

    d_tests_count_is(text.section_count, 4u, "four sections");
    d_tests_count_is(text.entry_count, 8u, "eight entries");
    d_tests_count_is(text.broken, D_GIT_TEXT_NONE, "read to its end");

    // each entry's key and value, as git config --list gives them
    for (size_t at = 0u; ( (at < 8u) && (at < text.entry_count) ); ++at)
    {
        (void)snprintf(what, sizeof(what), "entry %lu's key",
                       (unsigned long)at);
        d_tests_is(d_git_text_string(&text, text.entries[at].key), keys[at],
                   what);
        (void)snprintf(what, sizeof(what), "entry %lu's value",
                       (unsigned long)at);
        d_tests_is(d_git_text_string(&text, text.entries[at].value),
                   (values[at]) ? values[at] : "(none)", what);
    }

    d_tests_check(text.sections[2].legacy, "[a.B] the old form");
    d_tests_is(d_git_text_string(&text, text.sections[2].prefix), "a.b",
               "the old form's subsection in lower case");
    d_tests_count_is(text.entries[3].last_line, 7u,
                     "a continued value's last line");
    d_tests_count_is(text.lines[7].entry, 3u, "a continued line's entry");
    d_tests_count_is(text.entries[5].line, 10u,
                     "an entry on its header's line");
    d_tests_check(text.lines[10].kind == D_GIT_TEXT_SECTION,
                  "that line a section's");
    d_tests_count_is(text.sections[0].entry_count, 3u,
                     "a section's entries");
    d_git_text_release(&text);

    // an entry before any section is read, as git reads it
    if (d_tests_read(&text, "x = 1\n", D_GIT_TEXT_CONFIG,
                     "an entry before any section reads"))
    {
        d_tests_count_is(text.broken, D_GIT_TEXT_NONE, "and is not refused");
        d_tests_is( (text.entry_count > 0u)
                    ? d_git_text_string(&text, text.entries[0].key) : "",
                    "x", "its key, the name alone");
        d_git_text_release(&text);
    }

    return;
}


// d_tests_refusal
//   struct: a configuration git refuses, and the line its fatal error
//   names.
struct d_tests_refusal
{
    const char* text;
    size_t      line;
};


/*
d_tests_config_refusals
  Where git stops reading a configuration, and the line it says it stopped
on -- one past the last when a header runs into the end, or a line's end
follows a subsection's closing quote, as git counts them.
*/
static void
d_tests_config_refusals(void)
{
    static const struct d_tests_refusal refusals[] =
    {
        { "[s]\n[bad\n",           2u }, { "[s]\n[bad",            3u },
        { "[s \"l\"]\n[bad\n",     2u }, { "[a]\n[b \n",           2u },
        { "[a]\n[b ",              2u }, { "[a]\n[b \"x",          2u },
        { "[a]\n[b \"x\"",         3u }, { "[a]\n[b \"x\"\n",      3u },
        { "[a]\n[b \"x\"y",        2u }, { "[",                    2u },
        { "[a]\nx y",              2u }, { "[a]\nx = \"abc",       2u },
        { "[a]\nx = \\q",          2u }, { "\xef\xbb",             2u },
        { "\xef\xbbX",             1u }, { "\xef\n",               2u },
        { "[a]\n[b.",              3u }, { "[a]\nx=1\n[b\n\n",     3u },
        { "[a]\n1x = 2\n",         2u }, { "[a]\n\tx = \"a\\",      3u }
    };
    char what[128];

    // each, at the line git names
    for (size_t at = 0u; at < (sizeof(refusals) / sizeof(refusals[0]));
         ++at)
    {
        struct d_git_text text;
        char              want[64];

        (void)snprintf(what, sizeof(what), "refusal %lu reads",
                       (unsigned long)at);

        // read, and stopped
        if (!d_tests_read(&text, refusals[at].text, D_GIT_TEXT_CONFIG, what))
        {
            continue;
        }

        (void)snprintf(want, sizeof(want), "bad config line %lu",
                       (unsigned long)refusals[at].line);
        (void)snprintf(what, sizeof(what), "refusal %lu's line",
                       (unsigned long)at);
        d_tests_is( (text.problem_count > 0u)
                    ? d_git_text_string(&text, text.problems[0].what)
                    : "(read to the end)", want, what);
        d_git_text_release(&text);
    }

    return;
}


/*
d_tests_config_marks
  A byte-order mark read past and kept; lines ending in \r\n read as lines,
and the file said to be written so.
*/
static void
d_tests_config_marks(void)
{
    struct d_git_text text;

    // a mark, then \r\n endings
    if (d_tests_read(&text, "\xef\xbb\xbf[a]\r\n\tb = c\r\n",
                     D_GIT_TEXT_CONFIG, "a marked configuration reads"))
    {
        d_tests_check(text.bom, "the mark seen");
        d_tests_check(text.crlf, "\\r\\n endings seen");
        d_tests_is(d_git_text_string(&text, text.entries[0].value), "c",
                   "no \\r in a value");
        d_git_text_release(&text);
    }

    return;
}


//==============================================================================
// wildmatch
//==============================================================================


// d_tests_wild
//   struct: one of wildmatch's vectors: whether the pattern matches the
//   text with D_GIT_TEXT_PATHNAME, with it and D_GIT_TEXT_CASEFOLD, with
//   neither, and with D_GIT_TEXT_CASEFOLD alone, each '1' or '0' -- as git's
//   own wildmatch answered them.
struct d_tests_wild
{
    const char* want;
    const char* pattern;
    const char* text;
};

static const struct d_tests_wild d_tests_wilds[] =
{
    { "1111", "main.c", "main.c" },
    { "0000", "main.c", "main.h" },
    { "1111", "m?in.c", "main.c" },
    { "0000", "m?in.c", "min.c" },
    { "1111", "*.c", "main.c" },
    { "0011", "*.c", "src/main.c" },
    { "1111", "*.c", ".c" },
    { "1111", "*", "" },
    { "1111", "", "" },
    { "0000", "", "x" },
    { "0000", "x", "" },
    { "0000", "?", "" },
    { "1111", "?", "a" },
    { "0011", "?", "/" },
    { "0011", "*", "a/b" },
    { "0011", "a*b", "a/b" },
    { "0011", "a?b", "a/b" },
    { "1111", "src/*.c", "src/main.c" },
    { "0011", "src/*.c", "src/lib/main.c" },
    { "1100", "src/**/*.c", "src/main.c" },
    { "1111", "src/**/*.c", "src/lib/main.c" },
    { "1111", "src/**/*.c", "src/lib/deep/main.c" },
    { "1100", "**/main.c", "main.c" },
    { "1111", "**/main.c", "a/b/main.c" },
    { "0000", "**/main.c", "a/b/xmain.c" },
    { "1111", "src/**", "src/" },
    { "1111", "src/**", "src/a" },
    { "1111", "src/**", "src/a/b/c" },
    { "0000", "src/**", "src" },
    { "1111", "src/**b", "src/b" },
    { "0011", "src/**b", "src/a/b" },
    { "1111", "src**/x", "src/x" },
    { "1111", "src**/x", "srcfoo/x" },
    { "1100", "a/**/b", "a/b" },
    { "1111", "a/**/b", "a/x/b" },
    { "1111", "a/**/b", "a/x/y/b" },
    { "0000", "a/**/b", "ab" },
    { "0000", "a/**/b", "a/xb" },
    { "1111", "**", "" },
    { "1111", "**", "a/b/c" },
    { "1111", "**/", "a/" },
    { "0000", "**/", "a" },
    { "1111", "**/**/c", "a/b/c" },
    { "1111", "***", "a/b" },
    { "0011", "x***y", "x/q/y" },
    { "1111", "[abc]", "b" },
    { "0000", "[abc]", "d" },
    { "1111", "[a-c]x", "bx" },
    { "0000", "[a-c]x", "dx" },
    { "1111", "[!a-c]", "d" },
    { "0000", "[!a-c]", "b" },
    { "1111", "[^a-c]", "z" },
    { "1111", "[]a]", "]" },
    { "1111", "[]a]", "a" },
    { "0000", "[]a]", "b" },
    { "0000", "[!]]", "]" },
    { "1111", "[!]]", "x" },
    { "1111", "[a-]", "-" },
    { "1111", "[a-]", "a" },
    { "1111", "[-a]", "-" },
    { "0000", "[a-c-e]", "d" },
    { "1111", "[a-c-e]", "-" },
    { "1111", "[\\]]", "]" },
    { "1111", "[\\a]", "a" },
    { "1111", "[\\-x]", "-" },
    { "0000", "[x\\-]", "\\" },
    { "1111", "\\*", "*" },
    { "0000", "\\*", "a" },
    { "1111", "\\?", "?" },
    { "0000", "a\\", "a" },
    { "0000", "a\\", "a\\" },
    { "1111", "foo\\bar", "foobar" },
    { "0000", "[a", "[a" },
    { "0000", "[a", "a" },
    { "0000", "a[", "a[" },
    { "1111", "[[:alpha:]]", "q" },
    { "0000", "[[:alpha:]]", "5" },
    { "1111", "[[:digit:]]x", "7x" },
    { "1111", "[[:alnum:]_]", "_" },
    { "1111", "[[:space:]]", " " },
    { "0000", "[[:space:]]", "x" },
    { "1111", "[[:upper:]]", "Q" },
    { "0101", "[[:upper:]]", "q" },
    { "1111", "[[:lower:]]", "q" },
    { "0101", "[[:lower:]]", "Q" },
    { "1111", "[[:xdigit:]]", "F" },
    { "0000", "[[:xdigit:]]", "g" },
    { "1111", "[[:punct:]]", "!" },
    { "0000", "[[:punct:]]", "a" },
    { "1111", "[[:blank:]]", "\t" },
    { "0000", "[[:cntrl:]]", "a" },
    { "1111", "[[:graph:]]", "~" },
    { "1111", "[[:print:]]", " " },
    { "0000", "[[:bogus:]]", "b" },
    { "0000", "[[:alpha:]", "a" },
    { "1111", "[[:alpha:][:digit:]]", "3" },
    { "1111", "[![:digit:]]", "x" },
    { "0000", "[![:digit:]]", "4" },
    { "0101", "[A-Z]", "q" },
    { "0101", "[a-z]", "Q" },
    { "0101", "README", "readme" },
    { "0101", "ReadMe.*", "README.MD" },
    { "1111", "[Mm]akefile", "makefile" },
    { "1111", "*[[:upper:]]*", "abCde" },
    { "1111", "*/*", "a/b" },
    { "0011", "*/*", "a/b/c" },
    { "1111", "*/b", "a/b" },
    { "1111", "/a", "/a" },
    { "1111", "a/", "a/" },
    { "0000", "a/", "a" },
    { "1111", ".*", ".hidden" },
    { "1111", "*", ".hidden" },
    { "1111", "*.tar.gz", "x.tar.gz" },
    { "0000", "*.tar.gz", "x.tar" },
    { "0000", "*a*a*a*a*a*a*a*b",
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaa" },
    { "1111", "*a*a*a*a*a*a*a*a",
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaa" },
    { "0000", "**/*a*a*a*a*b",
      "a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/"
      "aaaaaaaaaaaaaaaaaaaaa" },
    { "1100", "a/**/**/b", "a/b" },
    { "1111", "a/*/b", "a//b" },
    { "1111", "a*/", "ab/" },
    { "0000", "?.c", "\xc3\xa4.c" },
    { "1111", "*.c", "\xc3\xa4.c" },
    { "0000", "[\xc3\xa4]", "\xc3\xa4" },
    { "1100", "x/**/", "x/" },
    { "1111", "x/**/", "x/y/" },
};


/*
d_tests_wildmatch
  Each vector under each of the four flags; then patterns of many stars
against long texts, which git's wildmatch abandons early rather than
trying every way, so they take no time at all.
*/
static void
d_tests_wildmatch(void)
{
    static const unsigned flags[4] = {
        D_GIT_TEXT_PATHNAME, D_GIT_TEXT_PATHNAME | D_GIT_TEXT_CASEFOLD, 0u,
        D_GIT_TEXT_CASEFOLD };
    const size_t count = sizeof(d_tests_wilds) / sizeof(d_tests_wilds[0]);
    char         what[256];
    char         text[4096];
    char         pattern[256];

    puts("wildmatch");

    // each vector, under each flag
    for (size_t at = 0u; at < count; ++at)
    {
        const struct d_tests_wild* const wild = &d_tests_wilds[at];

        // each flag
        for (size_t mode = 0u; mode < 4u; ++mode)
        {
            const bool got = d_git_text_wildmatch(wild->pattern,
                                                  strlen(wild->pattern),
                                                  wild->text,
                                                  strlen(wild->text),
                                                  flags[mode]);

            (void)snprintf(what, sizeof(what), "[%s] against [%s], mode %lu",
                           wild->pattern, wild->text, (unsigned long)mode);
            d_tests_check(got == (wild->want[mode] == '1'), what);
        }
    }

    // the length given ends a pattern
    d_tests_check(d_git_text_wildmatch("a*xyz", 2u, "abc", 3u, 0u),
                  "a pattern ends at its length");

    memset(text, 'a', sizeof(text) - 1u);
    text[sizeof(text) - 1u] = '\0';

    // forty stars, each before an a, then a b none of the text has
    for (size_t at = 0u; at < 40u; ++at)
    {
        pattern[at * 2u]      = '*';
        pattern[at * 2u + 1u] = 'a';
    }

    pattern[80] = 'b';
    pattern[81] = '\0';

    const clock_t start   = clock();
    const bool    matched = d_git_text_wildmatch(pattern, 81u, text,
                                                 sizeof(text) - 1u, 0u);
    const double  seconds = (double)(clock() - start) / CLOCKS_PER_SEC;

    d_tests_check(!matched, "forty stars do not match");
    d_tests_check(seconds < 2.0, "forty stars give up at once");

    return;
}


//==============================================================================
// lookups
//==============================================================================


// d_tests_decision
//   struct: a path asked about, whether it is a directory, and what git
//   check-ignore -v --no-index said decides it: the scope, from 0, and the
//   line, from 1, 0 for nothing.
struct d_tests_decision
{
    const char* path;
    bool        directory;
    size_t      scope;
    size_t      line;
};


/*
d_tests_scope
  A scope of a lookup: a file's records, the directory it is in, and
whether its macros count.
*/
static struct d_git_text_scope
d_tests_scope(
    const struct d_git_text* _text,
    const char*              _base,
    bool                     _macros
)
{
    struct d_git_text_scope scope;

    scope.text   = _text;
    scope.base   = _base;
    scope.macros = _macros;

    return scope;
}


/*
d_tests_ignoring
  Ignoring over three scopes -- info/exclude, the top's .gitignore, and
sub/.gitignore -- each path decided as git check-ignore decided it: the
last pattern that matches, in the last scope that has one; a directory
above ignored deciding for what is below it, a negation there too.
*/
static void
d_tests_ignoring(void)
{
    static const struct d_tests_decision decisions[] =
    {
        { "a.log",         false, 1u, 1u }, { "keep.log",      false, 1u, 2u },
        { "sub/keep.log",  false, 2u, 2u }, { "sub/x.log",     false, 1u, 1u },
        { "build",         true,  1u, 3u }, { "build/x.c",     false, 1u, 3u },
        { "build/keep.c",  false, 1u, 3u }, { "top.txt",       false, 1u, 5u },
        { "sub/top.txt",   false, 2u, 1u }, { "x/top.txt",     false, 0u, 0u },
        { "doc/a/b/c.tmp", false, 1u, 6u }, { "doc/c.tmp",     false, 1u, 6u },
        { "secret",        false, 0u, 1u }, { "sub/secret",    false, 0u, 1u },
        { "sub/local/f",   false, 2u, 3u }, { "sub/local",     true,  2u, 3u },
        { "A.LOG",         false, 0u, 0u }, { "build",         false, 0u, 0u }
    };
    struct d_git_text texts[3];
    bool              read = true;
    char              what[128];

    puts("lookups");
    read = ( (d_tests_read(&texts[0], "secret\n", D_GIT_TEXT_IGNORE,
                           "info/exclude reads")) &&
             (read) );
    read = ( (d_tests_read(&texts[1],
                           "*.log\n!keep.log\nbuild/\n!build/keep.c\n"
                           "/top.txt\ndoc/**/*.tmp\n",
                           D_GIT_TEXT_IGNORE, "the top's .gitignore reads")) &&
             (read) );
    read = ( (d_tests_read(&texts[2], "*.txt\n!keep.log\nlocal/\n",
                           D_GIT_TEXT_IGNORE, "sub/.gitignore reads")) &&
             (read) );

    const struct d_git_text_scope scopes[3] = {
        d_tests_scope(&texts[0], "", false),
        d_tests_scope(&texts[1], "", false),
        d_tests_scope(&texts[2], "sub", false) };

    // each path, as git decided it
    for (size_t at = 0u;
         ( (read) && (at < (sizeof(decisions) / sizeof(decisions[0]))) );
         ++at)
    {
        const struct d_tests_decision* const decision = &decisions[at];
        const struct d_git_text_query        query    = {
            decision->path, strlen(decision->path), decision->directory,
            false };
        struct d_git_text_match              match;
        const bool decided = d_git_text_ignored(scopes, 3u, &query, &match);
        const size_t line  = (decided)
                             ? texts[match.scope].patterns[match.pattern].line
                               + 1u
                             : 0u;

        (void)snprintf(what, sizeof(what), "%s decided by scope %lu line %lu",
                       decision->path, (unsigned long)decision->scope,
                       (unsigned long)decision->line);
        d_tests_check( ( (decision->line == 0u) && (!decided) ) ||
                       ( (decided) && (match.scope == decision->scope) &&
                         (line == decision->line) ), what);
    }

    const struct d_git_text_query folded = { "A.LOG", 5u, false, true };
    struct d_git_text_match       match;

    d_tests_check( (read) &&
                   (d_git_text_ignored(scopes, 3u, &folded, &match)) &&
                   (match.ignored),
                   "A.LOG ignored when names compare in either case");

    // the texts let go of
    for (size_t at = 0u; at < 3u; ++at)
    {
        d_git_text_release(&texts[at]);
    }

    return;
}


/*
d_tests_attribute_values
  Gives a path's attributes as git check-attr -a writes them, a line each,
ordered by name.
*/
static void
d_tests_attribute_values(
    const struct d_git_text_scope* _scopes,
    size_t                         _count,
    const char*                    _path,
    char*                          _out,
    size_t                         _capacity
)
{
    const struct d_git_text_query query = { _path, strlen(_path), false,
                                            false };
    struct d_git_text_values      values;
    size_t                        used  = 0u;

    _out[0] = '\0';

    // the values, each a line
    if (d_git_text_attributes(_scopes, _count, &query, &values))
    {
        // each, as check-attr writes it
        for (size_t at = 0u; at < values.count; ++at)
        {
            const struct d_git_text_value* const value = &values.items[at];
            const int written = snprintf(
                &_out[used], _capacity - used, "%.*s: %.*s\n",
                (int)value->name_length, value->name,
                (int)((value->state == D_GIT_TEXT_VALUE)
                      ? value->value_length
                      : (value->state == D_GIT_TEXT_SET) ? 3u : 5u),
                (value->state == D_GIT_TEXT_VALUE) ? value->value
                : (value->state == D_GIT_TEXT_SET) ? "set" : "unset");

            used += ( (written > 0) &&
                      ((size_t)written < (_capacity - used)) )
                    ? (size_t)written
                    : 0u;
        }
    }

    d_git_text_values_release(&values);

    return;
}


/*
d_tests_attributing
  Attributes over four scopes -- the user's, the top's, sub's, and
info/attributes -- as git check-attr -a gave them: the highest scope's
last line deciding, macros expanding where they are set, `!` leaving an
attribute unspecified, git's own binary macro, and a macro outside the
top refused.
*/
static void
d_tests_attributing(void)
{
    static const char* const paths[] = {
        "a.c", "sub/a.c", "special.c", "x.bin", "w.x", "sub/w.y" };
    static const char* const wants[] = {
        "diff: cpp\neol: lf\nmine: set\ntext: set\n",
        "diff: unset\neol: lf\nmine: set\ntext: set\n",
        "diff: cpp\ntext: unset\n",
        "binary: set\ndiff: unset\nmerge: unset\ntext: unset\n",
        "foo: set\nrootmac: set\n",
        "submac: set\n" };
    struct d_git_text texts[4];
    bool              read = true;
    char              got[512];

    read = ( (d_tests_read(&texts[0], "[attr]mine text eol=lf\n",
                           D_GIT_TEXT_ATTRIBUTES, "the user's reads")) &&
             (read) );
    read = ( (d_tests_read(&texts[1],
                           "*.c diff=cpp mine\n*.bin binary\n"
                           "[attr]rootmac foo\n*.x rootmac\n",
                           D_GIT_TEXT_ATTRIBUTES, "the top's reads")) &&
             (read) );
    read = ( (d_tests_read(&texts[2],
                           "*.c -diff\n[attr]submac bar\n*.y submac\n",
                           D_GIT_TEXT_ATTRIBUTES, "sub's reads")) &&
             (read) );
    read = ( (d_tests_read(&texts[3], "special.c !mine -text\n",
                           D_GIT_TEXT_ATTRIBUTES,
                           "info/attributes reads")) &&
             (read) );

    const struct d_git_text_scope scopes[4] = {
        d_tests_scope(&texts[0], "", true),
        d_tests_scope(&texts[1], "", true),
        d_tests_scope(&texts[2], "sub", false),
        d_tests_scope(&texts[3], "", true) };

    // each path, as git gave it
    for (size_t at = 0u; ( (read) && (at < 6u) ); ++at)
    {
        d_tests_attribute_values(scopes, 4u, paths[at], got, sizeof(got));
        d_tests_is(got, wants[at], paths[at]);
    }

    // the texts let go of
    for (size_t at = 0u; at < 4u; ++at)
    {
        d_git_text_release(&texts[at]);
    }

    return;
}


//==============================================================================
// keys, values and quoting
//==============================================================================


/*
d_tests_keys
  Keys as git compares them, and the keys git refuses.
*/
static void
d_tests_keys(void)
{
    struct d_git_buffer key;
    size_t              section    = 0u;
    size_t              subsection = 0u;

    puts("keys, values and quoting");
    memset(&key, 0, sizeof(key));
    d_tests_check(d_git_text_key(&key, "Remote.Origin.URL"),
                  "a key with a subsection");
    d_tests_is(key.text, "remote.Origin.url",
               "the subsection kept, the rest in lower case");
    d_git_buffer_free(&key);
    d_tests_check(d_git_text_key(&key, "branch.a.b.c.merge"),
                  "a subsection with dots");
    d_tests_is(key.text, "branch.a.b.c.merge", "its dots kept");
    d_git_buffer_free(&key);
    d_tests_check(!d_git_text_key(&key, "nosection"), "no section refused");
    d_git_buffer_free(&key);
    d_tests_check(!d_git_text_key(&key, "a.1b"),
                  "a name not beginning with a letter refused");
    d_git_buffer_free(&key);
    d_tests_check(!d_git_text_key(&key, "a b.c"),
                  "a blank in a section refused");
    d_git_buffer_free(&key);
    d_tests_check(!d_git_text_key(&key, "a.x\ny.z"),
                  "a newline in a subsection refused");
    d_git_buffer_free(&key);
    d_git_text_key_parts("remote.origin.url", &section, &subsection);
    d_tests_check( (section == 6u) && (subsection == 6u),
                   "a key's parts");
    d_git_text_key_parts("core.bare", &section, &subsection);
    d_tests_check( (section == 4u) && (subsection == D_GIT_TEXT_NONE),
                   "a key without a subsection");

    return;
}


/*
d_tests_values
  Values as git reads booleans and numbers.
*/
static void
d_tests_values(void)
{
    static const char* const trues[]  = { "true", "Yes", "ON", "1", "-2",
                                          NULL };
    static const char* const falses[] = { "false", "No", "off", "0", "" };
    bool                     flag     = false;
    int64_t                  number   = 0;

    // each true, a name alone among them
    for (size_t at = 0u; at < 6u; ++at)
    {
        d_tests_check( (d_git_text_bool(trues[at], &flag)) && (flag),
                       (trues[at]) ? trues[at] : "a name alone is true");
    }

    // each false
    for (size_t at = 0u; at < 5u; ++at)
    {
        d_tests_check( (d_git_text_bool(falses[at], &flag)) && (!flag),
                       falses[at][0] ? falses[at] : "\"\" is false");
    }

    d_tests_check(!d_git_text_bool("maybe", &flag), "maybe is no boolean");
    d_tests_check( (d_git_text_int("1k", &number)) && (number == 1024),
                   "1k is 1024");
    d_tests_check( (d_git_text_int("2M", &number)) &&
                   (number == 2 * 1024 * 1024), "2M");
    d_tests_check( (d_git_text_int("-3g", &number)) &&
                   (number == -3LL * 1024 * 1024 * 1024), "-3g");
    d_tests_check( (d_git_text_int("0x10", &number)) && (number == 16),
                   "0x10 is hex");
    d_tests_check( (d_git_text_int("010", &number)) && (number == 8),
                   "010 is octal");
    d_tests_check(!d_git_text_int("12x", &number), "12x is no number");
    d_tests_check(!d_git_text_int("9223372036854775807k", &number),
                  "a number past 64 bits refused");
    d_tests_check(!d_git_text_int(NULL, &number),
                  "a name alone is no number");

    return;
}


/*
d_tests_quoting
  Values, subsections and paths quoted as git writes them, and text read
from C's quotes.
*/
static void
d_tests_quoting(void)
{
    static const char* const values[][2] = {
        { "plain",       "plain" },
        { " lead",       "\" lead\"" },
        { "trail ",      "\"trail \"" },
        { "a;b",         "\"a;b\"" },
        { "a#b",         "\"a#b\"" },
        { "tab\there",   "tab\\there" },
        { "q\"uote",     "q\\\"uote" },
        { "back\\slash", "back\\\\slash" },
        { "nl\nline",    "nl\\nline" },
        { "",            "" } };
    struct d_git_buffer quoted;
    size_t              used = 0u;

    memset(&quoted, 0, sizeof(quoted));

    // each value
    for (size_t at = 0u; at < (sizeof(values) / sizeof(values[0])); ++at)
    {
        d_tests_check(d_git_text_quote_value(&quoted, values[at][0]),
                      "a value quotes");
        d_tests_is( (quoted.text) ? quoted.text : "", values[at][1],
                    (values[at][0][0]) ? values[at][0] : "an empty value");
        d_git_buffer_free(&quoted);
    }

    (void)d_git_text_quote_subsection(&quoted, "we\"ird\\name");
    d_tests_is(quoted.text, "we\\\"ird\\\\name", "a subsection's escapes");
    d_git_buffer_free(&quoted);
    (void)d_git_text_quote_path(&quoted, "plain/path.c", 12u);
    d_tests_is(quoted.text, "plain/path.c", "a plain path as it is");
    d_git_buffer_free(&quoted);
    (void)d_git_text_quote_path(&quoted, "tab\there", 8u);
    d_tests_is(quoted.text, "\"tab\\there\"", "a tab quoted");
    d_git_buffer_free(&quoted);
    (void)d_git_text_quote_path(&quoted, "\xc3\xa4.txt", 6u);
    d_tests_is(quoted.text, "\"\\303\\244.txt\"",
               "bytes past ASCII in octal, as core.quotePath has it");
    d_git_buffer_free(&quoted);
    d_tests_check( (d_git_text_unquote(&quoted, "\"a\\tb\\101\" rest", 15u,
                                       &used)) && (used == 10u),
                   "a quoted text read");
    d_tests_is(quoted.text, "a\tbA", "its escapes, octal among them");
    d_git_buffer_free(&quoted);
    d_tests_check(!d_git_text_unquote(&quoted, "\"open", 5u, &used),
                  "an unclosed quote refused");
    d_git_buffer_free(&quoted);
    d_tests_check(!d_git_text_unquote(&quoted, "bare", 4u, &used),
                  "unquoted text refused");
    d_git_buffer_free(&quoted);

    return;
}


//==============================================================================
// documents
//==============================================================================


// d_tests_edit_kind
//   enum: which of git config's changes an edit is.
enum d_tests_edit_kind
{
    D_TESTS_SET = 0,
    D_TESTS_ADD,
    D_TESTS_UNSET,
    D_TESTS_RENAME,
    D_TESTS_REMOVE
};

// d_tests_edit
//   struct: a change to a configuration, as git config made it, and the
//   text it left -- the text it was, for one refused. Three differ from
//   git config's on purpose: a value set keeps a comment after it and the
//   name as written, which git config rewrites, and a file of \r\n endings
//   gets one on a line added, where git config writes \n.
struct d_tests_edit
{
    enum d_tests_edit_kind kind;
    bool                   kept;
    const char*            name;
    const char*            value;
    const char*            before;
    const char*            after;
};

static const struct d_tests_edit d_tests_edits[] =
{
    { D_TESTS_SET, true, "core.editor", "vim",
      "[core]\n\tbare = false\n",
      "[core]\n\tbare = false\n\teditor = vim\n" },
    { D_TESTS_SET, true, "user.name", "A B",
      "[core]\n\tbare = false\n",
      "[core]\n\tbare = false\n[user]\n\tname = A B\n" },
    { D_TESTS_SET, true, "remote.origin.url", "x",
      "",
      "[remote \"origin\"]\n\turl = x\n" },
    { D_TESTS_SET, true, "core.bare", "true",
      "[core]\n\tbare = false ; c\n",
      "[core]\n\tbare = true ; c\n" },
    { D_TESTS_ADD, true, "core.editor", "b",
      "[core]\n\tbare = false\n\teditor = a\n",
      "[core]\n\tbare = false\n\teditor = a\n\teditor = b\n" },
    { D_TESTS_ADD, true, "core.editor", "b",
      "[core]\n\teditor = a\n[user]\n\tname = n\n[core]\n\tfilemode = true\n",
      "[core]\n\teditor = a\n[user]\n\tname = n\n[core]\n\tfilemode = true\n"
      "\teditor = b\n" },
    { D_TESTS_UNSET, true, "core.editor", NULL,
      "[core]\n\tbare = false\n\teditor = a\n",
      "[core]\n\tbare = false\n" },
    { D_TESTS_UNSET, true, "core.editor", NULL,
      "[core]\n\teditor = a\n[user]\n\tname = n\n",
      "[user]\n\tname = n\n" },
    { D_TESTS_UNSET, true, "core.editor", "all",
      "[core]\n\teditor = a\n\teditor = b\n\tx = 1\n",
      "[core]\n\tx = 1\n" },
    { D_TESTS_UNSET, true, "core.editor", NULL,
      "# top\n[core]\n\t# about\n\teditor = a\n[user]\n\tname = n\n",
      "# top\n[core]\n\t# about\n[user]\n\tname = n\n" },
    { D_TESTS_RENAME, true, "remote.origin", "remote.up",
      "[remote \"origin\"]\n\turl = x\n[branch \"main\"]\n\tremote = origin\n",
      "[remote \"up\"]\n\turl = x\n[branch \"main\"]\n\tremote = origin\n" },
    { D_TESTS_REMOVE, true, "a", NULL,
      "[a]\n\tx = 1\n[b]\n\ty = 2\n[a]\n\tz = 3\n",
      "[b]\n\ty = 2\n" },
    { D_TESTS_SET, true, "a.b", " lead",
      "",
      "[a]\n\tb = \" lead\"\n" },
    { D_TESTS_SET, true, "a.b", "x;y",
      "",
      "[a]\n\tb = \"x;y\"\n" },
    { D_TESTS_SET, true, "a.b", "x#y",
      "",
      "[a]\n\tb = \"x#y\"\n" },
    { D_TESTS_SET, true, "a.b", "tab\there",
      "",
      "[a]\n\tb = tab\\there\n" },
    { D_TESTS_SET, true, "a.b", "q\"uote",
      "",
      "[a]\n\tb = q\\\"uote\n" },
    { D_TESTS_SET, true, "a.b", "back\\slash",
      "",
      "[a]\n\tb = back\\\\slash\n" },
    { D_TESTS_SET, true, "a.b", "nl\nline",
      "",
      "[a]\n\tb = nl\\nline\n" },
    { D_TESTS_SET, true, "a.b", "trail ",
      "",
      "[a]\n\tb = \"trail \"\n" },
    { D_TESTS_SET, true, "branch.we\"ird.remote", "x",
      "",
      "[branch \"we\\\"ird\"]\n\tremote = x\n" },
    { D_TESTS_SET, true, "branch.back\\slash.remote", "x",
      "",
      "[branch \"back\\\\slash\"]\n\tremote = x\n" },
    { D_TESTS_SET, true, "Core.Editor", "b",
      "[core]\n\teditor = a\n",
      "[core]\n\teditor = b\n" },
    { D_TESTS_SET, true, "core.newKey", "b",
      "[Core]\n\tEditor = a\n",
      "[Core]\n\tEditor = a\n\tnewKey = b\n" },
    { D_TESTS_SET, true, "core.editor", "vim",
      "[core]\r\n\tbare = false\r\n",
      "[core]\r\n\tbare = false\r\n\teditor = vim\r\n" },
    { D_TESTS_SET, true, "core.editor", "vim",
      "\xef\xbb\xbf[core]\n\tbare = false\n",
      "\xef\xbb\xbf[core]\n\tbare = false\n\teditor = vim\n" },
    { D_TESTS_SET, true, "core.editor", "vim",
      "[core]\n\tbare = false",
      "[core]\n\tbare = false\n\teditor = vim\n" },
    { D_TESTS_SET, true, "x.y", "z",
      "[core]\n\tbare = false",
      "[core]\n\tbare = false\n[x]\n\ty = z\n" },
    { D_TESTS_SET, true, "core.editor", "vim",
      "[core] bare = false\n",
      "[core] bare = false\n\teditor = vim\n" },
    { D_TESTS_SET, true, "core.bare", "false",
      "[core]\n\tbare\n",
      "[core]\n\tbare = false\n" },
    { D_TESTS_SET, true, "core.sub.y", "2",
      "[core.sub]\n\tx = 1\n",
      "[core.sub]\n\tx = 1\n\ty = 2\n" },
    { D_TESTS_SET, true, "core.sub.y", "2",
      "[core.Sub]\n\tx = 1\n",
      "[core.Sub]\n\tx = 1\n\ty = 2\n" },
    { D_TESTS_SET, true, "A.b.y", "2",
      "[a \"b\"]\n\tx = 1\n",
      "[a \"b\"]\n\tx = 1\n\ty = 2\n" },
    { D_TESTS_SET, true, "a.x", "3",
      "[a]\n\tx = 1 \\\n  continued\n\ty = 2\n",
      "[a]\n\tx = 3\n\ty = 2\n" },
    { D_TESTS_SET, false, "a.x", "3",
      "[a]\n\tx = 1\n\tx = 2\n",
      "[a]\n\tx = 1\n\tx = 2\n" },
    { D_TESTS_SET, true, "a.y", "1",
      "[a]\n[b]\n\tx = 1\n",
      "[a]\n\ty = 1\n[b]\n\tx = 1\n" },
    { D_TESTS_UNSET, false, "a.nope", NULL,
      "[a]\n\tx = 1\n",
      "[a]\n\tx = 1\n" },
    { D_TESTS_REMOVE, false, "nope", NULL,
      "[a]\n\tx = 1\n",
      "[a]\n\tx = 1\n" },
    { D_TESTS_RENAME, true, "a", "b.c",
      "[a]\n\tx = 1\n",
      "[b \"c\"]\n\tx = 1\n" },
    { D_TESTS_SET, true, "1a.b", "c",
      "[a]\n\tx = 1\n",
      "[a]\n\tx = 1\n[1a]\n\tb = c\n" },
    { D_TESTS_SET, false, "a.1b", "c",
      "[a]\n\tx = 1\n",
      "[a]\n\tx = 1\n" },
    { D_TESTS_SET, true, "a.y", "2",
      "[a] ; comment\n\tx = 1\n",
      "[a] ; comment\n\tx = 1\n\ty = 2\n" },
    { D_TESTS_UNSET, true, "a.x", NULL,
      "[a]\n\tx = 1 # note\n",
      "" },
    { D_TESTS_SET, true, "a.z", "2",
      "[a]\n\tx = 1\n[a \"s\"]\n\ty = 1\n",
      "[a]\n\tx = 1\n\tz = 2\n[a \"s\"]\n\ty = 1\n" },
    { D_TESTS_SET, true, "a.s.z", "2",
      "[a \"s\"]\n\ty = 1\n[a]\n\tx = 1\n",
      "[a \"s\"]\n\ty = 1\n\tz = 2\n[a]\n\tx = 1\n" },
    { D_TESTS_SET, true, "a.x", "v",
      "[a]\n\tx = 1 \\\n",
      "[a]\n\tx = v\n" },
    { D_TESTS_SET, true, "a.x", "v",
      "[a]\n\tx = 1 \\\n#c\n\ty = 2\n",
      "[a]\n\tx = v\n\ty = 2\n" },
    { D_TESTS_SET, true, "a.x", "v",
      "[a]\n\tx = 1 \\\n  2\n",
      "[a]\n\tx = v\n" },
    { D_TESTS_UNSET, true, "a.x", NULL,
      "[a]\n\tx = 1 \\\n#c\n\ty = 2\n",
      "[a]\n\ty = 2\n" },
};


/*
d_tests_edit_one
  An edit made to a configuration of its text; whether it was kept.
*/
static bool
d_tests_edit_one(
    struct d_git_doc*          _doc,
    const struct d_tests_edit* _edit
)
{
    // a table of the edits
    switch (_edit->kind)
    {
        case D_TESTS_SET:
            return d_git_doc_set(_doc, _edit->name, _edit->value);
        case D_TESTS_ADD:
            return d_git_doc_add(_doc, _edit->name, _edit->value);
        case D_TESTS_UNSET:
            return d_git_doc_unset(_doc, _edit->name, (_edit->value != NULL));
        case D_TESTS_RENAME:
            return d_git_doc_rename_section(_doc, _edit->name, _edit->value);
        default:
            return d_git_doc_remove_section(_doc, _edit->name);
    }
}


/*
d_tests_config_edits
  Each of git config's changes, made as git config made it: set, add,
unset and unset-all, rename-section, remove-section; a section emptied
taken away with its comments; values and subsections quoted; the old
section form, a continued value, \r\n endings and a mark kept; and each
refusal leaving the text as it was.
*/
static void
d_tests_config_edits(void)
{
    const size_t count = sizeof(d_tests_edits) / sizeof(d_tests_edits[0]);
    char         what[128];
    char         why[256];

    puts("documents");

    // each edit
    for (size_t at = 0u; at < count; ++at)
    {
        const struct d_tests_edit* const edit = &d_tests_edits[at];
        struct d_git_doc* const          doc  =
            d_git_doc_make("config", D_GIT_TEXT_CONFIG, edit->before,
                           strlen(edit->before), why, sizeof(why));

        (void)snprintf(what, sizeof(what), "edit %lu of %s is made",
                       (unsigned long)at, edit->name);

        // the document made
        if (!doc)
        {
            d_tests_check(false, what);

            continue;
        }

        const bool kept = d_tests_edit_one(doc, edit);

        (void)snprintf(what, sizeof(what), "edit %lu of %s %s",
                       (unsigned long)at, edit->name,
                       (edit->kept) ? "kept" : "refused");
        d_tests_check(kept == edit->kept, what);
        d_tests_check( (kept) || (d_git_doc_problem(doc)[0] != '\0'),
                       "a refusal says why");
        (void)snprintf(what, sizeof(what), "edit %lu's text",
                       (unsigned long)at);
        d_tests_is(d_git_doc_text(doc, NULL), edit->after, what);
        d_git_doc_free(doc);
    }

    return;
}


/*
d_tests_make
  A document of a text, the check named failing when it cannot be made.
*/
static struct d_git_doc*
d_tests_make(
    const char*          _path,
    enum d_git_text_kind _kind,
    const char*          _text,
    const char*          _what
)
{
    char                    why[256];
    struct d_git_doc* const doc = d_git_doc_make(_path, _kind, _text,
                                                 strlen(_text), why,
                                                 sizeof(why));

    d_tests_check(doc != NULL, _what);

    return doc;
}


/*
d_tests_line_edits
  Lines inserted, set and removed in an ignore file, each giving the line
it made; a line's ending kept as the file's; a change that would merge two
lines, or make a pattern of another's comment, refused.
*/
static void
d_tests_line_edits(void)
{
    struct d_git_doc* const doc = d_tests_make(".gitignore",
                                               D_GIT_TEXT_IGNORE,
                                               "*.o\r\nbuild/\r\n",
                                               "an ignore file is made");

    // the file made
    if (!doc)
    {
        return;
    }

    d_tests_count_is(d_git_doc_insert_line(doc, 0u, "# built"), 0u,
                     "a line inserted first");
    d_tests_count_is(d_git_doc_insert_line(doc, 3u, "*.tmp"), 3u,
                     "a line added after the last");
    d_tests_count_is(d_git_doc_set_line(doc, 1u, "*.obj"), 1u,
                     "a line set");
    d_tests_check(d_git_doc_remove_line(doc, 2u), "a line removed");
    d_tests_is(d_git_doc_text(doc, NULL), "# built\r\n*.obj\r\n*.tmp\r\n",
               "\\r\\n endings kept on the lines made");
    d_tests_count_is(d_git_doc_set_line(doc, 1u, "two\nlines"),
                     D_GIT_TEXT_NONE, "a line of two lines refused");
    d_tests_check(!d_git_doc_splice(doc, 7u, 9u, "", 0u),
                  "an ending taken away, merging two lines, refused");
    d_tests_check(d_git_doc_splice(doc, 9u, 14u, "*.exe", 5u),
                  "a pattern changed in place");
    d_tests_is(d_git_doc_text(doc, NULL), "# built\r\n*.exe\r\n*.tmp\r\n",
               "the splice kept");
    d_tests_count_is(d_git_doc_changes(doc), 5u, "five changes kept");
    d_tests_check(d_git_doc_changed(doc), "the text is not as it was made");
    d_git_doc_free(doc);

    return;
}


/*
d_tests_state_edits
  States set on a pattern's line -- one of the name replaced, another
added -- and taken off; a name git refuses refused.
*/
static void
d_tests_state_edits(void)
{
    struct d_git_doc* const doc = d_tests_make(".gitattributes",
                                               D_GIT_TEXT_ATTRIBUTES,
                                               "*.c text diff=cpp\n"
                                               "*.png -text -diff\n",
                                               "an attribute file is made");

    // the file made
    if (!doc)
    {
        return;
    }

    d_tests_check(d_git_doc_set_state(doc, 0u, "diff", D_GIT_TEXT_UNSET,
                                      NULL), "a state replaced");
    d_tests_check(d_git_doc_set_state(doc, 0u, "eol", D_GIT_TEXT_VALUE,
                                      "lf"), "a state added");
    d_tests_check(d_git_doc_set_state(doc, 1u, "merge",
                                      D_GIT_TEXT_UNSPECIFIED, NULL),
                  "an unspecified state added");
    d_tests_check(d_git_doc_remove_state(doc, 1u, "text"),
                  "a state taken off");
    d_tests_check(!d_git_doc_set_state(doc, 0u, "bad name",
                                       D_GIT_TEXT_SET, NULL),
                  "a name git refuses refused");
    d_tests_check(!d_git_doc_set_state(doc, 0u, "eol", D_GIT_TEXT_VALUE,
                                       "a b"),
                  "a value with a blank refused");
    d_tests_is(d_git_doc_text(doc, NULL),
               "*.c text -diff eol=lf\n*.png -diff !merge\n",
               "the states as set");
    d_git_doc_free(doc);

    return;
}


/*
d_tests_entry_edits
  One entry of several given a value, or its name alone, and taken away;
a change to an entry that would move the next into another section
refused.
*/
static void
d_tests_entry_edits(void)
{
    struct d_git_doc* const doc = d_tests_make("config", D_GIT_TEXT_CONFIG,
                                               "[a]\n\tx = 1\n\tx = 2\n"
                                               "\ty = 3\n",
                                               "a configuration is made");

    // the file made
    if (!doc)
    {
        return;
    }

    d_tests_check(d_git_doc_set_entry(doc, 1u, "two"),
                  "the second of a key's entries set");
    d_tests_check(d_git_doc_set_entry(doc, 0u, NULL),
                  "an entry made a name alone");
    d_tests_is(d_git_doc_text(doc, NULL),
               "[a]\n\tx\n\tx = two\n\ty = 3\n", "the entries as set");
    d_tests_check(!d_git_doc_splice(doc, 7u, 8u, "x\n[b]", 5u),
                  "a splice that moves an entry into another section "
                  "refused");
    d_tests_check(d_git_doc_remove_entry(doc, 0u), "an entry taken away");
    d_tests_is(d_git_doc_text(doc, NULL), "[a]\n\tx = two\n\ty = 3\n",
               "the entry gone");
    d_tests_check(d_git_doc_set(doc, "a.x", "1"), "a key of one entry set");
    d_tests_is(d_git_doc_text(doc, NULL), "[a]\n\tx = 1\n\ty = 3\n",
               "its value in place");
    d_git_doc_free(doc);

    return;
}


/*
d_tests_saving
  A document saved, its directories made; saved again unchanged; read back
from its file; a file that is not there not read.
*/
static void
d_tests_saving(
    const char* _directory
)
{
    char                    path[1024];
    char                    why[256];
    struct d_git_doc* const doc = d_tests_make("", D_GIT_TEXT_CONFIG,
                                               "\xef\xbb\xbf[a]\n\tb = 1\n",
                                               "a marked file is made");

    (void)snprintf(path, sizeof(path), "%s/saved/sub/config", _directory);
    (void)remove(path);

    // the file made
    if (!doc)
    {
        return;
    }

    d_tests_check(d_git_doc_set(doc, "a.c", "2"), "a key set on it");
    d_tests_check(d_git_doc_save(doc, path, why, sizeof(why))
                      == D_GIT_SAVE_WRITTEN, "saved, its directories made");
    d_tests_check(d_git_doc_save(doc, path, why, sizeof(why))
                      == D_GIT_SAVE_UNCHANGED, "saved again: unchanged");
    d_git_doc_free(doc);

    struct d_git_doc* const back = d_git_doc_read(path, D_GIT_TEXT_OTHER, why,
                                                  sizeof(why));

    d_tests_check( (back) &&
                   (d_git_doc_kind(back) == D_GIT_TEXT_CONFIG),
                   "read back, its kind its name's");
    d_tests_is( (back) ? d_git_doc_text(back, NULL) : NULL,
                "\xef\xbb\xbf[a]\n\tb = 1\n\tc = 2\n",
                "the mark kept on disk");
    d_git_doc_free(back);
    (void)snprintf(path, sizeof(path), "%s/saved/missing/config",
                   _directory);
    d_tests_check(d_git_doc_read(path, D_GIT_TEXT_OTHER, why, sizeof(why))
                      == NULL, "a file that is not there not read");

    return;
}


//==============================================================================
// a repository built from nothing
//==============================================================================


// d_tests_world
//   struct: a repository the checks build: its work tree, its directory,
//   and the repository open on it.
struct d_tests_world
{
    char               top[1024];
    char               git_dir[1040];
    struct d_git_repo* repo;
};


/*
d_tests_adler
  Adler-32, as zlib ends a stream with it.
*/
static uint32_t
d_tests_adler(
    const unsigned char* _bytes,
    size_t               _length
)
{
    uint32_t low  = 1u;
    uint32_t high = 0u;

    // each byte
    for (size_t at = 0u; at < _length; ++at)
    {
        low  = (low + _bytes[at]) % 65521u;
        high = (high + low) % 65521u;
    }

    return (high << 16) | low;
}


/*
d_tests_stored
  Bytes as a zlib stream of stored blocks, as deflate writes what it does
not compress; allocated, NULL when memory runs out.
*/
static unsigned char*
d_tests_stored(
    const unsigned char* _bytes,
    size_t               _length,
    size_t*              _out_length
)
{
    const size_t         blocks = (_length / 65535u) + 1u;
    unsigned char* const out    = malloc(_length + (blocks * 5u) + 6u);
    size_t               used   = 2u;
    size_t               at     = 0u;

    // room for it
    if (!out)
    {
        return NULL;
    }

    out[0] = 0x78u;
    out[1] = 0x01u;

    // each block, the last marked so
    for (size_t block = 0u; block < blocks; ++block)
    {
        const size_t size = ( (_length - at) > 65535u ) ? 65535u
                                                        : (_length - at);

        out[used++] = (block + 1u == blocks) ? 1u : 0u;
        out[used++] = (unsigned char)(size & 0xffu);
        out[used++] = (unsigned char)(size >> 8);
        out[used++] = (unsigned char)(~size & 0xffu);
        out[used++] = (unsigned char)((~size >> 8) & 0xffu);
        memcpy(&out[used], &_bytes[at], size);
        used += size;
        at   += size;
    }

    const uint32_t adler = d_tests_adler(_bytes, _length);

    out[used++]  = (unsigned char)(adler >> 24);
    out[used++]  = (unsigned char)(adler >> 16);
    out[used++]  = (unsigned char)(adler >> 8);
    out[used++]  = (unsigned char)adler;
    *_out_length = used;

    return out;
}


/*
d_tests_object_path
  The loose file of an object's name: objects/xx/yyyy...
*/
static void
d_tests_object_path(
    const struct d_tests_world* _world,
    const struct d_git_oid*     _oid,
    char*                       _out,
    size_t                      _capacity
)
{
    char hex[(2u * D_GIT_OID_MOST) + 1u];

    d_git_repo_hex(_world->repo, _oid, hex);
    (void)snprintf(_out, _capacity, "%s/objects/%.2s/%s", _world->git_dir,
                   hex, &hex[2]);

    return;
}


/*
d_tests_object
  An object written loose, as git writes one, deflated in stored blocks:
its name, from its type and content.
*/
static bool
d_tests_object(
    struct d_tests_world*  _world,
    enum d_git_object_type _type,
    const void*            _content,
    size_t                 _length,
    struct d_git_oid*      _out
)
{
    char                 header[64];
    char                 path[1200];
    const int            head = snprintf(header, sizeof(header), "%s %lu",
                                         d_git_object_type_name(_type),
                                         (unsigned long)_length);
    unsigned char* const raw  = malloc((size_t)head + 1u + _length);
    size_t               size = 0u;

    // room for the object
    if (!raw)
    {
        return false;
    }

    memcpy(raw, header, (size_t)head + 1u);
    memcpy(&raw[head + 1], _content, _length);
    d_git_repo_hash_object(_world->repo, _type, _content, _length, _out);
    d_tests_object_path(_world, _out, path, sizeof(path));

    unsigned char* const deflated = d_tests_stored(raw, (size_t)head + 1u
                                                        + _length, &size);
    const bool           put      = ( (deflated) &&
                                      (d_tests_put(path, deflated, size)) );

    free(raw);
    free(deflated);

    return put;
}


/*
d_tests_blob
  A blob of a text, written; its name.
*/
static struct d_git_oid
d_tests_blob(
    struct d_tests_world* _world,
    const char*           _text
)
{
    struct d_git_oid oid;

    memset(&oid, 0, sizeof(oid));
    d_tests_check(d_tests_object(_world, D_GIT_OBJECT_BLOB, _text,
                                 strlen(_text), &oid), "a blob is written");

    return oid;
}


// d_tests_entry
//   struct: one entry a tree the checks write holds.
struct d_tests_entry
{
    const char*      mode;
    const char*      name;
    struct d_git_oid oid;
};


/*
d_tests_tree
  A tree of entries, given in git's order, written; its name.
*/
static struct d_git_oid
d_tests_tree(
    struct d_tests_world*       _world,
    const struct d_tests_entry* _entries,
    size_t                      _count
)
{
    const size_t     hash = d_git_repo_hash_size(_world->repo);
    unsigned char    content[2048];
    size_t           used = 0u;
    struct d_git_oid oid;

    memset(&oid, 0, sizeof(oid));

    // each entry: its mode, a blank, its name, a NUL and its name's bytes
    for (size_t at = 0u; at < _count; ++at)
    {
        const int head = snprintf((char*)&content[used],
                                  sizeof(content) - used, "%s %s",
                                  _entries[at].mode, _entries[at].name);

        used += (size_t)head + 1u;
        memcpy(&content[used], _entries[at].oid.bytes, hash);
        used += hash;
    }

    d_tests_check(d_tests_object(_world, D_GIT_OBJECT_TREE, content, used,
                                 &oid), "a tree is written");

    return oid;
}


/*
d_tests_commit
  A commit of a tree, with parents, at a time, written; its name.
*/
static struct d_git_oid
d_tests_commit(
    struct d_tests_world*   _world,
    const struct d_git_oid* _tree,
    const struct d_git_oid* _parents,
    size_t                  _count,
    long                    _time
)
{
    char             content[2048];
    char             hex[(2u * D_GIT_OID_MOST) + 1u];
    size_t           used = 0u;
    struct d_git_oid oid;

    memset(&oid, 0, sizeof(oid));
    d_git_repo_hex(_world->repo, _tree, hex);
    used += (size_t)snprintf(&content[used], sizeof(content) - used,
                             "tree %s\n", hex);

    // each parent, in order
    for (size_t at = 0u; at < _count; ++at)
    {
        d_git_repo_hex(_world->repo, &_parents[at], hex);
        used += (size_t)snprintf(&content[used], sizeof(content) - used,
                                 "parent %s\n", hex);
    }

    used += (size_t)snprintf(&content[used], sizeof(content) - used,
                             "author A U Thor <author@example.com> %ld "
                             "+0100\ncommitter C O Mitter "
                             "<committer@example.com> %ld +0000\n\n"
                             "commit at %ld\n\nits body\n",
                             _time, _time, _time);
    d_tests_check(d_tests_object(_world, D_GIT_OBJECT_COMMIT, content, used,
                                 &oid), "a commit is written");

    return oid;
}


/*
d_tests_world_make
  A repository's bones beneath a directory -- .git/HEAD on main, its
config, its objects -- and the repository opened on them.
*/
static bool
d_tests_world_make(
    struct d_tests_world* _world,
    const char*           _directory,
    const char*           _name,
    const char*           _config
)
{
    char why[512];

    memset(_world, 0, sizeof(*_world));
    (void)snprintf(_world->top, sizeof(_world->top), "%s/%s", _directory,
                   _name);
    (void)snprintf(_world->git_dir, sizeof(_world->git_dir), "%s/.git",
                   _world->top);

    char objects[1100];
    char heads[1100];
    char tags[1100];

    (void)snprintf(objects, sizeof(objects), "%s/objects/info/x",
                   _world->git_dir);
    (void)snprintf(heads, sizeof(heads), "%s/refs/heads/x", _world->git_dir);
    (void)snprintf(tags, sizeof(tags), "%s/refs/tags/x", _world->git_dir);

    const bool made = ( (d_tests_put_text(_world->git_dir, "HEAD",
                                          "ref: refs/heads/main\n")) &&
                        (d_tests_put_text(_world->git_dir, "config",
                                          _config)) &&
                        (d_fs_make_parent(objects)) &&
                        (d_fs_make_parent(heads)) &&
                        (d_fs_make_parent(tags)) );

    _world->repo = (made) ? d_git_repo_open(_world->top, why, sizeof(why))
                          : NULL;

    // why not, when not
    if (!_world->repo)
    {
        printf("  FAIL  the repository %s is made: %s\n", _name,
               (made) ? why : "its files cannot be written");
        g_checks++;
        g_failed++;
    }

    return (_world->repo != NULL);
}


/*
d_tests_world_reopen
  The repository opened again, to read what was written since.
*/
static bool
d_tests_world_reopen(
    struct d_tests_world* _world
)
{
    char why[512];

    d_git_repo_free(_world->repo);
    _world->repo = d_git_repo_open(_world->top, why, sizeof(why));
    d_tests_check(_world->repo != NULL, "the repository opens again");

    return (_world->repo != NULL);
}


/*
d_tests_ref
  A loose ref written: a name in hex, or text of its own.
*/
static void
d_tests_ref(
    struct d_tests_world*   _world,
    const char*             _name,
    const struct d_git_oid* _oid,
    const char*             _text
)
{
    char line[256];
    char hex[(2u * D_GIT_OID_MOST) + 1u];

    // a name, or the text given
    if (_oid)
    {
        d_git_repo_hex(_world->repo, _oid, hex);
        (void)snprintf(line, sizeof(line), "%s\n", hex);
    }
    else
    {
        (void)snprintf(line, sizeof(line), "%s", _text);
    }

    d_tests_check(d_tests_put_text(_world->git_dir, _name, line),
                  "a ref is written");

    return;
}


/*
d_tests_put_u32, d_tests_put_u16
  A number written big-endian, as the index holds its numbers.
*/
static void
d_tests_put_u32(
    unsigned char* _out,
    uint32_t       _value
)
{
    _out[0] = (unsigned char)(_value >> 24);
    _out[1] = (unsigned char)(_value >> 16);
    _out[2] = (unsigned char)(_value >> 8);
    _out[3] = (unsigned char)_value;

    return;
}


static void
d_tests_put_u16(
    unsigned char* _out,
    uint32_t       _value
)
{
    _out[0] = (unsigned char)(_value >> 8);
    _out[1] = (unsigned char)_value;

    return;
}


// d_tests_staged
//   struct: one entry of an index the checks write: its path, mode,
//   object and size.
struct d_tests_staged
{
    const char*      path;
    uint32_t         mode;
    struct d_git_oid oid;
    uint32_t         size;
};


/*
d_tests_index
  An index of version 2 written of entries given in git's order: no stat
data but sizes, so git compares each file's content, and a checksum of
zeros, which git reads as none.
*/
static bool
d_tests_index(
    struct d_tests_world*        _world,
    const struct d_tests_staged* _entries,
    size_t                       _count
)
{
    const size_t  hash = d_git_repo_hash_size(_world->repo);
    unsigned char bytes[4096];
    size_t        used = 12u;

    memset(bytes, 0, sizeof(bytes));
    memcpy(bytes, "DIRC", 4u);
    d_tests_put_u32(&bytes[4], 2u);
    d_tests_put_u32(&bytes[8], (uint32_t)_count);

    // each entry: stat data, mode, size, name, flags, path, padding
    for (size_t at = 0u; at < _count; ++at)
    {
        const size_t length = strlen(_entries[at].path);
        const size_t whole  = 40u + hash + 2u + length;

        d_tests_put_u32(&bytes[used + 24u], _entries[at].mode);
        d_tests_put_u32(&bytes[used + 36u], _entries[at].size);
        memcpy(&bytes[used + 40u], _entries[at].oid.bytes, hash);
        d_tests_put_u16(&bytes[used + 40u + hash], (uint32_t)length);
        memcpy(&bytes[used + 42u + hash], _entries[at].path, length);
        used += (whole + 8u) & ~(size_t)7u;
    }

    used += hash;

    char path[1200];

    (void)snprintf(path, sizeof(path), "%s/index", _world->git_dir);

    return d_tests_put(path, bytes, used);
}


//==============================================================================
// the repository's checks
//==============================================================================


// d_tests_hash
//   struct: what git hash-object names a blob of n `x`s, in SHA-1 and in
//   SHA-256 -- lengths whose headers bring the hashed bytes to either side
//   of a block's end.
struct d_tests_hash
{
    size_t      length;
    const char* sha1;
    const char* sha256;
};

static const struct d_tests_hash d_tests_hashes[] =
{
    { 0u,       "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391",
      "473a0f4c3be8a93681a267e3b1e9a7dcda1185436fe141f7749120a303721813" },
    { 1u,       "c1b0730e0133447badcfd47fd144e254807b06e1",
      "4b6cea43da6e13c24f191bcb97b51a58781d1ccdd8281d96291a2582f5177b78" },
    { 47u,      "719ddd7888a02660234770b651274214ff7df239",
      "5c6a18205fae9a7b0fe254e5262e3208e66a6afbc8df4f2d3976cef6e0a4d1f9" },
    { 48u,      "3b3abca28de4b8ce6ea9dac5da512120ed7c5a0c",
      "8fc63468a87c9f8894b375f6ed954d63c39a84ea262a0e4189815ff45ca47152" },
    { 55u,      "e329c5109ce10a7367561757d2f5a053c528f11e",
      "c73ec628ce0ff36b5e3432300521141e80692fa32f0695714e27c751b8a64cbe" },
    { 56u,      "355e203863c368f0404d7fc7ae4761122c117dd2",
      "62ecade9a3c81d7186d91e112660a28c2e8ad9b156f296fcff4960feadc91df1" },
    { 57u,      "f32768e30e0507780e5b9edc8afa9d3f1009926c",
      "ff68a8684337291495f7852567cea06c1abd6be4d114ef84dbabf872a7e5105b" },
    { 110u,     "5b47a7823a0eb565ed152a5cfb1bf549955ef0f9",
      "f1d89675d036480075af210366c0a41fd4075645eaecb69b94d32bbe5a601878" },
    { 111u,     "caa12a76d1ee289e28788808afe369025e50a334",
      "45feda5d434bc55e115b94bd4b8e9ee91b4a6c6d70b02cf2ab31c64720578280" },
    { 1000000u, "8eb708f936a80a54e0daa13707cba9b938bf257b",
      "9b6af36b10d5ee58c91f2f5ec2cc6ba02b046df1bc049056ffb58697857d5e3a" }
};


/*
d_tests_hashing
  Blobs named as git names them, in a repository of each hash.
*/
static void
d_tests_hashing(
    const struct d_tests_world* _sha1,
    const struct d_tests_world* _sha256
)
{
    char* const x = malloc(1000000u);
    char        hex[(2u * D_GIT_OID_MOST) + 1u];

    // room for the longest
    if (!x)
    {
        return;
    }

    memset(x, 'x', 1000000u);
    d_tests_count_is(d_git_repo_hash_size(_sha1->repo), 20u, "SHA-1's size");
    d_tests_count_is(d_git_repo_hash_size(_sha256->repo), 32u,
                     "SHA-256's size");

    // each length, in each hash
    for (size_t at = 0u;
         at < (sizeof(d_tests_hashes) / sizeof(d_tests_hashes[0]));
         ++at)
    {
        struct d_git_oid oid;

        d_git_repo_hash_object(_sha1->repo, D_GIT_OBJECT_BLOB, x,
                               d_tests_hashes[at].length, &oid);
        d_git_repo_hex(_sha1->repo, &oid, hex);
        d_tests_is(hex, d_tests_hashes[at].sha1, "a blob's SHA-1 name");
        d_git_repo_hash_object(_sha256->repo, D_GIT_OBJECT_BLOB, x,
                               d_tests_hashes[at].length, &oid);
        d_git_repo_hex(_sha256->repo, &oid, hex);
        d_tests_is(hex, d_tests_hashes[at].sha256, "a blob's SHA-256 name");
    }

    free(x);

    return;
}

static const unsigned char d_tests_z_fixed[] =
{
    0x78, 0xda, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x63, 0xc8, 0x48, 0xcd,
    0xc9, 0xc9, 0xe7, 0x02, 0x00, 0x1d, 0xc5, 0x04, 0x14
};

static const unsigned char d_tests_z_dynamic[] =
{
    0x78, 0xda, 0x95, 0xd7, 0x49, 0x4e, 0x03, 0x31, 0x10, 0x40, 0x51, 0xd6,
    0x9c, 0xc2, 0x07, 0x60, 0x11, 0xbb, 0x3c, 0xd0, 0xd7, 0xe9, 0xa6, 0x23,
    0x22, 0xa2, 0xb6, 0x08, 0x61, 0x38, 0x3e, 0x52, 0x6e, 0xf0, 0xb6, 0x96,
    0xfe, 0xea, 0xa9, 0xec, 0xf2, 0x7a, 0x9d, 0x6b, 0x8a, 0x65, 0x39, 0x3d,
    0x5d, 0x2f, 0xc7, 0x9e, 0x4e, 0x69, 0x9e, 0xd3, 0xfd, 0x7d, 0x4f, 0x5f,
    0xfb, 0x36, 0x8f, 0xb7, 0xf4, 0xb3, 0x6f, 0xf7, 0x79, 0x7b, 0x79, 0x1c,
    0x7d, 0x7e, 0x5f, 0xb6, 0x8f, 0xb4, 0xde, 0xe6, 0xef, 0x91, 0xce, 0xf3,
    0xef, 0xf9, 0x11, 0x64, 0x0d, 0x8a, 0x06, 0xa1, 0x41, 0xd5, 0xa0, 0x69,
    0xd0, 0x35, 0x18, 0x1a, 0xbc, 0x6a, 0xb0, 0x30, 0x9c, 0x53, 0xb3, 0x75,
    0x66, 0xec, 0xcc, 0xda, 0x99, 0xb9, 0x33, 0x7b, 0x67, 0x06, 0xcf, 0x2c,
    0x9e, 0x99, 0x3c, 0xb3, 0x79, 0x61, 0xf3, 0xe2, 0xf3, 0xcd, 0xe6, 0x85,
    0xcd, 0x0b, 0x9b, 0x17, 0x36, 0x2f, 0x6c, 0x5e, 0xd8, 0xbc, 0xb0, 0x79,
    0x61, 0xf3, 0x60, 0xf3, 0x60, 0xf3, 0xf0, 0x4b, 0x9d, 0xcd, 0x83, 0xcd,
    0x83, 0xcd, 0x83, 0xcd, 0x83, 0xcd, 0x83, 0xcd, 0x83, 0xcd, 0x2b, 0x9b,
    0x57, 0x36, 0xaf, 0x6c, 0x5e, 0xfd, 0x25, 0x67, 0xf3, 0xca, 0xe6, 0x95,
    0xcd, 0x2b, 0x9b, 0x57, 0x36, 0xaf, 0x6c, 0xde, 0xd8, 0xbc, 0xb1, 0x79,
    0x63, 0xf3, 0xc6, 0xe6, 0xcd, 0xd7, 0x37, 0x36, 0x6f, 0x6c, 0xde, 0xd8,
    0xbc, 0xb1, 0x79, 0x63, 0xf3, 0xce, 0xe6, 0x9d, 0xcd, 0x3b, 0x9b, 0x77,
    0x36, 0xef, 0x6c, 0xde, 0x7d, 0x67, 0x67, 0xf3, 0xce, 0xe6, 0x9d, 0xcd,
    0x3b, 0x9b, 0x0f, 0x36, 0x1f, 0x6c, 0x3e, 0xd8, 0x7c, 0xb0, 0xf9, 0x60,
    0xf3, 0xc1, 0xe6, 0xc3, 0x3f, 0x6a, 0x6c, 0x3e, 0xd8, 0x7c, 0x88, 0xf9,
    0x3f, 0x99, 0x57, 0x6e, 0x60
};

static const unsigned char d_tests_z_stored[] =
{
    0x78, 0x01, 0x01, 0x35, 0x01, 0xca, 0xfe, 0x62, 0x6c, 0x6f, 0x62, 0x20,
    0x33, 0x30, 0x30, 0x00, 0x00, 0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31,
    0x38, 0x3f, 0x46, 0x4d, 0x54, 0x5b, 0x62, 0x69, 0x70, 0x77, 0x7e, 0x85,
    0x8c, 0x93, 0x9a, 0xa1, 0xa8, 0xaf, 0xb6, 0xbd, 0xc4, 0xcb, 0xd2, 0xd9,
    0xe0, 0xe7, 0xee, 0xf5, 0xfc, 0x03, 0x0a, 0x11, 0x18, 0x1f, 0x26, 0x2d,
    0x34, 0x3b, 0x42, 0x49, 0x50, 0x57, 0x5e, 0x65, 0x6c, 0x73, 0x7a, 0x81,
    0x88, 0x8f, 0x96, 0x9d, 0xa4, 0xab, 0xb2, 0xb9, 0xc0, 0xc7, 0xce, 0xd5,
    0xdc, 0xe3, 0xea, 0xf1, 0xf8, 0xff, 0x06, 0x0d, 0x14, 0x1b, 0x22, 0x29,
    0x30, 0x37, 0x3e, 0x45, 0x4c, 0x53, 0x5a, 0x61, 0x68, 0x6f, 0x76, 0x7d,
    0x84, 0x8b, 0x92, 0x99, 0xa0, 0xa7, 0xae, 0xb5, 0xbc, 0xc3, 0xca, 0xd1,
    0xd8, 0xdf, 0xe6, 0xed, 0xf4, 0xfb, 0x02, 0x09, 0x10, 0x17, 0x1e, 0x25,
    0x2c, 0x33, 0x3a, 0x41, 0x48, 0x4f, 0x56, 0x5d, 0x64, 0x6b, 0x72, 0x79,
    0x80, 0x87, 0x8e, 0x95, 0x9c, 0xa3, 0xaa, 0xb1, 0xb8, 0xbf, 0xc6, 0xcd,
    0xd4, 0xdb, 0xe2, 0xe9, 0xf0, 0xf7, 0xfe, 0x05, 0x0c, 0x13, 0x1a, 0x21,
    0x28, 0x2f, 0x36, 0x3d, 0x44, 0x4b, 0x52, 0x59, 0x60, 0x67, 0x6e, 0x75,
    0x7c, 0x83, 0x8a, 0x91, 0x98, 0x9f, 0xa6, 0xad, 0xb4, 0xbb, 0xc2, 0xc9,
    0xd0, 0xd7, 0xde, 0xe5, 0xec, 0xf3, 0xfa, 0x01, 0x08, 0x0f, 0x16, 0x1d,
    0x24, 0x2b, 0x32, 0x39, 0x40, 0x47, 0x4e, 0x55, 0x5c, 0x63, 0x6a, 0x71,
    0x78, 0x7f, 0x86, 0x8d, 0x94, 0x9b, 0xa2, 0xa9, 0xb0, 0xb7, 0xbe, 0xc5,
    0xcc, 0xd3, 0xda, 0xe1, 0xe8, 0xef, 0xf6, 0xfd, 0x04, 0x0b, 0x12, 0x19,
    0x20, 0x27, 0x2e, 0x35, 0x3c, 0x43, 0x4a, 0x51, 0x58, 0x5f, 0x66, 0x6d,
    0x74, 0x7b, 0x82, 0x89, 0x90, 0x97, 0x9e, 0xa5, 0xac, 0xb3, 0xba, 0xc1,
    0xc8, 0xcf, 0xd6, 0xdd, 0xe4, 0xeb, 0xf2, 0xf9, 0x00, 0x07, 0x0e, 0x15,
    0x1c, 0x23, 0x2a, 0x31, 0x38, 0x3f, 0x46, 0x4d, 0x54, 0x5b, 0x62, 0x69,
    0x70, 0x77, 0x7e, 0x85, 0x8c, 0x93, 0x9a, 0xa1, 0xa8, 0xaf, 0xb6, 0xbd,
    0xc4, 0xcb, 0xd2, 0xd9, 0xe0, 0xe7, 0xee, 0xf5, 0xfc, 0x03, 0x0a, 0x11,
    0x18, 0x1f, 0x26, 0x2d, 0x24, 0x6b, 0x94, 0xb1
};

static const unsigned char d_tests_z_blocks[] =
{
    0x78, 0x9c, 0x4a, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x34, 0x35, 0x30, 0x60,
    0x48, 0x4c, 0x4a, 0x1e, 0x45, 0xa3, 0x68, 0x08, 0x21, 0x00, 0x00, 0x00,
    0x00, 0xff, 0xff, 0x1a, 0x45, 0x23, 0x0f, 0x01, 0x00, 0x00, 0x00, 0xff,
    0xff, 0x4b, 0x4e, 0x4c, 0x4a, 0x1e, 0x45, 0xa4, 0x22, 0x00, 0xa1, 0x33,
    0x40, 0xdc
};


/*
d_tests_vector_content
  What each deflated vector holds: "hello\n"; eighty numbered lines; three
hundred bytes, each seven more than the last; "abc" five hundred times.
*/
static char*
d_tests_vector_content(
    size_t  _vector,
    size_t* _out_length
)
{
    char* const out  = malloc(8192u);
    size_t      used = 0u;

    // room for any
    if (!out)
    {
        return NULL;
    }

    // a table of the vectors' contents
    switch (_vector)
    {
        case 0:
            used = (size_t)snprintf(out, 8192u, "hello\n");
            break;
        case 1:
            // each line, numbered
            for (int line = 0; line < 80; ++line)
            {
                used += (size_t)snprintf(&out[used], 8192u - used,
                                         "line %d of the second vector, "
                                         "the quick brown fox\n", line);
            }
            break;
        case 2:
            // each byte, seven past the last
            for (used = 0u; used < 300u; ++used)
            {
                out[used] = (char)((used * 7u) & 0xffu);
            }
            break;
        default:
            // abc, again and again
            for (used = 0u; used < 1500u; ++used)
            {
                out[used] = "abc"[used % 3u];
            }
            break;
    }

    *_out_length = used;

    return out;
}


/*
d_tests_inflating
  Objects zlib deflated -- fixed codes, dynamic codes, stored blocks, and
blocks flushed between -- read back whole; one whose check is wrong, one
cut short, and one whose content is not its name's, each refused.
  Long as the checks are many, each a line or two.
*/
static void
d_tests_inflating(
    struct d_tests_world* _world
)
{
    static const char* const names[4] = {
        "ce013625030ba8dba906f756967f9e9ca394464a",
        "7f2f80d2cc8f53a4ad10b6056993312f6339c042",
        "23e2880f02df641162c12d798f8537dfbeffc6b7",
        "60a086413ec2dd3d02987bf7d05a6e4084f0e86b" };
    static const unsigned char* const vectors[4] = {
        d_tests_z_fixed, d_tests_z_dynamic, d_tests_z_stored,
        d_tests_z_blocks };
    static const size_t sizes[4] = {
        sizeof(d_tests_z_fixed), sizeof(d_tests_z_dynamic),
        sizeof(d_tests_z_stored), sizeof(d_tests_z_blocks) };
    char                path[1200];
    unsigned char       bad[64];

    // each vector, written where its name says, and read
    for (size_t at = 0u; at < 4u; ++at)
    {
        struct d_git_oid    oid;
        struct d_git_object object;
        size_t              length  = 0u;
        char* const         content = d_tests_vector_content(at, &length);

        d_tests_check(d_git_repo_parse_hex(_world->repo, names[at], &oid),
                      "a vector's name");
        d_tests_object_path(_world, &oid, path, sizeof(path));
        d_tests_check(d_tests_put(path, vectors[at], sizes[at]),
                      "a vector written");
        d_tests_check( (d_git_repo_read(_world->repo, &oid, &object)) &&
                       (object.type == D_GIT_OBJECT_BLOB) &&
                       (content) && (object.size == length) &&
                       (memcmp(object.data, content, length) == 0),
                       names[at]);
        d_git_object_release(&object);
        free(content);
    }

    struct d_git_oid    hello;
    struct d_git_object object;

    (void)d_git_repo_parse_hex(_world->repo, names[0], &hello);
    d_tests_object_path(_world, &hello, path, sizeof(path));
    memcpy(bad, d_tests_z_fixed, sizeof(d_tests_z_fixed));
    bad[sizeof(d_tests_z_fixed) - 1u] ^= 0x01u;
    (void)d_tests_put(path, bad, sizeof(d_tests_z_fixed));
    d_tests_check(!d_git_repo_read(_world->repo, &hello, &object),
                  "a wrong check refused");
    d_git_object_release(&object);
    (void)d_tests_put(path, d_tests_z_fixed, sizeof(d_tests_z_fixed) - 6u);
    d_tests_check(!d_git_repo_read(_world->repo, &hello, &object),
                  "a stream cut short refused");
    d_git_object_release(&object);

    size_t               size     = 0u;
    unsigned char* const deflated = d_tests_stored(
                                        (const unsigned char*)"blob 6\0HELLO\n",
                                        13u, &size);

    (void)d_tests_put(path, deflated, size);
    d_tests_check(!d_git_repo_read(_world->repo, &hello, &object),
                  "content not its name's refused");
    d_git_object_release(&object);
    free(deflated);
    (void)d_tests_put(path, d_tests_z_fixed, sizeof(d_tests_z_fixed));

    return;
}


// d_tests_history
//   struct: the history the checks build: five commits, c4 a merge of c2
//   and c3, their trees, an annotated tag, and the blobs they hold.
struct d_tests_history
{
    struct d_git_oid commits[5];
    struct d_git_oid trees[5];
    struct d_git_oid tag;
    struct d_git_oid one;
    struct d_git_oid two;
    struct d_git_oid b;
    struct d_git_oid e;
    struct d_git_oid d;
};


/*
d_tests_history_make
  The history written: c1 a.txt "one"; c2 a.txt "two"; c3, beside c2 from
c1, adds b.txt; c4 merges c2 and c3; c5 adds d/e.txt. Refs: main at c5,
side at c3, the tag lw at c1, loose; old at c1 and the annotated v1, of c2,
packed, v1 with what it peels to; HEAD on main.
  Long as the history's objects and refs are each written in turn.
*/
static bool
d_tests_history_make(
    struct d_tests_world*   _world,
    struct d_tests_history* _out
)
{
    struct d_tests_entry entries[3];
    struct d_git_oid     parents[2];
    char                 text[1024];
    char                 hex[(2u * D_GIT_OID_MOST) + 1u];
    char                 peeled[(2u * D_GIT_OID_MOST) + 1u];

    _out->one = d_tests_blob(_world, "one\n");
    _out->two = d_tests_blob(_world, "two\n");
    _out->b   = d_tests_blob(_world, "b\n");
    _out->e   = d_tests_blob(_world, "e\n");
    entries[0].mode = "100644";
    entries[0].name = "a.txt";
    entries[0].oid  = _out->one;
    _out->trees[0]  = d_tests_tree(_world, entries, 1u);
    entries[0].oid  = _out->two;
    _out->trees[1]  = d_tests_tree(_world, entries, 1u);
    entries[0].oid  = _out->one;
    entries[1].mode = "100644";
    entries[1].name = "b.txt";
    entries[1].oid  = _out->b;
    _out->trees[2]  = d_tests_tree(_world, entries, 2u);
    entries[0].oid  = _out->two;
    _out->trees[3]  = d_tests_tree(_world, entries, 2u);
    entries[2].mode = "100644";
    entries[2].name = "e.txt";
    entries[2].oid  = _out->e;
    _out->d         = d_tests_tree(_world, &entries[2], 1u);
    entries[2].mode = "40000";
    entries[2].name = "d";
    entries[2].oid  = _out->d;
    _out->trees[4]  = d_tests_tree(_world, entries, 3u);
    _out->commits[0] = d_tests_commit(_world, &_out->trees[0], NULL, 0u,
                                      1000000000L);
    _out->commits[1] = d_tests_commit(_world, &_out->trees[1],
                                      &_out->commits[0], 1u, 1000002000L);
    _out->commits[2] = d_tests_commit(_world, &_out->trees[2],
                                      &_out->commits[0], 1u, 1000003000L);
    parents[0]       = _out->commits[1];
    parents[1]       = _out->commits[2];
    _out->commits[3] = d_tests_commit(_world, &_out->trees[3], parents, 2u,
                                      1000004000L);
    _out->commits[4] = d_tests_commit(_world, &_out->trees[4],
                                      &_out->commits[3], 1u, 1000005000L);
    d_git_repo_hex(_world->repo, &_out->commits[1], peeled);
    (void)snprintf(text, sizeof(text),
                   "object %s\ntype commit\ntag v1\ntagger T Agger "
                   "<tagger@example.com> 1000006000 -0230\n\nversion one\n",
                   peeled);
    d_tests_check(d_tests_object(_world, D_GIT_OBJECT_TAG, text,
                                 strlen(text), &_out->tag),
                  "a tag is written");
    d_tests_ref(_world, "refs/heads/main", &_out->commits[4], NULL);
    d_tests_ref(_world, "refs/heads/side", &_out->commits[2], NULL);
    d_tests_ref(_world, "refs/tags/lw", &_out->commits[0], NULL);
    d_git_repo_hex(_world->repo, &_out->commits[0], hex);
    (void)snprintf(text, sizeof(text),
                   "# pack-refs with: peeled fully-peeled sorted \n"
                   "%s refs/tags/old\n", hex);
    d_git_repo_hex(_world->repo, &_out->tag, hex);
    (void)snprintf(&text[strlen(text)], sizeof(text) - strlen(text),
                   "%s refs/tags/v1\n^%s\n", hex, peeled);
    d_tests_ref(_world, "packed-refs", NULL, text);

    return d_tests_world_reopen(_world);
}


/*
d_tests_names_of
  Whether an object's name is the one expected, the check named.
*/
static void
d_tests_names_of(
    struct d_tests_world*   _world,
    const char*             _revision,
    const struct d_git_oid* _want
)
{
    struct d_git_oid got;
    char             what[256];
    const bool       found = d_git_repo_rev(_world->repo, _revision, &got);

    (void)snprintf(what, sizeof(what), "%s names its object: %s", _revision,
                   (found) ? "" : d_git_repo_problem(_world->repo));
    d_tests_check( (found) && (d_git_repo_same_oid(_world->repo, &got,
                                                   _want)), what);

    return;
}


/*
d_tests_revisions
  Revisions as rev-parse names them: refs completed by its rules, a
packed tag peeled, parents and ancestors, a merge's second parent, a
tree's path, a short name, and what it refuses.
*/
static void
d_tests_revisions(
    struct d_tests_world*         _world,
    const struct d_tests_history* _history
)
{
    struct d_git_oid oid;
    char             hex[(2u * D_GIT_OID_MOST) + 1u];
    char             name[(2u * D_GIT_OID_MOST) + 16u];

    puts("a repository built from nothing");
    d_tests_names_of(_world, "HEAD", &_history->commits[4]);
    d_tests_names_of(_world, "@", &_history->commits[4]);
    d_tests_names_of(_world, "main", &_history->commits[4]);
    d_tests_names_of(_world, "refs/heads/side", &_history->commits[2]);
    d_tests_names_of(_world, "heads/side", &_history->commits[2]);
    d_tests_names_of(_world, "HEAD~1", &_history->commits[3]);
    d_tests_names_of(_world, "HEAD~2", &_history->commits[1]);
    d_tests_names_of(_world, "HEAD^^^", &_history->commits[0]);
    d_tests_names_of(_world, "HEAD~1^2", &_history->commits[2]);
    d_tests_names_of(_world, "HEAD^{tree}", &_history->trees[4]);
    d_tests_names_of(_world, "HEAD:d/e.txt", &_history->e);
    d_tests_names_of(_world, "HEAD:d", &_history->d);
    d_tests_names_of(_world, "side:b.txt", &_history->b);
    d_tests_names_of(_world, "v1", &_history->tag);
    d_tests_names_of(_world, "v1^{}", &_history->commits[1]);
    d_tests_names_of(_world, "v1^{commit}", &_history->commits[1]);
    d_tests_names_of(_world, "v1~1", &_history->commits[0]);
    d_tests_names_of(_world, "lw", &_history->commits[0]);
    d_tests_names_of(_world, "tags/old", &_history->commits[0]);
    d_git_repo_hex(_world->repo, &_history->commits[4], hex);
    (void)snprintf(name, sizeof(name), "%.7s", hex);
    d_tests_names_of(_world, name, &_history->commits[4]);
    (void)snprintf(name, sizeof(name), "%s^{tree}", hex);
    d_tests_names_of(_world, name, &_history->trees[4]);
    d_tests_check(!d_git_repo_rev(_world->repo, "HEAD^2", &oid),
                  "a second parent c5 has not refused");
    d_tests_check(!d_git_repo_rev(_world->repo, "nosuch", &oid),
                  "a name of nothing refused");
    d_tests_check(!d_git_repo_rev(_world->repo, "HEAD:nosuch", &oid),
                  "a path the tree has not refused");
    d_tests_check(!d_git_repo_rev(_world->repo, "HEAD@{1}", &oid),
                  "a reflog that is not there refused");
    d_tests_check(!d_git_repo_rev(_world->repo, ":/commit", &oid),
                  ":/text refused");
    d_tests_check(d_git_repo_has(_world->repo, &_history->tag),
                  "the tag is there");

    return;
}


/*
d_tests_log_is
  Whether a walk gives the commits expected, by their indices, newest
first.
*/
static void
d_tests_log_is(
    struct d_tests_world*           _world,
    const struct d_tests_history*   _history,
    const struct d_git_log_options* _options,
    const char*                     _want,
    const char*                     _what
)
{
    struct d_git_oids oids;
    char              got[64];
    size_t            used = 0u;

    got[0] = '\0';

    // the walk, each commit by its index from 1
    if (d_git_repo_log(_world->repo, _options, &oids))
    {
        // each commit given
        for (size_t at = 0u; at < oids.count; ++at)
        {
            // which of the history's it is
            for (size_t which = 0u; ( (which < 5u) && (used < 60u) );
                 ++which)
            {
                // this one
                if (d_git_repo_same_oid(_world->repo, &oids.items[at],
                                        &_history->commits[which]))
                {
                    got[used++] = (char)('1' + which);
                    got[used]   = '\0';
                }
            }
        }
    }

    d_git_oids_release(&oids);
    d_tests_is(got, _want, _what);

    return;
}


/*
d_tests_walks
  Walks as git rev-list walks: by date, a commit never before its child;
one excluded and all it reaches; first parents only; a path's history,
simplified through the parent a merge is the same as; the most given.
*/
static void
d_tests_walks(
    struct d_tests_world*         _world,
    const struct d_tests_history* _history
)
{
    static const char* const all[]     = { "HEAD" };
    static const char* const ranged[]  = { "HEAD", "^side" };
    static const char* const tagged[]  = { "v1", "side" };
    struct d_git_log_options options;

    memset(&options, 0, sizeof(options));
    options.revisions      = all;
    options.revision_count = 1u;
    d_tests_log_is(_world, _history, &options, "54321", "all of history");
    options.first_parent = true;
    d_tests_log_is(_world, _history, &options, "5421", "first parents");
    options.first_parent = false;
    options.most         = 2u;
    d_tests_log_is(_world, _history, &options, "54", "the two newest");
    options.most = 0u;
    options.path = "b.txt";
    d_tests_log_is(_world, _history, &options, "3", "b.txt's history");
    options.path = "a.txt";
    d_tests_log_is(_world, _history, &options, "21", "a.txt's history");
    options.path = "d";
    d_tests_log_is(_world, _history, &options, "5", "d's history");
    options.path           = NULL;
    options.revisions      = ranged;
    options.revision_count = 2u;
    d_tests_log_is(_world, _history, &options, "542", "HEAD ^side");
    options.revisions = tagged;
    d_tests_log_is(_world, _history, &options, "321",
                   "from a tag and a branch");

    return;
}


/*
d_tests_bases
  Merge bases, as git merge-base --all gives them.
*/
static void
d_tests_bases(
    struct d_tests_world*         _world,
    const struct d_tests_history* _history
)
{
    struct d_git_oids oids;

    d_tests_check( (d_git_repo_merge_base(_world->repo, &_history->commits[1],
                                          &_history->commits[2], &oids)) &&
                   (oids.count == 1u) &&
                   (d_git_repo_same_oid(_world->repo, &oids.items[0],
                                        &_history->commits[0])),
                   "c2 and c3 meet at c1");
    d_git_oids_release(&oids);
    d_tests_check( (d_git_repo_merge_base(_world->repo, &_history->commits[4],
                                          &_history->commits[2], &oids)) &&
                   (oids.count == 1u) &&
                   (d_git_repo_same_oid(_world->repo, &oids.items[0],
                                        &_history->commits[2])),
                   "c5 and c3 meet at c3");
    d_git_oids_release(&oids);
    d_tests_check( (d_git_repo_merge_base(_world->repo, &_history->tag,
                                          &_history->commits[2], &oids)) &&
                   (oids.count == 1u),
                   "a tag peeled to its commit");
    d_git_oids_release(&oids);

    return;
}


/*
d_tests_objects
  Objects read and parsed: a tree's entries in order, a commit's headers,
people and message, a tag's, and a name made short.
  Long as the checks are many, each a line or two.
*/
static void
d_tests_objects(
    struct d_tests_world*         _world,
    const struct d_tests_history* _history
)
{
    struct d_git_object     object;
    struct d_git_tree_entry entry;
    struct d_git_commit     commit;
    struct d_git_tag        tag;
    size_t                  at    = 0u;
    char                    names[64];
    char                    hex[(2u * D_GIT_OID_MOST) + 1u];

    names[0] = '\0';
    memset(&commit, 0, sizeof(commit));

    // a tree, each entry's name
    if (d_git_repo_read(_world->repo, &_history->trees[4], &object))
    {
        // each entry, in order
        while (d_git_repo_tree_next(_world->repo, &object, &at, &entry))
        {
            (void)snprintf(&names[strlen(names)], sizeof(names)
                           - strlen(names), "%.*s:%o ",
                           (int)entry.name_length, entry.name,
                           (unsigned)entry.mode);
        }
    }

    d_git_object_release(&object);
    d_tests_is(names, "a.txt:100644 b.txt:100644 d:40000 ",
               "a tree's entries");

    // a merge, parsed
    if ( (d_git_repo_read(_world->repo, &_history->commits[3], &object)) &&
         (d_git_repo_parse_commit(_world->repo, &object, &commit)) )
    {
        d_tests_count_is(commit.parent_count, 2u, "a merge's two parents");
        d_tests_check(d_git_repo_same_oid(_world->repo, &commit.tree,
                                          &_history->trees[3]),
                      "a commit's tree");
        d_tests_check( (commit.author.name_length == 8u) &&
                       (memcmp(commit.author.name, "A U Thor", 8u) == 0),
                       "its author's name");
        d_tests_check( (commit.author.time == 1000004000LL) &&
                       (commit.author.offset == 60) &&
                       (strcmp(commit.author.zone, "+0100") == 0),
                       "its author's time and zone");
        d_tests_check( (commit.message_length == 31u) &&
                       (memcmp(commit.message, "commit at 1000004000\n", 21u)
                            == 0), "its message");
    }
    else
    {
        d_tests_check(false, "a merge is read and parsed");
    }

    d_git_commit_release(&commit);
    d_git_object_release(&object);

    // a tag, parsed
    if ( (d_git_repo_read(_world->repo, &_history->tag, &object)) &&
         (d_git_repo_parse_tag(_world->repo, &object, &tag)) )
    {
        d_tests_check( (tag.type == D_GIT_OBJECT_COMMIT) &&
                       (d_git_repo_same_oid(_world->repo, &tag.object,
                                            &_history->commits[1])),
                       "a tag's object");
        d_tests_check( (tag.name_length == 2u) &&
                       (memcmp(tag.name, "v1", 2u) == 0), "its name");
        d_tests_check( (tag.has_tagger) && (tag.tagger.offset == -150),
                       "its tagger's zone");
    }
    else
    {
        d_tests_check(false, "a tag is read and parsed");
    }

    d_git_object_release(&object);
    d_git_repo_hex(_world->repo, &_history->commits[4], hex);
    hex[7] = '\0';
    d_tests_check( (d_git_repo_abbreviate(_world->repo, &_history->commits[4],
                                          0u, names)) &&
                   (strcmp(names, hex) == 0), "a name made short: 7 digits");

    return;
}


/*
d_tests_refs
  Refs loose and packed, sorted; one followed through HEAD; what a packed
tag peels to; and HEAD's branch.
*/
static void
d_tests_refs(
    struct d_tests_world*         _world,
    const struct d_tests_history* _history
)
{
    static const char* const names[] = {
        "refs/heads/main", "refs/heads/side", "refs/tags/lw",
        "refs/tags/old", "refs/tags/v1" };
    struct d_git_refs refs;
    struct d_git_oid  oid;
    char*             target = NULL;
    size_t            listed = 0u;

    memset(&refs, 0, sizeof(refs));

    // every ref
    if (d_git_repo_refs(_world->repo, "", &refs))
    {
        // each one of those expected, in its place
        for (size_t at = 0u; at < refs.count; ++at)
        {
            // the next expected
            if ( (listed < 5u) &&
                 (strcmp(refs.items[at].name, names[listed]) == 0) )
            {
                ++listed;
            }
        }
    }

    d_tests_check( (listed == 5u) && (refs.count == 5u), "the refs, sorted");

    // the annotated tag's peeled name
    for (size_t at = 0u; at < refs.count; ++at)
    {
        // v1
        if (strcmp(refs.items[at].name, "refs/tags/v1") == 0)
        {
            d_tests_check( (refs.items[at].has_peeled) &&
                           (d_git_repo_same_oid(_world->repo,
                                                &refs.items[at].peeled,
                                                &_history->commits[1])),
                           "v1 peels to c2");
        }
    }

    d_git_refs_release(&refs);
    d_tests_check( (d_git_repo_resolve(_world->repo, "HEAD", &oid, &target)) &&
                   (target) && (strcmp(target, "refs/heads/main") == 0) &&
                   (d_git_repo_same_oid(_world->repo, &oid,
                                        &_history->commits[4])),
                   "HEAD followed to main");
    free(target);
    target = d_git_repo_symbolic(_world->repo, "HEAD");
    d_tests_is(target, "refs/heads/main", "HEAD's branch");
    free(target);

    return;
}


/*
d_tests_status_is
  Whether status gives the lines expected, each "XY path\n".
*/
static void
d_tests_status_is(
    struct d_tests_world*              _world,
    const struct d_git_status_options* _options,
    const char*                        _want,
    const char*                        _what
)
{
    struct d_git_status status;
    char                got[1024];
    size_t              used = 0u;

    got[0] = '\0';

    // the status, a line each
    if (d_git_repo_status(_world->repo, _options, &status))
    {
        // each line
        for (size_t at = 0u; ( (at < status.count) && (used < 900u) ); ++at)
        {
            used += (size_t)snprintf(&got[used], sizeof(got) - used,
                                     "%c%c %s\n", status.items[at].index,
                                     status.items[at].work,
                                     status.items[at].path);
        }
    }
    else
    {
        (void)snprintf(got, sizeof(got), "failed: %s",
                       d_git_repo_problem(_world->repo));
    }

    d_git_status_release(&status);
    d_tests_is(got, _want, _what);

    return;
}


/*
d_tests_work_tree
  The index HEAD's tree was staged into, and a work tree changed from it:
a file the same, one changed in size, one gone, one untracked, one
ignored, and an untracked directory; status in each mode, as git status
--porcelain gives it; the index's entries; ignoring and attributes by the
repository's own files; settings by scope.
  Long as the work tree is written and then read, each check a line or two.
*/
static void
d_tests_work_tree(
    struct d_tests_world*         _world,
    const struct d_tests_history* _history
)
{
    struct d_tests_staged       staged[3];
    struct d_git_status_options options;
    struct d_git_ignore_match   match;
    struct d_git_text_values    values;

    puts("the work tree");
    staged[0].path = "a.txt";
    staged[0].mode = 0100644u;
    staged[0].oid  = _history->two;
    staged[0].size = 4u;
    staged[1].path = "b.txt";
    staged[1].mode = 0100644u;
    staged[1].oid  = _history->b;
    staged[1].size = 2u;
    staged[2].path = "d/e.txt";
    staged[2].mode = 0100644u;
    staged[2].oid  = _history->e;
    staged[2].size = 2u;

    const bool written =
        ( (d_tests_index(_world, staged, 3u)) &&
          (d_tests_put_text(_world->top, "a.txt", "two\n")) &&
          (d_tests_put_text(_world->top, "b.txt", "b changed\n")) &&
          (d_tests_put_text(_world->top, "u.txt", "new\n")) &&
          (d_tests_put_text(_world->top, "x.log", "log\n")) &&
          (d_tests_put_text(_world->top, "new/f.txt", "f\n")) &&
          (d_tests_put_text(_world->top, ".gitignore", "*.log\n")) &&
          (d_tests_put_text(_world->top, ".gitattributes",
                            "*.txt text eol=lf\n")) &&
          (d_tests_world_reopen(_world)) );

    d_tests_check(written, "the index and the work tree written");

    // a work tree to read
    if (!written)
    {
        return;
    }

    memset(&options, 0, sizeof(options));
    d_tests_count_is(d_git_repo_index_count(_world->repo), 3u,
                     "three entries staged");
    d_tests_count_is(d_git_repo_index_find(_world->repo, "d/e.txt", 0u), 2u,
                     "an entry found by its path");
    d_tests_status_is(_world, &options,
                      " M b.txt\n D d/e.txt\n?? .gitattributes\n"
                      "?? .gitignore\n?? new/\n?? u.txt\n",
                      "status: a directory untracked as itself");
    options.untracked = D_GIT_UNTRACKED_ALL;
    options.ignored   = true;
    d_tests_status_is(_world, &options,
                      " M b.txt\n D d/e.txt\n?? .gitattributes\n"
                      "?? .gitignore\n?? new/f.txt\n?? u.txt\n!! x.log\n",
                      "status -uall --ignored");
    options.untracked = D_GIT_UNTRACKED_NO;
    options.ignored   = false;
    d_tests_status_is(_world, &options, " M b.txt\n D d/e.txt\n",
                      "status -uno");
    d_tests_check( (d_git_repo_ignored(_world->repo, "x.log", &match)) &&
                   (match.ignored) && (match.line == 1u) &&
                   (strcmp(match.file, ".gitignore") == 0),
                   "x.log ignored by .gitignore:1");
    d_tests_check(!d_git_repo_ignored(_world->repo, "u.txt", &match),
                  "u.txt not ignored");

    const struct d_git_text_value* eol = NULL;

    d_tests_check( (d_git_repo_attributes(_world->repo, "d/e.txt",
                                          &values)) &&
                   ( (eol = d_git_text_value_of(&values, "eol")) != NULL ) &&
                   (eol->value_length == 2u) &&
                   (memcmp(eol->value, "lf", 2u) == 0),
                   "d/e.txt's eol by .gitattributes");
    d_git_text_values_release(&values);

    return;
}


/*
d_tests_settings
  A repository's own settings, in its configuration's order, by key and
scope; and opening from a directory within, and through a file naming
the repository.
*/
static void
d_tests_settings(
    struct d_tests_world* _world
)
{
    bool         set   = false;
    size_t       local = 0u;
    char         path[1200];
    char         why[1100];
    const size_t count = d_git_repo_settings(_world->repo);

    // the local settings, counted
    for (size_t at = 0u; at < count; ++at)
    {
        local += (d_git_repo_setting(_world->repo, at)->scope
                      == D_GIT_SCOPE_LOCAL) ? 1u : 0u;
    }

    d_tests_count_is(local, 4u, "four local settings");
    d_tests_is(d_git_repo_config(_world->repo, "User.Name", &set),
               "Pat Example", "a key in any case");
    d_tests_check( (d_git_repo_config(_world->repo, "core.flag", &set)
                        == NULL) && (set), "a name alone is set, no value");
    d_tests_check( (!d_git_repo_config(_world->repo, "no.such", &set)) &&
                   (!set), "a key not set");
    bool bare = true;

    d_tests_check( (d_git_text_bool(d_git_repo_config(_world->repo,
                                                      "core.bare", &set),
                                    &bare)) && (!bare),
                   "core.bare read as a boolean");
    (void)snprintf(path, sizeof(path), "%s/new", _world->top);

    struct d_git_repo* const within = d_git_repo_open(path, why, sizeof(why));

    d_tests_check( (within) &&
                   (strcmp(d_git_repo_git_dir(within),
                           d_git_repo_git_dir(_world->repo)) == 0),
                   "opened from a directory within");
    d_git_repo_free(within);
    (void)snprintf(path, sizeof(path), "%s/../linked", _world->top);
    (void)snprintf(why, sizeof(why), "gitdir: %s\n", _world->git_dir);
    (void)d_tests_put_text(path, ".git", why);

    struct d_git_repo* const linked = d_git_repo_open(path, why, sizeof(why));

    d_tests_check( (linked) &&
                   (strcmp(d_git_repo_git_dir(linked),
                           d_git_repo_git_dir(_world->repo)) == 0),
                   "opened through a .git file naming the repository");
    d_git_repo_free(linked);

    return;
}


/*
d_tests_repository
  A repository of each hash built beneath a directory, and read.
*/
static void
d_tests_repository(
    const char* _directory
)
{
    struct d_tests_world   sha1;
    struct d_tests_world   sha256;
    struct d_tests_history history;

    // the repositories made
    if ( (!d_tests_world_make(&sha1, _directory, "repo",
                              "[core]\n\trepositoryformatversion = 0\n"
                              "\tbare = false\n\tflag\n[user]\n"
                              "\tname = Pat Example\n")) ||
         (!d_tests_world_make(&sha256, _directory, "repo256",
                              "[core]\n\trepositoryformatversion = 1\n"
                              "[extensions]\n\tobjectformat = sha256\n")) )
    {
        d_git_repo_free(sha1.repo);

        return;
    }

    d_tests_hashing(&sha1, &sha256);
    d_tests_inflating(&sha1);

    // the history, and what is read of it
    if (d_tests_history_make(&sha1, &history))
    {
        d_tests_revisions(&sha1, &history);
        d_tests_walks(&sha1, &history);
        d_tests_bases(&sha1, &history);
        d_tests_objects(&sha1, &history);
        d_tests_refs(&sha1, &history);
        d_tests_work_tree(&sha1, &history);
        d_tests_settings(&sha1);
    }

    d_git_repo_free(sha1.repo);
    d_git_repo_free(sha256.repo);

    return;
}


//==============================================================================
// --corpus: every file and repository under a directory, read and proved
//==============================================================================


// d_tests_counts
//   struct: what a corpus or fuzz run has read, done and found wrong.
struct d_tests_counts
{
    size_t files;
    size_t records;
    size_t repositories;
    size_t objects;
    size_t changes;
    size_t kept;
    size_t refused;
    size_t failed;
};

// d_tests_fn_file
//   type: what a walk does with each file of a kind git reads.
typedef void (*d_tests_fn_file)(const char*            _path,
                                struct d_tests_counts* _counts);

// d_tests_fn_repo
//   type: what a walk does with each repository it finds.
typedef void (*d_tests_fn_repo)(const char*            _path,
                                struct d_tests_counts* _counts);


/*
d_tests_compare_names
  Names in byte order, for qsort.
*/
static int
d_tests_compare_names(
    const void* _left,
    const void* _right
)
{
    return strcmp(*(char* const*)_left, *(char* const*)_right);
}


/*
d_tests_show
  A text, its control bytes escaped, for a failure's reader; shown only
when DGIT_VERBOSE is set.
*/
static void
d_tests_show(
    const char* _label,
    const char* _text,
    size_t      _length
)
{
    // only when asked
    if (!getenv("DGIT_VERBOSE"))
    {
        return;
    }

    printf("        %s [", _label);

    // each byte, a control byte escaped
    for (size_t at = 0u; at < _length; ++at)
    {
        const unsigned char byte = (unsigned char)_text[at];

        // a table of the escapes
        if (byte == '\n')
        {
            fputs("\\n", stdout);
        }
        else if ( (byte < 0x20u) || (byte > 0x7eu) )
        {
            printf("\\x%02x", byte);
        }
        else
        {
            putchar(byte);
        }
    }

    puts("]");

    return;
}


/*
d_tests_fail
  A failure in a corpus or fuzz run, said and counted.
*/
static void
d_tests_fail(
    struct d_tests_counts* _counts,
    const char*            _path,
    const char*            _what
)
{
    printf("  FAIL  %s: %s\n", _path, _what);
    _counts->failed++;

    return;
}


/*
d_tests_walk
  Every file of a kind git reads under a directory, in name order, given to
`_file`; and every repository, a directory holding .git or a .git itself,
given to `_repo` -- its own files given to `_file` too.
  Long as each kind of name is told apart in one loop.
*/
static void
d_tests_walk(
    const char*            _directory,
    d_tests_fn_file        _file,
    d_tests_fn_repo        _repo,
    struct d_tests_counts* _counts
)
{
    char** names = NULL;
    size_t count = 0u;

    // a directory that cannot be listed holds nothing to read
    if (!d_fs_names(_directory, &names, &count))
    {
        d_tests_fail(_counts, _directory, "cannot be listed");

        return;
    }

    // in name order
    if (count > 1u)
    {
        qsort(names, count, sizeof(names[0]), d_tests_compare_names);
    }

    // each name the directory holds
    for (size_t at = 0u; at < count; ++at)
    {
        const size_t span = strlen(_directory) + strlen(names[at]) + 2u;
        char* const  path = malloc(span);

        // room for its path
        if (!path)
        {
            continue;
        }

        (void)snprintf(path, span, "%s/%s", _directory, names[at]);

        // a repository, a directory, or a file of a kind git reads
        if (strcmp(names[at], ".git") == 0)
        {
            // the repository, once, then the files of its directory
            if (_repo)
            {
                _repo(_directory, _counts);
            }

            // its files, a directory's or the one naming it
            if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
            {
                d_tests_walk(path, _file, NULL, _counts);
            }
        }
        else if (d_fs_kind_of(path) == D_FS_KIND_DIRECTORY)
        {
            // objects and refs hold nothing a text walk reads
            if ( (strcmp(names[at], "objects") != 0) &&
                 (strcmp(names[at], "refs") != 0) &&
                 (strcmp(names[at], "logs") != 0) )
            {
                d_tests_walk(path, _file, _repo, _counts);
            }
        }
        else if (d_git_text_kind_of(names[at]) != D_GIT_TEXT_OTHER)
        {
            _file(path, _counts);
        }

        free(path);
    }

    d_fs_names_free(names, count);

    return;
}


/*
d_tests_sound
  Whether a file's records are sound: its lines one after another to its
end, each record within the line it says, in order -- an entry's value
ending past its last line's end only where a `\` joined the end of the
text to it -- and a configuration's entries within their sections.
  Long as each kind of record is proved in its own loop.
*/
static bool
d_tests_sound(
    const struct d_git_text* _text,
    const char**             _why
)
{
    size_t at = 0u;

    // each line begins where the last one's ending ends
    for (size_t line = 0u; line < _text->line_count; ++line)
    {
        const struct d_git_text_line* const record = &_text->lines[line];

        // in order, its parts within it
        if ( (record->start != at) ||
             (record->end < record->start) ||
             (record->next < record->end) ||
             (record->next > _text->length) )
        {
            *_why = "lines out of order";

            return false;
        }

        at = record->next;
    }

    // to the end
    if (at != _text->length)
    {
        *_why = "lines short of the end";

        return false;
    }

    // each pattern on its line, within it
    for (size_t p = 0u; p < _text->pattern_count; ++p)
    {
        const struct d_git_text_pattern* const pattern = &_text->patterns[p];

        // its line holds it
        if ( (pattern->line >= _text->line_count) ||
             (_text->lines[pattern->line].pattern != p) ||
             (pattern->start < _text->lines[pattern->line].start) ||
             (pattern->end > _text->lines[pattern->line].end) ||
             ( (pattern->first_attribute + pattern->attribute_count)
                   > _text->attribute_count ) )
        {
            *_why = "a pattern outside its line";

            return false;
        }
    }

    // each entry in order, within its lines and its section
    for (size_t e = 0u; e < _text->entry_count; ++e)
    {
        const struct d_git_text_entry* const entry = &_text->entries[e];

        // its lines, its span and its section
        if ( (entry->last_line < entry->line) ||
             (entry->last_line >= _text->line_count) ||
             (entry->start < _text->lines[entry->line].start) ||
             (entry->end > _text->lines[entry->last_line].next) ||
             ( (entry->section != D_GIT_TEXT_NONE) &&
               (entry->section >= _text->section_count) ) ||
             ( (e > 0u) && (entry->start < _text->entries[e - 1u].end) ) )
        {
            *_why = "an entry out of place";

            return false;
        }
    }

    return true;
}


/*
d_tests_same_text
  Whether a document's text is these bytes.
*/
static bool
d_tests_same_text(
    const struct d_git_doc* _doc,
    const char*             _bytes,
    size_t                  _length
)
{
    size_t            length = 0u;
    const char* const text   = d_git_doc_text(_doc, &length);

    return ( (length == _length) &&
             ( (_length == 0u) || (memcmp(text, _bytes, _length) == 0) ) );
}


/*
d_tests_corpus_file
  A file read, proved sound, and each of its first lines set to itself --
the first's text past a byte-order mark -- which must be kept and change
nothing.
  Long as the file is read, proved and its lines set, each a step.
*/
static void
d_tests_corpus_file(
    const char*            _path,
    struct d_tests_counts* _counts
)
{
    char                    why[256];
    const char*             unsound = NULL;
    size_t                  length  = 0u;
    char* const             bytes   = d_tests_load(_path, &length);
    struct d_git_doc* const doc     = d_git_doc_read(_path, D_GIT_TEXT_OTHER,
                                                     why, sizeof(why));

    _counts->files++;

    // read, sound, and the file's own bytes
    if ( (!doc) ||
         (!bytes) )
    {
        d_tests_fail(_counts, _path, (doc) ? "cannot be loaded" : why);
    }
    else if (!d_tests_sound(d_git_doc_records(doc), &unsound))
    {
        d_tests_fail(_counts, _path, unsound);
    }
    else if (!d_tests_same_text(doc, bytes, length))
    {
        d_tests_fail(_counts, _path, "the document is not the file");
    }

    const struct d_git_text* const text  = (doc) ? d_git_doc_records(doc)
                                                 : NULL;
    const size_t                   lines = (text) ? text->line_count : 0u;

    _counts->records += (text) ? (lines + text->pattern_count
                                  + text->entry_count) : 0u;

    // each of the first lines, set to itself, past a line git refused
    for (size_t at = 0u;
         ( (doc) && (at < lines) && (at < 40u) &&
           (text->broken == D_GIT_TEXT_NONE) );
         ++at)
    {
        struct d_git_buffer               line;
        const struct d_git_text_line* const record =
            &d_git_doc_records(doc)->lines[at];
        const size_t                        from   =
            ( (at == 0u) && (d_git_doc_records(doc)->bom) ) ? 3u
                                                            : record->start;

        memset(&line, 0, sizeof(line));
        (void)d_git_buffer_add(&line, &bytes[from], record->end - from);

        // a line kept as it was, changing nothing
        if ( (!line.failed) &&
             ( (d_git_doc_set_line(doc, at, (line.text) ? line.text : "")
                    != at) ||
               (!d_tests_same_text(doc, bytes, length)) ) )
        {
            d_tests_fail(_counts, _path, "a line set to itself changed it");
            d_git_buffer_free(&line);

            break;
        }

        d_git_buffer_free(&line);
    }

    d_git_doc_free(doc);
    free(bytes);

    return;
}


/*
d_tests_corpus_tree
  A tree's objects read, each named as its content says, down to `_left`
more objects.
  Long as a tree's entries are read and proved in one loop.
*/
static void
d_tests_corpus_tree(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _tree,
    struct d_tests_counts*  _counts,
    size_t*                 _left,
    const char*             _path
)
{
    struct d_git_object     tree;
    struct d_git_tree_entry entry;
    size_t                  at = 0u;
    struct d_git_oid        named;

    // a tree read, its name its content's
    if (!d_git_repo_read(_repo, _tree, &tree))
    {
        d_tests_fail(_counts, _path, d_git_repo_problem(_repo));
        d_git_object_release(&tree);

        return;
    }

    d_git_repo_hash_object(_repo, tree.type, tree.data, tree.size, &named);
    _counts->objects++;

    // its name the content's
    if (!d_git_repo_same_oid(_repo, &named, _tree))
    {
        d_tests_fail(_counts, _path, "a tree not named by its content");
    }

    // each entry, while objects are left to read
    while ( (*_left > 0u) &&
            (d_git_repo_tree_next(_repo, &tree, &at, &entry)) )
    {
        struct d_git_object blob;

        --*_left;

        // a subtree walked; a blob read, a gitlink passed over
        if (entry.mode == 040000u)
        {
            d_tests_corpus_tree(_repo, &entry.oid, _counts, _left, _path);
        }
        else if (entry.mode != 0160000u)
        {
            // read, and named by its content
            if (!d_git_repo_read(_repo, &entry.oid, &blob))
            {
                d_tests_fail(_counts, _path, d_git_repo_problem(_repo));
            }
            else
            {
                d_git_repo_hash_object(_repo, blob.type, blob.data,
                                       blob.size, &named);
                _counts->objects++;

                // its name the content's
                if (!d_git_repo_same_oid(_repo, &named, &entry.oid))
                {
                    d_tests_fail(_counts, _path,
                                 "a blob not named by its content");
                }
            }

            d_git_object_release(&blob);
        }
    }

    d_git_object_release(&tree);

    return;
}


/*
d_tests_corpus_repo
  A repository opened; its refs resolved; the history they reach walked,
to a few thousand commits; each commit read and parsed and its tree's
objects read, to fifty thousand; and its index and status read.
  Long as the refs, the history, the trees and the status are each read in
turn.
*/
static void
d_tests_corpus_repo(
    const char*            _path,
    struct d_tests_counts* _counts
)
{
    char                     why[512];
    struct d_git_repo* const repo  = d_git_repo_open(_path, why, sizeof(why));
    struct d_git_refs        refs;
    struct d_git_log_options options;
    struct d_git_oids        commits;
    size_t                   left  = 50000u;

    memset(&refs, 0, sizeof(refs));
    memset(&options, 0, sizeof(options));
    memset(&commits, 0, sizeof(commits));
    _counts->repositories++;

    // the repository, and its refs
    if ( (!repo) ||
         (!d_git_repo_refs(repo, "", &refs)) )
    {
        d_tests_fail(_counts, _path, (repo) ? d_git_repo_problem(repo) : why);
        d_git_refs_release(&refs);
        d_git_repo_free(repo);

        return;
    }

    const char** const revisions = malloc((refs.count + 1u)
                                          * sizeof(*revisions));

    // each ref that leads somewhere, a revision to walk from
    for (size_t at = 0u; ( (revisions) && (at < refs.count) ); ++at)
    {
        // one that names a commit
        if (refs.items[at].resolved)
        {
            revisions[options.revision_count++] = refs.items[at].name;
        }
    }

    options.revisions = revisions;
    options.most      = 3000u;

    // the history walked, its commits and their trees read
    if ( (revisions) &&
         (options.revision_count > 0u) &&
         (!d_git_repo_log(repo, &options, &commits)) )
    {
        d_tests_fail(_counts, _path, d_git_repo_problem(repo));
    }

    // each commit, parsed, and its tree read
    for (size_t at = 0u; ( (at < commits.count) && (left > 0u) ); ++at)
    {
        struct d_git_object object;
        struct d_git_commit commit;

        memset(&commit, 0, sizeof(commit));

        // read and parsed
        if ( (!d_git_repo_read(repo, &commits.items[at], &object)) ||
             (!d_git_repo_parse_commit(repo, &object, &commit)) )
        {
            d_tests_fail(_counts, _path, "a commit does not read");
        }
        else
        {
            _counts->objects++;
            d_tests_corpus_tree(repo, &commit.tree, _counts, &left, _path);
        }

        d_git_commit_release(&commit);
        d_git_object_release(&object);
    }

    struct d_git_status_options status_options;
    struct d_git_status         status;

    memset(&status_options, 0, sizeof(status_options));
    memset(&status, 0, sizeof(status));

    // a work tree's status
    if ( (d_git_repo_work_tree(repo)) &&
         (!d_git_repo_status(repo, &status_options, &status)) )
    {
        d_tests_fail(_counts, _path, d_git_repo_problem(repo));
    }

    d_git_status_release(&status);
    d_git_oids_release(&commits);
    free(revisions);
    d_git_refs_release(&refs);
    d_git_repo_free(repo);

    return;
}


//==============================================================================
// --fuzz: random changes, each proved to touch only what it said
//==============================================================================


static unsigned long g_random = 1u;
static size_t        g_change = 0u;


/*
d_tests_random
  A number below `_below`, from a generator of a fixed sequence, so a seed
replays a run.
*/
static size_t
d_tests_random(
    size_t _below
)
{
    g_random = (g_random * 1103515245ul) + 12345ul;

    return (_below == 0u) ? 0u
                          : (size_t)(((g_random >> 16) & 0x7ffful) % _below);
}


/*
d_tests_random_text
  Text of fragments git's files are made of, a few at random.
*/
static void
d_tests_random_text(
    char*  _out,
    size_t _capacity,
    bool   _lines
)
{
    static const char* const pieces[] = {
        "", "x", "*", "!", "#", "\\", "\"", "[", "]", "[a]", "[a \"b\"]",
        "k = v", "=", " ", "\t", ";", "a.b", "text", "-diff", "eol=lf",
        "[attr]m", "/", "**", "?", "\\n", "\xef\xbb\xbf", "\r", "1" };
    const size_t count  = sizeof(pieces) / sizeof(pieces[0]);
    const size_t length = d_tests_random(6u);
    size_t       used   = 0u;

    _out[0] = '\0';

    // each piece, and now and then a line's end
    for (size_t at = 0u; at < length; ++at)
    {
        const char* const piece = ( (_lines) && (d_tests_random(5u) == 0u) )
                                  ? "\n"
                                  : pieces[d_tests_random(count)];

        // room for it
        if ( (used + strlen(piece) + 1u) < _capacity )
        {
            memcpy(&_out[used], piece, strlen(piece) + 1u);
            used += strlen(piece);
        }
    }

    return;
}


// d_tests_snapshot
//   struct: a document's text and its lines' kinds, before a change.
struct d_tests_snapshot
{
    char*                      text;
    size_t                     length;
    enum d_git_text_line_kind* kinds;
    struct d_git_text_line*    lines;
    size_t                     count;
};


/*
d_tests_snap, d_tests_snap_free
  A document's text and lines, copied; and let go of.
*/
static bool
d_tests_snap(
    const struct d_git_doc*  _doc,
    struct d_tests_snapshot* _out
)
{
    const struct d_git_text* const text   = d_git_doc_records(_doc);
    size_t                         length = 0u;
    const char* const              bytes  = d_git_doc_text(_doc, &length);

    memset(_out, 0, sizeof(*_out));
    _out->text  = malloc(length + 1u);
    _out->lines = malloc((text->line_count + 1u) * sizeof(*_out->lines));

    // room for both
    if ( (!_out->text) ||
         (!_out->lines) )
    {
        free(_out->text);
        free(_out->lines);

        return false;
    }

    memcpy(_out->text, bytes, length);
    _out->text[length] = '\0';
    _out->length       = length;
    _out->count        = text->line_count;

    // the lines, if there are any
    if (text->line_count > 0u)
    {
        memcpy(_out->lines, text->lines,
               text->line_count * sizeof(*_out->lines));
    }

    return true;
}


static void
d_tests_snap_free(
    struct d_tests_snapshot* _snap
)
{
    free(_snap->text);
    free(_snap->lines);

    return;
}


/*
d_tests_same_line
  Whether a line of a snapshot reads as a line of a document: its text and
its kind -- its ending aside, which a line added after one without gives
it.
*/
static bool
d_tests_same_line(
    const struct d_tests_snapshot* _before,
    size_t                         _old,
    const struct d_git_doc*        _after,
    size_t                         _new
)
{
    const struct d_git_text_line* const old    = &_before->lines[_old];
    const struct d_git_text_line* const now    =
        &d_git_doc_records(_after)->lines[_new];
    const char* const                   text   = d_git_doc_text(_after, NULL);
    const size_t                        length = old->end - old->start;

    return ( (old->kind == now->kind) &&
             (length == (now->end - now->start)) &&
             (memcmp(&_before->text[old->start], &text[now->start], length)
                  == 0) );
}


/*
d_tests_splice_kept
  Of a splice kept, whether every line wholly before it, and every line
wholly after it, reads as it did.
*/
static bool
d_tests_splice_kept(
    const struct d_tests_snapshot* _before,
    const struct d_git_doc*        _after,
    size_t                         _start,
    size_t                         _end
)
{
    const size_t count = d_git_doc_records(_after)->line_count;

    // the lines before, from the first
    for (size_t at = 0u;
         ( (at < _before->count) && (_before->lines[at].next <= _start) );
         ++at)
    {
        // each as it was
        if ( (at >= count) ||
             (!d_tests_same_line(_before, at, _after, at)) )
        {
            return false;
        }
    }

    // the lines after, from the last
    for (size_t back = 1u;
         ( (back <= _before->count) &&
           (_before->lines[_before->count - back].start >= _end) &&
           (_before->lines[_before->count - back].start > 0u) );
         ++back)
    {
        // each as it was
        if ( (back > count) ||
             (!d_tests_same_line(_before, _before->count - back, _after,
                                 count - back)) )
        {
            return false;
        }
    }

    return true;
}


/*
d_tests_fresh
  Whether a document's records are what reading its text afresh gives:
its lines, their kinds, and as many patterns, sections and entries.
*/
static bool
d_tests_fresh(
    const struct d_git_doc* _doc
)
{
    const struct d_git_text* const records = d_git_doc_records(_doc);
    struct d_git_text              fresh;
    char                           why[256];
    size_t                         length  = 0u;
    const char* const              text    = d_git_doc_text(_doc, &length);
    bool                           same    = d_git_text_read(
                                                 &fresh, text, length,
                                                 records->kind, why,
                                                 sizeof(why));

    same = ( (same) &&
             (fresh.line_count == records->line_count) &&
             (fresh.pattern_count == records->pattern_count) &&
             (fresh.section_count == records->section_count) &&
             (fresh.entry_count == records->entry_count) &&
             (fresh.broken == records->broken) );

    // each line's kind and place
    for (size_t at = 0u; ( (same) && (at < fresh.line_count) ); ++at)
    {
        same = ( (fresh.lines[at].kind == records->lines[at].kind) &&
                 (fresh.lines[at].start == records->lines[at].start) &&
                 (fresh.lines[at].next == records->lines[at].next) );
    }

    d_git_text_release(&fresh);

    return same;
}


/*
d_tests_fuzz_change
  One random change: a splice, a line set, inserted or removed, a key set,
added or unset, or a state set or taken off; whether it was kept, and the
span a splice replaced.
*/
static bool
d_tests_fuzz_change(
    struct d_git_doc* _doc,
    size_t*           _out_start,
    size_t*           _out_end,
    bool*             _out_splice
)
{
    static const char* const keys[] = { "a.b", "a.k", "core.bare",
                                        "x.y.z", "a.b.k" };
    static const char* const names[] = { "text", "diff", "eol", "m" };
    const size_t lines  = d_git_doc_records(_doc)->line_count;
    const size_t length = d_git_doc_records(_doc)->length;
    const size_t which  = d_tests_random(8u);
    char         text[128];

    d_tests_random_text(text, sizeof(text), (which == 0u));
    g_change     = which;
    *_out_splice = (which == 0u);
    *_out_start  = d_tests_random(length + 1u);
    *_out_end    = *_out_start + d_tests_random(length - *_out_start + 1u);

    // a table of the changes
    switch (which)
    {
        case 0:  return d_git_doc_splice(_doc, *_out_start, *_out_end, text,
                                         strlen(text));
        case 1:  return (d_git_doc_set_line(_doc, d_tests_random(lines + 1u),
                                            text) != D_GIT_TEXT_NONE);
        case 2:  return (d_git_doc_insert_line(_doc,
                                               d_tests_random(lines + 2u),
                                               text) != D_GIT_TEXT_NONE);
        case 3:  return d_git_doc_remove_line(_doc,
                                              d_tests_random(lines + 1u));
        case 4:  return d_git_doc_set(_doc, keys[d_tests_random(5u)], text);
        case 5:  return (d_tests_random(2u) == 0u)
                        ? d_git_doc_add(_doc, keys[d_tests_random(5u)], text)
                        : d_git_doc_unset(_doc, keys[d_tests_random(5u)],
                                          (d_tests_random(2u) == 0u));
        case 6:  return d_git_doc_set_state(
                            _doc, d_tests_random(
                                      d_git_doc_records(_doc)->pattern_count
                                      + 1u),
                            names[d_tests_random(4u)],
                            (enum d_git_text_state)d_tests_random(4u), "lf");
        default: return d_git_doc_remove_state(
                            _doc, d_tests_random(
                                      d_git_doc_records(_doc)->pattern_count
                                      + 1u),
                            names[d_tests_random(4u)]);
    }
}


/*
d_tests_fuzz_text
  Random changes to a text of a kind, each proved: one kept leaves sound
records, the same a fresh reading of its text gives, and -- for a splice --
every line outside it as it was; one refused leaves the text as it was and
says why.
  Long as each change is made and then proved, a branch for each proof.
*/
static void
d_tests_fuzz_text(
    const char*            _name,
    enum d_git_text_kind   _kind,
    const char*            _source,
    size_t                 _length,
    struct d_tests_counts* _counts
)
{
    char              why[256];
    struct d_git_doc* doc = d_git_doc_make(_name, _kind, _source, _length,
                                           why, sizeof(why));

    size_t            failed = _counts->failed;

    _counts->files++;

    // each change, the text begun again when it grows long
    for (size_t round = 0u; ( (doc) && (round < 300u) ); ++round)
    {
        struct d_tests_snapshot before;
        size_t                  start   = 0u;
        size_t                  end     = 0u;
        const char*             unsound = NULL;

        // the text as it is, kept to compare
        if (!d_tests_snap(doc, &before))
        {
            break;
        }

        bool       spliced = false;
        const bool kept    = d_tests_fuzz_change(doc, &start, &end,
                                                 &spliced);

        _counts->changes++;
        _counts->kept    += (kept) ? 1u : 0u;
        _counts->refused += (kept) ? 0u : 1u;

        // kept and sound, or refused and as it was
        if ( (kept) &&
             (!d_tests_sound(d_git_doc_records(doc), &unsound)) )
        {
            d_tests_fail(_counts, _name, unsound);
        }
        else if ( (!kept) &&
                  ( (!d_tests_same_text(doc, before.text, before.length)) ||
                    (d_git_doc_problem(doc)[0] == '\0') ) )
        {
            d_tests_fail(_counts, _name, "a refusal changed the text");
        }
        else if ( (kept) &&
                  (!d_tests_fresh(doc)) )
        {
            d_tests_fail(_counts, _name, "the records are not the text's");
        }
        else if ( (kept) &&
                  (spliced) &&
                  (!d_tests_splice_kept(&before, doc, start, end)) )
        {
            d_tests_fail(_counts, _name, "a splice changed lines outside it");
        }

        // what failed, shown when asked
        if (_counts->failed != failed)
        {
            size_t            length = 0u;
            const char* const now    = d_git_doc_text(doc, &length);

            const char change = (char)('0' + g_change);

            d_tests_show("change", &change, 1u);
            d_tests_show("before", before.text, before.length);
            d_tests_show("after ", now, length);
            d_tests_show("why   ", d_git_doc_problem(doc),
                         strlen(d_git_doc_problem(doc)));
            failed = _counts->failed;
        }

        d_tests_snap_free(&before);

        // a text grown long begun again
        if (d_git_doc_records(doc)->length > 4096u)
        {
            d_git_doc_free(doc);
            doc = d_git_doc_make(_name, _kind, _source, _length, why,
                                 sizeof(why));
        }
    }

    d_git_doc_free(doc);

    return;
}


/*
d_tests_fuzz_file
  Random changes to a file's text, held in memory; nothing is written.
*/
static void
d_tests_fuzz_file(
    const char*            _path,
    struct d_tests_counts* _counts
)
{
    size_t      length = 0u;
    char* const text   = d_tests_load(_path, &length);

    // a file that loads
    if (!text)
    {
        d_tests_fail(_counts, _path, "cannot be loaded");

        return;
    }

    d_tests_fuzz_text(_path, d_git_text_kind_of(_path), text, length,
                      _counts);
    free(text);

    return;
}


/*
d_tests_fuzz_seeds
  Random changes to texts of each kind of the checks' own.
*/
static void
d_tests_fuzz_seeds(
    struct d_tests_counts* _counts
)
{
    static const char* const ignores[] = {
        "", "*.o\n!keep.o\nbuild/\n", "# c\r\n\\#x\r\n  lead \\ \r\n" };
    static const char* const attributes[] = {
        "", "*.c text diff=cpp\n[attr]bin -diff -text\n*.png bin\n",
        "\"q x\" eol=lf\r\n!bad text\r\n" };
    static const char* const configs[] = {
        "", "[core]\n\tbare = false\n[remote \"o\"]\n\turl = x\n",
        "\xef\xbb\xbf[a.B]\r\n\tk = \"v ;\" ; c\r\n\tx = 1 \\\r\n 2\r\n",
        "[a]\n\tx = 1\n[bad\n[b]\n\ty = 2\n" };

    // each seed of each kind
    for (size_t at = 0u; at < 4u; ++at)
    {
        // an ignore file's and an attribute file's, of the first three
        if (at < 3u)
        {
            d_tests_fuzz_text(".gitignore", D_GIT_TEXT_IGNORE, ignores[at],
                              strlen(ignores[at]), _counts);
            d_tests_fuzz_text(".gitattributes", D_GIT_TEXT_ATTRIBUTES,
                              attributes[at], strlen(attributes[at]),
                              _counts);
        }

        d_tests_fuzz_text("config", D_GIT_TEXT_CONFIG, configs[at],
                          strlen(configs[at]), _counts);
    }

    return;
}


//==============================================================================
// --oracle: git's answers, as git's commands write them
//==============================================================================


/*
d_tests_print_path
  A path as git's porcelain writes one: in C's quotes when it holds a
control byte, a quote, a backslash or a byte past ASCII -- or, with
`_space`, a blank.
*/
static void
d_tests_print_path(
    const char* _path,
    bool        _space
)
{
    struct d_git_buffer quoted;

    memset(&quoted, 0, sizeof(quoted));
    (void)d_git_text_quote_path(&quoted, _path, strlen(_path));

    // quoted for a blank too, when git's porcelain quotes one
    if ( (_space) &&
         (quoted.text) &&
         (quoted.text[0] != '"') &&
         (strchr(_path, ' ')) )
    {
        printf("\"%s\"", quoted.text);
    }
    else
    {
        fputs( (quoted.text) ? quoted.text : "", stdout);
    }

    d_git_buffer_free(&quoted);

    return;
}


/*
d_tests_print_oid
  A name in hex, and what follows it.
*/
static void
d_tests_print_oid(
    const struct d_git_repo* _repo,
    const struct d_git_oid*  _oid,
    const char*              _after
)
{
    char hex[(2u * D_GIT_OID_MOST) + 1u];

    d_git_repo_hex(_repo, _oid, hex);
    printf("%s%s", hex, _after);

    return;
}


/*
d_tests_oracle_names
  rev and short: each revision's name in hex, whole or short, or FAIL.
*/
static int
d_tests_oracle_names(
    struct d_git_repo* _repo,
    char**             _args,
    int                _count,
    bool               _short
)
{
    // each revision
    for (int at = 0; at < _count; ++at)
    {
        struct d_git_oid oid;
        char             hex[(2u * D_GIT_OID_MOST) + 1u];

        // named, and shortened if asked
        if ( (!d_git_repo_rev(_repo, _args[at], &oid)) ||
             ( (_short) && (!d_git_repo_abbreviate(_repo, &oid, 0u, hex)) ) )
        {
            puts("FAIL");
        }
        else if (_short)
        {
            puts(hex);
        }
        else
        {
            d_tests_print_oid(_repo, &oid, "\n");
        }
    }

    return 0;
}


/*
d_tests_oracle_check
  cat-check: each name read from standard input, as git cat-file
--batch-check writes it -- name, type, size -- BADHASH after one whose
content is not its name's, "missing" for one not read.
*/
static int
d_tests_oracle_check(
    struct d_git_repo* _repo
)
{
    char line[256];

    // each name, a line
    while (fgets(line, (int)sizeof(line), stdin))
    {
        struct d_git_oid    oid;
        struct d_git_oid    named;
        struct d_git_object object;

        line[strcspn(line, "\r\n")] = '\0';

        // a name, read
        if ( (!d_git_repo_parse_hex(_repo, line, &oid)) ||
             (!d_git_repo_read(_repo, &oid, &object)) )
        {
            printf("%s missing\n", line);
            d_git_object_release(&object);

            continue;
        }

        d_git_repo_hash_object(_repo, object.type, object.data, object.size,
                               &named);
        printf("%s %s %lu%s\n", line, d_git_object_type_name(object.type),
               (unsigned long)object.size,
               (d_git_repo_same_oid(_repo, &oid, &named)) ? "" : " BADHASH");
        d_git_object_release(&object);
    }

    return 0;
}


/*
d_tests_oracle_log
  log: the commits revisions reach, as git rev-list gives them --
--first-parent, -n most, and -- path among the arguments.
*/
static int
d_tests_oracle_log(
    struct d_git_repo* _repo,
    char**             _args,
    int                _count
)
{
    struct d_git_log_options options;
    struct d_git_oids        oids;
    const char**             revisions = malloc(((size_t)_count + 1u)
                                                * sizeof(*revisions));

    memset(&options, 0, sizeof(options));

    // each argument: an option, or a revision
    for (int at = 0; ( (revisions) && (at < _count) ); ++at)
    {
        // a table of the options
        if (strcmp(_args[at], "--first-parent") == 0)
        {
            options.first_parent = true;
        }
        else if ( (strcmp(_args[at], "-n") == 0) &&
                  (at + 1 < _count) )
        {
            options.most = (size_t)strtoul(_args[++at], NULL, 10);
        }
        else if ( (strcmp(_args[at], "--") == 0) &&
                  (at + 1 < _count) )
        {
            options.path = _args[++at];
        }
        else
        {
            revisions[options.revision_count++] = _args[at];
        }
    }

    options.revisions = revisions;

    const bool walked = ( (revisions) &&
                          (d_git_repo_log(_repo, &options, &oids)) );

    // each commit, a line
    for (size_t at = 0u; ( (walked) && (at < oids.count) ); ++at)
    {
        d_tests_print_oid(_repo, &oids.items[at], "\n");
    }

    // why not
    if (!walked)
    {
        printf("FAIL %s\n", d_git_repo_problem(_repo));
    }

    d_git_oids_release(&oids);
    free(revisions);

    return (walked) ? 0 : 1;
}


/*
d_tests_oracle_tree
  ls-tree [-r] name: a tree's entries, as git ls-tree writes them.
*/
static void
d_tests_oracle_tree(
    struct d_git_repo*      _repo,
    const struct d_git_oid* _tree,
    const char*             _prefix,
    bool                    _deep
)
{
    struct d_git_object     tree;
    struct d_git_tree_entry entry;
    size_t                  at = 0u;
    char                    path[4096];

    // the tree read
    if (!d_git_repo_read(_repo, _tree, &tree))
    {
        printf("FAIL %s\n", d_git_repo_problem(_repo));

        return;
    }

    // each entry, a subtree walked when deep
    while (d_git_repo_tree_next(_repo, &tree, &at, &entry))
    {
        const bool subtree = (entry.mode == 040000u);

        (void)snprintf(path, sizeof(path), "%s%.*s", _prefix,
                       (int)entry.name_length, entry.name);

        // a subtree walked, or the entry written
        if ( (subtree) &&
             (_deep) )
        {
            (void)strcat(path, "/");
            d_tests_oracle_tree(_repo, &entry.oid, path, true);
        }
        else
        {
            printf("%06o %s ", (unsigned)entry.mode,
                   (subtree) ? "tree"
                             : (entry.mode == 0160000u) ? "commit" : "blob");
            d_tests_print_oid(_repo, &entry.oid, "\t");
            d_tests_print_path(path, false);
            putchar('\n');
        }
    }

    d_git_object_release(&tree);

    return;
}


/*
d_tests_oracle_work
  ls-files, check-ignore, check-attr and status, as git writes them:
ls-files -s; check-ignore -v --non-matching; check-attr -a, ordered by
name; status --porcelain --no-renames, with -uall, -uno, --ignored.
  Long as four commands are answered here, each a few lines.
*/
static int
d_tests_oracle_work(
    struct d_git_repo* _repo,
    const char*        _command,
    char**             _args,
    int                _count
)
{
    // ls-files -s: each entry
    if (strcmp(_command, "ls-files") == 0)
    {
        const size_t count = d_git_repo_index_count(_repo);

        // each entry, a line
        for (size_t at = 0u; ( (count != D_GIT_REPO_NONE) && (at < count) );
             ++at)
        {
            const struct d_git_index_entry* const entry =
                d_git_repo_index_entry(_repo, at);

            printf("%06o ", (unsigned)entry->mode);
            d_tests_print_oid(_repo, &entry->oid, "");
            printf(" %u\t", entry->stage);
            d_tests_print_path(entry->path, false);
            putchar('\n');
        }

        return (count == D_GIT_REPO_NONE) ? 1 : 0;
    }

    // check-ignore and check-attr: each path
    for (int at = 0;
         ( (at < _count) &&
           ( (strcmp(_command, "check-ignore") == 0) ||
             (strcmp(_command, "check-attr") == 0) ) );
         ++at)
    {
        struct d_git_ignore_match match;
        struct d_git_text_values  values;

        // what decides whether it is ignored, or its attributes
        if (_command[6] == 'i')
        {
            // the rule, or none
            if (d_git_repo_ignored(_repo, _args[at], &match))
            {
                printf("%s:%lu:%.*s\t%s\n", match.file,
                       (unsigned long)match.line, (int)match.pattern_length,
                       match.pattern, _args[at]);
            }
            else
            {
                printf("::\t%s\n", _args[at]);
            }

            continue;
        }

        (void)d_git_repo_attributes(_repo, _args[at], &values);

        // each attribute said
        for (size_t v = 0u; v < values.count; ++v)
        {
            const struct d_git_text_value* const value = &values.items[v];

            printf("%s: %.*s: ", _args[at], (int)value->name_length,
                   value->name);
            printf("%.*s\n",
                   (int)((value->state == D_GIT_TEXT_VALUE)
                         ? value->value_length
                         : (value->state == D_GIT_TEXT_SET) ? 3u : 5u),
                   (value->state == D_GIT_TEXT_VALUE) ? value->value
                   : (value->state == D_GIT_TEXT_SET) ? "set" : "unset");
        }

        d_git_text_values_release(&values);
    }

    return 0;
}


/*
d_tests_oracle_status
  status: as git status --porcelain --no-renames writes it.
*/
static int
d_tests_oracle_status(
    struct d_git_repo* _repo,
    char**             _args,
    int                _count
)
{
    struct d_git_status_options options;
    struct d_git_status         status;

    memset(&options, 0, sizeof(options));

    // each option
    for (int at = 0; at < _count; ++at)
    {
        options.untracked = (strcmp(_args[at], "-uall") == 0)
                            ? D_GIT_UNTRACKED_ALL
                            : (strcmp(_args[at], "-uno") == 0)
                              ? D_GIT_UNTRACKED_NO
                              : options.untracked;
        options.ignored   = ( (options.ignored) ||
                              (strcmp(_args[at], "--ignored") == 0) );
    }

    // the status read
    if (!d_git_repo_status(_repo, &options, &status))
    {
        printf("FAIL %s\n", d_git_repo_problem(_repo));
        d_git_status_release(&status);

        return 1;
    }

    // each line
    for (size_t at = 0u; at < status.count; ++at)
    {
        printf("%c%c ", status.items[at].index, status.items[at].work);
        d_tests_print_path(status.items[at].path, true);
        putchar('\n');
    }

    d_git_status_release(&status);

    return 0;
}


/*
d_tests_oracle_config
  config-list: each setting as git config --list --show-scope writes it;
config-file path: a file's entries as git config -f path --list does, or
the line git refuses.
*/
static int
d_tests_oracle_config(
    struct d_git_repo* _repo,
    const char*        _file
)
{
    static const char* const scopes[] = { "system", "global", "local",
                                          "worktree", "command" };
    char                     why[256];

    // a repository's settings
    if (!_file)
    {
        // each, in git's order
        for (size_t at = 0u; at < d_git_repo_settings(_repo); ++at)
        {
            const struct d_git_setting* const setting =
                d_git_repo_setting(_repo, at);

            printf("%s\t%s%s%s\n", scopes[setting->scope], setting->key,
                   (setting->value) ? "=" : "",
                   (setting->value) ? setting->value : "");
        }

        return 0;
    }

    struct d_git_doc* const doc = d_git_doc_read(_file, D_GIT_TEXT_CONFIG,
                                                 why, sizeof(why));
    const struct d_git_text* const text = (doc) ? d_git_doc_records(doc)
                                                : NULL;

    // a file read to its end, or the line git refuses
    if ( (text) &&
         (text->broken != D_GIT_TEXT_NONE) )
    {
        printf("BAD %s\n", d_git_text_string(text, text->problems[0].what));
    }

    // each entry
    for (size_t at = 0u;
         ( (text) && (text->broken == D_GIT_TEXT_NONE) &&
           (at < text->entry_count) );
         ++at)
    {
        const char* const value = d_git_text_string(text,
                                                    text->entries[at].value);

        printf("%s%s%s\n", d_git_text_string(text, text->entries[at].key),
               (value) ? "=" : "", (value) ? value : "");
    }

    d_git_doc_free(doc);

    return (doc) ? 0 : 1;
}


/*
d_tests_oracle
  As git's command answers, of the repository a path is in: rev, short,
cat-check, cat, log, merge-base, ls-tree [-r], ls-files, check-ignore,
check-attr, status, refs, config-list, config-file.
  Long as a dispatch on the command is.
*/
static int
d_tests_oracle(
    const char* _path,
    const char* _command,
    char**      _args,
    int         _count
)
{
    char                     why[512];
    struct d_git_repo* const repo = (strcmp(_command, "config-file") == 0)
                                    ? NULL
                                    : d_git_repo_open(_path, why, sizeof(why));
    struct d_git_oid         oid;
    struct d_git_oid         other;
    struct d_git_object      object;
    struct d_git_oids        oids;
    struct d_git_refs        refs;
    int                      status = 0;

    memset(&object, 0, sizeof(object));
    memset(&oids, 0, sizeof(oids));
    memset(&refs, 0, sizeof(refs));

    // a repository, unless only a file is read
    if ( (!repo) &&
         (strcmp(_command, "config-file") != 0) )
    {
        printf("FAIL %s\n", why);

        return 1;
    }

    // a table of the commands
    if (strcmp(_command, "config-file") == 0)
    {
        status = d_tests_oracle_config(NULL, (_count > 0) ? _args[0] : "");
    }
    else if ( (strcmp(_command, "rev") == 0) ||
              (strcmp(_command, "short") == 0) )
    {
        status = d_tests_oracle_names(repo, _args, _count,
                                      (_command[0] == 's'));
    }
    else if (strcmp(_command, "cat-check") == 0)
    {
        status = d_tests_oracle_check(repo);
    }
    else if ( (strcmp(_command, "cat") == 0) &&
              (_count > 0) )
    {
        status = ( (d_git_repo_rev(repo, _args[0], &oid)) &&
                   (d_git_repo_read(repo, &oid, &object)) &&
                   (fwrite(object.data, 1u, object.size, stdout)
                        == object.size) ) ? 0 : 1;
        d_git_object_release(&object);
    }
    else if (strcmp(_command, "log") == 0)
    {
        status = d_tests_oracle_log(repo, _args, _count);
    }
    else if ( (strcmp(_command, "merge-base") == 0) &&
              (_count > 1) )
    {
        status = ( (d_git_repo_rev(repo, _args[0], &oid)) &&
                   (d_git_repo_rev(repo, _args[1], &other)) &&
                   (d_git_repo_merge_base(repo, &oid, &other, &oids)) )
                 ? 0 : 1;

        // each base
        for (size_t at = 0u; at < oids.count; ++at)
        {
            d_tests_print_oid(repo, &oids.items[at], "\n");
        }

        d_git_oids_release(&oids);
    }
    else if ( (strcmp(_command, "ls-tree") == 0) &&
              (_count > 0) )
    {
        const bool deep = ( (_count > 1) && (strcmp(_args[0], "-r") == 0) );

        status = ( (d_git_repo_rev(repo, _args[(deep) ? 1 : 0], &oid)) &&
                   (d_git_repo_read(repo, &oid, &object)) ) ? 0 : 1;

        // a commit's tree, or a tree
        if ( (status == 0) &&
             (object.type != D_GIT_OBJECT_TREE) )
        {
            char peel[(2u * D_GIT_OID_MOST) + 16u];

            d_git_repo_hex(repo, &oid, peel);
            (void)strcat(peel, "^{tree}");
            status = (d_git_repo_rev(repo, peel, &oid)) ? 0 : 1;
        }

        d_git_object_release(&object);

        // the entries
        if (status == 0)
        {
            d_tests_oracle_tree(repo, &oid, "", deep);
        }
    }
    else if (strcmp(_command, "status") == 0)
    {
        status = d_tests_oracle_status(repo, _args, _count);
    }
    else if (strcmp(_command, "refs") == 0)
    {
        status = (d_git_repo_refs(repo, "", &refs)) ? 0 : 1;

        // each ref
        for (size_t at = 0u; at < refs.count; ++at)
        {
            d_tests_print_oid(repo, &refs.items[at].oid, " ");
            puts(refs.items[at].name);
        }

        d_git_refs_release(&refs);
    }
    else if (strcmp(_command, "config-list") == 0)
    {
        status = d_tests_oracle_config(repo, NULL);
    }
    else
    {
        status = d_tests_oracle_work(repo, _command, _args, _count);
    }

    d_git_repo_free(repo);

    return status;
}


//==============================================================================
// main
//==============================================================================


/*
main
  The unit checks; with --corpus every file and repository under the
directories, with --fuzz random changes, and with --oracle git's answers.
  Long as each mode is a few lines.
*/
int
main(
    int    argc,
    char** argv
)
{
    struct d_tests_counts counts;

    memset(&counts, 0, sizeof(counts));

    // a corpus run reads files and repositories, and reports them
    if ( (argc > 2) &&
         (strcmp(argv[1], "--corpus") == 0) )
    {
        // every directory named
        for (int at = 2; at < argc; ++at)
        {
            d_tests_walk(argv[at], d_tests_corpus_file, d_tests_corpus_repo,
                         &counts);
        }

        printf("\n%lu files, %lu records, %lu repositories, %lu objects: "
               "%lu failed\n", (unsigned long)counts.files,
               (unsigned long)counts.records,
               (unsigned long)counts.repositories,
               (unsigned long)counts.objects, (unsigned long)counts.failed);

        return ( (counts.failed == 0u) && (counts.files > 0u) ) ? 0 : 1;
    }

    // a fuzz run changes copies held in memory, and writes nothing
    if ( (argc > 2) &&
         (strcmp(argv[1], "--fuzz") == 0) )
    {
        g_random = strtoul(argv[2], NULL, 10);
        d_tests_fuzz_seeds(&counts);

        // every directory named
        for (int at = 3; at < argc; ++at)
        {
            d_tests_walk(argv[at], d_tests_fuzz_file, NULL, &counts);
        }

        printf("\n%lu files, %lu changes: %lu kept, %lu refused; "
               "%lu failed\n", (unsigned long)counts.files,
               (unsigned long)counts.changes, (unsigned long)counts.kept,
               (unsigned long)counts.refused, (unsigned long)counts.failed);

        return (counts.failed == 0u) ? 0 : 1;
    }

    // an oracle answers one command, as git does
    if ( (argc > 3) &&
         (strcmp(argv[1], "--oracle") == 0) )
    {
        return d_tests_oracle(argv[2], argv[3], &argv[4], argc - 4);
    }

    d_tests_kinds();
    d_tests_ignore_file();
    d_tests_attribute_file();
    d_tests_config_file();
    d_tests_config_refusals();
    d_tests_config_marks();
    d_tests_wildmatch();
    d_tests_ignoring();
    d_tests_attributing();
    d_tests_keys();
    d_tests_values();
    d_tests_quoting();
    d_tests_config_edits();
    d_tests_line_edits();
    d_tests_state_edits();
    d_tests_entry_edits();
    d_tests_saving( (argc > 1) ? argv[1] : ".");
    d_tests_repository( (argc > 1) ? argv[1] : ".");

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
