/*******************************************************************************
* djinterp [cpp/parsegen]                                            session.hpp
*
* A grammar session: text in, a runnable parser out.
*   This is the facade the vparse workbench (djinterp/tools/vparse) drives,
* and what a headless caller drives to get the same behaviour without a GUI.
* A session holds one grammar through its whole life -- source text, the
* program it compiled to, and whatever went wrong -- so a caller can show all
* three without tracking any of it. Both frontends land on the same
* peg::program: the metacircular notation (notation.hpp) and EBNF (ebnf.hpp).
*   NOTHING HERE THROWS. A grammar arrives from a text box and is wrong most
* of the time it is typed; a bad one is an ordinary result, reported through
* compile_result, not an exception. That matches how the frontends themselves
* report (notation::parse_grammar's `ok`, ebnf::parse's `ok`).
*   A session refuses the same faults in either notation, in the same words:
* text with no rule in it, a rule defined twice, a reference to a rule
* nothing defines. EBNF's frontend reports the last two itself; the
* metacircular notation's generator looks for neither, and would compile a
* reference to nothing into a call that never returns, so that check is made
* here.
*
* path:      /inc/djinterp/cpp/modules/parsegen/vparse/session.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.15
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_SESSION_HPP
#define DJINTERP_PARSEGEN_VPARSE_SESSION_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error, as
// the vparse headers it stands on are.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
// djinterp
#include "../../../djinterp.hpp"  // framework root
#include "./peg.hpp"           // peg::program


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// frontend
//   enum: which frontend reads the grammar text -- the metacircular notation,
// described in its own formalism (notation.hpp), or EBNF, with grouping, `?`,
// quoted literals, `.` and `#` comments (ebnf.hpp).
enum class frontend
{
    metacircular,
    ebnf
};

// compile_result
//   struct: what came of a load(). `error` is empty when `ok` is true, and
// `instructions` is the size of the compiled program -- the one number that
// says whether a grammar rewrite made the parser smaller.
struct compile_result
{
    bool        ok           = false;
    std::string error;
    std::size_t instructions = 0u;
};

// DMatchResult
//   struct: what came of running the loaded parser over one input.
struct DMatchResult
{
    bool        ok       = false;
    std::size_t consumed = 0u;
};

// session
//   class: one grammar, from text to a parser that runs.
class session
{
public:
    session() = default;

    /**
     * @brief Reads grammar text with the chosen frontend and compiles it.
     *
     * @note A grammar that defines no rule, defines one twice, or refers to
     *       one it does not define is refused, with either frontend.
     * @note Left recursion is not refused: such a grammar loads, and a
     *       match over it fails when the machine's step limit stops it.
     *
     * @param[in] _text      the grammar source.
     * @param[in] _frontend  which frontend reads it.
     * @post   On failure the session is as it was: the parser a previous
     *         load produced, and its source, are still in place.
     * @return whether it compiled, why not if it did not, and the size of
     *         the program if it did. A malformed grammar is this result,
     *         never an exception.
     */
    D_NODISCARD compile_result load(
        const std::string& _text,
        frontend           _frontend = frontend::ebnf
    );

    /**
     * @brief Runs the loaded parser over one input.
     *
     * @param[in] _input  the subject text.
     * @return whether the whole input matched, and the offset the run ended
     *         at: the input's length on a match and zero otherwise, since a
     *         failed run leaves the offset where it began. A session with no
     *         grammar loaded reports a failure.
     */
    D_NODISCARD DMatchResult match(
        const std::string& _input
    ) const;

    // what is loaded -- none of these can fail, and none changes the
    // session. The listing is one instruction per line, each prefixed with
    // its address; it is empty, and size() is zero, until a load succeeds.
    std::string disassembly() const;
    bool ready() const;
    std::size_t size() const;
    const std::string& source() const;

private:
    peg::program m_program;
    std::string  m_source;
    bool         m_ready = false;
};

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_SESSION_HPP
