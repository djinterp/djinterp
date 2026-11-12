/*******************************************************************************
* djinterp [c/parse]                                                  settings.c
*
* Parse's settings group (settings.h): the schema, the djin over storage of
* its own, its registration, and the reader expression core v2 consults.
*   The djin's frame is made over its storage the first time it is
* registered, before the program's threads start, and keeps its variables
* when it is taken off and registered again. The reader reads each row
* through a cache, which reads the frame again only when a binding anywhere
* in its chain has moved the generation.
*
*
* path:      /src/djinterp/c/modules/parse/settings.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/parse/settings.h"  // corresponding header


#if (D_INTERNAL_PARSE_SETTINGS == 1)

// std
#include <stddef.h>  // NULL
#include <string.h>  // memset
// djinterp
#include "../../../../../inc/djinterp/c/modules/parse/expr/parse.h"  // the core's settings


//==============================================================================
// 1.  THE SCHEMA
//==============================================================================


// D_INTERNAL_PARSE_SETTINGS_BINDINGS
//   constant: the djin's room for bindings: one per source, per row bound,
// for the six sources and the three rows, with room to spare.
#define D_INTERNAL_PARSE_SETTINGS_BINDINGS  24u

// D_INTERNAL_PARSE_SETTINGS_TEXT
//   constant: the djin's text arena; its rows hold no text of their own.
#define D_INTERNAL_PARSE_SETTINGS_TEXT      64u

// D_PARSE_EXPR_DEPTHS
//   constant: the depth guard's named value.
static const struct dj_option_env_name D_PARSE_EXPR_DEPTHS[] =
{
    { "unbounded", (int64_t)D_PARSE_EXPR_DEPTH_UNBOUNDED }
};

// D_PARSE_EXPR_ORDERS
//   constant: the error order's names, the values cfg_parse.h gives the knob.
static const struct dj_option_env_name D_PARSE_EXPR_ORDERS[] =
{
    { "leftmost",        (int64_t)D_PARSE_EXPR_ORDER_LEFTMOST        },
    { "structure_first", (int64_t)D_PARSE_EXPR_ORDER_STRUCTURE_FIRST }
};

// SCHEMA
//   constant: the rows, in enum dj_parse_setting's order.
static const struct dj_option_env_var SCHEMA[D_PARSE_SETTINGS] =
{
    D_OPTION_ENV_INT_RANGE_KNOB_NAMED("expr_max_depth",
                                      D_CFG_PARSE_EXPR_MAX_DEPTH, 1, 65535,
                                      D_PARSE_EXPR_DEPTHS,
                                      "the depth guard: 1 to 65535, or "
                                      "unbounded"),
    D_OPTION_ENV_INT_RANGE_KNOB("expr_max_errors",
                                D_CFG_PARSE_EXPR_MAX_ERRORS, 1, 65535,
                                "errors before a parse stops: 1 to 65535"),
    D_OPTION_ENV_CHOICE_KNOB("expr_error_order", D_PARSE_EXPR_ORDERS,
                             D_CFG_PARSE_EXPR_ERROR_ORDER,
                             "which error is the error: leftmost or "
                             "structure_first")
};


//==============================================================================
// 2.  THE DJIN
//==============================================================================


// dj_internal_parse_djin_storage
//   struct: the djin's storage.
D_OPTION_ENV_DECLARE(dj_internal_parse_djin_storage,
                     D_INTERNAL_PARSE_SETTINGS_BINDINGS,
                     D_PARSE_SETTINGS,
                     D_INTERNAL_PARSE_SETTINGS_TEXT);

// dj_internal_parse_djin
//   struct: the djin: its frame, the storage the frame points into, whether
// the frame is made, its first row's id as the frame stands, the registry
// it is in, and a cache per row.
struct dj_internal_parse_djin
{
    struct dj_option_env                  frame;
    struct dj_internal_parse_djin_storage storage;
    bool                                  made;
    uint32_t                              base;
    struct dj_option_env_registry*        registry;
    struct dj_option_env_cache            cache[D_PARSE_SETTINGS];
};

// d_internal_parse_djin
//   variable: the djin `parse`.
static struct dj_internal_parse_djin d_internal_parse_djin;

/*
d_internal_parse_base
  The first row's id, found by name: a frame's ids change with its depth, so
it is found again whenever the frame gains or loses a parent.
*/
static uint32_t
d_internal_parse_base(void)
{
    const struct dj_option_result r =
        d_option_env_find(&d_internal_parse_djin.frame, SCHEMA[0].name,
                          strlen(SCHEMA[0].name));

    return (r.is_ok) ? r.payload.value : D_OPTION_ENV_NONE;
}

/*
d_internal_parse_make
  Makes the djin's frame over its storage, named `parse`, with the schema
declared, once.
*/
static struct dj_option_result
d_internal_parse_make(void)
{
    struct dj_internal_parse_djin* const d = &d_internal_parse_djin;
    struct dj_option_result              r;

    if (d->made)
    {
        return d_option_ok(0u);
    }

    r = D_OPTION_ENV_VIEW(&d->frame, d->storage);

    if (r.is_ok)
    {
        r = d_option_env_set_name(&d->frame, "parse");
    }

    if (r.is_ok)
    {
        r = d_option_env_declare_all(&d->frame, SCHEMA,
                                     (uint32_t)D_PARSE_SETTINGS);
    }

    d->made = r.is_ok;
    d->base = (d->made) ? d_internal_parse_base() : D_OPTION_ENV_NONE;

    return r;
}

/*
d_internal_parse_read
  The reader the core consults: each row through its cache.
*/
static void
d_internal_parse_read(
    struct dj_parse_expr_settings* _settings
)
{
    struct dj_internal_parse_djin* const d    = &d_internal_parse_djin;
    const uint32_t                       base = d->base;

    if (base == D_OPTION_ENV_NONE)
    {
        return;
    }

    _settings->max_depth   = (uint32_t)d_option_env_cached(
                                 &d->frame, D_OPTION_ENV_GLOBAL,
                                 base + D_PARSE_SETTING_EXPR_MAX_DEPTH,
                                 &d->cache[0])->integer;
    _settings->max_errors  = (uint32_t)d_option_env_cached(
                                 &d->frame, D_OPTION_ENV_GLOBAL,
                                 base + D_PARSE_SETTING_EXPR_MAX_ERRORS,
                                 &d->cache[1])->integer;
    _settings->error_order = (uint8_t)d_option_env_cached(
                                 &d->frame, D_OPTION_ENV_GLOBAL,
                                 base + D_PARSE_SETTING_EXPR_ERROR_ORDER,
                                 &d->cache[2])->integer;

    return;
}


//==============================================================================
// 3.  REGISTERING
//==============================================================================


/*
d_parse_register
  Makes the djin once, adds it to the registry, and installs the reader.
*/
struct dj_option_result
d_parse_register(
    struct dj_option_env_registry* _registry
)
{
    struct dj_internal_parse_djin* const d        = &d_internal_parse_djin;
    struct dj_option_env_registry* const registry =
        (_registry) ? _registry : d_option_env_registry();
    struct dj_option_result              r        = d_internal_parse_make();

    if (!r.is_ok)
    {
        return r;
    }

    r = d_option_env_registry_add(registry, &d->frame);

    if (r.is_ok)
    {
        d->registry = registry;
        d->base     = d_internal_parse_base();
        memset(d->cache, 0, sizeof(d->cache));
        d_parse_expr_settings_from(d_internal_parse_read);
    }

    return r;
}

/*
d_parse_unregister
  The registry's remove, and the core back on its knobs.
*/
struct dj_option_result
d_parse_unregister(void)
{
    struct dj_internal_parse_djin* const d = &d_internal_parse_djin;
    struct dj_option_result              r;

    if (!d->registry)
    {
        return d_option_fail(D_OPTION_STATUS_KEY_ABSENT);
    }

    d_parse_expr_settings_from(NULL);
    r = d_option_env_registry_remove(d->registry, &d->frame);
    d->registry = NULL;
    d->base     = d_internal_parse_base();
    memset(d->cache, 0, sizeof(d->cache));

    return r;
}

/*
d_parse_djin
  The frame, made if it is not yet.
*/
struct dj_option_env*
d_parse_djin(void)
{
    (void)d_internal_parse_make();

    return &d_internal_parse_djin.frame;
}

/*
d_parse_setting_id
  The first row's id plus the row's place: a schema's ids are consecutive.
*/
uint32_t
d_parse_setting_id(
    int _setting
)
{
    (void)d_internal_parse_make();

    if ( (_setting < 0) || (_setting >= (int)D_PARSE_SETTINGS) ||
         (d_internal_parse_djin.base == D_OPTION_ENV_NONE) )
    {
        return D_OPTION_ENV_NONE;
    }

    return d_internal_parse_djin.base + (uint32_t)_setting;
}


#endif  // D_INTERNAL_PARSE_SETTINGS
