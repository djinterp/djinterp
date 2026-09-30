/*******************************************************************************
* djinterp [c]                                                     file_common.c
*
* Implementation of the notification hook and the allocation funnel declared
* in file_common.h.
*   A notice is assembled only after a handler is known to exist, so a build
* that compiles notifications in but installs no handler pays one load and one
* branch per notice. Every fs allocation passes the D_CFG_FILE_MAX_ALLOC
* ceiling here before it reaches the configured allocator.
*
*
* path:      /src/djinterp/c/fs/file_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_common.h"  // corresponding header
// std
#include <errno.h>   // errno, ENOMEM
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // fprintf, and stderr for the default stream
#include <stdlib.h>  // malloc, realloc, free: the allocator defaults
// djinterp
#include "../../../../inc/djinterp/config/c/fs/cfg_file_common.h"  // D_CFG_FILE_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)
    // d_internal_file_notify_handler
    //   variable: the active notification handler, or NULL for none.
    //   Set-once-at-startup is the contract: this is a plain pointer, not an
    // atomic, because the alternative is to make every fs module depend on
    // datomic.h to support a case (swapping the log sink from another thread
    // mid-call) that no sane program needs.
    #if D_CFG_IS_ON(D_CFG_FILE_NOTIFY_DEFAULT_HANDLER)
        static fn_file_notify d_internal_file_notify_handler =
            d_file_notify_default_handler;
    #else
        static fn_file_notify d_internal_file_notify_handler = NULL;
    #endif

    // d_internal_file_notify_context
    //   variable: opaque cookie handed back to the active handler.
    static void* d_internal_file_notify_context = NULL;
#endif  // D_INTERNAL_FILE_NOTIFY_LEVEL > 0

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
  With notifications compiled out there is no handler to report, so the
cookie is cleared and NULL returned -- the same answer as "none installed".
*/
fn_file_notify
d_file_notify_get_handler(
    void** _context
)
{
#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)
    // the cookie is optional
    if (_context)
    {
        *_context = d_internal_file_notify_context;
    }

    return d_internal_file_notify_handler;
#else
    // the cookie is optional
    if (_context)
    {
        *_context = NULL;
    }

    return NULL;
#endif
}

/*
d_file_notify_default_handler
  errno is saved and restored around the write, because a handler runs while
the failing call's errno is still the value its caller is about to read. It
is never installed unasked -- a library that writes to a stream nobody asked
it to write to corrupts somebody's stdout.
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

    const int saved_errno = errno;

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
#if D_FILE_BACKEND_IS_NATIVE
    return "native";
#elif D_FILE_BACKEND_IS_POSIX
    return "posix";
#else
    return "stdc";
#endif
}

//==============================================================================
// 6.  INTERNAL SUPPORT
//==============================================================================

#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)

/*
d_internal_file_notify_emit
  The record is built on the stack only after a handler is known to exist.
With none installed, which is the common case, a notice costs one load and one
branch. D_INTERNAL_FILE_NOTIFY has already applied the severity ceiling.
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
    const fn_file_notify handler = d_internal_file_notify_handler;

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

#else

/*
d_internal_file_notify_emit
  Notifications are compiled out, so D_INTERNAL_FILE_NOTIFY never calls this.
It is defined anyway so that the symbol the header declares always links.
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
    (void)_level;
    (void)_error;
    (void)_function;
    (void)_path;
    (void)_message;

    return;
}

#endif  // D_INTERNAL_FILE_NOTIFY_LEVEL > 0

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

    void* const result = D_CFG_FILE_MALLOC(_size);

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

    void* const result = D_CFG_FILE_REALLOC(_ptr,
                                            _size);

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
