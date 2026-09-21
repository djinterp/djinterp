/*******************************************************************************
* djinterp [parsegen]                                              parsegen.hpp
*
*   The C++ face of the parsegen umbrella.
*   Everything the subsystem needs is in parsegen.h; this adds the one part of
* an umbrella that cannot be written in C -- the namespace macro -- and
* nothing else. A parsegen header written in C includes parsegen.h directly;
* one written in C++ includes this and opens its scope with NS_PARSEGEN.
*
* path:      /inc/djinterp/parsegen/parsegen.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.02.13
*                                                          revised: 2026.09.19
******************************************************************************/

#ifndef DJINTERP_PARSEGEN_HPP_
#define DJINTERP_PARSEGEN_HPP_ 1

// djinterp
#include "../djinterp.hpp"      // framework root
#include "./parsegen.h"         // the subsystem umbrella this layer faces


// NS_PARSEGEN
//   namespace: the parser-generator namespace, a sibling of `parse` directly
// under `djinterp`.  Everything the subsystem declares lands here flat, so a
// family's opcodes and a frontend's reader are `parsegen::peg_char` and
// `parsegen::read_ebnf` rather than living in nested scopes that would have to
// be opened, closed, and qualified at every boundary.
#ifndef NS_PARSEGEN
    #define NS_PARSEGEN                 D_NAMESPACE(D_KEYWORD_PARSEGEN)
#endif


#endif  // DJINTERP_PARSEGEN_HPP_
