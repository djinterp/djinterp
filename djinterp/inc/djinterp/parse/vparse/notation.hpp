/******************************************************************************
* djinterp [vparse]                                               notation.hpp
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
* path:      /inc/djinterp/parse/vparse/notation.hpp
* author(s): vparse                                        created: 2026.06.19
******************************************************************************/

#ifndef DJINTERP_PARSE_VPARSE_NOTATION_
#define DJINTERP_PARSE_VPARSE_NOTATION_ 1

// std
#include <string>
#include <vector>
// djinterp
#include "./peg.hpp"
#include "./ruleset.hpp"


NS_DJINTERP
NS_PARSE

// notation
//   namespace: the grammar language and its front-end.

// tags
//   enum: the capture tags the grammar-of-grammars emits.
enum { NAME = 1, SETBODY, REF, STAR, PLUS, LITCH, BAR };

ruleset meta_grammar();
ruleset read_ruleset(const std::vector<capture>& _caps,
                     const std::string&               _text);
ruleset parse_grammar(const std::string& _text, bool* _ok = nullptr);


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_VPARSE_NOTATION_