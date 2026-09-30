/*******************************************************************************
* djinterp [c]                                                       file_sync.c
*
* Implementation of the durability calls declared in file_sync.h.
*   The descriptor-level sync resolves "durable" per platform -- _commit on
* Windows, F_FULLFSYNC on macOS where plain fsync stops short of the drive
* cache, fdatasync or fsync elsewhere -- and the stream-level calls layer
* stdio's flush on top of it. On the ISO C backend only the flush exists.
*
*
* path:      /src/djinterp/c/fs/file_sync.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_sync.h"  // corresponding header
// std
#include <errno.h>  // errno, EBADF, EINVAL, ENOSYS
#include <stdio.h>  // FILE, fflush, fileno
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_sync.h"  // D_INTERNAL_FILE_SYNC_*


//==============================================================================
// 1.  DURABILITY
//==============================================================================

#if D_FILE_BACKEND_IS_STDC

/*
d_file_sync_fd
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_sync_fd(
    int _fd
)
{
    (void)_fd;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_sync_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_sync_fd
  What "durable" means is a per-platform question answered here so callers
need not ask it. On Linux and Windows, fsync and _commit push the data to the
device. On macOS fsync explicitly does NOT ask the drive to flush its own
write cache -- it returns, the power fails, and the data is gone from the one
call whose purpose was to prevent that -- so D_CFG_FILE_SYNC_FULL routes there
to fcntl(F_FULLFSYNC), which is what SQLite and every serious database use.
A filesystem that refuses F_FULLFSYNC, as most network ones do, falls back to
fsync with a warning rather than failing a sync fsync can still service.
*/
int
d_file_sync_fd(
    int _fd
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_sync_fd",
                            NULL,
                            "descriptor is negative",
                            -1);

    int result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _commit(_fd);
#elif (D_INTERNAL_FILE_SYNC_FULL == 1)
    // macOS: the only call that actually reaches the platter
    result = fcntl(_fd,
                   F_FULLFSYNC,
                   0);

    // F_FULLFSYNC is unsupported on some filesystems (and on most network
    // ones); fall back rather than fail a sync that plain fsync can service
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               errno,
                               "d_file_sync_fd",
                               NULL,
                               "F_FULLFSYNC unsupported here; fsync cannot "
                               "reach the drive cache");
        D_INTERNAL_FILE_RETRY_EINTR(result,
                                    fsync(_fd));
    }
#elif (D_INTERNAL_FILE_SYNC_DATA_ONLY == 1)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fdatasync(_fd));
#else
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fsync(_fd));
#endif

    // a failed sync means the data is not durable, whatever else happened
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_sync_fd",
                               NULL,
                               "sync failed; the data is NOT durable");

        return -1;
    }

    return 0;
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_sync_stream
  The ISO C backend has no descriptors to sync, so this only reports ENOSYS.
*/
int
d_file_sync_stream(
    FILE* _stream
)
{
    (void)_stream;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_sync_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_sync_stream
  Two layers, in order, and both are required: fflush moves stdio's buffer
into the kernel, and only then can fsync move the kernel's copy onto the
device. Calling either alone is the classic way to believe you have durability
and not have it -- fflush leaves the data in the page cache, and fsync cannot
see what stdio has not handed over.
*/
int
d_file_sync_stream(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_sync_stream",
                            NULL,
                            "stream is NULL",
                            -1);

    // stdio first: fsync cannot flush what it cannot see
    if (fflush(_stream) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_sync_stream",
                               NULL,
                               "flush failed; syncing now would persist a "
                               "partial buffer");

        return -1;
    }

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int fd = _fileno(_stream);
#else
    const int fd = fileno(_stream);
#endif

    // a stream with no descriptor has nothing to sync
    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_file_sync_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    return d_file_sync_fd(fd);
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_flush_stream
  A NULL stream flushes every output stream, per ISO C, so there is nothing to
validate: NULL here is a meaningful argument, not a mistake.
*/
int
d_file_flush_stream(
    FILE* _stream
)
{
    const int result = fflush(_stream);

    // a failure means the kernel did not accept the buffered data
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_flush_stream",
                               NULL,
                               "flush failed; buffered data was not accepted");
    }

    return result;
}
