/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_util.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa_util.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"


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
bool
d_tests_file_setup
(
    void
)
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
bool
d_tests_file_teardown
(
    void
)
{
    struct d_dir_t*    dir;
    struct d_dirent_t* entry;
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
char*
d_tests_file_child
(
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


char*
d_tests_file_path
(
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


/*
d_tests_file_group
  Builds an interior node from assertions the caller has already created,
using the count the caller actually filled.
  The point is that the count is DERIVED rather than asserted. The old suite
wrote the number by hand -- new_interior("d_file_symlink", 4) followed by four
elements[idx++] lines -- so adding, removing or #if-ing out an assertion
silently left the tail of the array uninitialised, and the tree walker read
whatever was there. Here a conditionally-omitted assertion simply makes the
group smaller, and there is no second place to keep in sync.

Parameter(s):
  _name:  the group's name.
  _items: assertions to adopt.
  _count: how many of _items are populated.
Return:
  The group on success, or NULL on failure.
*/
struct d_test_object*
d_tests_file_group
(
    const char*            _name,
    struct d_test_object** _items,
    size_t                 _count
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior(_name, _count);

    if (!group)
    {
        return NULL;
    }

    for (idx = 0; idx < _count; ++idx)
    {
        group->elements[idx] = _items[idx];
    }

    return group;
}
