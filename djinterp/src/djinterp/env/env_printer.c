/*******************************************************************************
* djinterp [env]                                                   env_printer.c
*
*   The environment printer's machine (env_printer.h): its schema, its five
* presets, the compilation of its program, and the run of that program over
* a report. Compiled once: the report is collected in each caller's unit, and
* only its printing is shared.
*
*   A PRINT IS THREE WALKS OVER THE REPORT, AND NONE ALLOCATES. The first
* counts the sections that will show, which their separators and
* {section_count} need before the first is printed. For each section, the
* second counts its items and measures their column; the third prints them.
* Whether an item shows is asked anew each walk -- `select`, `features` and
* the filter are cheap, and no array of answers has to be sized for a report
* of any length.
*
*   THE VARIABLES IN FORCE ARE TAKEN ONCE PER SCOPE. Before the report and
* before each section, every variable a field names is read through the
* environment's frames for that scope into a table of template values, so a
* field that names a variable is an index into the table. Which variable a
* field names is the printer's to keep: its own first, then those declared,
* then any other a template names, each by id, so a field's number does not
* depend on which frame declared its variable.
*
*   THE DJIN IS STATIC. Its frame, its storage and its declarations -- the
* printer's variables and the sections' scopes -- are its initialiser's, done
* before the program runs, so no first call can race another.
*
*   EVERY WRITE IS THE STREAM'S, AND IS CHECKED ONCE A SECTION. The templates
* write and go on; the stream's status is sticky, so the first failure is the
* one reported, and a failed stream ends the print at the next section.
*
*
* path:      /src/djinterp/env/env_printer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.04
*******************************************************************************/
#include "../../../inc/djinterp/env/env_printer.h"  // corresponding header
// std
#include <errno.h>   // EINVAL
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memcmp, memcpy, strlen
// djinterp
#include "../../../inc/djinterp/c/util/ostream/ostream.h"         // d_ostream_*
#include "../../../inc/djinterp/c/text/ostream_template.h"        // templates
#include "../../../inc/djinterp/c/option/option_env.h"            // variables
#include "../../../inc/djinterp/c/option/option_env_registry.h"   // registry
#include "../../../inc/djinterp/config/env/cfg_env_printer.h"     // defaults
// re_std
#include "../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t, int64_t

#if RE_STD_HAS_INT64


// D_INTERNAL_ENV_PRINTER_SCOPES
//   constant: the scopes: one per section the printer knows, by slug.
#define D_INTERNAL_ENV_PRINTER_SCOPES  10u

// D_INTERNAL_ENV_PRINTER_PATH
//   constant: the bytes a {path} is built in: a slug, a '.' and a name.
#define D_INTERNAL_ENV_PRINTER_PATH    256u

// D_INTERNAL_ENV_PRINTER_LINE
//   constant: the bytes a variable's value is written into by a dump.
#define D_INTERNAL_ENV_PRINTER_LINE    256u

// D_INTERNAL_ENV_PRINTER_WIDTH
//   constant: the widest column key_width and name_width take.
#define D_INTERNAL_ENV_PRINTER_WIDTH   255


//==============================================================================
// THE SCHEMA
//==============================================================================

// STYLES
//   constant: the `style` variable's names.
static const struct dj_option_env_name STYLES[] =
{
    { "text",     D_ENV_STYLE_TEXT     },
    { "json",     D_ENV_STYLE_JSON     },
    { "markdown", D_ENV_STYLE_MARKDOWN },
    { "csv",      D_ENV_STYLE_CSV      },
    { "keyvalue", D_ENV_STYLE_KEYVALUE }
};

// SECTION_NAMES
//   constant: the `sections` variable's names: every section, and sets.
static const struct dj_option_env_name SECTION_NAMES[] =
{
    { "none",                0x0000                     },
    { "language",            D_ENV_SECTION_LANGUAGE     },
    { "compiler",            D_ENV_SECTION_COMPILER     },
    { "os",                  D_ENV_SECTION_OS           },
    { "architecture",        D_ENV_SECTION_ARCH         },
    { "posix",               D_ENV_SECTION_POSIX        },
    { "preprocessor_limits", D_ENV_SECTION_PP_LIMITS    },
    { "build",               D_ENV_SECTION_BUILD        },
    { "c_runtime",           D_ENV_SECTION_C_RUNTIME    },
    { "cpp_summary",         D_ENV_SECTION_CPP_SUMMARY  },
    { "cpp_features",        D_ENV_SECTION_CPP_FEATURES },
    { "core",                D_ENV_SECTIONS_CORE        },
    { "cpp",                 D_ENV_SECTIONS_CPP         },
    { "all",                 D_ENV_SECTIONS_ALL         }
};

// FEATURE_FILTERS
//   constant: the `features` variable's names.
static const struct dj_option_env_name FEATURE_FILTERS[] =
{
    { "all",         D_ENV_FEATURES_ALL         },
    { "supported",   D_ENV_FEATURES_SUPPORTED   },
    { "unsupported", D_ENV_FEATURES_UNSUPPORTED }
};

// SCHEMA
//   constant: the printer's variables, in enum dj_env_printer_var's order,
// their defaults pinned here.
static const struct dj_option_env_var SCHEMA[D_ENV_PRINTER_VARS] =
{
    D_OPTION_ENV_CHOICE_KNOB("style", STYLES, D_CFG_ENV_PRINTER_STYLE,
                             "the preset a template that is not set comes "
                             "from"),
    D_OPTION_ENV_FLAGS_KNOB("sections", SECTION_NAMES,
                            D_CFG_ENV_PRINTER_SECTIONS,
                            "the sections shown"),
    D_OPTION_ENV_TEXT("select", NULL,
                      "rules over section.name paths, [+|-]glob, the last "
                      "that matches deciding"),
    D_OPTION_ENV_CHOICE("features", FEATURE_FILTERS, D_ENV_FEATURES_ALL,
                        "the C++ features shown"),
    D_OPTION_ENV_TEXT("title", "djinterp environment", "the report's title"),
    D_OPTION_ENV_BOOL("show_title", 1, "whether the title is shown"),
    D_OPTION_ENV_BOOL("show_standards", 1,
                      "whether a feature's standard is shown"),
    D_OPTION_ENV_BOOL("show_descriptions", 1,
                      "whether a feature's description is shown"),
    D_OPTION_ENV_INT_RANGE("key_width", 0, 0, D_INTERNAL_ENV_PRINTER_WIDTH,
                           "an entry's label column; 0 fits the section's "
                           "longest"),
    D_OPTION_ENV_INT_RANGE("name_width", 0, 0, D_INTERNAL_ENV_PRINTER_WIDTH,
                           "a feature's name column; 0 fits the longest"),
    D_OPTION_ENV_TEXT("prefix", "", "written at the start of every line"),
    D_OPTION_ENV_TEXT("true_text", "yes", "how the text styles write true"),
    D_OPTION_ENV_TEXT("false_text", "no", "how the text styles write false"),
    D_OPTION_ENV_TEXT("report_begin", NULL,
                      "template: the report's start; unset: the style's"),
    D_OPTION_ENV_TEXT("report_end", NULL,
                      "template: the report's end; unset: the style's"),
    D_OPTION_ENV_TEXT("section_begin", NULL,
                      "template: a section's start; unset: the style's"),
    D_OPTION_ENV_TEXT("section_end", NULL,
                      "template: a section's end; unset: the style's"),
    D_OPTION_ENV_TEXT("section_separator", NULL,
                      "template: between sections; unset: the style's"),
    D_OPTION_ENV_TEXT("entry", NULL,
                      "template: an entry; unset: the style's"),
    D_OPTION_ENV_TEXT("feature", NULL,
                      "template: a C++ feature; unset: the style's"),
    D_OPTION_ENV_TEXT("note", NULL,
                      "template: a section's note; unset: the style's"),
    D_OPTION_ENV_TEXT("item_separator", NULL,
                      "template: between items; unset: the style's")
};

// SCOPES
//   constant: the scopes' names, the sections' slugs, in their ids' order.
static const char* const SCOPES[D_INTERNAL_ENV_PRINTER_SCOPES] =
{
    "language",
    "compiler",
    "os",
    "architecture",
    "posix",
    "preprocessor_limits",
    "build",
    "c_runtime",
    "cpp_summary",
    "cpp_features"
};

// FIELD_NAMES
//   constant: the fields' names, in enum dj_env_field's order.
static const char* const FIELD_NAMES[D_ENV_FIELDS] =
{
    "section",
    "section_title",
    "section_index",
    "section_count",
    "count",
    "has_features",
    "path",
    "name",
    "label",
    "value",
    "display",
    "kind",
    "index",
    "first",
    "last",
    "standard",
    "description",
    "supported",
    "note",
    "width"
};

// KIND_NAMES
//   constant: what {kind} says, by item kind; entries say their value's.
static const char* const KIND_NAMES[] =
{
    "report", "section", "entry", "feature", "note"
};

// VALUE_KIND_NAMES
//   constant: what {kind} says for an entry, by its value's kind.
static const char* const VALUE_KIND_NAMES[] =
{
    "text", "integer", "boolean"
};


//==============================================================================
// THE PRESETS
//==============================================================================

// PRESETS
//   constant: each style's template for each part, in enum dj_env_template's
// order. They are written in the language any program's templates are, and
// d_env_style_template gives them out to start a program's own from.
static const char* const PRESETS[D_ENV_STYLE_COUNT][D_ENV_TEMPLATE_COUNT] =
{
    // text: aligned lines; the column fits the section's longest label
    {
        "{?show_title}{prefix}{title}\n{?section_count}{prefix}\n{/}{/}",
        "",
        "{prefix}{section_title}\n",
        "",
        "{prefix}\n",
        "{prefix}  {label:<{width}}  {display}\n",
        "{prefix}  [{?supported}+{:}-{/}] "
            "{?show_standards}{standard:<6} {/}"
            "{name:<{width}}  {value:>6}"
            "{?show_descriptions}  {description}{/}\n",
        "{prefix}  ({note})\n",
        ""
    },
    // json: one object, a member per section, and the title first
    {
        "{prefix}{{\n"
            "{?show_title}{prefix}  \"title\": {title!json}"
            "{?section_count},{/}\n{/}",
        "{?section_count}\n{/}{prefix}}}\n",
        "{prefix}  {section!json}: {{\n",
        "\n{prefix}  }}",
        ",\n",
        "{prefix}    {name!json}: {value!json}",
        "{prefix}    {name!json}: {{ \"value\": {value!json}, "
            "\"supported\": {supported!json}"
            "{?show_standards}, \"standard\": {standard!json}{/}"
            "{?show_descriptions}, \"description\": {description!json}{/} }}",
        "{prefix}    \"note\": {note!json}",
        ",\n"
    },
    // markdown: a heading and a table per section
    {
        "{?show_title}{prefix}# {title!md}\n{?section_count}{prefix}\n{/}{/}",
        "",
        "{prefix}## {section_title!md}\n{prefix}\n"
            "{?has_features}"
            "{prefix}| Feature | Value | Supported |"
            "{?show_standards} Standard |{/}"
            "{?show_descriptions} Description |{/}\n"
            "{prefix}| --- | ---: | :---: |"
            "{?show_standards} --- |{/}"
            "{?show_descriptions} --- |{/}\n"
            "{:}"
            "{prefix}| Key | Value |\n"
            "{prefix}| --- | --- |\n"
            "{/}",
        "",
        "{prefix}\n",
        "{prefix}| {label!md} | {display!md} |\n",
        "{prefix}| `{name}` | {value} | "
            "{?supported}{true_text!md}{:}{false_text!md}{/} |"
            "{?show_standards} {standard!md} |{/}"
            "{?show_descriptions} {description!md} |{/}\n",
        "{prefix}| *{note!md}* | |\n",
        ""
    },
    // csv: a header, then a row per item; data, so no title and no prefix
    {
        "section,name,label,kind,value,supported,standard,description\n",
        "",
        "",
        "",
        "",
        "{section!csv},{name!csv},{label!csv},{kind},{value!csv},,,\n",
        "{section!csv},{name!csv},{label!csv},feature,{value},{supported},"
            "{?show_standards}{standard!csv}{/},"
            "{?show_descriptions}{description!csv}{/}\n",
        "{section!csv},note,,note,{note!csv},,,\n",
        ""
    },
    // keyvalue: section.name=value, for grep and for shells
    {
        "{?show_title}{prefix}# {title}\n{/}",
        "",
        "",
        "",
        "",
        "{prefix}{path}={value}\n",
        "{prefix}{path}={value}\n",
        "{prefix}# {path}: {note}\n",
        ""
    }
};


//==============================================================================
// THE DJIN
//==============================================================================

// dj_internal_env_printer_djin_storage
//   struct: the djin's storage: as a printer's.
D_OPTION_ENV_DECLARE(dj_internal_env_printer_djin_storage,
                     D_CFG_ENV_PRINTER_BINDINGS,
                     D_CFG_ENV_PRINTER_VARIABLES,
                     D_CFG_ENV_PRINTER_TEXT);

// dj_internal_env_printer_djin
//   struct: the djin: its frame, and the storage the frame points into.
struct dj_internal_env_printer_djin
{
    struct dj_option_env                        frame;
    struct dj_internal_env_printer_djin_storage storage;
};

// d_internal_env_printer_djin
//   variable: the djin "env_printer": the printer's variables declared, in
// the schema's order, and the sections' slugs as its scopes, by its
// initialiser -- what d_option_env_declare_all and d_option_env_set_scopes
// would leave -- so it is ready before the program runs.
static struct dj_internal_env_printer_djin d_internal_env_printer_djin =
{
    .frame =
    {
        .set =
        {
            .options        =
                d_internal_env_printer_djin.storage.option_block,
            .values         = (unsigned char*)
                d_internal_env_printer_djin.storage.value_block,
            .count          = 0u,
            .capacity       = (uint32_t)D_CFG_ENV_PRINTER_BINDINGS,
            .value_used     = 0u,
            .value_capacity = (uint32_t)(sizeof(uint64_t) *
                                         D_CFG_ENV_PRINTER_BINDINGS)
        },
        .vars          = d_internal_env_printer_djin.storage.var_block,
        .scopes        = SCOPES,
        .name          = "env_printer",
        .text          = d_internal_env_printer_djin.storage.text_block,
        .text_capacity = (size_t)D_CFG_ENV_PRINTER_TEXT,
        .var_count     = (uint32_t)D_ENV_PRINTER_VARS,
        .var_capacity  = (uint32_t)D_CFG_ENV_PRINTER_VARIABLES,
        .scope_count   = D_INTERNAL_ENV_PRINTER_SCOPES
    },
    .storage =
    {
        .var_block =
        {
            &SCHEMA[D_ENV_PRINTER_STYLE],
            &SCHEMA[D_ENV_PRINTER_SECTIONS],
            &SCHEMA[D_ENV_PRINTER_SELECT],
            &SCHEMA[D_ENV_PRINTER_FEATURES],
            &SCHEMA[D_ENV_PRINTER_TITLE],
            &SCHEMA[D_ENV_PRINTER_SHOW_TITLE],
            &SCHEMA[D_ENV_PRINTER_SHOW_STANDARDS],
            &SCHEMA[D_ENV_PRINTER_SHOW_DESCRIPTIONS],
            &SCHEMA[D_ENV_PRINTER_KEY_WIDTH],
            &SCHEMA[D_ENV_PRINTER_NAME_WIDTH],
            &SCHEMA[D_ENV_PRINTER_PREFIX],
            &SCHEMA[D_ENV_PRINTER_TRUE_TEXT],
            &SCHEMA[D_ENV_PRINTER_FALSE_TEXT],
            &SCHEMA[D_ENV_PRINTER_REPORT_BEGIN],
            &SCHEMA[D_ENV_PRINTER_REPORT_END],
            &SCHEMA[D_ENV_PRINTER_SECTION_BEGIN],
            &SCHEMA[D_ENV_PRINTER_SECTION_END],
            &SCHEMA[D_ENV_PRINTER_SECTION_SEPARATOR],
            &SCHEMA[D_ENV_PRINTER_ENTRY],
            &SCHEMA[D_ENV_PRINTER_FEATURE],
            &SCHEMA[D_ENV_PRINTER_NOTE],
            &SCHEMA[D_ENV_PRINTER_ITEM_SEPARATOR]
        }
    }
};

/*
d_env_printer_djin
  The static one.
*/
struct dj_option_env*
d_env_printer_djin(void)
{
    return &d_internal_env_printer_djin.frame;
}

/*
d_env_printer_register
  The registry's add, of the djin.
*/
struct dj_option_result
d_env_printer_register(
    struct dj_option_env_registry* _registry
)
{
    return d_option_env_registry_add((_registry != NULL)
                                         ? _registry
                                         : d_option_env_registry(),
                                     &d_internal_env_printer_djin.frame);
}


//==============================================================================
// COMPILING THE PROGRAM
//==============================================================================

/**
 * @brief Where the printer's variables are: the id of the first of them in
 *        the innermost frame that declares them, the one their names find.
 *
 * @param[in] _env  the printer's frame.
 * @return the id, or D_OPTION_ENV_NONE when no frame declares them.
 */
static uint32_t
d_internal_env_printer_find_schema(
    const struct dj_option_env* _env
)
{
    for (uint32_t k = d_option_env_var_count(_env); k > 0u; --k)
    {
        const uint32_t var = d_option_env_var_id(_env, k - 1u);

        if (d_option_env_var_at(_env, var) == &SCHEMA[0])
        {
            return var;
        }
    }

    return D_OPTION_ENV_NONE;
}

/**
 * @brief The id of the printer's first variable as its frame stands: the
 *        one kept, unless the frame moved since.
 *
 * @param[in] _printer  the printer.
 * @return the id, or D_OPTION_ENV_NONE.
 *
 * Only the printer's own frame can have moved: a frame with children, as
 * each of its ancestors is, cannot be given another parent, so the same
 * parent means the same chain, the same depths and the same ids.
 */
static uint32_t
d_internal_env_printer_base(
    const struct dj_env_printer* _printer
)
{
    return (_printer->env.parent == _printer->var_parent)
               ? _printer->var_base
               : d_internal_env_printer_find_schema(&_printer->env);
}

/**
 * @brief Brings the fields' ids up to date with the printer's frame.
 *
 * @param[in,out] _printer  the printer.
 * @return 1, or 0 when no frame declares the printer's variables any more.
 *
 * A frame given a new parent keeps its declarations but not their ids, which
 * carry its depth: the printer's own come from wherever they now are, and
 * those it declared are its frame's, at the depth it now has. The fields a
 * template gave out are dropped, to be given again as it compiles.
 */
static int32_t
d_internal_env_printer_rebase(
    struct dj_env_printer* _printer
)
{
    const uint32_t base = d_internal_env_printer_base(_printer);

    if (base == D_OPTION_ENV_NONE)
    {
        return 0;
    }

    if (_printer->env.parent == _printer->var_parent)
    {
        return 1;
    }

    _printer->var_base   = base;
    _printer->var_parent = _printer->env.parent;

    for (uint32_t i = 0u; i < _printer->var_fixed; ++i)
    {
        _printer->vars[i] = (i < (uint32_t)D_ENV_PRINTER_VARS)
                                ? base + i
                                : D_OPTION_ENV_ID(
                                      _printer->env.depth,
                                      D_OPTION_ENV_ID_INDEX(_printer->vars[i]));
    }

    _printer->var_count = _printer->var_fixed;
    _printer->compiled  = 0;

    return 1;
}

/**
 * @brief The field that names a variable: the one it has, or the next.
 *
 * @param[in,out] _printer  the printer.
 * @param[in]     _var      the variable.
 * @return the field, or -1 when every field is taken.
 */
static int32_t
d_internal_env_printer_field(
    struct dj_env_printer* _printer,
    uint32_t               _var
)
{
    for (uint32_t i = 0u; i < _printer->var_count; ++i)
    {
        if (_printer->vars[i] == _var)
        {
            return (int32_t)(D_ENV_FIELDS + i);
        }
    }

    if (_printer->var_count >= (uint32_t)D_CFG_ENV_PRINTER_VARIABLES)
    {
        return -1;
    }

    _printer->vars[_printer->var_count] = _var;

    return (int32_t)(D_ENV_FIELDS + _printer->var_count++);
}

/**
 * @brief A template name's field: one of the item's, or a variable's.
 *
 * @param[in] _context  the printer.
 * @param[in] _name     the name.
 * @param[in] _size     its length.
 * @return the field, or -1.
 */
static int32_t
d_internal_env_printer_lookup(
    void*       _context,
    const char* _name,
    size_t      _size
)
{
    struct dj_env_printer* const printer = (struct dj_env_printer*)_context;

    for (uint32_t i = 0u; i < D_ENV_FIELDS; ++i)
    {
        if ( (strlen(FIELD_NAMES[i]) == _size) &&
             (memcmp(FIELD_NAMES[i], _name, _size) == 0) )
        {
            return (int32_t)i;
        }
    }

    const struct dj_option_result found = d_option_env_find(&printer->env,
                                                            _name,
                                                            _size);

    return found.is_ok
               ? d_internal_env_printer_field(printer, found.payload.value)
               : -1;
}

/**
 * @brief Compiles the program if a variable changed since it last was.
 *
 * @param[in,out] _printer  the printer.
 * @return 1 when the program is current, 0 when a template does not compile,
 *         with the printer's error saying which and where.
 *
 * Each part's template is its variable's text, or its style's preset when
 * the variable is null; all nine share the printer's one pool of operations,
 * in order.
 */
static int32_t
d_internal_env_printer_compile(
    struct dj_env_printer* _printer
)
{
    const uint32_t generation = d_option_env_generation(&_printer->env);
    size_t         used       = 0u;

    if ( (_printer->compiled != 0)               &&
         (_printer->generation == generation)    &&
         (_printer->env.parent == _printer->var_parent) )
    {
        return 1;
    }

    if (d_internal_env_printer_rebase(_printer) == 0)
    {
        _printer->compiled     = 0;
        _printer->error.offset = 0u;
        _printer->error.part   = -1;
        _printer->error.status = D_OSTREAM_TEMPLATE_INVALID;

        return 0;
    }

    int64_t style = d_option_env_int(&_printer->env,
                                     D_OPTION_ENV_GLOBAL,
                                     _printer->var_base + D_ENV_PRINTER_STYLE);

    if ( (style < 0) ||
         (style >= D_ENV_STYLE_COUNT) )
    {
        style = D_ENV_STYLE_TEXT;
    }

    _printer->compiled     = 0;
    _printer->var_count    = _printer->var_fixed;
    _printer->error.offset = 0u;
    _printer->error.part   = -1;
    _printer->error.status = D_OSTREAM_TEMPLATE_OK;

    for (int32_t part = 0; part < D_ENV_TEMPLATE_COUNT; ++part)
    {
        struct dj_ostream_template_error error;
        const char*                      source = d_option_env_text(
            &_printer->env,
            D_OPTION_ENV_GLOBAL,
            _printer->var_base + (uint32_t)(D_ENV_PRINTER_REPORT_BEGIN + part));

        if (source == NULL)
        {
            source = PRESETS[style][part];
        }

        const int32_t status = d_ostream_template_compile(
                                   &_printer->templates[part],
                                   _printer->ops + used,
                                   D_CFG_ENV_PRINTER_TEMPLATE_OPS - used,
                                   source,
                                   d_internal_env_printer_lookup,
                                   _printer,
                                   &error);

        if (status != D_OSTREAM_TEMPLATE_OK)
        {
            _printer->error.offset = error.offset;
            _printer->error.part   = part;
            _printer->error.status = status;

            return 0;
        }

        used += _printer->templates[part].count;
    }

    _printer->compiled   = 1;
    _printer->generation = generation;

    return 1;
}

/**
 * @brief Compiles the program after a change, and turns a template that does
 *        not compile into the change's failure.
 *
 * @param[in,out] _printer  the printer.
 * @param[in]     _result   the change's own result.
 * @return _result, or INVALID_ARGUMENT for a template that does not compile.
 */
static struct dj_option_result
d_internal_env_printer_after(
    struct dj_env_printer*  _printer,
    struct dj_option_result _result
)
{
    if ( (_result.is_ok) &&
         (d_internal_env_printer_compile(_printer) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return _result;
}


//==============================================================================
// SETTING UP AND CHANGING VARIABLES
//==============================================================================

/*
d_env_printer_init
  A printer in no frame but its own.
*/
int32_t
d_env_printer_init(
    struct dj_env_printer* _printer
)
{
    return d_env_printer_init_in(_printer, NULL);
}

/*
d_env_printer_init_in
  A frame over the printer's own storage, under the parent; the printer's
variables an ancestor's when one declares them, else declared here with the
sections' slugs as its scopes. Nothing is compiled until something is
printed or set.
*/
int32_t
d_env_printer_init_in(
    struct dj_env_printer* _printer,
    struct dj_option_env*  _parent
)
{
    if (_printer == NULL)
    {
        return 0;
    }

    if ( (!D_OPTION_ENV_VIEW(&_printer->env, _printer->storage).is_ok) ||
         ( (_parent != NULL) &&
           (!d_option_env_set_parent(&_printer->env, _parent).is_ok) ) )
    {
        return 0;
    }

    uint32_t base = d_internal_env_printer_find_schema(&_printer->env);

    // no frame above declares them: this one does
    if (base == D_OPTION_ENV_NONE)
    {
        const struct dj_option_result declared = d_option_env_declare_all(
                                                     &_printer->env,
                                                     SCHEMA,
                                                     D_ENV_PRINTER_VARS);

        if ( (!declared.is_ok) ||
             (!d_option_env_set_scopes(&_printer->env,
                                       SCOPES,
                                       D_INTERNAL_ENV_PRINTER_SCOPES).is_ok) )
        {
            d_option_env_detach(&_printer->env);

            return 0;
        }

        base = declared.payload.value;
    }

    for (uint32_t i = 0u; i < (uint32_t)D_ENV_PRINTER_VARS; ++i)
    {
        _printer->vars[i] = base + i;
    }

    _printer->var_parent = _printer->env.parent;
    _printer->var_base   = base;
    _printer->var_count  = (uint32_t)D_ENV_PRINTER_VARS;
    _printer->var_fixed  = (uint32_t)D_ENV_PRINTER_VARS;

    for (int32_t part = 0; part < D_ENV_TEMPLATE_COUNT; ++part)
    {
        _printer->templates[part].source   = NULL;
        _printer->templates[part].ops      = NULL;
        _printer->templates[part].count    = 0u;
        _printer->templates[part].reserved = 0u;
    }

    _printer->error.offset   = 0u;
    _printer->error.part     = -1;
    _printer->error.status   = D_OSTREAM_TEMPLATE_OK;
    _printer->filter         = NULL;
    _printer->filter_context = NULL;
    _printer->field          = NULL;
    _printer->field_context  = NULL;
    _printer->generation     = 0u;
    _printer->compiled       = 0;

    return 1;
}

/*
d_env_printer_reset
  The bindings only: declarations and hooks stay.
*/
void
d_env_printer_reset(
    struct dj_env_printer* _printer
)
{
    if (_printer != NULL)
    {
        d_option_env_clear(&_printer->env);
        _printer->compiled = 0;
    }

    return;
}

/*
d_env_printer_release
  The frame detached; the program is stale with it.
*/
void
d_env_printer_release(
    struct dj_env_printer* _printer
)
{
    if (_printer != NULL)
    {
        d_option_env_detach(&_printer->env);
        _printer->compiled = 0;
    }

    return;
}

/*
d_env_printer_env
  The frame itself; a change made through it is seen at the next print by
the generation.
*/
struct dj_option_env*
d_env_printer_env(
    struct dj_env_printer* _printer
)
{
    return (_printer != NULL) ? &_printer->env : NULL;
}

/*
d_env_printer_var_id
  The first one's id and the variable's place among them, as the frame now
stands.
*/
uint32_t
d_env_printer_var_id(
    const struct dj_env_printer* _printer,
    int32_t                      _var
)
{
    if ( (_printer == NULL) ||
         (_var < 0)         ||
         (_var >= D_ENV_PRINTER_VARS) )
    {
        return D_OPTION_ENV_NONE;
    }

    const uint32_t base = d_internal_env_printer_base(_printer);

    return (base != D_OPTION_ENV_NONE) ? base + (uint32_t)_var
                                       : D_OPTION_ENV_NONE;
}

/*
d_env_printer_set_filter
  Stored as given.
*/
void
d_env_printer_set_filter(
    struct dj_env_printer* _printer,
    dj_env_filter_fn       _filter,
    void*                  _context
)
{
    if (_printer != NULL)
    {
        _printer->filter         = _filter;
        _printer->filter_context = _context;
    }

    return;
}

/*
d_env_printer_set_field
  Stored as given.
*/
void
d_env_printer_set_field(
    struct dj_env_printer* _printer,
    dj_env_field_fn        _field,
    void*                  _context
)
{
    if (_printer != NULL)
    {
        _printer->field         = _field;
        _printer->field_context = _context;
    }

    return;
}

/**
 * @brief Whether a name is taken for the printer's templates and statements:
 *        a field's, or a variable's the frame can see.
 *
 * @param[in] _printer  the printer.
 * @param[in] _name     the name, or null.
 * @return 1 or 0.
 */
static int32_t
d_internal_env_printer_taken(
    const struct dj_env_printer* _printer,
    const char*                  _name
)
{
    if (_name == NULL)
    {
        return 0;
    }

    for (uint32_t i = 0u; i < D_ENV_FIELDS; ++i)
    {
        if (strcmp(FIELD_NAMES[i], _name) == 0)
        {
            return 1;
        }
    }

    return d_option_env_find(&_printer->env, _name, strlen(_name)).is_ok ? 1
                                                                         : 0;
}

/*
d_env_printer_declare
  A field's name would hide the variable from every template, and an
ancestor's would be hidden by it from the printer, so either is refused as a
duplicate. The variable takes the field after the fixed ones; those a
template was given past them are given again at the next compilation.
*/
struct dj_option_result
d_env_printer_declare(
    struct dj_env_printer*          _printer,
    const struct dj_option_env_var* _var
)
{
    if ( (_printer == NULL) ||
         (_var == NULL)     ||
         (_var->name == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ( (d_internal_env_printer_taken(_printer, _var->name) != 0) ||
         (d_internal_env_printer_taken(_printer, _var->alias) != 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_DUPLICATE);
    }

    if ( (d_internal_env_printer_rebase(_printer) == 0) ||
         (_printer->var_fixed >= (uint32_t)D_CFG_ENV_PRINTER_VARIABLES) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CAPACITY);
    }

    const struct dj_option_result declared =
        d_option_env_declare(&_printer->env, _var);

    if (!declared.is_ok)
    {
        return declared;
    }

    _printer->vars[_printer->var_fixed] = declared.payload.value;
    _printer->var_count                 = ++_printer->var_fixed;
    _printer->compiled                  = 0;

    return d_option_ok(D_ENV_FIELDS + _printer->var_fixed - 1u);
}

/*
d_env_printer_set
  option_env's assign, then the program.
*/
struct dj_option_result
d_env_printer_set(
    struct dj_env_printer* _printer,
    const char*            _name,
    const char*            _value
)
{
    if ( (_printer == NULL) ||
         (_name == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_env_printer_after(
               _printer,
               d_option_env_assign(&_printer->env,
                                   _name,
                                   strlen(_name),
                                   _value,
                                   (_value != NULL) ? strlen(_value) : 0u));
}

/*
d_env_printer_put
  option_env's put, then the program.
*/
struct dj_option_result
d_env_printer_put(
    struct dj_env_printer* _printer,
    const char*            _statement
)
{
    if (_printer == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_env_printer_after(_printer,
                                        d_option_env_put(&_printer->env,
                                                         _statement));
}

/*
d_env_printer_load
  option_env's load, then the program.
*/
struct dj_option_result
d_env_printer_load(
    struct dj_env_printer* _printer,
    const char*            _script,
    size_t*                _error_offset
)
{
    if (_printer == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_env_printer_after(_printer,
                                        d_option_env_load(&_printer->env,
                                                          _script,
                                                          _error_offset));
}

/*
d_env_printer_import
  option_env's import, then the program.
*/
struct dj_option_result
d_env_printer_import(
    struct dj_env_printer* _printer,
    const char*            _prefix
)
{
    if (_printer == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_env_printer_after(_printer,
                                        d_option_env_import(&_printer->env,
                                                            _prefix));
}

/*
d_env_printer_set_template
  The new template is compiled alone first, into the printer's pool -- which
is recompiled whole before the next print anyway -- so one that does not
compile never replaces the one before it.
*/
struct dj_option_result
d_env_printer_set_template(
    struct dj_env_printer* _printer,
    int32_t                _part,
    const char*            _source
)
{
    if ( (_printer == NULL) ||
         (_part < 0)        ||
         (_part >= D_ENV_TEMPLATE_COUNT) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (d_internal_env_printer_rebase(_printer) == 0)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_ABSENT);
    }

    const uint32_t var = _printer->var_base +
                         (uint32_t)(D_ENV_PRINTER_REPORT_BEGIN + _part);

    // null: the style's template again
    if (_source == NULL)
    {
        return d_internal_env_printer_after(
                   _printer,
                   d_option_env_unset(&_printer->env,
                                      D_OPTION_ENV_GLOBAL,
                                      var));
    }

    struct dj_ostream_template       trial;
    struct dj_ostream_template_error error;
    const int32_t                    status = d_ostream_template_compile(
                                                 &trial,
                                                 _printer->ops,
                                                 D_CFG_ENV_PRINTER_TEMPLATE_OPS,
                                                 _source,
                                                 d_internal_env_printer_lookup,
                                                 _printer,
                                                 &error);

    _printer->compiled = 0;

    if (status != D_OSTREAM_TEMPLATE_OK)
    {
        _printer->error.offset = error.offset;
        _printer->error.part   = _part;
        _printer->error.status = status;

        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_env_printer_after(
               _printer,
               d_option_env_set_text_copy(&_printer->env,
                                          D_OPTION_ENV_GLOBAL,
                                          var,
                                          _source,
                                          strlen(_source)));
}

/*
d_env_printer_last_error
  The record of the last compilation.
*/
const struct dj_env_printer_error*
d_env_printer_last_error(
    const struct dj_env_printer* _printer
)
{
    return (_printer != NULL) ? &_printer->error : NULL;
}

/*
d_env_style_template
  The table, checked.
*/
const char*
d_env_style_template(
    int32_t _style,
    int32_t _part
)
{
    if ( (_style < 0)                  ||
         (_style >= D_ENV_STYLE_COUNT) ||
         (_part < 0)                   ||
         (_part >= D_ENV_TEMPLATE_COUNT) )
    {
        return NULL;
    }

    return PRESETS[_style][_part];
}


//==============================================================================
// WHAT SHOWS
//==============================================================================

/**
 * @brief A print in progress: the printer, the stream, the part being
 *        printed, and the variables in force for its scope.
 */
struct dj_internal_env_print
{
    struct dj_env_printer*  printer;
    struct dj_ostream*      os;
    struct dj_env_item      item;
    const char*             select;         // the scope's rules, or null
    int64_t                 features;       // the scope's feature filter
    uint32_t                loaded;         // the scope vars was read for
    size_t                  entry_width;    // the section's columns
    size_t                  feature_width;
    struct dj_ostream_value vars[D_CFG_ENV_PRINTER_VARIABLES];
    char                    path[D_INTERNAL_ENV_PRINTER_PATH];
};

/**
 * @brief Whether a glob matches a name: '*' any run, '?' any one character.
 *
 * @param[in] _glob  the glob.
 * @param[in] _size  its length.
 * @param[in] _name  the name, NUL-terminated.
 * @return 1 or 0.
 *
 * The usual linear matcher: on a mismatch after a '*', the '*' takes one more
 * character and matching resumes after it.
 */
static int
d_internal_env_printer_glob(
    const char* _glob,
    size_t      _size,
    const char* _name
)
{
    size_t g      = 0u;
    size_t n      = 0u;
    size_t star   = (size_t)-1;
    size_t marked = 0u;

    while (_name[n] != '\0')
    {
        if ( (g < _size) &&
             ( (_glob[g] == '?') ||
               (_glob[g] == _name[n]) ) )
        {
            ++g;
            ++n;
        }
        else if ( (g < _size) &&
                  (_glob[g] == '*') )
        {
            star   = g++;
            marked = n;
        }
        else if (star != (size_t)-1)
        {
            g = star + 1u;
            n = ++marked;
        }
        else
        {
            return 0;
        }
    }

    while ( (g < _size) &&
            (_glob[g] == '*') )
    {
        ++g;
    }

    return (g == _size) ? 1 : 0;
}

/**
 * @brief Whether `select`'s rules show a path: the last rule that matches
 *        decides; with none matching, a list that starts with an exclusion
 *        shows it and one that starts with an inclusion does not.
 *
 * @param[in] _rules  the rules, or null for none.
 * @param[in] _slug   the section's slug.
 * @param[in] _name   the item's name.
 * @return 1 or 0.
 *
 * A rule is [+|-]glob or [+|-]glob.glob, separated from the next by ',' or
 * space; a glob with no '.' is a section's and matches all its items.
 */
static int
d_internal_env_printer_selected(
    const char* _rules,
    const char* _slug,
    const char* _name
)
{
    int    shown = -1;
    size_t at    = 0u;

    if (_rules == NULL)
    {
        return 1;
    }

    while (_rules[at] != '\0')
    {
        // a rule: its sign, then its globs, up to a separator
        while ( (_rules[at] == ',') ||
                (_rules[at] == ' ') ||
                (_rules[at] == '\t') )
        {
            ++at;
        }

        if (_rules[at] == '\0')
        {
            break;
        }

        const int include = (_rules[at] != '-');

        if ( (_rules[at] == '-') ||
             (_rules[at] == '+') )
        {
            ++at;
        }

        const size_t start = at;
        size_t       dot   = (size_t)-1;

        while ( (_rules[at] != '\0') &&
                (_rules[at] != ',')  &&
                (_rules[at] != ' ')  &&
                (_rules[at] != '\t') )
        {
            if ( (_rules[at] == '.') &&
                 (dot == (size_t)-1) )
            {
                dot = at;
            }

            ++at;
        }

        if (shown < 0)
        {
            shown = (include != 0) ? 0 : 1;
        }

        const size_t section_size = (dot == (size_t)-1) ? (at - start)
                                                        : (dot - start);
        const int    matched =
            (d_internal_env_printer_glob(_rules + start,
                                         section_size,
                                         _slug) != 0) &&
            ( (dot == (size_t)-1) ||
              (d_internal_env_printer_glob(_rules + dot + 1u,
                                           at - dot - 1u,
                                           _name) != 0) );

        if ( (at > start) &&
             (matched != 0) )
        {
            shown = include;
        }
    }

    return (shown != 0) ? 1 : 0;
}

/**
 * @brief Whether an item shows: `select`, then `features` for a feature,
 *        then the filter.
 *
 * @param[in] _print    the print.
 * @param[in] _kind     the item's kind.
 * @param[in] _entry    an entry, or null.
 * @param[in] _feature  a feature, or null.
 * @return 1 or 0.
 */
static int
d_internal_env_printer_shows(
    const struct dj_internal_env_print* _print,
    int32_t                             _kind,
    const struct dj_env_entry*          _entry,
    const struct dj_env_feature*        _feature
)
{
    const char* const name = (_entry != NULL)   ? _entry->name
                           : (_feature != NULL) ? _feature->name
                           : "note";

    const char* const slug = (_print->item.section->slug != NULL)
                                 ? _print->item.section->slug
                                 : "";

    if (d_internal_env_printer_selected(_print->select,
                                        slug,
                                        (name != NULL) ? name : "") == 0)
    {
        return 0;
    }

    if ( (_feature != NULL) &&
         ( ( (_print->features == D_ENV_FEATURES_SUPPORTED) &&
             (_feature->value == 0L) ) ||
           ( (_print->features == D_ENV_FEATURES_UNSUPPORTED) &&
             (_feature->value != 0L) ) ) )
    {
        return 0;
    }

    if (_print->printer->filter != NULL)
    {
        struct dj_env_item item;

        memset(&item, 0, sizeof(item));
        item.report  = _print->item.report;
        item.section = _print->item.section;
        item.entry   = _entry;
        item.feature = _feature;
        item.scope   = _print->item.scope;
        item.kind    = _kind;

        return (_print->printer->filter(_print->printer->filter_context,
                                        &item) != 0) ? 1 : 0;
    }

    return 1;
}

/**
 * @brief The characters -- UTF-8 code points -- of a text.
 *
 * @param[in] _text  the text; null counts 0.
 * @return the count.
 */
static size_t
d_internal_env_printer_width(
    const char* _text
)
{
    size_t width = 0u;

    for (; ( (_text != NULL) && (*_text != '\0') ); ++_text)
    {
        if ((((unsigned char)*_text) & 0xC0u) != 0x80u)
        {
            ++width;
        }
    }

    return width;
}

/**
 * @brief Takes the variables in force for a scope.
 *
 * @param[in,out] _print  the print.
 * @param[in]     _scope  the scope.
 *
 * A scope no frame binds anything in reads exactly as the global one, so its
 * table is the global one's: the table is read again only when the scope it
 * holds differs, and most sections never cause a read.
 */
static void
d_internal_env_printer_load(
    struct dj_internal_env_print* _print,
    uint32_t                      _scope
)
{
    const struct dj_env_printer* const printer = _print->printer;
    const struct dj_option_env* const  env     = &printer->env;

    // a scope nothing is bound in reads as the global one: keep that
    const uint32_t scope = d_option_env_scope_is_bound(env, _scope)
                               ? _scope
                               : D_OPTION_ENV_GLOBAL;

    _print->item.scope = _scope;

    if (_print->loaded == scope)
    {
        return;
    }

    for (uint32_t i = 0u; i < printer->var_count; ++i)
    {
        struct dj_option_env_value value;
        struct dj_ostream_value*   slot = &_print->vars[i];

        (void)d_option_env_get(env, scope, printer->vars[i], &value);

        switch (value.kind)
        {
            case D_OPTION_ENV_KIND_BOOL:
                *slot = d_ostream_value_bool(value.integer != 0);
                break;
            case D_OPTION_ENV_KIND_REAL:
                *slot = d_ostream_value_real(value.real);
                break;
            case D_OPTION_ENV_KIND_TEXT:
                *slot = d_ostream_value_text(value.text);
                break;
            case D_OPTION_ENV_KIND_CHOICE:
                *slot = (value.text != NULL)
                            ? d_ostream_value_text(value.text)
                            : d_ostream_value_int64(value.integer);
                break;
            default:
                *slot = d_ostream_value_int64(value.integer);
                break;
        }
    }

    _print->select   = d_option_env_text(env,
                                         scope,
                                         printer->var_base +
                                             D_ENV_PRINTER_SELECT);
    _print->features = d_option_env_int(env,
                                        scope,
                                        printer->var_base +
                                            D_ENV_PRINTER_FEATURES);
    _print->loaded   = scope;

    return;
}

/**
 * @brief The scope a section's variables are bound in: its slug's, or the
 *        global one for a section the printer does not name.
 *
 * @param[in] _print    the print.
 * @param[in] _section  the section.
 * @return the scope.
 */
static uint32_t
d_internal_env_printer_scope(
    const struct dj_internal_env_print* _print,
    const struct dj_env_section*        _section
)
{
    const struct dj_option_result found =
        (_section->slug != NULL)
            ? d_option_env_find_scope(&_print->printer->env,
                                      _section->slug,
                                      strlen(_section->slug))
            : d_option_fail((int32_t)D_OPTION_STATUS_KEY_ABSENT);

    return found.is_ok ? found.payload.value : D_OPTION_ENV_GLOBAL;
}

/**
 * @brief Counts a section's shown items under the variables in force, and
 *        measures its two columns.
 *
 * @param[in,out] _print    the print; its section set, its widths written.
 * @return the count.
 */
static size_t
d_internal_env_printer_count(
    struct dj_internal_env_print* _print
)
{
    const struct dj_env_section* const section = _print->item.section;
    size_t                             count   = 0u;
    size_t                             labels  = 0u;
    size_t                             names   = 0u;

    for (size_t i = 0u; i < section->entry_count; ++i)
    {
        const struct dj_env_entry* const entry = &section->entries[i];

        if (d_internal_env_printer_shows(_print, D_ENV_ITEM_ENTRY, entry,
                                         NULL) != 0)
        {
            const size_t width = d_internal_env_printer_width(entry->label);

            labels = (width > labels) ? width : labels;
            ++count;
        }
    }

    for (size_t i = 0u; i < section->feature_count; ++i)
    {
        const struct dj_env_feature* const feature = &section->features[i];

        if (d_internal_env_printer_shows(_print, D_ENV_ITEM_FEATURE, NULL,
                                         feature) != 0)
        {
            const size_t width = d_internal_env_printer_width(feature->name);

            names = (width > names) ? width : names;
            ++count;
        }
    }

    // a note stands in for a section with nothing else
    if ( (section->entry_count == 0u)   &&
         (section->feature_count == 0u) &&
         (section->note != NULL)        &&
         (d_internal_env_printer_shows(_print, D_ENV_ITEM_NOTE, NULL,
                                       NULL) != 0) )
    {
        ++count;
    }

    const long key_width  = d_ostream_value_to_long(
                                &_print->vars[D_ENV_PRINTER_KEY_WIDTH]);
    const long name_width = d_ostream_value_to_long(
                                &_print->vars[D_ENV_PRINTER_NAME_WIDTH]);

    _print->entry_width   = (key_width > 0L) ? (size_t)key_width : labels;
    _print->feature_width = (name_width > 0L) ? (size_t)name_width : names;

    return count;
}


//==============================================================================
// RUNNING THE PROGRAM
//==============================================================================

/**
 * @brief An entry's value as a template value: text, integer or boolean.
 *
 * @param[in] _entry  the entry.
 * @return the value.
 */
static struct dj_ostream_value
d_internal_env_printer_entry_value(
    const struct dj_env_entry* _entry
)
{
    switch (_entry->kind)
    {
        case D_ENV_VALUE_INTEGER:
            return d_ostream_value_long(_entry->integer);
        case D_ENV_VALUE_BOOLEAN:
            return d_ostream_value_bool(_entry->integer != 0L);
        default:
            return d_ostream_value_text(_entry->text);
    }
}

/**
 * @brief A feature's standard without the parentheses env_cpp_features.h
 *        spells it with: "(C++11)" as "C++11".
 *
 * @param[in] _standard  the standard, or null.
 * @return the value.
 */
static struct dj_ostream_value
d_internal_env_printer_standard(
    const char* _standard
)
{
    const size_t length = (_standard != NULL) ? strlen(_standard) : 0u;

    if ( (length >= 2u) &&
         (_standard[0] == '(') &&
         (_standard[length - 1u] == ')') )
    {
        return d_ostream_value_text_n(_standard + 1, length - 2u);
    }

    return d_ostream_value_text(_standard);
}

/**
 * @brief The item's name, as {name} gives it.
 *
 * @param[in] _item  the item.
 * @return the name, or null.
 */
static const char*
d_internal_env_printer_name(
    const struct dj_env_item* _item
)
{
    switch (_item->kind)
    {
        case D_ENV_ITEM_ENTRY:
            return _item->entry->name;
        case D_ENV_ITEM_FEATURE:
            return _item->feature->name;
        case D_ENV_ITEM_NOTE:
            return "note";
        default:
            return NULL;
    }
}

/**
 * @brief The fields about where an item is: its section, its place.
 *
 * @param[in,out] _print  the print; its path buffer used.
 * @param[in]     _field  the field.
 * @param[out]    _value  the value.
 * @return 1 when the field is one of these, 0 when it is not.
 */
static int32_t
d_internal_env_printer_place(
    struct dj_internal_env_print* _print,
    uint32_t                      _field,
    struct dj_ostream_value*      _value
)
{
    const struct dj_env_item* const    item    = &_print->item;
    const struct dj_env_section* const section = item->section;
    const int32_t                      is_item =
        (item->kind >= D_ENV_ITEM_ENTRY) ? 1 : 0;

    switch (_field)
    {
        case D_ENV_FIELD_SECTION:
            *_value = (section != NULL) ? d_ostream_value_text(section->slug)
                                        : d_ostream_value_none();
            return 1;
        case D_ENV_FIELD_SECTION_TITLE:
            *_value = (section != NULL) ? d_ostream_value_text(section->title)
                                        : d_ostream_value_none();
            return 1;
        case D_ENV_FIELD_SECTION_INDEX:
            *_value = d_ostream_value_size(item->section_index);
            return 1;
        case D_ENV_FIELD_SECTION_COUNT:
            *_value = d_ostream_value_size(item->section_count);
            return 1;
        case D_ENV_FIELD_COUNT:
            *_value = d_ostream_value_size(item->count);
            return 1;
        case D_ENV_FIELD_HAS_FEATURES:
            *_value = d_ostream_value_bool( (section != NULL) &&
                                            (section->features != NULL) );
            return 1;
        case D_ENV_FIELD_INDEX:
            *_value = (is_item != 0) ? d_ostream_value_size(item->index)
                                     : d_ostream_value_none();
            return 1;
        case D_ENV_FIELD_FIRST:
            *_value = d_ostream_value_bool( (is_item != 0) &&
                                            (item->index == 0u) );
            return 1;
        case D_ENV_FIELD_LAST:
            *_value = d_ostream_value_bool( (is_item != 0) &&
                                            (item->index + 1u == item->count) );
            return 1;
        case D_ENV_FIELD_WIDTH:
            *_value = d_ostream_value_size(item->width);
            return 1;
        case D_ENV_FIELD_PATH:
        {
            const char* const name = d_internal_env_printer_name(item);
            const size_t      slug = (section != NULL)
                                         ? strlen(section->slug)
                                         : 0u;
            const size_t      size = (name != NULL) ? strlen(name) : 0u;

            // "slug.name", cut to the buffer; a section's path is its slug
            if ( (section == NULL) ||
                 (slug + size + 1u >= sizeof(_print->path)) )
            {
                *_value = (section != NULL)
                              ? d_ostream_value_text(section->slug)
                              : d_ostream_value_none();
                return 1;
            }

            memcpy(_print->path, section->slug, slug);
            _print->path[slug] = '.';
            memcpy(_print->path + slug + 1u, name, size);
            *_value = (name != NULL)
                          ? d_ostream_value_text_n(_print->path,
                                                   slug + 1u + size)
                          : d_ostream_value_text(section->slug);
            return 1;
        }
        default:
            return 0;
    }
}

/**
 * @brief The fields about what an item is: its name, value and the rest.
 *
 * @param[in]  _print  the print.
 * @param[in]  _field  the field.
 * @param[out] _value  the value; left NONE for a field the item lacks.
 */
static void
d_internal_env_printer_what(
    const struct dj_internal_env_print* _print,
    uint32_t                            _field,
    struct dj_ostream_value*            _value
)
{
    const struct dj_env_item* const    item    = &_print->item;
    const struct dj_env_entry* const   entry   = item->entry;
    const struct dj_env_feature* const feature = item->feature;

    *_value = d_ostream_value_none();

    switch (_field)
    {
        case D_ENV_FIELD_NAME:
            *_value = d_ostream_value_text(d_internal_env_printer_name(item));
            break;
        case D_ENV_FIELD_LABEL:
            *_value = (entry != NULL)   ? d_ostream_value_text(entry->label)
                    : (feature != NULL) ? d_ostream_value_text(feature->name)
                    : d_ostream_value_none();
            break;
        case D_ENV_FIELD_VALUE:
        case D_ENV_FIELD_DISPLAY:
            if (entry != NULL)
            {
                *_value = d_internal_env_printer_entry_value(entry);

                // display: a boolean as the words the variables give
                if ( (_field == D_ENV_FIELD_DISPLAY) &&
                     (entry->kind == D_ENV_VALUE_BOOLEAN) )
                {
                    *_value = _print->vars[(entry->integer != 0L)
                                               ? D_ENV_PRINTER_TRUE_TEXT
                                               : D_ENV_PRINTER_FALSE_TEXT];
                }
            }
            else if (feature != NULL)
            {
                *_value = d_ostream_value_long(feature->value);
            }
            else if (item->kind == D_ENV_ITEM_NOTE)
            {
                *_value = d_ostream_value_text(item->section->note);
            }

            break;
        case D_ENV_FIELD_KIND:
            *_value = d_ostream_value_text(
                          ( (entry != NULL) &&
                            (entry->kind >= D_ENV_VALUE_TEXT) &&
                            (entry->kind <= D_ENV_VALUE_BOOLEAN) )
                              ? VALUE_KIND_NAMES[entry->kind]
                              : KIND_NAMES[item->kind]);
            break;
        case D_ENV_FIELD_STANDARD:
            if (feature != NULL)
            {
                *_value = d_internal_env_printer_standard(feature->standard);
            }

            break;
        case D_ENV_FIELD_DESCRIPTION:
            if (feature != NULL)
            {
                *_value = d_ostream_value_text(feature->description);
            }

            break;
        case D_ENV_FIELD_SUPPORTED:
            if (feature != NULL)
            {
                *_value = d_ostream_value_bool(feature->value != 0L);
            }

            break;
        case D_ENV_FIELD_NOTE:
            if (item->section != NULL)
            {
                *_value = d_ostream_value_text(item->section->note);
            }

            break;
        default:
            break;
    }

    return;
}

/**
 * @brief The program's resolver: the field hook first, then a variable, a
 *        place field or a what field.
 *
 * @param[in]  _context  the print.
 * @param[in]  _field    the field.
 * @param[out] _value    the value.
 * @return 1, or 0 when the hook stopped the print.
 */
static int32_t
d_internal_env_printer_resolve(
    void*                    _context,
    uint32_t                 _field,
    struct dj_ostream_value* _value
)
{
    struct dj_internal_env_print* const print =
        (struct dj_internal_env_print*)_context;

    if (print->printer->field != NULL)
    {
        const int32_t answer = print->printer->field(
                                   print->printer->field_context,
                                   &print->item,
                                   _field,
                                   _value);

        if (answer != 0)
        {
            return (answer > 0) ? 1 : 0;
        }
    }

    if (_field >= D_ENV_FIELDS)
    {
        *_value = (_field - D_ENV_FIELDS < print->printer->var_count)
                      ? print->vars[_field - D_ENV_FIELDS]
                      : d_ostream_value_none();

        return 1;
    }

    if (d_internal_env_printer_place(print, _field, _value) == 0)
    {
        d_internal_env_printer_what(print, _field, _value);
    }

    return 1;
}

/**
 * @brief Runs one part's template for the item in hand.
 *
 * @param[in,out] _print  the print.
 * @param[in]     _part   the part.
 * @return 1 while the stream is good.
 */
static int32_t
d_internal_env_printer_render(
    struct dj_internal_env_print* _print,
    int32_t                       _part
)
{
    return d_ostream_template_render(_print->os,
                                     &_print->printer->templates[_part],
                                     d_internal_env_printer_resolve,
                                     _print);
}

/**
 * @brief Prints one item, its separator first when it is not the first.
 *
 * @param[in,out] _print    the print; its item set to this one.
 * @param[in]     _kind     the item's kind.
 * @param[in]     _entry    an entry, or null.
 * @param[in]     _feature  a feature, or null.
 * @param[in,out] _index    the items shown so far; counted.
 */
static void
d_internal_env_printer_item(
    struct dj_internal_env_print* _print,
    int32_t                       _kind,
    const struct dj_env_entry*    _entry,
    const struct dj_env_feature*  _feature,
    size_t*                       _index
)
{
    // the separator sees the item it comes before
    _print->item.kind    = _kind;
    _print->item.entry   = _entry;
    _print->item.feature = _feature;
    _print->item.index   = *_index;
    _print->item.width   = (_kind == D_ENV_ITEM_FEATURE)
                               ? _print->feature_width
                               : _print->entry_width;

    if (*_index > 0u)
    {
        (void)d_internal_env_printer_render(_print,
                                            D_ENV_TEMPLATE_ITEM_SEPARATOR);
    }

    (void)d_internal_env_printer_render(
              _print,
              (_kind == D_ENV_ITEM_ENTRY)   ? D_ENV_TEMPLATE_ENTRY
            : (_kind == D_ENV_ITEM_FEATURE) ? D_ENV_TEMPLATE_FEATURE
            : D_ENV_TEMPLATE_NOTE);
    ++*_index;

    return;
}

/**
 * @brief Prints a section's items: its entries, its features, or its note.
 *
 * @param[in,out] _print  the print, at the section.
 */
static void
d_internal_env_printer_items(
    struct dj_internal_env_print* _print
)
{
    const struct dj_env_section* const section = _print->item.section;
    size_t                             index   = 0u;

    for (size_t i = 0u; i < section->entry_count; ++i)
    {
        if (d_internal_env_printer_shows(_print, D_ENV_ITEM_ENTRY,
                                         &section->entries[i], NULL) != 0)
        {
            d_internal_env_printer_item(_print, D_ENV_ITEM_ENTRY,
                                        &section->entries[i], NULL, &index);
        }
    }

    for (size_t i = 0u; i < section->feature_count; ++i)
    {
        if (d_internal_env_printer_shows(_print, D_ENV_ITEM_FEATURE, NULL,
                                         &section->features[i]) != 0)
        {
            d_internal_env_printer_item(_print, D_ENV_ITEM_FEATURE, NULL,
                                        &section->features[i], &index);
        }
    }

    if ( (section->entry_count == 0u)   &&
         (section->feature_count == 0u) &&
         (section->note != NULL)        &&
         (d_internal_env_printer_shows(_print, D_ENV_ITEM_NOTE, NULL,
                                       NULL) != 0) )
    {
        d_internal_env_printer_item(_print, D_ENV_ITEM_NOTE, NULL, NULL,
                                    &index);
    }

    return;
}

/**
 * @brief Points the print at a section, with its scope's variables, and
 *        counts what of it shows.
 *
 * @param[in,out] _print    the print.
 * @param[in]     _section  the section.
 * @param[in]     _mask     `sections`.
 * @return the items it shows; 0 for a section `sections` leaves out.
 */
static size_t
d_internal_env_printer_enter(
    struct dj_internal_env_print* _print,
    const struct dj_env_section*  _section,
    int64_t                       _mask
)
{
    // a section the printer has no bit for shows unless every bit is off
    if ( ( (_section->id != 0) &&
           ((_mask & (int64_t)_section->id) == 0) ) ||
         ( (_section->id == 0) &&
           (_mask == 0) ) )
    {
        return 0u;
    }

    _print->item.section = _section;
    _print->item.entry   = NULL;
    _print->item.feature = NULL;
    _print->item.kind    = D_ENV_ITEM_SECTION;
    _print->item.index   = 0u;
    _print->item.width   = 0u;
    d_internal_env_printer_load(_print,
                                d_internal_env_printer_scope(_print, _section));

    return d_internal_env_printer_count(_print);
}

/**
 * @brief Points the print at the report as a whole, in the global scope.
 *
 * @param[in,out] _print  the print.
 */
static void
d_internal_env_printer_whole(
    struct dj_internal_env_print* _print
)
{
    _print->item.section = NULL;
    _print->item.entry   = NULL;
    _print->item.feature = NULL;
    _print->item.kind    = D_ENV_ITEM_REPORT;
    _print->item.index   = 0u;
    _print->item.count   = 0u;
    _print->item.width   = 0u;
    d_internal_env_printer_load(_print, D_OPTION_ENV_GLOBAL);

    return;
}

/*
d_env_printer_print
  The three walks: the sections that show, counted first; then each, counted
and measured, then printed. The print context is a stack value of a few KB
-- the variables in force and a path buffer.
*/
int32_t
d_env_printer_print(
    struct dj_env_printer*      _printer,
    struct dj_ostream*          _os,
    const struct dj_env_report* _report
)
{
    struct dj_internal_env_print print;
    size_t                       shown = 0u;

    if (_os == NULL)
    {
        return 0;
    }

    if ( (_printer == NULL) ||
         (_report == NULL)  ||
         ( (_report->count != 0u) &&
           (_report->sections == NULL) ) ||
         (d_internal_env_printer_compile(_printer) == 0) )
    {
        return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
    }

    memset(&print.item, 0, sizeof(print.item));
    print.loaded      = D_OPTION_ENV_NONE;
    print.printer     = _printer;
    print.os          = _os;
    print.item.report = _report;

    const int64_t mask = d_option_env_int(&_printer->env,
                                          D_OPTION_ENV_GLOBAL,
                                          _printer->var_base +
                                              D_ENV_PRINTER_SECTIONS);

    for (size_t i = 0u; i < _report->count; ++i)
    {
        shown += (d_internal_env_printer_enter(&print,
                                               &_report->sections[i],
                                               mask) != 0u) ? 1u : 0u;
    }

    print.item.section_count = shown;
    d_internal_env_printer_whole(&print);
    (void)d_internal_env_printer_render(&print, D_ENV_TEMPLATE_REPORT_BEGIN);

    for (size_t i = 0u, index = 0u;
         ( (i < _report->count) &&
           (d_ostream_status(_os) == D_OSTREAM_OK) );
         ++i)
    {
        const size_t count = d_internal_env_printer_enter(
                                 &print,
                                 &_report->sections[i],
                                 mask);

        if (count == 0u)
        {
            continue;
        }

        print.item.count         = count;
        print.item.section_index = index;

        if (index > 0u)
        {
            (void)d_internal_env_printer_render(
                      &print,
                      D_ENV_TEMPLATE_SECTION_SEPARATOR);
        }

        (void)d_internal_env_printer_render(&print,
                                            D_ENV_TEMPLATE_SECTION_BEGIN);
        d_internal_env_printer_items(&print);

        // the section's end sees the section, not its last item
        print.item.kind    = D_ENV_ITEM_SECTION;
        print.item.entry   = NULL;
        print.item.feature = NULL;
        print.item.index   = 0u;
        print.item.width   = 0u;
        (void)d_internal_env_printer_render(&print,
                                            D_ENV_TEMPLATE_SECTION_END);
        ++index;
    }

    if (d_ostream_status(_os) == D_OSTREAM_OK)
    {
        d_internal_env_printer_whole(&print);
        print.item.section_count = shown;
        (void)d_internal_env_printer_render(&print,
                                            D_ENV_TEMPLATE_REPORT_END);
    }

    return d_ostream_is_ok(_os);
}


//==============================================================================
// DUMPING THE VARIABLES
//==============================================================================

/**
 * @brief Writes text as a quoted value a script reads back: a backslash
 *        before a quote, a backslash, a line feed (n), a return (r) and a
 *        tab (t), and x and two hexadecimal digits for any other control.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _text  the text.
 */
static void
d_internal_env_printer_quote(
    struct dj_ostream* _os,
    const char*        _text
)
{
    static const char HEX[] = "0123456789abcdef";

    (void)d_ostream_write_char(_os, '"');

    for (; *_text != '\0'; ++_text)
    {
        const unsigned char c = (unsigned char)*_text;

        switch (c)
        {
            case '"':  (void)d_ostream_write(_os, "\\\"", 2u); break;
            case '\\': (void)d_ostream_write(_os, "\\\\", 2u); break;
            case '\n': (void)d_ostream_write(_os, "\\n", 2u);  break;
            case '\r': (void)d_ostream_write(_os, "\\r", 2u);  break;
            case '\t': (void)d_ostream_write(_os, "\\t", 2u);  break;
            default:
                if (c < 0x20u)
                {
                    char escape[4];

                    escape[0] = '\\';
                    escape[1] = 'x';
                    escape[2] = HEX[c >> 4];
                    escape[3] = HEX[c & 0x0Fu];
                    (void)d_ostream_write(_os, escape, 4u);
                }
                else
                {
                    (void)d_ostream_write_char(_os, (char)c);
                }

                break;
        }
    }

    (void)d_ostream_write_char(_os, '"');

    return;
}

/**
 * @brief Writes one "name = value" line: text quoted, a null text as an
 *        unset, the rest as option_env writes them.
 *
 * @param[in]     _printer  the printer.
 * @param[in,out] _os       the stream.
 * @param[in]     _scope    the scope; its slug and a '.' lead the name.
 * @param[in]     _var      the variable.
 * @param[in]     _comment  1 to write it commented out.
 */
static void
d_internal_env_printer_line(
    const struct dj_env_printer* _printer,
    struct dj_ostream*           _os,
    uint32_t                     _scope,
    uint32_t                     _var,
    int                          _comment
)
{
    const struct dj_option_env_var* const decl = d_option_env_var_at(
                                                    &_printer->env,
                                                    _var);
    char                                  line[D_INTERNAL_ENV_PRINTER_LINE];

    if (_comment != 0)
    {
        (void)d_ostream_write(_os, "# ", 2u);
    }

    if ( (decl->kind == D_OPTION_ENV_KIND_TEXT) &&
         (d_option_env_text(&_printer->env, _scope, _var) == NULL) )
    {
        (void)d_ostream_write_char(_os, '-');
    }

    if (_scope != D_OPTION_ENV_GLOBAL)
    {
        (void)d_ostream_write_cstr(_os,
                                   d_option_env_scope_name(&_printer->env,
                                                           _scope));
        (void)d_ostream_write_char(_os, '.');
    }

    (void)d_ostream_write_cstr(_os, decl->name);

    if (decl->kind == D_OPTION_ENV_KIND_TEXT)
    {
        const char* const text = d_option_env_text(&_printer->env,
                                                   _scope,
                                                   _var);

        // null text has no value to write: the line unsets it
        if (text != NULL)
        {
            (void)d_ostream_write(_os, " = ", 3u);
            d_internal_env_printer_quote(_os, text);
        }
    }
    else
    {
        const size_t length = d_option_env_format(&_printer->env,
                                                  _scope,
                                                  _var,
                                                  line,
                                                  sizeof(line));

        (void)d_ostream_write(_os, " = ", 3u);
        (void)d_ostream_write(_os,
                              line,
                              (length < sizeof(line)) ? length
                                                      : sizeof(line) - 1u);
    }

    (void)d_ostream_write_char(_os, '\n');

    return;
}

/**
 * @brief Whether a frame of the chain binds a variable in a scope itself,
 *        not through the global scope.
 *
 * @param[in] _env    the printer's frame.
 * @param[in] _scope  the scope.
 * @param[in] _var    the variable.
 * @return 1 or 0.
 */
static int
d_internal_env_printer_bound(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var
)
{
    for (const struct dj_option_env* frame = _env;
         frame != NULL;
         frame = frame->parent)
    {
        if (d_option_env_is_bound(frame, _scope, _var))
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Writes one variable's help and lines.
 *
 * @param[in]     _printer   the printer.
 * @param[in,out] _os        the stream.
 * @param[in]     _var       the variable.
 * @param[in]     _defaults  non-zero to write it unbound, commented out.
 */
static void
d_internal_env_printer_dump_var(
    const struct dj_env_printer* _printer,
    struct dj_ostream*           _os,
    uint32_t                     _var,
    int32_t                      _defaults
)
{
    const struct dj_option_env* const     env  = &_printer->env;
    const struct dj_option_env_var* const decl = d_option_env_var_at(env,
                                                                     _var);
    const int                             bound =
        d_internal_env_printer_bound(env, D_OPTION_ENV_GLOBAL, _var);

    if ( (_defaults != 0) ||
         (bound != 0) )
    {
        (void)d_ostream_write(_os, "# ", 2u);
        (void)d_ostream_write_cstr(_os, decl->name);
        (void)d_ostream_write(_os, " (", 2u);
        (void)d_ostream_write_cstr(_os, d_option_env_kind_name(decl->kind));

        // a choice's and flags' names
        for (uint32_t n = 0u; n < decl->name_count; ++n)
        {
            (void)d_ostream_write(_os, (n == 0u) ? ": " : ", ", 2u);
            (void)d_ostream_write_cstr(_os, decl->names[n].name);
        }

        (void)d_ostream_write(_os, ")", 1u);

        if (decl->help != NULL)
        {
            (void)d_ostream_write(_os, ": ", 2u);
            (void)d_ostream_write_cstr(_os, decl->help);
        }

        (void)d_ostream_write_char(_os, '\n');
        d_internal_env_printer_line(_printer,
                                    _os,
                                    D_OPTION_ENV_GLOBAL,
                                    _var,
                                    (bound == 0) ? 1 : 0);
    }

    // the sections' own bindings
    for (uint32_t i = 0u; i < D_INTERNAL_ENV_PRINTER_SCOPES; ++i)
    {
        const struct dj_option_result scope = d_option_env_find_scope(
                                                  env,
                                                  SCOPES[i],
                                                  strlen(SCOPES[i]));

        if ( (scope.is_ok) &&
             (d_internal_env_printer_bound(env,
                                           scope.payload.value,
                                           _var) != 0) )
        {
            d_internal_env_printer_line(_printer,
                                        _os,
                                        scope.payload.value,
                                        _var,
                                        0);
        }
    }

    return;
}

/*
d_env_printer_dump
  Variable by variable, from the printer's first on, as the frame sees them:
its help, its global line, then a line for each section that binds it
itself. A choice's and flags' names are listed after the help, so the dump
documents what it may say. A variable a frame above the printer's own
declares -- the registry root's -- is not the printer's to write.
*/
int32_t
d_env_printer_dump(
    const struct dj_env_printer* _printer,
    struct dj_ostream*           _os,
    int32_t                      _defaults
)
{
    if (_os == NULL)
    {
        return 0;
    }

    if (_printer == NULL)
    {
        return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
    }

    const struct dj_option_env* const env   = &_printer->env;
    const uint32_t                    base  = d_internal_env_printer_base(
                                                  _printer);
    const uint32_t                    first = d_option_env_var_index(env,
                                                                     base);

    if (first == D_OPTION_ENV_NONE)
    {
        return d_ostream_fail(_os, D_OSTREAM_INVALID, EINVAL);
    }

    for (uint32_t k = first; k < d_option_env_var_count(env); ++k)
    {
        d_internal_env_printer_dump_var(_printer,
                                        _os,
                                        d_option_env_var_id(env, k),
                                        _defaults);
    }

    return d_ostream_is_ok(_os);
}


#endif  // RE_STD_HAS_INT64
