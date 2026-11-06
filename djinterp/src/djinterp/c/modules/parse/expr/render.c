/*******************************************************************************
* djinterp [c/parse]                                                    render.c
*
* Expression core v2: definitions for render.h, the renderer.
*   A TASK STACK holds what is still to be written: a node, or a piece of
* text. A node popped is expanded into its pieces, pushed last first, so the
* text comes out in order with no recursion; a chain or a list expands into
* every operand at once.
*   BRACKETS. A child is bracketed where the parser, reading the text, would
* group otherwise: on an operator's left, where the operator arriving after
* the child would not reduce the child's own open operator first; on its
* right, where the child's first operator arriving after the parent's would
* reduce the parent's first. Both are d_internal_expr_decide's answers, the
* rule the parser itself follows.
*   FUSION. A prefix operator's spelling followed by an operator character
* (`- -x`, not `--x`) is separated by a space, so two tokens stay two.
*
*
* path:      /src/djinterp/c/modules/parse/expr/render.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/parse/expr/render.h"  // corresponding header

#if (D_INTERNAL_PARSE_EXPR_RENDER == 1)

// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, strlen, strchr
// djinterp
#include "../../../../../../inc/djinterp/c/dckdint.h"  // d_ckd_mul
#include "./expr_internal.h"                           // d_internal_expr_decide


// task kinds: a node; text as given; an infix operator's spelling, spaced;
// a prefix operator's spelling, guarded against fusion
#define D_INTERNAL_RENDER_NODE          1u
#define D_INTERNAL_RENDER_TEXT          2u
#define D_INTERNAL_RENDER_INFIX         3u
#define D_INTERNAL_RENDER_PREFIX        4u

// node task flags: BRACKET, the node is to be bracketed; WRITTEN, its
// brackets are written already, so it is not bracketed again
#define D_INTERNAL_RENDER_BRACKET       0x01u
#define D_INTERNAL_RENDER_WRITTEN       0x02u

// the task stack's own room before it needs the heap
#define D_INTERNAL_RENDER_TASKS   128u

// dj_internal_render_task
//   struct: one thing still to write: a node, with whether it is bracketed,
// or a piece of text, with how it is spaced.
struct dj_internal_render_task
{
    uint8_t     kind;
    uint8_t     flags;
    uint8_t     word;
    uint8_t     reserved;
    uint32_t    node;
    const char* text;
};

// dj_internal_renderer
//   struct: a render in progress.
struct dj_internal_renderer
{
    const struct dj_parse_expr_signature*      sig;
    const struct dj_parse_expr_term*           term;
    const struct dj_parse_expr_render_options* options;
    dj_parse_expr_write_fn                     write;
    void*                                      write_ctx;
    struct dj_internal_render_task*            tasks;
    uint32_t                                   count;
    uint32_t                                   capacity;
    bool                                       failed;
    bool                                       guard;
    const char*                                open;
    const char*                                close;
    struct dj_internal_render_task             local[D_INTERNAL_RENDER_TASKS];
};

// dj_internal_render_side
//   struct: how a node is open on one side to an operator beside it: the
// operator, its frame kind, its precedence and association, and whether it
// arrives as a postfix operator does. `op` is NULL where the node is closed
// on that side.
struct dj_internal_render_side
{
    const struct dj_parse_expr_op* op;
    uint8_t                        kind;
    uint8_t                        assoc;
    bool                           postfix;
    int32_t                        precedence;
};


//==============================================================================
// 1.  WRITING
//==============================================================================


/**
 * @brief Whether a character is one an operator's spelling is made of.
 *
 * @param[in] _c  the character.
 * @return true for the punctuation operators are spelled with.
 */
static bool
d_internal_render_op_char(
    char _c
)
{
    return ( (_c != '\0') &&
             (strchr("+-*/^!'=<>|&~%?:.@#$\\", _c) != NULL) );
}

/**
 * @brief Writes text, separating it from a prefix spelling it would fuse
 *        with.
 *
 * @param[in,out] _r     the renderer.
 * @param[in]     _text  the text; may be NULL or empty.
 */
static void
d_internal_render_emit(
    struct dj_internal_renderer* _r,
    const char*                  _text
)
{
    // nothing to write, or nothing more
    if ( (_r->failed) ||
         (!_text)     ||
         (_text[0] == '\0') )
    {
        return;
    }

    const size_t length = strlen(_text);

    // two operator characters in a row would read as one token
    if ( (_r->guard) &&
         (d_internal_render_op_char(_text[0])) &&
         (_r->write(_r->write_ctx, " ", 1u) != 1u) )
    {
        _r->failed = true;

        return;
    }

    _r->guard = false;

    if (_r->write(_r->write_ctx, _text, length) != length)
    {
        _r->failed = true;
    }

    return;
}

/**
 * @brief Writes a leaf: through the atom writer, as its text, or as its
 *        value.
 *
 * @param[in,out] _r     the renderer.
 * @param[in]     _node  the leaf.
 */
static void
d_internal_render_atom(
    struct dj_internal_renderer* _r,
    uint32_t                     _node
)
{
    const struct dj_parse_expr_node* node = &_r->term->nodes[_node];
    const struct dj_parse_expr_render_options* o = _r->options;
    char                             value[16];
    size_t                           end = 0u;

    // the language's own writer
    if ( (o) && (o->atom) )
    {
        _r->guard = false;

        if (!o->atom(o->atom_ctx, _r->term, _node, _r->write, _r->write_ctx))
        {
            _r->failed = true;
        }

        return;
    }

    // its text, where the text is given and holds its span: an empty span,
    // awk's empty block read raw, is empty text
    if ( (o)                                                           &&
         (o->text)                                                     &&
         (!d_ckd_add(&end, (size_t)node->offset, (size_t)node->length)) &&
         (end <= o->text_length) )
    {
        if (node->length == 0u)
        {
            _r->guard = false;

            return;
        }

        _r->guard = ( (_r->guard) &&
                      (d_internal_render_op_char(o->text[node->offset])) );

        if ( ( (_r->guard) &&
               (_r->write(_r->write_ctx, " ", 1u) != 1u) )              ||
             (_r->write(_r->write_ctx, o->text + node->offset,
                        (size_t)node->length) != (size_t)node->length) )
        {
            _r->failed = true;
        }

        _r->guard = false;

        return;
    }

    (void)snprintf(value, sizeof(value), "%lu", (unsigned long)node->first);
    d_internal_render_emit(_r, value);

    return;
}


//==============================================================================
// 2.  THE TASK STACK
//==============================================================================


/**
 * @brief Pushes a task.
 *
 * @param[in,out] _r      the renderer.
 * @param[in]     _kind   its kind.
 * @param[in]     _node   a node task's node.
 * @param[in]     _flags  a node task's flags.
 * @param[in]     _text   a text task's text.
 * @param[in]     _word   whether a spelled operator is a word.
 */
static void
d_internal_render_push(
    struct dj_internal_renderer* _r,
    uint8_t                      _kind,
    uint32_t                     _node,
    uint8_t                      _flags,
    const char*                  _text,
    bool                         _word
)
{
    // room, or the render fails
    if (_r->count >= _r->capacity)
    {
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
        size_t elements = 0u;
        size_t bytes    = 0u;

        if ( (d_ckd_mul(&elements, (size_t)_r->capacity, 2u))        ||
             (elements > (size_t)0xFFFFFFFFu)                        ||
             (d_ckd_mul(&bytes, elements,
                        sizeof(struct dj_internal_render_task))) )
        {
            _r->failed = true;

            return;
        }

        void* grown = (_r->tasks == _r->local)
                      ? malloc(bytes)
                      : realloc(_r->tasks, bytes);

        if (!grown)
        {
            _r->failed = true;

            return;
        }

        if (_r->tasks == _r->local)
        {
            memcpy(grown, _r->local,
                   (size_t)_r->count * sizeof(struct dj_internal_render_task));
        }

        _r->tasks    = (struct dj_internal_render_task*)grown;
        _r->capacity = (uint32_t)elements;
#else
        _r->failed = true;

        return;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
    }

    _r->tasks[_r->count].kind     = _kind;
    _r->tasks[_r->count].flags    = _flags;
    _r->tasks[_r->count].word     = (_word) ? 1u : 0u;
    _r->tasks[_r->count].reserved = 0u;
    _r->tasks[_r->count].node     = _node;
    _r->tasks[_r->count].text     = _text;
    ++_r->count;

    return;
}

/**
 * @brief Pushes a node to be written, bracketed or not.
 *
 * @param[in,out] _r        the renderer.
 * @param[in]     _node     the node.
 * @param[in]     _bracket  whether to bracket it.
 */
static void
d_internal_render_push_node(
    struct dj_internal_renderer* _r,
    uint32_t                     _node,
    bool                         _bracket
)
{
    d_internal_render_push(_r,
                           D_INTERNAL_RENDER_NODE,
                           _node,
                           (_bracket) ? D_INTERNAL_RENDER_BRACKET : 0u,
                           NULL,
                           false);

    return;
}

/**
 * @brief Pushes text to be written as given.
 *
 * @param[in,out] _r     the renderer.
 * @param[in]     _text  the text.
 */
static void
d_internal_render_push_text(
    struct dj_internal_renderer* _r,
    const char*                  _text
)
{
    d_internal_render_push(_r, D_INTERNAL_RENDER_TEXT, 0u, 0u, _text, false);

    return;
}


//==============================================================================
// 3.  BRACKETS
//==============================================================================


/**
 * @brief The operator a node's text begins with an operand of, as the
 *        parser meets it after that operand: its left side.
 *
 * @param[in]  _r     the renderer.
 * @param[in]  _node  the node.
 * @param[out] _side  its left side; `op` NULL where it is closed.
 */
static void
d_internal_render_left_side(
    const struct dj_internal_renderer* _r,
    uint32_t                           _node,
    struct dj_internal_render_side*    _side
)
{
    const struct dj_parse_expr_node* node = &_r->term->nodes[_node];
    const struct dj_parse_expr_op*   op   =
        (node->kind == D_PARSE_EXPR_NODE_APPLY)
        ? d_parse_expr_find_op(_r->sig, node->op)
        : NULL;

    memset(_side, 0, sizeof(*_side));

    // a chain's join is open as its first link is
    while ( (op) &&
            ((node->flags & D_PARSE_EXPR_NODE_CHAIN) != 0u) )
    {
        node = &_r->term->nodes[_r->term->args[node->first]];
        op   = d_parse_expr_find_op(_r->sig, node->op);
    }

    // a leaf, a hole, a prefix operator and a mixfix one with no lead begin
    // with a token, and are closed on the left
    if ( (!op)                                              ||
         ((node->flags & D_PARSE_EXPR_NODE_ROWS) != 0u)     ||
         (op->form == D_PARSE_EXPR_FORM_PREFIX)             ||
         ( (op->form == D_PARSE_EXPR_FORM_MIXFIX) &&
           (!op->mixfix.lead) ) )
    {
        return;
    }

    _side->op         = op;
    _side->precedence = op->precedence;
    _side->assoc      = op->assoc;
    _side->postfix    = (op->form != D_PARSE_EXPR_FORM_INFIX);

    return;
}

/**
 * @brief The operator a node's text ends in the operand of, as the parser
 *        holds it open when the next operator arrives: its right side.
 *
 * @param[in]  _r     the renderer.
 * @param[in]  _node  the node.
 * @param[out] _side  its right side; `op` NULL where it is closed.
 */
static void
d_internal_render_right_side(
    const struct dj_internal_renderer* _r,
    uint32_t                           _node,
    struct dj_internal_render_side*    _side
)
{
    const struct dj_parse_expr_node* node = &_r->term->nodes[_node];
    const struct dj_parse_expr_op*   op   =
        (node->kind == D_PARSE_EXPR_NODE_APPLY)
        ? d_parse_expr_find_op(_r->sig, node->op)
        : NULL;
    bool                             chain = false;

    memset(_side, 0, sizeof(*_side));

    // a chain's join is open as its links' operator is
    while ( (op) &&
            ((node->flags & D_PARSE_EXPR_NODE_CHAIN) != 0u) )
    {
        node  = &_r->term->nodes[_r->term->args[node->first + 1u]];
        op    = d_parse_expr_find_op(_r->sig, node->op);
        chain = true;
    }

    // a leaf, a hole, a postfix operator and a mixfix one with no tail end
    // with a token, and are closed on the right
    if ( (!op)                                              ||
         ((node->flags & D_PARSE_EXPR_NODE_ROWS) != 0u)     ||
         (op->form == D_PARSE_EXPR_FORM_POSTFIX)            ||
         ( (op->form == D_PARSE_EXPR_FORM_MIXFIX) &&
           (!op->mixfix.tail) ) )
    {
        return;
    }

    _side->op         = op;
    _side->precedence = op->precedence;
    _side->assoc      = op->assoc;
    _side->kind       =
        (op->form == D_PARSE_EXPR_FORM_PREFIX) ? D_INTERNAL_EXPR_FRAME_PREFIX
      : (op->form == D_PARSE_EXPR_FORM_MIXFIX) ? D_INTERNAL_EXPR_FRAME_TAIL
      : ( (chain) || (op->assoc == D_PARSE_EXPR_ASSOC_CHAIN) )
                                               ? D_INTERNAL_EXPR_FRAME_CHAIN
      : ((op->flags & D_PARSE_EXPR_OP_FLATTEN) != 0u)
                                               ? D_INTERNAL_EXPR_FRAME_FLAT
                                               : D_INTERNAL_EXPR_FRAME_INFIX;

    return;
}

/**
 * @brief The child a node's text ends with an operand of: its last child,
 *        or a chain's last link's right operand.
 *
 * @param[in] _r     the renderer.
 * @param[in] _node  an application open on the right.
 * @return the child, or D_PARSE_EXPR_NO_NODE where there is none.
 */
static uint32_t
d_internal_render_last_child(
    const struct dj_internal_renderer* _r,
    uint32_t                           _node
)
{
    const struct dj_parse_expr_node* node = &_r->term->nodes[_node];

    // a chain ends with its last link's right operand
    while ((node->flags & D_PARSE_EXPR_NODE_CHAIN) != 0u)
    {
        node = &_r->term->nodes[_r->term->args[node->first + node->count -
                                               1u]];
    }

    return ( (node->kind == D_PARSE_EXPR_NODE_APPLY) &&
             (node->count > 0u) )
           ? _r->term->args[node->first + node->count - 1u]
           : D_PARSE_EXPR_NO_NODE;
}

/**
 * @brief Whether a child is bracketed where it stands last in its parent,
 *        by the parent's own operator alone: the check that ends a walk
 *        down an open edge.
 *
 * @param[in] _r       the renderer.
 * @param[in] _parent  the parent, open on the right.
 * @param[in] _child   its last child.
 * @return true where the parent's operator alone would bracket it.
 *
 * It asks only the child's top operator. Where the full rule would bracket
 * a child this does not, a walk goes on past brackets that are there, and may
 * add a pair more than needed; it never leaves one out.
 */
static bool
d_internal_render_closes_right(
    const struct dj_internal_renderer* _r,
    uint32_t                           _parent,
    uint32_t                           _child
)
{
    struct dj_internal_render_side parent;
    struct dj_internal_render_side side;

    d_internal_render_right_side(_r, _parent, &parent);
    d_internal_render_left_side(_r, _child, &side);

    return ( (parent.op) &&
             (side.op)   &&
             (d_internal_expr_decide(parent.kind, parent.precedence,
                                     parent.assoc, parent.op, side.op,
                                     side.postfix) !=
              D_INTERNAL_EXPR_STOP) );
}

/**
 * @brief Whether a child on an operator's left needs brackets: whether the
 *        operator, arriving after it, would stop above any operator still
 *        open at the child's right edge, rather than reduce them all first.
 *
 * @param[in] _r         the renderer.
 * @param[in] _op        the operator after the child.
 * @param[in] _postfix   whether it arrives as a postfix operator does.
 * @param[in] _child     the child.
 * @return true where it would.
 *
 * Every operator on the child's right edge is open on the parser's stack
 * when the arriving one comes -- `+not sin` leaves `not` open as well as
 * `+` -- so the walk asks each, innermost last, down to a closed node or a
 * bracketed one.
 */
static bool
d_internal_render_left_needs(
    const struct dj_internal_renderer* _r,
    const struct dj_parse_expr_op*     _op,
    bool                               _postfix,
    uint32_t                           _child
)
{
    uint32_t node = _child;

    while (node != D_PARSE_EXPR_NO_NODE)
    {
        struct dj_internal_render_side side;

        d_internal_render_right_side(_r, node, &side);

        // a closed edge: nothing open to take the operator
        if (!side.op)
        {
            return false;
        }

        if (d_internal_expr_decide(side.kind, side.precedence, side.assoc,
                                   side.op, _op, _postfix) !=
            D_INTERNAL_EXPR_REDUCE)
        {
            return true;
        }

        const uint32_t last = d_internal_render_last_child(_r, node);

        node = ( (last == D_PARSE_EXPR_NO_NODE) ||
                 (d_internal_render_closes_right(_r, node, last)) )
               ? D_PARSE_EXPR_NO_NODE
               : last;
    }

    return false;
}

/**
 * @brief Whether a child on an operator's right needs brackets: whether any
 *        operator at its left edge, arriving with the parent's open, would
 *        reduce the parent first.
 *
 * @param[in] _r           the renderer.
 * @param[in] _kind        the parent's frame kind.
 * @param[in] _op          the parent's operator.
 * @param[in] _precedence  its precedence there.
 * @param[in] _assoc       its association there.
 * @param[in] _child       the child.
 * @return true where one would.
 *
 * The operators of the child's left edge arrive innermost first, each with
 * the parent's frame still open beneath the child's, so each is asked, down
 * to a closed node or a bracketed one.
 */
static bool
d_internal_render_right_needs(
    const struct dj_internal_renderer* _r,
    uint8_t                            _kind,
    const struct dj_parse_expr_op*     _op,
    int32_t                            _precedence,
    uint8_t                            _assoc,
    uint32_t                           _child
)
{
    uint32_t node = _child;

    while (node != D_PARSE_EXPR_NO_NODE)
    {
        struct dj_internal_render_side side;

        d_internal_render_left_side(_r, node, &side);

        // a closed edge: nothing arrives to take the parent's operand
        if (!side.op)
        {
            return false;
        }

        if (d_internal_expr_decide(_kind, _precedence, _assoc, _op, side.op,
                                   side.postfix) != D_INTERNAL_EXPR_STOP)
        {
            return true;
        }

        const struct dj_parse_expr_node* n = &_r->term->nodes[node];

        // down the left edge: a chain's first link's left operand, or the
        // first child, unless the node's operator brackets it
        while ((n->flags & D_PARSE_EXPR_NODE_CHAIN) != 0u)
        {
            n = &_r->term->nodes[_r->term->args[n->first]];
        }

        const uint32_t first = _r->term->args[n->first];
        struct dj_internal_render_side inner;

        d_internal_render_right_side(_r, first, &inner);
        node = ( (inner.op) &&
                 (d_internal_expr_decide(inner.kind, inner.precedence,
                                         inner.assoc, inner.op, side.op,
                                         side.postfix) !=
                  D_INTERNAL_EXPR_REDUCE) )
               ? D_PARSE_EXPR_NO_NODE
               : first;
    }

    return false;
}

/**
 * @brief Whether the token a node begins with has a role in operator
 *        position that a space before it would not turn off.
 *
 * @param[in] _r   the renderer.
 * @param[in] _n   the node, a leaf, a hole, a prefix operator's or a mixfix
 *                 one's with no lead.
 * @param[in] _op  its operator, or NULL for a leaf or a hole.
 * @return true where the parser would read that token as an operator.
 */
static bool
d_internal_render_token_ambiguous(
    const struct dj_internal_renderer* _r,
    const struct dj_parse_expr_node*   _n,
    const struct dj_parse_expr_op*     _op
)
{
    uint32_t cls   = D_PARSE_EXPR_OP_NONE;
    uint32_t count = 0u;

    // the class the node's first token is of
    if (_n->kind == D_PARSE_EXPR_NODE_LEAF)
    {
        cls = _n->op;
    }
    else if ( (_op) &&
              (_op->form == D_PARSE_EXPR_FORM_MIXFIX) )
    {
        cls = _op->mixfix.part[0];
    }
    else if (_op)
    {
        for (uint32_t i = 0u; i < _r->sig->role_count; ++i)
        {
            if ( (_r->sig->roles[i].kind == D_PARSE_EXPR_ROLE_OP)       &&
                 (_r->sig->roles[i].position == D_PARSE_EXPR_OPERAND)   &&
                 (_r->sig->roles[i].op == _op->id) )
            {
                cls = _r->sig->roles[i].cls;

                break;
            }
        }
    }

    const struct dj_parse_expr_role* roles =
        d_parse_expr_find_roles(_r->sig, cls, D_PARSE_EXPR_OPERATOR, &count);

    // a role a preceding space does not turn off
    for (uint32_t i = 0u; i < count; ++i)
    {
        if ((roles[i].flags & D_PARSE_EXPR_ROLE_ADJACENT) == 0u)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Whether an operand juxtaposed by an operator spelled "" begins with
 *        a token the parser would read as an operator after the operand
 *        before it: `a (-1)`, not `a -1`.
 *
 * @param[in] _r      the renderer.
 * @param[in] _child  the right operand.
 * @return true where it does.
 */
static bool
d_internal_render_starts_ambiguous(
    const struct dj_internal_renderer* _r,
    uint32_t                           _child
)
{
    uint32_t node = _child;

    // down the left edge to the node whose token comes first
    for (;;)
    {
        const struct dj_parse_expr_node* n  = &_r->term->nodes[node];
        const struct dj_parse_expr_op*   op =
            (n->kind == D_PARSE_EXPR_NODE_APPLY)
            ? d_parse_expr_find_op(_r->sig, n->op)
            : NULL;

        if ( (op)                                               &&
             ((n->flags & D_PARSE_EXPR_NODE_ROWS) == 0u)        &&
             (op->form != D_PARSE_EXPR_FORM_PREFIX)             &&
             ( (op->form != D_PARSE_EXPR_FORM_MIXFIX) ||
               (op->mixfix.lead) )                              &&
             (n->count > 0u) )
        {
            node = _r->term->args[n->first];

            continue;
        }

        return d_internal_render_token_ambiguous(_r, n, op);
    }
}


//==============================================================================
// 4.  EXPANSION
//==============================================================================


/**
 * @brief Pushes a part's or a separator's spelling, spaced as a word where
 *        it is one.
 *
 * @param[in,out] _r       the renderer.
 * @param[in]     _text    the spelling; NULL writes nothing.
 * @param[in]     _before  whether a word gets a space before it.
 * @param[in]     _after   whether anything gets a space after it.
 *
 * Pushed in reverse, as every expansion is: the stack writes the last
 * pushed first.
 */
static void
d_internal_render_push_part(
    struct dj_internal_renderer* _r,
    const char*                  _text,
    bool                         _before,
    bool                         _after
)
{
    const bool word = ( (_text) &&
                        ( ( (_text[0] >= 'a') && (_text[0] <= 'z') ) ||
                          ( (_text[0] >= 'A') && (_text[0] <= 'Z') ) ) );

    // nothing spelled
    if ( (!_text) ||
         (_text[0] == '\0') )
    {
        return;
    }

    if ( (word) ||
         (_after) )
    {
        d_internal_render_push_text(_r, " ");
    }

    d_internal_render_push_text(_r, _text);

    if ( (word) &&
         (_before) )
    {
        d_internal_render_push_text(_r, " ");
    }

    return;
}

/**
 * @brief Expands an infix application: a binary one, a flattened run, or a
 *        juxtaposition.
 *
 * @param[in,out] _r   the renderer.
 * @param[in]     _n   the node.
 * @param[in]     _op  its operator.
 */
static void
d_internal_render_infix(
    struct dj_internal_renderer*     _r,
    const struct dj_parse_expr_node* _n,
    const struct dj_parse_expr_op*   _op
)
{
    const uint32_t* c    = &_r->term->args[_n->first];
    const bool      word = ((_op->flags & D_PARSE_EXPR_OP_WORD) != 0u);
    const bool      bare = ( ((_n->flags & D_PARSE_EXPR_NODE_JUXTAPOSED) != 0u)
                             && ( (!_op->spelling) ||
                                  (_op->spelling[0] == '\0') ) );
    const uint8_t   kind =
        (_op->assoc == D_PARSE_EXPR_ASSOC_CHAIN) ? D_INTERNAL_EXPR_FRAME_CHAIN
      : ((_op->flags & D_PARSE_EXPR_OP_FLATTEN) != 0u)
                                               ? D_INTERNAL_EXPR_FRAME_FLAT
                                               : D_INTERNAL_EXPR_FRAME_INFIX;

    // last first; each operand bracketed by the rule of each side it has
    for (uint32_t i = _n->count; i > 0u; --i)
    {
        const uint32_t child   = c[i - 1u];
        bool           bracket = false;

        if (i > 1u)
        {
            bracket = ( (d_internal_render_right_needs(_r, kind, _op,
                                                       _op->precedence,
                                                       _op->assoc, child)) ||
                        ( (bare) &&
                          (d_internal_render_starts_ambiguous(_r, child)) ) );
        }

        if (i < _n->count)
        {
            bracket = ( (bracket) ||
                        (d_internal_render_left_needs(_r, _op, false,
                                                      child)) );
        }

        d_internal_render_push_node(_r, child, bracket);

        if (i > 1u)
        {
            d_internal_render_push(_r, D_INTERNAL_RENDER_INFIX, 0u, 0u,
                                   (bare) ? "" : _op->spelling, word);
        }
    }

    return;
}

/**
 * @brief Expands a chain's join as the chain: `a < b <= c`.
 *
 * @param[in,out] _r  the renderer.
 * @param[in]     _n  the join.
 *
 * The links are read from the right: a binary join nests to the left, so
 * its last child is the rightmost link not yet written and its first the
 * rest; a flattening join holds every link. Each operand between two links
 * is bracketed by the rule of both.
 */
static void
d_internal_render_chain(
    struct dj_internal_renderer*     _r,
    const struct dj_parse_expr_node* _n
)
{
    const struct dj_parse_expr_node* join  = _n;
    const struct dj_parse_expr_op*   after = NULL;

    while ( (!_r->failed) &&
            (join) )
    {
        const uint32_t* c    = &_r->term->args[join->first];
        const struct dj_parse_expr_node* down =
            &_r->term->nodes[c[0]];
        const bool      more = ((down->flags & D_PARSE_EXPR_NODE_CHAIN) != 0u);

        for (uint32_t i = join->count; i > ((more) ? 1u : 0u); --i)
        {
            const struct dj_parse_expr_node* link =
                &_r->term->nodes[c[i - 1u]];
            const struct dj_parse_expr_op*   op   =
                d_parse_expr_find_op(_r->sig, link->op);
            const uint32_t right = _r->term->args[link->first + 1u];

            if (!op)
            {
                _r->failed = true;

                return;
            }

            d_internal_render_push_node(
                _r, right,
                ( (d_internal_render_right_needs(
                       _r, D_INTERNAL_EXPR_FRAME_CHAIN, op, op->precedence,
                       D_PARSE_EXPR_ASSOC_CHAIN, right)) ||
                  ( (after) &&
                    (d_internal_render_left_needs(_r, after, false,
                                                  right)) ) ));
            d_internal_render_push(_r, D_INTERNAL_RENDER_INFIX, 0u, 0u,
                                   op->spelling,
                                   ((op->flags & D_PARSE_EXPR_OP_WORD) != 0u));

            after = op;

            // the leftmost link writes its left operand too
            if ( (i == 1u) &&
                 (!more) )
            {
                const uint32_t left = _r->term->args[link->first];

                d_internal_render_push_node(
                    _r, left, d_internal_render_left_needs(_r, op, false,
                                                           left));
            }
        }

        join = (more) ? down : NULL;
    }

    return;
}

/**
 * @brief Expands a mixfix application: its lead, its parts and holes, its
 *        tail.
 *
 * @param[in,out] _r   the renderer.
 * @param[in]     _n   the node.
 * @param[in]     _op  its operator.
 */
static void
d_internal_render_mixfix(
    struct dj_internal_renderer*     _r,
    const struct dj_parse_expr_node* _n,
    const struct dj_parse_expr_op*   _op
)
{
    const struct dj_parse_expr_mixfix* m      = &_op->mixfix;
    const uint32_t*                    c      = &_r->term->args[_n->first];
    const uint32_t                     lead   = (m->lead) ? 1u : 0u;
    const uint32_t                     tail   = (m->tail) ? 1u : 0u;
    const uint32_t                     holes  = (uint32_t)m->parts - 1u -
                                                ((m->list_hole) ? 1u : 0u);
    const uint32_t                     items  =
        _n->count - lead - tail - ((m->list_hole) ? holes : 0u);
    uint32_t                           at     = _n->count;

    // the tail, then each part and the hole before it, last first
    if (tail)
    {
        --at;
        d_internal_render_push_node(
            _r, c[at],
            d_internal_render_right_needs(_r, D_INTERNAL_EXPR_FRAME_TAIL, _op,
                                          _op->precedence, _op->assoc,
                                          c[at]));
    }

    for (uint32_t k = m->parts; k > 1u; --k)
    {
        d_internal_render_push_part(_r, m->part_spelling[k - 1u], true,
                                    false);

        // the list hole lies between parts 0 and 1
        if ( (k == 2u) &&
             (m->list_hole) )
        {
            for (uint32_t i = items; i > 0u; --i)
            {
                d_internal_render_push_node(_r, c[--at], false);

                if (i > 1u)
                {
                    d_internal_render_push_part(_r, m->separator_spelling,
                                                false, true);
                }
            }
        }
        else
        {
            d_internal_render_push_node(_r, c[--at], false);
        }
    }

    d_internal_render_push_part(_r, m->part_spelling[0],
                                (lead == 1u), false);

    if (lead)
    {
        d_internal_render_push_node(
            _r, c[0], d_internal_render_left_needs(_r, _op, true, c[0]));
    }

    return;
}

/**
 * @brief Expands a list written in rows: `[1, 2; 3, 4]`.
 *
 * @param[in,out] _r  the renderer.
 * @param[in]     _n  the rows' node.
 */
static void
d_internal_render_rows(
    struct dj_internal_renderer*     _r,
    const struct dj_parse_expr_node* _n
)
{
    const struct dj_parse_expr_mixfix* m = NULL;

    // the list operator whose rows these are
    for (uint32_t i = 0u; ( (i < _r->sig->op_count) && (!m) ); ++i)
    {
        const struct dj_parse_expr_op* op = &_r->sig->ops[i];

        if ( (op->form == D_PARSE_EXPR_FORM_MIXFIX) &&
             (op->mixfix.list_hole)                 &&
             (op->mixfix.rows_op == _n->op) )
        {
            m = &op->mixfix;
        }
    }

    if (!m)
    {
        _r->failed = true;

        return;
    }

    d_internal_render_push_part(_r, m->part_spelling[1], false, false);

    for (uint32_t r = _n->count; r > 0u; --r)
    {
        const struct dj_parse_expr_node* row =
            &_r->term->nodes[_r->term->args[_n->first + r - 1u]];

        for (uint32_t i = row->count; i > 0u; --i)
        {
            d_internal_render_push_node(
                _r, _r->term->args[row->first + i - 1u], false);

            if (i > 1u)
            {
                d_internal_render_push_part(_r, m->separator_spelling,
                                            false, true);
            }
        }

        if (r > 1u)
        {
            d_internal_render_push_part(_r, m->row_separator_spelling,
                                        false, true);
        }
    }

    d_internal_render_push_part(_r, m->part_spelling[0], false, false);

    return;
}

/**
 * @brief Expands a node task into its pieces.
 *
 * @param[in,out] _r     the renderer.
 * @param[in]     _task  the task, popped.
 */
static void
d_internal_render_node(
    struct dj_internal_renderer*          _r,
    const struct dj_internal_render_task* _task
)
{
    const struct dj_parse_expr_node* n    = &_r->term->nodes[_task->node];
    const uint32_t                   keep =
        ( (_r->options) &&
          ((_r->options->flags & D_PARSE_EXPR_RENDER_KEEP_GROUPS) != 0u) )
        ? D_PARSE_EXPR_NODE_GROUPED
        : 0u;

    // brackets, needed or kept, once
    if ( ((_task->flags & D_INTERNAL_RENDER_WRITTEN) == 0u)      &&
         ( ((_task->flags & D_INTERNAL_RENDER_BRACKET) != 0u) ||
           ((n->flags & keep) != 0u) ) )
    {
        d_internal_render_push_text(_r, _r->close);
        d_internal_render_push(_r, D_INTERNAL_RENDER_NODE, _task->node,
                               D_INTERNAL_RENDER_WRITTEN, NULL, false);
        d_internal_render_push_text(_r, _r->open);

        return;
    }

    if (n->kind == D_PARSE_EXPR_NODE_LEAF)
    {
        d_internal_render_atom(_r, _task->node);

        return;
    }

    if (n->kind == D_PARSE_EXPR_NODE_HOLE)
    {
        d_internal_render_emit(_r, ( (_r->options) && (_r->options->hole) )
                                   ? _r->options->hole
                                   : "?");

        return;
    }

    const struct dj_parse_expr_op* op = d_parse_expr_find_op(_r->sig, n->op);

    if ((n->flags & D_PARSE_EXPR_NODE_ROWS) != 0u)
    {
        d_internal_render_rows(_r, n);
    }
    else if ((n->flags & D_PARSE_EXPR_NODE_CHAIN) != 0u)
    {
        d_internal_render_chain(_r, n);
    }
    else if (!op)
    {
        _r->failed = true;
    }
    else if (op->form == D_PARSE_EXPR_FORM_PREFIX)
    {
        const uint32_t child = _r->term->args[n->first];

        d_internal_render_push_node(
            _r, child,
            d_internal_render_right_needs(_r, D_INTERNAL_EXPR_FRAME_PREFIX,
                                          op, op->precedence, op->assoc,
                                          child));
        d_internal_render_push(_r, D_INTERNAL_RENDER_PREFIX, 0u, 0u,
                               op->spelling,
                               ((op->flags & D_PARSE_EXPR_OP_WORD) != 0u));
    }
    else if (op->form == D_PARSE_EXPR_FORM_POSTFIX)
    {
        const uint32_t child = _r->term->args[n->first];

        d_internal_render_push_text(_r, op->spelling);
        d_internal_render_push_node(
            _r, child, d_internal_render_left_needs(_r, op, true, child));
    }
    else if (op->form == D_PARSE_EXPR_FORM_INFIX)
    {
        d_internal_render_infix(_r, n, op);
    }
    else
    {
        d_internal_render_mixfix(_r, n, op);
    }

    return;
}

/**
 * @brief Writes a spelled operator's task.
 *
 * @param[in,out] _r     the renderer.
 * @param[in]     _task  the task, popped.
 */
static void
d_internal_render_spelled(
    struct dj_internal_renderer*          _r,
    const struct dj_internal_render_task* _task
)
{
    const bool compact =
        ( (_r->options) &&
          ((_r->options->flags & D_PARSE_EXPR_RENDER_COMPACT) != 0u) &&
          (!_task->word) );

    // a prefix operator, a word spaced, a symbol guarded against fusion
    if (_task->kind == D_INTERNAL_RENDER_PREFIX)
    {
        d_internal_render_emit(_r, _task->text);
        _r->guard = (!_task->word);

        if (_task->word)
        {
            d_internal_render_emit(_r, " ");
        }

        return;
    }

    // juxtaposition spelled "" is a space; an infix operator spaced unless
    // compact
    if ( (!_task->text) ||
         (_task->text[0] == '\0') )
    {
        d_internal_render_emit(_r, " ");

        return;
    }

    if (!compact)
    {
        d_internal_render_emit(_r, " ");
    }

    d_internal_render_emit(_r, _task->text);

    if (compact)
    {
        _r->guard = true;
    }
    else
    {
        d_internal_render_emit(_r, " ");
    }

    return;
}


//==============================================================================
// 5.  THE ENTRIES
//==============================================================================


/*
d_parse_expr_render
  The brackets are the signature's transparent group's, where it has one, so a
language whose group is spelled otherwise reads its own brackets back.
*/
bool
d_parse_expr_render(
    const struct dj_parse_expr_signature*      _signature,
    const struct dj_parse_expr_term*           _term,
    uint32_t                                   _root,
    const struct dj_parse_expr_render_options* _options,
    dj_parse_expr_write_fn                     _write,
    void*                                      _write_ctx
)
{
    // a sealed signature, a node of a term, a writer
    if ( (!_signature)                                                 ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) == 0u)  ||
         (!d_parse_expr_term_node(_term, _root))                       ||
         (!_write) )
    {
        return false;
    }

    struct dj_internal_renderer r;

    memset(&r, 0, sizeof(r));
    r.sig       = _signature;
    r.term      = _term;
    r.options   = _options;
    r.write     = _write;
    r.write_ctx = _write_ctx;
    r.tasks     = r.local;
    r.capacity  = D_INTERNAL_RENDER_TASKS;
    r.open      = "(";
    r.close     = ")";

    // the language's own brackets
    for (uint32_t i = 0u; i < _signature->op_count; ++i)
    {
        const struct dj_parse_expr_op* op = &_signature->ops[i];

        if ( ((op->flags & D_PARSE_EXPR_OP_TRANSPARENT) != 0u) &&
             (op->mixfix.part_spelling[0])                     &&
             (op->mixfix.part_spelling[1]) )
        {
            r.open  = op->mixfix.part_spelling[0];
            r.close = op->mixfix.part_spelling[1];

            break;
        }
    }

    d_internal_render_push_node(&r, _root, false);

    while ( (!r.failed) &&
            (r.count > 0u) )
    {
        const struct dj_internal_render_task task = r.tasks[--r.count];

        if (task.kind == D_INTERNAL_RENDER_NODE)
        {
            d_internal_render_node(&r, &task);
        }
        else if (task.kind == D_INTERNAL_RENDER_TEXT)
        {
            d_internal_render_emit(&r, task.text);
        }
        else
        {
            d_internal_render_spelled(&r, &task);
        }
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    // a stack that left the renderer's own room
    if (r.tasks != r.local)
    {
        free(r.tasks);
    }
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    return (!r.failed);
}

// dj_internal_render_buffer
//   struct: a buffer being written, and how much would have been.
struct dj_internal_render_buffer
{
    char*  out;
    size_t size;
    size_t length;
};

/**
 * @brief The buffer writer: copies what fits, counts all of it.
 *
 * @param[in,out] _ctx     the buffer.
 * @param[in]     _bytes   the text.
 * @param[in]     _length  its length.
 * @return `_length`: a buffer never refuses, it cuts.
 */
static size_t
d_internal_render_to_buffer(
    void*       _ctx,
    const char* _bytes,
    size_t      _length
)
{
    struct dj_internal_render_buffer* b =
        (struct dj_internal_render_buffer*)_ctx;

    // what fits before the NUL
    for (size_t i = 0u; i < _length; ++i)
    {
        if (b->length + i + 1u < b->size)
        {
            b->out[b->length + i] = _bytes[i];
        }
    }

    b->length += _length;

    return _length;
}

/*
d_parse_expr_render_buffer
  As snprintf: the whole length is returned, so a caller learns the size it
needs from a first call with no buffer.
*/
size_t
d_parse_expr_render_buffer(
    const struct dj_parse_expr_signature*      _signature,
    const struct dj_parse_expr_term*           _term,
    uint32_t                                   _root,
    const struct dj_parse_expr_render_options* _options,
    char*                                      _out,
    size_t                                     _size
)
{
    struct dj_internal_render_buffer b;

    b.out    = _out;
    b.size   = (_out) ? _size : 0u;
    b.length = 0u;

    const bool whole = d_parse_expr_render(_signature, _term, _root,
                                           _options,
                                           d_internal_render_to_buffer, &b);

    // NUL-terminated where there is room for anything
    if (b.size > 0u)
    {
        b.out[(b.length < b.size) ? b.length : b.size - 1u] = '\0';
    }

    return (whole) ? b.length : (size_t)-1;
}

#endif  // D_INTERNAL_PARSE_EXPR_RENDER
