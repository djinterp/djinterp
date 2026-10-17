/*******************************************************************************
* djinterp [parse]                                                  arg_common.h
*
* The argument parser's shared vocabulary.
*   Everything the parser's modules -- types, validators, options, commands,
* help, configuration files, the parser itself -- say to each other and to a
* caller: the status every fallible call returns, with CLI11's exit codes
* beside it; CLI11's enumerations, value for value; the forward declarations
* that let the record types refer to one another; the growable byte buffer
* that validators, messages and help are built in; the callback signatures;
* and the error report a parser keeps.
*   It is the module's one `_common` header in the framework's sense: every
* other header of the parser includes it, and the C++ faces read the same
* definitions rather than restating them.
*   Strings that cross this interface are NUL-terminated `const char*` or
* `struct d_pack_text` spans (c/util/sink_common.h); output goes to a
* `struct d_pack_sink`, so help can be printed, captured in a buffer, or
* appended to a C++ string by the same code.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STATUS
    ------
    1.  Status codes
         1.  d_arg_status
         2.  d_arg_exit_code
    2.  Status queries
         1.  d_arg_status_is_error
         2.  d_arg_status_name / d_arg_status_exit_code
2.  VOCABULARY
    ----------
    1.  Options
         1.  d_arg_multi_option_policy
         2.  d_arg_callback_priority
         3.  d_arg_integer_rule
    2.  Commands
         1.  d_arg_extras_mode
         2.  d_arg_config_extras_mode
         3.  d_arg_prefix_mode
         4.  d_arg_startup
    3.  Output
         1.  d_arg_help_mode
         2.  d_arg_config_output_mode
    4.  Input
         1.  d_arg_classifier
         2.  d_arg_quoting
    5.  Limits
         1.  D_ARG_EXPECTED_UNLIMITED
3.  TYPES
    -----
    1.  Records
         1.  Forward declarations
    2.  The byte buffer
         1.  d_arg_buffer
         2.  D_ARG_BUFFER_INIT
    3.  Callbacks
         1.  d_arg_release_fn
         2.  d_arg_copy_fn
         3.  d_arg_command_fn
         4.  d_arg_preparse_fn
         5.  d_arg_results_fn
         6.  d_arg_each_fn
         7.  d_arg_command_text_fn
         8.  d_arg_option_text_fn
         9.  d_arg_failure_fn
    4.  The error report
         1.  d_arg_error
         2.  D_ARG_ERROR_INIT
    5.  Lists
         1.  d_arg_string_list
         2.  d_arg_text_list
         3.  d_arg_extra
         4.  d_arg_extra_list
         5.  d_arg_option_list
         6.  d_arg_command_list
         7.  d_arg_validator_list
         8.  D_ARG_LIST_INIT
4.  OPERATIONS
    ----------
    1.  Buffers
         1.  d_arg_buffer_init / d_arg_buffer_release / d_arg_buffer_clear
         2.  d_arg_buffer_truncate
         3.  d_arg_buffer_append and kin
         4.  d_arg_buffer_detach
         5.  d_arg_buffer_text / d_arg_buffer_sink
    2.  Sinks
         1.  d_arg_sink_file / d_arg_sink_stdout / d_arg_sink_stderr
         2.  d_arg_sink_write / d_arg_sink_cstr / d_arg_sink_printf
    3.  Errors
         1.  d_arg_error_init / d_arg_error_release
         2.  d_arg_error_set / d_arg_error_setf
         3.  d_arg_error_message / d_arg_error_exit_code
    4.  Lists
         1.  d_arg_string_list_release / d_arg_text_list_release
         2.  d_arg_string_list_append
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMON_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMON_H 1

// std
#include <stddef.h>                          // size_t
#include <stdio.h>                           // FILE
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../../env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF


//==============================================================================
// 1.  STATUS
//==============================================================================
// Every fallible call returns an `int` holding one of the statuses below:
// D_ARG_OK, a completed request that ends the program successfully (help,
// version), or an error. The errors are CLI11's, one for one, and each maps
// to CLI11's exit code; the last three have no CLI11 counterpart because
// CLI11 throws std::bad_alloc or misbehaves where this API reports.


// 1.1    Status codes
//------------------------------------------------------------------------------
// 1.1.1
// d_arg_status
//   enum: what a call or a parse came to. Ordered so that the completed
// requests sit between success and the errors, and every error is at or
// above D_ARG_ERROR_INCORRECT_CONSTRUCTION.
enum d_arg_status
{
    D_ARG_OK                           = 0,
    D_ARG_CALL_FOR_HELP                = 1,
    D_ARG_CALL_FOR_ALL_HELP            = 2,
    D_ARG_CALL_FOR_VERSION             = 3,
    D_ARG_CALL_FOR_COMPLETION          = 4,
    D_ARG_SUCCESS                      = 5,
    D_ARG_RUNTIME_ERROR                = 6,
    D_ARG_ERROR_INCORRECT_CONSTRUCTION = 7,
    D_ARG_ERROR_BAD_NAME               = 8,
    D_ARG_ERROR_OPTION_ALREADY_ADDED   = 9,
    D_ARG_ERROR_FILE                   = 10,
    D_ARG_ERROR_CONVERSION             = 11,
    D_ARG_ERROR_VALIDATION             = 12,
    D_ARG_ERROR_REQUIRED               = 13,
    D_ARG_ERROR_REQUIRES               = 14,
    D_ARG_ERROR_EXCLUDES               = 15,
    D_ARG_ERROR_EXTRAS                 = 16,
    D_ARG_ERROR_CONFIG                 = 17,
    D_ARG_ERROR_INVALID                = 18,
    D_ARG_ERROR_HORRIBLE               = 19,
    D_ARG_ERROR_OPTION_NOT_FOUND       = 20,
    D_ARG_ERROR_ARGUMENT_MISMATCH      = 21,
    D_ARG_ERROR_PARSE                  = 22,
    D_ARG_ERROR_OUT_OF_MEMORY          = 23,
    D_ARG_ERROR_BAD_ARGUMENT           = 24,
    D_ARG_ERROR_UNSUPPORTED            = 25,
    D_ARG_STATUS_COUNT                 = 26
};

// 1.1.2
// d_arg_exit_code
//   enum: CLI11's ExitCodes, numerically identical, so a program ported from
// CLI11 exits with the codes its scripts already test for. The completed
// requests exit with D_ARG_EXIT_SUCCESS; D_ARG_RUNTIME_ERROR exits with the
// code its callback chose; the three errors CLI11 lacks exit with
// D_ARG_EXIT_BASE_CLASS.
enum d_arg_exit_code
{
    D_ARG_EXIT_SUCCESS                = 0,
    D_ARG_EXIT_INCORRECT_CONSTRUCTION = 100,
    D_ARG_EXIT_BAD_NAME_STRING        = 101,
    D_ARG_EXIT_OPTION_ALREADY_ADDED   = 102,
    D_ARG_EXIT_FILE_ERROR             = 103,
    D_ARG_EXIT_CONVERSION_ERROR       = 104,
    D_ARG_EXIT_VALIDATION_ERROR       = 105,
    D_ARG_EXIT_REQUIRED_ERROR         = 106,
    D_ARG_EXIT_REQUIRES_ERROR         = 107,
    D_ARG_EXIT_EXCLUDES_ERROR         = 108,
    D_ARG_EXIT_EXTRAS_ERROR           = 109,
    D_ARG_EXIT_CONFIG_ERROR           = 110,
    D_ARG_EXIT_INVALID_ERROR          = 111,
    D_ARG_EXIT_HORRIBLE_ERROR         = 112,
    D_ARG_EXIT_OPTION_NOT_FOUND       = 113,
    D_ARG_EXIT_ARGUMENT_MISMATCH      = 114,
    D_ARG_EXIT_BASE_CLASS             = 127
};

// 1.2    Status queries
//------------------------------------------------------------------------------
D_EXTERN_C_BEGIN

// 1.2.1
// d_arg_status_is_error
/**
 * @brief Determines whether a status reports an error rather than success or
 *        a completed request.
 *
 * @param[in] _status  a d_arg_status value.
 * @return `true` for D_ARG_RUNTIME_ERROR and every D_ARG_ERROR_* status;
 *         `false` for D_ARG_OK and the completed requests.
 */
D_INLINE bool
d_arg_status_is_error(
    int _status
)
{
    return (_status >= (int)D_ARG_RUNTIME_ERROR);
}

// 1.2.2
// d_arg_status_name / d_arg_status_exit_code
//   pure lookups; neither allocates nor fails. A status outside the enum is
// named "Error" and exits with D_ARG_EXIT_BASE_CLASS.
const char* d_arg_status_name(int _status);
int         d_arg_status_exit_code(int _status);

D_EXTERN_C_END


//==============================================================================
// 2.  VOCABULARY
//==============================================================================
// CLI11's enumerations, in CLI11's order with CLI11's meanings, so a setting
// documented there means the same here.


// 2.1    Options
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_multi_option_policy
//   enum: what an option that was given more values than it expects keeps:
// fail (THROW), the last ones (TAKE_LAST, the default of a flag), the first
// ones, all of them joined by the delimiter or `\n`, all of them as given,
// their sum, or the last ones in reverse order.
enum d_arg_multi_option_policy
{
    D_ARG_MULTI_THROW      = 0,
    D_ARG_MULTI_TAKE_LAST  = 1,
    D_ARG_MULTI_TAKE_FIRST = 2,
    D_ARG_MULTI_JOIN       = 3,
    D_ARG_MULTI_TAKE_ALL   = 4,
    D_ARG_MULTI_SUM        = 5,
    D_ARG_MULTI_REVERSE    = 6
};

// 2.1.2
// d_arg_callback_priority
//   enum: when, during the processing that follows parsing, an option's
// callback runs: at the very start, before the requirements are checked,
// after them, or last; and in each case before or after the help flags are
// acted on (the PRE_HELP forms run before). An option's default is NORMAL,
// and the help, version and completion flags run at FIRST.
enum d_arg_callback_priority
{
    D_ARG_PRIORITY_FIRST_PRE_HELP            = 0,
    D_ARG_PRIORITY_FIRST                     = 1,
    D_ARG_PRIORITY_PRE_REQUIREMENTS_PRE_HELP = 2,
    D_ARG_PRIORITY_PRE_REQUIREMENTS          = 3,
    D_ARG_PRIORITY_NORMAL_PRE_HELP           = 4,
    D_ARG_PRIORITY_NORMAL                    = 5,
    D_ARG_PRIORITY_LAST_PRE_HELP             = 6,
    D_ARG_PRIORITY_LAST                      = 7
};

// 2.1.3
// d_arg_integer_rule
//   enum: how an integer with a leading zero reads. DECIMAL reads `010` as
// ten and needs `0o` for octal; C reads it as eight, as strtol with base 0
// and CLI11 do, so `08` is an error. Both take `0x`, `0o` and `0b`, and `_`
// and `'` between digits.
enum d_arg_integer_rule
{
    D_ARG_INTEGER_DECIMAL = 0,
    D_ARG_INTEGER_C       = 1
};

// 2.2    Commands
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_extras_mode
//   enum: what a command does with an argument it does not recognize: report
// it once parsing ends (ERROR, the default), report it at once, drop it, or
// keep it for remaining() -- alone, with the one argument after it, with
// every non-option argument after it, or as it is (CAPTURE).
enum d_arg_extras_mode
{
    D_ARG_EXTRAS_ERROR             = 0,
    D_ARG_EXTRAS_ERROR_IMMEDIATELY = 1,
    D_ARG_EXTRAS_IGNORE            = 2,
    D_ARG_EXTRAS_ASSUME_SINGLE     = 3,
    D_ARG_EXTRAS_ASSUME_MULTIPLE   = 4,
    D_ARG_EXTRAS_CAPTURE           = 5
};

// 2.2.2
// d_arg_config_extras_mode
//   enum: what a command does with a configuration-file entry no option
// claims: report it, ignore it (the default), ignore it and every unknown
// section, or keep it for remaining().
enum d_arg_config_extras_mode
{
    D_ARG_CONFIG_EXTRAS_ERROR      = 0,
    D_ARG_CONFIG_EXTRAS_IGNORE     = 1,
    D_ARG_CONFIG_EXTRAS_IGNORE_ALL = 2,
    D_ARG_CONFIG_EXTRAS_CAPTURE    = 3
};

// 2.2.3
// d_arg_prefix_mode
//   enum: whether a command stops parsing at an argument it does not
// recognize and keeps everything from there on for remaining(): never (OFF),
// only at `--`, at anything unrecognized, or at the first positional argument
// or `--`, still collecting unrecognized options before it as extras.
enum d_arg_prefix_mode
{
    D_ARG_PREFIX_OFF             = 0,
    D_ARG_PREFIX_SEPARATOR_ONLY  = 1,
    D_ARG_PREFIX_ON              = 2,
    D_ARG_PREFIX_POSITIONAL_ONLY = 3
};

// 2.2.4
// d_arg_startup
//   enum: whether a command or option group is enabled at the start of every
// parse: as it was left (STABLE), enabled, or disabled -- the mechanism of
// trigger_on and trigger_off.
enum d_arg_startup
{
    D_ARG_STARTUP_STABLE   = 0,
    D_ARG_STARTUP_ENABLED  = 1,
    D_ARG_STARTUP_DISABLED = 2
};

// 2.3    Output
//------------------------------------------------------------------------------
// 2.3.1
// d_arg_help_mode
//   enum: which help to produce: a command's own (NORMAL), with every
// subcommand expanded (ALL, for --help-all), or a subcommand's section of an
// expanded help (SUB).
enum d_arg_help_mode
{
    D_ARG_HELP_NORMAL = 0,
    D_ARG_HELP_ALL    = 1,
    D_ARG_HELP_SUB    = 2
};

// 2.3.2
// d_arg_config_output_mode
//   enum: which values a written configuration file holds: those given
// (ACTIVE), those and every default, or those and the defaults of the
// subcommands that were used.
enum d_arg_config_output_mode
{
    D_ARG_CONFIG_OUTPUT_ACTIVE                     = 0,
    D_ARG_CONFIG_OUTPUT_ALL_DEFAULTS               = 1,
    D_ARG_CONFIG_OUTPUT_ACTIVE_SUBCOMMAND_DEFAULTS = 2
};

// 2.4    Input
//------------------------------------------------------------------------------
// 2.4.1
// d_arg_classifier
//   enum: what an argument is taken to be: a positional argument or value
// (NONE), the `--` that ends options, `-x`, `--name`, `/name`, a subcommand's
// name, or the `++` that ends a subcommand. Kept with each unrecognized
// argument, so remaining() can hand back exactly what was not used.
enum d_arg_classifier
{
    D_ARG_CLASS_NONE                  = 0,
    D_ARG_CLASS_POSITIONAL_MARK       = 1,
    D_ARG_CLASS_SHORT                 = 2,
    D_ARG_CLASS_LONG                  = 3,
    D_ARG_CLASS_WINDOWS_STYLE         = 4,
    D_ARG_CLASS_SUBCOMMAND            = 5,
    D_ARG_CLASS_SUBCOMMAND_TERMINATOR = 6
};

// 2.4.2
// d_arg_quoting
//   enum: the rules that split a line of text into arguments: a POSIX
// shell's; the Microsoft C runtime's; CLI11's own, which its string parse
// uses (single, double and back quotes, backslash escapes); or GCC's and
// Clang's for response files, in which a backslash escapes the next
// character inside quotes as well as outside. NATIVE is the platform's
// choice between the first two, D_ENV_ARG_PARSER_QUOTING; NATIVE_RESPONSE
// is its choice for response files: the C runtime's on Windows, GCC's
// elsewhere.
enum d_arg_quoting
{
    D_ARG_QUOTING_POSIX           = D_ENV_ARG_PARSER_QUOTING_POSIX,
    D_ARG_QUOTING_WINDOWS         = D_ENV_ARG_PARSER_QUOTING_WINDOWS,
    D_ARG_QUOTING_CLI11           = 2,
    D_ARG_QUOTING_GNU             = 3,
    D_ARG_QUOTING_NATIVE          = D_ENV_ARG_PARSER_QUOTING,
    D_ARG_QUOTING_NATIVE_RESPONSE =
        ( (D_ARG_QUOTING_NATIVE == D_ARG_QUOTING_WINDOWS)
          ? D_ARG_QUOTING_WINDOWS
          : D_ARG_QUOTING_GNU )
};

// 2.5    Limits
//------------------------------------------------------------------------------
// 2.5.1
// D_ARG_EXPECTED_UNLIMITED
//   constant: the count meaning "as many as given" wherever a number of
// values is expected; CLI11's expected_max_vector_size, 2^29.
enum
{
    D_ARG_EXPECTED_UNLIMITED = (1 << 29)
};


//==============================================================================
// 3.  TYPES
//==============================================================================


// 3.1    Records
//------------------------------------------------------------------------------
// 3.1.1
// Forward declarations
//   struct: the records of the other headers, and this header's error report,
// declared here so that every header -- and every callback below -- can name
// every record.
struct d_arg_command;
struct d_arg_error;
struct d_arg_formatter;
struct d_arg_option;
struct d_arg_parser;
struct d_arg_type;
struct d_arg_validator;

// 3.2    The byte buffer
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_buffer
//   struct: a growable run of bytes, kept NUL-terminated, in which messages,
// transformed values and help text are built. A refused allocation sets
// `failed` and leaves the content as it was, so a long run of appends checks
// once at the end; every later append to a failed buffer does nothing.
//   Public-payload layout: read `data` and `length` directly, write only
// through the d_arg_buffer_* calls.
struct d_arg_buffer
{
    char*  data;
    size_t length;
    size_t capacity;
    int    failed;
};

// 3.2.2
// D_ARG_BUFFER_INIT
//   constant: the initializer of an empty buffer that owns nothing, as
// d_arg_buffer_init leaves one: `struct d_arg_buffer text =
// D_ARG_BUFFER_INIT;`.
#define D_ARG_BUFFER_INIT { NULL, 0u, 0u, 0 }

// 3.3    Callbacks
//------------------------------------------------------------------------------
//   Each callback receives the context it was registered with. Those that
// return `int` return a d_arg_status: D_ARG_OK to go on, D_ARG_SUCCESS or
// D_ARG_RUNTIME_ERROR to end the program (CLI11's Success and RuntimeError),
// or an error, ideally after describing it with d_arg_command_fail. The
// typedefs sit outside the C linkage block, so in C++ they are ordinary
// function types that a C++ function converts to.

// 3.3.1
// d_arg_release_fn
//   type: frees a callback's context when the record holding it is released,
// replaced, or removed; registered beside the context. NULL releases nothing.
// A call given a context with a release function owns the context from then
// on: it frees it with the record that keeps it, when another context
// replaces it -- one given again is kept, not freed -- or at once when the
// call fails or has no record to keep it in.
typedef void (*d_arg_release_fn)(void* _context);

// 3.3.2
// d_arg_copy_fn
//   type: duplicates a context for a record that is itself copied -- a
// validator, which options hold by value -- returning the copy, which the
// matching d_arg_release_fn frees, or NULL when it could not be made.
typedef void* (*d_arg_copy_fn)(const void* _context);

// 3.3.3
// d_arg_command_fn
//   type: a command's parse-complete or final callback.
typedef int (*d_arg_command_fn)(struct d_arg_command* _command,
                                void*                 _context);

// 3.3.4
// d_arg_preparse_fn
//   type: a command's pre-parse callback, told how many arguments remain
// after the one that started the command.
typedef int (*d_arg_preparse_fn)(struct d_arg_command* _command,
                                 size_t                _remaining,
                                 void*                 _context);

// 3.3.5
// d_arg_results_fn
//   type: an option's callback, given the option's values after validation and
// its multi-option policy; CLI11's callback_t. Reporting D_ARG_ERROR_CONVERSION
// without a message lets the parser write CLI11's "Could not convert".
typedef int (*d_arg_results_fn)(struct d_arg_option*      _option,
                                const struct d_pack_text* _results,
                                size_t                    _count,
                                void*                     _context);

// 3.3.6
// d_arg_each_fn
//   type: a function run on each value of an option as the option is
// validated; CLI11's each(). D_ARG_ERROR_VALIDATION rejects the value, with
// the message d_arg_option_fail recorded or else the status's own, as
// CLI11's option catches a ValidationError; any other status but D_ARG_OK
// ends the parse with it, as any other error an each() function throws.
typedef int (*d_arg_each_fn)(const struct d_arg_option* _option,
                             struct d_pack_text         _value,
                             void*                      _context);

// 3.3.7
// d_arg_command_text_fn
//   type: writes text that belongs to a command -- its usage line, footer, or
// version -- into a sink when it is needed rather than when it is defined.
typedef int (*d_arg_command_text_fn)(const struct d_arg_command* _command,
                                     struct d_pack_sink          _sink,
                                     void*                       _context);

// 3.3.8
// d_arg_option_text_fn
//   type: writes text that belongs to an option -- its type name or its
// default value -- into a sink when help asks for it.
typedef int (*d_arg_option_text_fn)(const struct d_arg_option* _option,
                                    struct d_pack_sink         _sink,
                                    void*                      _context);

// 3.3.9
// d_arg_failure_fn
//   type: writes the message a failed parse prints; CLI11's failure_message.
// d_arg_failure_simple and d_arg_failure_help are the two CLI11 provides.
typedef int (*d_arg_failure_fn)(const struct d_arg_command* _command,
                                const struct d_arg_error*   _error,
                                struct d_pack_sink          _sink,
                                void*                       _context);

// 3.4    The error report
//------------------------------------------------------------------------------
// 3.4.1
// d_arg_error
//   struct: what the last failure was, where it arose, and what to say about
// it. `message` is owned and NUL-terminated, or NULL when none could be
// built, in which case d_arg_error_message supplies the status's own text.
// `exit_code` is the status's CLI11 exit code, or a runtime error's chosen
// one. `command` and `option` are where it arose, either NULL when unknown,
// and set to NULL in a parser's reports when what they point at is
// released. A parser keeps one, and a definition error stays in it -- the
// first one is never overwritten by a later one -- until the next parse
// reports it.
struct d_arg_error
{
    int                         status;
    int                         exit_code;
    char*                       message;
    const struct d_arg_command* command;
    const struct d_arg_option*  option;
};

// 3.4.2
// D_ARG_ERROR_INIT
//   constant: the initializer of an error reporting D_ARG_OK, as
// d_arg_error_init leaves one.
#define D_ARG_ERROR_INIT { 0, 0, NULL, NULL, NULL }

// 3.5    Lists
//------------------------------------------------------------------------------
//   The growable arrays the records keep their parts in: `items` holds `count`
// elements in room for `capacity`, and is NULL while `capacity` is 0. Each
// is public-payload layout: read `items` and `count` directly; only the
// module's calls grow, shrink or free them.

// 3.5.1
// d_arg_string_list
//   struct: NUL-terminated strings, each its own heap block, owned by the
// list: names, aliases, and the copies a record keeps of values.
struct d_arg_string_list
{
    char** items;
    size_t count;
    size_t capacity;
};

// 3.5.2
// d_arg_text_list
//   struct: spans that borrow their bytes -- from `argv`, or from storage some
// d_arg_string_list owns -- and are NUL-terminated at `data[length]`: an
// option's values, as given and as reduced.
struct d_arg_text_list
{
    struct d_pack_text* items;
    size_t              count;
    size_t              capacity;
};

// 3.5.3
// d_arg_extra
//   struct: an argument a command did not use, with what it was taken to be
// (a d_arg_classifier value): one entry of remaining().
struct d_arg_extra
{
    struct d_pack_text text;
    int                classifier;
};

// 3.5.4
// d_arg_extra_list
//   struct: the arguments a command did not use, in the order met.
struct d_arg_extra_list
{
    struct d_arg_extra* items;
    size_t              count;
    size_t              capacity;
};

// 3.5.5
// d_arg_option_list
//   struct: options a record refers to without owning them -- the ones an
// option needs or excludes, the parse order -- or, in a command's own list,
// the options it owns.
struct d_arg_option_list
{
    struct d_arg_option** items;
    size_t                count;
    size_t                capacity;
};

// 3.5.6
// d_arg_command_list
//   struct: commands a record refers to, or, in a command's own list, the
// subcommands and option groups it owns.
struct d_arg_command_list
{
    struct d_arg_command** items;
    size_t                 count;
    size_t                 capacity;
};

// 3.5.7
// d_arg_validator_list
//   struct: validators held by value, in the order they run: an option's.
struct d_arg_validator_list
{
    struct d_arg_validator* items;
    size_t                  count;
    size_t                  capacity;
};

// 3.5.8
// D_ARG_LIST_INIT
//   constant: the initializer of an empty list of any of the kinds above,
// which owns nothing: `struct d_arg_string_list names = D_ARG_LIST_INIT;`.
#define D_ARG_LIST_INIT { NULL, 0u, 0u }


//==============================================================================
// 4.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 4.1    Buffers
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_buffer_init / d_arg_buffer_release / d_arg_buffer_clear
//   lifetime: init makes an empty buffer that owns nothing; release frees the
// storage and leaves an empty buffer; clear empties it and keeps the storage.
// None fails, and each accepts NULL.
void        d_arg_buffer_init(struct d_arg_buffer* _buffer);
void        d_arg_buffer_release(struct d_arg_buffer* _buffer);
void        d_arg_buffer_clear(struct d_arg_buffer* _buffer);

// 4.1.2
// d_arg_buffer_truncate
//   shortening: cuts the content back to its first `_length` bytes, keeping
// the storage and the terminator; a longer `_length` changes nothing. Does
// not fail, and accepts NULL.
void        d_arg_buffer_truncate(struct d_arg_buffer* _buffer,
                                  size_t               _length);

// 4.1.3
// d_arg_buffer_append and kin
/**
 * @brief Appends bytes to a buffer, growing it as needed.
 *
 * @note Every append keeps the buffer NUL-terminated, and an append to a
 *       buffer whose `failed` is set does nothing and reports the failure
 *       again.
 *
 * @param[in,out] _buffer  the buffer to append to.
 * @param[in]     _data    the bytes; may be NULL when `_length` is 0.
 * @param[in]     _length  how many bytes to append.
 * @post   on failure `_buffer->failed` is set and the content is unchanged.
 * @retval D_ARG_OK                   the bytes were appended.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  storage could not be grown.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_buffer` is NULL, or `_data` is NULL
 *                                    with a nonzero `_length`.
 */
int         d_arg_buffer_append(struct d_arg_buffer* _buffer,
                                const char*          _data,
                                size_t               _length);
/**
 * @brief Appends a NUL-terminated string, as d_arg_buffer_append.
 *
 * @param[in,out] _buffer  the buffer to append to.
 * @param[in]     _text    the string; NULL appends nothing.
 * @return a status, as d_arg_buffer_append's.
 */
int         d_arg_buffer_append_cstr(struct d_arg_buffer* _buffer,
                                     const char*          _text);
/**
 * @brief Appends a span, as d_arg_buffer_append.
 *
 * @param[in,out] _buffer  the buffer to append to.
 * @param[in]     _text    the span.
 * @return a status, as d_arg_buffer_append's.
 */
int         d_arg_buffer_append_text(struct d_arg_buffer* _buffer,
                                     struct d_pack_text   _text);
/**
 * @brief Appends one character, as d_arg_buffer_append.
 *
 * @param[in,out] _buffer     the buffer to append to.
 * @param[in]     _character  the character.
 * @return a status, as d_arg_buffer_append's.
 */
int         d_arg_buffer_append_char(struct d_arg_buffer* _buffer,
                                     char                 _character);
/**
 * @brief Appends `printf`-style formatted text.
 *
 * @param[in,out] _buffer  the buffer to append to.
 * @param[in]     _format  the format string.
 * @return a status, as d_arg_buffer_append's; D_ARG_ERROR_BAD_ARGUMENT also
 *         when the format cannot be expanded.
 */
int         d_arg_buffer_printf(struct d_arg_buffer* _buffer,
                                const char*          _format,
                                ...) D_FORMAT_PRINTF(2, 3);

// 4.1.4
// d_arg_buffer_detach
/**
 * @brief Takes ownership of a buffer's storage.
 *
 * @param[in,out] _buffer  the buffer to take the storage of.
 * @post   `_buffer` is empty and owns nothing.
 * @return the NUL-terminated content, which the caller frees with free();
 *         NULL when the buffer holds no storage or `_buffer` is NULL.
 */
char*       d_arg_buffer_detach(struct d_arg_buffer* _buffer);

// 4.1.5
// d_arg_buffer_text / d_arg_buffer_sink
//   views: text spans the current content, valid until the next change; sink
// appends whatever is written to it, reporting a short write once `failed`
// is set. Neither allocates.
struct d_pack_text d_arg_buffer_text(const struct d_arg_buffer* _buffer);
struct d_pack_sink d_arg_buffer_sink(struct d_arg_buffer* _buffer);

// 4.2    Sinks
//------------------------------------------------------------------------------
// 4.2.1
// d_arg_sink_file / d_arg_sink_stdout / d_arg_sink_stderr
//   sinks over a C stream, which they write with fwrite and neither open nor
// close. A NULL stream gives a sink that refuses every write.
struct d_pack_sink d_arg_sink_file(FILE* _stream);
struct d_pack_sink d_arg_sink_stdout(void);
struct d_pack_sink d_arg_sink_stderr(void);

// 4.2.2
// d_arg_sink_write / d_arg_sink_cstr / d_arg_sink_printf
//   writing: each returns D_ARG_OK when the sink took every byte, and
// D_ARG_ERROR_FILE when it refused any; NULL text writes nothing, but NULL
// data with a nonzero length is D_ARG_ERROR_BAD_ARGUMENT. d_arg_sink_printf
// also returns D_ARG_ERROR_OUT_OF_MEMORY when its text could not be
// formatted, and D_ARG_ERROR_BAD_ARGUMENT when the format cannot be
// expanded.
int                d_arg_sink_write(struct d_pack_sink _sink,
                                    const char*        _data,
                                    size_t             _length);
int                d_arg_sink_cstr(struct d_pack_sink _sink,
                                   const char*        _text);
int                d_arg_sink_printf(struct d_pack_sink _sink,
                                     const char*        _format,
                                     ...) D_FORMAT_PRINTF(2, 3);

// 4.3    Errors
//------------------------------------------------------------------------------
// 4.3.1
// d_arg_error_init / d_arg_error_release
//   lifetime: init makes an error reporting D_ARG_OK; release frees its
// message and makes it D_ARG_OK again. Neither fails.
void        d_arg_error_init(struct d_arg_error* _error);
void        d_arg_error_release(struct d_arg_error* _error);

// 4.3.2
// d_arg_error_set / d_arg_error_setf
/**
 * @brief Records a failure, replacing whatever the error held.
 *
 * @param[in,out] _error    the error to record into.
 * @param[in]     _status   the d_arg_status to record.
 * @param[in]     _command  the command the failure arose in; may be NULL.
 * @param[in]     _option   the option concerned; may be NULL.
 * @param[in]     _message  the message, which is copied; NULL for none.
 * @post   `_error->exit_code` is `_status`'s exit code. When the message
 *         cannot be copied, `_error->message` is NULL and the status stands.
 * @return `_status`, so a caller may `return d_arg_error_set(...)`.
 */
int         d_arg_error_set(struct d_arg_error*         _error,
                            int                         _status,
                            const struct d_arg_command* _command,
                            const struct d_arg_option*  _option,
                            const char*                 _message);
/**
 * @brief Records a failure with a `printf`-style message, as d_arg_error_set
 *        does with no command or option.
 *
 * @param[in,out] _error   the error to record into.
 * @param[in]     _status  the d_arg_status to record.
 * @param[in]     _format  the message's format string; NULL for none.
 * @return `_status`.
 */
int         d_arg_error_setf(struct d_arg_error* _error,
                             int                 _status,
                             const char*         _format,
                             ...) D_FORMAT_PRINTF(3, 4);

// 4.3.3
// d_arg_error_message / d_arg_error_exit_code
//   reading: message is never NULL -- the recorded text, or the status's own
// when none was recorded -- and exit_code is the code to end the process
// with. Both accept NULL, which reads as D_ARG_OK.
const char* d_arg_error_message(const struct d_arg_error* _error);
int         d_arg_error_exit_code(const struct d_arg_error* _error);

// 4.4    Lists
//------------------------------------------------------------------------------
// 4.4.1
// d_arg_string_list_release / d_arg_text_list_release
//   lifetime: free what a list owns -- every string and the array of a string
// list, the array of a text list -- and leave it empty. A zeroed list is an
// empty one, so a caller's list needs no initialization. Neither fails, and
// each ignores NULL.
void        d_arg_string_list_release(struct d_arg_string_list* _list);
void        d_arg_text_list_release(struct d_arg_text_list* _list);

// 4.4.2
// d_arg_string_list_append
/**
 * @brief Appends a copy of a span to a string list.
 *
 * @param[in,out] _list  the list.
 * @param[in]     _text  the text, copied with a terminator; it may hold NUL
 *                       bytes.
 * @retval D_ARG_OK                   appended.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the copy or the array could not grow;
 *                                    the list is unchanged.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_list` is NULL.
 */
int         d_arg_string_list_append(struct d_arg_string_list* _list,
                                     struct d_pack_text        _text);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMON_H
