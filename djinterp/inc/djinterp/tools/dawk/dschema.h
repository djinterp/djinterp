/*******************************************************************************
* djinterp [tools]                                                     dschema.h
*
* Property registry: what properties a host understands, known at load.
*   A host describes each property once -- its name, whether it inherits, its
* initial value, a check on its value, and the callback that evaluates it --
* and hands the table over.  The engine then refuses a sheet naming a
* property no host registered, before any file is read: a misspelt
* property is otherwise a declaration that silently never holds or fails.
* Bound to a symbol table, a registry maps a declaration to its entry by
* array index rather than by comparing names.
*
*
* path:      /inc/djinterp/tools/dawk/dschema.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Callbacks
    2.  Records
2.  OPERATIONS
    ----------
    1.  Validation
    2.  Binding and dispatch
*/

#ifndef DJINTERP_TOOLS_DAWK_DSCHEMA_H
#define DJINTERP_TOOLS_DAWK_DSCHEMA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree
#include "./dss.h"    // d_dss_sheet, d_dss_declaration


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Callbacks
//------------------------------------------------------------------------------
// 1.1.1
// d_schema_evaluate_fn
//   type: evaluates one declaration at one node; sets *_out_evaluated false
// when the declaration says nothing there.
typedef bool (*d_schema_evaluate_fn)(const struct d_dss_sheet*       _sheet,
                                     struct d_node_tree*             _tree,
                                     uint32_t                        _node,
                                     const struct d_dss_declaration* _decl,
                                     bool*                           _out_evaluated);
// 1.1.2
// d_schema_check_fn
//   type: checks a declaration's value at load; sets *_out_why on refusal.
typedef bool (*d_schema_check_fn)(const struct d_dss_sheet*       _sheet,
                                  const struct d_dss_declaration* _decl,
                                  const char**                    _out_why);

// 1.2    Records
//------------------------------------------------------------------------------
// 1.2.1
// d_schema_property
//   struct: one property a host understands.  `evaluate` NULL means the host
// reads the declaration itself (a modifier such as a severity); `host` is
// the host's own, for anything the engine has no word for, such as repair.
struct d_schema_property
{
    const char*           name;
    bool                  inherited;
    const char*           initial;    // NULL: no initial value
    d_schema_check_fn     check;      // NULL: any value
    d_schema_evaluate_fn  evaluate;
    const void*           host;
};

// 1.2.2
// d_schema
//   struct: a host's properties.
struct d_schema
{
    const struct d_schema_property*  properties;
    size_t                           count;
};

// 1.2.3
// d_schema_binding
//   struct: a schema indexed by the symbols of one table.
struct d_schema_binding;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Validation
//------------------------------------------------------------------------------
const struct d_schema_property*
     d_schema_find(const struct d_schema* _schema,
                   const char*            _name);
/**
 * @brief Refuses a sheet whose rules name a property the schema lacks or give
 *        one a value its check refuses.
 *
 * @note At-rule blocks are the host's to read and are not validated.
 *
 * @param[in]  _schema    the schema.
 * @param[in]  _sheet     the sheet.
 * @param[out] _out_line  receives the offending declaration's line.
 * @param[out] _out_name  receives its property name.
 * @param[out] _out_why   receives the refusal; may be `NULL`.
 * @return `true` if every rule declaration is known and valid.
 */
bool d_schema_validate(const struct d_schema*    _schema,
                       const struct d_dss_sheet* _sheet,
                       uint32_t*                 _out_line,
                       const char**              _out_name,
                       const char**              _out_why);

// 2.2    Binding and dispatch
//------------------------------------------------------------------------------
/**
 * @brief Indexes a schema by the symbols of a table, interning its names.
 *
 * @param[in]     _schema   the schema; it must outlive the binding.
 * @param[in,out] _symbols  the table the sheet and trees share.
 * @return the binding, or `NULL` if allocation failed.
 */
struct d_schema_binding*
     d_schema_bind(const struct d_schema* _schema,
                   struct d_symbol_table* _symbols);
void d_schema_binding_free(struct d_schema_binding* _binding);
// the entry for a property symbol, or NULL
const struct d_schema_property*
     d_schema_lookup(const struct d_schema_binding* _binding,
                     uint32_t                       _symbol);
/**
 * @brief Evaluates a declaration through its property's registered callback.
 *
 * @param[in]  _binding        the bound schema.
 * @param[in]  _sheet          the sheet, bound to the same table.
 * @param[in]  _tree           the tree.
 * @param[in]  _node           the node.
 * @param[in]  _declaration    the declaration.
 * @param[out] _out_evaluated  false when nothing was evaluated.
 * @return whether the declaration holds; meaningful only when evaluated.
 */
bool d_schema_evaluate(const struct d_schema_binding*  _binding,
                       const struct d_dss_sheet*       _sheet,
                       struct d_node_tree*             _tree,
                       uint32_t                        _node,
                       const struct d_dss_declaration* _declaration,
                       bool*                           _out_evaluated);


#endif  // DJINTERP_TOOLS_DAWK_DSCHEMA_H
