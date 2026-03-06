/******************************************************************************
* djinterp [test]                                              test_session.h
*
*   Test session - top-level test runner and coordinator.
*   A session manages the entire test execution lifecycle including:
*   - Global configuration
*   - Test discovery and registration
*   - Execution coordination
*   - Result aggregation and reporting
*   - Output formatting (console or file)
*
*   The session is the highest level in DTest hierarchy:
*     session -> module -> block -> test -> assertion/test_fn
*
*   Each level only knows about itself and its children, with configuration
*   flowing downward through run functions.
*
* 
* path:      \inc\test\test_session.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.25
******************************************************************************/

#ifndef DJINTERP_TEST_SESSION_
#define DJINTERP_TEST_SESSION_ 1

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "../djinterp.h"
#include "../dfile.h"
#include "../dstring.h"
#include "../dtime.h"
#include "../container/map/min_enum_map.h"
#include "../container/vector/ptr_vector.h"
#include "./test_module.h"
#include "./test_stats.h"


/******************************************************************************
 * D_TEST_SESSION MACRO
 *****************************************************************************/

// D_INTERNAL_SESSION_CONFIG_FROM_ARGS
//   macro: converts parenthesized args to a test config.
#define D_INTERNAL_SESSION_CONFIG_FROM_ARGS(paren_args)                      \
    d_test_session_validate_args(                                            \
        D_INTERNAL_TEST_ARG_ARRAY(paren_args),                               \
        D_INTERNAL_TEST_ARG_COUNT(paren_args)                                \
    )

// D_INTERNAL_SESSION_ARGS
//   macro: creates a session with config args and module children.
#define D_INTERNAL_SESSION_ARGS(paren_args, ...)                             \
    d_test_session_new_with_modules_and_config(                              \
        D_INTERNAL_SESSION_CONFIG_FROM_ARGS(paren_args),                     \
        D_INTERNAL_SESSION_MODULE_ARRAY(__VA_ARGS__),                        \
        D_VARG_COUNT(__VA_ARGS__)                                            \
    )

// D_INTERNAL_SESSION_NOARGS
//   macro: creates a session with module children only (no config).
#define D_INTERNAL_SESSION_NOARGS(...)                                       \
    d_test_session_new_with_modules(                                         \
        D_INTERNAL_SESSION_MODULE_ARRAY(__VA_ARGS__),                        \
        D_VARG_COUNT(__VA_ARGS__)                                            \
    )

// D_TEST_SESSION
//   macro: creates a test session with optional config and module children.
// Usage:
//   D_TEST_SESSION(module1, module2, ...)
//   D_TEST_SESSION(({ "name", "My Suite" }), module1, module2, ...)
#define D_TEST_SESSION(...)                                                  \
    D_INTERNAL_OPTARGS_DISPATCH(                                             \
        D_INTERNAL_SESSION_ARGS,                                             \
        D_INTERNAL_SESSION_NOARGS,                                           \
        __VA_ARGS__                                                          \
    )

// D_INTERNAL_SESSION_MODULE_ARRAY
//   macro: creates a module array, handling the empty case.
#define D_INTERNAL_SESSION_MODULE_ARRAY(...)                                 \
    D_CONCAT(D_INTERNAL_SESSION_MODULE_ARRAY_,                               \
             D_VARG_IS_EMPTY(__VA_ARGS__))(__VA_ARGS__)

// Empty case: return NULL
#define D_INTERNAL_SESSION_MODULE_ARRAY_1(...) NULL

// Non-empty case: return array of module pointers
#define D_INTERNAL_SESSION_MODULE_ARRAY_0(...)                               \
    (struct d_test_type*[]){ __VA_ARGS__ }


/******************************************************************************
 * SESSION STATUS
 *****************************************************************************/

// DTestSessionStatus
//   enum: status of a test session.
enum DTestSessionStatus
{
    D_TEST_SESSION_STATUS_UNKNOWN    = 0,
    D_TEST_SESSION_STATUS_CREATED    = 1,   // session created, not started
    D_TEST_SESSION_STATUS_RUNNING    = 2,   // currently executing
    D_TEST_SESSION_STATUS_PAUSED     = 3,   // execution paused
    D_TEST_SESSION_STATUS_COMPLETED  = 4,   // all tests completed
    D_TEST_SESSION_STATUS_ABORTED    = 5,   // execution aborted
    D_TEST_SESSION_STATUS_ERROR      = 6    // error occurred
};


/******************************************************************************
 * SESSION OPTIONS
 *****************************************************************************/

// DTestSessionOption
//   enum: session configuration options stored in settings map.
enum DTestSessionOption
{
    // execution control
    D_TEST_SESSION_OPT_ABORT_ON_FAILURE = 0x01,  // stop on first failure (bool)
    D_TEST_SESSION_OPT_FAIL_FAST        = 0x02,  // exit after N failures (size_t)
    D_TEST_SESSION_OPT_REPEAT_COUNT     = 0x03,  // repeat tests N times (size_t)
    D_TEST_SESSION_OPT_TIMEOUT_MS       = 0x04,  // global timeout in ms (size_t)
    
    // randomization
    D_TEST_SESSION_OPT_SHUFFLE          = 0x05,  // randomize test order (bool)
    D_TEST_SESSION_OPT_SHUFFLE_SEED     = 0x06,  // random seed (unsigned int)
    
    // parallelization (future)
    D_TEST_SESSION_OPT_PARALLEL         = 0x07,  // run in parallel (bool)
    D_TEST_SESSION_OPT_PARALLEL_WORKERS = 0x08,  // worker count (size_t)
    
    // output control
    D_TEST_SESSION_OPT_OUTPUT_FORMAT    = 0x10,  // DTestOutputFormat
    D_TEST_SESSION_OPT_OUTPUT_FILE      = 0x11,  // FILE* handle
    D_TEST_SESSION_OPT_FILENAME         = 0x12,  // output filename (const char*)
    D_TEST_SESSION_OPT_FILE_EXTENSION   = 0x13,  // default extension (const char*)
    D_TEST_SESSION_OPT_VERBOSE          = 0x14,  // verbosity level (int)
    D_TEST_SESSION_OPT_COLOR            = 0x15,  // use ANSI colors (bool)
    D_TEST_SESSION_OPT_SHOW_TIMESTAMPS  = 0x16,  // show timestamps (bool)
    D_TEST_SESSION_OPT_SHOW_DURATION    = 0x17,  // show test duration (bool)
    
    // filtering
    D_TEST_SESSION_OPT_FILTER_INCLUDE   = 0x20,  // include pattern (const char*)
    D_TEST_SESSION_OPT_FILTER_EXCLUDE   = 0x21,  // exclude pattern (const char*)
    D_TEST_SESSION_OPT_FILTER_TAGS      = 0x22,  // tag filter (const char*)
    
    // reporting
    D_TEST_SESSION_OPT_REPORT_PASSED    = 0x30,  // report passed tests (bool)
    D_TEST_SESSION_OPT_REPORT_SKIPPED   = 0x31,  // report skipped tests (bool)
    D_TEST_SESSION_OPT_REPORT_SUMMARY   = 0x32   // show summary at end (bool)
};


/******************************************************************************
 * OUTPUT FORMAT
 *****************************************************************************/

// DTestOutputFormat
//   enum: output format options.
enum DTestOutputFormat
{
    D_TEST_OUTPUT_CONSOLE = 0,   // human-readable console output
    D_TEST_OUTPUT_TEXT    = 1,   // plain text file output
    D_TEST_OUTPUT_VERBOSE = 2,   // verbose console output
    D_TEST_OUTPUT_MINIMAL = 3,   // minimal output (errors only)
    D_TEST_OUTPUT_SILENT  = 4    // no output (stats only)
};

// DTestVerbosity
//   enum: verbosity levels.
enum DTestVerbosity
{
    D_TEST_VERBOSITY_SILENT  = 0,  // no output
    D_TEST_VERBOSITY_MINIMAL = 1,  // errors only
    D_TEST_VERBOSITY_NORMAL  = 2,  // default level
    D_TEST_VERBOSITY_VERBOSE = 3,  // detailed output
    D_TEST_VERBOSITY_DEBUG   = 4   // all output including internals
};


/******************************************************************************
 * DEFAULT VALUES
 *****************************************************************************/

#define D_TEST_SESSION_DEFAULT_EXTENSION     ".txt"
#define D_TEST_SESSION_DEFAULT_VERBOSITY     D_TEST_VERBOSITY_NORMAL
#define D_TEST_SESSION_DEFAULT_OUTPUT_FORMAT D_TEST_OUTPUT_CONSOLE
#define D_TEST_SESSION_DEFAULT_REPEAT_COUNT  1
#define D_TEST_SESSION_DEFAULT_TIMEOUT_MS    0  // 0 = no timeout


/******************************************************************************
 * SESSION OUTPUT STATE
 *****************************************************************************/

// d_test_session_output
//   struct: manages output destination and formatting.
struct d_test_session_output
{
    FILE*                stream;          // output stream (stdout or file)
    enum DTestOutputFormat format;        // output format
    enum DTestVerbosity  verbosity;       // verbosity level
    bool                 owns_stream;     // true if we opened the file
    bool                 use_color;       // use ANSI color codes
    bool                 show_timestamps; // include timestamps
    bool                 show_duration;   // show test duration
    const char*          filename;        // output filename (if file)
    const char*          extension;       // default file extension
};


/******************************************************************************
 * SESSION STRUCTURE
 *****************************************************************************/

// d_test_session
//   struct: top-level DTest session manager.
struct d_test_session
{
    // test management - children are modules (d_test_type with type MODULE)
    struct d_ptr_vector*         children;       // registered modules

    // configuration
    struct d_test_options*       config;         // session configuration
    
    // output
    struct d_test_session_output output;        // output management
    
    // statistics
    struct d_test_statistics     stats;          // aggregated statistics
                                 
    // state                     
    enum DTestSessionStatus      status;         // current status
    size_t                       current_index;  // current test index
    size_t                       failure_count;  // failures so far
    size_t                       repeat_current; // current repeat iteration
                                 
    // timing                    
    double                       start_time_ms;  // session start time
    double                       end_time_ms;    // session end time
};


/******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR FUNCTIONS
 *****************************************************************************/

struct d_test_session* d_test_session_new(void);
struct d_test_session* d_test_session_new_with_config(struct d_test_options* _config);
struct d_test_session* d_test_session_new_with_modules(struct d_test_type** _modules,
                                                       size_t               _count);
struct d_test_session* d_test_session_new_with_modules_and_config(struct d_test_options* _config,
                                                                  struct d_test_type**  _modules,
                                                                  size_t                _count);
struct d_test_options* d_test_session_validate_args(struct d_test_arg* _args,
                                                   size_t             _arg_count);
/******************************************************************************
 * CONFIGURATION FUNCTIONS
 *****************************************************************************/

bool d_test_session_set_option(struct d_test_session*  _session,
                               enum DTestSessionOption _option,
                               const void*             _value);
void* d_test_session_get_option(const struct d_test_session* _session,
                                enum DTestSessionOption      _option);
bool d_test_session_set_output_format(struct d_test_session* _session,
                                      enum DTestOutputFormat _format);
bool d_test_session_set_output_file(struct d_test_session* _session,
                                    const char*            _filename);
bool d_test_session_set_verbosity(struct d_test_session* _session,
                                  enum DTestVerbosity    _verbosity);
bool d_test_session_enable_color(struct d_test_session* _session,
                                 bool                   _enable);


/******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

bool   d_test_session_add_child(struct d_test_session* _session,
                                struct d_test_type*    _child);
bool   d_test_session_add_children(struct d_test_session* _session,
                                   struct d_test_type**   _children,
                                   size_t                 _count);
size_t d_test_session_child_count(const struct d_test_session* _session);
struct d_test_type* d_test_session_get_child_at(const struct d_test_session* _session,
                                                size_t                       _index);
bool d_test_session_clear_children(struct d_test_session* _session);


/******************************************************************************
 * EXECUTION FUNCTIONS
 *****************************************************************************/

bool d_test_session_run(struct d_test_session* _session);

bool d_test_session_run_child(struct d_test_session* _session,
                              size_t                 _child_index);

bool d_test_session_run_by_name(struct d_test_session* _session,
                                const char*            _name);

bool d_test_session_pause(struct d_test_session* _session);
bool d_test_session_resume(struct d_test_session* _session);
bool d_test_session_abort(struct d_test_session* _session);
bool d_test_session_reset(struct d_test_session* _session);


/******************************************************************************
 * OUTPUT FUNCTIONS
 *****************************************************************************/

void d_test_session_write(struct d_test_session* _session,
                          const char*            _format,
                          ...);

void d_test_session_writeln(struct d_test_session* _session,
                            const char*            _format,
                            ...);

void d_test_session_write_header(struct d_test_session* _session);
void d_test_session_write_footer(struct d_test_session* _session);
void d_test_session_write_summary(struct d_test_session* _session);

void d_test_session_write_module_start(struct d_test_session*    _session,
                                       const struct d_test_type* _module);
void d_test_session_write_module_end(struct d_test_session*    _session,
                                     const struct d_test_type* _module,
                                     bool                      _passed);

void d_test_session_write_test_result(struct d_test_session* _session,
                                      const char*            _name,
                                      bool                   _passed,
                                      const char*            _message,
                                      double                 _duration_ms);

void d_test_session_flush(struct d_test_session* _session);


/******************************************************************************
 * STATISTICS FUNCTIONS
 *****************************************************************************/

const struct d_test_statistics* d_test_session_get_stats(
    const struct d_test_session* _session);

bool   d_test_session_all_passed(const struct d_test_session* _session);
size_t d_test_session_total_run(const struct d_test_session* _session);
size_t d_test_session_total_passed(const struct d_test_session* _session);
size_t d_test_session_total_failed(const struct d_test_session* _session);
size_t d_test_session_total_skipped(const struct d_test_session* _session);
double d_test_session_duration_ms(const struct d_test_session* _session);


/******************************************************************************
 * STATUS FUNCTIONS
 *****************************************************************************/

enum DTestSessionStatus d_test_session_get_status(
    const struct d_test_session* _session);

const char* d_test_session_status_to_string(enum DTestSessionStatus _status);

bool d_test_session_is_running(const struct d_test_session* _session);
bool d_test_session_is_complete(const struct d_test_session* _session);


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

void d_test_session_print(const struct d_test_session* _session);
int d_test_session_exit_code(const struct d_test_session* _session);

void d_test_session_free(struct d_test_session* _session);


#endif  // DJINTERP_TEST_SESSION_