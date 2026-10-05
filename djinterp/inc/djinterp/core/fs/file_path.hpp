/*******************************************************************************
* djinterp [core]                                                  file_path.hpp
*
* djinterp::path -- a lexical path.
*   Header-only, C++98 through C++26, and a value type: copyable, comparable,
* and movable where the language has moves. It owns a d_string and calls
* c/fs/file_path for every decision, so C and C++ cannot disagree about what a
* path means -- there is one parser and it is the C one.
*   LEXICAL, like the module beneath it. parent() of "/nowhere/x" is
* "/nowhere" whether or not that exists, and normalized() does not resolve
* symlinks because it cannot see them. Nothing here touches a filesystem, so
* the whole type is testable with no disk, no permissions and no temp
* directory. To ask the filesystem, use d_path_resolve (file_dir).
*   NO OS ANYWHERE IN THIS FILE. Not one #if defined(_WIN32). Which grammar a
* path is in was decided by D_CFG_FILE_PATH_SYNTAX, and asking the platform
* again here would be a second answer to a settled question -- exactly the
* duplicate that made djinterp_qual_cfg.h and cfg_qualifiers.h a coin-flip.
*   FAILURE. A path owns a buffer, so construction can fail. With exceptions
* off -- C++98, -fno-exceptions -- a constructor cannot report that, so a
* failed one yields an INVALID path and every operation on an invalid path
* yields another invalid path. A failure propagates to wherever the caller
* actually looks, instead of being lost where it happened. Check valid(), or
* the bool conversion. D_CFG_PATH_THROW ADDS throwing on top; it never
* replaces this.
*
*
* path:      /inc/djinterp/core/fs/file_path.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LANGUAGE SUPPORT
    ----------------
    1.  Namespace macros
         1.  NS_DJINTERP / NS_INTERNAL / NS_END
2.  SCRATCH STORAGE
    ---------------
    1.  Lexical output buffer
         1.  path_scratch
3.  PATH
    ----
    1.  Private types
    2.  Construction and assignment
    3.  Observers
    4.  Decomposition
    5.  Composition
    6.  Inspection
    7.  Canonicalization
    8.  Comparison
    9.  Implementation
4.  FREE FUNCTIONS
    --------------
    1.  Composition
    2.  Exchange
*/

#ifndef DJINTERP_FS_FILE_PATH_HPP
#define DJINTERP_FS_FILE_PATH_HPP 1

// djinterp
#include "../../c/fs/file_path.h"                // d_path_* operations
#include "../../c/dstring.h"                     // d_string, the owned text
#include "file_common.hpp"                       // error, the D_* kit
#include "../../config/core/fs/cfg_file_path.h"  // D_INTERNAL_PATH_*
#include "../../env/env.h"                       // D_ENV_LANG_*
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)
// std
#if (D_INTERNAL_PATH_THROW == 1)
    #include <stdexcept>  // std::bad_alloc
#endif

#if (D_INTERNAL_PATH_HAS_SPACESHIP == 1)
    #include <compare>  // std::strong_ordering
    #include <cstring>  // std::strcmp
#endif


//==============================================================================
// 1.  LANGUAGE SUPPORT
//==============================================================================
// The move/noexcept/explicit spellings come from djinterp.hpp (D_MOVE_ENABLED,
// D_NOEXCEPT, D_EXPLICIT_BOOL), shared by every C++ module. path was one of
// the three headers that carried a private copy of that kit (D_PATH_*), and
// the promoted, single-source version keeps the tier rule it encoded: moves
// are ADDED on C++11, never substituted -- a C++98 caller writes copies, and
// that same source compiles on every later tier. (A destructive copy on C++98
// that became a real move on C++11 would compile in both and MEAN different
// things in each; that is what the rule avoids.)


// 1.1    Namespace macros
//------------------------------------------------------------------------------
// 1.1.1
// NS_DJINTERP / NS_INTERNAL / NS_END
//   macro: the namespace layer, from the root (djinterp.hpp, through
// file_common.hpp); this header kept a private copy until the root defined
// the kit (4.2). Everything public is FLAT in djinterp --
// djinterp::path, not djinterp::fs::path -- because the fs grouping is a
// property of the C modules' link granularity, and a C++ caller has no use
// for it. Implementation details go in djinterp::internal, which is the only
// nesting there is.


NS_DJINTERP


//==============================================================================
// 2.  SCRATCH STORAGE
//==============================================================================


NS_INTERNAL

    // 2.1    Lexical output buffer
    //--------------------------------------------------------------------------
    // 2.1.1
    // path_scratch
    //   class: a lexical operation's output buffer. The C module writes into a
    // caller-supplied buffer, so every lexical call needs one. This borrows the
    // stack until that is not enough, then allocates -- so the common case
    // costs nothing and the long-path case still works rather than failing.
    class path_scratch
    {
    public:
        /**
         * @brief Constructs a scratch buffer backed by the stack.
         */
        path_scratch(void)
            : m_heap(0)
        {
            m_buf = m_stack;
            m_size = (size_t)D_INTERNAL_PATH_STACK_BUF;
        }

        /**
         * @brief Releases the heap block, if the buffer ever grew into one.
         *
         * @post Every pointer obtained from data() is invalid.
         */
        ~path_scratch(void)
        {
            // only a grown buffer owns memory
            if (m_heap)
            {
                free(m_heap);
            }
        }

        /**
         * @brief Ensures the buffer holds at least `_need` bytes, moving to the
         *        heap when the stack cannot.
         *
         * @param[in] _need  the capacity required, in bytes.
         * @return true on success; false on allocation failure, which the
         *         caller turns into an invalid path.
         * @post On success, pointers from an earlier data() may be invalid.
         */
        bool grow(size_t _need)
        {
            // the buffer is already big enough
            if (_need <= m_size)
            {
                return true;
            }

            char* const block = (char*)realloc(m_heap,
                                               _need);

            // a failed realloc leaves the old block, and the buffer, intact
            if (!block)
            {
                return false;
            }

            m_heap = block;
            m_buf  = block;
            m_size = _need;

            return true;
        }

        // observers -- the current buffer and its capacity; neither can fail
        char* data(void)
        {
            return m_buf;
        }

        size_t size(void) const
        {
            return m_size;
        }

    private:
        // scratch is a borrowed buffer with a lifetime; copying one would give
        // two owners of m_heap
        path_scratch(const path_scratch&);
        path_scratch& operator=(const path_scratch&);

        char   m_stack[D_INTERNAL_PATH_STACK_BUF];
        char*  m_heap;
        char*  m_buf;
        size_t m_size;
    };

NS_END  // internal


//==============================================================================
// 3.  PATH
//==============================================================================


// path
//   class: a lexical path. Copyable, comparable, movable on C++11+. Every
// operation delegates to c/fs/file_path, so this type adds ownership and type
// safety and no logic of its own. If parent() ever disagrees with
// d_path_dirname, one of them is a bug.
class path
{
private:
    // 3.1    Private types
    //--------------------------------------------------------------------------
    // d_internal_invalid_tag
    //   type: selects the constructor that makes an invalid path without
    // asking whether this build wants to throw about it. It exists because the
    // two failure models collide otherwise: with D_CFG_PATH_THROW on,
    // `path((const char*)0)` -- which is how the lexical helpers below
    // reported failure -- would THROW bad_alloc out of a function that never
    // allocated, from a d_path_dirname that merely did not fit. The first
    // build with throwing enabled aborted on exactly that. A failed lexical
    // operation is not an allocation failure, so it must not be reported as
    // one; it yields an invalid path and propagates.
    struct d_internal_invalid_tag
    {};

    // fn_lex
    //   type: a c/fs lexical operation with the (path, buf, bufsize) shape.
    typedef char* (*fn_lex)(const char* _path, char* _buf, size_t _bufsize);

public:
    // 3.2    Construction and assignment
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs an empty path, which is valid.
     *
     * @throws std::bad_alloc only when D_CFG_PATH_THROW is 1 and the
     *         allocation fails; otherwise that leaves the path invalid.
     */
    path(void)
        : m_str(d_string_new())
    {
        d_internal_check();
    }

    /**
     * @brief Constructs a path from a C string.
     *
     * @note A `NULL` argument yields an INVALID path -- a NULL path is not an
     *       empty path, and conflating them is how a caller ends up operating
     *       on "" believing it has something. It never throws, even with
     *       D_CFG_PATH_THROW on: it is not a resource failure, so bad_alloc
     *       would be a lie about what went wrong, and the caller who passed
     *       NULL is not the caller who can handle memory exhaustion.
     *
     * @param[in] _cstr  the path text; copied.
     * @post valid() is false when `_cstr` was `NULL` or the copy failed.
     * @throws std::bad_alloc only when D_CFG_PATH_THROW is 1 and the copy
     *         cannot be allocated.
     */
    path(
        const char* _cstr
    )
        : m_str(_cstr ? d_string_new_from_cstr(_cstr) : 0)
    {
        // only a genuine allocation failure throws
        if (_cstr)
        {
            d_internal_check();
        }
    }

    /**
     * @brief Copies a path. Deep: two paths never share a buffer.
     *
     * @note Copying an INVALID path yields an invalid path and never throws,
     *       even with D_CFG_PATH_THROW on -- an already-invalid source is not
     *       a resource this operation ran out of.
     *
     * @param[in] _other  the path to copy.
     * @throws std::bad_alloc only when D_CFG_PATH_THROW is 1 and a valid
     *         source cannot be copied.
     */
    path(
        const path& _other
    )
        : m_str(_other.m_str ? d_string_new_copy(_other.m_str) : 0)
    {
        // The check guards on whether there was anything to copy, exactly as
        // the const char* constructor guards on whether there was anything to
        // parse. Without it, every operator/ onto an invalid path -- which
        // copies its left operand -- would abort a throwing build.
        if (_other.m_str)
        {
            d_internal_check();
        }
    }

    /**
     * @brief Copy-assigns a path.
     *
     * @param[in] _other  the path to copy.
     * @return `*this`; invalid if the copy could not be allocated.
     * @throws std::bad_alloc as for the copy constructor.
     */
    path& operator=(const path& _other)
    {
        // copy-and-swap: self-assignment and allocation failure are both
        // handled by construction rather than by a branch here
        path tmp(_other);
        swap(tmp);

        return *this;
    }

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief Moves a path, leaving the source invalid. C++11 and later.
     *
     * @note ADDITIVE: the copy above still exists and still works, so C++98
     *       source compiles here unchanged.
     *
     * @param[in,out] _other  the path to move from; invalid afterwards.
     */
    path(
        path&& _other
    ) D_NOEXCEPT
        : m_str(_other.m_str)
    {
        _other.m_str = 0;
    }

    /**
     * @brief Move-assigns a path, leaving the source invalid.
     *
     * @param[in,out] _other  the path to move from; invalid afterwards.
     * @return `*this`.
     */
    path& operator=(path&& _other) D_NOEXCEPT
    {
        // a self-move leaves the path as it was
        if (this != &_other)
        {
            // release what this path held before taking the other's text
            if (m_str)
            {
                d_string_free(m_str);
            }

            m_str = _other.m_str;
            _other.m_str = 0;
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    /**
     * @brief Releases the path's text.
     *
     * @post Every pointer obtained from c_str() or extension() is invalid.
     */
    ~path(void)
    {
        // an invalid path owns nothing
        if (m_str)
        {
            d_string_free(m_str);
        }
    }

    /**
     * @brief Exchanges two paths; cannot fail.
     *
     * @param[in,out] _other  the path to exchange with.
     */
    void swap(path& _other) D_NOEXCEPT
    {
        struct d_string* const tmp = m_str;

        m_str        = _other.m_str;
        _other.m_str = tmp;

        return;
    }

    // 3.3    Observers
    //--------------------------------------------------------------------------
    /**
     * @brief Reports whether construction succeeded.
     *
     * @note This is the failure channel that works on every tier. An invalid
     *       path is contagious -- every operation on one yields another -- so
     *       a failure arrives wherever the caller checks rather than being
     *       dropped where it happened.
     *
     * @return false after a failed allocation or a `NULL` C string.
     */
    bool valid(void) const
    {
        return (m_str != 0);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    /**
     * @brief valid(), spelled shorter: from C++11, as an explicit
     *        conversion; below, valid() is the spelling (decision 3.2).
     *
     * @note Absent below C++11, where only an implicit conversion exists:
     *       one would let `path_a < path_b` compile into a comparison of
     *       two booleans.
     *
     * @return valid().
     */
    D_EXPLICIT_BOOL operator bool(void) const
    {
        return valid();
    }
#endif

    /**
     * @brief Reports whether the path has no text.
     *
     * @note An INVALID path is empty too, but not the reverse: valid() is the
     *       one to ask to tell "" from "it failed".
     *
     * @return true for "" and for an invalid path.
     */
    bool empty(void) const
    {
        return ( (!m_str) ||
                 (d_string_length(m_str) == 0) );
    }

    /**
     * @brief Reports the path's length.
     *
     * @return the length in bytes, excluding the terminator; 0 when invalid.
     */
    size_t size(void) const
    {
        return m_str ? d_string_length(m_str) : 0;
    }

    /**
     * @brief Returns the path as a C string, for handing to the c/fs API.
     *
     * @note Never `NULL`, even when invalid: an invalid path reads as "". A
     *       caller who forgot to check valid() then passes "" to a C function
     *       that rejects it, rather than NULL to one that may not.
     *
     * @return the text, owned by this path.
     * @post The pointer is valid until this path is modified or destroyed.
     */
    const char* c_str(void) const
    {
        return m_str ? d_string_cstr(m_str) : "";
    }

    // 3.4    Decomposition
    //--------------------------------------------------------------------------
    /**
     * @brief Returns the directory component (d_path_dirname).
     *
     * @return the parent, or an invalid path when this one is invalid or the
     *         operation fails.
     */
    path parent(void) const
    {
        return d_internal_lex(&path::d_internal_dirname);
    }

    /**
     * @brief Returns the final component (d_path_basename).
     *
     * @return the final component, or an invalid path on failure.
     */
    path filename(void) const
    {
        return d_internal_lex(&path::d_internal_basename);
    }

    /**
     * @brief Returns the final component without its extension (d_path_stem).
     *
     * @return the stem, or an invalid path on failure.
     */
    path stem(void) const
    {
        return d_internal_lex(&path::d_internal_stem);
    }

    /**
     * @brief Returns the extension, dot included.
     *
     * @note A POINTER INTO this path, exactly as d_path_extension returns, so
     *       it costs nothing. "" rather than NULL when there is none: a caller
     *       comparing it to ".txt" should not have to null-check first.
     *
     * @return the extension, or "" when there is none or the path is invalid.
     * @post The pointer is valid until this path is modified or destroyed.
     */
    const char* extension(void) const
    {
        // an invalid path has no extension
        if (!m_str)
        {
            return "";
        }

        const char* const ext = d_path_extension(d_string_cstr(m_str));

        return ext ? ext : "";
    }

    // 3.5    Composition
    //--------------------------------------------------------------------------
    /**
     * @brief Joins another component onto this one with exactly one separator
     *        (d_path_join).
     *
     * @note An absolute `_other` REPLACES this path, which is what
     *       D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS decided in c/fs and is not
     *       re-decided here.
     *
     * @param[in] _other  the component to append.
     * @return `*this`, invalid if either path was invalid or the join failed.
     */
    path& append(const path& _other)
    {
        // an invalid operand makes the result invalid
        if ( (!m_str) ||
             (!_other.m_str) )
        {
            d_internal_invalidate();

            return *this;
        }

        // +2: the separator and the terminator
        const size_t           need = size() + _other.size() + 2;
        internal::path_scratch scratch;

        // a buffer that cannot grow invalidates the path
        if (!scratch.grow(need))
        {
            d_internal_invalidate();

            return *this;
        }

        // the join itself can refuse, for a result that does not fit
        if (!d_path_join(scratch.data(),
                         scratch.size(),
                         c_str(),
                         _other.c_str()))
        {
            d_internal_invalidate();

            return *this;
        }

        // keeping the result can still fail to allocate
        if (!d_string_assign_cstr(m_str,
                                  scratch.data()))
        {
            d_internal_invalidate();
        }

        return *this;
    }

    /**
     * @brief append(), spelled as the operator everyone reaches for.
     *
     * @param[in] _other  the component to append.
     * @return `*this`.
     */
    path& operator/=(const path& _other)
    {
        return append(_other);
    }

    // 3.6    Inspection
    //--------------------------------------------------------------------------
    /**
     * @brief Reports whether this path names a fixed starting point.
     *
     * @note d_path_is_absolute knows that "C:x" is drive-RELATIVE -- Win32
     *       keeps a per-drive cursor, and only "C:\x" is anchored.
     *
     * @return true for an absolute path; false otherwise and when invalid.
     */
    bool is_absolute(void) const
    {
        return m_str ? (d_path_is_absolute(d_string_cstr(m_str)) != 0) : false;
    }

    /**
     * @brief Reports whether this is a valid, relative path.
     *
     * @return true for a valid path that is not absolute.
     */
    bool is_relative(void) const
    {
        return ( (valid()) &&
                 (!is_absolute()) );
    }

    /**
     * @brief Reports the length of the leading root -- what ".." may never
     *        climb above.
     *
     * @return the root's length in bytes; 0 when there is none or the path is
     *         invalid.
     */
    size_t root_length(void) const
    {
        return m_str ? d_path_root_length(d_string_cstr(m_str)) : 0;
    }

    // 3.7    Canonicalization
    //--------------------------------------------------------------------------
    /**
     * @brief Returns a lexically cleaned copy: separator runs collapsed, "."
     *        dropped, ".." resolved against the preceding component.
     *
     * @warning LEXICAL, and the distinction is not academic: given /x/link ->
     *          /y/z, this says "/x/link/.." is "/x" because that is what the
     *          TEXT means, while the kernel says "/y". When the path names
     *          something that exists and the difference matters, ask the
     *          filesystem -- d_path_resolve, in file_dir.
     *
     * @return the normalized path, or an invalid path on failure.
     */
    path normalized(void) const
    {
        return d_internal_lex(&path::d_internal_norm);
    }

    // 3.8    Comparison
    //--------------------------------------------------------------------------
    /**
     * @brief Compares the text byte for byte.
     *
     * @note TEXTUAL, deliberately. "a/b", "a//b" and "./a/b" name the same
     *       file and compare UNEQUAL, because deciding otherwise would mean
     *       normalizing on every comparison -- and normalizing is lexical, so
     *       it would still be wrong across a symlink. Compare normalized() if
     *       that is what you meant; the call site should say so.
     *
     * @param[in] _other  the path to compare with.
     * @return true for identical text. Two invalid paths are equal; an invalid
     *         path equals nothing else, including an empty one.
     */
    bool operator==(const path& _other) const
    {
        // an invalid path equals only another invalid path
        if ( (!m_str) ||
             (!_other.m_str) )
        {
            return ( (!m_str) &&
                     (!_other.m_str) );
        }

        return d_string_equals(m_str,
                               _other.m_str);
    }

    /**
     * @brief The negation of operator==.
     *
     * @param[in] _other  the path to compare with.
     * @return true when the text differs.
     */
    bool operator!=(const path& _other) const
    {
        return !(*this == _other);
    }

#if (D_INTERNAL_PATH_HAS_SPACESHIP == 1)
    /**
     * @brief Orders paths byte-lexicographically. ADDED on C++20.
     *
     * @note The ==/!= above remain on every tier; this only supplies the
     *       relational operators, so a path becomes usable as a key in an
     *       ordered container without changing how any earlier standard
     *       compiles. It is consistent with operator== by construction: an
     *       invalid path orders BEFORE every valid one, and two invalids are
     *       equivalent. (Paths hold no embedded NUL, so a c_str() byte compare
     *       and the d_string equality operator== uses agree on path data.)
     *
     * @param[in] _other  the path to order against.
     * @return the ordering of the two texts.
     */
    std::strong_ordering operator<=>(const path& _other) const
    {
        const bool this_valid  = (m_str != 0);
        const bool other_valid = (_other.m_str != 0);

        // invalid paths order first, and equal only each other
        if ( (!this_valid) ||
             (!other_valid) )
        {
            // two invalid paths are equivalent
            if (this_valid == other_valid)
            {
                return std::strong_ordering::equal;
            }

            return this_valid ? std::strong_ordering::greater
                              : std::strong_ordering::less;
        }

        const int c = std::strcmp(c_str(),
                                  _other.c_str());

        return (c < 0) ? std::strong_ordering::less
             : (c > 0) ? std::strong_ordering::greater
             :           std::strong_ordering::equal;
    }
#endif  // D_INTERNAL_PATH_HAS_SPACESHIP

private:
    // 3.9    Implementation
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs an invalid path, unconditionally and without throwing.
     */
    path(d_internal_invalid_tag)
        : m_str(0)
    {}

    /**
     * @brief The throwing half, when a build has asked for one.
     *
     * @note With exceptions off this is nothing at all, and valid() carries
     *       the news instead -- which is why valid() is the primary channel and
     *       this is the addition.
     *
     * @throws std::bad_alloc when D_CFG_PATH_THROW is 1 and the path is
     *         invalid.
     */
    void d_internal_check(void) const
    {
#if (D_INTERNAL_PATH_THROW == 1)
        // an allocation this path needed did not happen
        if (!m_str)
        {
            throw std::bad_alloc();
        }
#endif

        return;
    }

    /**
     * @brief Returns the invalid path, for the failure paths below.
     *
     * @return an invalid path.
     */
    static path d_internal_invalid(void)
    {
        return path(d_internal_invalid_tag());
    }

    /**
     * @brief Marks this path failed, releasing what it held.
     *
     * @post valid() is false, and every pointer obtained from c_str() or
     *       extension() is invalid.
     */
    void d_internal_invalidate(void)
    {
        // an invalid path holds nothing to release
        if (m_str)
        {
            d_string_free(m_str);
            m_str = 0;
        }

        return;
    }

    // trampolines -- the c/fs lexical operations, adapted to fn_lex's shape
    static char* d_internal_dirname(const char* _path,
                                    char*       _buf,
                                    size_t      _bufsize)
    {
        return d_path_dirname(_path,
                              _buf,
                              _bufsize);
    }

    static char* d_internal_basename(const char* _path,
                                     char*       _buf,
                                     size_t      _bufsize)
    {
        return d_path_basename(_path,
                               _buf,
                               _bufsize);
    }

    static char* d_internal_stem(const char* _path,
                                 char*       _buf,
                                 size_t      _bufsize)
    {
        return d_path_stem(_path,
                           _buf,
                           _bufsize);
    }

    static char* d_internal_norm(const char* _path,
                                 char*       _buf,
                                 size_t      _bufsize)
    {
        return d_path_normalize(_path,
                                _buf,
                                _bufsize);
    }

    /**
     * @brief Runs one lexical operation into scratch and builds a path from
     *        the result.
     *
     * @note The four decompositions differ only by which C function they
     *       call, so they share this rather than repeating the
     *       scratch-grow-check dance four times -- exactly the kind of
     *       repetition that lets one of them drift.
     *
     * @param[in] _fn  the operation to run.
     * @return the result, or an invalid path when this path is invalid or the
     *         operation fails.
     */
    path d_internal_lex(fn_lex _fn) const
    {
        internal::path_scratch scratch;

        // an invalid path yields another
        if (!m_str)
        {
            return d_internal_invalid();
        }

        // +1 for the terminator; no lexical result here is longer than its
        // input, since every one of them removes text or rewrites it in place
        if (!scratch.grow(size() + 1))
        {
            return d_internal_invalid();
        }

        // an operation whose result does not fit yields an invalid path
        if (!_fn(d_string_cstr(m_str),
                 scratch.data(),
                 scratch.size()))
        {
            return d_internal_invalid();
        }

        return path(scratch.data());
    }

    struct d_string* m_str;
};


//==============================================================================
// 4.  FREE FUNCTIONS
//==============================================================================


// 4.1    Composition
//------------------------------------------------------------------------------
/**
 * @brief Joins two paths, yielding a third.
 *
 * @param[in] _lhs  the leading path.
 * @param[in] _rhs  the component to append.
 * @return the joined path, invalid when either operand is or the join fails.
 */
inline path
operator/(
    const path& _lhs,
    const path& _rhs
)
{
    path result(_lhs);

    result.append(_rhs);

    return result;
}

// 4.2    Exchange
//------------------------------------------------------------------------------
/**
 * @brief ADL-findable swap, so generic code picks this up rather than falling
 *        back to std::swap's move-move-move.
 *
 * @param[in,out] _a  the first path.
 * @param[in,out] _b  the second path.
 */
inline void
swap(
    path& _a,
    path& _b
) D_NOEXCEPT
{
    _a.swap(_b);

    return;
}


NS_END  // djinterp

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_PATH_HPP
