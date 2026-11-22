/*******************************************************************************
* djinterp [c][text]                                               text_intern.h
*
*   Text interning: names stood for by small integers.
*   A symbol is a uint32_t standing for one distinct name. Names are numbered
* densely from 0 in the order they are first seen, so comparing two names is
* comparing two integers, and a symbol can index an array. A name is any run
* of bytes, given with its length: it need not be terminated, and may hold a
* NUL, though d_text_intern_text then shows only the part before it.
*   The table keeps its own copy of every name, so a caller's text need not
* outlive the call that interns it. The set only grows: there is no removal,
* since a symbol handed out must keep meaning its name.
*
*   Moved from dawk's dsymbol (tools/dawk), where the sheet and every tree it
* is matched against intern into one table, on 2026.11.18. The library's copy
* refuses, rather than wraps, where a table would outgrow its 32-bit offsets
* or numbers.
*
*
* path:      /inc/djinterp/c/text/text_intern.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.11.17
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  D_TEXT_INTERN_NONE
    2.  dj_text_intern
2.  OPERATIONS
    ----------
    1.  Lifecycle
    2.  Interning and lookup
    3.  Reading
*/
#ifndef DJINTERP_C_TEXT_INTERN_H
#define DJINTERP_C_TEXT_INTERN_H 1
// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"                         // framework root
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // uint32_t
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    D_TEXT_INTERN_NONE
//------------------------------------------------------------------------------
// D_TEXT_INTERN_NONE
//   constant: no symbol: a name never interned, or one that could not be.
#define D_TEXT_INTERN_NONE ((uint32_t)-1)

// 1.2    dj_text_intern
//------------------------------------------------------------------------------
// dj_text_intern
//   struct: a table of interned names. Its layout is private.
struct dj_text_intern;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
/**
 * @brief Makes an empty table. It allocates nothing more until its first
 *        name.
 *
 * @return the table, or NULL if it could not be allocated.
 */
struct dj_text_intern* d_text_intern_new(void);
/**
 * @brief Frees a table and every name in it; NULL is allowed.
 *
 * @param[in] _table  the table.
 */
void d_text_intern_free(struct dj_text_intern* _table);

// 2.2    Interning and lookup
//------------------------------------------------------------------------------
/**
 * @brief Gives a name's symbol, adding the name when it is new.
 *
 * @param[in,out] _table   the table.
 * @param[in]     _text    the name; need not be terminated.
 * @param[in]     _length  its length in bytes.
 * @return the symbol; D_TEXT_INTERN_NONE when an argument is NULL, or the
 *         table could not grow, or it would need more than 32 bits to hold
 *         the name's offset or number. A failure leaves the table as it was.
 */
uint32_t d_text_intern_add(struct dj_text_intern* _table,
                           const char*            _text,
                           size_t                 _length);
/**
 * @brief Gives a name's symbol without adding it.
 *
 * @param[in] _table   the table.
 * @param[in] _text    the name; need not be terminated.
 * @param[in] _length  its length in bytes.
 * @return the symbol, or D_TEXT_INTERN_NONE when the name was never added.
 */
uint32_t d_text_intern_find(const struct dj_text_intern* _table,
                            const char*                  _text,
                            size_t                       _length);

// 2.3    Reading
//------------------------------------------------------------------------------
/**
 * @brief Gives a symbol's name, terminated.
 *
 * @param[in] _table   the table.
 * @param[in] _symbol  the symbol.
 * @return the name, valid until the next d_text_intern_add or the table's
 *         free; "" for a symbol the table does not have.
 */
const char* d_text_intern_text(const struct dj_text_intern* _table,
                               uint32_t                     _symbol);
/**
 * @brief Gives a symbol's name's length in bytes, which counts any NUL the
 *        name holds.
 *
 * @param[in] _table   the table.
 * @param[in] _symbol  the symbol.
 * @return the length; 0 for a symbol the table does not have.
 */
size_t d_text_intern_length(const struct dj_text_intern* _table,
                            uint32_t                     _symbol);
/**
 * @brief Gives how many names the table holds, which, as symbols are dense,
 *        is one past the largest.
 *
 * @param[in] _table  the table; may be NULL.
 * @return the count; 0 for NULL.
 */
size_t d_text_intern_count(const struct dj_text_intern* _table);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_INTERN_H
