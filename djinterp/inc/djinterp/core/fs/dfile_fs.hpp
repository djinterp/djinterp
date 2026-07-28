/******************************************************************************
* djinterp [core]                                                 dfile_fs.hpp
*
* Filesystem-structure operations for the djinterp C++ toolkit.
*   This is the C++ counterpart to dfile_fs.h. It provides the directory
* class: a move-only RAII owner of a directory handle that supports both a
* manual read()/rewind() interface and range-based for iteration over its
* entries. Directory creation and removal, whole-file operations (remove,
* rename, copy), and symbolic links are provided as free functions in
* djinterp::file. Everything forwards inline to the extern "C" API.
*   The symbolic-link free functions are compiled only where the platform
* supports them, mirroring the D_FILE_HAS_SYMLINKS gate in the C header.
*
* 
* path:      /inc/djinterp/cpp/io/file/dfile_fs.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    class directory
      --------------
      1.  construction / destruction (opendir, adopt, move)
      2.  observers (is_open, get)
      3.  ownership (release, reset)
      4.  traversal (read, rewind)
      5.  range-based iteration (iterator, begin, end)

II.   ENTRY HELPERS
      -------------
      1.  type_of       (entry_type of a directory entry)

III.  DIRECTORY / FILE OPERATIONS
      --------------------------
      1.  mkdir / mkdir_p / rmdir
      2.  remove / unlink / rename / copy_file

IV.   SYMBOLIC LINKS (where supported)
      -------------------------------
      1.  symlink / read_symlink / is_symlink
*/

#ifndef DJINTERP_FILE_FS_
#define DJINTERP_FILE_FS_ 1

#include "../../../c/io/file/dfile_fs.h"
#include "./dfile_common.hpp"


NS_DJINTERP
D_NAMESPACE(file)

// I. class directory

// directory
//   type: move-only RAII wrapper owning a directory handle; closes on
// destruction. Supports manual traversal and range-based for iteration.
class directory
{
public:
    // iterator
    //   single-pass input iterator over directory entries. Dereferences to a
    // const d_dirent_t&; a past-the-end iterator holds a null entry.
    class iterator
    {
    public:
        D_INLINE iterator() noexcept
            : m_handle(nullptr)
            , m_entry(nullptr)
        {}

        D_INLINE explicit iterator(struct d_dir_t* _handle) noexcept
            : m_handle(_handle)
            , m_entry(nullptr)
        {
            advance();
        }

        D_NO_DISCARD D_INLINE const d_dirent_t& operator*() const noexcept
        {
            return *m_entry;
        }

        D_NO_DISCARD D_INLINE const d_dirent_t* operator->() const noexcept
        {
            return m_entry;
        }

        D_INLINE iterator& operator++() noexcept
        {
            advance();

            return *this;
        }

        D_NO_DISCARD D_INLINE bool operator==(const iterator& _o) const noexcept
        {
            return m_entry == _o.m_entry;
        }

        D_NO_DISCARD D_INLINE bool operator!=(const iterator& _o) const noexcept
        {
            return m_entry != _o.m_entry;
        }

    private:
        // read the next entry, or null at end of directory.
        D_INLINE void advance() noexcept
        {
            m_entry = (m_handle != nullptr) ? d_readdir(m_handle)
                                            : nullptr;
        }

        struct d_dir_t*    m_handle;
        struct d_dirent_t* m_entry;
    };

    // construction / destruction

    // construct an invalid (closed) directory.
    D_INLINE directory() noexcept
        : m_dir(nullptr)
    {}

    // open the directory at _path (see d_opendir).
    D_INLINE explicit directory(const char* _path) noexcept
        : m_dir(d_opendir(_path))
    {}

    directory(const directory&)            = delete;
    directory& operator=(const directory&) = delete;

    D_INLINE directory(directory&& _other) noexcept
        : m_dir(_other.m_dir)
    {
        _other.m_dir = nullptr;
    }

    D_INLINE directory& operator=(directory&& _other) noexcept
    {
        // guard against self-move before releasing our own handle
        if (this != &_other)
        {
            reset(_other.m_dir);
            _other.m_dir = nullptr;
        }

        return *this;
    }

    D_INLINE ~directory() noexcept
    {
        reset();
    }

    // observers

    D_NO_DISCARD D_INLINE bool is_open() const noexcept
    {
        return m_dir != nullptr;
    }

    D_NO_DISCARD D_INLINE explicit operator bool() const noexcept
    {
        return m_dir != nullptr;
    }

    D_NO_DISCARD D_INLINE struct d_dir_t* get() const noexcept
    {
        return m_dir;
    }

    // ownership

    // relinquish ownership and return the raw handle.
    D_NO_DISCARD D_INLINE struct d_dir_t* release() noexcept
    {
        struct d_dir_t* handle;

        handle  = m_dir;
        m_dir   = nullptr;

        return handle;
    }

    // close any current handle and adopt _handle (default: none).
    D_INLINE void reset(struct d_dir_t* _handle = nullptr) noexcept
    {
        // close the existing handle before replacing it
        if (m_dir != nullptr)
        {
            d_closedir(m_dir);
        }

        m_dir = _handle;

        return;
    }

    // traversal

    // read the next entry, or null at end of directory (see d_readdir).
    D_INLINE struct d_dirent_t* read() noexcept
    {
        return d_readdir(m_dir);
    }

    // rewind traversal to the first entry (see d_rewinddir).
    D_INLINE void rewind() noexcept
    {
        d_rewinddir(m_dir);

        return;
    }

    // range-based iteration

    // begin() reads the first entry; the directory is single-pass, so it is
    // intended to be iterated once.
    D_NO_DISCARD D_INLINE iterator begin() noexcept
    {
        return iterator(m_dir);
    }

    D_NO_DISCARD D_INLINE iterator end() noexcept
    {
        return iterator();
    }

private:
    struct d_dir_t* m_dir;
};


// II. entry helpers

// type_of
//   the entry_type reported by a directory entry's d_type field.
D_CONSTEXPR_INLINE
entry_type
type_of(const d_dirent_t& _entry)
{
    return static_cast<entry_type>(_entry.d_type);
}


// III. directory / file operations

// mkdir
//   create the directory _path with permission bits _mode (see d_mkdir).
D_INLINE
int
mkdir(const char* _path,
      uint32_t    _mode = S_IRWXU | S_IRWXG | S_IRWXO)
{
    return d_mkdir(_path, _mode);
}

// mkdir_p
//   create _path and any missing parent directories (see d_mkdir_p).
D_INLINE
int
mkdir_p(const char* _path,
        uint32_t    _mode = S_IRWXU | S_IRWXG | S_IRWXO)
{
    return d_mkdir_p(_path, _mode);
}

// rmdir
//   remove the empty directory _path (see d_rmdir).
D_INLINE
int
rmdir(const char* _path)
{
    return d_rmdir(_path);
}

// remove
//   remove the file or empty directory _path (see d_remove).
D_INLINE
int
remove(const char* _path)
{
    return d_remove(_path);
}

// unlink
//   remove the file _path (see d_unlink).
D_INLINE
int
unlink(const char* _path)
{
    return d_unlink(_path);
}

// rename
//   rename _oldpath to _newpath; when _overwrite is false the call fails if
// the destination already exists (see d_rename).
D_INLINE
int
rename(const char* _oldpath,
       const char* _newpath,
       bool        _overwrite = true)
{
    return d_rename(_oldpath, _newpath, _overwrite ? 1 : 0);
}

// copy_file
//   copy the contents of _source to _destination (see d_copy_file).
D_INLINE
int
copy_file(const char* _source,
          const char* _destination)
{
    return d_copy_file(_source, _destination);
}


// IV. symbolic links
#if D_FILE_HAS_SYMLINKS

// symlink
//   create a symbolic link at _linkpath pointing to _target (see d_symlink).
D_INLINE
int
symlink(const char* _target,
        const char* _linkpath)
{
    return d_symlink(_target, _linkpath);
}

// read_symlink
//   read the target of the symbolic link _path into _buf; returns the byte
// count, or a negative value on error (see d_readlink).
D_NO_DISCARD D_INLINE
ssize_t
read_symlink(const char* _path,
             char*       _buf,
             size_t      _bufsize)
{
    return d_readlink(_path, _buf, _bufsize);
}

// is_symlink
//   true if _path is a symbolic link (see d_is_symlink).
D_NO_DISCARD D_INLINE
bool
is_symlink(const char* _path)
{
    return d_is_symlink(_path) != 0;
}

#endif  // D_FILE_HAS_SYMLINKS

NS_END  // file
NS_END  // djinterp


#endif  // DJINTERP_FILE_FS_
