/*******************************************************************************
* djinterp [cpp/parse]                                       expr_face_tests.hpp
*
* Expression core v2's C++ face (cpp/modules/parse/expr/) in DTest's C face
* (c/modules/test/test_suite.h), at every C++ level from C++98, in three
* blocks:
*   face    the face's own rules: terms and their storage, copies and moves,
*           signatures, sources, the result's diagnostics, renders, folds and
*           walks, and what a callback throws;
*   parity  the face against the C API it stands on: one language built both
*           ways, and each text parsed both ways, the terms, results,
*           diagnostics, renders, folds, walks and digests held equal, node
*           for node;
*   bridge  the bridge to v1's term (bridge.hpp), from C++11, v1's floor;
*           below it each test is skipped, and says so.
*   The fixtures are C++ and the tests C-linked, as the spec's rows call
* them. Two faces, chosen by DTEST_SPEC_MODE: a section unit sees the face,
* the C core and the fixtures; the runner, which defines DTEST_SPEC_MODE
* first, sees the tests and the spec provider, d_tests_parse_expr_face_spec().
*
*
* TABLE OF CONTENTS
* =================
* 1.  FIXTURES
*     --------
*     1.  The language
*     2.  The C parse
* 2.  TESTS
*     -----
* 3.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/cpp/modules/parse/expr/expr_face_tests.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_PARSE_EXPR_FACE_TESTS_HPP
#define DJINTERP_TESTS_CPP_PARSE_EXPR_FACE_TESTS_HPP 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs

#ifndef DTEST_SPEC_MODE
    // std
    #include <algorithm>  // std::equal
    #include <cstddef>    // std::size_t
    #include <cstring>    // std::memset, std::strcmp
    #include <stdexcept>  // std::runtime_error
    #include <string>     // std::string
    #include <vector>     // std::vector
    // djinterp
    #include "../../../../../../inc/djinterp/cpp/modules/parse/expr/expr.hpp"    // the face
    #include "../../../../../../inc/djinterp/cpp/modules/parse/expr/bridge.hpp"  // from C++11
    #include "../../../../../../inc/djinterp/c/modules/parse/expr/expr.h"        // the C core
#endif  // DTEST_SPEC_MODE


//==============================================================================
// 1.  FIXTURES
//==============================================================================


#ifndef DTEST_SPEC_MODE

// D_TESTS_FACE_FULL
//   macro: 1 where the parts the suite stands on are built (the heap, the
// scanner and the renderer) and the depth guard's knob lets its texts
// through (16 or more); where not, each test is skipped, and says why.
#if ( (D_INTERNAL_PARSE_EXPR_HEAP == 1)   &&  \
      (D_INTERNAL_PARSE_EXPR_SCAN == 1)   &&  \
      (D_INTERNAL_PARSE_EXPR_RENDER == 1) &&  \
      (D_CFG_PARSE_EXPR_MAX_DEPTH >= 16) )
    #define D_TESTS_FACE_FULL 1
#else
    #define D_TESTS_FACE_FULL 0
#endif

// D_TESTS_FACE_WITHOUT
//   constant: the reason a test gives where D_TESTS_FACE_FULL is 0.
#define D_TESTS_FACE_WITHOUT \
    "the heap, the scanner or the renderer is not built, or the depth " \
    "guard's knob is under 16"

#endif  // DTEST_SPEC_MODE


#if ( (!defined(DTEST_SPEC_MODE)) && (D_TESTS_FACE_FULL == 1) )

// 1.1    The language
//------------------------------------------------------------------------------
// A calculator: numbers and names, a sign, the four operations and a power,
// a factorial, relations chained by `and`, `and`, `or` and `not` as words,
// a group, a call, an index, vectors with rows, bars, and a product written
// by juxtaposition, never between two numbers. Built by the C API and by the
// face, each in a section's fixtures (expr_face_tests_fixtures.cpp).

// classes
#define D_TESTS_FACE_C_NUM      1u
#define D_TESTS_FACE_C_NAME     2u
#define D_TESTS_FACE_C_STRAY    3u
#define D_TESTS_FACE_C_PLUS     4u
#define D_TESTS_FACE_C_MINUS    5u
#define D_TESTS_FACE_C_STAR     6u
#define D_TESTS_FACE_C_SLASH    7u
#define D_TESTS_FACE_C_CARET    8u
#define D_TESTS_FACE_C_BANG     9u
#define D_TESTS_FACE_C_LP       10u
#define D_TESTS_FACE_C_RP       11u
#define D_TESTS_FACE_C_COMMA    12u
#define D_TESTS_FACE_C_LB       13u
#define D_TESTS_FACE_C_RB       14u
#define D_TESTS_FACE_C_SEMI     15u
#define D_TESTS_FACE_C_LT       16u
#define D_TESTS_FACE_C_LE       17u
#define D_TESTS_FACE_C_AND      18u
#define D_TESTS_FACE_C_OR       19u
#define D_TESTS_FACE_C_NOT      20u
#define D_TESTS_FACE_C_BAR      21u

// operators
#define D_TESTS_FACE_O_NEG      1u
#define D_TESTS_FACE_O_ADD      2u
#define D_TESTS_FACE_O_SUB      3u
#define D_TESTS_FACE_O_MUL      4u
#define D_TESTS_FACE_O_DIV      5u
#define D_TESTS_FACE_O_POW      6u
#define D_TESTS_FACE_O_FACT     7u
#define D_TESTS_FACE_O_GROUP    8u
#define D_TESTS_FACE_O_CALL     9u
#define D_TESTS_FACE_O_INDEX    10u
#define D_TESTS_FACE_O_VECTOR   11u
#define D_TESTS_FACE_O_ROW      12u
#define D_TESTS_FACE_O_MATRIX   13u
#define D_TESTS_FACE_O_LT       14u
#define D_TESTS_FACE_O_LE       15u
#define D_TESTS_FACE_O_AND      16u
#define D_TESTS_FACE_O_OR       17u
#define D_TESTS_FACE_O_NOT      18u
#define D_TESTS_FACE_O_ABS      19u

// d_tests_face_texts, d_tests_face_text_count
//   constant: the texts the parity block parses both ways, well formed and
// not, and how many there are.
extern const char* const d_tests_face_texts[];
extern const std::size_t d_tests_face_text_count;

bool d_tests_face_c_language(::dj_parse_expr_signature* _signature);
bool d_tests_face_language(djinterp::parse::expr::DSignature& _signature);
::dj_parse_expr_scan_config d_tests_face_config(void);

// 1.2    The C parse
//------------------------------------------------------------------------------
// DTestsFaceCParse
//   struct: one parse by the C API alone: its term, its sink and the sink's
// room, and its result.
struct DTestsFaceCParse
{
    ::dj_parse_expr_term   m_term;
    ::dj_parse_diag_sink   m_sink;
    ::dj_parse_diagnostic  m_items[D_PARSE_DIAG_DEFAULT_ITEMS];
    char                   m_text[D_PARSE_DIAG_DEFAULT_TEXT];
    ::dj_parse_expr_result m_result;
};

bool d_tests_face_c_parse(const ::dj_parse_expr_signature* _signature,
                          const char*                      _text,
                          const ::dj_parse_expr_options*   _options,
                          DTestsFaceCParse&                _out);
void d_tests_face_c_release(DTestsFaceCParse& _parse);
djinterp::parse::expr::DResult
     d_tests_face_scan(const djinterp::parse::expr::DSignature& _signature,
                       const char*                              _text,
                       const ::dj_parse_expr_options*           _options =
                           NULL);
bool d_tests_face_same_span(const ::dj_parse_span& _a,
                            const ::dj_parse_span& _b);
bool d_tests_face_same_nodes(const ::dj_parse_expr_term* _a,
                             const ::dj_parse_expr_term* _b);

#endif  // fixtures


D_EXTERN_C_BEGIN


//==============================================================================
// 2.  TESTS
//==============================================================================


// the face (expr_face_tests_face.cpp)
bool d_tests_face_term_storage(void);
bool d_tests_face_term_copies(void);
bool d_tests_face_signature_building(void);
bool d_tests_face_sources(void);
bool d_tests_face_diagnostics(void);
bool d_tests_face_render(void);
bool d_tests_face_fold(void);
bool d_tests_face_walk(void);

// parity (expr_face_tests_parity.cpp)
bool d_tests_face_parity_signature(void);
bool d_tests_face_parity_built(void);
bool d_tests_face_parity_parse(void);
bool d_tests_face_parity_options(void);
bool d_tests_face_parity_render(void);
bool d_tests_face_parity_fold(void);
bool d_tests_face_parity_walk(void);

// the bridge (expr_face_tests_bridge.cpp)
bool d_tests_face_bridge_to(void);
bool d_tests_face_bridge_shares(void);
bool d_tests_face_bridge_refusals(void);
bool d_tests_face_bridge_from(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_parse_expr_face_face
//   constant: the face block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_face_face[] =
{
    { "d_tests_face_term_storage",
      "a term on the heap, and in the caller's storage until it is full",
      d_tests_face_term_storage },
    { "d_tests_face_term_copies",
      "copies are deep and equal; a move leaves the source empty",
      d_tests_face_term_copies },
    { "d_tests_face_signature_building",
      "each form defined, refusals noted, sealed and found",
      d_tests_face_signature_building },
    { "d_tests_face_sources",
      "functor, callable, array and vector sources; a throw stops a parse",
      d_tests_face_sources },
    { "d_tests_face_diagnostics",
      "copied with their messages, or left in the caller's sink",
      d_tests_face_diagnostics },
    { "d_tests_face_render",
      "into a string, through a writer and an atom writer; a throw fails",
      d_tests_face_render },
    { "d_tests_face_fold",
      "values of any type, a shared node once, holes, a throw unwinds",
      d_tests_face_fold },
    { "d_tests_face_walk",
      "depth first, skipped and stopped, parents given",
      d_tests_face_walk }
};

// d_tests_parse_expr_face_parity
//   constant: the parity block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_face_parity[] =
{
    { "d_tests_face_parity_signature",
      "the language built both ways holds the same rows",
      d_tests_face_parity_signature },
    { "d_tests_face_parity_built",
      "terms built and copied both ways are the same, node for node",
      d_tests_face_parity_built },
    { "d_tests_face_parity_parse",
      "each text: the same term, result and diagnostics",
      d_tests_face_parity_parse },
    { "d_tests_face_parity_options",
      "the same with the error allowance, the order and the guard set",
      d_tests_face_parity_options },
    { "d_tests_face_parity_render",
      "the same text, plain, kept groups and compact",
      d_tests_face_parity_render },
    { "d_tests_face_parity_fold",
      "the same value at each root, from the same nodes",
      d_tests_face_parity_fold },
    { "d_tests_face_parity_walk",
      "the same visits, plain, skipped and stopped; the same digest",
      d_tests_face_parity_walk }
};

// d_tests_parse_expr_face_bridge
//   constant: the bridge block's rows.
static const struct dj_test_case_spec d_tests_parse_expr_face_bridge[] =
{
    { "d_tests_face_bridge_to",
      "a parsed term as v1's, the same shape as the face's fold",
      d_tests_face_bridge_to },
    { "d_tests_face_bridge_shares",
      "a chain's shared operand shared in v1's term too",
      d_tests_face_bridge_shares },
    { "d_tests_face_bridge_refusals",
      "holes refused unless asked for; a root that is no node",
      d_tests_face_bridge_refusals },
    { "d_tests_face_bridge_from",
      "a v1 table defined in a v2 signature, which parses by it",
      d_tests_face_bridge_from }
};

// d_tests_parse_expr_face_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_parse_expr_face_blocks[] =
{
    { "face",
      "the face's own rules",
      d_tests_parse_expr_face_face,
      sizeof(d_tests_parse_expr_face_face) /
          sizeof(d_tests_parse_expr_face_face[0]) },
    { "parity",
      "the face against the C API, node for node",
      d_tests_parse_expr_face_parity,
      sizeof(d_tests_parse_expr_face_parity) /
          sizeof(d_tests_parse_expr_face_parity[0]) },
    { "bridge",
      "the bridge to v1's term, from C++11",
      d_tests_parse_expr_face_bridge,
      sizeof(d_tests_parse_expr_face_bridge) /
          sizeof(d_tests_parse_expr_face_bridge[0]) }
};

// d_tests_parse_expr_face_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_parse_expr_face_module =
{
    "parse_expr_face",
    "cpp/modules/parse/expr/: expression core v2's C++ face",
    d_tests_parse_expr_face_blocks,
    sizeof(d_tests_parse_expr_face_blocks) /
        sizeof(d_tests_parse_expr_face_blocks[0])
};

// d_tests_parse_expr_face_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_parse_expr_face_spec(void)
{
    return &d_tests_parse_expr_face_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_CPP_PARSE_EXPR_FACE_TESTS_HPP
