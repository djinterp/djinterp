/*******************************************************************************
* djinterp [c]                                                     file_common.c
*
* Implementation of the notification hook, the allocation funnel and the
* Win32 error translation declared in file_common.h.
*   A notice is assembled only after a handler is known to exist, so a build
* that compiles notifications in but installs no handler pays one load and one
* branch per notice. Every filesys allocation passes the D_CFG_FILE_MAX_ALLOC
* ceiling here before it reaches the configured allocator. The Win32 reasons
* a caller can act on are a table, and every other reason reads as EIO.
*
*
* path:      /src/djinterp/c/filesys/file_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // corresponding header
// std
#include <errno.h>   // errno, ENOMEM and, for Win32's errors, the E* codes
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // fprintf, and stderr for the default stream
#include <stdlib.h>  // malloc, realloc, free: the allocator defaults
#include <string.h>  // memset
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
    { ERROR_OUTOFMEMORY,           ENOMEM       }
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
d_internal_file_is_link_win32
  The attributes first, since most paths are no reparse point at all; then the
reparse point itself, opened without following it (FILE_FLAG_OPEN_REPARSE_POINT)
and asked for its tag (d_internal_file_handle_is_link_win32). A path that will
not open is not reported as a link: the caller's own call then meets the
reason and reports it. A link whose information cannot be read is still a
link, with that information zeroed.
*/
int
d_internal_file_is_link_win32(
    const char*                 _path,
    BY_HANDLE_FILE_INFORMATION* _info
)
{
    const DWORD attrs = GetFileAttributesA(_path);

    // only a reparse point can be a link
    if ( (attrs == INVALID_FILE_ATTRIBUTES) ||
         ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == 0) )
    {
        return 0;
    }

    const HANDLE handle = CreateFileA(_path,
                                      FILE_READ_ATTRIBUTES,
                                      FILE_SHARE_READ  |
                                      FILE_SHARE_WRITE |
                                      FILE_SHARE_DELETE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT |
                                      FILE_FLAG_BACKUP_SEMANTICS,
                                      NULL);

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
