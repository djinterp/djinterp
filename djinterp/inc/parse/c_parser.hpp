/******************************************************************************
* djinterp [core]                                              c_parser.hpp
*
* C source parser (libclang-backed):
*   This header defines a concrete parser that processes C source code
* through libclang and populates a symbol_tree arena.  It derives from
* parser_base via CRTP and conforms to the standard parser structural
* contract.
*
* Interface:
*   - input_type   = char      (source text)
*   - result_type  = node_id   (root of the parsed symbol subtree)
*   - do_parse(parse_state<char>&) -> parse_result<node_id>
*
*
* path:      /inc/cpp/parse/c_parser.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.18
******************************************************************************/

#ifndef DJINTERP_C_PARSER_
#define DJINTERP_C_PARSER_ 1

#include <cstddef>
#include <cstdint>
#include <vector>
#include <clang-c/Index.h>
#include "../core/djinterp.hpp"
#include "../arena/arena.hpp"
#include "../arena/arena_domain.hpp"
#include "../parse/parse.hpp"
#include "../parse/parser.hpp"
#include "./parse_context.hpp"
#include "./clang_util.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  c_parser
// ================================================================

// c_parser
//   class: parses C source code via libclang and populates a
// symbol_tree arena.  Conforms to the parser_base structural
// contract.
class c_parser
    : public parser_base<c_parser>
{
public:
    using input_type  = char;
    using result_type = arena::node_id;

    explicit c_parser
    (
        parse_context&                  _ctx,
        const char*                     _filename   = "input.c",
        const std::vector<const char*>& _extra_args = {}
    );

    parse_result<result_type> do_parse(parse_state<input_type>& _state);

private:
    parse_context&              m_ctx;
    const char*                 m_filename;
    std::vector<const char*>    m_args;
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_C_PARSER_
