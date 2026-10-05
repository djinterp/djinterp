/*******************************************************************************
* djinterp [c]                                                 registry_common.h
*
*   Shared types/utilities for:
*     1) in-memory djinterp cvar registry (Source-style cvars)
*     2) Windows Registry wrapper/sync helpers
*
*   NOTE: The cvar registry struct is `d_cvar_registry` to avoid collision
*   with the general-purpose `d_registry` in registry.h.  Both headers may
*   be included in the same translation unit.
*
*
* path:      /inc/djinterp/c/container/registry/registry_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.16
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_REGISTRY_REGISTRY_COMMON_H
#define DJINTERP_C_CONTAINER_REGISTRY_REGISTRY_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "../../meta/type_info.h"
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // uint16_t, uint32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// D_CVAR_REGISTRY_FLAG_<NONE-OWNS_VALUES>
//   macro: bit positions of the `flags` member of `d_cvar_registry`. They
// describe the registry as a whole: how keys are matched, and whether the
// registry frees the values it holds.
#define D_CVAR_REGISTRY_FLAG_NONE              0x00000000u
#define D_CVAR_REGISTRY_FLAG_CASE_SENSITIVE    0x00000001u
#define D_CVAR_REGISTRY_FLAG_OWNS_VALUES       0x00000002u

// D_REGISTRY_VALUE_FLAG_<NONE-INITIALIZED>
//   macro: bit positions of the `flags` member of `d_registry_value`. They
// describe one slot: whether it currently holds a value, whether that value's
// memory belongs to the registry, and whether the schema has assigned it a
// type and default.
#define D_REGISTRY_VALUE_FLAG_NONE             0x00000000u
#define D_REGISTRY_VALUE_FLAG_HAS_VALUE        0x00000001u
#define D_REGISTRY_VALUE_FLAG_OWNED            0x00000002u
#define D_REGISTRY_VALUE_FLAG_INITIALIZED      0x00000004u


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
    const void*   default_value;  // pointer default, or address of a scalar
    const char*   description;    // optional help text
};

// d_registry_value
//   type: mutable value slot keyed by enum_key.
//   notes:
//     - default_value is kept const
//     - the current value may be owned by the registry (the OWNED flag), in
//       which case free_fn releases it.
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

// D_REGISTRY_SCHEMA_ROW
//   macro: brace initializer for one `d_registry_schema_row`. Casts each field
// to its declared type so a schema table can be written as a list of rows
// without a cast at every entry. Alias rows repeat `_enum_key` to share a
// single value slot.
#define D_REGISTRY_SCHEMA_ROW(_key, _abbr, _enum_key, _type, _default, _desc) \
    {                                                                         \
        (_key),                                                               \
        (_abbr),                                                              \
        (uint16_t)(_enum_key),                                                \
        (d_type_info64)(_type),                                               \
        (const void*)(_default),                                              \
        (_desc)                                                               \
    }


//==============================================================================
// Shared function declarations (implemented in registry_common.c)
//==============================================================================

int d_registry_strcmp(const char* _a,
                      const char* _b,
                      bool        _case_sensitive);

uint16_t d_registry_schema_max_enum_key(
    const struct d_registry_schema_row* _schema,
    size_t                              _schema_count);


#endif  // defined(INT64_MAX)

#endif // DJINTERP_C_CONTAINER_REGISTRY_REGISTRY_COMMON_H
