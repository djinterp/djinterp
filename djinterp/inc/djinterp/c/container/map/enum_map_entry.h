/*******************************************************************************
* djinterp [c]                                                  enum_map_entry.h
*
*   Defines the key-value entry structure used by d_enum_map. Each entry maps
* an integer enum key to an arbitrary pointer value.
*
*
* path:      /inc/djinterp/c/container/map/enum_map_entry.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.11.27
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_MAP_ENUM_MAP_ENTRY_H
#define DJINTERP_C_CONTAINER_MAP_ENUM_MAP_ENTRY_H 1

// std
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../container.h"
#include "./map.h"
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // int32_t, intptr_t


// d_enum_map_entry
//   struct: a single key-value pair mapping an integer (representing an enum)
// to an arbitrary pointer value.
struct d_enum_map_entry
{
    int32_t key;       // integer representing enum values
    void*   value;     // associated value
};


// D_ENUM_ENTRY
//   macro: creates a d_key_value initializer from an enum key and a pointer
// value, casting the key to int32_t and the value to void*.
#define D_ENUM_ENTRY(_key, _val) \
    D_KEY_VALUE_T(_key, int32_t, _val, void*)

// D_ENUM_ENTRY_T
//   macro: creates a d_key_value initializer from an enum key and value,
// casting each to the specified types.
#define D_ENUM_ENTRY_T(_key, _type1, _val, _type2) \
    D_KEY_VALUE_T(_key, _type1, _val, _type2)

// D_ENUM_KEY_ENTRY
//   macro: legacy alias for D_ENUM_ENTRY.
#define D_ENUM_KEY_ENTRY(_key, _val) \
    D_ENUM_ENTRY(_key, _val)

// D_ENUM_ENTRY_STR
//   macro: creates a d_key_value initializer from an enum key and a string
// literal, casting the key to int and the string to void*.
#define D_ENUM_ENTRY_STR(_key, _str) \
    D_KEY_VALUE_T(_key, int, _str, void*)

// D_ENUM_ENTRY_INT
//   macro: creates a d_key_value initializer from an enum key and an integer
// value. the integer is cast through intptr_t to void*. only use for integers
// that fit in a pointer.
#define D_ENUM_ENTRY_INT(_key, _int_val) \
    D_KEY_VALUE_T(_key, int, (intptr_t)(_int_val), void*)

// D_ENUM_ENTRY_NULL
//   macro: creates a d_key_value initializer from an enum key with a NULL
// value.
#define D_ENUM_ENTRY_NULL(_key) \
    D_KEY_VALUE_T(_key, int, NULL, void*)

// D_ENUM_ENTRY_SELF
//   macro: creates a d_key_value initializer where the value equals the key,
// cast through intptr_t to void*. useful for identity mappings.
#define D_ENUM_ENTRY_SELF(_key) \
    D_KEY_VALUE_T(_key, int, (intptr_t)(_key), void*)

// D_ENUM_ENTRY_KEY_EQ
//   macro: evaluates to true if two enum entries have the same key. delegates
// to D_KEY_VALUE_KEY_EQ.
#define D_ENUM_ENTRY_KEY_EQ(_e1, _e2) \
    D_KEY_VALUE_KEY_EQ(_e1, _e2)

// D_ENUM_ENTRY_VAL_EQ
//   macro: evaluates to true if two enum entries have the same value.
// delegates to D_KEY_VALUE_VAL_EQ.
#define D_ENUM_ENTRY_VAL_EQ(_e1, _e2) \
    D_KEY_VALUE_VAL_EQ(_e1, _e2)

// D_ENUM_ENTRY_EQ
//   macro: evaluates to true if two enum entries are equal in both key and
// value. delegates to D_KEY_VALUE_EQ.
#define D_ENUM_ENTRY_EQ(_e1, _e2) \
    D_KEY_VALUE_EQ(_e1, _e2)

// D_ENUM_MAP_SENTINEL_KEY
//   constant: legacy alias for D_KEY_VALUE_SENTINEL_KEY.
#define D_ENUM_MAP_SENTINEL_KEY \
    D_KEY_VALUE_SENTINEL_KEY

// D_ENUM_ENTRY_SENTINEL
//   macro: legacy alias for D_KEY_VALUE_SENTINEL.
#define D_ENUM_ENTRY_SENTINEL \
    D_KEY_VALUE_SENTINEL

// D_ENUM_ENTRY_IS_SENTINEL
//   macro: legacy alias for D_KEY_VALUE_IS_SENTINEL.
#define D_ENUM_ENTRY_IS_SENTINEL(_entry) \
    D_KEY_VALUE_IS_SENTINEL(_entry)


#endif  // DJINTERP_C_CONTAINER_MAP_ENUM_MAP_ENTRY_H
