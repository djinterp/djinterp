/*******************************************************************************
* djinterp [c]                                                       file_desc.c
*
* Implementation of the descriptor lifecycle declared in file_desc.h.
*   One policy function, d_internal_desc_flags, keeps every descriptor
* d_file_open makes from a child process -- O_CLOEXEC on POSIX, the CRT's
* _O_NOINHERIT on Windows -- and on Windows makes it binary, so no call site
* has to remember either; d_file_dup_fd keeps its copy from a child the same
* way. Each call is one platform call behind a per-platform helper. On the
* ISO C backend there are no descriptors, and every function here is a stub
* that reports ENOSYS.
*
*
* path:      /src/djinterp/c/filesys/file_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_desc.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOSYS
#include <stdarg.h>  // va_list, va_start, va_arg, va_end
#include <stdio.h>   // FILE, fileno
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_desc.h"  // D_INTERNAL_FILE_DESC_*


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if !D_FILE_BACKEND_IS_STDC

// D_INTERNAL_DESC_NOINHERIT
//   constant: the open flag that keeps a descriptor from a child process when
// the build asks (D_CFG_FILE_DESC_CLOEXEC): O_CLOEXEC on POSIX, which exec
// honours; on Windows the CRT's _O_NOINHERIT, whose handle CreateProcess does
// not pass on; 0 where the platform has neither.
#if ( (D_INTERNAL_FILE_DESC_CLOEXEC == 1) &&                                   \
      D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32) )
    #define D_INTERNAL_DESC_NOINHERIT _O_NOINHERIT
#elif ( (D_INTERNAL_FILE_DESC_CLOEXEC == 1) &&                                 \
        defined(O_CLOEXEC) )
    #define D_INTERNAL_DESC_NOINHERIT O_CLOEXEC
#else
    #define D_INTERNAL_DESC_NOINHERIT 0
#endif

// D_INTERNAL_DESC_TMPFILE
//   constant: Linux's O_TMPFILE, which takes a mode as O_CREAT does, or 0 where
// there is none. The library is built without _GNU_SOURCE, so glibc names it
// only as __O_TMPFILE.
#if defined(O_TMPFILE)
    #define D_INTERNAL_DESC_TMPFILE O_TMPFILE
#elif defined(__O_TMPFILE)
    #define D_INTERNAL_DESC_TMPFILE __O_TMPFILE
#else
    #define D_INTERNAL_DESC_TMPFILE 0
#endif

/*
d_internal_desc_takes_mode
  Whether a call with these flags may create a file, and so carries a mode
argument: O_CREAT, or Linux's O_TMPFILE, which is several bits that must all
be present.
*/
static int
d_internal_desc_takes_mode(
    int _flags
)
{
    const int temporary = D_INTERNAL_DESC_TMPFILE;

    return ( ((_flags & O_CREAT) != 0) ||
             ( (temporary != 0) &&
               ((_flags & temporary) == temporary) ) );
}

/*
d_internal_desc_mode
  The creation mode a caller passed, unless it is 0: a caller who may create
with mode 0 almost certainly forgot the argument rather than intending a file
nobody can open, so it becomes D_CFG_FILE_DESC_CREATE_MODE, with a warning.
*/
static int
d_internal_desc_mode(
    int         _mode,
    const char* _path
)
{
    (void)_path;

    // a forgotten argument, most likely
    if (_mode == 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               0,
                               "d_file_open",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "creating with mode 0; using the configured "
                               "default");

        return D_INTERNAL_FILE_DESC_CREATE_MODE;
    }

    return _mode;
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

// D_INTERNAL_DESC_TEXT_MODES
//   constant: every flag that asks the CRT for a text mode -- _O_TEXT and the
// wide and UTF-8 ones. A caller who names any of them keeps it, rather than
// having _O_BINARY added beside it.
#define D_INTERNAL_DESC_TEXT_MODES                                             \
    (_O_TEXT | _O_WTEXT | _O_U16TEXT | _O_U8TEXT)

/*
d_internal_desc_flags
  Applies this build's open policy to a caller's flags. Two additions, each
closing a hole the caller would otherwise have to remember at every call site:
    _O_NOINHERIT  the descriptor's handle is not inherited by a child process
                  -- Windows' close-on-exec. Without it, any process spawned
                  with handles inherited holds the file open, and on Windows
                  an open file cannot be deleted or renamed.
    _O_BINARY     a text-mode descriptor translates line endings, so
                  d_file_read_fd of N bytes from an N-byte file returns fewer,
                  and the caller cannot tell why.
  A caller who asked for any text mode is left in it.
*/
static int
d_internal_desc_flags(
    int _flags
)
{
    int flags = _flags | D_INTERNAL_DESC_NOINHERIT;

    // binary, unless the build says otherwise or the caller named a text mode
    if ( (D_INTERNAL_FILE_DESC_BINARY == 1) &&
         ((flags & D_INTERNAL_DESC_TEXT_MODES) == 0) )
    {
        flags |= _O_BINARY;
    }

    return flags;
}

/*
d_internal_desc_open
  The CRT's _open, which takes the mode as an int.
*/
static int
d_internal_desc_open(
    const char* _path,
    int         _flags,
    int         _mode
)
{
    return _open(_path,
                 _flags,
                 _mode);
}

/*
d_internal_desc_fileno
  The CRT's _fileno, whose answer for a stream with no descriptor -- -2, when a
program without a console asks about stdout -- is not -1 and sets no errno;
both are put right here.
*/
static int
d_internal_desc_fileno(
    FILE* _stream
)
{
    const int fd = _fileno(_stream);

    // any negative answer is the documented -1, with a reason
    if (fd < 0)
    {
        D_INTERNAL_FILE_SET_ERR(EBADF);

        return -1;
    }

    return fd;
}

/*
d_internal_desc_noinherit
  Keeps a duplicate's handle from a child process, as _O_NOINHERIT keeps an
opened one: the CRT's _dup makes the copy inheritable, as POSIX dup clears
FD_CLOEXEC. A console's handle is left as it is, since Windows before 8
cannot change one -- as Python leaves it. A copy that cannot be kept back is
closed, and the dup fails with the reason.
*/
static int
d_internal_desc_noinherit(
    int _fd
)
{
    const HANDLE handle = (HANDLE)_get_osfhandle(_fd);

    // nothing to change on a console, and nothing that can be
    if ( (D_INTERNAL_FILE_DESC_DUP_CLOEXEC == 0) ||
         (GetFileType(handle) == FILE_TYPE_CHAR) ||
         (SetHandleInformation(handle,
                               HANDLE_FLAG_INHERIT,
                               0)) )
    {
        return _fd;
    }

    const int code = d_internal_file_errno_win32(GetLastError());

    (void)_close(_fd);
    D_INTERNAL_FILE_SET_ERR(code);
    (void)code;

    return -1;
}

/*
d_internal_desc_dup
  The CRT's _dup, its copy kept from a child process when the build asks
(d_internal_desc_noinherit).
*/
static int
d_internal_desc_dup(
    int _fd
)
{
    const int result = _dup(_fd);

    return (result < 0) ? result : d_internal_desc_noinherit(result);
}

/*
d_internal_desc_dup2
  The CRT's _dup2, which reports success as 0 rather than the new descriptor;
normalized to the POSIX contract, so callers have one shape to test.
*/
static int
d_internal_desc_dup2(
    int _fd,
    int _fd2
)
{
    return (_dup2(_fd,
                  _fd2) == 0) ? _fd2 : -1;
}

/*
d_internal_desc_close
  The CRT's _close.
*/
static int
d_internal_desc_close(
    int _fd
)
{
    return _close(_fd);
}

#else

/*
d_internal_desc_flags
  Applies this build's open policy to a caller's flags: O_CLOEXEC, atomic with
the open. Setting it afterwards with fcntl leaves a window in which another
thread's fork+exec inherits the descriptor.
*/
static int
d_internal_desc_flags(
    int _flags
)
{
    return _flags | D_INTERNAL_DESC_NOINHERIT;
}

/*
d_internal_desc_open
  POSIX open, which takes the mode as a mode_t.
*/
static int
d_internal_desc_open(
    const char* _path,
    int         _flags,
    int         _mode
)
{
    return open(_path,
                _flags,
                (mode_t)_mode);
}

/*
d_internal_desc_fileno
  POSIX fileno, which already answers -1 with EBADF for a stream that has no
descriptor.
*/
static int
d_internal_desc_fileno(
    FILE* _stream
)
{
    return fileno(_stream);
}

#if ( (D_INTERNAL_FILE_DESC_DUP_CLOEXEC == 1) &&                               \
      defined(F_DUPFD_CLOEXEC) )

/*
d_internal_desc_dup
  POSIX dup() clears close-on-exec on the copy, so a careful O_CLOEXEC open
followed by a dup silently yields an inheritable descriptor. F_DUPFD_CLOEXEC
is atomic; a kernel too old to know that command gets dup plus FD_CLOEXEC
instead, which leaves a brief window but still ends in the right state.
*/
static int
d_internal_desc_dup(
    int _fd
)
{
    int result = fcntl(_fd,
                       F_DUPFD_CLOEXEC,
                       0);

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
        result = dup(_fd);

        // restore close-on-exec, which dup cleared
        if (result >= 0)
        {
            (void)fcntl(result,
                        F_SETFD,
                        FD_CLOEXEC);
        }
    }

    return result;
}

#else

/*
d_internal_desc_dup
  POSIX dup, whose copy is inheritable: the build does not ask otherwise, or
the platform cannot do it atomically.
*/
static int
d_internal_desc_dup(
    int _fd
)
{
    return dup(_fd);
}

#endif  // D_INTERNAL_FILE_DESC_DUP_CLOEXEC == 1 && F_DUPFD_CLOEXEC

/*
d_internal_desc_dup2
  POSIX dup2, which already returns the new descriptor.
*/
static int
d_internal_desc_dup2(
    int _fd,
    int _fd2
)
{
    return dup2(_fd,
                _fd2);
}

/*
d_internal_desc_close
  POSIX close.
*/
static int
d_internal_desc_close(
    int _fd
)
{
    return close(_fd);
}

#endif  // D_CFG_FILE_HAS_WIN32

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
d_file_open
  POSIX leaves O_CREAT without a mode undefined, and in practice the call reads
whatever is on the stack and creates a file with those permissions -- a
security bug wearing the costume of a typo. So the mode is always read when
the flags may create (d_internal_desc_takes_mode: O_TMPFILE as well as
O_CREAT), and a mode of 0 is taken for a forgotten argument and replaced
(d_internal_desc_mode); the two cases cannot be told apart.
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

    int mode = D_INTERNAL_FILE_DESC_CREATE_MODE;

    // the mode argument exists only when the call may create
    if (d_internal_desc_takes_mode(_flags))
    {
        va_list args;

        va_start(args,
                 _flags);
        mode = d_internal_desc_mode((int)va_arg(args,
                                                int),
                                    _path);
        va_end(args);
    }

    int result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                d_internal_desc_open(
                                    _path,
                                    d_internal_desc_flags(_flags),
                                    mode));

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

/*
d_file_descriptor_stream
  A lookup, not an acquisition: nothing is opened, and the stream keeps
ownership of what is returned. A stream with no descriptor answers -1 with
EBADF on every platform (d_internal_desc_fileno).
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

    return d_internal_desc_fileno(_stream);
}

/*
d_file_dup_fd
  The copy is kept from a child process when the build asks
(d_internal_desc_dup), where the platform's own dup would clear that.
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

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                d_internal_desc_dup(_fd));

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

/*
d_file_dup2_fd
  Close-on-exec is deliberately left alone: the usual reason to call dup2 is
to install a descriptor on 0, 1 or 2 for a child to inherit, and forcing the
flag would defeat the call.
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

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                d_internal_desc_dup2(_fd,
                                                     _fd2));

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
    const int result = d_internal_desc_close(_fd);

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
