/*******************************************************************************
* djinterp [parsegen]                                                     lr.hpp
*
*   The LR (shift/reduce) operator family: its opcodes, its table format, and
* its table-driven driver. It knows nothing about any particular grammar --
* token kinds and nonterminals are plain ints supplied by the table and by
* whatever lexes the input.
*   It is the second recognizer family under the same machine, which is what
* the operator-agnostic substrate exists for. A left-recursive grammar such as
* E -> E + T | T, which the PEG family cannot run, runs here unchanged.
*   WHAT IS NOT HERE: table construction. A table is supplied, not built from a
* grammar; generating one (LR(0), SLR, LALR) is the `gen`-targets-`lr::table`
* item on the roadmap.
*   Consolidated from the standalone prototype's lr module.
*
*
* path:      /inc/djinterp/parsegen/vparse/lr.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_LR_HPP
#define DJINTERP_PARSEGEN_VPARSE_LR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <functional>       // std::function
#include <map>              // std::map
#include <string>           // std::string
#include <utility>          // std::pair
#include <vector>           // std::vector
// djinterp
#include "./machine.hpp"    // machine, op_set, NS_VPARSE


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace lr
{

// opcodes
//   enum: this family's private opcode space, numbered from zero like every
// other family's -- they never collide, because each family owns its op_set.
enum
{
    SHIFT = 0,
    REDUCE,
    ACCEPT,
    ERROR
};

// action
//   struct: what the table says to do in a state on a lookahead token. `arg` is
// the next state for SHIFT, or the production index for REDUCE.
struct action
{
    int  kind;
    long arg;
};

// table
//   struct: an LR parse table. GOTO has no opcode of its own: the driver folds
// it into REDUCE, which pops the right-hand side and then follows `gotos`.
// `semantic` computes a reduction's value; the two name functions exist only
// for traces, and both are optional.
struct table
{
    std::map<std::pair<int, int>, action>              actions;
    std::map<std::pair<int, int>, int>                 gotos;
    std::vector<std::pair<int, int>>                   productions;
    std::function<long(int, const std::vector<long>&)> semantic;
    std::function<std::string(int)>                    token_name;
    std::function<std::string(int)>                    production_name;
};

// tokens
//   type: the input as (kind, value) pairs. Must end with the end-of-input
// token the table accepts on.
using tokens = std::vector<std::pair<int, long>>;

// result
//   struct: the outcome of one run.
struct result
{
    bool        ok;
    long        value;
    std::string error;
    long        steps;
};

op_set make_ops();
result run(machine&      _machine,
           const table&  _table,
           const tokens& _tokens,
           const op_set& _ops);

}  // namespace lr

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSEGEN_VPARSE_LR_HPP
