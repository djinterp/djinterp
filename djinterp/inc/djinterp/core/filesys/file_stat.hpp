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
* for the genuine one-question case, and each is honest about being one query.
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
* guarantees (defined portably where the platform lacks them), the same way
* this layer uses SEEK_SET and D_LOCK_EX. There is no platform branch here;
* the c/filesys stat module already normalized st_mode.
*   WIDTH. A snapshot holds the C layer's d_stat_t, so size_type and
* time_type are 64-bit where D_FILE_WIDTH is 64 and 32-bit where it is 32,
* and there a status query fails with EOVERFLOW when a size, a time, a device
* or an inode number does not fit. Where a language level has had to narrow
* the width the rest of the build keeps at 64 (D_FILE_WIDTH_IS_NARROWED),
* file_status and its queries are absent instead, since no level changes what
* a shared type is. What a path NAMES needs no width: file_kind and the type
* queries ask for the mode alone (d_file_mode), so they answer on every
* build -- a file too large for a 32-bit snapshot still exists, and is still
* a regular file.
*
*
* path:      /inc/djinterp/core/filesys/file_stat.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FILE KIND
    ---------
    1.  File type
    2.  Classification
2.  TYPE QUERIES
    ------------
    1.  Shared query
    2.  Predicates
3.  FILE STATUS
    -----------
    1.  Types
    2.  Construction
    3.  Type predicates
    4.  Size, times and permissions
    5.  Implementation
4.  STATUS QUERIES
    --------------
    1.  Shared query
    2.  Snapshots
    3.  Size
*/

#ifndef DJINTERP_FILESYS_FILE_STAT_HPP
#define DJINTERP_FILESYS_FILE_STAT_HPP 1

// std
#include <cerrno>  // ENOENT, ENOTDIR, EINVAL
// djinterp
#include "./file_path.hpp"              // path
#include "./file_common.hpp"            // error, the D_* kit
#include "../../c/filesys/file_stat.h"  // d_file_stat, d_file_mode,
                                        // d_stat_t, S_IS*
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, uint32_t, int64_t,
                                              // uint64_t


NS_DJINTERP


//==============================================================================
// 1.  FILE KIND
//==============================================================================


// file_kind
//   class: what a path names, with no width -- the vocabulary file_status, a
// directory entry and the type queries share. It holds no data, so it exists
// on every build, and file_status inherits it.
class file_kind
{
public:
    // 1.1    File type
    //--------------------------------------------------------------------------
    // file_type
    //   enum: what a path names. A plain (unscoped) enum so it is available
    // unchanged on C++98; enum class would need a portability shim this
    // vocabulary is deliberately without. type_none is "no query has run";
    // not_found is "the query ran and there is nothing there" -- a real,
    // useful distinction (a permission error is none, an absent file is
    // not_found).
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

    // 1.2    Classification
    //--------------------------------------------------------------------------
    /**
     * @brief Reads the file type out of a mode word, using the portable S_IS*
     *        tests c/filesys guarantees.
     *
     * @note The one place a mode is interpreted: a snapshot classifies its
     *       own st_mode with it, and the type queries the mode d_file_mode
     *       reports.
     *
     * @param[in] _mode  the mode to classify.
     * @return the matching file type, or type_unknown.
     */
    static file_type type_of_mode(uint32_t _mode)
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
};


//==============================================================================
// 2.  TYPE QUERIES
//==============================================================================
// What a path names, from its mode alone. Each is one query, following
// symlinks. An absent file is a plain false with no error; a failure to find
// out -- a permission problem, say -- sets _ec. The mode has no width, so
// these answer on every build: a file too large for a 32-bit snapshot still
// exists, and is still a regular file.


// 2.1    Shared query
//------------------------------------------------------------------------------
NS_INTERNAL

    /**
     * @brief The shared body of the predicates below, and of the recursion's
     *        classification: what a path names, from its mode alone, so the
     *        answer never hangs on a size or a time fitting D_FILE_WIDTH.
     *
     * @note Absence and failure are told apart exactly as status_query tells
     *       them.
     *
     * @param[in]  _p       the path to query.
     * @param[in]  _follow  true to follow a final symlink to its target.
     * @param[out] _ec      cleared on success and for an absent file; EINVAL
     *                      for an invalid path, otherwise the platform's code.
     * @return the type; type_not_found for an absent file, type_none on
     *         failure.
     */
    inline file_kind::file_type
    type_query(
        const path& _p,
        bool        _follow,
        error&      _ec
    )
    {
        // an invalid path names nothing to query
        if (!_p.valid())
        {
            _ec.assign(EINVAL);

            return file_kind::type_none;
        }

        uint32_t  mode = 0;
        const int rc   = _follow ? d_file_mode(_p.c_str(),
                                               &mode)
                                 : d_file_mode_nofollow(_p.c_str(),
                                                        &mode);

        // tell absence, which is an answer, from a failure to find out
        if (rc != 0)
        {
            // nothing there is not an error
            if ( (errno == ENOENT) ||
                 (errno == ENOTDIR) )
            {
                _ec.clear();

                return file_kind::type_not_found;
            }

            _ec = error::from_errno();

            return file_kind::type_none;
        }

        _ec.clear();

        return file_kind::type_of_mode(mode);
    }

NS_END  // internal

// 2.2    Predicates
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
    const file_kind::file_type t = internal::type_query(_p,
                                                          true,
                                                          _ec);

    return ( (t != file_kind::type_none) &&
             (t != file_kind::type_not_found) );
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
    return internal::type_query(_p,
                                true,
                                _ec) == file_kind::type_regular;
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
    return internal::type_query(_p,
                                true,
                                _ec) == file_kind::type_directory;
}


// file_status and its queries carry a width; where this level has had to
// narrow it, they are absent rather than a different type (see the banner)
#if !D_FILE_WIDTH_IS_NARROWED


//==============================================================================
// 3.  FILE STATUS
//==============================================================================


// file_status
//   class: one filesystem snapshot of one path. A value type -- copy it, store
// it, compare its fields; it holds no handle. Its vocabulary of kinds is
// file_kind's, inherited, so file_status::type_regular still names it.
class file_status : public file_kind
{
public:
    // 3.1    Types
    //--------------------------------------------------------------------------
    // size_type / time_type
    //   type: the snapshot's byte count and its Unix seconds, at the C layer's
    // D_FILE_WIDTH -- the types of d_stat_t's st_size and st_modified.
#if (D_FILE_WIDTH == 64)
    typedef uint64_t size_type;
    typedef int64_t  time_type;
#else
    typedef uint32_t size_type;
    typedef int32_t  time_type;
#endif  // D_FILE_WIDTH == 64

    // 3.2    Construction
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
     *        in section 4 do.
     *
     * @note The type is computed from st_mode once, here, so every predicate
     *       below is a comparison rather than another mask.
     *
     * @param[in] _buf  the filled buffer; copied.
     */
    explicit file_status(
        const struct d_stat_t& _buf
    )
        : m_type(type_of_mode(_buf.st_mode)),
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

    // 3.3    Type predicates
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

    // 3.4    Size, times and permissions
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
    size_type size(void) const
    {
        return m_stat.st_size;
    }

    // timestamps -- Unix seconds. change_time is 0 on platforms with no
    // metadata-change concept; creation_time is 0 where unavailable and NEVER
    // falls back to change_time, so a 0 here means "not reported", not "the
    // epoch"
    time_type modified_time(void) const
    {
        return m_stat.st_modified;
    }

    time_type access_time(void) const
    {
        return m_stat.st_accessed;
    }

    time_type change_time(void) const
    {
        return m_stat.st_changed;
    }

    time_type creation_time(void) const
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
    // 3.5    Implementation
    //--------------------------------------------------------------------------
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
// 4.  STATUS QUERIES
//==============================================================================
// Each is one stat. At D_FILE_WIDTH 32 a value the snapshot cannot hold fails
// the query with EOVERFLOW, as d_file_stat32 does.


// 4.1    Shared query
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
     *                      for an invalid path, EOVERFLOW at D_FILE_WIDTH 32
     *                      for a value d_stat_t cannot hold, otherwise the
     *                      platform's code.
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

        struct d_stat_t buf = d_stat_t();
        const int       rc  = _follow ? d_file_stat(_p.c_str(),
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

// 4.2    Snapshots
//------------------------------------------------------------------------------
/**
 * @brief Snapshots a path, FOLLOWING a final symlink to its target: the usual
 *        question, "what is at this path".
 *
 * @param[in]  _p   the path to query.
 * @param[out] _ec  cleared on success and for an absent file; set otherwise,
 *                  EOVERFLOW at D_FILE_WIDTH 32 included.
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

// 4.3    Size
//------------------------------------------------------------------------------
/**
 * @brief Reports the size of a regular file, in bytes -- one stat, following
 *        symlinks.
 *
 * @param[in]  _p   the file to measure.
 * @param[out] _ec  cleared on success; ENOENT for an absent file, EINVAL for
 *                  a directory or special file (whose st_size is not a byte
 *                  count), EOVERFLOW at D_FILE_WIDTH 32 for a file whose
 *                  status does not fit, otherwise the platform's code.
 * @return the size; 0 on failure, so a 0 with a cleared `_ec` always came
 *         from a real, empty file.
 */
inline file_status::size_type
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

#endif  // !D_FILE_WIDTH_IS_NARROWED


NS_END  // djinterp

#endif  // DJINTERP_FILESYS_FILE_STAT_HPP
