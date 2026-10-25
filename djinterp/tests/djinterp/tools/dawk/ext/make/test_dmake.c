/*******************************************************************************
* djinterp [tools]                                                  test_dmake.c
*
* Exercises the Makefile extension's reader as it stands: `dmake.h`'s logical
* lines, joined by each make's rule for a backslash that ends a line, with
* LF and CR LF endings; the guess of a file's make from the first line only
* one make reads; and each make's statements -- GNU make's, POSIX make's as
* pdpmake reads them, BSD make's as bmake does, NMake's as Microsoft's
* documentation describes them -- each part in its place, and what each
* make refuses of a file whatever its variables hold, in its words.
* Then `dmake_doc.h`: every change a file takes, the layout each takes from
* the file, what each refuses, and saving.
*   With --corpus, it reads each file named on its standard input instead,
* as the make named or guessed, and proves the records sound: statements in
* order and apart, each over whole lines, its parts within it; comments in
* order; every line a statement's or blank or a comment.
*   With --fuzz, it makes changes at random to each file named instead --
* parts and words set, inserted and removed, commands added and removed,
* statements inserted, replaced and removed, and raw splices -- and proves
* of each, apart from the document's own check, that a change kept left
* the file readable, every byte and every statement and comment outside it
* as it was, and that a change refused left the file as it was.
*
*   test_dmake [work directory]           the unit checks
*   test_dmake --corpus [make] < files    every file named, one to a line
*   test_dmake --fuzz [seed] [n] < files  n changes to every file named
*
* path:      /tests/djinterp/tools/dawk/ext/make/test_dmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/make/dmake.h"  // corresponding header
// std
#include <stddef.h>  // ptrdiff_t
#include <stdio.h>   // printf, snprintf, fgets, remove
#include <stdlib.h>  // free, malloc, strtoul
#include <string.h>  // memcmp, memcpy, strcat, strcmp, strcspn, strlen,
                     // strstr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"            // d_text_read
#include "../../../../../../inc/djinterp/tools/dawk/ext/make/dmake_doc.h"  // d_make_doc


static int g_checks = 0;
static int g_failed = 0;


//==============================================================================
// 1.  HELPERS
//==============================================================================


/**
 * @brief Counts one check, and names it when it fails.
 *
 * @param[in] _condition  whether the check held.
 * @param[in] _what       what it checked.
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


/**
 * @brief Reads a text's lines and checks their places.
 *
 * @param[in] _text     the text.
 * @param[in] _dialect  the make whose rule joins its lines.
 * @param[in] _places   each line's start, end, next, line and end line, in
 *                      turn, five to a line.
 * @param[in] _count    how many lines it must have.
 * @param[in] _what     what is checked.
 */
static void
d_tests_lines(
    const char*         _text,
    enum d_make_dialect _dialect,
    const size_t*       _places,
    size_t              _count,
    const char*         _what
)
{
    struct d_make_lines lines;
    const bool          read = d_make_lines_read(&lines,
                                                 _text,
                                                 strlen(_text),
                                                 _dialect);
    bool                same = ( (read) &&
                                 (lines.count == _count) );

    // each line's places, while they agree
    for (size_t index = 0; (same) && (index < _count); index++)
    {
        const struct d_make_line* const line  = &lines.lines[index];
        const size_t* const             place = _places + (index * 5u);

        same = ( (line->start == place[0]) &&
                 (line->end == place[1]) &&
                 (line->next == place[2]) &&
                 (line->line == place[3]) &&
                 (line->end_line == place[4]) );
    }

    d_tests_check(same, _what);
    d_make_lines_release(&lines);

    return;
}


//==============================================================================
// 2.  LINES
//==============================================================================


/*
d_tests_lines_endings
  The line endings and the text's end.
  Tests the following:
  - an empty text has no line
  - LF and CR LF end a line, the CR no part of it, and set `crlf`
  - a lone CR is a byte of its line
  - the last line counts though no newline ends it
*/
static void
d_tests_lines_endings(void)
{
    static const size_t two[]  = { 0, 1, 2, 1, 1,   2, 3, 3, 2, 2 };
    static const size_t crlf[] = { 0, 1, 3, 1, 1,   3, 4, 6, 2, 2 };
    static const size_t cr[]   = { 0, 3, 4, 1, 1 };

    d_tests_lines("", D_MAKE_GNU, two, 0, "an empty text has no line");
    d_tests_lines("a\nb", D_MAKE_GNU, two, 2, "LF, and a last line unended");
    d_tests_lines("a\r\nb\r\n", D_MAKE_GNU, crlf, 2, "CR LF");
    d_tests_lines("a\rb\n", D_MAKE_GNU, cr, 1, "a lone CR is a byte");

    struct d_make_lines lines;

    (void)d_make_lines_read(&lines, "a\r\nb\n", 5u, D_MAKE_GNU);
    d_tests_check(lines.crlf, "crlf from the first line ending");
    d_make_lines_release(&lines);
    (void)d_make_lines_read(&lines, "a\nb\r\n", 5u, D_MAKE_GNU);
    d_tests_check(!lines.crlf, "LF from the first line ending");
    d_make_lines_release(&lines);

    return;
}


/*
d_tests_lines_continued
  Each make's rule for a backslash that ends a line.
  Tests the following:
  - GNU and BSD join at an odd run of backslashes, not an even one
  - POSIX, as pdpmake reads it, joins at any run
  - NMake joins unless an odd run of carets escapes the backslash
  - a continuation over CR LF, and one the text's end cuts off
*/
static void
d_tests_lines_continued(void)
{
    static const size_t even[]   = { 0, 3, 4, 1, 1,   4, 5, 6, 2, 2 };
    static const size_t joined[] = { 0, 5, 6, 1, 2 };
    static const size_t three[]  = { 0, 6, 7, 1, 2 };
    static const size_t over[]   = { 0, 5, 7, 1, 2 };
    static const size_t cut[]    = { 0, 2, 3, 1, 1 };

    d_tests_lines("a\\\\\nb\n", D_MAKE_GNU, even, 2, "GNU: `\\\\` ends");
    d_tests_lines("a\\\\\nb\n", D_MAKE_BSD, even, 2, "BSD: `\\\\` ends");
    d_tests_lines("a\\\\\nb\n", D_MAKE_POSIX, joined, 1,
                  "POSIX: `\\\\` joins");
    d_tests_lines("a\\\\\\\nb\n", D_MAKE_GNU, three, 1, "GNU: `\\\\\\`");
    d_tests_lines("a^\\\nb\n", D_MAKE_NMAKE, even, 2, "NMake: `^\\` ends");
    d_tests_lines("a^^\\\nb\n", D_MAKE_NMAKE, three, 1,
                  "NMake: `^^\\` joins");
    d_tests_lines("a\\\\\nb\n", D_MAKE_NMAKE, joined, 1,
                  "NMake: `\\\\` joins");
    d_tests_lines("a\\\r\nb\r\n", D_MAKE_GNU, over, 1, "over CR LF");
    d_tests_lines("a\\\n", D_MAKE_GNU, cut, 1, "cut off by the end");
    d_tests_check(d_make_continues("x\\", 0, 2, D_MAKE_GNU),
                  "continues: one backslash");
    d_tests_check(!d_make_continues("x\\", 1, 1, D_MAKE_GNU),
                  "continues: an empty line does not");

    return;
}


//==============================================================================
// 3.  DIALECTS
//==============================================================================


/**
 * @brief Checks the make guessed of a text.
 *
 * @param[in] _text      the text.
 * @param[in] _expected  the make it must be guessed.
 */
static void
d_tests_guess(
    const char*         _text,
    enum d_make_dialect _expected
)
{
    const enum d_make_dialect got = d_make_guess(_text, strlen(_text));
    char                      what[96];

    (void)snprintf(what,
                   sizeof(what),
                   "guess %s: %.60s",
                   d_make_dialect_name(_expected),
                   _text);
    d_tests_check(got == _expected, what);

    return;
}


/*
d_tests_dialects
  The names and the guess.
  Tests the following:
  - each make's name, and finding a make by it
  - BSD's and NMake's directives, the latter in either case, at a line's
    start only
  - GNU's directives and function calls
  - a recipe's line says nothing, and the first line that decides wins
  - `:=` and a pattern rule suggest GNU where no line decides
  - a file with none of these is POSIX's
*/
static void
d_tests_dialects(void)
{
    enum d_make_dialect found = D_MAKE_GNU;

    d_tests_check(strcmp(d_make_dialect_name(D_MAKE_BSD), "bsd") == 0,
                  "the name of BSD's make");
    d_tests_check( (d_make_dialect_of("nmake", &found)) &&
                   (found == D_MAKE_NMAKE),
                   "NMake by its name");
    d_tests_check(!d_make_dialect_of("ninja", &found), "no make's name");
    d_tests_guess(".include <bsd.prog.mk>\n", D_MAKE_BSD);
    d_tests_guess("X = 1\n.  if defined(X)\n.endif\n", D_MAKE_BSD);
    d_tests_guess(".c.o:\n\t$(CC) -c $<\n", D_MAKE_POSIX);
    d_tests_guess(".SUFFIXES: .c .o\n", D_MAKE_POSIX);
    d_tests_guess("!IF \"$(X)\" == \"\"\n!ENDIF\n", D_MAKE_NMAKE);
    d_tests_guess("!include <win32.mak>\n", D_MAKE_NMAKE);
    d_tests_guess("  !IF 1\n", D_MAKE_POSIX);
    d_tests_guess("ifeq ($(A),b)\nendif\n", D_MAKE_GNU);
    d_tests_guess("  override X = 1\n", D_MAKE_GNU);
    d_tests_guess("x := $(call f,a)\n", D_MAKE_GNU);
    d_tests_guess("y = ${filter-out a,$(x)}\n", D_MAKE_GNU);
    d_tests_guess("define:\n\techo\n", D_MAKE_POSIX);
    d_tests_guess("all:\n\t.if x\n\tifeq\n", D_MAKE_POSIX);
    d_tests_guess("x = 1\n.include \"y\"\nifeq (a,b)\n", D_MAKE_BSD);
    d_tests_guess("CC = cc\nall: x\n\t$(CC) -o x x.c\n", D_MAKE_POSIX);
    d_tests_guess("", D_MAKE_POSIX);
    d_tests_guess("SHELL := /bin/bash\nall:\n", D_MAKE_GNU);
    d_tests_guess("%.o: %.c\n\tcc -c $<\n", D_MAKE_GNU);
    d_tests_guess("X ::= 1\nY = a:=b\n", D_MAKE_POSIX);
    d_tests_guess("X := 1\n.include <bsd.prog.mk>\n", D_MAKE_BSD);

    return;
}


//==============================================================================
// 4.  GNU MAKE'S STATEMENTS
//==============================================================================


/**
 * @brief Writes bytes onto a description, a tab as `\t` and a line feed as
 *        `\n`, cut short at the description's room.
 *
 * @param[in,out] _out     the description, terminated.
 * @param[in]     _room    its size.
 * @param[in]     _text    the bytes.
 * @param[in]     _length  how many.
 */
static void
d_tests_put(
    char*       _out,
    size_t      _room,
    const char* _text,
    size_t      _length
)
{
    size_t at = strlen(_out);

    // each byte, while there is room for it and an escape
    for (size_t index = 0; (index < _length) && (at + 3u < _room); index++)
    {
        const char byte = _text[index];

        // a table of the bytes written escaped
        if (byte == '\t')
        {
            _out[at++] = '\\';
            _out[at++] = 't';
        }
        else if (byte == '\n')
        {
            _out[at++] = '\\';
            _out[at++] = 'n';
        }
        else
        {
            _out[at++] = byte;
        }
    }

    _out[at] = '\0';

    return;
}


/**
 * @brief Writes a part of a statement onto a description, `name=[text]`,
 *        when it is not empty.
 *
 * @param[in,out] _out    the description.
 * @param[in]     _room   its size.
 * @param[in]     _text   the file's text.
 * @param[in]     _name   the part's name.
 * @param[in]     _span   the part.
 */
static void
d_tests_part(
    char*              _out,
    size_t             _room,
    const char*        _text,
    const char*        _name,
    struct d_make_span _span
)
{
    // an empty part is not written
    if (_span.start >= _span.end)
    {
        return;
    }

    d_tests_put(_out, _room, " ", 1u);
    d_tests_put(_out, _room, _name, strlen(_name));
    d_tests_put(_out, _room, "=[", 2u);
    d_tests_put(_out, _room, _text + _span.start, _span.end - _span.start);
    d_tests_put(_out, _room, "]", 1u);

    return;
}


/**
 * @brief Describes a file's statements and comments.
 *
 * @param[in]  _make  the records.
 * @param[out] _out   receives the statements, ` | ` between, then ` || `
 *                    and the comments, when there are any.
 * @param[in]  _room  its size.
 *
 * A statement is `kind@line`, `-end` when it ends on another, then its
 * parts; a conditional adds `d` and its depth and `n` and its next branch,
 * `-` for none; an orphan recipe line adds `orphan`. A comment is
 * `#@line:statement=[text]`.
 *
 * Long as the description's parts, each a line or two, written in one order
 * so every case reads alike.
 */
static void
d_tests_describe(
    const struct d_make* _make,
    char*                _out,
    size_t               _room
)
{
    char number[48];

    _out[0] = '\0';

    // each statement
    for (size_t index = 0; index < _make->statement_count; index++)
    {
        const struct d_make_statement* const s = &_make->statements[index];
        const char* const                    t = _make->text;
        const char* const                    kind = d_make_kind_name(s->kind);

        (void)snprintf(number, sizeof(number), "%s%s@%lu",
                       (index > 0u) ? " | " : "", kind,
                       (unsigned long)s->line);
        d_tests_put(_out, _room, number, strlen(number));

        // a statement over more than one line
        if (s->end_line != s->line)
        {
            (void)snprintf(number, sizeof(number), "-%lu",
                           (unsigned long)s->end_line);
            d_tests_put(_out, _room, number, strlen(number));
        }

        d_tests_part(_out, _room, t, "modifiers", s->modifiers);
        d_tests_part(_out, _room, t, "word", s->word);
        d_tests_part(_out, _room, t, "targets", s->targets);
        d_tests_part(_out, _room, t, "colon", s->colon);
        d_tests_part(_out, _room, t, "pattern", s->pattern);
        d_tests_part(_out, _room, t, "name", s->name);
        d_tests_part(_out, _room, t, "op", s->separator);
        d_tests_part(_out, _room, t, "text", s->text);
        d_tests_part(_out, _room, t, "order", s->order_only);
        d_tests_part(_out, _room, t, "recipe", s->recipe);
        d_tests_part(_out, _room, t, "body", s->body);

        // a conditional's place among its branches
        if (s->kind == D_MAKE_CONDITIONAL)
        {
            (void)snprintf(number, sizeof(number), " d%lu n",
                           (unsigned long)s->depth);
            d_tests_put(_out, _room, number, strlen(number));
            (void)snprintf(number, sizeof(number), "%ld",
                           (s->next_branch == D_MAKE_NONE)
                               ? -1L
                               : (long)s->next_branch);
            d_tests_put(_out, _room, number, strlen(number));
        }

        // a recipe's line with no rule above
        if (s->orphan)
        {
            d_tests_put(_out, _room, " orphan", 7u);
        }
    }

    // each comment
    for (size_t index = 0; index < _make->comment_count; index++)
    {
        const struct d_make_comment* const c = &_make->comments[index];

        (void)snprintf(number, sizeof(number), "%s#@%lu:%ld=[",
                       (index > 0u) ? " " : " || ",
                       (unsigned long)c->line,
                       (c->within == D_MAKE_NONE) ? -1L : (long)c->within);
        d_tests_put(_out, _room, number, strlen(number));
        d_tests_put(_out, _room, _make->text + c->start, c->end - c->start);
        d_tests_put(_out, _room, "]", 1u);
    }

    return;
}


/**
 * @brief Reads a text as a make's and checks its description, or its
 *        refusal.
 *
 * @param[in] _dialect   the make.
 * @param[in] _text      the text.
 * @param[in] _expected  the description, or, for a text refused, the
 *                       problem: "line L: ...".
 */
static void
d_tests_as(
    enum d_make_dialect _dialect,
    const char*         _text,
    const char*         _expected
)
{
    struct d_make         make;
    struct d_make_problem problem;
    char                  got[2048];
    const bool            read = d_make_read(&make,
                                             _text,
                                             strlen(_text),
                                             _dialect,
                                             &problem);

    // a table of the two outcomes
    if (read)
    {
        d_tests_describe(&make, got, sizeof(got));
    }
    else
    {
        (void)snprintf(got, sizeof(got), "%s", problem.text);
    }

    const bool same = (strcmp(got, _expected) == 0);

    d_tests_check(same, _text);

    // a difference is shown whole
    if (!same)
    {
        printf("        got      %s\n        expected %s\n", got, _expected);
    }

    d_make_release(&make);

    return;
}


/**
 * @brief Reads a text as GNU make's and checks it, as d_tests_as does.
 *
 * @param[in] _text      the text.
 * @param[in] _expected  the description, or the problem.
 */
static void
d_tests_gnu(
    const char* _text,
    const char* _expected
)
{
    d_tests_as(D_MAKE_GNU, _text, _expected);

    return;
}


/*
d_tests_gnu_assignments
  Assignments, defines and undefines, as GNU make 4.3 read them here.
  Tests the following:
  - each operator; `:::=`, no operator in 4.3, read as `::` and `:=`
  - modifiers, and `unexport`, which 4.3 reads as a directive
  - a comment after a value, `\#` and a `#` in a reference not one
  - a value's trailing blanks kept, a continuation within it
  - assignments for targets, the value past a `;`
  - `define` with and without an operator, nested, an `endef` led by a
    tab no end, a comment after `endef`; `undefine`
*/
static void
d_tests_gnu_assignments(void)
{
    d_tests_gnu("X := a\\#b # c\n",
                "assignment@1 name=[X] op=[:=] text=[a\\#b]"
                " || #@1:0=[# c]");
    d_tests_gnu("X := $(subst #,a,b#c)\n",
                "assignment@1 name=[X] op=[:=] text=[$(subst #,a,b#c)]");
    d_tests_gnu("A=1\nB ::= 2\nC += 3\nD ?= 4\nE != echo\n",
                "assignment@1 name=[A] op=[=] text=[1]"
                " | assignment@2 name=[B] op=[::=] text=[2]"
                " | assignment@3 name=[C] op=[+=] text=[3]"
                " | assignment@4 name=[D] op=[?=] text=[4]"
                " | assignment@5 name=[E] op=[!=] text=[echo]");
    d_tests_gnu("override export X ?= 1\n",
                "assignment@1 modifiers=[override export] name=[X] op=[?=]"
                " text=[1]");
    d_tests_gnu("unexport X = 1\n",
                "directive@1 word=[unexport] text=[X = 1]");
    d_tests_gnu("X :::= y\n",
                "assignment@1 targets=[X] colon=[::] op=[:=] text=[y]");
    d_tests_gnu("X = a \\\n   b\n",
                "assignment@1-2 name=[X] op=[=] text=[a \\\\n   b]");
    d_tests_gnu("all: X = a;b # c\n",
                "assignment@1 targets=[all] colon=[:] name=[X] op=[=]"
                " text=[a;b # c]");
    d_tests_gnu("all: private X = a # c\n",
                "assignment@1 modifiers=[private] targets=[all] colon=[:]"
                " name=[X] op=[=] text=[a] || #@1:0=[# c]");
    d_tests_gnu("define X =\nab\n  endef # e\n",
                "define@1-3 word=[define] name=[X] op=[=] body=[ab\\n]"
                " || #@3:0=[# e]");
    d_tests_gnu("define X\n  define Y\n  endef\nab\nendef\n",
                "define@1-5 word=[define] name=[X]"
                " body=[  define Y\\n  endef\\nab\\n]");
    d_tests_gnu("define X\n\tendef\nendef\n",
                "define@1-3 word=[define] name=[X] body=[\\tendef\\n]");
    d_tests_gnu("override undefine X\n",
                "directive@1 modifiers=[override] word=[undefine] name=[X]"
                " text=[X]");

    return;
}


/*
d_tests_gnu_rules
  Rules and their recipes, as GNU make 4.3 read them here.
  Tests the following:
  - targets, `:`, `::` and `&:`, static patterns, order-only
    prerequisites, a recipe after `;` that keeps its `#`
  - a recipe's lines through comments, empty lines and conditionals
  - a tab-led assignment before any rule; a recipe's line with none, an
    orphan; an assignment ends a rule
  - `.RECIPEPREFIX`, an escaped colon, an expansion, `ifeq(` no directive
*/
static void
d_tests_gnu_rules(void)
{
    d_tests_gnu("a b &: c | d ; @echo $@ # x\n",
                "rule@1 targets=[a b] colon=[&:] text=[c] order=[d]"
                " recipe=[ @echo $@ # x]");
    d_tests_gnu("a b: %: %.c ; @echo $@\n",
                "rule@1 targets=[a b] colon=[:] pattern=[%] text=[%.c]"
                " recipe=[ @echo $@]");
    d_tests_gnu("a:: b # c\n",
                "rule@1 targets=[a] colon=[::] text=[b] || #@1:0=[# c]");
    d_tests_gnu("%.o: %.c\n\t$(CC) -c $<\n",
                "rule@1 targets=[%.o] colon=[:] text=[%.c]"
                " | recipe@2 word=[\\t] text=[$(CC) -c $<]");
    d_tests_gnu("all:\n# c\n\n\t@echo r1\n",
                "rule@1 targets=[all] colon=[:]"
                " | recipe@4 word=[\\t@] text=[echo r1] || #@2:-1=[# c]");
    d_tests_gnu("all:\nifeq (a,a)\n\t@echo r2\nelse\n\t-echo no\nendif\n",
                "rule@1 targets=[all] colon=[:]"
                " | conditional@2 word=[ifeq] text=[(a,a)] d0 n3"
                " | recipe@3 word=[\\t@] text=[echo r2]"
                " | conditional@4 word=[else] d0 n5"
                " | recipe@5 word=[\\t-] text=[echo no]"
                " | conditional@6 word=[endif] d0 n-1");
    d_tests_gnu("\tX = 3\nall:;@echo\n",
                "assignment@1 name=[X] op=[=] text=[3]"
                " | rule@2 targets=[all] colon=[:] recipe=[@echo]");
    d_tests_gnu("all:\nX=1\n\t@echo no\n",
                "rule@1 targets=[all] colon=[:]"
                " | assignment@2 name=[X] op=[=] text=[1]"
                " | recipe@3 word=[\\t@] text=[echo no] orphan");
    d_tests_gnu(".RECIPEPREFIX := >\nall:\n> @echo q1\n",
                "assignment@1 name=[.RECIPEPREFIX] op=[:=] text=[>]"
                " | rule@2 targets=[all] colon=[:]"
                " | recipe@3 word=[> @] text=[echo q1]");
    d_tests_gnu("a\\:b: c\n",
                "rule@1 targets=[a\\:b] colon=[:] text=[c]");
    d_tests_gnu("$(foreach x,a,$(eval $(x):))\nfoo # c\n",
                "expansion@1 text=[$(foreach x,a,$(eval $(x):))]"
                " | expansion@2 text=[foo] || #@2:1=[# c]");
    d_tests_gnu("ifeq(a,a)\n", "expansion@1 text=[ifeq(a,a)]");
    d_tests_gnu("x: y \\\n  z ; a\n",
                "rule@1-2 targets=[x] colon=[:] text=[y \\\\n  z]"
                " recipe=[ a]");

    return;
}


/*
d_tests_gnu_directives
  Conditionals, includes and the other directives.
  Tests the following:
  - an `else` chain: each branch's depth and the next branch
  - nesting: the inner conditional one deeper
  - includes and vpath, their arguments' trailing blanks left out
  - what make refuses whatever the variables hold, at make's line: a
    missing endif one line past the last, with and without a last newline;
    an extraneous endif or else; a second bare else; an unterminated define
  - an else with text sets no bare else
*/
static void
d_tests_gnu_directives(void)
{
    d_tests_gnu("ifdef A\nelse ifdef B\nelse\nendif\n",
                "conditional@1 word=[ifdef] text=[A] d0 n1"
                " | conditional@2 word=[else] text=[ifdef B] d0 n2"
                " | conditional@3 word=[else] d0 n3"
                " | conditional@4 word=[endif] d0 n-1");
    d_tests_gnu("ifeq (a,b)\nifdef A B\nendif\nendif\n",
                "conditional@1 word=[ifeq] text=[(a,b)] d0 n3"
                " | conditional@2 word=[ifdef] text=[A B] d1 n2"
                " | conditional@3 word=[endif] d1 n-1"
                " | conditional@4 word=[endif] d0 n-1");
    d_tests_gnu("-include a.mk b.mk   # c\nvpath %.c src\n",
                "include@1 word=[-include] text=[a.mk b.mk]"
                " | directive@2 word=[vpath] text=[%.c src]"
                " || #@1:0=[# c]");
    d_tests_gnu("ifeq (a,a)\nX=1\nall:;@echo x\n\n",
                "line 5: missing 'endif'");
    d_tests_gnu("ifeq (a,a)\nall:;@echo x",
                "line 3: missing 'endif'");
    d_tests_gnu("all:;@echo x\nendif\n", "line 2: extraneous 'endif'");
    d_tests_gnu("X=1\nelse\n", "line 2: extraneous 'else'");
    d_tests_gnu("ifeq (a,a)\nelse\nelse\nendif\n",
                "line 3: only one 'else' per conditional");
    d_tests_gnu("X=1\ndefine Y\nabc\n\nall:;@echo x\n",
                "line 2: missing 'endef', unterminated 'define'");
    d_tests_gnu("ifeq (a,a)\nelse foo\nelse\nendif\n",
                "conditional@1 word=[ifeq] text=[(a,a)] d0 n1"
                " | conditional@2 word=[else] text=[foo] d0 n2"
                " | conditional@3 word=[else] d0 n3"
                " | conditional@4 word=[endif] d0 n-1");

    return;
}


//==============================================================================
// 5.  THE OTHER MAKES' STATEMENTS
//==============================================================================


/*
d_tests_posix
  POSIX make's statements, as pdpmake read them here in its POSIX mode.
  Tests the following:
  - a comment is the first `#`, no escape, in a command too; a value's
    trailing blanks kept
  - a tab-led macro; a rule's commands through comments and empty lines
  - each operator, and `:=`, whose colon is the name's
  - a rule's `;` command, a first word with `:` then `;` a rule's
  - an include, an expansion
  - what pdpmake refuses whatever the macros hold, in its words
*/
static void
d_tests_posix(void)
{
    const enum d_make_dialect p = D_MAKE_POSIX;

    d_tests_as(p, "X = a\\#b # c\n",
               "assignment@1 name=[X] op=[=] text=[a\\] || #@1:0=[#b # c]");
    d_tests_as(p, "\tX = 3\nall:\n# c\n\n\t@echo a2 # x\n",
               "assignment@1 name=[X] op=[=] text=[3]"
               " | rule@2 targets=[all] colon=[:]"
               " | recipe@5 word=[\\t@] text=[echo a2 ]"
               " || #@3:-1=[# c] #@5:2=[# x]");
    d_tests_as(p, "X += 2\nX ::= a\nY :::= $(X)\nZ:=1\nW != echo\n",
               "assignment@1 name=[X] op=[+=] text=[2]"
               " | assignment@2 name=[X] op=[::=] text=[a]"
               " | assignment@3 name=[Y] op=[:::=] text=[$(X)]"
               " | assignment@4 name=[Z:] op=[=] text=[1]"
               " | assignment@5 name=[W] op=[!=] text=[echo]");
    d_tests_as(p, "all: b ; @echo b1 # x\na:b;c=d\n",
               "rule@1 targets=[all] colon=[:] text=[b] recipe=[ @echo b1 ]"
               " | rule@2 targets=[a] colon=[:] text=[b] recipe=[c=d]"
               " || #@1:0=[# x]");
    d_tests_as(p, "include a.mk # c\n-include b.mk\n$(X)\n",
               "include@1 word=[include] text=[a.mk]"
               " | include@2 word=[-include] text=[b.mk]"
               " | expansion@3 text=[$(X)] || #@1:0=[# c]");
    d_tests_as(p, "foo\n", "line 1: expected separator");
    d_tests_as(p, "X=1\n  # c\n", "line 2: expected separator");
    d_tests_as(p, "X=1\n\t@echo no\n", "line 2: command not allowed here");
    d_tests_as(p, "all: X=b\n", "line 1: invalid macro assignment");
    d_tests_as(p, "include \n", "line 1: no include file");
    d_tests_as(p, "-include\n", "line 1: expected separator");

    return;
}


/*
d_tests_bsd
  BSD make's statements, as bmake read them here.
  Tests the following:
  - `\#` escapes; a value's trailing blanks dropped
  - Parse_IsVar's assignments: `X:Y = 1` one, `all: X=b` a dependency,
    `::=` no operator
  - the `!` operator, a `;` command, a command's own comment
  - conditionals: each word, `.ifx` as `.if x`, blanks after the `.`,
    within a rule's commands
  - a loop, its parts and body; the includes and other directives
  - a tab-led line with no rule an orphan; an indented command in a rule
  - what bmake refuses whatever the variables hold, in its words
*/
static void
d_tests_bsd(void)
{
    const enum d_make_dialect b = D_MAKE_BSD;

    d_tests_as(b, "X = a\\#b # c\nX:Y = 1\nall: X=b\nX ::= a\n",
               "assignment@1 name=[X] op=[=] text=[a\\#b]"
               " | assignment@2 name=[X:Y] op=[=] text=[1]"
               " | rule@3 targets=[all] colon=[:] text=[X=b]"
               " | rule@4 targets=[X] colon=[::] text=[= a]"
               " || #@1:0=[# c]");
    d_tests_as(b, "X!=echo hi\nall ! b ; @echo d9 # x\n\t@echo $@ # y\n",
               "assignment@1 name=[X] op=[!=] text=[echo hi]"
               " | rule@2 targets=[all] colon=[!] text=[b]"
               " recipe=[ @echo d9 ]"
               " | recipe@3 word=[\\t@] text=[echo $@ # y]"
               " || #@2:1=[# x]");
    d_tests_as(b, ".if 1 # c\nX=1\n.elif 0\n.else\n.endif\n",
               "conditional@1 word=[if] text=[1] d0 n2"
               " | assignment@2 name=[X] op=[=] text=[1]"
               " | conditional@3 word=[elif] text=[0] d0 n3"
               " | conditional@4 word=[else] d0 n4"
               " | conditional@5 word=[endif] d0 n-1 || #@1:0=[# c]");
    d_tests_as(b, ".  ifdef X\n.ifx 1\n.endif\n.endif\n",
               "conditional@1 word=[ifdef] text=[X] d0 n3"
               " | conditional@2 word=[if] text=[x 1] d1 n2"
               " | conditional@3 word=[endif] d1 n-1"
               " | conditional@4 word=[endif] d0 n-1");
    d_tests_as(b, "all:\n.if 1\n\t@echo d2\n.endif\n  echo x\n",
               "rule@1 targets=[all] colon=[:]"
               " | conditional@2 word=[if] text=[1] d0 n3"
               " | recipe@3 word=[\\t@] text=[echo d2]"
               " | conditional@4 word=[endif] d0 n-1"
               " | recipe@5 word=[  ] text=[echo x]");
    d_tests_as(b, ".for i j in a b\nX+=$i\n.endfor # e\n",
               "loop@1-3 word=[for] name=[i j] op=[in] text=[a b]"
               " body=[X+=$i\\n] || #@3:0=[# e]");
    d_tests_as(b, ".include <bsd.prog.mk>\n.-include \"x\"\n"
               "include foo.mk\n",
               "include@1 word=[include] text=[<bsd.prog.mk>]"
               " | include@2 word=[-include] text=[\"x\"]"
               " | include@3 word=[include] text=[foo.mk]");
    d_tests_as(b, ".undef X\n.export-env Y\n.info hi\n\tX = 3\n",
               "directive@1 word=[undef] text=[X]"
               " | directive@2 word=[export] text=[-env Y]"
               " | directive@3 word=[info] text=[hi]"
               " | recipe@4 word=[\\t] text=[X = 3] orphan");
    d_tests_as(b, ".if 1\n.else\n.else\n.endif\n",
               "conditional@1 word=[if] text=[1] d0 n1"
               " | conditional@2 word=[else] d0 n2"
               " | conditional@3 word=[else] d0 n3"
               " | conditional@4 word=[endif] d0 n-1");
    d_tests_as(b, ".if 1\nX=1\n", "line 0: 1 open conditional");
    d_tests_as(b, ".if 1\n.if 2\n", "line 0: 2 open conditionals");
    d_tests_as(b, "X=1\n.endif\n", "line 2: if-less endif");
    d_tests_as(b, ".else\n", "line 1: if-less else");
    d_tests_as(b, ".elif 1\n", "line 1: if-less elif");
    d_tests_as(b, ".endfor\n", "line 1: for-less endfor");
    d_tests_as(b, ".for i in a\nX=1\n",
               "line 3: Unexpected end of file in for loop.");

    return;
}


/*
d_tests_nmake
  NMake's statements, as Microsoft's documentation describes them.
  Tests the following:
  - the documentation's comments: on a line, a macro, a dependency, a
    command; a command turned into a comment; one an inline command keeps
  - `^#` no comment; a drive's colon; an inference rule and `::`
  - the preprocessor's conditionals, `!ELSE IF`, includes, messages
  - an inline file; an empty line after a rule ends it, one between its
    commands does not
  - what NMake refuses whatever its macros hold
*/
static void
d_tests_nmake(void)
{
    const enum d_make_dialect n = D_MAKE_NMAKE;

    d_tests_as(n, "# Comment\nOPTIONS = /MAP  # on a macro\n\n"
               "all.exe : one.obj two.obj  # on a dependency\n"
               "    link one.obj two.obj  # in commands\n"
               "#   copy *.obj \\objects\n"
               "    copy one.exe \\release\n",
               "assignment@2 name=[OPTIONS] op=[=] text=[/MAP]"
               " | rule@4 targets=[all.exe] colon=[:] text=[one.obj two.obj]"
               " | recipe@5 word=[    ] text=[link one.obj two.obj]"
               " | recipe@7 word=[    ] text=[copy one.exe \\release]"
               " || #@1:-1=[# Comment] #@2:0=[# on a macro]"
               " #@4:1=[# on a dependency] #@5:2=[# in commands]"
               " #@6:-1=[#   copy *.obj \\objects]");
    d_tests_as(n, "my.exe : my.obj ; link my.obj  # kept\nX = a ^# b # c\n",
               "rule@1 targets=[my.exe] colon=[:] text=[my.obj]"
               " recipe=[ link my.obj  # kept]"
               " | assignment@2 name=[X] op=[=] text=[a ^# b]"
               " || #@2:1=[# c]");
    d_tests_as(n, "c:\\a.obj : c:\\a.c\n{src}.c{obj}.obj::\n\t@cl $<\n",
               "rule@1 targets=[c:\\a.obj] colon=[:] text=[c:\\a.c]"
               " | rule@2 targets=[{src}.c{obj}.obj] colon=[::]"
               " | recipe@3 word=[\\t@] text=[cl $<]");
    d_tests_as(n, "!IF \"$(X)\" == \"1\"\nY = 1\n!ELSE IF 1\n"
               "!  ELSEIFDEF Z\n!else\n!ENDIF\n",
               "conditional@1 word=[IF] text=[\"$(X)\" == \"1\"] d0 n2"
               " | assignment@2 name=[Y] op=[=] text=[1]"
               " | conditional@3 word=[ELSE] text=[IF 1] d0 n3"
               " | conditional@4 word=[ELSEIFDEF] text=[Z] d0 n4"
               " | conditional@5 word=[else] d0 n5"
               " | conditional@6 word=[ENDIF] d0 n-1");
    d_tests_as(n, "!include <win32.mak>\n!MESSAGE hi ^# there\n",
               "include@1 word=[include] text=[<win32.mak>]"
               " | directive@2 word=[MESSAGE] text=[hi ^# there]");
    d_tests_as(n, "a.exe: a.obj\n\t-2 link @<< # x\na.obj\n/out:a.exe\n"
               "<<\n",
               "rule@1 targets=[a.exe] colon=[:] text=[a.obj]"
               " | recipe@2-5 word=[\\t-2 ] text=[link @<<]"
               " body=[a.obj\\n/out:a.exe\\n] || #@2:1=[# x]");
    d_tests_as(n, "a.exe: a.obj\n\n\tlink a.obj\n",
               "rule@1 targets=[a.exe] colon=[:] text=[a.obj]"
               " | recipe@3 word=[\\t] text=[link a.obj] orphan");
    d_tests_as(n, "a.exe: a.obj\n\tcl a.c\n\n\tlink a.obj\n",
               "rule@1 targets=[a.exe] colon=[:] text=[a.obj]"
               " | recipe@2 word=[\\t] text=[cl a.c]"
               " | recipe@4 word=[\\t] text=[link a.obj]");
    d_tests_as(n, "!ENDIF\n",
               "line 1: fatal error U1021: syntax error : !ENDIF unexpected");
    d_tests_as(n, "X=1\n!ELSE\n",
               "line 2: fatal error U1021: syntax error : !ELSE unexpected");
    d_tests_as(n, "!IF 1\nX=1\n",
               "line 3: fatal error U1020: end-of-file found before next "
               "directive");

    return;
}


//==============================================================================
// 6.  THE DOCUMENT
//==============================================================================


/**
 * @brief Checks a document's text.
 *
 * @param[in] _doc       the document.
 * @param[in] _expected  the text it must hold.
 * @param[in] _what      what is checked.
 */
static void
d_tests_doc_is(
    const struct d_make_doc* _doc,
    const char*              _expected,
    const char*              _what
)
{
    size_t            length = 0;
    const char* const text   = d_make_doc_text(_doc, &length);
    const bool        same   = ( (length == strlen(_expected)) &&
                                 (memcmp(text, _expected, length) == 0) );

    d_tests_check(same, _what);

    // a difference is shown whole
    if (!same)
    {
        printf("        got      [%s]\n        expected [%s]\n        "
               "problem  %s\n", text, _expected, d_make_doc_problem(_doc));
    }

    return;
}


/**
 * @brief Checks that a change was refused, the text as it was.
 *
 * @param[in] _doc     the document.
 * @param[in] _kept    what the change gave.
 * @param[in] _before  the text before it.
 * @param[in] _why     a part of the reason it must give.
 * @param[in] _what    what is checked.
 */
static void
d_tests_doc_refused(
    const struct d_make_doc* _doc,
    bool                     _kept,
    const char*              _before,
    const char*              _why,
    const char*              _what
)
{
    const bool reason = (strstr(d_make_doc_problem(_doc), _why) != NULL);

    d_tests_check( (!_kept) &&
                   (reason),
                   _what);

    // a wrong reason is shown
    if (!reason)
    {
        printf("        problem  %s\n", d_make_doc_problem(_doc));
    }

    d_tests_doc_is(_doc, _before, _what);

    return;
}


/*
d_tests_doc_parts
  Parts set, and what is refused.
  Tests the following:
  - a value, a name, an operator; a body, its line feed added
  - a value that would read as less, an operator that makes another
    statement, a part a statement has not
  - a raw splice; where the last change was
*/
static void
d_tests_doc_parts(void)
{
    struct d_make_problem problem;
    struct d_make_doc*    doc = d_make_doc_make("", "X = 1 # c\nall:\n",
                                                15u, D_MAKE_GNU, &problem);
    size_t                start = 0;
    size_t                old_end = 0;
    size_t                new_end = 0;

    d_tests_check(doc != NULL, "a document of a text");
    (void)d_make_doc_set(doc, 0u, D_MAKE_PART_TEXT, "2");
    d_tests_doc_is(doc, "X = 2 # c\nall:\n", "a value set");
    d_tests_check( (d_make_doc_changed(doc, &start, &old_end, &new_end)) &&
                   (start == 4u) &&
                   (old_end == 5u) &&
                   (new_end == 5u),
                   "where the change was");
    (void)d_make_doc_set(doc, 0u, D_MAKE_PART_NAME, "Y");
    (void)d_make_doc_set(doc, 0u, D_MAKE_PART_SEPARATOR, ":=");
    d_tests_doc_is(doc, "Y := 2 # c\nall:\n", "a name and an operator set");
    d_tests_doc_refused(doc,
                        d_make_doc_set(doc, 0u, D_MAKE_PART_TEXT, "3 # x"),
                        "Y := 2 # c\nall:\n", "comment",
                        "a value that reads as less is refused");
    d_tests_doc_refused(doc,
                        d_make_doc_set(doc, 0u, D_MAKE_PART_SEPARATOR, ":::="),
                        "Y := 2 # c\nall:\n", "as asked",
                        "an operator 4.3 does not read is refused");
    d_tests_doc_refused(doc,
                        d_make_doc_set(doc, 1u, D_MAKE_PART_NAME, "Z"),
                        "Y := 2 # c\nall:\n", "no such part",
                        "a part a rule has not is refused");
    d_tests_check(d_make_doc_splice(doc, 0u, 0u, "# top\n", 6u),
                  "a raw splice");
    d_tests_doc_is(doc, "# top\nY := 2 # c\nall:\n", "...kept");
    d_make_doc_free(doc);

    doc = d_make_doc_make("", "define X\nab\nendef\n", 18u, D_MAKE_GNU,
                          &problem);
    (void)d_make_doc_set(doc, 0u, D_MAKE_PART_BODY, "cd\nef");
    d_tests_doc_is(doc, "define X\ncd\nef\nendef\n", "a body set");
    d_make_doc_free(doc);

    return;
}


/*
d_tests_doc_words
  A rule's words.
  Tests the following:
  - a prerequisite inserted at the end and the start, removed, set
  - the first order-only one written with its `|`, more after it, the last
    taken with its `|`
  - a target inserted and removed; the last one kept
  - order-only words refused in a make that has none
*/
static void
d_tests_doc_words(void)
{
    struct d_make_problem problem;
    const char* const     text = "all: a b\n\t@echo\n";
    struct d_make_doc*    doc  = d_make_doc_make("", text, strlen(text),
                                                 D_MAKE_GNU, &problem);

    (void)d_make_doc_insert_word(doc, 0u, D_MAKE_PART_TEXT, 2u, "c");
    (void)d_make_doc_insert_word(doc, 0u, D_MAKE_PART_TEXT, 0u, "z");
    d_tests_doc_is(doc, "all: z a b c\n\t@echo\n", "prerequisites inserted");
    (void)d_make_doc_remove_word(doc, 0u, D_MAKE_PART_TEXT, 1u);
    (void)d_make_doc_set_word(doc, 0u, D_MAKE_PART_TEXT, 0u, "y");
    d_tests_doc_is(doc, "all: y b c\n\t@echo\n", "removed and set");
    (void)d_make_doc_insert_word(doc, 0u, D_MAKE_PART_ORDER_ONLY, 0u, "d");
    (void)d_make_doc_insert_word(doc, 0u, D_MAKE_PART_ORDER_ONLY, 1u, "e");
    d_tests_doc_is(doc, "all: y b c | d e\n\t@echo\n", "order-only inserted");
    (void)d_make_doc_remove_word(doc, 0u, D_MAKE_PART_ORDER_ONLY, 1u);
    (void)d_make_doc_remove_word(doc, 0u, D_MAKE_PART_ORDER_ONLY, 0u);
    d_tests_doc_is(doc, "all: y b c\n\t@echo\n", "order-only removed, `|`");
    (void)d_make_doc_insert_word(doc, 0u, D_MAKE_PART_TARGETS, 1u, "two");
    (void)d_make_doc_remove_word(doc, 0u, D_MAKE_PART_TARGETS, 0u);
    d_tests_doc_is(doc, "two: y b c\n\t@echo\n", "a target in, a target out");
    d_tests_doc_refused(doc,
                        d_make_doc_remove_word(doc, 0u, D_MAKE_PART_TARGETS,
                                               0u),
                        "two: y b c\n\t@echo\n", "one target",
                        "the last target is kept");
    d_make_doc_free(doc);

    doc = d_make_doc_make("", "a.exe:\n\tlink\n", 13u, D_MAKE_NMAKE,
                          &problem);
    (void)d_make_doc_insert_word(doc, 0u, D_MAKE_PART_TEXT, 0u, "a.obj");
    d_tests_doc_is(doc, "a.exe: a.obj\n\tlink\n", "NMake: a first dependent");
    d_tests_doc_refused(doc,
                        d_make_doc_insert_word(doc, 0u,
                                               D_MAKE_PART_ORDER_ONLY, 0u,
                                               "x"),
                        "a.exe: a.obj\n\tlink\n", "only GNU",
                        "NMake: order-only refused");
    d_make_doc_free(doc);

    return;
}


/*
d_tests_doc_statements
  Commands and statements.
  Tests the following:
  - a command added among a rule's and after them, with its prefix, and in
    CR LF; one removed
  - statements inserted above the comment that leads into one; replaced by
    one of the kind, and not by another; removed
  - refused: a change that leaves a command with no rule, one that leaves
    the file unreadable
  - BSD: a value within a conditional; a command added
*/
static void
d_tests_doc_statements(void)
{
    struct d_make_problem problem;
    const char* const     text = "X = 1\n# the rule\nall:\n\t@echo\n";
    struct d_make_doc*    doc  = d_make_doc_make("", text, strlen(text),
                                                 D_MAKE_GNU, &problem);

    (void)d_make_doc_add_command(doc, 1u, 1u, "echo two");
    (void)d_make_doc_add_command(doc, 1u, 0u, "echo one");
    d_tests_doc_is(doc, "X = 1\n# the rule\nall:\n\techo one\n\t@echo\n"
                        "\techo two\n", "commands added");
    (void)d_make_doc_remove_command(doc, 2u);
    (void)d_make_doc_insert_statements(doc, 1u, "Z = 3");
    d_tests_doc_is(doc, "X = 1\nZ = 3\n# the rule\nall:\n\t@echo\n"
                        "\techo two\n", "above the comment");
    (void)d_make_doc_set_statement(doc, 1u, "W := 4");
    d_tests_doc_refused(doc, d_make_doc_set_statement(doc, 1u, "w:"),
                        "X = 1\nW := 4\n# the rule\nall:\n\t@echo\n"
                        "\techo two\n", "of the kind",
                        "a statement of another kind is refused");
    d_tests_doc_refused(doc, d_make_doc_insert_statements(doc, 3u, "Q=1"),
                        "X = 1\nW := 4\n# the rule\nall:\n\t@echo\n"
                        "\techo two\n", "outside it",
                        "a command left with no rule is refused");
    (void)d_make_doc_remove_statements(doc, 0u, 1u);
    d_tests_doc_is(doc, "# the rule\nall:\n\t@echo\n\techo two\n",
                   "statements removed");
    d_make_doc_free(doc);

    doc = d_make_doc_make("", "ifeq (a,a)\nX=1\nendif\n", 21u, D_MAKE_GNU,
                          &problem);
    d_tests_doc_refused(doc, d_make_doc_remove_statements(doc, 0u, 0u),
                        "ifeq (a,a)\nX=1\nendif\n", "extraneous 'endif'",
                        "a change that leaves the file unreadable");
    d_make_doc_free(doc);

    doc = d_make_doc_make("", "X = 1\r\nall:\r\n\t@echo\r\n", 21u,
                          D_MAKE_GNU, &problem);
    (void)d_make_doc_add_command(doc, 1u, 1u, "echo b");
    d_tests_doc_is(doc, "X = 1\r\nall:\r\n\t@echo\r\n\techo b\r\n",
                   "a command in CR LF");
    d_make_doc_free(doc);

    doc = d_make_doc_make("", ".if 1\nX = 1\n.endif\nall:\n\techo\n", 30u,
                          D_MAKE_BSD, &problem);
    (void)d_make_doc_set(doc, 1u, D_MAKE_PART_TEXT, "2");
    (void)d_make_doc_add_command(doc, 3u, 1u, "echo 2");
    d_tests_doc_is(doc, ".if 1\nX = 2\n.endif\nall:\n\techo\n\techo 2\n",
                   "BSD: a value and a command");
    d_make_doc_free(doc);

    return;
}


/*
d_tests_doc_save
  Saving.
  Tests the following:
  - written, then unchanged; written elsewhere, its directories made
  - a failure, under a file, and a document read back
*/
static void
d_tests_doc_save(
    const char* _work
)
{
    struct d_make_problem problem;
    char                  path[512];
    char                  other[512];
    char                  bad[512];
    struct d_make_doc*    doc = d_make_doc_make("", "X = 1\n", 6u,
                                                D_MAKE_GNU, &problem);

    (void)snprintf(path, sizeof(path), "%s/dmake_save.mk", _work);
    (void)snprintf(other, sizeof(other), "%s/dmake_dir/a/b.mk", _work);
    (void)snprintf(bad, sizeof(bad), "%s/dmake_save.mk/x.mk", _work);
    (void)remove(path);
    (void)remove(other);
    d_tests_check(d_make_doc_save(doc, path, &problem) == D_MAKE_SAVE_WRITTEN,
                  "saved");
    d_tests_check(d_make_doc_save(doc, path, &problem) ==
                  D_MAKE_SAVE_UNCHANGED,
                  "saved again, unchanged");
    d_tests_check(d_make_doc_save(doc, other, &problem) ==
                  D_MAKE_SAVE_WRITTEN,
                  "saved elsewhere, its directories made");
    d_tests_check(d_make_doc_save(doc, bad, &problem) == D_MAKE_SAVE_FAILED,
                  "a save under a file fails");
    d_make_doc_free(doc);

    doc = d_make_doc_read(path, D_MAKE_GNU, &problem);
    d_tests_doc_is(doc, "X = 1\n", "read back");
    d_make_doc_free(doc);
    (void)remove(path);
    (void)remove(other);

    return;
}


//==============================================================================
// 7.  THE CORPUS
//==============================================================================


/**
 * @brief Proves one file's records sound.
 *
 * @param[in] _make  the records of a file read whole.
 * @return why they are not, or `NULL` when they are.
 *
 * Long as a list of proofs, each a loop of a few lines over the records.
 */
static const char*
d_tests_sound(
    const struct d_make* _make
)
{
    const struct d_make_lines* const lines   = &_make->lines;
    size_t                           covered = 0;

    // each statement: in order, apart, over whole lines, its parts in it
    for (size_t index = 0; index < _make->statement_count; index++)
    {
        const struct d_make_statement* const s = &_make->statements[index];
        const struct d_make_span             parts[] =
        {
            s->modifiers, s->word, s->targets, s->colon, s->pattern,
            s->name, s->separator, s->text, s->order_only, s->recipe,
            s->body
        };

        // in order and apart from the one before
        if ( (s->start >= s->end) ||
             (s->end > _make->length) ||
             ( (index > 0u) &&
               (_make->statements[index - 1u].end > s->start) ) )
        {
            return "statements out of order";
        }

        // each part within the statement
        for (size_t part = 0; part < sizeof(parts) / sizeof(parts[0]); part++)
        {
            // a part outside its statement
            if ( (parts[part].start > parts[part].end) ||
                 (parts[part].start < s->start) ||
                 (parts[part].end > s->end) )
            {
                return "a part outside its statement";
            }
        }

        // the lines before it that no statement holds
        while ( (covered < lines->count) &&
                (lines->lines[covered].start < s->start) )
        {
            const struct d_make_line* const line = &lines->lines[covered];
            size_t                          at   = line->start;

            // its blanks
            while ( (at < line->end) &&
                    ( (_make->text[at] == ' ') ||
                      (_make->text[at] == '\t') ) )
            {
                at++;
            }

            // a line no statement holds is empty, or a comment
            if ( (at < line->end) &&
                 (_make->text[at] != '#') )
            {
                return "a line no statement holds";
            }

            covered++;
        }

        // the lines it holds
        while ( (covered < lines->count) &&
                (lines->lines[covered].start < s->end) )
        {
            covered++;
        }
    }

    // each comment: in order, within the text
    for (size_t index = 0; index < _make->comment_count; index++)
    {
        const struct d_make_comment* const c = &_make->comments[index];

        // in order, and a `#` that begins it
        if ( (c->end > _make->length) ||
             (_make->text[c->start] != '#') ||
             ( (index > 0u) &&
               (_make->comments[index - 1u].end > c->start) ) )
        {
            return "comments out of order";
        }
    }

    return NULL;
}


/**
 * @brief Reads every file named on the standard input, and proves each.
 *
 * @param[in] _dialect  the make named, or `NULL` to guess each file's.
 * @return 0 when every file read was sound; 1 otherwise.
 */
static int
d_tests_corpus(
    const char* _dialect
)
{
    char   path[4096];
    size_t files   = 0;
    size_t read    = 0;
    size_t unsound = 0;

    // each path, one to a line
    while (fgets(path, (int)sizeof(path), stdin))
    {
        path[strcspn(path, "\r\n")] = '\0';

        size_t      length = 0;
        char* const text   = d_text_read(path, &length);

        // a file that cannot be read is passed over
        if (!text)
        {
            continue;
        }

        enum d_make_dialect   dialect = d_make_guess(text, length);
        struct d_make         make;
        struct d_make_problem problem;

        // the make named wins over the guess
        if (_dialect)
        {
            (void)d_make_dialect_of(_dialect, &dialect);
        }

        files++;

        // a table of the outcomes: refused, sound, unsound
        if (!d_make_read(&make, text, length, dialect, &problem))
        {
            printf("refused  %s: %s\n", path, problem.text);
        }
        else if (d_tests_sound(&make))
        {
            printf("UNSOUND  %s: %s\n", path, d_tests_sound(&make));
            unsound++;
        }
        else
        {
            read++;
        }

        d_make_release(&make);
        free(text);
    }

    printf("test_dmake --corpus: %lu files, %lu read whole and sound, "
           "%lu unsound\n",
           (unsigned long)files,
           (unsigned long)read,
           (unsigned long)unsound);

    return (unsound == 0u) ? 0 : 1;
}


//==============================================================================
// 8.  THE FUZZ
//==============================================================================


// the pieces random text is made of
static const char* const g_pieces[] =
{
    "x", "$(Y)", "a b", "#", ";", ":", "=", "\\", "\n", "", "\t@echo hi",
    "ifeq (a,b)", "endif", "else", "define Z", "endef", ".if 1", ".endif",
    ".for i in a", ".endfor", "!IF 1", "!ENDIF", "X = 1", "all: x", "| y",
    "'", "%", "$", "$$", " ", "\t", "::", "+=", "?=", "${Z:a=b}", "-include",
    "<<", "^#", "\r\n", "&:"
};


/**
 * @brief Gives the next of a run of random numbers.
 *
 * @param[in,out] _state  the run's state.
 * @param[in]     _below  the bound.
 * @return a number below the bound, or 0 for none.
 */
static size_t
d_tests_random(
    unsigned long* _state,
    size_t         _below
)
{
    *_state = (*_state * 6364136223846793005UL) + 1442695040888963407UL;

    return (_below > 0u) ? (size_t)((*_state >> 33) % _below) : 0u;
}


/**
 * @brief Makes a random text of one to three pieces.
 *
 * @param[in,out] _state  the random run.
 * @param[out]    _out    receives the text; room for 128 bytes.
 */
static void
d_tests_random_text(
    unsigned long* _state,
    char*          _out
)
{
    const size_t count = 1u + d_tests_random(_state, 3u);
    const size_t pieces = sizeof(g_pieces) / sizeof(g_pieces[0]);

    _out[0] = '\0';

    // each piece
    for (size_t at = 0; at < count; at++)
    {
        (void)strcat(_out, g_pieces[d_tests_random(_state, pieces)]);
    }

    return;
}


/**
 * @brief Makes one random change.
 *
 * @param[in,out] _doc    the document.
 * @param[in,out] _state  the random run.
 * @return whether the change was kept.
 *
 * Long as a table of the changes, each a line or two.
 */
static bool
d_tests_random_change(
    struct d_make_doc* _doc,
    unsigned long*     _state
)
{
    const struct d_make* const make  = d_make_doc_records(_doc);
    const size_t               count = make->statement_count;
    const size_t               n     = d_tests_random(_state, count + 1u);
    const size_t               k     = d_tests_random(_state, 4u);
    const enum d_make_part     part  = (enum d_make_part)d_tests_random(
                                           _state, 6u);
    static const enum d_make_part lists[] =
    {
        D_MAKE_PART_TEXT, D_MAKE_PART_TARGETS, D_MAKE_PART_ORDER_ONLY
    };
    const enum d_make_part     words = lists[d_tests_random(_state, 3u)];
    size_t                     length = 0;
    char                       text[128];

    (void)d_make_doc_text(_doc, &length);

    const size_t               start = d_tests_random(_state, length + 1u);
    const size_t               end   = start + d_tests_random(
                                           _state,
                                           ((length - start) < 12u)
                                               ? (length - start + 1u)
                                               : 12u);

    d_tests_random_text(_state, text);

    // a table of the changes
    switch (d_tests_random(_state, 10u))
    {
        case 0:
            return d_make_doc_set(_doc, n, part, text);
        case 1:
            return d_make_doc_set_word(_doc, n, words, k, text);
        case 2:
            return d_make_doc_insert_word(_doc, n, words, k, text);
        case 3:
            return d_make_doc_remove_word(_doc, n, words, k);
        case 4:
            return d_make_doc_add_command(_doc, n, k, text);
        case 5:
            return d_make_doc_remove_command(_doc, n);
        case 6:
            return d_make_doc_insert_statements(_doc, n, text);
        case 7:
            return d_make_doc_set_statement(_doc, n, text);
        case 8:
            return d_make_doc_remove_statements(_doc, n, n + k);
        default:
            return d_make_doc_splice(_doc, start, end, text, strlen(text));
    }
}


/**
 * @brief Proves a change kept sound, apart from the document's check.
 *
 * @param[in] _old      the records of the text before it.
 * @param[in] _new      the records of a fresh reading of the text after.
 * @param[in] _splice   where the change was: start, old end, new end.
 * @return why it is not sound, or `NULL`.
 *
 * Long as each record outside the change is compared, a loop for the
 * statements and one for the comments.
 */
static const char*
d_tests_kept_sound(
    const struct d_make* _old,
    const struct d_make* _new,
    const size_t*        _splice
)
{
    const ptrdiff_t shift = (ptrdiff_t)_splice[2] - (ptrdiff_t)_splice[1];

    // the bytes outside the change
    if ( (memcmp(_old->text, _new->text, _splice[0]) != 0) ||
         (_old->length - _splice[1] != _new->length - _splice[2]) ||
         (memcmp(_old->text + _splice[1], _new->text + _splice[2],
                 _old->length - _splice[1]) != 0) )
    {
        return "a byte outside the change moved";
    }

    size_t before = 0;
    size_t after  = 0;

    // the statements wholly before the change, as they were
    while ( (before < _old->statement_count) &&
            (_old->statements[before].end <= _splice[0]) )
    {
        const struct d_make_statement* const a = &_old->statements[before];
        const struct d_make_statement* const b = &_new->statements[before];

        // the same kind, place and parts
        if ( (before >= _new->statement_count) ||
             (a->kind != b->kind) ||
             (a->start != b->start) ||
             (a->end != b->end) ||
             (a->text.start != b->text.start) ||
             (a->text.end != b->text.end) ||
             (a->orphan != b->orphan) ||
             (a->depth != b->depth) )
        {
            return "a statement before the change changed";
        }

        before++;
    }

    // the statements wholly after it, moved
    while ( (after < _old->statement_count) &&
            (after < _new->statement_count) &&
            (_old->statements[_old->statement_count - 1u - after].start >=
             _splice[1]) )
    {
        const struct d_make_statement* const a =
            &_old->statements[_old->statement_count - 1u - after];
        const struct d_make_statement* const b =
            &_new->statements[_new->statement_count - 1u - after];

        // the same kind, place and parts, moved
        if ( (a->kind != b->kind) ||
             ((ptrdiff_t)a->start + shift != (ptrdiff_t)b->start) ||
             ((ptrdiff_t)a->end + shift != (ptrdiff_t)b->end) ||
             ((ptrdiff_t)a->text.start + shift != (ptrdiff_t)b->text.start) ||
             ((ptrdiff_t)a->text.end + shift != (ptrdiff_t)b->text.end) ||
             (a->orphan != b->orphan) ||
             (a->depth != b->depth) )
        {
            return "a statement after the change changed";
        }

        after++;
    }

    // as many after in both
    if ( (after < _old->statement_count - before) &&
         (_old->statements[_old->statement_count - 1u - after].start >=
          _splice[1]) )
    {
        return "a statement after the change went";
    }

    return NULL;
}


/**
 * @brief Changes every file named on the standard input at random, and
 *        proves each change.
 *
 * @param[in] _seed   the random run's seed.
 * @param[in] _count  the changes to each file.
 * @return 0 when every change was sound; 1 otherwise.
 *
 * Long as the run over files and over changes, each step a few lines.
 */
static int
d_tests_fuzz(
    unsigned long _seed,
    size_t        _count
)
{
    char          path[4096];
    unsigned long state  = _seed;
    size_t        kept   = 0;
    size_t        refused = 0;
    size_t        failed = 0;

    // each path, one to a line
    while (fgets(path, (int)sizeof(path), stdin))
    {
        path[strcspn(path, "\r\n")] = '\0';

        size_t      length = 0;
        char* const text   = d_text_read(path, &length);

        // a file that cannot be read is passed over
        if (!text)
        {
            continue;
        }

        struct d_make_doc* const doc = d_make_doc_make(
                                           path, text, length,
                                           d_make_guess(text, length), NULL);

        free(text);

        // each change, while the file reads
        for (size_t at = 0; (doc) && (at < _count); at++)
        {
            size_t            before_length = 0;
            const char* const now           = d_make_doc_text(doc,
                                                              &before_length);
            char* const       before        = malloc(before_length + 1u);
            struct d_make     old;
            struct d_make     fresh;
            size_t            splice[3]     = { 0u, 0u, 0u };
            const char*       why           = NULL;

            memcpy(before, now, before_length + 1u);
            (void)d_make_read(&old, before, before_length,
                              d_make_doc_records(doc)->dialect, NULL);

            const bool  kept_one = d_tests_random_change(doc, &state);
            size_t      after_length = 0;
            const char* after   = d_make_doc_text(doc, &after_length);

            // a table of the two: a change kept, proved; one refused, left
            if (kept_one)
            {
                kept++;
                (void)d_make_doc_changed(doc, &splice[0], &splice[1],
                                         &splice[2]);
                why = (!d_make_read(&fresh, after, after_length,
                                    d_make_doc_records(doc)->dialect, NULL))
                          ? "the text kept does not read"
                          : d_tests_kept_sound(&old, &fresh, splice);
                d_make_release(&fresh);
            }
            else
            {
                refused++;
                why = ( (after_length != before_length) ||
                        (memcmp(after, before, before_length) != 0) )
                          ? "a change refused left the text changed"
                          : NULL;
            }

            // a change not sound
            if (why)
            {
                failed++;
                printf("FAIL  %s, change %lu: %s\n", path,
                       (unsigned long)at, why);
            }

            d_make_release(&old);
            free(before);
        }

        d_make_doc_free(doc);
    }

    printf("test_dmake --fuzz: %lu kept, %lu refused, %lu failures\n",
           (unsigned long)kept,
           (unsigned long)refused,
           (unsigned long)failed);

    return (failed == 0u) ? 0 : 1;
}


//==============================================================================
// 9.  MAIN
//==============================================================================


int
main(
    int    _argc,
    char** _argv
)
{
    // the corpus, instead of the units
    if ( (_argc > 1) &&
         (strcmp(_argv[1], "--corpus") == 0) )
    {
        return d_tests_corpus((_argc > 2) ? _argv[2] : NULL);
    }

    // the fuzz, instead of the units
    if ( (_argc > 1) &&
         (strcmp(_argv[1], "--fuzz") == 0) )
    {
        return d_tests_fuzz((_argc > 2) ? strtoul(_argv[2], NULL, 10) : 1u,
                            (_argc > 3) ? strtoul(_argv[3], NULL, 10) : 50u);
    }

    d_tests_lines_endings();
    d_tests_lines_continued();
    d_tests_dialects();
    d_tests_gnu_assignments();
    d_tests_gnu_rules();
    d_tests_gnu_directives();
    d_tests_posix();
    d_tests_bsd();
    d_tests_nmake();
    d_tests_doc_parts();
    d_tests_doc_words();
    d_tests_doc_statements();
    d_tests_doc_save((_argc > 1) ? _argv[1] : ".");
    printf("test_dmake: %d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
