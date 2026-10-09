/*******************************************************************************
* djinterp [parsegen]                                        vparse_tests_lr.cpp
*
* vparse tests: the LR family, over a hand-built SLR(1) table for a
* left-recursive grammar -- the one the PEG family cannot run.
*   The table is the standalone prototype's arithmetic demo:
*     0: E -> E + T    1: E -> T    2: T -> n
* with semantic actions that sum.
*
*
* path:      /tests/djinterp/parsegen/vparse/vparse_tests_lr.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <string>  // std::string
#include <vector>  // std::vector


namespace djinterp
{
namespace testing
{

namespace
{

// token kinds
//   enum: the demo's terminals. The lr module knows none of them: a kind is
// a plain int, supplied by the table and by whatever lexes the input.
enum
{
    TOK_N = 0,
    TOK_PLUS,
    TOK_END
};

// nonterminals
//   enum: the demo's left-hand sides, which the goto table is indexed by.
enum
{
    NT_E = 0,
    NT_T
};

// lex
//   function: digits and '+' into tokens; false on any other character.
bool
lex(
    const std::string& _text,
    vp::lr::tokens&    _out
)
{
    _out.clear();

    for (const char c : _text)
    {
        if ( (c >= '0') &&
             (c <= '9') )
        {
            _out.push_back({ TOK_N, c - '0' });
        }
        else if (c == '+')
        {
            _out.push_back({ TOK_PLUS, 0 });
        }
        else
        {
            return false;
        }
    }

    _out.push_back({ TOK_END, 0 });

    return true;
}

// sum
//   function: the table's semantic action -- production 0 adds, the others
// pass their one value through.
long
sum(
    int                      _production,
    const std::vector<long>& _values
)
{
    return (_production == 0) ? (_values[0] + _values[2]) : _values[0];
}

// arith_table
//   function: the SLR(1) table for E -> E + T | T ;  T -> n.
vp::lr::table
arith_table()
{
    using namespace vp::lr;

    table t;

    t.productions = { { NT_E, 3 }, { NT_E, 1 }, { NT_T, 1 } };
    t.semantic    = &sum;

    t.actions[{ 0, TOK_N    }] = { SHIFT,  3 };
    t.actions[{ 1, TOK_PLUS }] = { SHIFT,  4 };
    t.actions[{ 1, TOK_END  }] = { ACCEPT, 0 };
    t.actions[{ 2, TOK_PLUS }] = { REDUCE, 1 };
    t.actions[{ 2, TOK_END  }] = { REDUCE, 1 };
    t.actions[{ 3, TOK_PLUS }] = { REDUCE, 2 };
    t.actions[{ 3, TOK_END  }] = { REDUCE, 2 };
    t.actions[{ 4, TOK_N    }] = { SHIFT,  3 };
    t.actions[{ 5, TOK_PLUS }] = { REDUCE, 0 };
    t.actions[{ 5, TOK_END  }] = { REDUCE, 0 };

    t.gotos[{ 0, NT_E }] = 1;
    t.gotos[{ 0, NT_T }] = 2;
    t.gotos[{ 4, NT_T }] = 5;

    return t;
}

// run_arith
//   function: lexes one input and runs it against the table.
vp::lr::result
run_arith(
    const std::string& _text,
    vp::machine&       _machine,
    const vp::op_set&  _ops
)
{
    vp::lr::tokens tokens;

    if (!lex(_text, tokens))
    {
        return vp::lr::result{ false, 0, "lex", 0 };
    }

    return vp::lr::run(_machine, arith_table(), tokens, _ops);
}

// accepts
//   function: whether an input is accepted and reduces to an expected value.
bool
accepts(
    const std::string& _text,
    long               _value,
    const vp::op_set&  _ops
)
{
    vp::machine          machine;
    const vp::lr::result outcome = run_arith(_text, machine, _ops);

    return ( (outcome.ok)               &&
             (outcome.value == _value)  );
}

}  // namespace

/*
tests_lr_left_recursive
  The LR family on a grammar the PEG family cannot run.
  Tests the following:
  - E -> E + T | T, left-recursive, is accepted
  - its semantic actions compute the sum
  - what the grammar does not derive is refused
*/
bool
tests_lr_left_recursive()
{
    const vp::op_set ops = vp::lr::make_ops();

    if ( (!accepts("1+2+3", 6, ops))  ||
         (!accepts("2+2", 4, ops))    ||
         (!accepts("7", 7, ops))      )
    {
        return false;
    }

    // each of these stops in a state with no action for the token it holds
    for (const char* text : { "1+", "+1", "", "1++2" })
    {
        vp::machine machine;

        if (run_arith(text, machine, ops).ok)
        {
            return false;
        }
    }

    return true;
}

/*
tests_lr_trace_and_limits
  What a caller's machine sees of an LR run.
  Tests the following:
  - the driver traces every action, the last of them the accept
  - the step limit stops a run exactly where it says
  - a registry missing an operator is reported, not dereferenced
*/
bool
tests_lr_trace_and_limits()
{
    const vp::op_set         ops     = vp::lr::make_ops();
    vp::op_set               partial = vp::lr::make_ops();
    std::vector<std::string> trace;
    vp::machine              traced;
    vp::machine              limited;
    vp::machine              missing;

    traced.trace       = &trace;
    limited.step_limit = 2;
    partial.handlers.erase(vp::lr::REDUCE);

    const vp::lr::result whole = run_arith("1+2+3", traced, ops);

    // one line per action; the accept halts before it is counted as a step
    if ( (!whole.ok)                                              ||
         (static_cast<long>(trace.size()) != (whole.steps + 1))   ||
         (trace.back().find("ACCEPT") == std::string::npos)       )
    {
        return false;
    }

    const vp::lr::result stopped = run_arith("1+2+3", limited, ops);

    // the budget stops the run exactly where it says
    if ( (stopped.ok)           ||
         (stopped.steps != 2)   )
    {
        return false;
    }

    // a valid input, against a registry with no REDUCE
    return (!run_arith("1+2", missing, partial).ok);
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
