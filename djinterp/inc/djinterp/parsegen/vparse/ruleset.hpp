/*******************************************************************************
* djinterp [parsegen]                                                ruleset.hpp
*
*   The runtime grammar value -- the mutable twin of grammar<N, Σ, P, S>.  An
* ordered list of named rules; each rule a choice of alternatives; each
* alternative a sequence of terms (literal, class, or rule reference), any term
* optionally starred and/or capture-tagged.  The first rule is the start
* symbol.  A value, so it can be built or assembled from parsed grammar text.
*
*
* path:      /inc/djinterp/parsegen/vparse/ruleset.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_RULESET_HPP
#define DJINTERP_PARSEGEN_VPARSE_RULESET_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>
#include <vector>
// djinterp
#include "./machine.hpp"

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// term_kinds
//   enum: which recognizer a term denotes.
enum { T_CHAR = 0, T_SET, T_REF, T_ANY };

// term
//   struct: one grammar term, with optional star and capture tag (0 = none).
struct term
{
    int         kind = T_CHAR;
    char        ch   = 0;
    std::string set;
    std::string ref;
    bool        star = false;
    int         cap  = 0;
};

// rule
//   struct: a named production -- a choice of one or more alternatives, each a
// sequence of terms.
struct rule
{
    std::string                    name;
    std::vector<std::vector<term>> alts;
};

// ruleset
//   struct: the runtime grammar; first rule is the start symbol.
struct ruleset
{
    std::vector<rule> rules;
};

// lit
//   function: a literal-character term, optionally captured.
inline term lit(char _c, int _cap = 0)
{
    term t; t.kind = T_CHAR; t.ch = _c; t.cap = _cap; return t;
}

// cls
//   function: a character-class term, optionally starred and/or captured.
inline term cls(const std::string& _set, bool _star = false, int _cap = 0)
{
    term t; t.kind = T_SET; t.set = _set; t.star = _star; t.cap = _cap; return t;
}

// ref
//   function: a rule-reference term, optionally starred and/or captured.
inline term ref(const std::string& _name, bool _star = false, int _cap = 0)
{
    term t; t.kind = T_REF; t.ref = _name; t.star = _star; t.cap = _cap; return t;
}

// any_ch
//   function: an "any one character" term, optionally captured.
inline term any_ch(int _cap = 0)
{
    term t; t.kind = T_ANY; t.cap = _cap; return t;
}

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_RULESET_HPP
