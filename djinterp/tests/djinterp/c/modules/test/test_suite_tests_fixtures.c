/*******************************************************************************
* djinterp [test]                                    test_suite_tests_fixtures.c
*
* The shared fixtures of test_suite.h's suite: quiet inner suites and their
* reports read back.
*
*
* path:      /tests/djinterp/c/modules/test/test_suite_tests_fixtures.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_suite_tests.h"  // corresponding header

// std
#include <stdlib.h>  // malloc, free
#include <string.h>  // strstr


/*
d_tests_test_suite_quiet
  The temporary file is the suite's stream; the caller keeps the handle and
passes it to d_tests_test_suite_report, which reads and closes it.
*/
struct dj_test_suite*
d_tests_test_suite_quiet(
    int    _show,
    FILE** _stream
)
{
    *_stream = NULL;

    struct dj_test_suite* const suite = d_test_suite_new();

    // check if memory allocation was successful
    if (!suite)
    {
        return NULL;
    }

    struct dj_test_suite_options options = {0};

    d_test_suite_options_init(&options);
    options.show   = _show;
    options.stream = tmpfile();

    // no temporary file: the suite goes too
    if (!options.stream)
    {
        d_test_suite_free(suite);

        return NULL;
    }

    d_test_suite_set_options(suite,
                             &options);
    *_stream = options.stream;

    return suite;
}

/*
d_tests_test_suite_report
  Runs the suite, reads its stream back from the start, closes the stream and
frees the suite: a caller holds only the text afterwards.
*/
char*
d_tests_test_suite_report(
    struct dj_test_suite*   _suite,
    FILE*                   _stream,
    int*                    _code,
    struct dj_test_summary* _summary
)
{
    // parameter validation; the stream is closed and the suite freed anyway
    if ( (!_suite) ||
         (!_stream) )
    {
        // close the stream, when there is one
        if (_stream)
        {
            fclose(_stream);
        }

        d_test_suite_free(_suite);

        return NULL;
    }

    struct dj_test_summary summary = {0};

    const int code = d_test_suite_run(_suite,
                                      &summary);

    d_test_suite_free(_suite);

    char* text   = NULL;
    long  length = ftell(_stream);

    // the stream's length, and the stream rewound to read it
    if ( (length >= 0) &&
         (fseek(_stream,
                0L,
                SEEK_SET) == 0) )
    {
        text = (char*)malloc((size_t)length + 1);

        // read it, when the buffer was allocated
        if (text)
        {
            length       = (long)fread(text,
                                       1,
                                       (size_t)length,
                                       _stream);
            text[length] = '\0';
        }
    }

    fclose(_stream);

    // the exit code, when it is wanted
    if (_code)
    {
        *_code = code;
    }

    // the summary, when it is wanted
    if (_summary)
    {
        *_summary = summary;
    }

    return text;
}

/*
d_tests_test_suite_contains
  NULL-safe strstr.
*/
bool
d_tests_test_suite_contains(
    const char* _report,
    const char* _text
)
{
    // parameter validation
    if ( (!_report) ||
         (!_text) )
    {
        return false;
    }

    return (strstr(_report,
                   _text) != NULL);
}
