/******************************************************************************
* djinterp [parsegen]                                              parsegen.h
*
* The umbrella for the parser-GENERATOR subsystem.
*   Owns the subsystem keyword and the generator's diagnostic domains, and
* hands every parsegen header the framework contract. Declares no type of its
* own beyond that.
*
*   SUBSYSTEM MAP. parsegen is a SIBLING of parse, not a child of it, and it
* is FLAT: families, frontends, passes, and backends are directories on disk
* and prefixes in a name, never nested scopes.
*
*     parse     (d_parse_*)     everything usable without generating a parser
*                               -- the carrier, the execution substrate
*                               (machine, op_set), the diagnostic channel
*     parsegen  (d_parsegen_*)  everything that PRODUCES a parser -- grammars,
*                               analysis, passes, families, backends
*
*   The split is a dependency direction, not a filing convention. parsegen
* depends on parse because a generator's output is a parser; parse must not
* depend on parsegen, or a program that only runs a hand-authored program
* would drag the whole generator in. The test for a new type is the one
* question: is this usable without generating anything? A machine running a
* program someone wrote by hand is; a pass that rewrites a grammar is not.
*
*   The C++ face of this header is parsegen.hpp, which adds NS_PARSEGEN and
* nothing else -- a namespace macro is the only part of an umbrella that
* cannot be written in C.
*
*   Requires:  c/djinterp.h (qualifier kit, established here for every header
*              below) and parse/diagnostic.h (the domain base this subsystem
*              numbers from).
*
* path:      /inc/djinterp/parsegen/parsegen.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.02.13
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KEYWORDS
    --------
    1.  Subsystem keyword
         1.  D_KEYWORD_PARSEGEN
2.  DIAGNOSTIC DOMAINS
    ------------------
    1.  Stage domains
         1.  d_parsegen_diag_domain
*/

#ifndef DJINTERP_PARSEGEN_
#define DJINTERP_PARSEGEN_ 1

// djinterp
#include "../c/djinterp.h"                    // framework root
#include "../config/parsegen/cfg_parsegen.h"   // D_INTERNAL_PARSEGEN_* knobs
#include "../parse/diagnostic.h"               // D_PARSE_DIAG_DOMAIN_PARSEGEN,
                                               // the base this subsystem
                                               // numbers its domains from


//==============================================================================
// 1.  KEYWORDS
//==============================================================================


// 1.1    Subsystem keyword
//------------------------------------------------------------------------------
// 1.1.1
// D_KEYWORD_PARSEGEN
//   keyword: resolves to `parsegen`. Marks a unit of code as part of the
// parser-generator subsystem, and is what NS_PARSEGEN is built from in the
// C++ face.
#ifndef D_KEYWORD_PARSEGEN
#   define D_KEYWORD_PARSEGEN       parsegen
#endif  // D_KEYWORD_PARSEGEN


//==============================================================================
// 2.  DIAGNOSTIC DOMAINS
//==============================================================================
// Every stage that can report something claims a domain and numbers its own
// codes from zero within it -- the same partition an op_set gives an opcode
// space, for the same reason: two emitters can then be added independently and
// still never collide. The values are offsets from the base parse reserves for
// this subsystem, so parse may add core domains without renumbering anything
// here.


// 2.1    Stage domains
//------------------------------------------------------------------------------
// 2.1.1
// d_parsegen_diag_domain
//   enum: the domains the generator's stages emit under. A stage added later
// takes the next value; nothing below is renumbered, because a domain and code
// pair is the stable identity of a diagnostic and tests pin them.
enum d_parsegen_diag_domain
{
    D_PARSEGEN_DIAG_DOMAIN_FRONTEND = D_PARSE_DIAG_DOMAIN_PARSEGEN + 0,
    D_PARSEGEN_DIAG_DOMAIN_GRAMMAR  = D_PARSE_DIAG_DOMAIN_PARSEGEN + 1,
    D_PARSEGEN_DIAG_DOMAIN_ANALYSIS = D_PARSE_DIAG_DOMAIN_PARSEGEN + 2,
    D_PARSEGEN_DIAG_DOMAIN_PASS     = D_PARSE_DIAG_DOMAIN_PARSEGEN + 3,
    D_PARSEGEN_DIAG_DOMAIN_FAMILY   = D_PARSE_DIAG_DOMAIN_PARSEGEN + 4,
    D_PARSEGEN_DIAG_DOMAIN_BACKEND  = D_PARSE_DIAG_DOMAIN_PARSEGEN + 5
};


#endif  // DJINTERP_PARSEGEN_
