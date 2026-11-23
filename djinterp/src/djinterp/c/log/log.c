/*******************************************************************************
* djinterp [c][log]                                                        log.c
*
*   The log's levels, codes, settings, records and destinations. The
* writers are log_writers.c's. The threshold and each grade's level are
* cached per module, and read again only when the djin's generation moves,
* so the level check is a comparison; settings are written before the
* program starts its threads (knob_and_option.md, point 4).
*
*
* path:      /src/djinterp/c/log/log.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.19
*******************************************************************************/

#include "../../../../inc/djinterp/c/log/log.h"  // corresponding header

#if RE_STD_HAS_INT64

// std
#include <stdio.h>   // stderr, snprintf
#include <stdlib.h>  // strtod
#include <string.h>  // memcmp, strlen
#include <time.h>    // clock_gettime, timespec_get, time
// djinterp
#include "../../../../inc/djinterp/c/event/event_common.h"      // keys
#include "../../../../inc/djinterp/c/util/ostream/ostream_console.h"  // stderr
#include "../../../../inc/djinterp/c/util/ostream/ostream_file.h"  // files
#if D_CFG_LOG_THREADSAFE
    #include "../../../../inc/djinterp/c/sync/dmutex.h"  // the lock
#endif

#if !D_DESC_MACHINE_BUILT(D_DESC_FORMAT_COLLECT)
    #error "the log collects facts with desc's collecting machine: build it"
#endif

// D_INTERNAL_LOG_SCOPE_NAME
//   constant: the room for a module's scope name: its name, '.' as '_'.
#define D_INTERNAL_LOG_SCOPE_NAME  40u

// D_INTERNAL_LOG_OPTIONS
//   constant: 1 when the log has its djin.
#define D_INTERNAL_LOG_OPTIONS  (D_CFG_LOG_OPTIONS)


//==============================================================================
// THE STATE
//==============================================================================

// dj_internal_log_cache
//   struct: what the level check reads, per scope: the threshold, and each
// grade's level.
struct dj_internal_log_cache
{
    uint8_t threshold;
    uint8_t grades[D_LOG_GRADE_COUNT];
};

// dj_internal_log
//   struct: the log's state, one per program.
struct dj_internal_log
{
    const struct dj_log_module*  modules[D_CFG_LOG_MODULES_MAX];
    size_t                       module_count;
    char                         scope_names[D_CFG_LOG_SCOPES]
                                            [D_INTERNAL_LOG_SCOPE_NAME];
    const char*                  scopes[D_CFG_LOG_SCOPES];
    uint64_t                     generation;  // the djin's, cached at; +1
    struct dj_internal_log_cache cache[D_CFG_LOG_SCOPES + 1];
    struct dj_log_level          levels[D_CFG_LOG_LEVELS_MAX];
    size_t                       level_count;
    struct dj_ostream*           program_os;
    struct dj_ostream            own_os;
    struct dj_ostream_console    console;
    int                          own_open;    // 1 when own_os is made
    int                          own_file;    // 1 when it is a file
    int                          none;        // 1 for log.destination=none
    unsigned char                buffer[D_CFG_LOG_BUFFER + 1];
    const struct dj_log_writer*  writer;
    void*                        writer_context;
    fn_log_clock                 clock;
    void*                        clock_context;
    uint64_t                     failures;
#if D_CFG_LOG_THREADSAFE
    dj_mutex_t                   lock;
    int                          lock_ready;
#endif
};

static struct dj_internal_log d_internal_log;

#if D_CFG_LOG_THREADSAFE
// the lock, and the first cache, are each made once, by whichever thread
// logs first
static dj_once_flag_t d_internal_log_once       = D_ONCE_FLAG_INIT;
static dj_once_flag_t d_internal_log_cache_once = D_ONCE_FLAG_INIT;
#endif


//==============================================================================
// LEVELS
//==============================================================================

// BUILT_IN
//   constant: the seven levels, and a program's own (D_CFG_LOG_LEVELS 2, 3).
#define D_INTERNAL_LOG_EXTRA_LEVEL(name, severity)  { #name, (severity) },

static const struct dj_log_level BUILT_IN[] =
{
    { "trace",   D_LOG_TRACE   },
    { "debug",   D_LOG_DEBUG   },
    { "info",    D_LOG_INFO    },
    { "notice",  D_LOG_NOTICE  },
    { "warning", D_LOG_WARNING },
    { "error",   D_LOG_ERROR   },
    { "fatal",   D_LOG_FATAL   },
#if D_CFG_LOG_LEVELS >= 2
    D_CFG_LOG_LEVELS_EXTRA(D_INTERNAL_LOG_EXTRA_LEVEL)
#endif
};

#define D_INTERNAL_LOG_BUILT_IN  (sizeof(BUILT_IN) / sizeof(BUILT_IN[0]))

/*
d_internal_log_same
  Whether a counted name is a NUL-terminated one, ignoring ASCII case.
*/
static int
d_internal_log_same(
    const char* _name,
    size_t      _size,
    const char* _other
)
{
    size_t at;

    for (at = 0u; at < _size; ++at)
    {
        char a = _name[at];
        char b = _other[at];

        a = ( (a >= 'A') && (a <= 'Z') ) ? (char)(a - 'A' + 'a') : a;
        b = ( (b >= 'A') && (b <= 'Z') ) ? (char)(b - 'A' + 'a') : b;

        if ( (b == '\0') || (a != b) )
        {
            return 0;
        }
    }

    return (_other[_size] == '\0') ? 1 : 0;
}

/*
d_internal_log_level_at
  The i-th level: built in first, then those added.
*/
static const struct dj_log_level*
d_internal_log_level_at(
    size_t _index
)
{
    if (_index < D_INTERNAL_LOG_BUILT_IN)
    {
        return &BUILT_IN[_index];
    }

    _index -= D_INTERNAL_LOG_BUILT_IN;

    return (_index < d_internal_log.level_count)
               ? &d_internal_log.levels[_index] : NULL;
}

uint32_t
d_log_level_find(
    const char* _name,
    size_t      _size
)
{
    const struct dj_log_level* level;
    uint32_t                   number = 0u;
    size_t                     index;

    if ( (_name == NULL) || (_size == 0u) )
    {
        return 0u;
    }

    for (index = 0u; (level = d_internal_log_level_at(index)) != NULL;
         ++index)
    {
        if (d_internal_log_same(_name, _size, level->name) != 0)
        {
            return level->severity;
        }
    }

    for (index = 0u; index < _size; ++index)
    {
        if ( (_name[index] < '0') || (_name[index] > '9') || (number > 24u) )
        {
            return 0u;
        }

        number = number * 10u + (uint32_t)(_name[index] - '0');
    }

    return ( (number >= 1u) && (number <= 24u) ) ? number : 0u;
}

const char*
d_log_level_name(
    uint32_t _severity
)
{
    const struct dj_log_level* level;
    const struct dj_log_level* best = NULL;
    size_t                     index;

    for (index = 0u; (level = d_internal_log_level_at(index)) != NULL;
         ++index)
    {
        if ( (level->severity <= _severity) &&
             ( (best == NULL) || (level->severity > best->severity) ) )
        {
            best = level;
        }
    }

    return (best != NULL) ? best->name : "unset";
}

int
d_log_level_add(
    const char* _name,
    uint32_t    _severity
)
{
#if D_CFG_LOG_LEVELS >= 3
    const size_t size  = (_name != NULL) ? strlen(_name) : 0u;
    uint32_t     found;
    size_t       at;

    if ( (size == 0u) || (_severity < 1u) || (_severity > 24u) ||
         ( (_name[0] >= '0') && (_name[0] <= '9') ) )
    {
        return -1;
    }

    for (at = 0u; at < size; ++at)
    {
        const char c = _name[at];

        if ( !( ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
                ((c >= '0') && (c <= '9')) || (c == '_') ) )
        {
            return -1;
        }
    }

    found = d_log_level_find(_name, size);

    if (found != 0u)
    {
        return (found == _severity) ? 0 : -1;
    }

    if (D_INTERNAL_LOG_BUILT_IN + d_internal_log.level_count >=
        (size_t)D_CFG_LOG_LEVELS_MAX)
    {
        return -1;
    }

    d_internal_log.levels[d_internal_log.level_count].name     = _name;
    d_internal_log.levels[d_internal_log.level_count].severity = _severity;
    ++d_internal_log.level_count;

    return 0;
#else
    (void)_name;
    (void)_severity;

    return -1;
#endif
}


//==============================================================================
// CODES
//==============================================================================

#define D_INTERNAL_LOG_CLASS_NAME(id, name)  name,

// CLASS_NAMES, GRADE_NAMES
//   constants: the vocabulary's names, by number.
static const char* const CLASS_NAMES[] =
{
    D_LOG_CLASSES(D_INTERNAL_LOG_CLASS_NAME)
    D_CFG_LOG_CLASSES_EXTRA(D_INTERNAL_LOG_CLASS_NAME)
};

static const char* const GRADE_NAMES[] =
{
    "ok", "note", "caveat", "soft", "rejected", "failed", "broken"
};

const char*
d_log_class_name(
    uint32_t _class
)
{
    return (_class < (uint32_t)D_LOG_CLASS_COUNT) ? CLASS_NAMES[_class] : "?";
}

const char*
d_log_grade_name(
    uint32_t _grade
)
{
    return (_grade < (uint32_t)D_LOG_GRADE_COUNT) ? GRADE_NAMES[_grade] : "?";
}

/*
d_log_code_key
  FNV-1a 64 over "module.name", or "module:number" without names, fed a
part at a time, as d_event_key_of_name over the whole would be.
*/
uint64_t
d_log_code_key(
    const struct dj_log_code* _code
)
{
    char        digits[13];
    const char* parts[3];
    size_t      part;
    size_t      at;
    uint64_t    hash = D_EVENT_FNV1A64_OFFSET;
    uint32_t    number;

    if ( (_code == NULL) || (_code->module == NULL) )
    {
        return 0u;
    }

    parts[0] = _code->module->name;
    parts[1] = (_code->name != NULL) ? "." : ":";
    parts[2] = _code->name;

    if (_code->name == NULL)
    {
        // the magnitude, unsigned, so INT32_MIN's has a value too
        at     = sizeof(digits) - 1u;
        number = (_code->number < 0) ? (0u - (uint32_t)_code->number)
                                     : (uint32_t)_code->number;
        digits[at] = '\0';

        do
        {
            digits[--at] = (char)('0' + (number % 10u));
            number      /= 10u;
        }
        while (number != 0u);

        if (_code->number < 0)
        {
            digits[--at] = '-';
        }

        parts[2] = digits + at;
    }

    for (part = 0u; part < 3u; ++part)
    {
        for (at = 0u; (parts[part] != NULL) && (parts[part][at] != '\0');
             ++at)
        {
            hash ^= (uint64_t)(unsigned char)parts[part][at];
            hash *= D_EVENT_FNV1A64_PRIME;
        }
    }

    return (hash != 0u) ? hash : 1u;
}

/*
d_internal_log_collides
  Whether a module's codes collide: two numbers alike in it, or a key
another registered code has (D_CFG_EVENT_KEY_CHECK: 1 in debug builds, 2
always).
*/
static int
d_internal_log_collides(
    const struct dj_log_module* _module
)
{
    size_t a;
    size_t b;

    for (a = 0u; a < _module->count; ++a)
    {
        for (b = a + 1u; b < _module->count; ++b)
        {
            if (_module->codes[a]->number == _module->codes[b]->number)
            {
                return 1;
            }
        }
    }

#if (D_CFG_EVENT_KEY_CHECK == 2) ||                                            \
    ( (D_CFG_EVENT_KEY_CHECK == 1) && !defined(NDEBUG) )
    {
        size_t m;

        for (m = 0u; m < d_internal_log.module_count; ++m)
        {
            const struct dj_log_module* const other =
                d_internal_log.modules[m];

            for (a = 0u; a < _module->count; ++a)
            {
                for (b = 0u; b < other->count; ++b)
                {
                    if (d_log_code_key(_module->codes[a]) ==
                        d_log_code_key(other->codes[b]))
                    {
                        return 1;
                    }
                }
            }
        }
    }
#endif

    return 0;
}

/*
d_internal_log_scope_name
  A module's scope name: its name, each '.' as '_', as option_env's scopes
are identifiers.
*/
static void
d_internal_log_scope_name(
    char*       _scope,
    const char* _name
)
{
    size_t at;

    for (at = 0u; (_name[at] != '\0') &&
                  (at + 1u < D_INTERNAL_LOG_SCOPE_NAME); ++at)
    {
        _scope[at] = (_name[at] == '.') ? '_' : _name[at];
    }

    _scope[at] = '\0';

    return;
}


//==============================================================================
// THE DJIN
//==============================================================================

#if D_INTERNAL_LOG_OPTIONS

// LEVEL_NAMES
//   constant: the names log.level and the grades' rows take, with numbers.
#define D_INTERNAL_LOG_EXTRA_NAME(name, severity)  { #name, (severity) },

static const struct dj_option_env_name LEVEL_NAMES[] =
{
    { "trace",   (int64_t)D_LOG_TRACE   },
    { "debug",   (int64_t)D_LOG_DEBUG   },
    { "info",    (int64_t)D_LOG_INFO    },
    { "notice",  (int64_t)D_LOG_NOTICE  },
    { "warning", (int64_t)D_LOG_WARNING },
    { "error",   (int64_t)D_LOG_ERROR   },
    { "fatal",   (int64_t)D_LOG_FATAL   },
#if D_CFG_LOG_LEVELS >= 2
    D_CFG_LOG_LEVELS_EXTRA(D_INTERNAL_LOG_EXTRA_NAME)
#endif
};

// FORMATS_BUILT, FORMATS_UNBUILT
//   constants: log.format's names: the writers built, and those left out,
// which are refused by name.
#define D_INTERNAL_LOG_HAS(format)  (((D_CFG_LOG_FORMATS) >> (format)) & 1)

static const struct dj_option_env_name FORMATS_BUILT[] =
{
#if D_INTERNAL_LOG_HAS(0)
    { "text", (int64_t)D_LOG_FORMAT_TEXT },
#endif
#if D_INTERNAL_LOG_HAS(1)
    { "json", (int64_t)D_LOG_FORMAT_JSON },
#endif
#if D_INTERNAL_LOG_HAS(2)
    { "otlp", (int64_t)D_LOG_FORMAT_OTLP },
#endif
};

#if (D_CFG_LOG_FORMATS != 0x7)
static const struct dj_option_env_name FORMATS_UNBUILT[] =
{
#if !D_INTERNAL_LOG_HAS(0)
    { "text", (int64_t)D_LOG_FORMAT_TEXT },
#endif
#if !D_INTERNAL_LOG_HAS(1)
    { "json", (int64_t)D_LOG_FORMAT_JSON },
#endif
#if !D_INTERNAL_LOG_HAS(2)
    { "otlp", (int64_t)D_LOG_FORMAT_OTLP },
#endif
};
    #define D_INTERNAL_LOG_UNBUILT                                            \
        FORMATS_UNBUILT, (sizeof(FORMATS_UNBUILT) / sizeof(FORMATS_UNBUILT[0]))
#else
    #define D_INTERNAL_LOG_UNBUILT  NULL, 0u
#endif

// CODE_STYLES, NESTINGS, TIMES, INTEGERS, NONFINITES
//   constants: the other choices' names.
static const struct dj_option_env_name CODE_STYLES[] =
{
    { "name", 0 }, { "number", 1 }, { "both", 2 }
};

static const struct dj_option_env_name NESTINGS[] =
{
    { "nested", 0 }, { "dotted", 1 }
};

static const struct dj_option_env_name TIMES[] =
{
    { "utc", 0 },   { "utc_ms", 1 },   { "utc_us", 2 },   { "utc_ns", 3 },
    { "local", 4 }, { "local_ms", 5 }, { "local_us", 6 }, { "local_ns", 7 }
};

static const struct dj_option_env_name INTEGERS[] =
{
    { "number", 0 }, { "string", 1 }
};

static const struct dj_option_env_name NONFINITES[] =
{
    { "null", 0 }, { "string", 1 }
};

// D_INTERNAL_LOG_FORMAT_ROW
//   macro: log.format's row, its unbuilt names a pair given whole, so they
// are two of D_OPTION_ENV_ROW's arguments once expanded.
#define D_INTERNAL_LOG_FORMAT_ROW(unbuilt)                                    \
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_CHOICE, "format", FORMATS_BUILT,       \
                     sizeof(FORMATS_BUILT) / sizeof(FORMATS_BUILT[0]),        \
                     unbuilt, NULL, D_CFG_LOG_FORMAT, INT64_MIN, INT64_MAX,   \
                     0.0, NULL, "D_CFG_LOG_FORMAT",                           \
                     "the writer: text, json or otlp")

// D_INTERNAL_LOG_GRADE_ROW
//   macro: a grade's level, a row whose default is the knob's.
#define D_INTERNAL_LOG_GRADE_ROW(name, grade)                                 \
    D_OPTION_ENV_INT_RANGE_KNOB_NAMED(name, D_CFG_LOG_GRADE_LEVEL(grade), 1,  \
                                      24, LEVEL_NAMES,                        \
                                      "this grade's level: a level's name "   \
                                      "or a severity")

// SCHEMA
//   constant: the djin's variables, in D_LOG_VAR_*'s order, their defaults
// the knobs.
static const struct dj_option_env_var SCHEMA[D_LOG_VARS] =
{
    D_OPTION_ENV_INT_RANGE_KNOB_NAMED("level", D_CFG_LOG_LEVEL, 1, 24,
                                      LEVEL_NAMES,
                                      "the threshold: a level's name or a "
                                      "severity, 1 to 24"),
    D_OPTION_ENV_TEXT("levels", NULL,
                      "levels added, name=severity joined by ','"),
    D_INTERNAL_LOG_FORMAT_ROW(D_INTERNAL_LOG_UNBUILT),
    D_OPTION_ENV_TEXT("destination", "stderr",
                      "where records go: stderr, none, or a file's path"),
    D_OPTION_ENV_CHOICE_KNOB("code_style", CODE_STYLES, D_CFG_LOG_CODE_STYLE,
                             "a code in text: name, number or both"),
    D_OPTION_ENV_INT_RANGE_KNOB("facts_max", D_CFG_LOG_FACTS_MAX, 0,
                                D_CFG_LOG_FACTS_MAX,
                                "the facts a record keeps"),
    D_OPTION_ENV_INT_RANGE_KNOB("depth_max", D_CFG_LOG_DEPTH_MAX, 0,
                                D_CFG_LOG_DEPTH_MAX,
                                "the deepest a record's facts nest"),
    D_OPTION_ENV_CHOICE_KNOB("nesting", NESTINGS, D_CFG_LOG_NESTING,
                             "nested facts in JSON: nested or dotted"),
    D_INTERNAL_LOG_GRADE_ROW("grade_ok", 0),
    D_INTERNAL_LOG_GRADE_ROW("grade_note", 1),
    D_INTERNAL_LOG_GRADE_ROW("grade_caveat", 2),
    D_INTERNAL_LOG_GRADE_ROW("grade_soft", 3),
    D_INTERNAL_LOG_GRADE_ROW("grade_rejected", 4),
    D_INTERNAL_LOG_GRADE_ROW("grade_failed", 5),
    D_INTERNAL_LOG_GRADE_ROW("grade_broken", 6),
    D_OPTION_ENV_BOOL("severity_number", 1, "write the severity's number"),
    D_OPTION_ENV_BOOL_KNOB("scope", D_CFG_LOG_SCOPE,
                           "write the module as the scope"),
    D_OPTION_ENV_BOOL("resource", 0,
                      "write the resource on every record; OTLP always does"),
    D_OPTION_ENV_BOOL_KNOB("context", D_CFG_LOG_CONTEXT,
                           "write trace and span ids, where built"),
    D_OPTION_ENV_BOOL("observed_time", 0,
                      "write when the log saw a record, where built"),
    D_OPTION_ENV_BOOL("dropped_count", 1, "write the facts not kept"),
    D_OPTION_ENV_CHOICE_KNOB("time", TIMES, D_CFG_LOG_TIME,
                             "UTC or local, to s, ms, us or ns"),
    D_OPTION_ENV_CHOICE_KNOB("json_integers", INTEGERS,
                             D_CFG_LOG_JSON_INTEGERS,
                             "integers in JSON lines: number or string"),
    D_OPTION_ENV_CHOICE_KNOB("json_nonfinite", NONFINITES,
                             D_CFG_LOG_JSON_NONFINITE,
                             "NaN and infinities in JSON lines: null or "
                             "string"),
    D_OPTION_ENV_TEXT("service", NULL,
                      "the resource's service.name"),
    D_OPTION_ENV_INT_RANGE_KNOB("real_digits", D_CFG_LOG_REAL_DIGITS, 0, 17,
                                "a real's significant digits: 0 all, read "
                                "back exactly; 1 to 17 rounds")
};

// dj_internal_log_djin_storage
//   struct: the djin's storage.
D_OPTION_ENV_DECLARE(dj_internal_log_djin_storage,
                     D_CFG_LOG_BINDINGS,
                     D_LOG_VARS,
                     D_CFG_LOG_TEXT_ROOM);

// dj_internal_log_djin
//   struct: the djin and its storage.
struct dj_internal_log_djin
{
    struct dj_option_env                frame;
    struct dj_internal_log_djin_storage storage;
};

// d_internal_log_djin
//   variable: the djin "log", its variables declared by its initialiser, so
// it is ready before the program runs.
static struct dj_internal_log_djin d_internal_log_djin =
{
    .frame =
    {
        .set =
        {
            .options        = d_internal_log_djin.storage.option_block,
            .values         = (unsigned char*)
                                  d_internal_log_djin.storage.value_block,
            .count          = 0u,
            .capacity       = (uint32_t)D_CFG_LOG_BINDINGS,
            .value_used     = 0u,
            .value_capacity = (uint32_t)(sizeof(uint64_t) *
                                         D_CFG_LOG_BINDINGS)
        },
        .vars          = d_internal_log_djin.storage.var_block,
        .scopes        = (const char* const*)d_internal_log.scopes,
        .name          = "log",
        .text          = d_internal_log_djin.storage.text_block,
        .text_capacity = (uint32_t)D_CFG_LOG_TEXT_ROOM,
        .var_count     = (uint32_t)D_LOG_VARS,
        .var_capacity  = (uint32_t)D_LOG_VARS,
        .scope_count   = 0u
    },
    .storage =
    {
        .var_block =
        {
            &SCHEMA[0],  &SCHEMA[1],  &SCHEMA[2],  &SCHEMA[3],  &SCHEMA[4],
            &SCHEMA[5],  &SCHEMA[6],  &SCHEMA[7],  &SCHEMA[8],  &SCHEMA[9],
            &SCHEMA[10], &SCHEMA[11], &SCHEMA[12], &SCHEMA[13], &SCHEMA[14],
            &SCHEMA[15], &SCHEMA[16], &SCHEMA[17], &SCHEMA[18], &SCHEMA[19],
            &SCHEMA[20], &SCHEMA[21], &SCHEMA[22], &SCHEMA[23], &SCHEMA[24],
            &SCHEMA[25]
        }
    }
};

struct dj_option_env*
d_log_djin(void)
{
    return &d_internal_log_djin.frame;
}

struct dj_option_result
d_log_register(
    struct dj_option_env_registry* _registry
)
{
    return d_option_env_registry_add((_registry != NULL)
                                         ? _registry
                                         : d_option_env_registry(),
                                     &d_internal_log_djin.frame);
}

/*
d_internal_log_int
  A variable in a scope, through the djin; the scope's own binding, else the
global one, else the default.
*/
static int64_t
d_internal_log_int(
    uint32_t _scope,
    uint32_t _var
)
{
    const struct dj_option_env* const frame = &d_internal_log_djin.frame;

    return d_option_env_int(frame, _scope,
                            D_OPTION_ENV_ID(frame->depth, _var));
}

/*
d_internal_log_text
  A text variable, globally.
*/
static const char*
d_internal_log_text(
    uint32_t _var
)
{
    const struct dj_option_env* const frame = &d_internal_log_djin.frame;

    return d_option_env_text(frame, D_OPTION_ENV_GLOBAL,
                             D_OPTION_ENV_ID(frame->depth, _var));
}

/*
d_internal_log_bool
  A bool variable in a scope.
*/
static uint32_t
d_internal_log_bool(
    uint32_t _scope,
    uint32_t _var
)
{
    const struct dj_option_env* const frame = &d_internal_log_djin.frame;

    return d_option_env_bool(frame, _scope,
                             D_OPTION_ENV_ID(frame->depth, _var)) ? 1u : 0u;
}

#else  // !D_INTERNAL_LOG_OPTIONS: the knobs decide alone

struct dj_option_env*
d_log_djin(void)
{
    return NULL;
}

struct dj_option_result
d_log_register(
    struct dj_option_env_registry* _registry
)
{
    (void)_registry;

    return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
}

#endif  // D_INTERNAL_LOG_OPTIONS

/*
d_internal_log_scope_of
  The djin's scope for a module: its own when registered and in a scope,
else the global one; and the cache's slot, 0 for the global.
*/
static uint32_t
d_internal_log_scope_of(
    const struct dj_log_module* _module,
    size_t*                     _slot
)
{
    size_t index;

    *_slot = 0u;

    for (index = 0u; (_module != NULL) &&
                     (index < d_internal_log.module_count); ++index)
    {
        if ( (d_internal_log.modules[index] == _module) &&
             (index < (size_t)D_CFG_LOG_SCOPES) )
        {
            *_slot = index + 1u;
#if D_INTERNAL_LOG_OPTIONS
            {
                const struct dj_option_result found = d_option_env_find_scope(
                    &d_internal_log_djin.frame, d_internal_log.scopes[index],
                    strlen(d_internal_log.scopes[index]));

                return found.is_ok ? found.payload.value
                                   : D_OPTION_ENV_GLOBAL;
            }
#else
            return D_OPTION_ENV_GLOBAL;
#endif
        }
    }

    return D_OPTION_ENV_GLOBAL;
}

static void d_internal_log_lock(void);
static void d_internal_log_unlock(void);

/*
d_internal_log_refresh
  The cache made again, for the global scope and each module's, when the
djin's generation moved since it was made; under the lock, so two threads
that find it stale make it once. Settings are written before the program's
threads start, and d_log_init makes the cache, so threads then only read.
*/
static void
d_internal_log_refresh(void)
{
    size_t   slot;
    uint32_t grade;
#if D_INTERNAL_LOG_OPTIONS
    const uint64_t generation =
        d_option_env_generation(&d_internal_log_djin.frame) + 1u;
#else
    const uint64_t generation = 1u;
#endif

    if (d_internal_log.generation == generation)
    {
        return;
    }

    d_internal_log_lock();

    for (slot = 0u; (d_internal_log.generation != generation) &&
                    (slot <= d_internal_log.module_count); ++slot)
    {
        struct dj_internal_log_cache* const cache = &d_internal_log.cache[slot];
        uint32_t scope = D_OPTION_ENV_GLOBAL;
        size_t   unused;

        if (slot > (size_t)D_CFG_LOG_SCOPES)
        {
            break;
        }

        if (slot != 0u)
        {
            scope = d_internal_log_scope_of(d_internal_log.modules[slot - 1u],
                                            &unused);
        }

#if D_INTERNAL_LOG_OPTIONS
        cache->threshold = (uint8_t)d_internal_log_int(scope,
                                                       D_LOG_VAR_LEVEL);

        for (grade = 0u; grade < (uint32_t)D_LOG_GRADE_COUNT; ++grade)
        {
            cache->grades[grade] = (uint8_t)d_internal_log_int(
                                       scope, D_LOG_VAR_GRADE_OK + grade);
        }
#else
        (void)scope;
        cache->threshold = (uint8_t)D_CFG_LOG_LEVEL;

        for (grade = 0u; grade < (uint32_t)D_LOG_GRADE_COUNT; ++grade)
        {
            cache->grades[grade] = (uint8_t)D_CFG_LOG_GRADE_LEVEL(grade);
        }
#endif
    }

    d_internal_log.generation = generation;
    d_internal_log_unlock();

    return;
}

int
d_log_module_add(
    const struct dj_log_module* _module
)
{
    size_t index;

    if ( (_module == NULL) || (_module->name == NULL) ||
         ( (_module->codes == NULL) && (_module->count != 0u) ) )
    {
        return -1;
    }

    for (index = 0u; index < d_internal_log.module_count; ++index)
    {
        if (d_internal_log.modules[index] == _module)
        {
            return 0;
        }
    }

    if ( (d_internal_log.module_count >= (size_t)D_CFG_LOG_MODULES_MAX) ||
         (d_internal_log_collides(_module) != 0) )
    {
        return -1;
    }

    if (d_internal_log.module_count < (size_t)D_CFG_LOG_SCOPES)
    {
        index = d_internal_log.module_count;
        d_internal_log_scope_name(d_internal_log.scope_names[index],
                                  _module->name);
        d_internal_log.scopes[index] = d_internal_log.scope_names[index];
#if D_INTERNAL_LOG_OPTIONS
        // a scope that would make one of the log's settings' environment
        // names twice is refused, and the module with it (option_env's
        // decision 47); the scopes are as they were
        if (!d_option_env_set_scopes(&d_internal_log_djin.frame,
                                     d_internal_log.scopes,
                                     (uint32_t)(index + 1u)).is_ok)
        {
            return -1;
        }
#endif
    }

    d_internal_log.modules[d_internal_log.module_count] = _module;
    ++d_internal_log.module_count;
    d_internal_log.generation = 0u;

    return 0;
}

const struct dj_log_code*
d_log_code_find(
    const char* _module,
    size_t      _size,
    int32_t     _number
)
{
    size_t index;
    size_t code;

    for (index = 0u; (_module != NULL) &&
                     (index < d_internal_log.module_count); ++index)
    {
        const struct dj_log_module* const module =
            d_internal_log.modules[index];

        if ( (strlen(module->name) != _size) ||
             (memcmp(module->name, _module, _size) != 0) )
        {
            continue;
        }

        for (code = 0u; code < module->count; ++code)
        {
            if (module->codes[code]->number == _number)
            {
                return module->codes[code];
            }
        }
    }

    return NULL;
}

void
d_log_settings(
    struct dj_log_settings*     _settings,
    const struct dj_log_module* _module
)
{
    size_t         slot;
    const uint32_t scope = d_internal_log_scope_of(_module, &slot);

    if (_settings == NULL)
    {
        return;
    }

#if D_INTERNAL_LOG_OPTIONS
    _settings->format          = (uint32_t)d_internal_log_int(scope,
                                     D_LOG_VAR_FORMAT);
    _settings->code_style      = (uint32_t)d_internal_log_int(scope,
                                     D_LOG_VAR_CODE_STYLE);
    _settings->nesting         = (uint32_t)d_internal_log_int(scope,
                                     D_LOG_VAR_NESTING);
    _settings->json_integers   = (uint32_t)d_internal_log_int(scope,
                                     D_LOG_VAR_JSON_INTEGERS);
    _settings->json_nonfinite  = (uint32_t)d_internal_log_int(scope,
                                     D_LOG_VAR_JSON_NONFINITE);
    _settings->time            = (uint32_t)d_internal_log_int(scope,
                                     D_LOG_VAR_TIME);
    _settings->severity_number = d_internal_log_bool(scope,
                                     D_LOG_VAR_SEVERITY_NUMBER);
    _settings->scope           = d_internal_log_bool(scope,
                                     D_LOG_VAR_SCOPE) & (D_CFG_LOG_SCOPE);
    _settings->resource        = d_internal_log_bool(scope,
                                     D_LOG_VAR_RESOURCE) &
                                 (D_CFG_LOG_RESOURCE);
    _settings->context         = d_internal_log_bool(scope,
                                     D_LOG_VAR_CONTEXT) & (D_CFG_LOG_CONTEXT);
    _settings->observed_time   = d_internal_log_bool(scope,
                                     D_LOG_VAR_OBSERVED_TIME) &
                                 (D_CFG_LOG_OBSERVED_TIME);
    _settings->dropped_count   = d_internal_log_bool(scope,
                                     D_LOG_VAR_DROPPED_COUNT);
    _settings->service         = d_internal_log_text(D_LOG_VAR_SERVICE);
    _settings->real_digits     = (uint32_t)d_internal_log_int(scope,
                                     D_LOG_VAR_REAL_DIGITS);
#else
    (void)scope;
    _settings->format          = (uint32_t)D_CFG_LOG_FORMAT;
    _settings->code_style      = (uint32_t)D_CFG_LOG_CODE_STYLE;
    _settings->nesting         = (uint32_t)D_CFG_LOG_NESTING;
    _settings->json_integers   = (uint32_t)D_CFG_LOG_JSON_INTEGERS;
    _settings->json_nonfinite  = (uint32_t)D_CFG_LOG_JSON_NONFINITE;
    _settings->real_digits     = (uint32_t)D_CFG_LOG_REAL_DIGITS;
    _settings->time            = (uint32_t)D_CFG_LOG_TIME;
    _settings->severity_number = 1u;
    _settings->scope           = (uint32_t)D_CFG_LOG_SCOPE;
    _settings->resource        = 0u;
    _settings->context         = (uint32_t)D_CFG_LOG_CONTEXT;
    _settings->observed_time   = 0u;
    _settings->dropped_count   = 1u;
    _settings->service         = NULL;
#endif

    return;
}


//==============================================================================
// THE DESTINATION, THE WRITER AND THE CLOCK
//==============================================================================

/*
d_internal_log_now
  The system's time now, in ns since 1970, UTC: clock_gettime where POSIX
gives it, C11's timespec_get where that is all there is, else time(). Windows
takes timespec_get: dtime.h names CLOCK_REALTIME there for its own
d_clock_gettime, and the Microsoft runtime has no clock_gettime.
*/
static uint64_t
d_internal_log_now(
    void* _context
)
{
    (void)_context;

#if defined(CLOCK_REALTIME) && !defined(_WIN32)
    {
        struct timespec now;

        if (clock_gettime(CLOCK_REALTIME, &now) == 0)
        {
            return (uint64_t)now.tv_sec * 1000000000u +
                   (uint64_t)now.tv_nsec;
        }
    }
#elif defined(TIME_UTC)
    {
        struct timespec now;

        if (timespec_get(&now, TIME_UTC) == TIME_UTC)
        {
            return (uint64_t)now.tv_sec * 1000000000u +
                   (uint64_t)now.tv_nsec;
        }
    }
#endif

    return (uint64_t)time(NULL) * 1000000000u;
}

/*
d_internal_log_clock_now
  The time now, by the program's clock where it set one.
*/
static uint64_t
d_internal_log_clock_now(void)
{
    return (d_internal_log.clock != NULL)
               ? d_internal_log.clock(d_internal_log.clock_context)
               : d_internal_log_now(NULL);
}

#if D_CFG_LOG_THREADSAFE
/*
d_internal_log_lock_make
  Makes the lock: run once, through d_call_once, so two threads that log
first at the same moment make one lock between them. It lasts as long as
the program.
*/
static void
d_internal_log_lock_make(void)
{
    d_internal_log.lock_ready = (d_mutex_init(&d_internal_log.lock) == 0)
                                    ? 1 : -1;

    return;
}
#endif

/*
d_internal_log_lock, d_internal_log_unlock
  The lock around the writer and its stream, where built.
*/
static void
d_internal_log_lock(void)
{
#if D_CFG_LOG_THREADSAFE
    d_call_once(&d_internal_log_once, d_internal_log_lock_make);

    if (d_internal_log.lock_ready == 1)
    {
        (void)d_mutex_lock(&d_internal_log.lock);
    }
#endif

    return;
}

static void
d_internal_log_unlock(void)
{
#if D_CFG_LOG_THREADSAFE
    if (d_internal_log.lock_ready == 1)
    {
        (void)d_mutex_unlock(&d_internal_log.lock);
    }
#endif

    return;
}

/*
d_internal_log_close_own
  Closes the log's own stream: flushed, and a file it opened closed.
*/
static void
d_internal_log_close_own(void)
{
    if (d_internal_log.own_open != 0)
    {
        if (d_internal_log.own_file != 0)
        {
            (void)d_ostream_close(&d_internal_log.own_os);
        }
        else
        {
            (void)d_ostream_flush(&d_internal_log.own_os);
        }
    }

    d_internal_log.own_open = 0;
    d_internal_log.own_file = 0;
    d_internal_log.none     = 0;

    return;
}

/*
d_internal_log_open_own
  Opens the log's own stream as log.destination says: standard error, none,
or a file, appended to. A file that cannot be opened leaves standard error.
*/
static int
d_internal_log_open_own(void)
{
    const char* destination = "stderr";
    int         failed      = 0;

#if D_INTERNAL_LOG_OPTIONS
    if (d_internal_log_text(D_LOG_VAR_DESTINATION) != NULL)
    {
        destination = d_internal_log_text(D_LOG_VAR_DESTINATION);
    }
#endif

    d_internal_log_close_own();

    if (strcmp(destination, "none") == 0)
    {
        d_internal_log.none = 1;

        return 0;
    }

    if ( (strcmp(destination, "stderr") != 0) &&
         (destination[0] != '\0') )
    {
        if (d_ostream_open_file(&d_internal_log.own_os, destination, 1) != 0)
        {
            d_internal_log.own_file = 1;
        }
        else
        {
            failed = -1;
        }
    }

    if (d_internal_log.own_file == 0)
    {
        (void)d_ostream_init_console(&d_internal_log.own_os,
                                     &d_internal_log.console,
                                     D_OSTREAM_CONSOLE_ERR, 0);
    }

#if D_CFG_LOG_BUFFER > 0
    (void)d_ostream_set_buffer(&d_internal_log.own_os, d_internal_log.buffer,
                               (size_t)D_CFG_LOG_BUFFER);
#endif
    d_internal_log.own_open = 1;

    return failed;
}

/*
d_internal_log_stream
  Where a record goes: the program's stream, else the log's own, opened on
first use; null for none.
*/
static struct dj_ostream*
d_internal_log_stream(void)
{
    if (d_internal_log.program_os != NULL)
    {
        return d_internal_log.program_os;
    }

    if ( (d_internal_log.own_open == 0) &&
         (d_internal_log.none == 0) )
    {
        (void)d_internal_log_open_own();
    }

    return (d_internal_log.none != 0) ? NULL : &d_internal_log.own_os;
}

/*
d_internal_log_writer
  The writer: the program's, else log.format's.
*/
static const struct dj_log_writer*
d_internal_log_writer(
    uint32_t _format
)
{
    if (d_internal_log.writer != NULL)
    {
        return d_internal_log.writer;
    }

    return (_format == D_LOG_FORMAT_JSON) ? d_log_writer_json()
         : (_format == D_LOG_FORMAT_OTLP) ? d_log_writer_otlp()
                                          : d_log_writer_text();
}

/*
d_internal_log_levels_from_text
  log.levels: name=severity pairs joined by ',', each added; the names
copied, since the djin's text may move.
*/
static void
d_internal_log_levels_from_text(void)
{
#if (D_CFG_LOG_LEVELS >= 3) && D_INTERNAL_LOG_OPTIONS
    static char  names[D_CFG_LOG_LEVELS_MAX][24];
    static size_t used;
    const char*  at = d_internal_log_text(D_LOG_VAR_LEVELS);

    while ( (at != NULL) && (*at != '\0') && (used < sizeof(names) /
                                                     sizeof(names[0])) )
    {
        const char* const end    = strchr(at, ',');
        const size_t      size   = (end != NULL) ? (size_t)(end - at)
                                                 : strlen(at);
        const char* const equals = (const char*)memchr(at, '=', size);

        if ( (equals != NULL) &&
             ((size_t)(equals - at) < sizeof(names[0])) )
        {
            const size_t name = (size_t)(equals - at);

            memcpy(names[used], at, name);
            names[used][name] = '\0';

            if (d_log_level_add(names[used],
                                d_log_level_find(equals + 1,
                                                 size - name - 1u)) == 0)
            {
                ++used;
            }
        }

        at = (end != NULL) ? end + 1 : NULL;
    }
#endif

    return;
}

int
d_log_init(void)
{
    int failed;

    d_internal_log_lock();
    d_internal_log_levels_from_text();
    failed = d_internal_log_open_own();
    d_internal_log.generation = 0u;
    d_internal_log_unlock();

    // the cache made now, so threads that log afterwards only read it
    d_internal_log_refresh();

    return failed;
}

void
d_log_shutdown(void)
{
    d_internal_log_lock();
    d_internal_log_close_own();
    d_internal_log_unlock();

    return;
}

void
d_log_set_stream(
    struct dj_ostream* _os
)
{
    d_internal_log_lock();
    d_internal_log.program_os = _os;
    d_internal_log_unlock();

    return;
}

void
d_log_set_writer(
    const struct dj_log_writer* _writer,
    void*                       _context
)
{
    d_internal_log_lock();
    d_internal_log.writer         = _writer;
    d_internal_log.writer_context = _context;
    d_internal_log_unlock();

    return;
}

void
d_log_set_clock(
    fn_log_clock _clock,
    void*        _context
)
{
    d_internal_log.clock         = _clock;
    d_internal_log.clock_context = _context;

    return;
}

uint64_t
d_log_failures(void)
{
    return d_internal_log.failures;
}


//==============================================================================
// RECORDS
//==============================================================================

/*
d_internal_log_cache_of
  The cache's slot for a code's module, made again first if the settings
moved.
*/
static const struct dj_internal_log_cache*
d_internal_log_cache_of(
    const struct dj_log_code* _code
)
{
    size_t slot;

    // the first cache made once, so threads that log first at the same
    // moment, before d_log_init, do not race to make it
#if D_CFG_LOG_THREADSAFE
    d_call_once(&d_internal_log_cache_once, d_internal_log_refresh);
#endif
    d_internal_log_refresh();
    (void)d_internal_log_scope_of(_code->module, &slot);

    return &d_internal_log.cache[slot];
}

int
d_log_wants(
    const struct dj_log_code* _code,
    uint32_t                  _severity
)
{
#if D_CFG_LOG_ENABLED
    const struct dj_internal_log_cache* cache;

    if ( (_code == NULL) || (_code->grade >= (uint16_t)D_LOG_GRADE_COUNT) )
    {
        return 0;
    }

#if D_CFG_LOG_LEVELS == 0
    (void)_severity;
    (void)cache;

    return 1;
#else
    cache = d_internal_log_cache_of(_code);

    if (_severity == 0u)
    {
        _severity = cache->grades[_code->grade];
    }

#if (D_CFG_LOG_LEVEL_MIN > 0)
    if (_severity < (uint32_t)D_CFG_LOG_LEVEL_MIN)
    {
        return 0;
    }
#endif

    return (_severity >= (uint32_t)cache->threshold) ? 1 : 0;
#endif
#else
    (void)_code;
    (void)_severity;

    return 0;
#endif
}

int
d_log_open(
    struct dj_log_record*     _record,
    const struct dj_log_code* _code,
    uint32_t                  _severity
)
{
    size_t facts = (size_t)D_CFG_LOG_FACTS_MAX;
    size_t depth = (size_t)D_CFG_LOG_DEPTH_MAX;

    if (_record == NULL)
    {
        return 0;
    }

    _record->open = 0u;

    if (d_log_wants(_code, _severity) == 0)
    {
        return 0;
    }

#if D_INTERNAL_LOG_OPTIONS
    {
        size_t         slot;
        const uint32_t scope = d_internal_log_scope_of(_code->module, &slot);

        facts = (size_t)d_internal_log_int(scope, D_LOG_VAR_FACTS_MAX);
        depth = (size_t)d_internal_log_int(scope, D_LOG_VAR_DEPTH_MAX);
    }
#endif

    _record->code     = _code;
    _record->severity = (_severity != 0u)
                            ? _severity
                            : d_internal_log_cache_of(_code)
                                  ->grades[_code->grade];
    _record->open     = 1u;
    _record->time     = d_internal_log_clock_now();
    d_desc_collect_init(&_record->collected, _record->items,
                        (facts < (size_t)D_CFG_LOG_FACTS_MAX)
                            ? facts : (size_t)D_CFG_LOG_FACTS_MAX);
    d_desc_sink_init(&_record->sink, d_desc_collect_machine(),
                     &_record->collected);
    _record->sink.max_depth = (uint32_t)depth;

    return 1;
}

struct dj_desc_sink*
d_log_sink(
    struct dj_log_record* _record
)
{
    return ( (_record != NULL) && (_record->open != 0u) ) ? &_record->sink
                                                          : NULL;
}

// dj_internal_log_fields
//   struct: a body's fields: the code's list of fact names, and the facts.
struct dj_internal_log_fields
{
    const char*                         names;  // "(host, timeout_ms)"
    const struct dj_desc_collect_state* facts;
};

/*
d_internal_log_name_at
  The i-th name of a list "(a, b, c)": where it starts and its length; 0
when there is none.
*/
static size_t
d_internal_log_name_at(
    const char*  _names,
    uint32_t     _index,
    const char** _start
)
{
    const char* at = _names;
    uint32_t    index = 0u;

    while ( (at != NULL) && (*at != '\0') )
    {
        size_t size = 0u;

        while ( (*at == '(') || (*at == ',') || (*at == ' ') || (*at == ')') )
        {
            ++at;
        }

        while ( (at[size] != '\0') && (at[size] != ',') &&
                (at[size] != ' ')  && (at[size] != ')') )
        {
            ++size;
        }

        if (size == 0u)
        {
            return 0u;
        }

        if (index == _index)
        {
            *_start = at;

            return size;
        }

        at += size;
        ++index;
    }

    return 0u;
}

/*
d_internal_log_lookup
  A template's field: its place in the code's list of facts.
*/
static int32_t
d_internal_log_lookup(
    void*       _context,
    const char* _name,
    size_t      _size
)
{
    const struct dj_internal_log_fields* const fields =
        (const struct dj_internal_log_fields*)_context;
    const char* start;
    uint32_t    index;
    size_t      size;

    for (index = 0u; (size = d_internal_log_name_at(fields->names, index,
                                                    &start)) != 0u; ++index)
    {
        if ( (size == _size) && (memcmp(start, _name, size) == 0) )
        {
            return (int32_t)index;
        }
    }

    return -1;
}

/*
d_internal_log_resolve
  A field's value: the record's fact of that key at the top level, or none.
*/
static int32_t
d_internal_log_resolve(
    void*                    _context,
    uint32_t                 _field,
    struct dj_ostream_value* _value
)
{
    const struct dj_internal_log_fields* const fields =
        (const struct dj_internal_log_fields*)_context;
    const char*  start = NULL;
    const size_t size  = d_internal_log_name_at(fields->names, _field,
                                                &start);
    size_t       index;

    *_value = d_ostream_value_none();

    for (index = 0u; (size != 0u) && (index < fields->facts->count); ++index)
    {
        const struct dj_desc_item* const item = &fields->facts->items[index];

        if ( (item->kind == D_DESC_ITEM_FACT) && (item->fact.depth == 0u) &&
             (item->fact.key != NULL) &&
             (strlen(item->fact.key) == size) &&
             (memcmp(item->fact.key, start, size) == 0) )
        {
            *_value = item->fact.value;
            break;
        }
    }

    return 1;
}

/*
d_internal_log_body
  The body: the code's template written from the record's facts, cut where
a character starts if it is too long; the template as it is when it does
not compile. Returns its length.
*/
static size_t
d_internal_log_body(
    const struct dj_log_record* _record,
    char*                       _body,
    size_t                      _capacity
)
{
    struct dj_ostream_template_op    ops[D_CFG_LOG_TEMPLATE_OPS];
    struct dj_ostream_template       compiled;
    struct dj_ostream_template_error error;
    struct dj_internal_log_fields    fields;
    struct dj_ostream                os;
    struct dj_ostream_fixed          fixed;
    size_t                           size;

    if (_record->code->text == NULL)
    {
        _body[0] = '\0';

        return 0u;
    }

    fields.names = _record->code->facts;
    fields.facts = &_record->collected;
    (void)d_ostream_init_fixed(&os, &fixed, _body, _capacity, 1);

    if (d_ostream_template_compile(&compiled, ops,
                                   (size_t)D_CFG_LOG_TEMPLATE_OPS,
                                   _record->code->text,
                                   d_internal_log_lookup, &fields,
                                   &error) == (int32_t)D_OSTREAM_TEMPLATE_OK)
    {
        (void)d_ostream_template_render(&os, &compiled,
                                        d_internal_log_resolve, &fields);
    }
    else
    {
        (void)d_ostream_write_cstr(&os, _record->code->text);
    }

    (void)d_ostream_close(&os);
    size = d_ostream_fixed_length(&fixed);

    // a cut never falls inside a character: a last one left short goes
    if ( (d_ostream_fixed_truncated(&fixed) != 0) && (size > 0u) )
    {
        size_t               lead = size - 1u;
        const unsigned char* text = (const unsigned char*)_body;

        while ( (lead > 0u) && ((text[lead] & 0xC0u) == 0x80u) )
        {
            --lead;
        }

        if ( (size - lead) < ( ((text[lead] & 0xE0u) == 0xC0u) ? 2u
                             : ((text[lead] & 0xF0u) == 0xE0u) ? 3u
                             : ((text[lead] & 0xF8u) == 0xF0u) ? 4u : 1u ) )
        {
            size = lead;
        }

        _body[size] = '\0';
    }

    return size;
}

int32_t
d_log_module_check(
    const struct dj_log_module*       _module,
    const struct dj_log_code**        _bad,
    struct dj_ostream_template_error* _error
)
{
    struct dj_ostream_template_op    ops[D_CFG_LOG_TEMPLATE_OPS];
    struct dj_ostream_template       compiled;
    struct dj_ostream_template_error error;
    struct dj_internal_log_fields    fields;
    size_t                           index;
    int32_t                          status;

    if (_bad != NULL)
    {
        *_bad = NULL;
    }

    if ( (_module == NULL) ||
         ( (_module->codes == NULL) && (_module->count != 0u) ) )
    {
        return (int32_t)D_OSTREAM_TEMPLATE_INVALID;
    }

    fields.facts = NULL;

    for (index = 0u; index < _module->count; ++index)
    {
        const struct dj_log_code* const code = _module->codes[index];

        if (code == NULL)
        {
            return (int32_t)D_OSTREAM_TEMPLATE_INVALID;
        }

        if (code->text == NULL)
        {
            continue;
        }

        fields.names = code->facts;
        status       = d_ostream_template_compile(
                           &compiled, ops, (size_t)D_CFG_LOG_TEMPLATE_OPS,
                           code->text, d_internal_log_lookup, &fields,
                           &error);

        if (status != (int32_t)D_OSTREAM_TEMPLATE_OK)
        {
            if (_bad != NULL)
            {
                *_bad = code;
            }

            if (_error != NULL)
            {
                *_error = error;
            }

            return status;
        }
    }

    return (int32_t)D_OSTREAM_TEMPLATE_OK;
}

/*
d_internal_log_round_reals
  log.real_digits: each finite real fact rounded to that many significant
digits before any writer sees it, so every format writes the same value; 0
keeps them whole. snprintf and strtod read one decimal point, the locale's,
so the text between them reads back as written.
*/
static void
d_internal_log_round_reals(
    struct dj_desc_collect_state* _facts,
    uint32_t                      _digits
)
{
    char   text[40];
    size_t index;

    if ( (_digits == 0u) || (_digits > 17u) )
    {
        return;
    }

    for (index = 0u; index < _facts->count; ++index)
    {
        struct dj_ostream_value* const value =
            &_facts->items[index].fact.value;

        // NaN and the infinities have no digits to round: x - x is NaN
        if ( (value->kind == (int32_t)D_OSTREAM_VALUE_REAL) &&
             ((value->real - value->real) == 0.0) )
        {
            (void)snprintf(text, sizeof(text), "%.*e", (int)_digits - 1,
                           value->real);
            value->real = strtod(text, NULL);
        }
    }

    return;
}

int
d_log_close(
    struct dj_log_record* _record
)
{
    char                        body[D_CFG_LOG_BODY_MAX];
    struct dj_log_settings      settings;
    struct dj_log_entry         entry;
    const struct dj_log_writer* writer;
    struct dj_ostream*          os;
    int                         failed = 0;

    if ( (_record == NULL) || (_record->open == 0u) )
    {
        return 0;
    }

    _record->open = 0u;
    d_log_settings(&settings, _record->code->module);
    d_internal_log_round_reals(&_record->collected, settings.real_digits);
    entry.code      = _record->code;
    entry.severity  = _record->severity;
    entry.level     = d_log_level_name(_record->severity);
    entry.time      = _record->time;
    entry.observed  = (settings.observed_time != 0u)
                          ? d_internal_log_clock_now()
                          : entry.time;
    entry.body_size = d_internal_log_body(_record, body, sizeof(body));
    entry.body      = body;
    entry.facts     = &_record->collected;
    entry.dropped   = _record->collected.dropped;
    entry.settings  = &settings;

    d_internal_log_lock();
    os     = d_internal_log_stream();
    writer = d_internal_log_writer(settings.format);

    if ( (os != NULL) && (writer != NULL) && (writer->record != NULL) )
    {
        (void)writer->record(d_internal_log.writer_context, os, &entry);
        (void)((writer->flush != NULL)
                   ? writer->flush(d_internal_log.writer_context, os)
                   : d_ostream_flush(os));

        if (d_ostream_is_ok(os) == 0)
        {
            ++d_internal_log.failures;
            failed = 1;
        }
    }

    d_internal_log_unlock();

    return failed;
}

int
d_log_emit(
    const struct dj_log_code*  _code,
    uint32_t                   _severity,
    const struct dj_desc_fact* _facts,
    size_t                     _count
)
{
    struct dj_log_record record;
    size_t               index;

    if (d_log_open(&record, _code, _severity) == 0)
    {
        return 0;
    }

    for (index = 0u; (_facts != NULL) && (index < _count); ++index)
    {
        (void)d_desc_emit(&record.sink, _facts[index].key, _facts[index].label,
                          _facts[index].value, _facts[index].layer,
                          _facts[index].importance, _facts[index].truth);
    }

    return d_log_close(&record);
}

#endif  // RE_STD_HAS_INT64
