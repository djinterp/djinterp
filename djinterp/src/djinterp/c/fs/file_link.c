#include "../../../../inc/djinterp/c/fs/file_link.h"
#include "../../../../inc/djinterp/c/fs/file_stat.h"


#if (D_INTERNAL_FILE_HAS_SYMLINKS == 1)

///////////////////////////////////////////////////////////////////////////////
///             I.  SYMBOLIC LINKS                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
d_symlink
  Creates a symbolic link at _linkpath pointing at _target.
  The target is stored as text and is NOT resolved, checked, or required to
exist -- a dangling link is a legitimate object, and creating one is not an
error. Whether a relative target is interpreted against the link's directory
(POSIX) is likewise a read-time question, not a create-time one.
  On Windows this needs SeCreateSymbolicLinkPrivilege, which ordinary accounts
do not have. D_CFG_FILE_LINK_WIN32_UNPRIVILEGED asks for the Developer Mode
exemption, and this falls back without the flag on builds that reject it. When
neither works, expect EPERM -- from a call that compiled fine, because
D_FILE_LINK_IS_AVAILABLE is a claim about the platform and not about your
process.
  Windows also distinguishes file links from directory links at CREATION time
and cannot change its mind later, so the target is stat'd to decide. A
dangling target is therefore assumed to be a file.

Parameter(s):
  _target:   what the link should point at.
  _linkpath: where to create the link.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_symlink
(
    const char* _target,
    const char* _linkpath
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_target != NULL,
                            EINVAL,
                            "d_symlink",
                            NULL,
                            "target is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_linkpath != NULL,
                            EINVAL,
                            "d_symlink",
                            _target,
                            "link path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    {
        DWORD flags;

        // Win32 bakes the file/directory distinction into the link at
        // creation and cannot revise it, so it has to be decided now. POSIX
        // has no such notion and needs no stat.
        flags = d_is_dir(_target) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;

    #if (D_INTERNAL_FILE_LINK_UNPRIV == 1)
        if (CreateSymbolicLinkA(
                _linkpath,
                _target,
                flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
        {
            return 0;
        }

        // Windows 10 before 1703 rejects the whole call for the unknown flag
        // rather than ignoring it; retry without, so an old host still works
        if (GetLastError() == ERROR_INVALID_PARAMETER)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                                   0,
                                   "d_symlink",
                                   NULL,
                                   "unprivileged-create flag rejected; retrying without it");

            if (CreateSymbolicLinkA(_linkpath, _target, flags))
            {
                return 0;
            }
        }
    #else
        if (CreateSymbolicLinkA(_linkpath, _target, flags))
        {
            return 0;
        }
    #endif

        // almost always a missing privilege rather than a bad argument
        D_INTERNAL_FILE_SET_ERR(EPERM);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               EPERM,
                               "d_symlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_linkpath),
                               "symlink failed; SeCreateSymbolicLinkPrivilege or Developer Mode is required");

        return -1;
    }
#else
    if (symlink(_target, _linkpath) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_symlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_linkpath),
                               "symlink failed");

        return -1;
    }

    return 0;
#endif
}


/*
d_readlink
  Reads the text a symbolic link contains.
  The TEXT, not the resolution: a relative target comes back relative, and a
dangling one comes back intact. Use d_realpath (file_dir) to resolve.
  Follows readlink(2)'s contract exactly, including its sharp edge: the result
is NOT NUL-terminated, and a target that does not fit is TRUNCATED rather than
reported. Pass _bufsize - 1 and terminate at the returned length -- and if the
result equals _bufsize you cannot tell a fit from a truncation, so grow and
retry.

Parameter(s):
  _path:    the symbolic link to read.
  _buf:     buffer to receive the target text.
  _bufsize: size of _buf, in bytes.
Return:
  The number of bytes written (never NUL-terminated), or -1 on failure with
errno set. EINVAL when _path is not a symbolic link.
*/
ssize_t
d_readlink
(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_readlink",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_readlink",
                            _path,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_readlink",
                            _path,
                            "buffer size is 0",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    {
        HANDLE handle;
        DWORD  length;
        char   full[D_FILE_PATH_MAX];

        // a non-symlink must be EINVAL, matching readlink(2); Win32 would
        // happily open the file itself and report its own path
        if (!d_is_symlink(_path))
        {
            D_INTERNAL_FILE_FAIL(EINVAL,
                                 "d_readlink",
                                 _path,
                                 "path is not a symbolic link",
                                 -1);
        }

        handle = CreateFileA(_path,
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);

        if (handle == INVALID_HANDLE_VALUE)
        {
            D_INTERNAL_FILE_FAIL(ENOENT,
                                 "d_readlink",
                                 _path,
                                 "could not open the link",
                                 -1);
        }

        length = GetFinalPathNameByHandleA(handle,
                                           full,
                                           (DWORD)sizeof(full),
                                           FILE_NAME_NORMALIZED);
        CloseHandle(handle);

        if ( (length == 0) ||
             (length >= (DWORD)sizeof(full)) )
        {
            D_INTERNAL_FILE_FAIL(EIO,
                                 "d_readlink",
                                 _path,
                                 "could not read the link target",
                                 -1);
        }

        // strip the \\?\ prefix Win32 prepends, which POSIX callers do not
        // expect and did not ask for
        {
            const char* text;
            size_t      text_len;

            text = full;

            if (strncmp(text, "\\\\?\\", 4) == 0)
            {
                text += 4;
            }

            text_len = strlen(text);

            if (text_len > _bufsize)
            {
                text_len = _bufsize;   // truncate, per readlink's contract
            }

            memcpy(_buf, text, text_len);

            return (ssize_t)text_len;
        }
    }
#else
    {
        ssize_t result;

        result = readlink(_path, _buf, _bufsize);

        if (result < 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   errno,
                                   "d_readlink",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "readlink failed");

            return -1;
        }

        return result;
    }
#endif
}


/*
d_is_symlink
  Reports whether a path is a symbolic link.
  Uses d_lstat, necessarily: d_stat follows the link and would describe the
target, so it can never see a link at all.

Parameter(s):
  _path: path to test; may be NULL.
Return:
  Non-zero when the path is a symbolic link, 0 otherwise or when NULL.
*/
int
d_is_symlink
(
    const char* _path
)
{
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    DWORD attrs;

    if (!_path)
    {
        return 0;
    }

    // the CRT's stat family always follows reparse points, so lstat cannot
    // answer this on Windows -- ask the attribute directly
    attrs = GetFileAttributesA(_path);

    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        return 0;
    }

    return ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
#else
    struct d_stat_t st;

    // a NULL path is not a symlink; "no" is a meaningful answer to a yes/no
    if (!_path)
    {
        return 0;
    }

    // lstat, not stat: stat follows the link and would describe the target,
    // so it can never report a link
    if (d_lstat(_path, &st) != 0)
    {
        return 0;
    }

    return (S_ISLNK(st.st_mode) != 0);
#endif
}

#endif  // D_INTERNAL_FILE_HAS_SYMLINKS
