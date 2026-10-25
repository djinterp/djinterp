/*******************************************************************************
* djinterp [tools]                                                   dgit_file.c
*
* Definitions for the files of dgit_internal.h.
*   Reading and writing are ISO C. Asking what a path names, reading a link,
* a path's real name and the working directory are not, and each is written
* twice, for POSIX and for Windows, the system choosing between whole
* functions: no body here is split by a conditional. Windows has no link git
* can make there, no inode and no owner, so those answers are none and 0.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_file.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./dgit_internal.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fread, fwrite, fclose, ferror, remove,
                     // rename, snprintf; fseeko, ftello on POSIX
#include <stdlib.h>  // malloc, realloc, free, realpath; _fullpath on Windows
#include <string.h>  // memcpy, memset, strlen
// platform
#if defined(_WIN32)
    #include <direct.h>    // _mkdir, _getcwd
    #include <sys/stat.h>  // struct _stat64, _stat64, _S_IFDIR, _S_IFREG
                           // (stdio.h's _fseeki64, _ftelli64 besides)
#else
    #include <sys/stat.h>   // struct stat, lstat, mkdir, S_ISDIR, S_ISREG,
                            // S_ISLNK, S_IXUSR
    #include <sys/types.h>  // off_t, ssize_t
    #include <unistd.h>     // readlink, getcwd
#endif  // _WIN32


/*
d_git_file_read
  Read in pieces that double, so a file whose size cannot be asked -- a pipe
-- reads as well as one whose can.
  Long as a file is read a block at a time, its size not known until its
end.
*/
char*
d_git_file_read(
    const char* _path,
    size_t*     _out_length
)
{
    *_out_length = 0u;

    FILE* const file = (_path) ? fopen(_path, "rb") : NULL;

    // a file that cannot be opened reads as none
    if (!file)
    {
        return NULL;
    }

    size_t capacity = 4096u;
    size_t length   = 0u;
    char*  text     = malloc(capacity);

    // the pieces, until the end
    while (text)
    {
        // room for another piece, and the terminator
        if ((capacity - length) < 2u)
        {
            char* const grown = realloc(text, capacity * 2u);

            // memory that ran out reads nothing
            if (!grown)
            {
                free(text);
                text = NULL;

                break;
            }

            text      = grown;
            capacity *= 2u;
        }

        const size_t got = fread(&text[length],
                                 1u,
                                 capacity - length - 1u,
                                 file);

        length += got;

        // a short read is the end, or a failure
        if (got == 0u)
        {
            break;
        }
    }

    const bool failed = (ferror(file) != 0);

    (void)fclose(file);

    // a read that failed reads nothing
    if ( (!text) ||
         (failed) )
    {
        free(text);

        return NULL;
    }

    text[length] = '\0';
    *_out_length = length;

    return text;
}


#if defined(_WIN32)

/*
d_internal_is_separator, d_internal_make_directory, d_internal_is_directory
  Whether a byte separates a path's parts -- `/` and `\` -- a directory
made, 0 when it is, and whether a path is a directory.
*/
static bool
d_internal_is_separator(
    char _c
)
{
    return ( (_c == '/') || (_c == '\\') );
}


static int
d_internal_make_directory(
    const char* _path
)
{
    return _mkdir(_path);
}


static bool
d_internal_is_directory(
    const char* _path
)
{
    struct _stat64 info;

    return ( (_stat64(_path, &info) == 0) &&
             ( (info.st_mode & _S_IFDIR) != 0 ) );
}

#else

/*
d_internal_is_separator, d_internal_make_directory, d_internal_is_directory
  Whether a byte separates a path's parts -- `/` -- a directory made, 0
when it is, and whether a path is a directory.
*/
static bool
d_internal_is_separator(
    char _c
)
{
    return (_c == '/');
}


static int
d_internal_make_directory(
    const char* _path
)
{
    return mkdir(_path, 0777);
}


static bool
d_internal_is_directory(
    const char* _path
)
{
    struct stat info;

    return ( (stat(_path, &info) == 0) &&
             (S_ISDIR(info.st_mode)) );
}

#endif  // _WIN32


/*
d_internal_make_parents
  Every directory a file's path names that is missing.
*/
static bool
d_internal_make_parents(
    const char* _path
)
{
    const size_t length = strlen(_path);
    char* const  copy   = malloc(length + 1u);

    // a copy to cut at each separator
    if (!copy)
    {
        return false;
    }

    memcpy(copy, _path, length + 1u);

    // each separator after the first byte, its directory made
    for (size_t at = 1u; at < length; ++at)
    {
        // a byte that separates nothing
        if (!d_internal_is_separator(copy[at]))
        {
            continue;
        }

        copy[at] = '\0';

        const int made = d_internal_make_directory(copy);

        // made, or there already; past a drive's `C:` on Windows
        if ( (made != 0)                        &&
             (!d_internal_is_directory(copy))   &&
             ( (at < 2u) || (copy[at - 1u] != ':') ) )
        {
            free(copy);

            return false;
        }

        copy[at] = _path[at];
    }

    free(copy);

    return true;
}


/*
d_git_file_write
  Written beside its target first, so a write that fails part way leaves
the target as it was. A rename that will not replace a file, as Windows'
will not, is tried again once the target is gone.
  Long as the directories, the part file and the rename are each a step,
each with its failure.
*/
bool
d_git_file_write(
    const char* _path,
    const char* _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    const size_t length = strlen(_path);
    char* const  part   = malloc(length + 6u);

    // the name it is written to first
    if (!part)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return false;
    }

    memcpy(part, _path, length);
    memcpy(&part[length], ".part", 6u);

    // the directories it goes in
    if (!d_internal_make_parents(_path))
    {
        (void)snprintf(_problem,
                       _capacity,
                       "cannot make the directories of %s",
                       _path);
        free(part);

        return false;
    }

    FILE* const file = fopen(part, "wb");

    // the file beside it
    if (!file)
    {
        (void)snprintf(_problem, _capacity, "cannot write %s", part);
        free(part);

        return false;
    }

    const bool written = ( (_length == 0u) ||
                           (fwrite(_text, 1u, _length, file) == _length) );
    const bool closed  = (fclose(file) == 0);
    bool       renamed = ( (written) &&
                           (closed)  &&
                           (rename(part, _path) == 0) );

    // a rename that would not replace a file
    if ( (written) &&
         (closed)  &&
         (!renamed) )
    {
        (void)remove(_path);
        renamed = (rename(part, _path) == 0);
    }

    // a file not renamed into place is taken away
    if (!renamed)
    {
        (void)remove(part);
        (void)snprintf(_problem, _capacity, "cannot write %s", _path);
    }

    free(part);

    return renamed;
}


#if defined(_WIN32)

/*
d_git_file_info
  Windows' stat follows nothing that git would make a link of, so a path is
a directory, a file, or something else; it has no inode and no owner.
*/
bool
d_git_file_info(
    const char*             _path,
    struct d_git_file_info* _out
)
{
    struct _stat64 info;

    memset(_out, 0, sizeof(*_out));

    // nothing there
    if ( (!_path) ||
         (_stat64(_path, &info) != 0) )
    {
        return false;
    }

    _out->kind       = ( (info.st_mode & _S_IFDIR) != 0 )
                       ? D_GIT_FILE_DIRECTORY
                       : ( ( (info.st_mode & _S_IFREG) != 0 )
                           ? D_GIT_FILE_REGULAR
                           : D_GIT_FILE_OTHER );
    _out->executable = false;
    _out->size       = (uint64_t)info.st_size;
    _out->mtime      = (uint32_t)info.st_mtime;
    _out->ctime      = (uint32_t)info.st_ctime;

    return true;
}


char*
d_git_file_link(
    const char* _path,
    size_t*     _out_length
)
{
    (void)_path;
    *_out_length = 0u;

    return NULL;
}


char*
d_git_file_real(
    const char* _path
)
{
    struct d_git_file_info info;

    // a path that names nothing has no real name
    if ( (!_path) ||
         (!d_git_file_info(_path, &info)) )
    {
        return NULL;
    }

    return _fullpath(NULL, _path, 0u);
}


char*
d_git_file_cwd(void)
{
    return _getcwd(NULL, 0);
}


/*
d_git_file_open, d_git_file_read_at
  Windows' 64-bit places, _fseeki64 and _ftelli64.
*/
void*
d_git_file_open(
    const char* _path,
    uint64_t*   _out_size
)
{
    FILE* const file = (_path) ? fopen(_path, "rb") : NULL;

    *_out_size = 0u;

    // the file, and its size from its end
    if ( (!file) ||
         (_fseeki64(file, 0, SEEK_END) != 0) )
    {
        // a file opened but not measured is closed
        if (file)
        {
            (void)fclose(file);
        }

        return NULL;
    }

    const __int64 size = _ftelli64(file);

    *_out_size = (size > 0) ? (uint64_t)size : 0u;

    return file;
}


bool
d_git_file_read_at(
    void*    _file,
    uint64_t _offset,
    void*    _buffer,
    size_t   _length
)
{
    FILE* const file = _file;

    return ( (file) &&
             (_fseeki64(file, (__int64)_offset, SEEK_SET) == 0) &&
             (fread(_buffer, 1u, _length, file) == _length) );
}

#else

/*
d_git_file_info
  lstat's answer, a link not followed: a link is a link, as git's index
keeps it.
*/
bool
d_git_file_info(
    const char*             _path,
    struct d_git_file_info* _out
)
{
    struct stat info;

    memset(_out, 0, sizeof(*_out));

    // nothing there
    if ( (!_path) ||
         (lstat(_path, &info) != 0) )
    {
        return false;
    }

    _out->kind       = (S_ISLNK(info.st_mode))
                       ? D_GIT_FILE_LINK
                       : ( (S_ISDIR(info.st_mode))
                           ? D_GIT_FILE_DIRECTORY
                           : ( (S_ISREG(info.st_mode))
                               ? D_GIT_FILE_REGULAR
                               : D_GIT_FILE_OTHER ) );
    _out->executable = ((info.st_mode & S_IXUSR) != 0);
    _out->size       = (uint64_t)info.st_size;
    _out->mtime      = (uint32_t)info.st_mtime;
    _out->ctime      = (uint32_t)info.st_ctime;
    _out->dev        = (uint32_t)info.st_dev;
    _out->ino        = (uint32_t)info.st_ino;
    _out->uid        = (uint32_t)info.st_uid;
    _out->gid        = (uint32_t)info.st_gid;

    return true;
}


/*
d_git_file_link
  readlink, into a buffer that doubles until the link fits with room to
spare: readlink says nothing of a link it cut short.
*/
char*
d_git_file_link(
    const char* _path,
    size_t*     _out_length
)
{
    size_t capacity = 256u;

    *_out_length = 0u;

    // until the link fits with a byte to spare
    while (_path)
    {
        char* const text = malloc(capacity);

        // memory that ran out reads nothing
        if (!text)
        {
            return NULL;
        }

        const ssize_t got = readlink(_path, text, capacity);

        // no link, or one that fits
        if ( (got < 0) ||
             ((size_t)got < capacity) )
        {
            // not a link
            if (got < 0)
            {
                free(text);

                return NULL;
            }

            text[got]    = '\0';
            *_out_length = (size_t)got;

            return text;
        }

        free(text);
        capacity *= 2u;
    }

    return NULL;
}


char*
d_git_file_real(
    const char* _path
)
{
    return (_path) ? realpath(_path, NULL) : NULL;
}


/*
d_git_file_cwd
  getcwd into a buffer that doubles until the directory's name fits.
*/
char*
d_git_file_cwd(void)
{
    size_t capacity = 256u;

    // until the name fits
    for (;;)
    {
        char* const text = malloc(capacity);

        // memory that ran out gives nothing
        if (!text)
        {
            return NULL;
        }

        // the name, or a buffer too short
        if (getcwd(text, capacity))
        {
            return text;
        }

        free(text);

        // a name that is not too long, but cannot be had
        if (capacity > (1u << 20))
        {
            return NULL;
        }

        capacity *= 2u;
    }
}


/*
d_git_file_open, d_git_file_read_at
  POSIX's 64-bit places, fseeko and ftello, with off_t as wide as the
build makes it.
*/
void*
d_git_file_open(
    const char* _path,
    uint64_t*   _out_size
)
{
    FILE* const file = (_path) ? fopen(_path, "rb") : NULL;

    *_out_size = 0u;

    // the file, and its size from its end
    if ( (!file) ||
         (fseeko(file, 0, SEEK_END) != 0) )
    {
        // a file opened but not measured is closed
        if (file)
        {
            (void)fclose(file);
        }

        return NULL;
    }

    const off_t size = ftello(file);

    *_out_size = (size > 0) ? (uint64_t)size : 0u;

    return file;
}


bool
d_git_file_read_at(
    void*    _file,
    uint64_t _offset,
    void*    _buffer,
    size_t   _length
)
{
    FILE* const file = _file;

    return ( (file) &&
             (fseeko(file, (off_t)_offset, SEEK_SET) == 0) &&
             (fread(_buffer, 1u, _length, file) == _length) );
}

#endif  // _WIN32


void
d_git_file_close(
    void* _file
)
{
    // a file to close
    if (_file)
    {
        (void)fclose((FILE*)_file);
    }

    return;
}
