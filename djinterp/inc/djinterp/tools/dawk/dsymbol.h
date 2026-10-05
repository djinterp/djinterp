/*******************************************************************************
* djinterp [tools]                                                     dsymbol.h
*
* Symbol table: the one namespace a sheet and a tree share.
*   A symbol is a small integer standing for a name -- a node type, an
* attribute name, a property.  The sheet and every tree it is matched
* against intern into one table, so comparing two names is comparing two
* integers.  Before this table the sheet and the tree each kept their own,
* and every type test in the matcher was a strcmp through two lookups.
*   Symbols are names, not text: a tree's source text and attribute values
* stay in the tree, which is cleared per file, while symbols persist for
* the run.  The set is closed and small -- one per distinct name.
*
*
* path:      /inc/djinterp/tools/dawk/dsymbol.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
    2.  Opaque types
2.  OPERATIONS
    ----------
    1.  Lifecycle
    2.  Interning and lookup
*/

#ifndef DJINTERP_TOOLS_DAWK_DSYMBOL_H
#define DJINTERP_TOOLS_DAWK_DSYMBOL_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_SYMBOL_NONE
//   constant: no symbol; the same value as D_DSS_NO_INDEX, so either reads.
#define D_SYMBOL_NONE ((uint32_t)-1)

// 1.2    Opaque types
//------------------------------------------------------------------------------
// 1.2.1
// d_symbol_table
//   struct: interned names, numbered densely from 0 in order of first sight.
struct d_symbol_table;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
/**
 * @brief Allocates an empty table.
 *
 * @return the table, or `NULL` if allocation failed.
 */
struct d_symbol_table* d_symbol_table_new(void);
void                   d_symbol_table_free(struct d_symbol_table* _table);

// 2.2    Interning and lookup
//------------------------------------------------------------------------------
/**
 * @brief Returns a name's symbol, adding the name when it is new.
 *
 * @param[in,out] _table   the table.
 * @param[in]     _text    the name; need not be terminated.
 * @param[in]     _length  its length in bytes.
 * @return the symbol, or `D_SYMBOL_NONE` if the table could not grow.
 */
uint32_t               d_symbol_intern(struct d_symbol_table* _table,
                                       const char*            _text,
                                       size_t                 _length);
// lookup without adding; D_SYMBOL_NONE when the name was never interned
uint32_t               d_symbol_find(const struct d_symbol_table* _table,
                                     const char*                  _text,
                                     size_t                       _length);
// a symbol's text, valid until the next intern; "" for D_SYMBOL_NONE
const char*            d_symbol_text(const struct d_symbol_table* _table,
                                     uint32_t                     _symbol);
size_t                 d_symbol_count(const struct d_symbol_table* _table);


#endif  // DJINTERP_TOOLS_DAWK_DSYMBOL_H
