/*******************************************************************************
* djinterp [parse]                                               source_reader.h
*
* Translation phases 1 and 2, and the line index every consumer needs.
*   This module is below lexing and does not depend on it.  A verifier that
* measures banner geometry wants lines and columns and no tokens at all, and
* gets them here; the scanner wants a character stream with splices already
* invisible, and gets that here too.  Keeping the two apart is why the banner
* checker need not link a lexer.
*   A splice -- a backslash immediately followed by a newline -- is removed
* by the reader rather than by a pass over a copy of the file.  A copy would
* have to carry a position map to put a diagnostic back on a physical line,
* and the map would be consulted far more often than splices actually occur.
* Instead every character read tests one byte for a backslash, which is rare,
* and the physical position is always exact because it was never left.
*   The cost is that a token spanning a splice has a byte range wider than
* its spelling.  The reader reports this so the scanner can flag it; nothing
* here rewrites the source.
*
*
* path:      /inc/djinterp/parse/source/source_reader.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  D_SOURCE_END
         2.  Reader options
    2.  Types
         1.  d_source
         2.  d_reader
2.  OPERATIONS
    ----------
    1.  Source lifetime
    2.  Line index
    3.  Reading
    4.  Position
*/

#ifndef DJINTERP_PARSE_SOURCE_SOURCE_READER_H
#define DJINTERP_PARSE_SOURCE_SOURCE_READER_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint8_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_SOURCE_END
//   constant: the value a read returns past the end.  Negative, so it cannot
// be confused with any byte; a NUL byte inside the range is a byte like any
// other and is read as zero, because a source with an embedded NUL is
// ill-formed source rather than a shorter file.
#define D_SOURCE_END (-1)

// 1.1.2
// Reader options
//   constant: the two rewritings the reader may perform, both of which are
// language-level facts passed down from a dialect rather than decided here.
#define D_READER_OPT_NONE          0x0000u
#define D_READER_OPT_TRIGRAPH      0x0001u  // translate `??=` and the rest
#define D_READER_OPT_SPLICE_SPACE  0x0002u  // allow blanks before the newline
#define D_READER_OPT_UTF8          0x0004u  // validate UTF-8, count columns
                                            // in code points

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_source
//   struct: a borrowed byte range and its name.  The bytes are not copied and
//   must outlive every reader over them.  `name` is carried only so that a
//   diagnostic can quote it and may be NULL.  `id` is the `source` a
//   d_parse_span reports, zero for the primary input; a caller reading several
//   sources numbers them itself after d_source_init.
struct d_source
{
    const char*  text;
    size_t       length;
    const char*  name;
    uint32_t     id;
};

// 1.2.2
// d_reader
//   struct: a position in a source, with splices and trigraphs already
//   resolved.  Copying a reader by assignment is well defined and is how
//   lookahead is implemented: save, read ahead, restore.  There is no hidden
//   state and nothing to release.
//     `spliced` accumulates rather than reporting the last read, and is
//   cleared by its consumer -- the scanner clears it once per token -- so that
//   the question "did a splice lie anywhere in this token" is answerable
//   without inspecting every character of it.  `malformed` and
//   `malformed_offset` accumulate the same way under D_READER_OPT_UTF8: the
//   first ill-formed UTF-8 byte read since the consumer last cleared them.
//     Under D_READER_OPT_UTF8 the column counts code points, which is how the
//   style guide measures a line; without it, bytes.  `pending` is how many
//   continuation bytes of an already-validated sequence remain to be read.
struct d_reader
{
    const struct d_source*  source;
    size_t                  offset;            // physical byte offset
    uint32_t                line;              // one-based physical line
    uint32_t                column;            // one-based column
    unsigned                options;           // D_READER_OPT_* bits
    bool                    spliced;           // a splice crossed since cleared
    bool                    malformed;         // bad UTF-8 read since cleared
    uint8_t                 pending;           // continuation bytes to come
    size_t                  malformed_offset;  // the first bad byte
    uint32_t                malformed_line;    // and where it stands
    uint32_t                malformed_column;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Source lifetime
//------------------------------------------------------------------------------
// A source borrows; there is nothing to free.  d_source_init skips a UTF-8
// byte-order mark if one is present, so offset zero is the first byte of
// content rather than the first byte of the file, and reports whether it did.
void         d_source_init(struct d_source* _out_source,
                           const char*      _text,
                           size_t           _length,
                           const char*      _name);
bool         d_source_skip_bom(struct d_source* _source);

// 2.2    Line index
//------------------------------------------------------------------------------
// The line index is what a verifier uses without lexing anything.  Lines are
// physical: a spliced line is two lines here, because a style rule about line
// width is a rule about the file as written.
size_t       d_source_line_count(const struct d_source* _source);
/**
 * @brief Locates one physical line, its line ending excluded.
 *
 * @note Performs no lexing; a verifier needs no scanner linked to use it.
 *
 * @param[in]  _source      the source to read.
 * @param[in]  _line        the one-based line number.
 * @param[out] _out_text    receives a pointer into the source.
 * @param[out] _out_length  receives the line's length in bytes.
 * @return `true` if the line exists, `false` otherwise, with both outputs
 *         untouched.
 */
bool         d_source_line_at(const struct d_source* _source,
                              uint32_t               _line,
                              const char**           _out_text,
                              size_t*                _out_length);
uint32_t     d_source_line_of(const struct d_source* _source,
                              size_t                 _offset);

// 2.3    Reading
//------------------------------------------------------------------------------
/**
 * @brief Decodes one UTF-8 sequence, rejecting every ill-formed one.
 *
 * @note Well-formed means Table 3-7 of the Unicode Standard: no overlong
 *       form, no surrogate, nothing above U+10FFFF, and every continuation
 *       byte present.  C++23 makes an ill-formed UTF-8 source ill-formed;
 *       this is the test it names, and it is phase 1's, so it lives here.
 *
 * @param[in]  _text            the first byte of the sequence.
 * @param[in]  _length          bytes available from `_text`.
 * @param[out] _out_code_point  receives the code point; untouched on failure.
 * @return the sequence's length, 1 to 4, or 0 if the bytes at `_text` do not
 *         begin a well-formed sequence.
 */
size_t       d_source_utf8_decode(const char* _text,
                                  size_t      _length,
                                  uint32_t*   _out_code_point);

// d_reader_peek reports the logical character `_ahead` positions forward
// without moving, and d_reader_next reports the one at the position and moves
// past it.  Both return D_SOURCE_END past the end.  `_ahead` is counted in
// logical characters, so a splice between two of them is not a position.
void         d_reader_init(struct d_reader*       _out_reader,
                           const struct d_source* _source,
                           unsigned               _options);
int          d_reader_peek(const struct d_reader* _reader,
                           size_t                 _ahead);
int          d_reader_next(struct d_reader* _reader);
bool         d_reader_match(struct d_reader* _reader,
                            const char*      _text);
void         d_reader_seek(struct d_reader* _reader,
                           size_t           _offset);

// The physical pair ignores phase 1 and 2 entirely.  A raw string literal's
// contents are physical text -- a backslash-newline inside one is two
// characters, not a splice -- so a scanner reading a raw body must both peek
// and advance through these, never mixing a physical peek with a logical
// advance, which would consume the splice the peek had just reported.
int          d_reader_peek_physical(const struct d_reader* _reader,
                                    size_t                 _ahead);
int          d_reader_next_physical(struct d_reader* _reader);

// 2.4    Position
//------------------------------------------------------------------------------
bool         d_reader_at_end(const struct d_reader* _reader);
bool         d_reader_at_line_start(const struct d_reader* _reader);
size_t       d_reader_offset(const struct d_reader* _reader);
uint32_t     d_reader_line(const struct d_reader* _reader);
uint32_t     d_reader_column(const struct d_reader* _reader);


#endif  // DJINTERP_PARSE_SOURCE_SOURCE_READER_H
