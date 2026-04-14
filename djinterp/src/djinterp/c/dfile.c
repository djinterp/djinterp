#include "..\..\inc\c\dfile.h"


// suppress MSVC security warnings - this library provides its own safe wrappers
#if ( (defined(_MSC_VER)) &&  \
      (!defined(_CRT_SECURE_NO_WARNINGS)) )
    #define _CRT_SECURE_NO_WARNINGS 1
#endif

///////////////////////////////////////////////////////////////////////////////
///             INTERNAL DEFINITIONS                                        ///
///////////////////////////////////////////////////////////////////////////////

// internal directory handle structure
struct d_dir_t
{
#if defined(D_FILE_PLATFORM_WINDOWS)
    HANDLE           handle;
    WIN32_FIND_DATAA find_data;
    int              first_read;
    char             path[D_FILE_PATH_MAX];
#else
    DIR* handle;
#endif
    struct d_dirent_t entry;
};


// D_INTERNAL_FILE_COPY_BUF_SIZE
//   constant: buffer size for file copy operations.
#define D_INTERNAL_FILE_COPY_BUF_SIZE 65536


///////////////////////////////////////////////////////////////////////////////
///             III.  SECURE FILE OPENING                                   ///
///////////////////////////////////////////////////////////////////////////////

/*
d_fopen
  Portable file open with consistent behavior across platforms.

Parameter(s):
  _filename: path to file.
  _mode:     file open mode ("r", "w", "a", etc.).
Return:
  File pointer on success, NULL on failure.
*/
FILE*
d_fopen
(
    const char* _filename, 
    const char* _mode
)
{
    FILE* result;

    // parameter validation
    if ( (!_filename) || 
         (!_mode) )
    {
        return NULL;
    }

#if D_FILE_HAS_FOPEN_S
    result = NULL;
    if (fopen_s(&result, _filename, _mode) != 0)
    {
        return NULL;
    }

    return result;
#else
    return fopen(_filename, _mode);
#endif
}


/*
d_fopen_s
  Secure file open (C11 fopen_s equivalent).

Parameter(s):
  _stream:   pointer to receive the file pointer.
  _filename: path to file.
  _mode:     file open mode.
Return:
  0 on success, non-zero error code on failure.
*/
int
d_fopen_s
(
    FILE**      _stream, 
    const char* _filename, 
    const char* _mode
)
{
    // parameter validation
    if (!_stream)
    {
        return EINVAL;
    }

    *_stream = NULL;

    if ( (!_filename) || 
         (!_mode) )
    {
        return EINVAL;
    }

#if D_FILE_HAS_FOPEN_S
    return fopen_s(_stream, _filename, _mode);
#else
    *_stream = fopen(_filename, _mode);
    if (!*_stream)
    {
        return errno ? errno : ENOENT;
    }

    return 0;
#endif
}


/*
d_freopen
  Portable file reopen.

Parameter(s):
  _filename: path to file (or NULL to change mode only).
  _mode:     new file open mode.
  _stream:   existing file stream to reopen.
Return:
  File pointer on success, NULL on failure.
*/
FILE*
d_freopen
(
    const char* _filename, 
    const char* _mode, 
    FILE*       _stream
)
{
    // parameter validation
    if ( (!_mode) || 
         (!_stream) )
    {
        return NULL;
    }

#if D_FILE_HAS_FOPEN_S && defined(D_FILE_PLATFORM_WINDOWS)
    {
        FILE* result = NULL;

        if (freopen_s(&result, _filename, _mode, _stream) != 0)
        {
            return NULL;
        }

        return result;
    }
#else
    return freopen(_filename, _mode, _stream);
#endif
}


/*
d_freopen_s
  Secure file reopen (C11 freopen_s equivalent).

Parameter(s):
  _newstream: pointer to receive the new file pointer.
  _filename:  path to file (or NULL to change mode only).
  _mode:      new file open mode.
  _stream:    existing file stream to reopen.
Return:
  0 on success, non-zero error code on failure.
*/
int
d_freopen_s
(
    FILE**      _newstream, 
    const char* _filename, 
    const char* _mode, 
    FILE*       _stream
)
{
    // parameter validation
    if (!_newstream)
    {
        return EINVAL;
    }

    *_newstream = NULL;

    if ( (!_mode) || 
         (!_stream) )
    {
        return EINVAL;
    }

#if D_FILE_HAS_FOPEN_S && defined(D_FILE_PLATFORM_WINDOWS)
    return freopen_s(_newstream, _filename, _mode, _stream);
#else
    *_newstream = freopen(_filename, _mode, _stream);
    if (!*_newstream)
    {
        return errno ? errno : ENOENT;
    }

    return 0;
#endif
}


/*
d_fdopen
  Associate stream with file descriptor.

Parameter(s):
  _fd:   file descriptor.
  _mode: file open mode.
Return:
  File pointer on success, NULL on failure.
*/
FILE*
d_fdopen
(
    int         _fd, 
    const char* _mode
)
{
    // parameter validation
    if ( (_fd < 0) || 
         (!_mode) )
    {
        return NULL;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _fdopen(_fd, _mode);
#else
    return fdopen(_fd, _mode);
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   LARGE FILE SUPPORT                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
d_fseeko
  Seek with 64-bit offset support.

Parameter(s):
  _stream: file stream.
  _offset: byte offset.
  _whence: seek origin (SEEK_SET, SEEK_CUR, or SEEK_END).
Return:
  0 on success, -1 on failure.
*/
int
d_fseeko
(
    FILE*   _stream, 
    d_off_t _offset, 
    int     _whence
)
{
    // parameter validation
    if (!_stream)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _fseeki64(_stream, _offset, _whence);
#elif D_FILE_HAS_FSEEKO
    return fseeko(_stream, _offset, _whence);
#else
    // fallback to fseek (limited to 2GB)
    if ( (_offset > LONG_MAX) || 
         (_offset < LONG_MIN) )
    {
        errno = EOVERFLOW;

        return -1;
    }

    return fseek(_stream, (long)_offset, _whence);
#endif
}


/*
d_ftello
  Tell with 64-bit offset support.

Parameter(s):
  _stream: file stream.
Return:
  Current file position, or -1 on failure.
*/
d_off_t
d_ftello
(
    FILE* _stream
)
{
    // parameter validation
    if (!_stream)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _ftelli64(_stream);
#elif D_FILE_HAS_FSEEKO
    return ftello(_stream);
#else
    return (d_off_t)ftell(_stream);
#endif
}


/*
d_ftruncate
  Truncate file to specified length.

Parameter(s):
  _fd:     file descriptor.
  _length: new file length in bytes.
Return:
  0 on success, -1 on failure.
*/
int
d_ftruncate
(
    int     _fd, 
    d_off_t _length
)
{
    // parameter validation
    if (_fd < 0)
    {
        errno = EBADF;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _chsize_s(_fd, _length);
#else
    return ftruncate(_fd, _length);
#endif
}


/*
d_ftruncate_stream
  Truncate file stream to specified length.

Parameter(s):
  _stream: file stream.
  _length: new file length in bytes.
Return:
  0 on success, -1 on failure.
*/
int
d_ftruncate_stream
(
    FILE*   _stream, 
    d_off_t _length
)
{
    int fd;

    // parameter validation
    if (!_stream)
    {
        errno = EINVAL;

        return -1;
    }

    // flush before truncating
    d_fflush(_stream);

    fd = d_fileno(_stream);
    if (fd < 0)
    {
        return -1;
    }

    return d_ftruncate(fd, _length);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    FILE DESCRIPTOR OPERATIONS                            ///
///////////////////////////////////////////////////////////////////////////////

/*
d_fileno
  Get file descriptor from stream.

Parameter(s):
  _stream: file stream.
Return:
  File descriptor, or -1 on failure.
*/
int
d_fileno
(
    FILE* _stream
)
{
    // parameter validation
    if (!_stream)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _fileno(_stream);
#else
    return fileno(_stream);
#endif
}


/*
d_dup
  Duplicate file descriptor.

Parameter(s):
  _fd: file descriptor to duplicate.
Return:
  New file descriptor, or -1 on failure.
*/
int
d_dup
(
    int _fd
)
{
    if (_fd < 0)
    {
        errno = EBADF;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _dup(_fd);
#else
    return dup(_fd);
#endif
}


/*
d_dup2
  Duplicate file descriptor to specific number.

Parameter(s):
  _fd:  source file descriptor.
  _fd2: target file descriptor number.
Return:
  _fd2 on success, or -1 on failure.
*/
int
d_dup2
(
    int _fd, 
    int _fd2
)
{
    if ( (_fd < 0) || 
         (_fd2 < 0) )
    {
        errno = EBADF;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    // Windows _dup2 returns 0 on success, not the fd
    if (_dup2(_fd, _fd2) == 0)
    {
        return _fd2;
    }

    return -1;
#else
    return dup2(_fd, _fd2);
#endif
}


/*
d_close
  Close file descriptor.

Parameter(s):
  _fd: file descriptor to close.
Return:
  0 on success, -1 on failure.
*/
int
d_close
(
    int _fd
)
{
    if (_fd < 0)
    {
        errno = EBADF;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _close(_fd);
#else
    return close(_fd);
#endif
}


/*
d_read
  Read from file descriptor.

Parameter(s):
  _fd:    file descriptor.
  _buf:   buffer to read into.
  _count: maximum bytes to read.
Return:
  Number of bytes read, 0 at EOF, or -1 on error.
*/
ssize_t
d_read
(
    int    _fd, 
    void*  _buf, 
    size_t _count
)
{
    // parameter validation
    if ( (_fd < 0) || 
         (!_buf) )
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    // Windows _read takes unsigned int
    if (_count > UINT_MAX)
    {
        _count = UINT_MAX;
    }

    return _read(_fd, _buf, (unsigned int)_count);
#else
    return read(_fd, _buf, _count);
#endif
}


/*
d_write
  Write to file descriptor.

Parameter(s):
  _fd:    file descriptor.
  _buf:   buffer to write from.
  _count: bytes to write.
Return:
  Number of bytes written, or -1 on error.
*/
ssize_t
d_write
(
    int         _fd, 
    const void* _buf, 
    size_t      _count
)
{
    // parameter validation
    if ( (_fd < 0) || 
         (!_buf) )
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    if (_count > UINT_MAX)
    {
        _count = UINT_MAX;
    }

    return _write(_fd, _buf, (unsigned int)_count);
#else
    return write(_fd, _buf, _count);
#endif
}


/*
d_open
  Open file and return file descriptor.

Parameter(s):
  _path:  path to file.
  _flags: open flags (O_RDONLY, O_WRONLY, O_RDWR, etc.).
  ...:    optional mode for file creation.
Return:
  File descriptor on success, -1 on failure.
*/
int
d_open
(
    const char* _path, 
    int         _flags, 
    ...
)
{
    int     mode;
    va_list args;

    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

    mode = 0;

    // check if mode argument is provided (for O_CREAT)
    if (_flags & O_CREAT)
    {
        va_start(args, _flags);
        mode = va_arg(args, int);
        va_end(args);
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        int fd;

        if (_sopen_s(&fd, _path, _flags, _SH_DENYNO, mode) != 0)
        {
            return -1;
        }

        return fd;
    }
#else
    return open(_path, _flags, mode);
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   FILE SYNCHRONIZATION                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_fsync
  Synchronize file to disk.

Parameter(s):
  _fd: file descriptor.
Return:
  0 on success, -1 on failure.
*/
int
d_fsync
(
    int _fd
)
{
    if (_fd < 0)
    {
        errno = EBADF;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _commit(_fd);
#else
    return fsync(_fd);
#endif
}


/*
d_fsync_stream
  Flush and synchronize stream to disk.

Parameter(s):
  _stream: file stream.
Return:
  0 on success, -1 on failure.
*/
int
d_fsync_stream
(
    FILE* _stream
)
{
    int fd;

    // parameter validation
    if (!_stream)
    {
        errno = EINVAL;

        return -1;
    }

    // flush user-space buffers first
    if (fflush(_stream) != 0)
    {
        return -1;
    }

    fd = d_fileno(_stream);
    if (fd < 0)
    {
        return -1;
    }

    return d_fsync(fd);
}


/*
d_fflush
  Portable fflush wrapper.

Parameter(s):
  _stream: file stream (NULL flushes all streams).
Return:
  0 on success, EOF on failure.
*/
int
d_fflush
(
    FILE* _stream
)
{
    return fflush(_stream);
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  FILE LOCKING                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
d_flock
  Apply advisory lock to file descriptor.

Parameter(s):
  _fd:        file descriptor.
  _operation: lock operation (D_LOCK_SH, D_LOCK_EX, D_LOCK_UN, optionally
              OR'd with D_LOCK_NB).
Return:
  0 on success, -1 on failure.
*/
int
d_flock
(
    int _fd, 
    int _operation
)
{
    if (_fd < 0)
    {
        errno = EBADF;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        HANDLE     h;
        DWORD      flags;
        DWORD      low;
        DWORD      high;
        BOOL       result;
        OVERLAPPED ov;

        h = (HANDLE)_get_osfhandle(_fd);
        if (h == INVALID_HANDLE_VALUE)
        {
            errno = EBADF;

            return -1;
        }

        flags = 0;
        low   = 0xFFFFFFFF;
        high  = 0xFFFFFFFF;

        // initialize OVERLAPPED structure (offset 0 = lock entire file)
        d_memset(&ov, 0, sizeof(ov));

        if (_operation & D_LOCK_UN)
        {
            // unlock
            result = UnlockFileEx(h, 0, low, high, &ov);
        }
        else
        {
            if (_operation & D_LOCK_EX)
            {
                flags |= LOCKFILE_EXCLUSIVE_LOCK;
            }

            if (_operation & D_LOCK_NB)
            {
                flags |= LOCKFILE_FAIL_IMMEDIATELY;
            }

            result = LockFileEx(h, flags, 0, low, high, &ov);
        }

        if (!result)
        {
            errno = EWOULDBLOCK;

            return -1;
        }

        return 0;
    }
#else
    {
        int op;

        if (_operation & D_LOCK_UN)
        {
            op = LOCK_UN;
        }
        else if (_operation & D_LOCK_EX)
        {
            op = LOCK_EX;
        }
        else
        {
            op = LOCK_SH;
        }

        if (_operation & D_LOCK_NB)
        {
            op |= LOCK_NB;
        }

        return flock(_fd, op);
    }
#endif
}


/*
d_flock_stream
  Apply advisory lock to file stream.

Parameter(s):
  _stream:    file stream.
  _operation: lock operation.
Return:
  0 on success, -1 on failure.
*/
int
d_flock_stream
(
    FILE* _stream, 
    int   _operation
)
{
    int fd;

    // parameter validation
    if (!_stream)
    {
        errno = EINVAL;

        return -1;
    }

    fd = d_fileno(_stream);
    if (fd < 0)
    {
        return -1;
    }

    return d_flock(fd, _operation);
}


///////////////////////////////////////////////////////////////////////////////
///             VIII. TEMPORARY FILES                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tmpfile
  Create temporary file.

Parameter(s):
  (none)
Return:
  File pointer on success, NULL on failure.
*/
FILE*
d_tmpfile
(
    void
)
{
#if defined(D_FILE_PLATFORM_WINDOWS) && D_FILE_HAS_FOPEN_S
    {
        FILE* result = NULL;

        if (tmpfile_s(&result) != 0)
        {
            return NULL;
        }

        return result;
    }
#else
    return tmpfile();
#endif
}


/*
d_tmpfile_s
  Secure temporary file creation.

Parameter(s):
  _stream: pointer to receive the file pointer.
Return:
  0 on success, non-zero error code on failure.
*/
int
d_tmpfile_s
(
    FILE** _stream
)
{
    // parameter validation
    if (!_stream)
    {
        return EINVAL;
    }

    *_stream = NULL;

#if defined(D_FILE_PLATFORM_WINDOWS) && defined(__STDC_SECURE_LIB__)
    return tmpfile_s(_stream);
#else
    *_stream = tmpfile();
    if (!*_stream)
    {
        return errno ? errno : ENOENT;
    }

    return 0;
#endif
}


/*
d_mkstemp
  Create temporary file with template.

Parameter(s):
  _template: path template ending with "XXXXXX" (modified in place).
Return:
  File descriptor on success, -1 on failure.
*/
int
d_mkstemp
(
    char* _template
)
{
    // parameter validation
    if (!_template)
    {
        errno = EINVAL;

        return -1;
    }

#if D_FILE_HAS_MKSTEMP
    return mkstemp(_template);
#elif defined(D_FILE_PLATFORM_WINDOWS)
    {
        // Windows implementation using _mktemp_s and _sopen_s
        size_t len;
        int    fd;

        len = strlen(_template) + 1;

        if (_mktemp_s(_template, len) != 0)
        {
            return -1;
        }

        if (_sopen_s(&fd,
                     _template, 
                     _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY,
                     _SH_DENYNO,
                     _S_IREAD | _S_IWRITE) != 0)
        {
            return -1;
        }

        return fd;
    }
#else
    // minimal fallback (not cryptographically secure)
    {
        char*  p;
        size_t len;
        int    i;
        int    fd;

        len = strlen(_template);
        if (len < 6)
        {
            errno = EINVAL;

            return -1;
        }

        p = _template + len - 6;

        // replace XXXXXX with random characters
        for (i = 0; i < 6; i++)
        {
            if (p[i] != 'X')
            {
                errno = EINVAL;

                return -1;
            }

            p[i] = 'a' + (rand() % 26);
        }

        fd = open(_template, O_CREAT | O_EXCL | O_RDWR, 0600);

        return fd;
    }
#endif
}


/*
d_tmpnam_s
  Generate temporary filename.

Parameter(s):
  _s:       buffer to receive the filename.
  _maxsize: size of buffer.
Return:
  0 on success, non-zero error code on failure.
*/
int
d_tmpnam_s
(
    char*  _s, 
    size_t _maxsize
)
{
    // parameter validation
    if ( (!_s) || 
         (_maxsize == 0) )
    {
        return EINVAL;
    }

#if defined(D_FILE_PLATFORM_WINDOWS) && defined(__STDC_SECURE_LIB__)
    return tmpnam_s(_s, _maxsize);
#else
    {
        char* result;

        result = tmpnam(NULL);
        if (!result)
        {
            return errno ? errno : ENOENT;
        }

        if (strlen(result) >= _maxsize)
        {
            return ERANGE;
        }

        d_strcpy_s(_s, _maxsize, result);

        return 0;
    }
#endif
}


/*
d_tempdir
  Get system temporary directory.

Parameter(s):
  _buf:     buffer to receive the path.
  _bufsize: size of buffer.
Return:
  _buf on success, NULL on failure.
*/
char*
d_tempdir
(
    char*  _buf, 
    size_t _bufsize
)
{
    // parameter validation
    if ( (!_buf) || 
         (_bufsize == 0) )
    {
        return NULL;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        DWORD len;

        len = GetTempPathA((DWORD)_bufsize, _buf);
        if ( (len == 0) || 
             (len >= _bufsize) )
        {
            return NULL;
        }

        return _buf;
    }
#else
    {
        const char* tmpdir;

        // check environment variables
        tmpdir = getenv("TMPDIR");
        if (!tmpdir)
        {
            tmpdir = getenv("TMP");
        }

        if (!tmpdir)
        {
            tmpdir = getenv("TEMP");
        }

        if (!tmpdir)
        {
            tmpdir = "/tmp";
        }

        if (strlen(tmpdir) >= _bufsize)
        {
            return NULL;
        }

        d_strcpy_s(_buf, _bufsize, tmpdir);

        return _buf;
    }
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             IX.   FILE METADATA                                         ///
///////////////////////////////////////////////////////////////////////////////

/*
d_stat
  Get file status.

Parameter(s):
  _path: path to file.
  _buf:  pointer to d_stat_t to receive status.
Return:
  0 on success, -1 on failure.
*/
int
d_stat
(
    const char*      _path, 
    struct d_stat_t* _buf
)
{
    // parameter validation
    if ( (!_path) || 
         (!_buf) )
    {
        errno = EINVAL;

        return -1;
    }

    d_memset(_buf, 0, sizeof(struct d_stat_t));

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        struct __stat64 st;

        if (_stat64(_path, &st) != 0)
        {
            return -1;
        }

        _buf->st_size  = (uint64_t)st.st_size;
        _buf->st_mtime = (uint64_t)st.st_mtime;
        _buf->st_atime = (uint64_t)st.st_atime;
        _buf->st_ctime = (uint64_t)st.st_ctime;
        _buf->st_mode  = (uint32_t)st.st_mode;
        _buf->st_nlink = (uint32_t)st.st_nlink;
        _buf->st_uid   = (uint32_t)st.st_uid;
        _buf->st_gid   = (uint32_t)st.st_gid;
        _buf->st_dev   = (uint64_t)st.st_dev;
        _buf->st_ino   = (uint64_t)st.st_ino;

        return 0;
    }
#else
    {
        struct stat st;

        if (stat(_path, &st) != 0)
        {
            return -1;
        }

        _buf->st_size  = (uint64_t)st.st_size;
        _buf->st_mtime = (uint64_t)st.st_mtime;
        _buf->st_atime = (uint64_t)st.st_atime;
        _buf->st_ctime = (uint64_t)st.st_ctime;
        _buf->st_mode  = (uint32_t)st.st_mode;
        _buf->st_nlink = (uint32_t)st.st_nlink;
        _buf->st_uid   = (uint32_t)st.st_uid;
        _buf->st_gid   = (uint32_t)st.st_gid;
        _buf->st_dev   = (uint64_t)st.st_dev;
        _buf->st_ino   = (uint64_t)st.st_ino;

        return 0;
    }
#endif
}


/*
d_fstat
  Get file status from file descriptor.

Parameter(s):
  _fd:  file descriptor.
  _buf: pointer to d_stat_t to receive status.
Return:
  0 on success, -1 on failure.
*/
int
d_fstat
(
    int              _fd, 
    struct d_stat_t* _buf
)
{
    // parameter validation
    if ( (_fd < 0) || 
         (!_buf) )
    {
        errno = EINVAL;

        return -1;
    }

    d_memset(_buf, 0, sizeof(struct d_stat_t));

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        struct __stat64 st;

        if (_fstat64(_fd, &st) != 0)
        {
            return -1;
        }

        _buf->st_size  = (uint64_t)st.st_size;
        _buf->st_mtime = (uint64_t)st.st_mtime;
        _buf->st_atime = (uint64_t)st.st_atime;
        _buf->st_ctime = (uint64_t)st.st_ctime;
        _buf->st_mode  = (uint32_t)st.st_mode;
        _buf->st_nlink = (uint32_t)st.st_nlink;
        _buf->st_uid   = (uint32_t)st.st_uid;
        _buf->st_gid   = (uint32_t)st.st_gid;
        _buf->st_dev   = (uint64_t)st.st_dev;
        _buf->st_ino   = (uint64_t)st.st_ino;

        return 0;
    }
#else
    {
        struct stat st;

        if (fstat(_fd, &st) != 0)
        {
            return -1;
        }

        _buf->st_size  = (uint64_t)st.st_size;
        _buf->st_mtime = (uint64_t)st.st_mtime;
        _buf->st_atime = (uint64_t)st.st_atime;
        _buf->st_ctime = (uint64_t)st.st_ctime;
        _buf->st_mode  = (uint32_t)st.st_mode;
        _buf->st_nlink = (uint32_t)st.st_nlink;
        _buf->st_uid   = (uint32_t)st.st_uid;
        _buf->st_gid   = (uint32_t)st.st_gid;
        _buf->st_dev   = (uint64_t)st.st_dev;
        _buf->st_ino   = (uint64_t)st.st_ino;

        return 0;
    }
#endif
}


/*
d_lstat
  Get file status without following symlinks.

Parameter(s):
  _path: path to file.
  _buf:  pointer to d_stat_t to receive status.
Return:
  0 on success, -1 on failure.
*/
int
d_lstat
(
    const char*      _path, 
    struct d_stat_t* _buf
)
{
    // parameter validation
    if ( (!_path) || 
         (!_buf) )
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_POSIX)
    {
        struct stat st;

        d_memset(_buf, 0, sizeof(struct d_stat_t));

        if (lstat(_path, &st) != 0)
        {
            return -1;
        }

        _buf->st_size  = (uint64_t)st.st_size;
        _buf->st_mtime = (uint64_t)st.st_mtime;
        _buf->st_atime = (uint64_t)st.st_atime;
        _buf->st_ctime = (uint64_t)st.st_ctime;
        _buf->st_mode  = (uint32_t)st.st_mode;
        _buf->st_nlink = (uint32_t)st.st_nlink;
        _buf->st_uid   = (uint32_t)st.st_uid;
        _buf->st_gid   = (uint32_t)st.st_gid;
        _buf->st_dev   = (uint64_t)st.st_dev;
        _buf->st_ino   = (uint64_t)st.st_ino;

        return 0;
    }
#else
    // Windows: fall back to regular stat
    return d_stat(_path, _buf);
#endif
}


/*
d_access
  Check file access permissions.

Parameter(s):
  _path: path to file.
  _mode: access mode to check (F_OK, R_OK, W_OK, X_OK).
Return:
  0 if access allowed, -1 if not.
*/
int
d_access
(
    const char* _path, 
    int         _mode
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        // Windows _access doesn't support X_OK
        int win_mode = 0;

        if (_mode & R_OK)
        {
            win_mode |= 4;
        }

        if (_mode & W_OK)
        {
            win_mode |= 2;
        }

        // F_OK is 0, which means existence check
        return _access(_path, win_mode);
    }
#else
    return access(_path, _mode);
#endif
}


/*
d_chmod
  Change file permissions.

Parameter(s):
  _path: path to file.
  _mode: new permission mode.
Return:
  0 on success, -1 on failure.
*/
int
d_chmod
(
    const char* _path, 
    uint32_t    _mode
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _chmod(_path, (int)_mode);
#else
    return chmod(_path, (mode_t)_mode);
#endif
}


/*
d_file_size
  Get file size in bytes.

Parameter(s):
  _path: path to file.
Return:
  File size in bytes, or -1 on failure.
*/
int64_t
d_file_size
(
    const char* _path
)
{
    struct d_stat_t st;

    if (d_stat(_path, &st) != 0)
    {
        return -1;
    }

    return (int64_t)st.st_size;
}


/*
d_file_size_stream
  Get file size from stream.

Parameter(s):
  _stream: file stream.
Return:
  File size in bytes, or -1 on failure.
*/
int64_t
d_file_size_stream
(
    FILE* _stream
)
{
    d_off_t current;
    d_off_t size;

    // parameter validation
    if (!_stream)
    {
        return -1;
    }

    // save current position
    current = d_ftello(_stream);
    if (current < 0)
    {
        return -1;
    }

    // seek to end
    if (d_fseeko(_stream, 0, SEEK_END) != 0)
    {
        return -1;
    }

    // get size
    size = d_ftello(_stream);

    // restore position
    d_fseeko(_stream, current, SEEK_SET);

    return (int64_t)size;
}


/*
d_file_exists
  Check if file exists.

Parameter(s):
  _path: path to file.
Return:
  Non-zero if exists, 0 if not.
*/
int
d_file_exists
(
    const char* _path
)
{
    if (!_path)
    {
        return 0;
    }

    return (d_access(_path, F_OK) == 0) ? 1 : 0;
}


/*
d_is_file
  Check if path is a regular file.

Parameter(s):
  _path: path to check.
Return:
  Non-zero if regular file, 0 otherwise.
*/
int
d_is_file
(
    const char* _path
)
{
    struct d_stat_t st;

    if (d_stat(_path, &st) != 0)
    {
        return 0;
    }

    return S_ISREG(st.st_mode) ? 1 : 0;
}


/*
d_is_dir
  Check if path is a directory.

Parameter(s):
  _path: path to check.
Return:
  Non-zero if directory, 0 otherwise.
*/
int
d_is_dir
(
    const char* _path
)
{
    struct d_stat_t st;

    if (d_stat(_path, &st) != 0)
    {
        return 0;
    }

    return S_ISDIR(st.st_mode) ? 1 : 0;
}


///////////////////////////////////////////////////////////////////////////////
///             X.    DIRECTORY OPERATIONS                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_mkdir
  Create directory.

Parameter(s):
  _path: path for new directory.
  _mode: permission mode (ignored on Windows).
Return:
  0 on success, -1 on failure.
*/
int
d_mkdir
(
    const char* _path, 
    uint32_t    _mode
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    (void)_mode;  // unused on Windows

    return _mkdir(_path);
#else
    return mkdir(_path, (mode_t)_mode);
#endif
}


/*
d_mkdir_p
  Create directory and all parent directories.

Parameter(s):
  _path: path for new directory.
  _mode: permission mode.
Return:
  0 on success, -1 on failure.
*/
int
d_mkdir_p
(
    const char* _path, 
    uint32_t    _mode
)
{
    char   buf[D_FILE_PATH_MAX];
    char*  p;
    size_t len;

    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

    len = strlen(_path);
    if (len >= D_FILE_PATH_MAX)
    {
        errno = ENAMETOOLONG;

        return -1;
    }

    d_strcpy_s(buf, sizeof(buf), _path);

    // iterate through path components
    for (p = buf + 1; *p; p++)
    {
        if ( (*p == D_FILE_PATH_SEP) || 
             (*p == D_FILE_PATH_SEP_ALT) )
        {
            *p = '\0';

            // try to create directory (ignore if exists)
            if ( (d_mkdir(buf, _mode) != 0) && 
                 (errno != EEXIST) )
            {
                // check if it's actually a directory
                if (!d_is_dir(buf))
                {
                    return -1;
                }
            }

            *p = D_FILE_PATH_SEP;
        }
    }

    // create final directory
    if ( (d_mkdir(buf, _mode) != 0) && 
         (errno != EEXIST) )
    {
        if (!d_is_dir(buf))
        {
            return -1;
        }
    }

    return 0;
}


/*
d_rmdir
  Remove empty directory.

Parameter(s):
  _path: path to directory.
Return:
  0 on success, -1 on failure.
*/
int
d_rmdir
(
    const char* _path
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _rmdir(_path);
#else
    return rmdir(_path);
#endif
}


/*
d_opendir
  Open directory for reading.

Parameter(s):
  _path: path to directory.
Return:
  Directory handle on success, NULL on failure.
*/
struct d_dir_t*
d_opendir
(
    const char* _path
)
{
    struct d_dir_t* dir;

    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return NULL;
    }

    dir = malloc(sizeof(struct d_dir_t));
    if (!dir)
    {
        errno = ENOMEM;

        return NULL;
    }

    d_memset(dir, 0, sizeof(struct d_dir_t));

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        char search_path[D_FILE_PATH_MAX];
        size_t len;

        len = strlen(_path);
        if (len >= D_FILE_PATH_MAX - 3)
        {
            free(dir);
            errno = ENAMETOOLONG;

            return NULL;
        }

        d_strcpy_s(search_path, sizeof(search_path), _path);

        // append \* for FindFirstFile
        if ( (len > 0) && 
             (search_path[len - 1] != '\\') && 
             (search_path[len - 1] != '/') )
        {
            d_strcat_s(search_path, sizeof(search_path), "\\");
        }

        d_strcat_s(search_path, sizeof(search_path), "*");

        dir->handle = FindFirstFileA(search_path, &dir->find_data);
        if (dir->handle == INVALID_HANDLE_VALUE)
        {
            free(dir);

            return NULL;
        }

        dir->first_read = 1;
        d_strcpy_s(dir->path, sizeof(dir->path), _path);
    }
#else
    dir->handle = opendir(_path);
    if (!dir->handle)
    {
        free(dir);

        return NULL;
    }
#endif

    return dir;
}


/*
d_readdir
  Read next directory entry.

Parameter(s):
  _dir: directory handle.
Return:
  Pointer to directory entry, or NULL on end/error.
*/
struct d_dirent_t*
d_readdir
(
    struct d_dir_t* _dir
)
{
    // parameter validation
    if (!_dir)
    {
        return NULL;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        if (_dir->first_read)
        {
            _dir->first_read = 0;
        }
        else
        {
            if (!FindNextFileA(_dir->handle, &_dir->find_data))
            {
                return NULL;
            }
        }

        d_strcpy_s(_dir->entry.d_name, 
                   sizeof(_dir->entry.d_name),
                   _dir->find_data.cFileName);

        _dir->entry.d_ino = 0;

        // determine file type
        if (_dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            _dir->entry.d_type = DT_DIR;
        }
        else if (_dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            _dir->entry.d_type = DT_LNK;
        }
        else
        {
            _dir->entry.d_type = DT_REG;
        }

        return &_dir->entry;
    }
#else
    {
        struct dirent* entry;

        entry = readdir(_dir->handle);
        if (!entry)
        {
            return NULL;
        }

        d_strcpy_s(_dir->entry.d_name, 
                   sizeof(_dir->entry.d_name),
                   entry->d_name);

        _dir->entry.d_ino = entry->d_ino;

        #ifdef _DIRENT_HAVE_D_TYPE
            _dir->entry.d_type = entry->d_type;
        #else
            _dir->entry.d_type = DT_UNKNOWN;
        #endif

        return &_dir->entry;
    }
#endif
}


/*
d_closedir
  Close directory handle.

Parameter(s):
  _dir: directory handle.
Return:
  0 on success, -1 on failure.
*/
int
d_closedir
(
    struct d_dir_t* _dir
)
{
    int result;

    // parameter validation
    if (!_dir)
    {
        errno = EINVAL;

        return -1;
    }

    result = 0;

#if defined(D_FILE_PLATFORM_WINDOWS)
    if (!FindClose(_dir->handle))
    {
        result = -1;
    }
#else
    result = closedir(_dir->handle);
#endif

    free(_dir);

    return result;
}


/*
d_rewinddir
  Reset directory read position to beginning.

Parameter(s):
  _dir: directory handle.
Return:
  (none)
*/
void
d_rewinddir
(
    struct d_dir_t* _dir
)
{
    // parameter validation
    if (!_dir)
    {
        return;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        char search_path[D_FILE_PATH_MAX];

        FindClose(_dir->handle);

        d_strcpy_s(search_path, sizeof(search_path), _dir->path);
        d_strcat_s(search_path, sizeof(search_path), "\\*");

        _dir->handle = FindFirstFileA(search_path, &_dir->find_data);
        _dir->first_read = 1;
    }
#else
    rewinddir(_dir->handle);
#endif

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             XI.   FILE OPERATIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_remove
  Remove file or empty directory.

Parameter(s):
  _path: path to file or directory.
Return:
  0 on success, -1 on failure.
*/
int
d_remove
(
    const char* _path
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

    return remove(_path);
}


/*
d_unlink
  Remove file.

Parameter(s):
  _path: path to file.
Return:
  0 on success, -1 on failure.
*/
int
d_unlink
(
    const char* _path
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _unlink(_path);
#else
    return unlink(_path);
#endif
}


/*
d_rename
  Rename/move file with overwrite control.

Parameter(s):
  _oldpath:   current path.
  _newpath:   new path.
  _overwrite: if non-zero, replace existing destination.
Return:
  0 on success, -1 on failure.
*/
int
d_rename
(
    const char* _oldpath, 
    const char* _newpath, 
    int         _overwrite
)
{
    // parameter validation
    if ( (!_oldpath) || 
         (!_newpath) )
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        DWORD flags;

        flags = MOVEFILE_COPY_ALLOWED;
        if (_overwrite)
        {
            flags |= MOVEFILE_REPLACE_EXISTING;
        }

        if (!MoveFileExA(_oldpath, _newpath, flags))
        {
            errno = EACCES;

            return -1;
        }

        return 0;
    }
#else
    // POSIX rename always overwrites
    if (!_overwrite)
    {
        // check if destination exists
        if (d_access(_newpath, F_OK) == 0)
        {
            errno = EEXIST;

            return -1;
        }
    }

    return rename(_oldpath, _newpath);
#endif
}


/*
d_copy_file
  Copy file contents.

Parameter(s):
  _src: source file path.
  _dst: destination file path.
Return:
  0 on success, -1 on failure.
*/
int
d_copy_file
(
    const char* _src, 
    const char* _dst
)
{
    // parameter validation
    if ( (!_src) || 
         (!_dst) )
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    // use Windows CopyFile for efficiency
    if (!CopyFileA(_src, _dst, FALSE))
    {
        errno = EACCES;

        return -1;
    }

    return 0;
#else
    FILE*  src_file;
    FILE*  dst_file;
    char*  buffer;
    size_t bytes_read;
    int    result;

    // portable implementation
    src_file = d_fopen(_src, "rb");
    if (!src_file)
    {
        return -1;
    }

    dst_file = d_fopen(_dst, "wb");
    if (!dst_file)
    {
        fclose(src_file);

        return -1;
    }

    buffer = malloc(D_INTERNAL_FILE_COPY_BUF_SIZE);
    if (!buffer)
    {
        fclose(src_file);
        fclose(dst_file);
        errno = ENOMEM;

        return -1;
    }

    result = 0;

    while ((bytes_read = fread(buffer, 1, D_INTERNAL_FILE_COPY_BUF_SIZE, 
                               src_file)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, dst_file) != bytes_read)
        {
            result = -1;
            break;
        }
    }

    if (ferror(src_file))
    {
        result = -1;
    }

    free(buffer);
    fclose(src_file);
    fclose(dst_file);

    return result;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             XII.  PATH UTILITIES                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
d_getcwd
  Get current working directory.

Parameter(s):
  _buf:  buffer to receive the path.
  _size: size of buffer.
Return:
  _buf on success, NULL on failure.
*/
char*
d_getcwd
(
    char*  _buf, 
    size_t _size
)
{
    // parameter validation
    if ( (!_buf) || 
         (_size == 0) )
    {
        errno = EINVAL;

        return NULL;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _getcwd(_buf, (int)_size);
#else
    return getcwd(_buf, _size);
#endif
}


/*
d_chdir
  Change current working directory.

Parameter(s):
  _path: new working directory.
Return:
  0 on success, -1 on failure.
*/
int
d_chdir
(
    const char* _path
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _chdir(_path);
#else
    return chdir(_path);
#endif
}


/*
d_realpath
  Get canonical absolute path.

Parameter(s):
  _path:     path to resolve.
  _resolved: buffer to receive the result (or NULL to allocate).
Return:
  Pointer to resolved path, or NULL on failure.
*/
char*
d_realpath
(
    const char* _path, 
    char*       _resolved
)
{
    // parameter validation
    if (!_path)
    {
        errno = EINVAL;

        return NULL;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        char* result;

        if (_resolved)
        {
            result = _resolved;
        }
        else
        {
            result = malloc(D_FILE_PATH_MAX);
            if (!result)
            {
                errno = ENOMEM;

                return NULL;
            }
        }

        if (!_fullpath(result, _path, D_FILE_PATH_MAX))
        {
            if (!_resolved)
            {
                free(result);
            }

            return NULL;
        }

        // check if file/directory exists (POSIX realpath behavior)
        if (GetFileAttributesA(result) == INVALID_FILE_ATTRIBUTES)
        {
            if (!_resolved)
            {
                free(result);
            }

            errno = ENOENT;

            return NULL;
        }

        return result;
    }
#elif D_FILE_HAS_REALPATH
    return realpath(_path, _resolved);
#else
    // minimal fallback
    {
        char* result;

        if (_resolved)
        {
            result = _resolved;
        }
        else
        {
            result = malloc(D_FILE_PATH_MAX);
            if (!result)
            {
                errno = ENOMEM;

                return NULL;
            }
        }

        // just normalize the path
        if (!d_path_normalize(_path, result, D_FILE_PATH_MAX))
        {
            if (!_resolved)
            {
                free(result);
            }

            return NULL;
        }

        return result;
    }
#endif
}


/*
d_dirname
  Extract directory component from path.

Parameter(s):
  _path:    input path.
  _buf:     buffer to receive the result.
  _bufsize: size of buffer.
Return:
  _buf on success, NULL on failure.
*/
char*
d_dirname
(
    const char* _path, 
    char*       _buf, 
    size_t      _bufsize
)
{
    const char* last_sep;
    size_t      len;

    // parameter validation
    if ( (!_path)      || 
         (!_buf)       || 
         (_bufsize == 0) )
    {
        return NULL;
    }

    // find last separator
    last_sep = strrchr(_path, D_FILE_PATH_SEP);

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        const char* alt_sep;

        alt_sep = strrchr(_path, '/');
        if ( (alt_sep) && 
             ((!last_sep) || (alt_sep > last_sep)) )
        {
            last_sep = alt_sep;
        }
    }
#endif

    if (!last_sep)
    {
        // no separator, return "."
        if (_bufsize < 2)
        {
            return NULL;
        }

        _buf[0] = '.';
        _buf[1] = '\0';

        return _buf;
    }

    // handle root path
    if (last_sep == _path)
    {
        if (_bufsize < 2)
        {
            return NULL;
        }

        _buf[0] = D_FILE_PATH_SEP;
        _buf[1] = '\0';

        return _buf;
    }

    len = (size_t)(last_sep - _path);
    if (len >= _bufsize)
    {
        return NULL;
    }

    d_memcpy(_buf, _path, len);
    _buf[len] = '\0';

    return _buf;
}


/*
d_basename
  Extract filename component from path.

Parameter(s):
  _path:    input path.
  _buf:     buffer to receive the result.
  _bufsize: size of buffer.
Return:
  _buf on success, NULL on failure.
*/
char*
d_basename
(
    const char* _path, 
    char*       _buf, 
    size_t      _bufsize
)
{
    const char* last_sep;
    const char* name;
    size_t      len;

    // parameter validation
    if ( (!_path)      || 
         (!_buf)       || 
         (_bufsize == 0) )
    {
        return NULL;
    }

    // find last separator
    last_sep = strrchr(_path, D_FILE_PATH_SEP);

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        const char* alt_sep;

        alt_sep = strrchr(_path, '/');
        if ( (alt_sep) && 
             ((!last_sep) || (alt_sep > last_sep)) )
        {
            last_sep = alt_sep;
        }
    }
#endif

    if (last_sep)
    {
        name = last_sep + 1;
    }
    else
    {
        name = _path;
    }

    len = strlen(name);
    if (len >= _bufsize)
    {
        return NULL;
    }

    d_strcpy_s(_buf, _bufsize, name);

    return _buf;
}


/*
d_path_join
  Join path components.

Parameter(s):
  _buf:     buffer to receive the result.
  _bufsize: size of buffer.
  _path1:   first path component.
  _path2:   second path component.
Return:
  _buf on success, NULL on failure.
*/
char*
d_path_join
(
    char*       _buf, 
    size_t      _bufsize, 
    const char* _path1, 
    const char* _path2
)
{
    size_t len1;
    size_t len2;
    int    need_sep;

    // parameter validation
    if ( (!_buf)       || 
         (_bufsize == 0) )
    {
        return NULL;
    }

    // handle NULL paths
    if ( (!_path1) && 
         (!_path2) )
    {
        _buf[0] = '\0';

        return _buf;
    }

    if (!_path1)
    {
        return d_strcpy_s(_buf, _bufsize, _path2) == 0 ? _buf : NULL;
    }

    if (!_path2)
    {
        return d_strcpy_s(_buf, _bufsize, _path1) == 0 ? _buf : NULL;
    }

    len1 = strlen(_path1);
    len2 = strlen(_path2);

    // check if separator needed
    need_sep = 0;
    if (len1 > 0)
    {
        char last = _path1[len1 - 1];

        if ( (last != D_FILE_PATH_SEP) && 
             (last != D_FILE_PATH_SEP_ALT) )
        {
            need_sep = 1;
        }
    }

    // check if path2 starts with separator (absolute path)
    if ( (len2 > 0) && 
         ((_path2[0] == D_FILE_PATH_SEP) || (_path2[0] == D_FILE_PATH_SEP_ALT)) )
    {
        need_sep = 0;
    }

    // check buffer size
    if (len1 + len2 + need_sep >= _bufsize)
    {
        return NULL;
    }

    d_strcpy_s(_buf, _bufsize, _path1);

    if (need_sep)
    {
        _buf[len1]     = D_FILE_PATH_SEP;
        _buf[len1 + 1] = '\0';
    }

    d_strcat_s(_buf, _bufsize, _path2);

    return _buf;
}


/*
d_path_normalize
  Normalize path (resolve . and .., fix separators).

Parameter(s):
  _path:    input path.
  _buf:     buffer to receive the result.
  _bufsize: size of buffer.
Return:
  _buf on success, NULL on failure.
*/
char*
d_path_normalize
(
    const char* _path, 
    char*       _buf, 
    size_t      _bufsize
)
{
    char   temp[D_FILE_PATH_MAX];
    char*  components[D_FILE_PATH_MAX / 2];
    int    num_components;
    char*  p;
    char*  token;
    char*  saveptr;
    size_t len;
    int    i;
    int    is_absolute;

    // parameter validation
    if ( (!_path)       || 
         (!_buf)        || 
         (_bufsize == 0) )
    {
        return NULL;
    }

    len = strlen(_path);
    if (len >= D_FILE_PATH_MAX)
    {
        return NULL;
    }

    d_strcpy_s(temp, sizeof(temp), _path);

    // normalize separators
    for (p = temp; *p; p++)
    {
        if (*p == D_FILE_PATH_SEP_ALT)
        {
            *p = D_FILE_PATH_SEP;
        }
    }

    // check if absolute
    is_absolute = d_path_is_absolute(temp);

    // tokenize and resolve . and ..
    num_components = 0;
    token = d_strtok_r(temp, D_FILE_PATH_SEP_STR, &saveptr);

    while (token)
    {
        if ( (strcmp(token, ".") == 0) || 
             (strlen(token) == 0) )
        {
            // skip "." and empty components
        }
        else if (strcmp(token, "..") == 0)
        {
            // go up one level
            if (num_components > 0)
            {
                num_components--;
            }
        }
        else
        {
            components[num_components++] = token;
        }

        token = d_strtok_r(NULL, D_FILE_PATH_SEP_STR, &saveptr);
    }

    // build result
    _buf[0] = '\0';

    if (is_absolute)
    {
#if defined(D_FILE_PLATFORM_WINDOWS)
        // preserve drive letter
        if ( (len >= 2) && 
             (_path[1] == ':') )
        {
            _buf[0] = _path[0];
            _buf[1] = ':';
            _buf[2] = D_FILE_PATH_SEP;
            _buf[3] = '\0';
        }
        else
        {
            _buf[0] = D_FILE_PATH_SEP;
            _buf[1] = '\0';
        }
#else
        _buf[0] = D_FILE_PATH_SEP;
        _buf[1] = '\0';
#endif
    }

    for (i = 0; i < num_components; i++)
    {
        if (i > 0 || is_absolute)
        {
            if (strlen(_buf) + 1 >= _bufsize)
            {
                return NULL;
            }

            d_strcat_s(_buf, _bufsize, D_FILE_PATH_SEP_STR);
        }

        if (strlen(_buf) + strlen(components[i]) >= _bufsize)
        {
            return NULL;
        }

        d_strcat_s(_buf, _bufsize, components[i]);
    }

    // handle empty result
    if (_buf[0] == '\0')
    {
        _buf[0] = '.';
        _buf[1] = '\0';
    }

    return _buf;
}


/*
d_path_is_absolute
  Check if path is absolute.

Parameter(s):
  _path: path to check.
Return:
  Non-zero if absolute, 0 if relative.
*/
int
d_path_is_absolute
(
    const char* _path
)
{
    if ( (!_path) || 
         (_path[0] == '\0') )
    {
        return 0;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    // check for drive letter (C:\) or UNC path (\\)
    if ( (strlen(_path) >= 3) && 
         (isalpha((unsigned char)_path[0])) &&
         (_path[1] == ':') && 
         ((_path[2] == '\\') || (_path[2] == '/')) )
    {
        return 1;
    }

    if ( (strlen(_path) >= 2) && 
         (_path[0] == '\\') && 
         (_path[1] == '\\') )
    {
        return 1;
    }

    return 0;
#else
    return (_path[0] == '/') ? 1 : 0;
#endif
}


/*
d_get_extension
  Get file extension from path.

Parameter(s):
  _path: path to examine.
Return:
  Pointer to extension (including dot) within _path, or NULL if none.
*/
const char*
d_get_extension
(
    const char* _path
)
{
    const char* dot;
    const char* sep;

    if (!_path)
    {
        return NULL;
    }

    dot = strrchr(_path, '.');
    if (!dot)
    {
        return NULL;
    }

    // make sure dot is after last separator
    sep = strrchr(_path, D_FILE_PATH_SEP);

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        const char* alt_sep;

        alt_sep = strrchr(_path, '/');
        if ( (alt_sep) && 
             ((!sep) || (alt_sep > sep)) )
        {
            sep = alt_sep;
        }
    }
#endif

    if ( (sep) && 
         (dot < sep) )
    {
        return NULL;
    }

    // don't return dot for hidden files like ".bashrc"
    if ( (dot == _path) || 
         ((sep) && (dot == sep + 1)) )
    {
        return NULL;
    }

    return dot;
}


///////////////////////////////////////////////////////////////////////////////
///             XIII. SYMBOLIC LINKS                                        ///
///////////////////////////////////////////////////////////////////////////////

#if D_FILE_HAS_SYMLINKS

/*
d_symlink
  Create symbolic link.

Parameter(s):
  _target:   target path.
  _linkpath: path for the symbolic link.
Return:
  0 on success, -1 on failure.
*/
int
d_symlink
(
    const char* _target, 
    const char* _linkpath
)
{
    // parameter validation
    if ( (!_target) || 
         (!_linkpath) )
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        DWORD flags;

        flags = 0;

        // check if target is a directory
        if (d_is_dir(_target))
        {
            flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
        }

        // try unprivileged symlink first (Windows 10 1703+)
        flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

        if (!CreateSymbolicLinkA(_linkpath, _target, flags))
        {
            // retry without unprivileged flag
            flags &= ~SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

            if (!CreateSymbolicLinkA(_linkpath, _target, flags))
            {
                errno = EPERM;

                return -1;
            }
        }

        return 0;
    }
#else
    return symlink(_target, _linkpath);
#endif
}


/*
d_readlink
  Read symbolic link target.

Parameter(s):
  _path:    path to symbolic link.
  _buf:     buffer to receive the target.
  _bufsize: size of buffer.
Return:
  Number of bytes placed in _buf (not null-terminated), or -1 on error.
*/
ssize_t
d_readlink
(
    const char* _path, 
    char*       _buf, 
    size_t      _bufsize
)
{
    // parameter validation
    if ( (!_path) || 
         (!_buf)  || 
         (_bufsize == 0) )
    {
        errno = EINVAL;

        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    {
        HANDLE h;
        char   buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
        DWORD  bytes_returned;
        BOOL   result;

        h = CreateFileA(_path,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                        NULL);

        if (h == INVALID_HANDLE_VALUE)
        {
            errno = ENOENT;

            return -1;
        }

        result = DeviceIoControl(h,
                                 FSCTL_GET_REPARSE_POINT,
                                 NULL,
                                 0,
                                 buffer,
                                 sizeof(buffer),
                                 &bytes_returned,
                                 NULL);

        CloseHandle(h);

        if (!result)
        {
            errno = EINVAL;

            return -1;
        }

        // parse reparse point data - simplified for symlinks
        // full implementation would parse REPARSE_DATA_BUFFER
        errno = ENOSYS;

        return -1;
    }
#else
    return readlink(_path, _buf, _bufsize);
#endif
}


/*
d_is_symlink
  Check if path is a symbolic link.

Parameter(s):
  _path: path to check.
Return:
  Non-zero if symlink, 0 otherwise.
*/
int
d_is_symlink
(
    const char* _path
)
{
    struct d_stat_t st;

    if (d_lstat(_path, &st) != 0)
    {
        return 0;
    }

    return S_ISLNK(st.st_mode) ? 1 : 0;
}

#endif  // D_FILE_HAS_SYMLINKS


///////////////////////////////////////////////////////////////////////////////
///             XIV.  PIPE OPERATIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_popen
  Open process pipe.

Parameter(s):
  _command: command to execute.
  _mode:    mode ("r" for reading, "w" for writing).
Return:
  File pointer on success, NULL on failure.
*/
FILE*
d_popen
(
    const char* _command, 
    const char* _mode
)
{
    // parameter validation
    if ( (!_command) || 
         (!_mode) )
    {
        return NULL;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _popen(_command, _mode);
#else
    return popen(_command, _mode);
#endif
}


/*
d_pclose
  Close process pipe.

Parameter(s):
  _stream: file pointer from d_popen.
Return:
  Exit status of command, or -1 on error.
*/
int
d_pclose
(
    FILE* _stream
)
{
    // parameter validation
    if (!_stream)
    {
        return -1;
    }

#if defined(D_FILE_PLATFORM_WINDOWS)
    return _pclose(_stream);
#else
    return pclose(_stream);
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             XV.   BINARY I/O HELPERS                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
d_fread_all
  Read entire file into memory.

Parameter(s):
  _path: path to file.
  _size: pointer to receive the number of bytes read (may be NULL).
Return:
  Pointer to allocated buffer containing file contents, or NULL on failure.
  Caller must free the returned buffer.
*/
void*
d_fread_all
(
    const char* _path, 
    size_t*     _size
)
{
    FILE*   file;
    int64_t file_size;
    void*   buffer;
    size_t  bytes_read;

    // parameter validation
    if (!_path)
    {
        return NULL;
    }

    if (_size)
    {
        *_size = 0;
    }

    file = d_fopen(_path, "rb");
    if (!file)
    {
        return NULL;
    }

    file_size = d_file_size_stream(file);
    if (file_size < 0)
    {
        fclose(file);

        return NULL;
    }

    buffer = malloc((size_t)file_size + 1);  // +1 for null terminator
    if (!buffer)
    {
        fclose(file);
        errno = ENOMEM;

        return NULL;
    }

    bytes_read = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size)
    {
        free(buffer);

        return NULL;
    }

    // null-terminate for convenience with text files
    ((char*)buffer)[bytes_read] = '\0';

    if (_size)
    {
        *_size = bytes_read;
    }

    return buffer;
}


/*
d_fwrite_all
  Write buffer to file (creates or overwrites).

Parameter(s):
  _path: path to file.
  _data: data to write.
  _size: number of bytes to write.
Return:
  0 on success, -1 on failure.
*/
int
d_fwrite_all
(
    const char* _path, 
    const void* _data, 
    size_t      _size
)
{
    FILE*  file;
    size_t bytes_written;

    // parameter validation
    if ( (!_path) || 
         ((!_data) && (_size > 0)) )
    {
        errno = EINVAL;

        return -1;
    }

    file = d_fopen(_path, "wb");
    if (!file)
    {
        return -1;
    }

    if (_size > 0)
    {
        bytes_written = fwrite(_data, 1, _size, file);
        if (bytes_written != _size)
        {
            fclose(file);

            return -1;
        }
    }

    fclose(file);

    return 0;
}

/*
d_fappend_all
  Append buffer to file.

Parameter(s):
  _path: path to file.
  _data: data to append.
  _size: number of bytes to append.
Return:
  0 on success, -1 on failure.
*/
int
d_fappend_all
(
    const char* _path, 
    const void* _data, 
    size_t      _size
)
{
    FILE*  file;
    size_t bytes_written;

    // parameter validation
    if ( (!_path) || 
         ( (!_data) && (_size > 0) ) )
    {
        errno = EINVAL;

        return -1;
    }

    file = d_fopen(_path, "ab");
    if (!file)
    {
        return -1;
    }

    if (_size > 0)
    {
        bytes_written = fwrite(_data, 1, _size, file);
        if (bytes_written != _size)
        {
            fclose(file);

            return -1;
        }
    }

    fclose(file);

    return 0;
}

