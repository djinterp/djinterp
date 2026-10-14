/*******************************************************************************
* djinterp [parse]                                             arg_tests_files.c
*
* The scratch directory of the argument parser's C suites, declared in
* arg_tests_files.h.
*   The one file of the suites that asks the system itself, as arg_system.c
* is the one of the parser. Each system's primitives -- the working
* directory, a fresh temporary directory, making and removing a directory, a
* file, a mode, a link, a FIFO -- are chosen whole, a function at a time, by
* the environment layer's file status: POSIX's, or the Microsoft runtime's
* with every name widened from UTF-8. A system with neither makes nothing,
* and every call fails, so that a suite needing files fails rather than
* passes without them. The public calls are written once over the
* primitives.
*   Needs POSIX.1-2008 with XSI where the C library hides it in a strict ISO
* mode (mkdtemp, symlink, mkfifo), which the ladders' _XOPEN_SOURCE=700
* gives.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_tests_files.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_tests_files.h"  // corresponding header
// std
#include <errno.h>    // errno, ERANGE
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // FILE, fopen, fwrite, fclose, printf
#include <stdlib.h>   // malloc, free, getenv, mkdtemp
#include <string.h>   // strlen, memcpy, memset
// djinterp
#include "../../../../../../inc/djinterp/env/parse/cli/arg_parser/env_arg_parser.h"  // D_ENV_ARG_PARSER_STAT
// platform
#if (D_TESTS_ARG_FILES_POSIX == 1)
    #include <sys/types.h>  // mode_t
    #include <sys/stat.h>   // mkdir, chmod, mkfifo
    #include <unistd.h>     // getcwd, chdir, rmdir, unlink, symlink
#endif
#if (D_TESTS_ARG_FILES_WINDOWS == 1)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif  // WIN32_LEAN_AND_MEAN
    #include <windows.h>    // GetTempPathW, GetCurrentProcessId,
                            // MultiByteToWideChar
    #include <direct.h>     // _wmkdir, _wrmdir, _wchdir, _wgetcwd
    #include <io.h>         // _wchmod, _wunlink
    #include <sys/stat.h>   // _S_IREAD, _S_IWRITE
    #include <wchar.h>      // wchar_t, _wfopen, swprintf
#endif


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SCRATCH_ATTEMPTS
//   constant: how many names a fresh directory is tried under, where the
// system offers no call that finds one.
enum
{
    SCRATCH_ATTEMPTS = 64
};

/**
 * @brief Copies a text into memory of its own.
 *
 * @param[in] _text  the text.
 * @return the copy, which the caller frees; NULL when memory is refused.
 */
static char*
d_tests_arg_files_copy(
    const char* _text
)
{
    const size_t size = strlen(_text) + 1u;
    char* const  copy = malloc(size);

    // check if memory allocation was successful
    if (copy != NULL)
    {
        memcpy(copy,
               _text,
               size);
    }

    return copy;
}

#if (D_TESTS_ARG_FILES_POSIX == 1)

/**
 * @brief The working directory.
 *
 * @return its path, which the caller frees; NULL when it cannot be had.
 *
 * getcwd is asked into a buffer that doubles until the path fits, since
 * POSIX does not promise it allocates one itself.
 */
static void*
d_tests_arg_files_os_home(void)
{
    size_t size = 256u;

    // a buffer at a time, until the path fits
    while (size <= 65536u)
    {
        char* const buffer = malloc(size);

        // check if memory allocation was successful
        if (!buffer)
        {
            return NULL;
        }

        // the path fits
        if (getcwd(buffer,
                   size) != NULL)
        {
            return buffer;
        }

        free(buffer);

        // anything but a short buffer ends the search
        if (errno != ERANGE)
        {
            return NULL;
        }

        size *= 2u;
    }

    return NULL;
}

/**
 * @brief Makes a fresh directory under the system's temporary directory.
 *
 * @return its path, which the caller frees; NULL when it cannot be made.
 *
 * mkdtemp finds the unused name itself; the temporary directory is TMPDIR,
 * or /tmp without it.
 */
static void*
d_tests_arg_files_os_scratch(void)
{
    static const char NAME[] = "/djinterp-arg-XXXXXX";

    const char* const tmpdir = getenv("TMPDIR");
    const char* const base   = ( (tmpdir != NULL) &&
                                 (tmpdir[0] != '\0') )
                               ? tmpdir
                               : "/tmp";
    const size_t      length = strlen(base);
    char* const       path   = malloc(length + sizeof(NAME));

    // check if memory allocation was successful
    if (!path)
    {
        return NULL;
    }

    memcpy(path,
           base,
           length);
    memcpy(path + length,
           NAME,
           sizeof(NAME));

    // the directory made, or nothing kept
    if (mkdtemp(path) == NULL)
    {
        free(path);

        return NULL;
    }

    return path;
}

/**
 * @brief Makes a directory the working directory.
 *
 * @param[in] _path  the directory, as d_tests_arg_files_os_home or
 *                   d_tests_arg_files_os_scratch gave it.
 * @return whether it is.
 */
static bool
d_tests_arg_files_os_enter(
    const void* _path
)
{
    return (chdir((const char*)_path) == 0);
}

/**
 * @brief Removes an empty directory given by a system path.
 *
 * @param[in] _path  the directory, as d_tests_arg_files_os_scratch gave it.
 * @return whether it was removed.
 */
static bool
d_tests_arg_files_os_remove_scratch(
    const void* _path
)
{
    return (rmdir((const char*)_path) == 0);
}

/**
 * @brief Makes a directory.
 *
 * @param[in] _name  its name, relative.
 * @return whether it was made.
 */
static bool
d_tests_arg_files_os_folder(
    const char* _name
)
{
    return (mkdir(_name,
                  0755) == 0);
}

/**
 * @brief Opens a new file for writing bytes.
 *
 * @param[in] _name  its name, relative.
 * @return the stream, or NULL.
 */
static FILE*
d_tests_arg_files_os_open(
    const char* _name
)
{
    return fopen(_name,
                 "wb");
}

/**
 * @brief Sets permission bits.
 *
 * @param[in] _name  what was made, relative.
 * @param[in] _mode  the bits.
 * @return whether they were set.
 */
static bool
d_tests_arg_files_os_mode(
    const char*  _name,
    unsigned int _mode
)
{
    return (chmod(_name,
                  (mode_t)_mode) == 0);
}

/**
 * @brief Removes one thing made: a directory with rmdir, anything else --
 *        a link itself, not what it points to -- with unlink.
 *
 * @param[in] _name    its name, relative.
 * @param[in] _folder  whether it is a directory.
 * @return whether it was removed.
 */
static bool
d_tests_arg_files_os_remove(
    const char* _name,
    bool        _folder
)
{
    return (_folder)
           ? (rmdir(_name) == 0)
           : (unlink(_name) == 0);
}

#elif (D_TESTS_ARG_FILES_WINDOWS == 1)

/**
 * @brief Widens a UTF-8 name for the wide runtime.
 *
 * @param[in] _name  the name.
 * @return the wide name, which the caller frees; NULL when the name is not
 *         UTF-8 or memory is refused.
 */
static wchar_t*
d_tests_arg_files_wide(
    const char* _name
)
{
    const int count = MultiByteToWideChar(CP_UTF8,
                                          MB_ERR_INVALID_CHARS,
                                          _name,
                                          -1,
                                          NULL,
                                          0);

    // a name that is not UTF-8
    if (count <= 0)
    {
        return NULL;
    }

    wchar_t* const wide = malloc((size_t)count * sizeof(wchar_t));

    // the name widened, or nothing kept
    if ( (wide != NULL) &&
         (MultiByteToWideChar(CP_UTF8,
                              MB_ERR_INVALID_CHARS,
                              _name,
                              -1,
                              wide,
                              count) != count) )
    {
        free(wide);

        return NULL;
    }

    return wide;
}

/**
 * @brief The working directory.
 *
 * @return its path, wide, which the caller frees; NULL when it cannot be
 *         had.
 */
static void*
d_tests_arg_files_os_home(void)
{
    return _wgetcwd(NULL,
                    0);
}

/**
 * @brief Makes a fresh directory under the system's temporary directory.
 *
 * @return its path, wide, which the caller frees; NULL when it cannot be
 *         made.
 *
 * The runtime has no mkdtemp: names made of the process and a counter are
 * tried in turn, under GetTempPathW's directory, until one is new.
 */
static void*
d_tests_arg_files_os_scratch(void)
{
    static unsigned int counter = 0u;

    wchar_t      base[MAX_PATH + 1] = { 0 };
    const DWORD  length             = GetTempPathW(MAX_PATH + 1,
                                                   base);
    const size_t size               = (size_t)length + 64u;

    // a temporary directory to make it in
    if ( (length == 0u) ||
         (length > MAX_PATH) )
    {
        return NULL;
    }

    wchar_t* const path = malloc(size * sizeof(wchar_t));

    // each name until one is new
    for (int attempt = 0;
         (path != NULL) && (attempt < SCRATCH_ATTEMPTS);
         ++attempt)
    {
        (void)swprintf(path,
                       size,
                       L"%lsdjinterp-arg-%lu-%u",
                       base,
                       (unsigned long)GetCurrentProcessId(),
                       counter++);

        // a new directory
        if (_wmkdir(path) == 0)
        {
            return path;
        }
    }

    free(path);

    return NULL;
}

/**
 * @brief Makes a directory the working directory.
 *
 * @param[in] _path  the directory, wide.
 * @return whether it is.
 */
static bool
d_tests_arg_files_os_enter(
    const void* _path
)
{
    return (_wchdir((const wchar_t*)_path) == 0);
}

/**
 * @brief Removes an empty directory given by a system path.
 *
 * @param[in] _path  the directory, wide.
 * @return whether it was removed.
 */
static bool
d_tests_arg_files_os_remove_scratch(
    const void* _path
)
{
    return (_wrmdir((const wchar_t*)_path) == 0);
}

/**
 * @brief Makes a directory.
 *
 * @param[in] _name  its name, relative, UTF-8.
 * @return whether it was made.
 */
static bool
d_tests_arg_files_os_folder(
    const char* _name
)
{
    wchar_t* const wide = d_tests_arg_files_wide(_name);
    const bool     made = ( (wide != NULL) &&
                            (_wmkdir(wide) == 0) );

    free(wide);

    return made;
}

/**
 * @brief Opens a new file for writing bytes.
 *
 * @param[in] _name  its name, relative, UTF-8.
 * @return the stream, or NULL.
 */
static FILE*
d_tests_arg_files_os_open(
    const char* _name
)
{
    wchar_t* const wide   = d_tests_arg_files_wide(_name);
    FILE* const    stream = (wide != NULL)
                            ? _wfopen(wide,
                                      L"wb")
                            : NULL;

    free(wide);

    return stream;
}

/**
 * @brief Sets a file's read-only attribute from permission bits: read-only
 *        without the owner's write bit.
 *
 * @param[in] _name  what was made, relative, UTF-8.
 * @param[in] _mode  the bits.
 * @return whether the attribute was set.
 */
static bool
d_tests_arg_files_os_mode(
    const char*  _name,
    unsigned int _mode
)
{
    wchar_t* const wide = d_tests_arg_files_wide(_name);
    const int      bits = ((_mode & 0200u) != 0u)
                          ? (_S_IREAD | _S_IWRITE)
                          : _S_IREAD;
    const bool     set  = ( (wide != NULL) &&
                            (_wchmod(wide,
                                     bits) == 0) );

    free(wide);

    return set;
}

/**
 * @brief Removes one thing made: a directory with _wrmdir; a file with
 *        _wunlink, once it is writable, since a read-only file cannot be
 *        removed.
 *
 * @param[in] _name    its name, relative, UTF-8.
 * @param[in] _folder  whether it is a directory.
 * @return whether it was removed.
 */
static bool
d_tests_arg_files_os_remove(
    const char* _name,
    bool        _folder
)
{
    wchar_t* const wide = d_tests_arg_files_wide(_name);

    // a name the system cannot have
    if (!wide)
    {
        return false;
    }

    // a file is made writable first
    if (!_folder)
    {
        (void)_wchmod(wide,
                      _S_IREAD | _S_IWRITE);
    }

    const bool removed = (_folder)
                         ? (_wrmdir(wide) == 0)
                         : (_wunlink(wide) == 0);

    free(wide);

    return removed;
}

#else

/**
 * @brief No working directory, on a system that cannot say.
 *
 * @return NULL.
 */
static void*
d_tests_arg_files_os_home(void)
{
    return NULL;
}

/**
 * @brief No scratch directory, on a system that cannot make one.
 *
 * @return NULL.
 */
static void*
d_tests_arg_files_os_scratch(void)
{
    return NULL;
}

/**
 * @brief Enters nothing.
 *
 * @param[in] _path  unused.
 * @return false.
 */
static bool
d_tests_arg_files_os_enter(
    const void* _path
)
{
    (void)_path;

    return false;
}

/**
 * @brief Removes nothing.
 *
 * @param[in] _path  unused.
 * @return false.
 */
static bool
d_tests_arg_files_os_remove_scratch(
    const void* _path
)
{
    (void)_path;

    return false;
}

/**
 * @brief Makes nothing.
 *
 * @param[in] _name  unused.
 * @return false.
 */
static bool
d_tests_arg_files_os_folder(
    const char* _name
)
{
    (void)_name;

    return false;
}

/**
 * @brief Opens nothing.
 *
 * @param[in] _name  unused.
 * @return NULL.
 */
static FILE*
d_tests_arg_files_os_open(
    const char* _name
)
{
    (void)_name;

    return NULL;
}

/**
 * @brief Sets nothing.
 *
 * @param[in] _name  unused.
 * @param[in] _mode  unused.
 * @return false.
 */
static bool
d_tests_arg_files_os_mode(
    const char*  _name,
    unsigned int _mode
)
{
    (void)_name;
    (void)_mode;

    return false;
}

/**
 * @brief Removes nothing.
 *
 * @param[in] _name    unused.
 * @param[in] _folder  unused.
 * @return false.
 */
static bool
d_tests_arg_files_os_remove(
    const char* _name,
    bool        _folder
)
{
    (void)_name;
    (void)_folder;

    return false;
}

#endif  // the system

/**
 * @brief Notes, in the run's output, something the scratch directory could
 *        not do, and marks it failed.
 *
 * @param[in,out] _files  the scratch directory.
 * @param[in]     _what   what could not be done.
 * @param[in]     _name   to what.
 *
 * Printed as the support library prints a mismatch, so that a section that
 * fails for want of its files says so.
 */
static void
d_tests_arg_files_failed(
    struct d_tests_arg_files* _files,
    const char*               _what,
    const char*               _name
)
{
    printf("    scratch directory: could not %s %s\n",
           _what,
           _name);
    _files->failed = true;

    return;
}

/**
 * @brief Records something made, or the failure to make it.
 *
 * @param[in,out] _files   the scratch directory.
 * @param[in]     _name    its name.
 * @param[in]     _made    whether it was made.
 * @param[in]     _folder  whether it is a directory.
 * @return whether it was made and recorded.
 *
 * A thing made but not recorded -- the record full, or its name not copied
 * -- is removed again at once, so that nothing outlives the directory.
 */
static bool
d_tests_arg_files_record(
    struct d_tests_arg_files* _files,
    const char*               _name,
    bool                      _made,
    bool                      _folder
)
{
    char* const copy = ( (_made) &&
                         (_files->count < D_TESTS_ARG_FILES_MAX) )
                       ? d_tests_arg_files_copy(_name)
                       : NULL;

    // made and recorded
    if (copy != NULL)
    {
        _files->entries[_files->count].name   = copy;
        _files->entries[_files->count].folder = _folder;
        ++_files->count;

        return true;
    }

    // made, but past what the record holds
    if (_made)
    {
        (void)d_tests_arg_files_os_remove(_name,
                                          _folder);
    }

    d_tests_arg_files_failed(_files,
                             (_made) ? "keep" : "make",
                             _name);

    return false;
}

//==============================================================================
// 1.  ENTERING AND LEAVING
//==============================================================================

/*
d_tests_arg_files_enter
  The working directory is kept before the scratch directory is made, so
that a failure at any step can undo the steps before it and leave nothing.
*/
bool
d_tests_arg_files_enter(
    struct d_tests_arg_files* _files
)
{
    // parameter validation
    if (!_files)
    {
        return false;
    }

    memset(_files,
           0,
           sizeof(*_files));
    _files->home = d_tests_arg_files_os_home();
    _files->path = (_files->home != NULL)
                   ? d_tests_arg_files_os_scratch()
                   : NULL;

    // the directory made and entered
    if ( (_files->path != NULL) &&
         (d_tests_arg_files_os_enter(_files->path)) )
    {
        return true;
    }

    // a directory made but not entered is removed
    if (_files->path != NULL)
    {
        (void)d_tests_arg_files_os_remove_scratch(_files->path);
    }

    free(_files->path);
    free(_files->home);
    memset(_files,
           0,
           sizeof(*_files));
    printf("    scratch directory: could not make one to work in\n");

    return false;
}

/*
d_tests_arg_files_leave
  Everything made is removed in the reverse of the order it was made in, so
that a directory is empty by the time its turn comes; then the working
directory is restored, after which the scratch directory itself, empty,
can go.
*/
void
d_tests_arg_files_leave(
    struct d_tests_arg_files* _files
)
{
    // nothing to leave
    if ( (!_files) ||
         (!_files->path) )
    {
        return;
    }

    // each thing made, the last first
    while (_files->count > 0u)
    {
        --_files->count;

        struct d_tests_arg_files_entry* const entry =
            &_files->entries[_files->count];

        (void)d_tests_arg_files_os_remove(entry->name,
                                          entry->folder);
        free(entry->name);
    }

    (void)d_tests_arg_files_os_enter(_files->home);
    (void)d_tests_arg_files_os_remove_scratch(_files->path);
    free(_files->path);
    free(_files->home);
    memset(_files,
           0,
           sizeof(*_files));

    return;
}

//==============================================================================
// 2.  MAKING
//==============================================================================

/*
d_tests_arg_files_folder
  The directory, made and recorded.
*/
bool
d_tests_arg_files_folder(
    struct d_tests_arg_files* _files,
    const char*               _name
)
{
    // parameter validation
    if ( (!_files) ||
         (!_name) )
    {
        return false;
    }

    return d_tests_arg_files_record(_files,
                                    _name,
                                    d_tests_arg_files_os_folder(_name),
                                    true);
}

/*
d_tests_arg_files_write
  The file is written and closed before it is recorded, so that one the
record cannot hold is closed when it is removed again -- which Windows
requires -- and one whose writing failed is recorded all the same, to be
removed with the rest.
*/
bool
d_tests_arg_files_write(
    struct d_tests_arg_files* _files,
    const char*               _name,
    const void*               _data,
    size_t                    _size
)
{
    // parameter validation
    if ( (!_files)                   ||
         (!_name)                    ||
         ( (!_data) &&
           (_size > 0u) ) )
    {
        return false;
    }

    FILE* const stream  = d_tests_arg_files_os_open(_name);
    const bool  written = ( (stream != NULL) &&
                            ( (_size == 0u) ||
                              (fwrite(_data,
                                      1u,
                                      _size,
                                      stream) == _size) ) );
    const bool  closed  = ( (stream != NULL) &&
                            (fclose(stream) == 0) );
    const bool  kept    = d_tests_arg_files_record(_files,
                                                   _name,
                                                   (stream != NULL),
                                                   false);

    // every byte written and kept
    if ( (stream != NULL) &&
         ( (!written) ||
           (!closed) ) )
    {
        d_tests_arg_files_failed(_files,
                                 "write",
                                 _name);

        return false;
    }

    return kept;
}

/*
d_tests_arg_files_file
  The text's bytes, through d_tests_arg_files_write.
*/
bool
d_tests_arg_files_file(
    struct d_tests_arg_files* _files,
    const char*               _name,
    const char*               _content
)
{
    // parameter validation
    if (!_content)
    {
        return false;
    }

    return d_tests_arg_files_write(_files,
                                   _name,
                                   _content,
                                   strlen(_content));
}

/*
d_tests_arg_files_mode
  The system's primitive, its failure recorded.
*/
bool
d_tests_arg_files_mode(
    struct d_tests_arg_files* _files,
    const char*               _name,
    unsigned int              _mode
)
{
    // parameter validation
    if ( (!_files) ||
         (!_name) )
    {
        return false;
    }

    const bool set = d_tests_arg_files_os_mode(_name,
                                               _mode);

    // a mode that could not be set
    if (!set)
    {
        d_tests_arg_files_failed(_files,
                                 "set the mode of",
                                 _name);
    }

    return set;
}

#if (D_TESTS_ARG_FILES_POSIX == 1)

/*
d_tests_arg_files_link
  symlink writes the target as given, so a relative target is read from the
link's own directory.
*/
bool
d_tests_arg_files_link(
    struct d_tests_arg_files* _files,
    const char*               _name,
    const char*               _target
)
{
    // parameter validation
    if ( (!_files) ||
         (!_name)  ||
         (!_target) )
    {
        return false;
    }

    return d_tests_arg_files_record(_files,
                                    _name,
                                    (symlink(_target,
                                             _name) == 0),
                                    false);
}

/*
d_tests_arg_files_fifo
  mkfifo, readable and writable by its owner, as a file is made.
*/
bool
d_tests_arg_files_fifo(
    struct d_tests_arg_files* _files,
    const char*               _name
)
{
    // parameter validation
    if ( (!_files) ||
         (!_name) )
    {
        return false;
    }

    return d_tests_arg_files_record(_files,
                                    _name,
                                    (mkfifo(_name,
                                            0644) == 0),
                                    false);
}

#endif  // D_TESTS_ARG_FILES_POSIX
