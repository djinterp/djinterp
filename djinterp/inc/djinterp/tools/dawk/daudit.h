/*******************************************************************************
* djinterp [djinterp]                                                   daudit.h
*
* Repair audit:
*   Every repair states what it claims to change, and this file checks the
* claim against the line before and the line after, independently of the code
* that produced it.  A repair whose result does not match its claim is not
* written.  The check runs before the write, not after, so a wrong repair
* never reaches the file.
*   It exists because convergence is not correctness.  In the session that
* built this tool, a clean check and an idempotent fix both endorsed
* corrupted files; only an audit of what each changed line was allowed to
* become caught them.  That audit was a script run by hand afterwards.  This
* is the same audit as a gate the fixer cannot pass without.
*
*
* path:      /inc/djinterp/tools/dawk/daudit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DAUDIT_H
#define DJINTERP_TOOLS_DAWK_DAUDIT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// d_audit_kind
//   enum: what a repair is allowed to change.
enum d_audit_kind
{
    D_AUDIT_GEOMETRY = 0,  // whitespace only; every non-blank run identical
    D_AUDIT_FILL,          // a rule's fill run resized; nothing else changes
    D_AUDIT_SUBSTITUTE,    // one run replaced by a stated value
    D_AUDIT_SWAP,          // two runs exchanged
    D_AUDIT_INSERT         // a stated text placed at a point
};

// 1.2    Records
//------------------------------------------------------------------------------
// 1.2.1
// d_audit_claim
//   struct: a repair's statement of what it changed.  `expect` is the text
// the repair believed sat at `head` for `run` bytes; checking it against the
// line on disk is what catches a node whose geometry has gone stale.
struct d_audit_claim
{
    enum d_audit_kind  kind;
    size_t             head;      // 0-based start of the run
    size_t             run;       // its length on the line before
    const char*        expect;    // what the run should read; NULL to skip
    const char*        value;     // SUBSTITUTE / INSERT: the new text
    size_t             head2;     // SWAP: the second run
    size_t             run2;
    const char*        expect2;
    bool               replaces;  // INSERT: discards a single-word block comment
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Verification
//------------------------------------------------------------------------------
bool d_audit_verify(const char*                 _before,
                    const char*                 _after,
                    const struct d_audit_claim* _claim,
                    const char**                _out_reason);


#endif  // DJINTERP_TOOLS_DAWK_DAUDIT_H
