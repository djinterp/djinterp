/******************************************************************************
* djinterp [core]                                          parse_context.hpp
*
* Parse session context:
*   This header defines the shared output sinks that all parsers within
* a single parse session write into.  Bundling them into one struct
* avoids passing three or more references through every constructor and
* callback.
*
* Contents:
*   - parse_context     arena + cross_ref + string_table bundle
*
*
* path:      /inc/cpp/parse/parse_context.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.18
******************************************************************************/

#ifndef DJINTERP_PARSE_CONTEXT_
#define DJINTERP_PARSE_CONTEXT_ 1

#include "../core/djinterp.hpp"
#include "../arena/arena.hpp"
#include "../arena/cross_ref.hpp"
#include "../arena/arena_domain.hpp"
#include "../arena/string_table.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  parse_context
// ================================================================

// parse_context
//   struct: bundles the output sinks shared by all parsers
// operating within the same parse session.
struct parse_context
{
    arena::symbol_tree&     tree;
    arena::cross_ref&       xref;
    arena::string_table&    strings;

    parse_context
    (
        arena::symbol_tree&     _tree,
        arena::cross_ref&       _xref,
        arena::string_table&    _strings
    )
        : tree    (_tree),
          xref    (_xref),
          strings (_strings)
    {
    }
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_CONTEXT_
