#include "..\..\..\inc\c\test\test_standalone.h"


/******************************************************************************
 * INTERNAL HELPER FUNCTIONS
 *****************************************************************************/

/*
d_test_internal_strcat_safe
  Internal helper to safely concatenate strings with capacity tracking.

Parameter(s):
  _dest:          destination buffer
  _dest_capacity: total capacity of destination buffer
  _dest_len:      current length of string in destination (updated on success)
  _src:           source string to append
  _src_len:       length of source string to append
Return:
  0 on success, non-zero on failure (buffer too small).
*/
static int
d_test_internal_strcat_safe
(
    char*       _dest,
    size_t      _dest_capacity,
    size_t*     _dest_len,
    const char* _src,
    size_t      _src_len
)
{
    if ( (!_dest)     ||
         (!_dest_len) ||
         (!_src) )
    {
        return EINVAL;
    }

    // check if there's room (need space for src + null terminator)
    if (*_dest_len + _src_len >= _dest_capacity)
    {
        return ERANGE;
    }

    // copy source to end of destination
    d_memcpy(_dest + *_dest_len, _src, _src_len);
    *_dest_len        += _src_len;
    _dest[*_dest_len]  = '\0';

    return 0;
}


/******************************************************************************
 * ASSERTION FUNCTION
 *****************************************************************************/

/*
d_assert_standalone
  Performs an assertion and updates the test counter.

Parameter(s):
  _condition: the condition to test
  _test_name: name of the test/assertion
  _message:   message to display on failure
  _test_info: counter to update (can be NULL)
Return:
  The condition value (true if passed, false if failed).
*/
D_STATIC_INLINE bool
d_assert_standalone
(
    bool                   _condition,
    const char*            _test_name,
    const char*            _message,
    struct d_test_counter* _test_info
)
{
    if (!_test_info)
    {
        return _condition;
    }

    _test_info->assertions_total++;

    if (_condition)
    {
        _test_info->assertions_passed++;
        printf("%s    %s %s\n", D_INDENT, D_TEST_SYMBOL_PASS, _test_name);
    }
    else
    {
        printf("%s    %s %s - %s\n",
               D_INDENT,
               D_TEST_SYMBOL_FAIL,
               _test_name,
               _message);
    }

    return _condition;
}


/******************************************************************************
 * TEST OBJECT FUNCTIONS
 *****************************************************************************/

/*
d_test_object_new_leaf
  Creates a new leaf test object (assertion).

Parameter(s):
  _name:    name of the assertion
  _message: result message to display
  _result:  whether the assertion passed (true) or failed (false)
Return:
  Pointer to newly created test object, or NULL on allocation failure.
*/
struct d_test_object*
d_test_object_new_leaf
(
    const char* _name,
    const char* _message,
    bool        _result
)
{
    struct d_test_object* obj;

    obj = malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return NULL;
    }

    obj->is_leaf  = true;
    obj->name     = _name;
    obj->message  = _message;
    obj->result   = _result;
    obj->args     = NULL;
    obj->count    = 0;
    obj->elements = NULL;

    return obj;
}

/*
d_test_object_new_interior
  Creates a new interior test object (test group/block).

Parameter(s):
  _name:        name of the test group
  _child_count: number of children this group will have
Return:
  Pointer to newly created test object with allocated child array, or NULL
  on allocation failure.
*/
struct d_test_object*
d_test_object_new_interior
(
    const char* _name,
    size_t      _child_count
)
{
    struct d_test_object* obj;

    obj = malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return NULL;
    }

    obj->is_leaf = false;
    obj->name    = _name;
    obj->message = NULL;
    obj->result  = false;
    obj->args    = NULL;
    obj->count   = _child_count;

    if (_child_count > 0)
    {
        obj->elements = (struct d_test_object**)calloc(_child_count,
                                                       sizeof(struct d_test_object*));

        if (!obj->elements)
        {
            free(obj);

            return NULL;
        }
    }
    else
    {
        obj->elements = NULL;
    }

    return obj;
}

/*
d_test_object_free
  Recursively frees a test object and all its children.

Parameter(s):
  _obj: test object to free
Return:
  none.
*/
void
d_test_object_free
(
    struct d_test_object* _obj
)
{
    size_t i;

    if (!_obj)
    {
        return;
    }

    // recursively free children
    if (_obj->elements)
    {
        for (i = 0; i < _obj->count; i++)
        {
            d_test_object_free(_obj->elements[i]);
        }

        free(_obj->elements);
    }

    // free args if present
    if (_obj->args)
    {
        if (_obj->args->args)
        {
            free(_obj->args->args);
        }

        free(_obj->args);
    }

    free(_obj);

    return;
}

/*
d_test_object_add_child
  Adds a child test object to a parent at the specified index.

Parameter(s):
  _parent: parent test object (must be interior type)
  _child:  child test object to add
  _index:  index position to add child at
Return:
  none.
*/
void
d_test_object_add_child
(
    struct d_test_object* _parent,
    struct d_test_object* _child,
    size_t                _index
)
{
    if ( (!_parent) ||
         (!_child) )
    {
        return;
    }

    if (_parent->is_leaf == true)
    {
        return;
    }

    if (_index >= _parent->count)
    {
        return;
    }

    _parent->elements[_index] = _child;

    return;
}


/******************************************************************************
 * TEST COUNTER FUNCTIONS
 *****************************************************************************/

/*
d_test_counter_reset
  Resets all counter values to zero.

Parameter(s):
  _counter: counter to reset
Return:
  none.
*/
void
d_test_counter_reset
(
    struct d_test_counter* _counter
)
{
    if (!_counter)
    {
        return;
    }

    _counter->assertions_total  = 0;
    _counter->assertions_passed = 0;
    _counter->tests_total       = 0;
    _counter->tests_passed      = 0;

    return;
}

/*
d_test_counter_add
  Adds source counter values to destination counter.

Parameter(s):
  _dest: destination counter to add to
  _src:  source counter to add from
Return:
  none.
*/
void
d_test_counter_add
(
    struct d_test_counter*       _dest,
    const struct d_test_counter* _src
)
{
    if ( (!_dest) ||
         (!_src) )
    {
        return;
    }

    _dest->assertions_total  += _src->assertions_total;
    _dest->assertions_passed += _src->assertions_passed;
    _dest->tests_total       += _src->tests_total;
    _dest->tests_passed      += _src->tests_passed;

    return;
}


/******************************************************************************
 * TEMPLATE SUBSTITUTION
 *****************************************************************************/

/*
d_test_substitute_template
  Substitutes key-value pairs into a template string using delimiters.

Parameter(s):
  _template:   template string with delimited placeholders
  _delimiters: array of 2 strings - opening and closing delimiters
  _kv_count:   number of key-value pairs
  _key_values: 2D array of key-value string pairs
Return:
  Newly allocated string with substitutions performed, or NULL on failure.
  Caller must free returned string.
*/
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

    // validate parameters
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

    // ensure minimum capacity for null terminator
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
            // no more placeholders, copy rest of string
            remaining = strlen(current);

            // ... grow buffer if needed ...
            d_test_internal_strcat_safe(result,
                                        result_capacity,
                                        &result_length,
                                        current,
                                        remaining);
            break;
        }

        // copy text before placeholder
        if (next_open > current)
        {
            prefix_len = (size_t)(next_open - current);

            while (result_length + prefix_len >= result_capacity)
            {
                result_capacity *= 2;
                new_result       = (char*)realloc(result, result_capacity);

                if (!new_result)
                {
                    free(result);

                    return NULL;
                }

                result = new_result;
            }

            d_test_internal_strcat_safe(result,
                                        result_capacity,
                                        &(result_length),
                                        current,
                                        prefix_len);
        }

        // find closing delimiter
        close_pos = strstr(next_open + open_len, _delimiters[1]);

        if (!close_pos)
        {
            // no closing delimiter, copy rest as-is
            remaining = strlen(next_open);

            while (result_length + remaining >= result_capacity)
            {
                result_capacity *= 2;
                new_result       = (char*)realloc(result, result_capacity);

                if (!new_result)
                {
                    free(result);

                    return NULL;
                }

                result = new_result;
            }

             d_test_internal_strcat_safe(result,
                                        result_capacity,
                                        &(result_length),
                                        next_open,
                                        remaining);
            break;
        }

        // extract placeholder key
        placeholder_len = (size_t)(close_pos - (next_open + open_len));
        placeholder     = malloc(placeholder_len + 1);

        if (!placeholder)
        {
            free(result);

            return NULL;
        }

        // safe copy of placeholder key
        d_strncpy_s(placeholder,
                    (placeholder_len + 1),
                    (next_open + open_len),
                    placeholder_len);

        // find matching key-value pair
        found = false;

        for (i = 0; i < _kv_count; i++)
        {
            if (strcmp(placeholder, _key_values[i][0]) == 0)
            {
                value_len = strlen(_key_values[i][1]);

                while (result_length + value_len >= result_capacity)
                {
                    result_capacity *= 2;
                    new_result       = (char*)realloc(result, result_capacity);

                    if (!new_result)
                    {
                        free(placeholder);
                        free(result);

                        return NULL;
                    }

                    result = new_result;
                }

                d_test_internal_strcat_safe(result,
                                            result_capacity,
                                            &result_length,
                                            _key_values[i][1],
                                            value_len);
                found = true;
                break;
            }
        }

        if (!found)
        {
            // key not found, keep placeholder as-is
            full_placeholder_len = (size_t)(close_pos + close_len - next_open);

            while (result_length + full_placeholder_len >= result_capacity)
            {
                result_capacity *= 2;
                new_result       = (char*)realloc(result, result_capacity);

                if (!new_result)
                {
                    free(placeholder);
                    free(result);

                    return NULL;
                }

                result = new_result;
            }

            d_test_internal_strcat_safe(result,
                                        result_capacity,
                                        &result_length,
                                        next_open,
                                        full_placeholder_len);
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
d_test_default_print_object
  Default recursive print function for test objects.
  Prints leaves as assertions with pass/fail symbols, interior nodes as
  groups with nested indentation.

Parameter(s):
  _obj:          test object to print
  _indent_level: current indentation level
  _counter:      optional counter to update (can be NULL)
Return:
  none.
*/
void
d_test_default_print_object
(
    const struct d_test_object* _obj,
    size_t                      _indent_level,
    struct d_test_counter*      _counter
)
{
    size_t i;
    size_t j;

    if (!_obj)
    {
        return;
    }

    // print indentation
    for (i = 0; i < _indent_level; i++)
    {
        printf("  ");
    }

    if (_obj->is_leaf)
    {
        // leaf node - print assertion result
        const char* symbol = _obj->result
            ? D_TEST_SYMBOL_PASS 
            : D_TEST_SYMBOL_FAIL;

        printf("%s %s\n", symbol, _obj->message 
            ? _obj->message
            : "");

        // update counter if provided
        if (_counter)
        {
            _counter->assertions_total++;

            if (_obj->result)
            {
                _counter->assertions_passed++;
            }
        }
    }
    else
    {
        // interior node - print group header and recurse
        printf("--- Testing %s ---\n", _obj->name 
            ? _obj->name
            : "(unnamed)");

        // update test counter for interior nodes
        if (_counter)
        {
            _counter->tests_total++;
        }

        // track if all children passed
        bool all_passed = true;

        for (j = 0; j < _obj->count; j++)
        {
            if (_obj->elements[j])
            {
                d_test_default_print_object(_obj->elements[j],
                                            (_indent_level + 1),
                                            _counter);

                // check if this child (if leaf) failed
                if ( _obj->elements[j]->is_leaf &&
                    !_obj->elements[j]->result )
                {
                    all_passed = false;
                }
            }
        }

        // update test pass counter
        if ( (_counter) &&
             (all_passed) )
        {
            _counter->tests_passed++;
        }
    }

    return;
}

/*
d_test_default_print_object_to_file
  Default recursive print function for test objects to a FILE* stream.
  Prints leaves as assertions with pass/fail symbols, interior nodes as
  groups with nested indentation.

Parameter(s):
  _file:         file stream to print to
  _obj:          test object to print
  _indent_level: current indentation level
  _counter:      optional counter to update (can be NULL)
Return:
  none.
*/
void
d_test_default_print_object_to_file
(
    FILE*                       _file,
    const struct d_test_object* _obj,
    size_t                      _indent_level,
    struct d_test_counter*      _counter
)
{
    size_t i;
    size_t j;

    if ( (!_file) ||
         (!_obj) )
    {
        return;
    }

    // print indentation
    for (i = 0; i < _indent_level; i++)
    {
        fprintf(_file, "  ");
    }

    if (_obj->is_leaf)
    {
        // leaf node - print assertion result
        const char* symbol = _obj->result 
            ? D_TEST_SYMBOL_PASS
            : D_TEST_SYMBOL_FAIL;

        fprintf(_file, "%s %s\n", symbol, _obj->message ? _obj->message : "");

        // update counter if provided
        if (_counter)
        {
            _counter->assertions_total++;

            if (_obj->result)
            {
                _counter->assertions_passed++;
            }
        }
    }
    else
    {
        // interior node - print group header and recurse
        fprintf(_file, "--- Testing %s ---\n", _obj->name
            ? _obj->name
            : "(unnamed)");

        // update test counter
        if (_counter)
        {
            _counter->tests_total++;
        }

        bool all_passed = true;

        for (j = 0; j < _obj->count; j++)
        {
            if (_obj->elements[j])
            {
                d_test_default_print_object_to_file(_file,
                                                    _obj->elements[j],
                                                    _indent_level + 1,
                                                    _counter);

                if ( _obj->elements[j]->is_leaf &&
                    !_obj->elements[j]->result )
                {
                    all_passed = false;
                }
            }
        }

        // update test pass counter
        if ( (_counter) &&
             (all_passed) )
        {
            _counter->tests_passed++;
        }
    }

    return;
}


/******************************************************************************
 * OUTPUT FUNCTIONS
 *****************************************************************************/

/*
d_test_standalone_output_console
  Outputs test results to console with template substitution.

Parameter(s):
  _delimiters: array of 2 strings for template delimiters (open, close)
  _kv_count:   number of key-value pairs for substitution
  _key_values: 2D array of key-value string pairs
  _obj_count:  number of test objects to print
  _objects:    array of test object pointers
  _print_fn:   optional custom print function (NULL for default)
Return:
  none.
*/
void
d_test_standalone_output_console
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
    struct d_test_counter counter;
    fn_print_object       print_func;

    // suppress unused parameters
    (void)_delimiters;
    (void)_kv_count;
    (void)_key_values;

    if (!_objects)
    {
        return;
    }

    d_test_counter_reset(&counter);

    print_func = (_print_fn)
                     ? _print_fn
                     : d_test_default_print_object;

    // print each test object
    for (i = 0; i < _obj_count; i++)
    {
        if (_objects[i])
        {
            print_func(_objects[i], 0, &counter);
        }
    }

    return;
}

/*
d_test_standalone_output_file
  Outputs test results to file with template substitution.

Parameter(s):
  _filepath:   path to output file
  _delimiters: array of 2 strings for template delimiters (open, close)
  _kv_count:   number of key-value pairs for substitution
  _key_values: 2D array of key-value string pairs
  _obj_count:  number of test objects to print
  _objects:    array of test object pointers
  _print_fn:   optional custom FILE*-aware print function (NULL for default)
Return:
  none.
*/
void
d_test_standalone_output_file
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
    struct d_test_counter counter;
    fn_print_object_file  print_func;

    // suppress unused parameters
    (void)_delimiters;
    (void)_kv_count;
    (void)_key_values;

    if ( (!_filepath) ||
         (!_objects) )
    {
        return;
    }

    file = d_fopen(_filepath, "w");

    if (!file)
    {
        return;
    }

    d_test_counter_reset(&counter);
    print_func = (_print_fn)
                     ? _print_fn
                     : d_test_default_print_object_to_file;

    // print each test object
    for (i = 0; i < _obj_count; i++)
    {
        if (_objects[i])
        {
            print_func(file, _objects[i], 0, &counter);
        }
    }

    fclose(file);

    return;
}


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_test_sa_print_timestamp
  Prints the current timestamp in YYYY-MM-DD HH:MM:SS format.

Parameter(s):
  none.
Return:
  none.
*/
void
d_test_sa_print_timestamp
(
    void
)
{
    time_t    current_time;
    struct tm time_info;
    char      time_buffer[80];

    time(&current_time);

#if defined(_MSC_VER)
    // MSVC: use localtime_s (parameters reversed from POSIX)
    if (localtime_s(&time_info, &current_time) != 0)
    {
        d_strcpy_s(time_buffer, sizeof(time_buffer), "unknown time");
    }
    else
    {
        if (strftime(time_buffer,
                     sizeof(time_buffer),
                     "%Y-%m-%d %H:%M:%S",
                     &time_info) == 0)
        {
            d_strcpy_s(time_buffer, sizeof(time_buffer), "unknown time");
        }
    }
#else
    // POSIX: use localtime_r
    if (localtime_r(&current_time, &time_info) == NULL)
    {
        d_strcpy_s(time_buffer, sizeof(time_buffer), "unknown time");
    }
    else
    {
        if (strftime(time_buffer,
                     sizeof(time_buffer),
                     "%Y-%m-%d %H:%M:%S",
                     &time_info) == 0)
        {
            d_strcpy_s(time_buffer, sizeof(time_buffer), "unknown time");
        }
    }
#endif

    printf("%s", time_buffer);

    return;
}

/*
d_test_sa_get_elapsed_time
  Calculates elapsed time in seconds from clock ticks.

Parameter(s):
  _start: starting clock value
  _end:   ending clock value
Return:
  Elapsed time in seconds as a double.
*/
double
d_test_sa_get_elapsed_time
(
    clock_t _start,
    clock_t _end
)
{
    return (double)(_end - _start) / CLOCKS_PER_SEC;
}


/******************************************************************************
 * STANDALONE TEST OUTPUT FORMATTING FUNCTIONS
 *****************************************************************************/

/*
d_test_sa_create_framework_header
  Prints the framework header banner for a test suite.
  Creates the top-level banner with suite name, description, and timestamp.

Parameter(s):
  _suite_name:  name of the test suite
  _description: brief description of the test suite
Return:
  none.
*/
void
d_test_sa_create_framework_header
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
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("  TESTING: %s\n", _suite_name);
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("  Description: %s\n", _description);
    printf("  Date/Time:   ");
    d_test_sa_print_timestamp();
    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("\n");
    printf("  Starting test suite execution...\n");
    printf("\n");

    return;
}

/*
d_test_sa_create_module_test_header
  Prints the header for a module's test section.

Parameter(s):
  _module_name: name of the module being tested
  _description: brief description of the module
Return:
  none.
*/
void
d_test_sa_create_module_test_header
(
    const char* _module_name,
    const char* _description
)
{
    if (!_module_name)
    {
        _module_name = "(unknown)";
    }

    printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (_description)
    {
        printf("  MODULE: `%s`\n", _module_name);
        printf("  %s\n", _description);
    }
    else
    {
        printf("  MODULE: `%s`\n", _module_name);
    }

    printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);
    printf("\n");

    return;
}

/*
d_test_sa_create_module_test_results
  Prints the results summary for a single test module.

Parameter(s):
  _module_name: name of the module
  _counter:     test counter with assertion/test results
Return:
  none.
*/
void
d_test_sa_create_module_test_results
(
    const char*                  _module_name,
    const struct d_test_counter* _counter
)
{
    double assertion_rate;
    double test_rate;
    bool   passed;

    if (!_module_name)
    {
        _module_name = "(unknown)";
    }

    if (!_counter)
    {
        printf("\n");
        printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);
        printf("  MODULE RESULTS: %s\n", _module_name);
        printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);
        printf("  (no results available)\n");
        printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);

        return;
    }

    // calculate percentages
    assertion_rate = (_counter->assertions_total > 0)
        ? (100.0 * (double)_counter->assertions_passed /
                   (double)_counter->assertions_total)
        : 0.0;

    test_rate = (_counter->tests_total > 0)
        ? (100.0 * (double)_counter->tests_passed /
                   (double)_counter->tests_total)
        : 0.0;

    passed = (_counter->assertions_passed == _counter->assertions_total) &&
             (_counter->tests_passed == _counter->tests_total);

    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);
    printf("  MODULE RESULTS: %s\n", _module_name);
    printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);
    printf("  Assertions: %zu/%zu passed (%.2f%%)\n",
           _counter->assertions_passed,
           _counter->assertions_total,
           assertion_rate);
    printf("  Unit Tests: %zu/%zu passed (%.2f%%)\n",
           _counter->tests_passed,
           _counter->tests_total,
           test_rate);

    if (passed)
    {
        printf("  Status:     %s %s MODULE PASSED\n",
               D_TEST_SYMBOL_PASS,
               _module_name);
    }
    else
    {
        printf("  Status:     %s %s MODULE FAILED\n",
               D_TEST_SYMBOL_FAIL,
               _module_name);
    }

    printf("%s\n", D_TEST_SA_SEPARATOR_SINGLE);

    return;
}

/*
d_test_sa_create_comprehensive_results
  Prints comprehensive results for an entire test suite.

Parameter(s):
  _suite: suite results structure with all module data
Return:
  none.
*/
void
d_test_sa_create_comprehensive_results
(
    const struct d_test_sa_suite_results* _suite
)
{
    double module_rate;
    double assertion_rate;
    double test_rate;
    size_t assertions_failed;
    size_t tests_failed;
    bool   all_passed;

    if (!_suite)
    {
        printf("\n");
        printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
        printf("  COMPREHENSIVE TEST RESULTS\n");
        printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
        printf("  (no suite results available)\n");
        printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

        return;
    }

    // calculate rates
    module_rate = (_suite->modules_total > 0)
        ? (100.0 * (double)_suite->modules_passed /
                   (double)_suite->modules_total)
        : 0.0;

    assertion_rate = (_suite->totals.assertions_total > 0)
        ? (100.0 * (double)_suite->totals.assertions_passed /
                   (double)_suite->totals.assertions_total)
        : 0.0;

    test_rate = (_suite->totals.tests_total > 0)
        ? (100.0 * (double)_suite->totals.tests_passed /
                   (double)_suite->totals.tests_total)
        : 0.0;

    assertions_failed = _suite->totals.assertions_total -
                        _suite->totals.assertions_passed;
    tests_failed      = _suite->totals.tests_total -
                        _suite->totals.tests_passed;
    all_passed        = (_suite->modules_passed == _suite->modules_total);

    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("  COMPREHENSIVE TEST RESULTS\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    // module summary
    printf("\n");
    printf("  MODULE SUMMARY:\n");
    printf("    Modules Tested:       %zu\n", _suite->modules_total);
    printf("    Modules Passed:       %zu\n", _suite->modules_passed);
    printf("    Module Success Rate:  %.2f%%\n", module_rate);

    // assertion summary
    printf("\n");
    printf("  ASSERTION SUMMARY:\n");
    printf("    Total Assertions:     %zu\n", _suite->totals.assertions_total);
    printf("    Assertions Passed:    %zu\n", _suite->totals.assertions_passed);
    printf("    Assertions Failed:    %zu\n", assertions_failed);
    printf("    Assertion Pass Rate:  %.2f%%\n", assertion_rate);

    // unit test summary
    printf("\n");
    printf("  UNIT TEST SUMMARY:\n");
    printf("    Total Unit Tests:     %zu\n", _suite->totals.tests_total);
    printf("    Unit Tests Passed:    %zu\n", _suite->totals.tests_passed);
    printf("    Unit Tests Failed:    %zu\n", tests_failed);
    printf("    Unit Test Pass Rate:  %.2f%%\n", test_rate);

    // execution time if available
    if (_suite->total_time > 0.0)
    {
        printf("\n");
        printf("  EXECUTION TIME:\n");
        printf("    Total Time:           %.3f seconds\n", _suite->total_time);
    }

    // overall assessment
    printf("\n");
    printf("  OVERALL ASSESSMENT:\n");

    if (all_passed)
    {
        printf("    %s ALL TESTS PASSED\n", D_TEST_SYMBOL_PASS);
        printf("    %s Framework is ready for use\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("    %s SOME TESTS FAILED - ATTENTION REQUIRED\n",
               D_TEST_SYMBOL_FAIL);
        printf("    %s Review failed tests before proceeding\n",
               D_TEST_SYMBOL_FAIL);
        printf("    %s Check for memory leaks or logic errors\n",
               D_TEST_SYMBOL_FAIL);
        printf("    %s Verify all edge cases are handled properly\n",
               D_TEST_SYMBOL_FAIL);
    }

    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    return;
}

/*
d_test_sa_create_implementation_notes
  Prints implementation notes and recommendations.

Parameter(s):
  _section_count: number of note sections
  _sections:      array of note sections to display
Return:
  none.
*/
void
d_test_sa_create_implementation_notes
(
    size_t                               _section_count,
    const struct d_test_sa_note_section* _sections
)
{
    size_t i;
    size_t j;

    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("  IMPLEMENTATION NOTES & RECOMMENDATIONS\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    if ( (!_sections) ||
         (_section_count == 0) )
    {
        printf("  (no implementation notes)\n");
        printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

        return;
    }

    for (i = 0; i < _section_count; i++)
    {
        if (!_sections[i].title)
        {
            continue;
        }

        printf("\n");
        printf("  %s:\n", _sections[i].title);

        if ( _sections[i].items &&
             _sections[i].count > 0 )
        {
            for (j = 0; j < _sections[i].count; j++)
            {
                const char* prefix = _sections[i].items[j].prefix
                                         ? _sections[i].items[j].prefix
                                         : "[INFO]";
                const char* message = _sections[i].items[j].message
                                          ? _sections[i].items[j].message
                                          : "";

                printf("    %s %s\n", prefix, message);
            }
        }
    }

    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    return;
}

/*
d_test_sa_create_final_status
  Prints the final status line for the test suite.

Parameter(s):
  _framework_name: name of the framework/suite
  _passed:         true if all tests passed, false otherwise
Return:
  none.
*/
void
d_test_sa_create_final_status
(
    const char* _framework_name,
    bool        _passed
)
{
    if (!_framework_name)
    {
        _framework_name = "(unknown)";
    }

    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("  FINAL STATUS\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    if (_passed)
    {
        printf("\n");
        printf("  %s %s Test Suite COMPLETED SUCCESSFULLY\n",
               D_TEST_SYMBOL_SUCCESS,
               _framework_name);
        printf("\n");
        printf("  %s All modules validated and ready for use\n",
               D_TEST_SYMBOL_PASS);
        printf("  %s Proceed with confidence in framework stability\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("\n");
        printf("  %s %s Test Suite COMPLETED WITH FAILURES\n",
               D_TEST_SYMBOL_FAIL,
               _framework_name);
        printf("\n");
        printf("  %s Review and fix all failures before proceeding\n",
               D_TEST_SYMBOL_FAIL);
        printf("  %s Framework stability may be compromised\n",
               D_TEST_SYMBOL_FAIL);
    }

    printf("\n");
    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    return;
}


/******************************************************************************
 * UNIFIED TEST RUNNER FUNCTIONS
 *****************************************************************************/

/*
d_test_sa_runner_init
  Initializes a test runner with the given suite name and description.

Parameter(s):
  _runner:            the runner to initialize
  _suite_name:        name of the test suite
  _suite_description: description of the test suite
Return:
  none.
*/
void
d_test_sa_runner_init
(
    struct d_test_sa_runner* _runner,
    const char*              _suite_name,
    const char*              _suite_description
)
{
    if (!_runner)
    {
        return;
    }

    _runner->suite_name        = _suite_name;
    _runner->suite_description = _suite_description;
    _runner->module_count      = 0;
    _runner->wait_for_input    = true;
    _runner->show_notes        = true;

    // initialize results
    _runner->results.modules_total  = 0;
    _runner->results.modules_passed = 0;
    _runner->results.total_time     = 0.0;
    _runner->results.modules        = NULL;
    d_test_counter_reset(&_runner->results.totals);

    return;
}

/*
d_test_sa_runner_add_module
  Registers a test module with the runner (tree-based test function).

Parameter(s):
  _runner:      the runner to add the module to
  _name:        module name
  _description: module description
  _run_fn:      function that runs tests and returns a d_test_object tree
  _note_count:  number of implementation note sections
  _notes:       array of note sections
Return:
  none.
*/
void
d_test_sa_runner_add_module
(
    struct d_test_sa_runner*             _runner,
    const char*                          _name,
    const char*                          _description,
    fn_test_module                       _run_fn,
    size_t                               _note_count,
    const struct d_test_sa_note_section* _notes
)
{
    size_t idx;

    if ( (!_runner) ||
         (!_run_fn) )
    {
        return;
    }

    if (_runner->module_count >= D_TEST_SA_MAX_MODULES)
    {
        printf("ERROR: Maximum module count (%d) exceeded\n",
               D_TEST_SA_MAX_MODULES);

        return;
    }

    idx = _runner->module_count;

    _runner->modules[idx].name        = _name;
    _runner->modules[idx].description = _description;
    _runner->modules[idx].run_fn      = _run_fn;
    _runner->modules[idx].run_counter = NULL;
    _runner->modules[idx].note_count  = _note_count;
    _runner->modules[idx].notes       = _notes;

    _runner->module_count++;

    return;
}

/*
d_test_sa_runner_add_module_counter
  Registers a test module with the runner (counter-based test function).

Parameter(s):
  _runner:      the runner to add the module to
  _name:        module name
  _description: module description
  _run_fn:      function that runs tests and updates a counter directly
  _note_count:  number of implementation note sections
  _notes:       array of note sections
Return:
  none.
*/
void
d_test_sa_runner_add_module_counter
(
    struct d_test_sa_runner*             _runner,
    const char*                          _name,
    const char*                          _description,
    fn_test_module_counter               _run_fn,
    size_t                               _note_count,
    const struct d_test_sa_note_section* _notes
)
{
    size_t idx;

    if ( (!_runner) ||
         (!_run_fn) )
    {
        return;
    }

    if (_runner->module_count >= D_TEST_SA_MAX_MODULES)
    {
        printf("ERROR: Maximum module count (%d) exceeded\n",
               D_TEST_SA_MAX_MODULES);

        return;
    }

    idx = _runner->module_count;

    _runner->modules[idx].name        = _name;
    _runner->modules[idx].description = _description;
    _runner->modules[idx].run_fn      = NULL;
    _runner->modules[idx].run_counter = _run_fn;
    _runner->modules[idx].note_count  = _note_count;
    _runner->modules[idx].notes       = _notes;

    _runner->module_count++;

    return;
}

/*
d_test_sa_runner_set_wait_for_input
  Sets whether the runner should wait for user input before exiting.

Parameter(s):
  _runner: the runner to configure
  _wait:   true to wait for input, false to exit immediately
Return:
  none.
*/
void
d_test_sa_runner_set_wait_for_input
(
    struct d_test_sa_runner* _runner,
    bool                     _wait
)
{
    if (!_runner)
    {
        return;
    }

    _runner->wait_for_input = _wait;

    return;
}

/*
d_test_sa_runner_set_show_notes
  Sets whether the runner should display implementation notes.

Parameter(s):
  _runner: the runner to configure
  _show:   true to show notes, false to hide them
Return:
  none.
*/
void
d_test_sa_runner_set_show_notes
(
    struct d_test_sa_runner* _runner,
    bool                     _show
)
{
    if (!_runner)
    {
        return;
    }

    _runner->show_notes = _show;

    return;
}

/*
d_test_sa_runner_execute
  Executes all registered test modules and generates comprehensive output.

Parameter(s):
  _runner: the runner to execute
Return:
  0 if all tests passed, 1 if any tests failed.
*/
int
d_test_sa_runner_execute
(
    struct d_test_sa_runner* _runner
)
{
    size_t                 i;
    clock_t                suite_start;
    clock_t                suite_end;
    clock_t                module_start;
    clock_t                module_end;
    bool                   overall_result;
    struct d_test_counter  overall_counter;
    size_t                 modules_passed;

    if (!_runner)
    {
        return 1;
    }

    // initialize tracking
    overall_result = true;
    modules_passed = 0;
    d_test_counter_reset(&overall_counter);

    // record suite start time
    suite_start = clock();

    // print framework header
    d_test_sa_create_framework_header(_runner->suite_name,
                                      _runner->suite_description);

    // execute each registered module
    for (i = 0; i < _runner->module_count; i++)
    {
        struct d_test_counter  module_counter;
        bool                   module_result;

        d_test_counter_reset(&module_counter);
        module_result = false;

        // print module header
        d_test_sa_create_module_test_header(_runner->modules[i].name,
                                            _runner->modules[i].description);

        // record module start time
        module_start = clock();

        // execute module tests
        if (_runner->modules[i].run_fn)
        {
            // tree-based test function
            struct d_test_object* test_results;

            test_results = _runner->modules[i].run_fn();

            if (test_results)
            {
                d_test_default_print_object(test_results, 0, &module_counter);

                module_result =
                    (module_counter.assertions_passed ==
                     module_counter.assertions_total) &&
                    (module_counter.tests_passed ==
                     module_counter.tests_total);

                d_test_object_free(test_results);
            }
        }
        else if (_runner->modules[i].run_counter)
        {
            // counter-based test function
            module_result = _runner->modules[i].run_counter(&module_counter);
        }

        // record module end time
        module_end = clock();

        // print module results
        d_test_sa_create_module_test_results(_runner->modules[i].name,
                                             &module_counter);

        // print module notes if enabled
        if ( _runner->show_notes              &&
             _runner->modules[i].notes        &&
             _runner->modules[i].note_count > 0 )
        {
            d_test_sa_create_implementation_notes(
                _runner->modules[i].note_count,
                _runner->modules[i].notes);
        }

        // update overall counters
        d_test_counter_add(&overall_counter, &module_counter);

        if (module_result)
        {
            modules_passed++;
        }

        overall_result = overall_result && module_result;
    }

    // record suite end time
    suite_end = clock();

    // build suite results
    _runner->results.modules_total  = _runner->module_count;
    _runner->results.modules_passed = modules_passed;
    _runner->results.totals         = overall_counter;
    _runner->results.total_time     = d_test_sa_get_elapsed_time(suite_start,
                                                                  suite_end);

    // print comprehensive results
    d_test_sa_create_comprehensive_results(&_runner->results);

    // print final status
    d_test_sa_create_final_status(_runner->suite_name, overall_result);

    // wait for user input if configured
    if (_runner->wait_for_input)
    {
        printf("\nPress Enter to exit...");
        (void)getchar();
    }

    return overall_result ? 0 : 1;
}

/*
d_test_sa_runner_cleanup
  Cleans up any resources allocated by the runner.

Parameter(s):
  _runner: the runner to clean up
Return:
  none.
*/
void
d_test_sa_runner_cleanup
(
    struct d_test_sa_runner* _runner
)
{
    if (!_runner)
    {
        return;
    }

    // free module results array if allocated
    if (_runner->results.modules)
    {
        free(_runner->results.modules);
        _runner->results.modules = NULL;
    }

    return;
}

