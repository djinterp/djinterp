/*******************************************************************************
* djinterp [tests]                                          t_sqlite_extension.c
*
*   A loadable SQLite extension, for the harnesses' checks of extension
* loading: it adds one SQL function, d_tests_answer(), which returns 42. It
* has two entry points: SQLite's default, sqlite3_extension_init, which a
* load that names none finds, and d_tests_sqlite_extension_init, which also
* adds d_tests_named(), returning 7, for a load that names its own.
*   Build: as a shared library, against the SQLite whose sqlite3ext.h it
* includes; ci/check_db_sqlite.sh does, and hands its path to the harnesses
* in DJINTERP_TEST_SQLITE_EXTENSION.
*
*
* path:      /tests/djinterp/c/db/sqlite/t_sqlite_extension.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
// std
#include <stddef.h>  // NULL
// SQLite
#include <sqlite3ext.h>  // SQLITE_EXTENSION_INIT1, sqlite3_api_routines


SQLITE_EXTENSION_INIT1


// D_TESTS_SQLITE_EXTENSION_EXPORT
//   macro: what makes an entry point one the platform's loader finds by its
// name: Windows exports only what a library says to, where the library says
// anything.
#if defined(_WIN32)
    #define D_TESTS_SQLITE_EXTENSION_EXPORT __declspec(dllexport)
#else
    #define D_TESTS_SQLITE_EXTENSION_EXPORT
#endif


/*
d_tests_answer
  The SQL function d_tests_answer(): 42.
*/
static void
d_tests_answer(
    sqlite3_context* _context,
    int              _argc,
    sqlite3_value**  _argv
)
{
    (void)_argc;
    (void)_argv;

    sqlite3_result_int(_context, 42);

    return;
}

/*
d_tests_named
  The SQL function d_tests_named(): 7.
*/
static void
d_tests_named(
    sqlite3_context* _context,
    int              _argc,
    sqlite3_value**  _argv
)
{
    (void)_argc;
    (void)_argv;

    sqlite3_result_int(_context, 7);

    return;
}

/*
sqlite3_extension_init
  SQLite's default entry point: d_tests_answer().
*/
D_TESTS_SQLITE_EXTENSION_EXPORT int
sqlite3_extension_init(
    sqlite3*                    _db,
    char**                      _error,
    const sqlite3_api_routines* _api
)
{
    (void)_error;

    SQLITE_EXTENSION_INIT2(_api);

    return sqlite3_create_function(_db,
                                   "d_tests_answer",
                                   0,
                                   SQLITE_UTF8,
                                   NULL,
                                   &d_tests_answer,
                                   NULL,
                                   NULL);
}

/*
d_tests_sqlite_extension_init
  The entry point a load names: d_tests_answer() and d_tests_named().
*/
D_TESTS_SQLITE_EXTENSION_EXPORT int
d_tests_sqlite_extension_init(
    sqlite3*                    _db,
    char**                      _error,
    const sqlite3_api_routines* _api
)
{
    const int result = sqlite3_extension_init(_db, _error, _api);

    // the second function, once the first is there
    if (result != SQLITE_OK)
    {
        return result;
    }

    return sqlite3_create_function(_db,
                                   "d_tests_named",
                                   0,
                                   SQLITE_UTF8,
                                   NULL,
                                   &d_tests_named,
                                   NULL,
                                   NULL);
}
