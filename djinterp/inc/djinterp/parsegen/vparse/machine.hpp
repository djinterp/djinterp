/******************************************************************************
* djinterp [vparse]                                                machine.hpp
*
*   The operator-agnostic substrate for the vparse virtual parsing machine.
* Defines the shared run-state (`machine`), the type-erased operator and its
* per-family registry (`op_set`), and the C++20 `Voperator` concept naming the
* operator protocol.  The substrate knows nothing about any opcode or parsing
* family: operator families (peg, lr) live in their own modules and attach
* private per-run state through `machine::ext`.
*
*   The runtime dispatch here is deliberately NOT lifted into compile-time
* metaprogramming -- voperators are type-erased callables resolved at run time
* through `op_set`.  Per the framework's "keep leaves domain-specific" stance
* (carrier.hpp, D5), the meta floor is used where it fits (house conventions,
* the parser-contract typedefs, the concept face) and not forced onto the VM
* core, where dispatch is genuinely a runtime concern.
*
* path:      /inc/djinterp/parse/parsegen/vparse/machine.hpp
* link(s):   TBA
* author(s): vparse                                        created: 2026.06.19
******************************************************************************/

#ifndef DJINTERP_PARSE_PARSEGEN_VPARSE_MACHINE_
#define DJINTERP_PARSE_PARSEGEN_VPARSE_MACHINE_ 1

// std
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
// djinterp
#include "../parsegen.hpp"


NS_DJINTERP
NS_PARSEGEN


// ===========================================================================
// I.   machine  (the shared substrate)
// ===========================================================================

// machine
//   struct: the operator-agnostic run-state shared by every driver and every
// operator family.  Holds only the common substrate (input cursor, step
// counter, halt/result, optional trace) plus `ext`, an opaque pointer a driver
// aims at its family's private state for the duration of a run.  Field layout
// follows fixed_string's public-payload precedent -- the substrate IS the
// protocol the operators manipulate -- so these members are unprefixed.
struct machine
{
    // input_type
    //   type: element of the subject stream (parser / scanner contract; cf.
    // member_types.hpp has_input_type).
    using input_type  = char;

    // result_type
    //   type: value produced on success (parser / scanner contract; cf.
    // member_types.hpp has_result_type).
    using result_type = bool;

    // ---- shared substrate ----
    const std::string*        input      = nullptr;   // subject text
    int                       sp         = 0;         // char cursor
    long                      steps      = 0;         // operators dispatched
    bool                      halted     = false;
    bool                      ok         = false;
    std::string               error;
    std::vector<std::string>* trace      = nullptr;   // optional, per-step
    long                      step_limit = 100000;    // runaway guard

    // ext
    //   member: opaque handle to the active family's private state; set by a
    // driver before its run loop and read only by that family's operators.
    void*                     ext        = nullptr;
};


// ===========================================================================
// II.  voperator  +  the Voperator protocol
// ===========================================================================

// voperator
//   type: a type-erased parsing operator -- an effect on the machine.  Stored
// in `op_set` and dispatched by a driver.  Any value invocable as
// `void(machine&)` (free function, function pointer, lambda) is a voperator;
// see the Voperator concept below.
using voperator = std::function<void(machine&)>;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Voperator
    //   concept: names the operator protocol -- any type invocable as
    // `void(machine&)`.  PascalCase per the project's concept naming
    // convention (cf. carrier.hpp's Carrier).  The registry stores voperators
    // type-erased; this concept is the face for constraining generic code
    // that PRODUCES them.
    template<typename _Op>
    concept Voperator = requires(_Op _op, machine& _m)
    {
        _op(_m);
    };

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// ===========================================================================
// III. op_set  (the per-family operator registry)
// ===========================================================================

// op_set
//   struct: a per-family registry mapping an opcode to its voperator (and a
// display name).  Each operator family owns one; the engine dispatches purely
// through this map and never names an opcode, which is what makes the machine
// operator-agnostic.  Opcode spaces are therefore family-private and cannot
// collide across families.
struct op_set
{
    std::unordered_map<int, voperator>   handlers;
    std::unordered_map<int, std::string> names;

    // def
    //   function: register voperator `_fn` under opcode `_code`, with display
    // name `_name`.
    void
    def(
        int         _code,
        std::string _name,
        voperator   _fn
    )
    {
        handlers[_code] = std::move(_fn);
        names[_code]    = std::move(_name);

        return;
    }

    // find
    //   accessor: the voperator registered for `_code`, or nullptr if none.
    const voperator*
    find(
        int _code
    ) const
    {
        auto it = handlers.find(_code);

        return (it == handlers.end())
               ? nullptr
               : &it->second;
    }

    // name
    //   accessor: the display name registered for `_code`, or "?" if none.
    std::string
    name(
        int _code
    ) const
    {
        auto it = names.find(_code);

        return (it == names.end())
               ? std::string("?")
               : it->second;
    }
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PARSEGEN_VPARSE_MACHINE_