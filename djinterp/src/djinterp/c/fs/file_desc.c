/*******************************************************************************
* djinterp [c]                                                       file_desc.c
*
* Implementation of the descriptor lifecycle declared in file_desc.h.
*   One policy function, d_internal_desc_flags, adds close-on-exec and (on
* Windows) binary mode to every open, so no call site has to remember either.
* On the ISO C backend there are no descriptors, and every function here is a
* stub that reports ENOSYS.
*
*
* path:      /src/djinterp/c/fs/file_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_desc.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOSYS
#include <stdarg.h>  // va_list, va_start, va_arg, va_end
#include <stdio.h>   // FILE, fileno
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_desc.h"  // D_INTERNAL_FILE_DESC_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_desc_flags
  Applies this build's open policy to a caller's flags. Two additions, each
closing a hole the caller would otherwise have to remember at every call site:
    O_CLOEXEC  atomic with the open. Setting it afterwards with fcntl leaves a
               window in which another thread's fork+exec inherits the
               descriptor.
    O_BINARY   Windows only. A text-mode descriptor translates line endings,
               so d_file_read_fd of N bytes from an N-byte file returns fewer,
               and the caller cannot tell why.
  A caller who explicitly asked for text mode is left alone. Only the non-ISO
backends define this: the ISO C stubs never call it, and an unused static
function is a warning.
*/
static int
d_internal_desc_flags(
    int _flags
)
{
    int flags = _flags;

#if (D_INTERNAL_FILE_DESC_CLOEXEC == 1)
    #ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
    #endif  // O_CLOEXEC
#endif

#if (D_INTERNAL_FILE_DESC_BINARY == 1)
    #if ( (defined(O_BINARY)) &&                                               \
          (defined(O_TEXT)) )
    // honour an explicit O_TEXT; supply O_BINARY only where nothing was said
    if ((flags & O_TEXT) == 0)
    {
        flags |= O_BINARY;
    }
    #elif defined(O_BINARY)
    flags |= O_BINARY;
    #endif
#endif

    return flags;
}

#endif  // !D_FILE_BACKEND_IS_STDC

//==============================================================================
// 1.  DESCRIPTORS
//==============================================================================

#if D_FILE_BACKEND_IS_STDC

/*
d_file_open
  The ISO C backend has no descriptors, and none can be emulated, so this only
reports ENOSYS.
*/
int
d_file_open(
    const char* _path,
    int         _flags,
    ...
)
{
    (void)_path;
    (void)_flags;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_open",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_open
  POSIX leaves O_CREAT without a mode undefined, and in practice the call reads
whatever is on the stack and creates a file with those permissions -- a
security bug wearing the costume of a typo. So the mode is always read when
O_CREAT is present, and a mode of 0 is taken for a forgotten argument and
replaced with D_CFG_FILE_DESC_CREATE_MODE; the two cases cannot be told apart.
*/
int
d_file_open(
    const char* _path,
    int         _flags,
    ...
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_open",
                            NULL,
                            "path is NULL",
                            -1);

    const int flags = d_internal_desc_flags(_flags);
    int       mode  = D_INTERNAL_FILE_DESC_CREATE_MODE;

    // the mode argument exists only when the call may create
    if ((_flags & O_CREAT) != 0)
    {
        va_list args;

        va_start(args,
                 _flags);
        mode = (int)va_arg(args,
                           int);
        va_end(args);

        // a caller who passed O_CREAT with mode 0 almost certainly forgot the
        // argument rather than intending a file nobody can open
        if (mode == 0)
        {
            mode = D_INTERNAL_FILE_DESC_CREATE_MODE;
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   0,
                                   "d_file_open",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "O_CREAT with mode 0; using the configured "
                                   "default");
        }
    }

    int result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                _open(_path,
                                      flags,
                                      mode));
#else
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                open(_path,
                                     flags,
                                     (mode_t)mode));
#endif

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_open",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "open failed");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_descriptor_stream
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_descriptor_stream(
    FILE* _stream
)
{
    (void)_stream;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_descriptor_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_descriptor_stream
  A lookup, not an acquisition: nothing is opened, and the stream keeps
ownership of what is returned.
*/
int
d_file_descriptor_stream(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_descriptor_stream",
                            NULL,
                            "stream is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    return _fileno(_stream);
#else
    return fileno(_stream);
#endif
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_dup_fd
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_dup_fd(
    int _fd
)
{
    (void)_fd;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_dup_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_dup_fd
  POSIX dup() clears close-on-exec on the copy, so a careful O_CLOEXEC open
followed by a dup silently yields an inheritable descriptor. With
D_CFG_FILE_DESC_DUP_CLOEXEC set this uses F_DUPFD_CLOEXEC, which is atomic. A
kernel too old to know that command gets dup plus FD_CLOEXEC instead, which
leaves a brief window but still ends in the right state.
*/
int
d_file_dup_fd(
    int _fd
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_dup_fd",
                            NULL,
                            "descriptor is negative",
                            -1);

    int result = -1;

#if ( (D_INTERNAL_FILE_DESC_DUP_CLOEXEC == 1) &&                               \
      (defined(F_DUPFD_CLOEXEC)) )
    // atomic: no window in which the copy is inheritable
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                fcntl(_fd,
                                      F_DUPFD_CLOEXEC,
                                      0));

    // an old kernel may not know the command; fall back rather than fail
    if ( (result < 0) &&
         (errno == EINVAL) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                               0,
                               "d_file_dup_fd",
                               NULL,
                               "F_DUPFD_CLOEXEC unsupported; falling back to "
                               "dup");
        D_INTERNAL_FILE_RETRY_EINTR(result,
                                    dup(_fd));

        // restore close-on-exec, which dup cleared
        if (result >= 0)
        {
            (void)fcntl(result,
                        F_SETFD,
                        FD_CLOEXEC);
        }
    }
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                _dup(_fd));
#else
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                dup(_fd));
#endif

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_dup_fd",
                               NULL,
                               "dup failed");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_dup2_fd
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_dup2_fd(
    int _fd,
    int _fd2
)
{
    (void)_fd;
    (void)_fd2;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_dup2_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_dup2_fd
  Close-on-exec is deliberately left alone: the usual reason to call dup2 is
to install a descriptor on 0, 1 or 2 for a child to inherit, and forcing the
flag would defeat the call. The CRT's _dup2 reports success as 0 rather than
the new descriptor, so that is normalized to the POSIX contract.
*/
int
d_file_dup2_fd(
    int _fd,
    int _fd2
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_dup2_fd",
                            NULL,
                            "source descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_fd2 >= 0,
                            EBADF,
                            "d_file_dup2_fd",
                            NULL,
                            "target descriptor is negative",
                            -1);

    int result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                _dup2(_fd,
                                      _fd2));

    // the CRT reports success as 0 rather than the new descriptor; normalize
    // to the POSIX contract so callers have one shape to test
    if (result == 0)
    {
        result = _fd2;
    }
#else
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                dup2(_fd,
                                     _fd2));
#endif

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_dup2_fd",
                               NULL,
                               "dup2 failed");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_close_fd
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_close_fd(
    int _fd
)
{
    (void)_fd;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_close_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_close_fd
  Note what is NOT here: an EINTR retry. On Linux a close that returns EINTR
has already closed the descriptor, so retrying closes whatever a racing thread
just opened onto the same number -- a use-after-free with a file handle. POSIX
2008 made the state unspecified precisely because implementations disagreed.
Closing once and reporting the error is the only defensible behaviour.
*/
int
d_file_close_fd(
    int _fd
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_close_fd",
                            NULL,
                            "descriptor is negative",
                            -1);

    // deliberately not wrapped in D_INTERNAL_FILE_RETRY_EINTR -- see above
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _close(_fd);
#else
    const int result = close(_fd);
#endif

    // report the failure; the descriptor is released either way
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_close_fd",
                               NULL,
                               "close failed; the descriptor is gone "
                               "regardless");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC
