/*******************************************************************************
* djinterp [c]                                                     text_lineup.h
*
*   Text lineup: tokens laid out in aligned columns across wrapped lines, with
* a separator between them, a prefix before the first line and a suffix after
* the last, and a prefix and suffix on each line -- the shape of a long
* initializer list, an enum's members, or a macro's continued lines. Every
* token takes a slot as wide as the widest (or the template's minimum); a
* short token is padded right-aligned by default, or left-aligned or centred
* by flag. A line takes as many slots as its width allows, and the first line
* is padded so its slots line up with the lines below.
*
*   A dj_string_array is the token list: a growable array of owned strings,
* with a generator for numbered series ("item01", "item02", ...).
*
*   THE OUTPUT IS ALL OR NOTHING. d_text_lineup appends to a dj_text_buffer
* (c/container/buffer/text_buffer.h); a failure part way removes what it
* appended, so the buffer is as it was.
*
*   Restored on 2026.11.09 from the owner's copy of 2025.12.17, which had been
* lost from the tree, and ported to it: the buffer is the container module's
* dj_text_buffer, the type tags take dj_ (stream 3), and the template and its
* flags are named for the lineup, since text_template.h's dj_text_template is
* another type.
*
*
* path:      /inc/djinterp/c/text/text_lineup.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.17
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STRING ARRAYS
    -------------
    1.  dj_string_array
    2.  Making and freeing
    3.  Growing
    4.  Numbered series
2.  LINEUP
    ------
    1.  Flags
    2.  Flag queries
    3.  dj_text_lineup_template
    4.  d_text_lineup
*/

#ifndef DJINTERP_C_TEXT_TEXT_LINEUP_H
#define DJINTERP_C_TEXT_TEXT_LINEUP_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"                         // framework root
#include "../container/buffer/text_buffer.h"    // dj_text_buffer
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // uint32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  STRING ARRAYS
//==============================================================================


// 1.1    dj_string_array
//------------------------------------------------------------------------------
// dj_string_array
//   struct: a growable array of strings it owns, with its count and capacity.
struct dj_string_array
{
    size_t count;
    size_t capacity;
    char** elements;
};


// 1.2    Making and freeing
//------------------------------------------------------------------------------
/**
 * @brief Makes an empty array with room for _capacity strings.
 *
 * @param[in] _capacity  the initial capacity; 0 for none yet.
 * @return the array, or NULL when it could not be allocated.
 */
struct dj_string_array* d_string_array_new(size_t _capacity);

/**
 * @brief Frees an array and every string in it.
 *
 * @param[in] _array  the array; NULL is fine.
 */
void d_string_array_free(struct dj_string_array* _array);


// 1.3    Growing
//------------------------------------------------------------------------------
/**
 * @brief Makes room for at least _min_capacity strings, doubling.
 *
 * @return true when the room is there; false for a NULL array, a size that
 *         does not fit, or a failed allocation (the array is unchanged).
 */
bool d_string_array_reserve(struct dj_string_array* _array,
                            size_t                  _min_capacity);

/**
 * @brief Appends a copy of _string.
 *
 * @return true on success; false for a NULL argument or a failed
 *         allocation (the array is unchanged).
 */
bool d_string_array_append(struct dj_string_array* _array,
                           const char*             _string);


// 1.4    Numbered series
//------------------------------------------------------------------------------
/**
 * @brief Makes an array of numbered strings: _prefix, the number, _suffix,
 *        for each number from _start to _end by _step.
 *
 * @param[in] _start                the first number.
 * @param[in] _end                  the last number, inclusive when the steps
 *                                  reach it.
 * @param[in] _step                 the step; not 0, and toward _end.
 * @param[in] _prefix               before each number; NULL for none.
 * @param[in] _suffix               after each number; NULL for none.
 * @param[in] _zero_padding         the least width of a number that is not
 *                                  negative, padded with zeros; 0 for none.
 * @param[in] _additional_capacity  room for this many more strings.
 * @return the array, or NULL for a step of 0, a step away from _end, or a
 *         failed allocation.
 */
struct dj_string_array* d_string_array_generate_series_capacity(
    int         _start,
    int         _end,
    int         _step,
    const char* _prefix,
    const char* _suffix,
    size_t      _zero_padding,
    size_t      _additional_capacity);

/**
 * @brief d_string_array_generate_series_capacity with no extra room.
 */
struct dj_string_array* d_string_array_generate_series(int         _start,
                                                        int         _end,
                                                        int         _step,
                                                        const char* _prefix,
                                                        const char* _suffix,
                                                        size_t _zero_padding);


//==============================================================================
// 2.  LINEUP
//==============================================================================


// 2.1    Flags
//------------------------------------------------------------------------------
// dj_text_lineup_flag
//   enum: flags for a dj_text_lineup_template's flags. Five are accepted and
// not yet read: a non-first line always takes the line prefix
// (LINE_PREFIX_ALL_LINES), the first row is always aligned to the slots
// (ALIGN_FIRST_ROW_TO_SLOTS), and LINE_SUFFIX_ALL_LINES, ALIGN_SINGLE_ROW and
// LINE_SUFFIX_ON_SINGLE_LINE change nothing yet.
enum dj_text_lineup_flag
{
    D_TEXT_LINEUP_FLAG_NONE                       = 0x0000,
    D_TEXT_LINEUP_FLAG_LINE_PREFIX_ALL_LINES      = 0x0001,
    D_TEXT_LINEUP_FLAG_LINE_SUFFIX_ALL_LINES      = 0x0002,
    D_TEXT_LINEUP_FLAG_ALLOW_OVERSIZE_PREFIX      = 0x0004,
    D_TEXT_LINEUP_FLAG_ALLOW_OVERSIZE_SUFFIX      = 0x0008,
    D_TEXT_LINEUP_FLAG_ALIGN_SINGLE_ROW           = 0x0010,
    D_TEXT_LINEUP_FLAG_LINE_SUFFIX_ON_SINGLE_LINE = 0x0020,
    D_TEXT_LINEUP_FLAG_LINE_SUFFIX_AFTER_SUFFIX   = 0x0040,
    D_TEXT_LINEUP_FLAG_RIGHT_ALIGN_LINE_SUFFIX    = 0x0080,
    D_TEXT_LINEUP_FLAG_TOKEN_ALIGN_LEFT           = 0x0100,
    D_TEXT_LINEUP_FLAG_TOKEN_ALIGN_CENTER         = 0x0200,
    D_TEXT_LINEUP_FLAG_PAD_AFTER_SEPARATOR        = 0x0400,
    D_TEXT_LINEUP_FLAG_ALIGN_FIRST_ROW_TO_SLOTS   = 0x0800
};


// 2.2    Flag queries
//------------------------------------------------------------------------------
// D_TEXT_LINEUP_HAS
//   macro: whether the flags _value have _flag set.
#define D_TEXT_LINEUP_HAS(_value, _flag)                                       \
    ( ((uint32_t)(_value) & (uint32_t)(_flag)) != 0u )

// D_TEXT_LINEUP_ALLOWS_OVERSIZE_PREFIX ... D_TEXT_LINEUP_PADS_AFTER_SEPARATOR
//   macro: the flags d_text_lineup reads.
#define D_TEXT_LINEUP_ALLOWS_OVERSIZE_PREFIX(_v)                               \
    D_TEXT_LINEUP_HAS(_v, D_TEXT_LINEUP_FLAG_ALLOW_OVERSIZE_PREFIX)
#define D_TEXT_LINEUP_ALLOWS_OVERSIZE_SUFFIX(_v)                               \
    D_TEXT_LINEUP_HAS(_v, D_TEXT_LINEUP_FLAG_ALLOW_OVERSIZE_SUFFIX)
#define D_TEXT_LINEUP_LINE_SUFFIX_AFTER_SUFFIX(_v)                             \
    D_TEXT_LINEUP_HAS(_v, D_TEXT_LINEUP_FLAG_LINE_SUFFIX_AFTER_SUFFIX)
#define D_TEXT_LINEUP_RIGHT_ALIGNS_LINE_SUFFIX(_v)                             \
    D_TEXT_LINEUP_HAS(_v, D_TEXT_LINEUP_FLAG_RIGHT_ALIGN_LINE_SUFFIX)
#define D_TEXT_LINEUP_ALIGNS_TOKEN_LEFT(_v)                                    \
    D_TEXT_LINEUP_HAS(_v, D_TEXT_LINEUP_FLAG_TOKEN_ALIGN_LEFT)
#define D_TEXT_LINEUP_ALIGNS_TOKEN_CENTER(_v)                                  \
    D_TEXT_LINEUP_HAS(_v, D_TEXT_LINEUP_FLAG_TOKEN_ALIGN_CENTER)
#define D_TEXT_LINEUP_PADS_AFTER_SEPARATOR(_v)                                 \
    D_TEXT_LINEUP_HAS(_v, D_TEXT_LINEUP_FLAG_PAD_AFTER_SEPARATOR)


// 2.3    dj_text_lineup_template
//------------------------------------------------------------------------------
// dj_text_lineup_template
//   struct: what to lay out and how. The caller owns every pointer; the
// lineup only reads them.
struct dj_text_lineup_template
{
    struct dj_string_array* tokens;           // the tokens, in order
    char*                   separator;        // between tokens; NULL for none
    bool                    separator_before; // a separator before the first
    bool                    separator_after;  // a separator after the last
    char*                   line_prefix;      // before each line after the
                                              // first; NULL for none
    char*                   line_suffix;      // after each line but the last;
                                              // NULL for none
    size_t                  min_token_width;  // the least slot width
    uint32_t                flags;            // dj_text_lineup_flag values
};


// 2.4    d_text_lineup
//------------------------------------------------------------------------------
/**
 * @brief Lays out a template's tokens onto the end of a text buffer.
 *
 * @param[in]     _template        the tokens and how to lay them out.
 * @param[in,out] _text_buffer     appended to; unchanged on failure.
 * @param[in]     _line_width      the widest a line may be; 0 for one line:
 *                                 the tokens joined by the separator, with
 *                                 no slots, padding, line prefix or line
 *                                 suffix (min_token_width and the alignment
 *                                 flags shape a wrapped lineup only).
 * @param[in]     _prefix          before the first token; NULL for none.
 * @param[in]     _suffix          after the last token; NULL for none.
 * @param[in]     _indent_prefix   whether the line prefix precedes _prefix
 *                                 on the first line.
 * @param[in]     _outdent_suffix  whether a right-aligned line suffix pads
 *                                 out to _line_width on lines before the
 *                                 last.
 * @return true on success; false for a NULL argument, no tokens, a _prefix
 *         or _suffix wider than the line that the flags do not allow, or a
 *         failed allocation.
 */
bool d_text_lineup(const struct dj_text_lineup_template* _template,
                   struct dj_text_buffer*                _text_buffer,
                   size_t                                _line_width,
                   const char*                           _prefix,
                   const char*                           _suffix,
                   bool                                  _indent_prefix,
                   bool                                  _outdent_suffix);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_TEXT_LINEUP_H
