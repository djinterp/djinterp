/*******************************************************************************
* djinterp [parse]                                                  machine.hpp
*
*   The C++ face of the execution substrate declared in machine.h.
*   `machine` derives from d_parse_machine and `op_set` from d_parse_op_set,
* neither adds a data member, and both are asserted layout-identical to their
* base -- so a C++ driver hands its machine to a C operator, or a C driver
* dispatches into a C++ handler, with nothing in between.
*
*   The registration surface is where the C++ side earns its place.  A
* voperator is a plain function pointer plus a context pointer, which is what
* keeps the registry a POD and callable from either language; writing one by
* hand means spelling out the C signature and casting the context back.  The
* def() overloads below take an ordinary callable instead and generate that
* trampoline as a template, so a stateless handler registers as a lambda over
* `machine&` and still costs exactly one indirect call to a function that
* inlines its body.  No std::function, no allocation, no type erasure beyond
* the function pointer the C ABI already required.
*
*   ONE CAST, DOCUMENTED.  The trampolines receive d_parse_machine* and hand
* the handler a machine&.  That is valid because machine is standard-layout
* and adds no member, making it pointer-interconvertible with its base
* subobject -- but only for an object that really is a `machine`.  Every
* machine the C++ layer creates is one; a machine created in C is not, so
* def_raw() exists for handlers that must serve those.  as_machine() is the
* single place the conversion happens.
*
* path:      /inc/djinterp/parse/machine.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE MACHINE
    -----------
    1.  The run-state
         1.  machine
    2.  Conversion
         1.  as_machine
2.  OPERATORS
    ---------
    1.  The operator
         1.  voperator
         2.  op
    2.  The operator protocol
         1.  Voperator
         2.  StatelessVoperator
    3.  Trampolines
         1.  stateless_thunk
         2.  free_thunk
         3.  object_thunk
         4.  state_thunk
3.  THE REGISTRY
    ------------
    1.  The registry
         1.  op_set
    2.  Self-contained storage
         1.  fixed_op_set
    3.  Layout guarantees
*/

#ifndef DJINTERP_PARSE_MACHINE_HPP_
#define DJINTERP_PARSE_MACHINE_HPP_ 1

// std
#include <cstdint>              // std::uint32_t
#include <type_traits>          // std::is_standard_layout, std::is_empty,
                                // std::is_invocable_v
// djinterp
#include "../djinterp.hpp"      // framework root
#include "./diagnostic.hpp"     // parse::diagnostics, parse::span, NS_PARSE
#include "./machine.h"          // the C substrate this layer faces


NS_DJINTERP
NS_PARSE


//==============================================================================
// 1.  THE MACHINE
//==============================================================================


// 1.1    The run-state
//------------------------------------------------------------------------------
// 1.1.1
// machine
//   struct: the shared run-state.  Public-payload layout inherited from the C
// struct -- the substrate IS the protocol the operators manipulate -- so
// handlers read and write `ext`, `halted`, and `ok` directly.
struct machine : d_parse_machine
{
    // machine
    //   constructor: a fresh, un-halted run reporting into no sink.
    machine() noexcept
    {
        d_parse_machine_init(this, nullptr);
    }

    // machine
    //   constructor: a fresh, un-halted run reporting into a sink.
    explicit machine(
        d_parse_diag_sink& _diag
    ) noexcept
    {
        d_parse_machine_init(this, &_diag);
    }

    // reset
    //   function: returns the machine to a fresh run against the same sink.
    void
    reset() noexcept
    {
        d_parse_machine_init(this, diag);
    }

    // halt
    //   function: stops the run and records its outcome.
    void
    halt(
        bool _ok
    ) noexcept
    {
        d_parse_machine_halt(this, _ok ? 1 : 0);
    }

    // fail
    //   function: stops the run as failed and reports why through the sink.
    void
    fail(
        std::uint16_t _domain,
        std::uint16_t _code,
        const span&   _span,
        const char*   _message
    ) noexcept
    {
        d_parse_machine_fail(this, _domain, _code, _span, _message);
    }

    // step
    //   function: charges one dispatch against the step budget, returning
    // false once the run is halted or the budget is spent.
    bool
    step() noexcept
    {
        return (d_parse_machine_step(this) != 0);
    }

    // running
    //   accessor: whether the run may continue.
    constexpr bool
    running() const noexcept
    {
        return (halted == 0);
    }

    // succeeded
    //   accessor: whether the run halted successfully.  Meaningful only once
    // the run has halted.
    constexpr bool
    succeeded() const noexcept
    {
        return ( (halted != 0) &&
                 (ok != 0) );
    }

    // extension
    //   accessor: the active family's private state, typed.  The driver that
    // set `ext` is the only code that knows this type.
    template<typename _State>
    _State*
    extension() const noexcept
    {
        return static_cast<_State*>(ext);
    }

    // bind
    //   function: points `ext` at the family state for this run.
    template<typename _State>
    void
    bind(
        _State& _state
    ) noexcept
    {
        ext = static_cast<void*>(&_state);

        return;
    }
};


// 1.2    Conversion
//------------------------------------------------------------------------------
// 1.2.1
// as_machine
//   function: views a C machine as its C++ face.  The single sanctioned
// conversion point; see the banner for the guarantee it rests on and the
// condition under which it holds.
inline machine&
as_machine(
    d_parse_machine& _raw
) noexcept
{
    return static_cast<machine&>(_raw);
}


//==============================================================================
// 2.  OPERATORS
//==============================================================================


// 2.1    The operator
//------------------------------------------------------------------------------
// 2.1.1
// voperator
//   type: the C-ABI operator signature -- an effect on the machine, taking its
// context pointer.  Registered handlers are always this; the def() overloads
// below produce one from friendlier shapes.
using voperator = ::d_parse_voperator;

// 2.1.2
// op
//   type: one registered operator -- what to call, what to pass it, and what
// to call it in a trace.
using op = ::d_parse_op;


// 2.2    The operator protocol
//------------------------------------------------------------------------------
#if defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                               \
    (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1)

    // 2.2.1
    // Voperator
    //   concept: names the operator protocol -- anything invocable as
    // `void(machine&)`.  PascalCase per the project's concept convention.
    template<typename _Op>
    concept Voperator = requires(_Op _op, machine& _machine)
    {
        _op(_machine);
    };

    // 2.2.2
    // StatelessVoperator
    //   concept: a Voperator that carries nothing -- a captureless lambda or
    // an empty functor.  These register with no context pointer at all, which
    // is the case that reduces to a direct call after inlining.
    template<typename _Op>
    concept StatelessVoperator = ( Voperator<_Op>                  &&
                                   std::is_empty_v<_Op>            &&
                                   std::is_default_constructible_v<_Op> );

    #define D_INTERNAL_PARSE_VOP          Voperator
    #define D_INTERNAL_PARSE_VOP_STATELESS      StatelessVoperator

#else

    #define D_INTERNAL_PARSE_VOP          typename
    #define D_INTERNAL_PARSE_VOP_STATELESS      typename

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// 2.3    Trampolines
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.3.1
    // stateless_thunk
    //   function: the C-ABI shim for a callable that carries nothing.  The
    // context slot is unused and the functor is constructed on the spot, so
    // an optimiser sees straight through to the handler body.
    template<typename _Fn>
    void
    stateless_thunk(
        d_parse_machine* _machine,
        void*            _ctx
    )
    {
        (void)_ctx;

        _Fn{}(as_machine(*_machine));

        return;
    }

    // 2.3.2
    // free_thunk
    //   function: the C-ABI shim for a free function taking only the machine.
    // The function is a template argument, so the shim is a direct call with
    // an unused context slot.
    template<auto _Fn>
    void
    free_thunk(
        d_parse_machine* _machine,
        void*            _ctx
    )
    {
        (void)_ctx;

        _Fn(as_machine(*_machine));

        return;
    }

    // 2.3.3
    // object_thunk
    //   function: the C-ABI shim for a callable the caller owns and keeps
    // alive, reached through the context slot.
    template<typename _Obj>
    void
    object_thunk(
        d_parse_machine* _machine,
        void*            _ctx
    )
    {
        (*static_cast<_Obj*>(_ctx))(as_machine(*_machine));

        return;
    }

    // 2.3.4
    // state_thunk
    //   function: the C-ABI shim for a free function taking the machine and
    // its family's state.  The function is a template argument, so the call is
    // direct and only the state travels through the context slot.
    template<auto     _Fn,
             typename _State>
    void
    state_thunk(
        d_parse_machine* _machine,
        void*            _ctx
    )
    {
        _Fn(as_machine(*_machine), *static_cast<_State*>(_ctx));

        return;
    }

NS_END  // internal


//==============================================================================
// 3.  THE REGISTRY
//==============================================================================


// 3.1    The registry
//------------------------------------------------------------------------------
// 3.1.1
// op_set
//   class: a family's opcode-to-operator table, with lifetime.  Derives from
// the C registry and adds no data member, so `&ops` is a d_parse_op_set* the
// moment a C driver asks for one.  Move-only for the same reason the sink is.
class op_set : public d_parse_op_set
{
public:
    // op_set
    //   constructor: an empty registry over one opcode space, owning nothing.
    explicit op_set(
        std::uint16_t _family = 0u
    ) noexcept
    {
        d_parse_op_set_init(this, _family, nullptr, 0u);
    }

    // op_set
    //   constructor: a registry over a caller-supplied table.
    op_set(
        std::uint16_t _family,
        d_parse_op*   _ops,
        std::uint32_t _capacity
    ) noexcept
    {
        d_parse_op_set_init(this, _family, _ops, _capacity);
    }

    op_set(const op_set&)            = delete;
    op_set& operator=(const op_set&) = delete;

    // op_set
    //   constructor: takes over another registry's table and ownership.
    op_set(
        op_set&& _other
    ) noexcept
        : d_parse_op_set(_other)
    {
        d_parse_op_set_init(&_other, _other.family, nullptr, 0u);
    }

    // operator=
    //   function: releases this registry, then takes over another's.
    op_set&
    operator=(
        op_set&& _other
    ) noexcept
    {
        // guard against self-move, which would release the table being taken
        if (this != &_other)
        {
            d_parse_op_set_release(this);

            static_cast<d_parse_op_set&>(*this) = _other;

            d_parse_op_set_init(&_other, _other.family, nullptr, 0u);
        }

        return *this;
    }

    // ~op_set
    //   destructor: releases any table this registry owns.
    ~op_set() noexcept
    {
        d_parse_op_set_release(this);
    }

#if (D_INTERNAL_PARSE_OP_SET_HEAP == 1)
    // reserve
    //   function: replaces this registry's table with one it allocates and
    // owns, which then grows on demand.  Returns false if refused.
    D_NODISCARD bool
    reserve(
        std::uint32_t _capacity = 0u
    ) noexcept
    {
        const std::uint16_t space = family;

        d_parse_op_set_release(this);

        return (d_parse_op_set_init_heap(this, space, _capacity) == 0);
    }
#endif  // D_INTERNAL_PARSE_OP_SET_HEAP

    // def_raw
    //   function: registers a C-ABI operator directly.  The form to use for a
    // handler that must also serve machines created outside C++.
    D_NODISCARD bool
    def_raw(
        int         _code,
        const char* _name,
        voperator   _fn,
        void*       _ctx = nullptr
    ) noexcept
    {
        return (d_parse_op_set_def(this, _code, _name, _fn, _ctx) == 0);
    }

    // def
    //   function: registers a callable that carries nothing -- a captureless
    // lambda or an empty functor.  No context pointer is stored and the
    // trampoline inlines away.
    template<D_INTERNAL_PARSE_VOP_STATELESS _Fn>
    D_NODISCARD bool
    def(
        int         _code,
        const char* _name,
        _Fn
    ) noexcept
    {
        static_assert(std::is_empty<_Fn>::value,
                      "this def() takes a callable that captures nothing; use "
                      "def_object for one that owns state, or def_raw for a "
                      "plain C-ABI operator");

        return def_raw(_code, _name, &internal::stateless_thunk<_Fn>, nullptr);
    }

    // def
    //   function: registers a free function taking only the machine -- the
    // shape most handlers written in C++ take.  The function is a template
    // argument, so the dispatch is one indirect call to a direct call.
    template<auto _Fn>
    D_NODISCARD bool
    def(
        int         _code,
        const char* _name
    ) noexcept
    {
        static_assert(std::is_invocable_v<decltype(_Fn), machine&>,
                      "this def() takes a function callable as (machine&); "
                      "use def_state for one that also takes family state");

        return def_raw(_code, _name, &internal::free_thunk<_Fn>, nullptr);
    }

    // def_object
    //   function: registers a callable the caller owns.  The object must
    // outlive every dispatch through this registry.
    template<D_INTERNAL_PARSE_VOP _Obj>
    D_NODISCARD bool
    def_object(
        int         _code,
        const char* _name,
        _Obj&       _object
    ) noexcept
    {
        return def_raw(_code,
                       _name,
                       &internal::object_thunk<_Obj>,
                       static_cast<void*>(&_object));
    }

    // def_state
    //   function: registers a free function taking the machine and its
    // family's state.  The function is known at compile time, so the dispatch
    // is one indirect call to a direct call.
    template<auto _Fn,
             typename _State>
    D_NODISCARD bool
    def_state(
        int         _code,
        const char* _name,
        _State&     _state
    ) noexcept
    {
        static_assert(std::is_invocable_v<decltype(_Fn), machine&, _State&>,
                      "def_state takes a function callable as "
                      "(machine&, _State&)");

        return def_raw(_code,
                       _name,
                       &internal::state_thunk<_Fn, _State>,
                       static_cast<void*>(&_state));
    }

    // find
    //   accessor: the operator registered for an opcode, or null.
    const op*
    find(
        int _code
    ) const noexcept
    {
        return d_parse_op_set_find(this, _code);
    }

    // name_of
    //   accessor: the display name registered for an opcode, or "?".
    const char*
    name_of(
        int _code
    ) const noexcept
    {
        return d_parse_op_set_name(this, _code);
    }

    // dispatch
    //   function: charges a step, then runs the operator for an opcode.  An
    // unregistered opcode halts the run and reports it.
    bool
    dispatch(
        machine& _machine,
        int      _code
    ) const noexcept
    {
        return (d_parse_op_set_dispatch(&_machine, this, _code) != 0);
    }

    // covers
    //   accessor: whether this registry implements every opcode another one
    // defines.  The gate that keeps a second reading of one opcode space
    // honest -- an interpreter and a compiler over the same program, say.
    bool
    covers(
        const d_parse_op_set& _reference,
        int*                  _missing = nullptr
    ) const noexcept
    {
        return (d_parse_op_set_covers(this, &_reference, _missing) != 0);
    }

    // size
    //   accessor: one past the highest opcode ever defined.
    constexpr std::uint32_t
    size() const noexcept
    {
        return count;
    }

    // space
    //   accessor: which private opcode space this registry reads.
    constexpr std::uint16_t
    space() const noexcept
    {
        return family;
    }
};


// 3.2    Self-contained storage
//------------------------------------------------------------------------------
// 3.2.1
// fixed_op_set
//   class: a registry carrying its own table, for a family whose opcode space
// is known at compile time -- which is every family, since an opcode space is
// an enum.  Allocates nothing.
template<std::uint32_t _Capacity>
class fixed_op_set : public op_set
{
public:
    // fixed_op_set
    //   constructor: binds the embedded table as this registry's storage.
    explicit fixed_op_set(
        std::uint16_t _family = 0u
    ) noexcept
    {
        d_parse_op_set_init(this, _family, m_ops, _Capacity);
    }

private:
    d_parse_op m_ops[_Capacity];
};


// 3.3    Layout guarantees
//------------------------------------------------------------------------------
//   The claim this header makes is that its types cost nothing over the C
// ones.  These assertions are that claim, checked.
static_assert(sizeof(machine) == sizeof(d_parse_machine),
              "parse::machine must add no data member");
static_assert(alignof(machine) == alignof(d_parse_machine),
              "parse::machine must add no data member");
static_assert(std::is_standard_layout<machine>::value,
              "parse::machine must remain standard-layout, since the "
              "trampolines rest on pointer-interconvertibility with its base");
static_assert(sizeof(op_set) == sizeof(d_parse_op_set),
              "parse::op_set must add no data member");
static_assert(alignof(op_set) == alignof(d_parse_op_set),
              "parse::op_set must add no data member");
static_assert(std::is_standard_layout<op_set>::value,
              "parse::op_set must remain standard-layout");


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_MACHINE_HPP_
