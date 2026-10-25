/*******************************************************************************
* djinterp [tools]                                              dmake_internal.h
*
* What the Makefile reader's units share, and no other unit sees.
*   The reader's state as each dialect's reader grows the records: a
* statement or a comment added, a refusal recorded in make's words. And the
* scanning every dialect's reader does: blanks skipped, a continuation among
* them; a word's end; the first of a set of bytes outside every reference
* and every backslash's escape.
*
* path:      /src/djinterp/tools/dawk/ext/make/dmake_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Reading
         1.  d_make_reader
2.  OPERATIONS
    ----------
    1.  Records
    2.  Scanning
    3.  Dialects
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_INTERNAL_H
#define DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_INTERNAL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/ext/make/dmake.h"  // d_make


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Reading
//------------------------------------------------------------------------------
// 1.1.1
// d_make_reader
//   struct: the records being read, the room their arrays have, and where a
//   refusal is written.
struct d_make_reader
{
    struct d_make*         make;
    size_t                 statement_capacity;
    size_t                 comment_capacity;
    struct d_make_problem* problem;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Records
//------------------------------------------------------------------------------
// d_make_reader_statement adds a statement of a kind over a logical line --
// its places the line's, every part empty at the line's start, its branch
// D_MAKE_NONE -- and gives its index, or D_MAKE_NONE when memory runs out,
// the reader then failed.  An index stays good as statements are added; a
// pointer to one does not.
//   d_make_reader_comment adds a comment, `_within` the statement its line
// holds or D_MAKE_NONE; false when memory runs out, the reader then failed.
//   d_make_reader_fail records why reading stopped, at a physical line, and
// gives false.
size_t d_make_reader_statement(struct d_make_reader*     _reader,
                               enum d_make_kind          _kind,
                               const struct d_make_line* _line);
bool   d_make_reader_comment(struct d_make_reader* _reader,
                             size_t                _start,
                             size_t                _end,
                             size_t                _within);
bool   d_make_reader_fail(struct d_make_reader* _reader,
                          size_t                _line,
                          const char*           _message);

// 2.2    Scanning
//------------------------------------------------------------------------------
// Each scans from `_at` to `_end`, within one logical line.
//   d_make_skip_blanks  past the blanks -- space, tab, vertical tab, form
//                       feed, CR, and a continuation's backslash and line
//                       ending, which make reads as a space
//   d_make_trim_end     back from `_end` over the same, not before `_at`
//   d_make_word_end     past the run of bytes that are not blanks
//   d_make_find         the first byte of `_stops` that no backslash
//                       escapes -- an even run of them before it -- outside
//                       every reference, or `_end`
//   d_make_find_with    the same, `_escape` the escape: NMake's `^`
size_t d_make_skip_blanks(const char* _text,
                          size_t      _at,
                          size_t      _end);
size_t d_make_trim_end(const char* _text,
                       size_t      _at,
                       size_t      _end);
size_t d_make_word_end(const char* _text,
                       size_t      _at,
                       size_t      _end);
size_t d_make_find(const char* _text,
                   size_t      _at,
                   size_t      _end,
                   const char* _stops);
size_t d_make_find_with(const char* _text,
                        size_t      _at,
                        size_t      _end,
                        const char* _stops,
                        char        _escape);
// d_make_is_word reports whether `_start` to `_end` is a word.
bool   d_make_is_word(const char* _text,
                      size_t      _start,
                      size_t      _end,
                      const char* _word);

// 2.3    Dialects
//------------------------------------------------------------------------------
// Each reads the lines of `_reader->make` into its statements and comments,
// as its make reads them; false, the reader failed, when reading stops.
bool   d_make_read_gnu(struct d_make_reader* _reader);
bool   d_make_read_posix(struct d_make_reader* _reader);
bool   d_make_read_bsd(struct d_make_reader* _reader);
bool   d_make_read_nmake(struct d_make_reader* _reader);


#endif  // DJINTERP_TOOLS_DAWK_EXT_MAKE_DMAKE_INTERNAL_H
