/******************************************************************************
* djinterp [test]                                            free_tests.hpp
*
*   Declarations and shared fixtures for the parser/free.hpp unit suite -- the
* three "free" strata over the parser carrier: FreeAp (applicative), FreeSel
* (selective), and Free (monadic), plus lift_to_program / to_parser and the
* cross-stratum lifts.
*
*   The strata wrap parser<R, E>, so the fixtures are atomic parsers built
* directly on the parser handle: atom_val (succeed with a value, consume
* nothing), atom_char (consume one element iff it matches), atom_any (consume
* one element), and atom_fail (always fail).  run() interprets any stratum
* wrapper (they are all parser_expr-derived) on an input string and returns its
* parse_result; run_pos() returns how far the input was consumed, so tests can
* check residual threading.  Named callables (times2 / plus100 / to_code /
* add_len) are used where a parsed value must be transformed or where a parser
* must yield a function (for ap_apply / sel_select).
*
*   Every predicate lives flat in djinterp::testing and returns true iff all of
* its checks pass.  Each section .cpp keeps its predicates file-local (internal
* linkage) and exposes exactly one block-provider, declared below.
*
*   SECTIONS (mirroring parser/free.hpp's table of contents):
*     III-IV  monad stratum ............. free_tests_monad.cpp
*     V.      applicative stratum ....... free_tests_applicative.cpp
*     VI.     selective stratum ......... free_tests_selective.cpp
*     VII.    cross-stratum lifts ....... free_tests_crossstratum.cpp
*
*
* path:      /tests/djinterp/parse/parser/free_tests.hpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_PARSE_FREE_TESTS_
#define DJINTERP_PARSE_FREE_TESTS_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
// djinterp -- the header under test (which pulls in parse.hpp / parser.hpp /
//   the functional companion), plus the DTest authoring + runner surface.
//   NOTE: include paths are rooted at the djinterp include directory; adjust to
//   match your build tree.
#include "djinterp/parse/parser/free.hpp"
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   PARSE VOCABULARY (aliases)                          ///
///////////////////////////////////////////////////////////////////////////////

using ::djinterp::parse::parse_state;
using ::djinterp::parse::parse_result;
using ::djinterp::parse::parser;
using ::djinterp::parse::DParseStatusFailure;


///////////////////////////////////////////////////////////////////////////////
///                II.  RUNNERS                                             ///
///////////////////////////////////////////////////////////////////////////////

// run
//   helper: interprets any parser_expr-derived wrapper on an input string,
// returning its parse_result.  Erases through a parser<R,E> handle so every
// stratum wrapper is driven the same way.
template<typename _P>
parse_result<typename _P::result_type>
run(
    const _P&          _p,
    const std::string& _in
)
{
    parser<typename _P::result_type, typename _P::input_type> _handle(_p);
    parse_state<char> _st(_in.data(), _in.size(), 0);

    return _handle.parse(_st);
}

// run_pos
//   helper: like run() but returns how far the input was consumed (the final
// offset), for checking residual threading.
template<typename _P>
std::size_t
run_pos(
    const _P&          _p,
    const std::string& _in
)
{
    parser<typename _P::result_type, typename _P::input_type> _handle(_p);
    parse_state<char> _st(_in.data(), _in.size(), 0);

    _handle.parse(_st);

    return _st.offset;
}


///////////////////////////////////////////////////////////////////////////////
///                III. ATOMIC PARSERS                                      ///
///////////////////////////////////////////////////////////////////////////////

// atom_val
//   parser: always succeeds with _v, consuming no input.
template<typename _R>
parser<_R, char>
atom_val(
    _R _v
)
{
    return parser<_R, char>(
        [_v](parse_state<char>& /*_s*/) -> parse_result<_R>
        {
            return parse_result<_R>(_v);
        });
}

// atom_char
//   parser: consumes one element iff it equals _c; fails otherwise.
inline parser<char, char>
atom_char(
    char _c
)
{
    return parser<char, char>(
        [_c](parse_state<char>& _s) -> parse_result<char>
        {
            if ((!_s.at_end()) && (*_s.current() == _c))
            {
                char _d = *_s.current();
                _s.advance();
                return parse_result<char>(_d);
            }

            return parse_result<char>::make_error(
                DParseStatusFailure, _s.offset, "atom_char: mismatch");
        });
}

// atom_any
//   parser: consumes one element (any) and returns it; fails at end of input.
inline parser<char, char>
atom_any()
{
    return parser<char, char>(
        [](parse_state<char>& _s) -> parse_result<char>
        {
            if (!_s.at_end())
            {
                char _d = *_s.current();
                _s.advance();
                return parse_result<char>(_d);
            }

            return parse_result<char>::make_error(
                DParseStatusFailure, _s.offset, "atom_any: end of input");
        });
}

// atom_fail
//   parser: always fails without consuming input.
template<typename _R>
parser<_R, char>
atom_fail()
{
    return parser<_R, char>(
        [](parse_state<char>& _s) -> parse_result<_R>
        {
            return parse_result<_R>::make_error(
                DParseStatusFailure, _s.offset, "atom_fail");
        });
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  NAMED CALLABLES                                     ///
///////////////////////////////////////////////////////////////////////////////

// times2 / plus100 : int -> int
struct times2  { int operator()(int _x) const { return (_x * 2); } };
struct plus100 { int operator()(int _x) const { return (_x + 100); } };

// to_code : char -> int (the element's code)
struct to_code { int operator()(char _c) const { return static_cast<int>(_c); } };


///////////////////////////////////////////////////////////////////////////////
///                V.   SECTION BLOCK-PROVIDERS                             ///
///////////////////////////////////////////////////////////////////////////////

::djinterp::test::block_spec free_monad_block();
::djinterp::test::block_spec free_applicative_block();
::djinterp::test::block_spec free_selective_block();
::djinterp::test::block_spec free_crossstratum_block();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_PARSE_FREE_TESTS_
