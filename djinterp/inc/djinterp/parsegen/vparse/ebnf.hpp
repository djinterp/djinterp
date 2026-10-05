/*******************************************************************************
* djinterp [parsegen]                                                   ebnf.hpp
*
*   An EBNF frontend: grammar text to a syntax tree, and the tree to a PEG
* program.
*   It reads what the metacircular notation (notation.hpp) cannot: grouping
* with parentheses, `?`, multi-character string literals in single or double
* quotes, `.` for any symbol, and `#` comments. Alternation `|` is ORDERED
* choice here, compiled as a cascade of attempts, which is what the notation
* means by `|` as well.
*   The tree is kept as its own stage rather than compiled while parsing,
* because it is the form a second backend would read: the prototype noted that
* a different backend would target the LR table instead, and the neutral
* grammar in parsegen/grammar.h is where this tree is headed next.
*   Neither function throws. Failures are reported through `_ok` and `_error`,
* as notation::parse_grammar reports them.
*   Consolidated from the standalone prototype's grammar module; renamed ebnf so
* it cannot be confused with parsegen::grammar, the neutral grammar model.
*
*
* path:      /inc/djinterp/parsegen/vparse/ebnf.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_EBNF_HPP
#define DJINTERP_PARSEGEN_VPARSE_EBNF_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <memory>           // std::shared_ptr
#include <string>           // std::string
#include <utility>          // std::pair
#include <vector>           // std::vector
// djinterp
#include "./peg.hpp"        // peg::program, the compile target


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace ebnf
{

// node kinds
//   enum: what a syntax-tree node is. N_NOT has no surface syntax -- the parser
// never produces it -- and exists for the end-of-input gate compile() adds.
enum
{
    N_LIT = 0,
    N_ANY,
    N_SET,
    N_REF,
    N_SEQ,
    N_CHOICE,
    N_OPT,
    N_STAR,
    N_PLUS,
    N_NOT
};

struct node;

// node_ptr
//   type: shared ownership of a subtree.
using node_ptr = std::shared_ptr<node>;

// node
//   struct: one syntax-tree node. `text` holds a literal's characters, a
// class's explicit membership, or a referenced rule's name.
struct node
{
    int                   kind;
    std::string           text;
    std::vector<node_ptr> kids;
};

// rules
//   type: a parsed grammar's rules, in declaration order.
using rules = std::vector<std::pair<std::string, node_ptr>>;

std::pair<rules, std::string> parse(const std::string& _source,
                                    bool*              _ok    = nullptr,
                                    std::string*       _error = nullptr);
peg::program                  compile(const rules&       _rules,
                                      const std::string& _start,
                                      bool*              _ok    = nullptr,
                                      std::string*       _error = nullptr);

}  // namespace ebnf

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSEGEN_VPARSE_EBNF_HPP
