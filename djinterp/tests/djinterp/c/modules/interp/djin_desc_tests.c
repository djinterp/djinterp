/*******************************************************************************
* djinterp [c][interp]                                         djin_desc_tests.c
*
*   The djin describers of djin_desc.h, run through desc's JSON machine into
* an array, each JSON object held to the text it should be, whole: a record
* with commands and a describe slot of its own, a record with neither, one
* of its commands, a frame with its settings, and the null arguments.
*   Built and run by the djinterp program's leaf, which links what they
* write through; the djin suite's leaf, which globs djin_tests*.c, does not
* take this file.
*
*
* path:      /tests/djinterp/c/modules/interp/djin_desc_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/

// std
#include <stddef.h>  // size_t
#include <string.h>  // strcmp, strlen, strncmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/desc/desc_machines.h"  // JSON
#include "../../../../../inc/djinterp/c/modules/interp/djin_desc.h"  // under test
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
#include "../../../../../inc/djinterp/tools/cli/c/cli_settings.h"  // a frame


//==============================================================================
// 1.  FIXTURES
//==============================================================================

// D_TESTS_DJIN_DESC_ROOM
//   constant: the room a description is written into.
#define D_TESTS_DJIN_DESC_ROOM 2048u

// d_tests_djin_desc_room
//   variable: where a description is written.
static char d_tests_djin_desc_room[D_TESTS_DJIN_DESC_ROOM];

// d_tests_djin_desc_commands
//   constant: two commands, the second with no summary.
static const struct dj_djin_command d_tests_djin_desc_commands[] =
{
    D_DJIN_COMMAND("run", "run a program", NULL, NULL),
    D_DJIN_COMMAND("check", NULL, NULL, NULL)
};

/*
d_tests_djin_desc_own
  A describe slot of a djin's own: one fact of state.
*/
static int
d_tests_djin_desc_own(
    const struct dj_djin* _djin,
    struct dj_desc_sink*  _sink
)
{
    (void)_djin;

    return d_desc_emit(_sink, "loaded", NULL, d_ostream_value_bool(1),
                       (uint32_t)D_DESC_LAYER_STATE, 0u,
                       (uint32_t)D_DESC_TRUTH_OBSERVED);
}

/*
d_tests_djin_desc_json
  An object as compact JSON of the describe shape, into the room; the
describer's result.
*/
static int
d_tests_djin_desc_json(
    fn_desc_describer _describer,
    const void*       _object
)
{
    struct dj_ostream         os;
    struct dj_ostream_fixed   fixed;
    struct dj_desc_json_state state;
    struct dj_desc_sink       sink;
    int                       result;

    d_tests_djin_desc_room[0] = '\0';
    (void)d_ostream_init_fixed(&os, &fixed, d_tests_djin_desc_room,
                               sizeof(d_tests_djin_desc_room), 1);
    d_desc_json_init(&state, &os, 0u);
    d_desc_json_shape(&state, D_DJIN_DESC_SHAPE);
    d_desc_sink_init(&sink, d_desc_json_machine(), &state);
    result = d_desc_run(&sink, _describer, _object);
    (void)d_ostream_close(&os);

    return result;
}


//==============================================================================
// 2.  THE TESTS
//==============================================================================

/*
d_tests_djin_desc_record
  Identity, intent, both commands, and the slot's own fact last.
*/
static bool
d_tests_djin_desc_record(void)
{
    struct dj_djin djin = D_DJIN("awk", "0.9.0", "the awk machine",
                                 d_tests_djin_desc_commands, 2u, NULL);

    djin.describe = d_tests_djin_desc_own;

    D_TEST_EXPECT(d_tests_djin_desc_json(d_djin_describer, &djin) == 0);

    return D_TEST_EXPECT_TEXT(
        d_test_text_cstr(d_tests_djin_desc_room),
        d_test_text_cstr("{\"format\":\"djinterp.describe/1.0\","
                         "\"name\":\"awk\",\"version\":\"0.9.0\","
                         "\"summary\":\"the awk machine\","
                         "\"commands\":{\"run\":{\"summary\":"
                         "\"run a program\"},\"check\":{}},"
                         "\"loaded\":true}"));
}

/*
d_tests_djin_desc_bare
  A record of a name alone: no version, summary, commands or settings.
*/
static bool
d_tests_djin_desc_bare(void)
{
    const struct dj_djin djin = D_DJIN("bare", NULL, NULL, NULL, 0u, NULL);

    D_TEST_EXPECT(d_tests_djin_desc_json(d_djin_describer, &djin) == 0);

    return D_TEST_EXPECT_TEXT(
        d_test_text_cstr(d_tests_djin_desc_room),
        d_test_text_cstr("{\"format\":\"djinterp.describe/1.0\","
                         "\"name\":\"bare\"}"));
}

/*
d_tests_djin_desc_command
  One command: its djin, its name, its summary.
*/
static bool
d_tests_djin_desc_command(void)
{
    const struct dj_djin      djin = D_DJIN("awk", "0.9.0", NULL,
                                            d_tests_djin_desc_commands, 2u,
                                            NULL);
    struct dj_djin_command_of of;

    of.djin    = &djin;
    of.command = &d_tests_djin_desc_commands[0];

    D_TEST_EXPECT(d_tests_djin_desc_json(d_djin_command_describer,
                                         &of) == 0);

    return D_TEST_EXPECT_TEXT(
        d_test_text_cstr(d_tests_djin_desc_room),
        d_test_text_cstr("{\"format\":\"djinterp.describe/1.0\","
                         "\"djin\":\"awk\",\"name\":\"run\","
                         "\"summary\":\"run a program\"}"));
}

/*
d_tests_djin_desc_frame
  A frame's name, then its settings, each a level of the option core's: the
command-line kit's frame, `cli`, at its defaults.
*/
static bool
d_tests_djin_desc_frame(void)
{
    static const char head[] = "{\"format\":\"djinterp.describe/1.0\","
                               "\"name\":\"cli\",\"settings\":{";
    const struct dj_option_env* const frame = d_cli_settings_djin();
    const char*                       json  = d_tests_djin_desc_room;
    const size_t                      size  = sizeof(head) - 1u;

    D_TEST_EXPECT(frame != NULL);
    D_TEST_EXPECT(d_tests_djin_desc_json(d_djin_frame_describer, frame) == 0);
    D_TEST_EXPECT(strncmp(json, head, size) == 0);
    D_TEST_EXPECT(strstr(json, "\"plugin_strict_version\":{\"value\":"
                               "true,\"source\":\"default\"") != NULL);

    return D_TEST_EXPECT(strcmp(json + strlen(json) - 2u, "}}") == 0);
}

/*
d_tests_djin_desc_nulls
  Each describer given nothing, or a sink of nothing: -1, nothing written.
*/
static bool
d_tests_djin_desc_nulls(void)
{
    struct dj_desc_sink sink;
    struct dj_djin      djin = D_DJIN("awk", NULL, NULL, NULL, 0u, NULL);

    d_desc_sink_init(&sink, NULL, NULL);

    D_TEST_EXPECT(d_djin_describer(NULL, &sink) == -1);
    D_TEST_EXPECT(d_djin_describer(&djin, NULL) == -1);
    D_TEST_EXPECT(d_djin_frame_describer(NULL, &sink) == -1);
    D_TEST_EXPECT(d_djin_command_describer(NULL, &sink) == -1);

    return true;
}


//==============================================================================
// 3.  THE RUNNER
//==============================================================================

// d_tests_djin_desc_cases
//   constant: the tests, in the order they run.
static const struct dj_test_case_spec d_tests_djin_desc_cases[] =
{
    { "djin_desc_record",
      "a record's identity, commands and own facts",
      d_tests_djin_desc_record },
    { "djin_desc_bare",
      "a record of a name alone",
      d_tests_djin_desc_bare },
    { "djin_desc_command",
      "one of a djin's commands",
      d_tests_djin_desc_command },
    { "djin_desc_frame",
      "a frame's name and settings",
      d_tests_djin_desc_frame },
    { "djin_desc_nulls",
      "every describer given nothing",
      d_tests_djin_desc_nulls }
};

/*
main
  The tests, as one module of one block.
*/
int main(
    int    _argc,
    char** _argv
)
{
    const struct dj_test_block_spec blocks[] =
    {
        { "describers",
          "a djin, a command and a frame as desc's facts (djin_desc.c)",
          d_tests_djin_desc_cases,
          sizeof(d_tests_djin_desc_cases) / sizeof(d_tests_djin_desc_cases[0]) }
    };
    const struct dj_test_module_spec module =
    {
        "djin_desc",
        "the djin describers",
        blocks,
        1u
    };
    const struct dj_test_module_spec* const modules[] = { &module };

    return d_test_run_modules(_argc, _argv, "djinterp djin describers",
                              modules, 1u);
}
