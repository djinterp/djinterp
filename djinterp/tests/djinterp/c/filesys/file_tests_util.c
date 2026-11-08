/*******************************************************************************
* djinterp [test]                                              file_tests_util.c
*
*   Shared test-environment machinery for the fs suite: it stands up an
* isolated, absolute-pathed scratch directory with one standard fixture file
* before the sections run, and tears it down afterwards. The absolute path is
* the point -- a d_dir_set_current test can leave the working directory anywhere and the
* rest of the suite still resolves its fixtures.
*
*   Fixture CONTENT is written with raw stdio, never d_file_write_all, so a bug in
* the code under test cannot quietly stage (or fail to stage) a fixture and
* thereby hide a failure in an unrelated module. The directory scaffolding does
* dogfood d_dir_get_current / d_path_join / d_dir_create: they are the portability layer this
* whole project rests on, and if they are broken their own sections say so
* first.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_util.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * INTERNAL STATE
 *****************************************************************************/

// g_fs_test_root
//   variable: absolute path of the scratch directory, filled by setup.
static char g_fs_test_root[D_FS_TEST_PATH_BUF];

// g_fs_test_ready
//   variable: 1 once setup has resolved g_fs_test_root.
static int g_fs_test_ready = 0;


/******************************************************************************
 * TEST ENVIRONMENT HELPERS
 *****************************************************************************/

const char* d_fs_test_root(void)
{
    return g_fs_test_root;
}


char* d_fs_test_path(
    char*       _buf,
    size_t      _bufsize,
    const char* _name
)
{

    // parameter validation
    if ( (!_buf)          ||
         (_bufsize == 0)  ||
         (!_name)         ||
         (!g_fs_test_ready) )
    {
        return NULL;
    }

    //   Built with the PLATFORM separator, not through d_path_join.
    //   D_CFG_FILE_PATH_SYNTAX selects the grammar file_path parses and emits;
    // it does not change what the running kernel opens. On a POSIX host built
    // for the Windows grammar, d_path_join correctly emits a backslash, and
    // every path this returned then named one file with a backslash IN it
    // rather than a child of the scratch directory -- so a later opendir found
    // no fixture and a relative lookup after chdir saw nothing.
    //   d_path_join keeps its own tests, which follow the selected grammar
    // because that is precisely what they are testing. This helper is not one
    // of them: it exists to hand real paths to real syscalls.
    {
        const size_t root_len = strlen(g_fs_test_root);
        const size_t name_len = strlen(_name);

        if ((root_len + 1 + name_len + 1) > _bufsize)
        {
            return NULL;
        }

        memcpy(_buf, g_fs_test_root, root_len);
        _buf[root_len] = D_FILE_PATH_SEP;
        memcpy(_buf + root_len + 1, _name, name_len);
        _buf[root_len + 1 + name_len] = '\0';
    }

    return _buf;
}


bool d_fs_test_write(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    FILE*  file;
    size_t written;

    // parameter validation: a zero-length fixture is legal and needs no data
    if ( (!_path) ||
         ( (!_data) && (_size != 0) ) )
    {
        return false;
    }

    // raw stdio, binary: this must not depend on the whole-file write helpers
    // it exists to test around
    file = fopen(_path, "wb");

    if (!file)
    {
        return false;
    }

    written = (_size > 0) ? fwrite(_data, 1, _size, file) : 0;

    if (fclose(file) != 0)
    {
        return false;
    }

    return (written == _size);
}


bool d_fs_test_setup(void)
{
    char cwd[D_FS_TEST_PATH_BUF];
    char file_path[D_FS_TEST_PATH_BUF];
    int  mkdir_result;

    // resolve the scratch directory to an absolute path, so it keeps working
    // after a d_dir_set_current test moves the working directory
    if (!d_dir_get_current(cwd, sizeof(cwd)))
    {
        return false;
    }

    if (!d_path_join(g_fs_test_root, sizeof(g_fs_test_root), cwd, D_FS_TEST_DIR))
    {
        return false;
    }

    g_fs_test_ready = 1;

    // create it; a directory left over from a previous run is fine
    mkdir_result = d_dir_create(g_fs_test_root, S_IRWXU);

    if ( (mkdir_result != 0) &&
         (!d_dir_exists(g_fs_test_root)) )
    {
        g_fs_test_ready = 0;

        return false;
    }

    // stage the standard fixture file with a known, exact byte count
    if (!d_fs_test_path(file_path, sizeof(file_path), D_FS_TEST_FILE))
    {
        g_fs_test_ready = 0;

        return false;
    }

    if (!d_fs_test_write(file_path,
                         D_FS_TEST_CONTENT,
                         strlen(D_FS_TEST_CONTENT)))
    {
        g_fs_test_ready = 0;

        return false;
    }

    return true;
}


bool d_fs_test_teardown(void)
{
    struct dj_dir_t*    dir;
    struct dj_dirent_t* entry;
    char               path_buf[D_FS_TEST_PATH_BUF];

    if (!g_fs_test_ready)
    {
        return true;
    }

    // sweep every entry the scratch directory still holds. SKIP_DOTS is off by
    // default, so "." and ".." arrive here and must be stepped over by hand.
    dir = d_dir_open(g_fs_test_root);

    if (dir)
    {
        while ((entry = d_dir_read(dir)) != NULL)
        {
            if ( (strcmp(entry->d_name, ".") == 0) ||
                 (strcmp(entry->d_name, "..") == 0) )
            {
                continue;
            }

            if (d_fs_test_path(path_buf, sizeof(path_buf), entry->d_name))
            {
                // d_file_remove takes files and empty directories alike; a test
                // that left a non-empty subtree is the test's own bug, and a
                // leftover directory here is not worth failing teardown over
                d_file_remove(path_buf);
            }
        }

        d_dir_close(dir);
    }

    d_dir_remove(g_fs_test_root);
    g_fs_test_ready = 0;

    return true;
}

/******************************************************************************
 * d_tests_file_util_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * TEST UTILITIES
 *****************************************************************************/

/*
d_tests_file_setup
  Creates the scratch directory and the standard fixture file.

Parameter(s):
  none.
Return:
  true on success, false otherwise.
*/
bool d_tests_file_setup(void)
{
    char   path[D_TEST_FILE_PATH_BUF];
    size_t length;

    // a directory left over from a previous run is fine; only a failure that
    // leaves us without one is not
    if (d_dir_create(D_TEST_FILE_TEMP_DIR, S_IRWXU) != 0)
    {
        if (!d_dir_exists(D_TEST_FILE_TEMP_DIR))
        {
            return false;
        }
    }

    if (!d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME))
    {
        return false;
    }

    // d_file_write_all opens in binary mode, so the fixture is exactly
    // strlen(CONTENT) bytes on every platform. A text-mode write would make
    // it 37 on Windows and 35 elsewhere, and every size assertion downstream
    // would need a platform branch to say the same thing twice.
    length = strlen(D_TEST_FILE_TEST_CONTENT);

    if (d_file_write_all(path, D_TEST_FILE_TEST_CONTENT, length) != 0)
    {
        return false;
    }

    return true;
}


/*
d_tests_file_teardown
  Removes every file in the scratch directory, then the directory.

Parameter(s):
  none.
Return:
  true on success, false otherwise.
*/
bool d_tests_file_teardown(void)
{
    struct dj_dir_t*    dir;
    struct dj_dirent_t* entry;
    char               path[D_TEST_FILE_PATH_BUF];

    dir = d_dir_open(D_TEST_FILE_TEMP_DIR);

    if (dir)
    {
        while ((entry = d_dir_read(dir)) != NULL)
        {
            // "." and ".." are entries like any other, and removing them is
            // not a thing that goes well
            if ( (strcmp(entry->d_name, ".") == 0) ||
                 (strcmp(entry->d_name, "..") == 0) )
            {
                continue;
            }

            if (d_tests_file_path(path, sizeof(path), entry->d_name))
            {
                // d_file_remove takes directories too, so a test that left a
                // subdirectory behind is cleaned up as well -- provided it is
                // empty, which the tests ensure
                (void)d_file_remove(path);
            }
        }

        (void)d_dir_close(dir);
    }

    (void)d_dir_remove(D_TEST_FILE_TEMP_DIR);

    return true;
}


/*
d_tests_file_path
  Builds a path inside the scratch directory.

Parameter(s):
  _buf:      buffer to receive the path.
  _bufsize:  size of _buf, in bytes.
  _filename: the name to place inside the scratch directory.
Return:
  _buf on success, or NULL on failure.
*/
/*
d_tests_file_child
  Joins _name under _parent using the separator the HOST accepts.
  See the note in d_tests_file_path: D_CFG_FILE_PATH_SYNTAX selects the grammar
the lexical module reads and writes, not what the running kernel opens. A test
that builds a path in order to create it needs the second, so it cannot go
through d_path_join.

Parameter(s):
  _buf:     destination.
  _bufsize: its size.
  _parent:  the containing directory.
  _name:    the child's name.
Return:
  _buf on success, or NULL if anything was missing or it would not fit.
*/
char* d_tests_file_child(
    char*       _buf,
    size_t      _bufsize,
    const char* _parent,
    const char* _name
)
{
    size_t parent_len;
    size_t name_len;

    if ( (!_buf) || (_bufsize == 0) || (!_parent) || (!_name) )
    {
        return NULL;
    }

    parent_len = strlen(_parent);
    name_len   = strlen(_name);

    if ((parent_len + 1 + name_len + 1) > _bufsize)
    {
        return NULL;
    }

    memcpy(_buf, _parent, parent_len);
    _buf[parent_len] = D_TEST_FILE_HOST_SEP;
    memcpy(_buf + parent_len + 1, _name, name_len);
    _buf[parent_len + 1 + name_len] = '\0';

    return _buf;
}


char* d_tests_file_path(
    char*       _buf,
    size_t      _bufsize,
    const char* _filename
)
{
    // parameter validation
    if ( (!_buf) ||
         (_bufsize == 0) ||
         (!_filename) )
    {
        return NULL;
    }

    //   Built with the HOST's separator, not with d_path_join.
    //   D_CFG_FILE_PATH_SYNTAX selects which grammar the lexical module PARSES
    // and EMITS. That is a cross-compilation and parsing facility -- it does
    // not change what the running kernel accepts. So on a POSIX host built for
    // the Windows grammar, d_path_join correctly emits a backslash, and every
    // path this helper produced then named one file with a backslash in it
    // rather than a file inside the scratch directory. Eight filesystem tests
    // failed on that, in file_dir and file_space, none of which is about
    // paths.
    //   d_path_join keeps its own tests, which follow the selected grammar
    // because that is what they are testing. This helper is not one of them.
    return d_tests_file_child(_buf, _bufsize, D_TEST_FILE_TEMP_DIR, _filename);
}
