/*******************************************************************************
* djinterp [parse]                                            arg_common_tests.c
*
* The suite of the argument parser's shared vocabulary, declared in
* arg_common_tests.h.
*   The statuses are checked against CLI11 2.7.2's error classes: the names
* are their names, the exit codes their ExitCodes, and the messages a status
* carries when none was recorded are CLI11's default messages where CLI11
* has one. The enumerations are checked value for value against CLI11's, so
* that a setting documented there means the same here.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_common_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_common_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // FILE, tmpfile, fclose, fread, rewind, printf
#include <stdlib.h>   // free
#include <string.h>   // memcmp, memset, strcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // the vocabulary
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // d_tests_arg_same


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// CAUGHT
//   constant: CLI11's message for the requests a main function catches.
static const char CAUGHT[] =
    "This should be caught in your main function, see examples";

// SUCCEEDED
//   constant: CLI11's message for Success, the request to end successfully.
static const char SUCCEEDED[] =
    "Successfully completed, should be caught and quit";

// d_tests_arg_common_status_row
//   struct: what one status is called, the code it exits with, and the
// message it carries when none was recorded.
struct d_tests_arg_common_status_row
{
    int         status;
    const char* name;
    int         exit_code;
    const char* message;
};

// STATUS_ROWS
//   constant: every status, as CLI11 names it, ends a process with it and
// describes it; the three CLI11 lacks exit with its BaseClass code, 127.
static const struct d_tests_arg_common_status_row STATUS_ROWS[] =
{
    { D_ARG_OK,                           "OK",
      0,                                  "" },
    { D_ARG_CALL_FOR_HELP,                "CallForHelp",
      0,                                  CAUGHT },
    { D_ARG_CALL_FOR_ALL_HELP,            "CallForAllHelp",
      0,                                  CAUGHT },
    { D_ARG_CALL_FOR_VERSION,             "CallForVersion",
      0,                                  CAUGHT },
    { D_ARG_CALL_FOR_COMPLETION,          "CallForCompletion",
      0,                                  CAUGHT },
    { D_ARG_SUCCESS,                      "Success",
      0,                                  SUCCEEDED },
    { D_ARG_RUNTIME_ERROR,                "RuntimeError",
      1,                                  "Runtime error" },
    { D_ARG_ERROR_INCORRECT_CONSTRUCTION, "IncorrectConstruction",
      100,                                "Incorrect construction" },
    { D_ARG_ERROR_BAD_NAME,               "BadNameString",
      101,                                "Bad name string" },
    { D_ARG_ERROR_OPTION_ALREADY_ADDED,   "OptionAlreadyAdded",
      102,                                "Option already added" },
    { D_ARG_ERROR_FILE,                   "FileError",
      103,                                "File error" },
    { D_ARG_ERROR_CONVERSION,             "ConversionError",
      104,                                "Conversion error" },
    { D_ARG_ERROR_VALIDATION,             "ValidationError",
      105,                                "Validation error" },
    { D_ARG_ERROR_REQUIRED,               "RequiredError",
      106,                                "Required error" },
    { D_ARG_ERROR_REQUIRES,               "RequiresError",
      107,                                "Requires error" },
    { D_ARG_ERROR_EXCLUDES,               "ExcludesError",
      108,                                "Excludes error" },
    { D_ARG_ERROR_EXTRAS,                 "ExtrasError",
      109,                                "Extras error" },
    { D_ARG_ERROR_CONFIG,                 "ConfigError",
      110,                                "Config error" },
    { D_ARG_ERROR_INVALID,                "InvalidError",
      111,                                "Invalid error" },
    { D_ARG_ERROR_HORRIBLE,               "HorribleError",
      112,                                "Horrible error" },
    { D_ARG_ERROR_OPTION_NOT_FOUND,       "OptionNotFound",
      113,                                "Option not found" },
    { D_ARG_ERROR_ARGUMENT_MISMATCH,      "ArgumentMismatch",
      114,                                "Argument mismatch" },
    { D_ARG_ERROR_PARSE,                  "ParseError",
      111,                                "Parse error" },
    { D_ARG_ERROR_OUT_OF_MEMORY,          "OutOfMemory",
      127,                                "Out of memory" },
    { D_ARG_ERROR_BAD_ARGUMENT,           "BadArgument",
      127,                                "Bad argument" },
    { D_ARG_ERROR_UNSUPPORTED,            "Unsupported",
      127,                                "Unsupported" }
};

// STATUS_ROW_COUNT
//   constant: how many rows STATUS_ROWS holds, one for each status.
static const size_t STATUS_ROW_COUNT =
    sizeof(STATUS_ROWS) / sizeof(STATUS_ROWS[0]);

// LONG_TEXT
//   constant: text longer than a buffer's first allocation, so that an
// append must grow the storage more than once.
static const char LONG_TEXT[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/**
 * @brief Writes nothing.
 *
 * @param[in] _context  the sink's context; unused.
 * @param[in] _data     the bytes offered; unused.
 * @param[in] _size     how many; unused.
 * @return 0, for no byte taken.
 *
 * A sink's write function that takes nothing, as a full disk would.
 */
static size_t
d_tests_arg_common_refuse(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    (void)_data;
    (void)_size;

    return 0u;
}

/**
 * @brief Tells whether a buffer holds exactly the expected text.
 *
 * @param[in] _buffer    the buffer.
 * @param[in] _expected  the text it must hold.
 * @return `true` when its data holds `_expected` and nothing more, with a NUL
 *         after it; `false` otherwise.
 *
 * Whether a buffer holds exactly `_expected`, with a NUL after it.
 */
static bool
d_tests_arg_common_terminated(
    const struct d_arg_buffer* _buffer,
    const char*                _expected
)
{
    const size_t length = strlen(_expected);

    return ( (_buffer->data != NULL)                         &&
             (_buffer->length == length)                     &&
             (memcmp(_buffer->data,
                     _expected,
                     length) == 0)                           &&
             (_buffer->data[length] == '\0') );
}

//==============================================================================
// 1.  STATUS
//==============================================================================

/*
d_tests_arg_common_status_names
  What d_arg_status_name calls each status.
  Tests the following:
  - each status is named as CLI11 names its error class
  - a value outside the enumeration is "Error", CLI11's base class
*/
bool
d_tests_arg_common_status_names(void)
{
    bool named = (STATUS_ROW_COUNT == (size_t)D_ARG_STATUS_COUNT);

    // each status by its name
    for (size_t index = 0u; index < STATUS_ROW_COUNT; ++index)
    {
        named = ( (named) &&
                  (strcmp(d_arg_status_name(STATUS_ROWS[index].status),
                          STATUS_ROWS[index].name) == 0) );
    }

    return ( (named)                                                  &&
             (strcmp(d_arg_status_name(-1),
                     "Error") == 0)                                   &&
             (strcmp(d_arg_status_name((int)D_ARG_STATUS_COUNT),
                     "Error") == 0) );
}

/*
d_tests_arg_common_status_messages
  What d_arg_status_message says of each status.
  Tests the following:
  - each status's message is the one it carries when none was recorded:
    CLI11's default messages for the requests, Success and RuntimeError,
    and a message in the same words for every other status
  - a value outside the enumeration is "Error", CLI11's base class
*/
bool
d_tests_arg_common_status_messages(void)
{
    bool described = true;

    // each status by its message
    for (size_t index = 0u; index < STATUS_ROW_COUNT; ++index)
    {
        described = ( (described) &&
                      (strcmp(d_arg_status_message(STATUS_ROWS[index].status),
                              STATUS_ROWS[index].message) == 0) );
    }

    return ( (described)                                          &&
             (strcmp(d_arg_status_message(-1),
                     "Error") == 0)                               &&
             (strcmp(d_arg_status_message((int)D_ARG_STATUS_COUNT),
                     "Error") == 0) );
}

/*
d_tests_arg_common_status_exit_codes
  The code d_arg_status_exit_code ends a process with for each status.
  Tests the following:
  - each of CLI11's errors exits with its ExitCodes value, 100 to 114
  - success and the completed requests exit with 0, a runtime error with
    CLI11's default of 1
  - the statuses CLI11 lacks, and a value outside the enumeration, exit with
    127
*/
bool
d_tests_arg_common_status_exit_codes(void)
{
    bool coded = true;

    // each status by its code
    for (size_t index = 0u; index < STATUS_ROW_COUNT; ++index)
    {
        coded = ( (coded) &&
                  (d_arg_status_exit_code(STATUS_ROWS[index].status) ==
                   STATUS_ROWS[index].exit_code) );
    }

    return ( (coded)                                    &&
             (d_arg_status_exit_code(-5) == 127)        &&
             (d_arg_status_exit_code(1000) == 127)      &&
             ((int)D_ARG_EXIT_BASE_CLASS == 127) );
}

/*
d_tests_arg_common_status_is_error
  Where d_arg_status_is_error draws the line.
  Tests the following:
  - D_ARG_OK, the completed requests and D_ARG_SUCCESS are not errors
  - D_ARG_RUNTIME_ERROR and every D_ARG_ERROR_* status are
  - a value outside the enumeration, below it or past it, is, as it is named
    "Error" and exits with D_ARG_EXIT_BASE_CLASS
*/
bool
d_tests_arg_common_status_is_error(void)
{
    bool drawn = true;

    // each status on its side of the line
    for (int status = 0; status < (int)D_ARG_STATUS_COUNT; ++status)
    {
        drawn = ( (drawn) &&
                  (d_arg_status_is_error(status) ==
                   (status >= (int)D_ARG_RUNTIME_ERROR)) );
    }

    return ( (drawn)                                                          &&
             (!d_arg_status_is_error((int)D_ARG_SUCCESS))                     &&
             (d_arg_status_is_error((int)D_ARG_RUNTIME_ERROR))                &&
             (d_arg_status_is_error((int)D_ARG_ERROR_INCORRECT_CONSTRUCTION)) &&
             (d_arg_status_is_error(-1))                                      &&
             (d_arg_status_is_error((int)D_ARG_STATUS_COUNT)) );
}

//==============================================================================
// 2.  VOCABULARY
//==============================================================================

/*
d_tests_arg_common_vocabulary
  The enumerations CLI11 has, value for value.
  Tests the following:
  - MultiOptionPolicy, CallbackPriority, ExtrasMode, ConfigExtrasMode,
    PrefixCommandMode, AppFormatMode and the argument classifier, each in
    CLI11's order from 0
  - CLI11's expected_max_vector_size, 2^29, as the unlimited count
*/
bool
d_tests_arg_common_vocabulary(void)
{
    const bool policies   = ( (D_ARG_MULTI_THROW == 0)                &&
                              (D_ARG_MULTI_TAKE_LAST == 1)            &&
                              (D_ARG_MULTI_TAKE_FIRST == 2)           &&
                              (D_ARG_MULTI_JOIN == 3)                 &&
                              (D_ARG_MULTI_TAKE_ALL == 4)             &&
                              (D_ARG_MULTI_SUM == 5)                  &&
                              (D_ARG_MULTI_REVERSE == 6) );
    const bool priorities = ( (D_ARG_PRIORITY_FIRST_PRE_HELP == 0)    &&
                              (D_ARG_PRIORITY_FIRST == 1)             &&
                              (D_ARG_PRIORITY_PRE_REQUIREMENTS == 3)  &&
                              (D_ARG_PRIORITY_NORMAL == 5)            &&
                              (D_ARG_PRIORITY_LAST == 7) );
    const bool extras     = ( (D_ARG_EXTRAS_ERROR == 0)               &&
                              (D_ARG_EXTRAS_ERROR_IMMEDIATELY == 1)   &&
                              (D_ARG_EXTRAS_IGNORE == 2)              &&
                              (D_ARG_EXTRAS_ASSUME_SINGLE == 3)       &&
                              (D_ARG_EXTRAS_ASSUME_MULTIPLE == 4)     &&
                              (D_ARG_EXTRAS_CAPTURE == 5)             &&
                              (D_ARG_CONFIG_EXTRAS_ERROR == 0)        &&
                              (D_ARG_CONFIG_EXTRAS_IGNORE == 1)       &&
                              (D_ARG_CONFIG_EXTRAS_IGNORE_ALL == 2)   &&
                              (D_ARG_CONFIG_EXTRAS_CAPTURE == 3) );
    const bool modes      = ( (D_ARG_PREFIX_OFF == 0)                 &&
                              (D_ARG_PREFIX_SEPARATOR_ONLY == 1)      &&
                              (D_ARG_PREFIX_ON == 2)                  &&
                              (D_ARG_PREFIX_POSITIONAL_ONLY == 3)     &&
                              (D_ARG_HELP_NORMAL == 0)                &&
                              (D_ARG_HELP_ALL == 1)                   &&
                              (D_ARG_HELP_SUB == 2) );
    const bool classes    = ( (D_ARG_CLASS_NONE == 0)                 &&
                              (D_ARG_CLASS_POSITIONAL_MARK == 1)      &&
                              (D_ARG_CLASS_SHORT == 2)                &&
                              (D_ARG_CLASS_LONG == 3)                 &&
                              (D_ARG_CLASS_WINDOWS_STYLE == 4)        &&
                              (D_ARG_CLASS_SUBCOMMAND == 5)           &&
                              (D_ARG_CLASS_SUBCOMMAND_TERMINATOR == 6) );

    return ( (policies)                                &&
             (priorities)                              &&
             (extras)                                  &&
             (modes)                                   &&
             (classes)                                 &&
             (D_ARG_EXPECTED_UNLIMITED == 536870912) );
}

//==============================================================================
// 4.1  BUFFERS
//==============================================================================

/*
d_tests_arg_common_buffer_empty
  A buffer before anything is stored in it.
  Tests the following:
  - d_arg_buffer_init gives an empty string, never NULL, and no storage
  - D_ARG_BUFFER_INIT's buffer and the zeroed one read as empty text
  - detaching from either gives NULL, and releasing either is harmless
*/
bool
d_tests_arg_common_buffer_empty(void)
{
    struct d_arg_buffer initialized = D_ARG_BUFFER_INIT;
    struct d_arg_buffer constant    = D_ARG_BUFFER_INIT;

    d_arg_buffer_init(&initialized);

    const bool               empty   = ( (initialized.data != NULL)     &&
                                         (initialized.data[0] == '\0')  &&
                                         (initialized.length == 0u)     &&
                                         (initialized.capacity == 0u)   &&
                                         (initialized.failed == 0) );
    const struct d_pack_text text    = d_arg_buffer_text(&constant);
    char* const              taken   = d_arg_buffer_detach(&initialized);
    char* const              nothing = d_arg_buffer_detach(&constant);

    d_arg_buffer_release(&initialized);
    d_arg_buffer_release(&constant);
    d_arg_buffer_release(NULL);
    d_arg_buffer_init(NULL);

    return ( (empty)             &&
             (text.length == 0u) &&
             (taken == NULL)     &&
             (nothing == NULL) );
}

/*
d_tests_arg_common_buffer_append
  Appends of each kind, past the first allocation.
  Tests the following:
  - bytes, a string, a span and a character, in order, NUL-terminated
  - text longer than the first allocation, appended in pieces
  - nothing to append -- a NULL string, zero bytes -- appends nothing
*/
bool
d_tests_arg_common_buffer_append(void)
{
    struct d_arg_buffer      buffer = D_ARG_BUFFER_INIT;
    const struct d_pack_text ef     = d_pack_text_from_cstr("ef");
    bool                     added  =
        ( (d_arg_buffer_append(&buffer,
                               "ab_ignored",
                               2u) == D_ARG_OK)                   &&
          (d_arg_buffer_append_cstr(&buffer,
                                    "cd") == D_ARG_OK)            &&
          (d_arg_buffer_append_text(&buffer,
                                    ef) == D_ARG_OK)              &&
          (d_arg_buffer_append_char(&buffer,
                                    'g') == D_ARG_OK)             &&
          (d_arg_buffer_append_cstr(&buffer,
                                    NULL) == D_ARG_OK)            &&
          (d_arg_buffer_append(&buffer,
                               NULL,
                               0u) == D_ARG_OK) );

    const bool short_text = d_tests_arg_common_terminated(&buffer,
                                                          "abcdefg");

    d_arg_buffer_clear(&buffer);

    // the long text a few bytes at a time
    for (size_t at = 0u; at < sizeof(LONG_TEXT) - 1u; at += 7u)
    {
        const size_t rest = (sizeof(LONG_TEXT) - 1u) - at;

        added = ( (added) &&
                  (d_arg_buffer_append(&buffer,
                                       LONG_TEXT + at,
                                       (rest < 7u) ? rest : 7u)
                   == D_ARG_OK) );
    }

    const bool long_text = d_tests_arg_common_terminated(&buffer,
                                                         LONG_TEXT);

    d_arg_buffer_release(&buffer);

    return ( (added)      &&
             (short_text) &&
             (long_text) );
}

/*
d_tests_arg_common_buffer_append_refused
  Appends that cannot be made.
  Tests the following:
  - a NULL buffer, and NULL bytes with a length, are refused as bad
    arguments, and the buffer is unchanged
  - d_arg_buffer_printf with a NULL format is refused the same way
*/
bool
d_tests_arg_common_buffer_append_refused(void)
{
    struct d_arg_buffer buffer = D_ARG_BUFFER_INIT;

    const int  stored  = d_arg_buffer_append_cstr(&buffer,
                                                  "kept");
    const bool refused =
        ( (d_arg_buffer_append(NULL,
                               "x",
                               1u) == D_ARG_ERROR_BAD_ARGUMENT)        &&
          (d_arg_buffer_append(&buffer,
                               NULL,
                               3u) == D_ARG_ERROR_BAD_ARGUMENT)        &&
          (d_arg_buffer_append_char(NULL,
                                    'x') == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_buffer_printf(&buffer,
                               NULL) == D_ARG_ERROR_BAD_ARGUMENT) );
    const bool kept    = d_tests_arg_common_terminated(&buffer,
                                                       "kept");

    d_arg_buffer_release(&buffer);

    return ( (stored == D_ARG_OK) &&
             (refused)            &&
             (kept) );
}

/*
d_tests_arg_common_buffer_printf
  Formatted appends.
  Tests the following:
  - text formatted after what the buffer held
  - an expansion longer than the first allocation
  - an empty expansion appends nothing
*/
bool
d_tests_arg_common_buffer_printf(void)
{
    struct d_arg_buffer buffer = D_ARG_BUFFER_INIT;

    const bool formatted = ( (d_arg_buffer_append_cstr(&buffer,
                                                       "n=") == D_ARG_OK) &&
                             (d_arg_buffer_printf(&buffer,
                                                  "%d, %s",
                                                  42,
                                                  "two") == D_ARG_OK)    &&
                             (d_arg_buffer_printf(&buffer,
                                                  "%s",
                                                  "") == D_ARG_OK) );
    const bool short_text = d_tests_arg_common_terminated(&buffer,
                                                          "n=42, two");

    d_arg_buffer_clear(&buffer);

    const bool long_text = ( (d_arg_buffer_printf(&buffer,
                                                  "%s",
                                                  LONG_TEXT) == D_ARG_OK) &&
                             (d_tests_arg_common_terminated(&buffer,
                                                            LONG_TEXT)) );

    d_arg_buffer_release(&buffer);

    return ( (formatted)  &&
             (short_text) &&
             (long_text) );
}

/*
d_tests_arg_common_buffer_truncate_clear
  Shortening a buffer, and emptying it.
  Tests the following:
  - truncation keeps the first bytes and the terminator, and a longer length
    changes nothing
  - clearing empties the buffer and keeps its storage for the next appends
*/
bool
d_tests_arg_common_buffer_truncate_clear(void)
{
    struct d_arg_buffer buffer = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&buffer,
                                   "abcdef");
    d_arg_buffer_truncate(&buffer,
                          3u);

    const bool cut = d_tests_arg_common_terminated(&buffer,
                                                   "abc");

    d_arg_buffer_truncate(&buffer,
                          10u);

    const bool   longer   = d_tests_arg_common_terminated(&buffer,
                                                          "abc");
    const size_t capacity = buffer.capacity;

    d_arg_buffer_clear(&buffer);

    const bool cleared = ( (d_tests_arg_common_terminated(&buffer,
                                                          "")) &&
                           (buffer.capacity == capacity)       &&
                           (capacity > 0u) );

    (void)d_arg_buffer_append_cstr(&buffer,
                                   "xyz");

    const bool reused = d_tests_arg_common_terminated(&buffer,
                                                      "xyz");

    d_arg_buffer_release(&buffer);
    d_arg_buffer_truncate(NULL,
                          0u);
    d_arg_buffer_clear(NULL);

    return ( (cut)     &&
             (longer)  &&
             (cleared) &&
             (reused) );
}

/*
d_tests_arg_common_buffer_detach
  Taking a buffer's storage.
  Tests the following:
  - the caller receives the NUL-terminated content, to free itself
  - the buffer is left empty, owning nothing, and can be used again
*/
bool
d_tests_arg_common_buffer_detach(void)
{
    struct d_arg_buffer buffer = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&buffer,
                                   "taken");

    char* const taken   = d_arg_buffer_detach(&buffer);
    const bool  content = ( (taken != NULL)       &&
                            (strcmp(taken,
                                    "taken") == 0) );
    const bool  emptied = ( (buffer.length == 0u) &&
                            (buffer.capacity == 0u) );

    free(taken);
    (void)d_arg_buffer_append_cstr(&buffer,
                                   "again");

    const bool reused = d_tests_arg_common_terminated(&buffer,
                                                      "again");

    d_arg_buffer_release(&buffer);

    return ( (content) &&
             (emptied) &&
             (reused)  &&
             (d_arg_buffer_detach(NULL) == NULL) );
}

/*
d_tests_arg_common_buffer_sink
  A buffer written through a sink.
  Tests the following:
  - bytes, strings and formatted text written to the sink are appended
  - a NULL string writes nothing, successfully
*/
bool
d_tests_arg_common_buffer_sink(void)
{
    struct d_arg_buffer      buffer = D_ARG_BUFFER_INIT;
    const struct d_pack_sink sink   = d_arg_buffer_sink(&buffer);

    const bool written = ( (d_arg_sink_write(sink,
                                             "ab",
                                             2u) == D_ARG_OK)     &&
                           (d_arg_sink_cstr(sink,
                                            "cd") == D_ARG_OK)    &&
                           (d_arg_sink_cstr(sink,
                                            NULL) == D_ARG_OK)    &&
                           (d_arg_sink_printf(sink,
                                              "%c%d",
                                              'e',
                                              5) == D_ARG_OK) );
    const bool content = d_tests_arg_common_terminated(&buffer,
                                                       "abcde5");

    d_arg_buffer_release(&buffer);

    return ( (written) &&
             (content) );
}

//==============================================================================
// 4.2  SINKS
//==============================================================================

/*
d_tests_arg_common_sink_no_stream
  A sink over no stream, and one that takes nothing.
  Tests the following:
  - d_arg_sink_file(NULL) refuses every write with D_ARG_ERROR_FILE
  - a sink whose function takes nothing is refused the same way
  - NULL bytes with a length are a bad argument, and zero bytes succeed
*/
bool
d_tests_arg_common_sink_no_stream(void)
{
    const struct d_pack_sink none   = d_arg_sink_file(NULL);
    struct d_pack_sink       refuse = none;

    refuse.write   = d_tests_arg_common_refuse;
    refuse.context = NULL;

    return ( (d_arg_sink_write(none,
                               "x",
                               1u) == D_ARG_ERROR_FILE)          &&
             (d_arg_sink_cstr(none,
                              "text") == D_ARG_ERROR_FILE)       &&
             (d_arg_sink_write(refuse,
                               "x",
                               1u) == D_ARG_ERROR_FILE)          &&
             (d_arg_sink_write(refuse,
                               NULL,
                               2u) == D_ARG_ERROR_BAD_ARGUMENT)  &&
             (d_arg_sink_write(refuse,
                               "",
                               0u) == D_ARG_OK) );
}

/*
d_tests_arg_common_sink_file
  A sink over a C stream.
  Tests the following:
  - what is written to the sink is in the file, byte for byte
  Needs tmpfile(), which a system may refuse; the section then fails,
saying so.
*/
bool
d_tests_arg_common_sink_file(void)
{
    FILE* const stream = tmpfile();

    // a system that gives no temporary file cannot run the section
    if (!stream)
    {
        printf("    tmpfile() gave no file\n");

        return false;
    }

    char                     got[16] = { 0 };
    const struct d_pack_sink sink    = d_arg_sink_file(stream);
    const bool               written =
        ( (d_arg_sink_cstr(sink,
                           "to the ") == D_ARG_OK) &&
          (d_arg_sink_printf(sink,
                             "%s",
                             "file") == D_ARG_OK) );

    rewind(stream);

    const size_t count = fread(got,
                               1u,
                               sizeof(got) - 1u,
                               stream);

    (void)fclose(stream);

    return ( (written)      &&
             (count == 11u) &&
             (strcmp(got,
                     "to the file") == 0) );
}

/*
d_tests_arg_common_sink_printf
  Formatted writes through sinks.
  Tests the following:
  - the expansion reaches the sink whole
  - a sink that refuses it reports D_ARG_ERROR_FILE
  - a NULL format is a bad argument
*/
bool
d_tests_arg_common_sink_printf(void)
{
    struct d_arg_buffer      buffer = D_ARG_BUFFER_INIT;
    const struct d_pack_sink sink   = d_arg_buffer_sink(&buffer);
    struct d_pack_sink       refuse = sink;

    refuse.write = d_tests_arg_common_refuse;

    const bool printed = ( (d_arg_sink_printf(sink,
                                              "[%s|%u]",
                                              "x",
                                              7u) == D_ARG_OK)          &&
                           (d_tests_arg_common_terminated(&buffer,
                                                          "[x|7]")) );
    const bool refused = ( (d_arg_sink_printf(refuse,
                                              "%s",
                                              "y") == D_ARG_ERROR_FILE) &&
                           (d_arg_sink_printf(sink,
                                              NULL)
                            == D_ARG_ERROR_BAD_ARGUMENT) );

    d_arg_buffer_release(&buffer);

    return ( (printed) &&
             (refused) );
}

//==============================================================================
// 4.3  ERRORS
//==============================================================================

/*
d_tests_arg_common_error_empty
  An error that reports nothing.
  Tests the following:
  - d_arg_error_init and D_ARG_ERROR_INIT both report D_ARG_OK, with exit
    code 0 and the empty message
  - a NULL error reads the same, and releasing one is harmless
*/
bool
d_tests_arg_common_error_empty(void)
{
    struct d_arg_error initialized = D_ARG_ERROR_INIT;
    struct d_arg_error constant    = D_ARG_ERROR_INIT;

    memset(&initialized,
           0xff,
           sizeof(initialized));
    d_arg_error_init(&initialized);

    const bool empty = ( (initialized.status == (int)D_ARG_OK)         &&
                         (initialized.exit_code == 0)                  &&
                         (initialized.message == NULL)                 &&
                         (initialized.command == NULL)                 &&
                         (initialized.option == NULL)                  &&
                         (constant.status == (int)D_ARG_OK)            &&
                         (strcmp(d_arg_error_message(&constant),
                                 "") == 0)                             &&
                         (strcmp(d_arg_error_message(NULL),
                                 "") == 0)                             &&
                         (d_arg_error_exit_code(NULL) == 0) );

    d_arg_error_release(&initialized);
    d_arg_error_release(NULL);

    return empty;
}

/*
d_tests_arg_common_error_set
  Recording a failure, and replacing it.
  Tests the following:
  - the status, its exit code, a copy of the message, and where the failure
    arose are recorded, and the status is returned
  - a second failure replaces the first, freeing its message
  - releasing returns the error to D_ARG_OK
*/
bool
d_tests_arg_common_error_set(void)
{
    struct d_arg_error error   = D_ARG_ERROR_INIT;
    char               text[8] = "--x req";

    const int  first    = d_arg_error_set(&error,
                                          (int)D_ARG_ERROR_REQUIRED,
                                          NULL,
                                          NULL,
                                          text);
    const bool recorded = ( (first == (int)D_ARG_ERROR_REQUIRED)        &&
                            (error.exit_code == 106)                    &&
                            (error.message != NULL)                     &&
                            (error.message != text)                     &&
                            (strcmp(d_arg_error_message(&error),
                                    "--x req") == 0) );

    text[0] = '!';

    const bool copied   = (strcmp(error.message,
                                  "--x req") == 0);
    const int  second   = d_arg_error_set(&error,
                                          (int)D_ARG_ERROR_EXTRAS,
                                          NULL,
                                          NULL,
                                          "extra");
    const bool replaced = ( (second == (int)D_ARG_ERROR_EXTRAS)         &&
                            (d_arg_error_exit_code(&error) == 109)      &&
                            (strcmp(d_arg_error_message(&error),
                                    "extra") == 0) );

    d_arg_error_release(&error);

    return ( (recorded)                              &&
             (copied)                                &&
             (replaced)                              &&
             (error.status == (int)D_ARG_OK)         &&
             (error.message == NULL) );
}

/*
d_tests_arg_common_error_setf
  Recording a failure with a formatted message.
  Tests the following:
  - the message is the format's expansion
  - a NULL format records no message, and the status's own is read
*/
bool
d_tests_arg_common_error_setf(void)
{
    struct d_arg_error error = D_ARG_ERROR_INIT;

    const int  status    = d_arg_error_setf(&error,
                                            (int)D_ARG_ERROR_VALIDATION,
                                            "%s: %d not in range",
                                            "--n",
                                            12);
    const bool formatted = ( (status == (int)D_ARG_ERROR_VALIDATION)    &&
                             (strcmp(d_arg_error_message(&error),
                                     "--n: 12 not in range") == 0) );
    const int  again     = d_arg_error_setf(&error,
                                            (int)D_ARG_ERROR_CONFIG,
                                            NULL);
    const bool fallback  = ( (again == (int)D_ARG_ERROR_CONFIG)         &&
                             (error.message == NULL)                    &&
                             (strcmp(d_arg_error_message(&error),
                                     "Config error") == 0)              &&
                             (d_arg_error_exit_code(&error) == 110) );

    d_arg_error_release(&error);

    return ( (formatted) &&
             (fallback) );
}

/*
d_tests_arg_common_error_defaults
  The message each status carries when none was recorded.
  Tests the following:
  - CLI11's default messages for the requests, Success and RuntimeError
  - a message in the same words for every other status
*/
bool
d_tests_arg_common_error_defaults(void)
{
    bool described = true;

    // each status, recorded without a message
    for (size_t index = 0u; index < STATUS_ROW_COUNT; ++index)
    {
        struct d_arg_error error = D_ARG_ERROR_INIT;

        (void)d_arg_error_set(&error,
                              STATUS_ROWS[index].status,
                              NULL,
                              NULL,
                              NULL);
        described = ( (described)                              &&
                      (d_tests_arg_same(STATUS_ROWS[index].name,
                                        d_pack_text_from_cstr(
                                            d_arg_error_message(&error)),
                                        STATUS_ROWS[index].message)) );
        d_arg_error_release(&error);
    }

    return described;
}

//==============================================================================
// 4.4  LISTS
//==============================================================================

/*
d_tests_arg_common_string_list
  A list of owned strings.
  Tests the following:
  - each append copies its span, NUL bytes inside it included, and adds a
    terminator
  - many appends grow the list, in order
  - releasing frees every string and leaves an empty list; a NULL list is
    refused
*/
bool
d_tests_arg_common_string_list(void)
{
    static const char HELD[3] = { 'a', '\0', 'b' };
    struct d_arg_string_list list    = D_ARG_LIST_INIT;
    bool                     appended =
        (d_arg_string_list_append(&list,
                                  d_pack_text_from_span(HELD,
                                                        3u)) == D_ARG_OK);

    // enough more to grow the array several times
    for (int index = 0; index < 40; ++index)
    {
        appended = ( (appended) &&
                     (d_arg_string_list_append(&list,
                                               d_pack_text_from_cstr("n"))
                      == D_ARG_OK) );
    }

    const bool copied = ( (list.count == 41u)                      &&
                          (memcmp(list.items[0],
                                  HELD,
                                  3u) == 0)                        &&
                          (list.items[0][3] == '\0')               &&
                          (list.items[0] != HELD)                  &&
                          (strcmp(list.items[40],
                                  "n") == 0) );

    d_arg_string_list_release(&list);

    return ( (appended)                                          &&
             (copied)                                            &&
             (list.items == NULL)                                &&
             (list.count == 0u)                                  &&
             (d_arg_string_list_append(NULL,
                                       d_pack_text_from_cstr("x"))
              == D_ARG_ERROR_BAD_ARGUMENT) );
}

/*
d_tests_arg_common_lists_zeroed
  Lists that were never filled.
  Tests the following:
  - a zeroed list of either kind is an empty one, and releasing it -- or
    NULL -- is harmless
*/
bool
d_tests_arg_common_lists_zeroed(void)
{
    struct d_arg_string_list strings = D_ARG_LIST_INIT;
    struct d_arg_text_list   texts   = D_ARG_LIST_INIT;

    d_arg_string_list_release(&strings);
    d_arg_text_list_release(&texts);
    d_arg_string_list_release(NULL);
    d_arg_text_list_release(NULL);

    return ( (strings.items == NULL) &&
             (strings.count == 0u)   &&
             (texts.items == NULL)   &&
             (texts.count == 0u) );
}

//==============================================================================
// 5.  THE SUITE
//==============================================================================

// d_tests_arg_common
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_common[] =
{
    { "arg_common_status_names",
      d_tests_arg_common_status_names },
    { "arg_common_status_messages",
      d_tests_arg_common_status_messages },
    { "arg_common_status_exit_codes",
      d_tests_arg_common_status_exit_codes },
    { "arg_common_status_is_error",
      d_tests_arg_common_status_is_error },
    { "arg_common_vocabulary",
      d_tests_arg_common_vocabulary },
    { "arg_common_buffer_empty",
      d_tests_arg_common_buffer_empty },
    { "arg_common_buffer_append",
      d_tests_arg_common_buffer_append },
    { "arg_common_buffer_append_refused",
      d_tests_arg_common_buffer_append_refused },
    { "arg_common_buffer_printf",
      d_tests_arg_common_buffer_printf },
    { "arg_common_buffer_truncate_clear",
      d_tests_arg_common_buffer_truncate_clear },
    { "arg_common_buffer_detach",
      d_tests_arg_common_buffer_detach },
    { "arg_common_buffer_sink",
      d_tests_arg_common_buffer_sink },
    { "arg_common_sink_no_stream",
      d_tests_arg_common_sink_no_stream },
    { "arg_common_sink_file",
      d_tests_arg_common_sink_file },
    { "arg_common_sink_printf",
      d_tests_arg_common_sink_printf },
    { "arg_common_error_empty",
      d_tests_arg_common_error_empty },
    { "arg_common_error_set",
      d_tests_arg_common_error_set },
    { "arg_common_error_setf",
      d_tests_arg_common_error_setf },
    { "arg_common_error_defaults",
      d_tests_arg_common_error_defaults },
    { "arg_common_string_list",
      d_tests_arg_common_string_list },
    { "arg_common_lists_zeroed",
      d_tests_arg_common_lists_zeroed }
};

// d_tests_arg_common_count
//   constant: how many sections the table above holds in this build.
const size_t d_tests_arg_common_count =
    sizeof(d_tests_arg_common) / sizeof(d_tests_arg_common[0]);
