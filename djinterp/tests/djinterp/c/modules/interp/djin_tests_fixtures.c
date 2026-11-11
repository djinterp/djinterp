/*******************************************************************************
* djinterp [test]                                          djin_tests_fixtures.c
*
*   The suite's fixtures (djin_tests_fixtures.h).
*
*
* path:      /tests/djinterp/c/modules/interp/djin_tests_fixtures.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
// std
#include <string.h>  // memset
// djinterp
#include "./djin_tests_fixtures.h"  // the fixtures


// D_TESTS_DJIN_FS, D_TESTS_DJIN_STRICT
//   constant: two knobs the awk rows take their defaults from.
#define D_TESTS_DJIN_FS     " "
#define D_TESTS_DJIN_STRICT 1

// d_tests_djin_modes
//   constant: the names of the awk rows' choice.
static const struct dj_option_env_name d_tests_djin_modes[] =
{
    { "posix", 1 },
    { "gawk",  2 }
};

const struct dj_option_env_var d_tests_djin_awk_rows[4] =
{
    D_OPTION_ENV_TEXT_KNOB("fs", D_TESTS_DJIN_FS, "the field separator"),
    D_OPTION_ENV_BOOL_KNOB("strict", D_TESTS_DJIN_STRICT,
                           "refuse what POSIX leaves out"),
    D_OPTION_ENV_CHOICE("mode", d_tests_djin_modes, 2, "the dialect"),
    D_OPTION_ENV_INT_RANGE("depth", 64, 1, 4096, NULL)
};

const struct dj_option_env_var d_tests_djin_printer_rows[1] =
{
    D_OPTION_ENV_TEXT("style", NULL, "how values are shown")
};

const char* const d_tests_djin_reserved[] =
{
    "list", "describe", "get", "set", "help", NULL
};

const struct dj_djin_command d_tests_djin_awk_commands[2] =
{
    D_DJIN_COMMAND("run",      "run an awk program over the input",
                   NULL, NULL),
    D_DJIN_COMMAND("dump-ast", "print the program's tree", NULL, NULL)
};

/*
d_tests_djin_frame_make
  A view over the room, named, with the rows declared.
*/
bool
d_tests_djin_frame_make(
    struct dj_option_env*           _frame,
    struct d_tests_djin_room*       _room,
    const char*                     _name,
    const struct dj_option_env_var* _rows,
    uint32_t                        _count
)
{
    struct dj_option_result result;

    memset(_room, 0, sizeof(*_room));

    result = D_OPTION_ENV_VIEW(_frame, *_room);

    if (!result.is_ok)
    {
        return false;
    }

    result = d_option_env_set_name(_frame, _name);

    if (!result.is_ok)
    {
        return false;
    }

    result = d_option_env_declare_all(_frame, _rows, _count);

    return result.is_ok;
}
