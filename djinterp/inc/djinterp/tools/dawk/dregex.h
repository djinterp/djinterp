/******************************************************************************
* djinterp [dawk]                                                     dregex.h
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
*
*
* path:      /inc/djinterp/tools/dawk/dregex.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

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
         1.  d_regex
         2.  d_regex_status
         3.  d_regex_match
2.  OPERATIONS
    ----------
    1.  Compilation
    2.  Matching
    3.  Capture groups
    4.  Diagnostics
*/

#ifndef DJINTERP_TOOLS_DAWK_DREGEX_H
#define DJINTERP_TOOLS_DAWK_DREGEX_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


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
// d_regex
//   struct: a compiled pattern.  Carries the execution scratch buffers, so a
//   single instance must not be used from two threads at once.
struct d_regex;

// 1.2.2
// d_regex_status
//   enum: outcome of compilation.
enum d_regex_status
{
    D_REGEX_OK = 0,
    D_REGEX_ERR_SYNTAX,
    D_REGEX_ERR_RANGE,
    D_REGEX_ERR_MEMORY,
    D_REGEX_ERR_BUDGET
};

// 1.2.3
// d_regex_match
//   struct: the extent of a located match, as a byte offset and byte count
//   into the subject.  A zero length denotes a valid empty match.
struct d_regex_match
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
struct d_regex*     d_regex_compile(const char*          _pattern,
                                    enum d_regex_status* _out_status);
void                d_regex_free(struct d_regex* _regex);

// 2.2    Matching
//------------------------------------------------------------------------------
bool                d_regex_search(struct d_regex*       _regex,
                                   const char*           _text,
                                   size_t                _length,
                                   size_t                _from,
                                   struct d_regex_match* _out_match);
bool                d_regex_match_at(struct d_regex* _regex,
                                     const char*     _text,
                                     size_t          _length,
                                     size_t          _at,
                                     size_t*         _out_length);
bool                d_regex_test(struct d_regex* _regex,
                                 const char*     _text,
                                 size_t          _length);

// 2.3    Capture groups
//------------------------------------------------------------------------------
// d_regex_search_groups runs the simulation to fix the overall extent, then a
// bounded backtracking pass over that window to assign the groups.  Index zero
// of `_out_groups` is the whole match; a group that did not participate reports
// a start of D_REGEX_NO_MATCH.  D_REGEX_OK with a group-zero start of
// D_REGEX_NO_MATCH means the pattern did not match at all, which is distinct
// from the error statuses.
size_t              d_regex_group_count(const struct d_regex* _regex);
enum d_regex_status d_regex_search_groups(struct d_regex*       _regex,
                                          const char*           _text,
                                          size_t                _length,
                                          size_t                _from,
                                          struct d_regex_match* _out_groups,
                                          size_t                _count);

// 2.4    Diagnostics
//------------------------------------------------------------------------------
const char*         d_regex_status_text(enum d_regex_status _status);
size_t              d_regex_program_size(const struct d_regex* _regex);


#endif  // DJINTERP_TOOLS_DAWK_DREGEX_H
