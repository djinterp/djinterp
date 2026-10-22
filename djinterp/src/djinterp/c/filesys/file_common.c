/*******************************************************************************
* djinterp [c]                                                     file_common.c
*
* Implementation of the notification hook, the allocation funnel, the Win32
* error translation and the path conversions declared in file_common.h.
*   A notice is assembled only after a handler is known to exist, so a build
* that compiles notifications in but installs no handler pays one load and one
* branch per notice. Every filesys allocation passes the D_CFG_FILE_MAX_ALLOC
* ceiling here before it reaches the configured allocator. The Win32 reasons
* a caller can act on are a table, and every other reason reads as EIO. A
* build that hands Windows UTF-16 converts here, refusing text that is not
* UTF-8 or UTF-16 rather than substituting U+FFFD, which names another file.
*
*
* path:      /src/djinterp/c/filesys/file_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // corresponding header
// std
#include <errno.h>   // errno, ENOMEM and, for Win32's errors, the E* codes
#include <limits.h>  // INT_MAX
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // fprintf, and stderr for the default stream
#include <stdlib.h>  // malloc, realloc, free: the allocator defaults
#include <string.h>  // memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_common.h"  // D_CFG_FILE_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_COMMON_NOTIFY_ON
//   constant: 1 when this build compiles notifications in, so that a handler
// installed is one that runs.
#define D_INTERNAL_COMMON_NOTIFY_ON (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)

// D_INTERNAL_COMMON_BACKEND_NAME
//   constant: the display name of the backend this build resolved to.
#if D_FILE_BACKEND_IS_NATIVE
    #define D_INTERNAL_COMMON_BACKEND_NAME "native"
#elif D_FILE_BACKEND_IS_POSIX
    #define D_INTERNAL_COMMON_BACKEND_NAME "posix"
#else
    #define D_INTERNAL_COMMON_BACKEND_NAME "stdc"
#endif  // D_FILE_BACKEND_IS_NATIVE, D_FILE_BACKEND_IS_POSIX

// D_INTERNAL_COMMON_ALLOC_CEILING
//   constant: D_CFG_FILE_MAX_ALLOC as a size_t, or 0 for no ceiling. A ceiling
// past what a size_t holds refuses nothing a size_t can ask for, so it is no
// ceiling -- not the remainder a cast would leave, which on a 32-bit build
// makes a 4 GiB ceiling one of 0 bytes and refuses every allocation. A
// ceiling of 0 is settled here, where comparing it would draw a warning.
#if (D_CFG_NORM(D_CFG_FILE_MAX_ALLOC) == 0)
    #define D_INTERNAL_COMMON_ALLOC_CEILING ((size_t)0)
#else
    #define D_INTERNAL_COMMON_ALLOC_CEILING                                    \
        ( ((D_CFG_FILE_MAX_ALLOC) > (size_t)-1)                                \
              ? (size_t)0                                                      \
              : (size_t)(D_CFG_FILE_MAX_ALLOC) )
#endif  // D_CFG_FILE_MAX_ALLOC == 0

// D_INTERNAL_COMMON_ATTRIBUTES
//   macro: GetFileAttributes, in the A or W form the build's path text takes
// (file_common.h, 6.6).
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_COMMON_ATTRIBUTES GetFileAttributesW
#else
    #define D_INTERNAL_COMMON_ATTRIBUTES GetFileAttributesA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_COMMON_CREATE_FILE
//   macro: CreateFile, in the A or W form the build's path text takes.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_COMMON_CREATE_FILE CreateFileW
#else
    #define D_INTERNAL_COMMON_CREATE_FILE CreateFileA
#endif  // D_INTERNAL_FILE_WIDE == 1

// d_internal_file_notify_handler
//   variable: the active notification handler, or NULL for none.
//   Set-once-at-startup is the contract: this is a plain pointer, not an
// atomic, because the alternative is to make every filesys module depend on
// datomic.h to support a case (swapping the log sink from another thread
// mid-call) that no sane program needs. A build with notifications compiled
// out keeps it too, never read, so that each call below is one definition.
#if D_CFG_IS_ON(D_CFG_FILE_NOTIFY_DEFAULT_HANDLER)
    static fn_file_notify d_internal_file_notify_handler =
        d_file_notify_default_handler;
#else
    static fn_file_notify d_internal_file_notify_handler = NULL;
#endif  // D_CFG_FILE_NOTIFY_DEFAULT_HANDLER

// d_internal_file_notify_context
//   variable: opaque cookie handed back to the active handler.
static void* d_internal_file_notify_context = NULL;

//==============================================================================
// 5.  NOTIFICATIONS
//==============================================================================

/*
d_file_notify_set_handler
  A plain store into two file-scope pointers. Nothing synchronizes it, which
is why the contract is set-once-at-startup.
*/
void
d_file_notify_set_handler(
    fn_file_notify _handler,
    void*          _context
)
{
    d_internal_file_notify_handler = _handler;
    d_internal_file_notify_context = _context;

    return;
}

/*
d_file_notify_get_handler
  With notifications compiled out, nothing installed will ever run, so the
answer is "none installed": NULL, and the cookie cleared.
*/
fn_file_notify
d_file_notify_get_handler(
    void** _context
)
{
    // the cookie is optional
    if (_context)
    {
        *_context = (D_INTERNAL_COMMON_NOTIFY_ON)
                        ? d_internal_file_notify_context
                        : NULL;
    }

    return (D_INTERNAL_COMMON_NOTIFY_ON)
               ? d_internal_file_notify_handler
               : NULL;
}

/*
d_file_notify_default_handler
  errno is saved and restored around the write, because a handler runs while
the failing call's errno is still the value its caller is about to read. It
is never installed unasked -- a library that writes to a stream nobody asked
it to write to corrupts somebody's stdout. The line reads "function: message:
'path' (errno=N)", each part present only when the notice carries it; the
quotes keep a path apart from the message even when it holds spaces.
*/
void
d_file_notify_default_handler(
    const struct d_file_notice* _notice,
    void*                       _context
)
{
    (void)_context;

    // parameter validation
    if (!_notice)
    {
        return;
    }

    const int         saved_errno = errno;
    const char* const message     = (_notice->message != NULL)
                                        ? _notice->message
                                        : "";
    const char* const path        = (_notice->path != NULL)
                                        ? _notice->path
                                        : "";

    fprintf(D_CFG_FILE_NOTIFY_STREAM,
            "[djinterp/filesys] %s: %s%s%s%s%s%s (errno=%d)\n",
            d_file_notify_level_name(_notice->level),
            (_notice->function != NULL) ? _notice->function : "?",
            (message[0] != '\0') ? ": " : "",
            message,
            (path[0] != '\0') ? ": '" : "",
            path,
            (path[0] != '\0') ? "'" : "",
            _notice->error);

    errno = saved_errno;

    return;
}

/*
d_file_notify_level_name
  A switch over the enum; any other value falls through to "unknown", so the
result is never NULL and can go straight to printf.
*/
const char*
d_file_notify_level_name(
    int _level
)
{
    // map the level to its display name
    switch (_level)
    {
        case D_FILE_NOTIFY_NONE:
        {
            return "none";
        }
        case D_FILE_NOTIFY_ERROR:
        {
            return "error";
        }
        case D_FILE_NOTIFY_WARN:
        {
            return "warning";
        }
        case D_FILE_NOTIFY_INFO:
        {
            return "info";
        }
        case D_FILE_NOTIFY_TRACE:
        {
            return "trace";
        }
        default:
        {
            break;
        }
    }

    return "unknown";
}

/*
d_file_backend_name
  Resolved entirely at compile time: the backend is a build decision, so each
build has exactly one answer. It exists for diagnostics, and for a test suite
that has to skip what the build cannot do.
*/
const char*
d_file_backend_name(
    void
)
{
    return D_INTERNAL_COMMON_BACKEND_NAME;
}

//==============================================================================
// 6.  INTERNAL SUPPORT
//==============================================================================

/*
d_internal_file_notify_emit
  The record is built on the stack only after a handler is known to exist.
With none installed, which is the common case, a notice costs one load and one
branch. D_INTERNAL_FILE_NOTIFY has already applied the severity ceiling, and
with notifications compiled out it never calls this; a call that comes anyway
finds no handler to run.
*/
void
d_internal_file_notify_emit(
    int         _level,
    int         _error,
    const char* _function,
    const char* _path,
    const char* _message
)
{
    const fn_file_notify handler = (D_INTERNAL_COMMON_NOTIFY_ON)
                                       ? d_internal_file_notify_handler
                                       : NULL;

    // no handler is the common case; do not build a record nobody reads
    if (!handler)
    {
        return;
    }

    const struct d_file_notice notice = { _level,
                                          _error,
                                          _function,
                                          _path,
                                          _message };

    handler(&notice,
            d_internal_file_notify_context);

    return;
}

/*
d_internal_file_alloc_short
  An allocation that came back empty or was refused: ENOMEM, and a notice
naming the function and why.
*/
static void
d_internal_file_alloc_short(
    const char* _function,
    const char* _message
)
{
    // named only through errno and a notice, either of which a build may drop
    D_INTERNAL_FILE_SET_ERR(ENOMEM);
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                           ENOMEM,
                           _function,
                           NULL,
                           _message);
    (void)_function;
    (void)_message;

    return;
}

/*
d_internal_file_alloc_refused
  Whether a request is past D_INTERNAL_COMMON_ALLOC_CEILING. One that is gets
reported as a shortfall, and the allocator never sees it.
*/
static int
d_internal_file_alloc_refused(
    size_t      _size,
    const char* _function
)
{
    const size_t ceiling = D_INTERNAL_COMMON_ALLOC_CEILING;

    // no ceiling, or a request within it
    if ( (ceiling == 0) ||
         (_size <= ceiling) )
    {
        return 0;
    }

    d_internal_file_alloc_short(_function,
                                "request exceeds D_CFG_FILE_MAX_ALLOC");

    return 1;
}

/*
d_internal_file_alloc
  The ceiling is checked before the allocator sees the request because the
size usually came from a file's own metadata: without it, d_file_read_all on a
sparse 40 GiB file is an out-of-memory event in a process that only wanted to
read a config file.
*/
void*
d_internal_file_alloc(
    size_t _size
)
{
    // refuse an implausible request before handing it to the allocator
    if (d_internal_file_alloc_refused(_size,
                                      "d_internal_file_alloc"))
    {
        return NULL;
    }

    void* const result = D_CFG_FILE_MALLOC(_size);

    // report the shortfall; the caller only learns that it got NULL
    if (!result)
    {
        d_internal_file_alloc_short("d_internal_file_alloc",
                                    "allocator returned NULL");
    }

    return result;
}

/*
d_internal_file_realloc
  Keeps realloc's contract exactly, sharp edge included: on failure the
original block is untouched and still the caller's. The ceiling is the same
one d_internal_file_alloc applies, for the same reason.
*/
void*
d_internal_file_realloc(
    void*  _ptr,
    size_t _size
)
{
    // refuse an implausible request before handing it to the allocator
    if (d_internal_file_alloc_refused(_size,
                                      "d_internal_file_realloc"))
    {
        return NULL;
    }

    void* const result = D_CFG_FILE_REALLOC(_ptr,
                                            _size);

    // report the shortfall; the caller only learns that it got NULL
    if (!result)
    {
        d_internal_file_alloc_short("d_internal_file_realloc",
                                    "allocator returned NULL");
    }

    return result;
}

/*
d_internal_file_free
  NULL is filtered here rather than handed on, because a configured
D_CFG_FILE_FREE need not tolerate it the way free does.
*/
void
d_internal_file_free(
    void* _ptr
)
{
    // a NULL block is accepted and ignored
    if (_ptr)
    {
        D_CFG_FILE_FREE(_ptr);
    }

    return;
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

// d_internal_file_errno_pair
//   struct: a Win32 reason and the errno that a POSIX call failing for the
// same reason sets.
struct d_internal_file_errno_pair
{
    DWORD error;  // a GetLastError() value
    int   code;   // the errno that names it
};

// d_internal_file_errno_pairs
//   constant: the reasons a caller can act on, each with its errno. Any other
// reason is an I/O failure, which is what it looks like from outside.
static const struct d_internal_file_errno_pair d_internal_file_errno_pairs[] =
{
    { ERROR_FILE_NOT_FOUND,        ENOENT       },
    { ERROR_PATH_NOT_FOUND,        ENOENT       },
    { ERROR_INVALID_DRIVE,         ENOENT       },
    { ERROR_INVALID_NAME,          ENOENT       },
    { ERROR_BAD_PATHNAME,          ENOENT       },
    { ERROR_BAD_NETPATH,           ENOENT       },
    { ERROR_BAD_NET_NAME,          ENOENT       },
    { ERROR_ACCESS_DENIED,         EACCES       },
    { ERROR_NETWORK_ACCESS_DENIED, EACCES       },
    { ERROR_SHARING_VIOLATION,     EACCES       },
    { ERROR_LOCK_VIOLATION,        EACCES       },
    { ERROR_PRIVILEGE_NOT_HELD,    EPERM        },
    { ERROR_INVALID_HANDLE,        EBADF        },
    { ERROR_TOO_MANY_OPEN_FILES,   EMFILE       },
    { ERROR_INVALID_PARAMETER,     EINVAL       },
    { ERROR_NOT_A_REPARSE_POINT,   EINVAL       },
    { ERROR_ALREADY_EXISTS,        EEXIST       },
    { ERROR_FILE_EXISTS,           EEXIST       },
    { ERROR_NOT_SAME_DEVICE,       EXDEV        },
    { ERROR_DIRECTORY,             ENOTDIR      },
    { ERROR_DIR_NOT_EMPTY,         ENOTEMPTY    },
    { ERROR_FILENAME_EXCED_RANGE,  ENAMETOOLONG },
    { ERROR_CANT_RESOLVE_FILENAME, ELOOP        },
    { ERROR_WRITE_PROTECT,         EROFS        },
    { ERROR_DISK_FULL,             ENOSPC       },
    { ERROR_HANDLE_DISK_FULL,      ENOSPC       },
    { ERROR_NOT_ENOUGH_MEMORY,     ENOMEM       },
    { ERROR_OUTOFMEMORY,           ENOMEM       },
    { ERROR_INVALID_FUNCTION,      ENOTSUP      },
    { ERROR_NOT_SUPPORTED,         ENOTSUP      }
};

/*
d_internal_file_errno_win32
  One table for every module that calls Win32 directly -- a search, a move, a
handle query -- so a reason reads the same whichever call met it. Only the
reasons a caller can act on get a name of their own
(d_internal_file_errno_pairs); the rest are I/O failures. A failure is no hot
path, so the table is searched in order.
*/
int
d_internal_file_errno_win32(
    DWORD _error
)
{
    const size_t count = sizeof(d_internal_file_errno_pairs) /
                         sizeof(d_internal_file_errno_pairs[0]);

    // the reasons a caller can act on
    for (size_t index = 0u; index < count; index++)
    {
        if (d_internal_file_errno_pairs[index].error == _error)
        {
            return d_internal_file_errno_pairs[index].code;
        }
    }

    return EIO;
}

/*
d_internal_file_tag_is_link_win32
  The two reparse tags that name another path and that Win32 itself follows:
IO_REPARSE_TAG_SYMLINK and IO_REPARSE_TAG_MOUNT_POINT, a junction. Following
anything else is the filesystem's business, not a link's.
*/
int
d_internal_file_tag_is_link_win32(
    DWORD _tag
)
{
    return ( (_tag == IO_REPARSE_TAG_SYMLINK) ||
             (_tag == IO_REPARSE_TAG_MOUNT_POINT) );
}

/*
d_internal_file_handle_is_link_win32
  Whether an open reparse point is a link, by its tag. A reparse point whose
tag cannot be read -- the query fails, or answers as though there were no
reparse point -- is taken for one: mistaking a link for the directory it
names is the error that costs data.
*/
static int
d_internal_file_handle_is_link_win32(
    HANDLE _handle
)
{
    FILE_ATTRIBUTE_TAG_INFO tag = {0};

    // a tag that cannot be read is no evidence against a link
    if ( (!GetFileInformationByHandleEx(_handle,
                                        FileAttributeTagInfo,
                                        &tag,
                                        (DWORD)sizeof(tag))) ||
         ((tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) )
    {
        return 1;
    }

    return d_internal_file_tag_is_link_win32(tag.ReparseTag);
}

/*
d_internal_file_open_reparse_win32
  The reparse point itself, opened without following it
(FILE_FLAG_OPEN_REPARSE_POINT): enough access to read its attributes and tag,
shared with every other opener.
*/
static HANDLE
d_internal_file_open_reparse_win32(
    const d_internal_file_nchar_t* _path
)
{
    return D_INTERNAL_COMMON_CREATE_FILE(_path,
                                         FILE_READ_ATTRIBUTES,
                                         FILE_SHARE_READ  |
                                         FILE_SHARE_WRITE |
                                         FILE_SHARE_DELETE,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_FLAG_OPEN_REPARSE_POINT |
                                         FILE_FLAG_BACKUP_SEMANTICS,
                                         NULL);
}

/*
d_internal_file_is_link_win32
  The attributes first, since most paths are no reparse point at all; then the
reparse point itself (d_internal_file_open_reparse_win32), asked for its tag
(d_internal_file_handle_is_link_win32). A path that will not open, or will not
convert to the platform's text, is not reported as a link: the caller's own
call then meets the reason and reports it. A link whose information cannot be
read is still a link, with that information zeroed.
*/
int
d_internal_file_is_link_win32(
    const char*                 _path,
    BY_HANDLE_FILE_INFORMATION* _info
)
{
    struct d_internal_file_native        held  = {0};
    const d_internal_file_nchar_t* const path  =
        d_internal_file_native_of(&held,
                                  _path);
    const DWORD                          attrs =
        (path) ? D_INTERNAL_COMMON_ATTRIBUTES(path)
               : INVALID_FILE_ATTRIBUTES;

    // only a reparse point can be a link
    if ( (attrs == INVALID_FILE_ATTRIBUTES) ||
         ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == 0) )
    {
        d_internal_file_native_release(&held);

        return 0;
    }

    const HANDLE handle = d_internal_file_open_reparse_win32(path);

    d_internal_file_native_release(&held);

    // a reparse point that will not open cannot be told apart
    if (handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    const int link = d_internal_file_handle_is_link_win32(handle);

    // the link's own times and counts, for a caller describing it
    if ( (link) &&
         (_info != NULL) &&
         (!GetFileInformationByHandle(handle,
                                      _info)) )
    {
        memset(_info,
               0,
               sizeof(*_info));
    }

    CloseHandle(handle);

    return link;
}

#endif  // D_CFG_FILE_HAS_WIN32

#if (D_INTERNAL_FILE_WIDE == 1)

/*
d_internal_file_utf_errno
  The errno for a failed conversion: _too_long when the buffer was too small
-- ENAMETOOLONG for a path on its way in, ERANGE for text on its way out --
EILSEQ for text with no form in the other encoding, and the Win32 table's word
for anything else.
*/
static int
d_internal_file_utf_errno(
    DWORD _error,
    int   _too_long
)
{
    // the buffer was too small
    if (_error == ERROR_INSUFFICIENT_BUFFER)
    {
        return _too_long;
    }

    // text that is not UTF-8, or UTF-16 with a lone surrogate
    if (_error == ERROR_NO_UNICODE_TRANSLATION)
    {
        return EILSEQ;
    }

    return d_internal_file_errno_win32(_error);
}

/*
d_internal_file_to_native_text
  MultiByteToWideChar from UTF-8, told to refuse what is not UTF-8
(MB_ERR_INVALID_CHARS) -- a stray byte, an overlong form, an encoded surrogate
-- rather than put U+FFFD in its place. A capacity past what an int counts is
offered as INT_MAX characters, and one of 0, which would only ask the size
needed, is refused.
*/
const d_internal_file_nchar_t*
d_internal_file_to_native_text(
    const char*              _text,
    d_internal_file_nchar_t* _buf,
    size_t                   _count
)
{
    const int capacity = (_count > (size_t)INT_MAX) ? INT_MAX
                                                    : (int)_count;

    // the conversion, terminator included, into a buffer that has room
    if ( (capacity == 0) ||
         (MultiByteToWideChar(CP_UTF8,
                              MB_ERR_INVALID_CHARS,
                              _text,
                              -1,
                              _buf,
                              capacity) == 0) )
    {
        const int code = (capacity == 0)
                             ? ENAMETOOLONG
                             : d_internal_file_utf_errno(GetLastError(),
                                                         ENAMETOOLONG);

        // named only by errno, which a build may not report through
        D_INTERNAL_FILE_SET_ERR(code);
        (void)code;

        return NULL;
    }

    return _buf;
}

#if D_CFG_IS_ON(D_CFG_FILE_LONG_PATHS)

// D_INTERNAL_FILE_LONG_AT
//   constant: the length, in UTF-16 characters, from which a path takes the
// long form: MAX_PATH less the 12 CreateDirectoryW keeps for a name inside the
// directory it makes, the shortest limit a path call has.
#define D_INTERNAL_FILE_LONG_AT (MAX_PATH - 12)

/**
 * @brief A path in the form Windows takes past MAX_PATH: absolute, from
 *        GetFullPathNameW, behind the long-path prefix -- its UNC form for a
 *        share.
 *
 * @param[in,out] _buf    a converted path; rewritten in place where long.
 * @param[in]     _count  capacity of `_buf`, in characters.
 * @return `_buf`, or `NULL` with errno set: ENAMETOOLONG when the long form
 *         does not fit, ENOMEM, or the reason Windows gives.
 *
 * A path shorter than D_INTERNAL_FILE_LONG_AT once absolute stays as it is,
 * relative or not; so does a device path (\\.\), which the prefix would turn
 * into a file's, and one already prefixed (\\?\, or the kernel's \??\).
 * GetFullPathNameW is asked first for the size alone, so the absolute form is
 * built, on the heap, only for a path that takes it.
 */
static const d_internal_file_nchar_t*
d_internal_file_long_form(
    d_internal_file_nchar_t* _buf,
    size_t                   _count
)
{
    // a device path, or one in the long form already, goes as it is
    if ( (_buf[0] == L'\\')                         &&
         ( ( (_buf[1] == L'\\') &&
             ( (_buf[2] == L'?') ||
               (_buf[2] == L'.') ) )                ||
           ( (_buf[1] == L'?') &&
             (_buf[2] == L'?') ) )                  &&
         (_buf[3] == L'\\') )
    {
        return _buf;
    }

    const DWORD need = GetFullPathNameW(_buf,
                                        0,
                                        NULL,
                                        NULL);

    // Windows has said why
    if (need == 0)
    {
        const int code = d_internal_file_errno_win32(GetLastError());

        D_INTERNAL_FILE_SET_ERR(code);
        (void)code;

        return NULL;
    }

    // short enough, absolute, for every path call -- need counts the
    // terminator -- so it goes as it is
    if ((size_t)need <= (size_t)D_INTERNAL_FILE_LONG_AT)
    {
        return _buf;
    }

    d_internal_file_nchar_t* const full =
        (d_internal_file_nchar_t*)d_internal_file_alloc((size_t)need *
                                                        sizeof(*full));

    // d_internal_file_alloc has reported the failure
    if (!full)
    {
        return NULL;
    }

    const DWORD got   = GetFullPathNameW(_buf,
                                         need,
                                         full,
                                         NULL);
    const DWORD error = (got == 0) ? GetLastError() : 0;
    const int   unc   = ( (got >= 2)        &&
                          (full[0] == L'\\') &&
                          (full[1] == L'\\') );
    const int   dev   = ( (unc)             &&
                          (got >= 4)        &&
                          (full[2] == L'.') &&
                          (full[3] == L'\\') );
    // a share keeps one of its two backslashes, after \\?\UNC
    const d_internal_file_nchar_t* const prefix = (unc) ? L"\\\\?\\UNC"
                                                        : L"\\\\?\\";
    const size_t                         plen   = (unc) ? 7 : 4;
    const d_internal_file_nchar_t* const rest   = (unc) ? (full + 1) : full;
    const size_t                         rlen   = (unc) ? ((size_t)got - 1)
                                                        : (size_t)got;
    int                                  code   = 0;

    // the answer, a device that takes no prefix, or the reason for neither
    if ( (got == 0) ||
         (got >= need) )
    {
        code = (got == 0) ? d_internal_file_errno_win32(error) : EIO;
    }
    else if ( (!dev) &&
              (plen + rlen + 1 > _count) )
    {
        code = ENAMETOOLONG;
    }
    else if (!dev)
    {
        memcpy(_buf,
               prefix,
               plen * sizeof(*_buf));
        memcpy(_buf + plen,
               rest,
               (rlen + 1) * sizeof(*_buf));
    }

    d_internal_file_free(full);

    // named only by errno, which a build may not report through
    if (code != 0)
    {
        D_INTERNAL_FILE_SET_ERR(code);

        return NULL;
    }

    return _buf;
}

#else

/**
 * @brief A path as it is: this build takes no long form.
 *
 * @param[in] _buf    a converted path.
 * @param[in] _count  unused.
 * @return `_buf`.
 */
static const d_internal_file_nchar_t*
d_internal_file_long_form(
    d_internal_file_nchar_t* _buf,
    size_t                   _count
)
{
    (void)_count;

    return _buf;
}

#endif  // D_CFG_FILE_LONG_PATHS

/*
d_internal_file_to_native
  A path: its text (d_internal_file_to_native_text), then the long form where
the build takes long paths (d_internal_file_long_form).
*/
const d_internal_file_nchar_t*
d_internal_file_to_native(
    const char*              _text,
    d_internal_file_nchar_t* _buf,
    size_t                   _count
)
{
    // d_internal_file_to_native_text has set errno
    if (!d_internal_file_to_native_text(_text,
                                        _buf,
                                        _count))
    {
        return NULL;
    }

    return d_internal_file_long_form(_buf,
                                     _count);
}

/*
d_internal_file_from_native
  WideCharToMultiByte to UTF-8, told to refuse a lone surrogate
(WC_ERR_INVALID_CHARS) rather than put U+FFFD in its place. A failure may
leave part of the text in _out, so an empty string is put over it.
*/
int
d_internal_file_from_native(
    const d_internal_file_nchar_t* _native,
    char*                          _out,
    size_t                         _size
)
{
    const int capacity = (_size > (size_t)INT_MAX) ? INT_MAX
                                                   : (int)_size;

    // a capacity of 0 has no room even for the empty string
    if (capacity == 0)
    {
        D_INTERNAL_FILE_SET_ERR(ERANGE);

        return -1;
    }

    // the conversion, terminator included
    if (WideCharToMultiByte(CP_UTF8,
                            WC_ERR_INVALID_CHARS,
                            _native,
                            -1,
                            _out,
                            capacity,
                            NULL,
                            NULL) == 0)
    {
        const int code = d_internal_file_utf_errno(GetLastError(),
                                                   ERANGE);

        _out[0] = '\0';
        D_INTERNAL_FILE_SET_ERR(code);
        (void)code;

        return -1;
    }

    return 0;
}

#else

/*
d_internal_file_to_native_text
  The platform takes the text as it is.
*/
const d_internal_file_nchar_t*
d_internal_file_to_native_text(
    const char*              _text,
    d_internal_file_nchar_t* _buf,
    size_t                   _count
)
{
    (void)_buf;
    (void)_count;

    return _text;
}

/*
d_internal_file_to_native
  The platform takes a path as it is, too.
*/
const d_internal_file_nchar_t*
d_internal_file_to_native(
    const char*              _text,
    d_internal_file_nchar_t* _buf,
    size_t                   _count
)
{
    return d_internal_file_to_native_text(_text,
                                          _buf,
                                          _count);
}

/*
d_internal_file_from_native
  A copy of the platform's text, refused with ERANGE rather than truncated
when it does not fit.
*/
int
d_internal_file_from_native(
    const d_internal_file_nchar_t* _native,
    char*                          _out,
    size_t                         _size
)
{
    const size_t length = strlen(_native);

    // the text and its terminator must fit
    if ((length + 1) > _size)
    {
        // an empty string, where there is room for one
        if (_size > 0)
        {
            _out[0] = '\0';
        }

        D_INTERNAL_FILE_SET_ERR(ERANGE);

        return -1;
    }

    memcpy(_out,
           _native,
           length + 1);

    return 0;
}

#endif  // D_INTERNAL_FILE_WIDE == 1

#if (D_INTERNAL_FILE_NATIVE_ON_HEAP == 1)

/*
d_internal_file_native_room
  Allocated on first asking, so a holder that never converts costs nothing.
*/
d_internal_file_nchar_t*
d_internal_file_native_room(
    struct d_internal_file_native* _native
)
{
    // the block, the first time it is asked for
    if (!_native->room)
    {
        _native->room = (d_internal_file_nchar_t*)d_internal_file_alloc(
                            (size_t)D_INTERNAL_FILE_NATIVE_ROOM *
                            sizeof(*_native->room));
    }

    return _native->room;
}

/*
d_internal_file_native_release
  errno and the last error are saved around the free: a caller releases the
room between a call and the reading of why it failed.
*/
void
d_internal_file_native_release(
    struct d_internal_file_native* _native
)
{
    const int   saved_errno = errno;
    const DWORD saved_error = GetLastError();

    d_internal_file_free(_native->room);
    _native->room = NULL;
    errno         = saved_errno;
    SetLastError(saved_error);

    return;
}

#else

/*
d_internal_file_native_room
  The holder's own array.
*/
d_internal_file_nchar_t*
d_internal_file_native_room(
    struct d_internal_file_native* _native
)
{
    return _native->room;
}

/*
d_internal_file_native_release
  The array goes with the holder: nothing to return.
*/
void
d_internal_file_native_release(
    struct d_internal_file_native* _native
)
{
    (void)_native;

    return;
}

#endif  // D_INTERNAL_FILE_NATIVE_ON_HEAP == 1

#if (D_INTERNAL_FILE_PATH_ON_HEAP == 1)

/*
d_internal_file_path_room
  Allocated on first asking and cleared, as the array it stands in for was.
*/
char*
d_internal_file_path_room(
    struct d_internal_file_path* _path
)
{
    // the block, the first time it is asked for
    if (!_path->room)
    {
        _path->room = (char*)d_internal_file_alloc(
                          (size_t)D_INTERNAL_FILE_PATH_ROOM);

        // d_internal_file_alloc has reported the failure
        if (!_path->room)
        {
            D_INTERNAL_FILE_SET_ERR(ENOMEM);

            return NULL;
        }

        memset(_path->room,
               0,
               (size_t)D_INTERNAL_FILE_PATH_ROOM);
    }

    return _path->room;
}

/*
d_internal_file_path_release
  errno and the last error are saved around the free, as the native
holder's are (d_internal_file_native_release).
*/
void
d_internal_file_path_release(
    struct d_internal_file_path* _path
)
{
    const int   saved_errno = errno;
    const DWORD saved_error = GetLastError();

    d_internal_file_free(_path->room);
    _path->room = NULL;
    errno       = saved_errno;
    SetLastError(saved_error);

    return;
}

#else

/*
d_internal_file_path_room
  The holder's own array.
*/
char*
d_internal_file_path_room(
    struct d_internal_file_path* _path
)
{
    return _path->room;
}

/*
d_internal_file_path_release
  The array goes with the holder: nothing to return.
*/
void
d_internal_file_path_release(
    struct d_internal_file_path* _path
)
{
    (void)_path;

    return;
}

#endif  // D_INTERNAL_FILE_PATH_ON_HEAP == 1

/*
d_internal_file_native_of
  A failed conversion releases the room it was given, so the caller has
nothing to release on that path.
*/
const d_internal_file_nchar_t*
d_internal_file_native_of(
    struct d_internal_file_native* _native,
    const char*                    _path
)
{
    d_internal_file_nchar_t* const room = d_internal_file_native_room(_native);

    // d_internal_file_alloc has reported the failure
    if (!room)
    {
        return NULL;
    }

    const d_internal_file_nchar_t* const path =
        d_internal_file_to_native(_path,
                                  room,
                                  D_INTERNAL_FILE_NATIVE_MAX);

    // errno is the conversion's; the room goes with it
    if (!path)
    {
        d_internal_file_native_release(_native);
    }

    return path;
}
