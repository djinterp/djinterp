/*******************************************************************************
* djinterp [c]                                                       file_lock.c
*
* Implementation of the advisory locks declared in file_lock.h.
*   Three backends, chosen at build time: flock where the build selects it and
* the platform has it, LockFileEx on Windows, and whole-file fcntl record locks
* everywhere else. djinterp's D_LOCK_* operations are translated to each
* backend's own constants rather than assumed to match them.
*
*
* path:      /src/djinterp/c/fs/file_lock.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_lock.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, EWOULDBLOCK, EAGAIN, EACCES
#include <stdio.h>   // FILE, fileno, SEEK_SET
#include <string.h>  // memset
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_lock.h"  // D_INTERNAL_FILE_LOCK_BACKEND


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if ( (D_INTERNAL_FILE_VALIDATE == 1) &&                                       \
      (!D_FILE_BACKEND_IS_STDC) )

/*
d_internal_lock_check_op
  Exactly one of SH / EX / UN must be present. Two is not a richer request but
a contradiction, and a platform handed the OR of two flags does something
arbitrary rather than complain. Defined only where D_INTERNAL_FILE_REQUIRE
expands to a check and d_file_lock_fd is not the ISO C stub -- the only
builds that call it; anywhere else it would be an unused static function.
*/
static int
d_internal_lock_check_op(
    int _operation
)
{
    const int mode  = _operation & (D_LOCK_SH | D_LOCK_EX | D_LOCK_UN);
    int       count = 0;

    // count the operations present
    if ((mode & D_LOCK_SH) != 0)
    {
        ++count;
    }

    if ((mode & D_LOCK_EX) != 0)
    {
        ++count;
    }

    if ((mode & D_LOCK_UN) != 0)
    {
        ++count;
    }

    return (count == 1);
}

#endif  // D_INTERNAL_FILE_VALIDATE && !D_FILE_BACKEND_IS_STDC

//==============================================================================
// 1.  LOCKING
//==============================================================================

#if D_FILE_BACKEND_IS_STDC

/*
d_file_lock_fd
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_lock_fd(
    int _fd,
    int _operation
)
{
    (void)_fd;
    (void)_operation;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_lock_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_lock_fd
  Every backend locks the whole file: fcntl is given l_len 0, which means "to
end of file, however it grows", and LockFileEx the maximum range. Win32
reports failure without an errno, so a refusal there is recorded as
EWOULDBLOCK, the one failure worth naming. A refused non-blocking request is
an answer rather than a malfunction, and is reported at info severity.
*/
int
d_file_lock_fd(
    int _fd,
    int _operation
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_lock_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(d_internal_lock_check_op(_operation),
                            EINVAL,
                            "d_file_lock_fd",
                            NULL,
                            "operation must be exactly one of SH / EX / UN",
                            -1);

    int result = -1;

#if ( (D_INTERNAL_FILE_LOCK_BACKEND == D_CFG_FILE_LOCK_BACKEND_FLOCK) &&       \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_FLOCK)) )
    {
        // djinterp's D_LOCK_* are deliberately not LOCK_*: Windows has no
        // such constants and Solaris numbers them differently, so they are
        // translated here rather than assumed to match
        int op = LOCK_UN;

        if ((_operation & D_LOCK_SH) != 0)
        {
            op = LOCK_SH;
        }
        else if ((_operation & D_LOCK_EX) != 0)
        {
            op = LOCK_EX;
        }

        // a non-blocking request fails rather than waits
        if ((_operation & D_LOCK_NB) != 0)
        {
            op |= LOCK_NB;
        }

        D_INTERNAL_FILE_RETRY_EINTR(result,
                                    flock(_fd,
                                          op));
    }
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    {
        const HANDLE handle = (HANDLE)_get_osfhandle(_fd);

        // a descriptor with no OS handle cannot be locked
        if (handle == INVALID_HANDLE_VALUE)
        {
            D_INTERNAL_FILE_FAIL(EBADF,
                                 "d_file_lock_fd",
                                 NULL,
                                 "descriptor has no OS handle",
                                 -1);
        }

        OVERLAPPED overlapped;

        memset(&overlapped,
               0,
               sizeof(overlapped));

        // release, or take the lock with the requested strength
        if ((_operation & D_LOCK_UN) != 0)
        {
            result = UnlockFileEx(handle,
                                  0,
                                  MAXDWORD,
                                  MAXDWORD,
                                  &overlapped) ? 0 : -1;
        }
        else
        {
            DWORD flags = 0;

            if ((_operation & D_LOCK_EX) != 0)
            {
                flags |= LOCKFILE_EXCLUSIVE_LOCK;
            }

            if ((_operation & D_LOCK_NB) != 0)
            {
                flags |= LOCKFILE_FAIL_IMMEDIATELY;
            }

            result = LockFileEx(handle,
                                flags,
                                0,
                                MAXDWORD,
                                MAXDWORD,
                                &overlapped) ? 0 : -1;
        }

        // Win32 sets no errno; name the one failure worth naming
        if (result != 0)
        {
            D_INTERNAL_FILE_SET_ERR(EWOULDBLOCK);
        }
    }
#else
    {
        struct flock fl;

        memset(&fl,
               0,
               sizeof(fl));
        fl.l_whence = SEEK_SET;
        fl.l_start  = 0;
        fl.l_len    = 0;   // 0 means "to end of file", however it grows

        // translate the operation into a record-lock type
        if ((_operation & D_LOCK_SH) != 0)
        {
            fl.l_type = F_RDLCK;
        }
        else if ((_operation & D_LOCK_EX) != 0)
        {
            fl.l_type = F_WRLCK;
        }
        else
        {
            fl.l_type = F_UNLCK;
        }

        const int cmd = ((_operation & D_LOCK_NB) != 0) ? F_SETLK : F_SETLKW;

        D_INTERNAL_FILE_RETRY_EINTR(result,
                                    fcntl(_fd,
                                          cmd,
                                          &fl));
    }
#endif

    // report the failure at the severity it deserves
    if (result != 0)
    {
        // a refused non-blocking lock is an ANSWER, not a malfunction; do not
        // report it at error severity
        if ( ((_operation & D_LOCK_NB) != 0) &&
             ( (errno == EWOULDBLOCK) ||
               (errno == EAGAIN)      ||
               (errno == EACCES) ) )
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                                   errno,
                                   "d_file_lock_fd",
                                   NULL,
                                   "lock is held elsewhere; non-blocking "
                                   "request declined");
        }
        else
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   errno,
                                   "d_file_lock_fd",
                                   NULL,
                                   "lock operation failed");
        }

        return -1;
    }

    return 0;
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_lock_stream
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_lock_stream(
    FILE* _stream,
    int   _operation
)
{
    (void)_stream;
    (void)_operation;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_lock_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_lock_stream
  Deliberately does NOT flush first, unlike d_file_truncate_stream: a lock is
about coordination, not about the bytes, and flushing here would make taking a
lock perform I/O the caller did not ask for.
*/
int
d_file_lock_stream(
    FILE* _stream,
    int   _operation
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_lock_stream",
                            NULL,
                            "stream is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int fd = _fileno(_stream);
#else
    const int fd = fileno(_stream);
#endif

    // a stream with no descriptor cannot be locked
    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_file_lock_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    return d_file_lock_fd(fd,
                          _operation);
}

#endif  // D_FILE_BACKEND_IS_STDC
