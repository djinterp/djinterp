/******************************************************************************
* djinterp [c]                                                   file_common.c
*
* path:      /src/djinterp/c/fs/file_common.c
******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"


// Internal definitions

// d_internal_file_notify_handler
//   variable: the active notification handler, or NULL for none.
//   Set-once-at-startup is the contract: this is a plain pointer, not an
// atomic, because the alternative is to make every fs module depend on
// datomic.h to support a case (swapping the log sink from another thread
// mid-call) that no sane program needs.
#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)
    #if D_CFG_IS_ON(D_CFG_FILE_NOTIFY_DEFAULT_HANDLER)
        static fn_file_notify d_internal_file_notify_handler =
            d_file_notify_default_handler;
    #else
        static fn_file_notify d_internal_file_notify_handler = NULL;
    #endif

    // d_internal_file_notify_context
    //   variable: opaque cookie handed back to the active handler.
    static void* d_internal_file_notify_context = NULL;
#endif


// V.    Notifications

/*
d_file_notify_set_handler
  Installs the notification handler for the whole fs subframework.
  Call it once, during startup, before any thread is using an fs module.
Passing NULL removes the current handler and silences notifications without
recompiling.
  When this build compiled notifications out (D_CFG_FILE_NOTIFY == 0), the
call is accepted and does nothing, so a caller need not guard it.

Parameter(s):
  _handler: the handler to install, or NULL to remove the current one.
  _context: an opaque cookie passed back to _handler on every notice; it is
            stored, not copied, so it must outlive the handler.
Return:
  none.
*/
void
d_file_notify_set_handler
(
    fn_file_notify _handler,
    void*          _context
)
{
#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)
    d_internal_file_notify_handler = _handler;
    d_internal_file_notify_context = _context;
#else
    (void)_handler;
    (void)_context;
#endif

    return;
}


/*
d_file_notify_get_handler
  Retrieves the active notification handler, so a caller can chain onto an
existing one rather than displacing it.

Parameter(s):
  _context: receives the cookie associated with the handler; may be NULL if
            the caller does not want it.
Return:
  The active handler, or NULL if none is installed or this build compiled
notifications out.
*/
fn_file_notify
d_file_notify_get_handler
(
    void** _context
)
{
#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)
    if (_context)
    {
        *_context = d_internal_file_notify_context;
    }

    return d_internal_file_notify_handler;
#else
    if (_context)
    {
        *_context = NULL;
    }

    return NULL;
#endif
}


/*
d_file_notify_default_handler
  djinterp's built-in handler: writes one line per notice to the stream named
by D_CFG_FILE_NOTIFY_STREAM.
  It is not installed unless D_CFG_FILE_NOTIFY_DEFAULT_HANDLER is 1 -- a
library that writes to a stream nobody asked it to write to is a library that
corrupts somebody's stdout -- but it is always available to install by hand.
  errno is saved and restored, because a handler runs while the failing call's
errno is still the value the caller is about to read.

Parameter(s):
  _notice:  the record to report.
  _context: unused by this handler.
Return:
  none.
*/
void
d_file_notify_default_handler
(
    const struct d_file_notice* _notice,
    void*                       _context
)
{
    int saved_errno;

    (void)_context;

    // parameter validation
    if (!_notice)
    {
        return;
    }

    saved_errno = errno;

    fprintf(D_CFG_FILE_NOTIFY_STREAM,
            "[djinterp/fs] %s: %s%s%s%s (errno=%d)\n",
            d_file_notify_level_name(_notice->level),
            _notice->function ? _notice->function : "?",
            _notice->message ? ": " : "",
            _notice->message ? _notice->message : "",
            _notice->path ? _notice->path : "",
            _notice->error);

    errno = saved_errno;

    return;
}


/*
d_file_notify_level_name
  Maps a severity to its display name.

Parameter(s):
  _level: one of enum d_file_notify_level.
Return:
  A static, NUL-terminated name; "unknown" for a value outside the enum. Never
NULL, so a caller may pass it straight to printf.
*/
const char*
d_file_notify_level_name
(
    int _level
)
{
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
  Reports which backend this build resolved to, for diagnostics and for a
test suite that has to skip what the build cannot do.

Parameter(s):
  none.
Return:
  A static, NUL-terminated backend name: "native", "posix" or "stdc".
*/
const char*
d_file_backend_name
(
    void
)
{
#if D_FILE_BACKEND_IS_NATIVE
    return "native";
#elif D_FILE_BACKEND_IS_POSIX
    return "posix";
#else
    return "stdc";
#endif
}


/*
d_internal_file_notify_emit
  Dispatches one notice to the active handler. The fs modules reach this only
through D_INTERNAL_FILE_NOTIFY, which has already established that this build
compiles the given severity in.

Parameter(s):
  _level:    one of enum d_file_notify_level.
  _error:    errno-style code, or 0 when not applicable.
  _function: originating function name; a static literal.
  _path:     path involved, or NULL.
  _message:  short description; a static literal.
Return:
  none.
*/
void
d_internal_file_notify_emit
(
    int         _level,
    int         _error,
    const char* _function,
    const char* _path,
    const char* _message
)
{
#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)
    struct d_file_notice notice;
    fn_file_notify       handler;

    handler = d_internal_file_notify_handler;

    // no handler is the common case; do not build a record nobody reads
    if (!handler)
    {
        return;
    }

    notice.level    = _level;
    notice.error    = _error;
    notice.function = _function;
    notice.path     = _path;
    notice.message  = _message;

    handler(&notice, d_internal_file_notify_context);
#else
    (void)_level;
    (void)_error;
    (void)_function;
    (void)_path;
    (void)_message;
#endif

    return;
}


// VI.   Internal support

/*
d_internal_file_alloc
  Allocates through the configured allocator, refusing anything past the
D_CFG_FILE_MAX_ALLOC ceiling first.
  The ceiling exists because the size fed to this function usually came from
a file's own metadata: without it, d_fread_all on a sparse 40 GiB file is an
out-of-memory event in a process that only wanted to read a config file.

Parameter(s):
  _size: bytes to allocate.
Return:
  A pointer to the block on success, or NULL on failure or refusal, with
errno set to ENOMEM when this build reports through errno.
*/
void*
d_internal_file_alloc
(
    size_t _size
)
{
    void* result;

#if (D_CFG_FILE_MAX_ALLOC > 0)
    // refuse an implausible request before handing it to the allocator
    if (_size > (size_t)D_CFG_FILE_MAX_ALLOC)
    {
        D_INTERNAL_FILE_SET_ERR(ENOMEM);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ENOMEM,
                               "d_internal_file_alloc",
                               NULL,
                               "request exceeds D_CFG_FILE_MAX_ALLOC");

        return NULL;
    }
#endif

    result = D_CFG_FILE_MALLOC(_size);

    // report the shortfall; the caller only learns that it got NULL
    if (!result)
    {
        D_INTERNAL_FILE_SET_ERR(ENOMEM);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ENOMEM,
                               "d_internal_file_alloc",
                               NULL,
                               "allocator returned NULL");
    }

    return result;
}


/*
d_internal_file_realloc
  Grows or shrinks a block through the configured allocator, subject to the
same ceiling as d_internal_file_alloc.
  It keeps realloc's contract exactly, including the sharp edge: on failure
the original block is still valid and still the caller's to free. The caller
must not assign the result over its only pointer to the block.

Parameter(s):
  _ptr:  the block to resize; NULL behaves as an allocation.
  _size: the new size, in bytes.
Return:
  A pointer to the resized block on success, or NULL on failure or refusal
with the original block untouched.
*/
void*
d_internal_file_realloc
(
    void*  _ptr,
    size_t _size
)
{
    void* result;

#if (D_CFG_FILE_MAX_ALLOC > 0)
    // refuse an implausible request before handing it to the allocator
    if (_size > (size_t)D_CFG_FILE_MAX_ALLOC)
    {
        D_INTERNAL_FILE_SET_ERR(ENOMEM);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ENOMEM,
                               "d_internal_file_realloc",
                               NULL,
                               "request exceeds D_CFG_FILE_MAX_ALLOC");

        return NULL;
    }
#endif

    result = D_CFG_FILE_REALLOC(_ptr, _size);

    // report the shortfall; the caller only learns that it got NULL
    if (!result)
    {
        D_INTERNAL_FILE_SET_ERR(ENOMEM);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ENOMEM,
                               "d_internal_file_realloc",
                               NULL,
                               "allocator returned NULL");
    }

    return result;
}


/*
d_internal_file_free
  Releases a block obtained from d_internal_file_alloc.

Parameter(s):
  _ptr: the block to release; may be NULL.
Return:
  none.
*/
void
d_internal_file_free
(
    void* _ptr
)
{
    if (_ptr)
    {
        D_CFG_FILE_FREE(_ptr);
    }

    return;
}
