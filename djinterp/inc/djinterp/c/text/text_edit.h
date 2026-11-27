/*******************************************************************************
* djinterp [c][text]                                                 text_edit.h
*
*   Text edit: changes to a text, kept apart from it until they are applied.
*   A change replaces the bytes from one offset to another of the text as it
* was read with a new text; an insertion is a change of no bytes. Changes are
* made in any order against the text as read, so every offset a caller holds
* stays good while it makes them. Two that would overlap are refused, as
* neither could then be kept whole. Applying them writes a new text in which
* every byte no change touched is the old text's own.
*   A caller that edits a format - a CMake file, a Visual Studio project -
* decides what makes a change sound in it; this only keeps the changes apart
* and applies them.
*
*   Moved from dawk's dsplice (tools/dawk), whose hosts edit CMake, Visual
* Studio and Ninja files with it, on 2026.11.18. Reading and writing a whole
* file moved to text_source.h. The library's copy refuses a change made with no
* text to put in, and d_text_edit_apply refuses a change that lies past the
* text.
*
*
* path:      /inc/djinterp/c/text/text_edit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.11.18
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  dj_text_edit
    2.  dj_text_edit_result
    3.  dj_text_edit_fn_between
2.  OPERATIONS
    ----------
    1.  Lifecycle
    2.  Changes
    3.  Applying
*/
#ifndef DJINTERP_C_TEXT_EDIT_H
#define DJINTERP_C_TEXT_EDIT_H 1
// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"  // framework root
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    dj_text_edit
//------------------------------------------------------------------------------
// dj_text_edit
//   struct: the changes made to one text and not yet applied. Its layout is
// private.
struct dj_text_edit;

// 1.2    dj_text_edit_result
//------------------------------------------------------------------------------
// dj_text_edit_result
//   enum: what became of a change.  OVERLAPS is a real conflict with another
// change; INVALID is a change no caller should make, so a computed-offset bug
// shows as one.  INVALID comes last, so the others keep their values.
enum dj_text_edit_result
{
    D_TEXT_EDIT_ADDED = 0,  // it is kept, to be applied
    D_TEXT_EDIT_OVERLAPS,   // it meets a change already made
    D_TEXT_EDIT_NO_MEMORY,  // it could not be kept
    D_TEXT_EDIT_INVALID     // no set, an end before its start, no text with
                            // a length, or a change that changes nothing
};

// 1.3    dj_text_edit_fn_between
//------------------------------------------------------------------------------
// dj_text_edit_fn_between
//   function: asked, where a change's text meets the text before or after
// it, for a byte to stand between the two - the space that keeps two
// arguments apart - or '\0' for none.
typedef char (*dj_text_edit_fn_between)(void* _user,
                                        char  _before,
                                        char  _after);


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
/**
 * @brief Makes an empty set of changes.
 *
 * @return the set, or NULL if it could not be allocated.
 */
struct dj_text_edit* d_text_edit_new(void);
/**
 * @brief Forgets every change, and frees the set; NULL is allowed.
 *
 * @param[in] _edit  the set.
 */
void d_text_edit_free(struct dj_text_edit* _edit);
/**
 * @brief Forgets every change, keeping the set for more; NULL is allowed.
 *
 * @param[in,out] _edit  the set.
 */
void d_text_edit_clear(struct dj_text_edit* _edit);
/**
 * @brief Gives how many changes are waiting.
 *
 * @param[in] _edit  the set; may be NULL.
 * @return the count; 0 for NULL.
 */
size_t d_text_edit_count(const struct dj_text_edit* _edit);

// 2.2    Changes
//------------------------------------------------------------------------------
// Two changes overlap where both replace bytes and their spans meet, and
// where an insertion falls strictly inside a replacement. Insertions at one
// place are applied in the order they were made, and before a replacement
// starting there.
/**
 * @brief Records a change: the bytes [_start, _end) of the text as read
 *        become _text.
 *
 * @param[in,out] _edit    the set.
 * @param[in]     _start   where the bytes replaced start.
 * @param[in]     _end     where they end; _start for an insertion.
 * @param[in]     _text    the new bytes, copied; need not be terminated; may
 *                         be NULL only when _length is 0.
 * @param[in]     _length  their count.
 * @return D_TEXT_EDIT_ADDED; D_TEXT_EDIT_OVERLAPS when it meets a change
 *         already made; D_TEXT_EDIT_INVALID when _edit is NULL, _end
 *         precedes _start, _text is NULL with a length, or the change
 *         changes nothing (an insertion of no bytes);
 *         D_TEXT_EDIT_NO_MEMORY when it could not be kept. Nothing is
 *         recorded unless it is added.
 */
enum dj_text_edit_result d_text_edit_add(struct dj_text_edit* _edit,
                                         size_t               _start,
                                         size_t               _end,
                                         const char*          _text,
                                         size_t               _length);

// 2.3    Applying
//------------------------------------------------------------------------------
/**
 * @brief Applies every change to a text.
 *
 * @note The changes stay in the set, for another text or another apply;
 *       their order in it is settled here.
 *
 * @param[in,out] _edit        the changes; may be NULL, for none.
 * @param[in]     _text        the text they were made against; may be NULL
 *                             only when _length is 0.
 * @param[in]     _length      its length.
 * @param[in]     _between     asked at each meeting of a change's text and
 *                             the text beside it; may be NULL.
 * @param[in]     _user        passed to _between.
 * @param[out]    _out_length  receives the new text's length; 0 when there
 *                             is none. May be NULL.
 * @return the new text, terminated, for the caller to free; NULL when a
 *         change lies past the text's end, or memory runs out.
 */
char* d_text_edit_apply(struct dj_text_edit*    _edit,
                        const char*             _text,
                        size_t                  _length,
                        dj_text_edit_fn_between _between,
                        void*                   _user,
                        size_t*                 _out_length);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_EDIT_H
