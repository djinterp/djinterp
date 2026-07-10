// djinterp [test] : mediator_tests_signal_hub.cpp
//   The named-channel multicast signal hub (section VI): connect (and the
// C++17 string_view overload), connect_typed, disconnect / disconnect_all,
// emit and emit_typed, slot_count / slot_count_for, has_channel, and clear.
// Only the string_view overload test body is dialect-gated.
//
//   NOTE: signal_hub::connect is ambiguous for a string LITERAL under C++17 —
// the literal converts equally to const std::string& and to std::string_view
// (BUG 3 in the suite header's notes; the module's own usage example trips
// it).  These tests pass an explicit signal_id(...) to select the std::string
// overload unambiguously, which is what any caller must do until it is fixed.

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_signal_id_type
  Verifies the signal_id alias.
  Tests the following:
  - signal_id is std::string
*/
bool
tests_signal_id_type()
{
    static_assert(std::is_same<signal_id, std::string>::value,
                  "signal_id is std::string");

    return std::is_same<signal_id, std::string>::value;
}

/*
tests_signal_hub_connect_emit
  Verifies a connected handler fires on its channel with the payload.
  Tests the following:
  - connect registers exactly one slot
  - emit on that channel invokes the handler with the type-erased payload
*/
bool
tests_signal_hub_connect_emit()
{
    signal_hub hub;
    int        seen = 0;

    hub.connect(signal_id("chan"),
                [&](const compat::any& _e) { seen = _e.get<int>(); });

    bool ok = (hub.slot_count() == 1);

    hub.emit("chan", compat::any(42));
    ok = ok && (seen == 42);

    return ok;
}

/*
tests_signal_hub_connect_literal
  Verifies connect accepts a bare string literal channel name unambiguously.
  This exercises the const char* overload that resolves the C++17 ambiguity
  between the const std::string& and std::string_view overloads (BUG 3): the
  module's own usage example, hub.connect("name", handler), must compile.
  Tests the following:
  - connect("literal", handler) registers a slot and emit reaches it
*/
bool
tests_signal_hub_connect_literal()
{
    signal_hub hub;
    int        seen = 0;

    hub.connect("literal", [&](const compat::any& _e) { seen = _e.get<int>(); });

    hub.emit("literal", compat::any(64));

    bool ok = true;

    ok = ok && (seen == 64);
    ok = ok && (hub.slot_count_for("literal") == 1);

    return ok;
}

/*
tests_signal_hub_multicast
  Verifies every handler on a channel fires.
  Tests the following:
  - two handlers on the same channel both fire on a single emit
*/
bool
tests_signal_hub_multicast()
{
    signal_hub hub;
    int        a = 0;
    int        b = 0;

    hub.connect(signal_id("chan"), [&](const compat::any&) { ++a; });
    hub.connect(signal_id("chan"), [&](const compat::any&) { ++b; });

    hub.emit("chan", compat::any(0));

    bool ok = true;

    ok = ok && (a == 1);
    ok = ok && (b == 1);
    ok = ok && (hub.slot_count_for("chan") == 2);

    return ok;
}

/*
tests_signal_hub_channels
  Verifies emit reaches only the addressed channel.
  Tests the following:
  - a handler on "a" does not fire when "b" is emitted
  - slot_count_for is per channel
*/
bool
tests_signal_hub_channels()
{
    signal_hub hub;
    int        ca = 0;
    int        cb = 0;

    hub.connect(signal_id("a"), [&](const compat::any&) { ++ca; });
    hub.connect(signal_id("b"), [&](const compat::any&) { ++cb; });

    hub.emit("a", compat::any(0));

    bool ok = true;

    ok = ok && (ca == 1);
    ok = ok && (cb == 0);
    ok = ok && (hub.slot_count_for("a") == 1);
    ok = ok && (hub.slot_count_for("b") == 1);
    ok = ok && (hub.slot_count() == 2);

    return ok;
}

/*
tests_signal_hub_disconnect
  Verifies disconnect removes exactly the slot for a token.
  Tests the following:
  - the removed handler no longer fires
  - a sibling on the same channel continues to fire
  - slot_count drops by one
*/
bool
tests_signal_hub_disconnect()
{
    signal_hub hub;
    int        a = 0;
    int        b = 0;

    subscription_token tok =
        hub.connect(signal_id("chan"), [&](const compat::any&) { ++a; });
    hub.connect(signal_id("chan"), [&](const compat::any&) { ++b; });

    hub.disconnect(tok);

    bool ok = (hub.slot_count() == 1);

    hub.emit("chan", compat::any(0));
    ok = ok && (a == 0);
    ok = ok && (b == 1);

    return ok;
}

/*
tests_signal_hub_disconnect_all
  Verifies disconnect_all clears a whole channel while sparing others.
  Tests the following:
  - every slot on the named channel is removed
  - slots on other channels survive
*/
bool
tests_signal_hub_disconnect_all()
{
    signal_hub hub;
    int        a = 0;
    int        other = 0;

    hub.connect(signal_id("chan"), [&](const compat::any&) { ++a; });
    hub.connect(signal_id("chan"), [&](const compat::any&) { ++a; });
    hub.connect(signal_id("keep"), [&](const compat::any&) { ++other; });

    hub.disconnect_all("chan");

    bool ok = true;

    ok = ok && (hub.slot_count_for("chan") == 0);
    ok = ok && (hub.slot_count_for("keep") == 1);

    hub.emit("chan", compat::any(0));
    ok = ok && (a == 0);

    hub.emit("keep", compat::any(0));
    ok = ok && (other == 1);

    return ok;
}

/*
tests_signal_hub_connect_typed
  Verifies connect_typed extracts the typed event from the any wrapper.
  Tests the following:
  - a typed handler fires with the unwrapped payload for a matching any
  - the same handler is skipped for an any holding a different type
*/
bool
tests_signal_hub_connect_typed()
{
    signal_hub hub;
    int        seen = 0;

    hub.connect_typed<ping>("chan",
                            [&](const ping& _e) { seen += _e.n; });

    hub.emit("chan", compat::any(ping{ 6 }));       // match
    bool ok = (seen == 6);

    hub.emit("chan", compat::any(pong{ 99 }));      // non-match: skipped
    ok = ok && (seen == 6);

    return ok;
}

/*
tests_signal_hub_emit_typed
  Verifies emit_typed wraps a typed event and delivers it on the channel.
  Tests the following:
  - emit_typed<ping> reaches a plain any handler as a wrapped ping
  - it reaches a connect_typed<ping> handler as an unwrapped ping
*/
bool
tests_signal_hub_emit_typed()
{
    signal_hub hub;
    bool       raw_holds_ping = false;
    int        typed_seen = 0;

    hub.connect(signal_id("chan"),
                [&](const compat::any& _e) { raw_holds_ping = _e.holds<ping>(); });
    hub.connect_typed<ping>("chan",
                            [&](const ping& _e) { typed_seen = _e.n; });

    hub.emit_typed<ping>("chan", ping{ 8 });

    bool ok = true;

    ok = ok && (raw_holds_ping);
    ok = ok && (typed_seen == 8);

    return ok;
}

/*
tests_signal_hub_string_view_connect
  Verifies the C++17 std::string_view connect overload.
  Tests the following:
  - connecting via a string_view channel name and emitting by the equivalent
    std::string reaches the handler
*/
bool
tests_signal_hub_string_view_connect()
{
#if D_MEDIATOR_HAS_STRING_VIEW
    signal_hub hub;
    int        seen = 0;

    hub.connect(std::string_view("sv_chan"),
                [&](const compat::any& _e) { seen = _e.get<int>(); });

    hub.emit("sv_chan", compat::any(15));

    bool ok = true;

    ok = ok && (seen == 15);
    ok = ok && (hub.slot_count_for("sv_chan") == 1);

    return ok;
#else
    return true;
#endif
}

/*
tests_signal_hub_has_channel
  Verifies has_channel reports channel occupancy.
  Tests the following:
  - true for a channel with a connected slot
  - false for an unknown channel
  - false again after the channel's slots are removed
*/
bool
tests_signal_hub_has_channel()
{
    signal_hub hub;

    subscription_token tok =
        hub.connect(signal_id("live"), [](const compat::any&) {});

    bool ok = true;

    ok = ok && ( hub.has_channel("live"));
    ok = ok && (!hub.has_channel("absent"));

    hub.disconnect(tok);
    ok = ok && (!hub.has_channel("live"));

    return ok;
}

/*
tests_signal_hub_clear
  Verifies clear removes every slot on every channel.
  Tests the following:
  - slot_count is zero after clear
  - emit after clear invokes nothing
*/
bool
tests_signal_hub_clear()
{
    signal_hub hub;
    int        seen = 0;

    hub.connect(signal_id("a"), [&](const compat::any&) { ++seen; });
    hub.connect(signal_id("b"), [&](const compat::any&) { ++seen; });

    hub.clear();

    bool ok = (hub.slot_count() == 0);

    hub.emit("a", compat::any(0));
    hub.emit("b", compat::any(0));
    ok = ok && (seen == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
