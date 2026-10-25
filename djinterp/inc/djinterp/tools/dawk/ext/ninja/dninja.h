/*******************************************************************************
* djinterp [tools]                                                      dninja.h
*
* The Ninja build language, read without loss.
*   A file of Ninja's -- a build.ninja, a rules.ninja, any file it includes --
* read as ninja's own lexer and parser read it: every statement, a variable,
* a rule, a build, a pool, a default, an include or a subninja, each with
* its name, its paths in their sections, and the bindings indented beneath
* it; every value as the pieces it is written in, text and variable
* references; and every comment. Every record keeps its place in the
* buffer, so the bytes between two records are the file's own and an edit
* is a splice that leaves the rest of the file as it was.
*   What ninja's parser refuses of a file on its own is refused here, with
* ninja's own words and the line ninja gives: a lexing error, a token out of
* place, a binding a rule or a pool may not have, a rule with no command. What
* takes more than the one file -- a rule not defined, a pool, an output two
* builds make -- is dninja_eval's to report, as ninja reports it.
*   Lines end in LF or CR LF; a lone CR is ninja's lexing error. `$^`, a
* newline in a value, is read only in a file that requires ninja 1.14 or
* later, as ninja reads it.
*   The writers make the text of a path or a value from what it is to hold,
* and give the paths ninja's canonical form and their shell-escaped form.
*
* path:      /inc/djinterp/tools/dawk/ext/ninja/dninja.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_NINJA_NONE
    2.  Kinds
         1.  d_ninja_kind
         2.  d_ninja_section
         3.  d_ninja_piece_kind
    3.  Records
         1.  d_ninja_piece
         2.  d_ninja_value
         3.  d_ninja_path
         4.  d_ninja_binding
         5.  d_ninja_statement
         6.  d_ninja_comment
         7.  d_ninja
2.  OPERATIONS
    ----------
    1.  Reading
    2.  Names
    3.  Values
    4.  Writing
    5.  Paths
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_H
#define DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_NINJA_NONE
//   constant: the index of a record that is not there -- a statement with no
//   value, a comment outside every statement.
#define D_NINJA_NONE ((size_t)-1)

// 1.2    Kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_ninja_kind
//   enum: what a statement is.
enum d_ninja_kind
{
    D_NINJA_VARIABLE = 0,  // name = value
    D_NINJA_RULE,          // rule name, and its bindings
    D_NINJA_BUILD,         // build outputs: rule inputs, and its bindings
    D_NINJA_POOL,          // pool name, and its depth
    D_NINJA_DEFAULT,       // default targets
    D_NINJA_INCLUDE,       // include path
    D_NINJA_SUBNINJA       // subninja path
};

// 1.2.2
// d_ninja_section
//   enum: where a path stands: in one of a build's six lists, among a
//   default's targets, or as an include's or a subninja's file.
enum d_ninja_section
{
    D_NINJA_OUTPUT = 0,       // build a ...
    D_NINJA_IMPLICIT_OUTPUT,  // build a | b ...
    D_NINJA_INPUT,            // ...: rule c
    D_NINJA_IMPLICIT,         // ...: rule c | d
    D_NINJA_ORDER_ONLY,       // ...: rule c || e
    D_NINJA_VALIDATION,       // ...: rule c |@ f
    D_NINJA_TARGET,           // default g
    D_NINJA_FILE              // include h.ninja
};

// 1.2.3
// d_ninja_piece_kind
//   enum: a piece of a value: text, its escapes and line continuations
//   among it, or a variable's reference, `$name` or `${name}`.
enum d_ninja_piece_kind
{
    D_NINJA_TEXT = 0,
    D_NINJA_REFERENCE
};

// 1.3    Records
//------------------------------------------------------------------------------
// Every record is a place in the buffer: `start` is its first byte and `end`
// one past its last; `line` is the line it starts on, from 1.

// 1.3.1
// d_ninja_piece
//   struct: a piece of a value.  A reference's name is `name_start` to
//   `name_end`; text has none.
struct d_ninja_piece
{
    enum d_ninja_piece_kind kind;
    size_t                  start;
    size_t                  end;
    size_t                  name_start;
    size_t                  name_end;
};

// 1.3.2
// d_ninja_value
//   struct: a value or a path as written, its pieces `piece_count` records
//   of the pieces array from `first_piece`.  A value's line ending is no
//   part of it; its trailing spaces are.
struct d_ninja_value
{
    size_t start;
    size_t end;
    size_t first_piece;
    size_t piece_count;
};

// 1.3.3
// d_ninja_path
//   struct: a path of a statement, the value that writes it and where it
//   stands.
struct d_ninja_path
{
    size_t               value;
    enum d_ninja_section section;
};

// 1.3.4
// d_ninja_binding
//   struct: a binding indented beneath a rule, a build or a pool: its line,
//   indentation to line ending, its name and its value.  `end_line` is the
//   line its value ends on, past any continuation.
struct d_ninja_binding
{
    size_t start;
    size_t end;
    size_t line;
    size_t end_line;
    size_t name_start;
    size_t name_end;
    size_t value;
};

// 1.3.5
// d_ninja_statement
//   struct: one statement, from its first byte past its last line ending:
//   its first line's and its bindings'.  `header_end` is past the first
//   line's ending.  `name` is a variable's, a rule's or a pool's, and a
//   build's rule, and for the rest empty, at the statement's start; `value`
//   a variable's, or an include's or a subninja's file, or D_NINJA_NONE;
//   the paths are `path_count` records of the paths array from
//   `first_path`, in the order written, an include's or a subninja's file
//   among them; the bindings likewise.  `next_line` is the line of what
//   ninja reads after it, past any comment: where ninja reports what it
//   finds wrong with the statement once it has read it whole.
struct d_ninja_statement
{
    enum d_ninja_kind kind;
    size_t            start;
    size_t            end;
    size_t            header_end;
    size_t            line;
    size_t            end_line;
    size_t            next_line;
    size_t            name_start;
    size_t            name_end;
    size_t            value;
    size_t            first_path;
    size_t            path_count;
    size_t            first_binding;
    size_t            binding_count;
};

// 1.3.6
// d_ninja_comment
//   struct: one comment line, its `#` to its line ending, which is no part
//   of it.  `within` is the statement whose bindings it stands among.
struct d_ninja_comment
{
    size_t start;
    size_t end;
    size_t line;
    size_t within;
};

// 1.3.7
// d_ninja
//   struct: one file read.  The text is the caller's, borrowed while the
//   records are used; the arrays are the reader's, in source order, and are
//   released with d_ninja_release.  `line_count` counts the last line even
//   when no newline ends it; `crlf` is whether the first line ending is
//   CR LF; `major` and `minor` are the version the file's
//   ninja_required_version asks for, 0 for none.  When reading fails,
//   `error_line` and `error` say where and why, in ninja's words, and the
//   records hold the statements before; `partial` says that the failure
//   came within a statement, which stands past the last, at
//   `statements[statement_count]`, as far as it was read -- its name, its
//   paths, its bindings -- for what ninja would have made of it before it
//   failed.
struct d_ninja
{
    const char*               text;
    size_t                    length;
    struct d_ninja_statement* statements;
    size_t                    statement_count;
    struct d_ninja_path*      paths;
    size_t                    path_count;
    struct d_ninja_binding*   bindings;
    size_t                    binding_count;
    struct d_ninja_value*     values;
    size_t                    value_count;
    struct d_ninja_piece*     pieces;
    size_t                    piece_count;
    struct d_ninja_comment*   comments;
    size_t                    comment_count;
    size_t*                   line_starts;
    size_t                    line_count;
    bool                      crlf;
    int                       major;
    int                       minor;
    bool                      partial;
    size_t                    error_line;
    char                      error[192];
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads a Ninja file's text into records.
 *
 * @note What ninja's parser refuses of the file alone is refused, as
 *       dninja.h's banner says; a refusal names the line ninja names.
 *
 * @param[out] _out       receives the records; release it with
 *                        d_ninja_release, whether or not reading succeeded.
 * @param[in]  _text      the text, borrowed for as long as `_out` is used.
 * @param[in]  _length    its length in bytes.
 * @param[out] _problem   receives why reading stopped, as "line L: ...";
 *                        may be `NULL`.
 * @param[in]  _capacity  the size of `_problem`.
 * @return `true` if the whole text was read; `false` otherwise.
 */
bool        d_ninja_read(struct d_ninja* _out,
                         const char*     _text,
                         size_t          _length,
                         char*           _problem,
                         size_t          _capacity);
void        d_ninja_release(struct d_ninja* _ninja);
// d_ninja_line_of gives the line, from 1, that holds a place in the text.
size_t      d_ninja_line_of(const struct d_ninja* _ninja,
                            size_t                _at);

// 2.2    Names
//------------------------------------------------------------------------------
// d_ninja_kind_name names a kind as its keyword spells it, "variable" for a
// variable; d_ninja_section_name names a section: output, implicit_output,
// input, implicit, order_only, validation, target, file; d_ninja_is_name
// reports whether a text is a name ninja reads, `[A-Za-z0-9_.-]+`.
const char* d_ninja_kind_name(enum d_ninja_kind _kind);
const char* d_ninja_section_name(enum d_ninja_section _section);
bool        d_ninja_is_name(const char* _text,
                            size_t      _length);
// d_ninja_is_reserved reports whether a binding's name is one a rule may
// have: command, depfile, dyndep, description, deps, generator, pool,
// restat, rspfile, rspfile_content, msvc_deps_prefix.
bool        d_ninja_is_reserved(const char* _name,
                                size_t      _length);

// 2.3    Values
//------------------------------------------------------------------------------
/**
 * @brief Writes a text piece's text, its escapes decoded and its line
 *        continuations dropped.
 *
 * @param[in]  _ninja  the file the piece was read from.
 * @param[in]  _piece  the piece, of kind D_NINJA_TEXT.
 * @param[out] _out    receives the text, terminated; it must hold the
 *                     piece's length in bytes and one more.
 * @return the text's length.
 */
size_t      d_ninja_text_of(const struct d_ninja*       _ninja,
                            const struct d_ninja_piece* _piece,
                            char*                       _out);
// d_ninja_unparse writes a value as ninja's Unparse does: its text decoded
// and each reference as `${name}`; `_out` holds the value's length in
// bytes, twice, and one more; the length written is returned.
size_t      d_ninja_unparse(const struct d_ninja*       _ninja,
                            const struct d_ninja_value* _value,
                            char*                       _out);

// 2.4    Writing
//------------------------------------------------------------------------------
/**
 * @brief Writes a text as a path, or as a value, that reads back as itself.
 *
 * @note A `$` is written `$$`; in a path a space is `$ ` and a `:` is `$:`;
 *       in a value a leading space is `$ `, which would otherwise be eaten.
 *       A newline is `$^`, written only when `_newline` allows it, for a
 *       file that requires ninja 1.14.  What ninja has no way to write is
 *       refused: a `|` or a newline in a path, a CR or a NUL anywhere.
 *
 * @param[in]  _text        the text.
 * @param[in]  _length      its length in bytes.
 * @param[in]  _path        a path, rather than a value.
 * @param[in]  _newline     whether a newline may be written `$^`.
 * @param[out] _out_length  receives the length written; may be `NULL`.
 * @post   The caller frees the text.
 * @return the text, terminated, or `NULL` when it cannot be written so or
 *         memory runs out.
 */
char*       d_ninja_escape(const char* _text,
                           size_t      _length,
                           bool        _path,
                           bool        _newline,
                           size_t*     _out_length);

// 2.5    Paths
//------------------------------------------------------------------------------
// d_ninja_canonical writes a path in ninja's canonical form, as
// CanonicalizePath makes it: `.` and empty components dropped, `..` taken
// back over the component before it where there is one, and "." for a path
// that comes to nothing; on Windows `\` separates too.  `_out` holds the
// path's length and two more; the length written is returned.
// d_ninja_shell_escape writes a path as ninja writes $in and $out into a
// command: as it stands when every byte is one a shell takes as itself,
// and quoted otherwise -- in `'`, each `'` written `'\''`, or, `_win32`,
// in `"` with backslashes before a quote doubled; the caller frees it.
size_t      d_ninja_canonical(const char* _path,
                              size_t      _length,
                              bool        _win32,
                              char*       _out);
char*       d_ninja_shell_escape(const char* _path,
                                 size_t      _length,
                                 bool        _win32,
                                 size_t*     _out_length);


#endif  // DJINTERP_TOOLS_DAWK_EXT_NINJA_DNINJA_H
