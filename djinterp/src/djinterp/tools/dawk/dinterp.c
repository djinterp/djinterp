/******************************************************************************
* djinterp [dawk]                                                    dinterp.c
*
*   Definitions for the non-inline declarations in dinterp.h.
*     Two flags carry the lazy field discipline. `fields_valid` says the record
* has been divided; `record_valid` says $0 agrees with the fields. Reading a
* field or NF forces the first, writing a field clears the second, and writing
* $0 clears the first. Everything awk specifies about fields follows from
* keeping those two honest.
*     Evaluation writes into a caller-supplied cell rather than returning one,
* so ownership never leaves the caller. Temporaries come from a free list to
* keep the allocator out of the inner loop.
*
*
* path:      /src/djinterp/tools/dawk/dinterp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dinterp.h"  // corresponding header
#include "../../../../inc/djinterp/tools/dawk/dsource.h"  // d_awk_source
// std
#include <math.h>     // sin, cos, atan2, exp, log, sqrt, floor, fmod
#include <stdio.h>    // FILE, fopen, fgetc, printf, snprintf
#include <stdlib.h>   // malloc, free, rand, srand
#include <string.h>   // memcpy, memcmp, strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dregex.h"  // d_regex_search
#include "../../../../inc/djinterp/tools/dawk/dvalue.h"  // d_awk_value


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Control flow
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_flow
//   enum: the pending non-local transfer, if any.  Every statement checks it
//   after each child, which is cheaper than unwinding with setjmp and keeps
//   the interpreter reentrant.
enum d_internal_flow
{
    D_INTERNAL_FLOW_NONE = 0,
    D_INTERNAL_FLOW_BREAK,
    D_INTERNAL_FLOW_CONTINUE,
    D_INTERNAL_FLOW_NEXT,
    D_INTERNAL_FLOW_RETURN,
    D_INTERNAL_FLOW_EXIT,
    D_INTERNAL_FLOW_ERROR
};

// 1.2    Frames
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_frame
//   struct: one call's parameters and locals.  A parameter bound to an array
//   holds a borrowed pointer, since arrays pass by reference; a scalar holds
//   a cell the frame owns.
struct d_internal_frame
{
    char* const*         names;
    size_t               count;
    struct d_awk_value** cells;
    struct d_awk_array** arrays;
    bool*                owns_array;
};

// 1.3    Streams
//------------------------------------------------------------------------------
// 1.3.1
// d_internal_stream_kind
//   enum: what a named stream is.  The direction is part of the identity: a
//   program may hold "f" open for reading and for writing at once, and close()
//   shuts both.
enum d_internal_stream_kind
{
    D_INTERNAL_STREAM_IN_FILE = 0,
    D_INTERNAL_STREAM_IN_PIPE,
    D_INTERNAL_STREAM_OUT_FILE,
    D_INTERNAL_STREAM_OUT_APPEND,
    D_INTERNAL_STREAM_OUT_PIPE
};

// 1.3.2
// d_internal_stream
//   struct: one stream the program opened by name.  `eof` is remembered so a
//   getline past the end keeps reporting zero rather than reopening.
struct d_internal_stream
{
    char*                          name;
    FILE*                          handle;
    enum d_internal_stream_kind    kind;
    bool                           eof;
};

// 1.4    Interpreter
//------------------------------------------------------------------------------
// 1.4.1
// d_awk_interp
//   struct: program, globals, record state and the temporary free list.
struct d_awk_interp
{
    struct d_awk_program* program;
    struct d_awk_array*   globals;
    struct d_awk_array**  owned;
    size_t                owned_count;
    size_t                owned_capacity;

    // record state
    char*                 record;
    size_t                record_length;
    size_t                record_capacity;
    bool                  fields_valid;
    bool                  record_valid;
    char*                 split_fs;
    size_t                split_fs_capacity;
    struct d_awk_value**  fields;
    size_t                field_count;
    size_t                field_capacity;

    // input
    char**                inputs;
    size_t                input_count;
    size_t                argv_at;
    FILE*                 stream;
    bool                  stream_owned;
    bool                  used_stdin;
    bool                  opened_any;
    char*                 read_buffer;
    size_t                read_capacity;

    // host record source; NULL keeps POSIX RS splitting
    struct d_awk_source*  source;

    // streams the program opened by name
    struct d_internal_stream* streams;
    size_t                    stream_count;
    size_t                    stream_capacity;

    // frames
    struct d_internal_frame* frames;
    size_t                   frame_count;
    size_t                   frame_capacity;

    // temporaries
    struct d_awk_value**  pool;
    size_t                pool_count;
    size_t                pool_capacity;

    enum d_internal_flow  flow;
    struct d_awk_value*   retval;
    int                   exit_status;
    bool                  exiting;
    unsigned              seed;
    unsigned              prev_seed;
    char                  error[256];
};


static bool d_internal_eval(struct d_awk_interp* _in,
                            struct d_awk_node*   _node,
                            struct d_awk_value*  _out);
static bool d_internal_exec(struct d_awk_interp* _in,
                            struct d_awk_node*   _node);
static bool d_internal_read_one(struct d_awk_interp* _in,
                                FILE*                _stream,
                                size_t*              _out_length);
static bool d_internal_next_main_record(struct d_awk_interp* _in,
                                        size_t*              _out_length);
static bool d_internal_getline(struct d_awk_interp* _in,
                               struct d_awk_node*   _node,
                               struct d_awk_value*  _out);
static FILE* d_internal_stream_open(struct d_awk_interp*        _in,
                                    const char*                 _name,
                                    enum d_internal_stream_kind _kind);
static int   d_internal_stream_close(struct d_awk_interp* _in,
                                     const char*          _name);


/*
d_internal_fail
  Records a runtime diagnostic and raises the error flow.

Parameter(s):
  _in:      the interpreter.
  _message: the text of the diagnostic.
Return:
  Always false, so callers can `return d_internal_fail(...)`.
*/
static bool
d_internal_fail(
    struct d_awk_interp* _in,
    const char*          _message
)
{
    // keep the first diagnostic rather than the last
    if (_in->error[0] == '\0')
    {
        snprintf(_in->error, sizeof(_in->error), "dawk: %s", _message);
    }

    _in->flow = D_INTERNAL_FLOW_ERROR;

    return false;
}


//==============================================================================
// 2.  TEMPORARIES AND VARIABLES
//==============================================================================


/*
d_internal_acquire
  Takes a scratch cell from the free list, or makes one.

Parameter(s):
  _in: the interpreter.
Return:
  The cell, or NULL on failure.
*/
static struct d_awk_value*
d_internal_acquire(
    struct d_awk_interp* _in
)
{
    // a cell already on the list is reused rather than reallocated
    if (_in->pool_count > 0)
    {
        struct d_awk_value* cell = _in->pool[--_in->pool_count];

        d_awk_value_set_uninit(cell);

        return cell;
    }

    return d_awk_value_new();
}


/*
d_internal_release
  Returns a scratch cell to the free list.

Parameter(s):
  _in:   the interpreter.
  _cell: the cell to return; may be NULL.
Return:
  none.
*/
static void
d_internal_release(
    struct d_awk_interp* _in,
    struct d_awk_value*  _cell
)
{
    // a cell that cannot be held is simply freed
    if (!_cell)
    {
        return;
    }

    // grow the list when it is full
    if (_in->pool_count == _in->pool_capacity)
    {
        const size_t capacity = (_in->pool_capacity == 0)
                              ? 32u
                              : (_in->pool_capacity * 2u);

        struct d_awk_value** grown =
            realloc(_in->pool, capacity * sizeof(*grown));

        // free the cell rather than losing track of it
        if (!grown)
        {
            d_awk_value_free(_cell);
            return;
        }

        _in->pool          = grown;
        _in->pool_capacity = capacity;
    }

    d_awk_value_set_uninit(_cell);
    _in->pool[_in->pool_count++] = _cell;

    return;
}


/*
d_internal_local
  Finds a parameter or local of the innermost call, if any.

Parameter(s):
  _in:     the interpreter.
  _name:   the variable name.
  _out_at: receives the index within the frame; may be NULL.
Return:
  The frame holding it, or NULL when the name is not local.
*/
static struct d_internal_frame*
d_internal_local(
    struct d_awk_interp* _in,
    const char*          _name,
    size_t*              _out_at
)
{
    // only the innermost frame is visible; awk has no nested scopes
    if (_in->frame_count == 0)
    {
        return NULL;
    }

    struct d_internal_frame* frame = &_in->frames[_in->frame_count - 1u];

    for (size_t at = 0; at < frame->count; ++at)
    {
        if (strcmp(frame->names[at], _name) == 0)
        {
            // report the slot when the caller asked for it
            if (_out_at)
            {
                *_out_at = at;
            }

            return frame;
        }
    }

    return NULL;
}


/*
d_internal_cell
  Resolves a scalar variable to its cell, creating a global if needed.

Parameter(s):
  _in:   the interpreter.
  _name: the variable name.
Return:
  The cell, or NULL on failure.
*/
static struct d_awk_value*
d_internal_cell(
    struct d_awk_interp* _in,
    const char*          _name
)
{
    size_t                         at    = 0;
    struct d_internal_frame* const frame = d_internal_local(_in, _name, &at);

    // a local shadows a global of the same name
    if (frame)
    {
        // a slot first used as a scalar acquires a cell here
        if (!frame->cells[at])
        {
            frame->cells[at] = d_awk_value_new();
        }

        return frame->cells[at];
    }

    return d_awk_array_lookup(_in->globals, _name, strlen(_name));
}


/*
d_internal_array
  Resolves a variable to its array, creating one if needed.

Parameter(s):
  _in:   the interpreter.
  _name: the variable name.
Return:
  The array, or NULL on failure.
*/
static struct d_awk_array*
d_internal_array(
    struct d_awk_interp* _in,
    const char*          _name
)
{
    size_t                         at    = 0;
    struct d_internal_frame* const frame = d_internal_local(_in, _name, &at);

    // a local slot may already hold a borrowed or an owned array
    if (frame)
    {
        // a slot first used as an array acquires one, owned by the frame
        if (!frame->arrays[at])
        {
            frame->arrays[at]     = d_awk_array_new();
            frame->owns_array[at] = true;
        }

        return frame->arrays[at];
    }

    struct d_awk_value* const cell = d_awk_array_lookup(_in->globals,
                                                        _name,
                                                        strlen(_name));

    // abandon the lookup when the global could not be created
    if (!cell)
    {
        return NULL;
    }

    // a global first used as an array acquires one, owned by the interpreter
    if (d_awk_value_tag_of(cell) != D_AWK_VAL_ARRAY)
    {
        struct d_awk_array* const array = d_awk_array_new();

        // abandon the lookup when the array could not be held
        if (!array)
        {
            return NULL;
        }

        // grow the ownership list when it is full
        if (_in->owned_count == _in->owned_capacity)
        {
            const size_t capacity = (_in->owned_capacity == 0)
                                  ? 8u
                                  : (_in->owned_capacity * 2u);

            struct d_awk_array** grown =
                realloc(_in->owned, capacity * sizeof(*grown));

            // free the array rather than losing track of it
            if (!grown)
            {
                d_awk_array_free(array);
                return NULL;
            }

            _in->owned          = grown;
            _in->owned_capacity = capacity;
        }

        _in->owned[_in->owned_count++] = array;
        (void)d_awk_value_set_array(cell, array);
    }

    return d_awk_value_array_of(cell);
}


/*
d_internal_special
  Returns the text of a special variable, or a default when it is unset.
CAUTION:
  Unset and empty are different, and the difference is load-bearing.  FS=""
  splits a record into characters and RS="" selects paragraph mode; falling
  back to the default whenever the text is empty makes both unreachable.  The
  tag decides, not the length.

Parameter(s):
  _in:       the interpreter.
  _name:     the variable name.
  _fallback: the value to use when the variable holds nothing.
Return:
  The text, owned by the variable's cell.
*/
static const char*
d_internal_special(
    struct d_awk_interp* _in,
    const char*          _name,
    const char*          _fallback
)
{
    struct d_awk_value* const cell = d_internal_cell(_in, _name);

    // an unset or empty special falls back to its documented default
    if (!cell)
    {
        return _fallback;
    }

    // an explicitly assigned empty string is a value, not an absence
    if (d_awk_value_tag_of(cell) == D_AWK_VAL_UNINIT)
    {
        return _fallback;
    }

    return d_awk_value_text(cell, NULL, NULL);
}


//==============================================================================
// 3.  RECORDS AND FIELDS
//==============================================================================
// The two validity flags are the whole of the lazy discipline.  Reading a
// field or NF forces a split; writing a field invalidates $0; writing $0
// invalidates the fields.  Nothing else may touch them.


/*
d_internal_set_record
  Replaces $0 and invalidates the fields.

Parameter(s):
  _in:     the interpreter.
  _text:   the record bytes.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if the record was stored, or
  - false, otherwise.
*/
static bool
d_internal_set_record(
    struct d_awk_interp* _in,
    const char*          _text,
    size_t               _length
)
{
    // grow the buffer when the record and its terminator do not fit
    if (_in->record_capacity < (_length + 1u))
    {
        char* grown = realloc(_in->record, _length + 1u);

        // report the failure rather than truncating the record
        if (!grown)
        {
            return d_internal_fail(_in, "out of memory");
        }

        _in->record          = grown;
        _in->record_capacity = _length + 1u;
    }

    // a zero-length record still needs a terminated buffer
    if (_length > 0)
    {
        memcpy(_in->record, _text, _length);
    }

    _in->record[_length] = '\0';
    _in->record_length   = _length;
    _in->record_valid    = true;
    _in->fields_valid    = false;

    // A record is divided by the FS in force when it ARRIVED, not by whatever
    // FS holds when the division is finally demanded.  Lazy splitting makes
    // that distinction observable, so the separator is captured here:
    //   FS=":"; $0="a:b:c"; FS="-"; print $1   ->   a
    // Reading FS at split time would print the whole record instead.
    const char* const separator = d_internal_special(_in, "FS", " ");
    const size_t      span      = strlen(separator);

    // grow the snapshot when the separator and its terminator do not fit
    if (_in->split_fs_capacity < (span + 1u))
    {
        char* grown = realloc(_in->split_fs, span + 1u);

        // a snapshot that cannot be held falls back to the live FS
        if (!grown)
        {
            return true;
        }

        _in->split_fs          = grown;
        _in->split_fs_capacity = span + 1u;
    }

    memcpy(_in->split_fs, separator, span + 1u);

    return true;
}


/*
d_internal_push_field
  Appends one field to the field vector.

Parameter(s):
  _in:     the interpreter.
  _text:   the field bytes.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if the field was appended, or
  - false, otherwise.
*/
static bool
d_internal_push_field(
    struct d_awk_interp* _in,
    const char*          _text,
    size_t               _length
)
{
    // grow the vector when it is full
    if (_in->field_count == _in->field_capacity)
    {
        const size_t capacity = (_in->field_capacity == 0)
                              ? 16u
                              : (_in->field_capacity * 2u);

        struct d_awk_value** grown =
            realloc(_in->fields, capacity * sizeof(*grown));

        // report the failure rather than dropping the field
        if (!grown)
        {
            return d_internal_fail(_in, "out of memory");
        }

        // a freshly grown slot holds no cell until one is needed
        for (size_t at = _in->field_capacity; at < capacity; ++at)
        {
            grown[at] = NULL;
        }

        _in->fields         = grown;
        _in->field_capacity = capacity;
    }

    // a slot reuses its cell across records rather than reallocating
    if (!_in->fields[_in->field_count])
    {
        _in->fields[_in->field_count] = d_awk_value_new();

        // report the failure rather than leaving an empty slot
        if (!_in->fields[_in->field_count])
        {
            return d_internal_fail(_in, "out of memory");
        }
    }

    // a field is input-derived, so the numeric string rule applies
    if (!d_awk_value_set_input(_in->fields[_in->field_count],
                               _text,
                               _length))
    {
        return d_internal_fail(_in, "out of memory");
    }

    _in->field_count++;

    return true;
}


/*
d_internal_split_into
  Divides text by a separator, appending each piece through a callback.
NOTE:
  The three POSIX rules are all here.  A separator of a single blank splits on
  runs of blanks and tabs and discards leading and trailing ones; any other
  single character splits on that byte literally; anything longer is an
  extended regular expression.

Parameter(s):
  _in:        the interpreter.
  _text:      the bytes to divide.
  _length:    the number of bytes.
  _separator: the separator text.
  _append:    receives each piece.
  _user:      passed through to the callback.
Return:
  A boolean value corresponding to either:
  - true, if the text was divided, or
  - false, otherwise.
*/
static bool
d_internal_split_into(
    struct d_awk_interp* _in,
    const char*          _text,
    size_t               _length,
    const char*          _separator,
    bool               (*_append)(struct d_awk_interp*,
                                  void*,
                                  const char*,
                                  size_t),
    void*                _user
)
{
    const size_t span = strlen(_separator);

    // the default separator splits on runs of blanks and trims the ends
    if ((span == 1u) && (_separator[0] == ' '))
    {
        size_t at = 0;

        while (at < _length)
        {
            // skip the run of blanks before the next field
            while ( (at < _length) &&
                    ((_text[at] == ' ')  || (_text[at] == '\t') ||
                     (_text[at] == '\n')) )
            {
                at++;
            }

            // a trailing run of blanks yields no field
            if (at >= _length)
            {
                break;
            }

            const size_t start = at;

            while ( (at < _length) &&
                    (_text[at] != ' ')  && (_text[at] != '\t') &&
                    (_text[at] != '\n') )
            {
                at++;
            }

            // abandon the split when the piece could not be appended
            if (!_append(_in, _user, &_text[start], at - start))
            {
                return false;
            }
        }

        return true;
    }

    // an empty record yields no fields whatever the separator
    if (_length == 0)
    {
        return true;
    }

    // An empty separator splits into individual characters.  POSIX leaves
    // this undefined; gawk and one-true-awk both do it, and T.split tests it
    // four ways.  Without it the regex path below matches empty at offset
    // zero, makes no progress, and yields the whole string as one field.
    if (span == 0)
    {
        for (size_t at = 0; at < _length; ++at)
        {
            // abandon the split when a character could not be appended
            if (!_append(_in, _user, &_text[at], 1u))
            {
                return false;
            }
        }

        return true;
    }

    // a single character other than blank separates literally
    if (span == 1u)
    {
        size_t start = 0;

        for (size_t at = 0; at <= _length; ++at)
        {
            // the end of the text closes the final field
            if ((at == _length) || (_text[at] == _separator[0]))
            {
                // abandon the split when the piece could not be appended
                if (!_append(_in, _user, &_text[start], at - start))
                {
                    return false;
                }

                start = at + 1u;
            }
        }

        return true;
    }

    enum d_regex_status status = D_REGEX_OK;
    struct d_regex*     regex  = d_regex_compile(_separator, &status);

    // a separator that will not compile is a runtime error
    if (!regex)
    {
        return d_internal_fail(_in, d_regex_status_text(status));
    }

    size_t start = 0;
    size_t from  = 0;

    // each match of the separator closes the field before it
    while (from <= _length)
    {
        struct d_regex_match match = { 0, 0 };

        // no further separator leaves the remainder as the last field
        if (!d_regex_search(regex, _text, _length, from, &match))
        {
            break;
        }

        // an empty match would not advance, so it ends the scan
        if (match.length == 0)
        {
            break;
        }

        // abandon the split when the piece could not be appended
        if (!_append(_in, _user, &_text[start], match.start - start))
        {
            d_regex_free(regex);
            return false;
        }

        start = match.start + match.length;
        from  = start;
    }

    const bool ok = _append(_in, _user, &_text[start], _length - start);

    d_regex_free(regex);

    return ok;
}


/*
d_internal_field_sink
  Appends one piece to the interpreter's field vector.

Parameter(s):
  _in:     the interpreter.
  _user:   unused.
  _text:   the piece bytes.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if the field was appended, or
  - false, otherwise.
*/
static bool
d_internal_field_sink(
    struct d_awk_interp* _in,
    void*                _user,
    const char*          _text,
    size_t               _length
)
{
    (void)_user;

    return d_internal_push_field(_in, _text, _length);
}


/*
d_internal_ensure_fields
  Divides the record into fields if that has not already happened.

Parameter(s):
  _in: the interpreter.
Return:
  A boolean value corresponding to either:
  - true, if the fields are available, or
  - false, otherwise.
*/
static bool
d_internal_ensure_fields(
    struct d_awk_interp* _in
)
{
    // the split is performed once per record, on first demand
    if (_in->fields_valid)
    {
        return true;
    }

    _in->field_count  = 0;
    _in->fields_valid = true;

    // the separator captured when the record arrived, not the live one
    const char* const separator = _in->split_fs
                                ? _in->split_fs
                                : d_internal_special(_in, "FS", " ");

    return d_internal_split_into(_in,
                                 _in->record,
                                 _in->record_length,
                                 separator,
                                 d_internal_field_sink,
                                 NULL);
}


/*
d_internal_ensure_record
  Rebuilds $0 from the fields if a field has been written since.

Parameter(s):
  _in: the interpreter.
Return:
  A boolean value corresponding to either:
  - true, if $0 is current, or
  - false, otherwise.
*/
static bool
d_internal_ensure_record(
    struct d_awk_interp* _in
)
{
    // the rebuild happens once, on first demand after a field assignment
    if (_in->record_valid)
    {
        return true;
    }

    const char* const ofs    = d_internal_special(_in, "OFS", " ");
    const size_t      span   = strlen(ofs);
    size_t            needed = 0;

    for (size_t at = 0; at < _in->field_count; ++at)
    {
        size_t length = 0;

        (void)d_awk_value_text(_in->fields[at], NULL, &length);
        needed += length + ((at > 0) ? span : 0u);
    }

    char* buffer = malloc(needed + 1u);

    // report the failure rather than leaving a stale record
    if (!buffer)
    {
        return d_internal_fail(_in, "out of memory");
    }

    size_t used = 0;

    for (size_t at = 0; at < _in->field_count; ++at)
    {
        // the separator joins fields rather than leading them
        if (at > 0)
        {
            memcpy(&buffer[used], ofs, span);
            used += span;
        }

        size_t            length = 0;
        const char* const text   = d_awk_value_text(_in->fields[at],
                                                    NULL,
                                                    &length);

        memcpy(&buffer[used], text, length);
        used += length;
    }

    buffer[used] = '\0';

    const bool ok = d_internal_set_record(_in, buffer, used);

    free(buffer);

    // the rebuild must not itself invalidate the fields it came from
    _in->fields_valid = true;

    return ok;
}


/*
d_internal_field_count
  Returns NF, forcing a split if needed.

Parameter(s):
  _in: the interpreter.
Return:
  The number of fields.
*/
static size_t
d_internal_field_count(
    struct d_awk_interp* _in
)
{
    (void)d_internal_ensure_fields(_in);

    return _in->field_count;
}


/*
d_internal_set_field
  Assigns one field, extending the record with empty fields if needed.
NOTE:
  Assigning to $0 replaces the record and forces a fresh split; assigning to a
  field beyond NF creates the intervening empty fields, which is why merely
  *referencing* such a field must not come through here.

Parameter(s):
  _in:    the interpreter.
  _index: the field number; zero means the whole record.
  _value: the value to assign.
Return:
  A boolean value corresponding to either:
  - true, if the assignment succeeded, or
  - false, otherwise.
*/
static bool
d_internal_set_field(
    struct d_awk_interp* _in,
    size_t               _index,
    struct d_awk_value*  _value
)
{
    size_t            length = 0;
    const char* const text   = d_awk_value_text(
                                   _value,
                                   d_internal_special(_in, "CONVFMT", "%.6g"),
                                   &length);

    // assigning the whole record discards the previous division
    if (_index == 0)
    {
        return d_internal_set_record(_in, text, length);
    }

    // abandon the assignment when the record could not be divided
    if (!d_internal_ensure_fields(_in))
    {
        return false;
    }

    // a field past the end brings the intervening empty ones into being
    while (_in->field_count < _index)
    {
        // abandon the assignment when a filler field could not be held
        if (!d_internal_push_field(_in, "", 0))
        {
            return false;
        }
    }

    // abandon the assignment when the field could not be held
    if (!d_awk_value_set_input(_in->fields[_index - 1u], text, length))
    {
        return d_internal_fail(_in, "out of memory");
    }

    _in->record_valid = false;

    return true;
}


//==============================================================================
// 4.  EXPRESSIONS
//==============================================================================


/*
d_internal_regex_of
  Returns the compiled pattern for a match operand.
NOTE:
  A literal is compiled once when the tree is built.  A dynamic pattern is a
  string computed at run time and must be compiled here, so the caller is told
  whether to release the result.

Parameter(s):
  _in:        the interpreter.
  _node:      the operand.
  _out_owned: receives whether the caller must free the pattern.
Return:
  The pattern, or NULL on failure.
*/
static struct d_regex*
d_internal_regex_of(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    bool*                _out_owned
)
{
    *_out_owned = false;

    // a literal carries its pattern, compiled when the program was parsed
    if (_node->kind == D_AWK_N_REGEX)
    {
        return _node->regex;
    }

    struct d_awk_value* const scratch = d_internal_acquire(_in);

    // abandon the match when the scratch cell could not be held
    if (!scratch)
    {
        (void)d_internal_fail(_in, "out of memory");
        return NULL;
    }

    // abandon the match when the pattern text failed to evaluate
    if (!d_internal_eval(_in, _node, scratch))
    {
        d_internal_release(_in, scratch);
        return NULL;
    }

    size_t            length = 0;
    const char* const text   = d_awk_value_text(scratch, NULL, &length);

    enum d_regex_status status = D_REGEX_OK;
    struct d_regex*     regex  = d_regex_compile(text, &status);

    d_internal_release(_in, scratch);

    // a dynamic pattern that will not compile is a runtime error
    if (!regex)
    {
        (void)d_internal_fail(_in, d_regex_status_text(status));
        return NULL;
    }

    *_out_owned = true;

    return regex;
}


/*
d_internal_truth
  Evaluates a node and reports its truth.

Parameter(s):
  _in:   the interpreter.
  _node: the node to evaluate.
Return:
  A boolean value corresponding to either:
  - true, if the value is true in awk's sense, or
  - false, otherwise or on failure.
*/
static bool
d_internal_truth(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node
)
{
    struct d_awk_value* const scratch = d_internal_acquire(_in);

    // abandon the test when the scratch cell could not be held
    if (!scratch)
    {
        return d_internal_fail(_in, "out of memory");
    }

    const bool ok    = d_internal_eval(_in, _node, scratch);
    const bool truth = ok ? d_awk_value_is_true(scratch) : false;

    d_internal_release(_in, scratch);

    return truth;
}


/*
d_internal_subscript
  Joins a node's subscript parts into a single key.

Parameter(s):
  _in:         the interpreter.
  _node:       the node whose list holds the parts.
  _buffer:     receives the key; owned by the caller on success.
  _out_length: receives the key length.
Return:
  A boolean value corresponding to either:
  - true, if the key was built, or
  - false, otherwise.
*/
static bool
d_internal_subscript(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    char**               _buffer,
    size_t*              _out_length
)
{
    const char* parts[16];
    size_t      lengths[16];
    struct d_awk_value* cells[16];
    size_t      count = (_node->count < 16u) ? _node->count : 16u;

    for (size_t at = 0; at < count; ++at)
    {
        cells[at] = d_internal_acquire(_in);

        // abandon the subscript when a part could not be evaluated
        if ((!cells[at]) || (!d_internal_eval(_in, _node->list[at], cells[at])))
        {
            for (size_t back = 0; back <= at; ++back)
            {
                d_internal_release(_in, cells[back]);
            }

            return false;
        }

        parts[at] = d_awk_value_text(
                        cells[at],
                        d_internal_special(_in, "CONVFMT", "%.6g"),
                        &lengths[at]);
    }

    const char* const subsep = d_internal_special(_in, "SUBSEP", "\034");
    const size_t      needed = d_awk_subscript_join(NULL,
                                                    0,
                                                    parts,
                                                    lengths,
                                                    count,
                                                    subsep);

    *_buffer = malloc(needed + 1u);

    // report the failure rather than returning a partial key
    if (!*_buffer)
    {
        for (size_t at = 0; at < count; ++at)
        {
            d_internal_release(_in, cells[at]);
        }

        return d_internal_fail(_in, "out of memory");
    }

    (void)d_awk_subscript_join(*_buffer,
                               needed + 1u,
                               parts,
                               lengths,
                               count,
                               subsep);
    *_out_length = needed;

    for (size_t at = 0; at < count; ++at)
    {
        d_internal_release(_in, cells[at]);
    }

    return true;
}


/*
d_internal_assign_to
  Stores a value into whatever a target node denotes.

Parameter(s):
  _in:     the interpreter.
  _target: the variable, field or subscript to assign to.
  _value:  the value to store.
Return:
  A boolean value corresponding to either:
  - true, if the assignment succeeded, or
  - false, otherwise.
*/
static bool
d_internal_assign_to(
    struct d_awk_interp* _in,
    struct d_awk_node*   _target,
    struct d_awk_value*  _value
)
{
    // a plain name assigns to its cell, which the container owns
    if (_target->kind == D_AWK_N_VAR)
    {
        // NF is computed rather than stored, and writing it resizes
        if (strcmp(_target->text, "NF") == 0)
        {
            // abandon the assignment when the record could not be divided
            if (!d_internal_ensure_fields(_in))
            {
                return false;
            }

            const double wanted = d_awk_value_number(_value);
            const size_t count  = (wanted < 0.0) ? 0u : (size_t)wanted;

            // shrinking discards fields; growing appends empty ones
            while (_in->field_count > count)
            {
                _in->field_count--;
            }

            while (_in->field_count < count)
            {
                // abandon the assignment when a field could not be held
                if (!d_internal_push_field(_in, "", 0))
                {
                    return false;
                }
            }

            _in->record_valid = false;

            return true;
        }

        struct d_awk_value* const cell = d_internal_cell(_in, _target->text);

        // abandon the assignment when the cell could not be resolved
        if (!cell)
        {
            return d_internal_fail(_in, "out of memory");
        }

        // the container owns the cell, so assignment copies into it
        return d_awk_value_copy(cell, _value)
             ? true
             : d_internal_fail(_in, "out of memory");
    }

    // a field assigns through the record machinery
    if (_target->kind == D_AWK_N_FIELD)
    {
        struct d_awk_value* const index = d_internal_acquire(_in);

        // abandon the assignment when the index could not be evaluated
        if ((!index) || (!d_internal_eval(_in, _target->a, index)))
        {
            d_internal_release(_in, index);
            return false;
        }

        const double number = d_awk_value_number(index);

        d_internal_release(_in, index);

        // a negative field number has no meaning
        if (number < 0.0)
        {
            return d_internal_fail(_in, "attempt to access field -1");
        }

        return d_internal_set_field(_in, (size_t)number, _value);
    }

    char*  key    = NULL;
    size_t length = 0;

    // abandon the assignment when the subscript could not be built
    if (!d_internal_subscript(_in, _target, &key, &length))
    {
        return false;
    }

    struct d_awk_array* const array = d_internal_array(_in,
                                                       _target->a->text);
    struct d_awk_value* const cell  = array
                                    ? d_awk_array_lookup(array, key, length)
                                    : NULL;

    free(key);

    // abandon the assignment when the element could not be created
    if (!cell)
    {
        return d_internal_fail(_in, "out of memory");
    }

    return d_awk_value_copy(cell, _value)
         ? true
         : d_internal_fail(_in, "out of memory");
}


//==============================================================================
// 5.  FORMATTED OUTPUT
//==============================================================================


/*
d_internal_format
  Renders a printf-style format with awk's conversions.
NOTE:
  Each conversion is copied out into its own small format string and handed to
  snprintf, which keeps the platform's own rounding and padding rather than
  reimplementing them.  `*` takes its width or precision from the next
  argument, as in C.

Parameter(s):
  _in:         the interpreter.
  _node:       the node whose list holds the format and its arguments.
  _out:        receives the rendered text.
Return:
  A boolean value corresponding to either:
  - true, if the text was rendered, or
  - false, otherwise.
*/
static bool
d_internal_format(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    struct d_awk_value*  _out
)
{
    // a format is required
    if (_node->count == 0)
    {
        return d_internal_fail(_in, "printf: no format supplied");
    }

    struct d_awk_value* const fmt_cell = d_internal_acquire(_in);

    // abandon the rendering when the format could not be evaluated
    if ((!fmt_cell) || (!d_internal_eval(_in, _node->list[0], fmt_cell)))
    {
        d_internal_release(_in, fmt_cell);
        return false;
    }

    size_t            fmt_length = 0;
    const char* const format     = d_awk_value_text(fmt_cell,
                                                    NULL,
                                                    &fmt_length);

    char*  out      = NULL;
    size_t used     = 0;
    size_t capacity = 0;
    size_t argument = 1;
    bool   ok       = true;

    for (size_t at = 0; (at < fmt_length) && (ok); ++at)
    {
        char   piece[512];
        size_t piece_length = 0;

        // anything outside a conversion is copied through unchanged
        if (format[at] != '%')
        {
            piece[0]     = format[at];
            piece_length = 1;
        }
        else if (((at + 1u) < fmt_length) && (format[at + 1u] == '%'))
        {
            piece[0]     = '%';
            piece_length = 1;
            at++;
        }
        else
        {
            char   spec[64];
            size_t spec_length = 0;
            int    stars[2]    = { 0, 0 };
            size_t star_count  = 0;

            spec[spec_length++] = '%';
            at++;

            // flags, width and precision, with `*` taking an argument each
            while ( (at < fmt_length)                       &&
                    (spec_length < (sizeof(spec) - 8u))     &&
                    (strchr("-+ #0123456789.*", format[at]) != NULL) )
            {
                // a star consumes an argument as its numeric value
                if (format[at] == '*')
                {
                    struct d_awk_value* const cell = d_internal_acquire(_in);

                    // abandon the rendering when the width failed
                    if ( (!cell)                                  ||
                         (argument >= _node->count)               ||
                         (!d_internal_eval(_in,
                                           _node->list[argument++],
                                           cell)) )
                    {
                        d_internal_release(_in, cell);
                        ok = d_internal_fail(_in,
                                             "printf: missing width argument");
                        break;
                    }

                    // record the value and emit it into the specification
                    if (star_count < 2u)
                    {
                        stars[star_count++] = (int)d_awk_value_number(cell);
                    }

                    spec_length += (size_t)snprintf(&spec[spec_length],
                                                    sizeof(spec) - spec_length,
                                                    "%d",
                                                    stars[star_count - 1u]);
                    d_internal_release(_in, cell);
                }
                else
                {
                    spec[spec_length++] = format[at];
                }

                at++;
            }

            // abandon the rendering when a width argument was missing
            if (!ok)
            {
                break;
            }

            // a format that ends inside a conversion is an error
            if (at >= fmt_length)
            {
                ok = d_internal_fail(_in, "printf: truncated conversion");
                break;
            }

            const char conversion = format[at];

            struct d_awk_value* const cell = d_internal_acquire(_in);
            bool                      have = false;

            // every conversion but %% consumes one argument
            if (argument < _node->count)
            {
                have = d_internal_eval(_in, _node->list[argument++], cell);

                // abandon the rendering when the argument failed
                if (!have)
                {
                    d_internal_release(_in, cell);
                    ok = false;
                    break;
                }
            }

            switch (conversion)
            {
                case 'd':
                case 'i':
                {
                    spec[spec_length++] = 'l';
                    spec[spec_length++] = 'l';
                    spec[spec_length++] = 'd';
                    spec[spec_length]   = '\0';
                    piece_length = (size_t)snprintf(
                        piece,
                        sizeof(piece),
                        spec,
                        (long long)d_awk_value_number(cell));
                    break;
                }

                case 'o':
                case 'x':
                case 'X':
                case 'u':
                {
                    spec[spec_length++] = 'l';
                    spec[spec_length++] = 'l';
                    spec[spec_length++] = (conversion == 'u') ? 'u'
                                                              : conversion;
                    spec[spec_length]   = '\0';
                    piece_length = (size_t)snprintf(
                        piece,
                        sizeof(piece),
                        spec,
                        (long long)d_awk_value_number(cell));
                    break;
                }

                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                {
                    spec[spec_length++] = conversion;
                    spec[spec_length]   = '\0';
                    piece_length = (size_t)snprintf(piece,
                                                    sizeof(piece),
                                                    spec,
                                                    d_awk_value_number(cell));
                    break;
                }

                case 'c':
                {
                    size_t            length = 0;
                    const char* const text   = d_awk_value_text(cell,
                                                                NULL,
                                                                &length);

                    // a numeric argument gives a code, a string its first byte
                    const int byte =
                        (d_awk_value_tag_of(cell) == D_AWK_VAL_NUMBER)
                      ? (int)d_awk_value_number(cell)
                      : ((length > 0) ? (int)(unsigned char)text[0] : 0);

                    spec[spec_length++] = 'c';
                    spec[spec_length]   = '\0';
                    piece_length = (size_t)snprintf(piece,
                                                    sizeof(piece),
                                                    spec,
                                                    byte);
                    break;
                }

                default:
                {
                    size_t            length = 0;
                    const char* const text   = d_awk_value_text(
                        cell,
                        d_internal_special(_in, "CONVFMT", "%.6g"),
                        &length);

                    (void)length;
                    spec[spec_length++] = 's';
                    spec[spec_length]   = '\0';
                    piece_length = (size_t)snprintf(piece,
                                                    sizeof(piece),
                                                    spec,
                                                    text);
                    break;
                }
            }

            d_internal_release(_in, cell);

            // a conversion wider than the scratch buffer is clamped
            if (piece_length >= sizeof(piece))
            {
                piece_length = sizeof(piece) - 1u;
            }
        }

        // grow the output when the piece does not fit
        if ((used + piece_length + 1u) > capacity)
        {
            size_t wanted = (capacity == 0) ? 128u : (capacity * 2u);

            while (wanted < (used + piece_length + 1u))
            {
                wanted *= 2u;
            }

            char* grown = realloc(out, wanted);

            // abandon the rendering when the output could not grow
            if (!grown)
            {
                ok = d_internal_fail(_in, "out of memory");
                break;
            }

            out      = grown;
            capacity = wanted;
        }

        memcpy(&out[used], piece, piece_length);
        used += piece_length;
    }

    d_internal_release(_in, fmt_cell);

    // report the rendering only when every conversion succeeded
    if (ok)
    {
        ok = d_awk_value_set_string(_out, out ? out : "", used);
    }

    free(out);

    return ok;
}


//==============================================================================
// 6.  BUILTIN FUNCTIONS
//==============================================================================


/*
d_internal_substitute
  Implements sub and gsub.

Parameter(s):
  _in:    the interpreter.
  _node:  the call node.
  _all:   true for gsub, false for sub.
  _out:   receives the number of substitutions made.
Return:
  A boolean value corresponding to either:
  - true, if the substitution ran, or
  - false, otherwise.
*/
static bool
d_internal_substitute(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    bool                 _all,
    struct d_awk_value*  _out
)
{
    // the pattern and the replacement are both required
    if (_node->count < 2u)
    {
        return d_internal_fail(_in, "sub: too few arguments");
    }

    bool                  owned = false;
    struct d_regex* const regex = d_internal_regex_of(_in,
                                                      _node->list[0],
                                                      &owned);

    // abandon the substitution when the pattern failed
    if (!regex)
    {
        return false;
    }

    struct d_awk_node* const target = (_node->count > 2u)
                                    ? _node->list[2]
                                    : NULL;

    struct d_awk_value* const repl_cell = d_internal_acquire(_in);
    struct d_awk_value* const subject   = d_internal_acquire(_in);
    bool                      ok        = ((repl_cell) && (subject));

    // the subject defaults to the whole record
    if (ok)
    {
        ok = d_internal_eval(_in, _node->list[1], repl_cell);
    }

    if (ok)
    {
        // abandon the substitution when the record could not be rebuilt
        if (target)
        {
            ok = d_internal_eval(_in, target, subject);
        }
        else
        {
            ok = d_internal_ensure_record(_in) &&
                 d_awk_value_set_input(subject,
                                       _in->record,
                                       _in->record_length);
        }
    }

    size_t            repl_length = 0;
    const char* const replacement = ok
                                  ? d_awk_value_text(repl_cell,
                                                     NULL,
                                                     &repl_length)
                                  : "";
    size_t            text_length = 0;
    const char* const text        = ok
                                  ? d_awk_value_text(
                                        subject,
                                        d_internal_special(_in,
                                                           "CONVFMT",
                                                           "%.6g"),
                                        &text_length)
                                  : "";

    char*  out      = NULL;
    size_t used     = 0;
    size_t capacity = 0;
    size_t from     = 0;
    long   made     = 0;

    // each match is replaced, with `&` standing for the matched text
    while (ok)
    {
        struct d_regex_match match = { 0, 0 };

        // no further match leaves the remainder to be copied
        if (!d_regex_search(regex, text, text_length, from, &match))
        {
            break;
        }

        const size_t extra = (match.start - from) + (repl_length * 2u) + 2u;

        // grow the output when the piece does not fit
        if ((used + extra + 1u) > capacity)
        {
            size_t wanted = (capacity == 0) ? 128u : (capacity * 2u);

            while (wanted < (used + extra + 1u))
            {
                wanted *= 2u;
            }

            char* grown = realloc(out, wanted);

            // abandon the substitution when the output could not grow
            if (!grown)
            {
                ok = d_internal_fail(_in, "out of memory");
                break;
            }

            out      = grown;
            capacity = wanted;
        }

        memcpy(&out[used], &text[from], match.start - from);
        used += match.start - from;

        // the replacement expands `&` and honours a backslash before it
        for (size_t at = 0; at < repl_length; ++at)
        {
            if ((replacement[at] == '\\') && ((at + 1u) < repl_length) &&
                ((replacement[at + 1u] == '&') ||
                 (replacement[at + 1u] == '\\')))
            {
                out[used++] = replacement[++at];
            }
            else if (replacement[at] == '&')
            {
                memcpy(&out[used], &text[match.start], match.length);
                used += match.length;
            }
            else
            {
                out[used++] = replacement[at];
            }
        }

        made++;

        // an empty match must advance the scan or it would not terminate
        if (match.length == 0)
        {
            // the byte at the empty match is carried across unchanged
            if (match.start < text_length)
            {
                out[used++] = text[match.start];
            }

            from = match.start + 1u;
        }
        else
        {
            from = match.start + match.length;
        }

        // one substitution is all sub performs
        if (!_all)
        {
            break;
        }

        // a scan past the end is complete
        if (from > text_length)
        {
            break;
        }
    }

    // the tail after the last match is copied through
    if ((ok) && (made > 0))
    {
        const size_t tail = (from <= text_length) ? (text_length - from) : 0u;

        // grow the output when the tail does not fit
        if ((used + tail + 1u) > capacity)
        {
            char* grown = realloc(out, used + tail + 1u);

            // abandon the substitution when the output could not grow
            if (!grown)
            {
                ok = d_internal_fail(_in, "out of memory");
            }
            else
            {
                out      = grown;
                capacity = used + tail + 1u;
            }
        }

        if (ok)
        {
            memcpy(&out[used], &text[from], tail);
            used += tail;

            struct d_awk_value* const result = d_internal_acquire(_in);

            ok = (result) &&
                 d_awk_value_set_string(result, out, used);

            // the result is written back to whatever the subject denoted
            if (ok)
            {
                ok = target
                   ? d_internal_assign_to(_in, target, result)
                   : d_internal_set_field(_in, 0, result);
            }

            d_internal_release(_in, result);
        }
    }

    d_awk_value_set_number(_out, (double)made);

    free(out);
    d_internal_release(_in, repl_cell);
    d_internal_release(_in, subject);

    // a dynamic pattern belongs to this call alone
    if (owned)
    {
        d_regex_free(regex);
    }

    return ok;
}


/*
d_internal_split_sink
  Appends one piece to the array a split is filling.

Parameter(s):
  _in:     the interpreter.
  _user:   the array and the running index.
  _text:   the piece bytes.
  _length: the number of bytes.
Return:
  A boolean value corresponding to either:
  - true, if the element was stored, or
  - false, otherwise.
*/
struct d_internal_split_target
{
    struct d_awk_array* array;
    size_t              index;
};

static bool
d_internal_split_sink(
    struct d_awk_interp* _in,
    void*                _user,
    const char*          _text,
    size_t               _length
)
{
    struct d_internal_split_target* const target = _user;

    char   key[32];
    const int written = snprintf(key,
                                 sizeof(key),
                                 "%zu",
                                 ++target->index);

    struct d_awk_value* const cell = d_awk_array_lookup(target->array,
                                                        key,
                                                        (size_t)written);

    // abandon the split when the element could not be created
    if (!cell)
    {
        return d_internal_fail(_in, "out of memory");
    }

    // a split piece is input-derived, so the numeric string rule applies
    return d_awk_value_set_input(cell, _text, _length)
         ? true
         : d_internal_fail(_in, "out of memory");
}


/*
d_internal_builtin
  Evaluates a call to a builtin function.

Parameter(s):
  _in:   the interpreter.
  _node: the call node.
  _out:  receives the result.
Return:
  A boolean value corresponding to either:
  - true, if the call succeeded, or
  - false, otherwise.
*/
static bool
d_internal_builtin(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    struct d_awk_value*  _out
)
{
    const char* const name = _node->text;

    // the substitutions write back through their subject
    if ((strcmp(name, "sub") == 0) || (strcmp(name, "gsub") == 0))
    {
        return d_internal_substitute(_in,
                                     _node,
                                     (strcmp(name, "gsub") == 0),
                                     _out);
    }

    // split fills an array, so its second argument is not an ordinary value
    if (strcmp(name, "split") == 0)
    {
        // the text and the array are both required
        if (_node->count < 2u)
        {
            return d_internal_fail(_in, "split: too few arguments");
        }

        struct d_awk_value* const text_cell = d_internal_acquire(_in);
        struct d_awk_value* const sep_cell  = d_internal_acquire(_in);
        bool                      ok        = ((text_cell) && (sep_cell));

        if (ok)
        {
            ok = d_internal_eval(_in, _node->list[0], text_cell);
        }

        const char* separator = d_internal_special(_in, "FS", " ");
        char        sep_copy[256];

        // a third argument overrides FS for this call only
        if ((ok) && (_node->count > 2u))
        {
            // a literal pattern is used as its own text
            if (_node->list[2]->kind == D_AWK_N_REGEX)
            {
                snprintf(sep_copy,
                         sizeof(sep_copy),
                         "%s",
                         _node->list[2]->text);
            }
            else
            {
                ok = d_internal_eval(_in, _node->list[2], sep_cell);
                snprintf(sep_copy,
                         sizeof(sep_copy),
                         "%s",
                         d_awk_value_text(sep_cell, NULL, NULL));
            }

            separator = sep_copy;
        }

        struct d_internal_split_target target = { NULL, 0 };

        if (ok)
        {
            target.array = d_internal_array(_in, _node->list[1]->text);
            ok           = (target.array != NULL);
        }

        if (ok)
        {
            size_t            length = 0;
            const char* const text   = d_awk_value_text(text_cell,
                                                        NULL,
                                                        &length);

            d_awk_array_clear(target.array);
            ok = d_internal_split_into(_in,
                                       text,
                                       length,
                                       separator,
                                       d_internal_split_sink,
                                       &target);
        }

        d_awk_value_set_number(_out, (double)target.index);
        d_internal_release(_in, text_cell);
        d_internal_release(_in, sep_cell);

        return ok;
    }

    // match sets RSTART and RLENGTH as well as returning the offset
    if (strcmp(name, "match") == 0)
    {
        // the subject and the pattern are both required
        if (_node->count < 2u)
        {
            return d_internal_fail(_in, "match: too few arguments");
        }

        struct d_awk_value* const subject = d_internal_acquire(_in);
        bool                      ok      = (subject != NULL);

        if (ok)
        {
            ok = d_internal_eval(_in, _node->list[0], subject);
        }

        bool                  owned = false;
        struct d_regex* const regex = ok
                                    ? d_internal_regex_of(_in,
                                                          _node->list[1],
                                                          &owned)
                                    : NULL;

        if (regex)
        {
            size_t               length = 0;
            const char* const    text   = d_awk_value_text(subject,
                                                           NULL,
                                                           &length);
            struct d_regex_match match  = { 0, 0 };
            const bool           found  = d_regex_search(regex,
                                                         text,
                                                         length,
                                                         0,
                                                         &match);

            d_awk_value_set_number(_out,
                                   found ? (double)(match.start + 1u) : 0.0);
            d_awk_value_set_number(d_internal_cell(_in, "RSTART"),
                                   found ? (double)(match.start + 1u) : 0.0);
            d_awk_value_set_number(d_internal_cell(_in, "RLENGTH"),
                                   found ? (double)match.length : -1.0);

            // a dynamic pattern belongs to this call alone
            if (owned)
            {
                d_regex_free(regex);
            }
        }
        else
        {
            ok = false;
        }

        d_internal_release(_in, subject);

        return ok;
    }

    // sprintf shares the whole of printf's rendering
    if (strcmp(name, "sprintf") == 0)
    {
        return d_internal_format(_in, _node, _out);
    }

    // length with no argument measures the record, not a value
    if ((strcmp(name, "length") == 0) && (_node->count == 0))
    {
        // abandon the call when the record could not be rebuilt
        if (!d_internal_ensure_record(_in))
        {
            return false;
        }

        d_awk_value_set_number(_out, (double)_in->record_length);

        return true;
    }

    // length of a bare name that denotes an array counts its elements
    if ( (strcmp(name, "length") == 0)                    &&
         (_node->count == 1u)                             &&
         (_node->list[0]->kind == D_AWK_N_VAR) )
    {
        size_t                   at    = 0;
        struct d_internal_frame* frame = d_internal_local(_in,
                                                          _node->list[0]->text,
                                                          &at);
        struct d_awk_value* cell = frame
                                 ? NULL
                                 : d_awk_array_find(_in->globals,
                                                    _node->list[0]->text,
                                                    strlen(
                                                        _node->list[0]->text));

        // only a name already holding an array is counted
        if ((frame) && (frame->arrays[at]))
        {
            d_awk_value_set_number(_out,
                                   (double)d_awk_array_count(
                                       frame->arrays[at]));
            return true;
        }

        if ((cell) && (d_awk_value_tag_of(cell) == D_AWK_VAL_ARRAY))
        {
            d_awk_value_set_number(_out,
                                   (double)d_awk_array_count(
                                       d_awk_value_array_of(cell)));
            return true;
        }
    }

    // every remaining builtin takes ordinary values
    struct d_awk_value* args[4] = { NULL, NULL, NULL, NULL };
    const size_t        count   = (_node->count < 4u) ? _node->count : 4u;
    bool                ok      = true;

    for (size_t at = 0; (at < count) && (ok); ++at)
    {
        args[at] = d_internal_acquire(_in);
        ok       = (args[at]) && d_internal_eval(_in, _node->list[at],
                                                 args[at]);
    }

    // abandon the call when an argument failed
    if (!ok)
    {
        for (size_t at = 0; at < count; ++at)
        {
            d_internal_release(_in, args[at]);
        }

        return false;
    }

    const double first  = (count > 0) ? d_awk_value_number(args[0]) : 0.0;
    const double second = (count > 1u) ? d_awk_value_number(args[1]) : 0.0;

    if (strcmp(name, "length") == 0)
    {
        size_t length = 0;

        (void)d_awk_value_text(args[0],
                               d_internal_special(_in, "CONVFMT", "%.6g"),
                               &length);
        d_awk_value_set_number(_out, (double)length);
    }
    else if (strcmp(name, "substr") == 0)
    {
        size_t            length = 0;
        const char* const text   = d_awk_value_text(
                                       args[0],
                                       d_internal_special(_in,
                                                          "CONVFMT",
                                                          "%.6g"),
                                       &length);

        // POSIX counts the characters at positions m through m+n-1, so a
        // start before the string shortens the result.  mawk instead keeps
        // n characters from the clamped start; gawk and the standard agree
        // with the reading taken here.
        const double start = (count > 1u) ? second : 1.0;

        double from = start;

        // an omitted length runs to the end of the string, which is not the
        // same as a length of len+1 once the start is negative
        double to = (count > 2u)
                  ? (start + d_awk_value_number(args[2]))
                  : ((double)length + 1.0);

        // clamp both ends into the string rather than indexing outside it
        if (from < 1.0)
        {
            from = 1.0;
        }

        if (to > ((double)length + 1.0))
        {
            to = (double)length + 1.0;
        }

        const size_t begin = (size_t)from;
        const size_t end   = (to > from) ? (size_t)to : begin;

        (void)d_awk_value_set_string(_out,
                                     &text[begin - 1u],
                                     end - begin);
    }
    else if (strcmp(name, "index") == 0)
    {
        size_t            hay_length = 0;
        size_t            pin_length = 0;
        const char* const hay        = d_awk_value_text(args[0],
                                                        NULL,
                                                        &hay_length);
        const char* const pin        = d_awk_value_text(args[1],
                                                        NULL,
                                                        &pin_length);
        double            found      = 0.0;

        // an empty needle is found at the first position
        for (size_t at = 0; (pin_length <= hay_length) &&
                            (at + pin_length <= hay_length); ++at)
        {
            if (memcmp(&hay[at], pin, pin_length) == 0)
            {
                found = (double)(at + 1u);
                break;
            }
        }

        d_awk_value_set_number(_out, found);
    }
    else if (strcmp(name, "toupper") == 0)
    {
        size_t            length = 0;
        const char* const text   = d_awk_value_text(args[0], NULL, &length);
        char*             copy   = malloc(length + 1u);

        ok = (copy != NULL);

        if (ok)
        {
            for (size_t at = 0; at < length; ++at)
            {
                copy[at] = ((text[at] >= 'a') && (text[at] <= 'z'))
                         ? (char)(text[at] - 'a' + 'A')
                         : text[at];
            }

            ok = d_awk_value_set_string(_out, copy, length);
            free(copy);
        }
    }
    else if (strcmp(name, "tolower") == 0)
    {
        size_t            length = 0;
        const char* const text   = d_awk_value_text(args[0], NULL, &length);
        char*             copy   = malloc(length + 1u);

        ok = (copy != NULL);

        if (ok)
        {
            for (size_t at = 0; at < length; ++at)
            {
                copy[at] = ((text[at] >= 'A') && (text[at] <= 'Z'))
                         ? (char)(text[at] - 'A' + 'a')
                         : text[at];
            }

            ok = d_awk_value_set_string(_out, copy, length);
            free(copy);
        }
    }
    else if (strcmp(name, "int") == 0)
    {
        d_awk_value_set_number(_out,
                               (first < 0.0) ? ceil(first) : floor(first));
    }
    else if (strcmp(name, "sqrt") == 0)
    {
        d_awk_value_set_number(_out, sqrt(first));
    }
    else if (strcmp(name, "exp") == 0)
    {
        d_awk_value_set_number(_out, exp(first));
    }
    else if (strcmp(name, "log") == 0)
    {
        d_awk_value_set_number(_out, log(first));
    }
    else if (strcmp(name, "sin") == 0)
    {
        d_awk_value_set_number(_out, sin(first));
    }
    else if (strcmp(name, "cos") == 0)
    {
        d_awk_value_set_number(_out, cos(first));
    }
    else if (strcmp(name, "atan2") == 0)
    {
        d_awk_value_set_number(_out, atan2(first, second));
    }
    else if (strcmp(name, "rand") == 0)
    {
        d_awk_value_set_number(_out,
                               (double)rand() / ((double)RAND_MAX + 1.0));
    }
    else if (strcmp(name, "srand") == 0)
    {
        const unsigned previous = _in->seed;

        _in->seed = (count > 0) ? (unsigned)first : (unsigned)0;
        srand(_in->seed);

        // POSIX returns the PREVIOUS seed, which is easy to get wrong
        d_awk_value_set_number(_out, (double)previous);
    }
    else if (strcmp(name, "close") == 0)
    {
        d_awk_value_set_number(
            _out,
            (double)d_internal_stream_close(
                _in,
                d_awk_value_text(args[0], NULL, NULL)));
    }
    else if (strcmp(name, "system") == 0)
    {
        // every buffer is flushed first, so the child's output interleaves
        // with ours in the order a reader would expect
        (void)fflush(stdout);
        (void)fflush(stderr);

        const int status = system(d_awk_value_text(args[0], NULL, NULL));

        d_awk_value_set_number(_out,
                               (status < 0) ? -1.0
                                            : (double)((status >> 8) & 0xFF));
    }
    else
    {
        ok = d_internal_fail(_in, "unsupported builtin");
    }

    for (size_t at = 0; at < count; ++at)
    {
        d_internal_release(_in, args[at]);
    }

    return ok;
}


//==============================================================================
// 7.  USER FUNCTIONS
//==============================================================================


/*
d_internal_call
  Calls a user-defined function.
NOTE:
  Scalars pass by value and arrays by reference, which is awk's rule and the
  only place an array crosses a frame boundary.  A parameter past those the
  caller supplied is an uninitialised local, which is how awk declares locals
  at all.

Parameter(s):
  _in:   the interpreter.
  _node: the call node.
  _out:  receives the return value.
Return:
  A boolean value corresponding to either:
  - true, if the call completed, or
  - false, otherwise.
*/
static bool
d_internal_call(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    struct d_awk_value*  _out
)
{
    struct d_awk_function* target = NULL;

    for (size_t at = 0; at < _in->program->function_count; ++at)
    {
        if (strcmp(_in->program->functions[at].name, _node->text) == 0)
        {
            target = &_in->program->functions[at];
            break;
        }
    }

    // a call to an undefined function is a runtime error
    if (!target)
    {
        return d_internal_fail(_in, "call to an undefined function");
    }

    // a runaway recursion is reported rather than exhausting the stack
    if (_in->frame_count >= 512u)
    {
        return d_internal_fail(_in, "function call nesting too deep");
    }

    // grow the frame stack when it is full
    if (_in->frame_count == _in->frame_capacity)
    {
        const size_t capacity = (_in->frame_capacity == 0)
                              ? 16u
                              : (_in->frame_capacity * 2u);

        struct d_internal_frame* grown =
            realloc(_in->frames, capacity * sizeof(*grown));

        // abandon the call when the stack could not grow
        if (!grown)
        {
            return d_internal_fail(_in, "out of memory");
        }

        _in->frames         = grown;
        _in->frame_capacity = capacity;
    }

    const size_t count = target->param_count;

    struct d_internal_frame frame;

    memset(&frame, 0, sizeof(frame));
    frame.names      = target->params;
    frame.count      = count;
    frame.cells      = calloc(count ? count : 1u, sizeof(*frame.cells));
    frame.arrays     = calloc(count ? count : 1u, sizeof(*frame.arrays));
    frame.owns_array = calloc(count ? count : 1u, sizeof(*frame.owns_array));

    // abandon the call when the frame could not be held
    if ((!frame.cells) || (!frame.arrays) || (!frame.owns_array))
    {
        free(frame.cells);
        free(frame.arrays);
        free(frame.owns_array);

        return d_internal_fail(_in, "out of memory");
    }

    bool ok = true;

    // arguments bind in the caller's scope, before the frame is pushed
    for (size_t at = 0; (at < count) && (ok); ++at)
    {
        // a parameter past the supplied arguments is an empty local
        if (at >= _node->count)
        {
            continue;
        }

        struct d_awk_node* const argument = _node->list[at];

        // A bare name may be an array, and awk does not say which until the
        // callee uses it.  A name already holding an array binds by
        // reference; a name holding nothing yet binds BOTH ways, so the slot
        // stays untyped and whichever the callee does is right.  Only a name
        // already holding a scalar is passed purely by value.
        if (argument->kind == D_AWK_N_VAR)
        {
            size_t                   outer_at = 0;
            struct d_internal_frame* outer    = d_internal_local(
                                                    _in,
                                                    argument->text,
                                                    &outer_at);

            if ((outer) && (outer->arrays[outer_at]))
            {
                frame.arrays[at] = outer->arrays[outer_at];
                continue;
            }

            struct d_awk_value* const existing =
                outer ? outer->cells[outer_at]
                      : d_awk_array_find(_in->globals,
                                         argument->text,
                                         strlen(argument->text));

            if ( (existing) &&
                 (d_awk_value_tag_of(existing) == D_AWK_VAL_ARRAY) )
            {
                frame.arrays[at] = d_awk_value_array_of(existing);
                continue;
            }

            // a name that holds nothing yet could still become an array
            if ( (!existing) ||
                 (d_awk_value_tag_of(existing) == D_AWK_VAL_UNINIT) )
            {
                frame.arrays[at] = d_internal_array(_in, argument->text);
                frame.cells[at]  = d_awk_value_new();
                ok               = (frame.arrays[at] != NULL) &&
                                   (frame.cells[at] != NULL);
                continue;
            }
        }

        frame.cells[at] = d_awk_value_new();
        ok              = (frame.cells[at] != NULL) &&
                          d_internal_eval(_in, argument, frame.cells[at]);
    }

    // the frame becomes visible only once every argument has been evaluated
    if (ok)
    {
        _in->frames[_in->frame_count++] = frame;

        ok = d_internal_exec(_in, target->body);

        // a return unwinds only as far as this call
        if (_in->flow == D_INTERNAL_FLOW_RETURN)
        {
            _in->flow = D_INTERNAL_FLOW_NONE;

            // the returned value is copied out before the frame dies
            if (_in->retval)
            {
                ok = d_awk_value_copy(_out, _in->retval) && ok;
                d_internal_release(_in, _in->retval);
                _in->retval = NULL;
            }
        }

        _in->frame_count--;
    }

    for (size_t at = 0; at < count; ++at)
    {
        d_awk_value_free(frame.cells[at]);

        // only an array the frame created is the frame's to destroy
        if (frame.owns_array[at])
        {
            d_awk_array_free(frame.arrays[at]);
        }
    }

    free(frame.cells);
    free(frame.arrays);
    free(frame.owns_array);

    return ok;
}


//==============================================================================
// 8.  EVALUATION
//==============================================================================


static bool
d_internal_eval(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    struct d_awk_value*  _out
)
{
    // an error already raised stops further evaluation
    if (_in->flow == D_INTERNAL_FLOW_ERROR)
    {
        return false;
    }

    switch (_node->kind)
    {
        case D_AWK_N_NUMBER:
        {
            d_awk_value_set_number(_out, _node->number);
            return true;
        }

        case D_AWK_N_STRING:
        {
            // a program literal is never a numeric string
            return d_awk_value_set_string(_out, _node->text, _node->length)
                 ? true
                 : d_internal_fail(_in, "out of memory");
        }

        case D_AWK_N_REGEX:
        {
            // a bare pattern is a match against the whole record
            if (!d_internal_ensure_record(_in))
            {
                return false;
            }

            d_awk_value_set_number(_out,
                                   d_regex_test(_node->regex,
                                                _in->record,
                                                _in->record_length)
                                 ? 1.0
                                 : 0.0);
            return true;
        }

        case D_AWK_N_VAR:
        {
            // NF is computed from the fields rather than stored
            if (strcmp(_node->text, "NF") == 0)
            {
                d_awk_value_set_number(_out,
                                       (double)d_internal_field_count(_in));
                return true;
            }

            struct d_awk_value* const cell = d_internal_cell(_in,
                                                             _node->text);

            // an unset variable reads as uninitialised, not as an error
            if (!cell)
            {
                d_awk_value_set_uninit(_out);
                return true;
            }

            return d_awk_value_copy(_out, cell)
                 ? true
                 : d_internal_fail(_in, "out of memory");
        }

        case D_AWK_N_FIELD:
        {
            struct d_awk_value* const index = d_internal_acquire(_in);

            // abandon the read when the index could not be evaluated
            if ((!index) || (!d_internal_eval(_in, _node->a, index)))
            {
                d_internal_release(_in, index);
                return false;
            }

            const double number = d_awk_value_number(index);

            d_internal_release(_in, index);

            // a negative field number has no meaning
            if (number < 0.0)
            {
                return d_internal_fail(_in, "attempt to access a field < 0");
            }

            const size_t at = (size_t)number;

            // the whole record is rebuilt on demand rather than eagerly
            if (at == 0)
            {
                // abandon the read when the record could not be rebuilt
                if (!d_internal_ensure_record(_in))
                {
                    return false;
                }

                return d_awk_value_set_input(_out,
                                             _in->record,
                                             _in->record_length)
                     ? true
                     : d_internal_fail(_in, "out of memory");
            }

            // abandon the read when the record could not be divided
            if (!d_internal_ensure_fields(_in))
            {
                return false;
            }

            // a field past the end reads as empty and is NOT created
            if (at > _in->field_count)
            {
                d_awk_value_set_uninit(_out);
                return true;
            }

            return d_awk_value_copy(_out, _in->fields[at - 1u])
                 ? true
                 : d_internal_fail(_in, "out of memory");
        }

        case D_AWK_N_INDEX:
        {
            char*  key    = NULL;
            size_t length = 0;

            // abandon the read when the subscript could not be built
            if (!d_internal_subscript(_in, _node, &key, &length))
            {
                return false;
            }

            struct d_awk_array* const array = d_internal_array(_in,
                                                               _node->a->text);

            // referencing a subscript CREATES the element, as awk specifies
            struct d_awk_value* const cell = array
                                           ? d_awk_array_lookup(array,
                                                                key,
                                                                length)
                                           : NULL;

            free(key);

            // abandon the read when the element could not be created
            if (!cell)
            {
                return d_internal_fail(_in, "out of memory");
            }

            return d_awk_value_copy(_out, cell)
                 ? true
                 : d_internal_fail(_in, "out of memory");
        }

        case D_AWK_N_IN:
        {
            char*  key    = NULL;
            size_t length = 0;

            // abandon the test when the subscript could not be built
            if (!d_internal_subscript(_in, _node, &key, &length))
            {
                return false;
            }

            struct d_awk_array* const array = d_internal_array(_in,
                                                               _node->b->text);

            // membership must NOT create the element, unlike a reference
            const bool present = (array) &&
                                 (d_awk_array_find(array, key, length) != NULL);

            free(key);
            d_awk_value_set_number(_out, present ? 1.0 : 0.0);

            return true;
        }

        case D_AWK_N_ASSIGN:
        {
            struct d_awk_value* const value = d_internal_acquire(_in);

            // abandon the assignment when the value could not be evaluated
            if ((!value) || (!d_internal_eval(_in, _node->b, value)))
            {
                d_internal_release(_in, value);
                return false;
            }

            // a compound assignment folds the old value into the new one
            if (_node->op != D_AWK_TOK_ASSIGN)
            {
                struct d_awk_value* const old = d_internal_acquire(_in);

                // abandon the assignment when the target could not be read
                if ((!old) || (!d_internal_eval(_in, _node->a, old)))
                {
                    d_internal_release(_in, value);
                    d_internal_release(_in, old);
                    return false;
                }

                const double left  = d_awk_value_number(old);
                const double right = d_awk_value_number(value);
                double       result = 0.0;

                switch (_node->op)
                {
                    case D_AWK_TOK_ADD_ASSIGN: result = left + right;  break;
                    case D_AWK_TOK_SUB_ASSIGN: result = left - right;  break;
                    case D_AWK_TOK_MUL_ASSIGN: result = left * right;  break;
                    case D_AWK_TOK_POW_ASSIGN: result = pow(left, right); break;

                    case D_AWK_TOK_DIV_ASSIGN:
                    {
                        // division by zero is a runtime error in awk
                        if (right == 0.0)
                        {
                            d_internal_release(_in, value);
                            d_internal_release(_in, old);
                            return d_internal_fail(_in, "division by zero");
                        }

                        result = left / right;
                        break;
                    }

                    default:
                    {
                        // the remainder shares division's zero check
                        if (right == 0.0)
                        {
                            d_internal_release(_in, value);
                            d_internal_release(_in, old);
                            return d_internal_fail(_in,
                                                   "division by zero in %");
                        }

                        result = fmod(left, right);
                        break;
                    }
                }

                d_awk_value_set_number(value, result);
                d_internal_release(_in, old);
            }

            const bool ok = d_internal_assign_to(_in, _node->a, value) &&
                            d_awk_value_copy(_out, value);

            d_internal_release(_in, value);

            return ok;
        }

        case D_AWK_N_TERNARY:
        {
            return d_internal_truth(_in, _node->a)
                 ? d_internal_eval(_in, _node->b, _out)
                 : d_internal_eval(_in, _node->c, _out);
        }

        case D_AWK_N_CONCAT:
        {
            struct d_awk_value* const left  = d_internal_acquire(_in);
            struct d_awk_value* const right = d_internal_acquire(_in);
            bool                      ok    = ((left) && (right));

            if (ok)
            {
                ok = d_internal_eval(_in, _node->a, left) &&
                     d_internal_eval(_in, _node->b, right);
            }

            if (ok)
            {
                const char* const convfmt = d_internal_special(_in,
                                                               "CONVFMT",
                                                               "%.6g");
                size_t            left_length  = 0;
                size_t            right_length = 0;
                const char* const left_text    = d_awk_value_text(
                                                     left,
                                                     convfmt,
                                                     &left_length);

                char* joined = malloc(left_length + 1u);

                ok = (joined != NULL);

                if (ok)
                {
                    memcpy(joined, left_text, left_length);

                    const char* const right_text = d_awk_value_text(
                                                       right,
                                                       convfmt,
                                                       &right_length);

                    char* grown = realloc(joined,
                                          left_length + right_length + 1u);

                    ok = (grown != NULL);

                    if (ok)
                    {
                        joined = grown;
                        memcpy(&joined[left_length], right_text, right_length);
                        ok = d_awk_value_set_string(_out,
                                                    joined,
                                                    left_length +
                                                    right_length);
                    }
                }

                free(joined);
            }

            d_internal_release(_in, left);
            d_internal_release(_in, right);

            return ok ? true : d_internal_fail(_in, "out of memory");
        }

        case D_AWK_N_MATCH:
        {
            struct d_awk_value* const subject = d_internal_acquire(_in);
            bool                      ok      = (subject != NULL);

            if (ok)
            {
                ok = d_internal_eval(_in, _node->a, subject);
            }

            bool                  owned = false;
            struct d_regex* const regex = ok
                                        ? d_internal_regex_of(_in,
                                                              _node->b,
                                                              &owned)
                                        : NULL;

            if (regex)
            {
                size_t            length = 0;
                const char* const text   = d_awk_value_text(
                                               subject,
                                               d_internal_special(_in,
                                                                  "CONVFMT",
                                                                  "%.6g"),
                                               &length);
                const bool        hit    = d_regex_test(regex, text, length);

                d_awk_value_set_number(
                    _out,
                    ((_node->op == D_AWK_TOK_MATCH) == hit) ? 1.0 : 0.0);

                // a dynamic pattern belongs to this evaluation alone
                if (owned)
                {
                    d_regex_free(regex);
                }
            }
            else
            {
                ok = false;
            }

            d_internal_release(_in, subject);

            return ok;
        }

        case D_AWK_N_UNARY:
        {
            struct d_awk_value* const cell = d_internal_acquire(_in);

            // abandon the evaluation when the operand failed
            if ((!cell) || (!d_internal_eval(_in, _node->a, cell)))
            {
                d_internal_release(_in, cell);
                return false;
            }

            // negation is logical; the signs are arithmetic
            if (_node->op == D_AWK_TOK_NOT)
            {
                d_awk_value_set_number(_out,
                                       d_awk_value_is_true(cell) ? 0.0 : 1.0);
            }
            else
            {
                const double number = d_awk_value_number(cell);

                d_awk_value_set_number(_out,
                                       (_node->op == D_AWK_TOK_MINUS)
                                     ? -number
                                     : number);
            }

            d_internal_release(_in, cell);

            return true;
        }

        case D_AWK_N_PREINCR:
        case D_AWK_N_POSTINCR:
        {
            struct d_awk_value* const cell = d_internal_acquire(_in);

            // abandon the evaluation when the target could not be read
            if ((!cell) || (!d_internal_eval(_in, _node->a, cell)))
            {
                d_internal_release(_in, cell);
                return false;
            }

            const double before = d_awk_value_number(cell);
            const double after  = (_node->op == D_AWK_TOK_INCR)
                                ? (before + 1.0)
                                : (before - 1.0);

            d_awk_value_set_number(cell, after);

            const bool ok = d_internal_assign_to(_in, _node->a, cell);

            // a prefix form yields the new value, a postfix one the old
            d_awk_value_set_number(_out,
                                   (_node->kind == D_AWK_N_PREINCR)
                                 ? after
                                 : before);
            d_internal_release(_in, cell);

            return ok;
        }

        case D_AWK_N_BINARY:
        {
            // the logical operators short-circuit, so they evaluate lazily
            if (_node->op == D_AWK_TOK_AND)
            {
                const bool left = d_internal_truth(_in, _node->a);

                d_awk_value_set_number(
                    _out,
                    (left && d_internal_truth(_in, _node->b)) ? 1.0 : 0.0);

                return (_in->flow != D_INTERNAL_FLOW_ERROR);
            }

            if (_node->op == D_AWK_TOK_OR)
            {
                const bool left = d_internal_truth(_in, _node->a);

                d_awk_value_set_number(
                    _out,
                    (left || d_internal_truth(_in, _node->b)) ? 1.0 : 0.0);

                return (_in->flow != D_INTERNAL_FLOW_ERROR);
            }

            struct d_awk_value* const left  = d_internal_acquire(_in);
            struct d_awk_value* const right = d_internal_acquire(_in);
            bool                      ok    = ((left) && (right));

            if (ok)
            {
                ok = d_internal_eval(_in, _node->a, left) &&
                     d_internal_eval(_in, _node->b, right);
            }

            if (ok)
            {
                switch (_node->op)
                {
                    case D_AWK_TOK_LT:
                    case D_AWK_TOK_LE:
                    case D_AWK_TOK_GT:
                    case D_AWK_TOK_GE:
                    case D_AWK_TOK_EQ:
                    case D_AWK_TOK_NE:
                    {
                        const int order = d_awk_value_compare(
                            left,
                            right,
                            d_internal_special(_in, "CONVFMT", "%.6g"));
                        bool      truth = false;

                        switch (_node->op)
                        {
                            case D_AWK_TOK_LT: truth = (order < 0);  break;
                            case D_AWK_TOK_LE: truth = (order <= 0); break;
                            case D_AWK_TOK_GT: truth = (order > 0);  break;
                            case D_AWK_TOK_GE: truth = (order >= 0); break;
                            case D_AWK_TOK_EQ: truth = (order == 0); break;
                            default:           truth = (order != 0); break;
                        }

                        d_awk_value_set_number(_out, truth ? 1.0 : 0.0);
                        break;
                    }

                    default:
                    {
                        const double a = d_awk_value_number(left);
                        const double b = d_awk_value_number(right);

                        switch (_node->op)
                        {
                            case D_AWK_TOK_PLUS:
                                d_awk_value_set_number(_out, a + b);
                                break;

                            case D_AWK_TOK_MINUS:
                                d_awk_value_set_number(_out, a - b);
                                break;

                            case D_AWK_TOK_STAR:
                                d_awk_value_set_number(_out, a * b);
                                break;

                            case D_AWK_TOK_CARET:
                                d_awk_value_set_number(_out, pow(a, b));
                                break;

                            case D_AWK_TOK_SLASH:
                            {
                                // division by zero is a runtime error
                                if (b == 0.0)
                                {
                                    ok = d_internal_fail(_in,
                                                         "division by zero");
                                    break;
                                }

                                d_awk_value_set_number(_out, a / b);
                                break;
                            }

                            default:
                            {
                                // the remainder shares the zero check
                                if (b == 0.0)
                                {
                                    ok = d_internal_fail(
                                             _in,
                                             "division by zero in %");
                                    break;
                                }

                                d_awk_value_set_number(_out, fmod(a, b));
                                break;
                            }
                        }

                        break;
                    }
                }
            }

            d_internal_release(_in, left);
            d_internal_release(_in, right);

            return ok;
        }

        case D_AWK_N_BUILTIN:
        {
            return d_internal_builtin(_in, _node, _out);
        }

        case D_AWK_N_CALL:
        {
            return d_internal_call(_in, _node, _out);
        }

        case D_AWK_N_GETLINE:
        {
            return d_internal_getline(_in, _node, _out);
        }

        case D_AWK_N_GROUPLIST:
        {
            // print flattens one of these; nowhere else accepts it
            return d_internal_fail(_in,
                                   "an expression list is only valid as "
                                   "print's arguments or before `in`");
        }

        default:
        {
            return d_internal_fail(_in, "unsupported expression");
        }
    }
}


//==============================================================================
// 9.  NAMED STREAMS
//==============================================================================
// A stream is identified by its name AND its direction, so "f" open for
// reading and "f" open for writing are two entries and close("f") shuts both.


/*
d_internal_stream_open
  Finds a named stream, opening it on first use.
NOTE:
  A `>` redirection truncates only when the stream is first opened; every
  later write to the same name appends, which is what makes
  `{ print > "out" }` accumulate rather than keep one line.

Parameter(s):
  _in:   the interpreter.
  _name: the file or command name.
  _kind: the direction and flavour wanted.
Return:
  The handle, or NULL on failure.
*/
static FILE*
d_internal_stream_open(
    struct d_awk_interp*        _in,
    const char*                 _name,
    enum d_internal_stream_kind _kind
)
{
    const bool input = ( (_kind == D_INTERNAL_STREAM_IN_FILE) ||
                         (_kind == D_INTERNAL_STREAM_IN_PIPE) );

    for (size_t at = 0; at < _in->stream_count; ++at)
    {
        const bool same_side =
            input == ( (_in->streams[at].kind ==
                        D_INTERNAL_STREAM_IN_FILE) ||
                       (_in->streams[at].kind ==
                        D_INTERNAL_STREAM_IN_PIPE) );

        // a stream already open on this side is reused rather than reopened
        if ((same_side) && (strcmp(_in->streams[at].name, _name) == 0))
        {
            return _in->streams[at].handle;
        }
    }

    FILE* handle = NULL;

    // the standard names are recognised before the filesystem is consulted
    if ((_kind == D_INTERNAL_STREAM_IN_FILE) && (strcmp(_name, "-") == 0))
    {
        handle = stdin;
    }
    else if ( (_kind == D_INTERNAL_STREAM_OUT_FILE) ||
              (_kind == D_INTERNAL_STREAM_OUT_APPEND) )
    {
        // the two standard outputs are named rather than opened
        if (strcmp(_name, "/dev/stdout") == 0)
        {
            handle = stdout;
        }
        else if (strcmp(_name, "/dev/stderr") == 0)
        {
            handle = stderr;
        }
        else
        {
            handle = fopen(_name,
                           (_kind == D_INTERNAL_STREAM_OUT_APPEND) ? "a"
                                                                   : "w");
        }
    }
    else if (_kind == D_INTERNAL_STREAM_IN_FILE)
    {
        handle = fopen(_name, "r");
    }
    else
    {
        // a pipe must not interleave with buffered output of our own
        (void)fflush(stdout);
        handle = popen(_name, input ? "r" : "w");
    }

    // a stream that will not open is reported to the caller, not fatal
    if (!handle)
    {
        return NULL;
    }

    // grow the registry when it is full
    if (_in->stream_count == _in->stream_capacity)
    {
        const size_t capacity = (_in->stream_capacity == 0)
                              ? 8u
                              : (_in->stream_capacity * 2u);

        struct d_internal_stream* grown =
            realloc(_in->streams, capacity * sizeof(*grown));

        // report the failure rather than losing track of the handle
        if (!grown)
        {
            (void)d_internal_fail(_in, "out of memory");
            return NULL;
        }

        _in->streams         = grown;
        _in->stream_capacity = capacity;
    }

    char* const copy = malloc(strlen(_name) + 1u);

    // report the failure rather than registering a nameless stream
    if (!copy)
    {
        (void)d_internal_fail(_in, "out of memory");
        return NULL;
    }

    memcpy(copy, _name, strlen(_name) + 1u);

    _in->streams[_in->stream_count].name   = copy;
    _in->streams[_in->stream_count].handle = handle;
    _in->streams[_in->stream_count].kind   = _kind;
    _in->streams[_in->stream_count].eof    = false;
    _in->stream_count++;

    return handle;
}


/*
d_internal_stream_close
  Closes every stream open under a name.

Parameter(s):
  _in:   the interpreter.
  _name: the name to close.
Return:
  The status of the last close, or -1 when nothing was open.
*/
static int
d_internal_stream_close(
    struct d_awk_interp* _in,
    const char*          _name
)
{
    int  status = -1;
    bool found  = false;

    for (size_t at = 0; at < _in->stream_count; )
    {
        // a name may be open in both directions, and both are closed
        if (strcmp(_in->streams[at].name, _name) != 0)
        {
            at++;
            continue;
        }

        struct d_internal_stream* const stream = &_in->streams[at];
        const bool                      pipe   =
            ( (stream->kind == D_INTERNAL_STREAM_IN_PIPE) ||
              (stream->kind == D_INTERNAL_STREAM_OUT_PIPE) );

        // the standard streams are borrowed and must not be closed
        if ( (stream->handle == stdin)  ||
             (stream->handle == stdout) ||
             (stream->handle == stderr) )
        {
            status = 0;
        }
        else if (pipe)
        {
            (void)fflush(stdout);
            status = pclose(stream->handle);
        }
        else
        {
            status = fclose(stream->handle);
        }

        found = true;
        free(stream->name);

        // the tail entry fills the hole, since order does not matter
        _in->streams[at] = _in->streams[--_in->stream_count];
    }

    return found ? status : -1;
}


//==============================================================================
// 10.  STATEMENTS
//==============================================================================


/*
d_internal_output
  Writes a print or printf statement to standard output.

Parameter(s):
  _in:   the interpreter.
  _node: the statement node.
Return:
  A boolean value corresponding to either:
  - true, if the statement was written, or
  - false, otherwise.
*/
static bool
d_internal_output(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node
)
{
    FILE* sink = stdout;

    // a redirection names a file or a command to write through
    if (_node->b)
    {
        struct d_awk_value* const name = d_internal_acquire(_in);

        // abandon the write when the destination could not be evaluated
        if ((!name) || (!d_internal_eval(_in, _node->b, name)))
        {
            d_internal_release(_in, name);
            return false;
        }

        const enum d_internal_stream_kind kind =
            (_node->op == D_AWK_TOK_PIPE)   ? D_INTERNAL_STREAM_OUT_PIPE
          : ((_node->op == D_AWK_TOK_APPEND) ? D_INTERNAL_STREAM_OUT_APPEND
                                             : D_INTERNAL_STREAM_OUT_FILE);

        sink = d_internal_stream_open(_in,
                                      d_awk_value_text(name, NULL, NULL),
                                      kind);
        d_internal_release(_in, name);

        // a destination that will not open is a runtime error
        if (!sink)
        {
            return d_internal_fail(_in, "cannot open output destination");
        }
    }

    // printf renders its whole output through the format
    if (_node->kind == D_AWK_N_PRINTF)
    {
        struct d_awk_value* const text = d_internal_acquire(_in);
        bool                      ok   = (text != NULL);

        if (ok)
        {
            ok = d_internal_format(_in, _node, text);
        }

        if (ok)
        {
            size_t            length = 0;
            const char* const bytes  = d_awk_value_text(text, NULL, &length);

            (void)fwrite(bytes, 1u, length, sink);
        }

        d_internal_release(_in, text);

        return ok;
    }

    const char* const ofs = d_internal_special(_in, "OFS", " ");
    const char* const ors = d_internal_special(_in, "ORS", "\n");

    // print with no arguments writes the whole record
    if (_node->count == 0)
    {
        // abandon the write when the record could not be rebuilt
        if (!d_internal_ensure_record(_in))
        {
            return false;
        }

        (void)fwrite(_in->record, 1u, _in->record_length, sink);
        (void)fputs(ors, sink);

        return true;
    }

    // OFMT rather than CONVFMT governs a number printed by print
    const char* const ofmt = d_internal_special(_in, "OFMT", "%.6g");

    for (size_t at = 0; at < _node->count; ++at)
    {
        struct d_awk_value* const cell = d_internal_acquire(_in);

        // abandon the write when an argument could not be evaluated
        if ((!cell) || (!d_internal_eval(_in, _node->list[at], cell)))
        {
            d_internal_release(_in, cell);
            return false;
        }

        // the separator joins arguments rather than leading them
        if (at > 0)
        {
            (void)fputs(ofs, sink);
        }

        size_t            length = 0;
        const char* const text   = d_awk_value_text(cell, ofmt, &length);

        (void)fwrite(text, 1u, length, sink);
        d_internal_release(_in, cell);
    }

    (void)fputs(ors, sink);

    return true;
}


static bool
d_internal_exec(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node
)
{
    // a pending transfer suspends execution until it is handled
    if ((!_node) || (_in->flow != D_INTERNAL_FLOW_NONE))
    {
        return (_in->flow != D_INTERNAL_FLOW_ERROR);
    }

    switch (_node->kind)
    {
        case D_AWK_N_BLOCK:
        {
            for (size_t at = 0; at < _node->count; ++at)
            {
                // abandon the block when a statement failed or transferred
                if (!d_internal_exec(_in, _node->list[at]))
                {
                    return false;
                }

                // a transfer leaves the remaining statements unrun
                if (_in->flow != D_INTERNAL_FLOW_NONE)
                {
                    break;
                }
            }

            return true;
        }

        case D_AWK_N_EXPR_STMT:
        {
            struct d_awk_value* const cell = d_internal_acquire(_in);
            const bool                ok   = (cell) &&
                                             d_internal_eval(_in,
                                                             _node->a,
                                                             cell);

            d_internal_release(_in, cell);

            return ok;
        }

        case D_AWK_N_PRINT:
        case D_AWK_N_PRINTF:
        {
            return d_internal_output(_in, _node);
        }

        case D_AWK_N_IF:
        {
            return d_internal_truth(_in, _node->a)
                 ? d_internal_exec(_in, _node->b)
                 : d_internal_exec(_in, _node->c);
        }

        case D_AWK_N_WHILE:
        {
            while (d_internal_truth(_in, _node->a))
            {
                // abandon the loop when the body failed
                if (!d_internal_exec(_in, _node->b))
                {
                    return false;
                }

                // a break leaves the loop; a continue only this iteration
                if (_in->flow == D_INTERNAL_FLOW_BREAK)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                    break;
                }

                if (_in->flow == D_INTERNAL_FLOW_CONTINUE)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                    continue;
                }

                // any other transfer propagates out of the loop
                if (_in->flow != D_INTERNAL_FLOW_NONE)
                {
                    break;
                }
            }

            return (_in->flow != D_INTERNAL_FLOW_ERROR);
        }

        case D_AWK_N_DO:
        {
            do
            {
                // abandon the loop when the body failed
                if (!d_internal_exec(_in, _node->a))
                {
                    return false;
                }

                // a break leaves the loop; a continue tests the condition
                if (_in->flow == D_INTERNAL_FLOW_BREAK)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                    break;
                }

                if (_in->flow == D_INTERNAL_FLOW_CONTINUE)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                }

                // any other transfer propagates out of the loop
                if (_in->flow != D_INTERNAL_FLOW_NONE)
                {
                    break;
                }
            }
            while (d_internal_truth(_in, _node->b));

            return (_in->flow != D_INTERNAL_FLOW_ERROR);
        }

        case D_AWK_N_FOR:
        {
            // the initialiser runs once, before the first test
            if (_node->a)
            {
                struct d_awk_value* const cell = d_internal_acquire(_in);
                const bool                ok   = (cell) &&
                                                 d_internal_eval(_in,
                                                                 _node->a,
                                                                 cell);

                d_internal_release(_in, cell);

                // abandon the loop when the initialiser failed
                if (!ok)
                {
                    return false;
                }
            }

            // an absent condition is an infinite loop
            while ((!_node->b) || (d_internal_truth(_in, _node->b)))
            {
                // abandon the loop when the body failed
                if (!d_internal_exec(_in, _node->d))
                {
                    return false;
                }

                // a break leaves the loop; a continue runs the step
                if (_in->flow == D_INTERNAL_FLOW_BREAK)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                    break;
                }

                if (_in->flow == D_INTERNAL_FLOW_CONTINUE)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                }

                // any other transfer propagates out of the loop
                if (_in->flow != D_INTERNAL_FLOW_NONE)
                {
                    break;
                }

                // the step runs after the body and before the next test
                if (_node->c)
                {
                    struct d_awk_value* const cell = d_internal_acquire(_in);
                    const bool                ok   = (cell) &&
                                                     d_internal_eval(_in,
                                                                     _node->c,
                                                                     cell);

                    d_internal_release(_in, cell);

                    // abandon the loop when the step failed
                    if (!ok)
                    {
                        return false;
                    }
                }
            }

            return (_in->flow != D_INTERNAL_FLOW_ERROR);
        }

        case D_AWK_N_FORIN:
        {
            struct d_awk_array* const array = d_internal_array(_in,
                                                               _node->b->text);

            // abandon the loop when the array could not be resolved
            if (!array)
            {
                return d_internal_fail(_in, "out of memory");
            }

            size_t              cursor = 0;
            const char*         key    = NULL;
            size_t              length = 0;
            struct d_awk_value* cell   = d_internal_acquire(_in);

            // abandon the loop when the scratch cell could not be held
            if (!cell)
            {
                return d_internal_fail(_in, "out of memory");
            }

            while (d_awk_array_next(array, &cursor, &key, &length, NULL))
            {
                // a subscript is input-derived, so "1" iterates numerically
                if (!d_awk_value_set_input(cell, key, length))
                {
                    d_internal_release(_in, cell);
                    return d_internal_fail(_in, "out of memory");
                }

                // abandon the loop when the variable could not be set
                if (!d_internal_assign_to(_in, _node->a, cell))
                {
                    d_internal_release(_in, cell);
                    return false;
                }

                // abandon the loop when the body failed
                if (!d_internal_exec(_in, _node->d))
                {
                    d_internal_release(_in, cell);
                    return false;
                }

                // a break leaves the loop; a continue only this iteration
                if (_in->flow == D_INTERNAL_FLOW_BREAK)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                    break;
                }

                if (_in->flow == D_INTERNAL_FLOW_CONTINUE)
                {
                    _in->flow = D_INTERNAL_FLOW_NONE;
                    continue;
                }

                // any other transfer propagates out of the loop
                if (_in->flow != D_INTERNAL_FLOW_NONE)
                {
                    break;
                }
            }

            d_internal_release(_in, cell);

            return (_in->flow != D_INTERNAL_FLOW_ERROR);
        }

        case D_AWK_N_BREAK:
        {
            _in->flow = D_INTERNAL_FLOW_BREAK;
            return true;
        }

        case D_AWK_N_CONTINUE:
        {
            _in->flow = D_INTERNAL_FLOW_CONTINUE;
            return true;
        }

        case D_AWK_N_NEXT:
        {
            _in->flow = D_INTERNAL_FLOW_NEXT;
            return true;
        }

        case D_AWK_N_NEXTFILE:
        {
            // the current operand is abandoned; the next one opens normally
            if ((_in->stream) && (_in->stream_owned))
            {
                (void)fclose(_in->stream);
            }

            _in->stream       = NULL;
            _in->stream_owned = false;
            _in->flow         = D_INTERNAL_FLOW_NEXT;

            return true;
        }

        case D_AWK_N_EXIT:
        {
            // an exit value is optional and defaults to the current status
            if (_node->a)
            {
                struct d_awk_value* const cell = d_internal_acquire(_in);
                const bool                ok   = (cell) &&
                                                 d_internal_eval(_in,
                                                                 _node->a,
                                                                 cell);

                if (ok)
                {
                    _in->exit_status = (int)d_awk_value_number(cell);
                }

                d_internal_release(_in, cell);

                // abandon the exit when the value failed
                if (!ok)
                {
                    return false;
                }
            }

            _in->flow = D_INTERNAL_FLOW_EXIT;

            return true;
        }

        case D_AWK_N_RETURN:
        {
            // a return value is optional and defaults to uninitialised
            if (_node->a)
            {
                struct d_awk_value* const cell = d_internal_acquire(_in);
                const bool                ok   = (cell) &&
                                                 d_internal_eval(_in,
                                                                 _node->a,
                                                                 cell);

                // abandon the return when the value failed
                if (!ok)
                {
                    d_internal_release(_in, cell);
                    return false;
                }

                d_internal_release(_in, _in->retval);
                _in->retval = cell;
            }

            _in->flow = D_INTERNAL_FLOW_RETURN;

            return true;
        }

        case D_AWK_N_DELETE:
        {
            char*  key    = NULL;
            size_t length = 0;

            // abandon the deletion when the subscript could not be built
            if (!d_internal_subscript(_in, _node, &key, &length))
            {
                return false;
            }

            struct d_awk_array* const array = d_internal_array(_in,
                                                               _node->a->text);

            // a deletion of an absent element is not an error
            if (array)
            {
                (void)d_awk_array_delete(array, key, length);
            }

            free(key);

            return true;
        }

        case D_AWK_N_DELETE_ALL:
        {
            struct d_awk_array* const array = d_internal_array(_in,
                                                               _node->a->text);

            // clearing an array that does not exist is not an error
            if (array)
            {
                d_awk_array_clear(array);
            }

            return true;
        }

        default:
        {
            struct d_awk_value* const cell = d_internal_acquire(_in);
            const bool                ok   = (cell) &&
                                             d_internal_eval(_in,
                                                             _node,
                                                             cell);

            d_internal_release(_in, cell);

            return ok;
        }
    }
}


//==============================================================================
// 11.  THE MAIN LOOP
//==============================================================================


/*
d_internal_next_stream
  Opens the next input named by ARGV, applying any assignment operands.
NOTE:
  The walk reads ARGV at the moment it advances rather than from a list built
  at startup, so a program that rewrites ARGV in BEGIN is obeyed.  An operand
  of the form name=value is an assignment performed when it is reached, not a
  file, which is how a program can change FS between files.

Parameter(s):
  _in: the interpreter.
Return:
  A boolean value corresponding to either:
  - true, if a stream is open, or
  - false, when the operands are exhausted.
*/
static bool
d_internal_next_stream(
    struct d_awk_interp* _in
)
{
    // close whatever the previous operand left open
    if ((_in->stream) && (_in->stream_owned))
    {
        (void)fclose(_in->stream);
    }

    _in->stream       = NULL;
    _in->stream_owned = false;

    struct d_awk_array* const argv = d_internal_array(_in, "ARGV");
    const double              argc = d_awk_value_number(
                                         d_internal_cell(_in, "ARGC"));

    while ((argv) && ((double)_in->argv_at < argc))
    {
        char   key[32];
        const int written = snprintf(key, sizeof(key), "%zu", _in->argv_at++);

        struct d_awk_value* const slot = d_awk_array_find(argv,
                                                          key,
                                                          (size_t)written);

        // a slot the program deleted or emptied contributes nothing
        if (!slot)
        {
            continue;
        }

        size_t            length = 0;
        const char* const text   = d_awk_value_text(slot, NULL, &length);

        if (length == 0)
        {
            continue;
        }

        // an assignment operand takes effect here rather than opening a file
        if (d_awk_interp_assign(_in, text))
        {
            continue;
        }

        _in->stream       = (strcmp(text, "-") == 0) ? stdin : fopen(text, "r");
        _in->stream_owned = (_in->stream != stdin);
        _in->opened_any   = true;

        // a file that will not open is a runtime error
        if (!_in->stream)
        {
            return d_internal_fail(_in, "cannot open input file");
        }

        (void)d_awk_value_set_input(d_internal_cell(_in, "FILENAME"),
                                    text,
                                    length);
        d_awk_value_set_number(d_internal_cell(_in, "FNR"), 0.0);

        return true;
    }

    // operands that were all assignments still leave standard input to read
    if ((!_in->opened_any) && (!_in->used_stdin))
    {
        _in->used_stdin = true;
        _in->stream     = stdin;

        return true;
    }

    return false;
}


/*
d_internal_read_one
  Reads one record from a stream, honouring RS and its paragraph mode.

Parameter(s):
  _in:         the interpreter, which owns the shared read buffer.
  _stream:     the stream to read.
  _out_length: receives the record length.
Return:
  A boolean value corresponding to either:
  - true, if a record was read, or
  - false, at end of stream.
*/
static bool
d_internal_read_one(
    struct d_awk_interp* _in,
    FILE*                _stream,
    size_t*              _out_length
)
{
    const char* const rs        = d_internal_special(_in, "RS", "\n");
    const bool        paragraph = (rs[0] == '\0');
    const char        delimiter = paragraph ? '\n' : rs[0];

    size_t used     = 0;
    int    byte     = 0;
    bool   any      = false;
    int    newlines = 0;

    // paragraph mode skips the blank lines before a record
    if (paragraph)
    {
        while ((byte = fgetc(_stream)) == '\n')
        {
            continue;
        }

        // a stream holding only blank lines yields no record
        if (byte != EOF)
        {
            (void)ungetc(byte, _stream);
        }
    }

    while ((byte = fgetc(_stream)) != EOF)
    {
        any = true;

        // a paragraph ends at a blank line rather than a single newline
        if (paragraph)
        {
            newlines = (byte == '\n') ? (newlines + 1) : 0;

            // two consecutive newlines close the record
            if (newlines == 2)
            {
                used--;
                break;
            }
        }
        else if (byte == delimiter)
        {
            break;
        }

        // grow the buffer when the byte does not fit
        if ((used + 2u) > _in->read_capacity)
        {
            const size_t wanted = (_in->read_capacity == 0)
                                ? 256u
                                : (_in->read_capacity * 2u);
            char*        grown  = realloc(_in->read_buffer, wanted);

            // abandon the read when the buffer could not grow
            if (!grown)
            {
                (void)d_internal_fail(_in, "out of memory");
                return false;
            }

            _in->read_buffer   = grown;
            _in->read_capacity = wanted;
        }

        _in->read_buffer[used++] = (char)byte;
    }

    // a stream at its end with no trailing bytes yields no record
    if ((!any) && (byte == EOF))
    {
        return false;
    }

    // a buffer that was never grown still needs a terminator
    if (_in->read_capacity == 0)
    {
        _in->read_buffer   = malloc(1u);
        _in->read_capacity = 1u;

        // abandon the read when even one byte could not be held
        if (!_in->read_buffer)
        {
            (void)d_internal_fail(_in, "out of memory");
            return false;
        }
    }

    _in->read_buffer[used] = '\0';
    *_out_length           = used;

    return true;
}


/*
d_internal_next_record
  Yields the next record from the host source when one is installed, and from
RS splitting otherwise.  The source hands back a borrowed buffer, so its bytes
are copied into the interpreter's read buffer before the loop sees them; that
keeps the lifetime rule for a source as weak as possible -- valid until the
next call -- and leaves every downstream invariant identical in both modes.
*/
static bool
d_internal_next_record(
    struct d_awk_interp* _in,
    size_t*              _out_length
)
{
    // no host source means the POSIX path, unchanged
    if (!_in->source)
    {
        return d_internal_next_main_record(_in, _out_length);
    }

    const char* text   = NULL;
    size_t      length = 0;

    // an exhausted source ends the record loop
    if (!_in->source->next_record(_in->source->user, &text, &length))
    {
        return false;
    }

    // a source that reports success without bytes is a wiring bug
    if ((!text) && (length > 0))
    {
        return d_internal_fail(_in, "record source returned no bytes");
    }

    // grow the read buffer when the record and its terminator do not fit
    if (_in->read_capacity < (length + 1u))
    {
        char* const grown = realloc(_in->read_buffer, length + 1u);

        // report the failure rather than truncating the record
        if (!grown)
        {
            return d_internal_fail(_in, "out of memory");
        }

        _in->read_buffer   = grown;
        _in->read_capacity = length + 1u;
    }

    if (length > 0)
    {
        memcpy(_in->read_buffer, text, length);
    }

    _in->read_buffer[length] = '\0';
    *_out_length             = length;

    return true;
}


/*
d_internal_next_main_record
  Reads the next record from the main input, advancing between operands.
NOTE:
  A plain getline shares this with the main loop, so the two never read the
  same record twice.

Parameter(s):
  _in:         the interpreter.
  _out_length: receives the record length.
Return:
  A boolean value corresponding to either:
  - true, if a record was read, or
  - false, when every input is exhausted.
*/
static bool
d_internal_next_main_record(
    struct d_awk_interp* _in,
    size_t*              _out_length
)
{
    while (true)
    {
        // open the next operand when no stream is current
        if ((!_in->stream) && (!d_internal_next_stream(_in)))
        {
            return false;
        }

        // a record from the current stream is the answer
        if (d_internal_read_one(_in, _in->stream, _out_length))
        {
            return true;
        }

        // an error during the read is not an end of input
        if (_in->flow == D_INTERNAL_FLOW_ERROR)
        {
            return false;
        }

        // an exhausted stream is dropped so the next operand is opened
        if ((_in->stream) && (_in->stream_owned))
        {
            (void)fclose(_in->stream);
        }

        _in->stream       = NULL;
        _in->stream_owned = false;
    }
}


/*
d_internal_getline
  Evaluates any of the six getline forms.
NOTE:
  Which counters move depends on the form, and the pattern is not arbitrary:
  a form that reads the MAIN input advances NR and FNR, a form that reads a
  command advances NR only, and a form that reads a named file advances
  neither.  Setting $0 always recomputes NF, since the fields are lazy.

Parameter(s):
  _in:   the interpreter.
  _node: the getline node.
  _out:  receives 1, 0 at end of input, or -1 on error.
Return:
  A boolean value corresponding to either:
  - true, if the form was evaluated, or
  - false, on a fatal error.
*/
static bool
d_internal_getline(
    struct d_awk_interp* _in,
    struct d_awk_node*   _node,
    struct d_awk_value*  _out
)
{
    FILE* stream    = NULL;
    bool  main_input = false;

    // the source is a file, a command, or the main input
    if ((_node->op == D_AWK_TOK_LT) || (_node->op == D_AWK_TOK_PIPE))
    {
        struct d_awk_value* const name = d_internal_acquire(_in);

        // abandon the read when the source name could not be evaluated
        if ((!name) || (!d_internal_eval(_in, _node->b, name)))
        {
            d_internal_release(_in, name);
            return false;
        }

        size_t            length = 0;
        const char* const text   = d_awk_value_text(name, NULL, &length);

        stream = d_internal_stream_open(
                     _in,
                     text,
                     (_node->op == D_AWK_TOK_LT)
                         ? D_INTERNAL_STREAM_IN_FILE
                         : D_INTERNAL_STREAM_IN_PIPE);
        d_internal_release(_in, name);

        // a source that will not open reports -1 rather than failing
        if (!stream)
        {
            d_awk_value_set_number(_out, -1.0);
            return true;
        }
    }
    else
    {
        main_input = true;
    }

    size_t length = 0;
    bool   got    = main_input
                  ? d_internal_next_main_record(_in, &length)
                  : d_internal_read_one(_in, stream, &length);

    // an error during the read is fatal rather than an end of input
    if (_in->flow == D_INTERNAL_FLOW_ERROR)
    {
        return false;
    }

    // the end of the source reports zero and changes nothing
    if (!got)
    {
        d_awk_value_set_number(_out, 0.0);
        return true;
    }

    // a target takes the record; without one it becomes $0 and resplits
    if (_node->a)
    {
        struct d_awk_value* const cell = d_internal_acquire(_in);
        bool                      ok   = (cell != NULL);

        if (ok)
        {
            ok = d_awk_value_set_input(cell, _in->read_buffer, length) &&
                 d_internal_assign_to(_in, _node->a, cell);
        }

        d_internal_release(_in, cell);

        // abandon the read when the target could not be assigned
        if (!ok)
        {
            return false;
        }
    }
    else if (!d_internal_set_record(_in, _in->read_buffer, length))
    {
        return false;
    }

    // the main input advances both counters; a command advances only NR
    if ((main_input) || (_node->op == D_AWK_TOK_PIPE))
    {
        struct d_awk_value* const nr = d_internal_cell(_in, "NR");

        d_awk_value_set_number(nr, d_awk_value_number(nr) + 1.0);
    }

    if (main_input)
    {
        struct d_awk_value* const fnr = d_internal_cell(_in, "FNR");

        d_awk_value_set_number(fnr, d_awk_value_number(fnr) + 1.0);
    }

    d_awk_value_set_number(_out, 1.0);

    return true;
}


/*
d_internal_run_rules
  Runs the main rules against the current record.

Parameter(s):
  _in: the interpreter.
Return:
  A boolean value corresponding to either:
  - true, if the record was processed, or
  - false, otherwise.
*/
static bool
d_internal_run_rules(
    struct d_awk_interp* _in
)
{
    for (size_t at = 0; at < _in->program->rule_count; ++at)
    {
        struct d_awk_rule* const rule = &_in->program->rules[at];

        // only the main rules see records
        if (rule->kind != D_AWK_RULE_MAIN)
        {
            continue;
        }

        bool matched = true;

        // a range is entered by its first pattern and left by its second
        if (rule->pattern_end)
        {
            if (!rule->active)
            {
                rule->active = d_internal_truth(_in, rule->pattern);
                matched      = rule->active;

                // a range whose ends both match on one record is one record
                if ((rule->active) &&
                    (d_internal_truth(_in, rule->pattern_end)))
                {
                    rule->active = false;
                }
            }
            else
            {
                matched = true;

                // the closing pattern is tested on every later record
                if (d_internal_truth(_in, rule->pattern_end))
                {
                    rule->active = false;
                }
            }
        }
        else if (rule->pattern)
        {
            matched = d_internal_truth(_in, rule->pattern);
        }

        // abandon the record when a pattern raised an error
        if (_in->flow == D_INTERNAL_FLOW_ERROR)
        {
            return false;
        }

        // a rule that does not match contributes nothing
        if (!matched)
        {
            continue;
        }

        // a rule with no action prints the record
        if (!rule->action)
        {
            // abandon the record when the record could not be rebuilt
            if (!d_internal_ensure_record(_in))
            {
                return false;
            }

            (void)fwrite(_in->record, 1u, _in->record_length, stdout);
            (void)fputs(d_internal_special(_in, "ORS", "\n"), stdout);

            continue;
        }

        // abandon the record when the action failed
        if (!d_internal_exec(_in, rule->action))
        {
            return false;
        }

        // next abandons the remaining rules for this record only
        if (_in->flow == D_INTERNAL_FLOW_NEXT)
        {
            _in->flow = D_INTERNAL_FLOW_NONE;
            break;
        }

        // exit and errors propagate out of the main loop
        if (_in->flow != D_INTERNAL_FLOW_NONE)
        {
            break;
        }
    }

    return (_in->flow != D_INTERNAL_FLOW_ERROR);
}


//==============================================================================
// 12.  PUBLIC INTERFACE
//==============================================================================


/*
d_awk_interp_new
  Creates an interpreter over a parsed program.

Parameter(s):
  _program: the program to run; borrowed, not owned.
Return:
  The interpreter, or NULL on failure.
*/
struct d_awk_interp*
d_awk_interp_new(
    struct d_awk_program* _program
)
{
    // parameter validation first
    if (!_program)
    {
        return NULL;
    }

    struct d_awk_interp* interp = calloc(1, sizeof(*interp));

    // abandon the allocation when the handle could not be held
    if (!interp)
    {
        return NULL;
    }

    interp->program = _program;
    interp->globals = d_awk_array_new();

    // abandon the allocation when the globals could not be held
    if (!interp->globals)
    {
        free(interp);
        return NULL;
    }

    // the special variables start at their documented defaults
    (void)d_awk_value_set_string(d_internal_cell(interp, "FS"), " ", 1u);
    (void)d_awk_value_set_string(d_internal_cell(interp, "OFS"), " ", 1u);
    (void)d_awk_value_set_string(d_internal_cell(interp, "ORS"), "\n", 1u);
    (void)d_awk_value_set_string(d_internal_cell(interp, "RS"), "\n", 1u);
    (void)d_awk_value_set_string(d_internal_cell(interp, "SUBSEP"),
                                 "\034",
                                 1u);
    (void)d_awk_value_set_string(d_internal_cell(interp, "CONVFMT"),
                                 "%.6g",
                                 4u);
    (void)d_awk_value_set_string(d_internal_cell(interp, "OFMT"), "%.6g", 4u);
    d_awk_value_set_number(d_internal_cell(interp, "NR"), 0.0);
    d_awk_value_set_number(d_internal_cell(interp, "FNR"), 0.0);
    d_awk_value_set_number(d_internal_cell(interp, "RSTART"), 0.0);
    d_awk_value_set_number(d_internal_cell(interp, "RLENGTH"), -1.0);

    // ARGV[0] names the interpreter, as POSIX specifies; the operands follow
    struct d_awk_array* const argv = d_internal_array(interp, "ARGV");

    // abandon the allocation when ARGV could not be created
    if (!argv)
    {
        d_awk_interp_free(interp);
        return NULL;
    }

    (void)d_awk_value_set_input(d_awk_array_lookup(argv, "0", 1u), "awk", 3u);
    d_awk_value_set_number(d_internal_cell(interp, "ARGC"), 1.0);
    interp->argv_at = 1u;

    srand(0);

    return interp;
}


/*
d_awk_interp_free
  Releases an interpreter and everything it owns.

Parameter(s):
  _interp: the interpreter to release; may be NULL.
Return:
  none.
*/
void
d_awk_interp_free(
    struct d_awk_interp* _interp
)
{
    if (_interp)
    {
        // the source releases what it owns before the interpreter goes away
        if ((_interp->source) && (_interp->source->release))
        {
            _interp->source->release(_interp->source->user);
        }

        // a file still open at teardown is closed here
        if ((_interp->stream) && (_interp->stream_owned))
        {
            (void)fclose(_interp->stream);
        }

        for (size_t at = 0; at < _interp->field_capacity; ++at)
        {
            d_awk_value_free(_interp->fields[at]);
        }

        for (size_t at = 0; at < _interp->owned_count; ++at)
        {
            d_awk_array_free(_interp->owned[at]);
        }

        for (size_t at = 0; at < _interp->pool_count; ++at)
        {
            d_awk_value_free(_interp->pool[at]);
        }

        for (size_t at = 0; at < _interp->input_count; ++at)
        {
            free(_interp->inputs[at]);
        }

        // a stream the program left open is closed at teardown
        while (_interp->stream_count > 0)
        {
            struct d_internal_stream* const stream =
                &_interp->streams[--_interp->stream_count];
            const bool pipe =
                ( (stream->kind == D_INTERNAL_STREAM_IN_PIPE) ||
                  (stream->kind == D_INTERNAL_STREAM_OUT_PIPE) );

            if ( (stream->handle != stdin)  &&
                 (stream->handle != stdout) &&
                 (stream->handle != stderr) )
            {
                if (pipe)
                {
                    (void)pclose(stream->handle);
                }
                else
                {
                    (void)fclose(stream->handle);
                }
            }

            free(stream->name);
        }

        free(_interp->streams);
        free(_interp->read_buffer);
        free(_interp->split_fs);

        d_awk_value_free(_interp->retval);
        d_awk_array_free(_interp->globals);
        free(_interp->fields);
        free(_interp->owned);
        free(_interp->pool);
        free(_interp->inputs);
        free(_interp->frames);
        free(_interp->record);
        free(_interp);
    }

    return;
}


/*
d_awk_interp_assign
  Applies a `name=value` assignment, as -v and a file operand both give.

Parameter(s):
  _interp: the interpreter.
  _text:   the assignment text.
Return:
  A boolean value corresponding to either:
  - true, if the assignment was applied, or
  - false, if the text is not an assignment.
*/
bool
d_awk_interp_assign(
    struct d_awk_interp* _interp,
    const char*          _text
)
{
    const char* const split = strchr(_text, '=');

    // text without an equals sign is not an assignment
    if ((!_interp) || (!split) || (split == _text))
    {
        return false;
    }

    char* const name = malloc((size_t)(split - _text) + 1u);

    // report the failure rather than applying half an assignment
    if (!name)
    {
        return false;
    }

    memcpy(name, _text, (size_t)(split - _text));
    name[split - _text] = '\0';

    struct d_awk_value* const cell = d_internal_cell(_interp, name);
    const bool                ok   = (cell != NULL) &&
                                     d_awk_value_set_input(cell,
                                                           split + 1,
                                                           strlen(split + 1));

    free(name);

    return ok;
}


/*
d_awk_interp_add_input
  Appends a file operand to the input list.

Parameter(s):
  _interp: the interpreter.
  _path:   the path to append.
Return:
  A boolean value corresponding to either:
  - true, if the path was appended, or
  - false, otherwise.
*/
bool
d_awk_interp_add_input(
    struct d_awk_interp* _interp,
    const char*          _path
)
{
    // parameter validation first
    if ((!_interp) || (!_path))
    {
        return false;
    }

    struct d_awk_array* const argv = d_internal_array(_interp, "ARGV");
    struct d_awk_value* const argc = d_internal_cell(_interp, "ARGC");

    // report the failure rather than dropping the operand
    if ((!argv) || (!argc))
    {
        return false;
    }

    char      key[32];
    const int written = snprintf(key,
                                 sizeof(key),
                                 "%d",
                                 (int)d_awk_value_number(argc));

    struct d_awk_value* const slot = d_awk_array_lookup(argv,
                                                        key,
                                                        (size_t)written);

    // report the failure rather than leaving a gap in ARGV
    if (!slot)
    {
        return false;
    }

    // an operand is input-derived, so a numeric one compares numerically
    if (!d_awk_value_set_input(slot, _path, strlen(_path)))
    {
        return false;
    }

    d_awk_value_set_number(argc, d_awk_value_number(argc) + 1.0);

    return true;
}


/*
d_awk_interp_set_source
  Installs a record source in place of RS splitting.  The descriptor is
borrowed, not copied, so it must outlive the interpreter; its `release` is
called at teardown.  Refusing a NULL `next_record` matters because an empty
input and an unwired source are indistinguishable once the loop starts, and
only one of them is a bug worth reporting.
*/
bool
d_awk_interp_set_source(
    struct d_awk_interp* _interp,
    struct d_awk_source* _source
)
{
    // parameter validation first
    if (!_interp)
    {
        return false;
    }

#ifdef D_AWK_STRICT_POSIX

    (void)_source;

    return d_internal_fail(_interp,
                           "record sources are refused under strict POSIX");

#else

    // a source without a reader is a wiring bug, not an empty input
    if ((_source) && (!_source->next_record))
    {
        return d_internal_fail(_interp, "record source has no reader");
    }

    _interp->source = _source;

    return true;

#endif  // D_AWK_STRICT_POSIX
}


/*
d_awk_interp_set_environ
  Fills ENVIRON from a `name=value` vector.

Parameter(s):
  _interp:  the interpreter.
  _environ: the vector, terminated by a NULL entry.
Return:
  A boolean value corresponding to either:
  - true, if ENVIRON was filled, or
  - false, otherwise.
*/
bool
d_awk_interp_set_environ(
    struct d_awk_interp* _interp,
    char* const*         _environ
)
{
    // parameter validation first
    if ((!_interp) || (!_environ))
    {
        return false;
    }

    struct d_awk_array* const array = d_internal_array(_interp, "ENVIRON");

    // abandon the fill when ENVIRON could not be created
    if (!array)
    {
        return false;
    }

    for (size_t at = 0; _environ[at]; ++at)
    {
        const char* const split = strchr(_environ[at], '=');

        // an entry without an equals sign is not a binding
        if (!split)
        {
            continue;
        }

        struct d_awk_value* const slot = d_awk_array_lookup(
                                             array,
                                             _environ[at],
                                             (size_t)(split - _environ[at]));

        // an entry that cannot be stored is skipped rather than fatal
        if (slot)
        {
            (void)d_awk_value_set_input(slot, split + 1, strlen(split + 1));
        }
    }

    return true;
}


/*
d_awk_interp_run
  Runs BEGIN, then the main rules over every record, then END.
NOTE:
  The record loop is skipped entirely when the program has no main or END
  rule, which is what lets `dawk 'BEGIN { ... }'` finish without reading
  standard input.

Parameter(s):
  _interp: the interpreter.
Return:
  The exit status the program requested, or 2 on a runtime error.
*/
int
d_awk_interp_run(
    struct d_awk_interp* _interp
)
{
    // parameter validation first
    if (!_interp)
    {
        return 2;
    }

    for (size_t at = 0; at < _interp->program->rule_count; ++at)
    {
        // only the BEGIN rules run before any input is read
        if (_interp->program->rules[at].kind != D_AWK_RULE_BEGIN)
        {
            continue;
        }

        (void)d_internal_exec(_interp, _interp->program->rules[at].action);

        // exit and errors skip the remaining BEGIN rules
        if (_interp->flow != D_INTERNAL_FLOW_NONE)
        {
            break;
        }
    }

    bool wants_input = false;

    for (size_t at = 0; at < _interp->program->rule_count; ++at)
    {
        const enum d_awk_rule_kind kind = _interp->program->rules[at].kind;

        // a main or END rule is a reason to read the input
        if (kind != D_AWK_RULE_BEGIN)
        {
            wants_input = true;
            break;
        }
    }

    // the record loop runs only when some rule could observe a record
    if ( (wants_input)                                    &&
         (_interp->flow != D_INTERNAL_FLOW_EXIT)          &&
         (_interp->flow != D_INTERNAL_FLOW_ERROR) )
    {
        size_t length = 0;

        while (d_internal_next_record(_interp, &length))
        {
            // abandon the loop when the record could not be stored
            if (!d_internal_set_record(_interp,
                                       _interp->read_buffer,
                                       length))
            {
                break;
            }

            struct d_awk_value* const nr  = d_internal_cell(_interp, "NR");
            struct d_awk_value* const fnr = d_internal_cell(_interp, "FNR");

            d_awk_value_set_number(nr, d_awk_value_number(nr) + 1.0);
            d_awk_value_set_number(fnr, d_awk_value_number(fnr) + 1.0);

            // abandon the loop when a rule failed
            if (!d_internal_run_rules(_interp))
            {
                break;
            }

            // exit leaves the record loop but still runs END
            if (_interp->flow != D_INTERNAL_FLOW_NONE)
            {
                break;
            }
        }
    }

    const bool exiting = (_interp->flow == D_INTERNAL_FLOW_EXIT);

    // an exit clears the transfer so END can still run, as awk requires
    if (exiting)
    {
        _interp->flow = D_INTERNAL_FLOW_NONE;
    }

    if (_interp->flow != D_INTERNAL_FLOW_ERROR)
    {
        for (size_t at = 0; at < _interp->program->rule_count; ++at)
        {
            // only the END rules run after the input is exhausted
            if (_interp->program->rules[at].kind != D_AWK_RULE_END)
            {
                continue;
            }

            (void)d_internal_exec(_interp,
                                  _interp->program->rules[at].action);

            // a second exit or an error skips the remaining END rules
            if (_interp->flow != D_INTERNAL_FLOW_NONE)
            {
                break;
            }
        }
    }

    (void)fflush(stdout);

    // a runtime error reports a status of its own
    if (_interp->flow == D_INTERNAL_FLOW_ERROR)
    {
        return 2;
    }

    return _interp->exit_status;
}


/*
d_awk_interp_error
  Returns the runtime diagnostic, if any.

Parameter(s):
  _interp: the interpreter.
Return:
  The diagnostic, or NULL when none was recorded.
*/
const char*
d_awk_interp_error(
    const struct d_awk_interp* _interp
)
{
    // parameter validation first
    if ((!_interp) || (_interp->error[0] == '\0'))
    {
        return NULL;
    }

    return _interp->error;
}
