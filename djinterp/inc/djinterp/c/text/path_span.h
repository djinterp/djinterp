/*******************************************************************************
* djinterp [c]                                                       path_span.h
*
* A generic path: an address -- a word of labels -- read from its rendering
* as text, without copying it.
*   A grammar says how labels are framed (the separators, and the one a
* rendering writes), where the path proper ends (terminators, as '?' and '#'
* end a URL's path) and how an anchor is spelled. A span reads text by a
* grammar: it owns nothing and allocates nothing, so a file path, an n-ary
* tree's path and a URL's path are the same five words of state, and the
* grammar a caller writes is all that tells them apart. The vocabulary is
* core/paradigm/path/path.hpp's: the anchor contributes no label, a path's
* level is its number of labels, and paths in different anchors have no meet.
*
* path:      /inc/djinterp/c/text/path_span.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Grammar
         1.  d_path_anchor_fn
         2.  d_path_grammar
         3.  D_PATH_GRAMMAR_FOLD_CASE
         4.  D_PATH_GRAMMAR_DOT_LABELS
    2.  Spans and labels
         1.  d_path_label
         2.  d_path_span
         3.  D_PATH_SPAN_TERMINATED
         4.  D_PATH_SPAN_NO_MEET
2.  OPERATIONS
    ----------
    1.  Reading
    2.  Parts
    3.  Order
    4.  Rendering
*/

#ifndef DJINTERP_C_TEXT_PATH_SPAN_H
#define DJINTERP_C_TEXT_PATH_SPAN_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL, size_t
// djinterp
#include "../djinterp.h"  // framework root


D_EXTERN_C_BEGIN

//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Grammar
//------------------------------------------------------------------------------
// 1.1.1
// d_path_anchor_fn
//   type: measures the anchor a path's text begins with -- "/" for a
// filesystem, "https://host/" for a URL -- and returns its length in bytes,
// 0 for a relative path. _length bytes of _text are the path proper.
struct d_path_grammar;
typedef size_t (*d_path_anchor_fn)(const char*                  _text,
                                   size_t                       _length,
                                   const struct d_path_grammar* _grammar);

// 1.1.2
// d_path_grammar
//   struct: how a path is rendered as text. A caller writes one per kind of
// path; none ships with the type.
struct d_path_grammar
{
    const char*      separators;   // every character that frames labels
    char             separator;    // the one a rendering writes
    const char*      terminators;  // ends of the path proper, or NULL
    d_path_anchor_fn anchor;       // or NULL: the leading separators
    unsigned int     flags;        // D_PATH_GRAMMAR_*
};

// 1.1.3
// D_PATH_GRAMMAR_FOLD_CASE
//   constant: labels and anchors compare without regard to ASCII case.
#define D_PATH_GRAMMAR_FOLD_CASE 0x1u

// 1.1.4
// D_PATH_GRAMMAR_DOT_LABELS
//   constant: "." names its own place and ".." its parent's, as a filesystem
// and a URL (RFC 3986, 5.2.4) read them: a rendering drops the one and folds
// the other into the label before it.
#define D_PATH_GRAMMAR_DOT_LABELS 0x2u

// 1.2    Spans and labels
//------------------------------------------------------------------------------
// 1.2.1
// d_path_label
//   struct: a run of a path's text: a label, separators excluded, or another
// run a span hands back -- its anchor, the rest after its path proper.
struct d_path_label
{
    const char* text;    // not NUL-terminated; NULL for none
    size_t      length;  // in bytes
};

// 1.2.2
// d_path_span
//   struct: a path read from text it does not own.
struct d_path_span
{
    const char*                  text;     // the rendering
    size_t                       length;   // bytes of the path proper
    size_t                       extent;   // bytes read, the rest included
    size_t                       anchor;   // bytes of the anchor; 0: relative
    const struct d_path_grammar* grammar;  // how the text is read
};

// 1.2.3
// D_PATH_SPAN_TERMINATED
//   constant: a length meaning "up to the text's terminating NUL".
#define D_PATH_SPAN_TERMINATED ((size_t)-1)

// 1.2.4
// D_PATH_SPAN_NO_MEET
//   constant: d_path_span_meet's answer for paths in different anchors, which
// have no meet at all -- not a meet of level 0.
#define D_PATH_SPAN_NO_MEET ((size_t)-1)


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// None allocates and none fails: a NULL span reads as an empty one.


// 2.1    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads a path from text by a grammar.
 *
 * @param[in] _text     the rendering; it must outlive the span.
 * @param[in] _length   bytes of `_text` to read, or D_PATH_SPAN_TERMINATED.
 * @param[in] _grammar  how to read it; it must outlive the span.
 * @return the span, its path proper ending at the first terminator; every
 *         field zero for a NULL `_text` or `_grammar`.
 */
struct d_path_span  d_path_span_of(const char*                  _text,
                                   size_t                       _length,
                                   const struct d_path_grammar* _grammar);
/**
 * @brief Steps to a path's next label: none is empty, and the anchor is none.
 *
 * @param[in]     _span    the path.
 * @param[in,out] _cursor  0 before the first label; moved past each.
 * @param[out]    _label   receives the label; may be NULL.
 * @return true for a label, false past the last.
 */
bool                d_path_span_next(const struct d_path_span* _span,
                                     size_t*                   _cursor,
                                     struct d_path_label*      _label);

// 2.2    Parts
//------------------------------------------------------------------------------
// level: the labels a path has; is_anchored: whether an anchor begins it;
// anchor: that anchor's text; last: its last label, empty at level 0; rest:
// the text after its path proper (a URL's "?q#f"); parent: the path less its
// last label, anchor kept, rest dropped -- itself at level 0
size_t              d_path_span_level(const struct d_path_span* _span);
bool                d_path_span_is_anchored(const struct d_path_span* _span);
struct d_path_label d_path_span_anchor(const struct d_path_span* _span);
struct d_path_label d_path_span_last(const struct d_path_span* _span);
struct d_path_label d_path_span_rest(const struct d_path_span* _span);
struct d_path_span  d_path_span_parent(const struct d_path_span* _span);

// 2.3    Order
//------------------------------------------------------------------------------
// Labels compare byte for byte, without ASCII case under
// D_PATH_GRAMMAR_FOLD_CASE, and anchors alike, any separator matching any
// other; the first path's grammar decides. A path is in the prefix order:
// starts_with says whether _prefix is _span or an ancestor of it.
bool                d_path_label_equal(struct d_path_label          _a,
                                       struct d_path_label          _b,
                                       const struct d_path_grammar* _grammar);
/**
 * @brief The level of two paths' meet: the labels they share from the anchor.
 *
 * @param[in] _a  a path.
 * @param[in] _b  another.
 * @return the labels in common, or D_PATH_SPAN_NO_MEET for paths in
 *         different anchors -- one anchored and one not, among them.
 */
size_t              d_path_span_meet(const struct d_path_span* _a,
                                     const struct d_path_span* _b);
bool                d_path_span_starts_with(const struct d_path_span* _span,
                                            const struct d_path_span* _prefix);

// 2.4    Rendering
//------------------------------------------------------------------------------
/**
 * @brief Writes a path in its grammar's form: its anchor as read, then its
 *        labels, each framed by the grammar's separator.
 *
 * @note Under D_PATH_GRAMMAR_DOT_LABELS a "." goes, and a ".." folds the
 *       label before it away; a ".." with none before it stays in a relative
 *       path and goes from an anchored one, which cannot ascend above its
 *       anchor. An anchor that does not end in a separator is framed off from
 *       the first label by one. The rest is not written.
 *
 * @param[in]  _span  the path.
 * @param[out] _buf   receives the text and a terminator; may be NULL where
 *                    `_size` is 0.
 * @param[in]  _size  capacity of `_buf`, in bytes.
 * @return the bytes the whole rendering takes, its terminator aside: more
 *         than `_size - 1` says it was cut short, as snprintf's answer does.
 */
size_t              d_path_span_render(const struct d_path_span* _span,
                                       char*                     _buf,
                                       size_t                    _size);
/**
 * @brief Writes `_more` taken from `_base`: `_more` alone where it is
 *        anchored, and otherwise `_base`'s labels then `_more`'s, rendered as
 *        one path, so that a ".." of `_more` can fold a label of `_base` away.
 *
 * @param[in]  _base  the path `_more` is taken from.
 * @param[in]  _more  the path taken from it.
 * @param[out] _buf   receives the text and a terminator; may be NULL where
 *                    `_size` is 0.
 * @param[in]  _size  capacity of `_buf`, in bytes.
 * @return the bytes the whole rendering takes, as d_path_span_render's.
 */
size_t              d_path_span_render_join(const struct d_path_span* _base,
                                            const struct d_path_span* _more,
                                            char*                     _buf,
                                            size_t                    _size);

D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_PATH_SPAN_H
