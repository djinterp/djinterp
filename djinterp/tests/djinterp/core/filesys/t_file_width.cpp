/*******************************************************************************
* djinterp [core]                                               t_file_width.cpp
*
*   Runtime harness for core/filesys at the C layer's D_FILE_WIDTH (dfile.hpp
* has the rules). It builds at every level from ISO strict C++98, at either
* width and below the 64-bit floor, and checks, on the fixtures
* ci/check_filesys.sh makes in the scratch directory it is given:
*   - what a path names, which has no width: exists, is_regular_file and
*     is_directory, for a 5 GiB file and a 2040 time;
*   - a directory's walk, walk and remove_all, over entries whose inode
*     numbers need more than 32 bits where t_file_shim.c runs beneath --
*     none of them asks for one;
*   - file_status, file_size, file's position and space_info, at the width:
*     at 64 they read past 4 GiB and 2038, and at 32 they fail with
*     EOVERFLOW where the C layer does. Where the level has had to narrow
*     the width (D_FILE_WIDTH_IS_NARROWED) they are absent, and unchecked.
*   The fixtures:
*     big5g   5 GiB + 1, sparse      big3g   3 GiB, sparse
*     future  modified in 2040       link    a symbolic link to big5g
*     tree/   big, 5 GiB + 1, sparse, and an empty sub/
*     wdir/   a, wide1, b, wide2 and c, all empty
*   POSIX only. ci/check_filesys.sh runs it in every configuration, and the
* C++ ladder compiles it at every rung.
*
*
* path:      /tests/djinterp/core/filesys/t_file_width.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/
// std
#include <cerrno>  // EOVERFLOW
#include <cstdio>  // std::printf, SEEK_END, SEEK_SET
// djinterp
#include "../../../../inc/djinterp/core/filesys/dfile.hpp"  // unit under test


static int g_checks   = 0;
static int g_failures = 0;
static int g_seen     = 0;

/*
d_tests_file_cpp_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_file_cpp_expect(
    bool        _ok,
    const char* _what
)
{
    ++g_checks;

    // report a failure with what was being checked
    if (!_ok)
    {
        ++g_failures;
        std::printf("  FAIL: %s\n",
                    _what);
    }

    return;
}

/*
d_tests_file_cpp_report
  Prints one group's verdict, so that a log names the group a failure sits in.
*/
static void
d_tests_file_cpp_report(
    bool        _ok,
    const char* _group
)
{
    std::printf("  %-4s %s\n",
                _ok ? "ok" : "FAIL",
                _group);

    return;
}

/*
d_tests_file_cpp_count
  walk's visitor: counts every entry it is shown.
*/
static void
d_tests_file_cpp_count(
    const djinterp::path& _p,
    unsigned              _depth,
    bool                  _dir
)
{
    (void)_p;
    (void)_depth;
    (void)_dir;
    ++g_seen;

    return;
}

/*
d_tests_file_cpp_kinds
  What a path names, which has no width.
  Tests the following:
  - a 5 GiB file exists and is a regular file, at every width
  - a file modified in 2040 is a regular file, at every width
  - a directory is one, and an absent path is a plain false, no error
*/
static bool
d_tests_file_cpp_kinds(
    const djinterp::path& _root
)
{
    const int            before = g_failures;
    const djinterp::path big5   = _root / djinterp::path("big5g");
    const djinterp::path future = _root / djinterp::path("future");
    const djinterp::path tree   = _root / djinterp::path("tree");
    const djinterp::path absent = _root / djinterp::path("absent");
    djinterp::error      ec;

    d_tests_file_cpp_expect( (djinterp::exists(big5,
                                               ec)) &&
                             (djinterp::is_regular_file(big5,
                                                        ec)) &&
                             (!djinterp::is_directory(big5,
                                                      ec)) &&
                             (!ec.failed()),
                             "a 5 GiB file exists, and is a regular file" );
    d_tests_file_cpp_expect( (djinterp::is_regular_file(future,
                                                        ec)) &&
                             (!ec.failed()),
                             "a 2040 file is a regular file" );
    d_tests_file_cpp_expect( (djinterp::is_directory(tree,
                                                     ec)) &&
                             (!djinterp::exists(absent,
                                                ec)) &&
                             (!ec.failed()),
                             "a directory is one; an absent path, no error" );

    return (g_failures == before);
}

/*
d_tests_file_cpp_read
  Reads a directory to its end, counting its entries and how many of them are
regular files, and reports whether the read ended cleanly.
*/
static bool
d_tests_file_cpp_read(
    const djinterp::path& _dir,
    int&                  _count,
    int&                  _regular
)
{
    djinterp::error           ec;
    djinterp::directory       d(_dir,
                                ec);
    djinterp::directory_entry e;

    _count   = 0;
    _regular = 0;

    // every entry until the end
    while (d.read(e,
                  ec))
    {
        ++_count;
        _regular += (e.type() == djinterp::file_kind::type_regular);
    }

    return ( (d.is_open()) &&
             (!ec.failed()) &&
             (!d.failed()) );
}

/*
d_tests_file_cpp_walk
  A directory whose entries the shim gives inode numbers past 32 bits.
  Tests the following:
  - directory reads all five, as regular files, to a clean end
  - walk visits all five
  - remove_all removes the directory and all five
*/
static bool
d_tests_file_cpp_walk(
    const djinterp::path& _root
)
{
    const int            before  = g_failures;
    const djinterp::path wdir    = _root / djinterp::path("wdir");
    int                  count   = 0;
    int                  regular = 0;
    djinterp::error      ec;

    d_tests_file_cpp_expect( (d_tests_file_cpp_read(wdir,
                                                    count,
                                                    regular)) &&
                             (count == 5) &&
                             (regular == 5),
                             "directory reads all five, regular, cleanly" );
    g_seen = 0;
    d_tests_file_cpp_expect( (djinterp::walk(wdir,
                                             d_tests_file_cpp_count,
                                             ec)) &&
                             (!ec.failed()) &&
                             (g_seen == 5),
                             "walk visits all five" );
    d_tests_file_cpp_expect( (djinterp::remove_all(wdir,
                                                   ec)) &&
                             (!ec.failed()) &&
                             (!djinterp::exists(wdir,
                                                ec)),
                             "remove_all removes them" );

    return (g_failures == before);
}

/*
d_tests_file_cpp_tree
  A directory holding a file too large for 32 bits, and a subdirectory.
  Tests the following:
  - directory reads both entries
  - walk visits both
  - remove_all removes the tree, the large file included
*/
static bool
d_tests_file_cpp_tree(
    const djinterp::path& _root
)
{
    const int            before  = g_failures;
    const djinterp::path tree    = _root / djinterp::path("tree");
    int                  count   = 0;
    int                  regular = 0;
    djinterp::error      ec;

    d_tests_file_cpp_expect( (d_tests_file_cpp_read(tree,
                                                    count,
                                                    regular)) &&
                             (count == 2) &&
                             (regular == 1),
                             "directory reads a 5 GiB file and a directory" );
    g_seen = 0;
    d_tests_file_cpp_expect( (djinterp::walk(tree,
                                             d_tests_file_cpp_count,
                                             ec)) &&
                             (!ec.failed()) &&
                             (g_seen == 2),
                             "walk visits both" );
    d_tests_file_cpp_expect( (djinterp::remove_all(tree,
                                                   ec)) &&
                             (!ec.failed()) &&
                             (!djinterp::exists(tree,
                                                ec)),
                             "remove_all removes the tree" );

    return (g_failures == before);
}

#if !D_FILE_WIDTH_IS_NARROWED
/*
d_tests_file_cpp_status
  What carries a width, at the width, for what fits either.
  Tests the following:
  - file_status's size_type and time_type, and d_off_t, have D_FILE_WIDTH
    bits
  - status and file_size read 3 GiB, which fits either
  - symlink_status sees a link as a link
  - a stream seeks and tells a position that fits
*/
static bool
d_tests_file_cpp_status(
    const djinterp::path& _root
)
{
    typedef djinterp::file_status::size_type size_type;
    typedef djinterp::file_status::time_type time_type;

    const int             before = g_failures;
    const djinterp::path  big3   = _root / djinterp::path("big3g");
    djinterp::error       ec;
    djinterp::file        f(_root / djinterp::path("big5g"),
                            "rb");
    djinterp::file_status st     = djinterp::status(big3,
                                                    ec);

    d_tests_file_cpp_expect( ((int)(sizeof(size_type) * 8) == D_FILE_WIDTH) &&
                             ((int)(sizeof(time_type) * 8) == D_FILE_WIDTH) &&
                             ((int)(sizeof(d_off_t) * 8) == D_FILE_WIDTH),
                             "the snapshot's types have D_FILE_WIDTH bits" );
    d_tests_file_cpp_expect( (!ec.failed()) &&
                             (st.is_regular_file()) &&
                             (st.size() == (size_type)0xC0000000u) &&
                             (djinterp::file_size(big3,
                                                  ec) ==
                              (size_type)0xC0000000u) &&
                             (!ec.failed()),
                             "status and file_size read 3 GiB" );
    st = djinterp::symlink_status(_root / djinterp::path("link"),
                                  ec);
    d_tests_file_cpp_expect( (!ec.failed()) &&
                             (st.is_symlink()),
                             "symlink_status sees the link" );
    d_tests_file_cpp_expect( (f.is_open()) &&
                             (f.seek(100,
                                     SEEK_SET,
                                     ec)) &&
                             (f.tell(ec) == 100) &&
                             (!ec.failed()),
                             "seek and tell a position that fits" );

    return (g_failures == before);
}
#else
/*
d_tests_file_cpp_status
  Nothing to check: this level has narrowed the width, so what carries one
is absent.
*/
static bool
d_tests_file_cpp_status(
    const djinterp::path& _root
)
{
    (void)_root;

    return true;
}
#endif  // !D_FILE_WIDTH_IS_NARROWED

#if ( (!D_FILE_WIDTH_IS_NARROWED) && (D_FILE_WIDTH == 64) )
/*
d_tests_file_cpp_large
  Past 4 GiB and 2038, at 64.
  Tests the following:
  - status and file_size read 5 GiB + 1, and a 2040 time
  - a stream tells a position past 4 GiB, and its status reads the size
  - space_info counts bytes
*/
static bool
d_tests_file_cpp_large(
    const djinterp::path& _root
)
{
    typedef djinterp::file_status::size_type size_type;
    typedef djinterp::file_status::time_type time_type;

    const int             before = g_failures;
    const size_type       five   = (size_type)5 * 1024 * 1024 * 1024 + 1;
    const djinterp::path  big5   = _root / djinterp::path("big5g");
    djinterp::error       ec;
    djinterp::file        f(big5,
                            "rb");
    djinterp::file_status st     = djinterp::status(big5,
                                                    ec);

    d_tests_file_cpp_expect( (!ec.failed()) &&
                             (st.size() == five) &&
                             (djinterp::file_size(big5,
                                                  ec) == five),
                             "status and file_size read 5 GiB + 1" );
    st = djinterp::status(_root / djinterp::path("future"),
                          ec);
    d_tests_file_cpp_expect( (!ec.failed()) &&
                             (st.modified_time() > (time_type)2147483647),
                             "status reads a 2040 time" );
    d_tests_file_cpp_expect( (f.seek(0,
                                     SEEK_END,
                                     ec)) &&
                             (f.tell(ec) == (d_off_t)five) &&
                             (f.status(ec).size() == five) &&
                             (!ec.failed()),
                             "a stream tells 5 GiB + 1, and its size" );

    const djinterp::space_info sp = djinterp::space(_root,
                                                    ec);

    d_tests_file_cpp_expect( (!ec.failed()) &&
                             (sp.capacity > 0) &&
                             (sp.available <= sp.capacity),
                             "space_info counts bytes" );

    return (g_failures == before);
}
#elif !D_FILE_WIDTH_IS_NARROWED
/*
d_tests_file_cpp_large
  Past 4 GiB and 2038, at 32: refused, as the C layer refuses it.
  Tests the following:
  - status of 5 GiB + 1 and of a 2040 time fail with EOVERFLOW, and
    file_size with it
  - a stream fails to tell a position past 2 GiB, and its status to hold
    5 GiB, with EOVERFLOW
  - space_info counts KiB
*/
static bool
d_tests_file_cpp_large(
    const djinterp::path& _root
)
{
    const int             before = g_failures;
    const djinterp::path  big5   = _root / djinterp::path("big5g");
    djinterp::error       ec;
    djinterp::file        f(big5,
                            "rb");
    djinterp::file_status st     = djinterp::status(big5,
                                                    ec);

    d_tests_file_cpp_expect( (ec.value() == EOVERFLOW) &&
                             (!st.exists()) &&
                             (djinterp::file_size(big5,
                                                  ec) == 0) &&
                             (ec.value() == EOVERFLOW),
                             "status and file_size refuse 5 GiB + 1" );
    st = djinterp::status(_root / djinterp::path("future"),
                          ec);
    d_tests_file_cpp_expect(ec.value() == EOVERFLOW,
                            "status refuses a 2040 time");
    d_tests_file_cpp_expect( (f.seek(0,
                                     SEEK_END,
                                     ec)) &&
                             (f.tell(ec) == -1) &&
                             (ec.value() == EOVERFLOW) &&
                             (!f.status(ec).exists()) &&
                             (ec.value() == EOVERFLOW),
                             "a stream refuses to tell or size 5 GiB + 1" );

    const djinterp::space_info sp = djinterp::space(_root,
                                                    ec);

    d_tests_file_cpp_expect( (!ec.failed()) &&
                             (sp.capacity_kib > 0) &&
                             (sp.available_kib <= sp.capacity_kib),
                             "space_info counts KiB" );

    return (g_failures == before);
}
#else
/*
d_tests_file_cpp_large
  Nothing to check: this level has narrowed the width, so what carries one
is absent.
*/
static bool
d_tests_file_cpp_large(
    const djinterp::path& _root
)
{
    (void)_root;

    return true;
}
#endif  // !D_FILE_WIDTH_IS_NARROWED, D_FILE_WIDTH == 64

/*
main
  Runs every group this build can check against the scratch directory its one
argument names, and exits 0 only when every check passed.
*/
int
main(
    int    _argc,
    char** _argv
)
{
    // the scratch directory, holding the fixtures, is the one argument
    if (_argc != 2)
    {
        std::printf("usage: t_file_width <scratch directory>\n");

        return 2;
    }

    const djinterp::path root(_argv[1]);

    std::printf("t_file_width.cpp: D_FILE_WIDTH %d, narrowed %d\n",
                (int)D_FILE_WIDTH,
                (int)D_FILE_WIDTH_IS_NARROWED);
    d_tests_file_cpp_report(d_tests_file_cpp_kinds(root),
                            "what a path names");
    d_tests_file_cpp_report(d_tests_file_cpp_walk(root),
                            "a walk over wide inode numbers");

    // what carries a width, where the level keeps the build's
    if (!D_FILE_WIDTH_IS_NARROWED)
    {
        d_tests_file_cpp_report(d_tests_file_cpp_status(root),
                                "status, size and position that fit");
        d_tests_file_cpp_report(d_tests_file_cpp_large(root),
                                "past 4 GiB and 2038, at the width");
    }

    d_tests_file_cpp_report(d_tests_file_cpp_tree(root),
                            "a tree with a 5 GiB file");
    std::printf("t_file_width.cpp: %d checks, %d failed\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
