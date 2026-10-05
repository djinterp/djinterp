/*******************************************************************************
* djinterp [parse]                                                clang_util.hpp
*
* libclang shared utilities:
*   This header provides the common libclang infrastructure shared by
* both the C and C++ parsers.  It is intentionally free of parser_base
* coupling — it operates purely on libclang cursors and the arena/
* string_table types.
*
* Contents:
*   - cx_to_string           CXString -> std::string conversion
*   - cx_source_location     cursor -> source_location extraction
*   - cx_stable_id           cursor -> stable_id hashing
*   - cx_kind_to_symbol_kind CXCursorKind -> symbol_kind mapping
*   - cx_access_specifier    CXCursor -> access_specifier mapping
*   - clang_visitor_context  callback context for clang_visitChildren
*   - clang_visit_recursive  shared recursive visitor
*   - clang_parse_tu         translation unit creation helper
*
* Link:
*   Requires linking against libclang (-lclang).
*
*
* path:      /inc/djinterp/parse/parsers/cpp/clang_util.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSERS_CPP_CLANG_UTIL_HPP
#define DJINTERP_PARSE_PARSERS_CPP_CLANG_UTIL_HPP 1

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
#include <functional>
#include <string>
#include <vector>
// clang
#include <clang-c/Index.h>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../core/container/arena/arena.hpp"
#include "cross_ref.hpp"
#include "../../parse_context.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t, uint8_t,
                                                   // uint16_t


NS_DJINTERP
NS_PARSE


// ================================================================
//  CXString helpers
// ================================================================

std::string             cx_to_string(CXString _cx);


// ================================================================
//  cursor data extraction
// ================================================================

arena::source_location  cx_source_location(CXCursor _cursor, arena::string_table& _strings);
re_std::uint64_t           cx_stable_id(CXCursor _cursor);
arena::string_id        cx_raw_comment(CXCursor _cursor, arena::string_table& _strings);
arena::string_id        cx_type_spelling(CXCursor _cursor, arena::string_table& _strings);
arena::string_id        cx_cursor_spelling(CXCursor _cursor, arena::string_table& _strings);
arena::string_id        cx_function_signature(CXCursor _cursor, arena::string_table& _strings);
re_std::uint8_t            cx_storage_class(CXCursor _cursor);


// ================================================================
//  internal  —  hashing (used by multiple .cpp files)
// ================================================================

NS_INTERNAL

    re_std::uint64_t fnv1a_64(const char* _data, std::size_t _length);
    re_std::uint64_t fnv1a_64_string(const std::string& _str);
    re_std::uint64_t fnv1a_64_combine(re_std::uint64_t _a, re_std::uint64_t _b);

NS_END  // internal


// ================================================================
//  kind mapping
// ================================================================

re_std::uint16_t           cx_kind_to_symbol_kind_c(CXCursorKind _kind);
re_std::uint16_t           cx_kind_to_symbol_kind_cpp(CXCursorKind _kind);
re_std::uint8_t            cx_access_specifier(CXCursor _cursor);


// ================================================================
//  visitor infrastructure
// ================================================================

// cursor_filter_fn
//   typedef: callback type for deciding whether a cursor should
// be visited.  Returns true to visit, false to skip.
typedef bool (*cursor_filter_fn)(CXCursorKind _kind);

// cursor_enrich_fn
//   typedef: callback type for enriching a symbol_data with
// language-specific details after the common fields have been
// populated.
typedef void (*cursor_enrich_fn)(
    CXCursor                _cursor,
    arena::symbol_data&     _data,
    arena::string_table&    _strings
);

// clang_visitor_context
//   struct: callback context passed through clang_visitChildren
// as CXClientData.
struct clang_visitor_context
{
    parse_context*      ctx;
    arena::node_id      parent_id;
    cursor_filter_fn    filter;
    cursor_enrich_fn    enrich;
};

CXChildVisitResult clang_visit_cursor(CXCursor _cursor, CXCursor _parent, CXClientData _data);


// ================================================================
//  translation unit helpers
// ================================================================

bool clang_parse_tu(const char*         _source_data,
                    std::size_t         _source_length,
                    const char*         _filename,
                    const char* const*  _args,
                    int                 _num_args,
                    unsigned            _options,
                    CXIndex&            _out_index,
                    CXTranslationUnit&  _out_tu);

arena::node_id clang_walk_tu(CXTranslationUnit _tu,
                             parse_context&     _ctx,
                             const char*        _filename,
                             cursor_filter_fn   _filter,
                             cursor_enrich_fn   _enrich);


NS_END  // parse
NS_END  // djinterp

#endif  // D_INTERNAL_PARSE_LIBCLANG

#endif  // DJINTERP_PARSE_PARSERS_CPP_CLANG_UTIL_HPP
