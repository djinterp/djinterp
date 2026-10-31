/*******************************************************************************
* djinterp [test]                 event_dispatcher_tests_dispatcher_dispatch.cpp
*
*   Section III (immediate surface) -- the DEventDispatcher facade's
* synchronous half.  bind and the management operations (unbind / enable /
* disable / is_enabled / contains) delegate to the registry and are reflected
* in the aggregate counts; fire dispatches an occurrence immediately and
* returns the enriched (count, verdict) result, masked by enable/disable; run
* folds a homogeneous trace immediately; compile stages the static word into a
* DFusedStep; merge folds another dispatcher's registry in; the typed and
* aggregate queries report per-event and total handler counts; and the
* component accessors (registry / events / table, mutable and const) expose the
* live sub-objects reflecting the dispatcher's state.
*
*
* path:      /tests/djinterp/cpp/event/event_dispatcher_tests_dispatcher_dispatch.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.30
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "event_dispatcher_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_dispatcher_bind_and_management
bool tests_dispatcher_bind_and_management()
{
    bool ok = true;

    DEventDispatcher d;
    int sink = 0;

    // bind yields a valid id and is reflected in contains / is_enabled / counts.
    DHandlerId a = d.bind<DEvInt>(DSumming{&sink});
    ok = D_ED_CHECK(a.is_valid()) && ok;
    ok = D_ED_CHECK(d.contains(a)) && ok;
    ok = D_ED_CHECK(d.is_enabled(a)) && ok;
    ok = D_ED_CHECK(d.handler_count() == 1u) && ok;
    ok = D_ED_CHECK(d.enabled_count() == 1u) && ok;

    // disable / enable flip the mask through the facade.
    ok = D_ED_CHECK(d.disable(a)) && ok;
    ok = D_ED_CHECK(!d.is_enabled(a)) && ok;
    ok = D_ED_CHECK(d.enabled_count() == 0u) && ok;
    ok = D_ED_CHECK(d.enable(a)) && ok;
    ok = D_ED_CHECK(d.is_enabled(a)) && ok;

    // unbind removes it.
    ok = D_ED_CHECK(d.unbind(a)) && ok;
    ok = D_ED_CHECK(!d.contains(a)) && ok;
    ok = D_ED_CHECK(d.handler_count() == 0u) && ok;

    return ok;
}


// tests_dispatcher_fire_immediate
bool tests_dispatcher_fire_immediate()
{
    bool ok = true;

    DEventDispatcher d;

    // no handler -> (0, pass).
    DDispatchResult r0 = d.fire<DEvInt>(1);
    ok = D_ED_CHECK(r0.invoked == 0u) && ok;
    ok = D_ED_CHECK(!r0.consumed()) && ok;

    // a bound handler fires immediately and receives the payload.
    int sink = 0;
    DHandlerId a = d.bind<DEvInt>(DSumming{&sink});
    DDispatchResult r = d.fire<DEvInt>(5);
    ok = D_ED_CHECK(r.invoked == 1u) && ok;
    ok = D_ED_CHECK(sink == 5) && ok;

    // disabling masks it out of fire.
    d.disable(a);
    DDispatchResult r2 = d.fire<DEvInt>(9);
    ok = D_ED_CHECK(r2.invoked == 0u) && ok;
    ok = D_ED_CHECK(sink == 5) && ok;           // unchanged

    // a consuming handler reports consume.
    DEventDispatcher d2;
    int n = 0;
    d2.bind<DEvInt>(consumer{&n});
    DDispatchResult rc = d2.fire<DEvInt>(0);
    ok = D_ED_CHECK(rc.invoked == 1u) && ok;
    ok = D_ED_CHECK(rc.consumed()) && ok;

    return ok;
}


// tests_dispatcher_run
bool tests_dispatcher_run()
{
    bool ok = true;

    // run folds a homogeneous trace immediately (delegates to the registry).
    DEventDispatcher d;
    int sink = 0;
    d.bind<DEvInt>(DSumming{&sink});

    int data[] = { 1, 2, 3 };
    run_result r = d.run<DEvInt>(data, data + 3);

    ok = D_ED_CHECK(r.occurrences == 3u) && ok;
    ok = D_ED_CHECK(r.handlers_invoked == 3u) && ok;
    ok = D_ED_CHECK(r.consumed_count == 0u) && ok;
    ok = D_ED_CHECK(sink == 6) && ok;

    return ok;
}


// tests_dispatcher_compile
bool tests_dispatcher_compile()
{
    bool ok = true;

    // compile stages the static word into a DFusedStep (delegates to registry).
    DEventDispatcher d;
    std::vector<int> log;
    d.bind<DEvInt>(DOrderRec{&log, 1});
    d.bind<DEvInt>(DOrderRec{&log, 2});

    DFusedStep<DEvInt> step = d.compile<DEvInt>();
    ok = D_ED_CHECK(step.size() == 2u) && ok;

    log.clear();
    verdict v = step(0);
    ok = D_ED_CHECK(v == verdict::pass) && ok;
    ok = D_ED_CHECK(log.size() == 2u && log[0] == 1 && log[1] == 2) && ok;

    return ok;
}


// tests_dispatcher_merge
bool tests_dispatcher_merge()
{
    bool ok = true;

    DEventDispatcher dst;
    dst.bind<DEvInt>(DSumming{0});

    DEventDispatcher src;
    src.bind<DEvInt>(DSumming{0});
    src.bind<DEvTwo>(DTwoSum{0});

    // merge folds src's registry into dst and returns the count merged.
    std::size_t merged = dst.merge(src);
    ok = D_ED_CHECK(merged == 2u) && ok;
    ok = D_ED_CHECK(dst.handler_count() == 3u) && ok;
    ok = D_ED_CHECK(dst.handler_count_for<DEvInt>() == 2u) && ok;
    ok = D_ED_CHECK(dst.handler_count_for<DEvTwo>() == 1u) && ok;

    // src is left intact.
    ok = D_ED_CHECK(src.handler_count() == 2u) && ok;

    return ok;
}


// tests_dispatcher_typed_and_aggregate_queries
bool tests_dispatcher_typed_and_aggregate_queries()
{
    bool ok = true;

    DEventDispatcher d;
    DHandlerId a = d.bind<DEvInt>(DSumming{0});
    d.bind<DEvInt>(DSumming{0});
    d.bind<DEvTwo>(DTwoSum{0});

    // typed queries.
    ok = D_ED_CHECK(d.handler_count_for<DEvInt>() == 2u) && ok;
    ok = D_ED_CHECK(d.handler_count_for<DEvTwo>() == 1u) && ok;
    ok = D_ED_CHECK(d.has_handlers_for<DEvInt>()) && ok;
    ok = D_ED_CHECK(!d.has_handlers_for<DEvNone>()) && ok;

    // aggregate queries.
    ok = D_ED_CHECK(d.handler_count() == 3u) && ok;
    ok = D_ED_CHECK(d.enabled_count() == 3u) && ok;
    d.disable(a);
    ok = D_ED_CHECK(d.enabled_count() == 2u) && ok;
    ok = D_ED_CHECK(d.handler_count() == 3u) && ok;

    return ok;
}


// tests_dispatcher_component_access
bool tests_dispatcher_component_access()
{
    bool ok = true;

    DEventDispatcher d;
    DHandlerId a = d.bind<DEvInt>(DSumming{0});
    (void)a;
    d.queue<DEvInt>(0);

    // mutable accessors expose the live sub-objects.
    DEventRegistry& reg = d.registry();
    DEventQueue&    evq = d.events();
    DEventTable&    tbl = d.table();
    ok = D_ED_CHECK(reg.handler_count() == 1u) && ok;
    ok = D_ED_CHECK(evq.pending() == 1u) && ok;
    ok = D_ED_CHECK(tbl.total_count() == 1u) && ok;

    // const accessors agree.
    const DEventDispatcher& cd = d;
    ok = D_ED_CHECK(cd.registry().handler_count() == 1u) && ok;
    ok = D_ED_CHECK(cd.events().pending() == 1u) && ok;
    ok = D_ED_CHECK(cd.table().total_count() == 1u) && ok;

    // the accessor returns the same object the dispatcher mutates: a registry
    // edit is visible through the reference, and a fire is visible to table().
    d.disable(a);
    ok = D_ED_CHECK(reg.enabled_count() == 0u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
