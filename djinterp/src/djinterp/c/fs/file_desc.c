#include "../../../../inc/djinterp/c/fs/file_desc.h"


///////////////////////////////////////////////////////////////////////////////
///             INTERNAL DEFINITIONS                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
d_internal_desc_flags
  Applies this build's open policy to a caller's flags.
  Two additions, both of which close a hole the caller would otherwise have to
remember at every call site:
    O_CLOEXEC -- atomic with the open. Setting it afterwards with fcntl leaves
                 a window in which another thread's fork+exec inherits the
                 descriptor.
    O_BINARY  -- Windows only. A text-mode descriptor translates line endings,
                 so d_read of N bytes from an N-byte file returns fewer and
                 the caller cannot tell why.
  A caller who explicitly asked for the opposite is left alone.

Parameter(s):
  _flags: the caller's open flags.
Return:
  The flags to hand the platform.
*/
static int
d_internal_desc_flags
(
    int _flags
)
{
    int flags;

    flags = _flags;

#if (D_INTERNAL_FILE_DESC_CLOEXEC == 1)
    #ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
    #endif
#endif

#if (D_INTERNAL_FILE_DESC_BINARY == 1)
    #if ( defined(O_BINARY) && defined(O_TEXT) )
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


///////////////////////////////////////////////////////////////////////////////
///             I.  ACQUISITION                                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_open
  Opens a file and returns a descriptor (POSIX open equivalent).
  The variadic third argument is the creation mode, and is read only when
_flags contains O_CREAT. When O_CREAT is set and no mode is supplied, POSIX
says the behaviour is undefined -- in practice it reads whatever is on the
stack and creates a file with those permissions, which is a security bug
wearing the costume of a typo. This substitutes D_CFG_FILE_DESC_CREATE_MODE
instead, and cannot tell the two cases apart, so it always reads the argument
when O_CREAT is present. Pass one.

Parameter(s):
  _path:  path to open.
  _flags: O_RDONLY / O_WRONLY / O_RDWR, optionally OR'd with O_CREAT, O_TRUNC,
          O_APPEND, O_EXCL, ...
  ...:    mode_t creation mode; required when _flags contains O_CREAT.
Return:
  A descriptor on success, or -1 on failure with errno set.
*/
int
d_open
(
    const char* _path,
    int         _flags,
    ...
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_path;
    (void)_flags;

    // the ISO C backend has no descriptors; this cannot be emulated
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_open",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    va_list args;
    int     result;
    int     flags;
    int     mode;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_open",
                            NULL,
                            "path is NULL",
                            -1);

    flags = d_internal_desc_flags(_flags);
    mode  = D_INTERNAL_FILE_DESC_CREATE_MODE;

    // the mode argument exists only when the call may create
    if ((_flags & O_CREAT) != 0)
    {
        va_start(args, _flags);
        mode = (int)va_arg(args, int);
        va_end(args);

        // a caller who passed O_CREAT with mode 0 almost certainly forgot the
        // argument rather than intending a file nobody can open
        if (mode == 0)
        {
            mode = D_INTERNAL_FILE_DESC_CREATE_MODE;
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   0,
                                   "d_open",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "O_CREAT with mode 0; using the configured default");
        }
    }

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result, _open(_path, flags, mode));
    #else
    D_INTERNAL_FILE_RETRY_EINTR(result, open(_path, flags, (mode_t)mode));
    #endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_open",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "open failed");
    }

    return result;
#endif
}


/*
d_fileno
  Returns the descriptor a stream is built on.
  Borrowed, not owned: the stream still owns it, closing it out from under the
stream is undefined, and it dies with the stream. Use d_dup if you need one
that outlives it.

Parameter(s):
  _stream: an open stream.
Return:
  The descriptor on success, or -1 on failure with errno set.
*/
int
d_fileno
(
    FILE* _stream
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_stream;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_fileno",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_fileno",
                            NULL,
                            "stream is NULL",
                            -1);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    return _fileno(_stream);
    #else
    return fileno(_stream);
    #endif
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             II.  DUPLICATION                                            ///
///////////////////////////////////////////////////////////////////////////////

/*
d_dup
  Duplicates a descriptor onto the lowest free number.
  The copy shares the file offset and status flags with the original -- it is
a second handle on one open file description, not a second open.
  It does NOT share close-on-exec: POSIX specifies that dup() clears the flag
on the new descriptor, so a careful O_CLOEXEC open followed by a dup silently
yields an inheritable descriptor. When D_CFG_FILE_DESC_DUP_CLOEXEC is set this
uses F_DUPFD_CLOEXEC instead, which is atomic and gets it right.

Parameter(s):
  _fd: an open descriptor.
Return:
  The new descriptor on success, or -1 on failure with errno set.
*/
int
d_dup
(
    int _fd
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_dup",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_dup",
                            NULL,
                            "descriptor is negative",
                            -1);

    #if ( (D_INTERNAL_FILE_DESC_DUP_CLOEXEC == 1) && defined(F_DUPFD_CLOEXEC) )
    // atomic: no window in which the copy is inheritable
    D_INTERNAL_FILE_RETRY_EINTR(result, fcntl(_fd, F_DUPFD_CLOEXEC, 0));

    // an old kernel may not know the command; fall back rather than fail
    if ( (result < 0) &&
         (errno == EINVAL) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                               0,
                               "d_dup",
                               NULL,
                               "F_DUPFD_CLOEXEC unsupported; falling back to dup");
        D_INTERNAL_FILE_RETRY_EINTR(result, dup(_fd));

        if (result >= 0)
        {
            (void)fcntl(result, F_SETFD, FD_CLOEXEC);
        }
    }
    #elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result, _dup(_fd));
    #else
    D_INTERNAL_FILE_RETRY_EINTR(result, dup(_fd));
    #endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dup",
                               NULL,
                               "dup failed");
    }

    return result;
#endif
}


/*
d_dup2
  Duplicates a descriptor onto a chosen number, closing whatever was there.
  Deliberately does NOT force close-on-exec, unlike d_dup: the usual reason to
call dup2 is to install a descriptor onto 0/1/2 for a child to inherit, and
making it close-on-exec would defeat the call.
  dup2(fd, fd) with a valid fd is a documented no-op and is not an error.

Parameter(s):
  _fd:  an open descriptor.
  _fd2: the descriptor number to install it onto.
Return:
  _fd2 on success, or -1 on failure with errno set.
*/
int
d_dup2
(
    int _fd,
    int _fd2
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_fd2;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_dup2",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_dup2",
                            NULL,
                            "source descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_fd2 >= 0,
                            EBADF,
                            "d_dup2",
                            NULL,
                            "target descriptor is negative",
                            -1);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result, _dup2(_fd, _fd2));

    // the CRT reports success as 0 rather than the new descriptor; normalize
    // to the POSIX contract so callers have one shape to test
    if (result == 0)
    {
        result = _fd2;
    }
    #else
    D_INTERNAL_FILE_RETRY_EINTR(result, dup2(_fd, _fd2));
    #endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dup2",
                               NULL,
                               "dup2 failed");
    }

    return result;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             III.  RELEASE                                               ///
///////////////////////////////////////////////////////////////////////////////

/*
d_close
  Closes a descriptor.
  Note what is NOT here: an EINTR retry. On Linux a close that returns EINTR
has already closed the descriptor, so retrying closes whatever a racing thread
just opened onto the same number -- a use-after-free with a file handle. POSIX
2008 made the state unspecified precisely because implementations disagreed.
Closing once and reporting the error is the only defensible behaviour.

Parameter(s):
  _fd: the descriptor to close.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_close
(
    int _fd
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_close",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_close",
                            NULL,
                            "descriptor is negative",
                            -1);

    // deliberately not wrapped in D_INTERNAL_FILE_RETRY_EINTR -- see above
    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _close(_fd);
    #else
    result = close(_fd);
    #endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_close",
                               NULL,
                               "close failed; the descriptor is gone regardless");
    }

    return result;
#endif
}
