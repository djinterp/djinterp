/******************************************************************************
* djinterp [test]                                         dfile_tests_sa_util.c
*
*   Utility functions for dfile module tests.
*   Provides test environment setup, teardown, and path helpers.
*
*
* path:      \src\test\dfile_tests_sa_util.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_dfile_setup
  Creates the test environment including a temporary directory and standard
test files.
  Tests the following:
  - creates temporary directory for test files
  - creates standard test file with known content

Return:
  true if setup succeeded, false otherwise.
*/
bool
d_tests_dfile_setup
(
    void
)
{
    char   path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*  test_file;
    int    mkdir_result;
    size_t content_len;

    // create test directory
    mkdir_result = d_mkdir(D_TEST_DFILE_TEMP_DIR, S_IRWXU | S_IRWXG | S_IROTH);

    // directory may already exist, that's okay
    if ( (mkdir_result != 0) && 
         (!d_is_dir(D_TEST_DFILE_TEMP_DIR)) )
    {
        return false;
    }

    // create standard test file
    if (!d_tests_dfile_get_test_path(path_buf,
                                    sizeof(path_buf),
                                    D_TEST_DFILE_TEST_FILENAME))
    {
        return false;
    }

    // use binary mode to ensure consistent file size across platforms
    test_file = d_fopen(path_buf, "wb");

    if (!test_file)
    {
        return false;
    }

    // write standard content using fwrite for exact byte count
    content_len = strlen(D_TEST_DFILE_TEST_CONTENT);
    fwrite(D_TEST_DFILE_TEST_CONTENT, 1, content_len, test_file);
    fclose(test_file);

    return true;
}


/*
d_tests_dfile_teardown
  Cleans up the test environment by removing test files and directories.
  Tests the following:
  - removes test files
  - removes test directory

Return:
  true if teardown succeeded, false otherwise.
*/
bool
d_tests_dfile_teardown
(
    void
)
{
    struct d_dir_t*    dir;
    struct d_dirent_t* entry;
    char               path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];

    // remove all files in test directory
    dir = d_opendir(D_TEST_DFILE_TEMP_DIR);

    if (dir)
    {
        while ((entry = d_readdir(dir)) != NULL)
        {
            // skip . and ..
            if ( (strcmp(entry->d_name, ".") == 0) || 
                 (strcmp(entry->d_name, "..") == 0) )
            {
                continue;
            }

            // construct full path and remove
            if (d_tests_dfile_get_test_path(path_buf,
                                           sizeof(path_buf),
                                           entry->d_name))
            {
                d_remove(path_buf);
            }
        }

        d_closedir(dir);
    }

    // remove test directory
    d_rmdir(D_TEST_DFILE_TEMP_DIR);

    return true;
}


/*
d_tests_dfile_get_test_path
  Constructs a full path within the test directory.

Parameter(s):
  _buf:      buffer to receive the constructed path
  _bufsize:  size of buffer
  _filename: filename to append to test directory
Return:
  _buf on success, NULL on failure.
*/
char*
d_tests_dfile_get_test_path
(
    char*       _buf,
    size_t      _bufsize,
    const char* _filename
)
{

    if ( (!_buf) || 
         (_bufsize == 0) || 
         (!_filename) )
    {
        return NULL;
    }

    return d_path_join(_buf, _bufsize, D_TEST_DFILE_TEMP_DIR, _filename);
}
