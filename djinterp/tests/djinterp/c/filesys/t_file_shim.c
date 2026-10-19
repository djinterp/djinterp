/*******************************************************************************
* djinterp [c]                                                     t_file_shim.c
*
*   An LD_PRELOAD library for t_file_width.c and t_file_width.cpp, so that the
* 32-bit twins' EOVERFLOW paths run on an ordinary disk, and a copy's engine
* can be told from the outside:
*   - a directory entry named wide* reads back with its inode number raised
*     past 32 bits;
*   - a filesystem queried through a path naming fake5t reports 5 TiB;
*   - every copy_file_range call is counted, and a process that made one
*     prints "t_file_shim: copy_file_range calls N" to stderr at exit.
*   It wraps both spellings of each call: the large-file one (readdir64,
* statvfs64), which a library built with -D_FILE_OFFSET_BITS=64 calls, and
* the plain one, which a 64-bit build calls; a plain entry widens only where
* its inode number has the room.
*   Linux and glibc only, and built with -D_GNU_SOURCE for RTLD_NEXT:
*     cc -shared -fPIC -D_GNU_SOURCE -Wall -Wextra -Werror                    \
*        tests/djinterp/c/filesys/t_file_shim.c -o t_file_shim.so -ldl
*
*
* path:      /tests/djinterp/c/filesys/t_file_shim.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/
// std
#include <dirent.h>       // DIR, struct dirent, struct dirent64
#include <dlfcn.h>        // dlsym, RTLD_NEXT
#include <stdio.h>        // fprintf, stderr
#include <stdlib.h>       // atexit
#include <string.h>       // strncmp, strstr
#include <sys/statvfs.h>  // struct statvfs, struct statvfs64
#include <sys/types.h>    // loff_t, ssize_t
#include <unistd.h>       // copy_file_range


static int g_copies = 0;

/*
d_tests_shim_next
  The next definition of a symbol after this library's -- the C library's --
as an object pointer, which the callers below store into function pointers
through memcpy, ISO C having no conversion between the two.
*/
static void*
d_tests_shim_next(
    const char* _symbol
)
{
    return dlsym(RTLD_NEXT,
                 _symbol);
}

/*
d_tests_shim_wide
  Whether an entry is one the shim widens: its name begins with "wide".
*/
static int
d_tests_shim_wide(
    const char* _name
)
{
    return (strncmp(_name,
                    "wide",
                    4) == 0);
}

/*
d_tests_shim_fake
  Whether a path is one the shim reports 5 TiB under: it names fake5t.
*/
static int
d_tests_shim_fake(
    const char* _path
)
{
    return (strstr(_path,
                   "fake5t") != NULL);
}

/*
d_tests_shim_report
  Prints the count of copy_file_range calls; registered by the first one.
*/
static void
d_tests_shim_report(
    void
)
{
    fprintf(stderr,
            "t_file_shim: copy_file_range calls %d\n",
            g_copies);

    return;
}

/*
readdir64
  The C library's, with a wide* entry's inode number raised by 2^32.
*/
struct dirent64*
readdir64(
    DIR* _dir
)
{
    struct dirent64* (*real)(DIR*) = NULL;
    void* const              next  = d_tests_shim_next("readdir64");

    memcpy(&real,
           &next,
           sizeof(real));

    struct dirent64* const e = real(_dir);

    // only the entries the fixtures name wide*
    if ( (e != NULL) &&
         (d_tests_shim_wide(e->d_name)) )
    {
        e->d_ino += ((ino64_t)1 << 16) << 16;
    }

    return e;
}

/*
readdir
  The same, for a build whose plain readdir has a 64-bit inode number; a
narrower one is left as it is.
*/
struct dirent*
readdir(
    DIR* _dir
)
{
    struct dirent* (*real)(DIR*) = NULL;
    void* const            next  = d_tests_shim_next("readdir");

    memcpy(&real,
           &next,
           sizeof(real));

    struct dirent* const e = real(_dir);

    // only the entries the fixtures name wide*, and only where there is room
    if ( (e != NULL) &&
         (sizeof(e->d_ino) >= 8) &&
         (d_tests_shim_wide(e->d_name)) )
    {
        e->d_ino += ((ino_t)1 << 16) << 16;
    }

    return e;
}

/*
statvfs64
  The C library's, reporting 5 TiB in 4 KiB blocks under a fake5t path.
*/
int
statvfs64(
    const char*        _path,
    struct statvfs64*  _buf
)
{
    int (*real)(const char*, struct statvfs64*) = NULL;
    void* const next = d_tests_shim_next("statvfs64");

    memcpy(&real,
           &next,
           sizeof(real));

    const int rc = real(_path,
                        _buf);

    // only the directory the fixtures name fake5t
    if ( (rc == 0) &&
         (d_tests_shim_fake(_path)) )
    {
        _buf->f_frsize = 4096;
        _buf->f_blocks = (fsblkcnt64_t)5 << 28;
    }

    return rc;
}

/*
statvfs
  The same, for the plain call; 5 TiB in 4 KiB blocks fits 32 bits.
*/
int
statvfs(
    const char*      _path,
    struct statvfs*  _buf
)
{
    int (*real)(const char*, struct statvfs*) = NULL;
    void* const next = d_tests_shim_next("statvfs");

    memcpy(&real,
           &next,
           sizeof(real));

    const int rc = real(_path,
                        _buf);

    // only the directory the fixtures name fake5t
    if ( (rc == 0) &&
         (d_tests_shim_fake(_path)) )
    {
        _buf->f_frsize = 4096;
        _buf->f_blocks = (fsblkcnt_t)5 << 28;
    }

    return rc;
}

/*
copy_file_range
  The C library's, counted; the first call arranges the report at exit.
*/
ssize_t
copy_file_range(
    int          _in,
    loff_t*      _in_offset,
    int          _out,
    loff_t*      _out_offset,
    size_t       _length,
    unsigned int _flags
)
{
    ssize_t (*real)(int, loff_t*, int, loff_t*, size_t, unsigned int) = NULL;
    void* const next = d_tests_shim_next("copy_file_range");

    memcpy(&real,
           &next,
           sizeof(real));

    // the first call arranges the report
    if (g_copies++ == 0)
    {
        atexit(&d_tests_shim_report);
    }

    return real(_in,
                _in_offset,
                _out,
                _out_offset,
                _length,
                _flags);
}
