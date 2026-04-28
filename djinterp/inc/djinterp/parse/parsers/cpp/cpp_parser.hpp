/******************************************************************************
* djinterp [core]                                            cpp_parser.hpp
*
* C++ source parser (libclang-backed):
*   This header defines a concrete parser that processes C++ source code
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
* path:      /inc/cpp/parse/cpp_parser.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.18
******************************************************************************/

#ifndef DJINTERP_CPP_PARSER_
#define DJINTERP_CPP_PARSER_ 1

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
//  C++ storage flag encoding
// ================================================================

// cpp_storage_flag
//   constants: packed into symbol_data::storage for C++ method
// qualifiers.  The lower two bits match the C encoding (static,
// extern); the upper bits carry C++-specific qualifiers.
namespace cpp_storage_flag
{
    constexpr std::uint8_t none          = 0x00;
    constexpr std::uint8_t static_       = 0x01;
    constexpr std::uint8_t extern_       = 0x02;
    constexpr std::uint8_t virtual_      = 0x04;
    constexpr std::uint8_t pure_virtual  = 0x08;
    constexpr std::uint8_t const_        = 0x10;
    constexpr std::uint8_t inline_       = 0x20;
    constexpr std::uint8_t constexpr_    = 0x40;
};


// ================================================================
//  cpp_parser
// ================================================================

// cpp_parser
//   class: parses C++ source code via libclang and populates a
// symbol_tree arena.  Conforms to the parser_base structural
// contract.
class cpp_parser
    : public parser_base<cpp_parser>
{
public:
    using input_type  = char;
    using result_type = arena::node_id;

    explicit cpp_parser
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


#endif  // DJINTERP_CPP_PARSER_
