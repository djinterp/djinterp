/*******************************************************************************
* djinterp [parsegen]                                                    peg.hpp
*
*   The PEG operator family, rebased onto the parse carrier.  run() consumes a
* parse_state<char> (parsing from its offset) and follows the formal match-or-
* restore contract: on success it advances the state's offset to the match end
* and, if a sink is given, hands back the tagged capture spans; on failure it
* leaves the offset unchanged.  Opcodes are family-private; MARK / CAP are
* backtrack-safe.  A compiled program becomes a first-class parser<R, char> via
* the adapter (adapter.hpp).
*
*
* path:      /inc/djinterp/parsegen/vparse/peg.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_PEG_HPP
#define DJINTERP_PARSEGEN_VPARSE_PEG_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>
#include <vector>
// djinterp
#include "./machine.hpp"
#include "../../parse/parse.hpp"

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// the parse carrier vparse builds on (djinterp::parse)
using ::djinterp::parse::parse_state;
using ::djinterp::parse::parse_result;
using ::djinterp::parse::parse_error;
using ::djinterp::parse::parse_status;
using ::djinterp::parse::DParseStatusSuccess;
using ::djinterp::parse::DParseStatusFailure;
using ::djinterp::parse::DParseStatusEndOfInput;
using ::djinterp::parse::DParseStatusMalformed;

// peg
//   namespace: the PEG operator family.
namespace peg {

// opcodes
//   enum: this family's private opcode space.
enum
{
    CHAR = 0, ANY, CHOICE, JUMP, CALL, RETURN, COMMIT, FAIL, MATCH, SET,
    MARK, CAP
};

// instr
//   struct: one PEG instruction (arg is a jump target or a capture tag).
struct instr
{
    int         op;
    int         arg = -1;
    char        ch  = 0;
    std::string set;
};

// program
//   type: a flat PEG instruction stream.
using program = std::vector<instr>;

// capture
//   struct: one tagged span -- the tag and the half-open input range.
struct capture
{
    int tag;
    int start;
    int end;
};

op_set      make_ops();
bool        run(parse_state<char>&    _state,
                const program&        _prog,
                const op_set&         _ops,
                std::vector<capture>* _caps = nullptr);
std::string fmt(const instr& _ins);

// run (caller's machine)
//   function: run() on a machine the caller supplies, as lr::run takes one.
// Its step_limit is honoured, and when its trace is set, one line is appended
// per dispatch, before the operator runs:
//
//     ip=<ip> off=<cursor> calls=<n> back=<n> marks=<n> caps=<n> | <fmt(instr)>
//
// Everything before " | " is space-separated key=value integers; everything
// after it is the instruction's disassembly. On return, steps / ok / halted /
// error describe the run and ext is cleared. The four-argument run() above is
// this with a default machine.
bool        run(machine&              _machine,
                parse_state<char>&    _state,
                const program&        _prog,
                const op_set&         _ops,
                std::vector<capture>* _caps = nullptr);

}  // namespace peg

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_PEG_HPP
