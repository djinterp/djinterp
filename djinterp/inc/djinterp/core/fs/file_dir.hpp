/*******************************************************************************
* djinterp [core]                                                   file_dir.hpp
*
* djinterp::directory -- an open directory you can walk with a range-for
* (roadmap Phase 6).
*   This is the first C++ piece with iterator machinery rather than a flat set
* of methods, and it makes the same D4 ownership decision file does, for the
* same reason: a directory handle cannot be shared by copying.
*   THE READDIR TRAP, HANDLED ONCE. file_dir.h is explicit that the d_dirent_t
* d_dir_read returns is BORROWED -- it stays valid only until the next
* d_dir_read on the same handle, and dies with d_dir_close. So directory_entry
* does not hold that pointer; it COPIES the name the moment it reads it. An
* entry you keep is yours; it does not dangle when the walk moves on.
*   SINGLE PASS. A directory is an input range, walked once. begin() reads the
* first entry from the current position and ++ reads the next; there is no
* going back within a pass. To walk again, call rewind() (or open a fresh
* directory). "." and ".." are skipped, as a C++ directory walk is expected
* to.
*   TWO WAYS IN, ONE ENGINE. read(entry, ec) is the error-code-primary form --
* it returns false at the end OR on a read error, with _ec clear on a clean end
* and set on a real one, so a mid-walk failure is never silently an "end". The
* range-for is the ergonomic form built on the same read: because operator++
* cannot carry an error, a read failure ends the loop and STASHES the reason,
* which last_error()/failed() report after the loop. Either way a failure is
* visible; it is never thrown and never lost.
*   NO OS. Entry types come from d_type via the DT_* constants file_common.h
* guarantees (POSIX numbering, defined portably), the same way this layer uses
* SEEK_SET. There is no platform branch here.
*
*
* path:      /inc/djinterp/core/fs/file_dir.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DIRECTORY ENTRY
    ---------------
    1.  Construction
    2.  Observers
    3.  Type predicates
    4.  Filling
2.  DIRECTORY ITERATOR
    ------------------
    1.  Construction
    2.  Access and advance
    3.  Comparison
3.  DIRECTORY
    ---------
    1.  Construction and destruction
    2.  Reading
    3.  Range protocol
    4.  Observers
    5.  Implementation
4.  ITERATOR MEMBERS
    ----------------
    1.  Access and advance
5.  FREE FUNCTIONS
    --------------
    1.  Creation
    2.  Removal
*/

#ifndef DJINTERP_FS_FILE_DIR_HPP
#define DJINTERP_FS_FILE_DIR_HPP 1

// std
#include <cerrno>   // errno, EBADF, EINVAL
#include <cstring>  // unused: is_dot compares bytes directly
// djinterp
#include "../../djinterp.hpp"     // framework root
#include "file_path.hpp"          // path
#include "file_common.hpp"        // error, the D_* kit
#include "file_stat.hpp"          // file_status::file_type
#include "../../c/fs/file_dir.h"  // d_dir_open, d_dir_read, d_dir_rewind,
#include "../../env/env.h"        // D_ENV_LANG_*
                                  // d_dir_close, d_dir_create, d_dir_remove
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


NS_DJINTERP


//==============================================================================
// 1.  DIRECTORY ENTRY
//==============================================================================


// directory_entry
//   class: one directory entry. A VALUE -- it owns a COPY of the name (the
// d_dir_read buffer it came from is borrowed and short-lived), so it is safe
// to keep and copy. Its type comes from the entry's d_type, which some
// filesystems do not fill in a readdir -- type_known() says whether to trust
// the predicates or to status() the path instead.
class directory_entry
{
public:
    // 1.1    Construction
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs an empty entry of unknown type.
     */
    directory_entry(void)
        : m_type(DT_UNKNOWN)
    {}

    // 1.2    Observers
    //--------------------------------------------------------------------------
    /**
     * @brief Returns the entry's name -- the filename ALONE, not a full path;
     *        join it onto the directory's path to open it.
     *
     * @return the name, owned by this entry.
     */
    const path& name(void) const
    {
        return m_name;
    }

    /**
     * @brief Reports whether the readdir reported a usable type.
     *
     * @return false when the predicates below are all false and the real
     *         type must come from a status() on the joined path.
     */
    bool type_known(void) const
    {
        return m_type != DT_UNKNOWN;
    }

    // 1.3    Type predicates
    //--------------------------------------------------------------------------
    // one kind each, as the readdir reported it -- all false when it did not
    bool is_regular_file(void) const
    {
        return m_type == DT_REG;
    }

    bool is_directory(void) const
    {
        return m_type == DT_DIR;
    }

    bool is_symlink(void) const
    {
        return m_type == DT_LNK;
    }

    bool is_block_file(void) const
    {
        return m_type == DT_BLK;
    }

    bool is_character_file(void) const
    {
        return m_type == DT_CHR;
    }

    bool is_fifo(void) const
    {
        return m_type == DT_FIFO;
    }

    bool is_socket(void) const
    {
        return m_type == DT_SOCK;
    }

    /**
     * @brief Reports the type in file_status's vocabulary, for callers
     *        already speaking it.
     *
     * @note An unreported d_type maps to type_none, meaning "readdir did not
     *       say" -- distinct from type_unknown, which would be "it exists and
     *       is none of the named kinds".
     *
     * @return the matching file type, or type_none.
     */
    file_status::file_type type(void) const
    {
        // translate the readdir's DT_* value
        switch (m_type)
        {
            case DT_REG:
            {
                return file_status::type_regular;
            }
            case DT_DIR:
            {
                return file_status::type_directory;
            }
            case DT_LNK:
            {
                return file_status::type_symlink;
            }
            case DT_CHR:
            {
                return file_status::type_character;
            }
            case DT_BLK:
            {
                return file_status::type_block;
            }
            case DT_FIFO:
            {
                return file_status::type_fifo;
            }
            case DT_SOCK:
            {
                return file_status::type_socket;
            }
            default:
            {
                return file_status::type_none;
            }
        }
    }

    // 1.4    Filling
    //--------------------------------------------------------------------------
    /**
     * @brief Fills this entry from a readdir result, COPYING the name so
     *        nothing here points into the borrowed d_dirent_t. Used by
     *        directory.
     *
     * @param[in] _de  the readdir result; only borrowed.
     */
    void assign(const struct d_dirent_t& _de)
    {
        m_name = path(_de.d_name);
        m_type = _de.d_type;
    }

private:
    path    m_name;
    uint8_t m_type;
};


//==============================================================================
// 2.  DIRECTORY ITERATOR
//==============================================================================


// directory
//   class: forward declaration; defined in section 3. The iterator points at
// one.
class directory;

// directory_iterator
//   class: a single-pass input cursor over a directory. NON-OWNING -- it holds
// a pointer to the directory (which owns the handle), not the handle itself,
// so copying an iterator is cheap and copies share the one walk. A null
// directory pointer is the end sentinel; begin() reads the first entry and ++
// reads the next, nulling the pointer when the walk is exhausted so it
// compares equal to end().
class directory_iterator
{
public:
    // 2.1    Construction
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs the end sentinel.
     */
    directory_iterator(void)
        : m_dir(0)
    {}

    /**
     * @brief Constructs a cursor over a directory whose current entry has
     *        already been read.
     *
     * @param[in] _dir  the directory to walk; borrowed, not owned.
     */
    explicit directory_iterator(
        directory* _dir
    )
        : m_dir(_dir)
    {}

    // 2.2    Access and advance
    //--------------------------------------------------------------------------
    // defined in section 4, where directory is complete: the current entry,
    // and the step to the next one
    const directory_entry& operator*(void) const;
    const directory_entry* operator->(void) const;
    directory_iterator&    operator++(void);

    // 2.3    Comparison
    //--------------------------------------------------------------------------
    // two cursors are equal when they walk the same directory, or are both at
    // the end
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


//==============================================================================
// 3.  DIRECTORY
//==============================================================================


// directory
//   class: an open directory. Owns the d_dir_t*; non-copyable on every tier,
// movable on C++11+ (the same D4 rule as file). Walk it with read() or
// range-for.
class directory
{
    friend class directory_iterator;

public:
    // 3.1    Construction and destruction
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs a directory that has nothing open; is_open() is
     *        false.
     */
    directory(void)
        : m_dir(0),
          m_at_end(false)
    {}

    /**
     * @brief Opens a directory for walking.
     *
     * @param[in]  _path  the directory to open.
     * @param[out] _ec    cleared on success; EINVAL for an invalid path,
     *                    otherwise the platform's code.
     * @post is_open() reports whether the directory opened.
     */
    explicit directory(
        const path& _path,
        error&      _ec
    )
        : m_dir(0),
          m_at_end(false)
    {
        // an invalid path names nothing to open
        if (!_path.valid())
        {
            _ec.assign(EINVAL);

            return;
        }

        m_dir = d_dir_open(_path.c_str());

        // c/fs reports why the open failed through errno
        if (!m_dir)
        {
            _ec = error::from_errno();

            return;
        }

        _ec.clear();
    }

    /**
     * @brief Closes the handle.
     *
     * @note A destructor cannot report a close error, so it is dropped.
     *
     * @post Every entry reference obtained through an iterator is invalid.
     */
    ~directory(void)
    {
        // nothing to close for a directory that never opened
        if (m_dir)
        {
            (void)d_dir_close(m_dir);
        }
    }

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief Moves the handle, and the walk's state, into a new directory.
     *        C++11 and later.
     *
     * @param[in,out] _other  the directory to move from; it owns nothing
     *                        afterwards.
     */
    directory(
        directory&& _other
    ) D_NOEXCEPT
        : m_dir(_other.m_dir),
          m_current(_other.m_current),
          m_at_end(_other.m_at_end),
          m_error(_other.m_error)
    {
        _other.m_dir    = 0;
        _other.m_at_end = false;
    }

    /**
     * @brief Move-assigns, closing the handle this directory held first.
     *
     * @param[in,out] _other  the directory to move from; it owns nothing
     *                        afterwards.
     * @return `*this`.
     */
    directory& operator=(directory&& _other) D_NOEXCEPT
    {
        // a self-move leaves the directory as it was
        if (this != &_other)
        {
            // close what this directory held before taking the other's
            if (m_dir)
            {
                (void)d_dir_close(m_dir);
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
#endif  // D_MOVE_ENABLED

    // 3.2    Reading
    //--------------------------------------------------------------------------
    /**
     * @brief Reads the next entry: the form that never confuses an error for
     *        an ending.
     *
     * @param[out] _out  receives the entry on success.
     * @param[out] _ec   clear on success and at a clean end; set on a read
     *                   failure, and EBADF when nothing is open.
     * @return true with `_out` filled; false at the end of the walk OR on a
     *         read error, which `_ec` tells apart.
     */
    bool read(directory_entry& _out,
              error&           _ec)
    {
        // a closed directory has nothing to read
        if (!m_dir)
        {
            _ec.assign(EBADF);

            return false;
        }

        read_next();

        // the end of the walk, clean or not
        if (m_at_end)
        {
            _ec = m_error;   // clear on a clean end, set on error

            return false;
        }

        _out = m_current;
        _ec.clear();

        return true;
    }

    /**
     * @brief Returns to the first entry, to walk again.
     *
     * @param[out] _ec  cleared on success; EBADF when nothing is open,
     *                  otherwise the platform's code.
     * @return true on success.
     * @post The error a previous walk stashed is cleared.
     */
    bool rewind(error& _ec)
    {
        // a closed directory has nothing to rewind
        if (!m_dir)
        {
            _ec.assign(EBADF);

            return false;
        }

        // c/fs reports a failed rewind through errno
        if (d_dir_rewind(m_dir) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        m_at_end = false;
        m_error.clear();
        _ec.clear();

        return true;
    }

    // 3.3    Range protocol
    //--------------------------------------------------------------------------
    /**
     * @brief Reads the first entry from the current position -- single pass --
     *        and returns a cursor at it.
     *
     * @note A read error during iteration ends the loop; last_error() reports
     *       it afterwards.
     *
     * @return a cursor at the first entry, or end() when there is none.
     */
    directory_iterator begin(void)
    {
        // an open directory reads its first entry; a closed one is empty
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

    /**
     * @brief Returns the end sentinel.
     *
     * @return the end sentinel.
     */
    directory_iterator end(void)
    {
        return directory_iterator();
    }

    // 3.4    Observers
    //--------------------------------------------------------------------------
    // whether a directory is open for walking
    bool is_open(void) const
    {
        return m_dir != 0;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // from C++11, as an explicit conversion; below, is_open() is the
    // spelling (decision 3.2)
    D_EXPLICIT_BOOL operator bool(void) const
    {
        return m_dir != 0;
    }
#endif

    // after a range-for: the error the walk ended on, and whether there was
    // one -- a clean walk leaves both clear
    const error& last_error(void) const
    {
        return m_error;
    }

    bool failed(void) const
    {
        return m_error.failed();
    }

private:
    // 3.5    Implementation
    //--------------------------------------------------------------------------
    /**
     * @brief Advances to the next real entry, skipping "." and "..".
     *
     * @note A NULL from the readdir with errno clear is a clean end, and with
     *       errno set a failure -- the classic readdir distinction, made once,
     *       here.
     *
     * @post At the end of the walk m_at_end is set; on a genuine readdir error
     *       m_error also records the errno.
     */
    void read_next(void)
    {
        struct d_dirent_t* de = 0;

        // take entries until one is not "." or ".."
        for (;;)
        {
            errno = 0;
            de    = d_dir_read(m_dir);

            // the end of the walk, or a failure
            if (!de)
            {
                m_at_end = true;

                // a set errno means the read failed rather than ran out
                if (errno != 0)
                {
                    m_error = error::from_errno();
                }

                return;
            }

            // a real entry ends the search
            if (!is_dot(de->d_name))
            {
                break;
            }
        }

        m_current.assign(*de);
    }

    /**
     * @brief Reports whether a name is "." or "..", the two entries a walk
     *        skips.
     *
     * @param[in] _name  the entry's name.
     * @return true for "." and "..".
     */
    static bool is_dot(const char* _name)
    {
        return ( (_name[0] == '.') &&
                 ( (_name[1] == '\0') ||
                   ( (_name[1] == '.') &&
                     (_name[2] == '\0') ) ) );
    }

    // never copyable -- two owners would double-close the handle
    D_DELETED_FN(directory(const directory& _other))
    D_DELETED_FN(directory& operator=(const directory& _other))

    struct d_dir_t* m_dir;
    directory_entry m_current;
    bool            m_at_end;
    error           m_error;
};


//==============================================================================
// 4.  ITERATOR MEMBERS
//==============================================================================
// directory_iterator's members that need directory complete.


// 4.1    Access and advance
//------------------------------------------------------------------------------
/**
 * @brief Returns the current entry.
 *
 * @pre The cursor is not end().
 * @return the entry, owned by the directory.
 * @post The reference is valid until the next advance.
 */
inline const directory_entry&
directory_iterator::operator*(void) const
{
    return m_dir->m_current;
}

/**
 * @brief Returns the current entry, for member access.
 *
 * @pre The cursor is not end().
 * @return the entry, owned by the directory.
 */
inline const directory_entry*
directory_iterator::operator->(void) const
{
    return &m_dir->m_current;
}

/**
 * @brief Advances to the next entry, becoming end() when the walk is
 *        exhausted or a read fails.
 *
 * @pre The cursor is not end().
 * @return `*this`.
 */
inline directory_iterator&
directory_iterator::operator++(void)
{
    m_dir->read_next();

    // an exhausted walk turns this cursor into the end sentinel
    if (m_dir->m_at_end)
    {
        m_dir = 0;
    }

    return *this;
}


//==============================================================================
// 5.  FREE FUNCTIONS
//==============================================================================


// 5.1    Creation
//------------------------------------------------------------------------------
/**
 * @brief Makes one directory.
 *
 * @note Its parents must already exist -- that is what d_dir_create does;
 *       d_dir_create_parents would make the chain. An existing path is
 *       reported as-is, so the caller can check for EEXIST.
 *
 * @param[in]  _p   the directory to create.
 * @param[out] _ec  cleared on success; EINVAL for an invalid path, otherwise
 *                  the platform's code.
 * @return true on success.
 */
inline bool
create_directory(
    const path& _p,
    error&      _ec
)
{
    // an invalid path names nothing to create
    if (!_p.valid())
    {
        _ec.assign(EINVAL);

        return false;
    }

    // c/fs creates the directory, or reports through errno
    if (d_dir_create(_p.c_str(),
                     (uint32_t)0777) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}

// 5.2    Removal
//------------------------------------------------------------------------------
/**
 * @brief Removes one EMPTY directory.
 *
 * @note It does not walk and delete: the platform refuses a non-empty one
 *       (ENOTEMPTY), reported through `_ec`. remove_all, in
 *       file_recursive.hpp, empties a tree.
 *
 * @param[in]  _p   the directory to remove.
 * @param[out] _ec  cleared on success; EINVAL for an invalid path, otherwise
 *                  the platform's code.
 * @return true on success.
 */
inline bool
remove_directory(
    const path& _p,
    error&      _ec
)
{
    // an invalid path names nothing to remove
    if (!_p.valid())
    {
        _ec.assign(EINVAL);

        return false;
    }

    // c/fs removes the directory, or reports through errno
    if (d_dir_remove(_p.c_str()) != 0)
    {
        _ec = error::from_errno();

        return false;
    }

    _ec.clear();

    return true;
}


NS_END  // djinterp

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_DIR_HPP
