/*******************************************************************************
* djinterp [core]                                                  file_stat.hpp
*
* djinterp::file_status -- what the filesystem knows about a path, captured
* once (roadmap Phase 5).
*   This is the C++ answer to the warning at the top of file_stat.h: the
* convenience predicates each cost a syscall and each can disagree with the
* next if the file changes in between, so the right shape is to stat ONCE and
* read the fields.
*   file_status IS that one stat. Construct it (a single d_file_stat call),
* then ask it as many questions as you like -- type, size, timestamps,
* permissions -- and every answer describes the SAME snapshot.
* is_regular_file() and size() on one file_status cannot race each other,
* because there is no second syscall between them.
*   The free predicates below -- exists(), is_directory(), file_size() -- exist
* for the genuine one-question case, and each is honest about being one stat.
* When you have more than one question, take a file_status and ask it, not
* three of these.
*   TOCTOU. Every path-based query here is a hazard by construction: what
* status(p) saw and what the next open(p) gets may be two different files.
* When it matters, open the file and call file::status() -- an fstat on the
* live descriptor, which names one file for its whole lifetime and cannot be
* swapped underneath you. That method lives in file_stream.hpp; this header is
* what it returns.
*   A VALUE, not a resource. file_status owns nothing -- it is a struct and an
* enum. It copies freely on every tier, needs no move, and carries none of the
* D_*_ portability kit, because a snapshot has no handle to guard. Its tier
* ladder is therefore FLAT: the same source builds identically on every
* standard, which is the point -- not every type needs the ceremony file does.
*   NO OS. Type is read from st_mode with the S_IS* tests file_common.h
* guarantees against a d_stat_t (defined portably where the platform lacks
* them), the same way this layer uses SEEK_SET and D_LOCK_EX. There is no
* platform branch here; the c/fs stat module already normalized st_mode.
*
*
* path:      /inc/djinterp/core/fs/file_stat.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FILE STATUS
    -----------
    1.  File type
    2.  Construction
    3.  Type predicates
    4.  Size, times and permissions
    5.  Implementation
2.  QUERIES
    -------
    1.  Shared query
    2.  Snapshots
3.  ONE-QUESTION HELPERS
    --------------------
    1.  Predicates
    2.  Size
*/

#ifndef DJINTERP_FS_FILE_STAT_HPP
#define DJINTERP_FS_FILE_STAT_HPP 1

// std
#include <cerrno>  // ENOENT, ENOTDIR, EINVAL
// djinterp
#include "./file_path.hpp"         // path
#include "./file_common.hpp"       // error, the D_* kit
#include "../../c/fs/file_stat.h"  // d_file_stat, d_file_stat_nofollow,
                                   // d_stat_t, S_IS*
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


NS_DJINTERP


//==============================================================================
// 1.  FILE STATUS
//==============================================================================


// file_status
//   class: one filesystem snapshot of one path. A value type -- copy it, store
// it, compare its fields; it holds no handle.
class file_status
{
public:
    // 1.1    File type
    //--------------------------------------------------------------------------
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

    // 1.2    Construction
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs an empty snapshot: type_none, no query behind it.
     */
    file_status(void)
        : m_type(type_none)
    {
        zero();
    }

    /**
     * @brief Constructs a snapshot from a filled stat buffer, as the queries
     *        in section 2 do.
     *
     * @note The type is computed from st_mode once, here, so every predicate
     *       below is a comparison rather than another mask.
     *
     * @param[in] _buf  the filled buffer; copied.
     */
    explicit file_status(
        const struct d_stat_t& _buf
    )
        : m_type(type_from_mode(_buf.st_mode)),
          m_stat(_buf)
    {}

    /**
     * @brief Constructs a snapshot that carries only a type and no data -- for
     *        type_none and type_not_found, which have no fields to report.
     *
     * @param[in] _type  the type to report.
     */
    explicit file_status(
        file_type _type
    )
        : m_type(_type)
    {
        zero();
    }

    // 1.3    Type predicates
    //--------------------------------------------------------------------------
    /**
     * @brief Reports what the path names, as one value.
     *
     * @return the file type.
     */
    file_type type(void) const
    {
        return m_type;
    }

    /**
     * @brief Reports whether the path names something.
     *
     * @return false for both "not there" and "no query yet"; read type() to
     *         tell those apart.
     */
    bool exists(void) const
    {
        return ( (m_type != type_none) &&
                 (m_type != type_not_found) );
    }

    // one kind each -- none can fail, and all describe the one snapshot
    bool is_regular_file(void) const
    {
        return m_type == type_regular;
    }

    bool is_directory(void) const
    {
        return m_type == type_directory;
    }

    bool is_symlink(void) const
    {
        return m_type == type_symlink;
    }

    bool is_block_file(void) const
    {
        return m_type == type_block;
    }

    bool is_character_file(void) const
    {
        return m_type == type_character;
    }

    bool is_fifo(void) const
    {
        return m_type == type_fifo;
    }

    bool is_socket(void) const
    {
        return m_type == type_socket;
    }

    /**
     * @brief Reports whether the path exists but is none of the named kinds.
     *
     * @return true for type_unknown.
     */
    bool is_other(void) const
    {
        return m_type == type_unknown;
    }

    // 1.4    Size, times and permissions
    //--------------------------------------------------------------------------
    /**
     * @brief Reports the size field of the snapshot, in bytes.
     *
     * @note Meaningful for a regular file; for anything else it is whatever
     *       the platform put there, which is why the free file_size() refuses
     *       a non-regular file.
     *
     * @return the size, in bytes.
     */
    uint64_t size(void) const
    {
        return m_stat.st_size;
    }

    // timestamps -- Unix seconds. change_time is 0 on platforms with no
    // metadata-change concept; creation_time is 0 where unavailable and NEVER
    // falls back to change_time, so a 0 here means "not reported", not "the
    // epoch"
    int64_t modified_time(void) const
    {
        return m_stat.st_modified;
    }

    int64_t access_time(void) const
    {
        return m_stat.st_accessed;
    }

    int64_t change_time(void) const
    {
        return m_stat.st_changed;
    }

    int64_t creation_time(void) const
    {
        return m_stat.st_created;
    }

    /**
     * @brief Reports the permission bits: the low nine of the mode, rwx for
     *        user, group and other.
     *
     * @return the permission bits.
     */
    uint32_t permissions(void) const
    {
        return m_stat.st_mode & (uint32_t)0777;
    }

    /**
     * @brief Reports the full mode word, type bits and all, for a caller who
     *        wants to apply its own S_IS* or permission masks.
     *
     * @return the mode.
     */
    uint32_t mode(void) const
    {
        return m_stat.st_mode;
    }

    /**
     * @brief Reports how many names this file has.
     *
     * @return the hard-link count.
     */
    uint32_t hard_links(void) const
    {
        return m_stat.st_nlink;
    }

    /**
     * @brief Returns the raw captured buffer, for the fields this class does
     *        not surface (uid, gid, dev, ino, the sub-second parts).
     *
     * @return the buffer, owned by this snapshot.
     */
    const struct d_stat_t& native(void) const
    {
        return m_stat;
    }

private:
    // 1.5    Implementation
    //--------------------------------------------------------------------------
    /**
     * @brief Reads the file type out of an st_mode, using the portable S_IS*
     *        tests c/fs guarantees against a d_stat_t.
     *
     * @note The one place a mode is interpreted; every predicate above reads
     *       the result.
     *
     * @param[in] _mode  the mode to classify.
     * @return the matching file type, or type_unknown.
     */
    static file_type type_from_mode(uint32_t _mode)
    {
        // map the mode's file type onto a file_type, one kind at a time
        if (S_ISREG(_mode))
        {
            return type_regular;
        }

        if (S_ISDIR(_mode))
        {
            return type_directory;
        }

        if (S_ISLNK(_mode))
        {
            return type_symlink;
        }

        if (S_ISCHR(_mode))
        {
            return type_character;
        }

        if (S_ISBLK(_mode))
        {
            return type_block;
        }

        if (S_ISFIFO(_mode))
        {
            return type_fifo;
        }

        if (S_ISSOCK(_mode))
        {
            return type_socket;
        }

        return type_unknown;
    }

    /**
     * @brief Clears the buffer for the field-less snapshots (none and
     *        not_found), so their accessors read a defined 0 rather than
     *        garbage.
     */
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


//==============================================================================
// 2.  QUERIES
//==============================================================================


// 2.1    Shared query
//------------------------------------------------------------------------------
NS_INTERNAL

    /**
     * @brief The shared body of status() and symlink_status(); the only
     *        difference between them is whether a symlink is followed.
     *
     * @note An absent file (ENOENT or ENOTDIR) is reported as type_not_found
     *       with NO error, because successfully learning a file is not there
     *       is not a failure; any other errno -- a permission problem, say --
     *       is a real error and yields type_none with `_ec` set. This mirrors
     *       what a std::filesystem status query does with its error_code.
     *
     * @param[in]  _p       the path to query.
     * @param[in]  _follow  true to follow a final symlink to its target.
     * @param[out] _ec      cleared on success and for an absent file; EINVAL
     *                      for an invalid path, otherwise the platform's code.
     * @return the snapshot.
     */
    inline file_status
    status_query(
        const path& _p,
        bool        _follow,
        error&      _ec
    )
    {
        // an invalid path names nothing to query
        if (!_p.valid())
        {
            _ec.assign(EINVAL);

            return file_status();
        }

        struct d_stat_t buf;
        const int       rc = _follow ? d_file_stat(_p.c_str(),
                                                   &buf)
                                     : d_file_stat_nofollow(_p.c_str(),
                                                            &buf);

        // tell absence, which is an answer, from a failure to find out
        if (rc != 0)
        {
            // nothing there is not an error
            if ( (errno == ENOENT) ||
                 (errno == ENOTDIR) )
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

// 2.2    Snapshots
//------------------------------------------------------------------------------
/**
 * @brief Snapshots a path, FOLLOWING a final symlink to its target: the usual
 *        question, "what is at this path".
 *
 * @param[in]  _p   the path to query.
 * @param[out] _ec  cleared on success and for an absent file; set otherwise.
 * @return the snapshot; type_not_found for an absent file.
 */
inline file_status
status(
    const path& _p,
    error&      _ec
)
{
    return internal::status_query(_p,
                                  true,
                                  _ec);
}

/**
 * @brief Snapshots a path WITHOUT following a final symlink, so a symlink
 *        reports as type_symlink rather than as whatever it points at.
 *
 * @param[in]  _p   the path to query.
 * @param[out] _ec  cleared on success and for an absent file; set otherwise.
 * @return the snapshot; type_not_found for an absent file.
 */
inline file_status
symlink_status(
    const path& _p,
    error&      _ec
)
{
    return internal::status_query(_p,
                                  false,
                                  _ec);
}


//==============================================================================
// 3.  ONE-QUESTION HELPERS
//==============================================================================
// Each is one stat, following symlinks. An absent file is a plain false with
// no error; a failure to find out -- a permission problem, say -- sets _ec.


// 3.1    Predicates
//------------------------------------------------------------------------------
/**
 * @brief Reports whether a path names anything.
 *
 * @param[in]  _p   the path to test.
 * @param[out] _ec  cleared unless the question could not be answered.
 * @return true when the path exists.
 */
inline bool
exists(
    const path& _p,
    error&      _ec
)
{
    return status(_p,
                  _ec).exists();
}

/**
 * @brief Reports whether a path names an ordinary file.
 *
 * @param[in]  _p   the path to test.
 * @param[out] _ec  cleared unless the question could not be answered.
 * @return true for a regular file.
 */
inline bool
is_regular_file(
    const path& _p,
    error&      _ec
)
{
    return status(_p,
                  _ec).is_regular_file();
}

/**
 * @brief Reports whether a path names a directory.
 *
 * @param[in]  _p   the path to test.
 * @param[out] _ec  cleared unless the question could not be answered.
 * @return true for a directory.
 */
inline bool
is_directory(
    const path& _p,
    error&      _ec
)
{
    return status(_p,
                  _ec).is_directory();
}

// 3.2    Size
//------------------------------------------------------------------------------
/**
 * @brief Reports the size of a regular file, in bytes.
 *
 * @param[in]  _p   the file to measure.
 * @param[out] _ec  cleared on success; ENOENT for an absent file, EINVAL for
 *                  a directory or special file (whose st_size is not a byte
 *                  count), otherwise the platform's code.
 * @return the size; 0 on failure, so a 0 with a cleared `_ec` always came
 *         from a real, empty file.
 */
inline uint64_t
file_size(
    const path& _p,
    error&      _ec
)
{
    const file_status s = status(_p,
                                 _ec);

    // the query itself failed
    if (_ec.failed())
    {
        return 0;
    }

    // nothing there to measure
    if (!s.exists())
    {
        _ec.assign(ENOENT);

        return 0;
    }

    // only a regular file's size is a byte count
    if (!s.is_regular_file())
    {
        _ec.assign(EINVAL);

        return 0;
    }

    _ec.clear();

    return s.size();
}


NS_END  // djinterp

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_STAT_HPP
