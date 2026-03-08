/******************************************************************************
* djinterp [test]                                               test_session.c
*
*   Implementation of test session - the top-level DTest coordinator.
*   Manages test execution, output formatting, and result aggregation.
*
* path:      \src\test\test_session.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/
#include "../../../inc/c/test/test_session.h"


/******************************************************************************
 * INTERNAL HELPERS - TIMING
 *****************************************************************************/

/*
d_internal_session_get_time_ms
  Gets the current time in milliseconds.
*/
static double
d_internal_session_get_time_ms
(
    void
)
{
    return (double)clock() / (double)CLOCKS_PER_SEC * 1000.0;
}

/******************************************************************************
 * INTERNAL HELPERS - OUTPUT
 *****************************************************************************/

/*
d_internal_session_init_output
  Initializes the output structure with defaults.
*/
static void
d_internal_session_init_output
(
    struct d_test_session_output* _output
)
{
    if (!_output)
    {
        return;
    }

    _output->stream          = stdout;
    _output->format          = D_TEST_SESSION_DEFAULT_OUTPUT_FORMAT;
    _output->verbosity       = D_TEST_SESSION_DEFAULT_VERBOSITY;
    _output->owns_stream     = false;
    _output->use_color       = true;
    _output->show_timestamps = false;
    _output->show_duration   = true;
    _output->filename        = NULL;
    _output->extension       = D_TEST_SESSION_DEFAULT_EXTENSION;

    return;
}

/*
d_internal_session_close_output
  Closes the output stream if we own it.
*/
static void
d_internal_session_close_output
(
    struct d_test_session_output* _output
)
{
    if (!_output)
    {
        return;
    }

    if ( (_output->owns_stream) && 
         (_output->stream)      && 
         (_output->stream != stdout) && 
         (_output->stream != stderr) )
    {
        fclose(_output->stream);
    }

    _output->stream      = stdout;
    _output->owns_stream = false;

    return;
}

/*
d_internal_session_open_file
  Opens a file for output, handling extension defaults.
*/
static bool
d_internal_session_open_file
(
    struct d_test_session_output* _output,
    const char*                   _filename
)
{
    char   full_path[512];
    size_t len;
    bool   has_extension;
    size_t i;
    FILE*  file;

    if ( (!_output) || (!_filename) )
    {
        return false;
    }

    d_internal_session_close_output(_output);

    len           = d_strnlen(_filename, sizeof(full_path));
    has_extension = false;

    for (i = len; i > 0; i--)
    {
        if (_filename[i - 1] == '.')
        {
            has_extension = true;
            break;
        }

        if ( (_filename[i - 1] == '/') || 
             (_filename[i - 1] == '\\') )
        {
            break;
        }
    }

    if (has_extension)
    {
        if (len >= sizeof(full_path))
        {
            return false;
        }

        d_strcpy_s(full_path, sizeof(full_path), _filename);
    }
    else
    {
        const char* ext;

        ext = _output->extension ? _output->extension 
                                 : D_TEST_SESSION_DEFAULT_EXTENSION;

        if (len + d_strnlen(ext, 16) >= sizeof(full_path))
        {
            return false;
        }

        d_strcpy_s(full_path, sizeof(full_path), _filename);
        d_strcat_s(full_path, sizeof(full_path), ext);
    }

#if defined(_MSC_VER)
    {
        errno_t err;

        err = fopen_s(&file, full_path, "w");

        if (err != 0 || file == NULL)
        {
            return false;
        }
    }
#else
    file = fopen(full_path, "w");

    if (!file)
    {
        return false;
    }
#endif

    _output->stream      = file;
    _output->owns_stream = true;
    _output->filename    = _filename;

    return true;
}

/******************************************************************************
 * INTERNAL HELPERS - COLOR
 *****************************************************************************/

#define D_ANSI_RESET   "\033[0m"
#define D_ANSI_RED     "\033[31m"
#define D_ANSI_GREEN   "\033[32m"
#define D_ANSI_YELLOW  "\033[33m"
#define D_ANSI_BLUE    "\033[34m"
#define D_ANSI_MAGENTA "\033[35m"
#define D_ANSI_CYAN    "\033[36m"
#define D_ANSI_BOLD    "\033[1m"
#define D_ANSI_DIM     "\033[2m"


static const char*
d_internal_session_color_pass
(
    const struct d_test_session* _session
)
{
    if ( (_session) && (_session->output.use_color) )
    {
        return D_ANSI_GREEN;
    }

    return "";
}


static const char*
d_internal_session_color_fail
(
    const struct d_test_session* _session
)
{
    if ( (_session) && (_session->output.use_color) )
    {
        return D_ANSI_RED;
    }

    return "";
}


static const char*
d_internal_session_color_skip
(
    const struct d_test_session* _session
)
{
    if ( (_session) && (_session->output.use_color) )
    {
        return D_ANSI_YELLOW;
    }

    return "";
}


static const char*
d_internal_session_color_reset
(
    const struct d_test_session* _session
)
{
    if ( (_session) && (_session->output.use_color) )
    {
        return D_ANSI_RESET;
    }

    return "";
}


static const char*
d_internal_session_color_bold
(
    const struct d_test_session* _session
)
{
    if ( (_session) && (_session->output.use_color) )
    {
        return D_ANSI_BOLD;
    }

    return "";
}

/******************************************************************************
 * INTERNAL HELPERS - CHILDREN
 *****************************************************************************/

static bool
d_internal_session_add_children
(
    struct d_test_session* _session,
    struct d_test_type**   _children,
    size_t                 _child_count
)
{
    size_t i;

    if ( (!_session)                       ||
         (!_session->children)             ||
         (!_children && _child_count > 0) )
    {
        return false;
    }

    for (i = 0; i < _child_count; i++)
    {
        if (!_children[i])
        {
            continue;
        }

        if (_children[i]->type != D_TEST_TYPE_MODULE)
        {
            continue;
        }

        if (!d_ptr_vector_push_back(_session->children, _children[i]))
        {
            return false;
        }
    }

    return true;
}

/******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR FUNCTIONS
 *****************************************************************************/

struct d_test_session*
d_test_session_new
(
    void
)
{
    struct d_test_session* session;

    session = (struct d_test_session*)calloc(1, sizeof(struct d_test_session));

    if (!session)
    {
        return NULL;
    }

    session->children = d_ptr_vector_new_default();

    if (!session->children)
    {
        free(session);

        return NULL;
    }

    session->config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!session->config)
    {
        d_ptr_vector_free(session->children);
        free(session);

        return NULL;
    }

    d_internal_session_init_output(&session->output);
    D_STATISTICS_RESET(&session->stats);

    session->status         = D_TEST_SESSION_STATUS_CREATED;
    session->current_index  = 0;
    session->failure_count  = 0;
    session->repeat_current = 0;
    session->start_time_ms  = 0.0;
    session->end_time_ms    = 0.0;

    return session;
}


struct d_test_session*
d_test_session_new_with_config
(
    struct d_test_options* _config
)
{
    struct d_test_session* session;

    session = d_test_session_new();

    if (!session)
    {
        return NULL;
    }

    if (_config)
    {
        if (session->config)
        {
            d_test_options_free(session->config);
        }

        session->config = _config;
    }

    return session;
}


struct d_test_session*
d_test_session_new_with_modules
(
    struct d_test_type** _modules,
    size_t               _count
)
{
    struct d_test_session* session;

    session = d_test_session_new();

    if (!session)
    {
        return NULL;
    }

    if (_modules && _count > 0)
    {
        if (!d_internal_session_add_children(session, _modules, _count))
        {
            d_test_session_free(session);

            return NULL;
        }
    }

    return session;
}


struct d_test_session*
d_test_session_new_with_modules_and_config
(
    struct d_test_options* _config,
    struct d_test_type**  _modules,
    size_t                _count
)
{
    struct d_test_session* session;
    void*                  value;

    session = d_test_session_new();

    if (!session)
    {
        return NULL;
    }

    if (_config)
    {
        if (session->config)
        {
            d_test_options_free(session->config);
        }

        session->config = _config;

        if (_config->settings)
        {
            value = d_min_enum_map_get(_config->settings, 
                                       D_TEST_SESSION_OPT_FILENAME);

            if (value)
            {
                d_test_session_set_output_file(session, (const char*)value);
            }

            value = d_min_enum_map_get(_config->settings, 
                                       D_TEST_SESSION_OPT_FILE_EXTENSION);

            if (value)
            {
                session->output.extension = (const char*)value;
            }

            value = d_min_enum_map_get(_config->settings, 
                                       D_TEST_SESSION_OPT_VERBOSE);

            if (value)
            {
                session->output.verbosity = (enum DTestVerbosity)(uintptr_t)value;
            }

            value = d_min_enum_map_get(_config->settings, 
                                       D_TEST_SESSION_OPT_COLOR);

            if (value)
            {
                session->output.use_color = (bool)(uintptr_t)value;
            }

            value = d_min_enum_map_get(_config->settings, 
                                       D_TEST_SESSION_OPT_OUTPUT_FORMAT);

            if (value)
            {
                d_test_session_set_output_format(session, 
                    (enum DTestOutputFormat)(uintptr_t)value);
            }

            value = d_min_enum_map_get(_config->settings,
                                       D_TEST_SESSION_OPT_SHOW_TIMESTAMPS);

            if (value)
            {
                session->output.show_timestamps = (bool)(uintptr_t)value;
            }

            value = d_min_enum_map_get(_config->settings,
                                       D_TEST_SESSION_OPT_SHOW_DURATION);

            if (value)
            {
                session->output.show_duration = (bool)(uintptr_t)value;
            }
        }
    }

    if (_modules && _count > 0)
    {
        if (!d_internal_session_add_children(session, _modules, _count))
        {
            d_test_session_free(session);

            return NULL;
        }
    }

    return session;
}


struct d_test_options*
d_test_session_validate_args
(
    struct d_test_arg* _args,
    size_t             _arg_count
)
{
    struct d_test_options*      config;
    struct d_test_registry_row* row;
    size_t                      i;

    if (!_args || _arg_count == 0)
    {
        return NULL;
    }

    config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return NULL;
    }

    if (!config->settings)
    {
        config->settings = d_min_enum_map_new();

        if (!config->settings)
        {
            d_test_options_free(config);

            return NULL;
        }
    }

    for (i = 0; i < _arg_count; i++)
    {
        if (!_args[i].key)
        {
            continue;
        }

        // look up the key in the registry
        row = d_test_registry_find(_args[i].key);

        // validate: key must exist AND have SESSION flag set
        if ( (!row) || 
             ((row->command_flags & D_TEST_REGISTRY_FLAG_SESSION) == 0) )
        {
            continue;
        }

        // store with the registry flag as the key
        d_min_enum_map_put(config->settings, 
                           (int)row->flag, 
                           (void*)_args[i].value);
    }

    return config;
}


void
d_test_session_free
(
    struct d_test_session* _session
)
{
    if (!_session)
    {
        return;
    }

    d_internal_session_close_output(&_session->output);

    if (_session->children)
    {
        d_ptr_vector_free(_session->children);
    }

    if (_session->config)
    {
        d_test_options_free(_session->config);
    }

    free(_session);

    return;
}

/******************************************************************************
 * CONFIGURATION FUNCTIONS
 *****************************************************************************/

D_INLINE bool
d_test_session_set_option
(
    struct d_test_session*  _session,
    enum DTestSessionOption _option,
    const void*             _value
)
{
    return ( (_session)         && 
             (_session->config) && 
             (_session->config->settings) )
        ?  d_min_enum_map_put(_session->config->settings, 
                              (int)_option, 
                              (void*)_value)
        : false;
}

D_INLINE void*
d_test_session_get_option
(
    const struct d_test_session* _session,
    enum DTestSessionOption      _option
)
{
    return ( (_session)         && 
             (_session->config) && 
             (_session->config->settings) )
        ? d_min_enum_map_get(_session->config->settings, 
                             (int)_option)
        : NULL;
}


bool
d_test_session_set_output_format
(
    struct d_test_session* _session,
    enum DTestOutputFormat _format
)
{
    if (!_session)
    {
        return false;
    }

    _session->output.format = _format;

    switch (_format)
    {
        case D_TEST_OUTPUT_VERBOSE:
            _session->output.verbosity = D_TEST_VERBOSITY_VERBOSE;
            break;

        case D_TEST_OUTPUT_MINIMAL:
            _session->output.verbosity = D_TEST_VERBOSITY_MINIMAL;
            break;

        case D_TEST_OUTPUT_SILENT:
            _session->output.verbosity = D_TEST_VERBOSITY_SILENT;
            break;

        default:
            break;
    }

    return true;
}


bool
d_test_session_set_output_file
(
    struct d_test_session* _session,
    const char*            _filename
)
{
    if ( (!_session) || 
         (!_filename) )
    {
        return false;
    }

    _session->output.use_color = false;
    _session->output.format    = D_TEST_OUTPUT_TEXT;

    return d_internal_session_open_file(&_session->output, _filename);
}


bool
d_test_session_set_verbosity
(
    struct d_test_session* _session,
    enum DTestVerbosity    _verbosity
)
{
    if (!_session)
    {
        return false;
    }

    _session->output.verbosity = _verbosity;

    return true;
}


bool
d_test_session_enable_color
(
    struct d_test_session* _session,
    bool                   _enable
)
{
    if (!_session)
    {
        return false;
    }

    _session->output.use_color = _enable;

    return true;
}

/******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

bool
d_test_session_add_child
(
    struct d_test_session* _session,
    struct d_test_type*    _child
)
{
    if ( (!_session)           || 
         (!_session->children) || 
         (!_child) )
    {
        return false;
    }

    if (_child->type != D_TEST_TYPE_MODULE)
    {
        return false;
    }

    return d_ptr_vector_push_back(_session->children, _child);
}


bool
d_test_session_add_children
(
    struct d_test_session* _session,
    struct d_test_type**   _children,
    size_t                 _count
)
{
    size_t i;

    if ( (!_session) || (!_children && _count > 0) )
    {
        return false;
    }

    for (i = 0; i < _count; i++)
    {
        if (!d_test_session_add_child(_session, _children[i]))
        {
            return false;
        }
    }

    return true;
}


size_t
d_test_session_child_count
(
    const struct d_test_session* _session
)
{
    if ( (!_session) || (!_session->children) )
    {
        return 0;
    }

    return d_ptr_vector_size(_session->children);
}


struct d_test_type*
d_test_session_get_child_at
(
    const struct d_test_session* _session,
    size_t                       _index
)
{
    if ( (!_session) || (!_session->children) )
    {
        return NULL;
    }

    return (struct d_test_type*)d_ptr_vector_at(_session->children, 
                                                 (d_index)_index);
}


bool
d_test_session_clear_children
(
    struct d_test_session* _session
)
{
    if ( (!_session) || (!_session->children) )
    {
        return false;
    }

    d_ptr_vector_clear(_session->children);

    return true;
}

/******************************************************************************
 * EXECUTION FUNCTIONS
 *****************************************************************************/

bool
d_test_session_run
(
    struct d_test_session* _session
)
{
    size_t              i;
    size_t              child_count;
    size_t              repeat_count;
    size_t              fail_fast;
    struct d_test_type* child;
    bool                child_passed;
    bool                all_passed;
    bool                abort_on_failure;
    void*               opt_value;

    if (!_session)
    {
        return false;
    }

    if (_session->status == D_TEST_SESSION_STATUS_RUNNING)
    {
        return false;
    }

    opt_value = d_test_session_get_option(_session, 
                                          D_TEST_SESSION_OPT_ABORT_ON_FAILURE);
    abort_on_failure = opt_value ? (bool)(uintptr_t)opt_value : false;

    opt_value = d_test_session_get_option(_session, 
                                          D_TEST_SESSION_OPT_REPEAT_COUNT);
    repeat_count = opt_value ? (size_t)(uintptr_t)opt_value 
                             : D_TEST_SESSION_DEFAULT_REPEAT_COUNT;

    opt_value = d_test_session_get_option(_session, 
                                          D_TEST_SESSION_OPT_FAIL_FAST);
    fail_fast = opt_value ? (size_t)(uintptr_t)opt_value : 0;

    _session->status         = D_TEST_SESSION_STATUS_RUNNING;
    _session->current_index  = 0;
    _session->failure_count  = 0;
    _session->start_time_ms  = d_internal_session_get_time_ms();

    D_STATISTICS_RESET(&_session->stats);
    d_test_statistics_start_timer(&_session->stats);

    d_test_session_write_header(_session);

    all_passed   = true;
    child_count  = d_test_session_child_count(_session);

    for (_session->repeat_current = 0; 
         _session->repeat_current < repeat_count; 
         _session->repeat_current++)
    {
        if (repeat_count > 1)
        {
            d_test_session_writeln(_session, 
                                   "\n--- Iteration %zu of %zu ---",
                                   _session->repeat_current + 1,
                                   repeat_count);
        }

        for (i = 0; i < child_count; i++)
        {
            if (_session->status == D_TEST_SESSION_STATUS_PAUSED)
            {
                continue;
            }

            if (_session->status == D_TEST_SESSION_STATUS_ABORTED)
            {
                break;
            }

            _session->current_index = i;

            child = d_test_session_get_child_at(_session, i);

            if ( (!child) || (!child->D_KEYWORD_TEST_MODULE) )
            {
                continue;
            }

            d_test_session_write_module_start(_session, child);

            child_passed = d_test_module_run(child->D_KEYWORD_TEST_MODULE, 
                                             _session->config);

            if (child_passed)
            {
                D_STAT_COUNTER_INC_MODULE_PASS(&_session->stats);
            }
            else
            {
                D_STAT_COUNTER_INC_MODULE_FAIL(&_session->stats);
                _session->failure_count++;
                all_passed = false;
            }

            d_test_session_write_module_end(_session, child, child_passed);

            if ( (abort_on_failure) && (!child_passed) )
            {
                _session->status = D_TEST_SESSION_STATUS_ABORTED;
                break;
            }

            if ( (fail_fast > 0) && (_session->failure_count >= fail_fast) )
            {
                _session->status = D_TEST_SESSION_STATUS_ABORTED;
                break;
            }
        }

        if (_session->status == D_TEST_SESSION_STATUS_ABORTED)
        {
            break;
        }
    }

    _session->end_time_ms = d_internal_session_get_time_ms();
    d_test_statistics_stop_timer(&_session->stats);

    if (_session->status != D_TEST_SESSION_STATUS_ABORTED)
    {
        _session->status = D_TEST_SESSION_STATUS_COMPLETED;
    }

    d_test_session_write_summary(_session);
    d_test_session_write_footer(_session);
    d_test_session_flush(_session);

    return all_passed;
}


bool
d_test_session_run_child
(
    struct d_test_session* _session,
    size_t                 _child_index
)
{
    struct d_test_type* child;

    if (!_session)
    {
        return false;
    }

    child = d_test_session_get_child_at(_session, _child_index);

    if ( (!child) || (!child->D_KEYWORD_TEST_MODULE) )
    {
        return false;
    }

    return d_test_module_run(child->D_KEYWORD_TEST_MODULE, _session->config);
}


bool
d_test_session_run_by_name
(
    struct d_test_session* _session,
    const char*            _name
)
{
    size_t              i;
    size_t              count;
    struct d_test_type* child;
    const char*         child_name;
    bool                all_passed;
    bool                found;

    if ( (!_session) || (!_name) )
    {
        return false;
    }

    all_passed = true;
    found      = false;
    count      = d_test_session_child_count(_session);

    for (i = 0; i < count; i++)
    {
        child = d_test_session_get_child_at(_session, i);

        if ( (!child) || (!child->D_KEYWORD_TEST_MODULE) )
        {
            continue;
        }

        child_name = d_test_module_get_name(child->D_KEYWORD_TEST_MODULE);

        if ( (child_name) && (strcmp(child_name, _name) == 0) )
        {
            found = true;

            if (!d_test_module_run(child->D_KEYWORD_TEST_MODULE, 
                                   _session->config))
            {
                all_passed = false;
            }
        }
    }

    return found && all_passed;
}


bool
d_test_session_pause
(
    struct d_test_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    if (_session->status != D_TEST_SESSION_STATUS_RUNNING)
    {
        return false;
    }

    _session->status = D_TEST_SESSION_STATUS_PAUSED;

    return true;
}


bool
d_test_session_resume
(
    struct d_test_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    if (_session->status != D_TEST_SESSION_STATUS_PAUSED)
    {
        return false;
    }

    _session->status = D_TEST_SESSION_STATUS_RUNNING;

    return true;
}


bool
d_test_session_abort
(
    struct d_test_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    _session->status = D_TEST_SESSION_STATUS_ABORTED;

    return true;
}


bool
d_test_session_reset
(
    struct d_test_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    _session->status         = D_TEST_SESSION_STATUS_CREATED;
    _session->current_index  = 0;
    _session->failure_count  = 0;
    _session->repeat_current = 0;
    _session->start_time_ms  = 0.0;
    _session->end_time_ms    = 0.0;

    D_STATISTICS_RESET(&_session->stats);

    return true;
}

/******************************************************************************
 * OUTPUT FUNCTIONS
 *****************************************************************************/

void
d_test_session_write
(
    struct d_test_session* _session,
    const char*            _format,
    ...
)
{
    va_list args;

    if ( (!_session)               || 
         (!_session->output.stream) || 
         (!_format) )
    {
        return;
    }

    if (_session->output.verbosity == D_TEST_VERBOSITY_SILENT)
    {
        return;
    }

    va_start(args, _format);
    vfprintf(_session->output.stream, _format, args);
    va_end(args);

    return;
}


void
d_test_session_writeln
(
    struct d_test_session* _session,
    const char*            _format,
    ...
)
{
    va_list args;

    if ( (!_session)                || 
         (!_session->output.stream) || 
         (!_format) )
    {
        return;
    }

    if (_session->output.verbosity == D_TEST_VERBOSITY_SILENT)
    {
        return;
    }

    va_start(args, _format);
    vfprintf(_session->output.stream, _format, args);
    va_end(args);

    fprintf(_session->output.stream, "\n");

    return;
}


void
d_test_session_write_header
(
    struct d_test_session* _session
)
{
    time_t    now;
    struct tm tm_result;
    char      time_buf[64];

    if (!_session)
    {
        return;
    }

    if (_session->output.verbosity < D_TEST_VERBOSITY_NORMAL)
    {
        return;
    }

    time(&now);
    d_localtime(&now, &tm_result);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_result);

    d_test_session_writeln(_session, 
        "%s========================================%s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session));

    d_test_session_writeln(_session, 
        "%s         DTest Session Started%s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session));

    d_test_session_writeln(_session, 
        "%s========================================%s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session));

    if (_session->output.show_timestamps)
    {
        d_test_session_writeln(_session, "Started: %s", time_buf);
    }

    d_test_session_writeln(_session, 
        "Modules: %zu",
        d_test_session_child_count(_session));

    d_test_session_writeln(_session, "");

    return;
}


void
d_test_session_write_footer
(
    struct d_test_session* _session
)
{
    time_t    now;
    struct tm tm_result;
    char      time_buf[64];

    if (!_session)
    {
        return;
    }

    if (_session->output.verbosity < D_TEST_VERBOSITY_NORMAL)
    {
        return;
    }

    d_test_session_writeln(_session, "");

    d_test_session_writeln(_session, 
        "%s========================================%s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session));

    d_test_session_writeln(_session, 
        "%s         DTest Session Complete%s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session));

    d_test_session_writeln(_session, 
        "%s========================================%s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session));

    if (_session->output.show_timestamps)
    {
        time(&now);
        d_localtime(&now, &tm_result);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_result);
        d_test_session_writeln(_session, "Finished: %s", time_buf);
    }

    return;
}


void
d_test_session_write_summary
(
    struct d_test_session* _session
)
{
    double   duration;
    bool     all_passed;
    size_t   total_run;
    size_t   total_passed;
    size_t   total_failed;
    size_t   total_skipped;

    if (!_session)
    {
        return;
    }

    if (_session->output.verbosity < D_TEST_VERBOSITY_MINIMAL)
    {
        return;
    }

    duration      = d_test_session_duration_ms(_session);
    all_passed    = d_test_session_all_passed(_session);
    total_run     = d_test_session_total_run(_session);
    total_passed  = d_test_session_total_passed(_session);
    total_failed  = d_test_session_total_failed(_session);
    total_skipped = d_test_session_total_skipped(_session);

    d_test_session_writeln(_session, "");
    d_test_session_writeln(_session, 
        "%s--- Summary ---%s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session));

    d_test_session_writeln(_session, 
        "Status: %s%s%s",
        all_passed ? d_internal_session_color_pass(_session)
                   : d_internal_session_color_fail(_session),
        all_passed ? "PASSED" : "FAILED",
        d_internal_session_color_reset(_session));

    d_test_session_writeln(_session, "Total:   %zu", total_run);

    d_test_session_writeln(_session, 
        "Passed:  %s%zu%s",
        d_internal_session_color_pass(_session),
        total_passed,
        d_internal_session_color_reset(_session));

    d_test_session_writeln(_session, 
        "Failed:  %s%zu%s",
        total_failed > 0 ? d_internal_session_color_fail(_session) : "",
        total_failed,
        d_internal_session_color_reset(_session));

    if (total_skipped > 0)
    {
        d_test_session_writeln(_session, 
            "Skipped: %s%zu%s",
            d_internal_session_color_skip(_session),
            total_skipped,
            d_internal_session_color_reset(_session));
    }

    if (_session->output.show_duration)
    {
        d_test_session_writeln(_session, "Duration: %.2f ms", duration);
    }

    return;
}


void
d_test_session_write_module_start
(
    struct d_test_session*    _session,
    const struct d_test_type* _module
)
{
    const char* name;

    if ( (!_session) || (!_module) || (!_module->D_KEYWORD_TEST_MODULE) )
    {
        return;
    }

    if (_session->output.verbosity < D_TEST_VERBOSITY_NORMAL)
    {
        return;
    }

    name = d_test_module_get_name(_module->D_KEYWORD_TEST_MODULE);

    d_test_session_writeln(_session, 
        "%s[MODULE]%s %s",
        d_internal_session_color_bold(_session),
        d_internal_session_color_reset(_session),
        name ? name : "(unnamed)");

    return;
}


void
d_test_session_write_module_end
(
    struct d_test_session*    _session,
    const struct d_test_type* _module,
    bool                      _passed
)
{
    const char* name;

    if ( (!_session) || (!_module) || (!_module->D_KEYWORD_TEST_MODULE) )
    {
        return;
    }

    if (_session->output.verbosity < D_TEST_VERBOSITY_NORMAL)
    {
        return;
    }

    name = d_test_module_get_name(_module->D_KEYWORD_TEST_MODULE);

    d_test_session_writeln(_session, 
        "  %s%s%s %s",
        _passed ? d_internal_session_color_pass(_session)
                : d_internal_session_color_fail(_session),
        _passed ? D_TEST_SYMBOL_PASS : D_TEST_SYMBOL_FAIL,
        d_internal_session_color_reset(_session),
        name ? name : "(unnamed)");

    return;
}


void
d_test_session_write_test_result
(
    struct d_test_session* _session,
    const char*            _name,
    bool                   _passed,
    const char*            _message,
    double                 _duration_ms
)
{
    if (!_session)
    {
        return;
    }

    if ( (_session->output.verbosity == D_TEST_VERBOSITY_MINIMAL) && (_passed) )
    {
        return;
    }

    if (_session->output.verbosity < D_TEST_VERBOSITY_MINIMAL)
    {
        return;
    }

    d_test_session_write(_session, 
        "    %s%s%s %s",
        _passed ? d_internal_session_color_pass(_session)
                : d_internal_session_color_fail(_session),
        _passed ? D_TEST_SYMBOL_PASS : D_TEST_SYMBOL_FAIL,
        d_internal_session_color_reset(_session),
        _name ? _name : "(unnamed)");

    if ( (_session->output.show_duration) && (_duration_ms > 0) )
    {
        d_test_session_write(_session, " (%.2f ms)", _duration_ms);
    }

    d_test_session_writeln(_session, "");

    if ( (_message) && (!_passed) )
    {
        d_test_session_writeln(_session, "      %s", _message);
    }

    return;
}


void
d_test_session_flush
(
    struct d_test_session* _session
)
{
    if ( (_session) && (_session->output.stream) )
    {
        fflush(_session->output.stream);
    }

    return;
}

/******************************************************************************
 * STATISTICS FUNCTIONS
 *****************************************************************************/

const struct d_test_statistics*
d_test_session_get_stats
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return NULL;
    }

    return &_session->stats;
}


bool
d_test_session_all_passed
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    return _session->failure_count == 0;
}


size_t
d_test_session_total_run
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return 0;
    }

    return d_test_statistics_total_run(&_session->stats);
}


size_t
d_test_session_total_passed
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return 0;
    }

    return d_test_statistics_total_passed(&_session->stats);
}


size_t
d_test_session_total_failed
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return 0;
    }

    return d_test_statistics_total_failed(&_session->stats);
}


size_t
d_test_session_total_skipped
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return 0;
    }

    return d_test_statistics_total_skipped(&_session->stats);
}


double
d_test_session_duration_ms
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return 0.0;
    }

    if (_session->end_time_ms > 0)
    {
        return _session->end_time_ms - _session->start_time_ms;
    }

    return d_internal_session_get_time_ms() - _session->start_time_ms;
}

/******************************************************************************
 * STATUS FUNCTIONS
 *****************************************************************************/

enum DTestSessionStatus
d_test_session_get_status
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return D_TEST_SESSION_STATUS_UNKNOWN;
    }

    return _session->status;
}


const char*
d_test_session_status_to_string
(
    enum DTestSessionStatus _status
)
{
    switch (_status)
    {
        case D_TEST_SESSION_STATUS_CREATED:
            return "CREATED";

        case D_TEST_SESSION_STATUS_RUNNING:
            return "RUNNING";

        case D_TEST_SESSION_STATUS_PAUSED:
            return "PAUSED";

        case D_TEST_SESSION_STATUS_COMPLETED:
            return "COMPLETED";

        case D_TEST_SESSION_STATUS_ABORTED:
            return "ABORTED";

        case D_TEST_SESSION_STATUS_ERROR:
            return "ERROR";

        case D_TEST_SESSION_STATUS_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}


bool
d_test_session_is_running
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    return _session->status == D_TEST_SESSION_STATUS_RUNNING;
}


bool
d_test_session_is_complete
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    return (_session->status == D_TEST_SESSION_STATUS_COMPLETED) ||
           (_session->status == D_TEST_SESSION_STATUS_ABORTED);
}

/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

void
d_test_session_print
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        printf("Session: NULL\n");

        return;
    }

    printf("DTest Session\n");
    printf("  Status:  %s\n", d_test_session_status_to_string(_session->status));
    printf("  Modules: %zu\n", d_test_session_child_count(_session));
    printf("  Run:     %zu\n", d_test_session_total_run(_session));
    printf("  Passed:  %zu\n", d_test_session_total_passed(_session));
    printf("  Failed:  %zu\n", d_test_session_total_failed(_session));

    return;
}


int
d_test_session_exit_code
(
    const struct d_test_session* _session
)
{
    if (!_session)
    {
        return 2;
    }

    if (_session->status == D_TEST_SESSION_STATUS_ERROR)
    {
        return 2;
    }

    if (_session->status == D_TEST_SESSION_STATUS_ABORTED)
    {
        return 2;
    }

    if (_session->failure_count > 0)
    {
        return 1;
    }

    return 0;
}