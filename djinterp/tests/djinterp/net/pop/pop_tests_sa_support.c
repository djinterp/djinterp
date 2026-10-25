/*******************************************************************************
* djinterp [net]                                          pop_tests_sa_support.c
*
* What the POP3 kernel's tests share: a scripted connection, and the sinks
* and comparisons the tests check their results with.
*
*
* path:      /tests/djinterp/net/pop/pop_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./pop_tests_sa.h"  // corresponding header
// std
#include <string.h>  // memcmp, memcpy, strlen


/*
d_tests_pop_script_read
  The current step's bytes, as many as the read asks for; the step's error
comes with the last of them.
*/
static struct d_net_io_result
d_tests_pop_script_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    struct d_tests_pop_script* const script =
        (struct d_tests_pop_script*)_connection;

    script->reads += 1u;

    // past the last step, the stream has ended
    if (script->at >= script->count)
    {
        return d_net_io_result_make(0u,
                                    D_NET_OK);
    }

    const struct d_tests_pop_step* const step = &script->steps[script->at];
    const size_t                         size = strlen(step->bytes);
    const size_t                         left = size - script->offset;
    const size_t                         take = (_size < left) ? _size
                                                               : left;

    memcpy(_buffer,
           step->bytes + script->offset,
           take);
    script->offset += take;

    // part of the step: its error waits for the rest
    if (script->offset < size)
    {
        return d_net_io_result_make(take,
                                    D_NET_OK);
    }

    script->at     += 1u;
    script->offset  = 0u;

    return d_net_io_result_make(take,
                                step->error);
}

/*
d_tests_pop_script_write
  A script is read, never written.
*/
static struct d_net_io_result
d_tests_pop_script_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    (void)_connection;
    (void)_data;
    (void)_size;

    return d_net_io_result_make(0u,
                                D_NET_ERROR_CLOSED);
}

/*
d_tests_pop_script_is_open
  A script stays open; its steps decide what each read reports.
*/
static bool
d_tests_pop_script_is_open(
    const struct d_net_connection* _connection
)
{
    (void)_connection;

    return true;
}

/*
d_tests_pop_script_close
  A script holds nothing to release.
*/
static void
d_tests_pop_script_close(
    struct d_net_connection* _connection
)
{
    (void)_connection;

    return;
}

/*
SCRIPT_OPERATIONS
  The required operations, and none of the optional ones.
*/
static const struct d_net_connection_vtable SCRIPT_OPERATIONS =
{
    d_tests_pop_script_read,
    d_tests_pop_script_write,
    d_tests_pop_script_is_open,
    d_tests_pop_script_close,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
d_tests_pop_script_init
  At the first step, with no reads made.
*/
void
d_tests_pop_script_init(
    struct d_tests_pop_script*     _script,
    const struct d_tests_pop_step* _steps,
    size_t                         _count
)
{
    (void)d_net_connection_init(&_script->base,
                                &SCRIPT_OPERATIONS);
    _script->steps  = _steps;
    _script->count  = _count;
    _script->at     = 0u;
    _script->offset = 0u;
    _script->reads  = 0u;

    return;
}

/*
d_tests_pop_command
  The canonical keyword is the formatter's to write; `keyword` stays empty.
*/
struct d_pop_command_line
d_tests_pop_command(
    enum d_pop_command _command,
    const char*        _argument
)
{
    struct d_pop_command_line line = { .command = _command };

    // one argument, or none
    if (_argument != NULL)
    {
        line.args[0].data   = _argument;
        line.args[0].length = strlen(_argument);
        line.arg_count      = 1u;
    }

    return line;
}

/*
d_tests_pop_response
  No response code: `code` is NONE and `code_text` empty.
*/
struct d_pop_response
d_tests_pop_response(
    enum d_pop_indicator _indicator,
    const char*          _text
)
{
    struct d_pop_response response = { .indicator = _indicator };

    response.text.data   = _text;
    response.text.length = strlen(_text);

    return response;
}

/*
d_tests_pop_capture_write
  Takes what fits, and no more.
*/
static size_t
d_tests_pop_capture_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_pop_capture* const capture =
        (struct d_tests_pop_capture*)_context;
    const size_t                      room    =
        sizeof(capture->data) - capture->length;
    const size_t                      take    = (_size < room) ? _size
                                                               : room;

    memcpy(capture->data + capture->length,
           _data,
           take);
    capture->length += take;

    return take;
}

/*
d_tests_pop_capture_sink
  The capture is the sink's context.
*/
struct d_pack_sink
d_tests_pop_capture_sink(
    struct d_tests_pop_capture* _capture
)
{
    const struct d_pack_sink sink = { d_tests_pop_capture_write,
                                      _capture };

    return sink;
}

/*
d_tests_pop_capture_is
  Length first, then the bytes.
*/
bool
d_tests_pop_capture_is(
    const struct d_tests_pop_capture* _capture,
    const char*                       _text
)
{
    const size_t length = strlen(_text);

    return ( (_capture->length == length) &&
             (memcmp(_capture->data,
                     _text,
                     length) == 0) );
}

/*
d_tests_pop_connection_write
  All of it, or nothing taken.
*/
static size_t
d_tests_pop_connection_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    return (d_net_write_all((struct d_net_connection*)_context,
                            _data,
                            _size,
                            NULL) == D_NET_OK) ? _size
                                               : 0u;
}

/*
d_tests_pop_connection_sink
  The connection is the sink's context.
*/
struct d_pack_sink
d_tests_pop_connection_sink(
    struct d_net_connection* _connection
)
{
    const struct d_pack_sink sink = { d_tests_pop_connection_write,
                                      _connection };

    return sink;
}

/*
d_tests_pop_text_is
  Length first, then the bytes.
*/
bool
d_tests_pop_text_is(
    struct d_pack_text _text,
    const char*        _expected
)
{
    const size_t length = strlen(_expected);

    return ( (_text.length == length) &&
             ( (length == 0u) ||
               (memcmp(_text.data,
                       _expected,
                       length) == 0) ) );
}
