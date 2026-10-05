/*******************************************************************************
* djinterp [c]                                                             map.h
*
*
* path:      /inc/djinterp/c/container/map/map.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.09.27
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_MAP_MAP_H
#define DJINTERP_C_CONTAINER_MAP_MAP_H 1

// djinterp
#include "../../djinterp.h"
#include "../container.h"


// D_KEY_VALUE
//   macro: creates a d_key_value initializer from a key and value without any
// type casting.
#define D_KEY_VALUE(_key, _val) \
    {                           \
      .key   = (_key),          \
      .value = (_val)           \
    }

// D_KEY_VALUE_T
//   macro: creates a d_key_value initializer from a key and value, casting
// each to the specified types.
#define D_KEY_VALUE_T(_key, _type1, _val, _type2) \
    {                                             \
      .key   = (_type1)(_key),                    \
      .value = (_type2)(_val)                     \
    }


///////////////////////////////////////////////////////////////////////////////
///                    ENTRY COMPARISON MACROS                              ///
///////////////////////////////////////////////////////////////////////////////

// D_KEY_VALUE_KEY_EQ
//   macro: evaluates to true if two d_key_value entries have the same key.
#define D_KEY_VALUE_KEY_EQ(_e1, _e2) \
    ((_e1).key == (_e2).key)

// D_KEY_VALUE_VAL_EQ
//   macro: evaluates to true if two d_key_value entries have the same value.
#define D_KEY_VALUE_VAL_EQ(_e1, _e2) \
    ((_e1).value == (_e2).value)

// D_KEY_VALUE_EQ
//   macro: evaluates to true if two d_key_value entries are equal in both key
// and value.
#define D_KEY_VALUE_EQ(_e1, _e2) \
    ( ((_e1).key == (_e2).key) && ((_e1).value == (_e2).value) )


///////////////////////////////////////////////////////////////////////////////
///                    KEY EXTRACTION MACRO                                 ///
///////////////////////////////////////////////////////////////////////////////

// D_KEY_VALUE_KEY_AS
//   macro: extracts the key from a d_key_value entry, cast to the specified
// type.
#define D_KEY_VALUE_KEY_AS(_entry, _type) \
    ((_type)((_entry).key))


///////////////////////////////////////////////////////////////////////////////
///                        SENTINEL MARKERS                                ///
///////////////////////////////////////////////////////////////////////////////

// D_KEY_VALUE_SENTINEL_KEY
//   constant: special key value used to mark the end of static d_key_value
// arrays. uses INT64_MIN to minimize collision with valid keys.
#ifndef D_KEY_VALUE_SENTINEL_KEY
    #define D_KEY_VALUE_SENTINEL_KEY INT64_MIN
#endif

// D_KEY_VALUE_SENTINEL
//   macro: creates a sentinel d_key_value entry to mark the end of a static
// array.
#define D_KEY_VALUE_SENTINEL             \
    {                                    \
      .key   = D_KEY_VALUE_SENTINEL_KEY, \
      .value = NULL                      \
    }

// D_KEY_VALUE_IS_SENTINEL
//   macro: evaluates to true if a d_key_value entry is the sentinel marker.
#define D_KEY_VALUE_IS_SENTINEL(_entry) \
    ((_entry).key == D_KEY_VALUE_SENTINEL_KEY)


#endif  // DJINTERP_C_CONTAINER_MAP_MAP_H
