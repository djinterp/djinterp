/*******************************************************************************
* djinterp [parsegen]                                                adapter.hpp
*
*   The join to djinterp::parse.  as_parser wraps a compiled vparse program as a
* first-class parser<std::string, char>: a value invocable as
* parse_state<char>& -> parse_result<std::string>, advancing the state on a
* match (the matched text is the result) and leaving it on failure.  Once a
* vparse parser wears the handle it composes with the combinator world through
* the functional protocols the real parser.hpp specialises on parser<R, E>.
*
*
* path:      /inc/djinterp/parsegen/vparse/adapter.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_ADAPTER_HPP
#define DJINTERP_PARSEGEN_VPARSE_ADAPTER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>
// djinterp
#include "../../parse/parser/parser.hpp"
#include "./peg.hpp"

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

using ::djinterp::parse::parser;

parser<std::string, char> as_parser(const peg::program& _prog);

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_ADAPTER_HPP
