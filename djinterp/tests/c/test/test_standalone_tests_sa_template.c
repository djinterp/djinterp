#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * X. TEMPLATE SUBSTITUTION TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_substitute_template
  Tests the d_test_substitute_template function.
  Tests the following:
  - NULL template returns NULL
  - NULL delimiters returns NULL
  - Simple substitution works
  - Multiple substitutions work
  - Unknown keys are left unchanged
  - Empty template works
*/
bool
d_tests_sa_standalone_substitute_template
(
    struct d_test_counter* _counter
)
{
    bool        result;
    char*       output;
    const char* delimiters[2];

    result = true;

    // set up common delimiters
    delimiters[0] = "{{";
    delimiters[1] = "}}";

    // test 1: NULL template returns NULL
    output = d_test_substitute_template(NULL, delimiters, 0, NULL);

    result = d_assert_standalone(
        output == NULL,
        "template_null_template",
        "NULL template should return NULL",
        _counter) && result;

    // test 2: NULL delimiters returns NULL
    output = d_test_substitute_template("test", NULL, 0, NULL);

    result = d_assert_standalone(
        output == NULL,
        "template_null_delimiters",
        "NULL delimiters should return NULL",
        _counter) && result;

    // test 3: NULL opening delimiter returns NULL
    {
        const char* bad_delims[2];

        bad_delims[0] = NULL;
        bad_delims[1] = "}}";

        output = d_test_substitute_template("test", bad_delims, 0, NULL);

        result = d_assert_standalone(
            output == NULL,
            "template_null_open_delimiter",
            "NULL opening delimiter should return NULL",
            _counter) && result;
    }

    // test 4: NULL closing delimiter returns NULL
    {
        const char* bad_delims[2];

        bad_delims[0] = "{{";
        bad_delims[1] = NULL;

        output = d_test_substitute_template("test", bad_delims, 0, NULL);

        result = d_assert_standalone(
            output == NULL,
            "template_null_close_delimiter",
            "NULL closing delimiter should return NULL",
            _counter) && result;
    }

    // test 5: template with no placeholders returns copy
    output = d_test_substitute_template("Hello World", delimiters, 0, NULL);

    result = d_assert_standalone(
        output != NULL,
        "template_no_placeholders_not_null",
        "Template without placeholders should return non-NULL",
        _counter) && result;

    if (output)
    {
        result = d_assert_standalone(
            d_strcasecmp(output, "Hello World") == 0,
            "template_no_placeholders_copy",
            "Template without placeholders should return copy",
            _counter) && result;

        free(output);
    }

    // test 6: simple substitution works
    {
        const char* kv_simple[][2] =
        {
            { "name", "Claude" }
        };

        output = d_test_substitute_template("Hello {{name}}!", delimiters, 1,
                                            kv_simple);

        result = d_assert_standalone(
            output != NULL,
            "template_simple_not_null",
            "Simple substitution should return non-NULL",
            _counter) && result;

        if (output)
        {
            result = d_assert_standalone(
                d_strcasecmp(output, "Hello Claude!") == 0,
                "template_simple_substitution",
                "Simple substitution should work",
                _counter) && result;

            free(output);
        }
    }

    // test 7: multiple substitutions work
    {
        const char* kv_multiple[][2] =
        {
            { "greeting", "Hello" },
            { "name",     "World" }
        };

        output = d_test_substitute_template("{{greeting}}, {{name}}!",
                                            delimiters, 2, kv_multiple);

        result = d_assert_standalone(
            output != NULL,
            "template_multiple_not_null",
            "Multiple substitutions should return non-NULL",
            _counter) && result;

        if (output)
        {
            result = d_assert_standalone(
                d_strcasecmp(output, "Hello, World!") == 0,
                "template_multiple_substitution",
                "Multiple substitutions should work",
                _counter) && result;

            free(output);
        }
    }

    // test 8: unknown keys are left unchanged
    {
        const char* kv_known[][2] =
        {
            { "known", "value" }
        };

        output = d_test_substitute_template("{{known}} {{unknown}}",
                                            delimiters, 1, kv_known);

        result = d_assert_standalone(
            output != NULL,
            "template_unknown_not_null",
            "Unknown keys should return non-NULL",
            _counter) && result;

        if (output)
        {
            result = d_assert_standalone(
                d_strcasecmp(output, "value {{unknown}}") == 0,
                "template_unknown_preserved",
                "Unknown keys should be preserved",
                _counter) && result;

            free(output);
        }
    }

    // test 9: empty template works
    output = d_test_substitute_template("", delimiters, 0, NULL);

    result = d_assert_standalone(
        output != NULL,
        "template_empty_not_null",
        "Empty template should return non-NULL",
        _counter) && result;

    if (output)
    {
        result = d_assert_standalone((d_strnlen(output, 10) == 0),
                                     "template_empty_result",
                                     "Empty template should return empty string",
                                     _counter) && 
                                     result;

        free(output);
    }

    // test 10: unclosed delimiter is handled
    {
        const char* kv_unclosed[][2] =
        {
            { "name", "value" }
        };

        output = d_test_substitute_template("Hello {{name", delimiters, 1,
                                            kv_unclosed);

        result = d_assert_standalone(
            output != NULL,
            "template_unclosed_not_null",
            "Unclosed delimiter should return non-NULL",
            _counter) && result;

        if (output)
        {
            free(output);
        }
    }

    // test 11: adjacent placeholders work
    {
        const char* kv_adjacent[][2] =
        {
            { "a", "X" },
            { "b", "Y" }
        };

        output = d_test_substitute_template("{{a}}{{b}}", delimiters, 2,
                                            kv_adjacent);

        result = d_assert_standalone(
            output != NULL,
            "template_adjacent_not_null",
            "Adjacent placeholders should return non-NULL",
            _counter) && result;

        if (output)
        {
            result = d_assert_standalone(
                d_strcasecmp(output, "XY") == 0,
                "template_adjacent_substitution",
                "Adjacent placeholders should work",
                _counter) && result;

            free(output);
        }
    }

    return result;
}

/*
d_tests_sa_standalone_template_all
  Aggregation function that runs all template substitution tests.
*/
bool
d_tests_sa_standalone_template_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Template Substitution\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_standalone_substitute_template(_counter) && result;

    return result;
}
