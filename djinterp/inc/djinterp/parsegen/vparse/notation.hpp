/*******************************************************************************
* djinterp [parsegen]                                               notation.hpp
*
*   The grammar language and its metacircular front-end.  meta_grammar() is the
* grammar of grammars -- the notation's own syntax, expressed as a ruleset, so
* the generator compiles the parser that reads grammar text.  The notation
* supports alternatives (|), one-or-more (+), zero-or-more (*), character
* classes ([...] with ranges), rule references, and character literals ('x').
* read_ruleset folds the captured spans into a target ruleset; parse_grammar
* runs the whole text -> ruleset front-end.
*
*       Grammar := Rule*
*       Rule    := name '=' Alt ('|' Alt)* ';'
*       Alt     := Term*
*       Term    := ( '[' class ']' | 'c' | name ) ('*' | '+')?
*
*
* path:      /inc/djinterp/parsegen/vparse/notation.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_NOTATION_HPP
#define DJINTERP_PARSEGEN_VPARSE_NOTATION_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>
#include <vector>
// djinterp
#include "./peg.hpp"
#include "./ruleset.hpp"

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// notation
//   namespace: the grammar language and its front-end.
namespace notation {

// tags
//   enum: the capture tags the grammar-of-grammars emits.
enum { NAME = 1, SETBODY, REF, STAR, PLUS, LITCH, BAR };

ruleset meta_grammar();
ruleset read_ruleset(const std::vector<peg::capture>& _caps,
                     const std::string&               _text);
ruleset parse_grammar(const std::string& _text, bool* _ok = nullptr);

}  // namespace notation

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_NOTATION_HPP
