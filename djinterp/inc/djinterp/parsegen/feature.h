/******************************************************************************
* djinterp [parsegen]                                               feature.h
*
* What a grammar uses, and what a stage can cope with, as one 64-bit mask.
*   This is the vocabulary that keeps four notations times N parser families
* from being an N-by-M problem. A frontend reports what the grammar it read
* actually USES. A family declares what it SUPPORTS. A pass declares what it
* consumes and what it introduces. Selection is then set arithmetic on one
* word, and a stage added later is a row of declarations rather than an edit
* to whatever chose between the existing ones.
*
*   THE ONE THAT MATTERS MOST. D_PARSEGEN_ORDERED_CHOICE and
* D_PARSEGEN_UNORDERED_CHOICE are separate bits and neither is a default.
* PEG's `|` commits to the first alternative that matches; BNF's does not.
* Collapsing them at the grammar level would silently change which language a
* grammar denotes, and the damage would show up as a wrong parse rather than
* as an error. A pass may lower unordered to ordered, but only where the
* alternatives' first sets are disjoint -- which is a proof obligation, and
* exactly the kind of thing a feature bit makes it possible to state.
*
*   A BIT IS VOCABULARY, NOT A PROMISE. Naming a feature here says the concept
* has a stable name, not that anything implements it. Whether a given pipeline
* can handle a grammar is answered by querying the registry, never by reading
* this list.
*
*   Requires: parsegen/parsegen.h (subsystem umbrella).
*
* path:      /inc/djinterp/parsegen/feature.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE VOCABULARY
    --------------
    1.  The mask
         1.  d_parsegen_features
    2.  Grammar shape
         1.  Choice and repetition
         2.  Recursion and predicates
    3.  Grammar content
         1.  Terminals
         2.  Semantics
    4.  Input and outcome
         1.  Subject
         2.  Result
    5.  Reserved range
         1.  D_PARSEGEN_FEATURE_USER
2.  OPERATIONS
    ----------
    1.  Set arithmetic
    2.  Naming
*/

#ifndef DJINTERP_PARSEGEN_FEATURE_
#define DJINTERP_PARSEGEN_FEATURE_ 1

// std
#include <stddef.h>         // size_t
#include <stdint.h>         // uint64_t
// djinterp
#include "./parsegen.h"     // subsystem umbrella


//==============================================================================
// 1.  THE VOCABULARY
//==============================================================================


// 1.1    The mask
//------------------------------------------------------------------------------
// 1.1.1
// d_parsegen_features
//   type: a set of capability bits. One word, so every question selection asks
// -- does this stage reject anything this grammar uses, does it get everything
// it needs -- is two instructions.
typedef uint64_t d_parsegen_features;

// D_PARSEGEN_FEATURE_NONE
//   constant: the empty set.
#define D_PARSEGEN_FEATURE_NONE         ((d_parsegen_features)0)


// 1.2    Grammar shape
//------------------------------------------------------------------------------
// 1.2.1
// Choice and repetition
//   constant: how alternatives and repetition are written and what they mean.
#define D_PARSEGEN_ORDERED_CHOICE       ((d_parsegen_features)1 << 0)
#define D_PARSEGEN_UNORDERED_CHOICE     ((d_parsegen_features)1 << 1)
#define D_PARSEGEN_BOUNDED_REPEAT       ((d_parsegen_features)1 << 2)
#define D_PARSEGEN_EMPTY_PRODUCTION     ((d_parsegen_features)1 << 3)

// 1.2.2
// Recursion and predicates
//   constant: the structural properties that decide which families can run a
// grammar at all.
#define D_PARSEGEN_LEFT_RECURSION       ((d_parsegen_features)1 << 4)
#define D_PARSEGEN_SYNTACTIC_PREDICATE  ((d_parsegen_features)1 << 5)
#define D_PARSEGEN_PRECEDENCE           ((d_parsegen_features)1 << 6)
#define D_PARSEGEN_RULE_PARAMETERS      ((d_parsegen_features)1 << 7)


// 1.3    Grammar content
//------------------------------------------------------------------------------
// 1.3.1
// Terminals
//   constant: what a grammar may write where a single symbol is expected.
#define D_PARSEGEN_CHARACTER_CLASS      ((d_parsegen_features)1 << 8)
#define D_PARSEGEN_LITERAL_STRING       ((d_parsegen_features)1 << 9)
#define D_PARSEGEN_CASE_INSENSITIVE     ((d_parsegen_features)1 << 10)
#define D_PARSEGEN_UNICODE              ((d_parsegen_features)1 << 11)

// 1.3.2
// Semantics
//   constant: what a grammar attaches to a match beyond the match itself.
#define D_PARSEGEN_CAPTURE              ((d_parsegen_features)1 << 12)
#define D_PARSEGEN_HOST_ACTION          ((d_parsegen_features)1 << 13)
#define D_PARSEGEN_ERROR_RECOVERY       ((d_parsegen_features)1 << 14)


// 1.4    Input and outcome
//------------------------------------------------------------------------------
// 1.4.1
// Subject
//   constant: what the grammar is written over, and how far ahead a decision
// may need to look.
#define D_PARSEGEN_TOKEN_STREAM         ((d_parsegen_features)1 << 16)
#define D_PARSEGEN_INDENTATION          ((d_parsegen_features)1 << 17)
#define D_PARSEGEN_LOOKAHEAD_K          ((d_parsegen_features)1 << 18)

// 1.4.2
// Result
//   constant: what the parser is expected to produce.
#define D_PARSEGEN_AMBIGUITY            ((d_parsegen_features)1 << 20)
#define D_PARSEGEN_MEMOIZATION          ((d_parsegen_features)1 << 21)
#define D_PARSEGEN_INCREMENTAL          ((d_parsegen_features)1 << 22)


// 1.5    Reserved range
//------------------------------------------------------------------------------
// 1.5.1
// D_PARSEGEN_FEATURE_USER
//   constant: the first bit djinterp will never claim. An application or an
// out-of-tree stage numbers its own capabilities upward from here, so adding
// one never collides with a framework bit added later.
#define D_PARSEGEN_FEATURE_USER         ((d_parsegen_features)1 << 48)

// D_PARSEGEN_FEATURE_COUNT
//   constant: how many bits this level names, which bounds a rendering loop.
#define D_PARSEGEN_FEATURE_COUNT        64u


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Set arithmetic
//------------------------------------------------------------------------------
/*
d_parsegen_features_has
  Whether a set contains every bit of another.

Parameter(s):
  _set:      the set to query.
  _required: the bits to look for; the empty set is always present.
Return:
  A boolean value corresponding to either:
  - 1, if every required bit is set, or
  - 0, otherwise.
*/
D_INLINE int
d_parsegen_features_has(
    d_parsegen_features _set,
    d_parsegen_features _required
)
{
    return ((_set & _required) == _required) ? 1 : 0;
}

/*
d_parsegen_features_any
  Whether a set shares any bit with another.
NOTE:
  This is the rejection test: a stage cannot run a grammar when the grammar
uses anything the stage rejects.

Parameter(s):
  _set:   the set to query.
  _probe: the bits to look for.
Return:
  A boolean value corresponding to either:
  - 1, if any probed bit is set, or
  - 0, otherwise.
*/
D_INLINE int
d_parsegen_features_any(
    d_parsegen_features _set,
    d_parsegen_features _probe
)
{
    return ((_set & _probe) != 0u) ? 1 : 0;
}

/*
d_parsegen_features_missing
  The bits a set requires that another does not supply.
NOTE:
  Not merely a predicate: this is what a diagnostic prints when a stage cannot
be used, so the reader is told which capability is absent rather than that
something, somewhere, did not line up.

Parameter(s):
  _available: the bits that are present.
  _required:  the bits that are needed.
Return:
  The required bits that are absent; the empty set when none are.
*/
D_INLINE d_parsegen_features
d_parsegen_features_missing(
    d_parsegen_features _available,
    d_parsegen_features _required
)
{
    return _required & ~_available;
}

// 2.2    Naming
//------------------------------------------------------------------------------
//   Every diagnostic about capability has to name one, so the names live here
// beside the bits rather than in whatever reported the problem.
const char*     d_parsegen_feature_name(unsigned _bit);
size_t          d_parsegen_features_render(d_parsegen_features _set,
                                           char*               _out,
                                           size_t              _size);

D_EXTERN_C_END


#endif  // DJINTERP_PARSEGEN_FEATURE_
