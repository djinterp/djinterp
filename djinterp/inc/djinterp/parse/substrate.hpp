/*******************************************************************************
* djinterp [parse]                                                 substrate.hpp
*
*   The C++ face of the parse execution substrate: the one place the
* substrate's C++ headers get NS_PARSE from.
*   parse.hpp is NS_PARSE's canonical home, but it also carries the parser
* carrier and the functional layer, and the substrate must build without them:
* a program that only runs a hand-authored instruction stream should not pay for
* parser combinators. So the substrate reads NS_PARSE from here instead. The
* definitions are guarded and token-identical to parse.hpp's, which is what lets
* the two headers be included in either order -- an identical redefinition is
* well-formed, and a divergent one would not be, so keep them in step.
*   This mirrors parsegen.hpp: an umbrella's C++ face holds the namespace macro
* and nothing else.
*
*
* path:      /inc/djinterp/parse/substrate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_PARSE_SUBSTRATE_HPP
#define DJINTERP_PARSE_SUBSTRATE_HPP 1

// djinterp
#include "../djinterp.hpp"  // framework root


// D_KEYWORD_PARSE
//   keyword: resolves to `parse`. Guarded rather than owned -- parse.hpp is its
// canonical home, and this spelling must stay token-identical to it.
#ifndef D_KEYWORD_PARSE
    #define D_KEYWORD_PARSE             parse
#endif  // D_KEYWORD_PARSE

// NS_PARSE
//   namespace: the parse subsystem namespace. Guarded for the same reason.
#ifndef NS_PARSE
    #define NS_PARSE                    D_NAMESPACE(D_KEYWORD_PARSE)
#endif  // NS_PARSE


#endif  // DJINTERP_PARSE_SUBSTRATE_HPP
