/*******************************************************************************
* djinterp [c]                                                       file_sync.c
*
* Implementation of the durability calls declared in file_sync.h.
*   What "durable" means is answered once per platform, in
* d_internal_file_sync_raw -- FlushFileBuffers on Windows, F_FULLFSYNC on
* Apple's systems, where plain fsync stops short of the drive cache, fdatasync
* or fsync elsewhere -- for d_file_sync_fd and file_io's durable writes alike,
* and the stream-level call layers stdio's flush on top of it. On the ISO C
* backend only the flush exists.
*
*
* path:      /src/djinterp/c/filesys/file_sync.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_sync.h"  // corresponding header
// std
#include <errno.h>  // errno, EBADF, EDQUOT, EINVAL, EIO, ENOSPC, ENOSYS
#include <stdio.h>  // FILE, fflush
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_desc.h"    // d_file_descriptor_stream
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_sync.h"  // D_INTERNAL_FILE_SYNC_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if ( (!D_FILE_BACKEND_IS_STDC) &&                                             \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_WIN32)) &&                                  \
      (D_INTERNAL_FILE_SYNC_FULL == 1) )

// D_INTERNAL_SYNC_F_FULLFSYNC
//   constant: fcntl's full-sync command. Darwin's <fcntl.h> hides the name
// from a build at a POSIX level without _DARWIN_C_SOURCE, but the kernel
// takes the command whatever the build was shown, and 51 is its number in
// Darwin's ABI.
#ifdef F_FULLFSYNC
    #define D_INTERNAL_SYNC_F_FULLFSYNC F_FULLFSYNC
#else
    #define D_INTERNAL_SYNC_F_FULLFSYNC 51
#endif  // F_FULLFSYNC

// D_INTERNAL_SYNC_EDQUOT
//   constant: the errno for an exhausted quota, where the platform names one,
// or -1, which no errno is, where it does not.
#ifdef EDQUOT
    #define D_INTERNAL_SYNC_EDQUOT EDQUOT
#else
    #define D_INTERNAL_SYNC_EDQUOT (-1)
#endif  // EDQUOT

/*
d_internal_sync_lost
  Whether a failed F_FULLFSYNC said something about the data rather than about
the command: an I/O error, or a full disk or quota. Those are reported as they
are. Asked next, fsync could report success for the same data -- an error,
once reported, need not be reported again -- and the data would be called
durable just after the device had said it could not keep it.
*/
static int
d_internal_sync_lost(
    int _code
)
{
    return ( (_code == EIO)    ||
             (_code == ENOSPC) ||
             (_code == D_INTERNAL_SYNC_EDQUOT) );
}

#endif  // !D_FILE_BACKEND_IS_STDC && !_HAS_WIN32 && _SYNC_FULL

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
d_file_sync_fd
  What "durable" means is a per-platform question answered once, so callers
need not ask it (d_internal_file_sync_raw). On Linux and Windows, fsync and
FlushFileBuffers push the data to the device. On Apple's systems fsync
explicitly does NOT ask the drive to flush its own write cache -- it returns,
the power fails, and the data is gone from the one call whose purpose was to
prevent that -- so D_CFG_FILE_SYNC_FULL routes there to fcntl(F_FULLFSYNC).
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

    // a failed sync means the data is not durable, whatever else happened
    if (d_internal_file_sync_raw(_fd) != 0)
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

    const int fd = d_file_descriptor_stream(_stream);

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

//==============================================================================
// 2.  INTERNAL SUPPORT
//==============================================================================

#if D_FILE_BACKEND_IS_STDC

/*
d_internal_file_sync_raw
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_internal_file_sync_raw(
    int _fd
)
{
    (void)_fd;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_file_sync_raw
  FlushFileBuffers, which on Windows reaches the device and asks it to empty
its own cache -- called directly, since the CRT's _commit reports every
failure, a full disk and a failing device among them, as EBADF. Only a disk
file has anything durable behind it: a pipe, a console or a device is refused
with EINVAL, as Linux refuses one, where FlushFileBuffers would wait on a pipe
until its reader had drained it. A descriptor without write access, which
FlushFileBuffers needs, gives EBADF, as a write through one does on POSIX.
*/
int
d_internal_file_sync_raw(
    int _fd
)
{
    const HANDLE handle = (HANDLE)_get_osfhandle(_fd);

    // not a descriptor the CRT knows, which _get_osfhandle has called EBADF
    if (handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    // nothing durable behind a pipe, a console or a device
    if (GetFileType(handle) != FILE_TYPE_DISK)
    {
        D_INTERNAL_FILE_SET_ERR(EINVAL);

        return -1;
    }

    // done; otherwise the reason, as the errno an fsync would have set
    if (FlushFileBuffers(handle))
    {
        return 0;
    }

    const DWORD error = GetLastError();
    const int   code  = (error == ERROR_ACCESS_DENIED)
                            ? EBADF
                            : d_internal_file_errno_win32(error);

    // named only by errno, which a build may not report through
    D_INTERNAL_FILE_SET_ERR(code);
    (void)code;

    return -1;
}

#elif (D_INTERNAL_FILE_SYNC_FULL == 1)

/*
d_internal_file_sync_raw
  Apple's systems: fsync explicitly does NOT ask the drive to flush its own
write cache, so a full sync is fcntl(F_FULLFSYNC) -- what SQLite and every
serious database there use -- retried across interruptions like any other
call. A filesystem that refuses the command, as most network ones do, gets
plain fsync with a warning rather than a failure fsync can spare it; a failure
that is about the data (d_internal_sync_lost) is reported, not retried.
*/
int
d_internal_file_sync_raw(
    int _fd
)
{
    int result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fcntl(_fd,
                                      D_INTERNAL_SYNC_F_FULLFSYNC,
                                      0));

    // done, or a failure that fsync must not be given the chance to hide
    if ( (result == 0) ||
         (d_internal_sync_lost(errno)) )
    {
        return result;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                           errno,
                           "d_file_sync_fd",
                           NULL,
                           "F_FULLFSYNC refused; fsync, which cannot reach "
                           "the drive cache, instead");
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fsync(_fd));

    return result;
}

#elif (D_INTERNAL_FILE_SYNC_DATA_ONLY == 1)

/*
d_internal_file_sync_raw
  fdatasync: a file's data, and the metadata needed to read it back, retried
across interruptions. A directory gets fsync instead: fdatasync need not wait
for a new entry to reach the device, and the entry is what a directory is
synced for -- which is why SQLite and PostgreSQL sync directories with fsync.
A descriptor fstat cannot describe gets fdatasync, which will say why.
*/
int
d_internal_file_sync_raw(
    int _fd
)
{
    struct stat st     = {0};
    int         result = -1;

    // a directory's entries are what fdatasync may leave behind
    if ( (fstat(_fd,
                &st) == 0) &&
         (S_ISDIR(st.st_mode)) )
    {
        D_INTERNAL_FILE_RETRY_EINTR(result,
                                    fsync(_fd));

        return result;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fdatasync(_fd));

    return result;
}

#else

/*
d_internal_file_sync_raw
  fsync, which on Linux and the other POSIX systems here reaches the device,
retried across interruptions.
*/
int
d_internal_file_sync_raw(
    int _fd
)
{
    int result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fsync(_fd));

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC, _HAS_WIN32, _SYNC_FULL, _SYNC_DATA_ONLY
