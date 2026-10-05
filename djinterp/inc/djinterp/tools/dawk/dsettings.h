/*******************************************************************************
* djinterp [djinterp]                                                dsettings.h
*
* Settings registry:
*   Every runtime flag and setting dawk has, in one place: its name, its type,
* its default, the values it accepts, and where its current value came from.
* A component registers the settings it reads; nothing reads a setting it did
* not register, and nothing registers a setting it does not read, because a
* setting that changes nothing is a silent lie about what the tool will do.
*   Values arrive from four sources, and a value only yields to one of higher
* standing: default, then sheet, then environment, then command line.  The
* order sources are applied in therefore cannot change the outcome.
*   The model is djinterp's c/container/registry: rows keyed by their first
* member, aliases, and a sorted lookup searched by bisection.  That container
* sits behind the env include-graph skew, so this is a local implementation
* shaped to be swapped onto it once the skew is resolved.  C11 only, no
* dependencies, so the conforming core may use it.
*
*
* path:      /inc/djinterp/tools/dawk/dsettings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DSETTINGS_H
#define DJINTERP_TOOLS_DAWK_DSETTINGS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // FILE


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// d_setting_kind
//   enum: the type of a setting's value.
enum d_setting_kind
{
    D_SETTING_BOOL = 0,   // true / false, and the usual spellings of each
    D_SETTING_INT,        // an integer within [minimum, maximum]
    D_SETTING_ENUM        // one word from `choices`
};

// 1.1.2
// d_setting_source
//   enum: where a value came from, in ascending standing.
enum d_setting_source
{
    D_SETTING_DEFAULT = 0,
    D_SETTING_SHEET,
    D_SETTING_ENVIRONMENT,
    D_SETTING_COMMAND_LINE
};

// 1.2    Records
//------------------------------------------------------------------------------
// 1.2.1
// d_setting_def
//   struct: one setting's definition.  The name is the first member, as the
// djinterp registry requires of its rows.  `unbuilt` lists values that are
// ruled but not yet implemented; they are refused by name rather than
// accepted and ignored.
struct d_setting_def
{
    const char*          name;
    enum d_setting_kind  kind;
    const char*          fallback;   // the default, written as its text
    const char* const*   choices;    // ENUM: NULL-terminated
    const char* const*   unbuilt;    // ENUM: NULL-terminated, or NULL
    long                 minimum;    // INT
    long                 maximum;    // INT
    const char*          alias;      // an older name, or NULL
    const char*          summary;    // one line, for listings
};

// 1.2.2
// d_settings
//   struct: the registry.
struct d_settings;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle and registration
//------------------------------------------------------------------------------
struct d_settings*   d_settings_new(void);
void                 d_settings_free(struct d_settings* _settings);
bool                 d_settings_register(struct d_settings*          _settings,
                                         const struct d_setting_def* _defs,
                                         size_t                      _count);

// 2.2    Assignment
//------------------------------------------------------------------------------
bool                 d_settings_set(struct d_settings*    _settings,
                                    const char*           _name,
                                    const char*           _value,
                                    enum d_setting_source _source,
                                    char*                 _error,
                                    size_t                _error_size);
bool                 d_settings_apply(struct d_settings*    _settings,
                                      const char*           _list,
                                      enum d_setting_source _source,
                                      char*                 _error,
                                      size_t                _error_size);

// 2.3    Reading
//------------------------------------------------------------------------------
const char*          d_settings_text(const struct d_settings* _settings,
                                     const char*              _name);
long                 d_settings_int(const struct d_settings* _settings,
                                    const char*              _name);
bool                 d_settings_bool(const struct d_settings* _settings,
                                     const char*              _name);
bool                 d_settings_is(const struct d_settings* _settings,
                                   const char*              _name,
                                   const char*              _word);
enum d_setting_source
                     d_settings_source(const struct d_settings* _settings,
                                       const char*              _name);

// 2.4    Listing
//------------------------------------------------------------------------------
size_t               d_settings_count(const struct d_settings* _settings);
const struct d_setting_def*
                     d_settings_def_at(const struct d_settings* _settings,
                                       size_t                   _at);
const char*          d_settings_value_at(const struct d_settings* _settings,
                                         size_t                   _at);
enum d_setting_source
                     d_settings_source_at(const struct d_settings* _settings,
                                          size_t                   _at);
const char*          d_settings_source_name(enum d_setting_source _source);

// 2.5    Command line
//------------------------------------------------------------------------------
int                  d_settings_take_option(struct d_settings* _settings,
                                            int                _argc,
                                            char**             _argv,
                                            int*               _at,
                                            char*              _error,
                                            size_t             _error_size);
void                 d_settings_describe(const struct d_settings* _settings,
                                         FILE*                    _out);


#endif  // DJINTERP_TOOLS_DAWK_DSETTINGS_H
