/*******************************************************************************
* djinterp [c]                                                       file_link.c
*
* Implementation of the symbolic-link calls declared in file_link.h.
*   symlink and readlink on POSIX. On Windows, CreateSymbolicLinkA with the
* file-or-directory kind decided up front, and readlink emulated by opening the
* link and asking for the final path it reaches. The whole file compiles to
* nothing when this build has no symbolic links.
*
*
* path:      /src/djinterp/c/fs/file_link.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_link.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL, ENOENT, EPERM, EIO
#include <stddef.h>  // size_t
#include <string.h>  // memcpy, strlen, strncmp
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/fs/file_stat.h"    // d_dir_exists, d_file_stat_nofollow
#include "../../../../inc/djinterp/config/c/fs/cfg_file_link.h"  // D_INTERNAL_FILE_LINK_UNPRIV


#if (D_INTERNAL_FILE_HAS_SYMLINKS == 1)

//==============================================================================
// 1.  SYMBOLIC LINKS
//==============================================================================

/*
d_file_symlink
  POSIX needs no stat: a link is text, and its kind is a read-time question.
Win32 bakes file-versus-directory into the link at creation and cannot revise
it, so the target is examined first, and a dangling target makes a file link.
The unprivileged-create flag is tried first when configured; Windows 10 before
1703 rejects the whole call for the unknown flag rather than ignoring it, so
the call is then retried without it. A failure after that is almost always a
missing privilege, and is reported as EPERM.
*/
int
d_file_symlink(
    const char* _target,
    const char* _linkpath
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_target != NULL,
                            EINVAL,
                            "d_file_symlink",
                            NULL,
                            "target is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_linkpath != NULL,
                            EINVAL,
                            "d_file_symlink",
                            _target,
                            "link path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    {
        // Win32 bakes the file/directory distinction into the link at
        // creation and cannot revise it, so it has to be decided now. POSIX
        // has no such notion and needs no stat.
        const DWORD flags = d_dir_exists(_target) ? SYMBOLIC_LINK_FLAG_DIRECTORY
                                                  : 0;

    #if (D_INTERNAL_FILE_LINK_UNPRIV == 1)
        // Developer Mode lets an ordinary account create links
        if (CreateSymbolicLinkA(_linkpath,
                                _target,
                                flags |
                                SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
        {
            return 0;
        }

        // Windows 10 before 1703 rejects the whole call for the unknown flag
        // rather than ignoring it; retry without, so an old host still works
        if (GetLastError() == ERROR_INVALID_PARAMETER)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                                   0,
                                   "d_file_symlink",
                                   NULL,
                                   "unprivileged-create flag rejected; "
                                   "retrying without it");

            // the plain call needs the privilege itself
            if (CreateSymbolicLinkA(_linkpath,
                                    _target,
                                    flags))
            {
                return 0;
            }
        }
    #else
        // the plain call needs the privilege itself
        if (CreateSymbolicLinkA(_linkpath,
                                _target,
                                flags))
        {
            return 0;
        }
    #endif

        // almost always a missing privilege rather than a bad argument
        D_INTERNAL_FILE_SET_ERR(EPERM);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               EPERM,
                               "d_file_symlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_linkpath),
                               "symlink failed; SeCreateSymbolicLinkPrivilege "
                               "or Developer Mode is required");

        return -1;
    }
#else
    // report the failure; errno is the platform's
    if (symlink(_target,
                _linkpath) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_symlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_linkpath),
                               "symlink failed");

        return -1;
    }

    return 0;
#endif
}

/*
d_file_readlink
  POSIX readlink(2) is called as is. Win32 has no equivalent, so the link is
opened with backup semantics and GetFinalPathNameByHandle asked for the path
it reaches, with the \\?\ prefix Win32 prepends stripped. A path that is not a
link is refused with EINVAL first, matching readlink(2), because Win32 would
happily open a plain file and report its own path.
*/
ssize_t
d_file_readlink(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_readlink",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_readlink",
                            _path,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_file_readlink",
                            _path,
                            "buffer size is 0",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    {
        // a non-symlink must be EINVAL, matching readlink(2); Win32 would
        // happily open the file itself and report its own path
        if (!d_file_is_symlink(_path))
        {
            D_INTERNAL_FILE_FAIL(EINVAL,
                                 "d_file_readlink",
                                 _path,
                                 "path is not a symbolic link",
                                 -1);
        }

        const HANDLE handle = CreateFileA(_path,
                                          0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS,
                                          NULL);

        // a link that cannot be opened cannot be read
        if (handle == INVALID_HANDLE_VALUE)
        {
            D_INTERNAL_FILE_FAIL(ENOENT,
                                 "d_file_readlink",
                                 _path,
                                 "could not open the link",
                                 -1);
        }

        char        full[D_FILE_PATH_MAX];
        const DWORD length = GetFinalPathNameByHandleA(handle,
                                                       full,
                                                       (DWORD)sizeof(full),
                                                       FILE_NAME_NORMALIZED);

        CloseHandle(handle);

        // zero is failure; a value past the buffer is the size it needed
        if ( (length == 0) ||
             (length >= (DWORD)sizeof(full)) )
        {
            D_INTERNAL_FILE_FAIL(EIO,
                                 "d_file_readlink",
                                 _path,
                                 "could not read the link target",
                                 -1);
        }

        // strip the \\?\ prefix Win32 prepends, which POSIX callers do not
        // expect and did not ask for
        const char* text = full;

        if (strncmp(text,
                    "\\\\?\\",
                    4) == 0)
        {
            text += 4;
        }

        size_t text_len = strlen(text);

        // truncate, per readlink's contract
        if (text_len > _bufsize)
        {
            text_len = _bufsize;
        }

        memcpy(_buf,
               text,
               text_len);

        return (ssize_t)text_len;
    }
#else
    {
        const ssize_t result = readlink(_path,
                                        _buf,
                                        _bufsize);

        // report the failure; errno is the platform's
        if (result < 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   errno,
                                   "d_file_readlink",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "readlink failed");

            return -1;
        }

        return result;
    }
#endif
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_file_is_symlink
  The CRT's stat family always follows reparse points, so lstat cannot answer
this on Windows; the reparse-point attribute is asked for directly.
*/
int
d_file_is_symlink(
    const char* _path
)
{
    // a NULL path is not a symlink; "no" is a meaningful answer to a yes/no
    if (!_path)
    {
        return 0;
    }

    const DWORD attrs = GetFileAttributesA(_path);

    // a path that cannot be examined is not reported as a link
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        return 0;
    }

    return ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
}

#else

/*
d_file_is_symlink
  d_file_stat_nofollow, necessarily: d_file_stat follows the link and describes
the target, so it could never report a link at all.
*/
int
d_file_is_symlink(
    const char* _path
)
{
    // a NULL path is not a symlink; "no" is a meaningful answer to a yes/no
    if (!_path)
    {
        return 0;
    }

    struct d_stat_t st;

    // a path that cannot be examined is not reported as a link
    if (d_file_stat_nofollow(_path,
                             &st) != 0)
    {
        return 0;
    }

    return (S_ISLNK(st.st_mode) != 0);
}

#endif  // D_CFG_FILE_HAS_WIN32

#endif  // D_INTERNAL_FILE_HAS_SYMLINKS
