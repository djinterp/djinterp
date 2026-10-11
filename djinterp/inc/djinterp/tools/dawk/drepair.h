/*******************************************************************************
* djinterp [tools]                                                     drepair.h
*
* Repairs a rule states for its own findings.
*   A rule whose every match is a finding declares `violation`. That names no
* value, so the evaluator has nothing to repair it towards; `repair`, beside it
* in the same rule, says how the finding is put right:
*
*     repair: blank-before 1;              the node's line has that many
*                                          empty lines above it
*     repair: trailing-comment "words";    the node's line ends `// words`
*     repair: trailing-comment symbol;     the same, the words being the
*                                          node's attribute of that name
*     repair: trailing-comment "words" unmeasured;
*                                          the same, though the line pass
*                                          80 characters by it: for a line
*                                          no rule measures
*     repair: own-line;                    the node, a brace that ends its
*                                          line, begins the next; one whose
*                                          body is on its line has the body
*                                          laid out under it
*     repair: one-per-line;                the node, a function, has each of
*                                          its parameters on a line of its
*                                          own, as a definition is laid out
*     repair: join;                        the node, a brace with nothing
*                                          between it and its partner on a
*                                          later line, has it beside it
*     repair: shift;                       beside `start-column`: the node
*                                          moves to its column, and every
*                                          line of it moves with it, and
*                                          the comments beside it in its
*                                          column; its siblings that are as
*                                          far out move too, or none does
*
*   Each changes white space or adds a comment and nothing else. A repair is
* noted while a file's findings are evaluated and made when its pass is over,
* from the last line up, so that no edit moves a line another still has to
* find. This is the edit collection dedit.h says its single-line path would
* one day need.
*
* path:      /inc/djinterp/tools/dawk/drepair.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  D_REPAIR_TEXT_MAX
         2.  D_REPAIR_PARAM_MAX
    2.  Enumerations
         1.  d_repair_kind
    3.  Records
         1.  d_repair_edit
         2.  d_repair_plan
2.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Planning
    3.  Applying
*/

#ifndef DJINTERP_TOOLS_DAWK_DREPAIR_H
#define DJINTERP_TOOLS_DAWK_DREPAIR_H 1

// std
#include <stdbool.h>                          // bool
#include <stddef.h>                           // size_t
// djinterp
#include "./dnode.h"                          // struct d_node_tree
#include "./dss.h"                            // struct d_dss_sheet, d_dss_rule
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint8_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_REPAIR_TEXT_MAX
//   constant: the longest comment a repair may add, with its terminator.
#define D_REPAIR_TEXT_MAX 96

// 1.1.2
// D_REPAIR_PARAM_MAX
//   constant: the most parameters a list may have and still be laid out; a
// longer one is left as it is.
#define D_REPAIR_PARAM_MAX 24

// 1.2    Enumerations
//------------------------------------------------------------------------------
// 1.2.1
// d_repair_kind
//   enum: what an edit does. The order is the order two edits of one line
// are made in: what rewrites the line first, what moves it last.
enum d_repair_kind
{
    D_REPAIR_COMMENT = 0,  // a comment added at the line's end
    D_REPAIR_SPLIT,        // a closing-position brace moved to its own line
    D_REPAIR_LAYOUT,       // a parameter list laid out one to a line
    D_REPAIR_JOIN,         // an empty pair of braces put on one line
    D_REPAIR_EXPAND,       // a body on its brace's line laid out under it
    D_REPAIR_BLANKS,       // the empty lines above the line set to a count
    D_REPAIR_SHIFT         // an item moved to its column, whole; made alone
};

// 1.3    Records
//------------------------------------------------------------------------------
// 1.3.1
// d_repair_edit
//   struct: one repair, noted against the file as it was read. For a
// shift, the lines `open_line` to `close_line` move by `delta` -- the
// item's, with the comments beside it that are its -- and `group` is the
// node the item is a child of, which its siblings share. For a
// layout, `line` and `column` are the list's `(`, `close_line` and
// `close_column` its `)`, and `indent` the spaces before the function; each
// parameter is where its first token stands, and its name where its declared
// name stands, or line 0 when the name is not the parameter's own last word.
struct d_repair_edit
{
    uint32_t  line;                     // 1-based
    uint8_t   kind;                     // d_repair_kind
    uint32_t  count;                    // BLANKS: empty lines wanted above;
                                        // COMMENT: 1 when unmeasured
    uint32_t  column;                   // 1-based: the node's
    uint32_t  indent;                   // SPLIT, LAYOUT: spaces before it
    char      text[D_REPAIR_TEXT_MAX];  // COMMENT: the comment's words
    uint32_t  owner_line;               // LAYOUT: the function's own line
    uint32_t  close_line;               // LAYOUT, JOIN: the closing
                                        // bracket; SHIFT: the last line
                                        // that moves
    uint32_t  close_column;
    int       delta;                    // SHIFT: columns right, or left if
                                        // negative
    uint32_t  group;                    // SHIFT: the item's parent node
    uint32_t  open_line;                // SHIFT: the first line that moves
    uint32_t  parts;                    // LAYOUT: parameters; EXPAND: what
                                        // the braces hold
    uint32_t  part_line[D_REPAIR_PARAM_MAX];
    uint32_t  part_column[D_REPAIR_PARAM_MAX];
    uint32_t  name_line[D_REPAIR_PARAM_MAX];
    uint32_t  name_column[D_REPAIR_PARAM_MAX];
};

// 1.3.2
// d_repair_plan
//   struct: the repairs noted for one file.
struct d_repair_plan
{
    struct d_repair_edit* edits;
    size_t                count;
    size_t                capacity;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifetime
//------------------------------------------------------------------------------
// a plan starts empty, is cleared between files and keeps its storage until
// freed; none of the three can fail
void   d_repair_plan_init(struct d_repair_plan* _plan);
void   d_repair_plan_clear(struct d_repair_plan* _plan);
void   d_repair_plan_free(struct d_repair_plan* _plan);

// 2.2    Planning
//------------------------------------------------------------------------------
/**
 * @brief Notes the repair a rule states for a node it found.
 *
 * @param[in,out] _plan   the file's plan.
 * @param[in]     _sheet  the sheet the rule is of.
 * @param[in]     _tree   the tree the node is in.
 * @param[in]     _node   the node the rule's `violation` failed on.
 * @param[in]     _rule   the rule.
 * @return `true` if an edit was noted; `false` if the rule states no repair,
 *         the repair does not apply to this node, or memory ran out.
 */
bool   d_repair_plan_note(struct d_repair_plan*     _plan,
                          const struct d_dss_sheet* _sheet,
                          struct d_node_tree*       _tree,
                          uint32_t                  _node,
                          const struct d_dss_rule*  _rule);
/**
 * @brief Reports whether a rule says how its findings are repaired.
 *
 * @note A rule that does is repaired that way or not at all: where its own
 *       repair leaves a node -- it would have to move part of something, or
 *       move it somewhere it was not meant to go -- no other repair is to
 *       take the node up in its place.
 *
 * @param[in] _sheet  the sheet the rule is of.
 * @param[in] _rule   the rule.
 * @return `true` if the rule carries a `repair` declaration with a value,
 *         `false` otherwise.
 */
bool   d_repair_stated(const struct d_dss_sheet* _sheet,
                       const struct d_dss_rule*  _rule);

// 2.3    Applying
//------------------------------------------------------------------------------
/**
 * @brief Makes a plan's repairs in a file, from its last line up.
 *
 * @note An edit is checked against the line on disk before it is written,
 *       and one whose line is not what it was noted against is left for the
 *       next pass. A repair that would change anything but white space or
 *       add anything but its comment is refused, and counted.
 *
 * @param[in,out] _plan           the plan; its edits are put in order.
 * @param[in]     _path           the file.
 * @param[in]     _touched        lines already rewritten this pass, which no
 *                                edit here may touch.
 * @param[in]     _touched_count  how many.
 * @param[in,out] _refused        incremented for each repair refused.
 * @return the number of lines added, removed or rewritten.
 * @post Every node measured from `_path` before the call is stale.
 */
size_t d_repair_plan_apply(struct d_repair_plan* _plan,
                           const char*           _path,
                           const uint32_t*       _touched,
                           size_t                _touched_count,
                           size_t*               _refused);


#endif  // DJINTERP_TOOLS_DAWK_DREPAIR_H
