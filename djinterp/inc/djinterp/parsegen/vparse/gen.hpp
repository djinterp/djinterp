/*******************************************************************************
* djinterp [parsegen]                                                    gen.hpp
*
*   The virtual parser GENERATOR: an operator family whose voperators emit and
* back-patch instructions of an OUTPUT program instead of consuming input.  A
* generation program (gen::program) runs on the same machine via a PC-driven
* driver and halts with a peg::program -- generator and generated are the same
* kind of object.  Understands the full ruleset formalism: alternatives (an
* ordered-choice cascade), repetition (a CHOICE/COMMIT loop), and captures
* (MARK/CAP).  plan() linearises a ruleset into directives; emission and address
* resolution happen in the voperators.
*
*
* path:      /inc/djinterp/parsegen/vparse/gen.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_GEN_HPP
#define DJINTERP_PARSEGEN_VPARSE_GEN_HPP 1

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
#include "./peg.hpp"
#include "./ruleset.hpp"

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// gen
//   namespace: the generator operator family.
namespace gen {

// directives
//   enum: this family's private opcode space -- the generation directives.
enum
{
    G_PREAMBLE = 0, G_RULE_BEGIN, G_RULE_END, G_ALT_CHOICE, G_ALT_COMMIT,
    G_EMIT_CHAR, G_EMIT_SET, G_EMIT_REF, G_EMIT_ANY, G_EMIT_MARK, G_EMIT_CAP,
    G_STAR_BEGIN, G_STAR_END, G_FINISH
};

// gen_instr
//   struct: one generation directive plus operands.
struct gen_instr
{
    int         op;
    char        ch  = 0;
    std::string text;
    int         tag = 0;
};

// gen_program
//   type: a generation program.
using gen_program = std::vector<gen_instr>;

op_set       make_ops();
gen_program  plan(const ruleset& _grammar);
peg::program generate(const gen_program& _plan, const op_set& _ops);
peg::program compile(const ruleset& _grammar);

}  // namespace gen

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_GEN_HPP
