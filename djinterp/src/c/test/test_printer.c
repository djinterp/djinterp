#include "../../../inc/c/test/test_printer.h"


/******************************************************************************
 * INTERNAL STATE
 *****************************************************************************/

static FILE* g_d_test_printer_output_file = NULL;


/******************************************************************************
 * INTERNAL DUAL-OUTPUT HELPER
 *****************************************************************************/

#define D_INTERNAL_TPRINTF(...)                                 \
    do {                                                        \
        printf(__VA_ARGS__);                                    \
        if (g_d_test_printer_output_file)                       \
        {                                                       \
            fprintf(g_d_test_printer_output_file,               \
                    __VA_ARGS__);                                \
        }                                                       \
    } while (0)


/******************************************************************************
 * INTERNAL HELPER FUNCTIONS
 *****************************************************************************/

static int
d_test_internal_strcat_safe
(
    char*       _dest,
    size_t      _dest_capacity,
    size_t*     _dest_len,
    const char* _source,
    size_t      _source_len
)
{
    if ( (!_dest)     ||
         (!_dest_len) ||
         (!_source) )
    {
        return EINVAL;
    }

    if (*_dest_len + _source_len >= _dest_capacity)
    {
        return ERANGE;
    }

    d_memcpy(_dest + *_dest_len, _source, _source_len);
    *_dest_len        += _source_len;
    _dest[*_dest_len]  = '\0';

    return 0;
}


/******************************************************************************
 * PRINTER STATE MANAGEMENT
 *****************************************************************************/

void
d_test_printer_set_output_file
(
    FILE* _file
)
{
    g_d_test_printer_output_file = _file;

    return;
}

FILE*
d_test_printer_get_output_file
(
    void
)
{
    return g_d_test_printer_output_file;
}


/******************************************************************************
 * CLI OPTION FUNCTIONS
 *****************************************************************************/

#if D_CFG_TEST_ENABLE_CLI

/*
d_test_printer_options_parse
  Parses command-line arguments directly into a runner's arg list.

Parameter(s):
  _runner_args: runner arg list to set options on
  _argc:        argument count from main
  _argv:        argument vector from main
Return:
  true if parsing succeeded and execution should continue, false if the
program should exit (e.g. --help was requested).
*/
bool
d_test_printer_options_parse
(
    struct d_test_arg_list* _runner_args,
    int                     _argc,
    char*                   _argv[]
)
{
    int i;

    if ( (!_runner_args) ||
         (!_argv) )
    {
        return true;
    }

    for (i = 1; i < _argc; i++)
    {
        if (!_argv[i])
        {
            continue;
        }

        if (strcmp(_argv[i], "-na") == 0)
        {
            d_test_arg_list_set(_runner_args,
                D_TEST_ARG_OPT_NUMBER_ASSERTIONS,
                D_TEST_ARG_TRUE);
        }
        else if (strcmp(_argv[i], "-nt") == 0)
        {
            d_test_arg_list_set(_runner_args,
                D_TEST_ARG_OPT_NUMBER_TESTS,
                D_TEST_ARG_TRUE);
        }
        else if (strcmp(_argv[i], "-gn") == 0)
        {
            d_test_arg_list_set(_runner_args,
                D_TEST_ARG_OPT_GLOBAL_NUMBERING,
                D_TEST_ARG_TRUE);
        }
        else if (strcmp(_argv[i], "-ni") == 0)
        {
            d_test_arg_list_set(_runner_args,
                D_TEST_ARG_OPT_SHOW_INFO,
                D_TEST_ARG_FALSE);
        }
        else if (strcmp(_argv[i], "-nf") == 0)
        {
            d_test_arg_list_set(_runner_args,
                D_TEST_ARG_OPT_SHOW_MODULE_FOOTER,
                D_TEST_ARG_FALSE);
        }
        else if (strcmp(_argv[i], "-lf") == 0)
        {
            d_test_arg_list_set(_runner_args,
                D_TEST_ARG_OPT_LIST_FAILURES,
                D_TEST_ARG_TRUE);
        }
        else if (strcmp(_argv[i], "-o") == 0)
        {
            if (i + 1 < _argc)
            {
                i++;
                d_test_arg_list_set(
                    _runner_args,
                    D_TEST_ARG_OUTPUT_FILE,
                    (void*)_argv[i]);
            }
            else
            {
                printf("ERROR: -o requires a file path "
                       "argument\n");

                return false;
            }
        }
        else if ( (strcmp(_argv[i], "-h") == 0)    ||
                  (strcmp(_argv[i], "--help") == 0) )
        {
            d_test_printer_options_print_usage(
                _argv[0]);

            return false;
        }
        else
        {
            printf("WARNING: unknown option '%s' "
                   "(ignored)\n",
                   _argv[i]);
        }
    }

    return true;
}

void
d_test_printer_options_print_usage
(
    const char* _program_name
)
{
    if (!_program_name)
    {
        _program_name = "test_runner";
    }

    printf("\n");
    printf("Usage: %s [options]\n", _program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -na          Number assertions in "
           "output\n");
    printf("  -nt          Number unit tests in "
           "output\n");
    printf("  -gn          Global numbering "
           "(continuous across modules)\n");
    printf("  -ni          Suppress [INFO] diagnostic "
           "lines\n");
    printf("  -nf          Suppress per-module result "
           "footers\n");
    printf("  -lf          List all failures at end "
           "of run\n");
    printf("  -o <file>    Mirror all output to "
           "file\n");
    printf("  -h, --help   Show this help message\n");
    printf("\n");
    printf("Defaults: no numbering, all output shown, "
           "stdout only.\n");
    printf("\n");

    return;
}

#endif  // D_CFG_TEST_ENABLE_CLI


/******************************************************************************
 * FAILURE LIST PRINT FUNCTIONS
 *****************************************************************************/

void
d_test_printer_failure_list_print
(
    const struct d_test_standalone_failure_list* _list
)
{
    size_t      i;
    const char* mod;
    const char* name;
    const char* msg;

    if ( (!_list)            ||
         (_list->count == 0) )
    {
        return;
    }

    printf("\n");
    printf("%s\n", D_TEST_PRINTER_SEPARATOR_DOUBLE);
    printf("  FAILURE SUMMARY (%zu failure%s)\n",
           _list->count,
           (_list->count == 1) ? "" : "s");
    printf("%s\n", D_TEST_PRINTER_SEPARATOR_DOUBLE);

    for (i = 0; i < _list->count; i++)
    {
        mod = _list->entries[i].module_name
                  ? _list->entries[i].module_name
                  : "(unknown)";
        name = _list->entries[i].test_name
                   ? _list->entries[i].test_name
                   : "(unnamed)";
        msg = _list->entries[i].message
                  ? _list->entries[i].message
                  : "";

        printf("  %3zu) [%s] %s", (i + 1), mod, name);

        if (msg[0] != '\0')
        {
            printf(" - %s", msg);
        }

        printf("\n");
    }

    printf("%s\n", D_TEST_PRINTER_SEPARATOR_DOUBLE);

    return;
}

void
d_test_printer_failure_list_print_file
(
    FILE*                                       _file,
    const struct d_test_standalone_failure_list* _list
)
{
    size_t      i;
    const char* mod;
    const char* name;
    const char* msg;

    if ( (!_file)            ||
         (!_list)            ||
         (_list->count == 0) )
    {
        return;
    }

    fprintf(_file, "\n");
    fprintf(_file, "%s\n",
            D_TEST_PRINTER_SEPARATOR_DOUBLE);
    fprintf(_file,
            "  FAILURE SUMMARY (%zu failure%s)\n",
            _list->count,
            (_list->count == 1) ? "" : "s");
    fprintf(_file, "%s\n",
            D_TEST_PRINTER_SEPARATOR_DOUBLE);

    for (i = 0; i < _list->count; i++)
    {
        mod = _list->entries[i].module_name
                  ? _list->entries[i].module_name
                  : "(unknown)";
        name = _list->entries[i].test_name
                   ? _list->entries[i].test_name
                   : "(unnamed)";
        msg = _list->entries[i].message
                  ? _list->entries[i].message
                  : "";

        fprintf(_file, "  %3zu) [%s] %s",
                (i + 1), mod, name);

        if (msg[0] != '\0')
        {
            fprintf(_file, " - %s", msg);
        }

        fprintf(_file, "\n");
    }

    fprintf(_file, "%s\n",
            D_TEST_PRINTER_SEPARATOR_DOUBLE);

    return;
}


/******************************************************************************
 * TEMPLATE SUBSTITUTION
 *****************************************************************************/

char*
d_test_substitute_template
(
    const char* _template,
    const char* _delimiters[2],
    size_t      _kv_count,
    const char* _key_values[][2]
)
{
    char*       result;
    size_t      result_capacity;
    size_t      result_length;
    const char* current;
    const char* next_open;
    char*       placeholder;
    size_t      placeholder_len;
    size_t      i;
    size_t      open_len;
    size_t      close_len;
    const char* close_pos;
    bool        found;
    char*       new_result;
    size_t      remaining;
    size_t      prefix_len;
    size_t      value_len;
    size_t      full_placeholder_len;

    if ( (!_template)         ||
         (!_delimiters)       ||
         (!_delimiters[0])    ||
         (!_delimiters[1])    ||
         (!_delimiters[0][0]) ||
         (!_delimiters[1][0]) ||
         ( (_kv_count > 0) &&
           (!_key_values)) )
    {
        return NULL;
    }

    result_capacity = strlen(_template) * 2;

    if (result_capacity < 1)
    {
        result_capacity = 1;
    }

    result = malloc(result_capacity);

    if (!result)
    {
        return NULL;
    }

    result[0]     = '\0';
    result_length = 0;
    current       = _template;
    open_len      = strlen(_delimiters[0]);
    close_len     = strlen(_delimiters[1]);

    while (*(current))
    {
        next_open = strstr(current, _delimiters[0]);

        if (!next_open)
        {
            remaining = strlen(current);

            d_test_internal_strcat_safe(
                result, result_capacity,
                &result_length,
                current, remaining);

            break;
        }

        if (next_open > current)
        {
            prefix_len =
                (size_t)(next_open - current);

            while (result_length + prefix_len >=
                   result_capacity)
            {
                result_capacity *= 2;
                new_result = (char*)realloc(
                    result, result_capacity);

                if (!new_result)
                {
                    free(result);
                    return NULL;
                }

                result = new_result;
            }

            d_test_internal_strcat_safe(
                result, result_capacity,
                &result_length,
                current, prefix_len);
        }

        close_pos = strstr(next_open + open_len,
                           _delimiters[1]);

        if (!close_pos)
        {
            remaining = strlen(next_open);

            while (result_length + remaining >=
                   result_capacity)
            {
                result_capacity *= 2;
                new_result = (char*)realloc(
                    result, result_capacity);

                if (!new_result)
                {
                    free(result);
                    return NULL;
                }

                result = new_result;
            }

            d_test_internal_strcat_safe(
                result, result_capacity,
                &result_length,
                next_open, remaining);

            break;
        }

        placeholder_len =
            (size_t)(close_pos -
                     (next_open + open_len));
        placeholder = malloc(placeholder_len + 1);

        if (!placeholder)
        {
            free(result);
            return NULL;
        }

        d_strncpy_s(placeholder,
                    (placeholder_len + 1),
                    (next_open + open_len),
                    placeholder_len);

        found = false;

        for (i = 0; i < _kv_count; i++)
        {
            if (strcmp(placeholder,
                       _key_values[i][0]) == 0)
            {
                value_len =
                    strlen(_key_values[i][1]);

                while (result_length + value_len >=
                       result_capacity)
                {
                    result_capacity *= 2;
                    new_result = (char*)realloc(
                        result, result_capacity);

                    if (!new_result)
                    {
                        free(placeholder);
                        free(result);
                        return NULL;
                    }

                    result = new_result;
                }

                d_test_internal_strcat_safe(
                    result, result_capacity,
                    &result_length,
                    _key_values[i][1], value_len);

                found = true;
                break;
            }
        }

        if (!found)
        {
            full_placeholder_len =
                (size_t)(close_pos + close_len -
                         next_open);

            while (result_length +
                   full_placeholder_len >=
                   result_capacity)
            {
                result_capacity *= 2;
                new_result = (char*)realloc(
                    result, result_capacity);

                if (!new_result)
                {
                    free(placeholder);
                    free(result);
                    return NULL;
                }

                result = new_result;
            }

            d_test_internal_strcat_safe(
                result, result_capacity,
                &result_length,
                next_open, full_placeholder_len);
        }

        free(placeholder);
        current = close_pos + close_len;
    }

    return result;
}


/******************************************************************************
 * DEFAULT PRINT FUNCTIONS
 *****************************************************************************/

/*
d_test_printer_print_object
  Default recursive print function for standalone test objects. Reads
name and message from the object's arg list metadata. Children are read
from D_TEST_ARG_CHILDREN / D_TEST_ARG_CHILD_COUNT.
*/
void
d_test_printer_print_object
(
    const struct d_test_object* _obj,
    size_t                      _indent_level,
    struct d_test_counter*      _assertions,
    struct d_test_counter*      _tests
)
{
    const char*            name;
    const char*            msg;
    struct d_test_object** children;
    size_t                 child_count;
    size_t                 i;
    size_t                 j;

    if (!_obj)
    {
        return;
    }

    // print indentation
    for (i = 0; i < _indent_level; i++)
    {
        printf("  ");

        if (g_d_test_printer_output_file)
        {
            fprintf(g_d_test_printer_output_file,
                    "  ");
        }
    }

    if (_obj->is_leaf)
    {
        const char* symbol = _obj->result
            ? D_TEST_SYMBOL_PASS
            : D_TEST_SYMBOL_FAIL;

        msg = d_test_arg_list_get_meta(
                  _obj->args,
                  D_TEST_META_MESSAGE,
                  "");

        D_INTERNAL_TPRINTF("%s %s\n", symbol, msg);

        if (_assertions)
        {
            d_test_counter_increment(
                _assertions, D_TEST_COUNT_TOTAL);

            if (_obj->result)
            {
                d_test_counter_increment(
                    _assertions,
                    D_TEST_COUNT_PASSED);
            }
            else
            {
                d_test_counter_increment(
                    _assertions,
                    D_TEST_COUNT_FAILED);
            }
        }
    }
    else
    {
        name = d_test_arg_list_get_meta(
                   _obj->args,
                   D_TEST_META_NAME,
                   "(unnamed)");

        D_INTERNAL_TPRINTF("--- Testing %s ---\n",
                           name);

        if (_tests)
        {
            d_test_counter_increment(
                _tests, D_TEST_COUNT_TOTAL);
        }

        // get children from args
        children = NULL;
        child_count = 0;

        if (_obj->args)
        {
            children = (struct d_test_object**)
                           d_test_arg_list_get(
                               _obj->args,
                               D_TEST_ARG_CHILDREN);
            child_count = d_test_arg_list_get_size(
                              _obj->args,
                              D_TEST_ARG_CHILD_COUNT,
                              0);
        }

        if (children)
        {
            bool all_passed = true;

            for (j = 0; j < child_count; j++)
            {
                if (children[j])
                {
                    d_test_printer_print_object(
                        children[j],
                        (_indent_level + 1),
                        _assertions, _tests);

                    if ( children[j]->is_leaf &&
                        !children[j]->result )
                    {
                        all_passed = false;
                    }
                }
            }

            if (_tests)
            {
                if (all_passed)
                {
                    d_test_counter_increment(
                        _tests,
                        D_TEST_COUNT_PASSED);
                }
                else
                {
                    d_test_counter_increment(
                        _tests,
                        D_TEST_COUNT_FAILED);
                }
            }
        }
    }

    return;
}

void
d_test_printer_print_object_to_file
(
    FILE*                       _file,
    const struct d_test_object* _obj,
    size_t                      _indent_level,
    struct d_test_counter*      _assertions,
    struct d_test_counter*      _tests
)
{
    const char*            name;
    const char*            msg;
    struct d_test_object** children;
    size_t                 child_count;
    size_t                 i;
    size_t                 j;

    if ( (!_file) ||
         (!_obj) )
    {
        return;
    }

    for (i = 0; i < _indent_level; i++)
    {
        fprintf(_file, "  ");
    }

    if (_obj->is_leaf)
    {
        const char* symbol = _obj->result
            ? D_TEST_SYMBOL_PASS
            : D_TEST_SYMBOL_FAIL;

        msg = d_test_arg_list_get_meta(
                  _obj->args,
                  D_TEST_META_MESSAGE,
                  "");

        fprintf(_file, "%s %s\n", symbol, msg);

        if (_assertions)
        {
            d_test_counter_increment(
                _assertions, D_TEST_COUNT_TOTAL);

            if (_obj->result)
            {
                d_test_counter_increment(
                    _assertions,
                    D_TEST_COUNT_PASSED);
            }
            else
            {
                d_test_counter_increment(
                    _assertions,
                    D_TEST_COUNT_FAILED);
            }
        }
    }
    else
    {
        name = d_test_arg_list_get_meta(
                   _obj->args,
                   D_TEST_META_NAME,
                   "(unnamed)");

        fprintf(_file, "--- Testing %s ---\n", name);

        if (_tests)
        {
            d_test_counter_increment(
                _tests, D_TEST_COUNT_TOTAL);
        }

        children = NULL;
        child_count = 0;

        if (_obj->args)
        {
            children = (struct d_test_object**)
                           d_test_arg_list_get(
                               _obj->args,
                               D_TEST_ARG_CHILDREN);
            child_count = d_test_arg_list_get_size(
                              _obj->args,
                              D_TEST_ARG_CHILD_COUNT,
                              0);
        }

        if (children)
        {
            bool all_passed = true;

            for (j = 0; j < child_count; j++)
            {
                if (children[j])
                {
                    d_test_printer_print_object_to_file(
                        _file, children[j],
                        _indent_level + 1,
                        _assertions, _tests);

                    if ( children[j]->is_leaf &&
                        !children[j]->result )
                    {
                        all_passed = false;
                    }
                }
            }

            if (_tests)
            {
                if (all_passed)
                {
                    d_test_counter_increment(
                        _tests,
                        D_TEST_COUNT_PASSED);
                }
                else
                {
                    d_test_counter_increment(
                        _tests,
                        D_TEST_COUNT_FAILED);
                }
            }
        }
    }

    return;
}


/******************************************************************************
 * OUTPUT FUNCTIONS
 *****************************************************************************/

void
d_test_printer_output_console
(
    const char*                  _delimiters[2],
    size_t                       _kv_count,
    const char*                  _key_values[][2],
    size_t                       _obj_count,
    struct d_test_object* const* _objects,
    fn_print_object              _print_fn
)
{
    size_t                i;
    struct d_test_counter assertions;
    struct d_test_counter tests;
    fn_print_object       print_func;

    (void)_delimiters;
    (void)_kv_count;
    (void)_key_values;

    if (!_objects)
    {
        return;
    }

    d_test_counter_init(&assertions,
                        D_TEST_COUNTER_ASSERT_STD);
    d_test_counter_init(&tests,
                        D_TEST_COUNTER_TEST_STD);

    print_func = (_print_fn)
                     ? _print_fn
                     : d_test_printer_print_object;

    for (i = 0; i < _obj_count; i++)
    {
        if (_objects[i])
        {
            print_func(_objects[i], 0,
                       &assertions, &tests);
        }
    }

    d_test_counter_free(&assertions);
    d_test_counter_free(&tests);

    return;
}

void
d_test_printer_output_file
(
    const char*                  _filepath,
    const char*                  _delimiters[2],
    size_t                       _kv_count,
    const char*                  _key_values[][2],
    size_t                       _obj_count,
    struct d_test_object* const* _objects,
    fn_print_object_file         _print_fn
)
{
    FILE*                 file;
    size_t                i;
    struct d_test_counter assertions;
    struct d_test_counter tests;
    fn_print_object_file  print_func;

    (void)_delimiters;
    (void)_kv_count;
    (void)_key_values;

    if ( (!_filepath) ||
         (!_objects) )
    {
        return;
    }

#if D_CFG_TEST_FILE_ENABLE
    file = d_fopen(_filepath, "w");
#else
    file = fopen(_filepath, "w");
#endif

    if (!file)
    {
        return;
    }

    d_test_counter_init(&assertions,
                        D_TEST_COUNTER_ASSERT_STD);
    d_test_counter_init(&tests,
                        D_TEST_COUNTER_TEST_STD);

    print_func = (_print_fn)
        ? _print_fn
        : d_test_printer_print_object_to_file;

    for (i = 0; i < _obj_count; i++)
    {
        if (_objects[i])
        {
            print_func(file, _objects[i], 0,
                       &assertions, &tests);
        }
    }

    d_test_counter_free(&assertions);
    d_test_counter_free(&tests);

    fclose(file);

    return;
}


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

void
d_test_printer_print_timestamp
(
    void
)
{
    time_t    current_time;
    struct tm time_info;
    char      time_buffer[80];

    time(&current_time);

#if defined(_MSC_VER)
    if (localtime_s(&time_info,
                    &current_time) != 0)
    {
        d_strcpy_s(time_buffer,
                   sizeof(time_buffer),
                   "unknown time");
    }
    else
    {
        if (strftime(time_buffer,
                     sizeof(time_buffer),
                     "%Y-%m-%d %H:%M:%S",
                     &time_info) == 0)
        {
            d_strcpy_s(time_buffer,
                       sizeof(time_buffer),
                       "unknown time");
        }
    }
#else
    if (localtime_r(&current_time,
                    &time_info) == NULL)
    {
        d_strcpy_s(time_buffer,
                   sizeof(time_buffer),
                   "unknown time");
    }
    else
    {
        if (strftime(time_buffer,
                     sizeof(time_buffer),
                     "%Y-%m-%d %H:%M:%S",
                     &time_info) == 0)
        {
            d_strcpy_s(time_buffer,
                       sizeof(time_buffer),
                       "unknown time");
        }
    }
#endif

    printf("%s", time_buffer);

    return;
}


/******************************************************************************
 * STANDALONE OUTPUT FORMATTING FUNCTIONS
 *****************************************************************************/

void
d_test_printer_create_framework_header
(
    const char* _suite_name,
    const char* _description
)
{
    if (!_suite_name)
    {
        _suite_name = "(unknown suite)";
    }

    if (!_description)
    {
        _description = "(no description)";
    }

    printf("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);
    D_INTERNAL_TPRINTF("  TESTING: %s\n",
        _suite_name);
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);
    D_INTERNAL_TPRINTF("  Description: %s\n",
        _description);
    printf("  Date/Time:   ");
    d_test_printer_print_timestamp();
    printf("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);
    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF(
        "  Starting test suite execution...\n");
    D_INTERNAL_TPRINTF("\n");

    return;
}

void
d_test_printer_create_module_test_header
(
    const char* _module_name,
    const char* _description
)
{
    if (!_module_name)
    {
        _module_name = "(unknown)";
    }

    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_SINGLE);

    if (_description)
    {
        D_INTERNAL_TPRINTF("  MODULE: `%s`\n",
                           _module_name);
        D_INTERNAL_TPRINTF("  %s\n", _description);
    }
    else
    {
        D_INTERNAL_TPRINTF("  MODULE: `%s`\n",
                           _module_name);
    }

    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_SINGLE);
    D_INTERNAL_TPRINTF("\n");

    return;
}

/*
d_test_printer_create_module_test_results
  Prints per-module results. Reads counters and pass status from the
module's result arg_list.

Parameter(s):
  _module_name: module name for display
  _result_args: per-module result arg list containing:
                  D_TEST_ARG_ASSERTION_COUNTER, D_TEST_ARG_TEST_COUNTER,
                  D_TEST_ARG_PASSED
*/
void
d_test_printer_create_module_test_results
(
    const char*                  _module_name,
    const struct d_test_arg_list* _result_args
)
{
    const struct d_test_counter* a_ctr;
    const struct d_test_counter* t_ctr;
    double                       assertion_rate;
    double                       test_rate;
    bool                         passed;
    size_t                       a_total;
    size_t                       a_passed;
    size_t                       t_total;
    size_t                       t_passed;

    if (!_module_name)
    {
        _module_name = "(unknown)";
    }

    if (!_result_args)
    {
        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF("%s\n",
            D_TEST_PRINTER_SEPARATOR_SINGLE);
        D_INTERNAL_TPRINTF("  MODULE RESULTS: %s\n",
                           _module_name);
        D_INTERNAL_TPRINTF("%s\n",
            D_TEST_PRINTER_SEPARATOR_SINGLE);
        D_INTERNAL_TPRINTF(
            "  (no results available)\n");
        D_INTERNAL_TPRINTF("%s\n",
            D_TEST_PRINTER_SEPARATOR_SINGLE);

        return;
    }

    a_ctr = D_TEST_ARG_AS_PTR(
                const struct d_test_counter,
                d_test_arg_list_get(
                    _result_args,
                    D_TEST_ARG_ASSERTION_COUNTER));
    t_ctr = D_TEST_ARG_AS_PTR(
                const struct d_test_counter,
                d_test_arg_list_get(
                    _result_args,
                    D_TEST_ARG_TEST_COUNTER));

    a_total  = d_test_counter_get(a_ctr,
                                  D_TEST_COUNT_TOTAL);
    a_passed = d_test_counter_get(a_ctr,
                                  D_TEST_COUNT_PASSED);
    t_total  = d_test_counter_get(t_ctr,
                                  D_TEST_COUNT_TOTAL);
    t_passed = d_test_counter_get(t_ctr,
                                  D_TEST_COUNT_PASSED);

    assertion_rate = (a_total > 0)
        ? (100.0 * (double)a_passed /
                    (double)a_total)
        : 0.0;

    test_rate = (t_total > 0)
        ? (100.0 * (double)t_passed /
                    (double)t_total)
        : 0.0;

    passed = d_test_arg_list_get_bool(
                 _result_args,
                 D_TEST_ARG_PASSED,
                 false);

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_SINGLE);
    D_INTERNAL_TPRINTF("  MODULE RESULTS: %s\n",
                       _module_name);
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_SINGLE);
    D_INTERNAL_TPRINTF(
        "  Assertions: %zu/%zu passed (%.2f%%)\n",
        a_passed, a_total, assertion_rate);
    D_INTERNAL_TPRINTF(
        "  Unit Tests: %zu/%zu passed (%.2f%%)\n",
        t_passed, t_total, test_rate);

    if (passed)
    {
        D_INTERNAL_TPRINTF(
            "  Status:     %s %s MODULE PASSED\n",
            D_TEST_SYMBOL_PASS,
            _module_name);
    }
    else
    {
        D_INTERNAL_TPRINTF(
            "  Status:     %s %s MODULE FAILED\n",
            D_TEST_SYMBOL_FAIL,
            _module_name);
    }

    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_SINGLE);

    return;
}

/*
d_test_printer_create_comprehensive_results
  Prints comprehensive suite results. All data is read from the
runner's arg_list.

Parameter(s):
  _runner_args: the runner's arg list containing:
                  D_TEST_ARG_MODULES_TOTAL, D_TEST_ARG_MODULES_PASSED,
                  D_TEST_ARG_ASSERTION_COUNTER, D_TEST_ARG_TEST_COUNTER,
                  D_TEST_ARG_ELAPSED_TIME
*/
void
d_test_printer_create_comprehensive_results
(
    const struct d_test_arg_list* _runner_args
)
{
    const struct d_test_counter* a_ctr;
    const struct d_test_counter* t_ctr;
    const double*                elapsed;
    double                       module_rate;
    double                       assertion_rate;
    double                       test_rate;
    size_t                       a_total;
    size_t                       a_passed;
    size_t                       a_failed;
    size_t                       t_total;
    size_t                       t_passed;
    size_t                       t_failed;
    size_t                       mod_total;
    size_t                       mod_passed;
    bool                         all_passed;

    if (!_runner_args)
    {
        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF("%s\n",
            D_TEST_PRINTER_SEPARATOR_DOUBLE);
        D_INTERNAL_TPRINTF(
            "  COMPREHENSIVE TEST RESULTS\n");
        D_INTERNAL_TPRINTF("%s\n",
            D_TEST_PRINTER_SEPARATOR_DOUBLE);
        D_INTERNAL_TPRINTF(
            "  (no suite results available)\n");
        D_INTERNAL_TPRINTF("%s\n",
            D_TEST_PRINTER_SEPARATOR_DOUBLE);

        return;
    }

    mod_total  = d_test_arg_list_get_size(
                     _runner_args,
                     D_TEST_ARG_MODULES_TOTAL, 0);
    mod_passed = d_test_arg_list_get_size(
                     _runner_args,
                     D_TEST_ARG_MODULES_PASSED, 0);

    a_ctr = D_TEST_ARG_AS_PTR(
                const struct d_test_counter,
                d_test_arg_list_get(
                    _runner_args,
                    D_TEST_ARG_ASSERTION_COUNTER));
    t_ctr = D_TEST_ARG_AS_PTR(
                const struct d_test_counter,
                d_test_arg_list_get(
                    _runner_args,
                    D_TEST_ARG_TEST_COUNTER));

    a_total  = d_test_counter_get(a_ctr,
                                  D_TEST_COUNT_TOTAL);
    a_passed = d_test_counter_get(a_ctr,
                                  D_TEST_COUNT_PASSED);
    a_failed = d_test_counter_get(a_ctr,
                                  D_TEST_COUNT_FAILED);
    t_total  = d_test_counter_get(t_ctr,
                                  D_TEST_COUNT_TOTAL);
    t_passed = d_test_counter_get(t_ctr,
                                  D_TEST_COUNT_PASSED);
    t_failed = d_test_counter_get(t_ctr,
                                  D_TEST_COUNT_FAILED);

    elapsed = D_TEST_ARG_AS_PTR(
                  const double,
                  d_test_arg_list_get(
                      _runner_args,
                      D_TEST_ARG_ELAPSED_TIME));

    module_rate = (mod_total > 0)
        ? (100.0 * (double)mod_passed /
                    (double)mod_total)
        : 0.0;

    assertion_rate = (a_total > 0)
        ? (100.0 * (double)a_passed /
                    (double)a_total)
        : 0.0;

    test_rate = (t_total > 0)
        ? (100.0 * (double)t_passed /
                    (double)t_total)
        : 0.0;

    all_passed = (mod_passed == mod_total);

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);
    D_INTERNAL_TPRINTF(
        "  COMPREHENSIVE TEST RESULTS\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("  MODULE SUMMARY:\n");
    D_INTERNAL_TPRINTF(
        "    Modules Tested:       %zu\n",
        mod_total);
    D_INTERNAL_TPRINTF(
        "    Modules Passed:       %zu\n",
        mod_passed);
    D_INTERNAL_TPRINTF(
        "    Module Success Rate:  %.2f%%\n",
        module_rate);

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("  ASSERTION SUMMARY:\n");
    D_INTERNAL_TPRINTF(
        "    Total Assertions:     %zu\n",
        a_total);
    D_INTERNAL_TPRINTF(
        "    Assertions Passed:    %zu\n",
        a_passed);
    D_INTERNAL_TPRINTF(
        "    Assertions Failed:    %zu\n",
        a_failed);
    D_INTERNAL_TPRINTF(
        "    Assertion Pass Rate:  %.2f%%\n",
        assertion_rate);

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("  UNIT TEST SUMMARY:\n");
    D_INTERNAL_TPRINTF(
        "    Total Unit Tests:     %zu\n",
        t_total);
    D_INTERNAL_TPRINTF(
        "    Unit Tests Passed:    %zu\n",
        t_passed);
    D_INTERNAL_TPRINTF(
        "    Unit Tests Failed:    %zu\n",
        t_failed);
    D_INTERNAL_TPRINTF(
        "    Unit Test Pass Rate:  %.2f%%\n",
        test_rate);

    if ( (elapsed) &&
         (*elapsed > 0.0) )
    {
        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF("  EXECUTION TIME:\n");
        D_INTERNAL_TPRINTF(
            "    Total Time:           %.3f seconds\n",
            *elapsed);
    }

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("  OVERALL ASSESSMENT:\n");

    if (all_passed)
    {
        D_INTERNAL_TPRINTF(
            "    %s ALL TESTS PASSED\n",
            D_TEST_SYMBOL_PASS);
        D_INTERNAL_TPRINTF(
            "    %s Framework is ready for use\n",
            D_TEST_SYMBOL_PASS);
    }
    else
    {
        D_INTERNAL_TPRINTF(
            "    %s SOME TESTS FAILED - "
            "ATTENTION REQUIRED\n",
            D_TEST_SYMBOL_FAIL);
        D_INTERNAL_TPRINTF(
            "    %s Review failed tests before "
            "proceeding\n",
            D_TEST_SYMBOL_FAIL);
        D_INTERNAL_TPRINTF(
            "    %s Check for memory leaks or "
            "logic errors\n",
            D_TEST_SYMBOL_FAIL);
        D_INTERNAL_TPRINTF(
            "    %s Verify all edge cases are "
            "handled properly\n",
            D_TEST_SYMBOL_FAIL);
    }

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);

    return;
}

void
d_test_printer_create_implementation_notes
(
    size_t                            _section_count,
    const struct d_test_note_section* _sections
)
{
    size_t i;
    size_t j;

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);
    D_INTERNAL_TPRINTF(
        "  IMPLEMENTATION NOTES &"
        " RECOMMENDATIONS\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);

    if ( (!_sections) ||
         (_section_count == 0) )
    {
        D_INTERNAL_TPRINTF(
            "  (no implementation notes)\n");
        D_INTERNAL_TPRINTF("%s\n",
            D_TEST_PRINTER_SEPARATOR_DOUBLE);

        return;
    }

    for (i = 0; i < _section_count; i++)
    {
        if (!_sections[i].title)
        {
            continue;
        }

        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF("  %s:\n",
                           _sections[i].title);

        if ( _sections[i].items &&
             _sections[i].count > 0 )
        {
            for (j = 0;
                 j < _sections[i].count;
                 j++)
            {
                const char* message =
                    _sections[i].items[j].message
                        ? _sections[i].items[j].message
                        : "";

                D_INTERNAL_TPRINTF("    %s\n",
                                   message);
            }
        }
    }

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);

    return;
}

void
d_test_printer_create_final_status
(
    const char* _framework_name,
    bool        _passed
)
{
    if (!_framework_name)
    {
        _framework_name = "(unknown)";
    }

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);
    D_INTERNAL_TPRINTF("  FINAL STATUS\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);

    if (_passed)
    {
        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF(
            "  %s %s Test Suite COMPLETED "
            "SUCCESSFULLY\n",
            D_TEST_SYMBOL_SUCCESS,
            _framework_name);
        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF(
            "  %s All modules validated and ready "
            "for use\n",
            D_TEST_SYMBOL_PASS);
        D_INTERNAL_TPRINTF(
            "  %s Proceed with confidence in "
            "framework stability\n",
            D_TEST_SYMBOL_PASS);
    }
    else
    {
        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF(
            "  %s %s Test Suite COMPLETED "
            "WITH FAILURES\n",
            D_TEST_SYMBOL_FAIL,
            _framework_name);
        D_INTERNAL_TPRINTF("\n");
        D_INTERNAL_TPRINTF(
            "  %s Review and fix all failures "
            "before proceeding\n",
            D_TEST_SYMBOL_FAIL);
        D_INTERNAL_TPRINTF(
            "  %s Framework stability may be "
            "compromised\n",
            D_TEST_SYMBOL_FAIL);
    }

    D_INTERNAL_TPRINTF("\n");
    D_INTERNAL_TPRINTF("%s\n",
        D_TEST_PRINTER_SEPARATOR_DOUBLE);

    return;
}
