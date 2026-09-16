/******************************************************************************
* djinterp [vparse]                                                    peg.hpp
*
*   The PEG / recursive-descent operator family: its opcode space, instruction
* format (`instr` / `program`), the program-counter-driven driver (`run`), and
* a disassembler (`fmt`).  Opcodes are numbered from zero in this family's
* namespace and never collide with another family's.  Definitions live in
* peg.cpp; this header carries only declarations and the data formats.
*
* path:      /inc/djinterp/parse/parsegen/vparse/peg.hpp
* link(s):   TBA
* author(s): vparse                                        created: 2026.06.19
******************************************************************************/

#ifndef DJINTERP_PARSE_PARSEGEN_VPARSE_PEG_
#define DJINTERP_PARSE_PARSEGEN_VPARSE_PEG_ 1

// std
#include <string>
#include <vector>
// djinterp
#include "./machine.hpp"


NS_DJINTERP
NS_PARSEGEN


// opcodes
//   enum: this family's private opcode space.  Used as keys into a peg op_set;
// the engine dispatches on these values without naming them.
enum
{
    CHAR = 0, ANY, CHOICE, JUMP, CALL, RETURN, COMMIT, FAIL, MATCH, SET
};

// instr
//   struct: one PEG instruction -- an opcode plus its operands (a jump target,
// a literal char, or a character-class set).  Public-payload layout, so the
// operand fields are unprefixed.
struct instr
{
    int         op;
    int         arg = -1;
    char        ch  = 0;
    std::string set;
};

// program
//   type: a flat PEG instruction stream, indexed by the program counter.
using program = std::vector<instr>;

op_set      make_ops();
void        run(machine&        _m,
                const program&  _prog,
                const op_set&   _ops);
std::string fmt(const instr& _ins);


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PARSEGEN_VPARSE_PEG_
