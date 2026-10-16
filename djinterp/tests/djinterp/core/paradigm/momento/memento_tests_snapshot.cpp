/*******************************************************************************
* djinterp [core]                                     memento_tests_snapshot.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_snapshot.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_snapshot.cpp
//   The snapshot strategy policies (section III): deep_copy_snapshot,
// clone_snapshot, serialized_snapshot, delta_snapshot (baseline + delta, and
// the internal delta_record), external_snapshot, and the C++14
// make_external_snapshot factory. Each policy is driven through a full
// capture -> mutate -> restore round-trip.

// std
#include <type_traits>
#include <vector>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_deep_copy_snapshot
  Verifies deep_copy_snapshot captures by copy and restores by assignment.
  Tests the following:
  - snapshot_type_for<State> is the state type itself
  - a captured value restores the pre-mutation state
*/
bool
tests_deep_copy_snapshot()
{
    static_assert(std::is_same<
                      deep_copy_snapshot::snapshot_type_for<int>,
                      int>::value,
                  "snapshot is the state type");

    int s = 5;
    int snap = deep_copy_snapshot::capture(s);
    s = 99;
    deep_copy_snapshot::restore(s, snap);

    return (s == 5);
}

/*
tests_clone_snapshot
  Verifies clone_snapshot captures via clone().
  Tests the following:
  - a captured clone restores the pre-mutation state
*/
bool
tests_clone_snapshot()
{
    clonable_state c(7);
    clonable_state snap = clone_snapshot::capture(c);
    c.value = 1;
    clone_snapshot::restore(c, snap);

    return (c.value == 7);
}

/*
tests_serialized_snapshot
  Verifies serialized_snapshot captures via serialize() and restores via
  deserialize().
  Tests the following:
  - the snapshot is the serialized byte form
  - deserialization restores the pre-mutation state
*/
bool
tests_serialized_snapshot()
{
    serializable_state x(3);
    std::vector<char> snap = serialized_snapshot::capture(x);
    x.value = 1;
    serialized_snapshot::restore(x, snap);

    bool ok = true;

    ok = ok && (snap.size() == 1);
    ok = ok && (x.value == 3);

    return ok;
}

/*
tests_delta_snapshot_baseline
  Verifies delta_snapshot's baseline capture and restore.
  Tests the following:
  - the baseline record is flagged as a baseline
  - restoring from the baseline reconstructs the captured state from a default
*/
bool
tests_delta_snapshot_baseline()
{
    diffable_state a(10);

    delta_snapshot::snapshot_type_for<diffable_state> base =
        delta_snapshot::capture_baseline(a);

    diffable_state restored;
    delta_snapshot::restore_from_baseline(restored, base);

    bool ok = true;

    ok = ok && (base.is_baseline);
    ok = ok && (restored.value == 10);

    return ok;
}

/*
tests_delta_snapshot_delta
  Verifies delta_snapshot's incremental delta capture and application.
  Tests the following:
  - the delta record is not flagged as a baseline
  - applying the delta advances the previous state to the current one
*/
bool
tests_delta_snapshot_delta()
{
    diffable_state previous(10);
    diffable_state current(25);

    delta_snapshot::snapshot_type_for<diffable_state> rec =
        delta_snapshot::capture_delta(previous, current);

    diffable_state state(10);
    delta_snapshot::apply_delta(state, rec);

    bool ok = true;

    ok = ok && (!rec.is_baseline);
    ok = ok && (state.value == 25);

    return ok;
}

/*
tests_delta_record
  Verifies the internal delta_record wrapper directly.
  Tests the following:
  - the default record is not a baseline
  - the explicit record stores the diff and the baseline flag
*/
bool
tests_delta_record()
{
    internal::delta_record<int> def;
    internal::delta_record<int> rec(42, true);

    bool ok = true;

    ok = ok && (!def.is_baseline);
    ok = ok && (rec.diff == 42);
    ok = ok && (rec.is_baseline);

    return ok;
}

/*
tests_external_snapshot
  Verifies external_snapshot delegates capture and restore to user callables.
  Tests the following:
  - capture invokes the user capture callable
  - restore invokes the user restore callable, reconstructing the state
*/
bool
tests_external_snapshot()
{
    struct doubler
    {
        int operator()(const int& _s) const { return _s * 2; }
    };
    struct halver
    {
        void operator()(int& _s, const int& _snap) const { _s = _snap / 2; }
    };

    external_snapshot<doubler, halver> pol{ doubler(), halver() };

    int s = 8;
    int snap = pol.capture(s);
    s = 0;
    pol.restore(s, snap);

    bool ok = true;

    ok = ok && (snap == 16);
    ok = ok && (s == 8);

    return ok;
}

/*
tests_make_external_snapshot
  Verifies the C++14 make_external_snapshot factory.
  Tests the following:
  - the factory builds a working policy from two lambdas that round-trips state
*/
bool
tests_make_external_snapshot()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    auto pol = make_external_snapshot(
        [](const int& _s) { return _s + 100; },
        [](int& _s, const int& _snap) { _s = _snap - 100; });

    int s = 5;
    auto snap = pol.capture(s);
    s = 0;
    pol.restore(s, snap);

    bool ok = true;

    ok = ok && (snap == 105);
    ok = ok && (s == 5);

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
