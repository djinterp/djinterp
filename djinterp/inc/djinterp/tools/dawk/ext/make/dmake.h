/*******************************************************************************
* djinterp [tools]                                                       dmake.h
*
* Makefiles, read without loss, as each make reads them.
*   Four makes are read -- GNU make, POSIX make as pdpmake reads it, BSD make
* (bmake) and Microsoft's NMake -- the make chosen for each file: named by
* the caller, or guessed from what the file holds.
*   A file is read into its logical lines, each its physical lines joined
* where its make joins them; then into statements -- assignments, `define`
* blocks, rules, the lines of their recipes, conditionals, includes, the
* other directives -- each with the places of its parts; and comments. A
* physical line ends in LF or CR LF, the CR no part of it, as all four read
* it; a lone CR is a byte like any other. A line that ends in a backslash
* continues on the next: in GNU make and bmake when the backslashes that end
* it are odd in number, so `\\` ends a line with a backslash; in POSIX make,
* as pdpmake reads it, whatever their number; in NMake unless a caret
* escapes the backslash, `^\`, as Microsoft's documentation says. Every
* record keeps its place in the buffer, so the bytes between two records are
* the file's own and an edit is a splice that leaves the rest as it was.
*   Make reads a file as it evaluates it: a conditional's false branch is
* skipped, a line may be a rule only once its references are expanded, and
* whether a line led by the recipe prefix is a recipe's depends on whether a
* rule is live above it. The reader reads every branch, a line by what it
* holds before expansion, and a recipe's line by the statements above it;
* where make's reading turns on what evaluating gives, dmake_eval decides
* as make does. So the reader refuses only what make refuses whatever the
* file's variables hold: a conditional that does not close or that closes
* nothing, a second `else`, a `define` no `endef` ends -- in make's words,
* at the line make names. A `define` in a conditional's branch is read as
* make reads one it evaluates, its `define`s nested; in a branch make skips,
* make nests none, and an `endef` led by the prefix ends one there too.
*   GNU make is read as 4.3 reads it: `:::=` is not an operator there.
*
* path:      /inc/djinterp/tools/dawk/ext/make/dmake.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_MAKE_NONE
    2.  Kinds
         1.  d_make_dialect
         2.  d_make_kind
    3.  Records
         1.  d_make_line
         2.  d_make_lines
         3.  d_make_span
         4.  d_make_statement
         5.  d_make_comment
         6.  d_make
         7.  d_make_problem
2.  OPERATIONS
    ----------
    1.  Dialects
    2.  Lines
    3.  Reading
    4.  Names and references
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_H
#define DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_MAKE_NONE
//   constant: the index of a record that is not there -- a comment outside
//   every statement, the branch after a conditional's last.
#define D_MAKE_NONE ((size_t)-1)

// 1.2    Kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_make_dialect
//   enum: the make a file is read as.
enum d_make_dialect
{
    D_MAKE_GNU = 0,  // GNU make 4.3
    D_MAKE_POSIX,    // POSIX.1-2024 make, as pdpmake reads it
    D_MAKE_BSD,      // BSD make, as bmake reads it
    D_MAKE_NMAKE     // Microsoft's NMake
};

// 1.2.2
// d_make_kind
//   enum: what a statement is.
enum d_make_kind
{
    D_MAKE_ASSIGNMENT = 0,  // name op value, perhaps for targets
    D_MAKE_DEFINE,          // define name [op], its lines, endef
    D_MAKE_RULE,            // targets: prerequisites
    D_MAKE_RECIPE,          // a line of a rule's recipe
    D_MAKE_CONDITIONAL,     // ifeq, else, endif and their kin
    D_MAKE_INCLUDE,         // include and its kin
    D_MAKE_DIRECTIVE,       // the other directives
    D_MAKE_EXPANSION,       // a line only its expansion gives a meaning
    D_MAKE_LOOP             // BSD's .for, its lines, .endfor
};

// 1.3    Records
//------------------------------------------------------------------------------
// Every record is a place in the buffer: `start` is its first byte and `end`
// one past its last; `line` is the line it starts on, from 1.

// 1.3.1
// d_make_line
//   struct: one logical line: from its first byte to the end of its last
//   physical line, that line's ending no part of it, and the continuations
//   between, each a backslash and a line ending, all part of it.  `next` is
//   past the last line ending, where the next line starts; `end_line` is
//   the physical line it ends on.
struct d_make_line
{
    size_t start;
    size_t end;
    size_t next;
    size_t line;
    size_t end_line;
};

// 1.3.2
// d_make_lines
//   struct: a file's logical lines, in order.  The text is the caller's,
//   borrowed while the lines are used; the array is the reader's, released
//   with d_make_lines_release.  The last line counts though no newline ends
//   it, and an empty text has none; `crlf` is whether the first line ending
//   is CR LF.
struct d_make_lines
{
    const char*         text;
    size_t              length;
    enum d_make_dialect dialect;
    struct d_make_line* lines;
    size_t              count;
    bool                crlf;
};

// 1.3.3
// d_make_span
//   struct: a part of a statement, `start` to `end`.  A part a statement
//   does not have is empty, and stands where it would be written.
struct d_make_span
{
    size_t start;
    size_t end;
};

// 1.3.4
// d_make_statement
//   struct: one statement, from its first byte past its last line ending.
//   Its parts, by kind:
//     modifiers  an assignment's, a define's or an undefine's words before
//                its name: override, export, private
//     word       a directive's word, as written: ifeq, -include, define;
//                a recipe line's prefix and the run of `@`, `-`, `+` and
//                blanks after it
//     targets    a rule's targets, or the targets of an assignment made for
//                them; `colon` their separator: `:`, `::`, `&:`, `&::`
//     pattern    a static pattern rule's target pattern
//     name       the variable an assignment, a define or an undefine names
//     separator  an assignment's operator; a define's, when it has one
//     text       an assignment's value, its trailing blanks not part of
//                it, though GNU and POSIX make keep them in the value, as
//                dmake_eval does; a rule's prerequisites, to its `|`; a
//                recipe line's command; a directive's argument, its
//                trailing blanks not part of it; an expansion's line; a
//                loop's values, after its `in`
//     order_only a rule's prerequisites after its `|`
//     recipe     a rule's recipe after its `;`
//     body       a define's or a loop's lines, from the first after its
//                own to the line that closes it; an NMake recipe line's
//                inline files, from the line after it to the last `<<`
//                line
//   A loop's variables are its `name`, and its `in` its `separator`.
//   A conditional's `depth` is the count of conditionals open about it, from
//   0, and `next_branch` the index of the conditional that continues it --
//   the `else` after an `if`, the `endif` after an `else` -- D_MAKE_NONE for
//   an `endif`.  `orphan` marks a recipe's line with no rule above it, which
//   make refuses where no rule is live.
struct d_make_statement
{
    enum d_make_kind   kind;
    size_t             start;
    size_t             end;
    size_t             line;
    size_t             end_line;
    struct d_make_span modifiers;
    struct d_make_span word;
    struct d_make_span targets;
    struct d_make_span colon;
    struct d_make_span pattern;
    struct d_make_span name;
    struct d_make_span separator;
    struct d_make_span text;
    struct d_make_span order_only;
    struct d_make_span recipe;
    struct d_make_span body;
    size_t             depth;
    size_t             next_branch;
    bool               orphan;
};

// 1.3.5
// d_make_comment
//   struct: one comment, its `#` to the end of its logical line, its
//   continuations among it.  `within` is the statement whose line it ends,
//   or D_MAKE_NONE for a line that holds only the comment.
struct d_make_comment
{
    size_t start;
    size_t end;
    size_t line;
    size_t within;
};

// 1.3.6
// d_make
//   struct: one file read.  The text is the caller's, borrowed while the
//   records are used; the arrays are the reader's, in source order, and are
//   released with d_make_release.  When reading fails, `error_line` and
//   `error` say where and why, in make's words, and the records hold what
//   was read before.
struct d_make
{
    const char*              text;
    size_t                   length;
    enum d_make_dialect      dialect;
    struct d_make_lines      lines;
    struct d_make_statement* statements;
    size_t                   statement_count;
    struct d_make_comment*   comments;
    size_t                   comment_count;
    size_t                   error_line;
    char                     error[192];
};

// 1.3.7
// d_make_problem
//   struct: why a file could not be read, or a change made or a file
//   saved: "line L: ..." in its make's words, or the system's.
struct d_make_problem
{
    char text[256];
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Dialects
//------------------------------------------------------------------------------
// d_make_dialect_name names a make: gnu, posix, bsd, nmake; d_make_dialect_of
// finds a make by that name, giving false for a name that is none.
//   d_make_guess guesses a file's make from the first of its lines, outside
// a recipe, that only one make reads: a directive of BSD's, `.include` or
// `.if` and the rest, gives BSD; one of NMake's, `!include` or `!IF` and
// the rest, NMake; one of GNU's -- `define`, `ifeq`, `ifdef` and the rest --
// or a call of one of its functions, `$(call ...`, GNU. A file with none of
// these is GNU's when an assignment by `:=` or a pattern rule suggests it,
// as POSIX mode refuses both, and POSIX's otherwise.
const char*         d_make_dialect_name(enum d_make_dialect _dialect);
bool                d_make_dialect_of(const char*          _name,
                                      enum d_make_dialect* _out);
enum d_make_dialect d_make_guess(const char* _text,
                                 size_t      _length);

// 2.2    Lines
//------------------------------------------------------------------------------
/**
 * @brief Reads a file's text into its logical lines.
 *
 * @param[out] _out      receives the lines; release them with
 *                       d_make_lines_release, whether or not reading
 *                       succeeded.
 * @param[in]  _text     the text, borrowed for as long as `_out` is used.
 * @param[in]  _length   its length in bytes.
 * @param[in]  _dialect  the make whose joining of lines is followed.
 * @return `true` if the lines were read; `false` when memory runs out.
 */
bool                d_make_lines_read(struct d_make_lines* _out,
                                      const char*          _text,
                                      size_t               _length,
                                      enum d_make_dialect  _dialect);
void                d_make_lines_release(struct d_make_lines* _lines);
// d_make_continues reports whether a physical line, from `_start` to
// `_end`, its ending no part of it, continues on the next in a make.
bool                d_make_continues(const char*         _text,
                                     size_t              _start,
                                     size_t              _end,
                                     enum d_make_dialect _dialect);

// 2.3    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads a makefile's text into records.
 *
 * @note What make refuses of the file whatever its variables hold is
 *       refused, as dmake.h's banner says, at the line make names.
 *
 * @param[out] _out       receives the records; release them with
 *                        d_make_release, whether or not reading succeeded.
 * @param[in]  _text      the text, borrowed for as long as `_out` is used.
 * @param[in]  _length    its length in bytes.
 * @param[in]  _dialect   the make it is read as.
 * @param[out] _problem   receives why reading stopped, as "line L: ...";
 *                        may be `NULL`.
 * @return `true` if the whole text was read; `false` otherwise.
 */
bool                d_make_read(struct d_make*         _out,
                                const char*            _text,
                                size_t                 _length,
                                enum d_make_dialect    _dialect,
                                struct d_make_problem* _problem);
void                d_make_release(struct d_make* _make);
// d_make_line_of gives the physical line, from 1, that holds a place.
size_t              d_make_line_of(const struct d_make* _make,
                                   size_t               _at);

// 2.4    Names and references
//------------------------------------------------------------------------------
// d_make_kind_name names a kind: assignment, define, rule, recipe,
// conditional, include, directive, expansion, loop.
//   d_make_reference_end gives the place past a reference that starts with
// the `$` at `_at`, as make finds its end: `$(` and `${` closed by the
// bracket that matches, brackets of the same kind counted and others not,
// or the line's end, `_end`, for one never closed; `$x` and `$$` two bytes.
const char*         d_make_kind_name(enum d_make_kind _kind);
size_t              d_make_reference_end(const char* _text,
                                         size_t      _at,
                                         size_t      _end);
// d_make_words gives the words of a part, as make splits a list of
// targets or prerequisites: at blanks, a continuation among them, outside
// every reference, so `$(a b)` is one word; up to `_room` words' places go
// to `_out`, which may be `NULL`, and how many there are is returned.
size_t              d_make_words(const struct d_make* _make,
                                 struct d_make_span   _span,
                                 struct d_make_span*  _out,
                                 size_t               _room);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_H
