/*******************************************************************************
* djinterp [parse]                                            cpp_def_parser.hpp
*
* C++ definition parser (libclang-backed):
*   This header defines a parser specialized for extracting function and
* method definitions from .cpp implementation files. It captures
* everything a documentation/change-tracking tool needs about a
* definition: the body range, local variables, call expressions, and
* the relationship back to the declaration.
*
* Interface:
*   - input_type   = char      (source text)
*   - result_type  = node_id   (root of the parsed definition subtree)
*   - do_parse(parse_state<char>&) -> parse_result<node_id>
*
* Arena structure produced:
*   The root node represents the translation unit.  Each definition is
* a child of the root, carrying a definition_data payload.  Children
* of a definition node represent local declarations (variables,
* parameters) and call expressions found in the body.
*
*
* path:      /inc/djinterp/parse/parsers/cpp/cpp_def_parser.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSERS_CPP_CPP_DEF_PARSER_HPP
#define DJINTERP_PARSE_PARSERS_CPP_CPP_DEF_PARSER_HPP 1

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
#include <string>
#include <vector>
// clang
#include <clang-c/Index.h>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../core/container/arena/arena.hpp"
#include "cross_ref.hpp"
#include "../../parse.hpp"
#include "../../parser/parser.hpp"
#include "../../parse_context.hpp"
#include "./clang_util.hpp"
#include "./cpp_parser.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t


NS_DJINTERP
NS_PARSE


// ================================================================
//  definition-specific symbol kinds
// ================================================================

// def_symbol_kind
//   constants: extend arena::symbol_kind for definition-internal
// nodes that only appear in the def_parser output.
namespace def_symbol_kind
{
    constexpr re_std::uint16_t function_def    = 0x0100;
    constexpr re_std::uint16_t call_expr       = 0x0101;
    constexpr re_std::uint16_t local_var       = 0x0102;
    constexpr re_std::uint16_t return_stmt     = 0x0103;
    constexpr re_std::uint16_t member_ref      = 0x0104;
    constexpr re_std::uint16_t decl_ref        = 0x0105;
};


// ================================================================
//  cpp_def_parser
// ================================================================

// cpp_def_parser
//   class: parses C++ implementation files and extracts all
// function/method definitions with their internal structure
// (parameters, local variables, call expressions, member
// references, return statements).
class cpp_def_parser
    : public parser_base<cpp_def_parser>
{
public:
    using input_type  = char;
    using result_type = arena::node_id;

    explicit cpp_def_parser
    (
        parse_context&                  _ctx,
        const char*                     _filename   = "input.cpp",
        const char*                     _standard   = "-std=c++17",
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

#endif  // DJINTERP_PARSE_PARSERS_CPP_CPP_DEF_PARSER_HPP
