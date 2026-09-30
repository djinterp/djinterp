/*******************************************************************************
* djinterp [c]                                                        file_dir.c
*
* Implementation of the directory operations declared in file_dir.h.
*   struct d_dir_t wraps the platform's handle -- a DIR* on POSIX, a
* FindFirstFile search on Windows -- and owns the entry d_dir_read hands out,
* so the entry's lifetime is the handle's on every target. Entry types come
* from the kernel where it reports them, and from one stat per entry where it
* does not.
*
*
* path:      /src/djinterp/c/fs/file_dir.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_dir.h"  // corresponding header
// std
#include <errno.h>   // errno, EEXIST, EINVAL, ENAMETOOLONG, ENOENT, ENOTDIR
#include <stddef.h>  // NULL, size_t
#include <stdint.h>  // uint8_t, uint32_t, uint64_t
#include <stdlib.h>  // realpath
#include <string.h>  // memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*, d_internal_file_alloc
#include "../../../../inc/djinterp/c/fs/file_path.h"    // d_path_join, d_path_root_length
#include "../../../../inc/djinterp/c/fs/file_stat.h"    // d_file_stat_nofollow, d_dir_exists
#include "../../../../inc/djinterp/config/c/fs/cfg_file_dir.h"  // D_INTERNAL_FILE_DIR_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_dir_t
//   struct: the opaque directory handle promised by file_common.h. It owns the
// d_dirent_t that d_dir_read hands back, which makes that pointer's lifetime a
// property of the HANDLE rather than of the platform: POSIX readdir returns a
// pointer into the DIR's own storage and says almost nothing about how long
// it lasts, while a copy here gives one answer on every target -- valid until
// the next d_dir_read on this handle, dead at d_dir_close.
struct d_dir_t
{
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    HANDLE            handle;                    // FindFirstFile handle
    WIN32_FIND_DATAA  find_data;                 // the entry Win32 hands back
    int               pending;                   // 1 when find_data is unread
    char              pattern[D_FILE_PATH_MAX];  // for rewind
#else
    DIR*              handle;                    // the platform's stream
#endif
    struct d_dirent_t entry;                     // what d_dir_read returns
    char              path[D_FILE_PATH_MAX];     // for the stat fallback
};

#if (D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1)

/*
d_internal_dir_type_by_stat
  The expensive path, taken only when the kernel would not say: one stat per
entry, which on a network filesystem is one round trip per entry (see
D_CFG_FILE_DIR_FILL_TYPE). lstat rather than stat, so a dangling symlink
reports DT_LNK instead of vanishing. An entry that cannot be described is
DT_UNKNOWN.
*/
static uint8_t
d_internal_dir_type_by_stat(
    struct d_dir_t* _dir,
    const char*     _name
)
{
    char full[D_FILE_PATH_MAX];

    // an entry whose full path cannot be built cannot be described
    if (!d_path_join(full,
                     sizeof(full),
                     _dir->path,
                     _name))
    {
        return DT_UNKNOWN;
    }

    struct d_stat_t st;

    // lstat: describe the link itself, so a dangling one is still DT_LNK
    // rather than an entry that appears not to exist
    if (d_file_stat_nofollow(full,
                             &st) != 0)
    {
        return DT_UNKNOWN;
    }

    // map the mode's file type onto the matching DT_* constant
    if (S_ISREG(st.st_mode))
    {
        return DT_REG;
    }

    if (S_ISDIR(st.st_mode))
    {
        return DT_DIR;
    }

    if (S_ISLNK(st.st_mode))
    {
        return DT_LNK;
    }

    if (S_ISCHR(st.st_mode))
    {
        return DT_CHR;
    }

    if (S_ISBLK(st.st_mode))
    {
        return DT_BLK;
    }

    if (S_ISFIFO(st.st_mode))
    {
        return DT_FIFO;
    }

    if (S_ISSOCK(st.st_mode))
    {
        return DT_SOCK;
    }

    return DT_UNKNOWN;
}

#endif  // D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1

/*
d_internal_dir_copy_name
  Local rather than d_strcpy_s from the string module: this is five lines, and
the alternative is that every consumer of file_dir links dstring for one call.
The fs subframework's only dependencies should be the ones it genuinely needs.
A name that does not fit is refused rather than truncated.
*/
static int
d_internal_dir_copy_name(
    char*       _dst,
    size_t      _dstsize,
    const char* _src
)
{
    const size_t length = strlen(_src);

    // a name that does not fit is refused, not truncated
    if ((length + 1) > _dstsize)
    {
        return -1;
    }

    memcpy(_dst,
           _src,
           length + 1);

    return 0;
}

/*
d_internal_dir_is_dots
  Recognises exactly "." and "..", by their bytes rather than by string
comparison.
*/
static int
d_internal_dir_is_dots(
    const char* _name
)
{
    // both names begin with a dot
    if (_name[0] != '.')
    {
        return 0;
    }

    // "."
    if (_name[1] == '\0')
    {
        return 1;
    }

    // ".."
    if ( (_name[1] == '.') &&
         (_name[2] == '\0') )
    {
        return 1;
    }

    return 0;
}

//==============================================================================
// 1.  DIRECTORIES
//==============================================================================

/*
d_dir_create
  mkdir, or the CRT's _mkdir, which takes no mode: Windows has no permission
bits for directories. A missing parent is a failure here -- creating parents
is d_dir_create_parents' job.
*/
int
d_dir_create(
    const char* _path,
    uint32_t    _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_create",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Windows has no permission bits for directories
    (void)_mode;
    const int result = _mkdir(_path);
#else
    const int result = mkdir(_path,
                             (mode_t)_mode);
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_create",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "mkdir failed");

        return -1;
    }

    return 0;
}

/*
d_dir_create_parents
  The request is "ensure this path exists", so an existing path satisfies it.
The path is copied into a working buffer and each component boundary is
briefly overwritten with a terminator, so every prefix can be created in
turn; the root is skipped, since "/" and "C:\" exist by definition and mkdir
on them fails in platform-specific ways. An EEXIST from any prefix is
expected -- another process building the same tree is the normal case -- and
is a failure only when the thing that exists is not a directory, or when it
is the leaf and D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK is 0. Parents get
D_CFG_FILE_DIR_CREATE_MODE: passing 0700 for "/a/b/c" means "c should be
private" far more often than "make /a and /a/b private too".
*/
int
d_dir_create_parents(
    const char* _path,
    uint32_t    _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_create_parents",
                            NULL,
                            "path is NULL",
                            -1);

    const size_t length = strlen(_path);
    char         work[D_FILE_PATH_MAX];

    // the working copy must hold the path and its terminator
    if ((length + 1) > sizeof(work))
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_dir_create_parents",
                             _path,
                             "path is longer than D_FILE_PATH_MAX",
                             -1);
    }

    // an empty path names nothing to create
    if (length == 0)
    {
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_dir_create_parents",
                             _path,
                             "path is empty",
                             -1);
    }

    memcpy(work,
           _path,
           length + 1);

    // never try to create the root itself: "/" and "C:\" already exist by
    // definition, and mkdir("/") returns EEXIST or EACCES depending on the
    // platform's mood
    const size_t root_len = d_path_root_length(work);
    size_t       idx      = (root_len > 0) ? root_len : 0;

    // walk the components, creating each prefix in turn
    for (; idx <= length; ++idx)
    {
        // act only where a component ends
        if ( (idx == length)    ||
             (work[idx] == '/') ||
             (work[idx] == D_FILE_PATH_SEP) )
        {
            // an empty component ("a//b") is not a directory to create
            if (idx == 0)
            {
                continue;
            }

            const char saved = work[idx];

            work[idx] = '\0';

            // a prefix that is still empty has nothing to create
            if (work[0] != '\0')
            {
                // the caller's mode is for the leaf; parents get the
                // configured one
                const uint32_t mode =
                    (idx == length) ? _mode
                                    : (uint32_t)D_INTERNAL_FILE_DIR_CREATE_MODE;

                // a failure matters only if a directory did not result
                if (d_dir_create(work,
                                 mode) != 0)
                {
                    // already there is the normal case, not a race: another
                    // process building the same tree is expected
                    if (errno != EEXIST)
                    {
                        work[idx] = saved;
                        D_INTERNAL_FILE_NOTIFY(
                            D_FILE_NOTIFY_ERROR,
                            errno,
                            "d_dir_create_parents",
                            D_INTERNAL_FILE_NOTIFY_PATH(_path),
                            "could not create an intermediate directory");

                        return -1;
                    }

                    // EEXIST on a NON-directory is a real failure: the path
                    // cannot be brought into existence as a directory
                    if (!d_dir_exists(work))
                    {
                        work[idx] = saved;
                        D_INTERNAL_FILE_FAIL(ENOTDIR,
                                             "d_dir_create_parents",
                                             _path,
                                             "a component exists and is "
                                             "not a directory",
                                             -1);
                    }

#if D_CFG_IS_OFF(D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK)
                    // the caller asked for mkdir semantics on the leaf
                    if (idx == length)
                    {
                        work[idx] = saved;
                        D_INTERNAL_FILE_FAIL(EEXIST,
                                             "d_dir_create_parents",
                                             _path,
                                             "target exists and "
                                             "EXISTING_OK is 0",
                                             -1);
                    }
#endif
                }
            }

            work[idx] = saved;
        }
    }

    return 0;
}

/*
d_dir_remove
  rmdir, or the CRT's _rmdir. Empty specifically: there is deliberately no
recursive form, because deleting a tree is a policy decision with a
symlink-traversal hazard attached.
*/
int
d_dir_remove(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_remove",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _rmdir(_path);
#else
    const int result = rmdir(_path);
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_remove",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "rmdir failed");

        return -1;
    }

    return 0;
}

/*
d_dir_open
  The handle is allocated through the fs allocator and keeps a copy of the
path, for the stat fallback. Win32 has no opendir: FindFirstFile both opens
AND reads the first entry, so that entry is stashed and marked pending for the
first d_dir_read to hand out -- skipping it is the classic bug in every
hand-rolled Win32 dirent shim. Any failure releases the handle before
reporting, keeping the platform's errno.
*/
struct d_dir_t*
d_dir_open(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_open",
                            NULL,
                            "path is NULL",
                            NULL);

    const size_t length = strlen(_path);

    // the handle keeps a copy of the path, which must fit
    if ((length + 1) > D_FILE_PATH_MAX)
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_dir_open",
                             _path,
                             "path is longer than D_FILE_PATH_MAX",
                             NULL);
    }

    struct d_dir_t* const dir =
        (struct d_dir_t*)d_internal_file_alloc(sizeof(struct d_dir_t));

    // d_internal_file_alloc has already reported the failure
    if (!dir)
    {
        return NULL;
    }

    memset(dir,
           0,
           sizeof(*dir));
    memcpy(dir->path,
           _path,
           length + 1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Win32 has no opendir: FindFirstFile both opens AND reads the first
    // entry. So the entry has to be stashed here and handed out by the first
    // d_dir_read, or it is silently skipped -- which is the classic bug in
    // every hand-rolled Win32 dirent shim.
    if (!d_path_join(dir->pattern,
                     sizeof(dir->pattern),
                     _path,
                     "*"))
    {
        d_internal_file_free(dir);

        return NULL;
    }

    dir->handle = FindFirstFileA(dir->pattern,
                                 &dir->find_data);

    // a search that cannot start releases the handle before reporting
    if (dir->handle == INVALID_HANDLE_VALUE)
    {
        d_internal_file_free(dir);
        D_INTERNAL_FILE_SET_ERR(ENOENT);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ENOENT,
                               "d_dir_open",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "opendir failed");

        return NULL;
    }

    dir->pending = 1;
#else
    dir->handle = opendir(_path);

    // a directory that cannot open releases the handle, keeping opendir's
    // errno across the release
    if (!dir->handle)
    {
        const int saved_errno = errno;

        d_internal_file_free(dir);
        errno = saved_errno;
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               saved_errno,
                               "d_dir_open",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "opendir failed");

        return NULL;
    }
#endif

    return dir;
}

/*
d_dir_read
  Each pass takes one platform entry, copies its name into the handle's own
entry -- so the result's lifetime is the handle's -- and fills the type:
from the find data on Windows, from the kernel where it reports d_type, and by
one lstat per entry where it answers DT_UNKNOWN. A name longer than
D_FILE_NAME_MAX is skipped with a warning rather than truncated, and "." and
".." are skipped when the build asks. POSIX readdir returns NULL for both the
end and an error, so errno is cleared before each call to tell them apart.
*/
struct d_dirent_t*
d_dir_read(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_read",
                            NULL,
                            "handle is NULL",
                            NULL);

    // take platform entries until one is worth returning
    for (;;)
    {
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        // hand out the entry FindFirstFile already produced before asking for
        // another one
        if (!_dir->pending)
        {
            // ERROR_NO_MORE_FILES is the end, not a failure
            if (!FindNextFileA(_dir->handle,
                               &_dir->find_data))
            {
                return NULL;
            }
        }

        _dir->pending = 0;

        // a name longer than D_FILE_NAME_MAX cannot be reported
        if (d_internal_dir_copy_name(_dir->entry.d_name,
                                     sizeof(_dir->entry.d_name),
                                     _dir->find_data.cFileName) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   ENAMETOOLONG,
                                   "d_dir_read",
                                   NULL,
                                   "entry name exceeds D_FILE_NAME_MAX; "
                                   "skipped");
            continue;
        }

        _dir->entry.d_ino = 0;   // Win32 has no inode to report

    #if D_CFG_IS_ON(D_CFG_FILE_DIR_FILL_TYPE)
        // the find data carries the type; a reparse point is reported as a
        // link
        if ((_dir->find_data.dwFileAttributes &
             FILE_ATTRIBUTE_REPARSE_POINT) != 0)
        {
            _dir->entry.d_type = DT_LNK;
        }
        else if ((_dir->find_data.dwFileAttributes &
                  FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            _dir->entry.d_type = DT_DIR;
        }
        else
        {
            _dir->entry.d_type = DT_REG;
        }
    #else
        _dir->entry.d_type = DT_UNKNOWN;
    #endif
#else
        // POSIX: readdir returns NULL for BOTH end-of-directory and error,
        // and errno is the only way to tell. Clear it so a stale value from
        // some earlier call cannot be mistaken for this one's failure.
        errno = 0;

        struct dirent* const native = readdir(_dir->handle);

        // the end, or a failure worth reporting
        if (!native)
        {
            // a set errno means readdir failed rather than ran out
            if (errno != 0)
            {
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                       errno,
                                       "d_dir_read",
                                       NULL,
                                       "readdir failed");
            }

            return NULL;
        }

        // copy out of the platform's storage into ours, so the lifetime of
        // what we return is a property of the handle
        if (d_internal_dir_copy_name(_dir->entry.d_name,
                                     sizeof(_dir->entry.d_name),
                                     native->d_name) != 0)
        {
            // a name longer than D_FILE_NAME_MAX cannot be reported; skipping
            // it silently would be worse than saying so
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   ENAMETOOLONG,
                                   "d_dir_read",
                                   NULL,
                                   "entry name exceeds D_FILE_NAME_MAX; "
                                   "skipped");
            continue;
        }

        _dir->entry.d_ino  = (uint64_t)native->d_ino;
        _dir->entry.d_type = DT_UNKNOWN;

    #if (D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL == 1)
        // free: the kernel already told us
        _dir->entry.d_type = (uint8_t)native->d_type;
    #endif

    #if (D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1)
        // ...except when it did not. XFS without ftype and most network
        // filesystems answer DT_UNKNOWN, and then the only way to know is to
        // ask -- one stat per entry. See D_CFG_FILE_DIR_FILL_TYPE.
        if (_dir->entry.d_type == DT_UNKNOWN)
        {
            _dir->entry.d_type = d_internal_dir_type_by_stat(
                                     _dir,
                                     _dir->entry.d_name);
        }
    #endif
#endif

#if D_CFG_IS_ON(D_CFG_FILE_DIR_SKIP_DOTS)
        // "." and ".." are not entries the caller asked to see
        if (d_internal_dir_is_dots(_dir->entry.d_name))
        {
            continue;
        }
#else
        (void)d_internal_dir_is_dots;
#endif

        return &_dir->entry;
    }
}

/*
d_dir_rewind
  Reports failure, unlike POSIX rewinddir, which returns void. That is fine on
POSIX, where the call cannot fail -- but Win32 has no rewind at all, so the
search is closed and restarted, and THAT can fail. A void return would hide
it and leave the caller iterating a dead handle.
*/
int
d_dir_rewind(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_rewind",
                            NULL,
                            "handle is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // no Win32 rewind: close the search and start it again
    if (_dir->handle != INVALID_HANDLE_VALUE)
    {
        FindClose(_dir->handle);
    }

    _dir->handle = FindFirstFileA(_dir->pattern,
                                  &_dir->find_data);

    // a search that cannot restart leaves nothing pending
    if (_dir->handle == INVALID_HANDLE_VALUE)
    {
        _dir->pending = 0;
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_dir_rewind",
                             NULL,
                             "could not restart the directory search",
                             -1);
    }

    _dir->pending = 1;
#else
    rewinddir(_dir->handle);
#endif

    return 0;
}

/*
d_dir_close
  The handle is released whatever the platform says about the close: it is
not usable either way, and leaking it would be the worse failure. FindClose
sets no errno, so its failure is recorded as EBADF.
*/
int
d_dir_close(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_close",
                            NULL,
                            "handle is NULL",
                            -1);

    int result = 0;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // a search that never started has nothing to close
    if (_dir->handle != INVALID_HANDLE_VALUE)
    {
        // FindClose sets no errno; name the failure
        if (!FindClose(_dir->handle))
        {
            D_INTERNAL_FILE_SET_ERR(EBADF);
            result = -1;
        }
    }
#else
    // a stream that never opened has nothing to close
    if (_dir->handle)
    {
        result = closedir(_dir->handle);
    }
#endif

    // release the handle whatever the platform said: it is not usable either
    // way, and leaking it would be the worse failure
    d_internal_file_free(_dir);

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_close",
                               NULL,
                               "closedir failed");

        return -1;
    }

    return 0;
}

/*
d_dir_get_current
  Requires a buffer. POSIX getcwd(NULL, 0) allocates instead, which is
convenient and is also a second ownership contract for one function -- so this
one does not offer it.
*/
char*
d_dir_get_current(
    char*  _buf,
    size_t _size
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_dir_get_current",
                            NULL,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_size > 0,
                            EINVAL,
                            "d_dir_get_current",
                            NULL,
                            "buffer size is 0",
                            NULL);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    char* const result = _getcwd(_buf,
                                 (int)_size);
#else
    char* const result = getcwd(_buf,
                                _size);
#endif

    // report the failure; errno is the platform's
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_get_current",
                               NULL,
                               "getcwd failed");

        return NULL;
    }

    return _buf;
}

/*
d_dir_set_current
  chdir, or the CRT's _chdir. Process-wide by nature, which is why the
declaration steers callers toward absolute paths instead.
*/
int
d_dir_set_current(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_set_current",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _chdir(_path);
#else
    const int result = chdir(_path);
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_set_current",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "chdir failed");

        return -1;
    }

    return 0;
}

/*
d_path_resolve
  realpath where the platform has it. Windows' _fullpath is lexical and does
NOT verify existence, so its answer is checked: reporting a canonical path for
something that is not there would make the two platforms disagree about what
this function means. With no resolver at all this refuses with ENOSYS rather
than return a lexical answer that silently differs from what the kernel would
do with symlinks. A result allocated here goes through the fs allocator, so
D_CFG_FILE_MALLOC still owns every byte this subframework hands out, and it is
released again on failure with the failure's errno kept.
*/
char*
d_path_resolve(
    const char* _path,
    char*       _resolved
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_path_resolve",
                            NULL,
                            "path is NULL",
                            NULL);

#if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 0)
    D_INTERNAL_FILE_REQUIRE(_resolved != NULL,
                            EINVAL,
                            "d_path_resolve",
                            _path,
                            "buffer is NULL and REALPATH_ALLOC is 0",
                            NULL);
#endif

#if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1)
    char* owned = NULL;

    // the allocation goes through the fs allocator, not the platform's, so
    // D_CFG_FILE_MALLOC still owns every byte this subframework hands out
    if (!_resolved)
    {
        owned = (char*)d_internal_file_alloc(D_FILE_PATH_MAX);

        // d_internal_file_alloc has already reported the failure
        if (!owned)
        {
            return NULL;
        }

        _resolved = owned;
    }
#endif

    char* result = NULL;

#if (D_INTERNAL_FILE_HAS_REALPATH == 1)
    result = realpath(_path,
                      _resolved);
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _fullpath(_resolved,
                       _path,
                       D_FILE_PATH_MAX);

    // _fullpath is lexical and does NOT verify existence, unlike realpath.
    // Reporting a canonical path for something that is not there would make
    // the two platforms disagree about what this function means.
    if (result)
    {
        // a resolved path must name something that exists
        if (!d_file_exists(result))
        {
            result = NULL;
            D_INTERNAL_FILE_SET_ERR(ENOENT);
        }
    }
#else
    // no resolver here: refuse rather than return a lexical answer that
    // silently differs from what the kernel would do with symlinks
    (void)_path;
    (void)_resolved;
    result = NULL;
    D_INTERNAL_FILE_SET_ERR(ENOSYS);
#endif

    // report the failure, releasing any buffer allocated here
    if (!result)
    {
#if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1)
        // free what was allocated here, keeping the failure's errno
        if (owned)
        {
            const int saved_errno = errno;

            d_internal_file_free(owned);
            errno = saved_errno;
        }
#endif

        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_path_resolve",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "realpath failed");

        return NULL;
    }

    return result;
}
