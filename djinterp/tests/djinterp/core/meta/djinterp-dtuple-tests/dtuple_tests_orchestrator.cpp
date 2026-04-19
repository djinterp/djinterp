/******************************************************************************
* djinterp [testing]                              dtuple_tests_orchestrator.cpp
*
*   Orchestrator for the dtuple compile-time trait test suite.  Binds
* every built-in lifecycle listener and every custom event listener,
* walks each semantic test group in sequence with on_group_start /
* on_group_end boundaries, demonstrates mid-run enable / disable of
* multiple listener ids, drives a compile-time aggregate trait_suite
* through the trait_suite_object runtime adapter, constructs a
* demonstration test_tree<basic_test, demo_tree_backing> and queries
* its count_passed / all_passed methods, unbinds every listener via
* teardown_dtuple_listeners, and returns the accumulated
* session_result.
*
*   This file also owns:
*     - the anonymous-namespace demo_tree_backing shim used to
*       demonstrate test_tree without depending on a full n-ary
*       tree container implementation
*     - the dtuple_aggregate_suite trait_suite definition (one
*       representative trait_record per component), kept out of the
*       header so per-group .cpp files do not drag it in
*     - record_outcome and record_group_boundary helpers, separated
*       from the orchestrator body so the narrative-flow of
*       run_dtuple_tests reads as a flat sequence of group walks
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_orchestrator.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// TEST TREE BACKING SHIM
// =========================================================================
//   test_tree<_Element, _Underlying> requires _Underlying to expose
// root() and size() (per the static_asserts in test_tree.hpp).  A
// full n-ary tree container is out of scope for a test driver, so
// we use a minimal std::vector-backed shim that satisfies the
// protocol: size() and begin()/end() come from std::vector;
// root() is an added one-liner returning front().  This gives the
// tree a single-spine layout (root at index 0, children flat after)
// which is enough to demonstrate the overlay querying surface
// (count_passed, all_passed, etc.).

namespace {

    // demo_tree_backing
    //   class: minimal n-ary tree stand-in for the
    // test_tree demonstration.  Inherits from std::vector
    // for iteration, size, clear, and ctor forwarding; adds
    // the required root() accessor that test_tree demands.
    template<typename _Element>
    class demo_tree_backing : public std::vector<_Element>
    {
    public:
        using base = std::vector<_Element>;
        using base::base;  // inherit constructors

        // root
        //   returns a reference to the first element, treated
        // as the tree's root.  Present so the test_tree
        // protocol's root() check is satisfied; the overlay
        // itself does not exercise this in count_* walks.
        _Element&
        root()
        {
            return this->front();
        }

        const _Element&
        root() const
        {
            return this->front();
        }
    };

}  // anonymous namespace


// =========================================================================
// COMPILE-TIME AGGREGATE SUITE
// =========================================================================
//   dtuple_aggregate_suite gathers one representative
// trait_record per dtuple component.  Every record's boolean
// parameter is a dtuple expression evaluated at template-
// instantiation time, so the suite as a whole exists only if
// every check is true.  trait_suite_object wraps the result
// for the runtime printer pipeline.

namespace {

    // dtuple_aggregate_suite
    //   trait: compile-time trait_suite spanning the full
    // dtuple surface.  Each record pins one representative
    // fact from one component; the aggregate exists iff
    // every fact holds.  Because this is a compile-time
    // aggregate, no runtime work is needed to "re-evaluate"
    // its members — the printer just reads the static
    // `passed_count`, `total`, and `all_passed` members.
    using dtuple_aggregate_suite = trait_suite<
        trait_record<
            std::is_same<first_arg_t<int, char>, int>::value>,
        trait_record<
            is_tuple<std::tuple<int>>::value>,
        trait_record<
            std::is_same<to_tuple_t<int>,
                         std::tuple<int>>::value>,
        trait_record<
            std::is_same<make_tuple_of_t<int, 3>,
                         std::tuple<int, int, int>>::value>,
        trait_record<
            std::is_same<wrap_all_t<int, std::add_pointer>,
                         int*>::value>,
        trait_record<
            std::is_same<typename tuple_join<std::tuple<int>,
                                              std::tuple<char>
                                             >::type,
                         std::tuple<int, char>>::value>,
        trait_record<
            std::is_same<typename tuple_join<std::tuple<int>,
                                              std::tuple<char>,
                                              std::tuple<long>
                                             >::type,
                         std::tuple<int, char, long>>::value>,
        trait_record<
            std::is_same<tuple_type_at_t<1, int, char, long>,
                         char>::value>,
        trait_record<
            std::is_same<tuple_apply_all_t<std::add_pointer_t,
                                            int, char>,
                         std::tuple<int*, char*>>::value>,
        trait_record<
            (tuple_count_type<int,
                              std::tuple<int, char, int>
                             >::value == 2)>,
        trait_record<
            std::is_same<tuple_subsequence_t<1, 3,
                             std::tuple<int, char, long, double>>,
                         std::tuple<char, long>>::value>,
        trait_record<
            std::is_same<type_select_t<type_case<true, int>>,
                         int>::value>,
        trait_record<
            is_tuple_homogeneous<std::tuple<int, int>>::value>,
        trait_record<
            is_2d_tuple<std::tuple<std::tuple<int>>>::value>,
        trait_record<
            std::is_same<normalize_tuple_t<std::tuple<const int&>>,
                         std::tuple<int>>::value>,
        // reference-qualified indexing round-trip
        trait_record<
            std::is_same<tuple_type_at_t<0,
                             std::tuple<int&, const char&&>>,
                         int&>::value>,
        // empty-tuple sweep sanity
        trait_record<
            std::is_same<tuple_flatten_types_t<std::tuple<>>,
                         std::tuple<>>::value>
    >;

}  // anonymous namespace


// =========================================================================
// I.   SETUP / TEARDOWN HELPERS
// =========================================================================

/*
setup_dtuple_listeners
  Binds one listener for every built-in lifecycle event and
  every custom event declared in dtuple_tests.hpp.  The compile-
  check listener is bound DISABLED so it does not flood the
  output; the orchestrator enables it around whichever test it
  wants verbose reporting for.

Parameter(s):
  _handler: the handler to bind listeners on.  Must outlive the
            returned handle set.
Return:
  A listener_handle_set carrying every listener id so the
  orchestrator can selectively enable / disable individual
  listeners and teardown_dtuple_listeners can unbind them all.
*/
listener_handle_set
setup_dtuple_listeners(
    test_handler& _handler
)
{
    listener_handle_set h;

    // ---- built-in lifecycle listeners ----

    h.session_start = _handler.on<events::on_session_start>(
        [](event_context& /*_ctx*/)
        {
            std::printf(
                "============================================\n"
                "  DTest demo: dtuple compile-time test suite\n"
                "============================================\n");
        });

    h.session_end = _handler.on<events::on_session_end>(
        [](event_context& /*_ctx*/,
           std::size_t    _passed,
           std::size_t    _failed)
        {
            std::printf(
                "--------------------------------------------\n"
                "  finished: %zu passed, %zu failed\n"
                "--------------------------------------------\n",
                _passed,
                _failed);
        });

    h.module_start = _handler.on<events::on_module_start>(
        [](event_context&    /*_ctx*/,
           const basic_test* _mod)
        {
            std::printf("  ==> module: %s\n",
                        (_mod && _mod->name())
                            ? _mod->name() : "(unnamed)");
        });

    h.module_end = _handler.on<events::on_module_end>(
        [](event_context&    /*_ctx*/,
           const basic_test* _mod)
        {
            std::printf("  <== module: %s\n",
                        (_mod && _mod->name())
                            ? _mod->name() : "(unnamed)");
        });

    h.test_passed = _handler.on<events::on_test_passed>(
        [](event_context&   /*_ctx*/,
           const basic_test* _obj)
        {
            std::printf("  [PASS] %s\n",
                        (_obj && _obj->name())
                            ? _obj->name() : "(unnamed)");
        });

    h.test_failed = _handler.on<events::on_test_failed>(
        [](event_context&    /*_ctx*/,
           const basic_test* _obj)
        {
            std::fputs("\xE2\x80\xBC THIS SHOULD NOT HAPPEN",
                       stderr);
            if ( (_obj) &&
                 (_obj->name()) )
            {
                std::fprintf(stderr, " (%s)", _obj->name());
            }
            std::fputc('\n', stderr);
        });

    h.test_skipped = _handler.on<events::on_test_skipped>(
        [](event_context&    /*_ctx*/,
           const basic_test* _obj)
        {
            std::printf("  [SKIP] %s\n",
                        (_obj && _obj->name())
                            ? _obj->name() : "(unnamed)");
        });

    h.test_error = _handler.on<events::on_test_error>(
        [](event_context&    /*_ctx*/,
           const basic_test* _obj,
           const char*       _msg)
        {
            std::fprintf(stderr,
                         "  [ERR ] %s: %s\n",
                         (_obj && _obj->name())
                             ? _obj->name() : "(unnamed)",
                         _msg ? _msg : "(no message)");
        });

    h.status_change = _handler.on<events::on_status_change>(
        [](event_context&    /*_ctx*/,
           const basic_test* /*_obj*/,
           test_status       /*_from*/,
           test_status       /*_to*/)
        {
            // intentionally silent — bound only to demonstrate
            // that every built-in event has a listener attached
            // in this demo
        });

    h.listener_threw = _handler.on<events::on_listener_threw>(
        [](event_context& /*_ctx*/,
           const char*    _event_name,
           const char*    _what)
        {
            std::fprintf(stderr,
                         "\xE2\x80\xBC listener threw "
                         "(event=%s): %s\n",
                         _event_name ? _event_name : "(unknown)",
                         _what       ? _what       : "(no message)");
        });

    // ---- custom-event listeners ----

    h.compile_check = _handler.on<on_compile_check>(
        [](event_context& /*_ctx*/,
           const char*    _name)
        {
            std::printf("    .. %s\n",
                        _name ? _name : "(unnamed)");
        });
    // keep verbose compile-check output off by default
    _handler.disable(h.compile_check);

    h.dtuple_demo = _handler.on<on_dtuple_demo>(
        [](event_context& /*_ctx*/)
        {
            std::printf("  (dtuple demo block engaged)\n");
        });

    h.group_start = _handler.on<on_group_start>(
        [](event_context& /*_ctx*/,
           const char*    _group)
        {
            std::printf(
                "\n  ---------- GROUP: %s ----------\n",
                _group ? _group : "(unnamed)");
        });

    h.group_end = _handler.on<on_group_end>(
        [](event_context& /*_ctx*/,
           const char*    _group,
           std::size_t    _passed,
           std::size_t    _total)
        {
            std::printf(
                "  ---------- /GROUP: %s "
                "(%zu/%zu passed) ----------\n",
                _group ? _group : "(unnamed)",
                _passed,
                _total);
        });

    return h;
}


/*
teardown_dtuple_listeners
  Unbinds every listener in the supplied handle set so the
  handler is returned to its pre-run state.

Parameter(s):
  _handler: the handler the listeners are bound on.
  _handles: the handle set returned from setup_dtuple_listeners.
Return:
  none.
*/
void
teardown_dtuple_listeners(
    test_handler&              _handler,
    const listener_handle_set& _handles
)
{
    _handler.off(_handles.session_start);
    _handler.off(_handles.session_end);
    _handler.off(_handles.module_start);
    _handler.off(_handles.module_end);
    _handler.off(_handles.test_passed);
    _handler.off(_handles.test_failed);
    _handler.off(_handles.test_skipped);
    _handler.off(_handles.test_error);
    _handler.off(_handles.status_change);
    _handler.off(_handles.listener_threw);
    _handler.off(_handles.compile_check);
    _handler.off(_handles.dtuple_demo);
    _handler.off(_handles.group_start);
    _handler.off(_handles.group_end);

    return;
}


// =========================================================================
// II.  OUTCOME RECORDING HELPERS
// =========================================================================

/*
record_outcome
  Fires the appropriate built-in lifecycle event with a stable
  const char* name, then updates the handler's pass/fail
  counters.  Constructs a transient basic_test solely so the
  standard lifecycle events have a nameable payload to carry.

Parameter(s):
  _handler: the handler whose counters are updated and whose
            listeners receive the dispatch.
  _name:    the test's display name, kept alive for the duration
            of the dispatch.
  _result:  true for pass, false for fail.
Return:
  none.
*/
void
record_outcome(
    test_handler& _handler,
    const char*   _name,
    bool          _result
)
{
    basic_test node;
    node.set_name(_name);
    node.set_status(_result ? basic_test::status_passed
                            : basic_test::status_failed);

    if (_result)
    {
        _handler.fire<events::on_test_passed>(&node);
        _handler.record(test_status::passed);
    }
    else
    {
        _handler.fire<events::on_test_failed>(&node);
        _handler.record(test_status::failed);
    }

    return;
}


/*
record_group_boundary
  Fires on_group_start or on_group_end with the supplied
  payload.  Mirrors the built-in on_module_start / on_module_end
  signals but on the custom-event channel so group-level sinks
  can be attached or detached independently of the module-level
  ones.

Parameter(s):
  _handler:    the handler doing the dispatch.
  _group_name: the semantic group's name (e.g. "structural").
  _is_start:   true fires on_group_start, false fires on_group_end.
  _passed:     passed count payload (on_group_end only; ignored
               on start).
  _total:      total count payload (on_group_end only; ignored
               on start).
Return:
  none.
*/
void
record_group_boundary(
    test_handler& _handler,
    const char*   _group_name,
    bool          _is_start,
    std::size_t   _passed,
    std::size_t   _total
)
{
    if (_is_start)
    {
        _handler.fire<on_group_start>(_group_name);
    }
    else
    {
        _handler.fire<on_group_end>(_group_name,
                                     _passed,
                                     _total);
    }

    return;
}


// =========================================================================
// III. INTERNAL GROUP-WALK HELPER
// =========================================================================

namespace {

    // group_snapshot
    //   struct: captures the handler's pass / total counters at
    // a point in time so the per-group delta can be computed
    // when the group ends.  Kept in an anonymous namespace
    // because it is an implementation detail of run_dtuple_tests.
    struct group_snapshot
    {
        std::size_t start_passed;
        std::size_t start_total;
    };

    // snapshot_handler
    //   helper: records the handler's current counters into a
    // group_snapshot.
    group_snapshot
    snapshot_handler(
        const test_handler& _handler
    )
    {
        group_snapshot s;
        s.start_passed = _handler.passed();
        s.start_total  = _handler.total();

        return s;
    }

    // build_module_node
    //   helper: constructs a transient basic_test representing a
    // module-level payload for events::on_module_start /
    // on_module_end dispatch.  Kept out of record_outcome because
    // modules have no pass/fail semantics at this layer — that is
    // resolved at group-close time.
    basic_test
    build_module_node(
        const char* _name
    )
    {
        basic_test node;
        node.set_name(_name);
        node.set_status(basic_test::status_pending);

        return node;
    }

}  // anonymous namespace


// =========================================================================
// IV.  ORCHESTRATOR
// =========================================================================

session_result
run_dtuple_tests(
    test_handler& _handler
)
{
    // ---- bind every lifecycle + custom listener ----

    listener_handle_set handles = setup_dtuple_listeners(_handler);

    // ---- demonstrate deferred dispatch (queue + process_all) ----
    //   queue the demo notification before starting the session.
    // Any event fired directly on the handler between now and
    // process_all() still dispatches immediately — only the
    // queued one waits.

    _handler.queue<on_dtuple_demo>();

    // ---- start session ----

    _handler.start_session();

    // process the queued demo notification now that the session
    // has started.  This prints "(dtuple demo block engaged)"
    // before any test runs.
    _handler.process_all();

    // ---- structural group ----

    {
        basic_test mod_node = build_module_node("structural");
        _handler.fire<events::on_module_start>(&mod_node);

        group_snapshot snap = snapshot_handler(_handler);
        record_group_boundary(_handler, "structural",
                              true, 0, 0);

        // flip on the verbose compile-check sink for this group
        // only — purely to demonstrate enable/disable mid-run
        _handler.enable(handles.compile_check);

        record_outcome(_handler, "first_arg",
                       tests_dtuple_first_arg(_handler));
        record_outcome(_handler, "is_tuple",
                       tests_dtuple_is_tuple(_handler));
        record_outcome(_handler, "to_tuple",
                       tests_dtuple_to_tuple(_handler));
        record_outcome(_handler, "make_tuple_of",
                       tests_dtuple_make_tuple_of(_handler));

        // back to quiet mode
        _handler.disable(handles.compile_check);

        record_group_boundary(_handler, "structural", false,
                              (_handler.passed() - snap.start_passed),
                              (_handler.total()  - snap.start_total));

        _handler.fire<events::on_module_end>(&mod_node);
    }

    // ---- composition group ----

    {
        basic_test mod_node = build_module_node("composition");
        _handler.fire<events::on_module_start>(&mod_node);

        group_snapshot snap = snapshot_handler(_handler);
        record_group_boundary(_handler, "composition",
                              true, 0, 0);

        record_outcome(_handler, "wrap_all_and_modifiers",
                       tests_dtuple_wrap_all_and_modifiers(_handler));
        record_outcome(_handler, "tuple_join",
                       tests_dtuple_tuple_join(_handler));

        record_group_boundary(_handler, "composition", false,
                              (_handler.passed() - snap.start_passed),
                              (_handler.total()  - snap.start_total));

        _handler.fire<events::on_module_end>(&mod_node);
    }

    // ---- indexing group ----

    {
        basic_test mod_node = build_module_node("indexing");
        _handler.fire<events::on_module_start>(&mod_node);

        group_snapshot snap = snapshot_handler(_handler);
        record_group_boundary(_handler, "indexing",
                              true, 0, 0);

        record_outcome(_handler, "tuple_type_at",
                       tests_dtuple_tuple_type_at(_handler));
        record_outcome(_handler, "tuple_subsequence",
                       tests_dtuple_tuple_subsequence(_handler));
        record_outcome(_handler, "tuple_split",
                       tests_dtuple_tuple_split(_handler));
        record_outcome(_handler, "indexing_refqual_roundtrip",
                       tests_dtuple_indexing_refqual_roundtrip(
                           _handler));
        record_outcome(_handler, "indexing_stress_depth",
                       tests_dtuple_indexing_stress_depth(_handler));

        record_group_boundary(_handler, "indexing", false,
                              (_handler.passed() - snap.start_passed),
                              (_handler.total()  - snap.start_total));

        _handler.fire<events::on_module_end>(&mod_node);
    }

    // ---- transforms group ----

    {
        basic_test mod_node = build_module_node("transforms");
        _handler.fire<events::on_module_start>(&mod_node);

        group_snapshot snap = snapshot_handler(_handler);
        record_group_boundary(_handler, "transforms",
                              true, 0, 0);

        record_outcome(_handler, "tuple_apply_all",
                       tests_dtuple_tuple_apply_all(_handler));
        record_outcome(_handler, "count_and_remove",
                       tests_dtuple_count_and_remove(_handler));
        record_outcome(_handler, "tuple_count_type",
                       tests_dtuple_tuple_count_type(_handler));

        record_group_boundary(_handler, "transforms", false,
                              (_handler.passed() - snap.start_passed),
                              (_handler.total()  - snap.start_total));

        _handler.fire<events::on_module_end>(&mod_node);
    }

    // ---- classification group ----

    {
        basic_test mod_node = build_module_node("classification");
        _handler.fire<events::on_module_start>(&mod_node);

        group_snapshot snap = snapshot_handler(_handler);
        record_group_boundary(_handler, "classification",
                              true, 0, 0);

        record_outcome(_handler, "type_selector",
                       tests_dtuple_type_selector(_handler));
        record_outcome(_handler, "homogeneity_and_2d",
                       tests_dtuple_homogeneity_and_2d(_handler));
        record_outcome(_handler, "is_2d_intersect_homog",
                       tests_dtuple_is_2d_intersect_homog(_handler));

        record_group_boundary(_handler, "classification", false,
                              (_handler.passed() - snap.start_passed),
                              (_handler.total()  - snap.start_total));

        _handler.fire<events::on_module_end>(&mod_node);
    }

    // ---- flatten group ----

    {
        basic_test mod_node = build_module_node("flatten");
        _handler.fire<events::on_module_start>(&mod_node);

        group_snapshot snap = snapshot_handler(_handler);
        record_group_boundary(_handler, "flatten",
                              true, 0, 0);

        record_outcome(_handler, "flatten_and_normalize",
                       tests_dtuple_flatten_and_normalize(_handler));

        record_group_boundary(_handler, "flatten", false,
                              (_handler.passed() - snap.start_passed),
                              (_handler.total()  - snap.start_total));

        _handler.fire<events::on_module_end>(&mod_node);
    }

    // ---- meta group ----

    {
        basic_test mod_node = build_module_node("meta");
        _handler.fire<events::on_module_start>(&mod_node);

        group_snapshot snap = snapshot_handler(_handler);
        record_group_boundary(_handler, "meta",
                              true, 0, 0);

        record_outcome(_handler, "robustness",
                       tests_dtuple_robustness(_handler));
        record_outcome(_handler, "logical_relationships",
                       tests_dtuple_logical_relationships(_handler));
        record_outcome(_handler, "alias_consistency",
                       tests_dtuple_alias_consistency(_handler));
        record_outcome(_handler, "zero_size_sweep",
                       tests_dtuple_zero_size_sweep(_handler));
        record_outcome(_handler, "incomplete_types",
                       tests_dtuple_incomplete_types(_handler));
        record_outcome(_handler, "sfinae_friendliness",
                       tests_dtuple_sfinae_friendliness(_handler));
        record_outcome(_handler, "composition_interplay",
                       tests_dtuple_composition_interplay(_handler));
        record_outcome(_handler, "concepts_demo",
                       tests_dtuple_concepts_demo(_handler));

        record_group_boundary(_handler, "meta", false,
                              (_handler.passed() - snap.start_passed),
                              (_handler.total()  - snap.start_total));

        _handler.fire<events::on_module_end>(&mod_node);
    }

    // ---- runtime adapter for the compile-time aggregate suite ----
    //   drive the printer from the static suite outcome.  The
    // aggregate is a different type from basic_test, so we
    // construct a parallel basic_test that mirrors its outcome
    // and fire the lifecycle event with that.

    trait_suite_object<dtuple_aggregate_suite> aggregate(
        "dtuple_compile_time_aggregate");

    basic_test aggregate_node;
    aggregate_node.set_name("dtuple_compile_time_aggregate");
    aggregate_node.set_status(
        static_cast<bool>(aggregate)
            ? basic_test::status_passed
            : basic_test::status_failed);

    if (static_cast<bool>(aggregate))
    {
        _handler.fire<events::on_test_passed>(&aggregate_node);
        _handler.record(test_status::passed);
    }
    else
    {
        _handler.fire<events::on_test_failed>(&aggregate_node);
        _handler.record(test_status::failed);
    }

    std::printf(
        "  (compile-time aggregate: %zu / %zu records pass)\n",
        aggregate.passed(),
        aggregate.total());

    // ---- test_tree demonstration ----
    //   Build a test_tree overlay against the demo backing shim,
    // populate it with a handful of (synthetic) passing test
    // outcomes, and query its count_passed / all_passed methods.
    // This exercises the overlay query surface without touching
    // the handler's own counters.

    {
        using demo_tree =
            test_tree<basic_test, demo_tree_backing<basic_test>>;

        demo_tree tree;

        // populate the tree via the underlying container.  The
        // root is at index 0, remaining entries represent its
        // children in a single-spine layout.
        for (int i = 0; i < 4; ++i)
        {
            basic_test node;
            char       name_buf[32];
            std::snprintf(name_buf, sizeof(name_buf),
                          "tree_demo_leaf_%d", i);
            node.set_name(name_buf);
            node.set_status(basic_test::status_passed);
            tree.underlying().push_back(node);
        }

        std::printf(
            "  (test_tree demo: %zu / %zu passed, all_passed=%s)\n",
            tree.count_passed(),
            tree.underlying().size(),
            tree.all_passed() ? "true" : "false");
    }

    // ---- close out session and unbind listeners ----

    _handler.end_session();

    session_result final_result = _handler.result();

    teardown_dtuple_listeners(_handler, handles);

    return final_result;
}


NS_END  // testing
NS_END  // djinterp
