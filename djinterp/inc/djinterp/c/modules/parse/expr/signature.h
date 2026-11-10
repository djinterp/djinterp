/*******************************************************************************
* djinterp [c/parse]                                                 signature.h
*
* Expression core v2: the signature, a language's operators and roles.
*   An OPERATOR says what it builds and how tightly it binds: its form
* (prefix, infix, postfix, or mixfix, an operator of several tokens such as a
* call, an index, a group, a list, a pair of bars or `?:`), its precedence
* (larger binds tighter), its association (left, right, none, or chain) and
* its spelling for the renderer.
*   A ROLE says what one token class does in one position: in operand
* position, where nothing before it ends an operand, and in operator
* position, where one does. So `-` may be a prefix operator in one and an
* infix one in the other, `(` a group in one and a call in the other, and the
* parser asks the role that fits where the token stands.
*   A signature is built once, sealed (checked and indexed), and read-only
* after that, so threads may share it.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/signature.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Positions and forms
         1.  Positions
         2.  Forms
         3.  Associations
    2.  Flags
         1.  Operator flags
         2.  Role kinds
         3.  Role flags
    3.  Limits
         1.  D_PARSE_EXPR_OP_NONE
         2.  D_PARSE_EXPR_MAX_PARTS
         3.  D_PARSE_EXPR_ITEMS_UNBOUNDED
2.  TYPES
    -----
    1.  Operators
         1.  dj_parse_expr_mixfix
         2.  dj_parse_expr_op
    2.  Roles
         1.  dj_parse_expr_custom_fn
         2.  dj_parse_expr_role
    3.  The signature
         1.  dj_parse_expr_juxtapose_fn
         2.  dj_parse_expr_signature
3.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Operators
    3.  Roles
    4.  Sealing and finding
*/

#ifndef DJINTERP_C_PARSE_EXPR_SIGNATURE_H
#define DJINTERP_C_PARSE_EXPR_SIGNATURE_H 1

// djinterp
#include "../../../djinterp.h"                              // framework root
#include "../../../../config/c/modules/parse/cfg_parse.h"   // D_INTERNAL_PARSE_EXPR_HEAP
#include "../diagnostic.h"                                  // dj_parse_diag_sink
#include "./term.h"                                         // dj_parse_expr_term
// re_std
#include "../../../../../re_std/c/cstdint/dstdint.h"  // uint8_t, uint16_t,
                                                      // int32_t, uint32_t


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Positions and forms
//------------------------------------------------------------------------------
// 1.1.1
// Positions
//   constant: where a token stands. OPERAND: at the start, after an operator,
// an opening part or a separator, where an operand must begin; OPERATOR: just
// after an operand ends.
#define D_PARSE_EXPR_OPERAND            0u
#define D_PARSE_EXPR_OPERATOR           1u

// 1.1.2
// Forms
//   constant: an operator's shape. A mixfix operator has two to four keyword
// tokens with holes between them; one with a lead follows an operand, and one
// with a tail ends in one.
#define D_PARSE_EXPR_FORM_PREFIX        1u
#define D_PARSE_EXPR_FORM_INFIX         2u
#define D_PARSE_EXPR_FORM_POSTFIX       3u
#define D_PARSE_EXPR_FORM_MIXFIX        4u

// 1.1.3
// Associations
//   constant: how two operators of one precedence group. NONE refuses the
// second beside the first; CHAIN collects `a < b <= c` and joins its links
// with the operator the chain names.
#define D_PARSE_EXPR_ASSOC_LEFT         1u
#define D_PARSE_EXPR_ASSOC_RIGHT        2u
#define D_PARSE_EXPR_ASSOC_NONE         3u
#define D_PARSE_EXPR_ASSOC_CHAIN        4u


// 1.2    Flags
//------------------------------------------------------------------------------
// 1.2.1
// Operator flags
//   constant: FLATTEN: a run of this left-associative operator is one n-ary
// node; WORD: spelled as a word, so the renderer spaces it and it never
// begins a juxtaposed operand; TRANSPARENT: a group, whose one item is flagged
// GROUPED and stands for it with no node of its own (with a list hole, a
// count other than one is the operator's node, as awk's `(i, j)` or a
// tuple); the algebraic three are carried for simplifiers and read by nothing
// here.
#define D_PARSE_EXPR_OP_FLATTEN         0x0001u
#define D_PARSE_EXPR_OP_WORD            0x0002u
#define D_PARSE_EXPR_OP_TRANSPARENT     0x0004u
#define D_PARSE_EXPR_OP_COMMUTATIVE     0x0008u
#define D_PARSE_EXPR_OP_ASSOCIATIVE     0x0010u
#define D_PARSE_EXPR_OP_IDEMPOTENT      0x0020u

// 1.2.2
// Role kinds
//   constant: ATOM: the token is a leaf; OP: it begins its operator; CUSTOM:
// a hook of the language's reads it.
#define D_PARSE_EXPR_ROLE_ATOM          1u
#define D_PARSE_EXPR_ROLE_OP            2u
#define D_PARSE_EXPR_ROLE_CUSTOM        3u

// 1.2.3
// Role flags
//   constant: CALLABLE, on an atom: a leaf of the class may stand before a
// CALLEE role. CALLEE, on a lead mixfix: only after an ungrouped CALLABLE
// leaf. ADJACENT: only where no space precedes the token. NO_JUXTAPOSE: the
// token never begins a juxtaposed operand.
#define D_PARSE_EXPR_ROLE_CALLABLE      0x0001u
#define D_PARSE_EXPR_ROLE_CALLEE        0x0002u
#define D_PARSE_EXPR_ROLE_ADJACENT      0x0004u
#define D_PARSE_EXPR_ROLE_NO_JUXTAPOSE  0x0008u


// 1.3    Limits
//------------------------------------------------------------------------------
// 1.3.1
// D_PARSE_EXPR_OP_NONE
//   constant: the operator id meaning none; no operator may have it.
#define D_PARSE_EXPR_OP_NONE            0u

// 1.3.2
// D_PARSE_EXPR_MAX_PARTS
//   constant: the most keyword tokens a mixfix operator has: enough for
// `for x in y do z`.
#define D_PARSE_EXPR_MAX_PARTS          4u

// 1.3.3
// D_PARSE_EXPR_ITEMS_UNBOUNDED
//   constant: a list's `max_items` meaning no bound.
#define D_PARSE_EXPR_ITEMS_UNBOUNDED    0xFFFFu


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Operators
//------------------------------------------------------------------------------
// 2.1.1
// dj_parse_expr_mixfix
//   struct: a mixfix operator's parts. `lead`: it follows an operand, which is
// its first child. `parts`: its keyword tokens' classes, `part[0]` the one
// that begins it. `list_hole`: the hole between parts 0 and 1 is a list,
// split at `separator` and, where `row_separator` is not 0, into rows; every
// other hole holds one operand. `tail`: an operand follows the last part, at
// the operator's precedence. Where rows are seen, each row is a `row_op`
// node and the whole a `rows_op` node. The spellings are what the renderer
// writes for each part and separator; none is copied. `hole_mode` is the
// parse mode its holes are read in, one bit, 0 for the mode outside it; the
// mode outside comes back at its last part, so a tail is read in it (F18).
struct dj_parse_expr_mixfix
{
    uint8_t     lead;
    uint8_t     parts;
    uint8_t     list_hole;
    uint8_t     tail;
    uint32_t    part[D_PARSE_EXPR_MAX_PARTS];
    const char* part_spelling[D_PARSE_EXPR_MAX_PARTS];
    uint32_t    separator;
    uint32_t    row_separator;
    const char* separator_spelling;
    const char* row_separator_spelling;
    uint32_t    row_op;
    uint32_t    rows_op;
    uint16_t    min_items;
    uint16_t    max_items;
    uint32_t    hole_mode;
};

// 2.1.2
// dj_parse_expr_op
//   struct: one operator. `join` is a chain's: the infix operator that joins
// its links. `spelling` is what the renderer writes for a prefix, infix or
// postfix operator; a mixfix operator's are its parts'.
struct dj_parse_expr_op
{
    uint32_t                    id;
    uint8_t                     form;
    uint8_t                     assoc;
    uint16_t                    flags;
    int32_t                     precedence;
    uint32_t                    join;
    const char*                 spelling;
    struct dj_parse_expr_mixfix mixfix;
};


// 2.2    Roles
//------------------------------------------------------------------------------
// 2.2.1
// dj_parse_expr_custom_fn
//   type: a custom role's hook. It is given the parser, the token that called
// it (taken already), and in operator position the operand before it
// (D_PARSE_EXPR_NO_NODE in operand position); it may peek at and take more
// tokens, build nodes and report, through parse.h's hook interface. It
// returns 1 with the node it built in `_out`, 0 to decline (the token is
// then read as if the role were not there, and must not have been taken
// past), or -1 on a failure it has reported.
struct dj_parse_expr_parser;
struct dj_parse_expr_token;
typedef int (*dj_parse_expr_custom_fn)(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    uint32_t                          _left,
    uint32_t*                         _out);

// 2.2.2
// dj_parse_expr_role
//   struct: one role. `modes` is the set of parse modes the role is there in,
// each mode a bit (a parse's own is 1 by default), 0 meaning every mode. A
// class may have several roles in one position; they are tried in the order
// they were defined, and the first whose conditions hold is the one; a custom
// role that declines is passed over, and where none takes the token,
// juxtaposition is tried. `op` is an operator role's
// operator; a custom role in operator position may name one too, and is then
// called after the frames that bind tighter than it are reduced, so that its
// `_left` is the operand that operator would take.
struct dj_parse_expr_role
{
    uint32_t                cls;
    uint8_t                 position;
    uint8_t                 kind;
    uint16_t                flags;
    uint32_t                op;
    uint32_t                modes;
    uint32_t                order;
    dj_parse_expr_custom_fn custom;
    void*                   custom_ctx;
};


// 2.3    The signature
//------------------------------------------------------------------------------
// 2.3.1
// dj_parse_expr_juxtapose_fn
//   type: the language's say on whether two operands side by side are its
// juxtaposition operator: the term so far, the operand before, and the token
// that would begin the one after. Returns true to apply it.
typedef bool (*dj_parse_expr_juxtapose_fn)(
    void*                             _ctx,
    const struct dj_parse_expr_term*  _term,
    uint32_t                          _left,
    const struct dj_parse_expr_token* _token);

// 2.3.2
// dj_parse_expr_signature
//   struct: the language. Operators are sorted by id and roles by class,
// position and order when the signature is sealed; until then neither may be
// read by the parser. `juxtapose` is the juxtaposition operator's id, or
// D_PARSE_EXPR_OP_NONE.
struct dj_parse_expr_signature
{
    struct dj_parse_expr_op*   ops;
    uint32_t                   op_count;
    uint32_t                   op_capacity;
    struct dj_parse_expr_role* roles;
    uint32_t                   role_count;
    uint32_t                   role_capacity;
    uint32_t                   juxtapose;
    dj_parse_expr_juxtapose_fn may_juxtapose;
    void*                      juxtapose_ctx;
    uint32_t                   flags;
};

// signature flags: OWNS_OPS and OWNS_ROLES, allocated by the signature;
// SEALED, checked and indexed
#define D_PARSE_EXPR_SIGNATURE_OWNS_OPS     0x01u
#define D_PARSE_EXPR_SIGNATURE_OWNS_ROLES   0x02u
#define D_PARSE_EXPR_SIGNATURE_SEALED       0x04u


//==============================================================================
// 3.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 3.1    Lifetime
//------------------------------------------------------------------------------
/**
 * @brief Binds a signature to caller-supplied storage, empty.
 *
 * @param[out] _signature   the signature; may not be NULL.
 * @param[in]  _ops         room for the operators; may be NULL with 0.
 * @param[in]  _op_count    how many operators `_ops` holds.
 * @param[in]  _roles       room for the roles; may be NULL with 0.
 * @param[in]  _role_count  how many roles `_roles` holds.
 */
void d_parse_expr_signature_init(struct dj_parse_expr_signature* _signature,
                                 struct dj_parse_expr_op*        _ops,
                                 uint32_t                        _op_count,
                                 struct dj_parse_expr_role*      _roles,
                                 uint32_t                        _role_count);
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
/**
 * @brief Initialises an empty signature that allocates as it grows.
 *
 * @param[out] _signature  the signature; may not be NULL.
 */
void d_parse_expr_signature_init_heap(
         struct dj_parse_expr_signature* _signature);
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
/**
 * @brief Frees whatever storage the signature allocated, and empties it.
 *
 * @param[in,out] _signature  the signature; may be NULL.
 */
void d_parse_expr_signature_release(
         struct dj_parse_expr_signature* _signature);

// 3.2    Operators
//------------------------------------------------------------------------------
// each defines one operator, and fails (returns false) where the signature is
// sealed, the id is D_PARSE_EXPR_OP_NONE, or the storage is full and cannot
// grow; ids and the rest are checked when the signature is sealed

/**
 * @brief Defines an operator from a whole descriptor.
 *
 * @param[in,out] _signature  the signature; may not be NULL.
 * @param[in]     _op         the descriptor, copied.
 * @return true when it was added, false otherwise.
 */
bool d_parse_expr_define(struct dj_parse_expr_signature* _signature,
                         const struct dj_parse_expr_op*  _op);
/**
 * @brief Defines a prefix, infix or postfix operator.
 *
 * @param[in,out] _signature   the signature; may not be NULL.
 * @param[in]     _id          the operator's id.
 * @param[in]     _form        D_PARSE_EXPR_FORM_PREFIX, _INFIX or _POSTFIX.
 * @param[in]     _precedence  larger binds tighter.
 * @param[in]     _assoc       a D_PARSE_EXPR_ASSOC_* value.
 * @param[in]     _flags       D_PARSE_EXPR_OP_* flags.
 * @param[in]     _spelling    what the renderer writes; not copied.
 * @return true when it was added, false otherwise.
 */
bool d_parse_expr_define_op(struct dj_parse_expr_signature* _signature,
                            uint32_t                        _id,
                            uint8_t                         _form,
                            int32_t                         _precedence,
                            uint8_t                         _assoc,
                            uint16_t                        _flags,
                            const char*                     _spelling);
/**
 * @brief Defines a chain: an infix operator whose runs are joined.
 *
 * @param[in,out] _signature   the signature; may not be NULL.
 * @param[in]     _id          the operator's id.
 * @param[in]     _precedence  its precedence, shared by its chain's links.
 * @param[in]     _join        the infix operator joining the links.
 * @param[in]     _spelling    what the renderer writes; not copied.
 * @return true when it was added, false otherwise.
 */
bool d_parse_expr_define_chain(struct dj_parse_expr_signature* _signature,
                               uint32_t                        _id,
                               int32_t                         _precedence,
                               uint32_t                        _join,
                               const char*                     _spelling);
/**
 * @brief Defines a mixfix operator.
 *
 * @param[in,out] _signature   the signature; may not be NULL.
 * @param[in]     _id          the operator's id.
 * @param[in]     _precedence  a lead's binding, or a tail's floor.
 * @param[in]     _assoc       a tail's association.
 * @param[in]     _flags       D_PARSE_EXPR_OP_* flags.
 * @param[in]     _mixfix      its parts, copied.
 * @return true when it was added, false otherwise.
 */
bool d_parse_expr_define_mixfix(
         struct dj_parse_expr_signature*    _signature,
         uint32_t                           _id,
         int32_t                            _precedence,
         uint8_t                            _assoc,
         uint16_t                           _flags,
         const struct dj_parse_expr_mixfix* _mixfix);
/**
 * @brief Names the juxtaposition operator, and the hook that says where it
 *        applies.
 *
 * @param[in,out] _signature  the signature; may not be NULL.
 * @param[in]     _id         an infix operator's id, or D_PARSE_EXPR_OP_NONE
 *                            for none.
 * @param[in]     _hook       may be NULL, to apply it wherever it can.
 * @param[in]     _ctx        the hook's context.
 * @return true when it was set, false where the signature is sealed.
 */
bool d_parse_expr_juxtapose(struct dj_parse_expr_signature* _signature,
                            uint32_t                        _id,
                            dj_parse_expr_juxtapose_fn      _hook,
                            void*                           _ctx);

// 3.3    Roles
//------------------------------------------------------------------------------
/**
 * @brief Gives a token class a role in a position.
 *
 * @param[in,out] _signature  the signature; may not be NULL.
 * @param[in]     _role       the role, copied; its `order` is set here.
 * @return true when it was added, false where the signature is sealed or
 *         full and cannot grow.
 */
bool d_parse_expr_define_role(struct dj_parse_expr_signature*  _signature,
                              const struct dj_parse_expr_role* _role);
/**
 * @brief Makes a token class an atom in operand position.
 *
 * @param[in,out] _signature  the signature; may not be NULL.
 * @param[in]     _class      the token class.
 * @param[in]     _flags      D_PARSE_EXPR_ROLE_CALLABLE or 0.
 * @return true when it was added, false otherwise.
 */
bool d_parse_expr_define_atom(struct dj_parse_expr_signature* _signature,
                              uint32_t                        _class,
                              uint16_t                        _flags);
/**
 * @brief Makes a token class begin an operator in a position.
 *
 * @param[in,out] _signature  the signature; may not be NULL.
 * @param[in]     _class      the token class.
 * @param[in]     _position   D_PARSE_EXPR_OPERAND or _OPERATOR.
 * @param[in]     _op         the operator's id.
 * @param[in]     _flags      D_PARSE_EXPR_ROLE_* flags.
 * @return true when it was added, false otherwise.
 */
bool d_parse_expr_define_token(struct dj_parse_expr_signature* _signature,
                               uint32_t                        _class,
                               uint8_t                         _position,
                               uint32_t                        _op,
                               uint16_t                        _flags);

// 3.4    Sealing and finding
//------------------------------------------------------------------------------
/**
 * @brief Checks and indexes a signature, after which it is read-only.
 *
 * @param[in,out] _signature  the signature; may not be NULL.
 * @param[in,out] _sink       where each problem is reported, in the domain
 *                            D_PARSE_DIAG_DOMAIN_EXPR; may be NULL.
 * @return true when the signature is sound and now sealed; false, with each
 *         problem reported, when it is not.
 */
bool d_parse_expr_signature_seal(struct dj_parse_expr_signature* _signature,
                                 struct dj_parse_diag_sink*      _sink);
/**
 * @brief An operator, by id, in a sealed signature.
 *
 * @param[in] _signature  the signature; may be NULL.
 * @param[in] _id         the operator's id.
 * @return the operator, or NULL where there is none or it is not sealed.
 */
const struct dj_parse_expr_op*
     d_parse_expr_find_op(const struct dj_parse_expr_signature* _signature,
                          uint32_t                              _id);
/**
 * @brief The roles of a class in a position, in a sealed signature.
 *
 * @param[in]  _signature  the signature; may be NULL.
 * @param[in]  _class      the token class.
 * @param[in]  _position   D_PARSE_EXPR_OPERAND or _OPERATOR.
 * @param[out] _count      receives how many there are; may not be NULL.
 * @return the first of them, in order, or NULL with 0 where there are none.
 */
const struct dj_parse_expr_role*
     d_parse_expr_find_roles(
         const struct dj_parse_expr_signature* _signature,
         uint32_t                              _class,
         uint8_t                               _position,
         uint32_t*                             _count);

D_EXTERN_C_END


#endif  // DJINTERP_C_PARSE_EXPR_SIGNATURE_H
