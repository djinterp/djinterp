/******************************************************************************
* djinterp [core]                                           dfile_metadata.hpp
*
* File metadata, permissions, and inspection for the djinterp C++ toolkit.
*   This is the C++ counterpart to dfile_metadata.h. It provides the status
* value type, a thin wrapper over the C d_stat_t that offers typed accessors
* and file-kind predicates, together with free functions for the existence,
* size, accessibility, and permission queries. Everything forwards inline to
* the extern "C" implementation.
*
* note on return conventions:
*   status validity and the accessible() check follow the POSIX convention
* that the underlying call returns 0 on success; the exists()/is_file()/
* is_directory() predicates treat a non-zero C result as true. If the C
* implementation chooses different conventions, these are the only spots to
* adjust.
*
* 
* path:      /inc/djinterp/cpp/io/file/dfile_metadata.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    class status
      -----------
      1.  factories (of, of_link, of_descriptor)
      2.  validity (valid, operator bool)
      3.  field accessors (size, mode, links, uid, gid, times, ids)
      4.  kind predicates (is_regular, is_directory, is_symlink, ...)
      5.  raw access

II.   QUERY FUNCTIONS
      --------------
      1.  exists / is_file / is_directory
      2.  size (by path, by stream)
      3.  accessible
      4.  chmod
*/

#ifndef DJINTERP_FILE_METADATA_
#define DJINTERP_FILE_METADATA_ 1

#include "../../../c/io/file/dfile_metadata.h"
#include "./dfile_common.hpp"


NS_DJINTERP
D_NAMESPACE(file)

// I. class status

// status
//   type: value wrapper over d_stat_t with typed accessors and file-kind
// predicates. A status that failed to load tests false.
class status
{
public:
    // factories

    // stat _path, following symlinks (see d_stat).
    D_NO_DISCARD D_INLINE static status of(const char* _path) noexcept
    {
        status result;

        result.m_valid = (d_stat(_path, &result.m_stat) == 0);

        return result;
    }

    // stat _path without following symlinks (see d_lstat).
    D_NO_DISCARD D_INLINE static status of_link(const char* _path) noexcept
    {
        status result;

        result.m_valid = (d_lstat(_path, &result.m_stat) == 0);

        return result;
    }

    // stat an open descriptor _fd (see d_fstat).
    D_NO_DISCARD D_INLINE static status of_descriptor(int _fd) noexcept
    {
        status result;

        result.m_valid = (d_fstat(_fd, &result.m_stat) == 0);

        return result;
    }

    // validity

    D_NO_DISCARD D_INLINE bool valid() const noexcept
    {
        return m_valid;
    }

    D_NO_DISCARD D_INLINE explicit operator bool() const noexcept
    {
        return m_valid;
    }

    // field accessors

    D_NO_DISCARD D_INLINE uint64_t size() const noexcept
    {
        return m_stat.st_size;
    }

    D_NO_DISCARD D_INLINE uint32_t mode() const noexcept
    {
        return m_stat.st_mode;
    }

    D_NO_DISCARD D_INLINE uint32_t links() const noexcept
    {
        return m_stat.st_nlink;
    }

    D_NO_DISCARD D_INLINE uint32_t uid() const noexcept
    {
        return m_stat.st_uid;
    }

    D_NO_DISCARD D_INLINE uint32_t gid() const noexcept
    {
        return m_stat.st_gid;
    }

    D_NO_DISCARD D_INLINE uint64_t modified_time() const noexcept
    {
        return m_stat.st_mtime;
    }

    D_NO_DISCARD D_INLINE uint64_t accessed_time() const noexcept
    {
        return m_stat.st_atime;
    }

    D_NO_DISCARD D_INLINE uint64_t changed_time() const noexcept
    {
        return m_stat.st_ctime;
    }

    // kind predicates

    D_NO_DISCARD D_INLINE bool is_regular() const noexcept
    {
        return S_ISREG(m_stat.st_mode);
    }

    D_NO_DISCARD D_INLINE bool is_directory() const noexcept
    {
        return S_ISDIR(m_stat.st_mode);
    }

    D_NO_DISCARD D_INLINE bool is_symlink() const noexcept
    {
        return S_ISLNK(m_stat.st_mode);
    }

    D_NO_DISCARD D_INLINE bool is_char_device() const noexcept
    {
        return S_ISCHR(m_stat.st_mode);
    }

    D_NO_DISCARD D_INLINE bool is_block_device() const noexcept
    {
        return S_ISBLK(m_stat.st_mode);
    }

    D_NO_DISCARD D_INLINE bool is_fifo() const noexcept
    {
        return S_ISFIFO(m_stat.st_mode);
    }

    D_NO_DISCARD D_INLINE bool is_socket() const noexcept
    {
        return S_ISSOCK(m_stat.st_mode);
    }

    // raw access

    D_NO_DISCARD D_INLINE const d_stat_t& raw() const noexcept
    {
        return m_stat;
    }

private:
    d_stat_t m_stat{};
    bool     m_valid{false};
};


// II. query functions

// exists
//   true if _path exists (see d_file_exists).
D_NO_DISCARD D_INLINE
bool
exists(const char* _path)
{
    return d_file_exists(_path) != 0;
}

// is_file
//   true if _path is a regular file (see d_is_file).
D_NO_DISCARD D_INLINE
bool
is_file(const char* _path)
{
    return d_is_file(_path) != 0;
}

// is_directory
//   true if _path is a directory (see d_is_dir).
D_NO_DISCARD D_INLINE
bool
is_directory(const char* _path)
{
    return d_is_dir(_path) != 0;
}

// size
//   size of the file at _path in bytes, or a negative value on error (see
// d_file_size).
D_NO_DISCARD D_INLINE
int64_t
size(const char* _path)
{
    return d_file_size(_path);
}

// size
//   size of the file behind an open stream (see d_file_size_stream).
D_NO_DISCARD D_INLINE
int64_t
size(FILE* _stream)
{
    return d_file_size_stream(_stream);
}

// accessible
//   true if _path is accessible under the requested mode (see d_access).
D_NO_DISCARD D_INLINE
bool
accessible(const char* _path,
           access_flag _mode)
{
    return d_access(_path, to_int(_mode)) == 0;
}

// chmod
//   change the permission bits of _path (see d_chmod); returns 0 on success.
D_INLINE
int
chmod(const char* _path,
      uint32_t    _mode)
{
    return d_chmod(_path, _mode);
}

NS_END  // file
NS_END  // djinterp


#endif  // DJINTERP_FILE_METADATA_
