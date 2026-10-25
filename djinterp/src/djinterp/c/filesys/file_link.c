/*******************************************************************************
* djinterp [c]                                                       file_link.c
*
* Implementation of the symbolic-link calls declared in file_link.h.
*   symlink and readlink on POSIX. On Windows, CreateSymbolicLinkA with the
* file-or-directory kind decided up front, from where the link will resolve
* its target, and readlink read from the link's own reparse data -- the text
* it holds, not the path it reaches. Windows counts a junction as a link
* (file_common.h, 6.5). The whole file compiles to nothing when this build has
* no symbolic links.
*
*
* path:      /src/djinterp/c/filesys/file_link.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_link.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL, ENAMETOOLONG
#include <stddef.h>  // NULL, size_t
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_dir.h"     // d_dir_entry_path
#include "../../../../inc/djinterp/c/filesys/file_path.h"    // d_path_dirname, d_path_root_length
#include "../../../../inc/djinterp/c/filesys/file_stat.h"    // d_dir_exists, d_file_mode_nofollow
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_link.h"  // D_INTERNAL_FILE_LINK_UNPRIV
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_FILE_HAS_SYMLINKS == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

// D_INTERNAL_LINK_SYMLINK_NAMES
//   constant: where a symbolic link's names begin in its reparse data -- after
// the tag, the data length, the four name offsets and lengths, and the flags.
#define D_INTERNAL_LINK_SYMLINK_NAMES 20

// D_INTERNAL_LINK_JUNCTION_NAMES
//   constant: where a junction's names begin: a junction has no flags word.
#define D_INTERNAL_LINK_JUNCTION_NAMES 16

/*
d_internal_link_word
  The little-endian 16-bit field at _offset of reparse data, read byte by
byte: the data carries no alignment promise for its fields.
*/
static size_t
d_internal_link_word(
    const BYTE* _data,
    size_t      _offset
)
{
    return ( (size_t)_data[_offset] |
             ((size_t)_data[_offset + 1] << 8) );
}

/*
d_internal_link_tag
  The reparse tag that opens the data, little-endian as the fields are.
*/
static DWORD
d_internal_link_tag(
    const BYTE* _data
)
{
    return ( (DWORD)_data[0]         |
             ((DWORD)_data[1] << 8)  |
             ((DWORD)_data[2] << 16) |
             ((DWORD)_data[3] << 24) );
}

/*
d_internal_link_name
  The text a link's reparse data holds: its print name -- the target as its
creator wrote it -- or, where that is empty (a volume mount point leaves it
so), its substitute name, the kernel's \??\ path, respelled as the Win32
verbatim path \\?\ it is. Every offset is checked against the bytes there
are, so damaged data is refused rather than read past. Returns 0 with the
name's extent in UTF-16 units, or -1 with EINVAL for anything but a symbolic
link or a junction.
*/
static int
d_internal_link_name(
    BYTE*         _data,
    size_t        _got,
    const WCHAR** _name,
    size_t*       _length
)
{
    const DWORD  tag   = (_got >= 4) ? d_internal_link_tag(_data) : 0;
    const size_t names = (tag == IO_REPARSE_TAG_SYMLINK)
                             ? (size_t)D_INTERNAL_LINK_SYMLINK_NAMES
                             : (size_t)D_INTERNAL_LINK_JUNCTION_NAMES;

    // only a link's data names a path, and only whole data can be read
    if ( (!d_internal_file_tag_is_link_win32(tag)) ||
         (_got < names) )
    {
        D_INTERNAL_FILE_SET_ERR(EINVAL);

        return -1;
    }

    // the print name, or the substitute where there is none
    size_t offset = d_internal_link_word(_data, 12);
    size_t length = d_internal_link_word(_data, 14);

    if (length == 0)
    {
        offset = d_internal_link_word(_data, 8);
        length = d_internal_link_word(_data, 10);
    }

    // a name outside the data, or not of whole units, is damage
    if ( ((offset % sizeof(WCHAR)) != 0) ||
         ((length % sizeof(WCHAR)) != 0) ||
         (names + offset + length > _got) )
    {
        D_INTERNAL_FILE_SET_ERR(EINVAL);

        return -1;
    }

    WCHAR* const name = (WCHAR*)(void*)(_data + names + offset);

    // the kernel's \??\ reads as \\?\ to Win32, which the caller speaks
    if ( (length >= 4 * sizeof(WCHAR)) &&
         (name[0] == L'\\')            &&
         (name[1] == L'?')             &&
         (name[2] == L'?')             &&
         (name[3] == L'\\') )
    {
        name[1] = L'\\';
    }

    *_name   = name;
    *_length = length / sizeof(WCHAR);

    return 0;
}

// D_INTERNAL_LINK_OPEN
//   macro: CreateFile in the form the build's path text takes: the W
// function where the build hands Windows UTF-16 (D_INTERNAL_FILE_WIDE).
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_LINK_OPEN CreateFileW
#else
    #define D_INTERNAL_LINK_OPEN CreateFileA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_LINK_MAKE
//   macro: CreateSymbolicLink, the W or the A function.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_LINK_MAKE CreateSymbolicLinkW
#else
    #define D_INTERNAL_LINK_MAKE CreateSymbolicLinkA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_LINK_CODEPAGE, D_INTERNAL_LINK_CP_FLAGS, D_INTERNAL_LINK_CP_ERROR
//   macros: how a build that speaks the code page gives a link's UTF-16 name
// back: in the ANSI code page the A calls speak, with no refusal to ask for,
// and EIO for a name outside it, whose text would not be the link's. A build
// that hands Windows UTF-16 spells the name in WTF-8 instead, which has a
// spelling for every name (d_internal_file_utf16_to_wtf8).
#define D_INTERNAL_LINK_CODEPAGE CP_ACP
#define D_INTERNAL_LINK_CP_FLAGS 0
#define D_INTERNAL_LINK_CP_ERROR EIO

/*
d_internal_link_convert
  A link's UTF-16 name in the text this build's paths are: UTF-8 where it
hands Windows UTF-16, the ANSI code page the -A calls speak otherwise
(D_INTERNAL_LINK_CODEPAGE), in a block the caller frees, its length in _made.
UTF-8 here is WTF-8, which spells a lone surrogate too, so only the code page
refuses a name: EIO for one outside it, whose text would not be the link's.
*/
static char*
d_internal_link_convert(
    const WCHAR* _name,
    size_t       _length,
    int*         _made
)
{
#if (D_INTERNAL_FILE_WIDE == 1)
    size_t need = 0;

    // every sequence of units has a WTF-8 spelling: measure, then write
    (void)d_internal_file_utf16_to_wtf8(_name,
                                        _length,
                                        NULL,
                                        0,
                                        &need);

    char* const text = (char*)d_internal_file_alloc(need + 1);

    // d_internal_file_alloc has already reported the failure
    if (!text)
    {
        return NULL;
    }

    (void)d_internal_file_utf16_to_wtf8(_name,
                                        _length,
                                        text,
                                        need + 1,
                                        NULL);
    *_made = (int)need;

    return text;
#else
    const int need = WideCharToMultiByte(D_INTERNAL_LINK_CODEPAGE,
                                         D_INTERNAL_LINK_CP_FLAGS,
                                         _name,
                                         (int)_length,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL);

    // a name that will not convert has no text to give
    if (need <= 0)
    {
        D_INTERNAL_FILE_SET_ERR(D_INTERNAL_LINK_CP_ERROR);

        return NULL;
    }

    char* const text = (char*)d_internal_file_alloc((size_t)need);

    // d_internal_file_alloc has already reported the failure
    if (!text)
    {
        return NULL;
    }

    *_made = WideCharToMultiByte(D_INTERNAL_LINK_CODEPAGE,
                                 D_INTERNAL_LINK_CP_FLAGS,
                                 _name,
                                 (int)_length,
                                 text,
                                 need,
                                 NULL,
                                 NULL);

    // measured, and still refused
    if (*_made <= 0)
    {
        d_internal_file_free(text);
        D_INTERNAL_FILE_SET_ERR(D_INTERNAL_LINK_CP_ERROR);

        return NULL;
    }

    return text;
#endif  // D_INTERNAL_FILE_WIDE == 1
}

/*
d_internal_link_narrow
  A link's name as the caller reads it (d_internal_link_convert), truncated to
_bufsize as readlink truncates, and unterminated as readlink leaves it.
*/
static ssize_t
d_internal_link_narrow(
    const WCHAR* _name,
    size_t       _length,
    char*        _buf,
    size_t       _bufsize
)
{
    int made = 0;

    // an empty name narrows to nothing
    if (_length == 0)
    {
        return 0;
    }

    char* const text = d_internal_link_convert(_name,
                                               _length,
                                               &made);

    // d_internal_link_convert has already set errno
    if (!text)
    {
        return -1;
    }

    const size_t keep = ((size_t)made < _bufsize) ? (size_t)made : _bufsize;

    memcpy(_buf,
           text,
           keep);
    d_internal_file_free(text);

    return (ssize_t)keep;
}

/*
d_internal_link_fetch
  A link's reparse data, from the link itself -- opened without following it,
so a dangling link reads as well as any -- into _data, which holds
MAXIMUM_REPARSE_DATA_BUFFER_SIZE bytes. Returns the bytes read, or 0 with
errno set: a path that is no reparse point at all is EINVAL, as readlink(2)
reports a file that is no link.
*/
static DWORD
d_internal_link_fetch(
    const char* _path,
    BYTE*       _data
)
{
    struct d_internal_file_native        held = {0};
    const d_internal_file_nchar_t* const path =
        d_internal_file_native_of(&held,
                                  _path);

    // the conversion has set errno
    if (!path)
    {
        return 0;
    }

    const HANDLE handle = D_INTERNAL_LINK_OPEN(path,
                                               0,
                                               FILE_SHARE_READ  |
                                               FILE_SHARE_WRITE |
                                               FILE_SHARE_DELETE,
                                               NULL,
                                               OPEN_EXISTING,
                                               FILE_FLAG_OPEN_REPARSE_POINT |
                                               FILE_FLAG_BACKUP_SEMANTICS,
                                               NULL);

    d_internal_file_native_release(&held);

    DWORD got = 0;

    // a link that will not open cannot be read
    if (handle == INVALID_HANDLE_VALUE)
    {
        const int code = d_internal_file_errno_win32(GetLastError());

        D_INTERNAL_FILE_SET_ERR(code);
        (void)code;

        return 0;
    }

    // the data, or the reason there is none
    if (!DeviceIoControl(handle,
                         FSCTL_GET_REPARSE_POINT,
                         NULL,
                         0,
                         _data,
                         (DWORD)MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
                         &got,
                         NULL))
    {
        const int code = d_internal_file_errno_win32(GetLastError());

        got = 0;
        D_INTERNAL_FILE_SET_ERR(code);
        (void)code;
    }

    CloseHandle(handle);

    return got;
}

/*
d_internal_link_read
  readlink for Windows: the reparse data (d_internal_link_fetch), the name it
holds (d_internal_link_name), and that name narrowed for the caller
(d_internal_link_narrow).
*/
static ssize_t
d_internal_link_read(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    BYTE* const data =
        (BYTE*)d_internal_file_alloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

    // d_internal_file_alloc has already reported the failure
    if (!data)
    {
        return -1;
    }

    const WCHAR*  name   = NULL;
    size_t        length = 0;
    ssize_t       result = -1;
    const DWORD   got    = d_internal_link_fetch(_path,
                                                 data);

    // the name the data holds, narrowed as the caller reads it
    if ( (got != 0) &&
         (d_internal_link_name(data,
                               got,
                               &name,
                               &length) == 0) )
    {
        result = d_internal_link_narrow(name,
                                        length,
                                        _buf,
                                        _bufsize);
    }

    const int saved_errno = errno;

    d_internal_file_free(data);
    errno = saved_errno;

    return result;
}

/*
d_internal_link_target
  A target as Windows must store it: with backslashes, since a relative
target written with forward slashes is stored as written and then not
followed. Returns 0, or -1 with ENAMETOOLONG when it does not fit _bufsize.
*/
static int
d_internal_link_target(
    const char* _target,
    char*       _buf,
    size_t      _bufsize
)
{
    const size_t length = strlen(_target);
    size_t       i      = 0;

    // the target and its terminator must fit
    if (length >= _bufsize)
    {
        D_INTERNAL_FILE_SET_ERR(ENAMETOOLONG);

        return -1;
    }

    // the terminator comes along
    for (i = 0; i <= length; ++i)
    {
        _buf[i] = (_target[i] == '/') ? '\\' : _target[i];
    }

    return 0;
}

/*
d_internal_link_is_dir
  Whether a target is a directory, as Windows must know when it makes the
link -- judged where the link will resolve it: a relative target from the
link's own directory, not from the working directory. A target that cannot be
judged is no directory, as a dangling one is not.
*/
static int
d_internal_link_is_dir_in(
    const char* _target,
    const char* _linkpath,
    char*       _dir_room,
    char*       _full_room
)
{
    char* const dir = _dir_room;
    char* const full = _full_room;

    // a rooted target names the same place from anywhere
    if (d_path_root_length(_target) != 0)
    {
        return d_dir_exists(_target);
    }

    // a relative one is read from the link's directory
    if ( (!d_path_dirname(_linkpath,
                          dir,
                          (D_FILE_PATH_MAX + 1))) ||
         (!d_dir_entry_path(full,
                            (D_FILE_PATH_MAX + 1),
                            dir,
                            _target)) )
    {
        return 0;
    }

    return d_dir_exists(full);
}

/*
d_internal_link_is_dir
  d_internal_link_is_dir's work is d_internal_link_is_dir_in; this keeps the
room it builds a path in, on the heap where that is 32 KiB
(d_internal_file_path).
*/
static int
d_internal_link_is_dir(
    const char* _target,
    const char* _linkpath
)
{
    struct d_internal_file_path dir_held  = {0};
    struct d_internal_file_path full_held = {0};
    char* const dir = d_internal_file_path_room(&dir_held);
    char* const full = d_internal_file_path_room(&full_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (( (!dir) ||
         (!full) ))
    {
        d_internal_file_path_release(&dir_held);
        d_internal_file_path_release(&full_held);

        return -1;
    }

    const int result = d_internal_link_is_dir_in(_target,
                                                 _linkpath,
                                                 dir,
                                                 full);

    d_internal_file_path_release(&dir_held);
    d_internal_file_path_release(&full_held);

    return result;
}

/*
d_internal_link_make
  CreateSymbolicLink, given the platform's text, with the unprivileged-create
flag where the build asks
for it -- and again without it when the system rejects the flag itself, as
Windows 10 before 1703 rejects the whole call rather than ignore it. Returns
the call's answer; GetLastError says why it failed.
*/
static BOOL
d_internal_link_make(
    const d_internal_file_nchar_t* _linkpath,
    const d_internal_file_nchar_t* _target,
    DWORD                          _flags
)
{
    // the plain call, where the build does not ask for the flag
    if (D_INTERNAL_FILE_LINK_UNPRIV == 0)
    {
        return D_INTERNAL_LINK_MAKE(_linkpath,
                                    _target,
                                    _flags);
    }

    // Developer Mode lets an ordinary account create links
    if (D_INTERNAL_LINK_MAKE(_linkpath,
                             _target,
                             _flags |
                                 SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
    {
        return TRUE;
    }

    // anything but a refusal of the flag itself is the answer
    if (GetLastError() != ERROR_INVALID_PARAMETER)
    {
        return FALSE;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                           0,
                           "d_file_symlink",
                           NULL,
                           "unprivileged-create flag rejected; retrying "
                           "without it");

    // the plain call needs the privilege itself
    return D_INTERNAL_LINK_MAKE(_linkpath,
                                _target,
                                _flags);
}

/*
d_internal_link_create
  symlink for Windows. Win32 bakes file-versus-directory into the link at
creation and cannot revise it, so the target is examined first, where the
link will resolve it (d_internal_link_is_dir); a dangling target makes a file
link. A failure is the reason Win32 gives -- EPERM for the missing privilege,
EEXIST for a name already taken, and so on.
*/
static int
d_internal_link_create_in(
    const char* _target,
    const char* _linkpath,
    char*       _target_room
)
{
    char* const target = _target_room;

    // the target as Windows must store it
    if (d_internal_link_target(_target,
                               target,
                               (D_FILE_PATH_MAX + 1)) != 0)
    {
        return -1;
    }

    const int is_dir = d_internal_link_is_dir(target,
                                              _linkpath);

    // d_internal_file_path_room has set errno
    if (is_dir < 0)
    {
        return -1;
    }

    const DWORD flags = (is_dir) ? (DWORD)SYMBOLIC_LINK_FLAG_DIRECTORY
                                 : (DWORD)0;

    struct d_internal_file_native        link_held = {0};
    struct d_internal_file_native        text_held = {0};
    const d_internal_file_nchar_t* const link =
        d_internal_file_native_of(&link_held,
                                  _linkpath);
    const d_internal_file_nchar_t* const text =
        (link) ? d_internal_file_native_of(&text_held,
                                           target)
               : NULL;

    // the conversion has set errno
    if (!text)
    {
        d_internal_file_native_release(&link_held);

        return -1;
    }

    const BOOL made = d_internal_link_make(link,
                                           text,
                                           flags);

    d_internal_file_native_release(&link_held);
    d_internal_file_native_release(&text_held);

    // made, or refused for a reason Win32 names
    if (made)
    {
        return 0;
    }

    const DWORD error = GetLastError();

    // a filesystem that holds no links -- FAT, most shares -- is EPERM, as
    // symlink(2) names it, not the table's general ENOTSUP
    const int code = ( (error == ERROR_INVALID_FUNCTION) ||
                       (error == ERROR_NOT_SUPPORTED) )
                         ? EPERM
                         : d_internal_file_errno_win32(error);

    D_INTERNAL_FILE_SET_ERR(code);
    (void)code;

    return -1;
}

/*
d_internal_link_create
  d_internal_link_create's work is d_internal_link_create_in; this keeps the
room it builds a path in, on the heap where that is 32 KiB
(d_internal_file_path).
*/
static int
d_internal_link_create(
    const char* _target,
    const char* _linkpath
)
{
    struct d_internal_file_path target_held = {0};
    char* const target = d_internal_file_path_room(&target_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (!target)
    {
        d_internal_file_path_release(&target_held);

        return -1;
    }

    const int result = d_internal_link_create_in(_target,
                                                 _linkpath,
                                                 target);

    d_internal_file_path_release(&target_held);

    return result;
}

#else

/*
d_internal_link_create
  symlink(2): a link is text, and its kind is a question for read time, so
nothing about the target is asked.
*/
static int
d_internal_link_create(
    const char* _target,
    const char* _linkpath
)
{
    return symlink(_target,
                   _linkpath);
}

/*
d_internal_link_read
  readlink(2), as it is.
*/
static ssize_t
d_internal_link_read(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    return readlink(_path,
                    _buf,
                    _bufsize);
}

#endif  // D_CFG_FILE_HAS_WIN32

//==============================================================================
// 1.  SYMBOLIC LINKS
//==============================================================================

/*
d_file_symlink
  The platform's creation (d_internal_link_create), and the failure's notice.
*/
int
d_file_symlink(
    const char* _target,
    const char* _linkpath
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_target != NULL,
                            EINVAL,
                            "d_file_symlink",
                            NULL,
                            "target is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_linkpath != NULL,
                            EINVAL,
                            "d_file_symlink",
                            _target,
                            "link path is NULL",
                            -1);

    // report the failure; errno is the platform's
    if (d_internal_link_create(_target,
                               _linkpath) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_symlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_linkpath),
                               "symlink failed");

        return -1;
    }

    return 0;
}

/*
d_file_readlink
  The platform's reading (d_internal_link_read), and the failure's notice.
*/
ssize_t
d_file_readlink(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_readlink",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_readlink",
                            _path,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_file_readlink",
                            _path,
                            "buffer size is 0",
                            -1);

    const ssize_t result = d_internal_link_read(_path,
                                                _buf,
                                                _bufsize);

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_readlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "readlink failed");

        return -1;
    }

    return result;
}

/*
d_file_is_symlink
  d_file_mode_nofollow, necessarily: a following query describes the target,
so it could never report a link at all. It describes a link as one on every
platform -- on Windows a junction too -- needs no width, and raises no notice
for a path that is not there.
*/
int
d_file_is_symlink(
    const char* _path
)
{
    uint32_t mode = 0;

    // a NULL path is not a symlink; "no" is a meaningful answer to a yes/no
    if (!_path)
    {
        return 0;
    }

    // a path that cannot be examined is not reported as a link
    if (d_file_mode_nofollow(_path,
                             &mode) != 0)
    {
        return 0;
    }

    return (S_ISLNK(mode) != 0);
}

#endif  // D_INTERNAL_FILE_HAS_SYMLINKS
