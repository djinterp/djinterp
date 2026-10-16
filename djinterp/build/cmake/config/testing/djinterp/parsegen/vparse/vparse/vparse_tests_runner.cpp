// std
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
// djinterp
#include <djinterp/parsegen/vparse/machine.hpp>
#include <djinterp/parsegen/vparse/peg.hpp>




// build_digits_program
//   function: hand-assemble the PEG program for `S = [0-9]+ ;` (one or more
// digits, whole input).  Exercises CALL/RETURN, SET, the CHOICE/COMMIT star
// loop, the `!.` end-of-input predicate, and MATCH -- 8 of the 9 opcodes
// (JUMP is reserved for optimization and stays unused, as expected).
using djinterp::parsegen::program;

static program
build_digits_program()
{
    using namespace djinterp::parsegen;

    const std::string digits = "0123456789";
    program           p;

    //  0: CALL 6        enter rule S
    //  1: CHOICE 5      \                          .
    //  2: ANY            )  end-of-input test  !.
    //  3: COMMIT 4       )
    //  4: FAIL          /
    //  5: MATCH         accept
    //  6: SET [0-9]     S: one digit ...
    //  7: CHOICE 10     \                          .
    //  8: SET [0-9]      )  ... then zero or more
    //  9: COMMIT 7      /
    // 10: RETURN
    p.push_back(instr{ CALL,    6,  0, ""     });
    p.push_back(instr{ CHOICE,  5,  0, ""     });
    p.push_back(instr{ ANY,    -1,  0, ""     });
    p.push_back(instr{ COMMIT,  4,  0, ""     });
    p.push_back(instr{ FAIL,   -1,  0, ""     });
    p.push_back(instr{ MATCH,  -1,  0, ""     });
    p.push_back(instr{ SET,    -1,  0, digits });
    p.push_back(instr{ CHOICE, 10,  0, ""     });
    p.push_back(instr{ SET,    -1,  0, digits });
    p.push_back(instr{ COMMIT,  7,  0, ""     });
    p.push_back(instr{ RETURN, -1,  0, ""     });

    return p;
}

int
main()
{
    using djinterp::parsegen::machine;
    using djinterp::parsegen::op_set;

    op_set  ops  = djinterp::parsegen::make_ops();
    program prog = build_digits_program();

    std::cout << "vparse on djinterp -- PEG family, program `S = [0-9]+`\n";
    std::cout << "program: " << prog.size() << " instructions\n\n";

    // disassembly
    for (std::size_t a = 0; a < prog.size(); ++a)
    {
        std::cout << "  " << std::setw(2) << a
                  << "  " << fmt(prog[a]) << "\n";
    }

    std::cout << "\nruns:\n";

    const std::vector<std::string> cases =
        { "12345", "007", "12", "12a", "a12", "" };

    for (const std::string& s : cases)
    {
        machine m;
        m.input = &s;

        run(m, prog, ops);

        std::cout << "  " << (m.ok ? "OK  " : "FAIL")
                  << "  " << std::left << std::setw(8) << ("'" + s + "'")
                  << std::right
                  << "  consumed=" << m.sp << "/" << s.size()
                  << "  steps=" << m.steps;

        // surface any driver-level error (step limit, unknown opcode)
        if (!m.error.empty())
        {
            std::cout << "  (" << m.error << ")";
        }

        std::cout << "\n";
    }

    return 0;
}
