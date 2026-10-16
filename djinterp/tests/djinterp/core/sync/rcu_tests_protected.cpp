/*******************************************************************************
* djinterp [core]                                        rcu_tests_protected.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_protected.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    rcu_tests_protected.cpp
//   Section VI: rcu_protected<T> - the complete RCU value, wiring a deferred
// reclaimer to an atomic data pointer and exposing read / update / modify.  A
// writer's update() copies-swaps-retires; modify() applies a function to a copy
// and publishes it; both advance the epoch and grow the pending list (the old
// value is retired, not freed on the spot).  A reader takes a read_lock() and
// calls read() under it.
//
//   TWO version facts shape this section.  First, read_lock() returns a
// non-movable rcu_read_guard BY VALUE, so it is well-formed only at C++17+; the
// read-side tests are therefore gated to C++17 and degrade to a stated no-op
// below it, while the writer-side and query tests run from the C++11 floor.
// Second, rcu_protected holds a SINGLE reader slot, so its read API is single-
// reader by construction - which is why the concurrent safety test lives in the
// concurrency section and uses per-thread slots directly, not rcu_protected.

// djinterp
#include "rcu_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_protected_constructs_from_an_lvalue
  an rcu_protected can be built from an existing value.
  Tests the following:
  - after construction from an lvalue, current_epoch() and pending_reclamation()
    are both 0 (nothing retired yet)
  - at C++17+, a read under a lock returns the initial value
*/
bool
tests_protected_constructs_from_an_lvalue()
{
    int                initial = 42;
    rcu_protected<int> value(initial);

    bool ok = ( (value.current_epoch() == 0) &&
                (value.pending_reclamation() == 0) );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    rcu_protected<int>::rcu_read_guard guard = value.read_lock();
    ok = ok && (value.read(guard) == 42);
#endif

    return ok;
}

/*
tests_protected_constructs_from_an_rvalue
  an rcu_protected can be built by moving a value in.
  Tests the following:
  - after construction from an rvalue, current_epoch() and pending_reclamation()
    are both 0
  - at C++17+, a read returns the moved-in value
*/
bool
tests_protected_constructs_from_an_rvalue()
{
    rcu_protected<int> value(int(7));

    bool ok = ( (value.current_epoch() == 0) &&
                (value.pending_reclamation() == 0) );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    rcu_protected<int>::rcu_read_guard guard = value.read_lock();
    ok = ok && (value.read(guard) == 7);
#endif

    return ok;
}

/*
tests_protected_update_advances_the_epoch
  publishing a new value opens a new epoch.
  Tests the following:
  - current_epoch() is 0 before update() and 1 after
  update() retires the old value, and a retirement advances the epoch.
*/
bool
tests_protected_update_advances_the_epoch()
{
    rcu_protected<int> value(1);

    const bool before = (value.current_epoch() == 0);
    value.update(2);
    const bool after = (value.current_epoch() == 1);

    return (before && after);
}

/*
tests_protected_update_retires_the_old_value
  the replaced value goes onto the pending list.
  Tests the following:
  - pending_reclamation() is 0 before update() and 1 after (the old value is
    retired for deferred reclamation, not freed immediately)
  The default reader pool is 64, so the auto-scan threshold (128) is far away
  and the single retirement simply accumulates.
*/
bool
tests_protected_update_retires_the_old_value()
{
    rcu_protected<int> value(1);

    const bool before = (value.pending_reclamation() == 0);
    value.update(2);
    const bool after = (value.pending_reclamation() == 1);

    return (before && after);
}

/*
tests_protected_modify_advances_and_retires
  modify() publishes a transformed copy and retires the original.
  Tests the following:
  - after one modify(), current_epoch() == 1 and pending_reclamation() == 1
  modify() is read-copy-apply-swap-retire, so like update() it opens an epoch
  and retires the prior value.
*/
bool
tests_protected_modify_advances_and_retires()
{
    rcu_protected<int> value(10);

    value.modify([](int& x){ x += 5; });

    return ( (value.current_epoch() == 1) &&
             (value.pending_reclamation() == 1) );
}

/*
tests_protected_updates_accumulate_pending
  successive writes pile up retired values until a scan.
  Tests the following:
  - five updates leave current_epoch() == 5 and pending_reclamation() == 5
  Each write retires one old value; below the auto-scan threshold they simply
  accumulate, one per epoch.
*/
bool
tests_protected_updates_accumulate_pending()
{
    rcu_protected<int> value(0);

    for (int i = 1; i <= 5; ++i)
    {
        value.update(i);
    }

    return ( (value.current_epoch() == 5) &&
             (value.pending_reclamation() == 5) );
}

/*
tests_protected_is_not_copyable
  an RCU value is a unique owner, not a value type.
  Tests the following:
  - rcu_protected<int> is not copy-constructible or copy-assignable (both
    deleted)
  It owns the live data pointer, a reader slot and a reclaimer; copying would
  alias all three.
*/
bool
tests_protected_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<rcu_protected<int> >::value &&
          !std::is_copy_assignable<rcu_protected<int> >::value );

    static_assert(no_copy, "rcu_protected must not be copyable");

    return no_copy;
}

/*
tests_protected_type_aliases_identify_the_strategy
  rcu_protected advertises itself for trait-based dispatch.
  Tests the following:
  - rcu_protected_type is rcu_protected<T> itself (the self-marker containers
    forward so has_rcu_protected_type reports true)
  - concurrency_strategy_tag is rcu_strategy_tag (the tag-alias fast path read
    by the strategy traits)
*/
bool
tests_protected_type_aliases_identify_the_strategy()
{
    D_CONSTEXPR bool self_marker =
        std::is_same<rcu_protected<int>::rcu_protected_type,
                     rcu_protected<int> >::value;

    D_CONSTEXPR bool tagged =
        std::is_same<rcu_protected<int>::concurrency_strategy_tag,
                     rcu_strategy_tag>::value;

    static_assert(self_marker && tagged,
                  "rcu_protected must identify itself as RCU strategy");

    return (self_marker && tagged);
}

/*
tests_protected_read_guard_is_not_copyable_or_movable
  the reader token cannot be copied or moved - the root of the C++17 note.
  Tests the following:
  - rcu_protected<int>::rcu_read_guard is not copy-constructible
  - it is not move-constructible either: its move ctor is declared `= default`,
    but that default is DELETED because the epoch_guard it holds is non-movable
  This is why read_lock(), which returns the guard by value, needs C++17's
  guaranteed copy elision - see the header section.
*/
bool
tests_protected_read_guard_is_not_copyable_or_movable()
{
    typedef rcu_protected<int>::rcu_read_guard guard_type;

    D_CONSTEXPR bool pinned =
        ( !std::is_copy_constructible<guard_type>::value &&
          !std::is_move_constructible<guard_type>::value );

    static_assert(pinned,
                  "rcu_read_guard must be neither copyable nor movable");

    return pinned;
}

/*
tests_protected_read_returns_the_current_value
  a read under a lock yields the published value.  [C++17+]
  Tests the following:
  - at C++17+, read() under a read_lock() returns the value the object was built
    with
  Gated because read_lock() is only well-formed at C++17+ (it returns a non-
  movable guard by value); below C++17 this degrades to a stated no-op.
*/
bool
tests_protected_read_returns_the_current_value()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    rcu_protected<int>                 value(42);
    rcu_protected<int>::rcu_read_guard guard = value.read_lock();

    return (value.read(guard) == 42);
#else
    return true;
#endif
}

/*
tests_protected_updates_are_visible_to_readers
  a reader sees each newly published value.  [C++17+]
  Tests the following:
  - at C++17+, after update(20) a read returns 20, and after a further
    update(30) a read returns 30
  Gated for the same read_lock() reason as above.
*/
bool
tests_protected_updates_are_visible_to_readers()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    rcu_protected<int> value(10);

    value.update(20);
    {
        rcu_protected<int>::rcu_read_guard guard = value.read_lock();
        if (value.read(guard) != 20)
        {
            return false;
        }
    }

    value.update(30);
    {
        rcu_protected<int>::rcu_read_guard guard = value.read_lock();
        if (value.read(guard) != 30)
        {
            return false;
        }
    }

    return true;
#else
    return true;
#endif
}

/*
tests_protected_modify_is_visible_to_readers
  a modify's transformation is visible to a subsequent read.  [C++17+]
  Tests the following:
  - at C++17+, after modify(+23) on an initial 100, a read returns 123
  Gated for the same read_lock() reason as above.
*/
bool
tests_protected_modify_is_visible_to_readers()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    rcu_protected<int> value(100);

    value.modify([](int& x){ x += 23; });

    rcu_protected<int>::rcu_read_guard guard = value.read_lock();

    return (value.read(guard) == 123);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
