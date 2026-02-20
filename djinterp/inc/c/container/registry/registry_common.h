/*******************************************************************************
* djinterp [container]                                        registry_common.h
*
*   Shared types/utilities for:
*     1) in-memory djinterp cvar registry (Source-style cvars)
*     2) Windows Registry wrapper/sync helpers
*
*   NOTE: The cvar registry struct is `d_cvar_registry` to avoid collision
*   with the general-purpose `d_registry` in registry.h.  Both headers may
*   be included in the same translation unit.
*
* path:      \inc\container\registry\registry_common.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.12.16
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_REGISTRY_COMMON_
#define DJINTERP_C_CONTAINER_REGISTRY_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include "..\..\djinterp.h"
#include "..\..\meta\type_info.h"



// `d_cvar_registry` flags
#define D_CVAR_REGISTRY_FLAG_NONE              0x00000000u
#define D_CVAR_REGISTRY_FLAG_CASE_SENSITIVE    0x00000001u
#define D_CVAR_REGISTRY_FLAG_OWNS_VALUES       0x00000002u

// `d_registry_value` flags
#define D_REGISTRY_VALUE_FLAG_NONE             0x00000000u
#define D_REGISTRY_VALUE_FLAG_HAS_VALUE        0x00000001u  // current value set
#define D_REGISTRY_VALUE_FLAG_OWNED            0x00000002u  // registry owns current value memory
#define D_REGISTRY_VALUE_FLAG_INITIALIZED      0x00000004u  // schema assigned type/default


//==============================================================================
// Schema vs Values
//
// - Schema rows can have duplicates (aliases), but they must share a single
//   value slot via enum_key.
// - Values are stored densely by enum_key (0..max_enum).
//==============================================================================

// d_registry_schema_row
//   type: immutable lookup row (may be duplicated for aliases).
struct d_registry_schema_row
{
    const char*   key;            // canonical key (or alias)
    const char*   abbreviation;   // optional short key (or alias), may be NULL
    uint16_t      enum_key;       // dense enum index (preferred)
    d_type_info64 type;           // D_TYPE_INFO_*
    const void*   default_value;  // pointer-typed default (or address of scalar default)
    const char*   description;    // optional help text
};

// d_registry_value
//   type: mutable value slot keyed by enum_key.
//   notes:
//     - default_value is kept const
//     - current value may be owned by registry (OWNED flag) and freed by free_fn.
struct d_registry_value
{
    d_type_info64  type;
    const void*    default_value;

    void*          value;          // current value (pointer or boxed bytes)
    fn_free        free_fn;        // optional destructor for owned values
    uint32_t       flags;
};

// d_cvar_registry
//   type: schema + dense values map for typed console variables.
//   notes:
//     - Renamed from `d_registry` to avoid collision with the general-purpose
//       d_registry defined in registry.h.
struct d_cvar_registry
{
    const struct d_registry_schema_row* schema;
    size_t                              schema_count;

    struct d_registry_value*            values;        // indexed by enum_key
    size_t                              values_count;  // typically max_enum + 1

    uint32_t                            flags;
};


//==============================================================================
// Convenience schema initializer
//==============================================================================

#define D_REGISTRY_SCHEMA_ROW(_key, _abbr, _enum_key, _type, _default, _desc) \
    {                                                                        \
        (_key),                                                              \
        (_abbr),                                                             \
        (uint16_t)(_enum_key),                                               \
        (d_type_info64)(_type),                                              \
        (const void*)(_default),                                             \
        (_desc)                                                              \
    }


//==============================================================================
// Shared function declarations (implemented in registry_common.c)
//==============================================================================

int      d_registry_strcmp(const char* _a, const char* _b, bool _case_sensitive);

uint16_t d_registry_schema_max_enum_key(const struct d_registry_schema_row* _schema, size_t _schema_count);


#endif // DJINTERP_C_CONTAINER_REGISTRY_COMMON_
