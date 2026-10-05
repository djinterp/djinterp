/*******************************************************************************
* djinterp [parsegen]                                                machine.hpp
*
*   The operator-agnostic substrate for the vparse virtual parsing machine: the
* shared run-state (`machine`), the type-erased operator and its per-family
* registry (`op_set`), and the C++20 `Voperator` concept.  The substrate knows
* no opcode and no family; operator families (peg, lr, gen) attach private
* per-run state -- including their cursor, where they have one -- through
* `machine::ext`.  Runtime dispatch is deliberately not lifted to compile time
* (carrier.hpp D5): the meta floor is used where structure is static, not on the
* VM core.
*
*
* path:      /inc/djinterp/parsegen/vparse/machine.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_MACHINE_HPP
#define DJINTERP_PARSEGEN_VPARSE_MACHINE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../parsegen.hpp"  // NS_PARSEGEN

#ifndef D_KEYWORD_VPARSE
    #define D_KEYWORD_VPARSE            vparse
#endif
#ifndef NS_VPARSE
    #define NS_VPARSE                   D_NAMESPACE(D_KEYWORD_VPARSE)
#endif

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// machine
//   struct: the operator-agnostic run-state -- dispatch/run control only.  The
// cursor lives in a family's private state (peg threads a parse_state), not
// here, which is what keeps the substrate agnostic.
struct machine
{
    using input_type = char;

    long                      steps      = 0;
    bool                      halted     = false;
    bool                      ok         = false;
    std::string               error;
    std::vector<std::string>* trace      = nullptr;
    long                      step_limit = 100000;
    void*                     ext        = nullptr;   // active family's state
};

// voperator
//   type: a type-erased parsing operator -- an effect on the machine.
using voperator = std::function<void(machine&)>;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Voperator
    //   concept: any type invocable as void(machine&).  PascalCase per the
    // project's concept naming convention (cf. carrier.hpp's Carrier).
    template<typename Op>
    concept Voperator = requires(Op _op, machine& _m)
    {
        _op(_m);
    };

#endif

// op_set
//   struct: a per-family registry mapping an opcode to its voperator and a
// display name.  The engine dispatches purely through this map, which is what
// makes the machine operator-agnostic; opcode spaces are family-private.
struct op_set
{
    std::unordered_map<int, voperator>   handlers;
    std::unordered_map<int, std::string> names;

    // def
    //   function: register voperator `_fn` under opcode `_code`.
    void def(int _code, std::string _name, voperator _fn)
    {
        handlers[_code] = std::move(_fn);
        names[_code]    = std::move(_name);
        return;
    }

    // find
    //   accessor: the voperator for `_code`, or nullptr.
    const voperator* find(int _code) const
    {
        auto it = handlers.find(_code);
        return (it == handlers.end()) ? nullptr : &it->second;
    }

    // name
    //   accessor: the display name for `_code`, or "?".
    std::string name(int _code) const
    {
        auto it = names.find(_code);
        return (it == names.end()) ? std::string("?") : it->second;
    }
};

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_MACHINE_HPP
