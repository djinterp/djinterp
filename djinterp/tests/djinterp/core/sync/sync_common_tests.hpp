/*******************************************************************************
* djinterp [test]                                          sync_common_tests.hpp
*
*   Declarations and shared helpers for the sync_common.hpp unit-test suite.
*
*   WHAT THIS HEADER IS, AND WHAT THAT MEANS FOR TESTING:
*   sync_common.hpp holds the foundations the rest of the sync module was
* hand-rolling: ownership bases, a spin backoff, and a scope guard.  Every one
* of them is about a CONTRACT rather than a computation, so most of the suite
* is trait- and effect-based:
*     noncopyable / nonmovable   - bases that state a type's copy/move story;
*     D_NONCOPYABLE / D_NONMOVABLE - the same intent as in-class macros, which
*                                  must agree with the bases exactly;
*     cpu_relax / backoff        - the adaptive spin wait: pause, escalate to
*                                  a yield, reset on progress;
*     scope_guard / make_scope_guard - run-on-exit cleanup, dismissable.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - the bases and the macros must MATCH.  noncopyable keeps move; a bare
*       deleted copy constructor would suppress the implicit move, so the
*       D_NONCOPYABLE macro defaults the move members explicitly.  The suite
*       checks base and macro against each other, in both directions, because
*       a silent divergence here would quietly pin types that meant to move.
*     - the bases are usable ONLY as bases: their special members are
*       protected, so neither is default-constructible as a standalone object.
*       They are also empty, so inheriting one must not grow the derived type
*       (empty base optimization) - a size test guards that.
*     - backoff must remain a pure WAIT, never a synchronization primitive: it
*       is copyable (it holds only a step counter), its operations are
*       noexcept, and pause() must terminate promptly at every step - the
*       escalation to yield is bounded, not an unbounded spin.
*     - scope_guard fires exactly ONCE.  Moving it must transfer the duty, not
*       duplicate it, so the moved-from guard is disarmed; dismiss() cancels
*       it entirely.  Nested guards unwind LIFO, like any destructor.
*
*   VERSIONING: the suite floors at C++11, which is sync_common.hpp's own
* floor (it needs deleted/defaulted members and <thread>).  Nothing in the
* header is version-gated above that, so no test is either.
*
*   PORTABILITY: sync_common.hpp includes ../djinterp.hpp itself, so it is
* self-sufficient and CAN stand first in a TU (the lock_guard / atomic.hpp
* variant, not the live-#error variant).  The suite proves that by including
* it first, alone, in the header section.
*
*
* path:      /tests/djinterp/core/sync/sync_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TEST HELPERS
      -------------------
      a. probe types for the ownership bases and macros
      b. instrumented callable for the scope guard

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. I.   ownership bases and macros
      c.    II.  spin backoff
            d. III. scope guard
*/

#ifndef DJINTERP_TESTS_SYNC_COMMON_
#define DJINTERP_TESTS_SYNC_COMMON_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
//   sync_common.hpp includes the core itself, so it may stand first; the
// header section relies on exactly that.
#include "../../../../inc/djinterp/core/sync/sync_common.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "sync_common_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TEST HELPERS                                  ///
///////////////////////////////////////////////////////////////////////////////

// a.   probe types for the ownership bases and macros
//////////////////////////////////////////

// derived_noncopyable
//   type: inherits the noncopyable base.  Copy must be gone, move must
// survive - the base's whole point.
struct derived_noncopyable : private noncopyable
{
    int value;

    derived_noncopyable()
        : value(0)
    {}
};

// derived_nonmovable
//   type: inherits the nonmovable base.  Neither copy nor move survives.
struct derived_nonmovable : private nonmovable
{
    int value;

    derived_nonmovable()
        : value(0)
    {}
};

// macro_noncopyable
//   type: the macro form of the same intent, so the suite can hold the macro
// and the base against each other.
struct macro_noncopyable
{
    D_NONCOPYABLE(macro_noncopyable)

public:
    int value;

    macro_noncopyable()
        : value(0)
    {}
};

// macro_nonmovable
//   type: the macro form of the pinned contract.
struct macro_nonmovable
{
    D_NONMOVABLE(macro_nonmovable)

public:
    int value;

    macro_nonmovable()
        : value(0)
    {}
};

// plain_probe
//   type: the same payload with NO ownership declaration, as the baseline a
// size comparison is taken against (so the empty-base test measures the base,
// not the payload).
struct plain_probe
{
    int value;

    plain_probe()
        : value(0)
    {}
};


// b.   instrumented callable for the scope guard
//////////////////////////////////////////

// counting_action
//   type: a callable that counts how many times it ran, through a pointer to
// a caller-owned counter.  Copyable, so it can be stored by value in a
// scope_guard, while every copy still reports to the same counter.
struct counting_action
{
    int* counter;

    explicit counting_action(int* _counter)
        : counter(_counter)
    {}

    void operator()() const
    {
        ++(*counter);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_can_stand_first();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_header_defines_no_stray_type_macros();
bool tests_header_publishes_its_ownership_macros();
bool tests_header_is_gated_to_cpp11();

// b.   I.   ownership bases and macros
//////////////////////////////////////////
bool tests_noncopyable_blocks_copying();
bool tests_noncopyable_preserves_moving();
bool tests_nonmovable_blocks_copying();
bool tests_nonmovable_blocks_moving();
bool tests_macro_noncopyable_matches_the_base();
bool tests_macro_nonmovable_matches_the_base();
bool tests_bases_are_empty_and_cost_nothing();
bool tests_bases_are_not_standalone_objects();
bool tests_derived_types_stay_constructible();
bool tests_derived_types_keep_their_destructor();

// c.   II.  spin backoff
//////////////////////////////////////////
bool tests_cpu_relax_is_callable_and_noexcept();
bool tests_backoff_starts_at_the_shortest_spin();
bool tests_backoff_pause_is_noexcept();
bool tests_backoff_escalates_without_hanging();
bool tests_backoff_reset_returns_to_the_start();
bool tests_backoff_is_copyable();
bool tests_backoff_costs_only_a_counter();
bool tests_backoff_survives_a_long_spin_loop();

// d.   III. scope guard
//////////////////////////////////////////
bool tests_scope_guard_runs_on_scope_exit();
bool tests_scope_guard_runs_exactly_once();
bool tests_scope_guard_dismiss_cancels_the_action();
bool tests_scope_guard_moving_transfers_the_duty();
bool tests_scope_guard_is_not_copyable();
bool tests_scope_guard_nests_in_reverse_order();
bool tests_make_scope_guard_deduces_the_callable();
bool tests_scope_guard_accepts_a_function_object();
bool tests_scope_guard_dismiss_is_noexcept();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SYNC_COMMON_
