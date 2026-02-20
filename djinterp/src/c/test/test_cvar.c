#include "..\..\inc\test\test_cvar.h"


/******************************************************************************
 * ROW DEFINITION HELPER MACROS
 *****************************************************************************/

// D_INTERNAL_TEST_REGISTRY_ROW_PTR
//   macro: creates a registry row with a pointer value.
#define D_INTERNAL_TEST_REGISTRY_ROW_PTR(k, flg, f16, vt, p, h) \
    { (k), (flg), (f16), (vt), { .ptr = (p) }, (h) }

// D_INTERNAL_TEST_REGISTRY_ROW_BOOL
//   macro: creates a registry row with a boolean value.
#define D_INTERNAL_TEST_REGISTRY_ROW_BOOL(k, flg, f16, vt, v, h) \
    { (k), (flg), (f16), (vt), { .b = (v) }, (h) }

// D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T
//   macro: creates a registry row with a size_t value.
#define D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T(k, flg, f16, vt, v, h) \
    { (k), (flg), (f16), (vt), { .z = (v) }, (h) }

// D_INTERNAL_TEST_REGISTRY_ROW_UINT32
//   macro: creates a registry row with a uint32_t value.
#define D_INTERNAL_TEST_REGISTRY_ROW_UINT32(k, flg, f16, vt, v, h) \
    { (k), (flg), (f16), (vt), { .u32 = (v) }, (h) }

// D_INTERNAL_TEST_REGISTRY_ROW_UINT16
//   macro: creates a registry row with a uint16_t value.
#define D_INTERNAL_TEST_REGISTRY_ROW_UINT16(k, flg, f16, vt, v, h) \
    { (k), (flg), (f16), (vt), { .u16 = (v) }, (h) }

// D_INTERNAL_TEST_REGISTRY_ROW_INT32
//   macro: creates a registry row with an int32_t value.
#define D_INTERNAL_TEST_REGISTRY_ROW_INT32(k, flg, f16, vt, v, h) \
    { (k), (flg), (f16), (vt), { .i32 = (v) }, (h) }


/******************************************************************************
 * STATIC REGISTRY TABLE
 *****************************************************************************/

// D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY
//   constant: extra lookup entries reserved for aliases.
#define D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY 16

static struct d_test_registry_row g_test_registry_rows[] =
{
    // metadata entries
    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "authors",
        D_TEST_METADATA_AUTHORS,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING_ARRAY,
        NULL,
        "Author(s) of the test"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "category",
        D_TEST_METADATA_CATEGORY,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Test category"),

    // config entries
    D_INTERNAL_TEST_REGISTRY_ROW_BOOL(
        "config-enabled",
        D_TEST_CONFIG_ENABLED,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_BOOL,
        true,
        "Whether custom configuration is enabled"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "custom-data",
        D_TEST_METADATA_CUSTOM_DATA,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_VOID_PTR,
        NULL,
        "Arbitrary custom data pointer"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T(
        "date-created",
        D_TEST_METADATA_DATE_CREATED,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_SIZE_T,
        0,
        "Creation timestamp"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T(
        "date-modified",
        D_TEST_METADATA_DATE_MODIFIED,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_SIZE_T,
        0,
        "Last modification timestamp"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "dependencies",
        D_TEST_METADATA_DEPENDENCIES,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING_ARRAY,
        NULL,
        "Test dependencies"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "description",
        D_TEST_METADATA_DESCRIPTION,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Test description"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "framework-name",
        D_TEST_METADATA_FRAMEWORK_NAME,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Name of the testing framework"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "indent-string",
        D_TEST_CONFIG_INDENT_STR,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_STRING,
        D_TEST_DEFAULT_INDENT,
        "String used for indentation"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T(
        "max-failures",
        D_TEST_CONFIG_MAX_FAILURES,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_SIZE_T,
        D_TEST_DEFAULT_MAX_FAILURES,
        "Maximum failures before stopping (0 = unlimited)"),

    D_INTERNAL_TEST_REGISTRY_ROW_UINT16(
        "max-indent",
        D_TEST_CONFIG_INDENT_MAX_LEVEL,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_UINT16,
        D_TEST_DEFAULT_MAX_INDENT,
        "Maximum indentation level"),

    D_INTERNAL_TEST_REGISTRY_ROW_UINT32(
        "message-flags",
        D_TEST_CONFIG_MESSAGE_FLAGS,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_UINT32,
        0,
        "Packed message/settings flags"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "module-name",
        D_TEST_METADATA_MODULE_NAME,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Name of the test module"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "name",
        D_TEST_METADATA_NAME,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Test name"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "notes",
        D_TEST_METADATA_NOTES,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING_ARRAY,
        NULL,
        "Additional notes"),

    D_INTERNAL_TEST_REGISTRY_ROW_INT32(
        "priority",
        D_TEST_CONFIG_PRIORITY,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_INT32,
        0,
        "Test priority level"),

    D_INTERNAL_TEST_REGISTRY_ROW_BOOL(
        "skip",
        D_TEST_CONFIG_SKIP,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_BOOL,
        false,
        "Whether to skip this test"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "skip-reason",
        D_TEST_METADATA_SKIP_REASON,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Reason for skipping"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "submodule-name",
        D_TEST_METADATA_SUBMODULE_NAME,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Name of the test submodule"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "tags",
        D_TEST_METADATA_TAGS,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING_ARRAY,
        NULL,
        "Test tags"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T(
        "timeout",
        D_TEST_CONFIG_TIMEOUT_MS,
        D_TEST_REGISTRY_FLAG_IS_CONFIG,
        D_TYPE_INFO_SIZE_T,
        D_TEST_DEFAULT_TIMEOUT,
        "Test timeout in milliseconds"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "timestamp-format",
        D_TEST_METADATA_TIMESTAMP_FORMAT,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Format string for timestamps"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "version",
        D_TEST_METADATA_VERSION,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_VOID_PTR,
        NULL,
        "Version object"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR(
        "version-string",
        D_TEST_METADATA_VERSION_STRING,
        D_TEST_REGISTRY_FLAG_IS_METADATA,
        D_TYPE_INFO_STRING,
        NULL,
        "Version string")
};

#define D_INTERNAL_TEST_REGISTRY_ROW_COUNT \
    (sizeof(g_test_registry_rows) / sizeof(g_test_registry_rows[0]))

static struct d_registry_lookup_entry
    g_test_registry_lookup[D_INTERNAL_TEST_REGISTRY_ROW_COUNT +
                           D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY];

static struct d_registry g_test_registry =
{
    .rows            = (void*)g_test_registry_rows,
    .row_size        = sizeof(struct d_test_registry_row),
    .count           = D_INTERNAL_TEST_REGISTRY_ROW_COUNT,
    .capacity        = D_INTERNAL_TEST_REGISTRY_ROW_COUNT,
    .lookup          = g_test_registry_lookup,
    .lookup_count    = 0,
    .lookup_capacity = D_INTERNAL_TEST_REGISTRY_ROW_COUNT +
                       D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY,
    .flags           = D_REGISTRY_FLAG_STATIC_ROWS,
    .row_free        = NULL
};

static union d_test_value g_test_registry_defaults[D_INTERNAL_TEST_REGISTRY_ROW_COUNT];
static bool               g_test_registry_initialized = false;


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

static void
d_internal_test_registry_init_once
(
    void
)
{
    size_t i;

    if (g_test_registry_initialized)
    {
        return;
    }

    // snapshot default values before any runtime overrides
    for (i = 0; i < D_INTERNAL_TEST_REGISTRY_ROW_COUNT; i++)
    {
        g_test_registry_defaults[i] = g_test_registry_rows[i].value;
    }

    // build the sorted lookup table from row keys
    d_registry_rebuild_lookup(&g_test_registry);

    // add convenience aliases
    d_registry_add_alias(&g_test_registry, "indent-string",  "indent");
    d_registry_add_alias(&g_test_registry, "config-enabled", "enabled");
    d_registry_add_alias(&g_test_registry, "max-indent",     "indent-max");
    d_registry_add_alias(&g_test_registry, "max-indent",     "indent-level");
    d_registry_add_alias(&g_test_registry, "timeout",        "timeout-ms");
    d_registry_add_alias(&g_test_registry, "framework-name", "framework");
    d_registry_add_alias(&g_test_registry, "module-name",    "module");
    d_registry_add_alias(&g_test_registry, "submodule-name", "submodule");

    g_test_registry_initialized = true;

    return;
}

static struct d_test_registry_row*
d_internal_test_registry_find_by_flag_ex
(
    uint32_t _flag,
    size_t*  _out_index
)
{
    size_t i;

    if (_out_index)
    {
        *(_out_index) = 0;
    }

    for (i = 0; i < D_INTERNAL_TEST_REGISTRY_ROW_COUNT; i++)
    {
        if (g_test_registry_rows[i].flag == _flag)
        {
            if (_out_index)
            {
                *_out_index = i;
            }

            return &g_test_registry_rows[i];
        }
    }

    return NULL;
}


/******************************************************************************
 * PUBLIC API
 *****************************************************************************/

void
d_test_registry_init
(
    void
)
{
    d_internal_test_registry_init_once();

    return;
}

struct d_registry*
d_test_registry_registry
(
    void
)
{
    d_internal_test_registry_init_once();

    return &g_test_registry;
}

struct d_test_registry_row*
d_test_registry_find
(
    const char* _key
)
{
    d_internal_test_registry_init_once();

    if (!_key)
    {
        return NULL;
    }

    return (struct d_test_registry_row*)d_registry_get(&g_test_registry, _key);
}

struct d_test_registry_row*
d_test_registry_find_by_flag
(
    uint32_t _flag
)
{
    d_internal_test_registry_init_once();

    return d_internal_test_registry_find_by_flag_ex(_flag, NULL);
}

bool
d_test_registry_set
(
    uint32_t           _flag,
    union d_test_value _value
)
{
    struct d_test_registry_row* row;

    d_internal_test_registry_init_once();

    // disallow modifications if the registry is frozen
    if (D_REGISTRY_IS_FROZEN(&g_test_registry))
    {
        return false;
    }

    row = d_internal_test_registry_find_by_flag_ex(_flag, NULL);

    if (!row)
    {
        return false;
    }

    row->value = _value;

    return true;
}

union d_test_value
d_test_registry_get
(
    uint32_t _flag
)
{
    struct d_test_registry_row* row;

    d_internal_test_registry_init_once();

    row = d_internal_test_registry_find_by_flag_ex(_flag, NULL);

    if (!row)
    {
        return (union d_test_value){ .ptr = NULL };
    }

    return row->value;
}

void
d_test_registry_reset
(
    uint32_t _flag
)
{
    struct d_test_registry_row* row;
    size_t                      idx;

    d_internal_test_registry_init_once();

    if (D_REGISTRY_IS_FROZEN(&g_test_registry))
    {
        return;
    }

    idx = 0;
    row = d_internal_test_registry_find_by_flag_ex(_flag, &idx);

    if (!row)
    {
        return;
    }

    row->value = g_test_registry_defaults[idx];

    return;
}

void
d_test_registry_reset_all
(
    void
)
{
    size_t i;

    d_internal_test_registry_init_once();

    if (D_REGISTRY_IS_FROZEN(&g_test_registry))
    {
        return;
    }

    for (i = 0; i < D_INTERNAL_TEST_REGISTRY_ROW_COUNT; i++)
    {
        g_test_registry_rows[i].value = g_test_registry_defaults[i];
    }

    return;
}


/*
d_test_registry_is_valid_arg
  Checks if an argument key is valid for a given context.

Parameter(s):
  _key:                   the argument key to validate.
  _required_command_flag: the command flag bit that must be set
                          (e.g., D_TEST_REGISTRY_FLAG_SESSION).
Return:
  true if the key exists in the registry AND has the required command flag set.
*/
bool
d_test_registry_is_valid_arg
(
    const char* _key,
    uint16_t    _required_command_flag
)
{
    struct d_test_registry_row* row;

    d_internal_test_registry_init_once();

    if (!_key)
    {
        return false;
    }

    row = d_test_registry_find(_key);

    if (!row)
    {
        return false;
    }

    // check if the required command flag bit is set
    return (row->command_flags & _required_command_flag) != 0;
}


/******************************************************************************
 * PREDICATE FUNCTIONS FOR FILTERED ITERATION
 *****************************************************************************/

// d_test_registry_is_config_row
//   predicate: returns true if row is a config entry.
static inline bool
d_test_registry_is_config_row
(
    const void* _row,
    const void* _context
)
{
    const struct d_test_registry_row* row;

    (void)_context;
    row = (const struct d_test_registry_row*)_row;

    return (row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0;
}

// d_test_registry_is_metadata_row
//   predicate: returns true if row is a metadata entry.
static inline bool
d_test_registry_is_metadata_row
(
    const void* _row,
    const void* _context
)
{
    const struct d_test_registry_row* row;

    (void)_context;
    row = (const struct d_test_registry_row*)_row;

    return (row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) != 0;
}

// d_test_registry_is_required_row
//   predicate: returns true if row is required.
static inline bool
d_test_registry_is_required_row
(
    const void* _row,
    const void* _context
)
{
    const struct d_test_registry_row* row;

    (void)_context;
    row = (const struct d_test_registry_row*)_row;

    return (row->command_flags & D_TEST_REGISTRY_FLAG_IS_REQUIRED) != 0;
}