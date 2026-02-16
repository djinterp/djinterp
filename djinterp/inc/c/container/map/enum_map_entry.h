/******************************************************************************
* djinterp [container]                                        enum_map_entry.h
*
*   Defines the key-value entry structure used by d_enum_map. Each entry maps
* an integer enum key to an arbitrary pointer value.
*
*
* path:      \inc\container\map\enum_map_entry.h 
* link:      TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.11.27
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ENUM_MAP_ENTRY_
#define	DJINTERP_C_CONTAINER_ENUM_MAP_ENTRY_ 1

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\djinterp.h"
#include "..\container.h"


// d_enum_map_entry
//   struct: a single key-value pair mapping an integer (representing an enum)
// to an arbitrary pointer value.
struct d_enum_map_entry
{
    int   key;       // integer representing enum values
    void* value;     // associated value
};

// D_ENUM_ENTRY
//   macro: creates a d_enum_map_entry initializer from key and value.
//   Usage: D_ENUM_ENTRY(MY_ENUM_VALUE, ptr_to_data)
#define D_ENUM_ENTRY(_key, _val)                                              \
    {                                                                         \
      .key   = (int)(_key),                                                   \
      .value = (void*)(_val)                                                  \
    }

// D_ENUM_KEY_ENTRY (legacy alias)
//   macro: alias for D_ENUM_ENTRY for backwards compatibility.
#define D_ENUM_KEY_ENTRY(_key, _val)  D_ENUM_ENTRY(_key, _val)

// D_ENUM_ENTRY_STR
//   macro: creates a d_enum_map_entry with a string literal value.
//   Usage: D_ENUM_ENTRY_STR(MY_ENUM_VALUE, "string value")
#define D_ENUM_ENTRY_STR(_key, _str)                                          \
    {                                                                         \
      .key   = (int)(_key),                                                   \
      .value = (void*)(_str)                                                  \
    }

// D_ENUM_ENTRY_INT
//   macro: creates a d_enum_map_entry with an integer value cast to void*.
//   Usage: D_ENUM_ENTRY_INT(MY_ENUM_VALUE, 42)
//   Note: only use for integers that fit in a pointer (intptr_t range).
#define D_ENUM_ENTRY_INT(_key, _int_val)                                      \
    {                                                                         \
      .key   = (int)(_key),                                                   \
      .value = (void*)(intptr_t)(_int_val)                                    \
    }

// D_ENUM_ENTRY_NULL
//   macro: creates a d_enum_map_entry with a NULL value.
//   Usage: D_ENUM_ENTRY_NULL(MY_ENUM_VALUE)
#define D_ENUM_ENTRY_NULL(_key)                                               \
    {                                                                         \
      .key   = (int)(_key),                                                   \
      .value = NULL                                                           \
    }

// D_ENUM_ENTRY_SELF
//   macro: creates a d_enum_map_entry where the value equals the key (as int).
//   Useful for identity mappings.
//   Usage: D_ENUM_ENTRY_SELF(MY_ENUM_VALUE)
#define D_ENUM_ENTRY_SELF(_key)                                               \
    {                                                                         \
      .key   = (int)(_key),                                                   \
      .value = (void*)(intptr_t)(_key)                                        \
    }


///////////////////////////////////////////////////////////////////////////////
///                      ENTRY COMPARISON MACROS                            ///
///////////////////////////////////////////////////////////////////////////////

// D_ENUM_ENTRY_KEY_EQ
//   macro: returns true if two entries have the same key.
#define D_ENUM_ENTRY_KEY_EQ(_e1, _e2)                                         \
    ((_e1).key == (_e2).key)

// D_ENUM_ENTRY_VAL_EQ
//   macro: returns true if two entries have the same value.
#define D_ENUM_ENTRY_VAL_EQ(_e1, _e2)                                         \
    ((_e1).value == (_e2).value)

// D_ENUM_ENTRY_EQ
//   macro: returns true if two entries are equal (same key and value).
#define D_ENUM_ENTRY_EQ(_e1, _e2)                                             \
    ( ((_e1).key == (_e2).key) && ((_e1).value == (_e2).value) )


///////////////////////////////////////////////////////////////////////////////
///                        SENTINEL MARKERS                                 ///
///////////////////////////////////////////////////////////////////////////////

// D_ENUM_MAP_SENTINEL_KEY
//   constant: special key value used to mark the end of static entry arrays.
//   Uses INT_MIN to minimize collision with valid enum values.
#ifndef D_ENUM_MAP_SENTINEL_KEY
    #define D_ENUM_MAP_SENTINEL_KEY INT_MIN
#endif

// D_ENUM_ENTRY_SENTINEL
//   macro: creates a sentinel entry to mark end of static arrays.
#define D_ENUM_ENTRY_SENTINEL                                                 \
    {                                                                         \
      .key   = D_ENUM_MAP_SENTINEL_KEY,                                       \
      .value = NULL                                                           \
    }

// D_ENUM_ENTRY_IS_SENTINEL
//   macro: returns true if an entry is the sentinel marker.
#define D_ENUM_ENTRY_IS_SENTINEL(_entry)                                      \
    ((_entry).key == D_ENUM_MAP_SENTINEL_KEY)


#endif	// DJINTERP_C_CONTAINER_ENUM_MAP_ENTRY_
