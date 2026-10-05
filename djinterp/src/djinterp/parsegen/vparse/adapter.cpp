/*******************************************************************************
* djinterp [parsegen]                                                adapter.cpp
*
* Definitions for `adapter.hpp`.
*
*
* path:      /src/djinterp/parsegen/vparse/adapter.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.01
*******************************************************************************/
// djinterp
#include "djinterp/parsegen/vparse/adapter.hpp"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstddef>
#include <string>
#include <vector>

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// as_parser
//   function: wrap a compiled vparse program as a parser<std::string, char>.
// The returned handle runs the program over the parse_state from its offset;
// on a match it advances the offset (so a following parser continues where
// this one stopped) and yields the matched text, and on failure it leaves the
// offset put and yields a parse_error.  The op registry is built once and
// carried by the closure.
parser<std::string, char>
as_parser(const peg::program& _prog)
{
    const peg::program prog = _prog;
    const op_set       ops  = peg::make_ops();

    return parser<std::string, char>(
        [prog, ops](parse_state<char>& _st) -> parse_result<std::string>
        {
            const std::size_t         start = _st.offset;
            std::vector<peg::capture> caps;

            const bool matched = peg::run(_st, prog, ops, &caps);

            if (matched)
            {
                std::string text(_st.data + start, _st.data + _st.offset);
                return parse_result<std::string>::make_ok(text);
            }

            return parse_result<std::string>::make_error(
                DParseStatusFailure, _st.offset, "vparse: no match");
        });
}

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
