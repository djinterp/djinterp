/*******************************************************************************
* djinterp [c]                                                    t_file_width.c
*
*   Runtime harness for c/filesys's two widths (file_common.h has the rules).
* It builds as C from C99 at either width, and as C++ from C++98 -- below the
* 64-bit floor too, where only the 32-bit twins exist -- and checks, on
* sparse files it makes in the scratch directory it is given:
*   - offsets and lengths past 2 and 4 GiB: the 32-bit twins reach them, and
*     fail with EOVERFLOW only on a value they must hand back;
*   - times past 2038 and before 1901, where this build's time_t can set them;
*   - the type and mode queries, which never narrow;
*   - reading a directory at both widths and by name;
*   - capacity in bytes and in KiB;
*   - the unsuffixed names, which are the twins at D_FILE_WIDTH;
*   - a copy, and the creation time the build reports, or does not.
*   Built with -DT_FILE_SHIM, it must run under t_file_shim.c's LD_PRELOAD,
* which gives two directory entries inode numbers past 32 bits and one
* directory a 5 TiB filesystem: the 32-bit twins must fail on exactly those,
* and the name reader and the 64-bit twins must not.
*   Build it with the feature-test macros the library was built with: the
* creation time it expects follows D_FILE_STAT_HAS_BIRTHTIME, which reads
* them. POSIX only, for utimes. ci/check_filesys.sh runs it in every
* configuration; by hand, from the repo root:
*     cc -std=c99 -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror                  \
*        src/djinterp/c/filesys/file_*.c                                       \
*        tests/djinterp/c/filesys/t_file_width.c -o t_file_width
*     ./t_file_width <scratch directory>
*
*
* path:      /tests/djinterp/c/filesys/t_file_width.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/
// std
#include <errno.h>     // errno, ENOENT, EOVERFLOW
#include <fcntl.h>     // O_CREAT, O_RDWR, O_TRUNC, AT_FDCWD
#include <stdbool.h>   // bool
#include <stdio.h>     // FILE, fputc, fputs, fread, fwrite, printf, sprintf
#include <string.h>    // memcmp, memset
#include <sys/stat.h>  // statx, where the build declares it
#include <sys/time.h>  // struct timeval, utimes
#include <time.h>      // time_t
// djinterp
#include "../../../../inc/djinterp/c/filesys/dfile.h"  // unit under test
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint8_t,
                                                     // uint32_t, uint64_t


static int g_checks   = 0;
static int g_failures = 0;

// g_shim
//   constant: whether this build runs under t_file_shim.c, which only
// -DT_FILE_SHIM says.
#if defined(T_FILE_SHIM)
static const bool g_shim = true;
#else
static const bool g_shim = false;
#endif  // T_FILE_SHIM

// d_tests_file_paths
//   struct: the fixtures' paths, made once from the scratch directory.
struct d_tests_file_paths
{
    char root[512];    // the scratch directory
    char small[512];   // ten bytes
    char big3[512];    // 3 GiB + 1, sparse, made by the 32-bit twins
    char big5[512];    // 5 GiB + 1, sparse, made by the 64-bit twins
    char link[512];    // a symbolic link to big3
    char absent[512];  // nothing at all
    char dir[512];     // a directory holding a and b
    char wdir[512];    // a, wide1, b, wide2 and c; the shim widens wide*
    char fake[512];    // a directory the shim gives 5 TiB
    char src[512];     // a copy's source
    char dst[512];     // and its destination
};

/*
d_tests_file_expect
  Counts one check, and prints it when it fails, with errno as the check left
it -- which is usually why.
*/
static void
d_tests_file_expect(
    bool        _ok,
    const char* _what
)
{
    ++g_checks;

    // report a failure with what was being checked
    if (!_ok)
    {
        ++g_failures;
        printf("  FAIL: %s (errno %d)\n",
               _what,
               errno);
    }

    return;
}

/*
d_tests_file_report
  Prints one group's verdict, so that a log names the group a failure sits in.
*/
static void
d_tests_file_report(
    bool        _ok,
    const char* _group
)
{
    printf("  %-4s %s\n",
           _ok ? "ok" : "FAIL",
           _group);

    return;
}

/*
d_tests_file_join
  Writes _dir/_name into _out, which the caller sizes for both.
*/
static void
d_tests_file_join(
    char*       _out,
    const char* _dir,
    const char* _name
)
{
    sprintf(_out,
            "%s/%s",
            _dir,
            _name);

    return;
}

/*
d_tests_file_paths_make
  Names every fixture under the scratch directory; nothing is made yet.
*/
static struct d_tests_file_paths
d_tests_file_paths_make(
    const char* _root
)
{
    const char* const names[] = { "small.txt", "big3.bin", "big5.bin", "link",
                                  "absent", "dir", "wdir", "fake5t",
                                  "copy_src.bin", "copy_dst.bin" };
    struct d_tests_file_paths p = {0};
    char* const outs[] = { p.small, p.big3, p.big5, p.link, p.absent, p.dir,
                           p.wdir, p.fake, p.src, p.dst };

    sprintf(p.root,
            "%s",
            _root);

    // each fixture's name under the root
    for (size_t i = 0; i < (sizeof(names) / sizeof(names[0])); ++i)
    {
        d_tests_file_join(outs[i],
                          _root,
                          names[i]);
    }

    return p;
}

/*
d_tests_file_touch
  Creates an empty file, or empties one, through the library's own stream.
*/
static bool
d_tests_file_touch(
    const char* _path
)
{
    FILE* const f = d_file_open_stream(_path,
                                       "wb");

    // a file that cannot be made fails the checks that read it
    if (!f)
    {
        return false;
    }

    return (d_file_close_stream(f) == 0);
}

/*
d_tests_file_set_mtime
  Sets a path's access and modification times to one second count, given as a
double so that a count past 2038 can be written where time_t is 64 bits; the
caller passes one only there.
*/
static int
d_tests_file_set_mtime(
    const char* _path,
    double      _seconds
)
{
    struct timeval tv[2] = { { 0, 0 }, { 0, 0 } };

    tv[0].tv_sec = (time_t)_seconds;
    tv[1].tv_sec = (time_t)_seconds;

    return utimes(_path,
                  tv);
}

/*
d_tests_file_setup
  Makes the fixtures every group reads: the ten-byte file and a directory of
two empty ones. The large files are made by the groups that test making them.
*/
static bool
d_tests_file_setup(
    const struct d_tests_file_paths* _p
)
{
    const int   before    = g_failures;
    char        path[512] = {0};
    FILE* const f         = d_file_open_stream(_p->small,
                                               "wb");

    d_tests_file_expect( (f != NULL) &&
                         (fputs("0123456789",
                                f) >= 0) &&
                         (d_file_close_stream(f) == 0),
                         "make the ten-byte file" );
    d_tests_file_expect(d_dir_create(_p->dir,
                                     0755) == 0,
                        "make the directory");
    d_tests_file_join(path,
                      _p->dir,
                      "a");
    d_tests_file_expect(d_tests_file_touch(path),
                        "make dir/a");
    d_tests_file_join(path,
                      _p->dir,
                      "b");
    d_tests_file_expect(d_tests_file_touch(path),
                        "make dir/b");

    return (g_failures == before);
}

/*
d_tests_file_extent_make
  Makes a 3 GiB + 1 file through the 32-bit twins alone.
  Tests the following:
  - d_file_truncate_fd32 sets a length of 2 GiB - 1, its largest
  - d_file_seek_stream32 goes on to 3 GiB through SEEK_END and SEEK_CUR, so
    a byte written there lands past 3 GiB
  - d_file_tell_stream32 fails with EOVERFLOW there, the position not
    fitting what it must hand back
*/
static bool
d_tests_file_extent_make(
    const struct d_tests_file_paths* _p
)
{
    const int before = g_failures;
    const int fd     = d_file_open(_p->big3,
                                   O_RDWR | O_CREAT | O_TRUNC,
                                   0644);

    d_tests_file_expect( (fd >= 0) &&
                         (d_file_truncate_fd32(fd,
                                               0x7FFFFFFF) == 0),
                         "truncate32 a new file to 2 GiB - 1" );
    d_file_close_fd(fd);

    FILE* const f = d_file_open_stream(_p->big3,
                                       "rb+");

    // without the stream nothing below can run
    if (!f)
    {
        d_tests_file_expect(false,
                            "reopen the 3 GiB fixture as a stream");

        return false;
    }

    d_tests_file_expect( (d_file_seek_stream32(f,
                                               0,
                                               SEEK_END) == 0) &&
                         (d_file_seek_stream32(f,
                                               0x40000001,
                                               SEEK_CUR) == 0),
                         "seek32 to the end, 2 GiB - 1, and on to 3 GiB" );
    errno = 0;
    d_tests_file_expect( (d_file_tell_stream32(f) == -1) &&
                         (errno == EOVERFLOW),
                         "tell32 refuses 3 GiB with EOVERFLOW" );
    d_tests_file_expect( (fputc('!',
                                f) == '!') &&
                         (d_file_close_stream(f) == 0),
                         "write a byte at 3 GiB" );

    return (g_failures == before);
}

/*
d_tests_file_extent_read
  Reads the 3 GiB + 1 file back through the 32-bit twins.
  Tests the following:
  - d_file_stat32 holds 3 GiB + 1 in its uint32_t st_size
  - d_file_size32's int32_t refuses it with EOVERFLOW, and still reports a
    small file
  - d_file_pread_fd32 reads the hole as zeros, and d_file_pwrite_fd32 writes
    near 2 GiB where pread reads it back
*/
static bool
d_tests_file_extent_read(
    const struct d_tests_file_paths* _p
)
{
    const int         before = g_failures;
    struct d_stat32_t s32    = {0};
    char              buf[8] = { 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x' };
    const int         fd     = d_file_open(_p->big3,
                                           O_RDWR);

    d_tests_file_expect( (d_file_stat32(_p->big3,
                                        &s32) == 0) &&
                         (s32.st_size == 0xC0000001u),
                         "stat32 holds 3 GiB + 1" );
    errno = 0;
    d_tests_file_expect( (d_file_size32(_p->big3) == -1) &&
                         (errno == EOVERFLOW),
                         "size32 refuses 3 GiB + 1 with EOVERFLOW" );
    d_tests_file_expect(d_file_size32(_p->small) == 10,
                        "size32 of the ten-byte file");
    d_tests_file_expect( (d_file_pread_fd32(fd,
                                            buf,
                                            4,
                                            0x40000000) == 4) &&
                         (buf[0] == 0) &&
                         (buf[3] == 0),
                         "pread32 reads the hole at 1 GiB as zeros" );
    d_tests_file_expect( (d_file_pwrite_fd32(fd,
                                             "abc",
                                             3,
                                             0x7FFFFFF0) == 3) &&
                         (d_file_pread_fd32(fd,
                                            buf,
                                            3,
                                            0x7FFFFFF0) == 3) &&
                         (memcmp(buf,
                                 "abc",
                                 3) == 0),
                         "pwrite32 near 2 GiB, and pread32 reads it back" );
    d_file_close_fd(fd);

    return (g_failures == before);
}

/*
d_tests_file_times
  Times at 32 bits, set through utimes.
  Tests the following:
  - a 2030 time fits d_stat32_t, and reads back exactly
  - a 2040 time does not: d_file_stat32 fails with EOVERFLOW and zeroes its
    buffer -- checked only where this build's time_t can set 2040
*/
static bool
d_tests_file_times(
    const struct d_tests_file_paths* _p
)
{
    const int         before = g_failures;
    struct d_stat32_t s32    = {0};

    d_tests_file_expect( (d_tests_file_set_mtime(_p->small,
                                                 1893456000.0) == 0) &&
                         (d_file_stat32(_p->small,
                                        &s32) == 0) &&
                         (s32.st_modified == 1893456000) &&
                         (s32.st_size == 10),
                         "stat32 reads a 2030 time" );

    // 2040 only where time_t can say it
    if (sizeof(time_t) >= 8)
    {
        d_tests_file_expect(d_tests_file_set_mtime(_p->small,
                                                   2208988800.0) == 0,
                            "set a 2040 time");
        memset(&s32,
               0xAB,
               sizeof(s32));
        errno = 0;
        d_tests_file_expect( (d_file_stat32(_p->small,
                                            &s32) == -1) &&
                             (errno == EOVERFLOW) &&
                             (s32.st_size == 0) &&
                             (s32.st_mode == 0),
                             "stat32 refuses 2040, EOVERFLOW, zeroed" );

        // back to a time that fits, for the groups that follow
        d_tests_file_set_mtime(_p->small,
                               1893456000.0);
    }

    return (g_failures == before);
}

#if D_FILE_LINK_IS_AVAILABLE
/*
d_tests_file_kinds_link
  The mode through a symbolic link, with and without following it.
*/
static void
d_tests_file_kinds_link(
    const struct d_tests_file_paths* _p
)
{
    uint32_t mode = 0;

    d_tests_file_expect(d_file_symlink("big3.bin",
                                       _p->link) == 0,
                        "link to the 3 GiB file");
    d_tests_file_expect( (d_file_mode_nofollow(_p->link,
                                               &mode) == 0) &&
                         (S_ISLNK(mode)),
                         "mode_nofollow sees the link" );
    d_tests_file_expect( (d_file_mode(_p->link,
                                      &mode) == 0) &&
                         (S_ISREG(mode)),
                         "mode follows it to a regular file" );
    d_file_remove(_p->link);

    return;
}
#else
/*
d_tests_file_kinds_link
  Nothing to check: this platform has no symbolic links.
*/
static void
d_tests_file_kinds_link(
    const struct d_tests_file_paths* _p
)
{
    (void)_p;

    return;
}
#endif  // D_FILE_LINK_IS_AVAILABLE

/*
d_tests_file_kinds
  What a path names, which has no width.
  Tests the following:
  - the predicates answer for a file too large for d_stat32_t
  - d_file_mode and d_file_mode_nofollow, through a link where there are
    links
  - an absent path fails with ENOENT
*/
static bool
d_tests_file_kinds(
    const struct d_tests_file_paths* _p
)
{
    const int before = g_failures;
    uint32_t  mode   = 0;

    d_tests_file_expect( (d_file_exists(_p->big3) != 0) &&
                         (d_file_is_regular(_p->big3) != 0),
                         "the 3 GiB file exists, and is a regular file" );
    d_tests_file_expect( (d_file_mode(_p->big3,
                                      &mode) == 0) &&
                         (S_ISREG(mode)),
                         "mode of the 3 GiB file" );
    d_tests_file_kinds_link(_p);
    errno = 0;
    d_tests_file_expect( (d_file_mode(_p->absent,
                                      &mode) == -1) &&
                         (errno == ENOENT),
                         "mode of an absent path fails with ENOENT" );

    return (g_failures == before);
}

/*
d_tests_file_count32
  Reads a directory to its end at 32 bits, counting the entries other than
"." and "..", each with an inode number, and in *_over every entry the reader
skipped because its inode number did not fit (EOVERFLOW).
*/
static int
d_tests_file_count32(
    struct d_dir_t* _dir,
    int*            _over
)
{
    int count = 0;

    *_over = 0;

    // until a NULL with errno clear: the end
    for (;;)
    {
        errno = 0;

        const struct d_dirent32_t* const e = d_dir_read32(_dir);

        // a NULL is the end, or an entry too wide, which the reader skips
        if (!e)
        {
            if (errno != EOVERFLOW)
            {
                break;
            }

            ++*_over;

            continue;
        }

        count += ( (e->d_name[0] != '.') &&
                   (e->d_ino != 0) );
    }

    return count;
}

/*
d_tests_file_count_names
  Reads a directory to its end by name, counting the entries other than "."
and "..", and in *_regular how many are regular files. The reader has no
width, so no entry fails; errno is as the end left it.
*/
static int
d_tests_file_count_names(
    struct d_dir_t* _dir,
    int*            _regular
)
{
    int         count = 0;
    uint8_t     type  = 0;
    const char* name  = NULL;

    *_regular = 0;
    errno     = 0;

    // every name, until the end
    while ( (name = d_dir_read_name(_dir,
                                    &type)) != NULL )
    {
        // "." and ".." are no fixture's
        if (name[0] != '.')
        {
            ++count;
            *_regular += (type == DT_REG);
        }
    }

    return count;
}

/*
d_tests_file_directory
  A directory read at 32 bits and by name, one handle serving both.
  Tests the following:
  - d_dir_read32 sees both entries, each with an inode number
  - after d_dir_rewind, d_dir_read_name sees both again, as regular files,
    and ends with errno clear
*/
static bool
d_tests_file_directory(
    const struct d_tests_file_paths* _p
)
{
    const int             before  = g_failures;
    int                   over    = 0;
    int                   regular = 0;
    struct d_dir_t* const d       = d_dir_open(_p->dir);

    // without the handle nothing below can run
    if (!d)
    {
        d_tests_file_expect(false,
                            "open the directory");

        return false;
    }

    d_tests_file_expect( (d_tests_file_count32(d,
                                               &over) == 2) &&
                         (over == 0),
                         "read32 sees both entries" );
    d_tests_file_expect( (d_dir_rewind(d) == 0) &&
                         (d_tests_file_count_names(d,
                                                   &regular) == 2) &&
                         (regular == 2) &&
                         (errno == 0),
                         "read_name sees both again, as regular files" );
    d_tests_file_expect(d_dir_close(d) == 0,
                        "close the directory");

    return (g_failures == before);
}

/*
d_tests_file_capacity
  Capacity in KiB, the 32-bit twin's unit: present, and consistent -- or, on
a filesystem of 4 TiB or more, refused with EOVERFLOW, as it must be.
*/
static bool
d_tests_file_capacity(
    const struct d_tests_file_paths* _p
)
{
    const int          before = g_failures;
    struct d_space32_t k      = {0};

    errno = 0;

    const int rc = d_file_space32(_p->root,
                                  &k);

    // a scratch filesystem too large for the count is not this check's
    if ( (rc == -1) &&
         (errno == EOVERFLOW) )
    {
        printf("  note: the scratch filesystem is 4 TiB or more\n");

        return true;
    }

    d_tests_file_expect( (rc == 0) &&
                         (k.capacity_kib > 0) &&
                         (k.available_kib <= k.free_kib) &&
                         (k.free_kib <= k.capacity_kib),
                         "space32 reports consistent KiB" );

    return (g_failures == before);
}

/*
d_tests_file_defaults
  The unsuffixed names are the twins at D_FILE_WIDTH.
  Tests the following:
  - d_off_t and d_stat_t's st_size have D_FILE_WIDTH bits
  - d_file_size reports 3 GiB + 1 at 64, and refuses it with EOVERFLOW at 32
  - d_file_stat fills a d_stat_t
*/
static bool
d_tests_file_defaults(
    const struct d_tests_file_paths* _p
)
{
    const int       before = g_failures;
    struct d_stat_t st     = {0};

    d_tests_file_expect( ((int)(sizeof(d_off_t) * 8) == D_FILE_WIDTH) &&
                         ((int)(sizeof(st.st_size) * 8) == D_FILE_WIDTH),
                         "the unsuffixed types have D_FILE_WIDTH bits" );
    errno = 0;

    const d_off_t size = d_file_size(_p->big3);

    // the one answer each width gives
    if (D_FILE_WIDTH == 64)
    {
        d_tests_file_expect(size == (d_off_t)0xC0000001u,
                            "d_file_size reports 3 GiB + 1 at 64");
    }
    else
    {
        d_tests_file_expect( (size == -1) &&
                             (errno == EOVERFLOW),
                             "d_file_size refuses 3 GiB + 1 at 32" );
    }

    d_tests_file_expect( (d_file_stat(_p->small,
                                      &st) == 0) &&
                         (st.st_size == 10),
                         "d_file_stat of the ten-byte file" );

    return (g_failures == before);
}

#if D_FILE_64_IS_AVAILABLE
/*
d_tests_file_wide_make
  Makes a 5 GiB + 1 file through the 64-bit twins, and reads it back.
  Tests the following:
  - d_file_truncate_fd64, d_file_pwrite_fd64 and d_file_pread_fd64 past
    4 GiB
  - d_file_stat64 and d_file_size64 report it; d_file_stat32 refuses it with
    EOVERFLOW and zeroes its buffer
*/
static bool
d_tests_file_wide_make(
    const struct d_tests_file_paths* _p
)
{
    const int         before = g_failures;
    const d_off64_t   five   = (d_off64_t)5 * 1024 * 1024 * 1024 + 1;
    struct d_stat64_t s64    = {0};
    struct d_stat32_t s32    = {0};
    char              buf[4] = {0};
    const int         fd     = d_file_open(_p->big5,
                                           O_RDWR | O_CREAT | O_TRUNC,
                                           0644);

    d_tests_file_expect( (d_file_truncate_fd64(fd,
                                               five) == 0) &&
                         (d_file_pwrite_fd64(fd,
                                             "xyz",
                                             3,
                                             five - 3) == 3) &&
                         (d_file_pread_fd64(fd,
                                            buf,
                                            3,
                                            five - 3) == 3) &&
                         (memcmp(buf,
                                 "xyz",
                                 3) == 0),
                         "truncate64 to 5 GiB + 1, and write its end" );
    d_file_close_fd(fd);
    d_tests_file_expect( (d_file_stat64(_p->big5,
                                        &s64) == 0) &&
                         (s64.st_size == (uint64_t)five) &&
                         (d_file_size64(_p->big5) == five),
                         "stat64 and size64 report 5 GiB + 1" );
    memset(&s32,
           0xAB,
           sizeof(s32));
    errno = 0;
    d_tests_file_expect( (d_file_stat32(_p->big5,
                                        &s32) == -1) &&
                         (errno == EOVERFLOW) &&
                         (s32.st_size == 0) &&
                         (s32.st_ino == 0),
                         "stat32 refuses 5 GiB + 1, EOVERFLOW, zeroed" );

    return (g_failures == before);
}

/*
d_tests_file_wide_stream
  A stream's position and length past 4 GiB through the 64-bit twins, and a
position the 32-bit tell can still report.
*/
static bool
d_tests_file_wide_stream(
    const struct d_tests_file_paths* _p
)
{
    const int       before = g_failures;
    const d_off64_t five   = (d_off64_t)5 * 1024 * 1024 * 1024 + 1;
    FILE* const     f      = d_file_open_stream(_p->big5,
                                                "rb+");

    // without the stream nothing below can run
    if (!f)
    {
        d_tests_file_expect(false,
                            "open the 5 GiB file as a stream");

        return false;
    }

    d_tests_file_expect( (d_file_seek_stream64(f,
                                               0,
                                               SEEK_END) == 0) &&
                         (d_file_tell_stream64(f) == five),
                         "seek64 to the end, and tell64 reports it" );
    d_tests_file_expect( (d_file_seek_stream64(f,
                                               100,
                                               SEEK_SET) == 0) &&
                         (d_file_tell_stream32(f) == 100),
                         "tell32 reports a position that fits" );
    d_tests_file_expect( (d_file_truncate_stream64(f,
                                                   1000) == 0) &&
                         (d_file_close_stream(f) == 0),
                         "truncate_stream64 to 1000 bytes" );
    d_tests_file_expect( (d_file_size64(_p->big5) == 1000) &&
                         (d_file_size32(_p->big5) == 1000),
                         "both sizes report 1000" );

    return (g_failures == before);
}

/*
d_tests_file_wide_before
  A time before 1901, which needs 64 bits -- where the filesystem keeps it.
ext4 clamps it to 1901-12-13, which fits 32, so either answer is right as
long as it agrees with what d_file_stat64 read.
*/
static void
d_tests_file_wide_before(
    const struct d_tests_file_paths* _p
)
{
    struct d_stat64_t s64 = {0};
    struct d_stat32_t s32 = {0};

    d_tests_file_set_mtime(_p->small,
                           -2208988800.0);
    d_tests_file_expect(d_file_stat64(_p->small,
                                      &s64) == 0,
                        "stat64 of a time before 1901");
    errno = 0;

    const int rc = d_file_stat32(_p->small,
                                 &s32);

    // kept, it cannot fit; clamped, it must read back the same
    if (s64.st_modified < -2147483647 - 1)
    {
        d_tests_file_expect( (rc == -1) &&
                             (errno == EOVERFLOW),
                             "stat32 refuses a time before 1901" );
    }
    else
    {
        d_tests_file_expect( (rc == 0) &&
                             (s32.st_modified ==
                              (int32_t)s64.st_modified),
                             "stat32 reads the time the filesystem kept" );
    }

    // back to a time that fits, for the groups that follow
    d_tests_file_set_mtime(_p->small,
                           1893456000.0);

    return;
}

/*
d_tests_file_wide_rest
  The other 64-bit twins: the directory, the capacity in bytes, and a time
before 1901 where time_t can set one.
*/
static bool
d_tests_file_wide_rest(
    const struct d_tests_file_paths* _p
)
{
    const int             before = g_failures;
    struct d_space64_t    b      = {0};
    struct d_space32_t    k      = {0};
    struct d_dir_t* const d      = d_dir_open(_p->dir);
    int                   count  = 0;

    // every entry the 64-bit reader sees, the two dots included
    while ( (d != NULL) &&
            (d_dir_read64(d) != NULL) )
    {
        ++count;
    }

    d_tests_file_expect( (count == 4) &&
                         (d_dir_close(d) == 0),
                         "read64 sees both entries and the two dots" );
    d_tests_file_expect( (d_file_space64(_p->root,
                                         &b) == 0) &&
                         ( (d_file_space32(_p->root,
                                           &k) != 0) ||
                           (k.capacity_kib ==
                            (uint32_t)(b.capacity / 1024u)) ),
                         "space32, where it fits, is space64 in KiB" );

    // before 1901 only where time_t can say it
    if (sizeof(time_t) >= 8)
    {
        d_tests_file_wide_before(_p);
    }

    return (g_failures == before);
}

/*
d_tests_file_wide
  The 64-bit twins at full width, from a process of either pointer width.
  Tests the following:
  - a 5 GiB + 1 file, made, read and measured (d_tests_file_wide_make)
  - a stream past 4 GiB (d_tests_file_wide_stream)
  - the directory, capacity in bytes and a time before 1901
    (d_tests_file_wide_rest)
*/
static bool
d_tests_file_wide(
    const struct d_tests_file_paths* _p
)
{
    const bool made   = d_tests_file_wide_make(_p);
    const bool stream = d_tests_file_wide_stream(_p);
    const bool rest   = d_tests_file_wide_rest(_p);

    d_file_remove(_p->big5);

    return (made && stream && rest);
}
#else
/*
d_tests_file_wide
  Nothing to check: below the 64-bit floor there are no 64-bit twins.
*/
static bool
d_tests_file_wide(
    const struct d_tests_file_paths* _p
)
{
    (void)_p;

    return true;
}
#endif  // D_FILE_64_IS_AVAILABLE

/*
d_tests_file_shim_make
  Makes what the shim widens: a directory of five empty files, two named
wide*, and a directory it reports a 5 TiB filesystem under.
*/
static void
d_tests_file_shim_make(
    const struct d_tests_file_paths* _p
)
{
    const char* const names[]   = { "a", "wide1", "b", "wide2", "c" };
    char              path[512] = {0};

    d_dir_create(_p->wdir,
                 0755);
    d_dir_create(_p->fake,
                 0755);

    // the five entries
    for (size_t i = 0; i < (sizeof(names) / sizeof(names[0])); ++i)
    {
        d_tests_file_join(path,
                          _p->wdir,
                          names[i]);
        d_tests_file_touch(path);
    }

    return;
}

/*
d_tests_file_shim
  What the shim makes too wide for 32 bits.
  Tests the following:
  - d_dir_read32 skips exactly the two entries whose inode numbers need more
    than 32 bits, each with EOVERFLOW, and reads on past them to the end
  - d_dir_read_name reads all five, as regular files, to a clean end
  - d_file_space32 refuses a 5 TiB filesystem with EOVERFLOW, zeroed
*/
static bool
d_tests_file_shim(
    const struct d_tests_file_paths* _p
)
{
    const int             before  = g_failures;
    int                   over    = 0;
    int                   regular = 0;
    struct d_space32_t    k       = {0};
    struct d_dir_t* const d       = d_dir_open(_p->wdir);

    // without the handle the reads below have nothing to read
    if (!d)
    {
        d_tests_file_expect(false,
                            "open the shim's directory");

        return false;
    }

    d_tests_file_expect( (d_tests_file_count32(d,
                                               &over) == 3) &&
                         (over == 2),
                         "read32 skips the two wide entries, EOVERFLOW" );
    d_tests_file_expect( (d_dir_rewind(d) == 0) &&
                         (d_tests_file_count_names(d,
                                                   &regular) == 5) &&
                         (regular == 5) &&
                         (errno == 0),
                         "read_name reads all five to a clean end" );
    d_dir_close(d);
    memset(&k,
           0xAB,
           sizeof(k));
    errno = 0;
    d_tests_file_expect( (d_file_space32(_p->fake,
                                         &k) == -1) &&
                         (errno == EOVERFLOW) &&
                         (k.capacity_kib == 0) &&
                         (k.available_kib == 0),
                         "space32 refuses 5 TiB, EOVERFLOW, zeroed" );

    return (g_failures == before);
}

#if D_FILE_64_IS_AVAILABLE
/*
d_tests_file_shim64
  The 64-bit twins over what the shim widens: every entry, and 5 TiB.
*/
static bool
d_tests_file_shim64(
    const struct d_tests_file_paths* _p
)
{
    const int             before = g_failures;
    struct d_space64_t    b      = {0};
    struct d_dir_t* const d      = d_dir_open(_p->wdir);
    int                   count  = 0;

    // every entry, the wide ones included
    while ( (d != NULL) &&
            (d_dir_read64(d) != NULL) )
    {
        ++count;
    }

    d_tests_file_expect( (count == 7) &&
                         (d_dir_close(d) == 0),
                         "read64 reads all five and the two dots" );
    d_tests_file_expect( (d_file_space64(_p->fake,
                                         &b) == 0) &&
                         ((b.capacity >> 40) == 5),
                         "space64 reports 5 TiB" );

    return (g_failures == before);
}
#else
/*
d_tests_file_shim64
  Nothing to check: below the 64-bit floor there are no 64-bit twins.
*/
static bool
d_tests_file_shim64(
    const struct d_tests_file_paths* _p
)
{
    (void)_p;

    return true;
}
#endif  // D_FILE_64_IS_AVAILABLE

#if defined(STATX_BTIME)
/*
d_tests_file_birth
  The creation time statx reports for a path, the reference d_stat_t's
st_created must match: 0 where the filesystem keeps none, and -2 where statx
itself fails, which no st_created matches.
*/
static double
d_tests_file_birth(
    const char* _path
)
{
    struct statx stx = {0};

    // no reference at all fails the check that reads it
    if (statx(AT_FDCWD,
              _path,
              0,
              STATX_BTIME,
              &stx) != 0)
    {
        return -2.0;
    }

    return (stx.stx_mask & STATX_BTIME) ? (double)stx.stx_btime.tv_sec
                                        : 0.0;
}
#else
/*
d_tests_file_birth
  No reference: this build cannot ask the system itself, so -1, which lets
any creation time but 0 pass.
*/
static double
d_tests_file_birth(
    const char* _path
)
{
    (void)_path;

    return -1.0;
}
#endif  // STATX_BTIME

/*
d_tests_file_copy_make
  Writes 1 MiB of a pattern no off-by-one copy reproduces to _path, and keeps
it in _out for the comparison.
*/
static bool
d_tests_file_copy_make(
    const char* _path,
    char*       _out,
    size_t      _size
)
{
    FILE* const f = d_file_open_stream(_path,
                                       "wb");

    // the pattern: a multiplicative hash of the offset
    for (size_t i = 0; i < _size; ++i)
    {
        _out[i] = (char)((i * 2654435761u) >> 13);
    }

    return ( (f != NULL) &&
             (fwrite(_out,
                     1,
                     _size,
                     f) == _size) &&
             (d_file_close_stream(f) == 0) );
}

/*
d_tests_file_copy
  A copy, by whichever engine the build compiled, and a creation time.
  Tests the following:
  - d_file_copy of 1 MiB is exact (ci/check_filesys.sh counts, through the
    shim, whether copy_file_range made it)
  - st_created is what statx reports where D_FILE_STAT_HAS_BIRTHTIME says
    there is one, and 0 where it says there is none
*/
static bool
d_tests_file_copy(
    const struct d_tests_file_paths* _p
)
{
    static char     out[1 << 20]  = {0};
    static char     back[1 << 20] = {0};
    const int       before        = g_failures;
    struct d_stat_t st            = {0};

    d_tests_file_expect( (d_tests_file_copy_make(_p->src,
                                                 out,
                                                 sizeof(out))) &&
                         (d_file_copy(_p->src,
                                      _p->dst) == 0),
                         "copy 1 MiB" );

    FILE* const f = d_file_open_stream(_p->dst,
                                       "rb");

    d_tests_file_expect( (f != NULL) &&
                         (fread(back,
                                1,
                                sizeof(back),
                                f) == sizeof(back)) &&
                         (d_file_close_stream(f) == 0) &&
                         (memcmp(out,
                                 back,
                                 sizeof(out)) == 0),
                         "the copy is exact" );

    const double reference = d_tests_file_birth(_p->dst);
    const bool   stat_ok   = (d_file_stat(_p->dst,
                                          &st) == 0);

    // a creation time exactly where the build says there is one
    if (!D_FILE_STAT_HAS_BIRTHTIME)
    {
        d_tests_file_expect( (stat_ok) &&
                             (st.st_created == 0),
                             "no creation time: st_created reads 0" );
    }
    else if (reference == -1.0)
    {
        d_tests_file_expect( (stat_ok) &&
                             (st.st_created != 0),
                             "a creation time" );
    }
    else
    {
        d_tests_file_expect( (stat_ok) &&
                             ((double)st.st_created == reference),
                             "st_created is the creation time statx reads" );
    }

    d_file_remove(_p->src);
    d_file_remove(_p->dst);

    return (g_failures == before);
}

/*
d_tests_file_teardown
  Removes every fixture, so that a scratch directory can be used again.
*/
static void
d_tests_file_teardown(
    const struct d_tests_file_paths* _p
)
{
    const char* const files[]   = { "dir/a", "dir/b", "wdir/a", "wdir/wide1",
                                    "wdir/b", "wdir/wide2", "wdir/c" };
    char              path[512] = {0};

    // the files the directories hold, then the directories
    for (size_t i = 0; i < (sizeof(files) / sizeof(files[0])); ++i)
    {
        d_tests_file_join(path,
                          _p->root,
                          files[i]);
        d_file_remove(path);
    }

    d_dir_remove(_p->dir);
    d_dir_remove(_p->wdir);
    d_dir_remove(_p->fake);
    d_file_remove(_p->big3);
    d_file_remove(_p->small);

    return;
}

/*
d_tests_file_run
  Runs every group this build can check, in order: the later ones read what
the earlier ones made.
*/
static void
d_tests_file_run(
    const struct d_tests_file_paths* _p
)
{
    d_tests_file_report(d_tests_file_setup(_p),
                        "setup");
    d_tests_file_report(d_tests_file_extent_make(_p),
                        "3 GiB through the 32-bit twins");
    d_tests_file_report(d_tests_file_extent_read(_p),
                        "3 GiB read back at 32");
    d_tests_file_report(d_tests_file_times(_p),
                        "times at 32");
    d_tests_file_report(d_tests_file_kinds(_p),
                        "types and modes");
    d_tests_file_report(d_tests_file_directory(_p),
                        "a directory at 32 and by name");
    d_tests_file_report(d_tests_file_capacity(_p),
                        "capacity in KiB");
    d_tests_file_report(d_tests_file_defaults(_p),
                        "the unsuffixed names");

    // the 64-bit twins, where the build has them
    if (D_FILE_64_IS_AVAILABLE)
    {
        d_tests_file_report(d_tests_file_wide(_p),
                            "the 64-bit twins");
    }

    // what only the shim makes, where the build expects to run under it
    if (g_shim)
    {
        d_tests_file_shim_make(_p);
        d_tests_file_report(d_tests_file_shim(_p),
                            "the shim's wide entries and 5 TiB, at 32");
    }

    // and the same at 64
    if ( (g_shim) &&
         (D_FILE_64_IS_AVAILABLE) )
    {
        d_tests_file_report(d_tests_file_shim64(_p),
                            "the shim's wide entries and 5 TiB, at 64");
    }

    d_tests_file_report(d_tests_file_copy(_p),
                        "a copy and a creation time");

    return;
}

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
    // the scratch directory is the one argument
    if (_argc != 2)
    {
        printf("usage: t_file_width <scratch directory>\n");

        return 2;
    }

    const struct d_tests_file_paths paths = d_tests_file_paths_make(_argv[1]);

    printf("t_file_width: D_FILE_WIDTH %d, D_FILE_64_IS_AVAILABLE %d,"
           " narrowed %d, shim %d\n",
           (int)D_FILE_WIDTH,
           (int)D_FILE_64_IS_AVAILABLE,
           (int)D_FILE_WIDTH_IS_NARROWED,
           (int)g_shim);
    d_tests_file_run(&paths);
    d_tests_file_teardown(&paths);
    printf("t_file_width: %d checks, %d failed\n",
           g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
