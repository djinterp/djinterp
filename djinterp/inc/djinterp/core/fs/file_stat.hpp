/******************************************************************************
* djinterp [fs]                                                    file_stat.hpp
*
*   djinterp::file_status -- what the filesystem knows about a path, captured
* once (roadmap Phase 5). This is the C++ answer to the warning at the top of
* file_stat.h: the convenience predicates each cost a syscall and each can
* disagree with the next if the file changes in between, so the right shape is
* to stat ONCE and read the fields.
*
*   file_status IS that one stat. Construct it (a single d_stat call), then ask
* it as many questions as you like -- type, size, timestamps, permissions --
* and every answer describes the SAME snapshot. is_regular_file() and size()
* on one file_status cannot race each other, because there is no second
* syscall between them.
*
*   The free predicates below -- exists(), is_directory(), file_size() -- exist
* for the genuine one-question case, and each is honest about being one stat.
* When you have more than one question, take a file_status and ask it, not
* three of these.
*
*   TOCTOU. Every path-based query here is a hazard by construction: what
* status(p) saw and what the next open(p) gets may be two different files. When
* it matters, open the file and call file::status() -- an fstat on the live
* descriptor, which names one file for its whole lifetime and cannot be swapped
* underneath you. That method lives in file_stream.hpp; this header is what it returns.
*
*   A VALUE, not a resource. file_status owns nothing -- it is a struct and an
* enum. It copies freely on every tier, needs no move, and carries none of the
* D_*_ portability kit, because a snapshot has no handle to guard. Its tier
* ladder is therefore FLAT: the same source builds identically C++98 -> C++23,
* which is the point -- not every type needs the ceremony file does.
*
*   NO OS. Type is read from st_mode with the S_IS* tests file_common.h
* guarantees against a d_stat_t (defined portably where the platform lacks
* them), the same way this layer uses SEEK_SET and D_LOCK_EX. There is no
* platform branch here; the c/fs stat module already normalized st_mode.
*
* 
* path:      /inc/djinterp/core/fs/file_stat.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    file_status            the captured snapshot + its accessors
      1.  type / predicates
      2.  size / times / permissions
II.   QUERIES                status / symlink_status  (fill a snapshot)
III.  ONE-QUESTION HELPERS   exists / is_regular_file / is_directory / file_size
*/

#ifndef DJINTERP_FS_FILE_STAT_
#define DJINTERP_FS_FILE_STAT_ 1

// std
#include <cerrno>                    // ENOENT, ENOTDIR, EINVAL, EBADF
// djinterp
#include "./file_path.hpp"
#include "./file_common.hpp"
#include "../../c/fs/file_stat.h"    // d_stat, d_lstat, d_stat_t, S_IS*



NS_DJINTERP

// ===========================================================================
// I.   file_status
// ===========================================================================

// file_status
//   class: one filesystem snapshot of one path. A value type -- copy it, store
// it, compare its fields; it holds no handle.
class file_status
{
public:

    // file_type
    //   enum: what a path names. A plain (unscoped) enum so it is available
    // unchanged on C++98; enum class would need a portability shim this value
    // type is deliberately without. type_none is "no query has run"; not_found
    // is "the query ran and there is nothing there" -- a real, useful
    // distinction (a permission error is none, an absent file is not_found).
    enum file_type
    {
        type_none = 0,
        type_not_found,
        type_regular,
        type_directory,
        type_symlink,
        type_block,
        type_character,
        type_fifo,
        type_socket,
        type_unknown
    };

    // file_status
    //   function: an empty snapshot -- type_none, no query behind it.
    file_status(void)
        : m_type(type_none)
    {
        zero();
    }

    // file_status
    //   function: from a filled stat buffer. Computes the type from st_mode
    // once, here, so every predicate below is a comparison rather than another
    // mask. Used by the queries in section II.
    explicit file_status(const struct d_stat_t& _buf)
        : m_type(type_from_mode(_buf.st_mode))
        , m_stat(_buf)
    {
    }

    // file_status
    //   function: a snapshot that carries only a type and no data -- for
    // type_none and type_not_found, which have no fields to report.
    explicit file_status(file_type _type)
        : m_type(_type)
    {
        zero();
    }

    // --- I.1  type / predicates ---

    // type
    //   function: what the path names, as one value.
    file_type type(void) const
    {
        return m_type;
    }

    // exists
    //   function: whether the path names something. False for both "not there"
    // and "no query yet" -- if you must tell those apart, read type().
    bool exists(void) const
    {
        return m_type != type_none && m_type != type_not_found;
    }

    bool is_regular_file(void) const { return m_type == type_regular;   }
    bool is_directory(void)    const { return m_type == type_directory; }
    bool is_symlink(void)      const { return m_type == type_symlink;   }
    bool is_block_file(void)   const { return m_type == type_block;     }
    bool is_character_file(void) const { return m_type == type_character; }
    bool is_fifo(void)         const { return m_type == type_fifo;      }
    bool is_socket(void)       const { return m_type == type_socket;    }

    // is_other
    //   function: it exists, but is none of the named kinds.
    bool is_other(void) const
    {
        return m_type == type_unknown;
    }

    // --- I.2  size / times / permissions ---

    // size
    //   function: the size field of the snapshot, in bytes. Meaningful for a
    // regular file; for anything else it is whatever the platform put there
    // (the free file_size() below refuses a non-regular file for that reason).
    uint64_t size(void) const
    {
        return m_stat.st_size;
    }

    // modified_time / access_time / change_time / creation_time
    //   function: the four timestamps, Unix seconds. change_time is 0 on
    // platforms with no metadata-change concept; creation_time is 0 where
    // unavailable and NEVER falls back to change_time -- so a 0 here means
    // "not reported", not "the epoch".
    int64_t modified_time(void) const { return m_stat.st_modified; }
    int64_t access_time(void)   const { return m_stat.st_accessed; }
    int64_t change_time(void)   const { return m_stat.st_changed;  }
    int64_t creation_time(void) const { return m_stat.st_created;  }

    // permissions
    //   function: the permission bits (the low 9 of the mode: rwx for user,
    // group, other).
    uint32_t permissions(void) const
    {
        return m_stat.st_mode & (uint32_t)0777;
    }

    // mode
    //   function: the full mode word, type bits and all, for a caller who
    // wants to apply its own S_IS* / permission masks.
    uint32_t mode(void) const
    {
        return m_stat.st_mode;
    }

    // hard_links
    //   function: how many names this file has.
    uint32_t hard_links(void) const
    {
        return m_stat.st_nlink;
    }

    // native
    //   function: the raw captured buffer, for the field this class did not
    // surface (uid, gid, dev, ino, the sub-second parts).
    const struct d_stat_t& native(void) const
    {
        return m_stat;
    }

private:

    // type_from_mode
    //   function: read the file type out of an st_mode, using the portable
    // S_IS* tests c/fs guarantees against a d_stat_t. The one place a mode is
    // interpreted; every predicate above reads the result.
    static file_type type_from_mode(uint32_t _mode)
    {
        if (S_ISREG(_mode))  { return type_regular;   }
        if (S_ISDIR(_mode))  { return type_directory; }
        if (S_ISLNK(_mode))  { return type_symlink;   }
        if (S_ISCHR(_mode))  { return type_character; }
        if (S_ISBLK(_mode))  { return type_block;     }
        if (S_ISFIFO(_mode)) { return type_fifo;      }
        if (S_ISSOCK(_mode)) { return type_socket;    }

        return type_unknown;
    }

    // zero
    //   function: clear the buffer for the field-less snapshots (none /
    // not_found), so their accessors read a defined 0 rather than garbage.
    void zero(void)
    {
        m_stat.st_size          = 0;
        m_stat.st_modified      = 0;
        m_stat.st_accessed      = 0;
        m_stat.st_changed       = 0;
        m_stat.st_created       = 0;
        m_stat.st_modified_nsec = 0;
        m_stat.st_accessed_nsec = 0;
        m_stat.st_changed_nsec  = 0;
        m_stat.st_mode          = 0;
        m_stat.st_nlink         = 0;
        m_stat.st_uid           = 0;
        m_stat.st_gid           = 0;
        m_stat.st_dev           = 0;
        m_stat.st_ino           = 0;
    }

    file_type       m_type;
    struct d_stat_t m_stat;
};


// ===========================================================================
// II.  QUERIES
// ===========================================================================

// d_internal_status_query
//   function: the shared body of status() and symlink_status() -- the only
// difference between them is whether a symlink is followed, which is the
// _follow flag. An absent file (ENOENT/ENOTDIR) is reported as type_not_found
// with NO error, because successfully learning a file is not there is not a
// failure; any other errno (a permission problem, say) is a real error and
// yields type_none with _ec set. This mirrors what a std::filesystem status
// query does with its error_code.
NS_INTERNAL
    inline file_status
    status_query(const path& _p, bool _follow, error& _ec)
    {
        struct d_stat_t buf;
        int             rc;

        if (!_p.valid())
        {
            _ec.assign(EINVAL);
            return file_status();
        }

        rc = _follow ? d_stat(_p.c_str(), &buf)
                     : d_lstat(_p.c_str(), &buf);

        if (rc != 0)
        {
            if (errno == ENOENT || errno == ENOTDIR)
            {
                _ec.clear();
                return file_status(file_status::type_not_found);
            }

            _ec = error::from_errno();
            return file_status();
        }

        _ec.clear();
        return file_status(buf);
    }
NS_END  // internal


// status
//   function: snapshot a path, FOLLOWING a final symlink to its target. The
// usual question -- "what is at this path".
inline file_status
status(const path& _p, error& _ec)
{
    return internal::status_query(_p, true, _ec);
}

// symlink_status
//   function: snapshot a path WITHOUT following a final symlink -- so a symlink
// reports as type_symlink rather than as whatever it points at. What you want
// when the link itself is the subject.
inline file_status
symlink_status(const path& _p, error& _ec)
{
    return internal::status_query(_p, false, _ec);
}


// ===========================================================================
// III. ONE-QUESTION HELPERS
// ===========================================================================

// exists
//   function: does the path name anything (following symlinks). One stat. An
// absent file is a plain false with no error; a permission failure sets _ec.
inline bool
exists(const path& _p, error& _ec)
{
    return status(_p, _ec).exists();
}

// is_regular_file
//   function: is it an ordinary file (following symlinks). One stat.
inline bool
is_regular_file(const path& _p, error& _ec)
{
    return status(_p, _ec).is_regular_file();
}

// is_directory
//   function: is it a directory (following symlinks). One stat.
inline bool
is_directory(const path& _p, error& _ec)
{
    return status(_p, _ec).is_directory();
}

// file_size
//   function: the size of a regular file, in bytes. A directory or special
// file is EINVAL -- their st_size is not a byte count -- and an absent file is
// ENOENT, so a 0 return always came with a cleared _ec from a real, empty file.
inline uint64_t
file_size(const path& _p, error& _ec)
{
    file_status s = status(_p, _ec);

    if (_ec.failed())
    {
        return 0;
    }

    if (!s.exists())
    {
        _ec.assign(ENOENT);
        return 0;
    }

    if (!s.is_regular_file())
    {
        _ec.assign(EINVAL);
        return 0;
    }

    _ec.clear();
    return s.size();
}
NS_END  // djinterp


#endif // DJINTERP_FS_FILE_STAT_