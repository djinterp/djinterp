/*******************************************************************************
* djinterp [parse]                                                  c_parser.hpp
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
* path:      /inc/djinterp/parse/parsers/c/c_parser.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSERS_C_C_PARSER_HPP
#define DJINTERP_PARSE_PARSERS_C_C_PARSER_HPP 1

// D_INTERNAL_PARSE_LIBCLANG
//   detection: defined when libclang's C API, <clang-c/Index.h>, can be
// included. The parser below is built on it; without it this header
// compiles to nothing.
#if defined(__has_include)
    #if __has_include(<clang-c/Index.h>)
        #define D_INTERNAL_PARSE_LIBCLANG 1
    #endif
#endif

#ifdef D_INTERNAL_PARSE_LIBCLANG

// std
#include <cstddef>
#include <vector>
// clang
#include <clang-c/Index.h>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../core/container/arena/arena.hpp"
#include "../../parse.hpp"
#include "../../parser/parser.hpp"
#include "../../parse_context.hpp"
#include "../cpp/clang_util.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // fixed-width integers


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

#endif  // D_INTERNAL_PARSE_LIBCLANG

#endif  // DJINTERP_PARSE_PARSERS_C_C_PARSER_HPP
