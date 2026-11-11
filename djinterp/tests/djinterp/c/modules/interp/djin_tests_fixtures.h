/*******************************************************************************
* djinterp [test]                                          djin_tests_fixtures.h
*
*   The suite's fixtures: a frame's room, the schema rows two test djins
* declare, the program's reserved names, and a frame made from rows in its
* room. They are defined in djin_tests_fixtures.c.
*
*
* path:      /tests/djinterp/c/modules/interp/djin_tests_fixtures.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

#ifndef DJINTERP_TESTS_INTERP_DJIN_TESTS_FIXTURES_H
#define DJINTERP_TESTS_INTERP_DJIN_TESTS_FIXTURES_H 1

// djinterp
#include "./djin_tests.h"  // the suite, and djin.h


// d_tests_djin_room
//   struct: the room one test frame needs.
D_OPTION_ENV_DECLARE(d_tests_djin_room, 16, 8, 256);

// the rows of the djin "awk" (four) and of the frame "printer" (one)
extern const struct dj_option_env_var d_tests_djin_awk_rows[4];
extern const struct dj_option_env_var d_tests_djin_printer_rows[1];

// the program's own command names, ending in a null
extern const char* const d_tests_djin_reserved[];

// the djin "awk"'s two commands
extern const struct dj_djin_command d_tests_djin_awk_commands[2];

/**
 * @brief Makes a frame named `_name` in `_room`, declaring `_rows`.
 *
 * @return true when the frame was made.
 */
bool d_tests_djin_frame_make(
    struct dj_option_env*           _frame,
    struct d_tests_djin_room*       _room,
    const char*                     _name,
    const struct dj_option_env_var* _rows,
    uint32_t                        _count
);


#endif  // DJINTERP_TESTS_INTERP_DJIN_TESTS_FIXTURES_H
