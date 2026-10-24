/*******************************************************************************
* djinterp [parse]                                                      cli_io.c
*
* The command-line kit's streams, terminals and files, declared in cli_io.h.
*   The sinks hand their bytes to cli_system.c, which knows a console from a
* file. Colour is decided here from `--color`'s mode and the environment, and
* the terminal is asked to take ANSI sequences only when it would be
* coloured. Files go through djinterp's filesys, whose calls take UTF-8 names
* on every system; a file is replaced as filesys's own atomic write replaces
* one -- a sibling made exclusively with the mode it is to have, the old
* file's permission bits or a new file's, filled, synced and renamed over it
* -- so the replacement does not depend on the build's
* D_CFG_FILE_WRITE_ATOMIC. The sibling is made by the call filesys makes its
* own with (d_internal_file_temp_create), which alone creates one with a
* mode under the umask, as a new file must be. A path that is no file -- a
* device, a pipe, a socket -- is written to, since renaming over one would
* put a file where the device was.
*
*
* path:      /src/djinterp/parse/cli/c/cli_io.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../../../inc/djinterp/parse/cli/c/cli_io.h"  // corresponding header
// std
#include <errno.h>    // errno, EINVAL, EIO, EISDIR, ENOMEM, ENAMETOOLONG
#include <fcntl.h>    // O_WRONLY
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // stdin
#include <stdlib.h>   // strtol, and the hooks' malloc, realloc and free
#include <string.h>   // memcpy, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/filesys/file_desc.h"  // d_file_open, d_file_close_fd
#include "../../../../../inc/djinterp/c/filesys/file_dir.h"  // d_path_resolve
#include "../../../../../inc/djinterp/c/filesys/file_io.h"  // d_file_read_all, d_file_write_full_fd
#include "../../../../../inc/djinterp/c/filesys/file_link.h"  // d_file_is_symlink
#include "../../../../../inc/djinterp/c/filesys/file_ops.h"  // d_file_rename, d_file_unlink
#include "../../../../../inc/djinterp/c/filesys/file_stat.h"  // d_file_mode, d_file_chmod
#include "../../../../../inc/djinterp/c/filesys/file_sync.h"  // d_file_sync_fd
#include "../../../../../inc/djinterp/c/filesys/file_temp.h"  // d_internal_file_temp_create
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_sink
#include "../../../../../inc/djinterp/config/c/filesys/cfg_file_io.h"  // D_CFG_FILE_READ_NUL_TERMINATE, D_INTERNAL_FILE_WRITE_MODE
#include "./cli_internal.h"  // the system's answers
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  STREAMS
//==============================================================================

// 1.1    The standard sinks
//------------------------------------------------------------------------------

/**
 * @brief The sinks' write function: the stream is the context's address,
 *        cast back to a d_cli_stream value.
 *
 * @param[in] _context  the stream, as a pointer-sized integer.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return how many the stream took.
 */
static size_t
d_cli_internal_sink_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    const int stream = (_context != NULL)
                       ? (int)D_CLI_STREAM_ERROR
                       : (int)D_CLI_STREAM_OUTPUT;

    return d_cli_internal_write_stream(stream,
                                       _data,
                                       _size);
}

/*
d_cli_sink_output
  The context says which stream: NULL for standard output.
*/
struct d_pack_sink
d_cli_sink_output(void)
{
    struct d_pack_sink sink;

    sink.write   = d_cli_internal_sink_write;
    sink.context = NULL;

    return sink;
}

/*
d_cli_sink_error
  Any context that is not NULL is standard error: the sink's own address.
*/
struct d_pack_sink
d_cli_sink_error(void)
{
    static const char  error_stream = 'e';
    struct d_pack_sink sink;

    sink.write   = d_cli_internal_sink_write;
    sink.context = (void*)&error_stream;

    return sink;
}

// 1.2    Terminals
//------------------------------------------------------------------------------

/*
d_cli_is_terminal
  The system's answer.
*/
bool
d_cli_is_terminal(
    int _stream
)
{
    return d_cli_internal_is_terminal(_stream);
}

/*
d_cli_terminal_columns
  The terminal's own width, else COLUMNS, as the argument parser measures.
*/
size_t
d_cli_terminal_columns(void)
{
    const size_t columns = d_cli_internal_terminal_columns();

    // the terminal said
    if (columns > 0u)
    {
        return columns;
    }

    char*       text  = NULL;
    char*       end   = NULL;

    // a width there was no room to read is no width
    (void)d_cli_internal_getenv("COLUMNS",
                                &text);

    const long  value = (text != NULL)
                        ? strtol(text,
                                 &end,
                                 10)
                        : 0L;
    const bool  whole = ( (text != NULL)  &&
                          (end != text)   &&
                          (*end == '\0') );

    D_INTERNAL_CLI_FREE(text);

    return ( (whole)        &&
             (value >= 1L)  &&
             (value <= 10000L) )
           ? (size_t)value
           : 0u;
}

/*
d_cli_colour_wanted
  The mode first; then, for auto, every condition no-color.org and TERM=dumb
set, and last the terminal itself, which is asked to take sequences only
when everything else says yes.
*/
bool
d_cli_colour_wanted(
    int _stream,
    int _mode
)
{
#if (D_INTERNAL_CLI_COLOUR == 1)
    // asked outright
    if (_mode == (int)D_CLI_COLOUR_ALWAYS)
    {
        return true;
    }

    if (_mode == (int)D_CLI_COLOUR_NEVER)
    {
        return false;
    }

    // the standard streams that are written only
    if ( (_stream != (int)D_CLI_STREAM_OUTPUT) &&
         (_stream != (int)D_CLI_STREAM_ERROR) )
    {
        return false;
    }

    char*       no_colour = NULL;
    char*       terminal  = NULL;
    const int   unread    = d_cli_internal_getenv("NO_COLOR",
                                                  &no_colour) +
                            d_cli_internal_getenv("TERM",
                                                  &terminal);

    // what could not be read refuses colour, as NO_COLOR might have
    const bool  refused   = ( (unread != 0)                ||
                              ( (no_colour != NULL) &&
                                (no_colour[0] != '\0') )   ||
                              ( (terminal != NULL) &&
                                (strcmp(terminal,
                                        "dumb") == 0) ) );

    D_INTERNAL_CLI_FREE(no_colour);
    D_INTERNAL_CLI_FREE(terminal);

    return ( (!refused) &&
             (d_cli_internal_is_terminal(_stream)) &&
             (d_cli_internal_take_sequences(_stream)) );
#else
    (void)_stream;
    (void)_mode;

    return false;
#endif
}


//==============================================================================
// 2.  FILES
//==============================================================================

// 2.1    Names
//------------------------------------------------------------------------------

/*
d_cli_is_standard
  Exactly "-".
*/
bool
d_cli_is_standard(
    const char* _path
)
{
    return ( (_path != NULL)   &&
             (_path[0] == '-') &&
             (_path[1] == '\0') );
}

// 2.2    Reading
//------------------------------------------------------------------------------

/**
 * @brief Gives a buffer filesys read the NUL the kit promises, where the
 *        build's filesys does not add one.
 *
 * @param[in,out] _data    the buffer, which may be replaced.
 * @param[in]     _length  its payload's length.
 * @return 0, or ENOMEM, the buffer then freed.
 */
static int
d_cli_internal_terminate(
    char** _data,
    size_t _length
)
{
#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    (void)_data;
    (void)_length;

    return 0;
#else
    char* const grown = (char*)D_INTERNAL_CLI_REALLOC(*_data,
                                                      _length + 1u);

    // no room for the terminator
    if (!grown)
    {
        D_INTERNAL_CLI_FREE(*_data);
        *_data = NULL;

        return ENOMEM;
    }

    grown[_length] = '\0';
    *_data         = grown;

    return 0;
#endif
}

/*
d_cli_read_file
  filesys reads a file in binary mode, or standard input to its end, in one
allocation. A directory is refused by its mode before it is opened, so the
error is EISDIR on every system rather than whatever reading one gives.
*/
int
d_cli_read_file(
    const char* _path,
    char**      _data,
    size_t*     _length
)
{
    // parameter validation
    if ( (!_path) ||
         (!_data) ||
         (!_length) )
    {
        return EINVAL;
    }

    *_data   = NULL;
    *_length = 0u;

    size_t size = 0u;
    void*  read = NULL;

    // standard input, in binary mode
    if (d_cli_is_standard(_path))
    {
        d_cli_internal_binary_streams();
        errno = 0;
        read  = d_file_read_all_stream(stdin,
                                       &size);
    }
    else
    {
        uint32_t mode = 0u;

        // a directory is no file to read
        if ( (d_file_mode(_path,
                          &mode) == 0) &&
             (d_cli_internal_mode_kind(mode) == 2) )
        {
            return EISDIR;
        }

        errno = 0;
        read  = d_file_read_all(_path,
                                &size);
    }

    // filesys has set errno
    if (!read)
    {
        return (errno != 0) ? errno : EIO;
    }

    *_data = (char*)read;

    const int terminated = d_cli_internal_terminate(_data,
                                                    size);

    if (terminated != 0)
    {
        return terminated;
    }

    *_length = size;

    return 0;
}

// 2.3    Writing
//------------------------------------------------------------------------------

/**
 * @brief The name a replacement is made at: the file's own, `.~`, and the
 *        six `X`s d_file_temp_create replaces.
 *
 * @param[in] _path  the file's name.
 * @return the template, to be freed with free; NULL when there is no room.
 */
static char*
d_cli_internal_template(
    const char* _path
)
{
    static const char suffix[] = ".~XXXXXX";
    const size_t      length   = strlen(_path);
    const size_t      size     = length + sizeof(suffix);
    char* const       name     = (char*)D_INTERNAL_CLI_MALLOC(size);

    // no room
    if (!name)
    {
        return NULL;
    }

    memcpy(name,
           _path,
           length);
    memcpy(name + length,
           suffix,
           sizeof(suffix));

    return name;
}

/**
 * @brief Fills, syncs and closes a replacement; on failure removes it too.
 *
 * @param[in] _fd    the replacement's descriptor.
 * @param[in] _name  its name.
 * @param[in] _data  the bytes.
 * @param[in] _size  how many.
 * @return 0, or an errno value, the replacement then gone.
 */
static int
d_cli_internal_fill(
    int         _fd,
    const char* _name,
    const void* _data,
    size_t      _size
)
{
    int result = 0;

    // every byte, then on to the disk before it may be renamed
    if ( ( (_size > 0u) &&
           (d_file_write_full_fd(_fd,
                                 _data,
                                 _size) != (ssize_t)_size) ) ||
         (d_file_sync_fd(_fd) != 0) )
    {
        result = (errno != 0) ? errno : EIO;
    }

    // the close reports a failure the writes deferred
    if ( (d_file_close_fd(_fd) != 0) &&
         (result == 0) )
    {
        result = (errno != 0) ? errno : EIO;
    }

    if (result != 0)
    {
        (void)d_file_unlink(_name);
    }

    return result;
}

/**
 * @brief Writes to what no file can stand in for: a device, a pipe, a
 *        socket, opened as it is and written whole.
 *
 * @param[in] _path  its name.
 * @param[in] _data  the bytes.
 * @param[in] _size  how many.
 * @return 0, or an errno value.
 */
static int
d_cli_internal_write_through(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    errno = 0;

    const int fd = d_file_open(_path,
                               O_WRONLY);

    if (fd < 0)
    {
        return (errno != 0) ? errno : EIO;
    }

    int result = ( (_size == 0u) ||
                   (d_file_write_full_fd(fd,
                                         _data,
                                         _size) == (ssize_t)_size) )
                 ? 0
                 : ( (errno != 0) ? errno : EIO );

    // the close reports a failure the writes deferred
    if ( (d_file_close_fd(fd) != 0) &&
         (result == 0) )
    {
        result = (errno != 0) ? errno : EIO;
    }

    return result;
}

/**
 * @brief Replaces a file: a sibling made exclusively with the mode it is to
 *        have -- owner-only until the old file's bits are given it, or a new
 *        file's under the umask -- filled, synced, and renamed over it.
 *
 * @param[in] _path  the file, which need not exist; a regular file, never a
 *                   link.
 * @param[in] _data  the bytes.
 * @param[in] _size  how many.
 * @return 0, or an errno value, the file as it was.
 */
static int
d_cli_internal_replace(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    uint32_t    mode = 0u;
    const bool  kept = (d_file_mode(_path,
                                    &mode) == 0);
    char* const name = d_cli_internal_template(_path);

    if (!name)
    {
        return ENOMEM;
    }

    errno = 0;

    const int fd = d_internal_file_temp_create(
                       name,
                       kept ? 0600 : (int)D_INTERNAL_FILE_WRITE_MODE);

    // no sibling could be made: the directory, or the name
    if (fd < 0)
    {
        const int error = (errno != 0) ? errno : EIO;

        D_INTERNAL_CLI_FREE(name);

        return error;
    }

    int result = d_cli_internal_fill(fd,
                                     name,
                                     _data,
                                     _size);

    // the old file's permission bits, exactly; a refusal leaves the
    // replacement owner-only, narrower and never wider
    if ( (result == 0) &&
         (kept) )
    {
        (void)d_file_chmod(name,
                           mode & 07777u);
    }

    // the one step a reader can see
    if ( (result == 0) &&
         (d_file_rename(name,
                        _path,
                        1) != 0) )
    {
        result = (errno != 0) ? errno : EIO;
        (void)d_file_unlink(name);
    }

    D_INTERNAL_CLI_FREE(name);

    return result;
}

/**
 * @brief Writes standard output through the kit's sink, whole.
 *
 * @param[in] _data    the bytes.
 * @param[in] _length  how many.
 * @return 0, or EIO when the sink took fewer.
 */
static int
d_cli_internal_write_output(
    const void* _data,
    size_t      _length
)
{
    const struct d_pack_sink sink = d_cli_sink_output();

    return ( (_length == 0u) ||
             (sink.write(sink.context,
                         _data,
                         _length) == _length) )
           ? 0
           : EIO;
}

/**
 * @brief Where a write to a name goes: the file a symbolic link leads to,
 *        or the name itself -- one that is no link, or a link that leads
 *        nowhere, which is replaced as a file would be.
 *
 * @param[in]  _path    the name.
 * @param[out] _target  receives the file a link leads to, to be freed with
 *                      D_INTERNAL_CLI_FREE; NULL where the name itself is
 *                      written.
 * @return 0; or the reason a link could not be followed, for any reason but
 *         that it leads nowhere -- memory, permission, a loop -- so that a
 *         link is never replaced by a file for want of a few bytes.
 */
static int
d_cli_internal_link_target(
    const char* _path,
    char**      _target
)
{
    *_target = NULL;

    // no link: the name itself
    if (d_file_is_symlink(_path) == 0)
    {
        return 0;
    }

    *_target = d_path_resolve(_path,
                              NULL,
                              0u);

    const int reason = errno;

    // followed; or leading nowhere
    if ( (*_target != NULL) ||
         (reason == ENOENT) )
    {
        return 0;
    }

    return (reason != 0) ? reason : EIO;
}

/*
d_cli_write_file
  Standard output through the kit's sink. Otherwise the path is asked what it
is, through any link: a directory is refused; what is no file is written
to; a file -- or nothing yet -- is replaced, through a link to where it
points: d_path_resolve follows every link on the way, and the file there
must exist, which it does when the link is one to follow. A link it cannot
follow for any other reason than that it leads nowhere -- memory,
permission, a loop -- fails the write with that reason, so that the link is
never replaced by a file for want of a few bytes.
*/
int
d_cli_write_file(
    const char* _path,
    const void* _data,
    size_t      _length
)
{
    // parameter validation
    if ( (!_path) ||
         ( (!_data) &&
           (_length > 0u) ) )
    {
        return EINVAL;
    }

    // standard output
    if (d_cli_is_standard(_path))
    {
        return d_cli_internal_write_output(_data,
                                           _length);
    }

    uint32_t  mode = 0u;
    const int kind = (d_file_mode(_path,
                                  &mode) == 0)
                     ? d_cli_internal_mode_kind(mode)
                     : 1;

    // a directory is no file to write
    if (kind == 2)
    {
        return EISDIR;
    }

    // a device, a pipe, a socket: nothing can be renamed over it
    if (kind == 0)
    {
        return d_cli_internal_write_through(_path,
                                            _data,
                                            _length);
    }

    char*     target = NULL;
    const int reason = d_cli_internal_link_target(_path,
                                                  &target);

    // a link it could not follow
    if (reason != 0)
    {
        return reason;
    }

    // a link: replace the file it leads to, and keep the link; any other
    // name, a link that leads nowhere among them, as a file
    const int result = d_cli_internal_replace((target != NULL) ? target
                                                               : _path,
                                              _data,
                                              _length);

    D_INTERNAL_CLI_FREE(target);

    return result;
}

// 2.4    Memory
//------------------------------------------------------------------------------

/*
d_cli_free
  Every block the kit returns was allocated through filesys's hooks
(D_INTERNAL_CLI_MALLOC), and is freed through them here, in the kit's own
module, so a caller across a plugin's boundary need not share a runtime
with it.
*/
void
d_cli_free(
    void* _memory
)
{
    D_INTERNAL_CLI_FREE(_memory);

    return;
}
