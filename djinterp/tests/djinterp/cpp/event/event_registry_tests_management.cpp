/*******************************************************************************
* djinterp [test]                            event_registry_tests_management.cpp
*
*   Section III (management) -- the delegated lifecycle surface the registry
* forwards to its underlying DEventTable, plus the registry merge, the typed
* and aggregate queries, and table access.  Covers unbind (removal reflected in
* contains and the counts); enable/disable (mask flips reflected in is_enabled,
* enabled_count, and dispatch); is_enabled/contains on present/absent/disabled
* ids; merge (pointwise concatenation re-keyed into this registry, source left
* intact, empty registry as identity); the per-event typed queries
* handler_count_for / has_handlers_for / clear_for; the aggregate queries
* handler_count / enabled_count / type_count and clear; and table(), whose
* mutable and const overloads expose the same underlying store reflecting the
* registry's state.
*
*
* path:      /tests/djinterp/cpp/event/event_registry_tests_management.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "event_registry_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_unbind
bool tests_unbind()
{
    bool ok = true;

    DEventRegistry reg;
    DHandlerId a = reg.bind<DEvInt>(DSumming{0});
    DHandlerId b = reg.bind<DEvInt>(DSumming{0});

    // unbinding a present handler removes it and drops the count.
    ok = D_ER_CHECK(reg.unbind(a)) && ok;
    ok = D_ER_CHECK(!reg.contains(a)) && ok;
    ok = D_ER_CHECK(reg.contains(b)) && ok;
    ok = D_ER_CHECK(reg.handler_count() == 1u) && ok;

    // unbinding an unknown / already-removed id returns false.
    ok = D_ER_CHECK(!reg.unbind(a)) && ok;

    return ok;
}


// tests_enable_disable
bool tests_enable_disable()
{
    bool ok = true;

    DEventRegistry reg;
    std::vector<int> log;
    DHandlerId a = reg.bind<DEvInt>(DUnaryRec{&log, 1, verdict::pass});

    // disable flips the mask (reflected in is_enabled and enabled_count) and
    // removes the handler from dispatch.
    ok = D_ER_CHECK(reg.disable(a)) && ok;
    ok = D_ER_CHECK(!reg.is_enabled(a)) && ok;
    ok = D_ER_CHECK(reg.enabled_count() == 0u) && ok;
    DDispatchResult r = reg.dispatch<DEvInt>(0);
    ok = D_ER_CHECK(r.invoked == 0u) && ok;

    // a second disable is a no-op reported false.
    ok = D_ER_CHECK(!reg.disable(a)) && ok;

    // enable restores it.
    ok = D_ER_CHECK(reg.enable(a)) && ok;
    ok = D_ER_CHECK(reg.is_enabled(a)) && ok;
    ok = D_ER_CHECK(reg.enabled_count() == 1u) && ok;
    ok = D_ER_CHECK(!reg.enable(a)) && ok;   // already enabled

    return ok;
}


// tests_is_enabled_contains
bool tests_is_enabled_contains()
{
    bool ok = true;

    DEventRegistry reg;
    DHandlerId a = reg.bind<DEvInt>(DSumming{0});

    // present + enabled.
    ok = D_ER_CHECK(reg.contains(a)) && ok;
    ok = D_ER_CHECK(reg.is_enabled(a)) && ok;

    // an unknown id is neither contained nor enabled.
    DHandlerId bogus;
    bogus.value = 4242;
    ok = D_ER_CHECK(!reg.contains(bogus)) && ok;
    ok = D_ER_CHECK(!reg.is_enabled(bogus)) && ok;

    // a disabled handler is still contained but not enabled.
    reg.disable(a);
    ok = D_ER_CHECK(reg.contains(a)) && ok;
    ok = D_ER_CHECK(!reg.is_enabled(a)) && ok;

    return ok;
}


// tests_merge
bool tests_merge()
{
    bool ok = true;

    DEventRegistry dst;
    dst.bind<DEvInt>(DSumming{0});

    DEventRegistry src;
    src.bind<DEvInt>(DSumming{0});
    src.bind<DEvTwo>(DBinaryRec{0, 0});

    // merge appends src's handlers (re-keyed) into dst and returns the count.
    std::size_t merged = dst.merge(src);
    ok = D_ER_CHECK(merged == 2u) && ok;
    ok = D_ER_CHECK(dst.handler_count() == 3u) && ok;
    ok = D_ER_CHECK(dst.type_count() == 2u) && ok;
    ok = D_ER_CHECK(dst.handler_count_for<DEvInt>() == 2u) && ok;
    ok = D_ER_CHECK(dst.handler_count_for<DEvTwo>() == 1u) && ok;

    // the source is left untouched.
    ok = D_ER_CHECK(src.handler_count() == 2u) && ok;

    // the empty registry is the identity element of merge.
    DEventRegistry empty;
    ok = D_ER_CHECK(dst.merge(empty) == 0u) && ok;
    ok = D_ER_CHECK(dst.handler_count() == 3u) && ok;

    return ok;
}


// tests_typed_queries
bool tests_typed_queries()
{
    bool ok = true;

    DEventRegistry reg;
    reg.bind<DEvInt>(DSumming{0});
    reg.bind<DEvInt>(DSumming{0});
    reg.bind<DEvTwo>(DBinaryRec{0, 0});

    // per-event counts and presence.
    ok = D_ER_CHECK(reg.handler_count_for<DEvInt>() == 2u) && ok;
    ok = D_ER_CHECK(reg.handler_count_for<DEvTwo>() == 1u) && ok;
    ok = D_ER_CHECK(reg.handler_count_for<DEvNone>() == 0u) && ok;
    ok = D_ER_CHECK(reg.has_handlers_for<DEvInt>()) && ok;
    ok = D_ER_CHECK(!reg.has_handlers_for<DEvNone>()) && ok;

    // clear_for drops one event's handlers, leaving others intact.
    reg.clear_for<DEvInt>();
    ok = D_ER_CHECK(!reg.has_handlers_for<DEvInt>()) && ok;
    ok = D_ER_CHECK(reg.handler_count_for<DEvTwo>() == 1u) && ok;
    ok = D_ER_CHECK(reg.handler_count() == 1u) && ok;

    return ok;
}


// tests_aggregate_queries
bool tests_aggregate_queries()
{
    bool ok = true;

    DEventRegistry reg;
    DHandlerId a = reg.bind<DEvInt>(DSumming{0});
    reg.bind<DEvInt>(DSumming{0});
    reg.bind<DEvTwo>(DBinaryRec{0, 0});

    // totals across all events.
    ok = D_ER_CHECK(reg.handler_count() == 3u) && ok;
    ok = D_ER_CHECK(reg.enabled_count() == 3u) && ok;
    ok = D_ER_CHECK(reg.type_count() == 2u) && ok;

    // disabling one drops the enabled total but not the handler total.
    reg.disable(a);
    ok = D_ER_CHECK(reg.handler_count() == 3u) && ok;
    ok = D_ER_CHECK(reg.enabled_count() == 2u) && ok;

    // clear removes everything.
    reg.clear();
    ok = D_ER_CHECK(reg.handler_count() == 0u) && ok;
    ok = D_ER_CHECK(reg.enabled_count() == 0u) && ok;
    ok = D_ER_CHECK(reg.type_count() == 0u) && ok;

    return ok;
}


// tests_table_access
bool tests_table_access()
{
    bool ok = true;

    DEventRegistry reg;
    DHandlerId a = reg.bind<DEvInt>(DSumming{0});
    reg.bind<DEvTwo>(DBinaryRec{0, 0});
    (void)a;

    // the mutable overload exposes the underlying table reflecting registry
    // state.
    DEventTable& tbl = reg.table();
    ok = D_ER_CHECK(tbl.total_count() == 2u) && ok;
    ok = D_ER_CHECK(tbl.type_key_count() == 2u) && ok;

    // the const overload agrees.
    const DEventRegistry& creg = reg;
    const DEventTable& ctbl = creg.table();
    ok = D_ER_CHECK(ctbl.total_count() == 2u) && ok;
    ok = D_ER_CHECK(ctbl.enabled_count() == 2u) && ok;

    // it is the same table the registry mutates: a registry edit is visible
    // through the reference.
    reg.disable(a);
    ok = D_ER_CHECK(ctbl.enabled_count() == 1u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
