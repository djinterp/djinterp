/******************************************************************************
* djinterp [vparse]                                                    gen.hpp
*
*   The virtual parser GENERATOR: an operator family whose voperators emit and
* back-patch instructions of an OUTPUT program instead of consuming input.  A
* generation program (gen::program) runs on the same machine via a PC-driven
* driver and halts with a program -- generator and generated are the same
* kind of object.  Understands the full ruleset formalism: alternatives (an
* ordered-choice cascade), repetition (a CHOICE/COMMIT loop), and captures
* (MARK/CAP).  plan() linearises a ruleset into directives; emission and address
* resolution happen in the voperators.
*
*
* path:      /inc/djinterp/parse/vparse/gen.hpp
* author(s): vparse                                        created: 2026.06.19
******************************************************************************/

#ifndef DJINTERP_PARSE_VPARSE_GEN_
#define DJINTERP_PARSE_VPARSE_GEN_ 1

// std
#include <string>
#include <vector>
// djinterp
#include "./machine.hpp"
#include "./peg.hpp"
#include "./ruleset.hpp"


NS_DJINTERP
NS_PARSE

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
program      generate(const gen_program& _plan, const op_set& _ops);
program      compile(const ruleset& _grammar);


NS_END  // parse
NS_END  // djinterp

#endif  // DJINTERP_PARSE_VPARSE_GEN_
