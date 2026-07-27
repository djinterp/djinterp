#include "../../../../inc/djinterp/c/fs/file_lock.h"


///////////////////////////////////////////////////////////////////////////////
///             INTERNAL DEFINITIONS                                        ///
///////////////////////////////////////////////////////////////////////////////

//   Not built on the ISO C backend: the only caller is in the non-STDC
// branch below, so defining it there is an unused function and a warning.
#if ( (D_INTERNAL_FILE_VALIDATE == 1) && !D_FILE_BACKEND_IS_STDC )
/*
d_internal_lock_check_op
  Validates a lock operation.
  Exactly one of SH / EX / UN must be present. Passing two is not a richer
request, it is a contradiction, and a platform handed the OR of two flags will
do something arbitrary rather than complain.

Parameter(s):
  _operation: the caller's operation.
Return:
  1 when the operation is well-formed, 0 otherwise.
*/
static int
d_internal_lock_check_op
(
    int _operation
)
{
    int mode;
    int count;

    mode  = _operation & (D_LOCK_SH | D_LOCK_EX | D_LOCK_UN);
    count = 0;

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
#endif // D_INTERNAL_FILE_VALIDATE && !D_FILE_BACKEND_IS_STDC


///////////////////////////////////////////////////////////////////////////////
///             I.  LOCKING                                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_flock
  Takes or releases an advisory lock on a descriptor.
  Advisory: a process that does not ask is not stopped. This coordinates
cooperating programs and nothing else.
  Blocks until the lock is available unless D_LOCK_NB is given, in which case
a conflict returns -1 with errno EWOULDBLOCK -- which is a normal outcome to
test for, not a failure to report.
  The lock's lifetime depends on the backend and the two are not
interchangeable: with flock it belongs to the open file description (survives
dup, shared with a forked child, released at the last close of that open);
with fcntl it belongs to the process and is dropped when ANY descriptor to the
file is closed, including one a library opened behind your back. Check
D_FILE_LOCK_IS_PER_DESCRIPTION.

Parameter(s):
  _fd:        an open descriptor.
  _operation: exactly one of D_LOCK_SH, D_LOCK_EX or D_LOCK_UN, optionally
              OR'd with D_LOCK_NB.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_flock
(
    int _fd,
    int _operation
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_operation;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_flock",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_flock",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(d_internal_lock_check_op(_operation),
                            EINVAL,
                            "d_flock",
                            NULL,
                            "operation must be exactly one of SH / EX / UN",
                            -1);

    #if ( (D_INTERNAL_FILE_LOCK_BACKEND == D_CFG_FILE_LOCK_BACKEND_FLOCK) &&  \
          D_CFG_IS_ON(D_CFG_FILE_HAS_FLOCK) )
    {
        int op;

        // djinterp's D_LOCK_* are deliberately not LOCK_*: Windows has no
        // such constants and Solaris numbers them differently, so they are
        // translated here rather than assumed to match
        if ((_operation & D_LOCK_SH) != 0)
        {
            op = LOCK_SH;
        }
        else if ((_operation & D_LOCK_EX) != 0)
        {
            op = LOCK_EX;
        }
        else
        {
            op = LOCK_UN;
        }

        if ((_operation & D_LOCK_NB) != 0)
        {
            op |= LOCK_NB;
        }

        D_INTERNAL_FILE_RETRY_EINTR(result, flock(_fd, op));
    }
    #elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    {
        HANDLE   handle;
        OVERLAPPED overlapped;
        DWORD    flags;

        handle = (HANDLE)_get_osfhandle(_fd);

        if (handle == INVALID_HANDLE_VALUE)
        {
            D_INTERNAL_FILE_FAIL(EBADF,
                                 "d_flock",
                                 NULL,
                                 "descriptor has no OS handle",
                                 -1);
        }

        memset(&overlapped, 0, sizeof(overlapped));

        if ((_operation & D_LOCK_UN) != 0)
        {
            result = UnlockFileEx(handle, 0, MAXDWORD, MAXDWORD, &overlapped) ?
                     0 : -1;
        }
        else
        {
            flags = 0;

            if ((_operation & D_LOCK_EX) != 0)
            {
                flags |= LOCKFILE_EXCLUSIVE_LOCK;
            }

            if ((_operation & D_LOCK_NB) != 0)
            {
                flags |= LOCKFILE_FAIL_IMMEDIATELY;
            }

            result = LockFileEx(handle, flags, 0, MAXDWORD, MAXDWORD,
                                &overlapped) ? 0 : -1;
        }

        if (result != 0)
        {
            D_INTERNAL_FILE_SET_ERR(EWOULDBLOCK);
        }
    }
    #else
    {
        struct flock fl;
        int          cmd;

        memset(&fl, 0, sizeof(fl));
        fl.l_whence = SEEK_SET;
        fl.l_start  = 0;
        fl.l_len    = 0;   // 0 means "to end of file", however it grows

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

        cmd = ((_operation & D_LOCK_NB) != 0) ? F_SETLK : F_SETLKW;

        D_INTERNAL_FILE_RETRY_EINTR(result, fcntl(_fd, cmd, &fl));
    }
    #endif

    if (result != 0)
    {
        // a refused non-blocking lock is an ANSWER, not a malfunction; do not
        // report it at error severity
        if ( ((_operation & D_LOCK_NB) != 0) &&
             ( (errno == EWOULDBLOCK) ||
               (errno == EAGAIN) ||
               (errno == EACCES) ) )
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                                   errno,
                                   "d_flock",
                                   NULL,
                                   "lock is held elsewhere; non-blocking request declined");
        }
        else
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   errno,
                                   "d_flock",
                                   NULL,
                                   "lock operation failed");
        }

        return -1;
    }

    return 0;
#endif
}


/*
d_flock_stream
  Takes or releases an advisory lock through the stream that owns the file.
  Deliberately does NOT flush first, unlike d_ftruncate_stream: a lock is
about coordination, not about the bytes, and flushing here would make taking a
lock perform I/O the caller did not ask for. Flush yourself before releasing a
lock that guards data you have written.

Parameter(s):
  _stream:    an open stream.
  _operation: as d_flock.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_flock_stream
(
    FILE* _stream,
    int   _operation
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_stream;
    (void)_operation;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_flock_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int fd;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_flock_stream",
                            NULL,
                            "stream is NULL",
                            -1);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    fd = _fileno(_stream);
    #else
    fd = fileno(_stream);
    #endif

    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_flock_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    return d_flock(fd, _operation);
#endif
}
