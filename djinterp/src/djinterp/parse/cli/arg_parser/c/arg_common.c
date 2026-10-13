/*******************************************************************************
* djinterp [parse]                                                  arg_common.c
*
* Implementation of the shared vocabulary declared in arg_common.h.
*   Status names and exit codes, the growable byte buffer, the sinks over C
* streams and buffers, and the error report.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end, va_copy
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // FILE, fwrite, vsnprintf, stdout, stderr
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memmove, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_sink_emit
#include "../../../../../../inc/djinterp/env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF
#include "./arg_internal.h"  // d_arg_internal_copy_cstr
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uintptr_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// STATUS_NAMES
//   constant: d_arg_status_name's text for each status, one row per
// enumerator in the enumeration's order: CLI11's class names, and names in
// their style for the statuses CLI11 lacks.
static const char* const STATUS_NAMES[] =
{
    "OK",                     // D_ARG_OK
    "CallForHelp",            // D_ARG_CALL_FOR_HELP
    "CallForAllHelp",         // D_ARG_CALL_FOR_ALL_HELP
    "CallForVersion",         // D_ARG_CALL_FOR_VERSION
    "CallForCompletion",      // D_ARG_CALL_FOR_COMPLETION
    "Success",                // D_ARG_SUCCESS
    "RuntimeError",           // D_ARG_RUNTIME_ERROR
    "IncorrectConstruction",  // D_ARG_ERROR_INCORRECT_CONSTRUCTION
    "BadNameString",          // D_ARG_ERROR_BAD_NAME
    "OptionAlreadyAdded",     // D_ARG_ERROR_OPTION_ALREADY_ADDED
    "FileError",              // D_ARG_ERROR_FILE
    "ConversionError",        // D_ARG_ERROR_CONVERSION
    "ValidationError",        // D_ARG_ERROR_VALIDATION
    "RequiredError",          // D_ARG_ERROR_REQUIRED
    "RequiresError",          // D_ARG_ERROR_REQUIRES
    "ExcludesError",          // D_ARG_ERROR_EXCLUDES
    "ExtrasError",            // D_ARG_ERROR_EXTRAS
    "ConfigError",            // D_ARG_ERROR_CONFIG
    "InvalidError",           // D_ARG_ERROR_INVALID
    "HorribleError",          // D_ARG_ERROR_HORRIBLE
    "OptionNotFound",         // D_ARG_ERROR_OPTION_NOT_FOUND
    "ArgumentMismatch",       // D_ARG_ERROR_ARGUMENT_MISMATCH
    "OutOfMemory",            // D_ARG_ERROR_OUT_OF_MEMORY
    "BadArgument",            // D_ARG_ERROR_BAD_ARGUMENT
    "Unsupported"             // D_ARG_ERROR_UNSUPPORTED
};

D_STATIC_ASSERT( (sizeof(STATUS_NAMES) / sizeof(STATUS_NAMES[0])) ==
                 (size_t)D_ARG_STATUS_COUNT,
                 "every d_arg_status needs a name" );

// STATUS_EXIT_CODES
//   constant: d_arg_status_exit_code's code for each status, in the same
// order: CLI11's ExitCodes, 1 for RuntimeError as CLI11's default, and
// BaseClass for the statuses CLI11 lacks.
static const int STATUS_EXIT_CODES[] =
{
    D_ARG_EXIT_SUCCESS,                 // D_ARG_OK
    D_ARG_EXIT_SUCCESS,                 // D_ARG_CALL_FOR_HELP
    D_ARG_EXIT_SUCCESS,                 // D_ARG_CALL_FOR_ALL_HELP
    D_ARG_EXIT_SUCCESS,                 // D_ARG_CALL_FOR_VERSION
    D_ARG_EXIT_SUCCESS,                 // D_ARG_CALL_FOR_COMPLETION
    D_ARG_EXIT_SUCCESS,                 // D_ARG_SUCCESS
    1,                                  // D_ARG_RUNTIME_ERROR
    D_ARG_EXIT_INCORRECT_CONSTRUCTION,  // D_ARG_ERROR_INCORRECT_CONSTRUCTION
    D_ARG_EXIT_BAD_NAME_STRING,         // D_ARG_ERROR_BAD_NAME
    D_ARG_EXIT_OPTION_ALREADY_ADDED,    // D_ARG_ERROR_OPTION_ALREADY_ADDED
    D_ARG_EXIT_FILE_ERROR,              // D_ARG_ERROR_FILE
    D_ARG_EXIT_CONVERSION_ERROR,        // D_ARG_ERROR_CONVERSION
    D_ARG_EXIT_VALIDATION_ERROR,        // D_ARG_ERROR_VALIDATION
    D_ARG_EXIT_REQUIRED_ERROR,          // D_ARG_ERROR_REQUIRED
    D_ARG_EXIT_REQUIRES_ERROR,          // D_ARG_ERROR_REQUIRES
    D_ARG_EXIT_EXCLUDES_ERROR,          // D_ARG_ERROR_EXCLUDES
    D_ARG_EXIT_EXTRAS_ERROR,            // D_ARG_ERROR_EXTRAS
    D_ARG_EXIT_CONFIG_ERROR,            // D_ARG_ERROR_CONFIG
    D_ARG_EXIT_INVALID_ERROR,           // D_ARG_ERROR_INVALID
    D_ARG_EXIT_HORRIBLE_ERROR,          // D_ARG_ERROR_HORRIBLE
    D_ARG_EXIT_OPTION_NOT_FOUND,        // D_ARG_ERROR_OPTION_NOT_FOUND
    D_ARG_EXIT_ARGUMENT_MISMATCH,       // D_ARG_ERROR_ARGUMENT_MISMATCH
    D_ARG_EXIT_BASE_CLASS,              // D_ARG_ERROR_OUT_OF_MEMORY
    D_ARG_EXIT_BASE_CLASS,              // D_ARG_ERROR_BAD_ARGUMENT
    D_ARG_EXIT_BASE_CLASS               // D_ARG_ERROR_UNSUPPORTED
};

D_STATIC_ASSERT( (sizeof(STATUS_EXIT_CODES) /
                  sizeof(STATUS_EXIT_CODES[0])) ==
                 (size_t)D_ARG_STATUS_COUNT,
                 "every d_arg_status needs an exit code" );

// D_INTERNAL_ARG_CAUGHT
//   constant: CLI11's message for the requests that end a parse successfully,
// which a main function catches rather than prints.
#define D_INTERNAL_ARG_CAUGHT                                                  \
    "This should be caught in your main function, see examples"

// STATUS_MESSAGES
//   constant: d_arg_error_message's text for a status recorded without a
// message, in the same order: CLI11's default messages where CLI11 has one.
static const char* const STATUS_MESSAGES[] =
{
    "",                         // D_ARG_OK
    D_INTERNAL_ARG_CAUGHT,      // D_ARG_CALL_FOR_HELP
    D_INTERNAL_ARG_CAUGHT,      // D_ARG_CALL_FOR_ALL_HELP
    D_INTERNAL_ARG_CAUGHT,      // D_ARG_CALL_FOR_VERSION
    D_INTERNAL_ARG_CAUGHT,      // D_ARG_CALL_FOR_COMPLETION
    "Successfully completed, should be caught and quit",  // D_ARG_SUCCESS
    "Runtime error",            // D_ARG_RUNTIME_ERROR
    "Incorrect construction",   // D_ARG_ERROR_INCORRECT_CONSTRUCTION
    "Bad name string",          // D_ARG_ERROR_BAD_NAME
    "Option already added",     // D_ARG_ERROR_OPTION_ALREADY_ADDED
    "File error",               // D_ARG_ERROR_FILE
    "Conversion error",         // D_ARG_ERROR_CONVERSION
    "Validation error",         // D_ARG_ERROR_VALIDATION
    "Required error",           // D_ARG_ERROR_REQUIRED
    "Requires error",           // D_ARG_ERROR_REQUIRES
    "Excludes error",           // D_ARG_ERROR_EXCLUDES
    "Extras error",             // D_ARG_ERROR_EXTRAS
    "Config error",             // D_ARG_ERROR_CONFIG
    "Invalid error",            // D_ARG_ERROR_INVALID
    "Horrible error",           // D_ARG_ERROR_HORRIBLE
    "Option not found",         // D_ARG_ERROR_OPTION_NOT_FOUND
    "Argument mismatch",        // D_ARG_ERROR_ARGUMENT_MISMATCH
    "Out of memory",            // D_ARG_ERROR_OUT_OF_MEMORY
    "Bad argument",             // D_ARG_ERROR_BAD_ARGUMENT
    "Unsupported"               // D_ARG_ERROR_UNSUPPORTED
};

D_STATIC_ASSERT( (sizeof(STATUS_MESSAGES) /
                  sizeof(STATUS_MESSAGES[0])) ==
                 (size_t)D_ARG_STATUS_COUNT,
                 "every d_arg_status needs a default message" );

// d_arg_internal_empty
//   variable: what an empty buffer's `data` points at, so that `data` is a
// NUL-terminated string even before anything is stored. Never written: a
// buffer with no capacity of its own allocates before its first write.
static char d_arg_internal_empty[1] = { '\0' };

// D_INTERNAL_ARG_BUFFER_MINIMUM
//   constant: the least capacity a buffer grows to, so that short messages
// cost one allocation.
#define D_INTERNAL_ARG_BUFFER_MINIMUM 64u

/*
d_arg_internal_status_index
  A status as an index into the three tables, or -1 outside the enum; every
lookup goes through it, so no table is read out of range.
*/
D_STATIC int
d_arg_internal_status_index(
    int _status
)
{
    // outside the enumeration, the lookups fall back to "Error"
    if ( (_status < 0) ||
         (_status >= (int)D_ARG_STATUS_COUNT) )
    {
        return -1;
    }

    return _status;
}

/*
d_arg_internal_buffer_grow
  Grows a buffer's storage to hold `_needed` bytes, doubling so that a run of
appends costs amortized constant time. A buffer whose `data` is not its own
-- d_arg_internal_empty, or NULL in a zeroed buffer -- has capacity 0 and so
allocates afresh; `realloc(NULL, ...)` covers both.
*/
D_STATIC int
d_arg_internal_buffer_grow(
    struct d_arg_buffer* _buffer,
    size_t               _needed
)
{
    // nothing to do while the storage is large enough
    if (_needed <= _buffer->capacity)
    {
        return D_ARG_OK;
    }

    size_t capacity = (_buffer->capacity > 0u)
                      ? _buffer->capacity
                      : (size_t)D_INTERNAL_ARG_BUFFER_MINIMUM;

    // double until large enough, stopping short of overflow
    while (capacity < _needed)
    {
        // doubling would wrap: take exactly what is needed
        if (capacity > ((size_t)-1 / 2u))
        {
            capacity = _needed;

            break;
        }

        capacity *= 2u;
    }

    char* const previous = (_buffer->capacity > 0u)
                           ? _buffer->data
                           : NULL;
    char* const storage  = (char*)realloc(previous,
                                          capacity);

    // a refused allocation leaves the content as it was
    if (!storage)
    {
        _buffer->failed = 1;

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // a first allocation carries over the (empty) content's terminator
    if (_buffer->capacity == 0u)
    {
        storage[0] = '\0';
    }

    _buffer->data     = storage;
    _buffer->capacity = capacity;

    return D_ARG_OK;
}

/*
d_arg_internal_buffer_write
  The write function of d_arg_buffer_sink: appends, and reports a short write
-- zero bytes taken -- once the append fails, which d_sink_emit and every
caller turn into a refused write.
*/
D_STATIC size_t
d_arg_internal_buffer_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    const int status = d_arg_buffer_append((struct d_arg_buffer*)_context,
                                           (const char*)_data,
                                           _size);

    return (status == D_ARG_OK) ? _size : (size_t)0;
}

/*
d_arg_internal_file_write
  The write function of d_arg_sink_file: fwrite, whose count of whole items
of size 1 is the count of bytes taken. A NULL stream takes nothing.
*/
D_STATIC size_t
d_arg_internal_file_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    // a sink over no stream refuses every write
    if (!_context)
    {
        return 0u;
    }

    return fwrite(_data,
                  1u,
                  _size,
                  (FILE*)_context);
}

/*
d_arg_internal_vformat
  Appends `vsnprintf`'s expansion of `_format` to a buffer: measures with a
copy of the argument list, grows once, and formats in place, so the text is
formatted exactly twice whatever its length. A buffer that has failed
reports its failure again.
*/
int
d_arg_internal_vformat(
    struct d_arg_buffer* _buffer,
    const char*          _format,
    va_list              _arguments
)
{
    // a failed buffer reports its failure again
    if (_buffer->failed)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    va_list measure;

    va_copy(measure,
            _arguments);

    const int length = vsnprintf(NULL,
                                 0u,
                                 _format,
                                 measure);

    va_end(measure);

    // the format could not be expanded at all
    if (length < 0)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const size_t start  = _buffer->length;
    const int    status = d_arg_internal_buffer_grow(_buffer,
                                                     start +
                                                     (size_t)length +
                                                     1u);

    // the storage could not hold the expansion
    if (status != D_ARG_OK)
    {
        return status;
    }

    (void)vsnprintf(_buffer->data + start,
                    (size_t)length + 1u,
                    _format,
                    _arguments);

    _buffer->length = start + (size_t)length;

    return D_ARG_OK;
}

//==============================================================================
// 1.  STATUS
//==============================================================================

/*
d_arg_status_name
  A table lookup; see STATUS_NAMES.
*/
const char*
d_arg_status_name(
    int _status
)
{
    const int index = d_arg_internal_status_index(_status);

    return (index >= 0) ? STATUS_NAMES[index] : "Error";
}

/*
d_arg_status_exit_code
  A table lookup; see STATUS_EXIT_CODES.
*/
int
d_arg_status_exit_code(
    int _status
)
{
    const int index = d_arg_internal_status_index(_status);

    return (index >= 0)
           ? STATUS_EXIT_CODES[index]
           : (int)D_ARG_EXIT_BASE_CLASS;
}

//==============================================================================
// 4.  OPERATIONS
//==============================================================================

/*
d_arg_buffer_init
  Points `data` at d_arg_internal_empty rather than NULL, so the buffer reads
as an empty string before anything is stored.
*/
void
d_arg_buffer_init(
    struct d_arg_buffer* _buffer
)
{
    // nothing to initialize
    if (!_buffer)
    {
        return;
    }

    _buffer->data     = d_arg_internal_empty;
    _buffer->length   = 0u;
    _buffer->capacity = 0u;
    _buffer->failed   = 0;

    return;
}

/*
d_arg_buffer_release
  Only storage the buffer allocated -- capacity above 0 -- is freed.
*/
void
d_arg_buffer_release(
    struct d_arg_buffer* _buffer
)
{
    // nothing to release
    if (!_buffer)
    {
        return;
    }

    // the shared empty text and a zeroed buffer's NULL are not its own
    if (_buffer->capacity > 0u)
    {
        free(_buffer->data);
    }

    d_arg_buffer_init(_buffer);

    return;
}

/*
d_arg_buffer_clear
  Keeps the storage, and the failure: a buffer that has lost text stays
failed until it is released.
*/
void
d_arg_buffer_clear(
    struct d_arg_buffer* _buffer
)
{
    // nothing to clear
    if (!_buffer)
    {
        return;
    }

    _buffer->length = 0u;

    // only the buffer's own storage is written
    if (_buffer->capacity > 0u)
    {
        _buffer->data[0] = '\0';
    }

    return;
}

/*
d_arg_buffer_truncate
  Only the buffer's own storage is written; a buffer without storage holds
nothing to cut.
*/
void
d_arg_buffer_truncate(
    struct d_arg_buffer* _buffer,
    size_t               _length
)
{
    // nothing to cut, or nothing past the length
    if ( (!_buffer) ||
         (_length >= _buffer->length) )
    {
        return;
    }

    _buffer->length        = _length;
    _buffer->data[_length] = '\0';

    return;
}

/*
d_arg_internal_buffer_admit
  The checks an append passes before it grows anything: the arguments, a
buffer that already failed -- which reports its failure again -- and a
length that, with the text and its terminator, must fit a size_t; the last
failure marks the buffer failed.
*/
D_STATIC int
d_arg_internal_buffer_admit(
    struct d_arg_buffer* _buffer,
    const char*          _data,
    size_t               _length
)
{
    // a buffer is required, and data whenever there are bytes
    if ( (!_buffer) ||
         ( (!_data) &&
           (_length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a failed buffer reports its failure again
    if (_buffer->failed)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // the length, the terminator and the existing text must fit a size_t
    if (_length > ((size_t)-1 - _buffer->length - 1u))
    {
        _buffer->failed = 1;

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    return D_ARG_OK;
}

/*
d_arg_buffer_append
  `_data` may point into the buffer itself -- appending a part of the text to
the text -- so its offset is taken before growth can move the storage, and
the bytes are copied from the moved storage afterward. The test compares
addresses as integers, since ordering pointers into different objects is
undefined.
*/
int
d_arg_buffer_append(
    struct d_arg_buffer* _buffer,
    const char*          _data,
    size_t               _length
)
{
    const int admitted = d_arg_internal_buffer_admit(_buffer,
                                                     _data,
                                                     _length);

    // refused, or nothing to append
    if ( (admitted != D_ARG_OK) ||
         (_length == 0u) )
    {
        return admitted;
    }

    const uintptr_t source  = (uintptr_t)_data;
    const uintptr_t first   = (uintptr_t)_buffer->data;
    const bool      aliased = ( (_buffer->capacity > 0u) &&
                                (source >= first)        &&
                                (source <  first + _buffer->capacity) );
    const size_t    offset  = aliased ? (size_t)(source - first) : 0u;
    const int       status  = d_arg_internal_buffer_grow(_buffer,
                                                         _buffer->length +
                                                         _length +
                                                         1u);

    // growth was refused; the content is unchanged
    if (status != D_ARG_OK)
    {
        return status;
    }

    memmove(_buffer->data + _buffer->length,
            aliased ? (_buffer->data + offset) : _data,
            _length);

    _buffer->length               += _length;
    _buffer->data[_buffer->length]  = '\0';

    return D_ARG_OK;
}

/*
d_arg_buffer_append_cstr
  strlen and append; NULL is the empty string.
*/
int
d_arg_buffer_append_cstr(
    struct d_arg_buffer* _buffer,
    const char*          _text
)
{
    return d_arg_buffer_append(_buffer,
                               _text,
                               (_text != NULL) ? strlen(_text) : 0u);
}

/*
d_arg_buffer_append_text
  The span's own pointer and length; a NULL span is empty.
*/
int
d_arg_buffer_append_text(
    struct d_arg_buffer* _buffer,
    struct d_pack_text   _text
)
{
    return d_arg_buffer_append(_buffer,
                               _text.data,
                               (_text.data != NULL) ? _text.length : 0u);
}

/*
d_arg_buffer_append_char
  One byte through the general append.
*/
int
d_arg_buffer_append_char(
    struct d_arg_buffer* _buffer,
    char                 _character
)
{
    return d_arg_buffer_append(_buffer,
                               &_character,
                               1u);
}

/*
d_arg_buffer_printf
  The variadic shell of d_arg_internal_vformat, with the same checks as the
other appends.
*/
int
d_arg_buffer_printf(
    struct d_arg_buffer* _buffer,
    const char*          _format,
    ...
)
{
    // a buffer and a format are required
    if ( (!_buffer) ||
         (!_format) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a failed buffer reports its failure again
    if (_buffer->failed)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    va_list arguments;

    va_start(arguments,
             _format);

    const int status = d_arg_internal_vformat(_buffer,
                                              _format,
                                              arguments);

    va_end(arguments);

    return status;
}

/*
d_arg_buffer_detach
  Hands over the storage only when the buffer owns some; d_arg_internal_empty
is never handed out, since the caller would free it.
*/
char*
d_arg_buffer_detach(
    struct d_arg_buffer* _buffer
)
{
    // only owned storage can change hands
    if ( (!_buffer) ||
         (_buffer->capacity == 0u) )
    {
        d_arg_buffer_init(_buffer);

        return NULL;
    }

    char* const storage = _buffer->data;

    d_arg_buffer_init(_buffer);

    return storage;
}

/*
d_arg_buffer_text
  A view of the content, d_arg_internal_empty included; a NULL or zeroed
buffer views the empty string.
*/
struct d_pack_text
d_arg_buffer_text(
    const struct d_arg_buffer* _buffer
)
{
    // a zeroed buffer's NULL reads as the empty string
    if ( (_buffer == NULL) ||
         (_buffer->data == NULL) )
    {
        return d_arg_internal_span(d_arg_internal_empty,
                                   0u);
    }

    return d_arg_internal_span(_buffer->data,
                               _buffer->length);
}

/*
d_arg_buffer_sink
  A sink whose context is the buffer itself.
*/
struct d_pack_sink
d_arg_buffer_sink(
    struct d_arg_buffer* _buffer
)
{
    const struct d_pack_sink sink = { d_arg_internal_buffer_write,
                                      _buffer };

    return sink;
}

/*
d_arg_sink_file
  A sink whose context is the stream; the write function refuses when the
stream is NULL, so the sink is always callable.
*/
struct d_pack_sink
d_arg_sink_file(
    FILE* _stream
)
{
    const struct d_pack_sink sink = { d_arg_internal_file_write,
                                      _stream };

    return sink;
}

/*
d_arg_sink_stdout
  d_arg_sink_file over stdout.
*/
struct d_pack_sink
d_arg_sink_stdout(
    void
)
{
    return d_arg_sink_file(stdout);
}

/*
d_arg_sink_stderr
  d_arg_sink_file over stderr.
*/
struct d_pack_sink
d_arg_sink_stderr(
    void
)
{
    return d_arg_sink_file(stderr);
}

/*
d_arg_sink_write
  d_sink_emit, with its yes-or-no answer mapped onto the parser's statuses.
*/
int
d_arg_sink_write(
    struct d_pack_sink _sink,
    const char*        _data,
    size_t             _length
)
{
    // bytes need data
    if ( (!_data) &&
         (_length > 0u) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // nothing to write is always written
    if (_length == 0u)
    {
        return D_ARG_OK;
    }

    return d_sink_emit(_sink,
                       _data,
                       _length)
           ? D_ARG_OK
           : D_ARG_ERROR_FILE;
}

/*
d_arg_sink_cstr
  strlen and write; NULL writes nothing.
*/
int
d_arg_sink_cstr(
    struct d_pack_sink _sink,
    const char*        _text
)
{
    return d_arg_sink_write(_sink,
                            _text,
                            (_text != NULL) ? strlen(_text) : 0u);
}

/*
d_arg_sink_printf
  Formats into a buffer on the heap and writes it whole, so a sink sees one
write per call, as it would from d_arg_sink_write.
*/
int
d_arg_sink_printf(
    struct d_pack_sink _sink,
    const char*        _format,
    ...
)
{
    // a format is required
    if (!_format)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer text = D_ARG_BUFFER_INIT;
    va_list             arguments;

    va_start(arguments,
             _format);

    int status = d_arg_internal_vformat(&text,
                                        _format,
                                        arguments);

    va_end(arguments);

    // only a formatted text is written
    if (status == D_ARG_OK)
    {
        status = d_arg_sink_write(_sink,
                                  text.data,
                                  text.length);
    }

    d_arg_buffer_release(&text);

    return status;
}

/*
d_arg_error_init
  An error that reports nothing.
*/
void
d_arg_error_init(
    struct d_arg_error* _error
)
{
    // nothing to initialize
    if (!_error)
    {
        return;
    }

    _error->status    = D_ARG_OK;
    _error->exit_code = D_ARG_EXIT_SUCCESS;
    _error->message   = NULL;
    _error->command   = NULL;
    _error->option    = NULL;

    return;
}

/*
d_arg_error_release
  Frees the message and reports nothing again.
*/
void
d_arg_error_release(
    struct d_arg_error* _error
)
{
    // nothing to release
    if (!_error)
    {
        return;
    }

    free(_error->message);
    d_arg_error_init(_error);

    return;
}

/*
d_arg_error_set
  The new message is copied before the old one is freed, so a message taken
from the error itself -- d_arg_error_message's -- survives being recorded
again.
*/
int
d_arg_error_set(
    struct d_arg_error*         _error,
    int                         _status,
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    const char*                 _message
)
{
    // nowhere to record; the status still stands
    if (!_error)
    {
        return _status;
    }

    char* const copy = (_message != NULL)
                       ? d_arg_internal_copy_cstr(_message)
                       : NULL;

    free(_error->message);

    _error->status    = _status;
    _error->exit_code = d_arg_status_exit_code(_status);
    _error->message   = copy;
    _error->command   = _command;
    _error->option    = _option;

    return _status;
}

/*
d_arg_error_setf
  Formats into a buffer whose storage becomes the message; a format that
fails records the status without a message, as a refused copy does in
d_arg_error_set.
*/
int
d_arg_error_setf(
    struct d_arg_error* _error,
    int                 _status,
    const char*         _format,
    ...
)
{
    // nowhere to record; the status still stands
    if (!_error)
    {
        return _status;
    }

    struct d_arg_buffer text = D_ARG_BUFFER_INIT;
    va_list             arguments;

    // only a format can be expanded
    if (_format != NULL)
    {
        va_start(arguments,
                 _format);
        (void)d_arg_internal_vformat(&text,
                                     _format,
                                     arguments);
        va_end(arguments);
    }

    (void)d_arg_error_set(_error,
                          _status,
                          NULL,
                          NULL,
                          NULL);
    _error->message = d_arg_buffer_detach(&text);

    return _status;
}

/*
d_arg_error_message
  The recorded message, or the status's default from STATUS_MESSAGES, or
"Error" outside the enum.
*/
const char*
d_arg_error_message(
    const struct d_arg_error* _error
)
{
    // no error reads as D_ARG_OK, whose message is empty
    if (!_error)
    {
        return "";
    }

    // a recorded message wins
    if (_error->message != NULL)
    {
        return _error->message;
    }

    const int index = d_arg_internal_status_index(_error->status);

    return (index >= 0) ? STATUS_MESSAGES[index] : "Error";
}

/*
d_arg_error_exit_code
  The recorded code, which is the status's unless a runtime error chose its
own.
*/
int
d_arg_error_exit_code(
    const struct d_arg_error* _error
)
{
    return (_error != NULL) ? _error->exit_code : (int)D_ARG_EXIT_SUCCESS;
}

/*
d_arg_string_list_release
  Every string, then the array; a zeroed list frees nothing.
*/
void
d_arg_string_list_release(
    struct d_arg_string_list* _list
)
{
    // nothing to free without a list
    if (_list == NULL)
    {
        return;
    }

    d_arg_internal_string_truncate(_list,
                                   0u);
    free(_list->items);
    _list->items    = NULL;
    _list->capacity = 0u;

    return;
}

/*
d_arg_text_list_release
  The array alone: a text list borrows its bytes.
*/
void
d_arg_text_list_release(
    struct d_arg_text_list* _list
)
{
    // nothing to free without a list
    if (_list != NULL)
    {
        free(_list->items);
        _list->items    = NULL;
        _list->count    = 0u;
        _list->capacity = 0u;
    }

    return;
}

/*
d_arg_string_list_append
  d_arg_internal_string_copy without the copy's address.
*/
int
d_arg_string_list_append(
    struct d_arg_string_list* _list,
    struct d_pack_text        _text
)
{
    // validate the parameters
    if ( (!_list) ||
         ( (_text.data == NULL) &&
           (_text.length != 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_string_copy(_list,
                                      _text,
                                      NULL);
}
