/*******************************************************************************
* djinterp [c/test]                                                  test_cvar.c
*
* The test registry of test_cvar.h: the static table of DTest's config and
* metadata rows, the dj_registry over it, and its lookups and resets.
*   The table is sorted by key and never changes shape. The first call into
* this file builds the registry's lookup and its aliases, and records every
* row's value as its default; d_test_registry_reset and _reset_all restore from
* that record.
*
*
* path:      /src/djinterp/c/modules/test/test_cvar.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_cvar.h"  // corresponding header
// djinterp
#include "../../../../../inc/djinterp/c/container/registry/registry.h"  // d_registry_*
#include "../../../../../inc/djinterp/c/meta/type_info.h"               // D_TYPE_INFO_*


//==============================================================================
// 1.  ROW DEFINITION HELPER MACROS
//==============================================================================

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

//==============================================================================
// 2.  STATIC REGISTRY TABLE
//==============================================================================

// D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY
//   constant: extra lookup entries reserved for aliases.
#define D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY 16

// g_test_registry_rows
//   variable: the registry's rows, sorted by key; each value is its row's
// default until something sets it.
static struct dj_test_registry_row g_test_registry_rows[] =
{
    // metadata entries
    D_INTERNAL_TEST_REGISTRY_ROW_PTR("authors",
                                     D_TEST_METADATA_AUTHORS,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING_ARRAY,
                                     NULL,
                                     "Author(s) of the test"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("category",
                                     D_TEST_METADATA_CATEGORY,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Test category"),

    // config entries
    D_INTERNAL_TEST_REGISTRY_ROW_BOOL("config-enabled",
                                      D_TEST_OPTIONS_ENABLED,
                                      D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                      D_TYPE_INFO_BOOL,
                                      true,
                                      "Whether custom configuration is enabled"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("custom-data",
                                     D_TEST_METADATA_CUSTOM_DATA,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_VOID_PTR,
                                     NULL,
                                     "Arbitrary custom data pointer"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T("date-created",
                                        D_TEST_METADATA_DATE_CREATED,
                                        D_TEST_REGISTRY_FLAG_IS_METADATA,
                                        D_TYPE_INFO_SIZE_T,
                                        0,
                                        "Creation timestamp"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T("date-modified",
                                        D_TEST_METADATA_DATE_MODIFIED,
                                        D_TEST_REGISTRY_FLAG_IS_METADATA,
                                        D_TYPE_INFO_SIZE_T,
                                        0,
                                        "Last modification timestamp"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("dependencies",
                                     D_TEST_METADATA_DEPENDENCIES,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING_ARRAY,
                                     NULL,
                                     "Test dependencies"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("description",
                                     D_TEST_METADATA_DESCRIPTION,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Test description"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("framework-name",
                                     D_TEST_METADATA_FRAMEWORK_NAME,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Name of the testing framework"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("indent-string",
                                     D_TEST_OPTIONS_INDENT_STR,
                                     D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                     D_TYPE_INFO_STRING,
                                     D_TEST_DEFAULT_INDENT,
                                     "String used for indentation"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T("max-failures",
                                        D_TEST_OPTIONS_MAX_FAILURES,
                                        D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                        D_TYPE_INFO_SIZE_T,
                                        D_TEST_DEFAULT_MAX_FAILURES,
                                        "Maximum failures before stopping (0 = unlimited)"),

    D_INTERNAL_TEST_REGISTRY_ROW_UINT16("max-indent",
                                        D_TEST_OPTIONS_INDENT_MAX_LEVEL,
                                        D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                        D_TYPE_INFO_UINT16,
                                        D_TEST_DEFAULT_MAX_INDENT,
                                        "Maximum indentation level"),

    D_INTERNAL_TEST_REGISTRY_ROW_UINT32("message-flags",
                                        D_TEST_OPTIONS_MESSAGE_FLAGS,
                                        D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                        D_TYPE_INFO_UINT32,
                                        0,
                                        "Packed message/settings flags"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("module-name",
                                     D_TEST_METADATA_MODULE_NAME,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Name of the test module"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("name",
                                     D_TEST_METADATA_NAME,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Test name"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("notes",
                                     D_TEST_METADATA_NOTES,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING_ARRAY,
                                     NULL,
                                     "Additional notes"),

    D_INTERNAL_TEST_REGISTRY_ROW_INT32("priority",
                                       D_TEST_OPTIONS_PRIORITY,
                                       D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                       D_TYPE_INFO_INT32,
                                       0,
                                       "Test priority level"),

    D_INTERNAL_TEST_REGISTRY_ROW_BOOL("skip",
                                      D_TEST_OPTIONS_SKIP,
                                      D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                      D_TYPE_INFO_BOOL,
                                      false,
                                      "Whether to skip this test"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("skip-reason",
                                     D_TEST_METADATA_SKIP_REASON,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Reason for skipping"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("submodule-name",
                                     D_TEST_METADATA_SUBMODULE_NAME,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Name of the test submodule"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("tags",
                                     D_TEST_METADATA_TAGS,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING_ARRAY,
                                     NULL,
                                     "Test tags"),

    D_INTERNAL_TEST_REGISTRY_ROW_SIZE_T("timeout",
                                        D_TEST_OPTIONS_TIMEOUT_MS,
                                        D_TEST_REGISTRY_FLAG_IS_CONFIG,
                                        D_TYPE_INFO_SIZE_T,
                                        D_TEST_DEFAULT_TIMEOUT,
                                        "Test timeout in milliseconds"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("timestamp-format",
                                     D_TEST_METADATA_TIMESTAMP_FORMAT,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Format string for timestamps"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("version",
                                     D_TEST_METADATA_VERSION,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_VOID_PTR,
                                     NULL,
                                     "Version object"),

    D_INTERNAL_TEST_REGISTRY_ROW_PTR("version-string",
                                     D_TEST_METADATA_VERSION_STRING,
                                     D_TEST_REGISTRY_FLAG_IS_METADATA,
                                     D_TYPE_INFO_STRING,
                                     NULL,
                                     "Version string")
};

// D_INTERNAL_TEST_REGISTRY_ROW_COUNT
//   constant: the number of rows in g_test_registry_rows.
#define D_INTERNAL_TEST_REGISTRY_ROW_COUNT \
    (sizeof(g_test_registry_rows) / sizeof(g_test_registry_rows[0]))

// g_test_registry_lookup
//   variable: the registry's sorted lookup: one entry a row, and room for
// the aliases.
static struct dj_registry_lookup_entry
    g_test_registry_lookup[D_INTERNAL_TEST_REGISTRY_ROW_COUNT +
                           D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY];

// g_test_registry
//   variable: the registry over the static rows and their lookup; the rows
// are not its to free.
static struct dj_registry g_test_registry =
{
    .rows            = (void*)g_test_registry_rows,
    .row_size        = sizeof(struct dj_test_registry_row),
    .count           = D_INTERNAL_TEST_REGISTRY_ROW_COUNT,
    .capacity        = D_INTERNAL_TEST_REGISTRY_ROW_COUNT,
    .lookup          = g_test_registry_lookup,
    .lookup_count    = 0,
    .lookup_capacity = D_INTERNAL_TEST_REGISTRY_ROW_COUNT +
                       D_INTERNAL_TEST_REGISTRY_ALIAS_CAPACITY,
    .flags           = D_REGISTRY_FLAG_STATIC_ROWS,
    .row_free        = NULL
};

// g_test_registry_defaults
//   variable: every row's value as the table gives it, recorded before
// anything can set one, for the resets.
static union dj_test_value
    g_test_registry_defaults[D_INTERNAL_TEST_REGISTRY_ROW_COUNT];

// g_test_registry_initialized
//   variable: whether d_internal_test_registry_init_once has run.
static bool g_test_registry_initialized = false;

//==============================================================================
// 3.  INTERNAL HELPERS
//==============================================================================

/**
 * @brief Initializes the registry, on the first call only.
 *
 * The defaults are recorded before the lookup is built and the aliases added,
 * so they are the table's own values. Not thread-safe: the first call must not
 * race another.
 */
static void
d_internal_test_registry_init_once(void)
{
    // already initialized
    if (g_test_registry_initialized)
    {
        return;
    }

    // snapshot default values before any runtime overrides
    for (size_t i = 0; i < D_INTERNAL_TEST_REGISTRY_ROW_COUNT; i++)
    {
        g_test_registry_defaults[i] = g_test_registry_rows[i].value;
    }

    // build the sorted lookup table from row keys
    d_registry_rebuild_lookup(&g_test_registry);

    // add convenience aliases
    d_registry_add_alias(&g_test_registry,
                         "indent-string",
                         "indent");
    d_registry_add_alias(&g_test_registry,
                         "config-enabled",
                         "enabled");
    d_registry_add_alias(&g_test_registry,
                         "max-indent",
                         "indent-max");
    d_registry_add_alias(&g_test_registry,
                         "max-indent",
                         "indent-level");
    d_registry_add_alias(&g_test_registry,
                         "timeout",
                         "timeout-ms");
    d_registry_add_alias(&g_test_registry,
                         "framework-name",
                         "framework");
    d_registry_add_alias(&g_test_registry,
                         "module-name",
                         "module");
    d_registry_add_alias(&g_test_registry,
                         "submodule-name",
                         "submodule");

    g_test_registry_initialized = true;

    return;
}

/**
 * @brief Finds a row by its flag, and its index.
 *
 * @param[in]  _flag       a DTestConfigKey or a DTestMetadataFlag.
 * @param[out] _out_index  the row's index, 0 when there is none; may be
 *                         `NULL`.
 * @return the row, or `NULL` if no row has `_flag`.
 *
 * A linear scan of the table: the lookup is sorted by key, not by flag. The
 * index is the defaults' index too.
 */
static struct dj_test_registry_row*
d_internal_test_registry_find_by_flag_ex(
    uint32_t _flag,
    size_t*  _out_index
)
{
    // no row found yet
    if (_out_index)
    {
        *(_out_index) = 0;
    }

    // the first row with the flag
    for (size_t i = 0; i < D_INTERNAL_TEST_REGISTRY_ROW_COUNT; i++)
    {
        // this row has it
        if (g_test_registry_rows[i].flag == _flag)
        {
            // its index, when asked for
            if (_out_index)
            {
                *_out_index = i;
            }

            return &g_test_registry_rows[i];
        }
    }

    return NULL;
}

//==============================================================================
// 4.  PUBLIC API
//==============================================================================

/*
d_test_registry_init
  The work is d_internal_test_registry_init_once's, which every function here
calls first anyway.
*/
void
d_test_registry_init(void)
{
    d_internal_test_registry_init_once();

    return;
}

/*
d_test_registry_registry
  The static registry, initialized first.
*/
struct dj_registry*
d_test_registry_registry(void)
{
    d_internal_test_registry_init_once();

    return &g_test_registry;
}

/*
d_test_registry_find
  The registry's lookup resolves the key, aliases included.
*/
struct dj_test_registry_row*
d_test_registry_find(
    const char* _key
)
{
    d_internal_test_registry_init_once();

    // parameter validation
    if (!_key)
    {
        return NULL;
    }

    return (struct dj_test_registry_row*)d_registry_get(&g_test_registry,
                                                        _key);
}

/*
d_test_registry_find_by_flag
  A scan of the table by flag; the index is not wanted.
*/
struct dj_test_registry_row*
d_test_registry_find_by_flag(
    uint32_t _flag
)
{
    d_internal_test_registry_init_once();

    return d_internal_test_registry_find_by_flag_ex(_flag,
                                                    NULL);
}

/*
d_test_registry_set
  The value replaces the row's, whatever the row's type; nothing checks the two
agree.
*/
bool
d_test_registry_set(
    uint32_t            _flag,
    union dj_test_value _value
)
{
    d_internal_test_registry_init_once();

    // disallow modifications if the registry is frozen
    if (D_REGISTRY_IS_FROZEN(&g_test_registry))
    {
        return false;
    }

    struct dj_test_registry_row* const row =
        d_internal_test_registry_find_by_flag_ex(_flag,
                                                 NULL);

    // no row has the flag
    if (!row)
    {
        return false;
    }

    row->value = _value;

    return true;
}

/*
d_test_registry_get
  A flag no row has reads as a NULL pointer value.
*/
union dj_test_value
d_test_registry_get(
    uint32_t _flag
)
{
    d_internal_test_registry_init_once();

    const struct dj_test_registry_row* const row =
        d_internal_test_registry_find_by_flag_ex(_flag,
                                                 NULL);

    // no row has the flag
    if (!row)
    {
        return (union dj_test_value){ .ptr = NULL };
    }

    return row->value;
}

/*
d_test_registry_reset
  The row's index finds its default, recorded when the registry was
initialized.
*/
void
d_test_registry_reset(
    uint32_t _flag
)
{
    d_internal_test_registry_init_once();

    // a frozen registry keeps its values
    if (D_REGISTRY_IS_FROZEN(&g_test_registry))
    {
        return;
    }

    size_t                             idx = 0;
    struct dj_test_registry_row* const row =
        d_internal_test_registry_find_by_flag_ex(_flag,
                                                 &idx);

    // no row has the flag
    if (!row)
    {
        return;
    }

    row->value = g_test_registry_defaults[idx];

    return;
}

/*
d_test_registry_reset_all
  Every row takes back its recorded default.
*/
void
d_test_registry_reset_all(void)
{
    d_internal_test_registry_init_once();

    // a frozen registry keeps its values
    if (D_REGISTRY_IS_FROZEN(&g_test_registry))
    {
        return;
    }

    // each row, from its default
    for (size_t i = 0; i < D_INTERNAL_TEST_REGISTRY_ROW_COUNT; i++)
    {
        g_test_registry_rows[i].value = g_test_registry_defaults[i];
    }

    return;
}

/*
d_test_registry_is_valid_arg
  The key is looked up as d_test_registry_find looks it up, aliases included;
any one of the required bits on the row will do.
*/
bool
d_test_registry_is_valid_arg(
    const char* _key,
    uint16_t    _required_command_flag
)
{
    d_internal_test_registry_init_once();

    // parameter validation
    if (!_key)
    {
        return false;
    }

    const struct dj_test_registry_row* const row = d_test_registry_find(_key);

    // an unknown key
    if (!row)
    {
        return false;
    }

    // check if the required command flag bit is set
    return (row->command_flags & _required_command_flag) != 0;
}

//==============================================================================
// 5.  PREDICATE FUNCTIONS FOR FILTERED ITERATION
//==============================================================================

/*
d_test_registry_is_config_row
  The row's IS_CONFIG bit; the context is not used.
*/
bool
d_test_registry_is_config_row(
    const void* _row,
    const void* _context
)
{
    (void)_context;

    const struct dj_test_registry_row* const row =
        (const struct dj_test_registry_row*)_row;

    return (row->command_flags & D_TEST_REGISTRY_FLAG_IS_CONFIG) != 0;
}

/*
d_test_registry_is_metadata_row
  The row's IS_METADATA bit; the context is not used.
*/
bool
d_test_registry_is_metadata_row(
    const void* _row,
    const void* _context
)
{
    (void)_context;

    const struct dj_test_registry_row* const row =
        (const struct dj_test_registry_row*)_row;

    return (row->command_flags & D_TEST_REGISTRY_FLAG_IS_METADATA) != 0;
}

/*
d_test_registry_is_required_row
  The row's IS_REQUIRED bit; the context is not used.
*/
bool
d_test_registry_is_required_row(
    const void* _row,
    const void* _context
)
{
    (void)_context;

    const struct dj_test_registry_row* const row =
        (const struct dj_test_registry_row*)_row;

    return (row->command_flags & D_TEST_REGISTRY_FLAG_IS_REQUIRED) != 0;
}
