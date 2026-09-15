/******************************************************************************
* djinterp [vparse]                                                adapter.hpp
*
*   The join to djinterp::parse.  as_parser wraps a compiled vparse program as a
* first-class parser<std::string, char>: a value invocable as
* parse_state<char>& -> parse_result<std::string>, advancing the state on a
* match (the matched text is the result) and leaving it on failure.  Once a
* vparse parser wears the handle it composes with the combinator world through
* the functional protocols the real parser.hpp specialises on parser<R, E>.
*
*
* path:      /inc/djinterp/parse/vparse/adapter.hpp
* author(s): vparse                                        created: 2026.06.19
******************************************************************************/

#ifndef DJINTERP_PARSE_VPARSE_ADAPTER_
#define DJINTERP_PARSE_VPARSE_ADAPTER_ 1

// std
#include <string>
// djinterp
#include "../parser/parser.hpp"
#include "./peg.hpp"

NS_DJINTERP
NS_PARSE

parser<std::string, char> as_parser(const program& _prog);

NS_END  // parse
NS_END  // djinterp

#endif  // DJINTERP_PARSE_VPARSE_ADAPTER_
