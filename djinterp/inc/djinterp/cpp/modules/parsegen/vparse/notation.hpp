/*******************************************************************************
* djinterp [cpp/parsegen]                                           notation.hpp
*
*   The grammar language and its metacircular front-end.  meta_grammar() is the
* grammar of grammars -- the notation's own syntax, expressed as a ruleset, so
* the generator compiles the parser that reads grammar text.  The notation
* supports alternatives (|), one-or-more (+), zero-or-more (*), character
* classes ([...] with ranges), rule references, and character literals ('x').
* read_ruleset folds the captured spans into a target ruleset; parse_grammar
* runs the whole text -> ruleset front-end, its reader under read_limit, a
* step limit that grows with the text.
*
*       Grammar := Rule*
*       Rule    := name '=' Alt ('|' Alt)* ';'
*       Alt     := Term*
*       Term    := ( '[' class ']' | 'c' | name ) ('*' | '+')?
*
*
* path:      /inc/djinterp/cpp/modules/parsegen/vparse/notation.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_NOTATION_HPP
#define DJINTERP_PARSEGEN_VPARSE_NOTATION_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
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
ruleset read_ruleset(
    const std::vector<peg::capture>& _caps,
    const std::string&               _text
);
/**
 * @brief The step limit the reader runs under, for a text of a given length.
 *
 * @note Reading takes steps in proportion to the text: 128 a byte is about
 *       twice the most it takes. A caller that runs the reader on a machine
 *       of its own, as a traced reading does, gives it this limit to read as
 *       parse_grammar() reads.
 *
 * @param[in] _length  the text's length, in bytes.
 * @return a machine's default limit and 128 steps for every byte of the
 *         text, or, where that would not fit in a `long`, the most a `long`
 *         holds.
 */
long read_limit(
    std::size_t _length
);
/**
 * @brief Reads grammar text in the notation into a ruleset.
 *
 * @note The reader runs under read_limit() for the text, not under a
 *       machine's default limit: a long grammar is read to its end, not
 *       refused at the limit a match runs under.
 *
 * @param[in]  _text  the grammar text.
 * @param[out] _ok    set to whether the text was read; may be `nullptr`.
 * @return the rules the text defines, in order, or an empty ruleset when the
 *         text is not a grammar in the notation.
 */
ruleset parse_grammar(
    const std::string& _text,
    bool*              _ok = nullptr
);

}  // namespace notation

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_NOTATION_HPP
