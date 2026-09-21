/*******************************************************************************
* djinterp [dawk]                                                        dnode.h
*
* Node tree:
*   The tree a selector matches against.  A node carries an interned type, a
* run of interned attribute pairs, tree links, and the source geometry a
* layout property needs -- line, start column, end column, width.
*   Links are uint32_t indices rather than pointers, so the tree survives the
* array being reallocated and a node's identity is stable across stages.  The
* origin field is that identity: assigned once at build, never reassigned, so
* a defect found after a rewrite still names a place in the original file.
*   The tree is host-agnostic.  Nothing here knows what a banner is.
*
* path:      /inc/djinterp/tools/dawk/dnode.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DNODE_H
#define DJINTERP_TOOLS_DAWK_DNODE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t

// djinterp
#include "./dss.h"  // D_DSS_NO_INDEX, interned identifiers


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Records
//------------------------------------------------------------------------------
// 1.1.1
// d_node_attribute
//   struct: one interned name/value pair attached to a node.
struct d_node_attribute
{
    uint32_t  name;
    uint32_t  value;
    double    number;
    bool      numeric;
};

// 1.1.2
// d_node
//   struct: one node.  `text` is the node's own source text; `width`,
// `start_column` and `end_column` are the geometry facts a layout property
// compares against, counted in characters rather than bytes.
struct d_node
{
    uint32_t  type;
    uint32_t  text;

    uint32_t  parent;
    uint32_t  first_child;
    uint32_t  next_sibling;
    uint32_t  index_in_parent;

    uint32_t  first_attribute;
    uint32_t  attribute_count;

    uint32_t  origin;
    uint32_t  line;
    uint32_t  start_column;
    uint32_t  end_column;
    uint32_t  width;

    bool      present;
};

// 1.1.3
// d_node_tree
//   struct: the nodes, their attributes, and the intern table they name.
struct d_node_tree;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
struct d_node_tree* d_node_tree_new(void);
void                d_node_tree_free(struct d_node_tree* _tree);
void                d_node_tree_clear(struct d_node_tree* _tree);

// 2.2    Construction
//------------------------------------------------------------------------------
uint32_t            d_node_intern(struct d_node_tree* _tree,
                                  const char*         _text,
                                  size_t              _length);
uint32_t            d_node_add(struct d_node_tree* _tree,
                               uint32_t            _parent,
                               const char*         _type);
bool                d_node_set_attribute(struct d_node_tree* _tree,
                                         uint32_t            _node,
                                         const char*         _name,
                                         const char*         _value);
bool                d_node_set_text(struct d_node_tree* _tree,
                                    uint32_t            _node,
                                    const char*         _text,
                                    size_t              _length);

// 2.3    Inspection
//------------------------------------------------------------------------------
size_t              d_node_count(const struct d_node_tree* _tree);
struct d_node*      d_node_at(struct d_node_tree* _tree,
                              uint32_t            _at);
const struct d_node_attribute*
                    d_node_attribute_at(const struct d_node_tree* _tree,
                                        uint32_t                  _at);
const char*         d_node_text(const struct d_node_tree* _tree,
                                uint32_t                  _id);
uint32_t            d_node_find_id(const struct d_node_tree* _tree,
                                   const char*               _text);


#endif  // DJINTERP_TOOLS_DAWK_DNODE_H
