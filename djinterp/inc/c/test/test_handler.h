/******************************************************************************
* djinterp [test]                                                test_handler.h
*
*   Test handler - orchestrates test execution with event handling.
*
*   The test handler is the central coordinator for test execution. It:
*   - Manages test lifecycle events (setup, start, success, failure, teardown)
*   - Tracks test results and statistics
*   - Integrates with the event system for extensible behavior
*   - Coordinates execution of tests, blocks, modules, trees, and sessions
*   - Provides DSL macros for declarative test definition
*
* path:      \inc\test\test_handler.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.01.30
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_HANDLER_
#define DJINTERP_TEST_TEST_HANDLER_ 1

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../djinterp.h"
#include "../dmemory.h"
#include "../dfile.h"
#include "../dtime.h"
#include "../string_fn.h"
#include "../event/event.h"
#include "../event/event_handler.h"
#include "../test/test_options.h"
#include "../test/test_common.h"
#include "../test/assert.h"
#include "../test/test.h"
#include "../test/test_block.h"


// d_test_result
//   type: aggregated test execution statistics.
struct d_test_result
{
    // test function statistics
    size_t test_fns_run;
    size_t test_fns_passed;
    size_t test_fns_failed;
    size_t test_fns_skipped;

    // assertion statistics
    size_t assertions_run;
    size_t assertions_passed;
    size_t assertions_failed;

    // test statistics
    size_t tests_run;
    size_t tests_passed;
    size_t tests_failed;
    size_t tests_skipped;

    // block statistics
    size_t blocks_run;
    size_t blocks_passed;
    size_t blocks_failed;

    // module statistics
    size_t modules_run;
    size_t modules_passed;
    size_t modules_failed;
    size_t modules_skipped;

    // execution tracking
    size_t max_depth;
    size_t current_depth;
    double duration_ms;
};


// DTestHandlerFlag
//   enum: bit flags controlling test handler behavior.
enum DTestHandlerFlag
{
    D_TEST_HANDLER_FLAG_NONE           = 0,
    D_TEST_HANDLER_FLAG_ABORT_ON_FAIL  = 1 << 0,
    D_TEST_HANDLER_FLAG_TRACK_STACK    = 1 << 1,
    D_TEST_HANDLER_FLAG_EMIT_EVENTS    = 1 << 2,
    D_TEST_HANDLER_FLAG_VERBOSE        = 1 << 3,
    D_TEST_HANDLER_FLAG_SILENT         = 1 << 4,
    D_TEST_HANDLER_FLAG_TIME_TESTS     = 1 << 5,
    D_TEST_HANDLER_FLAG_CAPTURE_OUTPUT = 1 << 6
};


// d_test_handler
//   type: central coordinator for test execution, events, and statistics.
struct d_test_handler
{
    struct d_event_handler* event_handler;
    struct d_test_options*  default_config;
    struct d_test_result    results;
    struct d_min_stack*     result_stack;
    struct d_min_stack*     context_stack;

    uint32_t                flags;
    size_t                  current_depth;
    bool                    abort_on_failure;

    // output capture
    char*                   output_buffer;
    size_t                  output_capacity;
    size_t                  output_length;
    FILE*                   output_stream;
};


// d_test_context
//   type: snapshot of execution state at a particular point in the test
// hierarchy.
struct d_test_context
{
    struct d_test_handler*  handler;

    // current execution location
    struct d_test*          current_test;
    struct d_test_block*    current_block;
    struct d_test_module*   current_module;
    struct d_test_tree*     current_tree;
    struct d_test_session*  current_session;

    // execution state
    enum DTestEvent         event_type;
    size_t                  depth;
    struct d_assert*        last_assertion;

    // result at this context level
    bool                    result;
    double                  start_time_ms;
    double                  end_time_ms;
};


// d_test_kv
//   type: key-value pair used by the DSL for metadata.
struct d_test_kv
{
    const char* key;
    const char* value;
};


// I.    creation and destruction
struct d_test_handler* d_test_handler_new(struct d_test_options* _default_config);
struct d_test_handler* d_test_handler_new_with_events(struct d_test_options* _default_config, size_t _event_capacity);
struct d_test_handler* d_test_handler_new_full(struct d_test_options* _default_config, size_t _event_capacity, size_t _stack_capacity, uint32_t _flags);
void                   d_test_handler_free(struct d_test_handler* _handler);

// II.   flag management
void d_test_handler_set_flag(struct d_test_handler* _handler, enum DTestHandlerFlag _flag);
void d_test_handler_clear_flag(struct d_test_handler* _handler, enum DTestHandlerFlag _flag);
bool d_test_handler_has_flag(const struct d_test_handler* _handler, enum DTestHandlerFlag _flag);

// III.  event listener registration
bool d_test_handler_register_listener(struct d_test_handler* _handler, d_event_id _event_id, fn_callback _callback, bool _enabled);
bool d_test_handler_unregister_listener(struct d_test_handler* _handler, d_event_id _event_id);
bool d_test_handler_enable_listener(struct d_test_handler* _handler, d_event_id _id);
bool d_test_handler_disable_listener(struct d_test_handler* _handler, d_event_id _id);

// IV.   event emission
void d_test_handler_emit_event(struct d_test_handler* _handler, enum DTestEvent _event_type, struct d_test_context* _context);

// V.    test execution
bool d_test_handler_run_test(struct d_test_handler* _handler, struct d_test* _test, struct d_test_options* _run_config);
bool d_test_handler_run_block(struct d_test_handler* _handler, struct d_test_block* _block, struct d_test_options* _run_config);
bool d_test_handler_run_test_type(struct d_test_handler* _handler, struct d_test_type* _test_type, struct d_test_options* _run_config);
bool d_test_handler_run_module(struct d_test_handler* _handler, struct d_test_module* _module, struct d_test_options* _run_config);
bool d_test_handler_run_tree(struct d_test_handler* _handler, struct d_test_tree* _tree, struct d_test_options* _run_config);
bool d_test_handler_run_session(struct d_test_handler* _handler, struct d_test_session* _session);

// VI.   assertion tracking
void d_test_handler_record_assertion(struct d_test_handler* _handler, struct d_assert* _assertion);

// VII.  stack operations
bool d_test_handler_push_result(struct d_test_handler* _handler, const struct d_test_result* _result);
bool d_test_handler_pop_result(struct d_test_handler* _handler, struct d_test_result* _out);
bool d_test_handler_push_context(struct d_test_handler* _handler, const struct d_test_context* _context);
bool d_test_handler_pop_context(struct d_test_handler* _handler, struct d_test_context* _out);

// VIII. result queries
struct d_test_result d_test_handler_get_results(const struct d_test_handler* _handler);
void                 d_test_handler_reset_results(struct d_test_handler* _handler);
void                 d_test_handler_print_results(const struct d_test_handler* _handler, const char* _label);
double               d_test_handler_get_pass_rate(const struct d_test_handler* _handler);
double               d_test_handler_get_assertion_rate(const struct d_test_handler* _handler);

// IX.   context helpers
struct d_test_context* d_test_context_new(struct d_test_handler* _handler);
void                   d_test_context_init(struct d_test_context* _context, struct d_test_handler* _handler);
void                   d_test_context_free(struct d_test_context* _context);


// D_TEST_PP_NARG
//   macro: counts the number of variadic arguments (up to 63).
#define D_TEST_PP_NARG(...) \
    D_TEST_PP_NARG_(__VA_ARGS__, D_TEST_PP_RSEQ_N())

// D_TEST_PP_NARG_
//   macro: internal helper for D_TEST_PP_NARG.
#define D_TEST_PP_NARG_(...) \
    D_TEST_PP_128TH_ARG(__VA_ARGS__)

// D_TEST_PP_128TH_ARG
//   macro: selects the 64th argument from the list to determine count.
#define D_TEST_PP_128TH_ARG(                                    \
     _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10,        \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,         \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30,         \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,         \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,         \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60,         \
    _61, _62, _63, N, ...) N

// D_TEST_PP_RSEQ_N
//   macro: reverse sequence 63..0 used by argument counting.
#define D_TEST_PP_RSEQ_N()                                      \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,   \
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,   \
    35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22,   \
    21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  9,  8,   \
     7,  6,  5,  4,  3,  2,  1,  0


// D_TEST_KV
//   macro: creates a key-value pair for metadata.
#define D_TEST_KV(k, v) { .key = (k), .value = (v) }

// D_TEST_METADATA
//   macro: creates a metadata array with automatic count.
#define D_TEST_METADATA(...) \
    (struct d_test_kv[]){ __VA_ARGS__ }, D_TEST_PP_NARG(__VA_ARGS__)


// D_ASSERTION
//   macro: creates an assertion from a condition and messages.
#define D_ASSERTION(condition, pass_msg, fail_msg) \
    d_assert((condition), (pass_msg), (fail_msg))

// D_ASSERTION_EQ
//   macro: creates an equality assertion.
#define D_ASSERTION_EQ(a, b, pass_msg, fail_msg) \
    d_assert_eq((a), (b), NULL, (pass_msg), (fail_msg))

// D_TEST_FN_WRAP
//   macro: wraps a test function pointer into a d_test_fn struct.
#define D_TEST_FN_WRAP(fn) \
    ((struct d_test_fn){ .test_fn = (fn), .count = 0, .args = NULL })


// DTestDSLNodeType
//   enum: discriminator for DSL tree node union types.
enum DTestDSLNodeType
{
    D_TEST_DSL_NODE_ASSERTION,
    D_TEST_DSL_NODE_TEST_FN,
    D_TEST_DSL_NODE_TEST,
    D_TEST_DSL_NODE_BLOCK,
    D_TEST_DSL_NODE_MODULE
};

// d_test_dsl_node
//   type: tagged union representing a node in the DSL tree.
struct d_test_dsl_node
{
    enum DTestDSLNodeType type;
    union
    {
        struct d_assert*      assertion;
        struct d_test_fn*     test_fn;
        struct d_test*        test;
        struct d_test_block*  block;
        struct d_test_module* module;
    };
};


// D_TEST_DECL
//   macro: creates a test from an array of DSL nodes.
#define D_TEST_DECL(...) \
    d_test_handler_create_test_from_nodes(                      \
        (struct d_test_dsl_node[]){ __VA_ARGS__ },              \
        D_TEST_PP_NARG(__VA_ARGS__))

// D_BLOCK_DECL
//   macro: creates a named block from an array of DSL nodes.
#define D_BLOCK_DECL(_name, ...) \
    d_test_handler_create_block_from_nodes(                     \
        (_name),                                                \
        (struct d_test_dsl_node[]){ __VA_ARGS__ },              \
        D_TEST_PP_NARG(__VA_ARGS__))

// D_MODULE_DECL
//   macro: creates a module from metadata and child DSL nodes.
#define D_MODULE_DECL(_metadata, ...) \
    d_test_handler_create_module_from_decl(                     \
        (struct d_test_kv[])_metadata,                          \
        sizeof((struct d_test_kv[])_metadata)                   \
            / sizeof(struct d_test_kv),                         \
        (struct d_test_dsl_node[]){ __VA_ARGS__ },              \
        D_TEST_PP_NARG(__VA_ARGS__))


// X.    DSL helper functions
struct d_test_module* d_test_handler_create_module_with_metadata(const char* _name, const struct d_test_kv* _metadata, size_t _count);
struct d_test*        d_test_handler_create_test_from_nodes(const struct d_test_dsl_node* _nodes, size_t _count);
struct d_test_block*  d_test_handler_create_block_from_nodes(const char* _name, const struct d_test_dsl_node* _nodes, size_t _count);
struct d_test_module* d_test_handler_create_module_from_decl(const struct d_test_kv* _metadata, size_t _metadata_count, const struct d_test_dsl_node* _children, size_t _children_count);
bool                  d_test_module_set_metadata_str(struct d_test_module* _module, const char* _key, const char* _value);


// D_TEST_HANDLER_DEFAULT
//   macro: creates a handler with default settings.
#define D_TEST_HANDLER_DEFAULT() \
    d_test_handler_new(NULL)

// D_TEST_HANDLER_VERBOSE
//   macro: creates a handler with event emission and verbose output.
#define D_TEST_HANDLER_VERBOSE() \
    d_test_handler_new_full(NULL,                               \
                            64,                                 \
                            32,                                 \
                            D_TEST_HANDLER_FLAG_EMIT_EVENTS     \
                                | D_TEST_HANDLER_FLAG_VERBOSE)

// D_TEST_HANDLER_SILENT
//   macro: creates a handler with silent output.
#define D_TEST_HANDLER_SILENT() \
    d_test_handler_new_full(NULL,                               \
                            0,                                  \
                            0,                                  \
                            D_TEST_HANDLER_FLAG_SILENT)

// D_TEST_RUN_ALL
//   macro: runs all tests in a module through a handler.
#define D_TEST_RUN_ALL(_handler, _module) \
    d_test_handler_run_module((_handler), (_module), NULL)


// D_TEST_FIRE_EVENT
//   macro: emits an event through the handler.
#define D_TEST_FIRE_EVENT(handler, event_type, context) \
    d_test_handler_emit_event((handler), (event_type), (context))

// D_TEST_ADD_LISTENER
//   macro: registers an enabled listener on the handler.
#define D_TEST_ADD_LISTENER(handler, event_id, callback) \
    d_test_handler_register_listener((handler),                 \
                                     (event_id),               \
                                     (callback),               \
                                     true)

// D_TEST_REMOVE_LISTENER
//   macro: unregisters a listener from the handler.
#define D_TEST_REMOVE_LISTENER(handler, event_id) \
    d_test_handler_unregister_listener((handler), (event_id))


// D_TEST_BEGIN
//   macro: opens a quick-test function body.
#define D_TEST_BEGIN(name) \
    bool name(struct d_test* _test)                             \
    {                                                           \
        bool _result = true;                                    \
        (void)_test;

// D_TEST_END
//   macro: closes a quick-test function body.
#define D_TEST_END \
        return _result;                                         \
    }

// D_TEST_CHECK
//   macro: evaluates an assertion inside a quick-test, updating the
// cumulative result.
#define D_TEST_CHECK(assertion) \
    do                                                          \
    {                                                           \
        struct d_assert* _a = (assertion);                      \
        if ( (_a) && (!_a->result) )                            \
        {                                                       \
            _result = false;                                    \
        }                                                       \
        d_assert_free(_a);                                      \
    } while (0)


// D_TEST_CONTEXT_INIT
//   macro: designated-initializer for a d_test_context on the stack.
#define D_TEST_CONTEXT_INIT(handler_ptr,                        \
                            test_ptr,                           \
                            block_ptr,                          \
                            event,                              \
                            depth_val)                          \
    {                                                           \
        .handler         = (handler_ptr),                       \
        .current_test    = (test_ptr),                          \
        .current_block   = (block_ptr),                         \
        .current_module  = NULL,                                \
        .current_tree    = NULL,                                \
        .current_session = NULL,                                \
        .event_type      = (event),                             \
        .depth           = (depth_val),                         \
        .last_assertion  = NULL,                                \
        .result          = true,                                \
        .start_time_ms   = 0.0,                                \
        .end_time_ms     = 0.0                                  \
    }

// D_TEST_CONTEXT_SIMPLE
//   macro: shorthand context initializer with fewer parameters.
#define D_TEST_CONTEXT_SIMPLE(handler_ptr, test_ptr, event) \
    D_TEST_CONTEXT_INIT((handler_ptr),                          \
                        (test_ptr),                             \
                        NULL,                                   \
                        (event),                                \
                        0)


#endif  // DJINTERP_TEST_TEST_HANDLER_
