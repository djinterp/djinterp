#include "..\..\inc\test\test_cvar.h"


/******************************************************************************
 * INTERNAL CONSTANTS
 *****************************************************************************/

 /*
 D_TEST_CONFIG_KEY_INVALID
   Pseudo-enum value returned by d_test_config_key_from_string() when a key
   is not found or is not a config entry.

   NOTE: This value is *not* part of enum DTestConfigKey; callers should treat
   it as an error sentinel.
 */
#ifndef D_TEST_CONFIG_KEY_INVALID
#define D_TEST_CONFIG_KEY_INVALID ((enum DTestConfigKey)UINT32_MAX)
#endif


 /******************************************************************************
  * OVERRIDE ROW TYPE (PER-CONFIG STORAGE)
  *****************************************************************************/

  /*
  d_test_config_override_row
    Internal per-config override row. Key is the canonical primary key from the
    test registry (must be FIRST to satisfy d_registry key rules).
  */
struct d_test_config_override_row
{
    const char*        key;        // MUST BE FIRST
    uint32_t           flag;       // DTestConfigKey
    uint32_t           value_type; // d_type_info
    union d_test_value value;      // override value
};


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

static void
d_internal_test_registry_init_once
(
    void
)
{
    d_test_registry_init();

    return;
}

static const struct d_test_registry_row*
d_internal_test_registry_config_row_from_flag
(
    uint32_t _flag
)
{
    struct d_test_registry_row* row;

    /*
      NOTE:
        DTestConfigKey and DTestMetadataFlag share the same numeric space
        (both start at 0). Therefore, a simple \"find by flag\" is ambiguous.
        We must search for a row that matches BOTH:
          - the flag value, and
          - the entry kind (IS_CONFIG).
    */
    row = NULL;

    D_TEST_REGISTRY_FOREACH(row)
    {
        if ((row->flag == _flag) &&
            ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0))
        {
            return row;
        }
    }

    return NULL;
}

/*
d_internal_test_config_has_override
  Checks if config has an override value for the given flag.
*/
static bool
d_internal_test_config_has_override
(
    const struct d_test_config* _config,
    uint32_t                    _flag
)
{
    if ( (!_config) || (!_config->settings) )
    {
        return false;
    }

    return d_min_enum_map_contains(_config->settings, (int)_flag);
}

/*
d_internal_test_config_get_override_ptr
  Gets the override value pointer for the given flag.
*/
static void*
d_internal_test_config_get_override_ptr
(
    const struct d_test_config* _config,
    uint32_t                    _flag
)
{
    if ( (!_config) || (!_config->settings) )
    {
        return NULL;
    }

    return d_min_enum_map_get(_config->settings, (int)_flag);
}

/*
d_internal_test_config_set_override
  Sets an override value for the given flag.
*/
static bool
d_internal_test_config_set_override
(
    struct d_test_config* _config,
    uint32_t              _flag,
    void*                 _value
)
{
    if ( (!_config) || (!_config->settings) )
    {
        return false;
    }

    return d_min_enum_map_put(_config->settings, (int)_flag, _value);
}

static union d_test_value
d_internal_test_config_get_value
(
    const struct d_test_config* _config,
    uint32_t                    _key
)
{
    const struct d_test_registry_row* schema_row;
    void*                             override_ptr;

    schema_row = d_internal_test_registry_config_row_from_flag(_key);

    if (!schema_row)
    {
        return (union d_test_value) { .ptr = NULL };
    }

    // message flags live directly on the config object
    if (_key == D_TEST_CONFIG_MESSAGE_FLAGS)
    {
        if (_config)
        {
            return (union d_test_value) { .u32 = _config->flags };
        }

        // no config => schema default
        return schema_row->value;
    }

    if (!_config)
    {
        return schema_row->value;
    }

    // check for override
    if (d_internal_test_config_has_override(_config, _key))
    {
        override_ptr = d_internal_test_config_get_override_ptr(_config, _key);

        // for pointer-sized values stored directly
        return (union d_test_value) { .ptr = override_ptr };
    }

    return schema_row->value;
}


/******************************************************************************
 * PUBLIC API
 *****************************************************************************/

struct d_test_config*
d_test_config_new
(
    uint32_t _flags
)
{
    struct d_test_config* config;

    d_internal_test_registry_init_once();

    config = (struct d_test_config*)calloc(1, sizeof(*config));

    if (!config)
    {
        return NULL;
    }

    config->flags    = _flags;
    config->settings = d_min_enum_map_new();

    if (!config->settings)
    {
        free(config);
        return NULL;
    }

    config->stage_hooks = NULL;

    return config;
}

struct d_test_config*
d_test_config_new_preset
(
    uint32_t _preset
)
{
    return d_test_config_new(_preset);
}

struct d_test_config*
    d_test_config_new_copy
    (
        const struct d_test_config* _other
    )
{
    struct d_test_config* copy;

    if (!_other)
    {
        return NULL;
    }

    d_internal_test_registry_init_once();

    copy = (struct d_test_config*)calloc(1, sizeof(*copy));

    if (!copy)
    {
        return NULL;
    }

    copy->flags = _other->flags;

    if (_other->settings)
    {
        copy->settings = d_min_enum_map_new_copy(_other->settings);

        if (!copy->settings)
        {
            free(copy);
            return NULL;
        }
    }
    else
    {
        copy->settings = d_min_enum_map_new();

        if (!copy->settings)
        {
            free(copy);
            return NULL;
        }
    }

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

void
d_test_config_free
(
    struct d_test_config* _config
)
{
    if (!_config)
    {
        return;
    }

    if (_config->settings)
    {
        d_min_enum_map_free(_config->settings);
        _config->settings = NULL;
    }

    if (_config->stage_hooks)
    {
        d_min_enum_map_free(_config->stage_hooks);
        _config->stage_hooks = NULL;
    }

    free(_config);

    return;
}


/******************************************************************************
 * GETTERS
 *****************************************************************************/

bool
d_test_config_get_bool
(
    const struct d_test_config* _config,
    uint32_t                    _key
)
{
    union d_test_value v;

    v = d_internal_test_config_get_value(_config, _key);

    return v.b;
}

size_t
d_test_config_get_size_t
(
    const struct d_test_config* _config,
    uint32_t                    _key
)
{
    const struct d_test_registry_row* schema_row;
    union d_test_value                v;

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);
    v = d_internal_test_config_get_value(_config, _key);

    if (!schema_row)
    {
        return 0;
    }

    if (schema_row->value_type == D_TYPE_INFO_SIZE_T)
    {
        return v.z;
    }

    if (schema_row->value_type == D_TYPE_INFO_UINT16)
    {
        return (size_t)v.u16;
    }

    if (schema_row->value_type == D_TYPE_INFO_UINT32)
    {
        return (size_t)v.u32;
    }

    return 0;
}

int32_t
d_test_config_get_int32
(
    const struct d_test_config* _config,
    uint32_t                    _key
)
{
    const struct d_test_registry_row* schema_row;
    union d_test_value                v;

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);
    v = d_internal_test_config_get_value(_config, _key);

    if ((!schema_row) || (schema_row->value_type != D_TYPE_INFO_INT32))
    {
        return 0;
    }

    return v.i32;
}

uint32_t
d_test_config_get_uint32
(
    const struct d_test_config* _config,
    uint32_t                    _key
)
{
    const struct d_test_registry_row* schema_row;
    union d_test_value                v;

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);
    v = d_internal_test_config_get_value(_config, _key);

    if (!schema_row)
    {
        return 0;
    }

    if (schema_row->value_type == D_TYPE_INFO_UINT32)
    {
        return v.u32;
    }

    if (schema_row->value_type == D_TYPE_INFO_UINT16)
    {
        return (uint32_t)v.u16;
    }

    if (schema_row->value_type == D_TYPE_INFO_SIZE_T)
    {
        return (uint32_t)v.z;
    }

    return 0;
}

const char*
d_test_config_get_string
(
    const struct d_test_config* _config,
    uint32_t                    _key
)
{
    const struct d_test_registry_row* schema_row;
    union d_test_value                v;

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);
    v = d_internal_test_config_get_value(_config, _key);

    if ((!schema_row) || (schema_row->value_type != D_TYPE_INFO_STRING))
    {
        return NULL;
    }

    return (const char*)v.ptr;
}

void*
d_test_config_get_ptr
(
    const struct d_test_config* _config,
    uint32_t                    _key
)
{
    const struct d_test_registry_row* schema_row;
    union d_test_value                v;

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);
    v = d_internal_test_config_get_value(_config, _key);

    if (!schema_row)
    {
        return NULL;
    }

    /*
      Pointer-like config entries may be D_TYPE_INFO_VOID_PTR, D_TYPE_INFO_STRING,
      or D_TYPE_INFO_STRING_ARRAY. We return the raw pointer for all of them.
    */
    return (void*)v.ptr;
}


/******************************************************************************
 * SETTERS
 *****************************************************************************/

bool
d_test_config_set_bool
(
    struct d_test_config* _config,
    uint32_t              _key,
    bool                  _value
)
{
    const struct d_test_registry_row* schema_row;

    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);

    if ((!schema_row) || (schema_row->value_type != D_TYPE_INFO_BOOL))
    {
        return false;
    }

    return d_internal_test_config_set_override(_config, _key, (void*)(uintptr_t)_value);
}

bool
d_test_config_set_size_t
(
    struct d_test_config* _config,
    uint32_t              _key,
    size_t                _value
)
{
    const struct d_test_registry_row* schema_row;

    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);

    if (!schema_row)
    {
        return false;
    }

    if ( (schema_row->value_type == D_TYPE_INFO_SIZE_T) ||
         (schema_row->value_type == D_TYPE_INFO_UINT16) ||
         (schema_row->value_type == D_TYPE_INFO_UINT32) )
    {
        return d_internal_test_config_set_override(_config, _key, (void*)(uintptr_t)_value);
    }

    return false;
}

bool
d_test_config_set_int32
(
    struct d_test_config* _config,
    uint32_t              _key,
    int32_t               _value
)
{
    const struct d_test_registry_row* schema_row;

    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);

    if ((!schema_row) || (schema_row->value_type != D_TYPE_INFO_INT32))
    {
        return false;
    }

    return d_internal_test_config_set_override(_config, _key, (void*)(intptr_t)_value);
}

bool
d_test_config_set_uint32
(
    struct d_test_config* _config,
    uint32_t              _key,
    uint32_t              _value
)
{
    const struct d_test_registry_row* schema_row;

    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    if (_key == D_TEST_CONFIG_MESSAGE_FLAGS)
    {
        _config->flags = _value;
        return true;
    }

    schema_row = d_internal_test_registry_config_row_from_flag(_key);

    if (!schema_row)
    {
        return false;
    }

    if ( (schema_row->value_type == D_TYPE_INFO_UINT32) ||
         (schema_row->value_type == D_TYPE_INFO_UINT16) ||
         (schema_row->value_type == D_TYPE_INFO_SIZE_T) )
    {
        return d_internal_test_config_set_override(_config, _key, (void*)(uintptr_t)_value);
    }

    return false;
}

bool
d_test_config_set_string
(
    struct d_test_config* _config,
    uint32_t              _key,
    const char*           _value
)
{
    const struct d_test_registry_row* schema_row;

    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);

    if ((!schema_row) || (schema_row->value_type != D_TYPE_INFO_STRING))
    {
        return false;
    }

    return d_internal_test_config_set_override(_config, _key, (void*)_value);
}

bool
d_test_config_set_ptr
(
    struct d_test_config* _config,
    uint32_t              _key,
    void*                 _value
)
{
    const struct d_test_registry_row* schema_row;

    if (!_config)
    {
        return false;
    }

    d_internal_test_registry_init_once();

    schema_row = d_internal_test_registry_config_row_from_flag(_key);

    if (!schema_row)
    {
        return false;
    }

    return d_internal_test_config_set_override(_config, _key, _value);
}


/******************************************************************************
 * KEY LOOKUP
 *****************************************************************************/

enum DTestConfigKey
d_test_config_key_from_string
(
    const char* _key
)
{
    struct d_test_registry_row* row;

    if (!_key)
    {
        return D_TEST_CONFIG_KEY_INVALID;
    }

    d_internal_test_registry_init_once();

    row = d_test_registry_find(_key);

    if ( (!row) || 
         ((row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) == 0) )
    {
        return D_TEST_CONFIG_KEY_INVALID;
    }

    return (enum DTestConfigKey)row->flag;
}