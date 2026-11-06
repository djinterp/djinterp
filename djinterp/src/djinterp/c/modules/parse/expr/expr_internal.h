/*******************************************************************************
* djinterp [c/parse]                                             expr_internal.h
*
* Expression core v2: what the parser and the renderer share, and nothing
* else sees.
*   The renderer decides where brackets go by asking the parser's own rule:
* would the operator after a child, or the child's own operator, group
* otherwise than the term says? One rule, in one place, is what keeps a
* rendered term reading back as itself.
*
*
* path:      /src/djinterp/c/modules/parse/expr/expr_internal.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE SHARED RULE
    ---------------
    1.  Frame kinds
    2.  Decisions
    3.  d_internal_expr_decide
*/

#ifndef DJINTERP_C_PARSE_EXPR_EXPR_INTERNAL_H
#define DJINTERP_C_PARSE_EXPR_EXPR_INTERNAL_H 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/parse/expr/signature.h"  // dj_parse_expr_op
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint8_t, int32_t


//==============================================================================
// 1.  THE SHARED RULE
//==============================================================================


// 1.1    Frame kinds
//------------------------------------------------------------------------------
// what an open operator is to the parser: a prefix operator; an infix one; a
// chain; a flattening run; a mixfix operator awaiting a part; one past its
// last part, awaiting its tail
#define D_INTERNAL_EXPR_FRAME_PREFIX    1u
#define D_INTERNAL_EXPR_FRAME_INFIX     2u
#define D_INTERNAL_EXPR_FRAME_CHAIN     3u
#define D_INTERNAL_EXPR_FRAME_FLAT      4u
#define D_INTERNAL_EXPR_FRAME_MIXFIX    5u
#define D_INTERNAL_EXPR_FRAME_TAIL      6u


// 1.2    Decisions
//------------------------------------------------------------------------------
// what an arriving operator does with the open one below it: reduces it
// first, stops above it, extends it (a chain's link, a flattening run's
// operand), or meets it as a refused non-associative pair
#define D_INTERNAL_EXPR_REDUCE          1
#define D_INTERNAL_EXPR_STOP            2
#define D_INTERNAL_EXPR_EXTEND          3
#define D_INTERNAL_EXPR_NONASSOC        4


// 1.3    d_internal_expr_decide
//------------------------------------------------------------------------------
/**
 * @brief Decides what an arriving operator does with an open one.
 *
 * @param[in] _kind        the open operator's frame kind.
 * @param[in] _precedence  its precedence.
 * @param[in] _assoc       its association.
 * @param[in] _open        its operator.
 * @param[in] _op          the arriving operator.
 * @param[in] _postfix     whether the arriving one takes its left operand
 *                         and nothing after it: a postfix operator, or a
 *                         mixfix one with a lead.
 * @return D_INTERNAL_EXPR_REDUCE, _STOP, _EXTEND or _NONASSOC.
 */
int d_internal_expr_decide(uint8_t                        _kind,
                           int32_t                        _precedence,
                           uint8_t                        _assoc,
                           const struct dj_parse_expr_op* _open,
                           const struct dj_parse_expr_op* _op,
                           bool                           _postfix);


#endif  // DJINTERP_C_PARSE_EXPR_EXPR_INTERNAL_H
