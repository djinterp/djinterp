/******************************************************************************
* djinterp [core]                                                      dfile.h
*
* Cross-platform variants of file I/O functions.
*   This header provides portable implementations of file operations that are
* not consistently available across all platforms. It includes secure file
* opening, large file support, file locking, directory operations, path
* manipulation, temporary files, and file metadata functions.
*   Native implementations are used whenever available, with fallback
* implementations for platforms that lack them.
*
* path:      \inc\dfile.h                                  
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PLATFORM DETECTION AND INCLUDES
      --------------------------------
      1.  Standard headers
      2.  Platform-specific headers
      3.  Feature detection macros

II.   TYPE DEFINITIONS
      -----------------
      1.  d_off_t       (64-bit file offset type)
      2.  d_stat_t      (file status structure)
      3.  d_dir_t       (directory handle)
      4.  d_dirent_t    (directory entry)
      5.  File mode constants
      6.  File type constants

III.  SECURE FILE OPENING
      --------------------
      1.  d_fopen       (portable fopen)
      2.  d_fopen_s     (C11 fopen_s equivalent)
      3.  d_freopen     (portable freopen)
      4.  d_freopen_s   (C11 freopen_s equivalent)
      5.  d_fdopen      (POSIX fdopen equivalent)

IV.   LARGE FILE SUPPORT
      -------------------
      1.  d_fseeko      (POSIX fseeko / Windows _fseeki64 equivalent)
      2.  d_ftello      (POSIX ftello / Windows _ftelli64 equivalent)
      3.  d_ftruncate   (POSIX ftruncate equivalent)

V.    FILE DESCRIPTOR OPERATIONS
      --------------------------
      1.  d_fileno      (POSIX fileno / Windows _fileno equivalent)
      2.  d_dup         (POSIX dup equivalent)
      3.  d_dup2        (POSIX dup2 equivalent)
      4.  d_close       (POSIX close equivalent)
      5.  d_read        (POSIX read equivalent)
      6.  d_write       (POSIX write equivalent)

VI.   FILE SYNCHRONIZATION
      ----------------------
      1.  d_fsync       (POSIX fsync / Windows _commit equivalent)
      2.  d_fflush      (portable fflush wrapper)

VII.  FILE LOCKING
      -------------
      1.  d_flock       (advisory file locking)
      2.  d_funlock     (release file lock)
      3.  d_ftrylockf   (non-blocking lock attempt)

VIII. TEMPORARY FILES
      ----------------
      1.  d_tmpfile     (portable tmpfile)
      2.  d_tmpfile_s   (C11 tmpfile_s equivalent)
      3.  d_mkstemp     (POSIX mkstemp equivalent)
      4.  d_tmpnam_s    (C11 tmpnam_s equivalent)
      5.  d_tempdir     (get system temp directory)

IX.   FILE METADATA
      --------------
      1.  d_stat        (POSIX stat equivalent)
      2.  d_fstat       (POSIX fstat equivalent)
      3.  d_lstat       (POSIX lstat equivalent - no symlink follow)
      4.  d_access      (POSIX access equivalent)
      5.  d_chmod       (POSIX chmod equivalent)
      6.  d_file_size   (convenience: get file size)
      7.  d_file_exists (convenience: check if file exists)
      8.  d_is_file     (convenience: check if path is regular file)
      9.  d_is_dir      (convenience: check if path is directory)

X.    DIRECTORY OPERATIONS
      ---------------------
      1.  d_mkdir       (POSIX mkdir equivalent)
      2.  d_rmdir       (POSIX rmdir equivalent)
      3.  d_opendir     (POSIX opendir equivalent)
      4.  d_readdir     (POSIX readdir equivalent)
      5.  d_closedir    (POSIX closedir equivalent)
      6.  d_rewinddir   (POSIX rewinddir equivalent)

XI.   FILE OPERATIONS
      ----------------
      1.  d_remove      (portable remove)
      2.  d_unlink      (POSIX unlink equivalent)
      3.  d_rename      (portable rename with overwrite control)
      4.  d_copy_file   (copy file contents)

XII.  PATH UTILITIES
      ---------------
      1.  d_getcwd      (POSIX getcwd equivalent)
      2.  d_chdir       (POSIX chdir equivalent)
      3.  d_realpath    (POSIX realpath equivalent)
      4.  d_dirname     (extract directory from path)
      5.  d_basename    (extract filename from path)
      6.  d_path_join   (join path components)
      7.  d_path_exists (check if path exists)

XIII. SYMBOLIC LINKS (where supported)
      ---------------------------------
      1.  d_symlink     (create symbolic link)
      2.  d_readlink    (read symbolic link target)
      3.  d_is_symlink  (check if path is symbolic link)

XIV.  PIPE OPERATIONS
      ----------------
      1.  d_popen       (POSIX popen equivalent)
      2.  d_pclose      (POSIX pclose equivalent)
*/

#ifndef DJINTERP_FILE_
#define DJINTERP_FILE_ 1

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include ".\djinterp.h"
#include ".\dmemory.h"
#include ".\string_fn.h"


// I. Platform detection and includes

// suppress MSVC security warnings - this library provides its own safe wrappers
#if ( (D_ENV_CRT_MSVC) &&  \
      (!defined(_CRT_SECURE_NO_WARNINGS)) )
    #define _CRT_SECURE_NO_WARNINGS 1
#endif

// platform-specific includes
#if defined(D_ENV_PLATFORM_WINDOWS)
    #ifndef D_FILE_PLATFORM_WINDOWS
        #define D_FILE_PLATFORM_WINDOWS 1
    #endif

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <windows.h>
    #include <winioctl.h>   // for FSCTL_GET_REPARSE_POINT
    #include <io.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <direct.h>
    #include <share.h>

    // fallback definitions for older SDKs
    #ifndef FSCTL_GET_REPARSE_POINT
        #define FSCTL_GET_REPARSE_POINT 0x000900A8
    #endif
    #ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
        #define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
    #endif
    #ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
        #define SYMBOLIC_LINK_FLAG_DIRECTORY 0x1
    #endif
    #ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
        #define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
    #endif
#endif

#if ( defined(D_ENV_PLATFORM_LINUX) ||  \
      defined(D_ENV_PLATFORM_MACOS) ||  \
      defined(D_ENV_PLATFORM_UNIX) )
    #ifndef D_FILE_PLATFORM_POSIX
        #define D_FILE_PLATFORM_POSIX 1
    #endif
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/file.h>
    #include <dirent.h>
    #include <libgen.h>
#endif


// D_FILE_HAS_FOPEN_S
//   feature: detect if fopen_s is available.
#ifndef D_FILE_HAS_FOPEN_S
    #if ( defined(__STDC_LIB_EXT1__) ||  \
          defined(__STDC_SECURE_LIB__) )
        #define D_FILE_HAS_FOPEN_S 1
    #elif (D_ENV_MSC_VER >= 1400)
        #define D_FILE_HAS_FOPEN_S 1
    #else
        #define D_FILE_HAS_FOPEN_S 0
    #endif
#endif

// D_FILE_HAS_FSEEKO
//   feature: detect if fseeko/ftello are available.
#ifndef D_FILE_HAS_FSEEKO
    #if ( defined(_POSIX_C_SOURCE) &&  \
          (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_200112L) )
        #define D_FILE_HAS_FSEEKO 1
    #elif defined(D_FILE_PLATFORM_POSIX)
        #define D_FILE_HAS_FSEEKO 1
    #else
        #define D_FILE_HAS_FSEEKO 0
    #endif
#endif

// D_FILE_HAS_MKSTEMP
//   feature: detect if mkstemp is available.
#ifndef D_FILE_HAS_MKSTEMP
    #if defined(D_FILE_PLATFORM_POSIX)
        #define D_FILE_HAS_MKSTEMP 1
    #else
        #define D_FILE_HAS_MKSTEMP 0
    #endif
#endif

// D_FILE_HAS_REALPATH
//   feature: detect if realpath is available.
#ifndef D_FILE_HAS_REALPATH
    #if defined(D_FILE_PLATFORM_POSIX)
        #define D_FILE_HAS_REALPATH 1
    #else
        #define D_FILE_HAS_REALPATH 0
    #endif
#endif

// D_FILE_HAS_SYMLINKS
//   feature: detect if symbolic links are supported.
#ifndef D_FILE_HAS_SYMLINKS
    #if defined(D_FILE_PLATFORM_POSIX)
        #define D_FILE_HAS_SYMLINKS 1
    #elif defined(D_FILE_PLATFORM_WINDOWS)
        // Windows Vista+ supports symlinks but requires special privileges
        #define D_FILE_HAS_SYMLINKS 1
    #else
        #define D_FILE_HAS_SYMLINKS 0
    #endif
#endif


// II. Type definitions

// d_off_t
//   type: 64-bit file offset type for large file support.
#if defined(D_FILE_PLATFORM_WINDOWS)
    typedef int64_t d_off_t;
#elif defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
    typedef off_t d_off_t;
#elif defined(D_FILE_PLATFORM_POSIX)
    typedef off_t d_off_t;
#else
    typedef int64_t d_off_t;
#endif

// d_stat_t
//   type: portable file status structure.
struct d_stat_t
{
    uint64_t st_size;       // file size in bytes
    uint64_t st_mtime;      // modification time (Unix timestamp)
    uint64_t st_atime;      // access time (Unix timestamp)
    uint64_t st_ctime;      // creation/change time (Unix timestamp)
    uint32_t st_mode;       // file mode and permissions
    uint32_t st_nlink;      // number of hard links
    uint32_t st_uid;        // owner user ID
    uint32_t st_gid;        // owner group ID
    uint64_t st_dev;        // device ID
    uint64_t st_ino;        // inode number
};

// d_dirent_t
//   type: portable directory entry structure.
struct d_dirent_t
{
    char     d_name[260];   // filename (MAX_PATH on Windows)
    uint64_t d_ino;         // inode number (0 on Windows)
    uint8_t  d_type;        // file type (DT_* constants)
};

// d_dir_t
//   type: opaque directory handle.
struct d_dir_t;


// file type constants for d_dirent_t.d_type
#ifndef DT_UNKNOWN
    #define DT_UNKNOWN  0
#endif
#ifndef DT_REG
    #define DT_REG      8   // regular file
#endif
#ifndef DT_DIR
    #define DT_DIR      4   // directory
#endif
#ifndef DT_LNK
    #define DT_LNK      10  // symbolic link
#endif
#ifndef DT_CHR
    #define DT_CHR      2   // character device
#endif
#ifndef DT_BLK
    #define DT_BLK      6   // block device
#endif
#ifndef DT_FIFO
    #define DT_FIFO     1   // named pipe (FIFO)
#endif
#ifndef DT_SOCK
    #define DT_SOCK     12  // socket
#endif

// file access mode constants for d_access
#ifndef F_OK
    #define F_OK        0   // existence
#endif                      
#ifndef R_OK                
    #define R_OK        4   // read permission
#endif                      
#ifndef W_OK                
    #define W_OK        2   // write permission
#endif                      
#ifndef X_OK                
    #define X_OK        1   // execute permission
#endif

// file mode constants
#ifndef S_IRUSR
    #define S_IRUSR 0400    // owner read
#endif
#ifndef S_IWUSR
    #define S_IWUSR 0200    // owner write
#endif
#ifndef S_IXUSR
    #define S_IXUSR 0100    // owner execute
#endif
#ifndef S_IRGRP
    #define S_IRGRP 0040    // group read
#endif
#ifndef S_IWGRP
    #define S_IWGRP 0020    // group write
#endif
#ifndef S_IXGRP
    #define S_IXGRP 0010    // group execute
#endif
#ifndef S_IROTH
    #define S_IROTH 0004    // others read
#endif
#ifndef S_IWOTH
    #define S_IWOTH 0002    // others write
#endif
#ifndef S_IXOTH
    #define S_IXOTH 0001    // others execute
#endif

// combined permission macros
#ifndef S_IRWXU
    #define S_IRWXU (S_IRUSR |  \
                     S_IWUSR |  \
                     S_IXUSR)
#endif
#ifndef S_IRWXG
    #define S_IRWXG (S_IRGRP |  \
                     S_IWGRP |  \
                     S_IXGRP)
#endif
#ifndef S_IRWXO
    #define S_IRWXO (S_IROTH |  \
                     S_IWOTH |  \
                     S_IXOTH)
#endif

// file type macros for st_mode
#ifndef S_ISREG
    #define S_ISREG(m) (((m) & 0170000) == 0100000)
#endif
#ifndef S_ISDIR
    #define S_ISDIR(m) (((m) & 0170000) == 0040000)
#endif
#ifndef S_ISLNK
    #define S_ISLNK(m) (((m) & 0170000) == 0120000)
#endif
#ifndef S_ISCHR
    #define S_ISCHR(m) (((m) & 0170000) == 0020000)
#endif
#ifndef S_ISBLK
    #define S_ISBLK(m) (((m) & 0170000) == 0060000)
#endif
#ifndef S_ISFIFO
    #define S_ISFIFO(m) (((m) & 0170000) == 0010000)
#endif
#ifndef S_ISSOCK
    #define S_ISSOCK(m) (((m) & 0170000) == 0140000)
#endif

// file lock types for d_flock
#define D_LOCK_SH   1   // shared lock
#define D_LOCK_EX   2   // exclusive lock
#define D_LOCK_NB   4   // non-blocking
#define D_LOCK_UN   8   // unlock

// seek origins
#ifndef SEEK_SET
    #define SEEK_SET 0
#endif
#ifndef SEEK_CUR
    #define SEEK_CUR 1
#endif
#ifndef SEEK_END
    #define SEEK_END 2
#endif

// D_FILE_PATH_MAX
//   constant: maximum path length.
#ifndef D_FILE_PATH_MAX
    #if defined(D_FILE_PLATFORM_WINDOWS)
        #define D_FILE_PATH_MAX 260
    #elif defined(PATH_MAX)
        #define D_FILE_PATH_MAX PATH_MAX
    #else
        #define D_FILE_PATH_MAX 4096
    #endif
#endif

// D_FILE_NAME_MAX
//   constant: maximum filename length.
#ifndef D_FILE_NAME_MAX
    #if defined(D_FILE_PLATFORM_WINDOWS)
        #define D_FILE_NAME_MAX 255
    #elif defined(NAME_MAX)
        #define D_FILE_NAME_MAX NAME_MAX
    #else
        #define D_FILE_NAME_MAX 255
    #endif
#endif

// path separator
#if defined(D_FILE_PLATFORM_WINDOWS)
    #define D_FILE_PATH_SEP      '\\'
    #define D_FILE_PATH_SEP_STR  "\\"
    #define D_FILE_PATH_SEP_ALT  '/'
#else
    #define D_FILE_PATH_SEP      '/'
    #define D_FILE_PATH_SEP_STR  "/"
    #define D_FILE_PATH_SEP_ALT  '/'
#endif

// III. Secure file opening
FILE*              d_fopen(const char* _filename,
                           const char* _mode);
int                d_fopen_s(FILE**      _stream,
                             const char* _filename,
                             const char* _mode);
FILE*              d_freopen(const char* _filename,
                             const char* _mode,
                             FILE*       _stream);
int                d_freopen_s(FILE**      _newstream,
                               const char* _filename,
                               const char* _mode,
                               FILE*       _stream);
FILE*              d_fdopen(int         _fd,
                            const char* _mode);

// IV. Large file support
int                d_fseeko(FILE*   _stream,
                            d_off_t _offset,
                            int     _whence);
d_off_t            d_ftello(FILE* _stream);
int                d_ftruncate(int     _fd,
                               d_off_t _length);
int                d_ftruncate_stream(FILE*   _stream,
                                      d_off_t _length);

// V. File descriptor operations
int                d_fileno(FILE* _stream);
int                d_dup(int _fd);
int                d_dup2(int _fd,
                          int _fd2);
int                d_close(int _fd);
ssize_t            d_read(int    _fd,
                          void*  _buf,
                          size_t _count);
ssize_t            d_write(int         _fd,
                           const void* _buf,
                           size_t      _count);
int                d_open(const char* _path,
                          int         _flags,
                          ...);

// VI. File synchronization
int                d_fsync(int _fd);
int                d_fsync_stream(FILE* _stream);
int                d_fflush(FILE* _stream);

// VII. File locking
int                d_flock(int _fd,
                           int _operation);
int                d_flock_stream(FILE* _stream,
                                  int   _operation);

// VIII. Temporary files
FILE*              d_tmpfile(void);
int                d_tmpfile_s(FILE** _stream);
int                d_mkstemp(char* _template);
int                d_tmpnam_s(char*  _s,
                              size_t _maxsize);
char*              d_tempdir(char*  _buf,
                             size_t _bufsize);

// IX. File metadata
int                d_stat(const char*      _path,
                          struct d_stat_t* _buf);
int                d_fstat(int              _fd,
                           struct d_stat_t* _buf);
int                d_lstat(const char*      _path,
                           struct d_stat_t* _buf);
int                d_access(const char* _path,
                            int         _mode);
int                d_chmod(const char* _path,
                           uint32_t    _mode);
int64_t            d_file_size(const char* _path);
int64_t            d_file_size_stream(FILE* _stream);
int                d_file_exists(const char* _path);
int                d_is_file(const char* _path);
int                d_is_dir(const char* _path);

// X. Directory operations
int                d_mkdir(const char* _path,
                           uint32_t    _mode);
int                d_mkdir_p(const char* _path,
                             uint32_t    _mode);
int                d_rmdir(const char* _path);
struct d_dir_t*    d_opendir(const char* _path);
struct d_dirent_t* d_readdir(struct d_dir_t* _dir);
int                d_closedir(struct d_dir_t* _dir);
void               d_rewinddir(struct d_dir_t* _dir);

// XI. File operations
int                d_remove(const char* _path);
int                d_unlink(const char* _path);
int                d_rename(const char* _oldpath,
                            const char* _newpath,
                            int         _overwrite);
int                d_copy_file(const char* _src,
                               const char* _dst);

// XII. Path utilities
char*              d_getcwd(char*  _buf,
                            size_t _size);
int                d_chdir(const char* _path);
char*              d_realpath(const char* _path,
                              char*       _resolved);
char*              d_dirname(const char* _path,
                             char*       _buf,
                             size_t      _bufsize);
char*              d_basename(const char* _path,
                              char*       _buf,
                              size_t      _bufsize);
char*              d_path_join(char*       _buf,
                               size_t      _bufsize,
                               const char* _path1,
                               const char* _path2);
char*              d_path_normalize(const char* _path,
                                    char*       _buf,
                                    size_t      _bufsize);
int                d_path_is_absolute(const char* _path);
const char*        d_get_extension(const char* _path);

// XIII. Symbolic links
#if D_FILE_HAS_SYMLINKS
    int            d_symlink(const char* _target,
                             const char* _linkpath);
    ssize_t        d_readlink(const char* _path,
                              char*       _buf,
                              size_t      _bufsize);
    int            d_is_symlink(const char* _path);
#endif  // D_FILE_HAS_SYMLINKS

// XIV. Pipe operations
FILE*              d_popen(const char* _command,
                           const char* _mode);
int                d_pclose(FILE* _stream);

// XV. Binary I/O helpers
void*              d_fread_all(const char* _path,
                               size_t*     _size);
int                d_fwrite_all(const char* _path,
                                const void* _data,
                                size_t      _size);
int                d_fappend_all(const char* _path,
                                 const void* _data,
                                 size_t      _size);


#endif	// DJINTERP_FILE_