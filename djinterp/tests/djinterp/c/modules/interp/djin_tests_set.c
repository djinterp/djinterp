/*******************************************************************************
* djinterp [test]                                               djin_tests_set.c
*
*   The set's tests: making one, registering a djin and its settings, every
* record the set refuses -- with nothing changed by a refusal -- and finding
* a djin and a command by name.
*
*
* path:      /tests/djinterp/c/modules/interp/djin_tests_set.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
// std
#include <string.h>  // strlen
// djinterp
#include "./djin_tests.h"           // the suite's tests
#include "./djin_tests_fixtures.h"  // the fixtures


/*
d_tests_djin_set_init
  Tests the following:
  - a null set, or null room with a capacity, is INVALID
  - a set is made empty, with no room or registry too
*/
bool
d_tests_djin_set_init(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_djin_set    set;
    const struct dj_djin* room[2];

    d_test_suite_test(_suite, "set_init");
    d_test_suite_describe(_suite,
                          "an empty set over the room; its refusals");

    result = true;

    result = D_TEST_CHECK_MSG(_suite, "set_init_null_set",
                              d_djin_set_init(NULL, room, 2, NULL, NULL) ==
                                  D_DJIN_STATUS_INVALID,
                              "a null set is INVALID") && result;
    result = D_TEST_CHECK_MSG(_suite, "set_init_null_room",
                              d_djin_set_init(&set, NULL, 2, NULL, NULL) ==
                                  D_DJIN_STATUS_INVALID,
                              "null room with a capacity is INVALID") &&
             result;
    result = D_TEST_CHECK_MSG(_suite, "set_init_empty",
                              (d_djin_set_init(&set, NULL, 0, NULL, NULL) ==
                                   D_DJIN_STATUS_OK) &&
                              (d_djin_set_count(&set) == 0) &&
                              (d_djin_set_at(&set, 0) == NULL),
                              "no room is an empty set") && result;
    result = D_TEST_CHECK_MSG(_suite, "set_init_room",
                              (d_djin_set_init(&set, room, 2, NULL, NULL) ==
                                   D_DJIN_STATUS_OK) &&
                              (set.capacity == 2) &&
                              (d_djin_set_count(&set) == 0),
                              "a set over room starts empty") && result;

    return result;
}

/*
d_tests_djin_register
  Tests the following:
  - a djin with commands and settings registers, and is listed
  - its frame is then the registry's djin of its name
  - a djin with neither registers without a registry
*/
bool
d_tests_djin_register(
    struct dj_test_suite* _suite
)
{
    bool                          result;
    struct dj_option_env_registry registry;
    struct d_tests_djin_room      room;
    struct dj_option_env          frame;
    struct dj_djin_set            set;
    const struct dj_djin*         slots[4];
    struct dj_djin                awk = D_DJIN("awk", "0.9.0",
                                               "the awk machine",
                                               d_tests_djin_awk_commands, 2,
                                               NULL);
    const struct dj_djin          bare = D_DJIN("bare", "1", "nothing", NULL,
                                                0, NULL);

    d_test_suite_test(_suite, "register");
    d_test_suite_describe(_suite,
                          "djins listed, their frames in the registry");

    result = true;
    awk.settings = &frame;

    (void)d_option_env_registry_init(&registry, "djinterp");
    result = D_TEST_CHECK_MSG(_suite, "register_fixture",
                              d_tests_djin_frame_make(&frame, &room, "awk",
                                                      d_tests_djin_awk_rows,
                                                      4),
                              "the awk frame is made") && result;
    (void)d_djin_set_init(&set, slots, 4, &registry, d_tests_djin_reserved);

    result = D_TEST_CHECK_MSG(_suite, "register_ok",
                              d_djin_register(&set, &awk) ==
                                  D_DJIN_STATUS_OK,
                              "a djin in shape registers") && result;
    result = D_TEST_CHECK_MSG(_suite, "register_listed",
                              (d_djin_set_count(&set) == 1) &&
                              (d_djin_set_at(&set, 0) == &awk),
                              "it is listed first") && result;
    result = D_TEST_CHECK_MSG(_suite, "register_frame",
                              d_option_env_registry_find(&registry, "awk",
                                                         3) == &frame,
                              "its frame is the registry's \"awk\"") &&
             result;

    (void)d_djin_set_init(&set, slots, 4, NULL, NULL);
    result = D_TEST_CHECK_MSG(_suite, "register_bare",
                              (d_djin_register(&set, &bare) ==
                                   D_DJIN_STATUS_OK) &&
                              (d_djin_set_count(&set) == 1),
                              "a djin without settings needs no registry") &&
             result;

    return result;
}

/*
d_tests_djin_refused
  Registers `_djin` into a fresh set, expecting `_status`, and checks that
the refusal left the set and the registry as they were.
*/
static bool
d_tests_djin_refused(
    struct dj_test_suite* _suite,
    const char*           _name,
    const struct dj_djin* _djin,
    int32_t               _status
)
{
    struct dj_option_env_registry registry;
    struct dj_djin_set            set;
    const struct dj_djin*         slots[2];
    int32_t                       got;

    (void)d_option_env_registry_init(&registry, "djinterp");
    (void)d_djin_set_init(&set, slots, 2, &registry, d_tests_djin_reserved);

    got = d_djin_register(&set, _djin);

    if (got != _status)
    {
        D_TEST_NOTE("%s: %s, wanted %s", _name, d_djin_status_name(got),
                    d_djin_status_name(_status));
    }

    return D_TEST_CHECK_MSG(_suite, _name,
                            (got == _status) &&
                            (d_djin_set_count(&set) == 0) &&
                            (d_option_env_registry_count(&registry) == 0),
                            "refused, and nothing changed");
}

/*
d_tests_djin_register_refused
  Tests the following:
  - a null djin, an old ABI, a short record, and names, versions and
    summaries out of shape are INVALID
  - commands out of shape, or a name twice, are INVALID
  - a settings frame named otherwise than the djin is INVALID
  - a program's own name is RESERVED
*/
bool
d_tests_djin_register_refused(
    struct dj_test_suite* _suite
)
{
    static const struct dj_djin_command twice[2] =
    {
        D_DJIN_COMMAND("run", "a", NULL, NULL),
        D_DJIN_COMMAND("run", "b", NULL, NULL)
    };
    static const struct dj_djin_command badly[1] =
    {
        D_DJIN_COMMAND("Run", "a", NULL, NULL)
    };
    struct d_tests_djin_room room;
    struct dj_option_env     frame;
    struct dj_djin           djin = D_DJIN("awk", "1", "s", NULL, 0, NULL);
    bool                     result;

    d_test_suite_test(_suite, "register_refused");
    d_test_suite_describe(_suite,
                          "each refusal, with nothing changed");

    result = true;
    result = d_tests_djin_refused(_suite, "refused_null", NULL,
                                  D_DJIN_STATUS_INVALID) && result;

    djin.abi = D_DJIN_ABI + 1u;
    result = d_tests_djin_refused(_suite, "refused_abi", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.abi  = D_DJIN_ABI;
    djin.size = 4u;
    result = d_tests_djin_refused(_suite, "refused_size", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.size = (uint32_t)sizeof(struct dj_djin);

    djin.name = "Awk";
    result = d_tests_djin_refused(_suite, "refused_upper", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.name = "dump-ast";
    result = d_tests_djin_refused(_suite, "refused_dash", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.name = "awk";

    djin.version = NULL;
    result = d_tests_djin_refused(_suite, "refused_version", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.version = "1";
    djin.summary = "two\nlines";
    result = d_tests_djin_refused(_suite, "refused_summary", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.summary = "s";

    djin.command_count = 1;
    result = d_tests_djin_refused(_suite, "refused_no_commands", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.commands = badly;
    result = d_tests_djin_refused(_suite, "refused_command_name", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.commands      = twice;
    djin.command_count = 2;
    result = d_tests_djin_refused(_suite, "refused_command_twice", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.commands      = NULL;
    djin.command_count = 0;

    (void)d_tests_djin_frame_make(&frame, &room, "printer",
                                  d_tests_djin_printer_rows, 1);
    djin.settings = &frame;
    result = d_tests_djin_refused(_suite, "refused_frame_name", &djin,
                                  D_DJIN_STATUS_INVALID) && result;
    djin.settings = NULL;

    djin.name = "list";
    result = d_tests_djin_refused(_suite, "refused_reserved", &djin,
                                  D_DJIN_STATUS_RESERVED) && result;

    return result;
}

/*
d_tests_djin_register_settings
  Tests the following:
  - a frame already in the registry, as a module's static frame is, is
    taken as it is
  - another frame of the djin's name in the registry is a DUPLICATE
  - a second djin of a name is a DUPLICATE, and a full set is FULL, with
    neither frame added
  - settings with no registry to put them in are SETTINGS
*/
bool
d_tests_djin_register_settings(
    struct dj_test_suite* _suite
)
{
    bool                          result;
    struct dj_option_env_registry registry;
    struct d_tests_djin_room      rooms[3];
    struct dj_option_env          frames[3];
    struct dj_djin_set            set;
    const struct dj_djin*         slots[1];
    struct dj_djin                first  = D_DJIN("awk", "1", "s", NULL, 0,
                                                  NULL);
    struct dj_djin                second = D_DJIN("awk", "2", "s", NULL, 0,
                                                  NULL);
    struct dj_djin                other  = D_DJIN("printer", "1", "s", NULL,
                                                  0, NULL);

    d_test_suite_test(_suite, "register_settings");
    d_test_suite_describe(_suite,
                          "a frame there already, another of its name");

    result = true;
    (void)d_tests_djin_frame_make(&frames[0], &rooms[0], "awk",
                                  d_tests_djin_awk_rows, 4);
    (void)d_tests_djin_frame_make(&frames[1], &rooms[1], "awk",
                                  d_tests_djin_awk_rows, 4);
    (void)d_tests_djin_frame_make(&frames[2], &rooms[2], "printer",
                                  d_tests_djin_printer_rows, 1);

    // a module registered its frame before the program made its set
    (void)d_option_env_registry_init(&registry, "djinterp");
    (void)d_option_env_registry_add(&registry, &frames[0]);
    (void)d_djin_set_init(&set, slots, 1, &registry, NULL);

    second.settings = &frames[1];
    result = D_TEST_CHECK_MSG(_suite, "settings_other_frame",
                              (d_djin_register(&set, &second) ==
                                   D_DJIN_STATUS_DUPLICATE) &&
                              (d_djin_set_count(&set) == 0),
                              "another frame of the name is a duplicate") &&
             result;

    first.settings = &frames[0];
    result = D_TEST_CHECK_MSG(_suite, "settings_present",
                              (d_djin_register(&set, &first) ==
                                   D_DJIN_STATUS_OK) &&
                              (d_option_env_registry_count(&registry) == 1),
                              "a frame already there is taken as it is") &&
             result;

    result = D_TEST_CHECK_MSG(_suite, "settings_duplicate_djin",
                              d_djin_register(&set, &first) ==
                                  D_DJIN_STATUS_DUPLICATE,
                              "a djin of the name is registered") && result;

    other.settings = &frames[2];
    result = D_TEST_CHECK_MSG(_suite, "settings_full",
                              (d_djin_register(&set, &other) ==
                                   D_DJIN_STATUS_FULL) &&
                              (d_option_env_registry_count(&registry) == 1),
                              "a full set adds no frame") && result;

    (void)d_djin_set_init(&set, slots, 1, NULL, NULL);
    result = D_TEST_CHECK_MSG(_suite, "settings_no_registry",
                              d_djin_register(&set, &other) ==
                                  D_DJIN_STATUS_SETTINGS,
                              "settings need a registry") && result;

    return result;
}

/*
d_tests_djin_find
  Tests the following:
  - a djin is found by a name that need not end in a NUL
  - a command is found by name, and an unknown one is not
  - the statuses have their names
*/
bool
d_tests_djin_find(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_djin_set    set;
    const struct dj_djin* slots[2];
    const struct dj_djin  awk = D_DJIN("awk", "1", "s",
                                       d_tests_djin_awk_commands, 2, NULL);

    d_test_suite_test(_suite, "find");
    d_test_suite_describe(_suite,
                          "djins by place and name, commands by name");

    result = true;
    (void)d_djin_set_init(&set, slots, 2, NULL, NULL);
    (void)d_djin_register(&set, &awk);

    result = D_TEST_CHECK_MSG(_suite, "find_prefix",
                              (d_djin_set_find(&set, "awkward", 3) == &awk) &&
                              (d_djin_set_find(&set, "awkward", 7) == NULL) &&
                              (d_djin_set_find(&set, "aw", 2) == NULL),
                              "found by its whole name only") && result;
    result = D_TEST_CHECK_MSG(_suite, "find_command",
                              (d_djin_command_find(&awk, "dump-ast", 8) ==
                                   &d_tests_djin_awk_commands[1]) &&
                              (d_djin_command_find(&awk, "walk", 4) == NULL),
                              "a command by name") && result;
    result = D_TEST_CHECK_MSG(_suite, "status_names",
                              (strcmp(d_djin_status_name(D_DJIN_STATUS_OK),
                                      "ok") == 0) &&
                              (strcmp(d_djin_status_name(
                                          D_DJIN_STATUS_SETTINGS),
                                      "settings") == 0) &&
                              (strcmp(d_djin_status_name(-1),
                                      "unknown") == 0),
                              "each status has its name") && result;

    return result;
}

/*
d_tests_djin_set_all
  Section I's aggregation.
*/
bool
d_tests_djin_set_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "set");

    result = true;

    result = d_tests_djin_set_init(_suite)          && result;
    result = d_tests_djin_register(_suite)          && result;
    result = d_tests_djin_register_refused(_suite)  && result;
    result = d_tests_djin_register_settings(_suite) && result;
    result = d_tests_djin_find(_suite)              && result;

    return result;
}
