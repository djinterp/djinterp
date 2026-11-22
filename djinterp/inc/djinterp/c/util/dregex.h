/*******************************************************************************
* djinterp [c][utility]                                                 dregex.h
*
*   POSIX extended regular expressions with leftmost-longest semantics.
*     Patterns are parsed to a tree, compiled by Thompson construction to a
* flat instruction program, and executed by parallel NFA simulation.  There is
* no backtracking, so match time is bounded by O(n*m) in the length of the
* subject and the size of the program.
*     POSIX awk requires only the extent of the overall match: ERE has no
* backreferences, and sub() and gsub() expand `&` to the whole match rather
* than to a numbered group.  The simulator therefore records no submatches.
*     gawk's gensub() does refer to numbered groups.  Those are served by a
* second execution path: the simulation fixes the overall extent, and a bounded
* backtracking pass then chooses among the parses of that one window.  The fast
* path is unchanged and pays nothing for the feature.
*     The file is named dregex.h rather than regex.h because a translation
* unit that includes both this header and the POSIX <regex.h> -- the
* differential test does -- would otherwise resolve the angle-bracket form to
* this file whenever the include directory is on the search path.
*   Every function is C, with C linkage from C++. A compiled pattern carries
* its own scratch buffers, so one pattern serves one thread at a time; two
* threads compile a pattern each.
*
* HOW TO USE
* ----------
*     enum dj_regex_status  status;
*     struct dj_regex*      re = d_regex_compile("a(b+)c", &status);
*     struct dj_regex_match groups[2];
*
*     if ( (re) &&
*          (d_regex_search_groups(re, text, length, 0, groups, 2u) ==
*           D_REGEX_OK) &&
*          (groups[0].start != D_REGEX_NO_MATCH) )
*     {
*         // groups[1] is the run of b's
*     }
*
*     d_regex_free(re);
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header. Its limits are the constants
* of section 1.1.
*
*
* path:      /inc/djinterp/c/util/dregex.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.11.19
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  D_REGEX_DUP_MAX
         2.  D_REGEX_GROUP_MAX
         3.  D_REGEX_NO_MATCH
         4.  D_REGEX_BACKTRACK_MAX
    2.  Types
         1.  dj_regex
         2.  dj_regex_status
         3.  dj_regex_match
2.  OPERATIONS
    ----------
    1.  Compilation
    2.  Matching
    3.  Capture groups
    4.  Diagnostics
*/

#ifndef DJINTERP_C_UTIL_DREGEX_H
#define DJINTERP_C_UTIL_DREGEX_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../djinterp.h"  // framework root


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================
// The compiled-pattern handle, the status enumeration returned by compilation,
// and the structure describing a located match.


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_REGEX_DUP_MAX
//   constant: largest repetition count accepted in a `{n,m}` interval.  Equal
// to the POSIX RE_DUP_MAX minimum.  Intervals are expanded at compile time, so
// this bounds program growth.
#define D_REGEX_DUP_MAX 255

// 1.1.2
// D_REGEX_GROUP_MAX
//   constant: largest number of capturing groups accepted in one pattern.
// gensub() refers to groups 1 through 9; the remainder are available to a
// host layer.  Group 0 denotes the whole match and is not counted here.
#define D_REGEX_GROUP_MAX 32

// 1.1.3
// D_REGEX_NO_MATCH
//   constant: start offset reported for a group that did not participate in
// the match.  gensub() must distinguish a group that matched the empty string
// from one that never matched at all, so an unset group cannot be reported as
// a zero-length extent at offset zero.
#define D_REGEX_NO_MATCH ((size_t)-1)

// 1.1.4
// D_REGEX_BACKTRACK_MAX
//   constant: step budget for one capturing pass.  The pass enumerates parses
// of an already-fixed window, so the budget bounds a search that is otherwise
// exponential on pathological patterns; exhausting it yields the best parse
// found so far, or D_REGEX_ERR_BUDGET when none was completed.
#define D_REGEX_BACKTRACK_MAX 1000000

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// dj_regex
//   struct: a compiled pattern.  Carries the execution scratch buffers, so a
//   single instance must not be used from two threads at once.
struct dj_regex;

// 1.2.2
// dj_regex_status
//   enum: outcome of compilation.
enum dj_regex_status
{
    D_REGEX_OK = 0,
    D_REGEX_ERR_SYNTAX,
    D_REGEX_ERR_RANGE,
    D_REGEX_ERR_MEMORY,
    D_REGEX_ERR_BUDGET
};

// 1.2.3
// dj_regex_match
//   struct: the extent of a located match, as a byte offset and byte count
//   into the subject.  A zero length denotes a valid empty match.
struct dj_regex_match
{
    size_t start;   // offset of the first byte of the match
    size_t length;  // number of bytes matched; may be zero
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Compilation is separate from matching so that a pattern used in a loop is
// translated once.  Every matching entry point takes an explicit length, so
// subjects may contain embedded NUL bytes.


// 2.1    Compilation
//------------------------------------------------------------------------------
/**
 * @brief Parses and compiles a POSIX extended regular expression.
 *
 * @param[in]  _pattern     the pattern, NUL-terminated.
 * @param[out] _out_status  receives the outcome; may be NULL.
 * @return the compiled pattern, which d_regex_free releases; NULL with
 *         D_REGEX_ERR_SYNTAX for a NULL or malformed pattern,
 *         D_REGEX_ERR_RANGE for an interval over D_REGEX_DUP_MAX or more
 *         than D_REGEX_GROUP_MAX groups, and D_REGEX_ERR_MEMORY when memory
 *         runs out.
 */
struct dj_regex* d_regex_compile(
    const char*           _pattern,
    enum dj_regex_status* _out_status
);
/**
 * @brief Releases a compiled pattern and its scratch buffers.
 *
 * @param[in] _regex  the pattern; NULL does nothing.
 */
void d_regex_free(
    struct dj_regex* _regex
);

// 2.2    Matching
//------------------------------------------------------------------------------
/**
 * @brief Finds the leftmost-longest match that starts at or after `_from`.
 *
 * @param[in]  _regex      the compiled pattern; its scratch buffers are used.
 * @param[in]  _text       the subject; may be NULL when `_length` is 0.
 * @param[in]  _length     the subject's length in bytes; NULs are bytes.
 * @param[in]  _from       where the search starts; at most `_length`.
 * @param[out] _out_match  receives the match's extent; may be NULL; left
 *                         alone when there is none.
 * @return true when a match was found; false when none was, and for a NULL
 *         pattern, a NULL subject with a length, or `_from` past the end.
 */
bool d_regex_search(
    struct dj_regex*       _regex,
    const char*            _text,
    size_t                 _length,
    size_t                 _from,
    struct dj_regex_match* _out_match
);
/**
 * @brief Finds the longest match that starts exactly at `_at`.
 *
 * @param[in]  _regex       the compiled pattern; its scratch buffers are used.
 * @param[in]  _text        the subject; may be NULL when `_length` is 0.
 * @param[in]  _length      the subject's length in bytes.
 * @param[in]  _at          where the match must start; at most `_length`.
 * @param[out] _out_length  receives the match's length; may be NULL; left
 *                          alone when there is none.
 * @return true when a match starts there; false otherwise, and for the
 *         arguments d_regex_search refuses.
 */
bool d_regex_match_at(
    struct dj_regex* _regex,
    const char*      _text,
    size_t           _length,
    size_t           _at,
    size_t*          _out_length
);
/**
 * @brief Whether the pattern matches anywhere in the subject: d_regex_search
 *        from 0, without the extent.
 *
 * @param[in] _regex   the compiled pattern.
 * @param[in] _text    the subject; may be NULL when `_length` is 0.
 * @param[in] _length  the subject's length in bytes.
 * @return true when it matches.
 */
bool d_regex_test(
    struct dj_regex* _regex,
    const char*      _text,
    size_t           _length
);

// 2.3    Capture groups
//------------------------------------------------------------------------------
// d_regex_search_groups runs the simulation to fix the overall extent, then a
// bounded backtracking pass over that window to assign the groups.  Index zero
// of `_out_groups` is the whole match; a group that did not participate reports
// a start of D_REGEX_NO_MATCH.  D_REGEX_OK with a group-zero start of
// D_REGEX_NO_MATCH means the pattern did not match at all, which is distinct
// from the error statuses.
/**
 * @brief The pattern's capturing groups, not counting group 0, the whole
 *        match.
 *
 * @param[in] _regex  the compiled pattern.
 * @return the count; 0 for NULL.
 */
size_t d_regex_group_count(
    const struct dj_regex* _regex
);
/**
 * @brief Finds the leftmost-longest match from `_from`, and each group's
 *        extent within it, as GNU regex assigns them (dregex.c 6).
 *
 * @param[in]  _regex       the compiled pattern; its scratch buffers are used.
 * @param[in]  _text        the subject; may be NULL when `_length` is 0.
 * @param[in]  _length      the subject's length in bytes.
 * @param[in]  _from        where the search starts; at most `_length`.
 * @param[out] _out_groups  receives `_count` extents, index 0 the whole
 *                          match; every entry not reported, and every group
 *                          that did not take part, has a start of
 *                          D_REGEX_NO_MATCH.
 * @param[in]  _count       the entries of `_out_groups`; at least 1. Groups
 *                          past it are not reported.
 * @return D_REGEX_OK, a group-0 start of D_REGEX_NO_MATCH meaning no match;
 *         D_REGEX_ERR_SYNTAX for a NULL pattern or groups, a `_count` of 0,
 *         a NULL subject with a length, or `_from` past the end;
 *         D_REGEX_ERR_MEMORY when the pass's buffers cannot be had; and
 *         D_REGEX_ERR_BUDGET when the pass completes no parse within
 *         D_REGEX_BACKTRACK_MAX steps, though the match exists.
 */
enum dj_regex_status d_regex_search_groups(
    struct dj_regex*       _regex,
    const char*            _text,
    size_t                 _length,
    size_t                 _from,
    struct dj_regex_match* _out_groups,
    size_t                 _count
);

// 2.4    Diagnostics
//------------------------------------------------------------------------------
/**
 * @brief A status in words.
 *
 * @param[in] _status  the status.
 * @return a description the library owns; never NULL.
 */
const char* d_regex_status_text(
    enum dj_regex_status _status
);
/**
 * @brief The compiled program's instruction count, a measure of what a
 *        pattern costs to run.
 *
 * @param[in] _regex  the compiled pattern.
 * @return the count; 0 for NULL.
 */
size_t d_regex_program_size(
    const struct dj_regex* _regex
);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_DREGEX_H
