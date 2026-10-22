/*******************************************************************************
* djinterp [c]                                                       file_lock.c
*
* Implementation of the advisory locks declared in file_lock.h.
*   Three backends, chosen at build time, one helper each
* (d_internal_lock_raw): flock where the build selects it and the platform has
* it, LockFileEx on Windows -- made to behave as flock does where Windows
* differs -- and whole-file fcntl record locks everywhere else. djinterp's
* D_LOCK_* operations are translated to each backend's own constants rather
* than assumed to match them.
*
*
* path:      /src/djinterp/c/filesys/file_lock.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_lock.h"  // corresponding header
// std
#include <errno.h>  // errno, EBADF, EINVAL, EWOULDBLOCK, EAGAIN, EACCES
#include <stdio.h>  // FILE, SEEK_SET
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_desc.h"    // d_file_descriptor_stream
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_lock.h"  // D_INTERNAL_FILE_LOCK_BACKEND


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

#if !D_FILE_BACKEND_IS_STDC

#if (D_INTERNAL_FILE_LOCK_BACKEND == D_INTERNAL_FILE_LOCK_BACKEND_WIN32)

/*
d_internal_lock_errno
  A Win32 reason, as the errno flock would have set: a range another handle
holds is EWOULDBLOCK, the answer a non-blocking request expects, where every
other call reads it as EACCES; a filesystem that cannot lock is ENOLCK, where
the table says ENOTSUP; anything else -- a handle that is not one, EBADF,
among them -- reads as it does everywhere in c/filesys.
*/
static int
d_internal_lock_errno(
    DWORD _error
)
{
    // a filesystem that cannot lock is ENOLCK, flock's word for it, not the
    // table's general ENOTSUP
    if ( (_error == ERROR_INVALID_FUNCTION) ||
         (_error == ERROR_NOT_SUPPORTED) )
    {
        return ENOLCK;
    }

    // a range held elsewhere; the rest as the table has them
    return (_error == ERROR_LOCK_VIOLATION)
               ? EWOULDBLOCK
               : d_internal_file_errno_win32(_error);
}

/*
d_internal_lock_refuse
  Fails a lock call with GetLastError's reason (d_internal_lock_errno).
*/
static int
d_internal_lock_refuse(
    void
)
{
    const int code = d_internal_lock_errno(GetLastError());

    // named only by errno, which a build may not report through
    D_INTERNAL_FILE_SET_ERR(code);
    (void)code;

    return -1;
}

/*
d_internal_lock_release
  Releases the whole-file range the handle holds. Holding none is not an
error: POSIX unlocks an unlocked file without complaint, and ERROR_NOT_LOCKED
-- ERROR_LOCK_VIOLATION under Wine, for the same case -- says only that there
was nothing to do.
*/
static int
d_internal_lock_release(
    HANDLE _handle
)
{
    OVERLAPPED  overlapped = {0};
    const BOOL  released   = UnlockFileEx(_handle,
                                          0,
                                          MAXDWORD,
                                          MAXDWORD,
                                          &overlapped);
    const DWORD error      = released ? (DWORD)ERROR_SUCCESS
                                      : GetLastError();

    // released, or there was nothing to release
    if ( (released) ||
         (error == ERROR_NOT_LOCKED) ||
         (error == ERROR_LOCK_VIOLATION) )
    {
        return 0;
    }

    SetLastError(error);

    return d_internal_lock_refuse();
}

/*
d_internal_lock_raw
  LockFileEx over the whole file -- offset 0, the largest length -- with
flock's rule that a request replaces the lock the descriptor holds. Windows
ranges neither merge nor convert: a second lock from the same handle stacks,
needing two unlocks, or, exclusive over the handle's own, waits for itself
forever. So the held lock is released first, which is how flock documents a
conversion: not atomic, and a non-blocking request that loses the race in
between ends holding nothing.
*/
static int
d_internal_lock_raw(
    int _fd,
    int _operation
)
{
    const HANDLE handle     = (HANDLE)_get_osfhandle(_fd);
    OVERLAPPED   overlapped = {0};
    const DWORD  strength   = ((_operation & D_LOCK_EX) != 0)
                                  ? (DWORD)LOCKFILE_EXCLUSIVE_LOCK
                                  : (DWORD)0;
    const DWORD  waiting    = ((_operation & D_LOCK_NB) != 0)
                                  ? (DWORD)LOCKFILE_FAIL_IMMEDIATELY
                                  : (DWORD)0;

    // not a descriptor the CRT knows, which _get_osfhandle has called EBADF
    if (handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    // whatever the descriptor holds goes first
    if (d_internal_lock_release(handle) != 0)
    {
        return -1;
    }

    // released, which is all an unlock asks
    if ((_operation & D_LOCK_UN) != 0)
    {
        return 0;
    }

    // taken; or held by another handle, or refused for a reason of Windows'
    if (LockFileEx(handle,
                   strength | waiting,
                   0,
                   MAXDWORD,
                   MAXDWORD,
                   &overlapped))
    {
        return 0;
    }

    return d_internal_lock_refuse();
}

#elif (D_INTERNAL_FILE_LOCK_BACKEND == D_CFG_FILE_LOCK_BACKEND_FLOCK)

/*
d_internal_lock_raw
  flock, retried across interruptions. djinterp's D_LOCK_* are deliberately not
LOCK_*: Windows has no such constants and Solaris numbers them differently, so
they are translated here rather than assumed to match.
*/
static int
d_internal_lock_raw(
    int _fd,
    int _operation
)
{
    int op     = LOCK_UN;
    int result = -1;

    // the strength asked for; neither is an unlock
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

    return result;
}

#else

/*
d_internal_lock_raw
  A whole-file fcntl record lock, retried across interruptions: l_len 0 means
"to end of file, however it grows".
*/
static int
d_internal_lock_raw(
    int _fd,
    int _operation
)
{
    struct flock fl     = {0};
    int          result = -1;
    const int    cmd    = ((_operation & D_LOCK_NB) != 0) ? F_SETLK
                                                          : F_SETLKW;

    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    fl.l_type   = F_UNLCK;

    // translate the operation into a record-lock type
    if ((_operation & D_LOCK_SH) != 0)
    {
        fl.l_type = F_RDLCK;
    }
    else if ((_operation & D_LOCK_EX) != 0)
    {
        fl.l_type = F_WRLCK;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fcntl(_fd,
                                      cmd,
                                      &fl));

    return result;
}

#endif  // D_INTERNAL_FILE_LOCK_BACKEND

#if (EWOULDBLOCK == EAGAIN)

/*
d_internal_lock_held
  Whether an errno says the lock is held elsewhere: EWOULDBLOCK -- here one
number with EAGAIN -- or EACCES, which fcntl may use instead.
*/
static int
d_internal_lock_held(
    int _code
)
{
    return ( (_code == EAGAIN) ||
             (_code == EACCES) );
}

#else

/*
d_internal_lock_held
  Whether an errno says the lock is held elsewhere: EWOULDBLOCK, or EAGAIN or
EACCES, which fcntl may use instead.
*/
static int
d_internal_lock_held(
    int _code
)
{
    return ( (_code == EWOULDBLOCK) ||
             (_code == EAGAIN)      ||
             (_code == EACCES) );
}

#endif  // EWOULDBLOCK == EAGAIN

/*
d_internal_lock_report
  Reports a failed lock call at the severity it deserves: a refused
non-blocking request is an ANSWER, not a malfunction, and is reported at info
severity.
*/
static int
d_internal_lock_report(
    int _operation
)
{
    // a conflict under D_LOCK_NB is the answer the caller asked for
    if ( ((_operation & D_LOCK_NB) != 0) &&
         (d_internal_lock_held(errno)) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                               errno,
                               "d_file_lock_fd",
                               NULL,
                               "lock is held elsewhere; non-blocking "
                               "request declined");

        return -1;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                           errno,
                           "d_file_lock_fd",
                           NULL,
                           "lock operation failed");

    return -1;
}

#endif  // !D_FILE_BACKEND_IS_STDC

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
d_file_lock_fd
  Every backend locks the whole file (d_internal_lock_raw), and a request
replaces the lock the descriptor holds -- which flock and fcntl do natively
and Windows is made to.
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

    // done, or reported at the severity the failure deserves
    if (d_internal_lock_raw(_fd,
                            _operation) != 0)
    {
        return d_internal_lock_report(_operation);
    }

    return 0;
}

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

    const int fd = d_file_descriptor_stream(_stream);

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
