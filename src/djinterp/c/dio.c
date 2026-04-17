#include "..\..\inc\c\dio.h"


/*
d_sscanf
  Reads formatted data from a string.

Parameter(s):
  _buffer: string source to read from.
  _format: format control string.
  ...:     variable arguments for data storage.
Return:
  The number of fields successfully converted and assigned.
*/
int
d_sscanf
(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);
    result = d_vsscanf(_buffer, _format, args);
    va_end(args);

    return result;
}

/*
d_sscanf_s
  Secure variant of sscanf. Uses sscanf_s if Annex K or MSVC is available,
  otherwise falls back to standard sscanf.

Parameter(s):
  _buffer: string source to read from.
  _format: format control string.
  ...:     variable arguments (including buffer sizes for %s, %c, and %[).
Return:
  The number of fields successfully converted and assigned.
*/
int
d_sscanf_s
(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);
#if D_STUDIO_HAS_SCANF_S
#if defined(D_ENV_COMPILER_MSVC)
    result = vsscanf_s(_buffer, _format, args);
#else
    /* Standard C11 Annex K */
    result = vsscanf_s(_buffer, _format, args);
#endif
#else
    /* Fallback for legacy compilers */
    result = vsscanf(_buffer, _format, args);
#endif
    va_end(args);

    return result;
}

/*
d_vsscanf
  Reads formatted data from a string using a variable argument list.

Parameter(s):
  _buffer: string source to read from.
  _format: format control string.
  _argptr: pointer to a list of arguments.
Return:
  The number of fields successfully converted and assigned.
*/
int
d_vsscanf
(
    const char* _buffer,
    const char* _format,
    va_list     _argptr
)
{
    return vsscanf(_buffer, _format, _argptr);
}

/*
d_vsscanf_s
  Secure variant of vsscanf.

Parameter(s):
  _buffer: string source to read from.
  _format: format control string.
  _argptr: pointer to a list of arguments.
Return:
  The number of fields successfully converted and assigned.
*/
int
d_vsscanf_s
(
    const char* _buffer,
    const char* _format,
    va_list     _argptr
)
{
#if D_STUDIO_HAS_SCANF_S
    return vsscanf_s(_buffer, _format, _argptr);
#else
    return vsscanf(_buffer, _format, _argptr);
#endif
}

/*
d_fscanf
  Reads formatted data from a stream.

Parameter(s):
  _stream: pointer to a FILE object.
  _format: format control string.
  ...:     variable arguments for data storage.
Return:
  The number of fields successfully converted and assigned.
*/
int
d_fscanf
(
    FILE*       _stream,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);

    result = vfscanf(_stream, _format, args);

    va_end(args);

    return result;
}

/*
d_fscanf_s
  Secure variant of fscanf.

Parameter(s):
  _stream: pointer to a FILE object.
  _format: format control string.
  ...:     variable arguments (including buffer sizes for %s, %c, and %[).
Return:
  An integer value corresponding to the number of fields successfully converted
and assigned.
*/
int
d_fscanf_s
(
    FILE*       _stream,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);

#if D_STUDIO_HAS_SCANF_S
    result = vfscanf_s(_stream, _format, args);
#else
    result = vfscanf(_stream, _format, args);
#endif

    va_end(args);

    return result;
}

/*
d_sprintf_s
  Secure variant of sprintf. Writes formatted data to a string buffer with
  bounds checking.

Parameter(s):
  _buffer: pointer to the destination string buffer.
  _size:   maximum number of characters to write (including null terminator).
  _format: format control string.
  ...:     variable arguments for formatting.
Return:
  The number of characters written (excluding null), or a negative value on 
  failure.
*/
int
d_sprintf_s
(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);

    result = d_vsprintf_s(_buffer, _size, _format, args);

    va_end(args);

    return result;
}

/*
d_vsprintf_s
  Secure variant of vsprintf. Uses vsprintf_s if Annex K or MSVC is available,
  otherwise falls back to d_vsnprintf for safety.

Parameter(s):
  _buffer: pointer to the destination string buffer.
  _size:   maximum number of characters to write (including null terminator).
  _format: format control string.
  _argptr: pointer to a list of arguments.
Return:
  The number of characters written (excluding null), or a negative value on 
  failure.
*/
int
d_vsprintf_s
(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    va_list     _argptr
)
{
#if D_STUDIO_HAS_SCANF_S
    return vsprintf_s(_buffer, _size, _format, _argptr);
#else
    /* Fallback to vsnprintf as a safe alternative to prevent buffer overflow */
    return vsnprintf(_buffer, _size, _format, _argptr);
#endif
}

/*
d_snprintf
  Writes formatted data to a string buffer with a specified size limit.

Parameter(s):
  _buffer: pointer to the destination string buffer.
  _size:   maximum number of characters to write (including null terminator).
  _format: format control string.
  ...:     variable arguments for formatting.
Return:
  The number of characters that would have been written if _size was large 
  enough.
*/
int
d_snprintf
(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);
    result = d_vsnprintf(_buffer, _size, _format, args);
    va_end(args);

    return result;
}

/*
d_vsnprintf
  Writes formatted data to a string buffer with a size limit using a variable
  argument list.

Parameter(s):
  _buffer: pointer to the destination string buffer.
  _size:   maximum number of characters to write (including null terminator).
  _format: format control string.
  _argptr: pointer to a list of arguments.
Return:
  The number of characters that would have been written if _size was large 
  enough.
*/
int
d_vsnprintf
(
    char* _buffer,
    size_t      _size,
    const char* _format,
    va_list     _argptr
)
{
#if defined(D_ENV_COMPILER_MSVC) && (D_ENV_COMPILER_MAJOR < 14)
    /* Legacy MSVC versions use _vsnprintf */
    return _vsnprintf(_buffer, _size, _format, _argptr);
#else
    return vsnprintf(_buffer, _size, _format, _argptr);
#endif
}

/*
d_gets_s
  Securely reads a line from stdin into a buffer. Enforces the buffer size to
  prevent overflow.

Parameter(s):
  _buffer: pointer to the destination string buffer.
  _size:   size of the buffer.
Return:
  A pointer to the buffer on success, or NULL on error or EOF.
*/
char*
d_gets_s
(
    char* _buffer,
    size_t _size
)
{
    char* result;

    // parameter validation
    if ( (!_buffer) || 
         (_size == 0) )
    {
        return NULL;
    }

#if D_STUDIO_HAS_SCANF_S
    result = gets_s(_buffer, _size);
#else
    // safe fallback using fgets
    result = fgets(_buffer, (int)_size, stdin);

    if (result)
    {
        size_t len;
        len = strlen(_buffer);

        // remove trailing newline if present, similar to gets behavior
        if ( (len > 0) && 
             (_buffer[len - 1] == '\n') )
        {
            _buffer[len - 1] = '\0';
        }
    }
#endif

    return result;
}

/*
d_fputs
  Writes a string to the specified stream.

Parameter(s):
  _str:    null-terminated string to write.
  _stream: pointer to a FILE object.
Return:
  A non-negative value on success, or EOF on error.
*/
int
d_fputs
(
    const char* _str,
    FILE*       _stream
)
{
    return fputs(_str, _stream);
}

/*
d_fgets
  Reads a line from a stream into a buffer, stopping at a newline or when
  the buffer is full.

Parameter(s):
  _str:    pointer to the destination buffer.
  _num:    maximum number of characters to read (including null).
  _stream: pointer to a FILE object.
Return:
  A pointer to the buffer on success, or NULL on error or EOF.
*/
char*
d_fgets
(
    char* _str,
    int   _num,
    FILE* _stream
)
{
    return fgets(_str, _num, _stream);
}

/*
d_fgetpos
  Gets the current file position of the stream.

Parameter(s):
  _stream: pointer to a FILE object.
  _pos:    pointer to a d_off_t to store the current position.
Return:
  Zero on success, non-zero on failure.
*/
int
d_fgetpos
(
    FILE*    _stream,
    d_off_t* _pos
)
{
    d_off_t result;

    if (!_pos)
    {
        return -1;
    }

    result = d_ftello(_stream);

    if (result == -1)
    {
        return -1;
    }

    *_pos = result;

    return 0;
}

/*
d_fsetpos
  Sets the current file position of the stream.

Parameter(s):
  _stream: pointer to a FILE object.
  _pos:    pointer to a d_off_t containing the position to set.
Return:
  Zero on success, non-zero on failure.
*/
int
d_fsetpos
(
    FILE*          _stream,
    const d_off_t* _pos
)
{
    if (!_pos)
    {
        return -1;
    }

    return d_fseeko(_stream, *_pos, SEEK_SET);
}

/*
d_rewind
  Sets the file position to the beginning of the file and clears error 
  indicators.

Parameter(s):
  _stream: pointer to a FILE object.
Return:
  none.
*/
void
d_rewind
(
    FILE* _stream
)
{
    rewind(_stream);

    return;
}

/*
d_perror
  Prints a system error message to stderr.

Parameter(s):
  _s: string prefix to the error message.
Return:
  none.
*/
void
d_perror
(
    const char* _s
)
{
    perror(_s);

    return;
}

/*
d_feof
  Tests the end-of-file indicator for the given stream.

Parameter(s):
  _stream: pointer to a FILE object.
Return:
  Non-zero if the end-of-file indicator is set, zero otherwise.
*/
int
d_feof
(
    FILE* _stream
)
{
    return feof(_stream);
}

/*
d_ferror
  Tests the error indicator for the given stream.

Parameter(s):
  _stream: pointer to a FILE object.
Return:
  Non-zero if the error indicator is set, zero otherwise.
*/
int
d_ferror
(
    FILE* _stream
)
{
    return ferror(_stream);
}

/*
d_clearerr
  Resets the error and end-of-file indicators for the stream.

Parameter(s):
  _stream: pointer to a FILE object.
Return:
  none.
*/
void
d_clearerr
(
    FILE* _stream
)
{
    clearerr(_stream);

    return;
}

