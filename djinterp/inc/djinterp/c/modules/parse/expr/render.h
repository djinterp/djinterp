/*******************************************************************************
* djinterp [c/parse]                                                    render.h
*
* Expression core v2: the renderer, a term written back as text.
*   Brackets go where reading the text back would group otherwise, and
* nowhere else: the renderer asks, for each child, the parser's own rule --
* would the operator after the child, or the child's own operator, group
* otherwise than the term says -- so that what it writes reads back as the
* same term. A chain is written as the chain, `a < b <= c`; a flattened run
* with its operator between each operand; a juxtaposed node with its
* operator's spelling, which for an operator spelled "" is a space.
*   Text goes out through a write function, which lane D's stream takes, or
* into a buffer. Nothing recurses: a task stack of the renderer's own holds
* what is still to be written.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/render.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Writers
         1.  dj_parse_expr_write_fn
         2.  dj_parse_expr_atom_fn
    2.  Settings
         1.  Render flags
         2.  dj_parse_expr_render_options
2.  OPERATIONS
    ----------
    1.  Rendering
*/

#ifndef DJINTERP_C_PARSE_EXPR_RENDER_H
#define DJINTERP_C_PARSE_EXPR_RENDER_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../djinterp.h"                              // framework root
#include "../../../../config/c/modules/parse/cfg_parse.h"   // D_INTERNAL_PARSE_EXPR_RENDER
#include "./signature.h"                                    // dj_parse_expr_signature
#include "./term.h"                                         // dj_parse_expr_term
// re_std
#include "../../../../../re_std/c/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_PARSE_EXPR_RENDER == 1)

//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Writers
//------------------------------------------------------------------------------
// 1.1.1
// dj_parse_expr_write_fn
//   type: where text goes: writes `_length` bytes and returns how many it
// wrote; fewer is a failure, and stops the render.
typedef size_t (*dj_parse_expr_write_fn)(void*       _ctx,
                                         const char* _bytes,
                                         size_t      _length);

// 1.1.2
// dj_parse_expr_atom_fn
//   type: writes a leaf, through the write function it is given. Returns true,
// or false on a failure, which stops the render.
typedef bool (*dj_parse_expr_atom_fn)(
                 void*                            _ctx,
                 const struct dj_parse_expr_term* _term,
                 uint32_t                         _node,
                 dj_parse_expr_write_fn           _write,
                 void*                            _write_ctx);


// 1.2    Settings
//------------------------------------------------------------------------------
// 1.2.1
// Render flags
//   constant: KEEP_GROUPS writes the author's brackets where a node was
// written in a group, needed or not; COMPACT writes symbol operators without
// spaces around them (a word operator is always spaced).
#define D_PARSE_EXPR_RENDER_KEEP_GROUPS 0x01u
#define D_PARSE_EXPR_RENDER_COMPACT     0x02u

// 1.2.2
// dj_parse_expr_render_options
//   struct: how to render. `atom` writes a leaf; without one, a leaf is its
// span's text where `text` is given and holds the span, nothing for an empty
// span, and its value in decimal otherwise.
// `hole` is what a hole is written as, "?" where NULL.
struct dj_parse_expr_render_options
{
    dj_parse_expr_atom_fn atom;
    void*                 atom_ctx;
    const char*           text;
    size_t                text_length;
    const char*           hole;
    uint32_t              flags;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Rendering
//------------------------------------------------------------------------------
/**
 * @brief Writes a subtree as text.
 *
 * @param[in] _signature  the sealed signature the term was read by.
 * @param[in] _term       the term.
 * @param[in] _root       the subtree's root.
 * @param[in] _options    how; may be NULL for the defaults.
 * @param[in] _write      where the text goes.
 * @param[in] _write_ctx  its context.
 * @return true when the whole text was written; false where an argument is
 *         wrong, an operator is not in the signature, a write or the atom
 *         writer failed, or the task stack could not grow.
 */
bool   d_parse_expr_render(
           const struct dj_parse_expr_signature*      _signature,
           const struct dj_parse_expr_term*           _term,
           uint32_t                                   _root,
           const struct dj_parse_expr_render_options* _options,
           dj_parse_expr_write_fn                     _write,
           void*                                      _write_ctx);
/**
 * @brief Writes a subtree as text into a buffer, as snprintf writes.
 *
 * @param[in]  _signature  the sealed signature the term was read by.
 * @param[in]  _term       the term.
 * @param[in]  _root       the subtree's root.
 * @param[in]  _options    how; may be NULL for the defaults.
 * @param[out] _out        the buffer; may be NULL with a size 0.
 * @param[in]  _size       its size; the text is cut to fit, NUL-terminated
 *                         where the size is not 0.
 * @return the whole text's length, without its NUL, or (size_t)-1 where it
 *         could not be rendered.
 */
size_t d_parse_expr_render_buffer(
           const struct dj_parse_expr_signature*      _signature,
           const struct dj_parse_expr_term*           _term,
           uint32_t                                   _root,
           const struct dj_parse_expr_render_options* _options,
           char*                                      _out,
           size_t                                     _size);

D_EXTERN_C_END

#endif  // D_INTERNAL_PARSE_EXPR_RENDER


#endif  // DJINTERP_C_PARSE_EXPR_RENDER_H
