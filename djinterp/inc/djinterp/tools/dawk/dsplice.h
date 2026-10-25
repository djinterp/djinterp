/*******************************************************************************
* djinterp [tools]                                                     dsplice.h
*
* Changes to a text, kept apart from it until they are applied.
*   A change is a splice: the bytes from one offset to another of the text as
* it was read become a new text, and an insertion is a splice of no bytes.
* Changes are made in any order against the text as read, so every offset a
* caller holds stays good while it makes them; two that would overlap are
* refused, as neither could then be kept whole.  Applying them writes a new
* text in which every byte no change touched is the old text's own.
*   A file is read whole, and written whole beside its target and renamed
* over it, so a reader of the target sees the old file or the new one.
*   The hosts that edit a format -- CMake, Visual Studio -- share this; what
* makes a change sound in a format is theirs.
*
* path:      /inc/djinterp/tools/dawk/dsplice.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Changes
         1.  d_splices
         2.  d_splice_result
         3.  d_splice_fn_between
2.  OPERATIONS
    ----------
    1.  Changes
    2.  Files
*/

#ifndef DJINTERP_TOOLS_DAWK_DSPLICE_H
#define DJINTERP_TOOLS_DAWK_DSPLICE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Changes
//------------------------------------------------------------------------------
// 1.1.1
// d_splices
//   struct: the changes made to one text and not yet applied.  Layout is
//   private.
struct d_splices;

// 1.1.2
// d_splice_result
//   enum: what became of a change.
enum d_splice_result
{
    D_SPLICE_ADDED = 0,
    D_SPLICE_OVERLAPS,     // it meets a change already made
    D_SPLICE_NO_MEMORY
};

// 1.1.3
// d_splice_fn_between
//   function: asked, where a change's text meets the text before or after
//   it, for a byte to stand between the two -- the space that keeps two
//   arguments apart -- or '\0' for none.
typedef char (*d_splice_fn_between)(void* _user,
                                    char  _before,
                                    char  _after);


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Changes
//------------------------------------------------------------------------------
// Two changes overlap where both replace bytes and their spans meet, and
// where an insertion falls strictly inside a replacement.  Insertions at
// one place are applied in the order they were made, before a replacement
// starting there.
struct d_splices*    d_splices_new(void);
void                 d_splices_free(struct d_splices* _splices);
void                 d_splices_clear(struct d_splices* _splices);
size_t               d_splices_count(const struct d_splices* _splices);
enum d_splice_result d_splices_add(struct d_splices* _splices,
                                   size_t            _start,
                                   size_t            _end,
                                   const char*       _text,
                                   size_t            _length);
/**
 * @brief Applies every change to a text.
 *
 * @param[in,out] _splices     the changes; their order is settled here.
 * @param[in]     _text        the text they were made against.
 * @param[in]     _length      its length.
 * @param[in]     _between     asked at each meeting of a change's text and
 *                             the text beside it; may be `NULL`.
 * @param[in]     _user        passed to `_between`.
 * @param[out]    _out_length  receives the new text's length.
 * @post   The caller frees the new text.
 * @return the new text, terminated, or `NULL` when memory runs out.
 */
char*                d_splices_apply(struct d_splices*   _splices,
                                     const char*         _text,
                                     size_t              _length,
                                     d_splice_fn_between _between,
                                     void*               _user,
                                     size_t*             _out_length);

// 2.2    Files
//------------------------------------------------------------------------------
// d_text_read reads a whole file, terminated, for the caller to free; NULL
// when it cannot be read.  d_text_write writes `<path>.part` and renames it
// over the path, and a rename that will not replace a file -- Windows' -- is
// retried once the file is gone; it writes why it failed into `_problem`.
char*                d_text_read(const char* _path,
                                 size_t*     _out_length);
bool                 d_text_write(const char* _path,
                                  const char* _text,
                                  size_t      _length,
                                  char*       _problem,
                                  size_t      _capacity);


#endif  // DJINTERP_TOOLS_DAWK_DSPLICE_H
