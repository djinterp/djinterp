/******************************************************************************
* djinterp [c]                                                     file_sync.c
*
* path:      /src/djinterp/c/fs/file_sync.c
******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/fs/file_sync.h"


// I.    Kernel -> device

/*
d_fsync
  Forces a descriptor's data to durable storage.
  What "durable" means is a per-platform question this function answers and
most callers never think to ask. On Linux and Windows, fsync/_commit push the
data to the device and the device is expected to have it. On macOS, fsync()
explicitly does NOT ask the drive to flush its own write cache -- it returns,
the power fails, and the data is gone from the one call whose purpose was to
prevent exactly that. D_CFG_FILE_SYNC_FULL routes macOS to
fcntl(F_FULLFSYNC), which is what SQLite and every serious database there use,
and it is on by default.
  Check D_FILE_SYNC_REACHES_PLATTER at compile time to know which promise this
build makes.
  Note this syncs the FILE, not the directory entry. A newly created file can
survive while the name that reaches it does not; to persist the name, sync the
containing directory too.

Parameter(s):
  _fd: an open descriptor.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_fsync
(
    int _fd
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_fsync",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_fsync",
                            NULL,
                            "descriptor is negative",
                            -1);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _commit(_fd);
    #elif (D_INTERNAL_FILE_SYNC_FULL == 1)
    // macOS: the only call that actually reaches the platter
    result = fcntl(_fd, F_FULLFSYNC, 0);

    // F_FULLFSYNC is unsupported on some filesystems (and on most network
    // ones); fall back rather than fail a sync that plain fsync can service
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               errno,
                               "d_fsync",
                               NULL,
                               "F_FULLFSYNC unsupported here; fsync cannot "
                               "reach the drive cache");
        D_INTERNAL_FILE_RETRY_EINTR(result, fsync(_fd));
    }
    #elif (D_INTERNAL_FILE_SYNC_DATA_ONLY == 1)
    D_INTERNAL_FILE_RETRY_EINTR(result, fdatasync(_fd));
    #else
    D_INTERNAL_FILE_RETRY_EINTR(result, fsync(_fd));
    #endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fsync",
                               NULL,
                               "sync failed; the data is NOT durable");

        return -1;
    }

    return 0;
#endif
}


/*
d_fsync_stream
  Forces a stream's data all the way to durable storage.
  Two layers, in order, and both are required: fflush moves stdio's buffer
into the kernel, and only then can fsync move the kernel's copy onto the
device. Calling either alone is the classic way to believe you have durability
and not have it -- fflush leaves the data in the page cache, and fsync cannot
see what stdio has not handed over.

Parameter(s):
  _stream: an open stream.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_fsync_stream
(
    FILE* _stream
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_stream;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_fsync_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int fd;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_fsync_stream",
                            NULL,
                            "stream is NULL",
                            -1);

    // stdio first: fsync cannot flush what it cannot see
    if (fflush(_stream) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fsync_stream",
                               NULL,
                               "flush failed; syncing now would persist a "
                               "partial buffer");

        return -1;
    }

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    fd = _fileno(_stream);
    #else
    fd = fileno(_stream);
    #endif

    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_fsync_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    return d_fsync(fd);
#endif
}


// II.   Stdio -> kernel

/*
d_fflush
  Hands a stream's buffered bytes to the kernel.
  This is NOT a weaker fsync -- it is a different layer. After it returns the
data survives your process dying; it does not survive the machine losing
power, because it is sitting in the page cache. Use d_fsync_stream when you
mean durable.
  A NULL stream flushes every output stream, per ISO C, and is not an error.

Parameter(s):
  _stream: an open output stream, or NULL to flush all of them.
Return:
  0 on success, or EOF on failure with errno set.
*/
int
d_fflush
(
    FILE* _stream
)
{
    int result;

    // no validation: NULL is a documented, meaningful argument here rather
    // than a mistake, so there is nothing to reject
    result = fflush(_stream);

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fflush",
                               NULL,
                               "flush failed; buffered data was not accepted");
    }

    return result;
}
