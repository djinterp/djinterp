/*******************************************************************************
* djinterp [c][test]                                              test_options.c
*
* The option set of test_options.h: creating, copying and releasing one, and
* reading and overriding the registry's config keys through it.
*   An override lives in the option set's dj_min_enum_map, keyed by the
* DTestConfigKey, and is stored as the value itself, cast to a pointer. A read
* takes the override when there is one and the registry's default otherwise;
* the message flags are not an override at all, but the option set's `flags`.
*
*
* path:      /src/djinterp/c/modules/test/test_options.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_options.h"  // corresponding header
// std
#include <stdlib.h>  // calloc, free
// djinterp
#include "../../../../../inc/djinterp/c/container/registry/registry.h"  // dj_registry
#include "../../../../../inc/djinterp/c/meta/type_info.h"               // D_TYPE_INFO_*


//==============================================================================
// 1.  INTERNAL CONSTANTS
//==============================================================================

// D_TEST_OPTIONS_KEY_INVALID
//   constant: what d_test_options_key_from_string returns for a key that is not
// found or is not a config entry. Not one of DTestConfigKey's values: an error
// sentinel.
#ifndef D_TEST_OPTIONS_KEY_INVALID
    #define D_TEST_OPTIONS_KEY_INVALID ((enum DTestConfigKey)UINT32_MAX)
#endif  // D_TEST_OPTIONS_KEY_INVALID

// dj_test_options_override_row
//   struct: an override as a registry row, its key the test registry's primary
// key (first, as dj_registry requires). Unused while the overrides live in a
// dj_min_enum_map.
struct dj_test_options_override_row
{
    const char*         key;         // MUST BE FIRST
    uint32_t            flag;        // DTestConfigKey
    uint32_t            value_type;  // d_type_info
    union dj_test_value value;       // override value
};

//==============================================================================
// 2.  INTERNAL HELPERS
//==============================================================================

/**
 * @brief Makes the test registry's one-time initialization.
 *
 * d_test_registry_init does the work, once; every public function here that
 * reads the registry calls this first.
 */
static void
d_internal_test_registry_init_once(void)
{
    d_test_registry_init();

    return;
}

/**
 * @brief Finds the config row with a flag.
 *
 * @param[in] _flag  a DTestConfigKey.
 * @return the row, or `NULL` if no config row has `_flag`.
 *
 * DTestMetadataFlag starts at 0x100, past every DTestConfigKey, so a flag
 * names one row; the row must also be a config row, so a metadata flag never
 * reads as a config key. D_TEST_REGISTRY_FOREACH declares `row` itself, in its
 * loop.
 */
static const struct dj_test_registry_row*
d_internal_test_registry_config_row_from_flag(
    uint32_t _flag
)
{
    D_TEST_REGISTRY_FOREACH(row)
    {
        // the flag, on a config row
        if ( (row->flag == _flag) &&
             ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0) )
        {
            return row;
        }
    }

    return NULL;
}

/**
 * @brief Whether an option set overrides a config key.
 *
 * @param[in] _config  the option set; may be `NULL`.
 * @param[in] _flag    the config key.
 * @return `true` if `_config` has an override for `_flag`.
 */
static bool
d_internal_test_options_has_override(
    const struct dj_test_options* _config,
    uint32_t                      _flag
)
{
    // parameter validation; no map, no overrides
    if ( (!_config) ||
         (!_config->settings) )
    {
        return false;
    }

    return d_min_enum_map_contains(_config->settings,
                                   (int)_flag);
}

/**
 * @brief An option set's override of a config key.
 *
 * @param[in] _config  the option set; may be `NULL`.
 * @param[in] _flag    the config key.
 * @return the override, the value itself cast to a pointer; `NULL` if there
 *         is none.
 */
static void*
d_internal_test_options_get_override_ptr(
    const struct dj_test_options* _config,
    uint32_t                      _flag
)
{
    // parameter validation; no map, no overrides
    if ( (!_config) ||
         (!_config->settings) )
    {
        return NULL;
    }

    return d_min_enum_map_get(_config->settings,
                              (int)_flag);
}

/**
 * @brief Overrides a config key in an option set.
 *
 * @param[in,out] _config  the option set; may be `NULL`.
 * @param[in]     _flag    the config key.
 * @param[in]     _value   the value itself, cast to a pointer.
 * @return `true` if stored; `false` if `_config` or its map is `NULL`, or the
 *         map could not store it.
 */
static bool
d_internal_test_options_set_override(
    struct dj_test_options* _config,
    uint32_t                _flag,
    void*                   _value
)
{
    // parameter validation; no map, nowhere to store
    if ( (!_config) ||
         (!_config->settings) )
    {
        return false;
    }

    return d_min_enum_map_put(_config->settings,
                              (int)_flag,
                              _value);
}

/**
 * @brief Reads a config key through an option set.
 *
 * @param[in] _config  the option set; may be `NULL`, which reads the default.
 * @param[in] _key     the config key.
 * @return the override, else the registry's default; a `NULL` pointer value
 *         if `_key` is not a config key.
 *
 * The message flags are the option set's own `flags`, not an override. An
 * override comes back in the union's pointer member, and the getters read the
 * member their type needs from it: the low bytes of the pointer, so this
 * assumes a little-endian target for anything narrower than a pointer.
 */
static union dj_test_value
d_internal_test_options_get_value(
    const struct dj_test_options* _config,
    uint32_t                      _key
)
{
    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key
    if (!schema_row)
    {
        return (union dj_test_value){ .ptr = NULL };
    }

    // message flags live directly on the config object
    if (_key == D_TEST_OPTIONS_MESSAGE_FLAGS)
    {
        // the option set's flags, when there is an option set
        if (_config)
        {
            return (union dj_test_value){ .u32 = _config->flags };
        }

        // no config => schema default
        return schema_row->value;
    }

    // no option set, no overrides: the default
    if (!_config)
    {
        return schema_row->value;
    }

    // the override, when there is one; pointer-sized values stored directly
    if (d_internal_test_options_has_override(_config,
                                             _key))
    {
        void* const override_ptr =
            d_internal_test_options_get_override_ptr(_config,
                                                     _key);

        return (union dj_test_value){ .ptr = override_ptr };
    }

    return schema_row->value;
}

//==============================================================================
// 3.  PUBLIC API
//==============================================================================

/*
d_test_options_new
  The map of overrides is made with the option set, so no getter or setter
meets a missing map in an option set made here; there are no stage hooks until
something sets them.
*/
struct dj_test_options*
d_test_options_new(
    uint32_t _flags
)
{
    d_internal_test_registry_init_once();

    struct dj_test_options* const config =
        (struct dj_test_options*)calloc(1,
                                        sizeof(*config));

    // check if memory allocation was successful
    if (!config)
    {
        return NULL;
    }

    config->flags    = _flags;
    config->settings = d_min_enum_map_new();

    // without its map the option set is no use; release it
    if (!config->settings)
    {
        free(config);

        return NULL;
    }

    config->stage_hooks = NULL;

    return config;
}

/*
d_test_options_new_preset
  A preset is only a set of flags, so this is d_test_options_new.
*/
struct dj_test_options*
d_test_options_new_preset(
    uint32_t _preset
)
{
    return d_test_options_new(_preset);
}

/*
d_test_options_new_copy
  The copy always has a map: the source's, copied, or an empty one when the
source has none. The stage hooks are copied when the source has them; a copy
of them that fails leaves the copy without hooks rather than failing it.
*/
struct dj_test_options*
d_test_options_new_copy(
    const struct dj_test_options* _other
)
{
    // parameter validation
    if (!_other)
    {
        return NULL;
    }

    d_internal_test_registry_init_once();

    struct dj_test_options* const copy =
        (struct dj_test_options*)calloc(1,
                                        sizeof(*copy));

    // check if memory allocation was successful
    if (!copy)
    {
        return NULL;
    }

    copy->flags = _other->flags;

    // the source's overrides, or an empty map when it has none
    if (_other->settings)
    {
        copy->settings = d_min_enum_map_new_copy(_other->settings);

        // the copy of the map failed; release the option set
        if (!copy->settings)
        {
            free(copy);

            return NULL;
        }
    }
    else
    {
        copy->settings = d_min_enum_map_new();

        // the new map failed; release the option set
        if (!copy->settings)
        {
            free(copy);

            return NULL;
        }
    }

    // the stage hooks, when the source has them
    if (_other->stage_hooks)
    {
        copy->stage_hooks = d_min_enum_map_new_copy(_other->stage_hooks);
    }
    else
    {
        copy->stage_hooks = NULL;
    }

    return copy;
}

/*
d_test_options_free
  Each map is released, and its pointer cleared, before the option set itself.
*/
void
d_test_options_free(
    struct dj_test_options* _config
)
{
    // parameter validation
    if (!_config)
    {
        return;
    }

    // the overrides
    if (_config->settings)
    {
        d_min_enum_map_free(_config->settings);
        _config->settings = NULL;
    }

    // the stage hooks
    if (_config->stage_hooks)
    {
        d_min_enum_map_free(_config->stage_hooks);
        _config->stage_hooks = NULL;
    }

    free(_config);

    return;
}

//==============================================================================
// 4.  GETTERS
//==============================================================================

/*
d_test_options_get_bool
  The union's bool member, whatever the key's type.
*/
bool
d_test_options_get_bool(
    const struct dj_test_options* _config,
    uint32_t                      _key
)
{
    const union dj_test_value v = d_internal_test_options_get_value(_config,
                                                                    _key);

    return v.b;
}

/*
d_test_options_get_size_t
  The key's row gives its type; a uint16_t or uint32_t value is widened.
*/
size_t
d_test_options_get_size_t(
    const struct dj_test_options* _config,
    uint32_t                      _key
)
{
    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key
    if (!schema_row)
    {
        return 0;
    }

    const union dj_test_value v = d_internal_test_options_get_value(_config,
                                                                    _key);

    // a size_t key
    if (schema_row->value_type == D_TYPE_INFO_SIZE_T)
    {
        return v.z;
    }

    // a uint16_t key, widened
    if (schema_row->value_type == D_TYPE_INFO_UINT16)
    {
        return (size_t)v.u16;
    }

    // a uint32_t key, widened
    if (schema_row->value_type == D_TYPE_INFO_UINT32)
    {
        return (size_t)v.u32;
    }

    return 0;
}

/*
d_test_options_get_int32
  An int32_t key only.
*/
int32_t
d_test_options_get_int32(
    const struct dj_test_options* _config,
    uint32_t                      _key
)
{
    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key, or not an int32_t one
    if ( (!schema_row) ||
         (schema_row->value_type != D_TYPE_INFO_INT32) )
    {
        return 0;
    }

    const union dj_test_value v = d_internal_test_options_get_value(_config,
                                                                    _key);

    return v.i32;
}

/*
d_test_options_get_uint32
  The key's row gives its type; a uint16_t value is widened, a size_t one
narrowed.
*/
uint32_t
d_test_options_get_uint32(
    const struct dj_test_options* _config,
    uint32_t                      _key
)
{
    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key
    if (!schema_row)
    {
        return 0;
    }

    const union dj_test_value v = d_internal_test_options_get_value(_config,
                                                                    _key);

    // a uint32_t key
    if (schema_row->value_type == D_TYPE_INFO_UINT32)
    {
        return v.u32;
    }

    // a uint16_t key, widened
    if (schema_row->value_type == D_TYPE_INFO_UINT16)
    {
        return (uint32_t)v.u16;
    }

    // a size_t key, narrowed
    if (schema_row->value_type == D_TYPE_INFO_SIZE_T)
    {
        return (uint32_t)v.z;
    }

    return 0;
}

/*
d_test_options_get_string
  A string key only.
*/
const char*
d_test_options_get_string(
    const struct dj_test_options* _config,
    uint32_t                      _key
)
{
    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key, or not a string one
    if ( (!schema_row) ||
         (schema_row->value_type != D_TYPE_INFO_STRING) )
    {
        return NULL;
    }

    const union dj_test_value v = d_internal_test_options_get_value(_config,
                                                                    _key);

    return (const char*)v.ptr;
}

/*
d_test_options_get_ptr
  Pointer-like config entries may be D_TYPE_INFO_VOID_PTR, D_TYPE_INFO_STRING
or D_TYPE_INFO_STRING_ARRAY; the raw pointer is returned for all of them, and
for any other config key too.
*/
void*
d_test_options_get_ptr(
    const struct dj_test_options* _config,
    uint32_t                      _key
)
{
    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key
    if (!schema_row)
    {
        return NULL;
    }

    const union dj_test_value v = d_internal_test_options_get_value(_config,
                                                                    _key);

    return (void*)v.ptr;
}

//==============================================================================
// 5.  SETTERS
//==============================================================================

/*
d_test_options_set_bool
  The value is stored as an override, cast to a pointer through uintptr_t.
*/
bool
d_test_options_set_bool(
    struct dj_test_options* _config,
    uint32_t                _key,
    bool                    _value
)
{
    // parameter validation
    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key, or not a bool one
    if ( (!schema_row) ||
         (schema_row->value_type != D_TYPE_INFO_BOOL) )
    {
        return false;
    }

    return d_internal_test_options_set_override(_config,
                                                _key,
                                                (void*)(uintptr_t)_value);
}

/*
d_test_options_set_size_t
  Any of the three unsigned types takes the value, stored as an override cast
to a pointer through uintptr_t; the getters narrow it as the key's type needs.
*/
bool
d_test_options_set_size_t(
    struct dj_test_options* _config,
    uint32_t                _key,
    size_t                  _value
)
{
    // parameter validation
    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key
    if (!schema_row)
    {
        return false;
    }

    // a size_t, uint16_t or uint32_t key
    if ( (schema_row->value_type == D_TYPE_INFO_SIZE_T) ||
         (schema_row->value_type == D_TYPE_INFO_UINT16) ||
         (schema_row->value_type == D_TYPE_INFO_UINT32) )
    {
        return d_internal_test_options_set_override(_config,
                                                    _key,
                                                    (void*)(uintptr_t)_value);
    }

    return false;
}

/*
d_test_options_set_int32
  The value is stored as an override, cast to a pointer through intptr_t, so a
negative value keeps its sign.
*/
bool
d_test_options_set_int32(
    struct dj_test_options* _config,
    uint32_t                _key,
    int32_t                 _value
)
{
    // parameter validation
    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key, or not an int32_t one
    if ( (!schema_row) ||
         (schema_row->value_type != D_TYPE_INFO_INT32) )
    {
        return false;
    }

    return d_internal_test_options_set_override(_config,
                                                _key,
                                                (void*)(intptr_t)_value);
}

/*
d_test_options_set_uint32
  The message flags are the option set's own `flags`, set directly and never an
override; any other unsigned key takes an override, as in
d_test_options_set_size_t.
*/
bool
d_test_options_set_uint32(
    struct dj_test_options* _config,
    uint32_t                _key,
    uint32_t                _value
)
{
    // parameter validation
    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    // the message flags are the option set's own
    if (_key == D_TEST_OPTIONS_MESSAGE_FLAGS)
    {
        _config->flags = _value;

        return true;
    }

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key
    if (!schema_row)
    {
        return false;
    }

    // a uint32_t, uint16_t or size_t key
    if ( (schema_row->value_type == D_TYPE_INFO_UINT32) ||
         (schema_row->value_type == D_TYPE_INFO_UINT16) ||
         (schema_row->value_type == D_TYPE_INFO_SIZE_T) )
    {
        return d_internal_test_options_set_override(_config,
                                                    _key,
                                                    (void*)(uintptr_t)_value);
    }

    return false;
}

/*
d_test_options_set_string
  The pointer is the override; the string is not copied.
*/
bool
d_test_options_set_string(
    struct dj_test_options* _config,
    uint32_t                _key,
    const char*             _value
)
{
    // parameter validation
    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key, or not a string one
    if ( (!schema_row) ||
         (schema_row->value_type != D_TYPE_INFO_STRING) )
    {
        return false;
    }

    return d_internal_test_options_set_override(_config,
                                                _key,
                                                (void*)_value);
}

/*
d_test_options_set_ptr
  The pointer is the override, whatever the key's type.
*/
bool
d_test_options_set_ptr(
    struct dj_test_options* _config,
    uint32_t                _key,
    void*                   _value
)
{
    // parameter validation
    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* schema_row =
        d_internal_test_registry_config_row_from_flag(_key);

    // not a config key
    if (!schema_row)
    {
        return false;
    }

    return d_internal_test_options_set_override(_config,
                                                _key,
                                                _value);
}

//==============================================================================
// 6.  KEY LOOKUP
//==============================================================================

/*
d_test_options_key_from_string
  The registry resolves the name, aliases included; the row must then be a
config row, so a metadata key does not come back as a config key.
*/
enum DTestConfigKey
d_test_options_key_from_string(
    const char* _key
)
{
    // parameter validation
    if (!_key)
    {
        return D_TEST_OPTIONS_KEY_INVALID;
    }

    d_internal_test_registry_init_once();

    struct dj_test_registry_row* const row = d_test_registry_find(_key);

    // unknown, or not a config row
    if ( (!row) ||
         ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) == 0) )
    {
        return D_TEST_OPTIONS_KEY_INVALID;
    }

    return (enum DTestConfigKey)row->flag;
}
