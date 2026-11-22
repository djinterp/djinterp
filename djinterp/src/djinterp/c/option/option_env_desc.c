/*******************************************************************************
* djinterp [c][option]                                         option_env_desc.c
*
*   A frame's settings as desc facts: each visible variable a level, with its
* value in force, source, default, knob, help and refused sources, and on
* request its layers, what it accepts, and each scope's settings.
*
*
* path:      /src/djinterp/c/option/option_env_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

#include "../../../../inc/djinterp/c/option/option_env_desc.h"  // corresponding
                                                                 // header

#if RE_STD_HAS_INT64

//==============================================================================
// NAMES
//==============================================================================

// KIND_NAMES, SOURCE_NAMES, INDEX_KEYS
//   constants: the words a kind and a source are written by, and the keys of
// a setting's layers, by number; every one static, so a collecting machine
// may keep them.
static const char* const KIND_NAMES[] =
{
    "none", "bool", "int", "real", "text", "choice", "flags"
};

static const char* const SOURCE_NAMES[] =
{
    "default", "program", "file", "environment", "command_line", "override"
};

static const char* const INDEX_KEYS[D_OPTION_ENV_DESC_LAYERS_MAX] =
{
    "0", "1", "2",  "3",  "4",  "5",  "6",  "7",
    "8", "9", "10", "11", "12", "13", "14", "15"
};

/*
d_internal_option_env_desc_word
  A kind's or a source's word; "?" past the table.
*/
static const char*
d_internal_option_env_desc_word(
    const char* const* _table,
    size_t             _count,
    int32_t            _at
)
{
    return ( (_at >= 0) && ((size_t)_at < _count) ) ? _table[_at] : "?";
}


//==============================================================================
// VALUES
//==============================================================================

/*
d_internal_option_env_desc_name_of
  The name a row gives an integer, among its names; null for none.
*/
static const char*
d_internal_option_env_desc_name_of(
    const struct dj_option_env_var* _decl,
    int64_t                         _integer
)
{
    uint32_t i;

    for (i = 0u; (_decl->names != NULL) && (i < _decl->name_count); ++i)
    {
        if (_decl->names[i].value == _integer)
        {
            return _decl->names[i].name;
        }
    }

    return NULL;
}

/*
d_internal_option_env_desc_value
  One value as a fact of its kind: a bool, an integer or its name, a real,
text; flags as a level, a true fact for each name whose bits are all set.
*/
static int
d_internal_option_env_desc_value(
    struct dj_desc_sink*              _sink,
    const char*                       _key,
    const struct dj_option_env_var*   _decl,
    const struct dj_option_env_value* _value,
    uint32_t                          _layer,
    uint32_t                          _importance,
    uint32_t                          _truth
)
{
    struct dj_ostream_value value = d_ostream_value_none();
    const char*             name  = NULL;
    uint32_t                i;
    int                     stopped = 0;

    switch (_decl->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            value = d_ostream_value_bool((_value->integer != 0) ? 1 : 0);
            break;
        case D_OPTION_ENV_KIND_INT:
        case D_OPTION_ENV_KIND_CHOICE:
            name  = d_internal_option_env_desc_name_of(_decl,
                                                       _value->integer);
            value = (name != NULL) ? d_ostream_value_text(name)
                                   : d_ostream_value_int64(_value->integer);
            break;
        case D_OPTION_ENV_KIND_REAL:
            value = d_ostream_value_real(_value->real);
            break;
        case D_OPTION_ENV_KIND_TEXT:
            value = d_ostream_value_text(_value->text);
            break;
        case D_OPTION_ENV_KIND_FLAGS:
            // a level only where its facts are wanted, so none is empty
            if ( (d_desc_wants(_sink, _layer, _importance) == 0) ||
                 (d_desc_enter(_sink, _key, NULL) == 0) )
            {
                return (_sink->stopped != 0u) ? 1 : 0;
            }

            for (i = 0u; (stopped == 0) && (i < _decl->name_count); ++i)
            {
                if ( (_decl->names[i].value != 0) &&
                     ((_value->integer & _decl->names[i].value) ==
                      _decl->names[i].value) )
                {
                    stopped = d_desc_emit(_sink, _decl->names[i].name, NULL,
                                          d_ostream_value_bool(1), _layer,
                                          _importance, _truth);
                }
            }

            d_desc_leave(_sink);

            return (stopped != 0) ? 1 : ((_sink->stopped != 0u) ? 1 : 0);
        default:
            break;
    }

    return d_desc_emit(_sink, _key, NULL, value, _layer, _importance, _truth);
}

/*
d_internal_option_env_desc_default
  A row's default as a value: its integer, real or text.
*/
static void
d_internal_option_env_desc_default(
    const struct dj_option_env_var* _decl,
    struct dj_option_env_value*     _value
)
{
    _value->text    = _decl->text;
    _value->integer = _decl->integer;
    _value->real    = _decl->real;
    _value->kind    = _decl->kind;
    _value->bound   = 0;
    _value->source  = D_OPTION_ENV_SOURCE_DEFAULT;
    _value->depth   = D_OPTION_ENV_NONE;

    return;
}


//==============================================================================
// ONE SETTING
//==============================================================================

/*
d_internal_option_env_desc_refuses
  The sources a row refuses, as a level of true facts; nothing for none.
*/
static int
d_internal_option_env_desc_refuses(
    struct dj_desc_sink*            _sink,
    const struct dj_option_env_var* _decl
)
{
    int32_t source;
    int     stopped = 0;

    if ( (_decl->refused == 0u) ||
         (d_desc_wants(_sink, D_DESC_LAYER_POLICY, 2u) == 0) ||
         (d_desc_enter(_sink, "refuses", "refuses") == 0) )
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    for (source = 0; (stopped == 0) &&
                     (source < (int32_t)D_OPTION_ENV_SOURCES); ++source)
    {
        if ((_decl->refused & D_OPTION_ENV_REFUSES(source)) != 0u)
        {
            stopped = d_desc_emit(_sink, SOURCE_NAMES[source], NULL,
                                  d_ostream_value_bool(1),
                                  D_DESC_LAYER_POLICY, 2u,
                                  D_DESC_TRUTH_BUILD);
        }
    }

    d_desc_leave(_sink);

    return stopped;
}

/*
d_internal_option_env_desc_names
  A level of a row's names, each with its integer: its names, or those it
leaves unbuilt.
*/
static int
d_internal_option_env_desc_names(
    struct dj_desc_sink*             _sink,
    const char*                      _key,
    const struct dj_option_env_name* _names,
    uint32_t                         _count
)
{
    uint32_t i;
    int      stopped = 0;

    if ( (_count == 0u) ||
         (_names == NULL) ||
         (d_desc_wants(_sink, D_DESC_LAYER_POLICY, 3u) == 0) ||
         (d_desc_enter(_sink, _key, _key) == 0) )
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    for (i = 0u; (stopped == 0) && (i < _count); ++i)
    {
        stopped = d_desc_emit(_sink, _names[i].name, NULL,
                              d_ostream_value_int64(_names[i].value),
                              D_DESC_LAYER_POLICY, 3u, D_DESC_TRUTH_BUILD);
    }

    d_desc_leave(_sink);

    return stopped;
}

/*
d_internal_option_env_desc_accepts
  What a row accepts: an int's range, and the names of an int, a choice or
flags, with those ruled but not built.
*/
static int
d_internal_option_env_desc_accepts(
    struct dj_desc_sink*            _sink,
    const struct dj_option_env_var* _decl
)
{
    int stopped = 0;

    if (_decl->kind == D_OPTION_ENV_KIND_INT)
    {
        stopped = d_desc_emit(_sink, "minimum", "least",
                              d_ostream_value_int64(_decl->minimum),
                              D_DESC_LAYER_POLICY, 3u, D_DESC_TRUTH_BUILD);
        stopped = (stopped != 0)
                      ? 1
                      : d_desc_emit(_sink, "maximum", "most",
                                    d_ostream_value_int64(_decl->maximum),
                                    D_DESC_LAYER_POLICY, 3u,
                                    D_DESC_TRUTH_BUILD);
    }

    stopped = (stopped != 0)
                  ? 1
                  : d_internal_option_env_desc_names(_sink, "names",
                                                     _decl->names,
                                                     _decl->name_count);

    return (stopped != 0)
               ? 1
               : d_internal_option_env_desc_names(_sink, "unbuilt",
                                                  _decl->unbuilt,
                                                  _decl->unbuilt_count);
}

/*
d_internal_option_env_desc_layers
  Every binding that bears on a setting, in force first, the default last:
each a level keyed by its place, with its source, its value, and whether it
is the one in force.
*/
static int
d_internal_option_env_desc_layers(
    struct dj_desc_sink*            _sink,
    const struct dj_option_env*     _env,
    uint32_t                        _scope,
    uint32_t                        _var,
    const struct dj_option_env_var* _decl
)
{
    struct dj_option_env_layer layers[D_OPTION_ENV_DESC_LAYERS_MAX];
    size_t                     count;
    size_t                     i;
    int                        stopped = 0;

    count = d_option_env_layers(_env, _scope, _var, layers,
                                (size_t)D_OPTION_ENV_DESC_LAYERS_MAX);

    if ( (count == (size_t)-1) ||
         (d_desc_wants(_sink, D_DESC_LAYER_PROVENANCE, 3u) == 0) ||
         (d_desc_enter(_sink, "layers", "layers") == 0) )
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    count = (count < (size_t)D_OPTION_ENV_DESC_LAYERS_MAX)
                ? count : (size_t)D_OPTION_ENV_DESC_LAYERS_MAX;

    for (i = 0u; (stopped == 0) && (i < count); ++i)
    {
        if (d_desc_enter(_sink, INDEX_KEYS[i], NULL) == 0)
        {
            stopped = (_sink->stopped != 0u) ? 1 : 0;

            break;
        }

        stopped = d_desc_emit(_sink, "source", NULL,
                              d_ostream_value_text(
                                  d_internal_option_env_desc_word(
                                      SOURCE_NAMES, 6u,
                                      layers[i].value.source)),
                              D_DESC_LAYER_PROVENANCE, 3u,
                              D_DESC_TRUTH_RUN_TIME);
        stopped = (stopped != 0)
                      ? 1
                      : d_internal_option_env_desc_value(
                            _sink, "value", _decl, &layers[i].value,
                            D_DESC_LAYER_PROVENANCE, 3u,
                            D_DESC_TRUTH_RUN_TIME);
        stopped = (stopped != 0)
                      ? 1
                      : d_desc_emit(_sink, "in_force", NULL,
                                    d_ostream_value_bool(layers[i].shown),
                                    D_DESC_LAYER_PROVENANCE, 3u,
                                    D_DESC_TRUTH_RUN_TIME);
        d_desc_leave(_sink);
    }

    d_desc_leave(_sink);

    return stopped;
}

/*
d_internal_option_env_desc_setting
  One setting's level: its kind, value, source, default, knob and help, the
sources it refuses, and what the flags ask.
*/
static int
d_internal_option_env_desc_setting(
    const struct dj_option_env* _env,
    struct dj_desc_sink*        _sink,
    uint32_t                    _scope,
    uint32_t                    _var,
    uint32_t                    _flags
)
{
    const struct dj_option_env_var* const decl = d_option_env_var_at(_env,
                                                                     _var);
    struct dj_option_env_value            value;
    struct dj_option_env_value            fallback;
    int                                   stopped;

    if ( (decl == NULL) ||
         (!d_option_env_get(_env, _scope, _var, &value).is_ok) )
    {
        return 0;
    }

    if (d_desc_enter(_sink, decl->name, decl->name) == 0)
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    d_internal_option_env_desc_default(decl, &fallback);
    stopped = d_internal_option_env_desc_value(_sink, "value", decl, &value,
                                               D_DESC_LAYER_STATE, 0u,
                                               D_DESC_TRUTH_RUN_TIME);
    stopped = (stopped != 0)
                  ? 1
                  : d_desc_emit(_sink, "source", NULL,
                                d_ostream_value_text(
                                    d_internal_option_env_desc_word(
                                        SOURCE_NAMES, 6u, value.source)),
                                D_DESC_LAYER_PROVENANCE, 1u,
                                D_DESC_TRUTH_RUN_TIME);
    stopped = (stopped != 0)
                  ? 1
                  : d_internal_option_env_desc_value(_sink, "default", decl,
                                                     &fallback,
                                                     D_DESC_LAYER_PROVENANCE,
                                                     1u, D_DESC_TRUTH_BUILD);
    stopped = (stopped != 0)
                  ? 1
                  : d_desc_emit(_sink, "kind", NULL,
                                d_ostream_value_text(
                                    d_internal_option_env_desc_word(
                                        KIND_NAMES, 7u, decl->kind)),
                                D_DESC_LAYER_TYPE, 2u,
                                D_DESC_TRUTH_IMPLEMENTATION);

    // the knob behind the default, and the help, only where there are any
    if ( (stopped == 0) &&
         (decl->knob != NULL) )
    {
        stopped = d_desc_emit(_sink, "knob", NULL,
                              d_ostream_value_text(decl->knob),
                              D_DESC_LAYER_PROVENANCE, 2u,
                              D_DESC_TRUTH_BUILD);
    }

    if ( (stopped == 0) &&
         (decl->help != NULL) )
    {
        stopped = d_desc_emit(_sink, "help", NULL,
                              d_ostream_value_text(decl->help),
                              D_DESC_LAYER_INTENT, 2u,
                              D_DESC_TRUTH_IMPLEMENTATION);
    }

    stopped = (stopped != 0) ? 1
                             : d_internal_option_env_desc_refuses(_sink, decl);

    if ( (stopped == 0) &&
         ((_flags & D_OPTION_ENV_DESC_ACCEPTS) != 0u) )
    {
        stopped = d_internal_option_env_desc_accepts(_sink, decl);
    }

    if ( (stopped == 0) &&
         ((_flags & D_OPTION_ENV_DESC_LAYERS) != 0u) )
    {
        stopped = d_internal_option_env_desc_layers(_sink, _env, _scope,
                                                    _var, decl);
    }

    d_desc_leave(_sink);

    return stopped;
}


//==============================================================================
// THE FRAME
//==============================================================================

/*
d_internal_option_env_desc_scope
  Every setting the frame can see, in one scope, root's first.
*/
static int
d_internal_option_env_desc_scope(
    const struct dj_option_env* _env,
    struct dj_desc_sink*        _sink,
    uint32_t                    _scope,
    uint32_t                    _flags
)
{
    const uint32_t count = d_option_env_var_count(_env);
    uint32_t       k;
    int            stopped = 0;

    for (k = 0u; (stopped == 0) && (k < count); ++k)
    {
        stopped = d_internal_option_env_desc_setting(
                      _env, _sink, _scope, d_option_env_var_id(_env, k),
                      _flags);
    }

    return stopped;
}

/*
d_option_env_describe
  The global scope's settings; then, asked, each scope the frame and its
ancestors name, a level each, under its name.
*/
int
d_option_env_describe(
    const struct dj_option_env* _env,
    struct dj_desc_sink*        _sink,
    uint32_t                    _flags
)
{
    const struct dj_option_env* frame;
    uint32_t                    s;
    int                         stopped;

    if ( (_env == NULL) ||
         (_sink == NULL) )
    {
        return -1;
    }

    stopped = d_internal_option_env_desc_scope(_env, _sink,
                                               D_OPTION_ENV_GLOBAL, _flags);

    if ((_flags & D_OPTION_ENV_DESC_SCOPES) == 0u)
    {
        return stopped;
    }

    for (frame = _env; (stopped == 0) && (frame != NULL);
         frame = frame->parent)
    {
        for (s = 0u; (stopped == 0) && (s < frame->scope_count); ++s)
        {
            const uint32_t scope = D_OPTION_ENV_ID(frame->depth, s + 1u);
            const char*    name  = d_option_env_scope_name(_env, scope);

            if ( (name == NULL) ||
                 (d_desc_enter(_sink, name, name) == 0) )
            {
                stopped = (_sink->stopped != 0u) ? 1 : 0;

                continue;
            }

            stopped = d_internal_option_env_desc_scope(
                          _env, _sink, scope,
                          _flags & ~(uint32_t)D_OPTION_ENV_DESC_SCOPES);
            d_desc_leave(_sink);
        }
    }

    return stopped;
}

/*
d_option_env_describer
  The frame's settings, no flags.
*/
int
d_option_env_describer(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    return d_option_env_describe((const struct dj_option_env*)_object,
                                 _sink, 0u);
}

#endif  // RE_STD_HAS_INT64
