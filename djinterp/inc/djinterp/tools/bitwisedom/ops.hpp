/*******************************************************************************
* bitwisedom [bitwisedom]                                                ops.hpp
*
* Operations on symbols, and how they move the sheet's bits around.
*   An operation -- a shift, a rotation, a logical or arithmetic operation, a
* byte swap or a bit reversal -- applies to a set of symbols, or to the probe
* value, within the sheet's width. Before it is applied it is previewed:
* each target's value before and after, how many of its bits fell off the
* end, how its definition would be rewritten, and the distribution of the
* sheet's allocations before and after, with the collisions each makes.
*   Operations that only move bits -- shifts, rotations, swaps, reversals --
* also have a flow: for each bit of the result, the bit of the operand it
* came from, which is what the flow view draws.
*   APPLYING keeps a definition's kind where the result still fits it: a bit
* shifted is still a bit, a range rotated without wrapping is still a range,
* a field_value moved within its field is still a field_value, and an
* expression is wrapped in the operation, so the header still spells it.
* Otherwise the symbol becomes a literal of the bits it now has.
*
* path:      /inc/djinterp/tools/bitwisedom/ops.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  op_kind
    2.  Applying to a value
2.  PREVIEW AND APPLY
    -----------------
    1.  op_preview
    2.  preview_op, apply_op_to
*/

#ifndef BITWISEDOM_OPS_HPP
#define BITWISEDOM_OPS_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./resolve.hpp"  // resolution
#include "./sheet.hpp"    // sheet


namespace bitwisedom
{


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    op_kind
//------------------------------------------------------------------------------
// 1.1.1
// op_kind
//   enum: the operations, in the order the operations panel lists them.
enum class op_kind : int
{
    shift_left = 0,
    shift_right,
    shift_arith,
    rotate_left,
    rotate_right,
    complement,
    bit_and,
    bit_or,
    bit_xor,
    and_not,
    add,
    subtract,
    multiply,
    negate,
    swap_bytes,
    reverse,
    set_bit,
    clear_bit,
    toggle_bit
};

// 1.1.2
// OP_COUNT
//   constant: how many operations there are.
const int OP_COUNT = 19;

// 1.1.3
// op_info
//   struct: an operation's name, its C spelling where it has one, whether it
// takes an operand -- a count, a value or a bit -- and whether it only
// moves bits.
struct op_info
{
    const char* name     = "";
    const char* spelling = "";
    const char* operand  = "";
    bool        permutes = false;
};

// 1.2    Applying to a value
//------------------------------------------------------------------------------
// operations on one value -- pure; _value and the result in _width bits.
// A count -- of a shift, a rotation, a bit -- is the operand's integer;
// the bitwise operations, addition and subtraction take the operand whole;
// a product takes its integer.
D_NODISCARD op_info           describe(op_kind _op);
D_NODISCARD wide              apply_op(op_kind     _op,
                                       const wide& _value,
                                       const wide& _operand,
                                       int         _width);
D_NODISCARD u64               apply_op(op_kind _op,
                                       u64     _value,
                                       u64     _operand,
                                       int     _width);

/**
 * @brief Where each bit of an operation's result comes from.
 *
 * @param[in] _op       the operation; one that only moves bits.
 * @param[in] _operand  its count, for the shifts and rotations.
 * @param[in] _width    the sheet's width.
 * @return for each result bit, the operand bit it copies, or -1 where a
 *         zero fills in. Empty for an operation that does more than move.
 */
D_NODISCARD std::vector<int>  bit_flow(op_kind _op,
                                       u64     _operand,
                                       int     _width);


//==============================================================================
// 2.  PREVIEW AND APPLY
//==============================================================================


// 2.1    op_preview
//------------------------------------------------------------------------------
// 2.1.1
// op_row
//   struct: one target's part of a preview.
struct op_row
{
    int         id      = 0;
    wide        before;
    wide        after;
    int         lost    = 0;   // set bits that fell off the end, or cleared
    int         gained  = 0;   // set bits that were not set before
    std::string rewrite;       // how applying rewrites its definition
};

// 2.1.2
// op_preview
//   struct: what an operation would do, before it does it.
struct op_preview
{
    op_kind             op       = op_kind::shift_left;
    wide                operand;
    std::vector<op_row> rows;
    bool                probe    = false;
    wide                probe_before;
    wide                probe_after;
    std::vector<int>    usage_before;
    std::vector<int>    usage_after;
    wide                collisions_before;
    wide                collisions_after;
    std::vector<int>    flow;
};

// 2.2    preview_op, apply_op_to
//------------------------------------------------------------------------------
/**
 * @brief Previews an operation over a set of rows.
 *
 * @param[in] _sheet       the sheet.
 * @param[in] _resolution  its resolution.
 * @param[in] _targets     the rows; a group stands for every symbol in it.
 *                         Symbols with no value are passed over.
 * @param[in] _op          the operation.
 * @param[in] _operand     its operand.
 * @param[in] _probe       whether the probe value is a target too.
 * @return each target before and after, and the distribution before and
 *         after.
 */
D_NODISCARD op_preview preview_op(const sheet&            _sheet,
                                  const resolution&       _resolution,
                                  const std::vector<int>& _targets,
                                  op_kind                 _op,
                                  const wide&             _operand,
                                  bool                    _probe);

/**
 * @brief Applies an operation, rewriting each target's definition.
 *
 * @param[in,out] _sheet       the sheet.
 * @param[in]     _resolution  its resolution, from before the operation.
 * @param[in]     _targets     as for preview_op.
 * @param[in]     _op          the operation.
 * @param[in]     _operand     its operand.
 * @param[in]     _probe       whether the probe value is a target too.
 * @post   _resolution no longer describes _sheet.
 * @return how many rows, and the probe, were changed.
 */
int apply_op_to(sheet&                  _sheet,
                const resolution&       _resolution,
                const std::vector<int>& _targets,
                op_kind                 _op,
                const wide&             _operand,
                bool                    _probe);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_OPS_HPP
