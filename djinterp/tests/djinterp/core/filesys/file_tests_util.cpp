/*******************************************************************************
* djinterp [test]                                            file_tests_util.cpp
*
*   Shared environment for the core/filesys C++ suite: an isolated, absolute-pathed
* scratch directory with one standard fixture, stood up before run() and torn
* down after.
*
*   WHAT THESE HELPERS ARE BUILT ON, AND WHY. Every helper here reaches for the
* C layer (c/filesys) or raw stdio -- never for djinterp::file, djinterp::directory
* or the free functions this suite exists to test. The C layer sits BENEATH the
* code under test and has its own suite, so a helper built on it is independent
* evidence. Were teardown written with remove_all(), a broken remove_all would
* leave the scratch tree behind AND report every later case as passing on a
* dirty directory; were fs_test_contents_are written with djinterp::file, a
* broken read would corroborate a broken write. Neither can happen this way.
*
*   ABSOLUTE PATHS. The scratch directory is resolved through d_dir_get_current at
* setup. Deferred clauses run long after the section functions returned, and
* one of them changes nothing about the working directory -- but the C suite's
* chdir case proves how cheap it is to be wrong about that, so the paths do not
* depend on it.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_util.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <cstdio>
#include <cstring>
// djinterp
//   file_tests.hpp -> file.hpp -> the c/filesys headers, so the C entry points this
// file deliberately builds on (d_dir_open, d_dir_read, d_dir_close, d_dir_create,
// d_dir_remove, d_file_remove, d_dir_get_current, d_path_join, d_file_exists, d_file_symlink) are all
// already declared. No separate include, and no relative path to go stale.
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

namespace
{

// g_root
//   variable: the absolute scratch directory, filled by setup. A function
// local would be re-initialized on first use; this is written once and read by
// every deferred clause afterwards.
path g_root;

// g_ready
//   variable: true once setup has resolved g_root.
bool g_ready = false;

// g_symlink_probe
//   variable: -1 unknown, 0 no, 1 yes -- cached answer to "can this process
// create a symbolic link here?".
int g_symlink_probe = -1;


// remove_tree_c
//   function: recursively delete _p using ONLY the C layer, so teardown does
// not depend on the C++ recursion this suite tests. Dot entries arrive from
// d_dir_read (the C layer does not filter them by default) and are skipped by
// hand.
// returns true when _p is gone.
bool
remove_tree_c(const char* _p)
{
    struct d_dir_t*    dir;
    struct d_dirent_t* entry;

    if (!_p)
    {
        return false;
    }

    // a plain file, or an already-empty directory, needs no walk
    if (d_file_remove(_p) == 0)
    {
        return true;
    }

    dir = d_dir_open(_p);

    if (!dir)
    {
        // not a directory (or unreadable); nothing more to try
        return !d_file_exists(_p);
    }

    while ((entry = d_dir_read(dir)) != 0)
    {
        char child[1024];

        if ( (std::strcmp(entry->d_name, ".") == 0) ||
             (std::strcmp(entry->d_name, "..") == 0) )
        {
            continue;
        }

        if (d_path_join(child, sizeof(child), _p, entry->d_name))
        {
            remove_tree_c(child);
        }
    }

    d_dir_close(dir);

    return (d_dir_remove(_p) == 0);
}

} // anonymous namespace


/******************************************************************************
 * ENVIRONMENT
 *****************************************************************************/

const path&
fs_test_root(void)
{
    return g_root;
}


path
fs_test_path(const char* _name)
{
    if ( (!_name) ||
         (!g_ready) )
    {
        return path();
    }

    return g_root / path(_name);
}


bool
fs_test_write(const path&   _p,
              const void*   _data,
              ::std::size_t _size)
{
    std::FILE*    handle;
    ::std::size_t written;

    if ( (!_p.valid())          ||
         ((!_data) && (_size)) )
    {
        return false;
    }

    // raw stdio: this must not lean on djinterp::file, which it stages for
    handle = std::fopen(_p.c_str(), "wb");

    if (!handle)
    {
        return false;
    }

    written = (_size > 0) ? std::fwrite(_data, 1, _size, handle) : 0;

    if (std::fclose(handle) != 0)
    {
        return false;
    }

    return (written == _size);
}


bool
fs_test_contents_are(const path& _p,
                     const char* _expect)
{
    std::FILE*    handle;
    char          buf[4096];
    ::std::size_t want;
    ::std::size_t got;

    if ( (!_p.valid()) ||
         (!_expect) )
    {
        return false;
    }

    want = std::strlen(_expect);

    if (want >= sizeof(buf))
    {
        return false;
    }

    handle = std::fopen(_p.c_str(), "rb");

    if (!handle)
    {
        return false;
    }

    got = std::fread(buf, 1, sizeof(buf), handle);
    std::fclose(handle);

    // an exact match: same length AND same bytes, so a short read or a longer
    // file both register as a mismatch
    return ( (got == want) &&
             (std::memcmp(buf, _expect, want) == 0) );
}


bool
fs_test_exists(const path& _p)
{
    if (!_p.valid())
    {
        return false;
    }

    // the C layer's own query -- a different implementation from the C++
    // exists() a case may be trying to corroborate
    return (d_file_exists(_p.c_str()) != 0);
}


path
fs_test_invalid_path(void)
{
    path source("placeholder");
    path sink(static_cast<path&&>(source));

    // `source` handed its representation to `sink`, so it is now the invalid
    // path -- valid() false -- which is exactly what every free function's
    // EINVAL guard is written for.
    return source;
}


bool
fs_test_symlinks_creatable(void)
{
#if D_FILE_LINK_IS_AVAILABLE
    path target;
    path link;

    if (g_symlink_probe >= 0)
    {
        return (g_symlink_probe == 1);
    }

    if (!g_ready)
    {
        return false;
    }

    target = fs_test_path(D_CF_TEST_FILE);
    link   = fs_test_path("probe.symlink");

    d_file_remove(link.c_str());

    // probe with the C entry point, not create_symlink: this decides whether
    // the link CASES may run, so it must not depend on what they test
    if (d_file_symlink(target.c_str(), link.c_str()) == 0)
    {
        g_symlink_probe = 1;
        d_file_remove(link.c_str());
    }
    else
    {
        g_symlink_probe = 0;
    }

    return (g_symlink_probe == 1);
#else
    return false;
#endif
}


bool
fs_test_setup(void)
{
    char cwd[1024];
    path fixture;

    if (!d_dir_get_current(cwd, sizeof(cwd)))
    {
        return false;
    }

    g_root  = path(cwd) / path(D_CF_TEST_DIR);
    g_ready = g_root.valid();

    if (!g_ready)
    {
        return false;
    }

    // a tree left by an interrupted run would poison the directory cases
    remove_tree_c(g_root.c_str());

    if (d_dir_create(g_root.c_str(), 0700) != 0)
    {
        g_ready = false;

        return false;
    }

    fixture = fs_test_path(D_CF_TEST_FILE);

    if (!fs_test_write(fixture,
                       D_CF_TEST_CONTENT,
                       std::strlen(D_CF_TEST_CONTENT)))
    {
        g_ready = false;

        return false;
    }

    return true;
}


bool
fs_test_teardown(void)
{
    bool gone;

    if (!g_ready)
    {
        return true;
    }

    gone    = remove_tree_c(g_root.c_str());
    g_ready = false;

    return gone;
}

} // namespace testing
} // namespace djinterp
