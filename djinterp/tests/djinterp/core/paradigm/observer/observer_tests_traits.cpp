/*******************************************************************************
* djinterp [core]                                      observer_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/paradigm/observer/observer_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : observer_tests_traits.cpp
//   Capability traits (section 6): the fine-grained SFINAE detectors, the
// public _v aliases, and the composite identity traits (is_observable,
// is_delegate, is_event, is_observer, is_bounded, has_connection_tracking,
// observable_arity). Each is exercised against one instance of every tier and a
// non-observable control type, cross-checking the discriminations the traits
// are meant to draw.

// std
#include <cstddef>
#include <functional>
// djinterp
#include "observer_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace ot  = ::djinterp::observer_traits;

namespace
{
    // one representative of each tier, plus a non-observable control.
    using del_t = delegate<void(int)>;
    using evt_t = event<void(int, int), 4>;
    using obs_t = observer<void(int)>;
    using non_t = int;
}


/*
tests_trait_has_connect
  Verifies has_connect detects a .connect(callable) method.
  Tests the following:
  - true for every tier, false for a non-observable type
*/
bool
tests_trait_has_connect()
{
    static_assert( ot::has_connect_v<del_t>, "delegate connects");
    static_assert(!ot::has_connect_v<non_t>, "int does not");

    return ( ot::has_connect_v<del_t> && ot::has_connect_v<evt_t> &&
             ot::has_connect_v<obs_t> && !ot::has_connect_v<non_t> );
}

/*
tests_trait_has_notify
  Verifies has_notify detects a .notify member.
  Tests the following:
  - true for every tier, false for a non-observable type
*/
bool
tests_trait_has_notify()
{
    return ( ot::has_notify_v<del_t> && ot::has_notify_v<evt_t> &&
             ot::has_notify_v<obs_t> && !ot::has_notify_v<non_t> );
}

/*
tests_trait_has_disconnect_all
  Verifies has_disconnect_all detects a .disconnect_all() method.
  Tests the following:
  - true for every tier, false for a non-observable type
*/
bool
tests_trait_has_disconnect_all()
{
    return ( ot::has_disconnect_all_v<del_t> && ot::has_disconnect_all_v<evt_t> &&
             ot::has_disconnect_all_v<obs_t> && !ot::has_disconnect_all_v<non_t> );
}

/*
tests_trait_has_count
  Verifies has_count detects a .count() method.
  Tests the following:
  - true for every tier, false for a non-observable type
*/
bool
tests_trait_has_count()
{
    return ( ot::has_count_v<del_t> && ot::has_count_v<evt_t> &&
             ot::has_count_v<obs_t> && !ot::has_count_v<non_t> );
}

/*
tests_trait_has_compact
  Verifies has_compact detects a .compact() method.
  Tests the following:
  - false for delegate (no compaction)
  - true for event and observer (both defragment)
*/
bool
tests_trait_has_compact()
{
    bool ok = true;

    ok = ok && (!ot::has_compact_v<del_t>);
    ok = ok && ( ot::has_compact_v<evt_t>);
    ok = ok && ( ot::has_compact_v<obs_t>);

    return ok;
}

/*
tests_trait_type_alias_detectors
  Verifies the signature/callable/return type-alias detectors.
  Tests the following:
  - all three aliases are detected on every tier
  - none are detected on a non-observable type
*/
bool
tests_trait_type_alias_detectors()
{
    bool ok = true;

    ok = ok && ( ot::has_signature_type_v<del_t>);
    ok = ok && ( ot::has_callable_type_v<evt_t>);
    ok = ok && ( ot::has_return_type_v<obs_t>);

    ok = ok && (!ot::has_signature_type_v<non_t>);
    ok = ok && (!ot::has_callable_type_v<non_t>);
    ok = ok && (!ot::has_return_type_v<non_t>);

    return ok;
}

/*
tests_trait_has_static_capacity
  Verifies has_static_capacity detects a ::capacity member.
  Tests the following:
  - true for every tier (delegate 1, event N, observer 0 all expose it)
  - false for a non-observable type
*/
bool
tests_trait_has_static_capacity()
{
    return ( ot::has_static_capacity_v<del_t> && ot::has_static_capacity_v<evt_t> &&
             ot::has_static_capacity_v<obs_t> && !ot::has_static_capacity_v<non_t> );
}

/*
tests_trait_disconnect_detectors
  Verifies the disconnect-shape detectors.
  Tests the following:
  - has_disconnect_void is true only for delegate (argumentless disconnect)
  - has_disconnect_by_id is true only for event (disconnect by slot_id)
*/
bool
tests_trait_disconnect_detectors()
{
    bool ok = true;

    ok = ok && ( ot::has_disconnect_void_v<del_t>);
    ok = ok && (!ot::has_disconnect_void_v<evt_t>);
    ok = ok && (!ot::has_disconnect_void_v<obs_t>);

    ok = ok && (!ot::has_disconnect_by_id_v<del_t>);
    ok = ok && ( ot::has_disconnect_by_id_v<evt_t>);
    ok = ok && (!ot::has_disconnect_by_id_v<obs_t>);

    return ok;
}

/*
tests_trait_connect_return_detectors
  Verifies the connect-return-type detectors.
  Tests the following:
  - connect_returns_void is true only for delegate
  - connect_returns_slot_id is true only for event
  - connect_returns_connection is true only for observer
*/
bool
tests_trait_connect_return_detectors()
{
    bool ok = true;

    ok = ok && ( ot::connect_returns_void_v<del_t>);
    ok = ok && (!ot::connect_returns_void_v<evt_t>);
    ok = ok && (!ot::connect_returns_void_v<obs_t>);

    ok = ok && ( ot::connect_returns_slot_id_v<evt_t>);
    ok = ok && (!ot::connect_returns_slot_id_v<del_t>);
    ok = ok && (!ot::connect_returns_slot_id_v<obs_t>);

    ok = ok && ( ot::connect_returns_connection_v<obs_t>);
    ok = ok && (!ot::connect_returns_connection_v<del_t>);
    ok = ok && (!ot::connect_returns_connection_v<evt_t>);

    return ok;
}

/*
tests_trait_is_observable
  Verifies the is_observable composite (connect + notify + count).
  Tests the following:
  - true for every tier, false for a non-observable type
  - the _v alias mirrors the struct trait
*/
bool
tests_trait_is_observable()
{
    static_assert( ot::is_observable<del_t>::value, "delegate observable");
    static_assert(!ot::is_observable<non_t>::value, "int not observable");

    bool ok = true;

    ok = ok && ( ot::is_observable_v<del_t> && ot::is_observable_v<evt_t> &&
                 ot::is_observable_v<obs_t> && !ot::is_observable_v<non_t> );
    ok = ok && (ot::is_observable_v<obs_t> == ot::is_observable<obs_t>::value);

    return ok;
}

/*
tests_trait_is_delegate
  Verifies the is_delegate composite.
  Tests the following:
  - true only for delegate (single-slot, void-returning connect)
*/
bool
tests_trait_is_delegate()
{
    bool ok = true;

    ok = ok && ( ot::is_delegate_v<del_t>);
    ok = ok && (!ot::is_delegate_v<evt_t>);
    ok = ok && (!ot::is_delegate_v<obs_t>);
    ok = ok && (!ot::is_delegate_v<non_t>);

    return ok;
}

/*
tests_trait_is_event
  Verifies the is_event composite.
  Tests the following:
  - true only for event (static capacity, slot_id-returning connect)
*/
bool
tests_trait_is_event()
{
    bool ok = true;

    ok = ok && ( ot::is_event_v<evt_t>);
    ok = ok && (!ot::is_event_v<del_t>);
    ok = ok && (!ot::is_event_v<obs_t>);
    ok = ok && (!ot::is_event_v<non_t>);

    return ok;
}

/*
tests_trait_is_observer
  Verifies the is_observer composite.
  Tests the following:
  - true only for observer (connection-returning connect, has compact)
  - notably false for event even though event also has compact
*/
bool
tests_trait_is_observer()
{
    bool ok = true;

    ok = ok && ( ot::is_observer_v<obs_t>);
    ok = ok && (!ot::is_observer_v<evt_t>);   // has compact, but connect != connection
    ok = ok && (!ot::is_observer_v<del_t>);
    ok = ok && (!ot::is_observer_v<non_t>);

    return ok;
}

/*
tests_trait_is_bounded
  Verifies the is_bounded composite (non-zero static capacity).
  Tests the following:
  - true for delegate (capacity 1) and event (capacity N)
  - false for observer (capacity 0) and non-observable types
*/
bool
tests_trait_is_bounded()
{
    bool ok = true;

    ok = ok && ( ot::is_bounded_v<del_t>);
    ok = ok && ( ot::is_bounded_v<evt_t>);
    ok = ok && (!ot::is_bounded_v<obs_t>);
    ok = ok && (!ot::is_bounded_v<non_t>);

    return ok;
}

/*
tests_trait_has_connection_tracking
  Verifies has_connection_tracking (connection-returning connect).
  Tests the following:
  - true only for observer
*/
bool
tests_trait_has_connection_tracking()
{
    bool ok = true;

    ok = ok && ( ot::has_connection_tracking_v<obs_t>);
    ok = ok && (!ot::has_connection_tracking_v<del_t>);
    ok = ok && (!ot::has_connection_tracking_v<evt_t>);

    return ok;
}

/*
tests_trait_observable_arity
  Verifies observable_arity extracts a signature's argument count.
  Tests the following:
  - delegate<void(int)> reports arity 1
  - event<void(int,int),4> reports arity 2
  - a non-observable type reports arity 0 (the primary-template default)
*/
bool
tests_trait_observable_arity()
{
    static_assert(ot::observable_arity_v<del_t> == 1, "delegate arity");
    static_assert(ot::observable_arity_v<evt_t> == 2, "event arity");

    bool ok = true;

    ok = ok && (ot::observable_arity_v<del_t> == 1);
    ok = ok && (ot::observable_arity_v<evt_t> == 2);
    ok = ok && (ot::observable_arity_v<non_t> == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
