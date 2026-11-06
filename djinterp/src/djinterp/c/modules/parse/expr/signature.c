/*******************************************************************************
* djinterp [c/parse]                                                 signature.c
*
* Expression core v2: definitions for signature.h, a language's table.
*   Definition only appends; sealing sorts, checks, and marks the signature
* read-only. Operators are sorted by id and found by a binary search; roles by
* class, position and the order they were defined, so that a class's roles in
* one position are a run, tried in order. Signatures are small and sealed
* once, so the sorts are insertion sorts: stable, and with no scratch.
*
*
* path:      /src/djinterp/c/modules/parse/expr/signature.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/parse/expr/signature.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // free
#include <string.h>  // memset
// djinterp
#include "../../../../../../inc/djinterp/c/modules/parse/expr/parse.h"  // the codes, the end
#include "../../../../../../inc/djinterp/c/modules/parse/storage.h"     // d_parse_grow


/*
d_parse_expr_signature_init
  Caller storage is bound with its ownership flags clear, so growth is refused
and release leaves it alone.
*/
void
d_parse_expr_signature_init(
    struct dj_parse_expr_signature* _signature,
    struct dj_parse_expr_op*        _ops,
    uint32_t                        _op_count,
    struct dj_parse_expr_role*      _roles,
    uint32_t                        _role_count
)
{
    // nothing to bind
    if (!_signature)
    {
        return;
    }

    memset(_signature, 0, sizeof(*_signature));
    _signature->ops           = _ops;
    _signature->op_capacity   = (_ops) ? _op_count : 0u;
    _signature->roles         = _roles;
    _signature->role_capacity = (_roles) ? _role_count : 0u;
    _signature->juxtapose     = D_PARSE_EXPR_OP_NONE;

    return;
}

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
/*
d_parse_expr_signature_init_heap
  An owned signature starts with no storage; the first definition allocates.
*/
void
d_parse_expr_signature_init_heap(
    struct dj_parse_expr_signature* _signature
)
{
    d_parse_expr_signature_init(_signature, NULL, 0u, NULL, 0u);

    // the signature is ours to grow
    if (_signature)
    {
        _signature->flags = ( D_PARSE_EXPR_SIGNATURE_OWNS_OPS |
                              D_PARSE_EXPR_SIGNATURE_OWNS_ROLES );
    }

    return;
}
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

/*
d_parse_expr_signature_release
  Each array is freed only where the signature allocated it.
*/
void
d_parse_expr_signature_release(
    struct dj_parse_expr_signature* _signature
)
{
    // nothing to release
    if (!_signature)
    {
        return;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    // free what the signature allocated, and only that
    if ((_signature->flags & D_PARSE_EXPR_SIGNATURE_OWNS_OPS) != 0u)
    {
        free(_signature->ops);
    }

    if ((_signature->flags & D_PARSE_EXPR_SIGNATURE_OWNS_ROLES) != 0u)
    {
        free(_signature->roles);
    }
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    d_parse_expr_signature_init(_signature, NULL, 0u, NULL, 0u);

    return;
}

/**
 * @brief Makes room for one more element of a signature's array.
 *
 * @param[in,out] _data      the array.
 * @param[in]     _count     the elements it holds.
 * @param[in,out] _capacity  its room.
 * @param[in]     _stride    an element's size.
 * @param[in]     _owned     whether the signature allocated it.
 * @return true when there is room.
 */
static bool
d_internal_expr_sig_room(
    void**    _data,
    uint32_t  _count,
    uint32_t* _capacity,
    size_t    _stride,
    bool      _owned
)
{
    // a count at the top of the range has no room after it
    if (_count >= 0xFFFFFFFEu)
    {
        return false;
    }

    // the common case: it fits
    if (_count < *_capacity)
    {
        return true;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    void* grown = d_parse_grow(*_data,
                               _capacity,
                               _count + 1u,
                               (uint32_t)_stride,
                               (_owned) ? 1 : 0);

    // check if the growth was possible
    if (!grown)
    {
        return false;
    }

    *_data = grown;

    return true;
#else
    (void)_data;
    (void)_stride;
    (void)_owned;

    return false;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
}

/*
d_parse_expr_define
  The descriptor is copied whole; its strings are not, and must outlive the
signature, as a table of literals does.
*/
bool
d_parse_expr_define(
    struct dj_parse_expr_signature* _signature,
    const struct dj_parse_expr_op*  _op
)
{
    // a signature still open, and an operator with an id
    if ( (!_signature)                                                   ||
         (!_op)                                                          ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) != 0u)    ||
         (_op->id == D_PARSE_EXPR_OP_NONE) )
    {
        return false;
    }

    void* data = _signature->ops;

    // make room, or refuse
    if (!d_internal_expr_sig_room(
             &data,
             _signature->op_count,
             &_signature->op_capacity,
             sizeof(struct dj_parse_expr_op),
             ((_signature->flags & D_PARSE_EXPR_SIGNATURE_OWNS_OPS) != 0u)))
    {
        return false;
    }

    _signature->ops                         = (struct dj_parse_expr_op*)data;
    _signature->ops[_signature->op_count++] = *_op;

    return true;
}

/*
d_parse_expr_define_op
  A shorthand over d_parse_expr_define for the three classic forms.
*/
bool
d_parse_expr_define_op(
    struct dj_parse_expr_signature* _signature,
    uint32_t                        _id,
    uint8_t                         _form,
    int32_t                         _precedence,
    uint8_t                         _assoc,
    uint16_t                        _flags,
    const char*                     _spelling
)
{
    struct dj_parse_expr_op op;

    memset(&op, 0, sizeof(op));
    op.id         = _id;
    op.form       = _form;
    op.assoc      = _assoc;
    op.flags      = _flags;
    op.precedence = _precedence;
    op.spelling   = _spelling;

    return d_parse_expr_define(_signature, &op);
}

/*
d_parse_expr_define_chain
  A chain is an infix operator of association CHAIN with the joining
operator named; sealing checks that the join is an infix operator.
*/
bool
d_parse_expr_define_chain(
    struct dj_parse_expr_signature* _signature,
    uint32_t                        _id,
    int32_t                         _precedence,
    uint32_t                        _join,
    const char*                     _spelling
)
{
    struct dj_parse_expr_op op;

    memset(&op, 0, sizeof(op));
    op.id         = _id;
    op.form       = D_PARSE_EXPR_FORM_INFIX;
    op.assoc      = D_PARSE_EXPR_ASSOC_CHAIN;
    op.precedence = _precedence;
    op.join       = _join;
    op.spelling   = _spelling;

    return d_parse_expr_define(_signature, &op);
}

/*
d_parse_expr_define_mixfix
  The parts are copied into the descriptor; the list separator's spelling,
for the renderer, is the first part's own when none is given elsewhere.
*/
bool
d_parse_expr_define_mixfix(
    struct dj_parse_expr_signature*    _signature,
    uint32_t                           _id,
    int32_t                            _precedence,
    uint8_t                            _assoc,
    uint16_t                           _flags,
    const struct dj_parse_expr_mixfix* _mixfix
)
{
    // the parts are required
    if (!_mixfix)
    {
        return false;
    }

    struct dj_parse_expr_op op;

    memset(&op, 0, sizeof(op));
    op.id         = _id;
    op.form       = D_PARSE_EXPR_FORM_MIXFIX;
    op.assoc      = _assoc;
    op.flags      = _flags;
    op.precedence = _precedence;
    op.mixfix     = *_mixfix;

    return d_parse_expr_define(_signature, &op);
}

/*
d_parse_expr_juxtapose
  Checked when sealed, like every other reference to an operator.
*/
bool
d_parse_expr_juxtapose(
    struct dj_parse_expr_signature* _signature,
    uint32_t                        _id,
    dj_parse_expr_juxtapose_fn      _hook,
    void*                           _ctx
)
{
    // a signature still open
    if ( (!_signature) ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) != 0u) )
    {
        return false;
    }

    _signature->juxtapose     = _id;
    _signature->may_juxtapose = _hook;
    _signature->juxtapose_ctx = _ctx;

    return true;
}

/*
d_parse_expr_define_role
  The role's `order` is its place among every role defined, so that sorting by
class and position keeps each run in the order it was written.
*/
bool
d_parse_expr_define_role(
    struct dj_parse_expr_signature*  _signature,
    const struct dj_parse_expr_role* _role
)
{
    // a signature still open, and a role
    if ( (!_signature) ||
         (!_role)      ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) != 0u) )
    {
        return false;
    }

    void* data = _signature->roles;

    // make room, or refuse
    if (!d_internal_expr_sig_room(
             &data,
             _signature->role_count,
             &_signature->role_capacity,
             sizeof(struct dj_parse_expr_role),
             ((_signature->flags & D_PARSE_EXPR_SIGNATURE_OWNS_ROLES) != 0u)))
    {
        return false;
    }

    _signature->roles = (struct dj_parse_expr_role*)data;
    _signature->roles[_signature->role_count]       = *_role;
    _signature->roles[_signature->role_count].order = _signature->role_count;
    ++_signature->role_count;

    return true;
}

/*
d_parse_expr_define_atom
  A shorthand: an atom role in operand position.
*/
bool
d_parse_expr_define_atom(
    struct dj_parse_expr_signature* _signature,
    uint32_t                        _class,
    uint16_t                        _flags
)
{
    struct dj_parse_expr_role role;

    memset(&role, 0, sizeof(role));
    role.cls      = _class;
    role.position = D_PARSE_EXPR_OPERAND;
    role.kind     = D_PARSE_EXPR_ROLE_ATOM;
    role.flags    = _flags;

    return d_parse_expr_define_role(_signature, &role);
}

/*
d_parse_expr_define_token
  A shorthand: a token that begins an operator.
*/
bool
d_parse_expr_define_token(
    struct dj_parse_expr_signature* _signature,
    uint32_t                        _class,
    uint8_t                         _position,
    uint32_t                        _op,
    uint16_t                        _flags
)
{
    struct dj_parse_expr_role role;

    memset(&role, 0, sizeof(role));
    role.cls      = _class;
    role.position = _position;
    role.kind     = D_PARSE_EXPR_ROLE_OP;
    role.flags    = _flags;
    role.op       = _op;

    return d_parse_expr_define_role(_signature, &role);
}

/**
 * @brief Whether one role sorts before another: by class, position, order.
 *
 * @param[in] _a  the first role.
 * @param[in] _b  the second.
 * @return true when `_a` comes first.
 */
static bool
d_internal_expr_role_before(
    const struct dj_parse_expr_role* _a,
    const struct dj_parse_expr_role* _b
)
{
    if (_a->cls != _b->cls)
    {
        return (_a->cls < _b->cls);
    }

    if (_a->position != _b->position)
    {
        return (_a->position < _b->position);
    }

    return (_a->order < _b->order);
}

/**
 * @brief Sorts the operators by id and the roles by class, position and
 *        order, both stably.
 *
 * @param[in,out] _signature  the signature.
 */
static void
d_internal_expr_sig_sort(
    struct dj_parse_expr_signature* _signature
)
{
    // insertion sort: a few dozen operators, sealed once
    for (uint32_t i = 1u; i < _signature->op_count; ++i)
    {
        const struct dj_parse_expr_op key = _signature->ops[i];
        uint32_t                      j   = i;

        while ( (j > 0u) &&
                (_signature->ops[j - 1u].id > key.id) )
        {
            _signature->ops[j] = _signature->ops[j - 1u];
            --j;
        }

        _signature->ops[j] = key;
    }

    for (uint32_t i = 1u; i < _signature->role_count; ++i)
    {
        const struct dj_parse_expr_role key = _signature->roles[i];
        uint32_t                        j   = i;

        while ( (j > 0u) &&
                (d_internal_expr_role_before(&key,
                                             &_signature->roles[j - 1u])) )
        {
            _signature->roles[j] = _signature->roles[j - 1u];
            --j;
        }

        _signature->roles[j] = key;
    }

    return;
}

/**
 * @brief Finds an operator by id in sorted operators.
 *
 * @param[in] _signature  the signature, sorted.
 * @param[in] _id         the id.
 * @return the operator, or NULL.
 */
static const struct dj_parse_expr_op*
d_internal_expr_sig_op(
    const struct dj_parse_expr_signature* _signature,
    uint32_t                              _id
)
{
    uint32_t low  = 0u;
    uint32_t high = _signature->op_count;

    // a binary search over the ids
    while (low < high)
    {
        const uint32_t middle = low + ((high - low) / 2u);

        if (_signature->ops[middle].id < _id)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low < _signature->op_count) &&
             (_signature->ops[low].id == _id) )
           ? &_signature->ops[low]
           : NULL;
}

/**
 * @brief Reports one problem the seal found.
 *
 * @param[in,out] _sink     the sink; may be NULL.
 * @param[in]     _message  what is wrong.
 * @param[in]     _id       the operator or class it is about.
 */
static void
d_internal_expr_sig_problem(
    struct dj_parse_diag_sink* _sink,
    const char*                _message,
    uint32_t                   _id
)
{
    char text[128];

    (void)snprintf(text, sizeof(text), "%s (%lu)", _message,
                   (unsigned long)_id);
    (void)d_parse_diag_emit(_sink,
                            D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
                            (uint16_t)D_PARSE_EXPR_DIAG_BAD_SIGNATURE,
                            d_parse_span_unknown(),
                            text);

    return;
}

/**
 * @brief Checks a mixfix operator's parts.
 *
 * @param[in] _signature  the signature, sorted.
 * @param[in] _op         the operator.
 * @return the problem, or NULL where there is none.
 */
static const char*
d_internal_expr_mixfix_problem(
    const struct dj_parse_expr_signature* _signature,
    const struct dj_parse_expr_op*        _op
)
{
    const struct dj_parse_expr_mixfix* m = &_op->mixfix;

    // two to four parts, none the end of the input
    if ( (m->parts < 2u) ||
         (m->parts > D_PARSE_EXPR_MAX_PARTS) )
    {
        return "a mixfix operator has two to four parts";
    }

    for (uint32_t i = 0u; i < m->parts; ++i)
    {
        if (m->part[i] == D_PARSE_EXPR_CLASS_END)
        {
            return "a mixfix part is the end of the input";
        }
    }

    // a list's bounds in order, its rows' operators defined
    if ( (m->list_hole) &&
         (m->min_items > m->max_items) )
    {
        return "a list's least items exceed its most";
    }

    if ( (m->list_hole)                                        &&
         (m->row_separator != D_PARSE_EXPR_CLASS_END)          &&
         ( (!d_internal_expr_sig_op(_signature, m->row_op)) ||
           (!d_internal_expr_sig_op(_signature, m->rows_op)) ) )
    {
        return "a list's row operators are not defined";
    }

    // a transparent operator is a group: one hole, no lead, no tail, no rows;
    // a list hole makes a group of one item and a node of any other count
    if ( ((_op->flags & D_PARSE_EXPR_OP_TRANSPARENT) != 0u) &&
         ( (m->lead) || (m->tail) || (m->parts != 2u) ||
           ( (m->list_hole) &&
             (m->row_separator != D_PARSE_EXPR_CLASS_END) ) ) )
    {
        return "a transparent operator must be a group, with no rows";
    }

    return NULL;
}

/**
 * @brief Checks one operator.
 *
 * @param[in] _signature  the signature, sorted.
 * @param[in] _index      the operator's index.
 * @return the problem, or NULL where there is none.
 */
static const char*
d_internal_expr_op_problem(
    const struct dj_parse_expr_signature* _signature,
    uint32_t                              _index
)
{
    const struct dj_parse_expr_op* op = &_signature->ops[_index];

    // ids are unique
    if ( (_index > 0u) &&
         (_signature->ops[_index - 1u].id == op->id) )
    {
        return "an operator id is defined twice";
    }

    // a known form, and an association where it reads one
    if ( (op->form < D_PARSE_EXPR_FORM_PREFIX) ||
         (op->form > D_PARSE_EXPR_FORM_MIXFIX) )
    {
        return "an operator's form is not one of the four";
    }

    if ( (op->form == D_PARSE_EXPR_FORM_INFIX)          &&
         ( (op->assoc < D_PARSE_EXPR_ASSOC_LEFT) ||
           (op->assoc > D_PARSE_EXPR_ASSOC_CHAIN) ) )
    {
        return "an infix operator's association is not one of the four";
    }

    // a chain's join is an infix operator
    if (op->assoc == D_PARSE_EXPR_ASSOC_CHAIN)
    {
        const struct dj_parse_expr_op* join =
            d_internal_expr_sig_op(_signature, op->join);

        if ( (op->form != D_PARSE_EXPR_FORM_INFIX) ||
             (!join)                               ||
             (join->form != D_PARSE_EXPR_FORM_INFIX) )
        {
            return "a chain's join is not an infix operator";
        }
    }

    return (op->form == D_PARSE_EXPR_FORM_MIXFIX)
           ? d_internal_expr_mixfix_problem(_signature, op)
           : NULL;
}

/**
 * @brief Checks one role.
 *
 * @param[in] _signature  the signature, sorted.
 * @param[in] _role       the role.
 * @return the problem, or NULL where there is none.
 */
static const char*
d_internal_expr_role_problem(
    const struct dj_parse_expr_signature* _signature,
    const struct dj_parse_expr_role*      _role
)
{
    // a class, a position, a kind
    if (_role->cls == D_PARSE_EXPR_CLASS_END)
    {
        return "a role is given to the end of the input";
    }

    if (_role->position > D_PARSE_EXPR_OPERATOR)
    {
        return "a role's position is not one of the two";
    }

    if (_role->kind == D_PARSE_EXPR_ROLE_ATOM)
    {
        return (_role->position == D_PARSE_EXPR_OPERAND)
               ? NULL
               : "an atom role must stand in operand position";
    }

    // a custom role has a hook, and any operator it names is defined
    if (_role->kind == D_PARSE_EXPR_ROLE_CUSTOM)
    {
        return (!_role->custom)
               ? "a custom role has no hook"
               : ( (_role->op != D_PARSE_EXPR_OP_NONE) &&
                   (!d_internal_expr_sig_op(_signature, _role->op)) )
               ? "a custom role names no defined operator"
               : NULL;
    }

    const struct dj_parse_expr_op* op = d_internal_expr_sig_op(_signature,
                                                               _role->op);

    // an operator role names a defined operator of a fitting form
    if ( (_role->kind != D_PARSE_EXPR_ROLE_OP) ||
         (!op) )
    {
        return "a role names no defined operator";
    }

    const bool begins = ( (op->form == D_PARSE_EXPR_FORM_PREFIX) ||
                          ( (op->form == D_PARSE_EXPR_FORM_MIXFIX) &&
                            (!op->mixfix.lead) ) );

    return (begins == (_role->position == D_PARSE_EXPR_OPERAND))
           ? NULL
           : "a role's operator does not fit its position";
}

/*
d_parse_expr_signature_seal
  Sorts first, so that every check below can find operators by id; reports
every problem rather than the first, so one seal shows a table's faults
together; and marks the signature sealed only where none was found.
*/
bool
d_parse_expr_signature_seal(
    struct dj_parse_expr_signature* _signature,
    struct dj_parse_diag_sink*      _sink
)
{
    // a signature not yet sealed
    if ( (!_signature) ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) != 0u) )
    {
        return false;
    }

    bool sound = true;

    d_internal_expr_sig_sort(_signature);

    for (uint32_t i = 0u; i < _signature->op_count; ++i)
    {
        const char* problem = d_internal_expr_op_problem(_signature, i);

        if (problem)
        {
            d_internal_expr_sig_problem(_sink, problem,
                                        _signature->ops[i].id);
            sound = false;
        }
    }

    for (uint32_t i = 0u; i < _signature->role_count; ++i)
    {
        const char* problem =
            d_internal_expr_role_problem(_signature, &_signature->roles[i]);

        if (problem)
        {
            d_internal_expr_sig_problem(_sink, problem,
                                        _signature->roles[i].cls);
            sound = false;
        }
    }

    // the juxtaposition operator, where named, is an infix one
    if (_signature->juxtapose != D_PARSE_EXPR_OP_NONE)
    {
        const struct dj_parse_expr_op* op =
            d_internal_expr_sig_op(_signature, _signature->juxtapose);

        if ( (!op) ||
             (op->form != D_PARSE_EXPR_FORM_INFIX) )
        {
            d_internal_expr_sig_problem(
                _sink,
                "the juxtaposition operator is not an infix operator",
                _signature->juxtapose);
            sound = false;
        }
    }

    // sealed only where sound
    if (sound)
    {
        _signature->flags |= D_PARSE_EXPR_SIGNATURE_SEALED;
    }

    return sound;
}

/*
d_parse_expr_find_op
  A binary search, valid only once sealing has sorted the operators.
*/
const struct dj_parse_expr_op*
d_parse_expr_find_op(
    const struct dj_parse_expr_signature* _signature,
    uint32_t                              _id
)
{
    // a sealed signature
    if ( (!_signature) ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) == 0u) )
    {
        return NULL;
    }

    return d_internal_expr_sig_op(_signature, _id);
}

/*
d_parse_expr_find_roles
  The lower bound of (class, position), then the run's length.
*/
const struct dj_parse_expr_role*
d_parse_expr_find_roles(
    const struct dj_parse_expr_signature* _signature,
    uint32_t                              _class,
    uint8_t                               _position,
    uint32_t*                             _count
)
{
    // a count to report into, and a sealed signature
    if (!_count)
    {
        return NULL;
    }

    *_count = 0u;

    if ( (!_signature) ||
         ((_signature->flags & D_PARSE_EXPR_SIGNATURE_SEALED) == 0u) )
    {
        return NULL;
    }

    uint32_t low  = 0u;
    uint32_t high = _signature->role_count;

    // a binary search for the first role at or past (class, position)
    while (low < high)
    {
        const uint32_t                   middle = low + ((high - low) / 2u);
        const struct dj_parse_expr_role* role   = &_signature->roles[middle];

        if ( (role->cls < _class) ||
             ( (role->cls == _class) && (role->position < _position) ) )
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    uint32_t end = low;

    // the run of this class in this position
    while ( (end < _signature->role_count)              &&
            (_signature->roles[end].cls == _class)      &&
            (_signature->roles[end].position == _position) )
    {
        ++end;
    }

    *_count = end - low;

    return (end > low) ? &_signature->roles[low] : NULL;
}
