/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for dfile module standalone tests.
*   Tests cross-platform file I/O including secure file opening, large file
*   support, file descriptors, synchronization, locking, temporary files,
*   metadata, directories, path utilities, symbolic links, pipes, and
*   binary I/O helpers.
*
*
* path:      \.config\.msvs\testing\core\djinterp-c-dfile-tests-sa\main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "..\..\..\..\..\inc\test\test_standalone.h"
#include "..\..\..\..\..\tests\dfile_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_dfile_status_items[] =
{
    { "[INFO]", "Secure file opening (d_fopen, d_fopen_s, d_freopen) "
                "validated" },
    { "[INFO]", "Large file support (d_fseeko, d_ftello, d_ftruncate) "
                "working" },
    { "[INFO]", "File descriptor operations (d_fileno, d_dup, d_dup2) tested" },
    { "[INFO]", "File synchronization (d_fsync, d_fflush) functional" },
    { "[INFO]", "File locking (d_flock) operational across platforms" },
    { "[INFO]", "Temporary file functions (d_tmpfile, d_mkstemp) verified" },
    { "[INFO]", "File metadata (d_stat, d_fstat, d_access) working" },
    { "[INFO]", "Directory operations (d_mkdir, d_rmdir, d_opendir) tested" },
    { "[INFO]", "Path utilities (d_realpath, d_dirname, d_basename) validated" },
    { "[INFO]", "Binary I/O helpers (d_fread_all, d_fwrite_all) functional" }
};

static const struct d_test_sa_note_item g_dfile_issues_items[] =
{
    { "[WARN]", "Symbolic link support varies by platform (Windows vs POSIX)" },
    { "[WARN]", "File locking semantics differ between Windows and POSIX" },
    { "[WARN]", "Path separator handling may need attention for portability" },
    { "[NOTE]", "Temporary file locations vary by platform" },
    { "[NOTE]", "File permission modes are simplified on Windows" },
    { "[NOTE]", "Some operations require elevated privileges on certain OS" }
};

static const struct d_test_sa_note_item g_dfile_steps_items[] =
{
    { "[TODO]", "Add comprehensive Unicode path support tests" },
    { "[TODO]", "Test file operations with very long paths" },
    { "[TODO]", "Add concurrent file access stress tests" },
    { "[TODO]", "Test behavior with network file systems" },
    { "[TODO]", "Add memory-mapped file tests" },
    { "[TODO]", "Test file operations on read-only filesystems" }
};

static const struct d_test_sa_note_item g_dfile_guidelines_items[] =
{
    { "[BEST]", "Always use d_fopen_s for secure file opening" },
    { "[BEST]", "Check return values from all file operations" },
    { "[BEST]", "Use d_fclose to ensure proper file handle cleanup" },
    { "[BEST]", "Use d_path_join for constructing portable paths" },
    { "[BEST]", "Use d_file_exists before attempting file operations" },
    { "[BEST]", "Prefer d_mkdir_p for creating nested directories" }
};

static const struct d_test_sa_note_section g_dfile_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_dfile_status_items) / sizeof(g_dfile_status_items[0]),
      g_dfile_status_items },
    { "KNOWN ISSUES",
      sizeof(g_dfile_issues_items) / sizeof(g_dfile_issues_items[0]),
      g_dfile_issues_items },
    { "NEXT STEPS",
      sizeof(g_dfile_steps_items) / sizeof(g_dfile_steps_items[0]),
      g_dfile_steps_items },
    { "BEST PRACTICES",
      sizeof(g_dfile_guidelines_items) / sizeof(g_dfile_guidelines_items[0]),
      g_dfile_guidelines_items }
};


/******************************************************************************
 * MAIN ENTRY POINT
 *****************************************************************************/

int
main
(
    int    _argc,
    char** _argv
)
{
    struct d_test_sa_runner runner;

    // suppress unused parameter warnings
    (void)_argc;
    (void)_argv;

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "djinterp File I/O",
                          "Comprehensive Testing of Cross-Platform File "
                          "Operations and Path Utilities");

    // register the dfile module (tree-based)
    d_test_sa_runner_add_module(&runner,
                                "dfile",
                                "Secure file opening, large file support, "
                                "file descriptors, synchronization, locking, "
                                "temporary files, metadata, directories, "
                                "path utilities, and binary I/O",
                                d_tests_dfile_run_all,
                                sizeof(g_dfile_notes) /
                                    sizeof(g_dfile_notes[0]),
                                g_dfile_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}