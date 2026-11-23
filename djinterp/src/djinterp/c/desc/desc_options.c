/*******************************************************************************
* djinterp [c][desc]                                              desc_options.c
*
*   desc's djin, "desc", and what the units engine asks of it: the ring and
* the layers in force. Without the djin, the knobs answer.
*
*
* path:      /src/djinterp/c/desc/desc_options.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.17
*******************************************************************************/

#include "../../../../inc/djinterp/c/desc/desc_options.h"  // corresponding

// std
#include <string.h>  // strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/c/desc/desc_machines.h"  // describe's
                                                             // choices


#if D_DESC_OPTIONS_BUILT

//==============================================================================
// THE SCHEMA
//==============================================================================

// RINGS
//   constant: the `ring` variable's names.
static const struct dj_option_env_name RINGS[] =
{
    { "name",     (int64_t)D_DESC_RING_NAME     },
    { "line",     (int64_t)D_DESC_RING_LINE     },
    { "sentence", (int64_t)D_DESC_RING_SENTENCE },
    { "detailed", (int64_t)D_DESC_RING_DETAILED }
};

// LAYER_NAMES
//   constant: the `layers` variable's names: each layer, and sets.
static const struct dj_option_env_name LAYER_NAMES[] =
{
    { "none",       0x000                                      },
    { "identity",   (int64_t)1 << D_DESC_LAYER_IDENTITY        },
    { "type",       (int64_t)1 << D_DESC_LAYER_TYPE            },
    { "state",      (int64_t)1 << D_DESC_LAYER_STATE           },
    { "intent",     (int64_t)1 << D_DESC_LAYER_INTENT          },
    { "policy",     (int64_t)1 << D_DESC_LAYER_POLICY          },
    { "health",     (int64_t)1 << D_DESC_LAYER_HEALTH          },
    { "provenance", (int64_t)1 << D_DESC_LAYER_PROVENANCE      },
    { "contents",   (int64_t)1 << D_DESC_LAYER_CONTENTS        },
    { "cost",       (int64_t)1 << D_DESC_LAYER_COST            },
    { "all",        0x1FF                                      }
};

// STYLES, FORMS, FORMATS, TEXTS
//   constants: the choices of describe's four variables. A variable's names
// are a slice of its array, and the choices the knobs left out the rest, so
// they are refused by name.
static const struct dj_option_env_name STYLES[] =
{
    { "units", (int64_t)D_DESC_STYLE_UNITS },
    { "facts", (int64_t)D_DESC_STYLE_FACTS }
};

static const struct dj_option_env_name FORMS[] =
{
    { "facts", (int64_t)D_DESC_FORM_FACTS },
    { "text",  (int64_t)D_DESC_FORM_TEXT  }
};

static const struct dj_option_env_name FORMATS[] =
{
    { "text", (int64_t)D_DESC_FORMAT_TEXT },
    { "json", (int64_t)D_DESC_FORMAT_JSON }
};

static const struct dj_option_env_name TEXTS[] =
{
    { "report", (int64_t)D_DESC_TEXT_REPORT },
    { "line",   (int64_t)D_DESC_TEXT_LINE   }
};

// INTEGERS, NONFINITES
//   constants: how describe's JSON writes integers and NaN.
static const struct dj_option_env_name INTEGERS[] =
{
    { "number", 0 }, { "string", 1 }
};

static const struct dj_option_env_name NONFINITES[] =
{
    { "null", 0 }, { "string", 1 }
};

// D_INTERNAL_DESC_*_BUILT
//   macros: each variable's built names and its unbuilt ones, as a row's
// names, name_count, unbuilt, unbuilt_count.
#if D_CFG_DESC_UNITS && D_CFG_DESC_FACTS
    #define D_INTERNAL_DESC_STYLES_BUILT  STYLES, 2u, NULL, 0u
#elif D_CFG_DESC_UNITS
    #define D_INTERNAL_DESC_STYLES_BUILT  STYLES, 1u, STYLES + 1, 1u
#elif D_CFG_DESC_FACTS
    #define D_INTERNAL_DESC_STYLES_BUILT  STYLES + 1, 1u, STYLES, 1u
#else
    #define D_INTERNAL_DESC_STYLES_BUILT  NULL, 0u, STYLES, 2u
#endif

#if D_CFG_DESC_TEXT
    #define D_INTERNAL_DESC_FORMS_BUILT  FORMS, 2u, NULL, 0u
#else
    #define D_INTERNAL_DESC_FORMS_BUILT  FORMS, 1u, FORMS + 1, 1u
#endif

#if ((D_CFG_DESC_MACHINES & 0x3) == 0x3)
    #define D_INTERNAL_DESC_FORMATS_BUILT  FORMATS, 2u, NULL, 0u
#elif ((D_CFG_DESC_MACHINES & 0x3) == 0x1)
    #define D_INTERNAL_DESC_FORMATS_BUILT  FORMATS, 1u, FORMATS + 1, 1u
#elif ((D_CFG_DESC_MACHINES & 0x3) == 0x2)
    #define D_INTERNAL_DESC_FORMATS_BUILT  FORMATS + 1, 1u, FORMATS, 1u
#else
    #define D_INTERNAL_DESC_FORMATS_BUILT  NULL, 0u, FORMATS, 2u
#endif

// D_INTERNAL_DESC_CHOICE_ROW
//   macro: a CHOICE row of describe's, its names those built, its default a
// knob, named in the row.
#define D_INTERNAL_DESC_CHOICE_ROW(name, built, knob, help)                    \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_CHOICE, name, built, NULL,             \
                     (int64_t)(knob), INT64_MIN, INT64_MAX, 0.0, NULL, #knob, \
                     help)

// SCHEMA
//   constant: the djin's variables, in D_DESC_VAR_*'s order, their defaults
// the knobs.
static const struct dj_option_env_var SCHEMA[D_DESC_VARS] =
{
    D_OPTION_ENV_CHOICE_KNOB("ring", RINGS, D_CFG_DESC_RING,
                             "how much a description says: name, line, "
                             "sentence or detailed"),
    D_OPTION_ENV_FLAGS_KNOB("layers", LAYER_NAMES, D_CFG_DESC_LAYERS,
                            "the layers a description shows"),
    D_INTERNAL_DESC_CHOICE_ROW("style", D_INTERNAL_DESC_STYLES_BUILT,
                               D_CFG_DESC_STYLE,
                               "how describe writes: units (prose) or facts"),
    D_INTERNAL_DESC_CHOICE_ROW("form", D_INTERNAL_DESC_FORMS_BUILT,
                               D_CFG_DESC_FORM,
                               "the describer run when there are both: "
                               "facts or text"),
    D_INTERNAL_DESC_CHOICE_ROW("format", D_INTERNAL_DESC_FORMATS_BUILT,
                               D_CFG_DESC_FORMAT,
                               "the machine facts are written by: text or "
                               "json"),
    D_OPTION_ENV_CHOICE_KNOB("text", TEXTS, D_CFG_DESC_TEXT_PRESET,
                             "the text machine's preset: report or line"),
    D_OPTION_ENV_CHOICE_KNOB("json_integers", INTEGERS,
                             D_CFG_DESC_JSON_INTEGERS,
                             "integers in describe's JSON: number or string"),
    D_OPTION_ENV_CHOICE_KNOB("json_nonfinite", NONFINITES,
                             D_CFG_DESC_JSON_NONFINITE,
                             "NaN and infinities in describe's JSON: null or "
                             "string")
};


//==============================================================================
// THE DJIN
//==============================================================================

// dj_internal_desc_djin_storage
//   struct: the djin's storage: its bindings, its two variables, no text.
D_OPTION_ENV_DECLARE(dj_internal_desc_djin_storage,
                     D_CFG_DESC_BINDINGS,
                     D_DESC_VARS,
                     0u);

// dj_internal_desc_djin
//   struct: the djin: its frame, its storage, and its scopes' names, one per
// declared type.
struct dj_internal_desc_djin
{
    struct dj_option_env                 frame;
    struct dj_internal_desc_djin_storage storage;
    const char*                          scopes[D_CFG_DESC_SCOPES];
};

// d_internal_desc_djin
//   variable: the djin "desc", its variables declared by its initialiser --
// what d_option_env_declare_all would leave - so it is ready before the
// program runs. It has no scopes until a type is declared.
static struct dj_internal_desc_djin d_internal_desc_djin =
{
    .frame =
    {
        .set =
        {
            .options        = d_internal_desc_djin.storage.option_block,
            .values         = (unsigned char*)
                                  d_internal_desc_djin.storage.value_block,
            .count          = 0u,
            .capacity       = (uint32_t)D_CFG_DESC_BINDINGS,
            .value_used     = 0u,
            .value_capacity = (uint32_t)(sizeof(uint64_t) *
                                         D_CFG_DESC_BINDINGS)
        },
        .vars          = d_internal_desc_djin.storage.var_block,
        .scopes        = (const char* const*)d_internal_desc_djin.scopes,
        .name          = "desc",
        .text          = d_internal_desc_djin.storage.text_block,
        .text_capacity = 0u,
        .var_count     = (uint32_t)D_DESC_VARS,
        .var_capacity  = (uint32_t)D_DESC_VARS,
        .scope_count   = 0u
    },
    .storage =
    {
        .var_block =
        {
            &SCHEMA[D_DESC_VAR_RING],
            &SCHEMA[D_DESC_VAR_LAYERS],
            &SCHEMA[D_DESC_VAR_STYLE],
            &SCHEMA[D_DESC_VAR_FORM],
            &SCHEMA[D_DESC_VAR_FORMAT],
            &SCHEMA[D_DESC_VAR_TEXT],
            &SCHEMA[D_DESC_VAR_JSON_INTEGERS],
            &SCHEMA[D_DESC_VAR_JSON_NONFINITE]
        }
    }
};

struct dj_option_env*
d_desc_djin(void)
{
    return &d_internal_desc_djin.frame;
}

struct dj_option_result
d_desc_register(
    struct dj_option_env_registry* _registry
)
{
    return d_option_env_registry_add((_registry != NULL)
                                         ? _registry
                                         : d_option_env_registry(),
                                     &d_internal_desc_djin.frame);
}

/*
d_internal_desc_identifier
  Whether a name is an identifier, as a scope's must be.
*/
static int
d_internal_desc_identifier(
    const char* _name
)
{
    const char* at;

    if ( (_name == NULL)    ||
         (_name[0] == '\0') ||
         ((_name[0] >= '0') && (_name[0] <= '9')) )
    {
        return 0;
    }

    for (at = _name; *at != '\0'; ++at)
    {
        if ( !( ((*at >= 'a') && (*at <= 'z')) ||
                ((*at >= 'A') && (*at <= 'Z')) ||
                ((*at >= '0') && (*at <= '9')) ||
                (*at == '_') ) )
        {
            return 0;
        }
    }

    return 1;
}

int
d_desc_declare(
    const struct dj_desc_type* _type
)
{
    struct dj_internal_desc_djin* djin  = &d_internal_desc_djin;
    uint32_t                      count = djin->frame.scope_count;
    uint32_t                      index;

    if ( (_type == NULL) || (d_internal_desc_identifier(_type->name) == 0) )
    {
        return -1;
    }

    for (index = 0u; index < count; ++index)
    {
        if (strcmp(djin->scopes[index], _type->name) == 0)
        {
            return 0;
        }
    }

    if (count >= (uint32_t)D_CFG_DESC_SCOPES)
    {
        return -1;
    }

    djin->scopes[count] = _type->name;

    return d_option_env_set_scopes(&djin->frame,
                                   (const char* const*)djin->scopes,
                                   count + 1u).is_ok ? 0 : -1;
}

/*
d_internal_desc_option
  A variable in the type's scope when the type was declared, else globally;
a value outside [0, _count) is the knob's.
*/
static uint32_t
d_internal_desc_option(
    const struct dj_desc_type* _type,
    uint32_t                   _var,
    uint32_t                   _count,
    uint32_t                   _knob
)
{
    const struct dj_option_env* frame = &d_internal_desc_djin.frame;
    uint32_t                    scope = D_OPTION_ENV_GLOBAL;
    struct dj_option_result     found;
    int64_t                     value;

    if ( (_type != NULL) && (_type->name != NULL) )
    {
        found = d_option_env_find_scope(frame, _type->name,
                                        strlen(_type->name));

        if (found.is_ok)
        {
            scope = found.payload.value;
        }
    }

    value = d_option_env_int(frame, scope,
                             D_OPTION_ENV_ID(frame->depth, _var));

    return ( (value >= 0) && (value < (int64_t)_count) ) ? (uint32_t)value
                                                         : _knob;
}

uint32_t
d_desc_option_ring(
    const struct dj_desc_type* _type
)
{
    return d_internal_desc_option(_type, D_DESC_VAR_RING, D_DESC_RING_COUNT,
                                  (uint32_t)D_CFG_DESC_RING);
}

uint32_t
d_desc_option_style(
    const struct dj_desc_type* _type
)
{
    return d_internal_desc_option(_type, D_DESC_VAR_STYLE, 2u,
                                  (uint32_t)D_CFG_DESC_STYLE);
}

uint32_t
d_desc_option_form(
    const struct dj_desc_type* _type
)
{
    return d_internal_desc_option(_type, D_DESC_VAR_FORM, 2u,
                                  (uint32_t)D_CFG_DESC_FORM);
}

uint32_t
d_desc_option_format(
    const struct dj_desc_type* _type
)
{
    return d_internal_desc_option(_type, D_DESC_VAR_FORMAT, 2u,
                                  (uint32_t)D_CFG_DESC_FORMAT);
}

uint32_t
d_desc_option_text(
    const struct dj_desc_type* _type
)
{
    return d_internal_desc_option(_type, D_DESC_VAR_TEXT, 2u,
                                  (uint32_t)D_CFG_DESC_TEXT_PRESET);
}

uint32_t
d_desc_option_json_integers(
    const struct dj_desc_type* _type
)
{
    return d_internal_desc_option(_type, D_DESC_VAR_JSON_INTEGERS, 2u,
                                  (uint32_t)D_CFG_DESC_JSON_INTEGERS);
}

uint32_t
d_desc_option_json_nonfinite(
    const struct dj_desc_type* _type
)
{
    return d_internal_desc_option(_type, D_DESC_VAR_JSON_NONFINITE, 2u,
                                  (uint32_t)D_CFG_DESC_JSON_NONFINITE);
}

uint32_t
d_desc_option_layers(void)
{
    const struct dj_option_env* frame = &d_internal_desc_djin.frame;

    return (uint32_t)d_option_env_int(frame, D_OPTION_ENV_GLOBAL,
                                      D_OPTION_ENV_ID(frame->depth,
                                                      D_DESC_VAR_LAYERS)) &
           (uint32_t)D_DESC_LAYERS_ALL;
}

#else  // !D_DESC_OPTIONS_BUILT: the knobs decide alone

int
d_desc_declare(
    const struct dj_desc_type* _type
)
{
    return (_type != NULL) ? 0 : -1;
}

uint32_t
d_desc_option_ring(
    const struct dj_desc_type* _type
)
{
    (void)_type;

    return (uint32_t)D_CFG_DESC_RING;
}

uint32_t
d_desc_option_layers(void)
{
    return (uint32_t)D_CFG_DESC_LAYERS;
}

uint32_t
d_desc_option_style(
    const struct dj_desc_type* _type
)
{
    (void)_type;

    return (uint32_t)D_CFG_DESC_STYLE;
}

uint32_t
d_desc_option_form(
    const struct dj_desc_type* _type
)
{
    (void)_type;

    return (uint32_t)D_CFG_DESC_FORM;
}

uint32_t
d_desc_option_format(
    const struct dj_desc_type* _type
)
{
    (void)_type;

    return (uint32_t)D_CFG_DESC_FORMAT;
}

uint32_t
d_desc_option_text(
    const struct dj_desc_type* _type
)
{
    (void)_type;

    return (uint32_t)D_CFG_DESC_TEXT_PRESET;
}

uint32_t
d_desc_option_json_integers(
    const struct dj_desc_type* _type
)
{
    (void)_type;

    return (uint32_t)D_CFG_DESC_JSON_INTEGERS;
}

uint32_t
d_desc_option_json_nonfinite(
    const struct dj_desc_type* _type
)
{
    (void)_type;

    return (uint32_t)D_CFG_DESC_JSON_NONFINITE;
}

#endif  // D_DESC_OPTIONS_BUILT
