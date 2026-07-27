/******************************************************************************
* djinterp [fs]                                                  file_path.hpp
*
* djinterp::path -- a lexical path.
*
*   Header-only, C++98 through C++26, and a value type: copyable, comparable,
* and movable where the language has moves. It owns a d_string and calls
* c/fs/file_path for every decision, so C and C++ cannot disagree about what
* a path means -- there is one parser and it is the C one.
*
*   LEXICAL, like the module beneath it. parent() of "/nowhere/x" is
* "/nowhere" whether or not that exists, and normalized() does not resolve
* symlinks because it cannot see them. Nothing here touches a filesystem, so
* the whole type is testable with no disk, no permissions and no temp
* directory. To ask the filesystem, use d_realpath (file_dir).
*
*   NO OS ANYWHERE IN THIS FILE. Not one #if defined(_WIN32). Which grammar a
* path is in was decided by D_CFG_FILE_PATH_SYNTAX, and asking the platform
* again here would be a second answer to a settled question -- exactly the
* duplicate that made djinterp_qual_cfg.h and cfg_qualifiers.h a coin-flip.
*
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
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    LANGUAGE SUPPORT
      ----------------
      1.  NS_DJINTERP / NS_INTERNAL / NS_END
      2.  D_NOEXCEPT / D_MOVE_ENABLED / D_EXPLICIT_BOOL

II.   djinterp::path
      ---------------
      1.  Construction / assignment / destruction
      2.  Observers          empty, size, c_str, valid
      3.  Decomposition      parent, filename, stem, extension
      4.  Composition        append, operator/=
      5.  Inspection         is_absolute, root_length
      6.  Canonicalization   normalized
      7.  Comparison         operator== / != / <=> (C++20)

III.  FREE FUNCTIONS
      --------------
      1.  operator/
*/

#ifndef DJINTERP_FS_FILE_PATH_
#define DJINTERP_FS_FILE_PATH_ 1

#include "../../c/fs/file_path.h"
#include "../../c/dstring.h"
#include "file_common.hpp"          // common base: prelude (D_* kit) + error


// 0.    CONFIGURATION
//   These two knobs used to live in a config/cpp/ header. That module is gone,
// and they are declared here rather than re-homed somewhere central because
// this header is their ONLY consumer -- a knob read by one module belongs with
// that module, which is what keeps demand-loading honest. If a config/core/fs/
// tree ever appears they move there together, unchanged. The idiom is the
// framework's: an overridable D_CFG_*, validated, resolved into a D_INTERNAL_*
// that the code reads.
//   D_CFG_IS_BOOL / D_CFG_IS_ON reach here via file_common.hpp -> the C
// file_common.h -> cfg_file_common.h -> cfg_common.h.

// D_CFG_PATH_THROW
//   brief: 1 makes the throwing constructors throw std::bad_alloc when they
// cannot allocate. Off by default: the error-code surface is the real API and
// works with -fno-exceptions.
//   ONLY A RESOURCE FAILURE THROWS, even with this on. Three things stay
// non-throwing because none of them is one: path(NULL) (a caller bug, and
// bad_alloc would be a lie about what happened); a lexical operation whose
// result does not fit (nothing was allocated -- d_dirname simply said no); and
// any operation on an already-invalid path (the failure already happened).
// Conflating the invalid-path model with the throwing model is what made
// `path((const char*)0)` raise bad_alloc out of a function that never
// allocated, so the two are kept apart by a private tag constructor that does
// not consult this knob at all.
#ifndef D_CFG_PATH_THROW
#   define D_CFG_PATH_THROW         0
#endif

// D_CFG_PATH_STACK_BUF
//   brief: bytes of stack a lexical operation borrows before it allocates.
// 512 rather than D_FILE_PATH_MAX: on Linux that is 4096, and 4 KiB per frame
// in a recursive walk is a real price for a case that essentially never
// happens. A longer path still works -- it just costs one allocation.
#ifndef D_CFG_PATH_STACK_BUF
#   define D_CFG_PATH_STACK_BUF     512
#endif

#if !D_CFG_IS_BOOL(D_CFG_PATH_THROW)
#   error "D_CFG_PATH_THROW must be 0 or 1"
#endif
#if ( (D_CFG_PATH_STACK_BUF + 0) <= 0 )
#   error "D_CFG_PATH_STACK_BUF must be a positive byte count"
#endif

#define D_INTERNAL_PATH_THROW       (D_CFG_PATH_THROW + 0)
#define D_INTERNAL_PATH_STACK_BUF   (D_CFG_PATH_STACK_BUF + 0)

#if (D_INTERNAL_PATH_THROW == 1)
    #include <stdexcept>
#endif

// C++20 three-way comparison support (see operator<=> below). Gated on the
// standard language feature-test macro -- NOT on __cplusplus, whose C++23 value
// varies by compiler; the feature-test macro is the portable signal.
#if defined(__cpp_impl_three_way_comparison) && (__cpp_impl_three_way_comparison >= 201907L)
    #include <compare>
    #include <cstring>
    #define D_INTERNAL_PATH_HAS_SPACESHIP 1
#else
    #define D_INTERNAL_PATH_HAS_SPACESHIP 0
#endif


// I.    Language support

// NS_DJINTERP / NS_INTERNAL / NS_END
//   macro: the namespace layer. Everything public is FLAT in djinterp --
// djinterp::path, not djinterp::fs::path -- because the fs grouping is a
// property of the C modules' link granularity, and a C++ caller has no use
// for it. Implementation details go in djinterp::internal, which is the only
// nesting there is.
#ifndef NS_DJINTERP
    #define NS_DJINTERP namespace djinterp {
#endif
#ifndef NS_INTERNAL
    #define NS_INTERNAL namespace internal {
#endif
#ifndef NS_END
    #define NS_END }
#endif

// The move/noexcept/explicit spellings now come from djinterp.hpp
// (D_MOVE_ENABLED, D_NOEXCEPT, D_EXPLICIT_BOOL), shared by every C++ module.
// path was one of the three headers that carried a private copy of this kit
// (D_PATH_*); this is the promoted, single-source version. The tier rule those
// macros encode is unchanged: moves are ADDED on C++11, never substituted -- a
// C++98 caller writes copies, and that same source compiles on every later
// tier. (A destructive copy on C++98 that became a real move on C++11 would
// compile in both and MEAN different things in each; that is what this avoids.)


NS_DJINTERP

NS_INTERNAL

// internal::path_scratch
//   class: a lexical operation's output buffer.
//   The C module writes into a caller-supplied buffer, so every lexical call
// needs one. This borrows the stack until that is not enough, then allocates
// -- so the common case costs nothing and the long-path case still works
// rather than failing. D_CFG_PATH_STACK_BUF is 512 rather than PATH_MAX
// because 4 KiB per frame, in a recursive walk, is a real price for a case
// that essentially never occurs.
class path_scratch
{
public:
    path_scratch(void)
        : m_heap(0)
    {
        m_buf = m_stack;
        m_size = (size_t)D_INTERNAL_PATH_STACK_BUF;
    }

    ~path_scratch(void)
    {
        if (m_heap)
        {
            free(m_heap);
        }
    }

    // grow
    //   function: ensure at least _need bytes, moving to the heap if the
    // stack cannot hold them. Returns false on allocation failure, which the
    // caller turns into an invalid path.
    bool grow(size_t _need)
    {
        char* block;

        if (_need <= m_size)
        {
            return true;
        }

        block = (char*)realloc(m_heap, _need);

        if (!block)
        {
            return false;
        }

        m_heap = block;
        m_buf  = block;
        m_size = _need;

        return true;
    }

    char*  data(void) { return m_buf; }
    size_t size(void) const { return m_size; }

private:
    char   m_stack[D_INTERNAL_PATH_STACK_BUF];
    char*  m_heap;
    char*  m_buf;
    size_t m_size;

    // scratch is a borrowed buffer with a lifetime; copying one would give
    // two owners of m_heap
    path_scratch(const path_scratch&);
    path_scratch& operator=(const path_scratch&);
};

NS_END  // internal


// II.   djinterp::path

// path
//   class: a lexical path. Copyable, comparable, movable on C++11+.
//   Every operation delegates to c/fs/file_path, so this type adds ownership
// and type safety and no logic of its own. If parent() ever disagrees with
// d_dirname, one of them is a bug.
class path
{
public:

    // --- II.1  Construction ---

    // path
    //   function: an empty path. Always valid.
    path(void)
        : m_str(d_string_new())
    {
        d_internal_check();
    }

    // path
    //   function: from a C string. Invalid if the allocation fails, or if
    // _cstr is NULL -- a NULL path is not an empty path, and conflating them
    // is how a caller ends up operating on "" believing it has something.
    //
    //   A NULL argument does NOT throw, even with D_CFG_PATH_THROW on. It is
    // not a resource failure -- there is nothing to be out of -- so
    // bad_alloc would be a lie about what went wrong, and the caller who
    // passed NULL is not the caller who can handle a memory exhaustion.
    // Only a genuine allocation failure throws.
    path(const char* _cstr)
        : m_str(_cstr ? d_string_new_from_cstr(_cstr) : 0)
    {
        if (_cstr)
        {
            d_internal_check();
        }
    }

    // path
    //   function: copy. Deep -- two paths never share a buffer.
    //
    //   Copying an INVALID path yields an invalid path and does NOT throw,
    // even with D_CFG_PATH_THROW on -- for the same reason path(NULL) does
    // not (above): an already-invalid source is not a resource this operation
    // ran out of. Only a failed copy of a VALID source is an allocation
    // failure, and only that throws. The check therefore guards on whether
    // there was anything to copy, exactly as the const char* constructor
    // guards on whether there was anything to parse -- without this, every
    // operator/ onto an invalid path (which copies its left operand) would
    // abort a throwing build.
    path(const path& _other)
        : m_str(_other.m_str ? d_string_new_copy(_other.m_str) : 0)
    {
        if (_other.m_str)
        {
            d_internal_check();
        }
    }

    // operator=
    //   function: copy-assign, via copy-and-swap, so self-assignment and
    // allocation failure are both handled by construction rather than by a
    // branch here.
    path& operator=(const path& _other)
    {
        path tmp(_other);
        swap(tmp);

        return *this;
    }

#if (D_MOVE_ENABLED == 1)
    // path
    //   function: move. C++11+ only, and ADDITIVE -- the copy above still
    // exists and still works, so C++98 source compiles here unchanged.
    path(path&& _other) D_NOEXCEPT
        : m_str(_other.m_str)
    {
        _other.m_str = 0;
    }

    // operator=
    //   function: move-assign.
    path& operator=(path&& _other) D_NOEXCEPT
    {
        if (this != &_other)
        {
            if (m_str)
            {
                d_string_free(m_str);
            }

            m_str = _other.m_str;
            _other.m_str = 0;
        }

        return *this;
    }
#endif

    ~path(void)
    {
        if (m_str)
        {
            d_string_free(m_str);
        }
    }

    // swap
    //   function: exchange two paths. Cannot fail.
    void swap(path& _other) D_NOEXCEPT
    {
        struct d_string* tmp;

        tmp          = m_str;
        m_str        = _other.m_str;
        _other.m_str = tmp;

        return;
    }


    // --- II.2  Observers ---

    // valid
    //   function: false when construction failed -- an allocation the caller
    // could not be told about, or a NULL C string.
    //   This is the failure channel that works on every tier. An invalid path
    // is contagious: every operation on one yields another invalid one, so a
    // failure arrives wherever the caller checks rather than being dropped
    // where it happened.
    bool valid(void) const { return (m_str != 0); }

    // operator bool
    //   function: valid(), spelled shorter.
    //   NOT a safe-bool idiom on C++98. The idiom exists to stop an implicit
    // bool from decaying to int and letting `path_a < path_b` compile into a
    // comparison of two booleans -- but this class defines no relational
    // operators, so the hole it plugs is not open. Adding operator< later
    // means adding the idiom, or an explicit conversion, at the same time.
    D_EXPLICIT_BOOL operator bool(void) const { return valid(); }

    // empty
    //   function: true when the path has no text. An INVALID path is empty
    // too -- but the reverse does not hold, and valid() is the one to ask if
    // you need to tell "" from "it failed".
    bool empty(void) const
    {
        return (!m_str) || (d_string_length(m_str) == 0);
    }

    // size
    //   function: length in bytes, excluding the terminator. 0 when invalid.
    size_t size(void) const
    {
        return m_str ? d_string_length(m_str) : 0;
    }

    // c_str
    //   function: the path as a C string, for handing to the c/fs API.
    //   Never NULL, even when invalid -- an invalid path reads as "". A
    // caller who forgot to check valid() then passes "" to a C function that
    // rejects it, rather than passing NULL to one that may not.
    const char* c_str(void) const
    {
        return m_str ? d_string_cstr(m_str) : "";
    }


    // --- II.3  Decomposition ---

    // parent
    //   function: the directory component. d_dirname.
    path parent(void) const { return d_internal_lex(&path::d_internal_dirname); }

    // filename
    //   function: the final component. d_basename.
    path filename(void) const { return d_internal_lex(&path::d_internal_basename); }

    // stem
    //   function: the final component without its extension. d_path_stem.
    path stem(void) const { return d_internal_lex(&path::d_internal_stem); }

    // extension
    //   function: the extension including the dot, or "" when there is none.
    //   Returns a POINTER INTO this path, exactly as d_get_extension does, so
    // it costs nothing and lives as long as this path does. "" rather than
    // NULL for the no-extension case: a caller comparing it to ".txt" should
    // not have to null-check first.
    const char* extension(void) const
    {
        const char* ext;

        if (!m_str)
        {
            return "";
        }

        ext = d_get_extension(d_string_cstr(m_str));

        return ext ? ext : "";
    }


    // --- II.4  Composition ---

    // append
    //   function: join another component onto this one, with exactly one
    // separator. d_path_join -- including its rule that an absolute second
    // component REPLACES the first, which is what D_CFG_FILE_PATH_JOIN_-
    // ABSOLUTE_WINS decided and is not re-decided here.
    path& append(const path& _other)
    {
        internal::path_scratch scratch;
        size_t                 need;

        if ( (!m_str) ||
             (!_other.m_str) )
        {
            d_internal_invalidate();

            return *this;
        }

        // +2: the separator and the terminator
        need = size() + _other.size() + 2;

        if (!scratch.grow(need))
        {
            d_internal_invalidate();

            return *this;
        }

        if (!d_path_join(scratch.data(),
                         scratch.size(),
                         c_str(),
                         _other.c_str()))
        {
            d_internal_invalidate();

            return *this;
        }

        if (!d_string_assign_cstr(m_str, scratch.data()))
        {
            d_internal_invalidate();
        }

        return *this;
    }

    // operator/=
    //   function: append, spelled as the operator everyone reaches for.
    path& operator/=(const path& _other) { return append(_other); }


    // --- II.5  Inspection ---

    // is_absolute
    //   function: does this path name a fixed starting point.
    //   d_path_is_absolute, which knows that "C:x" is drive-RELATIVE -- Win32
    // keeps a per-drive cursor, and only "C:\x" is anchored. An invalid path
    // is not absolute.
    bool is_absolute(void) const
    {
        return m_str ? (d_path_is_absolute(d_string_cstr(m_str)) != 0) : false;
    }

    // is_relative
    //   function: the complement, for a valid path.
    bool is_relative(void) const { return valid() && (!is_absolute()); }

    // root_length
    //   function: bytes of leading root -- what ".." may never climb above.
    size_t root_length(void) const
    {
        return m_str ? d_path_root_length(d_string_cstr(m_str)) : 0;
    }


    // --- II.6  Canonicalization ---

    // normalized
    //   function: a lexically cleaned copy -- separator runs collapsed, "."
    // dropped, ".." resolved against the preceding component.
    //   LEXICAL, and the distinction is not academic: given /x/link -> /y/z,
    // this says "/x/link/.." is "/x" because that is what the TEXT means,
    // while the kernel says "/y". Both are defensible and they are not the
    // same answer. When the path names something that exists and the
    // difference matters, ask the filesystem -- d_realpath, in file_dir.
    path normalized(void) const { return d_internal_lex(&path::d_internal_norm); }


    // --- II.7  Comparison ---

    // operator==
    //   function: byte equality of the text.
    //   TEXTUAL, deliberately. "a/b" and "a//b" and "./a/b" name the same
    // file and compare UNEQUAL, because deciding otherwise would mean
    // normalizing on every comparison -- and normalizing is lexical, so it
    // would still be wrong across a symlink. Compare normalized() if that is
    // what you meant; the call site should say so.
    //   Two invalid paths are equal; an invalid path equals nothing else,
    // including an empty one.
    bool operator==(const path& _other) const
    {
        if ( (!m_str) ||
             (!_other.m_str) )
        {
            return ((!m_str) && (!_other.m_str));
        }

        return d_string_equals(m_str, _other.m_str);
    }

    bool operator!=(const path& _other) const { return !(*this == _other); }

#if (D_INTERNAL_PATH_HAS_SPACESHIP == 1)
    // operator<=>
    //   function: byte-lexicographic ORDERING, ADDED on C++20. The ==/!= above
    // remain on every tier; this only supplies the relational operators (< <= >
    // >=), so a path becomes usable as a key in an ordered container (std::set,
    // std::map, sort) without changing how any earlier standard compiles. This
    // is the tier ladder at its top -- a higher standard ADDS capability, never
    // alters the lower-tier surface.
    //   Consistent with operator== by construction: a <=> b is `equal` exactly
    // when a == b. An invalid path orders BEFORE every valid one, and two
    // invalids are equivalent -- matching == treating invalid as equal only to
    // invalid. (Paths hold no embedded NUL, so a c_str() byte compare and the
    // d_string equality operator== uses agree on path data.)
    std::strong_ordering operator<=>(const path& _other) const
    {
        const bool this_valid  = (m_str != 0);
        const bool other_valid = (_other.m_str != 0);

        if (!this_valid || !other_valid)
        {
            if (this_valid == other_valid)
            {
                return std::strong_ordering::equal;
            }

            return this_valid ? std::strong_ordering::greater
                              : std::strong_ordering::less;
        }

        const int c = std::strcmp(c_str(), _other.c_str());

        return (c < 0) ? std::strong_ordering::less
             : (c > 0) ? std::strong_ordering::greater
             :           std::strong_ordering::equal;
    }
#endif

private:

    // d_internal_invalid_tag
    //   type: selects the constructor that makes an invalid path without
    // asking whether this build wants to throw about it.
    //   It exists because the two failure models collide otherwise. With
    // D_CFG_PATH_THROW on, `path((const char*)0)` -- which is how the lexical
    // helpers below reported failure -- would THROW bad_alloc out of a
    // function that never allocated, from a d_dirname that merely did not
    // fit. The first build with throwing enabled aborted on exactly that.
    //   A failed lexical operation is not an allocation failure, so it must
    // not be reported as one. It yields an invalid path and propagates.
    struct d_internal_invalid_tag {};

    // path
    //   function: an invalid path, unconditionally and without throwing.
    path(d_internal_invalid_tag)
        : m_str(0)
    {
    }

    struct d_string* m_str;

    // d_internal_check
    //   function: the throwing half, when a build has asked for one. With
    // exceptions off this is nothing at all, and valid() carries the news
    // instead -- which is why valid() is the primary channel and this is the
    // addition.
    void d_internal_check(void) const
    {
#if (D_INTERNAL_PATH_THROW == 1)
        if (!m_str)
        {
            throw std::bad_alloc();
        }
#endif
        return;
    }

    // d_internal_invalid
    //   function: the invalid path, for the failure paths below.
    static path d_internal_invalid(void)
    {
        return path(d_internal_invalid_tag());
    }

    // d_internal_invalidate
    //   function: mark this path failed, releasing what it held.
    void d_internal_invalidate(void)
    {
        if (m_str)
        {
            d_string_free(m_str);
            m_str = 0;
        }

        return;
    }

    // fn_lex
    //   type: a c/fs lexical operation with the (path, buf, bufsize) shape.
    typedef char* (*fn_lex)(const char* _path, char* _buf, size_t _bufsize);

    static char* d_internal_dirname(const char* p, char* b, size_t n)
        { return d_dirname(p, b, n); }
    static char* d_internal_basename(const char* p, char* b, size_t n)
        { return d_basename(p, b, n); }
    static char* d_internal_stem(const char* p, char* b, size_t n)
        { return d_path_stem(p, b, n); }
    static char* d_internal_norm(const char* p, char* b, size_t n)
        { return d_path_normalize(p, b, n); }

    // d_internal_lex
    //   function: run one lexical operation into scratch and build a path
    // from the result.
    //   The four decompositions differ only by which C function they call, so
    // they share this rather than repeating the scratch-grow-check dance four
    // times -- which is exactly the kind of repetition that lets one of them
    // drift.
    path d_internal_lex(fn_lex _fn) const
    {
        internal::path_scratch scratch;

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

        if (!_fn(d_string_cstr(m_str), scratch.data(), scratch.size()))
        {
            return d_internal_invalid();
        }

        return path(scratch.data());
    }
};


// III.  Free functions

// operator/
//   function: join two paths, yielding a third.
inline path operator/(const path& _lhs, const path& _rhs)
{
    path result(_lhs);

    result.append(_rhs);

    return result;
}

// swap
//   function: ADL-findable swap, so generic code picks this up rather than
// falling back to std::swap's move-move-move.
inline void swap(path& _a, path& _b) D_NOEXCEPT
{
    _a.swap(_b);

    return;
}

NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_PATH_
