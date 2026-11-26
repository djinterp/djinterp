/*******************************************************************************
* djinterp [c][log]                                                        log.h
*
*   The log: a record is an event, a message code and desc facts, written by
* a writer (docs/djinterp/log/log_design.md).
*     D_LOG(net, connect_timeout, D_LOG_FACT_TEXT("host", host),
*                                 D_LOG_FACT_INT("timeout_ms", ms));
* writes, in text,
*     2026-11-13T15:04:05.123Z warning net.connect_timeout: connecting to
*     db1 took over 500 ms host=db1 timeout_ms=500
* and the same record as JSON lines or OTLP/JSON, with OpenTelemetry's names.
*
*   A CODE SAYS WHAT HAPPENED. Each module keeps a table of its codes beside
* it - a number, a name, a class, a grade, a template and the facts the
* template names - written once as a macro of rows (section 2.4); the codes'
* constants and their table come from it. The class says what kind of fault
* it is, from one vocabulary across modules; the grade what the event did,
* which sets the record's level (D_CFG_LOG_GRADE_LEVEL, log.grade_<grade>).
* Text may be reworded; a code's meaning may not.
*
*   THE LEVEL CHECK COMES FIRST, AND COSTS A CACHED READ. A record below the
* threshold - log.<module>.level, else log.level, else D_CFG_LOG_LEVEL --
* builds nothing; one below D_CFG_LOG_LEVEL_MIN is not even compiled. Facts
* are collected on the caller's stack through desc's collecting machine, up
* to log.facts_max and log.depth_max; past them, counted. The body is the
* code's template written from the facts. Nothing allocates.
*
*   A WRITER IS A MACHINE: begin, record, end, flush, which a program may
* replace (d_log_set_writer). Text, JSON lines and OTLP/JSON are built in.
* The destination is a stream: standard error, a file, or none
* (log.destination), or a program's own (d_log_set_stream). With
* D_CFG_LOG_THREADSAFE a lock keeps records whole; call d_log_init before
* the program starts its threads.
*
*   The log's djin, "log", holds its settings (section 5.2); each has a knob,
* an environment variable and a command-line option. Each module whose codes
* are registered is a scope of it, so log.net.level sets net's alone.
*
*   On the option core's 64-bit floor, as option_env.h is.
*
*
* path:      /inc/djinterp/c/log/log.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LEVELS
    ------
    1.  D_LOG_TRACE .. D_LOG_FATAL
    2.  dj_log_level
    3.  Finding and adding levels
2.  CODES
    -----
    1.  dj_log_class
    2.  dj_log_grade
    3.  dj_log_code, dj_log_module
    4.  Code tables
    5.  Finding codes
3.  RECORDS
    -------
    1.  Facts
    2.  dj_log_record
    3.  Writing a record
    4.  D_LOG
4.  WRITERS
    -------
    1.  D_LOG_FORMAT_*
    2.  dj_log_settings
    3.  dj_log_entry
    4.  dj_log_writer
    5.  The built-in writers
5.  THE LOG
    -------
    1.  Setting up
    2.  The djin
    3.  Destinations, writers and the clock
*/

#ifndef DJINTERP_C_LOG_H
#define DJINTERP_C_LOG_H 1

// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../djinterp.h"                           // framework root
#include "../desc/desc_machines.h"                 // facts, collecting
#include "../option/option_env_registry.h"         // the djin
#include "../text/ostream_template.h"              // values
#include "../util/ostream/ostream.h"               // struct dj_ostream
#include "../../config/c/log/cfg_log.h"            // D_CFG_LOG_*
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64, uint64_t

#if RE_STD_HAS_INT64


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  LEVELS
//==============================================================================

// 1.1
// D_LOG_TRACE .. D_LOG_FATAL
//   constants: the seven levels built in, by severity on OpenTelemetry's
// scale of 1 to 24, so a threshold is one comparison.
#define D_LOG_TRACE    1u
#define D_LOG_DEBUG    5u
#define D_LOG_INFO     9u
#define D_LOG_NOTICE  10u
#define D_LOG_WARNING 13u
#define D_LOG_ERROR   17u
#define D_LOG_FATAL   21u

// 1.2
// dj_log_level
//   struct: a level: its name and its severity.
struct dj_log_level
{
    const char* name;
    uint32_t    severity;
};

// 1.3    Finding and adding levels
//------------------------------------------------------------------------------
/**
 * @brief A level's severity by its name, ignoring case, or a severity
 *        written as a number.
 *
 * @param[in] _name  the name; need not end in a NUL.
 * @param[in] _size  its length.
 * @return the severity, 1 to 24; 0 for a name that is none.
 */
uint32_t d_log_level_find(const char* _name,
                          size_t      _size);

/**
 * @brief The name a record of a severity is written with: the level at it,
 *        or the nearest below it.
 *
 * @param[in] _severity  the severity.
 * @return the name; "unset" below every level.
 */
const char* d_log_level_name(uint32_t _severity);

/**
 * @brief Adds a level while the program runs, with D_CFG_LOG_LEVELS 3. The
 *        name is borrowed and must outlive the log.
 *
 * @param[in] _name      the name: an identifier.
 * @param[in] _severity  1 to 24.
 * @return 0 when added, or when it is already, the same; -1 for a bad name
 *         or severity, a name already at another severity, no room
 *         (D_CFG_LOG_LEVELS_MAX), or a build without the registry.
 */
int d_log_level_add(const char* _name,
                    uint32_t    _severity);


//==============================================================================
// 2.  CODES
//==============================================================================

// 2.1
// dj_log_class
//   enum: what kind of fault a code is, from one vocabulary across modules,
// so a reader treats every module's out-of-range index alike: the
// containers' table of 11-04, the syntax family, and the classes the log's
// decisions 57, 58, 66, 78 and 81 add. Each is written by its dotted name
// (d_log_class_name). A class is part of a code's meaning, fixed when it
// compiles and kept by the manifest; the tree's tables use only this
// vocabulary. A program's own tables may add classes of their own, by
// D_CFG_LOG_CLASSES_EXTRA (cfg_log.h), after these.
#define D_LOG_CLASSES(X)                                                       \
    X(NONE,                 "none")                                            \
    X(ACCESS_DENIED,        "access.denied")                                   \
    X(ACCESS_UNAUTHENTICATED, "access.unauthenticated")                        \
    X(ARGUMENT_NULL,        "argument.null")                                   \
    X(ARGUMENT_INVALID,     "argument.invalid")                                \
    X(ARGUMENT_UNMET,       "argument.unmet")                                  \
    X(GENERIC_CALLEE,       "generic.callee")                                  \
    X(HANDLE_RELEASED,      "handle.released")                                 \
    X(HANDLE_STALE,         "handle.stale")                                    \
    X(INTEGRITY_INVARIANT,  "integrity.invariant")                             \
    X(LIFECYCLE_UNINITIALIZED, "lifecycle.uninitialized")                      \
    X(LIFECYCLE_CLOSED,     "lifecycle.closed")                                \
    X(LIFECYCLE_FROZEN,     "lifecycle.frozen")                                \
    X(LOOKUP_NOT_FOUND,     "lookup.not_found")                                \
    X(POSITION_FROM_END,    "position.from_end")                               \
    X(POSITION_INDEX,       "position.index")                                  \
    X(POSITION_MISALIGNED,  "position.misaligned")                             \
    X(POSITION_OFFSET,      "position.offset")                                 \
    X(POSITION_RANGE,       "position.range")                                  \
    X(QUANTITY_ARITY,       "quantity.arity")                                  \
    X(QUANTITY_COUNT,       "quantity.count")                                  \
    X(QUANTITY_DEPTH,       "quantity.depth")                                  \
    X(QUANTITY_SIZE,        "quantity.size")                                   \
    X(RESOURCE_BUDGET,      "resource.budget")                                 \
    X(RESOURCE_IN_USE,      "resource.in_use")                                 \
    X(RESOURCE_MEMORY,      "resource.memory")                                 \
    X(SPAN_EMPTY,           "span.empty")                                      \
    X(SPAN_INVERTED,        "span.inverted")                                   \
    X(SPAN_OVERLAPPING,     "span.overlapping")                                \
    X(SYNC_BUSY,            "sync.busy")                                       \
    X(SYNC_WOULD_BLOCK,     "sync.would_block")                                \
    X(SYNC_DEADLOCK,        "sync.deadlock")                                   \
    X(TYPE_MISMATCH,        "type.mismatch")                                   \
    X(VALUE_NON_FINITE,     "value.non_finite")                                \
    X(VALUE_PRECISION,      "value.precision")                                 \
    X(VALUE_RANGE,          "value.range")                                     \
    X(SYNTAX_UNEXPECTED,    "syntax.unexpected")                               \
    X(SYNTAX_MISSING,       "syntax.missing")                                  \
    X(SYNTAX_UNBALANCED,    "syntax.unbalanced")                               \
    X(SYNTAX_AMBIGUOUS,     "syntax.ambiguous")                                \
    X(SYNTAX_ARITY,         "syntax.arity")                                    \
    X(CONSTRAINT_VIOLATED,  "constraint.violated")                             \
    X(OPERATION_UNSUPPORTED, "operation.unsupported")                          \
    X(IO_FAILED,            "io.failed")                                       \
    X(IO_TIMED_OUT,         "io.timed_out")                                    \
    X(IO_REFUSED,           "io.refused")                                      \
    X(IO_UNREACHABLE,       "io.unreachable")                                  \
    X(IO_RESET,             "io.reset")                                        \
    X(IO_WOULD_BLOCK,       "io.would_block")                                  \
    X(IO_INTERRUPTED,       "io.interrupted")                                  \
    X(FORMAT_MALFORMED,     "format.malformed")                                \
    X(FORMAT_UNSUPPORTED,   "format.unsupported")                              \
    X(FORMAT_TRUNCATED,     "format.truncated")                                \
    X(FORMAT_CHECKSUM,      "format.checksum")

#define D_INTERNAL_LOG_CLASS_ENUM(id, name)  D_LOG_CLASS_##id,

enum dj_log_class
{
    D_LOG_CLASSES(D_INTERNAL_LOG_CLASS_ENUM)
    D_CFG_LOG_CLASSES_EXTRA(D_INTERNAL_LOG_CLASS_ENUM)
    D_LOG_CLASS_COUNT
};

// 2.2
// dj_log_grade
//   enum: what the event did, which sets its record's level.
enum dj_log_grade
{
    D_LOG_GRADE_OK       = 0,  // done
    D_LOG_GRADE_NOTE     = 1,  // done, and worth saying
    D_LOG_GRADE_CAVEAT   = 2,  // done, with something to know
    D_LOG_GRADE_SOFT     = 3,  // an expected miss: not found, empty
    D_LOG_GRADE_REJECTED = 4,  // refused; nothing changed
    D_LOG_GRADE_FAILED   = 5,  // tried, and could not
    D_LOG_GRADE_BROKEN   = 6,  // an invariant does not hold
    D_LOG_GRADE_COUNT    = 7
};

// 2.3
// dj_log_code, dj_log_module
//   structs: one code, and the table a module's codes are in. A code's full
// name is "module.name"; with D_CFG_LOG_CODES 0 its name, template and facts
// are null, and it is written "module:number". A number is the value the
// module's own enum gives the code, sign and all (the log's decision 76):
// "db:-6".
struct dj_log_module;

struct dj_log_code
{
    const struct dj_log_module* module;    // its table
    const char*                 name;      // "connect_timeout"; may be null
    const char*                 text;      // its template; may be null
    const char*                 facts;     // "(host, timeout_ms)"; may be
                                           // null
    int32_t                     number;    // unique in its module; may be
                                           // negative
    uint16_t                    class_id;  // enum dj_log_class
    uint16_t                    grade;     // enum dj_log_grade
};

struct dj_log_module
{
    const char*                      name;   // "net", "parse.expr"
    const struct dj_log_code* const* codes;  // the table, in its order
    size_t                           count;
};

// 2.4    Code tables
//------------------------------------------------------------------------------
//   A module writes its codes once, as a macro of two parameters whose rows
// are X(m, number, name, class, grade, template, (facts...)), beside its
// header:
//     #define D_NET_LOG_CODES(X, m)                                       ..
//         X(m, 207, connect_timeout, RESOURCE_BUDGET, FAILED,               ..
//           "connecting to {host} took over {timeout_ms} ms",               ..
//           (host, timeout_ms))
// (each ".." a line's continuing backslash)
//     D_LOG_CODES_DECLARE(net, D_NET_LOG_CODES)          // in its header
//     D_LOG_CODES_DEFINE(net, "net", D_NET_LOG_CODES)    // in one source
// Class and grade are D_LOG_CLASS_* and D_LOG_GRADE_* without their prefix,
// checked by the compiler; the facts are the names the template may use.
// A module below the log keeps only the rows; the two lines are the log's,
// and the log registers its table itself (log_codes.h).
// The declaration gives d_log_code_<m>_<name> and d_log_module_<m>, and a
// code's grade as a constant, d_log_grade_<m>_<name>, for D_LOG.

// D_LOG_TEXT
//   macro: a code's text where names and templates are built
// (D_CFG_LOG_CODES), null where not. A module with its own knob,
// D_CFG_<MODULE>_LOG_CODES, defines its own the same way.
#if D_CFG_LOG_CODES
    #define D_LOG_TEXT(text)  (text)
#else
    #define D_LOG_TEXT(text)  NULL
#endif

#define D_INTERNAL_LOG_DECLARE_ROW(m, number, name, class_, grade, text,      \
                                   facts)                                     \
    extern const struct dj_log_code d_log_code_##m##_##name;                  \
    enum { d_log_grade_##m##_##name = D_LOG_GRADE_##grade };

#define D_INTERNAL_LOG_DEFINE_ROW(m, number, name, class_, grade, text,       \
                                  facts)                                      \
    const struct dj_log_code d_log_code_##m##_##name =                        \
    {                                                                         \
        &d_log_module_##m, D_LOG_TEXT(#name), D_LOG_TEXT(text),               \
        D_LOG_TEXT(#facts), (int32_t)(number),                                \
        (uint16_t)D_LOG_CLASS_##class_, (uint16_t)D_LOG_GRADE_##grade         \
    };

#define D_INTERNAL_LOG_POINTER_ROW(m, number, name, class_, grade, text,      \
                                   facts)                                     \
    &d_log_code_##m##_##name,

// D_LOG_CODES_DECLARE
//   macro: a module's codes and its table, declared, in its header.
#define D_LOG_CODES_DECLARE(m, table)                                         \
    extern const struct dj_log_module d_log_module_##m;                       \
    table(D_INTERNAL_LOG_DECLARE_ROW, m)

// D_LOG_CODES_DEFINE
//   macro: a module's codes and its table, defined, in one source; `name`
// is the module's name as text ("parse.expr" for m parse_expr).
#define D_LOG_CODES_DEFINE(m, name, table)                                    \
    table(D_INTERNAL_LOG_DEFINE_ROW, m)                                       \
    static const struct dj_log_code* const d_internal_log_codes_##m[] =       \
    {                                                                         \
        table(D_INTERNAL_LOG_POINTER_ROW, m)                                  \
    };                                                                        \
    const struct dj_log_module d_log_module_##m =                             \
    {                                                                         \
        (name), d_internal_log_codes_##m,                                     \
        sizeof(d_internal_log_codes_##m) / sizeof(d_internal_log_codes_##m[0])\
    };

// 2.5    Finding codes
//------------------------------------------------------------------------------
/**
 * @brief Registers a module's codes: d_log_code_find finds them, a debug
 *        build checks their event keys for a collision (the log's
 *        decision 21), and the module becomes a scope of the log's djin,
 *        so log.<module>.level may be set for it. Register before reading
 *        settings, and before the program starts its threads.
 *
 * @param[in] _module  the module's table; borrowed.
 * @return 0 when registered, or already; -1 for a null or bad table, a
 *         collision, no room (D_CFG_LOG_MODULES_MAX, D_CFG_LOG_SCOPES), or
 *         a module name whose scope would make two of the log's settings
 *         one environment name (option_env's decision 47).
 */
int d_log_module_add(const struct dj_log_module* _module);

/**
 * @brief A registered module's code, by the module's name and the code's
 *        number: what a subscriber that knows codes only at run time uses.
 *
 * @param[in] _module  the module's name; need not end in a NUL.
 * @param[in] _size    its length.
 * @param[in] _number  the code's number.
 * @return the code; null when there is none.
 */
const struct dj_log_code* d_log_code_find(const char* _module,
                                          size_t      _size,
                                          int32_t     _number);

/**
 * @brief A code's event key: FNV-1a 64 over its full name, "net.connect_
 *        timeout", or "net:207" where names are not built (the log's decision
 *        21 (c)).
 *
 * @param[in] _code  the code.
 * @return the key; 0 for a null code.
 */
uint64_t d_log_code_key(const struct dj_log_code* _code);

/**
 * @brief Checks a module's templates: each code's compiles against its list
 *        of facts. A template that does not compile is written as it is, so
 *        a module's own tests call this to catch one.
 *
 * @param[in]  _module  the module's table.
 * @param[out] _bad     the first code whose template fails; may be null.
 * @param[out] _error   where and why it fails; may be null.
 * @return D_OSTREAM_TEMPLATE_OK when every template compiles, or none is
 *         built (D_CFG_LOG_CODES 0); the first failure's status otherwise;
 *         D_OSTREAM_TEMPLATE_INVALID for a null or bad table.
 */
int32_t d_log_module_check(const struct dj_log_module*       _module,
                           const struct dj_log_code**        _bad,
                           struct dj_ostream_template_error* _error);

// a class's and a grade's names: "position.index", "rejected"; "?" for none
const char* d_log_class_name(uint32_t _class);
const char* d_log_grade_name(uint32_t _grade);


//==============================================================================
// 3.  RECORDS
//==============================================================================

// 3.1    Facts
//------------------------------------------------------------------------------
//   A record's own facts, written where D_LOG is: each a struct
// dj_desc_fact, on the state layer, observed. Text is borrowed until the
// record is written.
#define D_LOG_FACT(key, value)                                                 \
    { (key), NULL, (value), 0u, (uint16_t)D_DESC_LAYER_STATE, 0u,              \
      (uint8_t)D_DESC_TRUTH_OBSERVED }
#define D_LOG_FACT_TEXT(key, text)                                             \
    D_LOG_FACT((key), d_ostream_value_text(text))
#define D_LOG_FACT_INT(key, integer)                                           \
    D_LOG_FACT((key), d_ostream_value_int64((int64_t)(integer)))
#define D_LOG_FACT_UINT(key, integer)                                          \
    D_LOG_FACT((key), d_ostream_value_uint64((uint64_t)(integer)))
#define D_LOG_FACT_SIZE(key, size)                                             \
    D_LOG_FACT((key), d_ostream_value_size((size_t)(size)))
#define D_LOG_FACT_REAL(key, real)                                             \
    D_LOG_FACT((key), d_ostream_value_real((double)(real)))
#define D_LOG_FACT_BOOL(key, flag)                                             \
    D_LOG_FACT((key), d_ostream_value_bool((flag) ? 1 : 0))

// 3.2
// dj_log_record
//   struct: a record being made, on the caller's stack: its code, its
// level, and its facts, collected. Opened by d_log_open, written by
// d_log_close.
struct dj_log_record
{
    const struct dj_log_code*    code;
    uint32_t                     severity;
    uint32_t                     open;       // 1 between open and close
    uint64_t                     time;       // when it opened: the event's
    struct dj_desc_collect_state collected;
    struct dj_desc_sink          sink;
    struct dj_desc_item          items[D_CFG_LOG_FACTS_MAX];
};

// 3.3    Writing a record
//------------------------------------------------------------------------------
/**
 * @brief Whether a record of a code, at a severity, would be written: the
 *        threshold check, a cached read.
 *
 * @param[in] _code      the code.
 * @param[in] _severity  the record's level; 0 for its grade's, as the
 *                       options say.
 * @return 1 or 0.
 */
int d_log_wants(const struct dj_log_code* _code,
                uint32_t                  _severity);

/**
 * @brief Opens a record, if the log wants it, and takes its time, when
 *        the event happened: its facts then go to d_log_sink's sink --
 *        d_desc_emit, or any describer - up to log.facts_max and
 *        log.depth_max. A record may stay open across calls, as for an
 *        error whose note comes after it: no lock is held while it is.
 *
 * @param[out] _record    the record, on the stack or in its owner's state.
 * @param[in]  _code      the code.
 * @param[in]  _severity  its level; 0 for its grade's.
 * @return 1 when opened, and d_log_close must be called; 0 when the log
 *         does not want it, and nothing more is needed.
 */
int d_log_open(struct dj_log_record*     _record,
               const struct dj_log_code* _code,
               uint32_t                  _severity);

// the open record's sink, for its facts
struct dj_desc_sink* d_log_sink(struct dj_log_record* _record);

/**
 * @brief Writes an open record: its body from its code's template and its
 *        facts, then the writer's record, then a flush; and closes it. Its
 *        observed time, when the log saw it, is taken here.
 *
 * @param[in,out] _record  the record.
 * @return 0 when written; 1 when the destination failed, counted in
 *         d_log_failures.
 */
int d_log_close(struct dj_log_record* _record);

/**
 * @brief A record of flat facts, in one call: d_log_open, each fact, and
 *        d_log_close. What D_LOG expands to, and what a subscriber that
 *        knows its code only at run time calls (d_log_code_find).
 *
 * @param[in] _code      the code.
 * @param[in] _severity  its level; 0 for its grade's.
 * @param[in] _facts     the facts; may be null when _count is 0.
 * @param[in] _count     how many.
 * @return 0 when written or not wanted; 1 when the destination failed.
 */
int d_log_emit(const struct dj_log_code*  _code,
               uint32_t                   _severity,
               const struct dj_desc_fact* _facts,
               size_t                     _count);

// records the destination refused, and records not written whole
uint64_t d_log_failures(void);

// 3.4
// D_LOG
//   macro: one record of a code, at its grade's level, with flat facts:
// D_LOG(net, connect_timeout, D_LOG_FACT_TEXT("host", host)). The module's
// codes must be declared (D_LOG_CODES_DECLARE). A record whose level is
// below D_CFG_LOG_LEVEL_MIN compiles to nothing; one below the threshold
// builds nothing. It needs variadic macros: C99, or C++11; elsewhere call
// d_log_emit. The variadic definitions are the header's own, so the warning
// a strict build gives for each is quieted here, and only here.
D_ENV_PP_VARIADIC_QUIET_BEGIN
#if D_CFG_LOG_ENABLED &&                                                       \
    ( D_ENV_LANG_IS_CPP11_OR_HIGHER ||                                         \
      (!defined(__cplusplus) && D_ENV_LANG_IS_C99_OR_HIGHER) )
    #define D_LOG(m, ...)                                                      \
        D_INTERNAL_LOG(m, __VA_ARGS__, D_INTERNAL_LOG_END)
    #define D_INTERNAL_LOG_END  D_LOG_FACT(NULL, d_ostream_value_none())
    #define D_INTERNAL_LOG(m, code, ...)                                       \
        do                                                                     \
        {                                                                      \
            if ( (D_CFG_LOG_GRADE_LEVEL(d_log_grade_##m##_##code) >=           \
                  D_CFG_LOG_LEVEL_MIN) &&                                      \
                 (d_log_wants(&d_log_code_##m##_##code, 0u) != 0) )            \
            {                                                                  \
                const struct dj_desc_fact d_log_facts_[] = { __VA_ARGS__ };    \
                                                                               \
                (void)d_log_emit(&d_log_code_##m##_##code, 0u, d_log_facts_,   \
                                 (sizeof(d_log_facts_) /                       \
                                  sizeof(d_log_facts_[0])) - 1u);              \
            }                                                                  \
        }                                                                      \
        while (0)
#elif !D_CFG_LOG_ENABLED
    #define D_LOG(m, ...)  do { } while (0)
#endif
D_ENV_PP_VARIADIC_QUIET_END


//==============================================================================
// 4.  WRITERS
//==============================================================================

// 4.1
// D_LOG_FORMAT_*
//   constants: the built-in writers, log.format's values; also the bits of
// D_CFG_LOG_FORMATS, 1 << format.
#define D_LOG_FORMAT_TEXT  0u
#define D_LOG_FORMAT_JSON  1u
#define D_LOG_FORMAT_OTLP  2u

// 4.2
// dj_log_settings
//   struct: what the log's options say for a record, read once for it: what
// a writer writes by.
struct dj_log_settings
{
    uint32_t    format;           // D_LOG_FORMAT_*
    uint32_t    code_style;       // 0 name, 1 number, 2 both
    uint32_t    nesting;          // 0 nested, 1 dotted
    uint32_t    json_integers;    // 0 numbers, 1 strings
    uint32_t    json_nonfinite;   // 0 null, 1 strings
    uint32_t    time;             // D_CFG_LOG_TIME's values
    uint32_t    severity_number;  // 1 to write it
    uint32_t    scope;            // 1 to write the module as the scope
    uint32_t    resource;         // 1 to write the resource
    uint32_t    context;          // 1 to write trace and span ids
    uint32_t    observed_time;    // 1 to write the observed time
    uint32_t    dropped_count;    // 1 to write facts not kept
    uint32_t    real_digits;      // 0 all; else a real's significant
                                  // digits kept
    const char* service;          // the resource's service.name
};

// 4.3
// dj_log_entry
//   struct: a record as a writer is given it.
struct dj_log_entry
{
    const struct dj_log_code*           code;
    uint32_t                            severity;
    const char*                         level;      // its name
    uint64_t                            time;       // ns since 1970, UTC
    uint64_t                            observed;   // the same, when seen
    const char*                         body;       // the template, written
    size_t                              body_size;
    const struct dj_desc_collect_state* facts;      // its facts, collected
    size_t                              dropped;    // facts not kept
    const struct dj_log_settings*       settings;
};

// 4.4
// dj_log_writer
//   struct: a writer's operations, each of which may be null; each returns
// 0 to go on. begin and end bound a run of records (OTLP's batch); a record
// is written whole, then flushed.
struct dj_log_writer
{
    int (*begin)(void* _context, struct dj_ostream* _os);
    int (*record)(void*                      _context,
                  struct dj_ostream*         _os,
                  const struct dj_log_entry* _entry);
    int (*end)(void* _context, struct dj_ostream* _os);
    int (*flush)(void* _context, struct dj_ostream* _os);
};

// 4.5    The built-in writers
//------------------------------------------------------------------------------
//   Each is null where D_CFG_LOG_FORMATS leaves it out. Their context is
// unused: pass null.

// one line a record: time, level, code, body, then the facts as key=value
const struct dj_log_writer* d_log_writer_text(void);
// one JSON object a line, with OpenTelemetry's data model's names
const struct dj_log_writer* d_log_writer_json(void);
// one OTLP/JSON ExportLogsServiceRequest a line, as the specification's
// JSON encoding has it: what a collector's file receiver reads
const struct dj_log_writer* d_log_writer_otlp(void);

/**
 * @brief Writes a time as RFC 3339: UTC with "Z", or local with its offset,
 *        to 0, 3, 6 or 9 digits of a second (D_CFG_LOG_TIME's values).
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _time  ns since 1970, UTC.
 * @param[in]     _how   0 to 7.
 */
void d_log_write_time(struct dj_ostream* _os,
                      uint64_t           _time,
                      uint32_t           _how);


//==============================================================================
// 5.  THE LOG
//==============================================================================

// 5.1    Setting up
//------------------------------------------------------------------------------
/**
 * @brief Sets the log up: its lock, and its destination as log.destination
 *        says. Call it before the program starts its threads, after its
 *        settings are read; calling it again applies changed settings.
 *
 * @note With D_CFG_LOG_CODES_BELOW 1 it first registers the code tables of
 *       the modules below the log (log_codes.h), if d_log_register has not.
 *
 * @return 0; -1 when the destination could not be opened, the log then
 *         writing to standard error.
 */
int d_log_init(void);

/**
 * @brief Flushes the destination and ends the writer's run; closes a file
 *        the log opened.
 */
void d_log_shutdown(void);

// 5.2    The djin
//------------------------------------------------------------------------------
// D_LOG_VAR_*
//   constants: the djin's variables, by their place among its declarations.
#define D_LOG_VAR_LEVEL            0u
#define D_LOG_VAR_LEVELS           1u
#define D_LOG_VAR_FORMAT           2u
#define D_LOG_VAR_DESTINATION      3u
#define D_LOG_VAR_CODE_STYLE       4u
#define D_LOG_VAR_FACTS_MAX        5u
#define D_LOG_VAR_DEPTH_MAX        6u
#define D_LOG_VAR_NESTING          7u
#define D_LOG_VAR_GRADE_OK         8u   // .. D_LOG_VAR_GRADE_OK + 6, broken
#define D_LOG_VAR_SEVERITY_NUMBER 15u
#define D_LOG_VAR_SCOPE           16u
#define D_LOG_VAR_RESOURCE        17u
#define D_LOG_VAR_CONTEXT         18u
#define D_LOG_VAR_OBSERVED_TIME   19u
#define D_LOG_VAR_DROPPED_COUNT   20u
#define D_LOG_VAR_TIME            21u
#define D_LOG_VAR_JSON_INTEGERS   22u
#define D_LOG_VAR_JSON_NONFINITE  23u
#define D_LOG_VAR_SERVICE         24u
#define D_LOG_VAR_REAL_DIGITS     25u
#define D_LOG_VARS                26u

/**
 * @brief The log's djin: its frame, static, ready before the program runs;
 *        null where D_CFG_LOG_OPTIONS leaves it out.
 */
struct dj_option_env* d_log_djin(void);

/**
 * @brief Registers the log's djin, so the registry's settings reach it.
 *
 * @note With D_CFG_LOG_CODES_BELOW 1 it first registers the code tables of
 *       the modules below the log (log_codes.h), so log.<module>.level is
 *       there for each when settings are read.
 *
 * @param[in,out] _registry  the registry; null for the program's.
 * @return as d_option_env_registry_add; INVALID_ARGUMENT without the djin.
 */
struct dj_option_result d_log_register(
                            struct dj_option_env_registry* _registry);

/**
 * @brief What the options say for a module's records.
 *
 * @param[out] _settings  the settings.
 * @param[in]  _module    the module, whose scope applies; null for none.
 */
void d_log_settings(struct dj_log_settings*     _settings,
                    const struct dj_log_module* _module);

// 5.3    Destinations, writers and the clock
//------------------------------------------------------------------------------
/**
 * @brief Writes to a program's own stream instead of log.destination's,
 *        until set back with null. The stream is borrowed.
 */
void d_log_set_stream(struct dj_ostream* _os);

/**
 * @brief Writes with a program's own writer instead of log.format's, until
 *        set back with null.
 */
void d_log_set_writer(const struct dj_log_writer* _writer,
                      void*                       _context);

// fn_log_clock
//   function pointer: the time now, in ns since 1970, UTC.
typedef uint64_t (*fn_log_clock)(void* _context);

/**
 * @brief Takes the time from a program's own clock - a test fixes it --
 *        until set back with null.
 */
void d_log_set_clock(fn_log_clock _clock,
                     void*        _context);


D_EXTERN_C_END


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_C_LOG_H
