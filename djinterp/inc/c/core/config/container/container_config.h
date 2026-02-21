/******************************************************************************
* djinterp [container]                                       container_config.h
*
* Feature configuration for the container module.
*   Provides hierarchical enable/disable toggles for optional functionality
* (e.g. filter) across all container types. The toggle hierarchy is:
*
*   dconfig.h    D_CFG_FILTER              (global master toggle)
*       |
*       v
*   container_config.h
*       |-- D_CFG_CONTAINER_FILTER         (all containers)
*       |      |
*       |      |-- D_CFG_CONTAINER_FILTER_CONTIGUOUS
*       |      |      (array, vector, buffer, matrix, bit, stack)
*       |      |
*       |      |-- D_CFG_CONTAINER_FILTER_ASSOCIATIVE
*       |      |      (dictionary, hash, map, table, registry)
*       |      |
*       |      |-- D_CFG_CONTAINER_FILTER_LINKED
*       |      |      (list, graph, tree, node)
*       |      |
*       |      |-- D_CFG_CONTAINER_FILTER_SET
*       |             (set)
*       |
*       v
*   <container>.h
*       |-- D_CFG_CONTAINER_FILTER_<TYPE>  (individual container)
*
*   Each level inherits from the level above unless explicitly overridden.
* Individual container toggles (e.g. D_CFG_CONTAINER_FILTER_ARRAY) are NOT
* resolved here; they are resolved lazily in the container's own header or
* in the corresponding *_filter.h module. This keeps the per-container
* symbols out of every translation unit that includes container_config.h.
*
* ZERO-OVERHEAD GUARANTEE:
*   All toggles are resolved at compile time via preprocessor conditionals.
* When a feature is disabled, no code is generated and no symbols are emitted.
*
* USAGE:
*   To disable filter for all associative containers:
*       #define D_CFG_CONTAINER_FILTER_ASSOCIATIVE 0
*       #include "container.h"
*
*   To disable filter for a single container:
*       #define D_CFG_CONTAINER_FILTER_VECTOR 0
*       #include "container\array\vector.h"
*
* path:      \inc\container\container_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.19
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_CONFIG_
#define DJINTERP_C_CONTAINER_CONFIG_ 1

#include "..\..\..\dconfig.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    ALL-CONTAINER TOGGLE                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_CFG_CONTAINER_FILTER
//   configuration: enable filter functionality for all container types.
// Inherits from D_CFG_FILTER.  Override to 0 to disable filter across the
// entire container module without affecting non-container filter usage.
#ifndef D_CFG_CONTAINER_FILTER
    #define D_CFG_CONTAINER_FILTER D_CFG_FILTER
#endif  // D_CFG_CONTAINER_FILTER


///////////////////////////////////////////////////////////////////////////////
///             II.   CATEGORY-LEVEL TOGGLES                               ///
///////////////////////////////////////////////////////////////////////////////

// D_CFG_CONTAINER_FILTER_CONTIGUOUS
//   configuration: enable filter for contiguous (array-like) containers.
// Applies to: array, vector, buffer, matrix, bit, stack.
// Inherits from D_CFG_CONTAINER_FILTER.
#ifndef D_CFG_CONTAINER_FILTER_CONTIGUOUS
    #define D_CFG_CONTAINER_FILTER_CONTIGUOUS D_CFG_CONTAINER_FILTER
#endif  // D_CFG_CONTAINER_FILTER_CONTIGUOUS

// D_CFG_CONTAINER_FILTER_ASSOCIATIVE
//   configuration: enable filter for associative (key-value) containers.
// Applies to: dictionary, hash, map, table, registry.
// Inherits from D_CFG_CONTAINER_FILTER.
#ifndef D_CFG_CONTAINER_FILTER_ASSOCIATIVE
    #define D_CFG_CONTAINER_FILTER_ASSOCIATIVE D_CFG_CONTAINER_FILTER
#endif  // D_CFG_CONTAINER_FILTER_ASSOCIATIVE

// D_CFG_CONTAINER_FILTER_LINKED
//   configuration: enable filter for linked (pointer-based) containers.
// Applies to: list, graph, tree, node.
// Inherits from D_CFG_CONTAINER_FILTER.
#ifndef D_CFG_CONTAINER_FILTER_LINKED
    #define D_CFG_CONTAINER_FILTER_LINKED D_CFG_CONTAINER_FILTER
#endif  // D_CFG_CONTAINER_FILTER_LINKED

// D_CFG_CONTAINER_FILTER_SET
//   configuration: enable filter for set-like containers.
// Applies to: set.
// Inherits from D_CFG_CONTAINER_FILTER.
#ifndef D_CFG_CONTAINER_FILTER_SET
    #define D_CFG_CONTAINER_FILTER_SET D_CFG_CONTAINER_FILTER
#endif  // D_CFG_CONTAINER_FILTER_SET


///////////////////////////////////////////////////////////////////////////////
///             III.  ADDITIONAL CATEGORY GROUPINGS                         ///
///////////////////////////////////////////////////////////////////////////////
// These are convenience aliases that group categories differently. Each
// inherits from D_CFG_CONTAINER_FILTER unless overridden.

// D_CFG_CONTAINER_FILTER_ORDERED
//   configuration: enable filter for ordered containers (those that maintain
// insertion or sorted order).
// Applies to: array, vector, list, stack.
// Inherits from D_CFG_CONTAINER_FILTER.
#ifndef D_CFG_CONTAINER_FILTER_ORDERED
    #define D_CFG_CONTAINER_FILTER_ORDERED D_CFG_CONTAINER_FILTER
#endif  // D_CFG_CONTAINER_FILTER_ORDERED

// D_CFG_CONTAINER_FILTER_INDEXED
//   configuration: enable filter for containers supporting O(1) indexed
// access.
// Applies to: array, vector, buffer, matrix, bit.
// Inherits from D_CFG_CONTAINER_FILTER.
#ifndef D_CFG_CONTAINER_FILTER_INDEXED
    #define D_CFG_CONTAINER_FILTER_INDEXED D_CFG_CONTAINER_FILTER
#endif  // D_CFG_CONTAINER_FILTER_INDEXED

// D_CFG_CONTAINER_FILTER_HIERARCHICAL
//   configuration: enable filter for hierarchical (tree-based) containers.
// Applies to: tree, graph.
// Inherits from D_CFG_CONTAINER_FILTER.
#ifndef D_CFG_CONTAINER_FILTER_HIERARCHICAL
    #define D_CFG_CONTAINER_FILTER_HIERARCHICAL D_CFG_CONTAINER_FILTER
#endif  // D_CFG_CONTAINER_FILTER_HIERARCHICAL


///////////////////////////////////////////////////////////////////////////////
///             IV.   INTERNAL RESOLUTION HELPERS                           ///
///////////////////////////////////////////////////////////////////////////////
// These macros are used by individual container headers to resolve their
// own toggle from the category they belong to.  They are NOT intended for
// direct use by consumers.

// D_INTERNAL_CFG_CONTAINER_RESOLVE
//   macro: resolves a per-container toggle from its category default.
// If D_CFG_CONTAINER_FILTER_<TYPE> is already defined, it is kept.
// Otherwise it inherits from _category_default.
// This macro is expanded in the container's own header, not here.
#define D_INTERNAL_CFG_CONTAINER_RESOLVE(type_upper, category_default)       \
    D_CFG_CONTAINER_FILTER_##type_upper


#endif  // DJINTERP_C_CONTAINER_CONFIG_
