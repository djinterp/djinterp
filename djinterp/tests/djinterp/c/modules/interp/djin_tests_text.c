/*******************************************************************************
* djinterp [test]                                              djin_tests_text.c
*
*   The text's tests: the list, a djin's description and a frame's, each to
* the byte; a full name and a default too long to show, cut short; and every
* writer's refusal of a sink that takes less than it is given.
*
*
* path:      /tests/djinterp/c/modules/interp/djin_tests_text.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
// std
#include <string.h>  // memcmp, memcpy, memset, strlen
// djinterp
#include "./djin_tests.h"           // the suite's tests
#include "./djin_tests_fixtures.h"  // the fixtures


// D_TESTS_DJIN_TEXT_ROOM
//   constant: the room a test's text is written into.
#define D_TESTS_DJIN_TEXT_ROOM 2048u

// d_tests_djin_world
//   struct: what the text tests write about: a registry with the djins
// "awk" (commands and settings) and "bare" (neither), and the frame
// "printer", which no djin names.
struct d_tests_djin_world
{
    struct dj_option_env_registry registry;
    struct d_tests_djin_room      rooms[2];
    struct dj_option_env          awk_frame;
    struct dj_option_env          printer_frame;
    struct dj_djin                awk;
    struct dj_djin                bare;
    const struct dj_djin*         slots[4];
    struct dj_djin_set            set;
};

/*
d_tests_djin_world_make
  The world, registered; whether every step went as it should.
*/
static bool
d_tests_djin_world_make(
    struct d_tests_djin_world* _world
)
{
    const struct dj_djin awk  = D_DJIN("awk", "0.9.0", "the awk machine",
                                       d_tests_djin_awk_commands, 2, NULL);
    const struct dj_djin bare = D_DJIN("bare", "1", "nothing", NULL, 0,
                                       NULL);

    _world->awk  = awk;
    _world->bare = bare;
    _world->awk.settings = &_world->awk_frame;

    return ( (d_option_env_registry_init(&_world->registry,
                                         "djinterp").is_ok)              &&
             (d_tests_djin_frame_make(&_world->awk_frame, &_world->rooms[0],
                                      "awk", d_tests_djin_awk_rows, 4))  &&
             (d_tests_djin_frame_make(&_world->printer_frame,
                                      &_world->rooms[1], "printer",
                                      d_tests_djin_printer_rows, 1))     &&
             (d_djin_set_init(&_world->set, _world->slots, 4,
                              &_world->registry,
                              d_tests_djin_reserved) == D_DJIN_STATUS_OK) &&
             (d_djin_register(&_world->set, &_world->awk) ==
                  D_DJIN_STATUS_OK)                                      &&
             (d_djin_register(&_world->set, &_world->bare) ==
                  D_DJIN_STATUS_OK)                                      &&
             (d_option_env_registry_add(&_world->registry,
                                        &_world->printer_frame).is_ok) );
}

/*
d_tests_djin_same
  Whether the buffer holds exactly `_expected`; a note shows both when not.
*/
static bool
d_tests_djin_same(
    const char*                       _what,
    const struct dj_pack_buffer_sink* _buffer,
    const char*                       _expected
)
{
    size_t length = strlen(_expected);
    bool   same   = ( (!_buffer->overflow)         &&
                      (_buffer->written == length) &&
                      (memcmp(_buffer->buffer, _expected, length) == 0) );

    if (!same)
    {
        D_TEST_NOTE("%s: wrote\n%.*s\nwanted\n%s", _what,
                    (int)_buffer->written, (const char*)_buffer->buffer,
                    _expected);
    }

    return same;
}

/*
d_tests_djin_budget_write
  A sink's write that takes at most what is left of a budget, so that it
answers short; the buffer sink records an overflow instead.
*/
static size_t
d_tests_djin_budget_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    size_t* left = (size_t*)_context;
    size_t  take = (_size < *left) ? _size : *left;

    (void)_data;
    *left -= take;

    return take;
}

/*
d_tests_djin_write_list
  Tests the following:
  - the djins, a line each in the order registered, then the frame no djin
    names, as "settings only"
  - each column as wide as its widest entry, the last unpadded
*/
bool
d_tests_djin_write_list(
    struct dj_test_suite* _suite
)
{
    static const char expected[] =
        "awk      0.9.0  the awk machine\n"
        "bare     1      nothing\n"
        "printer  -      settings only\n";
    struct d_tests_djin_world  world;
    struct dj_pack_buffer_sink buffer;
    unsigned char              text[D_TESTS_DJIN_TEXT_ROOM];
    bool                       made;

    d_test_suite_test(_suite, "write_list");
    d_test_suite_describe(_suite,
                          "the list, to the byte");

    made = d_tests_djin_world_make(&world);
    d_pack_buffer_sink_init(&buffer, text, sizeof(text));

    return D_TEST_CHECK_MSG(_suite, "write_list",
                            (made) &&
                            (d_djin_write_list(&world.set,
                                 d_pack_sink_from_buffer(&buffer)) == 0) &&
                            (d_tests_djin_same("list", &buffer, expected)),
                            "the list, to the byte");
}

/*
d_tests_djin_write_description
  Tests the following:
  - a djin's heading, summary, commands and settings, to the byte: each
    setting's full name, kind, default, knob and help
  - a setting with no help ends at its knob
  - a frame no djin names is described as settings only
*/
bool
d_tests_djin_write_description(
    struct dj_test_suite* _suite
)
{
    static const char awk[] =
        "awk  0.9.0\n"
        "  the awk machine\n"
        "\n"
        "commands\n"
        "  run       run an awk program over the input\n"
        "  dump-ast  print the program's tree\n"
        "\n"
        "settings\n"
        "  awk.fs      text    \" \"   D_TESTS_DJIN_FS      "
            "the field separator\n"
        "  awk.strict  bool    true  D_TESTS_DJIN_STRICT  "
            "refuse what POSIX leaves out\n"
        "  awk.mode    choice  gawk  -                    the dialect\n"
        "  awk.depth   int     64    -\n";
    static const char printer[] =
        "printer\n"
        "  settings only\n"
        "\n"
        "settings\n"
        "  printer.style  text  unset  -  how values are shown\n";
    struct d_tests_djin_world  world;
    struct dj_pack_buffer_sink buffer;
    unsigned char              text[D_TESTS_DJIN_TEXT_ROOM];
    bool                       made;
    bool                       result;

    d_test_suite_test(_suite, "write_description");
    d_test_suite_describe(_suite,
                          "a djin's description and a frame's");

    result = true;
    made   = d_tests_djin_world_make(&world);

    d_pack_buffer_sink_init(&buffer, text, sizeof(text));
    result = D_TEST_CHECK_MSG(_suite, "write_description",
                              (made) &&
                              (d_djin_write_description(&world.awk,
                                   d_pack_sink_from_buffer(&buffer)) == 0) &&
                              (d_tests_djin_same("awk", &buffer, awk)),
                              "a djin's description, to the byte") && result;

    d_pack_buffer_sink_init(&buffer, text, sizeof(text));
    result = D_TEST_CHECK_MSG(_suite, "write_frame",
                              (made) &&
                              (d_djin_write_frame(&world.printer_frame,
                                   d_pack_sink_from_buffer(&buffer)) == 0) &&
                              (d_tests_djin_same("printer", &buffer,
                                                 printer)),
                              "a frame's description, to the byte") &&
             result;

    return result;
}

// d_tests_djin_flaky
//   struct: a sink's context that refuses one write, the one at `refuse`
// counting from 0, and takes every other whole; `calls` counts them.
struct d_tests_djin_flaky
{
    size_t calls;
    size_t refuse;
};

/*
d_tests_djin_flaky_write
  A sink's write that refuses the one write its context names.
*/
static size_t
d_tests_djin_flaky_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_djin_flaky* flaky = (struct d_tests_djin_flaky*)_context;
    size_t                     call  = flaky->calls++;

    (void)_data;

    return (call == flaky->refuse) ? 0 : _size;
}

/*
d_tests_djin_write_which
  Writes with the writer `_which` names: 0 the list, 1 the djin's
description, 2 the frame's.
*/
static int
d_tests_djin_write_which(
    struct d_tests_djin_world* _world,
    int                        _which,
    struct dj_pack_sink        _sink
)
{
    switch (_which)
    {
        case 0:
            return d_djin_write_list(&_world->set, _sink);
        case 1:
            return d_djin_write_description(&_world->awk, _sink);
        default:
            return d_djin_write_frame(&_world->printer_frame, _sink);
    }
}

/*
d_tests_djin_one_refused
  Whether the writer `_which` names answers 0 when none of its writes is
refused, and -1 when any one is, each in turn.
*/
static bool
d_tests_djin_one_refused(
    struct d_tests_djin_world* _world,
    int                        _which
)
{
    struct d_tests_djin_flaky flaky;
    struct dj_pack_sink       sink;
    size_t                    writes;
    size_t                    i;
    bool                      held;

    sink.write   = d_tests_djin_flaky_write;
    sink.context = &flaky;

    // a run that refuses nothing answers 0, and counts the writes
    flaky.calls  = 0;
    flaky.refuse = (size_t)-1;
    held         = (d_tests_djin_write_which(_world, _which, sink) == 0);
    writes       = flaky.calls;

    for (i = 0; (held) && (i < writes); ++i)
    {
        flaky.calls  = 0;
        flaky.refuse = i;
        held         = (d_tests_djin_write_which(_world, _which, sink) ==
                            -1);
    }

    return (held) && (writes > 0);
}

/*
d_tests_djin_append
  Appends `_text` to `_to` at `_at`, or, for a `_times` past 0, its first
character that many times; keeps `_to` NUL-terminated.
*/
static void
d_tests_djin_append(
    char*       _to,
    size_t*     _at,
    const char* _text,
    size_t      _times
)
{
    size_t length = (_times > 0) ? _times : strlen(_text);

    if (_times > 0)
    {
        memset(_to + *_at, _text[0], length);
    }
    else
    {
        memcpy(_to + *_at, _text, length);
    }

    *_at     += length;
    _to[*_at] = '\0';

    return;
}

/*
d_tests_djin_write_long
  Tests the following:
  - a full name longer than the room shows as the room's 127 characters,
    ending in "..."
  - a text default longer than the room shows as 95 characters, its quote
    and 91 of its own, ending in "..."
*/
bool
d_tests_djin_write_long(
    struct dj_test_suite* _suite
)
{
    char                       row_name[131];
    char                       row_text[121];
    char                       expected[D_TESTS_DJIN_TEXT_ROOM];
    struct dj_option_env_var   rows[1];
    struct dj_option_env       frame;
    struct d_tests_djin_room   room;
    struct dj_pack_buffer_sink buffer;
    unsigned char              text[D_TESTS_DJIN_TEXT_ROOM];
    size_t                     at;
    bool                       made;

    d_test_suite_test(_suite, "write_long");
    d_test_suite_describe(_suite,
                          "a name and a default too long, cut short");

    memset(row_name, 'n', sizeof(row_name) - 1);
    row_name[sizeof(row_name) - 1] = '\0';
    memset(row_text, 'v', sizeof(row_text) - 1);
    row_text[sizeof(row_text) - 1] = '\0';

    {
        const struct dj_option_env_var row =
            D_OPTION_ENV_TEXT(row_name, row_text, "h");

        rows[0] = row;
    }

    // "printer." and 116 of the name's characters, then "..."; the
    // default's quote and 91 of its characters, then "..."; the knob; the
    // help
    at = 0;
    d_tests_djin_append(expected, &at, "printer\n  settings only\n\n", 0);
    d_tests_djin_append(expected, &at, "settings\n  printer.", 0);
    d_tests_djin_append(expected, &at, "n", 116);
    d_tests_djin_append(expected, &at, "...  text  \"", 0);
    d_tests_djin_append(expected, &at, "v", 91);
    d_tests_djin_append(expected, &at, "...  -  h\n", 0);

    made = d_tests_djin_frame_make(&frame, &room, "printer", rows, 1);

    d_pack_buffer_sink_init(&buffer, text, sizeof(text));

    return D_TEST_CHECK_MSG(_suite, "write_long",
                            (made) &&
                            (d_djin_write_frame(&frame,
                                 d_pack_sink_from_buffer(&buffer)) == 0) &&
                            (d_tests_djin_same("long", &buffer, expected)),
                            "a name and a default too long, cut short");
}

/*
d_tests_djin_write_refused
  Tests the following:
  - each writer answers -1 to a sink that takes less than it is given, at
    every length short of the whole text
  - each answers -1 when any one of its writes is refused and the rest are
    taken, and 0 when none is
  - each answers -1 to what it cannot write about: no set, no djin, a frame
    with no name
*/
bool
d_tests_djin_write_refused(
    struct dj_test_suite* _suite
)
{
    struct d_tests_djin_world  world;
    struct dj_pack_buffer_sink buffer;
    unsigned char              text[D_TESTS_DJIN_TEXT_ROOM];
    struct dj_option_env       nameless;
    struct d_tests_djin_room   room;
    size_t                     room_given;
    size_t                     whole;
    bool                       made;
    bool                       short_refused;
    bool                       result;

    d_test_suite_test(_suite, "write_refused");
    d_test_suite_describe(_suite,
                          "a short sink, a refused write, nothing to write");

    result        = true;
    made          = d_tests_djin_world_make(&world);
    short_refused = made;

    // a sink that takes less than the whole list, at every length, refuses
    // it, and the description, which is longer
    d_pack_buffer_sink_init(&buffer, text, sizeof(text));
    (void)d_djin_write_list(&world.set, d_pack_sink_from_buffer(&buffer));
    whole = buffer.written;

    for (room_given = 0; room_given < whole; ++room_given)
    {
        size_t              left;
        struct dj_pack_sink budget;

        budget.write   = d_tests_djin_budget_write;
        budget.context = &left;

        left          = room_given;
        short_refused = (d_djin_write_list(&world.set, budget) == -1) &&
                        short_refused;
        left          = room_given;
        short_refused = (d_djin_write_description(&world.awk, budget) ==
                             -1) &&
                        short_refused;
    }

    result = D_TEST_CHECK_MSG(_suite, "write_short_sink", short_refused,
                              "a sink with too little room is refused") &&
             result;

    result = D_TEST_CHECK_MSG(_suite, "write_one_refused",
                              (made) &&
                              (d_tests_djin_one_refused(&world, 0)) &&
                              (d_tests_djin_one_refused(&world, 1)) &&
                              (d_tests_djin_one_refused(&world, 2)),
                              "any one write refused is answered -1") &&
             result;

    memset(&room, 0, sizeof(room));
    (void)D_OPTION_ENV_VIEW(&nameless, room);
    d_pack_buffer_sink_init(&buffer, text, sizeof(text));
    result = D_TEST_CHECK_MSG(_suite, "write_nothing",
                              (d_djin_write_list(NULL,
                                   d_pack_sink_from_buffer(&buffer)) == -1) &&
                              (d_djin_write_description(NULL,
                                   d_pack_sink_from_buffer(&buffer)) == -1) &&
                              (d_djin_write_frame(&nameless,
                                   d_pack_sink_from_buffer(&buffer)) == -1),
                              "nothing to write about is refused") && result;

    return result;
}

/*
d_tests_djin_text_all
  Section II's aggregation.
*/
bool
d_tests_djin_text_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text");

    result = true;

    result = d_tests_djin_write_list(_suite)        && result;
    result = d_tests_djin_write_description(_suite) && result;
    result = d_tests_djin_write_long(_suite)        && result;
    result = d_tests_djin_write_refused(_suite)     && result;

    return result;
}
