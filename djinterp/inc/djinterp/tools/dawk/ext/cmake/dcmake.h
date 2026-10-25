/*******************************************************************************
* djinterp [tools]                                                      dcmake.h
*
* The CMake language, read without loss.
*   Enough of cmake-language(7) to say what a CMake file holds and to change
* it in place: every command invocation, with its name and its arguments --
* unquoted, quoted and bracket, the legacy unquoted forms included -- every
* comment, line or bracket, and the blocks the commands make: if, foreach,
* while, function, macro and block, each paired with the command that ends it.
* Every record keeps its place in the buffer, so the bytes between two records
* are the file's own and an edit is a splice that leaves the rest of the file
* as it was.
*   Lines end in LF or CR LF; a lone CR is space. Columns count bytes from 1,
* as every other extension of the checker counts them.
*   Not done: evaluation. A variable reference and a generator expression
* are read as the text they are written as, and an unquoted argument is one
* argument even where CMake would divide it at a `;`; d_cmake_passed and
* d_cmake_list_items divide it as CMake does.
*   The writers make the text of an argument from a value, in the form asked
* for or the plainest that reads back as the value.
*   A stand-in, local to dawk: CMake belongs to the parse tier, and when that
* tier reads it, this unit gives way to it.
*
* path:      /inc/djinterp/tools/dawk/ext/cmake/dcmake.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_CMAKE_NONE
    2.  Kinds
         1.  d_cmake_argument_kind
         2.  d_cmake_block
         3.  d_cmake_role
    3.  Records
         1.  d_cmake_argument
         2.  d_cmake_command
         3.  d_cmake_comment
         4.  d_cmake
2.  OPERATIONS
    ----------
    1.  Reading
    2.  Names
    3.  Values
    4.  Lists
    5.  Writing
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_CMAKE_DCMAKE_H
#define DJINTERP_TOOLS_DAWK_EXT_CMAKE_DCMAKE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_CMAKE_NONE
//   constant: the index of a record that is not there -- a command no block
//   pairs, a comment outside every command.
#define D_CMAKE_NONE ((size_t)-1)

// 1.2    Kinds
//------------------------------------------------------------------------------
// 1.2.1
// d_cmake_argument_kind
//   enum: how an argument is written.  A parenthesis nested in an argument
//   list is an unquoted argument of its own, as CMake passes it.
enum d_cmake_argument_kind
{
    D_CMAKE_UNQUOTED = 0,  // a, ${x}, -Da="b c", (
    D_CMAKE_QUOTED,        // "a b"
    D_CMAKE_BRACKET        // [[a b]], [==[a]] b]==]
};

// 1.2.2
// d_cmake_block
//   enum: the blocks commands open and close.
enum d_cmake_block
{
    D_CMAKE_BLOCK_NONE = 0,
    D_CMAKE_BLOCK_IF,        // if, elseif, else, endif
    D_CMAKE_BLOCK_FOREACH,   // foreach, endforeach
    D_CMAKE_BLOCK_WHILE,     // while, endwhile
    D_CMAKE_BLOCK_FUNCTION,  // function, endfunction
    D_CMAKE_BLOCK_MACRO,     // macro, endmacro
    D_CMAKE_BLOCK_BLOCK      // block, endblock
};

// 1.2.3
// d_cmake_role
//   enum: what a command does to the block it belongs to.
enum d_cmake_role
{
    D_CMAKE_ROLE_NONE = 0,  // an ordinary command
    D_CMAKE_ROLE_OPEN,      // if, foreach, ...
    D_CMAKE_ROLE_BRANCH,    // elseif, else
    D_CMAKE_ROLE_CLOSE      // endif, endforeach, ...
};

// 1.3    Records
//------------------------------------------------------------------------------
// Every record is a place in the buffer: `start` is its first byte and `end`
// one past its last; `line` and `column` are where it starts, from 1, and
// `end_line` is the line its last byte is on.

// 1.3.1
// d_cmake_argument
//   struct: one argument, as written: an unquoted argument's text, a quoted
//   one's quotes and all, a bracket one's brackets and all.  `paren` is `(`
//   or `)` for a nested parenthesis and `\0` otherwise; `legacy` marks an
//   unquoted argument holding a quoted run or a make variable, `$(X)`;
//   `equals` is a bracket argument's count of `=`.
struct d_cmake_argument
{
    enum d_cmake_argument_kind kind;
    size_t                     start;
    size_t                     end;
    size_t                     line;
    size_t                     column;
    size_t                     end_line;
    char                       paren;
    bool                       legacy;
    size_t                     equals;
};

// 1.3.2
// d_cmake_command
//   struct: one command invocation.  The name is `name_length` bytes from
//   `start`, `open` is its `(` and `end` is one past its `)`.  Its arguments
//   are `argument_count` records of the arguments array from
//   `first_argument`.  `depth` is how many blocks hold it, an opener and its
//   closer standing at the depth of the block around them.  `partner` is an
//   opener's closer, and a branch's or a closer's opener; `unpaired` marks
//   an opener never closed and a branch or closer with no opener, whose
//   partner is D_CMAKE_NONE.
struct d_cmake_command
{
    size_t             start;
    size_t             name_length;
    size_t             open;
    size_t             end;
    size_t             line;
    size_t             column;
    size_t             end_line;
    size_t             first_argument;
    size_t             argument_count;
    size_t             depth;
    enum d_cmake_block block;
    enum d_cmake_role  role;
    size_t             partner;
    bool               unpaired;
};

// 1.3.3
// d_cmake_comment
//   struct: one comment, its `#` included.  `within` is the command whose
//   argument list holds it, and `trailing` marks a comment that follows
//   something else on its line.
struct d_cmake_comment
{
    size_t start;
    size_t end;
    size_t line;
    size_t column;
    size_t end_line;
    bool   bracket;
    size_t within;
    bool   trailing;
};

// 1.3.4
// d_cmake
//   struct: one file read.  The text is the caller's, borrowed while the
//   records are used; the arrays are the reader's, in source order, and are
//   released with d_cmake_release.  `line_count` counts the last line even
//   when no newline ends it; `crlf` is whether the first line ending is
//   CR LF.  When reading fails, `error_line` and `error_column` say where,
//   and the records hold what came before.
struct d_cmake
{
    const char*              text;
    size_t                   length;
    struct d_cmake_command*  commands;
    size_t                   command_count;
    struct d_cmake_argument* arguments;
    size_t                   argument_count;
    struct d_cmake_comment*  comments;
    size_t                   comment_count;
    size_t                   line_count;
    bool                     crlf;
    size_t                   error_line;
    size_t                   error_column;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads a CMake file's text into records.
 *
 * @note What CMake's own reader refuses is refused here, with where: a
 *       character no element begins with, a name with no `(`, an argument
 *       list or a quoted or bracket argument the file ends inside, a
 *       backslash ending an unquoted argument, and two commands on one line.
 *       A block left open or closed twice is not refused, since that is
 *       CMake's to report when it runs the file; the commands are marked
 *       `unpaired` instead.
 *
 * @param[out] _out       receives the records; release it with
 *                        d_cmake_release, whether or not reading succeeded.
 * @param[in]  _text      the text, borrowed for as long as `_out` is used.
 * @param[in]  _length    its length in bytes.
 * @param[out] _problem   receives why reading stopped, as
 *                        "line L, column C: ...".
 * @param[in]  _capacity  the size of `_problem`.
 * @return `true` if the whole text was read; `false` otherwise.
 */
bool        d_cmake_read(struct d_cmake* _out,
                         const char*     _text,
                         size_t          _length,
                         char*           _problem,
                         size_t          _capacity);
void        d_cmake_release(struct d_cmake* _cmake);
// d_cmake_generated reports whether CMake wrote the text -- an install
// script, a CTest file, a cache -- by the marker its first line carries, or
// whether its first 4 KiB carry the tree's own `Auto-generated. Do not edit
// by hand.`
bool        d_cmake_generated(const char* _text,
                              size_t      _length);

// 2.2    Names
//------------------------------------------------------------------------------
// d_cmake_is compares a command's name without regard to case, as CMake
// does.  d_cmake_block_name names a block as its opener is spelled, "if" to
// "block", and "" for D_CMAKE_BLOCK_NONE.
bool        d_cmake_is(const struct d_cmake*         _cmake,
                       const struct d_cmake_command* _command,
                       const char*                   _name);
const char* d_cmake_block_name(enum d_cmake_block _block);

// 2.3    Values
//------------------------------------------------------------------------------
/**
 * @brief Writes an argument's value: what CMake reads it as, before any
 *        variable is expanded.
 *
 * @note A quoted argument loses its quotes and its line continuations, and a
 *       bracket argument its brackets and a newline directly after the
 *       opening one.  `\t`, `\r` and `\n` are the characters they name;
 *       `\;` stays as it is in a quoted argument and is `;` in an unquoted
 *       one, as CMake passes it; a backslash before any other character that
 *       is not a letter or a digit is dropped.  A value is never longer than
 *       the argument as written.  A CR LF inside a quoted or a bracket
 *       argument is read as LF, as CMake reads it.
 *
 * @param[in]  _cmake     the file the argument was read from.
 * @param[in]  _argument  the argument.
 * @param[out] _out       receives the value, terminated; it must hold the
 *                        argument's length in bytes and one more.
 * @return the value's length.
 */
size_t      d_cmake_value(const struct d_cmake*          _cmake,
                          const struct d_cmake_argument* _argument,
                          char*                          _out);
// d_cmake_passed writes what CMake makes the arguments it passes of an
// argument: an unquoted one's text, its escapes decoded but `\;`, for
// d_cmake_list_items to divide, empty items dropped; a quoted or a bracket
// one's value, which CMake passes whole.  `_out` holds the argument's
// length and one more; the length written is returned.
size_t      d_cmake_passed(const struct d_cmake*          _cmake,
                           const struct d_cmake_argument* _argument,
                           char*                          _out);

// 2.4    Lists
//------------------------------------------------------------------------------
/**
 * @brief Divides a list as CMake divides one.
 *
 * @note An item ends at each `;` that is neither escaped, `\;` standing for
 *       a `;` of the item's own, nor inside square brackets, which are
 *       counted as they come.  A list with no `;` is one item as it stands.
 *       An empty list has no items.
 *
 * @param[in]  _list         the list.
 * @param[in]  _length       its length in bytes.
 * @param[in]  _keep_empty   whether an empty item counts, as it does to
 *                           list(); a command's arguments drop them.
 * @param[out] _out          receives the items, each terminated; it must
 *                           hold `_length` bytes and one more.
 * @param[out] _out_length   receives how many bytes were written; may be
 *                           `NULL`.
 * @return how many items there are.
 */
size_t      d_cmake_list_items(const char* _list,
                               size_t      _length,
                               bool        _keep_empty,
                               char*       _out,
                               size_t*     _out_length);

// 2.5    Writing
//------------------------------------------------------------------------------
// d_cmake_is_name reports whether a text is a command's name, a letter or
// `_` and then letters, digits and `_`.
bool        d_cmake_is_name(const char* _text,
                            size_t      _length);
/**
 * @brief Writes a value as an argument that reads back as the value.
 *
 * @note An unquoted argument is written only of a value that needs no
 *       escape: no space, none of `()#"\;`, no control character, and no
 *       `[` or `=` first.  A quoted one escapes `\`, `"` and a CR.  A bracket
 *       one takes as many `=` as keep its value from closing it, and cannot
 *       hold a CR LF.
 *
 * @param[in]  _value       the value.
 * @param[in]  _length      its length in bytes.
 * @param[in]  _kind        the argument's kind.
 * @param[out] _out_length  receives the text's length; may be `NULL`.
 * @post   The caller frees the text.
 * @return the text, terminated, or `NULL` when the value cannot be written
 *         so, or memory runs out.
 */
char*       d_cmake_write(const char*                _value,
                          size_t                     _length,
                          enum d_cmake_argument_kind _kind,
                          size_t*                    _out_length);
// d_cmake_write_any writes a value unquoted when it can be, and quoted
// otherwise; the caller frees it, and it is `NULL` only when memory runs
// out.
char*       d_cmake_write_any(const char* _value,
                              size_t      _length,
                              size_t*     _out_length);


#endif  // DJINTERP_TOOLS_DAWK_EXT_CMAKE_DCMAKE_H
