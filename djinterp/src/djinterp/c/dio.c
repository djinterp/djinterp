/*******************************************************************************
* djinterp [c]                                                             dio.c
*
* Definitions for the declarations in `dio.h`.
*   Each wrapper defers to the C library, choosing the Annex K / MSVC secure
* variant where D_STUDIO_HAS_SCANF_S is set and a standard fallback elsewhere.
*
*
* path:      /src/djinterp/c/dio.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.05.19
*                                                            revised: 2026.09.24
*******************************************************************************/
#include "../../../inc/djinterp/c/dio.h"  // corresponding header
// std
#include <stdarg.h>                         // va_list, va_start, va_end
#include <stdio.h>                          // vsscanf, vsnprintf, fgets, ...
#include <string.h>                         // strlen


// formatted input
/*
d_sscanf
  Collects the variadic arguments and defers to d_vsscanf.
*/
int
d_sscanf(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);
    const int result = d_vsscanf(_buffer,
                                 _format,
                                 args);
    va_end(args);

    return result;
}

/*
d_sscanf_s
  Collects the variadic arguments and calls vsscanf_s where
D_STUDIO_HAS_SCANF_S is set, vsscanf otherwise. vsscanf does not consume the
buffer-size arguments the _s convention adds after each %s, %c, and %[, so on
the fallback path such a call misreads its arguments.
*/
int
d_sscanf_s(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

#if D_STUDIO_HAS_SCANF_S
    const int result = vsscanf_s(_buffer,
                                 _format,
                                 args);
#else
    const int result = vsscanf(_buffer,
                               _format,
                               args);
#endif

    va_end(args);

    return result;
}

/*
d_vsscanf
  Delegates to vsscanf.
*/
int
d_vsscanf(
    const char* _buffer,
    const char* _format,
    va_list     _argptr
)
{
    return vsscanf(_buffer,
                   _format,
                   _argptr);
}

/*
d_vsscanf_s
  vsscanf_s where D_STUDIO_HAS_SCANF_S is set, vsscanf otherwise, with the
fallback caveat described at d_sscanf_s.
*/
int
d_vsscanf_s(
    const char* _buffer,
    const char* _format,
    va_list     _argptr
)
{
#if D_STUDIO_HAS_SCANF_S

    return vsscanf_s(_buffer,
                     _format,
                     _argptr);
#else

    return vsscanf(_buffer,
                   _format,
                   _argptr);
#endif
}

/*
d_fscanf
  Collects the variadic arguments and calls vfscanf.
*/
int
d_fscanf(
    FILE*       _stream,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const int result = vfscanf(_stream,
                               _format,
                               args);

    va_end(args);

    return result;
}

/*
d_fscanf_s
  As d_sscanf_s, over vfscanf_s and vfscanf, with the same fallback caveat.
*/
int
d_fscanf_s(
    FILE*       _stream,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

#if D_STUDIO_HAS_SCANF_S
    const int result = vfscanf_s(_stream,
                                 _format,
                                 args);
#else
    const int result = vfscanf(_stream,
                               _format,
                               args);
#endif

    va_end(args);

    return result;
}

// formatted output
/*
d_sprintf_s
  Collects the variadic arguments and defers to d_vsprintf_s.
*/
int
d_sprintf_s(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const int result = d_vsprintf_s(_buffer,
                                    _size,
                                    _format,
                                    args);

    va_end(args);

    return result;
}

/*
d_vsprintf_s
  vsprintf_s where D_STUDIO_HAS_SCANF_S is set; otherwise vsnprintf, which
truncates where vsprintf_s would report the overflow.
*/
int
d_vsprintf_s(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    va_list     _argptr
)
{
#if D_STUDIO_HAS_SCANF_S

    return vsprintf_s(_buffer,
                      _size,
                      _format,
                      _argptr);
#else

    // fallback: vsnprintf, which bounds the write to _size
    return vsnprintf(_buffer,
                     _size,
                     _format,
                     _argptr);
#endif
}

/*
d_snprintf
  Collects the variadic arguments and defers to d_vsnprintf.
*/
int
d_snprintf(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);
    const int result = d_vsnprintf(_buffer,
                                   _size,
                                   _format,
                                   args);
    va_end(args);

    return result;
}

/*
d_vsnprintf
  vsnprintf, except under MSVC before 2015, which has only _vsnprintf: that
one returns -1 and leaves the buffer unterminated when the output does not
fit.
*/
int
d_vsnprintf(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    va_list     _argptr
)
{
#if defined(D_ENV_COMPILER_MSVC) && (D_ENV_COMPILER_MAJOR < 14)

    // MSVC before 2015 has only _vsnprintf
    return _vsnprintf(_buffer,
                      _size,
                      _format,
                      _argptr);
#else

    return vsnprintf(_buffer,
                     _size,
                     _format,
                     _argptr);
#endif
}

// character and string I/O
#if D_STUDIO_HAS_SCANF_S

/*
d_gets_s
  gets_s, where D_STUDIO_HAS_SCANF_S is set.
*/
char*
d_gets_s(
    char*  _buffer,
    size_t _size
)
{
    // parameter validation
    if ( (!_buffer) ||
         (_size == 0) )
    {
        return NULL;
    }

    char* result = gets_s(_buffer,
                          _size);

    return result;
}

#else

/*
d_gets_s
  Fallback: fgets from stdin, with the trailing newline stripped. Unlike
gets_s, a line longer than the buffer is not an error: its first part is
returned and the rest stays in stdin. _size is narrowed to int for fgets.
*/
char*
d_gets_s(
    char*  _buffer,
    size_t _size
)
{
    // parameter validation
    if ( (!_buffer) ||
         (_size == 0) )
    {
        return NULL;
    }

    // safe fallback using fgets
    char* result = fgets(_buffer,
                         (int)_size,
                         stdin);

    if (result)
    {
        const size_t len = strlen(_buffer);

        // remove trailing newline if present, similar to gets behavior
        if ( (len > 0) &&
             (_buffer[len - 1] == '\n') )
        {
            _buffer[len - 1] = '\0';
        }
    }

    return result;
}

#endif

/*
d_fputs
  Delegates to fputs.
*/
int
d_fputs(
    const char* _str,
    FILE*       _stream
)
{
    return fputs(_str,
                 _stream);
}

/*
d_fgets
  Delegates to fgets.
*/
char*
d_fgets(
    char* _str,
    int   _num,
    FILE* _stream
)
{
    return fgets(_str,
                 _num,
                 _stream);
}

// stream positioning
/*
d_fgetpos
  Reads the position with d_file_tell_stream, as a 64-bit d_off_t rather than
fgetpos's opaque fpos_t.
*/
int
d_fgetpos(
    FILE*    _stream,
    d_off_t* _pos
)
{
    if (!_pos)
    {
        return -1;
    }

    const d_off_t result = d_file_tell_stream(_stream);

    if (result == -1)
    {
        return -1;
    }

    *_pos = result;

    return 0;
}

/*
d_fsetpos
  Seeks with d_file_seek_stream, measuring from the start of the stream.
*/
int
d_fsetpos(
    FILE*          _stream,
    const d_off_t* _pos
)
{
    if (!_pos)
    {
        return -1;
    }

    return d_file_seek_stream(_stream,
                              *_pos,
                              SEEK_SET);
}

// error handling
/*
d_perror
  Delegates to perror.
*/
void
d_perror(
    const char* _s
)
{
    perror(_s);

    return;
}

/*
d_feof
  Delegates to feof.
*/
int
d_feof(
    FILE* _stream
)
{
    return feof(_stream);
}

/*
d_ferror
  Delegates to ferror.
*/
int
d_ferror(
    FILE* _stream
)
{
    return ferror(_stream);
}

/*
d_clearerr
  Delegates to clearerr.
*/
void
d_clearerr(
    FILE* _stream
)
{
    clearerr(_stream);

    return;
}
