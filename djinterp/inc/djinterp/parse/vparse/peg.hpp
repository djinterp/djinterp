/******************************************************************************
* djinterp [parse]                                                      peg.hpp
*
*
* path:      /inc/djinterp/parse/vparse/peg.hpp
******************************************************************************/
#ifndef VPARSE_PEG_HPP
#define VPARSE_PEG_HPP
// PEG / recursive-descent operator family. Owns its opcodes, its instruction
// format, and its program-counter-driven driver. Numbers opcodes from 0 — they
// live in this namespace and never collide with another family's.

// std
#include <string>
#include <vector>
// djinterp
#include "./machine.hpp"


enum { CHAR = 0, ANY, CHOICE, JUMP, CALL, RETURN, COMMIT, FAIL, MATCH, SET };

struct Instr { int op; int arg = -1; char ch = 0; std::string set; };
using Program = std::vector<Instr>;

op_set      make_ops();                                      // build this family's registry
void        run(machine& M, const Program& prog, const op_set& ops);  // PC-driven driver
std::string fmt(const Instr& ins);                           // disassemble one instruction

#endif