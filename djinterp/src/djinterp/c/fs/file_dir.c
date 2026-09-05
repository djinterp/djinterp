/******************************************************************************
* djinterp [c]                                                      file_dir.c
*
* path:      /src/djinterp/c/fs/file_dir.c
******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/fs/file_dir.h"
#include "../../../../inc/djinterp/c/fs/file_stat.h"


// Internal definitions

/*
d_dir_t
  The opaque directory handle promised by file_common.h.
  It owns the d_dirent_t that d_readdir hands back, which is what makes that
pointer's lifetime a property of the HANDLE rather than of the platform.
POSIX readdir returns a pointer into the DIR's own storage and says almost
nothing about how long it lasts; copying into a member here gives one answer
on every target: valid until the next d_readdir on this handle, dead at
d_closedir.
*/
struct d_dir_t
{
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    HANDLE            handle;       // FindFirstFile / FindNextFile handle
    WIN32_FIND_DATAA  find_data;    // the entry Win32 hands back
    int               pending;      // 1 when find_data holds an unread entry
    char              pattern[D_FILE_PATH_MAX];  // for rewind
#else
    DIR*              handle;
#endif
    struct d_dirent_t entry;        // what d_readdir returns
    char              path[D_FILE_PATH_MAX];     // for the stat fallback
};


#if (D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1)

/*
d_internal_dir_type_by_stat
  Determines an entry's type by asking the filesystem.
  The expensive path, taken only when the kernel would not say. It is one stat
per entry, which on a network filesystem is one round trip per entry -- see
D_CFG_FILE_DIR_FILL_TYPE.
  Uses lstat, not stat: a dangling symlink must report DT_LNK, not vanish.

Parameter(s):
  _dir:  the handle, for its directory path.
  _name: the entry's filename.
Return:
  A DT_* constant; DT_UNKNOWN when the entry cannot be described.
*/
static uint8_t
d_internal_dir_type_by_stat
(
    struct d_dir_t* _dir,
    const char*     _name
)
{
    char            full[D_FILE_PATH_MAX];
    struct d_stat_t st;

    if (!d_path_join(full, sizeof(full), _dir->path, _name))
    {
        return DT_UNKNOWN;
    }

    // lstat: describe the link itself, so a dangling one is still DT_LNK
    // rather than an entry that appears not to exist
    if (d_lstat(full, &st) != 0)
    {
        return DT_UNKNOWN;
    }

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

#endif  // D_INTERNAL_FILE_DIR_TYPE_BY_STAT


/*
d_internal_dir_copy_name
  Copies an entry name into the handle's buffer, bounded.
  Local rather than d_strcpy_s from the string module: this is five lines, and
the alternative is that every consumer of file_dir links dstring for one call.
The fs subframework's only dependencies should be the ones it genuinely needs.

Parameter(s):
  _dst:     destination buffer.
  _dstsize: size of _dst, in bytes.
  _src:     NUL-terminated source name.
Return:
  0 on success, or -1 when the name does not fit.
*/
static int
d_internal_dir_copy_name
(
    char*       _dst,
    size_t      _dstsize,
    const char* _src
)
{
    size_t length;

    length = strlen(_src);

    if ((length + 1) > _dstsize)
    {
        return -1;
    }

    memcpy(_dst, _src, length + 1);

    return 0;
}


/*
d_internal_dir_is_dots
  Reports whether a name is "." or "..".

Parameter(s):
  _name: the entry name.
Return:
  1 for "." or "..", 0 otherwise.
*/
static int
d_internal_dir_is_dots
(
    const char* _name
)
{
    if (_name[0] != '.')
    {
        return 0;
    }

    if (_name[1] == '\0')
    {
        return 1;
    }

    if ( (_name[1] == '.') &&
         (_name[2] == '\0') )
    {
        return 1;
    }

    return 0;
}


// I.    Creation

/*
d_mkdir
  Creates a single directory.
  Every missing parent is a failure (ENOENT), not something to create -- that
is d_mkdir_p. An existing target is EEXIST.

Parameter(s):
  _path: path to create.
  _mode: permission bits. Ignored on Windows, which has no such concept for
         directories; the process umask still applies on POSIX.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_mkdir
(
    const char* _path,
    uint32_t    _mode
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_mkdir",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    (void)_mode;
    result = _mkdir(_path);
#else
    result = mkdir(_path, (mode_t)_mode);
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_mkdir",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "mkdir failed");

        return -1;
    }

    return 0;
}


/*
d_mkdir_p
  Creates a directory and any missing parents (mkdir -p).
  The request is "ensure this path exists", so a path that already exists has
satisfied it and this returns 0 (see D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK).
  Intermediate directories get D_CFG_FILE_DIR_CREATE_MODE, not _mode. Passing
0700 for "/a/b/c" means "c should be private" far more often than it means
"and make /a and /a/b private too".
  An EEXIST from an intermediate is ignored on purpose: another process
creating the same tree concurrently is the normal case, not a race worth
failing on.

Parameter(s):
  _path: path to create.
  _mode: permission bits for the FINAL component.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_mkdir_p
(
    const char* _path,
    uint32_t    _mode
)
{
    char   work[D_FILE_PATH_MAX];
    size_t root_len;
    size_t idx;
    size_t length;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_mkdir_p",
                            NULL,
                            "path is NULL",
                            -1);

    length = strlen(_path);

    if ((length + 1) > sizeof(work))
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_mkdir_p",
                             _path,
                             "path is longer than D_FILE_PATH_MAX",
                             -1);
    }

    if (length == 0)
    {
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_mkdir_p",
                             _path,
                             "path is empty",
                             -1);
    }

    memcpy(work, _path, length + 1);

    // never try to create the root itself: "/" and "C:\" already exist by
    // definition, and mkdir("/") returns EEXIST or EACCES depending on the
    // platform's mood
    root_len = d_path_root_length(work);
    idx      = (root_len > 0) ? root_len : 0;

    // walk the components, creating each prefix in turn
    for (; idx <= length; ++idx)
    {
        if ( (idx == length) ||
             (work[idx] == '/') ||
             (work[idx] == D_FILE_PATH_SEP) )
        {
            char saved;

            // an empty component ("a//b") is not a directory to create
            if (idx == 0)
            {
                continue;
            }

            saved     = work[idx];
            work[idx] = '\0';

            if (work[0] != '\0')
            {
                uint32_t mode;

                // the caller's mode is for the leaf; parents get the
                // configured one
                mode = (idx == length) ?
                       _mode :
                       (uint32_t)D_INTERNAL_FILE_DIR_CREATE_MODE;

                if (d_mkdir(work, mode) != 0)
                {
                    // already there is the normal case, not a race: another
                    // process building the same tree is expected
                    if (errno != EEXIST)
                    {
                        work[idx] = saved;
                        D_INTERNAL_FILE_NOTIFY(
                            D_FILE_NOTIFY_ERROR,
                            errno,
                            "d_mkdir_p",
                            D_INTERNAL_FILE_NOTIFY_PATH(_path),
                            "could not create an intermediate directory");

                        return -1;
                    }

                    // EEXIST on a NON-directory is a real failure: the path
                    // cannot be brought into existence as a directory
                    if (!d_is_dir(work))
                    {
                        work[idx] = saved;
                        D_INTERNAL_FILE_FAIL(ENOTDIR,
                                             "d_mkdir_p",
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
                                             "d_mkdir_p",
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


// II.   Removal

/*
d_rmdir
  Removes an empty directory.
  Empty specifically: a non-empty one fails with ENOTEMPTY (or EEXIST on some
platforms). There is deliberately no recursive form here -- deleting a tree is
a policy decision with a symlink-traversal hazard attached, and it does not
belong behind a name this innocent.

Parameter(s):
  _path: directory to remove.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_rmdir
(
    const char* _path
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_rmdir",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _rmdir(_path);
#else
    result = rmdir(_path);
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_rmdir",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "rmdir failed");

        return -1;
    }

    return 0;
}


// III.  Traversal

/*
d_opendir
  Opens a directory for reading.

Parameter(s):
  _path: directory to open.
Return:
  A handle on success, or NULL on failure with errno set. Release it with
d_closedir.
*/
struct d_dir_t*
d_opendir
(
    const char* _path
)
{
    struct d_dir_t* dir;
    size_t          length;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_opendir",
                            NULL,
                            "path is NULL",
                            NULL);

    length = strlen(_path);

    if ((length + 1) > D_FILE_PATH_MAX)
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_opendir",
                             _path,
                             "path is longer than D_FILE_PATH_MAX",
                             NULL);
    }

    dir = (struct d_dir_t*)d_internal_file_alloc(sizeof(struct d_dir_t));

    if (!dir)
    {
        return NULL;
    }

    memset(dir, 0, sizeof(*dir));
    memcpy(dir->path, _path, length + 1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Win32 has no opendir: FindFirstFile both opens AND reads the first
    // entry. So the entry has to be stashed here and handed out by the first
    // d_readdir, or it is silently skipped -- which is the classic bug in
    // every hand-rolled Win32 dirent shim.
    if (!d_path_join(dir->pattern, sizeof(dir->pattern), _path, "*"))
    {
        d_internal_file_free(dir);

        return NULL;
    }

    dir->handle = FindFirstFileA(dir->pattern, &dir->find_data);

    if (dir->handle == INVALID_HANDLE_VALUE)
    {
        d_internal_file_free(dir);
        D_INTERNAL_FILE_SET_ERR(ENOENT);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ENOENT,
                               "d_opendir",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "opendir failed");

        return NULL;
    }

    dir->pending = 1;
#else
    dir->handle = opendir(_path);

    if (!dir->handle)
    {
        int saved_errno;

        saved_errno = errno;
        d_internal_file_free(dir);
        errno = saved_errno;
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               saved_errno,
                               "d_opendir",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "opendir failed");

        return NULL;
    }
#endif

    return dir;
}


/*
d_readdir
  Reads the next entry from a directory.
  The returned pointer belongs to _dir: it stays valid until the next
d_readdir on the same handle and dies with d_closedir. Copy anything you need
to keep. (POSIX readdir is vaguer than this; owning the storage here is what
makes the guarantee sayable at all.)
  Entries arrive in whatever order the filesystem stores them -- not sorted,
not creation order, and not stable between runs.

Parameter(s):
  _dir: an open handle.
Return:
  The next entry, or NULL at end of directory. NULL is also returned on error,
which is distinguishable only by errno: set it to 0 before calling if you need
to tell them apart.
*/
struct d_dirent_t*
d_readdir
(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_readdir",
                            NULL,
                            "handle is NULL",
                            NULL);

    for (;;)
    {
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        // hand out the entry FindFirstFile already produced before asking for
        // another one
        if (!_dir->pending)
        {
            if (!FindNextFileA(_dir->handle, &_dir->find_data))
            {
                // ERROR_NO_MORE_FILES is the end, not a failure
                return NULL;
            }
        }

        _dir->pending = 0;

        if (d_internal_dir_copy_name(_dir->entry.d_name,
                                     sizeof(_dir->entry.d_name),
                                     _dir->find_data.cFileName) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   ENAMETOOLONG,
                                   "d_readdir",
                                   NULL,
                                   "entry name exceeds D_FILE_NAME_MAX; "
                                   "skipped");
            continue;
        }

        _dir->entry.d_ino = 0;   // Win32 has no inode to report

    #if D_CFG_IS_ON(D_CFG_FILE_DIR_FILL_TYPE)
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
        struct dirent* native;

        // POSIX: readdir returns NULL for BOTH end-of-directory and error,
        // and errno is the only way to tell. Clear it so a stale value from
        // some earlier call cannot be mistaken for this one's failure.
        errno  = 0;
        native = readdir(_dir->handle);

        if (!native)
        {
            if (errno != 0)
            {
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                       errno,
                                       "d_readdir",
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
                                   "d_readdir",
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
d_rewinddir
  Returns a directory handle to its first entry.
  Reports failure, unlike POSIX rewinddir, which returns void. That is fine on
POSIX where the call cannot fail -- but Win32 has no rewind at all, so this
closes and re-opens the search, and THAT can fail. A void return would hide it
and leave the caller iterating a dead handle.

Parameter(s):
  _dir: an open handle.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_rewinddir
(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_rewinddir",
                            NULL,
                            "handle is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // no Win32 rewind: close the search and start it again
    if (_dir->handle != INVALID_HANDLE_VALUE)
    {
        FindClose(_dir->handle);
    }

    _dir->handle = FindFirstFileA(_dir->pattern, &_dir->find_data);

    if (_dir->handle == INVALID_HANDLE_VALUE)
    {
        _dir->pending = 0;
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_rewinddir",
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
d_closedir
  Closes a directory handle and releases it.
  The entry last returned by d_readdir is invalidated here.

Parameter(s):
  _dir: the handle to close.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_closedir
(
    struct d_dir_t* _dir
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_closedir",
                            NULL,
                            "handle is NULL",
                            -1);

    result = 0;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    if (_dir->handle != INVALID_HANDLE_VALUE)
    {
        if (!FindClose(_dir->handle))
        {
            D_INTERNAL_FILE_SET_ERR(EBADF);
            result = -1;
        }
    }
#else
    if (_dir->handle)
    {
        result = closedir(_dir->handle);
    }
#endif

    // release the handle whatever the platform said: it is not usable either
    // way, and leaking it would be the worse failure
    d_internal_file_free(_dir);

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_closedir",
                               NULL,
                               "closedir failed");

        return -1;
    }

    return 0;
}


// IV.   Working directory & resolution

/*
d_getcwd
  Retrieves the current working directory.
  Requires a buffer. POSIX getcwd(NULL, 0) allocates instead, which is
convenient and is also a second ownership contract for one function -- so this
one does not offer it. A NULL buffer is EINVAL.
  Worth remembering that the process has ONE working directory shared by every
thread, so this answer can be stale before it returns if another thread calls
d_chdir.

Parameter(s):
  _buf:  buffer to receive the path.
  _size: size of _buf, in bytes.
Return:
  _buf on success, or NULL on failure with errno set (ERANGE when the path
does not fit).
*/
char*
d_getcwd
(
    char*  _buf,
    size_t _size
)
{
    char* result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_getcwd",
                            NULL,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_size > 0,
                            EINVAL,
                            "d_getcwd",
                            NULL,
                            "buffer size is 0",
                            NULL);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _getcwd(_buf, (int)_size);
#else
    result = getcwd(_buf, _size);
#endif

    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_getcwd",
                               NULL,
                               "getcwd failed");

        return NULL;
    }

    return _buf;
}


/*
d_chdir
  Changes the current working directory.
  Process-wide and shared by every thread, which makes it a poor way to scope
work: another thread's relative path resolves against whatever this last set.
Prefer absolute paths, or the *at() family, where you can.

Parameter(s):
  _path: directory to move to.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_chdir
(
    const char* _path
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_chdir",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _chdir(_path);
#else
    result = chdir(_path);
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_chdir",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "chdir failed");

        return -1;
    }

    return 0;
}


/*
d_realpath
  Resolves a path to its canonical absolute form, following every symbolic
link on the way.
  This is the counterpart to d_path_normalize, and the difference matters.
d_path_normalize is lexical: it says "/x/link/.." is "/x" because that is what
the TEXT means. This asks the filesystem, so given /x/link -> /y/z it says
"/y". Both are defensible; only this one describes what the kernel will
actually do.
  The price is that every component must EXIST. A path you are about to create
cannot be resolved here -- use d_path_normalize for that.

Parameter(s):
  _path:     path to resolve.
  _resolved: buffer of at least D_FILE_PATH_MAX bytes, or NULL to have the
             result allocated (requires D_CFG_FILE_DIR_REALPATH_ALLOC; release
             it with free, or the configured deallocator).
Return:
  The resolved path on success, or NULL on failure with errno set.
*/
char*
d_realpath
(
    const char* _path,
    char*       _resolved
)
{
    char* result;
#if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1)
    char* owned;
#endif

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_realpath",
                            NULL,
                            "path is NULL",
                            NULL);

#if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 0)
    D_INTERNAL_FILE_REQUIRE(_resolved != NULL,
                            EINVAL,
                            "d_realpath",
                            _path,
                            "buffer is NULL and REALPATH_ALLOC is 0",
                            NULL);
#endif

#if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1)
    owned = NULL;

    // the allocation goes through the fs allocator, not the platform's, so
    // D_CFG_FILE_MALLOC still owns every byte this subframework hands out
    if (!_resolved)
    {
        owned = (char*)d_internal_file_alloc(D_FILE_PATH_MAX);

        if (!owned)
        {
            return NULL;
        }

        _resolved = owned;
    }
#endif

#if (D_INTERNAL_FILE_HAS_REALPATH == 1)
    result = realpath(_path, _resolved);
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _fullpath(_resolved, _path, D_FILE_PATH_MAX);

    // _fullpath is lexical and does NOT verify existence, unlike realpath.
    // Reporting a canonical path for something that is not there would make
    // the two platforms disagree about what this function means.
    if (result)
    {
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

    if (!result)
    {
#if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1)
        if (owned)
        {
            int saved_errno;

            saved_errno = errno;
            d_internal_file_free(owned);
            errno = saved_errno;
        }
#endif
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_realpath",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "realpath failed");

        return NULL;
    }

    return result;
}
