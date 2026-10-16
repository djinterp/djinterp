/*******************************************************************************
* djinterp [tests]                                           test_sync_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_sync.hpp - the test
* coordination primitives (latch, barrier, gate, rendezvous, simultaneous
* start).
*
*   Two-faced per the DTest authoring guide (DTEST_SPEC_MODE).  Blocks map one
* to one onto section TUs:
*     latch         - test_latch (one-shot countdown)
*     barrier       - test_barrier (reusable N-way, optional completion)
*     gate          - test_gate (manual open/close)
*     rendezvous    - test_rendezvous<T> (two-thread payload exchange)
*     simultaneous  - simultaneous_start ("all go on my mark")
*
*
* path:      /tests/djinterp/test/sync/test_sync_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_SYNC_TESTS_
#define DJINTERP_TESTS_TEST_SYNC_TESTS_ 1

// std
#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
// djinterp
// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/test/sync/test_sync.hpp"                          // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_HY_CHECK - unique two-letter suffix (sYnc).  Variadic routed helper.
inline bool
hy_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HY_CHECK(...)                                                        \
    ::djinterp::testing::hy_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// hy_short / hy_settle
//   fixtures: canonical short timeout for negative (timeout) waits and a
// generous budget for positive coordination.  Kept small so the suite runs
// fast while remaining robust on a loaded machine.
inline std::chrono::milliseconds hy_short()  { return std::chrono::milliseconds(20); }
inline std::chrono::seconds      hy_settle() { return std::chrono::seconds(2); }

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   TEST LATCH                      (test_sync_tests_latch.cpp)
bool tests_latch_construct();
bool tests_latch_count_down_single();
bool tests_latch_count_down_amount();
bool tests_latch_reaches_zero_opens();
bool tests_latch_count_down_clamped();
bool tests_latch_wait_returns_when_open();
bool tests_latch_wait_for_timeout();
bool tests_latch_wait_for_success();
bool tests_latch_arrival_count_tracks();
bool tests_latch_arrive_and_wait_single();
bool tests_latch_is_open_alias();
bool tests_latch_blocks_until_zero_mt();
bool tests_latch_arrive_and_wait_mt();

// II.  TEST BARRIER                    (test_sync_tests_barrier.cpp)
bool tests_barrier_construct();
bool tests_barrier_single_way_advances();
bool tests_barrier_reusable();
bool tests_barrier_completion_runs();
bool tests_barrier_arrive_returns_phase();
bool tests_barrier_nway_mt();
bool tests_barrier_completion_once_per_phase_mt();
bool tests_barrier_arrive_and_drop_mt();

// III. TEST GATE                       (test_sync_tests_gate.cpp)
bool tests_gate_starts_closed();
bool tests_gate_open_close();
bool tests_gate_wait_when_open();
bool tests_gate_wait_for_timeout();
bool tests_gate_wait_for_success();
bool tests_gate_pass_count_accumulates();
bool tests_gate_wait_for_no_increment_on_timeout();
bool tests_gate_reclose();
bool tests_gate_blocks_until_open_mt();

// IV.  TEST RENDEZVOUS                 (test_sync_tests_rendezvous.cpp)
bool tests_rendezvous_try_receive_empty();
bool tests_rendezvous_try_receive_null_empty();
bool tests_rendezvous_send_receive_mt();
bool tests_rendezvous_receive_first_mt();
bool tests_rendezvous_try_receive_after_send_mt();
bool tests_rendezvous_payload_string_mt();
bool tests_rendezvous_multiple_exchanges_mt();

// V.   SIMULTANEOUS START              (test_sync_tests_simultaneous.cpp)
bool tests_simultaneous_construct();
bool tests_simultaneous_ready_increments();
bool tests_simultaneous_wait_until_ready_immediate();
bool tests_simultaneous_wait_until_ready_for_timeout();
bool tests_simultaneous_signal_sets_fired();
bool tests_simultaneous_go_after_signal();
bool tests_simultaneous_ready_and_go_after_signal();
bool tests_simultaneous_full_cycle_mt();
bool tests_simultaneous_wait_then_signal_mt();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
sync_spec()
{
    return dt::module_spec{
        "test_sync",
        "Test-coordination primitives: one-shot latch, reusable N-way barrier "
        "with optional completion, manual gate, two-thread rendezvous, and a "
        "simultaneous-start helper.",
        {
            dt::block_spec{
                "latch",
                "test_latch countdown, arrival accounting, clamping, and "
                "waited release.",
                {
                    { "tests_latch_construct",
                      "a fresh latch reports its initial count and is closed.",
                      &tests_latch_construct },
                    { "tests_latch_count_down_single",
                      "count_down() reduces remaining by one and records an "
                      "arrival.",
                      &tests_latch_count_down_single },
                    { "tests_latch_count_down_amount",
                      "count_down(n) reduces remaining by n.",
                      &tests_latch_count_down_amount },
                    { "tests_latch_reaches_zero_opens",
                      "reaching zero opens the latch (try_wait/is_open true).",
                      &tests_latch_reaches_zero_opens },
                    { "tests_latch_count_down_clamped",
                      "count_down past the remaining count is clamped, not "
                      "wrapped.",
                      &tests_latch_count_down_clamped },
                    { "tests_latch_wait_returns_when_open",
                      "wait() returns immediately on an already-open latch.",
                      &tests_latch_wait_returns_when_open },
                    { "tests_latch_wait_for_timeout",
                      "wait_for returns false when the latch stays closed.",
                      &tests_latch_wait_for_timeout },
                    { "tests_latch_wait_for_success",
                      "wait_for returns true once the latch is open.",
                      &tests_latch_wait_for_success },
                    { "tests_latch_arrival_count_tracks",
                      "arrival_count accumulates across count_down calls.",
                      &tests_latch_arrival_count_tracks },
                    { "tests_latch_arrive_and_wait_single",
                      "arrive_and_wait on a 1-count latch opens it and does not "
                      "block.",
                      &tests_latch_arrive_and_wait_single },
                    { "tests_latch_is_open_alias",
                      "is_open mirrors try_wait.",
                      &tests_latch_is_open_alias },
                    { "tests_latch_blocks_until_zero_mt",
                      "a waiter blocks until another thread counts the latch to "
                      "zero.",
                      &tests_latch_blocks_until_zero_mt },
                    { "tests_latch_arrive_and_wait_mt",
                      "N threads arrive_and_wait on an N-count latch and all "
                      "proceed.",
                      &tests_latch_arrive_and_wait_mt },
                }
            },
            dt::block_spec{
                "barrier",
                "test_barrier phase advance, reuse, completion callable, and "
                "arrive_and_drop.",
                {
                    { "tests_barrier_construct",
                      "a fresh barrier reports its expected count, zero "
                      "arrived, phase 0.",
                      &tests_barrier_construct },
                    { "tests_barrier_single_way_advances",
                      "a 1-way barrier releases immediately and advances the "
                      "phase.",
                      &tests_barrier_single_way_advances },
                    { "tests_barrier_reusable",
                      "the barrier is reusable across phases.",
                      &tests_barrier_reusable },
                    { "tests_barrier_completion_runs",
                      "the completion callable runs when the barrier trips.",
                      &tests_barrier_completion_runs },
                    { "tests_barrier_arrive_returns_phase",
                      "arrive_and_wait returns the phase the caller arrived in.",
                      &tests_barrier_arrive_returns_phase },
                    { "tests_barrier_nway_mt",
                      "N threads meet at an N-way barrier and the phase "
                      "advances once.",
                      &tests_barrier_nway_mt },
                    { "tests_barrier_completion_once_per_phase_mt",
                      "the completion runs exactly once per tripped phase.",
                      &tests_barrier_completion_once_per_phase_mt },
                    { "tests_barrier_arrive_and_drop_mt",
                      "arrive_and_drop releases the round and lowers the "
                      "expected count.",
                      &tests_barrier_arrive_and_drop_mt },
                }
            },
            dt::block_spec{
                "gate",
                "test_gate open/close state, waited passage, and pass "
                "accounting.",
                {
                    { "tests_gate_starts_closed",
                      "a fresh gate is closed with pass_count 0.",
                      &tests_gate_starts_closed },
                    { "tests_gate_open_close",
                      "open and close toggle is_open.",
                      &tests_gate_open_close },
                    { "tests_gate_wait_when_open",
                      "wait() passes an open gate and bumps pass_count.",
                      &tests_gate_wait_when_open },
                    { "tests_gate_wait_for_timeout",
                      "wait_for returns false on a closed gate.",
                      &tests_gate_wait_for_timeout },
                    { "tests_gate_wait_for_success",
                      "wait_for returns true on an open gate.",
                      &tests_gate_wait_for_success },
                    { "tests_gate_pass_count_accumulates",
                      "each passage increments pass_count.",
                      &tests_gate_pass_count_accumulates },
                    { "tests_gate_wait_for_no_increment_on_timeout",
                      "a timed-out wait_for does not increment pass_count.",
                      &tests_gate_wait_for_no_increment_on_timeout },
                    { "tests_gate_reclose",
                      "a re-closed gate blocks passage again.",
                      &tests_gate_reclose },
                    { "tests_gate_blocks_until_open_mt",
                      "a waiter blocks on a closed gate until it is opened.",
                      &tests_gate_blocks_until_open_mt },
                }
            },
            dt::block_spec{
                "rendezvous",
                "test_rendezvous<T> hand-off: try_receive, blocking send/"
                "receive, and repeated exchange.",
                {
                    { "tests_rendezvous_try_receive_empty",
                      "try_receive on an empty rendezvous returns false and "
                      "leaves the out slot untouched.",
                      &tests_rendezvous_try_receive_empty },
                    { "tests_rendezvous_try_receive_null_empty",
                      "try_receive tolerates a null out pointer when empty.",
                      &tests_rendezvous_try_receive_null_empty },
                    { "tests_rendezvous_send_receive_mt",
                      "a sent value is delivered to a receiver on another "
                      "thread.",
                      &tests_rendezvous_send_receive_mt },
                    { "tests_rendezvous_receive_first_mt",
                      "a receiver that arrives first blocks until a value is "
                      "sent.",
                      &tests_rendezvous_receive_first_mt },
                    { "tests_rendezvous_try_receive_after_send_mt",
                      "try_receive observes a pending sent value.",
                      &tests_rendezvous_try_receive_after_send_mt },
                    { "tests_rendezvous_payload_string_mt",
                      "a non-trivial payload transfers by value across the "
                      "rendezvous.",
                      &tests_rendezvous_payload_string_mt },
                    { "tests_rendezvous_multiple_exchanges_mt",
                      "successive sends are consumed in order by successive "
                      "receives.",
                      &tests_rendezvous_multiple_exchanges_mt },
                }
            },
            dt::block_spec{
                "simultaneous",
                "simultaneous_start ready accounting, the go signal, and the "
                "coordinated release.",
                {
                    { "tests_simultaneous_construct",
                      "a fresh helper reports its expected count, zero ready, "
                      "not fired.",
                      &tests_simultaneous_construct },
                    { "tests_simultaneous_ready_increments",
                      "ready() increments the ready count.",
                      &tests_simultaneous_ready_increments },
                    { "tests_simultaneous_wait_until_ready_immediate",
                      "wait_until_ready returns once enough threads are ready.",
                      &tests_simultaneous_wait_until_ready_immediate },
                    { "tests_simultaneous_wait_until_ready_for_timeout",
                      "wait_until_ready_for returns false before the quorum is "
                      "reached.",
                      &tests_simultaneous_wait_until_ready_for_timeout },
                    { "tests_simultaneous_signal_sets_fired",
                      "signal sets the fired flag.",
                      &tests_simultaneous_signal_sets_fired },
                    { "tests_simultaneous_go_after_signal",
                      "go returns immediately once signalled.",
                      &tests_simultaneous_go_after_signal },
                    { "tests_simultaneous_ready_and_go_after_signal",
                      "ready_and_go records readiness and returns when already "
                      "signalled.",
                      &tests_simultaneous_ready_and_go_after_signal },
                    { "tests_simultaneous_full_cycle_mt",
                      "N workers wait at go until the coordinator signals, then "
                      "all proceed.",
                      &tests_simultaneous_full_cycle_mt },
                    { "tests_simultaneous_wait_then_signal_mt",
                      "wait_then_signal blocks for the quorum before releasing "
                      "the workers.",
                      &tests_simultaneous_wait_then_signal_mt },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_SYNC_TESTS_
