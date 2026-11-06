/*******************************************************************************
* djinterp [c/parse]                                                     parse.c
*
* Expression core v2: definitions for parse.h, the parser.
*   THE MACHINE. Two explicit stacks: operands, each a node and the extent of
* text it was read from (a group's brackets included), held as parallel
* arrays so that a node's children are a slice of one; and frames, each an
* operator still open: a prefix operator awaiting its operand's end, an infix
* one its right operand's, a chain or a flattening run collecting operands,
* a mixfix operator awaiting its next part, or one past its last part
* awaiting its tail. A frame is reduced -- popped, and its node built from
* the operands above its base -- when an operator that binds looser arrives,
* or when what encloses it closes. A mixfix frame awaiting a part is never
* reduced past: it is what a bracket is.
*   POSITIONS. In operand position a token's operand-position roles are
* asked; in operator position its operator-position roles, after the next
* part and the separators of the innermost open mixfix operator, and before
* juxtaposition, which reads a token with no role there as the start of the
* next operand.
*   ERRORS. Each is reported where it is found and the parse goes on: a hole
* stands for a missing operand, a stray token is skipped, a mismatched close
* closes what is open inside the operator it closes, and the end closes what
* is still open. The result names the error the options' order puts first.
*
*
* path:      /src/djinterp/c/modules/parse/expr/parse.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/parse/expr/parse.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, memset
// djinterp
#include "../../../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul
#include "./expr_internal.h"                           // d_internal_expr_decide,
                                                       // the frame kinds


//==============================================================================
// 1.  THE PARSER'S STATE
//==============================================================================


// 1.1    Frames and operands
//------------------------------------------------------------------------------
// frame flags: JUXTAPOSED, its operator had no token; ROWS, a row separator
// was seen in its list; EMPTY, its list closed with no item
#define D_INTERNAL_EXPR_FRAME_JUXTAPOSED 0x01u
#define D_INTERNAL_EXPR_FRAME_ROWS       0x02u
#define D_INTERNAL_EXPR_FRAME_EMPTY      0x04u

// the stacks' own room before they need the heap: what a person writes
#define D_INTERNAL_EXPR_LOCAL_OPERANDS  128u
#define D_INTERNAL_EXPR_LOCAL_FRAMES    64u
#define D_INTERNAL_EXPR_LOCAL_LINKS     32u

// a quotation's most bytes in a message
#define D_INTERNAL_EXPR_QUOTE_MAX       32

// dj_internal_expr_frame
//   struct: an open operator. `base` is the operand stack's height where its
// operands begin (a lead's or a left operand's included); `part` the next
// part a mixfix awaits; `items` the items its list has finished; `row_base`
// where its current row began; `links` where a chain's links begin; `offset`
// where its node's span will begin; `token` the operator's token's span.
struct dj_internal_expr_frame
{
    const struct dj_parse_expr_op* op;
    uint8_t                        kind;
    uint8_t                        part;
    uint8_t                        flags;
    uint8_t                        assoc;
    int32_t                        precedence;
    uint32_t                       base;
    uint32_t                       items;
    uint32_t                       row_base;
    uint32_t                       links;
    uint32_t                       offset;
    uint32_t                       token_offset;
    uint32_t                       token_length;
};

// dj_internal_expr_link
//   struct: one link of a chain: its operator, and its token's span.
struct dj_internal_expr_link
{
    const struct dj_parse_expr_op* op;
    uint32_t                       offset;
    uint32_t                       length;
};


// 1.2    The parser
//------------------------------------------------------------------------------
// dj_parse_expr_parser
//   struct: a parse in progress. The hook interface takes it opaquely.
struct dj_parse_expr_parser
{
    const struct dj_parse_expr_signature* sig;
    struct dj_parse_expr_source*          source;
    struct dj_parse_expr_term*            term;
    struct dj_parse_diag_sink*            sink;
    struct dj_parse_expr_result*          result;
    uint32_t                              max_depth;
    uint32_t                              max_errors;
    uint32_t                              mode;
    uint32_t                              blame;
    uint8_t                               order;
    uint8_t                               flags;
    uint8_t                               position;
    bool                                  fatal;
    bool                                  stopped;
    bool                                  best_structural;
    uint32_t                              source_id;
    const char*                           text;
    size_t                                text_length;
    dj_parse_expr_check_fn                check;
    void*                                 check_ctx;
    struct dj_parse_expr_token            current;
    bool                                  has_current;
    struct dj_parse_expr_token            peeked;
    uint32_t                              peeked_mode;
    bool                                  has_peeked;
    uint32_t*                             nodes;
    uint32_t*                             starts;
    uint32_t*                             ends;
    uint32_t                              operand_count;
    uint32_t                              operand_capacity;
    struct dj_internal_expr_frame*        frames;
    uint32_t                              frame_count;
    uint32_t                              frame_capacity;
    struct dj_internal_expr_link*         links;
    uint32_t                              link_count;
    uint32_t                              link_capacity;
    uint32_t                              local_nodes[
                                              D_INTERNAL_EXPR_LOCAL_OPERANDS];
    uint32_t                              local_starts[
                                              D_INTERNAL_EXPR_LOCAL_OPERANDS];
    uint32_t                              local_ends[
                                              D_INTERNAL_EXPR_LOCAL_OPERANDS];
    struct dj_internal_expr_frame         local_frames[
                                              D_INTERNAL_EXPR_LOCAL_FRAMES];
    struct dj_internal_expr_link          local_links[
                                              D_INTERNAL_EXPR_LOCAL_LINKS];
};


//==============================================================================
// 2.  REPORTING
//==============================================================================


/**
 * @brief Writes a token's text, quoted, or a word for it, into a buffer.
 *
 * @param[in]  _p       the parser.
 * @param[in]  _offset  the text's first byte.
 * @param[in]  _length  its length.
 * @param[out] _out     the buffer.
 * @param[in]  _size    its size.
 */
static void
d_internal_expr_quote(
    const struct dj_parse_expr_parser* _p,
    uint32_t                           _offset,
    uint32_t                           _length,
    char*                              _out,
    size_t                             _size
)
{
    size_t end = 0u;

    // the end of the input has no text
    if (_length == 0u)
    {
        (void)snprintf(_out, _size, "the end of the input");

        return;
    }

    // quote the text where it is given and the span lies in it
    if ( (_p->text)                                                   &&
         (!d_ckd_add(&end, (size_t)_offset, (size_t)_length))         &&
         (end <= _p->text_length) )
    {
        const int shown = (_length > (uint32_t)D_INTERNAL_EXPR_QUOTE_MAX)
                          ? D_INTERNAL_EXPR_QUOTE_MAX
                          : (int)_length;

        (void)snprintf(_out, _size, "'%.*s'", shown, _p->text + _offset);

        return;
    }

    (void)snprintf(_out, _size, "a token");

    return;
}

/**
 * @brief Whether an error code is about the brackets' structure.
 *
 * @param[in] _code  the code, in the EXPR domain.
 * @return true for unclosed, unopened and mismatched.
 */
static bool
d_internal_expr_structural(
    uint16_t _code
)
{
    return ( (_code == D_PARSE_EXPR_DIAG_UNCLOSED)  ||
             (_code == D_PARSE_EXPR_DIAG_UNOPENED)  ||
             (_code == D_PARSE_EXPR_DIAG_MISMATCHED) );
}

/**
 * @brief Records an error the sink has been given, and decides whether it is
 *        now the error.
 *
 * @param[in,out] _p        the parser.
 * @param[in]     _domain   its domain.
 * @param[in]     _code     its code.
 * @param[in]     _offset   its span's first byte.
 * @param[in]     _length   its span's length.
 * @param[in]     _stored   its index in the sink, or 0xFFFFFFFF.
 */
static void
d_internal_expr_count_error(
    struct dj_parse_expr_parser* _p,
    uint16_t                     _domain,
    uint16_t                     _code,
    uint32_t                     _offset,
    uint32_t                     _length,
    uint32_t                     _stored
)
{
    struct dj_parse_expr_result* r          = _p->result;
    const bool                   structural =
        ( (_domain == D_PARSE_DIAG_DOMAIN_EXPR) &&
          (d_internal_expr_structural(_code)) );
    bool                         better     = (r->errors == 0u);

    // the leftmost, ties to the first; structure first where that is the order
    if (!better)
    {
        if ( (_p->order == D_PARSE_EXPR_ORDER_STRUCTURE_FIRST) &&
             (structural != _p->best_structural) )
        {
            better = structural;
        }
        else
        {
            better = (_offset < r->span.offset);
        }
    }

    if (better)
    {
        r->domain           = _domain;
        r->code             = _code;
        r->span             = d_parse_span_located(_p->source_id, _offset,
                                                   _length, 0u, 0u);
        r->diag_index       = _stored;
        _p->best_structural = structural;
    }

    ++r->errors;

    // a parse stops after its allowance of errors
    if (r->errors >= _p->max_errors)
    {
        _p->stopped = true;
    }

    return;
}

/**
 * @brief Reports an error of the EXPR domain at a span.
 *
 * @param[in,out] _p        the parser.
 * @param[in]     _code     its code.
 * @param[in]     _offset   its span's first byte.
 * @param[in]     _length   its span's length.
 * @param[in]     _message  its text.
 */
static void
d_internal_expr_error(
    struct dj_parse_expr_parser* _p,
    uint16_t                     _code,
    uint32_t                     _offset,
    uint32_t                     _length,
    const char*                  _message
)
{
    const uint32_t before = (_p->sink) ? _p->sink->count : 0u;

    (void)d_parse_diag_emit(_p->sink,
                            D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
                            _code,
                            d_parse_span_located(_p->source_id, _offset,
                                                 _length, 0u, 0u),
                            _message);

    d_internal_expr_count_error(
        _p,
        (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
        _code,
        _offset,
        _length,
        ( (_p->sink) && (_p->sink->count > before) ) ? before : 0xFFFFFFFFu);

    return;
}

/**
 * @brief Reports a note continuing the error before it, under that error's
 *        code, so a reader keyed on domain and code finds the note's text.
 *
 * @param[in,out] _p        the parser.
 * @param[in]     _code     the code of the error it continues.
 * @param[in]     _offset   its span's first byte.
 * @param[in]     _length   its span's length.
 * @param[in]     _message  its text.
 */
static void
d_internal_expr_note(
    struct dj_parse_expr_parser* _p,
    uint16_t                     _code,
    uint32_t                     _offset,
    uint32_t                     _length,
    const char*                  _message
)
{
    (void)d_parse_diag_emit_flagged(_p->sink,
                                    D_PARSE_SEVERITY_NOTE,
                                    (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
                                    _code,
                                    D_PARSE_DIAG_FLAG_CONTINUATION,
                                    d_parse_span_located(_p->source_id,
                                                         _offset,
                                                         _length,
                                                         0u,
                                                         0u),
                                    _message);

    return;
}

/**
 * @brief Reports a failure the parse cannot go on from, and stops it.
 *
 * @param[in,out] _p        the parser.
 * @param[in]     _domain   its domain.
 * @param[in]     _code     its code.
 * @param[in]     _offset   where the parse was.
 * @param[in]     _message  its text.
 */
static void
d_internal_expr_fatal(
    struct dj_parse_expr_parser* _p,
    uint16_t                     _domain,
    uint16_t                     _code,
    uint32_t                     _offset,
    const char*                  _message
)
{
    const uint32_t before = (_p->sink) ? _p->sink->count : 0u;

    (void)d_parse_diag_emit(_p->sink,
                            D_PARSE_SEVERITY_FATAL,
                            _domain,
                            _code,
                            d_parse_span_located(_p->source_id, _offset, 0u,
                                                 0u, 0u),
                            _message);
    d_internal_expr_count_error(
        _p,
        _domain,
        _code,
        _offset,
        0u,
        ( (_p->sink) && (_p->sink->count > before) ) ? before : 0xFFFFFFFFu);

    _p->fatal   = true;
    _p->stopped = true;

    return;
}

/**
 * @brief Reports that the term or a stack could not grow.
 *
 * @param[in,out] _p     the parser.
 * @param[in]     _term  whether it was the term, not a stack.
 *
 * Storage that cannot grow -- the caller's, or any where the heap is not
 * built in -- is full, a capacity failure; storage that could grow and did
 * not is out of memory.
 */
static void
d_internal_expr_no_room(
    struct dj_parse_expr_parser* _p,
    bool                         _term
)
{
    const uint32_t at = (_p->has_current) ? _p->current.offset : 0u;
    const uint32_t owned = ( D_PARSE_EXPR_TERM_OWNS_NODES |
                             D_PARSE_EXPR_TERM_OWNS_ARGS );
    const bool     fixed = ( (D_INTERNAL_PARSE_EXPR_HEAP == 0) ||
                             ( (_term) &&
                               ((_p->term->flags & owned) != owned) ) );

    if (fixed)
    {
        d_internal_expr_fatal(_p,
                              (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                              (uint16_t)D_PARSE_DIAG_CAPACITY,
                              at,
                              "the expression's storage is full");

        return;
    }

    d_internal_expr_fatal(_p,
                          (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                          (uint16_t)D_PARSE_DIAG_OUT_OF_MEMORY,
                          at,
                          "out of memory");

    return;
}


//==============================================================================
// 3.  STACKS AND TOKENS
//==============================================================================


/**
 * @brief Makes room on one of the parser's stacks for one more element.
 *
 * @param[in,out] _data      the stack's array.
 * @param[in]     _local     the parser's own array for it, never freed.
 * @param[in]     _count     the elements it holds.
 * @param[in,out] _capacity  its room.
 * @param[in]     _stride    an element's size.
 * @return true when there is room.
 *
 * A stack starts in the parser's own array and moves to the heap, doubling,
 * when it outgrows it; a build without the heap parses what that array
 * holds.
 */
static bool
d_internal_expr_stack_room(
    void**      _data,
    const void* _local,
    uint32_t    _count,
    uint32_t*   _capacity,
    size_t      _stride
)
{
    // the common case: it fits
    if (_count < *_capacity)
    {
        return true;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    size_t elements = 0u;
    size_t bytes    = 0u;

    // double, refusing a size that does not fit
    if ( (d_ckd_mul(&elements, (size_t)*_capacity, 2u))     ||
         (elements > (size_t)0xFFFFFFFFu)                   ||
         (d_ckd_mul(&bytes, elements, _stride)) )
    {
        return false;
    }

    void* grown = (*_data == _local) ? malloc(bytes)
                                     : realloc(*_data, bytes);

    // check if the stack could grow
    if (!grown)
    {
        return false;
    }

    // the parser's own array is copied out, never freed
    if (*_data == _local)
    {
        memcpy(grown, _local, (size_t)_count * _stride);
    }

    *_data     = grown;
    *_capacity = (uint32_t)elements;

    return true;
#else
    (void)_data;
    (void)_local;
    (void)_stride;

    return false;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
}

/**
 * @brief Pushes an operand.
 *
 * @param[in,out] _p       the parser.
 * @param[in]     _node    its node.
 * @param[in]     _offset  its extent's first byte.
 * @param[in]     _end     its extent's end.
 * @return true, or false (fatal reported) where the stack is full.
 *
 * The three arrays grow together: each is grown to the same capacity, and the
 * shared capacity moves only once all three have room.
 */
static bool
d_internal_expr_push_operand(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _node,
    uint32_t                     _offset,
    uint32_t                     _end
)
{
    uint32_t nodes_room  = _p->operand_capacity;
    uint32_t starts_room = _p->operand_capacity;
    uint32_t ends_room   = _p->operand_capacity;
    void*    nodes       = _p->nodes;
    void*    starts      = _p->starts;
    void*    ends        = _p->ends;
    bool     room        = (
        (d_internal_expr_stack_room(&nodes, _p->local_nodes,
                                    _p->operand_count, &nodes_room,
                                    sizeof(uint32_t))) &&
        (d_internal_expr_stack_room(&starts, _p->local_starts,
                                    _p->operand_count, &starts_room,
                                    sizeof(uint32_t))) &&
        (d_internal_expr_stack_room(&ends, _p->local_ends,
                                    _p->operand_count, &ends_room,
                                    sizeof(uint32_t))) );

    // whatever grew is kept, so a later push does not lose it
    _p->nodes  = (uint32_t*)nodes;
    _p->starts = (uint32_t*)starts;
    _p->ends   = (uint32_t*)ends;

    // make room, or stop
    if (!room)
    {
        d_internal_expr_no_room(_p, false);

        return false;
    }

    _p->operand_capacity = nodes_room;
    _p->nodes[_p->operand_count]  = _node;
    _p->starts[_p->operand_count] = _offset;
    _p->ends[_p->operand_count]   = _end;
    ++_p->operand_count;

    return true;
}

/**
 * @brief Pushes a frame, the depth guard counting it.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _frame  the frame, copied.
 * @return true, or false (fatal reported) where the stack is full or the
 *         nesting passes the guard.
 */
static bool
d_internal_expr_push_frame(
    struct dj_parse_expr_parser*         _p,
    const struct dj_internal_expr_frame* _frame
)
{
    void* data = _p->frames;

    // open operators nest at least as deep as the term they make
    if (_p->frame_count >= _p->max_depth)
    {
        d_internal_expr_fatal(_p,
                              (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
                              (uint16_t)D_PARSE_EXPR_DIAG_TOO_DEEP,
                              _frame->token_offset,
                              "the expression nests deeper than its limit");

        return false;
    }

    // make room, or stop
    if (!d_internal_expr_stack_room(&data,
                                    _p->local_frames,
                                    _p->frame_count,
                                    &_p->frame_capacity,
                                    sizeof(struct dj_internal_expr_frame)))
    {
        d_internal_expr_no_room(_p, false);

        return false;
    }

    _p->frames                    = (struct dj_internal_expr_frame*)data;
    _p->frames[_p->frame_count++] = *_frame;

    return true;
}

/**
 * @brief Pushes a chain's link.
 *
 * @param[in,out] _p       the parser.
 * @param[in]     _op      its operator.
 * @param[in]     _offset  its token's first byte.
 * @param[in]     _length  its token's length.
 * @return true, or false (fatal reported) where the stack is full.
 */
static bool
d_internal_expr_push_link(
    struct dj_parse_expr_parser*   _p,
    const struct dj_parse_expr_op* _op,
    uint32_t                       _offset,
    uint32_t                       _length
)
{
    void* data = _p->links;

    // make room, or stop
    if (!d_internal_expr_stack_room(&data,
                                    _p->local_links,
                                    _p->link_count,
                                    &_p->link_capacity,
                                    sizeof(struct dj_internal_expr_link)))
    {
        d_internal_expr_no_room(_p, false);

        return false;
    }

    _p->links = (struct dj_internal_expr_link*)data;
    _p->links[_p->link_count].op     = _op;
    _p->links[_p->link_count].offset = _offset;
    _p->links[_p->link_count].length = _length;
    ++_p->link_count;

    return true;
}

/**
 * @brief The token the parse is at, read in a position if not yet read.
 *
 * @param[in,out] _p         the parser.
 * @param[in]     _position  the position to read it in.
 * @return the token; where the source fails, the parse is stopped and an end
 *         token returned.
 */
static const struct dj_parse_expr_token*
d_internal_expr_current(
    struct dj_parse_expr_parser* _p,
    uint8_t                      _position
)
{
    // read already, or peeked by a hook
    if (_p->has_current)
    {
        return &_p->current;
    }

    if (_p->has_peeked)
    {
        _p->current     = _p->peeked;
        _p->has_current = true;
        _p->has_peeked  = false;

        return &_p->current;
    }

    memset(&_p->current, 0, sizeof(_p->current));

    const int got = _p->source->next(_p->source->ctx,
                                     _position,
                                     _p->mode,
                                     &_p->current);

    // a source that fails has reported; the parse cannot go on
    if (got < 0)
    {
        _p->fatal       = true;
        _p->stopped     = true;
        _p->current.cls = D_PARSE_EXPR_CLASS_END;
    }

    // the end is a token of its own, never taken
    if (got == 0)
    {
        _p->current.cls    = D_PARSE_EXPR_CLASS_END;
        _p->current.length = 0u;
    }

    _p->has_current = true;

    return &_p->current;
}

/**
 * @brief Takes the token the parse is at; the end is never taken.
 *
 * @param[in,out] _p  the parser.
 */
static void
d_internal_expr_advance(
    struct dj_parse_expr_parser* _p
)
{
    // the end stays, so every later read sees it
    if ( (_p->has_current) &&
         (_p->current.cls != D_PARSE_EXPR_CLASS_END) )
    {
        _p->has_current = false;
    }

    return;
}


//==============================================================================
// 4.  BUILDING
//==============================================================================


/**
 * @brief Runs the language's check on a node just built.
 *
 * @param[in,out] _p     the parser.
 * @param[in]     _node  the node.
 *
 * A refusal is counted as an error: as the hook's own first diagnostic where
 * it reported one, and as D_PARSE_EXPR_DIAG_REFUSED at the node otherwise.
 */
static void
d_internal_expr_run_check(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _node
)
{
    // no check, nothing to do
    if (!_p->check)
    {
        return;
    }

    const uint32_t before = (_p->sink) ? _p->sink->count : 0u;

    // kept: the node stands either way
    if (_p->check(_p->check_ctx, _p->term, _node, _p->sink))
    {
        return;
    }

    const struct dj_parse_expr_node*  node = &_p->term->nodes[_node];
    const struct dj_parse_diagnostic* told =
        ( (_p->sink) && (_p->sink->count > before) )
        ? d_parse_diag_at(_p->sink, before)
        : NULL;

    // the hook's own words where it gave them
    if (told)
    {
        d_internal_expr_count_error(_p, told->domain, told->code,
                                    told->span.offset, told->span.length,
                                    before);

        return;
    }

    d_internal_expr_error(_p,
                          (uint16_t)D_PARSE_EXPR_DIAG_REFUSED,
                          node->offset,
                          node->length,
                          "the language refuses this");

    return;
}

/**
 * @brief Makes an application of given children, checked.
 *
 * @param[in,out] _p         the parser.
 * @param[in]     _op        the operator's id.
 * @param[in]     _children  the children's nodes.
 * @param[in]     _count     how many.
 * @param[in]     _flags     node flags to set.
 * @param[in]     _start     where its span begins.
 * @param[in]     _end       where its span ends.
 * @return the node, or D_PARSE_EXPR_NO_NODE where the parse was stopped.
 *
 * The node's depth is checked against the guard, and the language's check
 * runs on it; a refusal is counted as an error and the node kept.
 */
static uint32_t
d_internal_expr_make(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _op,
    const uint32_t*              _children,
    uint32_t                     _count,
    uint8_t                      _flags,
    uint32_t                     _start,
    uint32_t                     _end
)
{
    const uint32_t node = d_parse_expr_term_apply(_p->term,
                                                  _op,
                                                  _children,
                                                  _count,
                                                  _start,
                                                  _end - _start);

    // check if the term had room
    if (node == D_PARSE_EXPR_NO_NODE)
    {
        d_internal_expr_no_room(_p, true);

        return node;
    }

    _p->term->nodes[node].flags |= _flags;

    // the depth guard, for what consumes the term recursively: reported at
    // the operator whose node passes it, where one is known
    if ((uint32_t)_p->term->nodes[node].depth > _p->max_depth)
    {
        d_internal_expr_fatal(_p,
                              (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
                              (uint16_t)D_PARSE_EXPR_DIAG_TOO_DEEP,
                              (_p->blame != D_PARSE_EXPR_NO_NODE)
                              ? _p->blame
                              : _start,
                              "the expression nests deeper than its limit");

        return D_PARSE_EXPR_NO_NODE;
    }

    d_internal_expr_run_check(_p, node);

    return node;
}

/**
 * @brief Replaces the operands from an index to the top with one
 *        application of them.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _op     the operator's id.
 * @param[in]     _from   the stack index of its first child.
 * @param[in]     _flags  node flags to set.
 * @param[in]     _start  where its span begins.
 * @param[in]     _end    where its span ends.
 * @return true, or false where the parse was stopped.
 *
 * The children are a slice of the node array, which the term copies, so
 * nothing is gathered.
 */
static bool
d_internal_expr_build(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _op,
    uint32_t                     _from,
    uint8_t                      _flags,
    uint32_t                     _start,
    uint32_t                     _end
)
{
    const uint32_t node = d_internal_expr_make(_p,
                                               _op,
                                               &_p->nodes[_from],
                                               _p->operand_count - _from,
                                               _flags,
                                               _start,
                                               _end);

    // the operator a reduction named answers for this node alone
    _p->blame = D_PARSE_EXPR_NO_NODE;

    // check if the parse could go on
    if (node == D_PARSE_EXPR_NO_NODE)
    {
        return false;
    }

    _p->operand_count = _from;

    return d_internal_expr_push_operand(_p, node, _start, _end);
}

/**
 * @brief Pushes a hole for a missing operand.
 *
 * @param[in,out] _p   the parser.
 * @param[in]     _at  where the operand should have begun.
 * @return true, or false where the parse was stopped.
 */
static bool
d_internal_expr_hole(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _at
)
{
    const uint32_t node = d_parse_expr_term_hole(_p->term, _at);

    // check if the term had room
    if (node == D_PARSE_EXPR_NO_NODE)
    {
        d_internal_expr_no_room(_p, true);

        return false;
    }

    return d_internal_expr_push_operand(_p, node, _at, _at);
}


//==============================================================================
// 5.  REDUCTION
//==============================================================================


/**
 * @brief Builds a chain's links and joins them.
 *
 * @param[in,out] _p  the parser.
 * @param[in]     _f  the chain's frame, popped.
 * @return true, or false where the parse was stopped.
 *
 * Each link is its operator over two neighbouring operands, so an operand
 * between two links is one node, the child of both. One link stands alone;
 * more are joined, left-nested by a binary join, or by one node where the
 * join flattens. Each link's node is kept in its slot of the link stack
 * until the join is made.
 */
static bool
d_internal_expr_reduce_chain(
    struct dj_parse_expr_parser*         _p,
    const struct dj_internal_expr_frame* _f
)
{
    const uint32_t links = _p->link_count - _f->links;
    const uint32_t base  = _f->base;
    uint32_t       made  = D_PARSE_EXPR_NO_NODE;

    // one link: the relation itself, no join
    if (links == 1u)
    {
        _p->link_count = _f->links;

        return d_internal_expr_build(_p, _p->links[_f->links].op->id, base,
                                     0u, _p->starts[base],
                                     _p->ends[base + 1u]);
    }

    const struct dj_parse_expr_op* join =
        d_parse_expr_find_op(_p->sig, _f->op->join);
    const bool flat = ((join->flags & D_PARSE_EXPR_OP_FLATTEN) != 0u);

    for (uint32_t i = 0u; i < links; ++i)
    {
        const uint32_t pair[2] = { _p->nodes[base + i],
                                   _p->nodes[base + i + 1u] };
        const uint32_t link    = d_internal_expr_make(
            _p, _p->links[_f->links + i].op->id, pair, 2u, 0u,
            _p->starts[base + i], _p->ends[base + i + 1u]);

        // check if the parse could go on
        if (link == D_PARSE_EXPR_NO_NODE)
        {
            return false;
        }

        // the links are kept where the operands were, which nothing reads
        // again: the slot of link i is below every operand link i + 1 reads
        _p->nodes[base + i] = link;

        if ( (!flat) &&
             (i > 0u) )
        {
            const uint32_t both[2] = { (i == 1u) ? _p->nodes[base] : made,
                                       link };

            made = d_internal_expr_make(_p, join->id, both, 2u,
                                        D_PARSE_EXPR_NODE_CHAIN,
                                        _p->starts[base],
                                        _p->ends[base + i + 1u]);

            if (made == D_PARSE_EXPR_NO_NODE)
            {
                return false;
            }
        }
    }

    const uint32_t start = _p->starts[base];
    const uint32_t end   = _p->ends[_p->operand_count - 1u];

    _p->link_count = _f->links;

    // a flattening join is one node over every link
    if (flat)
    {
        made = d_internal_expr_make(_p, join->id, &_p->nodes[base], links,
                                    D_PARSE_EXPR_NODE_CHAIN, start, end);

        if (made == D_PARSE_EXPR_NO_NODE)
        {
            return false;
        }
    }

    _p->operand_count = base;

    return d_internal_expr_push_operand(_p, made, start, end);
}

/**
 * @brief Pops the top frame, which is not an open mixfix, and builds its
 *        node.
 *
 * @param[in,out] _p  the parser.
 * @return true, or false where the parse was stopped.
 */
static bool
d_internal_expr_reduce_top(
    struct dj_parse_expr_parser* _p
)
{
    const struct dj_internal_expr_frame f   = _p->frames[_p->frame_count - 1u];
    const uint32_t                      top = _p->operand_count - 1u;
    uint8_t                             flags =
        ((f.flags & D_INTERNAL_EXPR_FRAME_JUXTAPOSED) != 0u)
        ? D_PARSE_EXPR_NODE_JUXTAPOSED
        : 0u;

    --_p->frame_count;
    _p->blame = f.token_offset;

    switch (f.kind)
    {
        case D_INTERNAL_EXPR_FRAME_PREFIX:
        case D_INTERNAL_EXPR_FRAME_TAIL:
            return d_internal_expr_build(_p, f.op->id, f.base, 0u, f.offset,
                                         _p->ends[top]);

        case D_INTERNAL_EXPR_FRAME_FLAT:
            flags |= ((_p->operand_count - f.base) > 2u)
                     ? D_PARSE_EXPR_NODE_FLATTENED
                     : 0u;

            return d_internal_expr_build(_p, f.op->id, f.base, flags,
                                         _p->starts[f.base], _p->ends[top]);

        case D_INTERNAL_EXPR_FRAME_CHAIN:
            return d_internal_expr_reduce_chain(_p, &f);

        default:
            return d_internal_expr_build(_p, f.op->id, f.base, flags,
                                         _p->starts[f.base], _p->ends[top]);
    }
}

/*
d_internal_expr_decide
  A prefix frame is reduced only by a looser operator: a prefix operator takes
into its operand every operator at least as tight as itself, as v1's climber
does. Between binary frames the open operator's own association decides a
tie, as v1's climber's floors do, and a postfix arrival at a tie applies to
the whole unless the open operator is right-associative. The renderer asks
the same question to decide its brackets (render.c).
*/
int
d_internal_expr_decide(
    uint8_t                        _kind,
    int32_t                        _precedence,
    uint8_t                        _assoc,
    const struct dj_parse_expr_op* _open,
    const struct dj_parse_expr_op* _op,
    bool                           _postfix
)
{
    // a bracket is never reduced past
    if (_kind == D_INTERNAL_EXPR_FRAME_MIXFIX)
    {
        return D_INTERNAL_EXPR_STOP;
    }

    // the looser waits; the tighter is reduced; a prefix operator ties to
    // its operand
    if ( (_kind == D_INTERNAL_EXPR_FRAME_PREFIX) ||
         (_precedence != _op->precedence) )
    {
        return (_precedence > _op->precedence) ? D_INTERNAL_EXPR_REDUCE
                                               : D_INTERNAL_EXPR_STOP;
    }

    if (_postfix)
    {
        return (_assoc == D_PARSE_EXPR_ASSOC_RIGHT) ? D_INTERNAL_EXPR_STOP
                                                    : D_INTERNAL_EXPR_REDUCE;
    }

    // a tie: refused, extended, or grouped by the open operator's association
    if ( (_assoc == D_PARSE_EXPR_ASSOC_NONE) ||
         (_op->assoc == D_PARSE_EXPR_ASSOC_NONE) )
    {
        return D_INTERNAL_EXPR_NONASSOC;
    }

    if ( ( (_kind == D_INTERNAL_EXPR_FRAME_CHAIN) &&
           (_op->assoc == D_PARSE_EXPR_ASSOC_CHAIN) )  ||
         ( (_kind == D_INTERNAL_EXPR_FRAME_FLAT) &&
           (_open == _op) ) )
    {
        return D_INTERNAL_EXPR_EXTEND;
    }

    return (_assoc == D_PARSE_EXPR_ASSOC_RIGHT) ? D_INTERNAL_EXPR_STOP
                                                : D_INTERNAL_EXPR_REDUCE;
}

/**
 * @brief Reports two operators of one precedence that do not associate,
 *        each named: `'==' cannot follow '<' without brackets`.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _f      the frame of the first.
 * @param[in]     _token  the second.
 */
static void
d_internal_expr_nonassoc(
    struct dj_parse_expr_parser*         _p,
    const struct dj_internal_expr_frame* _f,
    const struct dj_parse_expr_token*    _token
)
{
    char second[48];
    char first[48];
    char text[128];

    d_internal_expr_quote(_p, _token->offset, _token->length, second,
                          sizeof(second));
    d_internal_expr_quote(_p, _f->token_offset, _f->token_length, first,
                          sizeof(first));
    (void)snprintf(text, sizeof(text), "%s cannot follow %s without brackets",
                   second, first);
    d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE,
                          _token->offset, _token->length, text);
    d_internal_expr_note(_p, (uint16_t)D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE,
                         _f->token_offset, _f->token_length,
                         "the first operator is here");

    return;
}

/**
 * @brief Reduces the frames an arriving operator binds looser than.
 *
 * @param[in,out] _p        the parser.
 * @param[in]     _op       the arriving operator.
 * @param[in]     _postfix  as d_internal_expr_decide's.
 * @param[in]     _token    its token, for a non-associative pair's report.
 * @return STOP or EXTEND: what the frame left on top decided, STOP where none
 *         is left or the parse was stopped.
 */
static int
d_internal_expr_reduce_for(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_op*    _op,
    bool                              _postfix,
    const struct dj_parse_expr_token* _token
)
{
    while ( (_p->frame_count > 0u) &&
            (!_p->stopped) )
    {
        const struct dj_internal_expr_frame* f =
            &_p->frames[_p->frame_count - 1u];
        const int decided = d_internal_expr_decide(f->kind,
                                                   f->precedence,
                                                   f->assoc,
                                                   f->op,
                                                   _op,
                                                   _postfix);

        // a refused pair is reported, then read as left-associative
        if (decided == D_INTERNAL_EXPR_NONASSOC)
        {
            d_internal_expr_nonassoc(_p, f, _token);
        }
        else if (decided != D_INTERNAL_EXPR_REDUCE)
        {
            return decided;
        }

        (void)d_internal_expr_reduce_top(_p);
    }

    return D_INTERNAL_EXPR_STOP;
}

/**
 * @brief Reduces every frame above one, which stays.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _frame  the index of the frame to stop at.
 */
static void
d_internal_expr_reduce_above(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _frame
)
{
    while ( (_p->frame_count > _frame + 1u) &&
            (!_p->stopped) )
    {
        (void)d_internal_expr_reduce_top(_p);
    }

    return;
}

/**
 * @brief The index of the innermost open mixfix frame.
 *
 * @param[in] _p  the parser.
 * @return its index, or D_PARSE_EXPR_NO_NODE where none is open.
 */
static uint32_t
d_internal_expr_innermost(
    const struct dj_parse_expr_parser* _p
)
{
    for (uint32_t i = _p->frame_count; i > 0u; --i)
    {
        if (_p->frames[i - 1u].kind == D_INTERNAL_EXPR_FRAME_MIXFIX)
        {
            return i - 1u;
        }
    }

    return D_PARSE_EXPR_NO_NODE;
}


//==============================================================================
// 6.  MIXFIX OPERATORS
//==============================================================================


/**
 * @brief Whether a role is there in the parse's mode, and its token fits its
 *        spacing condition.
 *
 * @param[in] _p      the parser.
 * @param[in] _role   the role.
 * @param[in] _token  the token.
 * @return true when the role may be asked.
 */
static bool
d_internal_expr_role_active(
    const struct dj_parse_expr_parser* _p,
    const struct dj_parse_expr_role*   _role,
    const struct dj_parse_expr_token*  _token
)
{
    return ( ( (_role->modes == 0u) ||
               ((_role->modes & _p->mode) != 0u) )                       &&
             ( ((_role->flags & D_PARSE_EXPR_ROLE_ADJACENT) == 0u) ||
               ((_token->flags & D_PARSE_EXPR_TOKEN_SPACE_BEFORE) == 0u) ) );
}

/**
 * @brief Checks a list's items against its bounds.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _f      the list's frame.
 * @param[in]     _end    where the list, or the row, ends.
 */
static void
d_internal_expr_check_items(
    struct dj_parse_expr_parser*         _p,
    const struct dj_internal_expr_frame* _f,
    uint32_t                             _end
)
{
    const struct dj_parse_expr_mixfix* m = &_f->op->mixfix;
    char                               text[96];

    // within its bounds, nothing to say
    if ( (_f->items >= (uint32_t)m->min_items) &&
         (_f->items <= (uint32_t)m->max_items) )
    {
        return;
    }

    if (m->max_items == D_PARSE_EXPR_ITEMS_UNBOUNDED)
    {
        (void)snprintf(text, sizeof(text),
                       "%lu items, where at least %lu are needed",
                       (unsigned long)_f->items,
                       (unsigned long)m->min_items);
    }
    else
    {
        (void)snprintf(text, sizeof(text),
                       "%lu items, where %lu to %lu are taken",
                       (unsigned long)_f->items,
                       (unsigned long)m->min_items,
                       (unsigned long)m->max_items);
    }

    d_internal_expr_error(_p,
                          (uint16_t)D_PARSE_EXPR_DIAG_ARITY,
                          _f->token_offset,
                          _end - _f->token_offset,
                          text);

    return;
}

/**
 * @brief Closes a row of the innermost list, building it as its row
 *        operator.
 *
 * @param[in,out] _p    the parser.
 * @param[in]     _f    the list's frame, on top.
 * @param[in]     _end  where the row ends.
 * @return true, or false where the parse was stopped.
 */
static bool
d_internal_expr_close_row(
    struct dj_parse_expr_parser*   _p,
    struct dj_internal_expr_frame* _f,
    uint32_t                       _end
)
{
    const uint32_t from = _f->row_base;

    d_internal_expr_check_items(_p, _f, _end);

    // the row's items, as one node
    if (!d_internal_expr_build(_p,
                               _f->op->mixfix.row_op,
                               from,
                               0u,
                               _p->starts[from],
                               _p->ends[_p->operand_count - 1u]))
    {
        return false;
    }

    _f->row_base = _p->operand_count;
    _f->items    = 0u;
    _f->flags   |= D_INTERNAL_EXPR_FRAME_ROWS;

    return true;
}

/**
 * @brief Builds the top frame, a mixfix past its holes, and pops it.
 *
 * @param[in,out] _p    the parser.
 * @param[in]     _end  where its last part ends.
 * @return true, or false where the parse was stopped.
 *
 * A transparent operator leaves no node: its one item is flagged GROUPED,
 * and its extent widened over the brackets (an application's span too; a
 * leaf keeps its token's, which is its text).
 */
static bool
d_internal_expr_close_mixfix(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _end
)
{
    struct dj_internal_expr_frame      f = _p->frames[_p->frame_count - 1u];
    const struct dj_parse_expr_mixfix* m = &f.op->mixfix;

    --_p->frame_count;

    // a transparent operator is a group of one item; a list of others is its
    // node
    if ( ((f.op->flags & D_PARSE_EXPR_OP_TRANSPARENT) != 0u) &&
         ( (!m->list_hole) || (f.items == 1u) ) )
    {
        const uint32_t             top  = _p->operand_count - 1u;
        struct dj_parse_expr_node* node = &_p->term->nodes[_p->nodes[top]];

        node->flags |= D_PARSE_EXPR_NODE_GROUPED;

        // an application's span is its extent; a leaf's is its text
        if (node->kind == D_PARSE_EXPR_NODE_APPLY)
        {
            node->offset = f.offset;
            node->length = _end - f.offset;
        }

        _p->starts[top] = f.offset;
        _p->ends[top]   = _end;

        return true;
    }

    // a list with rows ends with its last row
    if ( (m->list_hole) &&
         ((f.flags & D_INTERNAL_EXPR_FRAME_ROWS) != 0u) )
    {
        if (!d_internal_expr_close_row(_p, &f, _end))
        {
            return false;
        }

        return d_internal_expr_build(_p, m->rows_op, f.base,
                                     D_PARSE_EXPR_NODE_ROWS, f.offset, _end);
    }

    if (m->list_hole)
    {
        d_internal_expr_check_items(_p, &f, _end);
    }

    return d_internal_expr_build(_p, f.op->id, f.base, 0u, f.offset, _end);
}

/**
 * @brief Reads the next part of the innermost open mixfix operator.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _m      the operator's frame index.
 * @param[in]     _token  the part.
 */
static void
d_internal_expr_next_part(
    struct dj_parse_expr_parser*      _p,
    uint32_t                          _m,
    const struct dj_parse_expr_token* _token
)
{
    d_internal_expr_reduce_above(_p, _m);

    // check if the parse could go on
    if (_p->stopped)
    {
        return;
    }

    struct dj_internal_expr_frame*     f   = &_p->frames[_m];
    const struct dj_parse_expr_mixfix* m   = &f->op->mixfix;
    const uint32_t                     end = _token->offset + _token->length;

    // the list's last item is done, unless it closed with none
    if ( (f->part == 1u)                                     &&
         (m->list_hole)                                      &&
         ((f->flags & D_INTERNAL_EXPR_FRAME_EMPTY) == 0u) )
    {
        ++f->items;
    }

    d_internal_expr_advance(_p);

    // a part with a hole after it
    if ((uint32_t)f->part + 1u < (uint32_t)m->parts)
    {
        ++f->part;
        _p->position = D_PARSE_EXPR_OPERAND;

        return;
    }

    // the last part: a tail to read, or the operator is whole
    if (m->tail)
    {
        if (m->list_hole)
        {
            d_internal_expr_check_items(_p, f, end);
        }

        f->kind      = D_INTERNAL_EXPR_FRAME_TAIL;
        _p->position = D_PARSE_EXPR_OPERAND;

        return;
    }

    (void)d_internal_expr_close_mixfix(_p, end);
    _p->position = D_PARSE_EXPR_OPERATOR;

    return;
}

/**
 * @brief Reads a separator of the innermost list, of items or of rows.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _m      the list's frame index.
 * @param[in]     _token  the separator.
 * @param[in]     _row    whether it separates rows.
 */
static void
d_internal_expr_separator(
    struct dj_parse_expr_parser*      _p,
    uint32_t                          _m,
    const struct dj_parse_expr_token* _token,
    bool                              _row
)
{
    d_internal_expr_reduce_above(_p, _m);

    // check if the parse could go on
    if (_p->stopped)
    {
        return;
    }

    struct dj_internal_expr_frame* f = &_p->frames[_m];

    ++f->items;

    if (_row)
    {
        (void)d_internal_expr_close_row(_p, f, _token->offset);
    }

    d_internal_expr_advance(_p);
    _p->position = D_PARSE_EXPR_OPERAND;

    return;
}

/**
 * @brief Closes the top frame, a mixfix, with what it lacks filled by
 *        holes: recovery from a close that never came.
 *
 * @param[in,out] _p   the parser, in operator position, its operand done.
 * @param[in]     _at  where the missing parts would have stood.
 */
static void
d_internal_expr_force_close(
    struct dj_parse_expr_parser* _p,
    uint32_t                     _at
)
{
    struct dj_internal_expr_frame*     f = &_p->frames[_p->frame_count - 1u];
    const struct dj_parse_expr_mixfix* m = &f->op->mixfix;
    const uint32_t                     missing =
        (uint32_t)m->parts - 1u - (uint32_t)f->part + ((m->tail) ? 1u : 0u);

    // the hole being read is done
    if ( (f->part == 1u)                                    &&
         (m->list_hole)                                     &&
         ((f->flags & D_INTERNAL_EXPR_FRAME_EMPTY) == 0u) )
    {
        ++f->items;
    }

    f->items = ( (m->list_hole) && (f->items < m->min_items) )
               ? m->min_items
               : f->items;

    // the holes and the tail it never reached
    for (uint32_t i = 0u; ( (i < missing) && (!_p->stopped) ); ++i)
    {
        (void)d_internal_expr_hole(_p, _at);
    }

    if (!_p->stopped)
    {
        (void)d_internal_expr_close_mixfix(_p, _at);
    }

    return;
}

/**
 * @brief Whether a token class is a part (other than the first) of any
 *        mixfix operator, or a separator of any list.
 *
 * @param[in] _p       the parser.
 * @param[in] _class   the class.
 * @param[in] _closes  true to ask only about parts.
 * @return true where it is.
 */
static bool
d_internal_expr_is_structural(
    const struct dj_parse_expr_parser* _p,
    uint32_t                           _class,
    bool                               _closes
)
{
    // asked only on the paths of an error, so a scan of the table suffices
    for (uint32_t i = 0u; i < _p->sig->op_count; ++i)
    {
        const struct dj_parse_expr_mixfix* m = &_p->sig->ops[i].mixfix;

        if (_p->sig->ops[i].form != D_PARSE_EXPR_FORM_MIXFIX)
        {
            continue;
        }

        for (uint32_t k = 1u; k < m->parts; ++k)
        {
            if (m->part[k] == _class)
            {
                return true;
            }
        }

        if ( (!_closes)                                                  &&
             (m->list_hole)                                              &&
             ( ( (m->separator != D_PARSE_EXPR_CLASS_END) &&
                 (m->separator == _class) )                          ||
               ( (m->row_separator != D_PARSE_EXPR_CLASS_END) &&
                 (m->row_separator == _class) ) ) )
        {
            return true;
        }
    }

    return false;
}


//==============================================================================
// 7.  THE TWO POSITIONS
//==============================================================================


/**
 * @brief Runs a custom role's hook.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _role   the role.
 * @param[in]     _left   the operand before, or D_PARSE_EXPR_NO_NODE.
 * @return 1 where it built a node, now on the operand stack (the operand
 *         before replaced); 0 where it declined, the token restored; -1 on
 *         a failure, with a hole standing for what it would have read.
 */
static int
d_internal_expr_run_custom(
    struct dj_parse_expr_parser*     _p,
    const struct dj_parse_expr_role* _role,
    uint32_t                         _left
)
{
    const struct dj_parse_expr_token token = _p->current;
    uint32_t                         out   = D_PARSE_EXPR_NO_NODE;

    d_internal_expr_advance(_p);

    const int built = _role->custom(_role->custom_ctx, _p, &token, _left,
                                    &out);

    // a token the hook read in another mode, and left, cannot be read again
    if ( (_p->has_peeked) &&
         (_p->peeked_mode != _p->mode) &&
         (!_p->stopped) )
    {
        d_internal_expr_fatal(_p,
                              (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                              (uint16_t)D_PARSE_DIAG_BAD_ARGUMENT,
                              _p->peeked.offset,
                              "a custom role left a token it read in "
                              "another mode");

        return -1;
    }

    // declined: the token is read again as if the role were not there
    if (built == 0)
    {
        _p->current     = token;
        _p->has_current = true;

        return 0;
    }

    // a node the hook made, of this term, within the guard
    if ( (built > 0)                                                  &&
         ( (out >= _p->term->node_count) ||
           ((uint32_t)_p->term->nodes[out].depth > _p->max_depth) ) )
    {
        d_internal_expr_fatal(_p,
                              (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                              (uint16_t)D_PARSE_DIAG_BAD_ARGUMENT,
                              token.offset,
                              "a custom role gave no node, or one too deep");

        return -1;
    }

    const uint32_t start = (_left != D_PARSE_EXPR_NO_NODE)
                           ? _p->starts[_p->operand_count - 1u]
                           : token.offset;
    const uint32_t end   = (built > 0)
                           ? ( _p->term->nodes[out].offset +
                               _p->term->nodes[out].length )
                           : token.offset + token.length;

    // a failure in operand position leaves a hole; in operator position the
    // operand before stands
    if (built < 0)
    {
        if (_left == D_PARSE_EXPR_NO_NODE)
        {
            (void)d_internal_expr_hole(_p, token.offset);
        }

        return -1;
    }

    // the operand before is the hook's now
    if (_left != D_PARSE_EXPR_NO_NODE)
    {
        --_p->operand_count;
    }

    (void)d_internal_expr_push_operand(_p, out, start,
                                       (end > start) ? end : start);

    return 1;
}

/**
 * @brief Opens an operator a token begins in operand position: a prefix
 *        operator, or a mixfix one without a lead.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _op     the operator.
 * @param[in]     _token  its token.
 */
static void
d_internal_expr_open(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_op*    _op,
    const struct dj_parse_expr_token* _token
)
{
    struct dj_internal_expr_frame f;

    memset(&f, 0, sizeof(f));
    f.op           = _op;
    f.kind         = (_op->form == D_PARSE_EXPR_FORM_PREFIX)
                     ? D_INTERNAL_EXPR_FRAME_PREFIX
                     : D_INTERNAL_EXPR_FRAME_MIXFIX;
    f.part         = 1u;
    f.assoc        = _op->assoc;
    f.precedence   = _op->precedence;
    f.base         = _p->operand_count;
    f.row_base     = _p->operand_count;
    f.links        = _p->link_count;
    f.offset       = _token->offset;
    f.token_offset = _token->offset;
    f.token_length = _token->length;

    // open it, or stop
    if (d_internal_expr_push_frame(_p, &f))
    {
        d_internal_expr_advance(_p);
    }

    return;
}

/**
 * @brief Whether the token closes the innermost list while it is still
 *        empty, as `f()` and `[]` do where the list takes no item.
 *
 * @param[in] _p      the parser, in operand position.
 * @param[in] _token  the token.
 * @return true where it does.
 */
static bool
d_internal_expr_closes_empty(
    const struct dj_parse_expr_parser* _p,
    const struct dj_parse_expr_token*  _token
)
{
    // the top frame is the list itself, nothing read in it yet
    if (_p->frame_count == 0u)
    {
        return false;
    }

    const struct dj_internal_expr_frame* f = &_p->frames[_p->frame_count - 1u];
    const struct dj_parse_expr_mixfix*   m = &f->op->mixfix;

    return ( (f->kind == D_INTERNAL_EXPR_FRAME_MIXFIX)                 &&
             (f->part == 1u)                                           &&
             (m->list_hole)                                            &&
             (m->min_items == 0u)                                      &&
             (f->items == 0u)                                          &&
             ((f->flags & D_INTERNAL_EXPR_FRAME_ROWS) == 0u)           &&
             (_p->operand_count == f->row_base)                        &&
             (_token->cls == m->part[1]) );
}

/**
 * @brief Whether a token has a role in a position, in the parse's mode.
 *
 * @param[in] _p         the parser.
 * @param[in] _token     the token.
 * @param[in] _position  D_PARSE_EXPR_OPERAND or D_PARSE_EXPR_OPERATOR.
 * @return true where one is there.
 */
static bool
d_internal_expr_has_role(
    const struct dj_parse_expr_parser* _p,
    const struct dj_parse_expr_token*  _token,
    uint8_t                            _position
)
{
    uint32_t                         count = 0u;
    const struct dj_parse_expr_role* roles =
        d_parse_expr_find_roles(_p->sig, _token->cls, _position, &count);

    for (uint32_t i = 0u; i < count; ++i)
    {
        if (d_internal_expr_role_active(_p, &roles[i], _token))
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Takes an atom: a leaf, pushed as an operand.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _token  the atom's token.
 */
static void
d_internal_expr_atom(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_token* _token
)
{
    const uint32_t node = d_parse_expr_term_leaf(_p->term,
                                                 _token->cls,
                                                 _token->value,
                                                 _token->offset,
                                                 _token->length);

    // check if the term had room
    if (node == D_PARSE_EXPR_NO_NODE)
    {
        d_internal_expr_no_room(_p, true);

        return;
    }

    if (d_internal_expr_push_operand(_p, node, _token->offset,
                                     _token->offset + _token->length))
    {
        d_internal_expr_advance(_p);
        _p->position = D_PARSE_EXPR_OPERATOR;
    }

    return;
}

/**
 * @brief Reads a token in operand position.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _token  the token.
 */
static void
d_internal_expr_step_operand(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_token* _token
)
{
    uint32_t                         count = 0u;
    const struct dj_parse_expr_role* roles =
        d_parse_expr_find_roles(_p->sig, _token->cls, D_PARSE_EXPR_OPERAND,
                                &count);
    char                             quote[48];
    char                             text[96];

    // the first role that fits
    for (uint32_t i = 0u; i < count; ++i)
    {
        const struct dj_parse_expr_role* role = &roles[i];

        if (!d_internal_expr_role_active(_p, role, _token))
        {
            continue;
        }

        if (role->kind == D_PARSE_EXPR_ROLE_ATOM)
        {
            d_internal_expr_atom(_p, _token);

            return;
        }

        if (role->kind == D_PARSE_EXPR_ROLE_OP)
        {
            d_internal_expr_open(_p, d_parse_expr_find_op(_p->sig, role->op),
                                 _token);

            return;
        }

        if (d_internal_expr_run_custom(_p, role, D_PARSE_EXPR_NO_NODE) != 0)
        {
            _p->position = D_PARSE_EXPR_OPERATOR;

            return;
        }
    }

    // an empty list closing is no missing operand
    if (d_internal_expr_closes_empty(_p, _token))
    {
        _p->frames[_p->frame_count - 1u].flags |= D_INTERNAL_EXPR_FRAME_EMPTY;
        _p->position                             = D_PARSE_EXPR_OPERATOR;

        return;
    }

    d_internal_expr_quote(_p, _token->offset, _token->length, quote,
                          sizeof(quote));

    // the end, a part or a separator where an operand must stand; or, where
    // the expression stops, a token with no role in either position
    if ( (_token->cls == D_PARSE_EXPR_CLASS_END)                    ||
         (d_internal_expr_is_structural(_p, _token->cls, false))    ||
         ( ((_p->flags & D_PARSE_EXPR_OPTION_STOP) != 0u) &&
           (!d_internal_expr_has_role(_p, _token,
                                      D_PARSE_EXPR_OPERATOR)) ) )
    {
        (void)snprintf(text, sizeof(text), "an operand is missing before %s",
                       quote);
        d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_MISSING_OPERAND,
                              _token->offset, _token->length, text);

        if (d_internal_expr_hole(_p, _token->offset))
        {
            _p->position = D_PARSE_EXPR_OPERATOR;
        }

        return;
    }

    // anything else is skipped
    (void)snprintf(text, sizeof(text), "unexpected %s", quote);
    d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_UNEXPECTED,
                          _token->offset, _token->length, text);
    d_internal_expr_advance(_p);

    return;
}

/**
 * @brief Reads an infix operator, or the juxtaposition operator, after an
 *        operand.
 *
 * @param[in,out] _p           the parser.
 * @param[in]     _op          the operator.
 * @param[in]     _token       its token; for juxtaposition, the token that
 *                             begins the next operand, which is not taken.
 * @param[in]     _juxtaposed  whether the operator has no token.
 */
static void
d_internal_expr_infix(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_op*    _op,
    const struct dj_parse_expr_token* _token,
    bool                              _juxtaposed
)
{
    const int decided = d_internal_expr_reduce_for(_p, _op, false, _token);
    const uint32_t length = (_juxtaposed) ? 0u : _token->length;

    // check if the parse could go on
    if (_p->stopped)
    {
        return;
    }

    // a chain grows by a link; a flattening run by nothing but its operand
    if (decided == D_INTERNAL_EXPR_EXTEND)
    {
        if ( (_p->frames[_p->frame_count - 1u].kind ==
              D_INTERNAL_EXPR_FRAME_CHAIN) &&
             (!d_internal_expr_push_link(_p, _op, _token->offset, length)) )
        {
            return;
        }
    }
    else
    {
        struct dj_internal_expr_frame f;

        memset(&f, 0, sizeof(f));
        f.op           = _op;
        f.kind         = (_op->assoc == D_PARSE_EXPR_ASSOC_CHAIN)
                         ? D_INTERNAL_EXPR_FRAME_CHAIN
                         : ( ((_op->flags & D_PARSE_EXPR_OP_FLATTEN) != 0u)
                             ? D_INTERNAL_EXPR_FRAME_FLAT
                             : D_INTERNAL_EXPR_FRAME_INFIX );
        f.flags        = (_juxtaposed) ? D_INTERNAL_EXPR_FRAME_JUXTAPOSED
                                       : 0u;
        f.assoc        = _op->assoc;
        f.precedence   = _op->precedence;
        f.base         = _p->operand_count - 1u;
        f.links        = _p->link_count;
        f.offset       = _p->starts[_p->operand_count - 1u];
        f.token_offset = _token->offset;
        f.token_length = length;

        if ( (!d_internal_expr_push_frame(_p, &f)) ||
             ( (f.kind == D_INTERNAL_EXPR_FRAME_CHAIN) &&
               (!d_internal_expr_push_link(_p, _op, _token->offset,
                                           length)) ) )
        {
            return;
        }
    }

    // the operator's token is taken; juxtaposition's is the next operand's
    if (!_juxtaposed)
    {
        d_internal_expr_advance(_p);
    }

    _p->position = D_PARSE_EXPR_OPERAND;

    return;
}

/**
 * @brief Reads a postfix operator, or a mixfix one with a lead, after an
 *        operand.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _op     the operator.
 * @param[in]     _token  its token.
 */
static void
d_internal_expr_postfix(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_op*    _op,
    const struct dj_parse_expr_token* _token
)
{
    (void)d_internal_expr_reduce_for(_p, _op, true, _token);

    // check if the parse could go on
    if (_p->stopped)
    {
        return;
    }

    const uint32_t top = _p->operand_count - 1u;

    // a postfix operator is whole at once
    if (_op->form == D_PARSE_EXPR_FORM_POSTFIX)
    {
        if (d_internal_expr_build(_p, _op->id, top, 0u, _p->starts[top],
                                  _token->offset + _token->length))
        {
            d_internal_expr_advance(_p);
        }

        return;
    }

    struct dj_internal_expr_frame f;

    memset(&f, 0, sizeof(f));
    f.op           = _op;
    f.kind         = D_INTERNAL_EXPR_FRAME_MIXFIX;
    f.part         = 1u;
    f.assoc        = _op->assoc;
    f.precedence   = _op->precedence;
    f.base         = top;
    f.row_base     = _p->operand_count;
    f.links        = _p->link_count;
    f.offset       = _p->starts[top];
    f.token_offset = _token->offset;
    f.token_length = _token->length;

    // open it, or stop
    if (d_internal_expr_push_frame(_p, &f))
    {
        d_internal_expr_advance(_p);
        _p->position = D_PARSE_EXPR_OPERAND;
    }

    return;
}

/**
 * @brief Whether the operand on top may be called: an ungrouped leaf of a
 *        callable class.
 *
 * @param[in] _p  the parser.
 * @return true where it may.
 */
static bool
d_internal_expr_callable(
    const struct dj_parse_expr_parser* _p
)
{
    const struct dj_parse_expr_node* node =
        &_p->term->nodes[_p->nodes[_p->operand_count - 1u]];
    uint32_t                         count = 0u;

    // a leaf, not written in brackets
    if ( (node->kind != D_PARSE_EXPR_NODE_LEAF) ||
         ((node->flags & D_PARSE_EXPR_NODE_GROUPED) != 0u) )
    {
        return false;
    }

    const struct dj_parse_expr_role* roles =
        d_parse_expr_find_roles(_p->sig, node->op, D_PARSE_EXPR_OPERAND,
                                &count);

    for (uint32_t i = 0u; i < count; ++i)
    {
        if ( (roles[i].kind == D_PARSE_EXPR_ROLE_ATOM) &&
             ((roles[i].flags & D_PARSE_EXPR_ROLE_CALLABLE) != 0u) )
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Asks a token's operator-position roles.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _token  the token.
 * @return true where a role read it.
 */
static bool
d_internal_expr_operator_roles(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_token* _token
)
{
    uint32_t                         count = 0u;
    const struct dj_parse_expr_role* roles =
        d_parse_expr_find_roles(_p->sig, _token->cls, D_PARSE_EXPR_OPERATOR,
                                &count);

    for (uint32_t i = 0u; i < count; ++i)
    {
        const struct dj_parse_expr_role* role = &roles[i];

        if ( (!d_internal_expr_role_active(_p, role, _token))          ||
             ( ((role->flags & D_PARSE_EXPR_ROLE_CALLEE) != 0u) &&
               (!d_internal_expr_callable(_p)) ) )
        {
            continue;
        }

        if (role->kind == D_PARSE_EXPR_ROLE_CUSTOM)
        {
            const struct dj_parse_expr_op* bound =
                d_parse_expr_find_op(_p->sig, role->op);

            // a role that names its operator is given the operand that
            // operator would take: `$i++` hands `++` the `$i`
            if (bound)
            {
                (void)d_internal_expr_reduce_for(
                          _p, bound, (bound->form != D_PARSE_EXPR_FORM_INFIX),
                          _token);
            }

            if ( (_p->stopped) ||
                 (d_internal_expr_run_custom(
                      _p, role, _p->nodes[_p->operand_count - 1u]) != 0) )
            {
                return true;
            }

            continue;
        }

        const struct dj_parse_expr_op* op =
            d_parse_expr_find_op(_p->sig, role->op);

        if (op->form == D_PARSE_EXPR_FORM_INFIX)
        {
            d_internal_expr_infix(_p, op, _token, false);
        }
        else
        {
            d_internal_expr_postfix(_p, op, _token);
        }

        return true;
    }

    return false;
}

/**
 * @brief Reads a token with no role after an operand as the start of the
 *        next, joined by the juxtaposition operator, where the signature has
 *        one and allows it here.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _token  the token.
 * @return true where it did.
 */
static bool
d_internal_expr_juxtapose(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_token* _token
)
{
    uint32_t count = 0u;
    bool     begins = false;

    // the signature must have the operator
    if (_p->sig->juxtapose == D_PARSE_EXPR_OP_NONE)
    {
        return false;
    }

    const struct dj_parse_expr_role* roles =
        d_parse_expr_find_roles(_p->sig, _token->cls, D_PARSE_EXPR_OPERAND,
                                &count);

    // the token must begin an operand, and not as a word operator
    for (uint32_t i = 0u; ( (i < count) && (!begins) ); ++i)
    {
        const struct dj_parse_expr_op* op =
            (roles[i].kind == D_PARSE_EXPR_ROLE_OP)
            ? d_parse_expr_find_op(_p->sig, roles[i].op)
            : NULL;

        begins = ( (d_internal_expr_role_active(_p, &roles[i], _token))   &&
                   ((roles[i].flags & D_PARSE_EXPR_ROLE_NO_JUXTAPOSE) == 0u) &&
                   ( (!op) ||
                     ((op->flags & D_PARSE_EXPR_OP_WORD) == 0u) ) );
    }

    // and the language must agree
    if ( (!begins) ||
         ( (_p->sig->may_juxtapose) &&
           (!_p->sig->may_juxtapose(_p->sig->juxtapose_ctx, _p->term,
                                    _p->nodes[_p->operand_count - 1u],
                                    _token)) ) )
    {
        return false;
    }

    d_internal_expr_infix(_p,
                          d_parse_expr_find_op(_p->sig, _p->sig->juxtapose),
                          _token,
                          true);

    return true;
}

/**
 * @brief Reports a close of an outer operator and closes what is open
 *        inside it.
 *
 * @param[in,out] _p       the parser.
 * @param[in]     _m       the innermost open mixfix frame.
 * @param[in]     _target  the outer one the token closes.
 * @param[in]     _token   the token, left to be read again.
 */
static void
d_internal_expr_mismatched(
    struct dj_parse_expr_parser*      _p,
    uint32_t                          _m,
    uint32_t                          _target,
    const struct dj_parse_expr_token* _token
)
{
    char quote[48];
    char text[128];

    d_internal_expr_quote(_p, _token->offset, _token->length, quote,
                          sizeof(quote));
    (void)snprintf(text, sizeof(text),
                   "%s closes an outer operator while an inner one is open",
                   quote);
    d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_MISMATCHED,
                          _token->offset, _token->length, text);
    d_internal_expr_note(_p, (uint16_t)D_PARSE_EXPR_DIAG_MISMATCHED,
                         _p->frames[_m].token_offset,
                         _p->frames[_m].token_length,
                         "the innermost open operator begins here");

    // close everything inside the operator the token closes
    while ( (!_p->stopped) &&
            (_p->frame_count > _target + 1u) )
    {
        const uint32_t inner = d_internal_expr_innermost(_p);

        d_internal_expr_reduce_above(_p, inner);

        if (!_p->stopped)
        {
            d_internal_expr_force_close(_p, _token->offset);
        }
    }

    return;
}

/**
 * @brief The index of an open mixfix frame, below the innermost, whose next
 *        part a token is.
 *
 * @param[in] _p      the parser.
 * @param[in] _m      the innermost open mixfix frame.
 * @param[in] _class  the token's class.
 * @return its index, or D_PARSE_EXPR_NO_NODE.
 */
static uint32_t
d_internal_expr_outer_part(
    const struct dj_parse_expr_parser* _p,
    uint32_t                           _m,
    uint32_t                           _class
)
{
    for (uint32_t i = _m; i > 0u; --i)
    {
        const struct dj_internal_expr_frame* f = &_p->frames[i - 1u];

        if ( (f->kind == D_INTERNAL_EXPR_FRAME_MIXFIX) &&
             (f->op->mixfix.part[f->part] == _class) )
        {
            return i - 1u;
        }
    }

    return D_PARSE_EXPR_NO_NODE;
}

/**
 * @brief Reads the end of the input after an operand: closes everything,
 *        reporting the innermost operator never closed.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _token  the end.
 */
static void
d_internal_expr_finish(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_token* _token
)
{
    bool reported = false;

    while ( (!_p->stopped) &&
            (_p->frame_count > 0u) )
    {
        const uint32_t m = d_internal_expr_innermost(_p);

        // no bracket open: every frame reduces
        if (m == D_PARSE_EXPR_NO_NODE)
        {
            (void)d_internal_expr_reduce_top(_p);

            continue;
        }

        d_internal_expr_reduce_above(_p, m);

        if ( (!reported) &&
             (!_p->stopped) )
        {
            char quote[48];
            char text[96];

            d_internal_expr_quote(_p, _p->frames[m].token_offset,
                                  _p->frames[m].token_length, quote,
                                  sizeof(quote));
            (void)snprintf(text, sizeof(text), "%s is never closed", quote);
            d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_UNCLOSED,
                                  _p->frames[m].token_offset,
                                  _p->frames[m].token_length, text);
            d_internal_expr_note(_p, (uint16_t)D_PARSE_EXPR_DIAG_UNCLOSED,
                                 _token->offset, 0u,
                                 "the expression ends here");
            reported = true;
        }

        if (!_p->stopped)
        {
            d_internal_expr_force_close(_p, _token->offset);
        }
    }

    _p->stopped = true;

    return;
}

/**
 * @brief Reports a closing part of another kind than the innermost open
 *        operator awaits, and closes that operator with it.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _m      the innermost open mixfix frame.
 * @param[in]     _token  the closing part, taken.
 * @param[in]     _quote  its text, quoted.
 */
static void
d_internal_expr_wrong_close(
    struct dj_parse_expr_parser*      _p,
    uint32_t                          _m,
    const struct dj_parse_expr_token* _token,
    const char*                       _quote
)
{
    char text[128];

    (void)snprintf(text, sizeof(text),
                   "%s does not close the innermost open operator", _quote);
    d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_MISMATCHED,
                          _token->offset, _token->length, text);
    d_internal_expr_note(_p, (uint16_t)D_PARSE_EXPR_DIAG_MISMATCHED,
                         _p->frames[_m].token_offset,
                         _p->frames[_m].token_length,
                         "the innermost open operator begins here");
    d_internal_expr_reduce_above(_p, _m);

    // the slip is read as the close it should have been
    if (!_p->stopped)
    {
        d_internal_expr_force_close(_p, _token->offset + _token->length);
        d_internal_expr_advance(_p);
    }

    return;
}

/**
 * @brief Reads a token after an operand that no role reads and that does not
 *        juxtapose: an outer operator's part, the end, the end of an
 *        expression that stops, a closing part, or a stray.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _m      the innermost open mixfix frame, or
 *                        D_PARSE_EXPR_NO_NODE.
 * @param[in]     _token  the token.
 */
static void
d_internal_expr_no_role(
    struct dj_parse_expr_parser*      _p,
    uint32_t                          _m,
    const struct dj_parse_expr_token* _token
)
{
    const uint32_t target = (_m != D_PARSE_EXPR_NO_NODE)
                            ? d_internal_expr_outer_part(_p, _m, _token->cls)
                            : D_PARSE_EXPR_NO_NODE;
    const bool     closes = d_internal_expr_is_structural(_p, _token->cls,
                                                          true);
    char           quote[48];
    char           text[96];

    if (target != D_PARSE_EXPR_NO_NODE)
    {
        d_internal_expr_mismatched(_p, _m, target, _token);

        return;
    }

    // the end; or, where the expression stops, a token it cannot go on with,
    // unless it closes something of another kind inside a bracket
    if ( (_token->cls == D_PARSE_EXPR_CLASS_END)                    ||
         ( ((_p->flags & D_PARSE_EXPR_OPTION_STOP) != 0u) &&
           ( (_m == D_PARSE_EXPR_NO_NODE) || (!closes) ) ) )
    {
        d_internal_expr_finish(_p, _token);

        return;
    }

    d_internal_expr_quote(_p, _token->offset, _token->length, quote,
                          sizeof(quote));

    // a closing part of another kind closes the innermost open operator: one
    // slip, one error
    if ( (_m != D_PARSE_EXPR_NO_NODE) &&
         (closes) )
    {
        d_internal_expr_wrong_close(_p, _m, _token, quote);

        return;
    }

    if (closes)
    {
        (void)snprintf(text, sizeof(text), "%s closes nothing that is open",
                       quote);
        d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_UNOPENED,
                              _token->offset, _token->length, text);
    }
    else
    {
        (void)snprintf(text, sizeof(text), "unexpected %s after an operand",
                       quote);
        d_internal_expr_error(_p, (uint16_t)D_PARSE_EXPR_DIAG_UNEXPECTED,
                              _token->offset, _token->length, text);
    }

    d_internal_expr_advance(_p);

    return;
}

/**
 * @brief Reads a token in operator position.
 *
 * @param[in,out] _p      the parser.
 * @param[in]     _token  the token.
 */
static void
d_internal_expr_step_operator(
    struct dj_parse_expr_parser*      _p,
    const struct dj_parse_expr_token* _token
)
{
    const uint32_t m = d_internal_expr_innermost(_p);

    // the innermost open operator's next part, or its list's separators
    if (m != D_PARSE_EXPR_NO_NODE)
    {
        const struct dj_internal_expr_frame* f  = &_p->frames[m];
        const struct dj_parse_expr_mixfix*   mx = &f->op->mixfix;
        const bool in_list = ( (f->part == 1u) && (mx->list_hole) );

        if (_token->cls == mx->part[f->part])
        {
            d_internal_expr_next_part(_p, m, _token);

            return;
        }

        if ( (in_list)                                          &&
             (_token->cls != D_PARSE_EXPR_CLASS_END)            &&
             ( (_token->cls == mx->separator) ||
               (_token->cls == mx->row_separator) ) )
        {
            d_internal_expr_separator(_p, m, _token,
                                      (_token->cls != mx->separator));

            return;
        }
    }

    // its own roles, then juxtaposition, then what has none
    if ( (!d_internal_expr_operator_roles(_p, _token)) &&
         (!d_internal_expr_juxtapose(_p, _token)) )
    {
        d_internal_expr_no_role(_p, m, _token);
    }

    return;
}


//==============================================================================
// 8.  THE ENTRIES
//==============================================================================


/**
 * @brief Gives a parser its settings, each option over its knob.
 *
 * @param[out] _p        the parser, zeroed.
 * @param[in]  _options  the options; may be NULL.
 */
static void
d_internal_expr_settings(
    struct dj_parse_expr_parser*        _p,
    const struct dj_parse_expr_options* _options
)
{
    struct dj_parse_expr_options none;

    memset(&none, 0, sizeof(none));

    const struct dj_parse_expr_options* o = (_options) ? _options : &none;

    _p->max_depth   = (o->max_depth != 0u)
                      ? o->max_depth
                      : (uint32_t)D_CFG_PARSE_EXPR_MAX_DEPTH;
    _p->max_errors  = (o->max_errors != 0u)
                      ? o->max_errors
                      : (uint32_t)D_CFG_PARSE_EXPR_MAX_ERRORS;
    _p->order       = (o->error_order != D_PARSE_EXPR_ORDER_DEFAULT)
                      ? o->error_order
                      : (uint8_t)D_CFG_PARSE_EXPR_ERROR_ORDER;
    _p->flags       = o->flags;
    _p->mode        = (o->mode != 0u) ? o->mode : 1u;
    _p->source_id   = o->source_id;
    _p->text        = o->text;
    _p->text_length = (o->text) ? o->text_length : 0u;
    _p->check       = o->check;
    _p->check_ctx   = o->check_ctx;

    return;
}

/**
 * @brief Starts a parser at its first token: in operand position, its stacks
 *        in their own rooms, no operator yet to blame.
 *
 * @param[out] _p  the parser.
 */
static void
d_internal_expr_rooms(
    struct dj_parse_expr_parser* _p
)
{
    _p->blame            = D_PARSE_EXPR_NO_NODE;
    _p->position         = D_PARSE_EXPR_OPERAND;
    _p->nodes            = _p->local_nodes;
    _p->starts           = _p->local_starts;
    _p->ends             = _p->local_ends;
    _p->operand_capacity = D_INTERNAL_EXPR_LOCAL_OPERANDS;
    _p->frames           = _p->local_frames;
    _p->frame_capacity   = D_INTERNAL_EXPR_LOCAL_FRAMES;
    _p->links            = _p->local_links;
    _p->link_capacity    = D_INTERNAL_EXPR_LOCAL_LINKS;

    return;
}

/**
 * @brief Frees whatever the parser's stacks took from the heap.
 *
 * @param[in,out] _p  the parser.
 */
static void
d_internal_expr_release_stacks(
    struct dj_parse_expr_parser* _p
)
{
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    // each stack that left the parser's own array
    if (_p->nodes != _p->local_nodes)
    {
        free(_p->nodes);
    }

    if (_p->starts != _p->local_starts)
    {
        free(_p->starts);
    }

    if (_p->ends != _p->local_ends)
    {
        free(_p->ends);
    }

    if (_p->frames != _p->local_frames)
    {
        free(_p->frames);
    }

    if (_p->links != _p->local_links)
    {
        free(_p->links);
    }
#else
    (void)_p;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    return;
}

/*
d_parse_expr_parse
  The loop reads the token at hand in the position the parse is in, until the
end has closed everything or the parse is stopped (its errors' allowance
spent, or a fatal failure). The parser itself, its stacks' first rooms
included, lives in this frame: a parse of what a person writes allocates
nothing but the term's nodes.
*/
int
d_parse_expr_parse(
    const struct dj_parse_expr_signature* _signature,
    struct dj_parse_expr_source*          _source,
    const struct dj_parse_expr_options*   _options,
    struct dj_parse_expr_term*            _term,
    struct dj_parse_diag_sink*            _sink,
    struct dj_parse_expr_result*          _result
)
{
    // a result to say what happened in
    if (!_result)
    {
        return D_PARSE_EXPR_FATAL;
    }

    memset(_result, 0, sizeof(*_result));
    _result->root       = D_PARSE_EXPR_NO_NODE;
    _result->diag_index = 0xFFFFFFFFu;
    _result->status     = D_PARSE_EXPR_FATAL;

    struct dj_parse_expr_parser p;

    memset(&p, 0, sizeof(p));
    d_internal_expr_settings(&p, _options);

    // a sealed signature, a source, a term, a guard a node can keep, and an
    // allowance of errors in its bound
    if ( (!_signature)                                                  ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) == 0u)   ||
         (!_source)                                                     ||
         (!_source->next)                                               ||
         (!_term)                                                       ||
         ( (p.max_depth > D_PARSE_EXPR_NODE_DEPTH_MAX) &&
           (p.max_depth != D_PARSE_EXPR_DEPTH_UNBOUNDED) )              ||
         (p.max_errors > D_PARSE_EXPR_ERRORS_MAX) )
    {
        return D_PARSE_EXPR_FATAL;
    }

    p.sig    = _signature;
    p.source = _source;
    p.term   = _term;
    p.sink   = _sink;
    p.result = _result;
    d_internal_expr_rooms(&p);

    const uint32_t floor = p.operand_count;

    while (!p.stopped)
    {
        const struct dj_parse_expr_token token =
            *d_internal_expr_current(&p, p.position);

        if (p.stopped)
        {
            break;
        }

        if (p.position == D_PARSE_EXPR_OPERAND)
        {
            d_internal_expr_step_operand(&p, &token);
        }
        else
        {
            d_internal_expr_step_operator(&p, &token);
        }
    }

    // one operand at the floor is the whole expression
    if ( (!p.fatal) &&
         (p.operand_count == floor + 1u) &&
         (p.frame_count == 0u) )
    {
        _result->root = p.nodes[floor];
        _term->root   = _result->root;
    }

    // the token the parse ended at, unread, for whatever reads on
    if (p.has_current)
    {
        _result->next = p.current;
    }

    _result->status = (p.fatal)                ? D_PARSE_EXPR_FATAL
                    : (_result->errors > 0u)   ? D_PARSE_EXPR_ERROR
                    : (_result->root != D_PARSE_EXPR_NO_NODE)
                                               ? D_PARSE_EXPR_OK
                                               : D_PARSE_EXPR_FATAL;

    d_internal_expr_release_stacks(&p);

    return _result->status;
}

/*
d_parse_expr_peek
  A hook's look ahead is the parser's second slot: the main loop takes from it
before it reads the source again, so nothing peeked is lost. The slot keeps
the mode its token was read in, since the source cannot read it again.
*/
int
d_parse_expr_peek(
    struct dj_parse_expr_parser* _parser,
    uint8_t                      _position,
    uint32_t                     _mode,
    struct dj_parse_expr_token*  _out
)
{
    // a parser and somewhere to put the token
    if ( (!_parser) ||
         (!_out) )
    {
        return -1;
    }

    const uint32_t mode = (_mode != 0u) ? _mode : _parser->mode;

    // a token read in one mode is not the next in another
    if ( (_parser->has_peeked) &&
         (_parser->peeked_mode != mode) )
    {
        return -1;
    }

    if (!_parser->has_peeked)
    {
        memset(&_parser->peeked, 0, sizeof(_parser->peeked));
        _parser->peeked_mode = mode;

        const int got = _parser->source->next(_parser->source->ctx,
                                              _position,
                                              mode,
                                              &_parser->peeked);

        // a failing source stops the parse
        if (got < 0)
        {
            _parser->fatal   = true;
            _parser->stopped = true;

            return -1;
        }

        if (got == 0)
        {
            _parser->peeked.cls    = D_PARSE_EXPR_CLASS_END;
            _parser->peeked.length = 0u;
        }

        _parser->has_peeked = true;
    }

    *_out = _parser->peeked;

    return (_out->cls == D_PARSE_EXPR_CLASS_END) ? 0 : 1;
}

/*
d_parse_expr_take
  A peek, then the slot emptied; the end stays in the slot, so the main loop
reads it.
*/
int
d_parse_expr_take(
    struct dj_parse_expr_parser* _parser,
    uint8_t                      _position,
    uint32_t                     _mode,
    struct dj_parse_expr_token*  _out
)
{
    const int got = d_parse_expr_peek(_parser, _position, _mode, _out);

    // the end is never taken
    if (got > 0)
    {
        _parser->has_peeked = false;
    }

    return got;
}

/*
d_parse_expr_parser_term
  The term is the parser's caller's; a hook builds into it with term.h.
*/
struct dj_parse_expr_term*
d_parse_expr_parser_term(
    struct dj_parse_expr_parser* _parser
)
{
    return (_parser) ? _parser->term : NULL;
}

/*
d_parse_expr_report
  Counted exactly as the parser's own errors are, so the result's error may be
the hook's.
*/
void
d_parse_expr_report(
    struct dj_parse_expr_parser* _parser,
    uint16_t                     _domain,
    uint16_t                     _code,
    uint32_t                     _offset,
    uint32_t                     _length,
    const char*                  _message
)
{
    // a parser to report through
    if (!_parser)
    {
        return;
    }

    const uint32_t before = (_parser->sink) ? _parser->sink->count : 0u;

    (void)d_parse_diag_emit(_parser->sink,
                            D_PARSE_SEVERITY_ERROR,
                            _domain,
                            _code,
                            d_parse_span_located(_parser->source_id,
                                                 _offset, _length, 0u, 0u),
                            _message);
    d_internal_expr_count_error(
        _parser,
        _domain,
        _code,
        _offset,
        _length,
        ( (_parser->sink) && (_parser->sink->count > before) )
        ? before
        : 0xFFFFFFFFu);

    return;
}


//==============================================================================
// 9.  SOURCES AND PLACES
//==============================================================================


/**
 * @brief The array source's `next`: the tokens in turn, then the end.
 *
 * @param[in,out] _ctx       the source's state.
 * @param[in]     _position  unused: the tokens are lexed already.
 * @param[in]     _mode      unused.
 * @param[out]    _out       the token.
 * @return 1 for a token, 0 at the end.
 */
static int
d_internal_expr_array_next(
    void*                       _ctx,
    uint8_t                     _position,
    uint32_t                    _mode,
    struct dj_parse_expr_token* _out
)
{
    struct dj_parse_expr_array_source* state =
        (struct dj_parse_expr_array_source*)_ctx;

    (void)_position;
    (void)_mode;

    // the tokens in turn
    if (state->next < state->count)
    {
        *_out = state->tokens[state->next++];

        return 1;
    }

    memset(_out, 0, sizeof(*_out));
    _out->cls    = D_PARSE_EXPR_CLASS_END;
    _out->offset = state->end;

    return 0;
}

/*
d_parse_expr_source_array
  The state is the caller's, so a source over an array allocates nothing.
*/
void
d_parse_expr_source_array(
    struct dj_parse_expr_source*       _source,
    struct dj_parse_expr_array_source* _state,
    const struct dj_parse_expr_token*  _tokens,
    uint32_t                           _count,
    uint32_t                           _end
)
{
    // both are required
    if ( (!_source) ||
         (!_state) )
    {
        return;
    }

    _state->tokens = _tokens;
    _state->count  = (_tokens) ? _count : 0u;
    _state->next   = 0u;
    _state->end    = _end;
    _source->next  = d_internal_expr_array_next;
    _source->ctx   = _state;

    return;
}

/*
d_parse_expr_locate
  One pass to the offset. A `\r` before a `\n` is not a break of its own, so
`\r\n` counts once; a column counts the bytes that begin a UTF-8 sequence,
which is to say the code points.
*/
void
d_parse_expr_locate(
    const char* _text,
    size_t      _length,
    size_t      _offset,
    uint32_t*   _line,
    uint32_t*   _column
)
{
    uint32_t     line   = 1u;
    uint32_t     column = 1u;
    const size_t end    = (_offset < _length) ? _offset : _length;

    for (size_t i = 0u; ( (_text) && (i < end) ); ++i)
    {
        const unsigned char c = (unsigned char)_text[i];

        // a break: \n, a lone \r, and \r\n once
        if ( (c == '\n') ||
             ( (c == '\r') &&
               ( (i + 1u >= _length) || (_text[i + 1u] != '\n') ) ) )
        {
            ++line;
            column = 1u;
        }
        else if ( (c != '\r') &&
                  ((c & 0xC0u) != 0x80u) )
        {
            ++column;
        }
    }

    if (_line)
    {
        *_line = line;
    }

    if (_column)
    {
        *_column = column;
    }

    return;
}
