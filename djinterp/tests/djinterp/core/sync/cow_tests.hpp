/*******************************************************************************
* djinterp [test]                                                  cow_tests.hpp
*
*   Declarations for the cow.hpp unit-test suite, plus the helper types the
* sections share.
*
*   WHY THE CORE HEADER IS INCLUDED FIRST, AND WHY THAT IS A BUG REPORT:
*   cow.hpp cannot be the first djinterp header in a translation unit.  Its
* first include is ./lock_policy.hpp, and lock_policy.hpp opens with
*
*     #ifndef DJINTERP_ENV_ENV_H
*         #error "lock_policy.hpp requires env.h to be included first"
*
* - a live guard - while cow.hpp never includes ../djinterp.hpp, which is what
* would have defined it.  So `#include "cow.hpp"` on its own does not compile.
* condvar.hpp, in the same directory, takes ../djinterp.hpp first and then
* lock_policy.hpp; cow.hpp is missing that one line.  This header therefore
* includes the core explicitly below, and tests_header_cannot_be_included_first
* is the standing note that says why.
*
*   WHAT THE SUITE MEASURES RATHER THAN ASSUMES:
*   Copy-on-write is a claim about WHEN A COPY HAPPENS, so almost nothing here
* can be tested by reading a value back - the value is the same either way.  The
* suite therefore instruments the payload: `tracked` counts every construction,
* copy, move and destruction of a T, so the tests can say exactly what cow.hpp
* did.  That is how "read() never clones", "write() clones exactly once, and
* only when shared", "snapshot() is zero-copy", and "nothing leaks" become
* assertions instead of hopes.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - cow_ptr's read(), write(), operator* and operator-> DEREFERENCE AN
*       UNCHECKED POINTER.  On a default-constructed (empty) cow_ptr they are
*       undefined behaviour, not an exception and not a null return.  The suite
*       exercises the queries that ARE null-safe (use_count, is_unique,
*       operator bool, detach, swap) and never dereferences an empty one - see
*       tests_the_empty_cow_ptr_must_not_be_dereferenced.
*     - cow_state::write() BUMPS THE VERSION BEFORE IT CLONES.  If the clone
*       throws, the exception propagates, the value is unchanged - and the
*       version has still advanced.  A reader that trusts the version now
*       believes in a mutation that never happened
*       (tests_state_write_bumps_the_version_even_when_the_clone_throws).
*     - cow_state::read() and write() return a REFERENCE, and the lock guard
*       that protected it dies on the way out.  The header says "callers must
*       not retain references"; what it does not say is that the reference can
*       be left dangling by a concurrent replace(), which frees the block it
*       points into.  snapshot() is the only safe way to hold state.
*     - cow_ptr<T>::write() and detach() need T to be COPY-constructible even
*       when the pointer is unique and no clone will happen: ensure_unique()'s
*       body is instantiated regardless.  A move-only T works for make(), the
*       readers and the copies - and stops at the first write().
*
*   PORTABILITY: the suite requires C++11, as cow.hpp does (its own C++11 gate
* is commented out, so nothing enforces that - see
* tests_header_gates_are_commented_out).
*
*
* path:      /tests/djinterp/core/sync/cow_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      -------
      a. counters (function-local, so every TU shares one set)
      b. instrumented payloads
      c.    deadlines and spin-waits (concurrency sections)

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. I.   the control block
      c.    II.  cow_ptr - lifetime
            d. II.  cow_ptr - access and the clone
            e. II.  cow_ptr - detach and swap
            f. III. immutable_snapshot
            g. IV.  cow_state - basics
            h. IV.  cow_state - the protocol
      i.    IV.  cow_state - the lock policies
            j. exception safety
            k. concurrency
*/

#ifndef DJINTERP_TESTS_COW_
#define DJINTERP_TESTS_COW_ 1

// std
// std  (NOT <cstdint>, <mutex> or <shared_mutex>: cow.hpp's own include chain
//       supplies those, and tests_header_provides_the_std_facilities only has
//       teeth from a TU that could not have gotten them anywhere else)
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
//   The core comes FIRST, and it has to: cow.hpp includes lock_policy.hpp
// before anything else, and lock_policy.hpp #errors out unless env.h is already
// in.  cow.hpp never brings it in.  Remove the next line and this suite stops
// compiling - which is precisely the bug
// tests_header_cannot_be_included_first records.
#include "djinterp/djinterp.hpp"
#include "../../../../inc/djinterp/core/sync/cow.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "cow_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                             ///
///////////////////////////////////////////////////////////////////////////////

// a.   counters
//////////////////////////////////////////
//   Held in function-local statics inside inline functions, so the header needs
// no out-of-line definition and every section TU shares ONE set of counters.
// Atomic because the concurrency section counts from many threads at once.

// probe_constructions
//   function: how many payloads were built from scratch.
D_INLINE std::atomic<long>&
probe_constructions()
{
    static std::atomic<long> counter(0);

    return counter;
}

// probe_copies
//   function: how many payloads were COPY-constructed.  This is the number cow
// exists to keep small: every clone shows up here, and nothing else should.
D_INLINE std::atomic<long>&
probe_copies()
{
    static std::atomic<long> counter(0);

    return counter;
}

// probe_moves
//   function: how many payloads were MOVE-constructed.
D_INLINE std::atomic<long>&
probe_moves()
{
    static std::atomic<long> counter(0);

    return counter;
}

// probe_destructions
//   function: how many payloads were destroyed.
D_INLINE std::atomic<long>&
probe_destructions()
{
    static std::atomic<long> counter(0);

    return counter;
}

// probe_live
//   function: how many payloads exist RIGHT NOW.  Back to zero at the end of
// every test, or something leaked.
D_INLINE std::atomic<long>&
probe_live()
{
    static std::atomic<long> counter(0);

    return counter;
}

// probe_reset
//   function: zero every counter.  Called at the top of any test that counts.
D_INLINE void
probe_reset()
{
    probe_constructions().store(0);
    probe_copies().store(0);
    probe_moves().store(0);
    probe_destructions().store(0);
    probe_live().store(0);

    return;
}

// probe_armed
//   function: is the throwing payload's copy constructor armed?  A reference,
// so a test can arm and disarm it around the operation it wants to fail.
D_INLINE bool&
probe_armed()
{
    static bool armed = false;

    return armed;
}


// b.   instrumented payloads
//////////////////////////////////////////

// tracked
//   type: a payload that reports its own lifetime.  Copy-on-write is a claim
// about WHEN a copy happens, and a value read back cannot tell you that - so
// the suite counts instead.  Every clone cow.hpp performs is one copy here, and
// every clone it AVOIDS is a copy that does not appear.
struct tracked
{
    int first;
    int second;

    explicit tracked(
        int _first  = 0,
        int _second = 0
    )
        : first(_first),
          second(_second)
    {
        probe_constructions().fetch_add(1);
        probe_live().fetch_add(1);
    }

    tracked(
        const tracked& _other
    )
        : first(_other.first),
          second(_other.second)
    {
        probe_copies().fetch_add(1);
        probe_live().fetch_add(1);
    }

    tracked(
        tracked&& _other
    ) D_NOEXCEPT
        : first(_other.first),
          second(_other.second)
    {
        probe_moves().fetch_add(1);
        probe_live().fetch_add(1);
    }

    tracked& operator=(const tracked&) = default;

    ~tracked()
    {
        probe_destructions().fetch_add(1);
        probe_live().fetch_sub(1);
    }
};

// throwing_copy
//   type: a payload whose COPY constructor throws while probe_armed() is set.
// Every clone in cow.hpp goes through a copy, so this is the lever that makes
// ensure_unique(), detach(), make() and replace() fail exactly where a test
// wants them to.
//
//   It declares a copy constructor and therefore has NO move constructor: the
// header's _Type&& control-block path falls back to this copy, which is what
// makes it usable in the cow_state constructors too.
struct throwing_copy
{
    int value;

    explicit throwing_copy(
        int _value = 0
    )
        : value(_value)
    {
        probe_constructions().fetch_add(1);
        probe_live().fetch_add(1);
    }

    throwing_copy(
        const throwing_copy& _other
    )
        : value(_other.value)
    {
        // check if this copy is the one the test wants to fail
        if (probe_armed())
        {
            throw copy_failed();
        }

        probe_copies().fetch_add(1);
        probe_live().fetch_add(1);
    }

    ~throwing_copy()
    {
        probe_destructions().fetch_add(1);
        probe_live().fetch_sub(1);
    }

    // copy_failed
    //   type: what a throwing copy throws.  Its own type, so a test can prove
    // the exception came from where it thinks it did.
    struct copy_failed
    {};
};

// move_only
//   type: a payload that cannot be copied.  cow_ptr's readers and copies work
// with it; write() and detach() do not, because ensure_unique()'s body is
// instantiated whether or not the pointer is shared, and its body copies.  The
// suite never instantiates those on this type - the boundary is the finding.
struct move_only
{
    int value;

    explicit move_only(
        int _value = 0
    )
        : value(_value)
    {}

    move_only(const move_only&)            = delete;
    move_only& operator=(const move_only&) = delete;

    move_only(
        move_only&& _other
    ) D_NOEXCEPT
        : value(_other.value)
    {
        _other.value = 0;
    }

    move_only& operator=(
        move_only&& _other
    ) D_NOEXCEPT
    {
        value        = _other.value;
        _other.value = 0;

        return *this;
    }
};


// c.   deadlines and spin-waits
//////////////////////////////////////////

// test_clock
//   type: the suite's clock - steady, never the wall clock.
typedef std::chrono::steady_clock test_clock;

// k_deadline_ms
//   constant: how long a concurrency test waits for something that MUST happen
// before it calls the operation broken.  A liveness bound, not a performance
// one.
D_STATIC D_CONSTEXPR unsigned k_deadline_ms = 3000;

// k_agents
//   constant: how many threads a concurrency test starts.
D_STATIC D_CONSTEXPR int k_agents = 4;

// k_writes
//   constant: how many mutations a concurrent writer performs.
D_STATIC D_CONSTEXPR int k_writes = 500;

// sleep_ms
//   function: park this thread for _ms milliseconds.
D_INLINE void
sleep_ms(
    unsigned _ms
)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));

    return;
}

// elapsed_ms
//   function: milliseconds since _start.
D_INLINE long long
elapsed_ms(
    const test_clock::time_point& _start
)
{
    return static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            test_clock::now() - _start).count());
}

// spin_until_count
//   function: wait for a counter to reach _target, up to _ms.  Returns false if
// the deadline passed first - which is how a stalled thread becomes a FAILED
// test rather than a hung runner.  It sleeps between polls: a hot spin on a
// single-core box can starve the very thread it waits for.
D_INLINE bool
spin_until_count(
    const std::atomic<int>& _counter,
    int                     _target,
    unsigned                _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (_counter.load() < _target)
    {
        // check if the deadline has passed
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_cannot_be_included_first();
bool tests_header_pulls_its_dependencies();
bool tests_header_takes_lock_guard_but_never_uses_it();
bool tests_header_gates_are_commented_out();
bool tests_header_provides_the_std_facilities();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_header_defines_no_stray_macros();

// b.   I.  the control block
//////////////////////////////////////////
bool tests_control_block_starts_at_one();
bool tests_control_block_add_ref();
bool tests_control_block_release_reports_the_last_reference();
bool tests_control_block_is_unique();
bool tests_control_block_refcount_ops_are_noexcept();
bool tests_control_block_holds_the_value_inline();
bool tests_control_block_constructs_the_value_in_place();
bool tests_control_block_copies_an_lvalue();
bool tests_control_block_moves_an_rvalue();
bool tests_control_block_lives_in_internal();

// c.   II. cow_ptr - lifetime
//////////////////////////////////////////
bool tests_cow_ptr_default_is_empty();
bool tests_cow_ptr_make_starts_unique();
bool tests_cow_ptr_make_constructs_in_place();
bool tests_cow_ptr_make_copies_an_lvalue();
bool tests_cow_ptr_make_moves_an_rvalue();
bool tests_cow_ptr_copy_shares_the_block();
bool tests_cow_ptr_move_steals_the_block();
bool tests_cow_ptr_copy_assignment_shares();
bool tests_cow_ptr_move_assignment_steals();
bool tests_cow_ptr_self_assignment_is_safe();
bool tests_cow_ptr_assigning_an_aliased_block_is_safe();
bool tests_cow_ptr_destructor_releases_the_last_reference();
bool tests_cow_ptr_empty_operations_are_safe();
bool tests_cow_ptr_lifetime_operations_are_noexcept();

// d.   II. cow_ptr - access and the clone
//////////////////////////////////////////
bool tests_cow_ptr_read_returns_the_value();
bool tests_cow_ptr_arrow_and_star_reach_the_value();
bool tests_cow_ptr_const_access_never_clones();
bool tests_cow_ptr_write_when_unique_does_not_clone();
bool tests_cow_ptr_write_when_shared_clones();
bool tests_cow_ptr_write_leaves_the_other_holder_untouched();
bool tests_cow_ptr_write_clones_exactly_once();
bool tests_cow_ptr_write_copies_the_value_exactly_once();
bool tests_cow_ptr_is_unique_and_use_count();
bool tests_cow_ptr_operator_bool();
bool tests_the_empty_cow_ptr_must_not_be_dereferenced();
bool tests_cow_ptr_carries_a_move_only_payload();

// e.   II. cow_ptr - detach and swap
//////////////////////////////////////////
bool tests_detach_clones_even_when_unique();
bool tests_detach_copies_the_value_exactly_once();
bool tests_detach_when_shared_leaves_the_others_alone();
bool tests_detach_on_an_empty_pointer_is_a_no_op();
bool tests_member_swap_exchanges_the_blocks();
bool tests_free_swap_is_found_by_adl();
bool tests_swap_touches_no_refcounts();
bool tests_swap_is_noexcept();

// f.   III. immutable_snapshot
//////////////////////////////////////////
bool tests_snapshot_from_a_value_copies_it();
bool tests_snapshot_from_a_cow_ptr_is_zero_copy();
bool tests_snapshot_version_defaults_to_zero();
bool tests_snapshot_carries_the_version_it_was_given();
bool tests_snapshot_get_arrow_and_star();
bool tests_snapshot_exposes_only_const_access();
bool tests_snapshot_copy_shares_the_block();
bool tests_snapshot_move_steals_the_block();
bool tests_snapshot_is_not_default_constructible();
bool tests_snapshot_outlives_its_source();
bool tests_snapshot_from_an_empty_cow_ptr_is_falsy();
bool tests_snapshot_use_count();

// g.   IV. cow_state - basics
//////////////////////////////////////////
bool tests_state_default_constructs_its_value();
bool tests_state_constructs_from_an_lvalue();
bool tests_state_constructs_from_an_rvalue();
bool tests_state_is_not_copyable();
bool tests_state_is_not_movable();
bool tests_state_exposes_its_policy_aliases();
bool tests_state_marks_itself_with_cow_state_type();
bool tests_state_declares_the_cow_strategy_tag();
bool tests_state_read_returns_the_value();
bool tests_state_version_starts_at_zero();
bool tests_state_exposes_its_mutex();
bool tests_state_defaults_to_the_default_policy();

// h.   IV. cow_state - the protocol
//////////////////////////////////////////
bool tests_state_write_mutates_and_bumps_the_version();
bool tests_state_write_without_a_snapshot_does_not_clone();
bool tests_state_snapshot_shares_the_block();
bool tests_state_write_after_a_snapshot_clones();
bool tests_state_snapshot_is_isolated_from_later_writes();
bool tests_state_snapshot_pins_its_version();
bool tests_state_modify_runs_under_the_write_lock();
bool tests_state_modify_returns_the_callables_result();
bool tests_state_modify_accepts_a_void_callable();
bool tests_state_modify_bumps_the_version();
bool tests_state_replace_swaps_the_whole_value();
bool tests_state_replace_leaves_old_snapshots_valid();
bool tests_state_read_and_write_return_unguarded_references();

// i.   IV. cow_state - the lock policies
//////////////////////////////////////////
bool tests_state_under_the_null_policy();
bool tests_state_under_the_exclusive_policy();
bool tests_state_under_the_timed_policy();
bool tests_state_under_the_shared_policy();
bool tests_state_under_the_shared_timed_policy();
bool tests_state_write_excludes_other_agents();
bool tests_state_null_policy_costs_no_mutex();
bool tests_every_policy_yields_a_usable_state();

// j.   exception safety
//////////////////////////////////////////
bool tests_make_propagates_a_throwing_constructor();
bool tests_a_failed_make_leaks_nothing();
bool tests_write_on_a_shared_pointer_is_strongly_safe();
bool tests_detach_is_strongly_safe();
bool tests_state_replace_is_strongly_safe();
bool tests_state_write_bumps_the_version_even_when_the_clone_throws();
bool tests_state_modify_bumps_the_version_even_when_the_callable_throws();
bool tests_state_modify_releases_the_lock_when_the_callable_throws();
bool tests_a_snapshot_survives_a_failed_write();

// k.   concurrency
//////////////////////////////////////////
bool tests_concurrent_copies_keep_the_refcount_exact();
bool tests_concurrent_snapshots_never_see_a_torn_value();
bool tests_a_held_snapshot_is_stable_under_writes();
bool tests_concurrent_writers_serialize();
bool tests_concurrent_readers_and_writers_leak_nothing();
bool tests_the_shared_policy_admits_concurrent_readers();
bool tests_shared_policy_read_after_snapshot_is_unlocked();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_COW_
