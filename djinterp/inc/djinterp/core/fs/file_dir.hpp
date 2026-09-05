/******************************************************************************
* djinterp [core]                                                 file_dir.hpp
*
*   djinterp::directory -- an open directory you can walk with a range-for
* (roadmap Phase 6). This is the first C++ piece with iterator machinery rather
* than a flat set of methods, and it makes the same D4 ownership decision file
* does, for the same reason: a directory handle cannot be shared by copying.
*
*   THE READDIR TRAP, HANDLED ONCE. file_dir.h is explicit that the d_dirent_t
* d_readdir returns is BORROWED -- it stays valid only until the next d_readdir
* on the same handle, and dies with d_closedir. So directory_entry does not
* hold that pointer; it COPIES the name the moment it reads it. An entry you
* keep is yours; it does not dangle when the walk moves on.
*
*   SINGLE PASS. A directory is an input range, walked once. begin() reads the
* first entry from the current position and ++ reads the next; there is no
* going back within a pass. To walk again, call rewind() (or open a fresh
* directory). "." and ".." are skipped, as a C++ directory walk is expected to.
*
*   TWO WAYS IN, ONE ENGINE. read(entry, ec) is the error-code-primary form --
* it returns false at the end OR on a read error, with _ec clear on a clean end
* and set on a real one, so a mid-walk failure is never silently an "end". The
* range-for is the ergonomic form built on the same read: because operator++
* cannot carry an error, a read failure ends the loop and STASHES the reason,
* which last_error()/failed() report after the loop. Either way a failure is
* visible; it is never thrown and never lost.
*
*   NO OS. Entry types come from d_type via the DT_* constants file_common.h
* guarantees (POSIX numbering, defined portably), the same way this layer uses
* SEEK_SET. There is no platform branch here.
*
*
* path:      /inc/djinterp/core/fs/file_dir.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    LANGUAGE SUPPORT       D_MOVE_ENABLED / _NOEXCEPT /
                             _EXPLICIT_BOOL / _DELETED_FN
I.    directory_entry        one entry: its name (owned) and its type
II.   directory_iterator     the input cursor (range-for)
III.  directory              the RAII handle + walk
IV.   FREE FUNCTIONS         create_directory / remove_directory
*/

#ifndef DJINTERP_FS_FILE_DIR_
#define DJINTERP_FS_FILE_DIR_ 1

// std
#include <cerrno>                 // EBADF, EINVAL
#include <cstring>                // std::strcmp (the "." / ".." skip)
// djinterp
#include "../../djinterp.hpp"
#include "file_path.hpp"
#include "file_common.hpp"
#include "file_stat.hpp"
// d_opendir, d_readdir, d_closedir, d_mkdir
#include "../../c/fs/file_dir.h"


NS_DJINTERP

// I.    directory_entry

// directory_entry
//   class: one directory entry. A VALUE -- it owns a COPY of the name (the
// d_readdir buffer it came from is borrowed and short-lived), so it is safe to
// keep and copy. Its type comes from the entry's d_type, which some
// filesystems do not fill in a readdir -- type_known() says whether to trust
// the predicates or to status() the path instead.
class directory_entry
{
public:
    directory_entry(void)
        : m_type(DT_UNKNOWN)
    {}

    // name
    //   function: the entry's name -- the filename ALONE, not a full path. Join
    // it onto the directory's path to open it.
    const path& name(void) const
    {
        return m_name;
    }

    // type_known
    //   function: whether the readdir reported a usable type. When false, the
    // predicates below are all false and the real type must come from a
    // status() on the joined path.
    bool type_known(void) const
    {
        return m_type != DT_UNKNOWN;
    }

    bool is_regular_file(void) const { return m_type == DT_REG;  }
    bool is_directory(void)    const { return m_type == DT_DIR;  }
    bool is_symlink(void)      const { return m_type == DT_LNK;  }
    bool is_block_file(void)   const { return m_type == DT_BLK;  }
    bool is_character_file(void) const { return m_type == DT_CHR; }
    bool is_fifo(void)         const { return m_type == DT_FIFO; }
    bool is_socket(void)       const { return m_type == DT_SOCK; }

    // type
    //   function: the type as a file_status::file_type, for callers already
    // speaking that vocabulary. An unreported d_type maps to type_none, meaning
    // "readdir did not say" -- distinct from type_unknown, which would be "it
    // exists and is none of the named kinds".
    file_status::file_type type(void) const
    {
        switch (m_type)
        {
            case DT_REG:  return file_status::type_regular;
            case DT_DIR:  return file_status::type_directory;
            case DT_LNK:  return file_status::type_symlink;
            case DT_CHR:  return file_status::type_character;
            case DT_BLK:  return file_status::type_block;
            case DT_FIFO: return file_status::type_fifo;
            case DT_SOCK: return file_status::type_socket;
            default:      return file_status::type_none;
        }
    }

    // assign
    //   function: fill from a readdir result, COPYING the name so nothing here
    // points into the borrowed d_dirent_t. Used by directory.
    void assign(const struct d_dirent_t& _de)
    {
        m_name = path(_de.d_name);
        m_type = _de.d_type;
    }

private:
    path    m_name;
    uint8_t m_type;
};


// II.   directory_iterator

class directory;   // defined below; the iterator points at one

// directory_iterator
//   class: a single-pass input cursor over a directory. NON-OWNING -- it holds
// a pointer to the directory (which owns the handle), not the handle itself,
// so copying an iterator is cheap and copies share the one walk. A null
// directory pointer is the end sentinel; begin() reads the first entry and ++
// reads the next, nulling the pointer when the walk is exhausted so it compares
// equal to end().
class directory_iterator
{
public:
    directory_iterator(void)
        : m_dir(0)
    {}

    explicit directory_iterator(directory* _dir)
        : m_dir(_dir)
    {}

    const directory_entry& operator*(void) const;
    const directory_entry* operator->(void) const;
    directory_iterator&    operator++(void);

    bool operator==(const directory_iterator& _o) const
    {
        return m_dir == _o.m_dir;
    }

    bool operator!=(const directory_iterator& _o) const
    {
        return m_dir != _o.m_dir;
    }

private:
    directory* m_dir;
};


// III.  directory

// directory
//   class: an open directory. Owns the d_dir_t*; non-copyable on every tier,
// movable on C++11+ (same D4 rule as file). Walk it with read() or range-for.
class directory
{
    friend class directory_iterator;

public:

    // directory
    //   function: an open-nothing directory. is_open() is false.
    directory(void)
        : m_dir(0)
        , m_at_end(false)
    {}

    // directory
    //   function: open _path for walking. Check the result with operator bool
    // / is_open(); to learn WHY an open failed, this cannot say (a constructor
    // has no return) -- there is no path-taking open-with-reason here because a
    // directory that failed to open has nothing to iterate anyway, and the
    // free create_directory/remove_directory carry their own reasons.
    explicit directory(const path& _path, error& _ec)
        : m_dir(0)
        , m_at_end(false)
    {
        if (!_path.valid())
        {
            _ec.assign(EINVAL);

            return;
        }

        m_dir = d_opendir(_path.c_str());

        if (!m_dir)
        {
            _ec = error::from_errno();

            return;
        }

        _ec.clear();
    }

    // ~directory
    //   function: close the handle. A close error cannot be reported from a
    // destructor and is dropped.
    ~directory(void)
    {
        if (m_dir)
        {
            (void)d_closedir(m_dir);
        }
    }

#if (D_MOVE_ENABLED == 1)
    directory(directory&& _other) D_NOEXCEPT
        : m_dir(_other.m_dir)
        , m_current(_other.m_current)
        , m_at_end(_other.m_at_end)
        , m_error(_other.m_error)
    {
        _other.m_dir    = 0;
        _other.m_at_end = false;
    }

    directory& operator=(directory&& _other) D_NOEXCEPT
    {
        if (this != &_other)
        {
            if (m_dir)
            {
                (void)d_closedir(m_dir);
            }

            m_dir     = _other.m_dir;
            m_current = _other.m_current;
            m_at_end  = _other.m_at_end;
            m_error   = _other.m_error;

            _other.m_dir    = 0;
            _other.m_at_end = false;
        }

        return *this;
    }
#endif

    // read
    //   function: read the next entry into _out. Returns true and fills _out on
    // success; returns false at the end of the walk OR on a read error, and _ec
    // tells them apart -- clear on a clean end, set on a real failure. This is
    // the form that never confuses an error for an ending.
    bool read(directory_entry& _out, error& _ec)
    {
        if (!m_dir)
        {
            _ec.assign(EBADF);

            return false;
        }

        read_next();

        if (m_at_end)
        {
            _ec = m_error;   // clear on a clean end, set on error

            return false;
        }

        _out = m_current;
        _ec.clear();

        return true;
    }

    // begin / end
    //   function: the range-for protocol. begin() reads the first entry (from
    // the current position -- single pass); end() is the null sentinel. A read
    // error during iteration ends the loop and is retrievable via last_error().
    directory_iterator begin(void)
    {
        if (m_dir)
        {
            read_next();
        }
        else
        {
            m_at_end = true;
        }

        return directory_iterator(m_at_end ? 0 : this);
    }

    directory_iterator end(void)
    {
        return directory_iterator();
    }

    // rewind
    //   function: return to the first entry, to walk again.
    bool rewind(error& _ec)
    {
        if (!m_dir)
        {
            _ec.assign(EBADF);

            return false;
        }

        if (d_rewinddir(m_dir) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        m_at_end = false;
        m_error.clear();
        _ec.clear();

        return true;
    }

    // is_open / operator bool
    //   function: whether a directory is open for walking.
    bool is_open(void) const
    {
        return m_dir != 0;
    }

    D_EXPLICIT_BOOL operator bool(void) const
    {
        return m_dir != 0;
    }

    // last_error / failed
    //   function: after a range-for, whether the walk ended on an error and
    // what it was. A clean walk leaves this clear.
    const error& last_error(void) const
    {
        return m_error;
    }

    bool failed(void) const
    {
        return m_error.failed();
    }

private:

    // read_next
    //   function: advance to the next real entry, skipping "." and "..". Sets
    // m_at_end at the end of the walk; on a genuine readdir error, sets
    // m_at_end
    // AND records the errno in m_error (a NULL return with errno clear is a
    // clean end, with errno set is a failure -- the classic readdir
    // distinction, made once, here).
    void read_next(void)
    {
        struct d_dirent_t* de;

        for (;;)
        {
            errno = 0;
            de    = d_readdir(m_dir);

            if (!de)
            {
                m_at_end = true;

                if (errno != 0)
                {
                    m_error = error::from_errno();
                }

                return;
            }

            if (!is_dot(de->d_name))
            {
                break;
            }
        }

        m_current.assign(*de);
    }

    // is_dot
    //   function: whether a name is "." or "..", the two entries a walk skips.
    static bool is_dot(const char* _name)
    {
        return (_name[0] == '.') &&
               (_name[1] == '\0' ||
                (_name[1] == '.' && _name[2] == '\0'));
    }

    struct d_dir_t* m_dir;
    directory_entry m_current;
    bool            m_at_end;
    error           m_error;

    // never copyable -- two owners would double-close the handle.
    D_DELETED_FN(directory(const directory& _other))
    D_DELETED_FN(directory& operator=(const directory& _other))
};


// --- directory_iterator members that need directory complete ---

inline const directory_entry&
directory_iterator::operator*(void) const
{
    return m_dir->m_current;
}

inline const directory_entry*
directory_iterator::operator->(void) const
{
    return &m_dir->m_current;
}

inline directory_iterator&
directory_iterator::operator++(void)
{
    m_dir->read_next();

    if (m_dir->m_at_end)
    {
        m_dir = 0;
    }

    return *this;
}


// IV.   Free functions

// create_directory
//   function: make one directory. Its parents must already exist (that is what
// d_mkdir does; d_mkdir_p would make the chain, and could be added later). An
// already-existing path is reported as-is -- the caller can check for EEXIST.
inline bool
create_directory(
    const path& _p,
    error&      _ec
)
{
    if (!_p.valid())
    {
        _ec.assign(EINVAL);

        return false;
    }

    if (d_mkdir(_p.c_str(), (uint32_t)0777) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}

// remove_directory
//   function: remove one EMPTY directory. A non-empty one is refused by the
// platform (ENOTEMPTY), reported through _ec -- this does not walk and delete.
inline bool
remove_directory(
    const path& _p,
    error&      _ec
)
{
    if (!_p.valid())
    {
        _ec.assign(EINVAL);

        return false;
    }

    if (d_rmdir(_p.c_str()) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}

NS_END  // djinterp

#endif  // DJINTERP_FS_FILE_DIR_
