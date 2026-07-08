/******************************************************************************
* djinterp [test]                                             test_builder.hpp
*
*   The fluent, functional builder over a test_tree.  Where test_tree.hpp
* is the storage and test_object.hpp the node, this module is the AUTHORING
* surface: a single chainable object that grows a rank-checked forest of
* test_objects and binds each leaf's deferred work into a
* test_callable_table, then walks the forest to evaluate it.
*
*   ONE OBJECT, MANY IDIOMS:
*   Every mutating method returns `test_builder&`, so the same surface
* serves three authoring styles with no separate API:
*
*     PROCEDURAL - statements in sequence, each standing alone:
*         suite.test_module("foo");
*         suite.test_block("flat props");
*         suite.test("is mutable").assert_(cond);
*
*     FUNCTIONAL - one chained expression, plus the scoped lambda forms
*     (module/block taking a `void(test_builder&)` body) for clean nesting
*     and the predicate-driven add_if / assert_all / assert_any:
*         make_suite()
*             .module("foo", [](test_builder<>& m){
*                 m.block("flat props", [](test_builder<>& b){
*                     b.test("is mutable").assert_(cond);
*                 });
*             })
*             .run();
*
*     HYBRID - mix freely (the example in the design brief):
*         suite.test_module("foo")
*              .test_block("")
*                  .test("is flat, static, mutable")
*                      .assert_(cond)
*                      .test_fn(&test_flat_static_mutable)
*              .add_if(want_extra, [](test_builder<>& s){ s.test_block("extra"); })
*              .test_block("container properties")
*                  .test("holds elements")
*                      .assert_([]{ return true; })
*                      .test_fn(&test_holds_elements)
*              .run();
*
*   A TEST IS A THUNK (THE FUNCTIONAL CORE):
*   A leaf's pass/fail is the value produced by a deferred `() -> bool`
* thunk held in the callable table; the node itself stores only the id.
* assert_ / test_fn / assert_all / assert_any COMPOSE thunks under boolean
* algebra (short-circuit AND across conjoined clauses, OR for any_of),
* mirroring functional/predicate.hpp's combinators.  A leaf with no clause
* bound stays `pending` - a not-yet-implemented test, never a failure.
*
*   NAMED vs ANONYMOUS:
*   test()/module()/block() add NAMED nodes evaluated later, at run().
* check()/expect() add ANONYMOUS leaves evaluated IMMEDIATELY, folding
* their outcome into the same tree (and so the same counts) without a name
* or a callable row.
*
*   RANK SAFETY:
*   The builder seeds the tree's kind set with three default kinds -
* module > block > test by rank, test a leaf - so test_tree's rank-checked
* append_child structurally forbids nonsense (a module inside a test).
* Callers may register their own kinds before building to extend the
* vocabulary.
*
*   EVENTS (LIFECYCLE + CUSTOM):
*   The builder owns an `event_dispatcher` (core/event).  run() fires the
* framework's lifecycle events from test_event.hpp as it walks - one
* on_session_start, an on_module_start per interior node, the full
* on_test_start / on_status_change / on_test_passed|failed|skipped|error /
* on_test_end sequence per leaf, and on_session_end carrying the pass/fail
* counts.  Subscribe with the chainable on_passed / on_failed / on_skipped /
* on_error sugar, the generic on<_Event>(handler), or events().bind<_Event>()
* when you need the handler_id back.  Emit your own (e.g. value-tagged) events
* mid-chain with fire<_Event>(payload...).  Handlers are plain callables
* returning void (always-pass) or `verdict` (verdict::consume halts the rest
* of that event's word); a handler that throws is caught and re-reported as
* on_listener_threw rather than aborting the run.
*
*   WHAT THIS MODULE DOES NOT OWN:
*   The rank-aware STRUCTURAL walk (enter/leave subtree pairing).  run() is a
* FLAT pre-order walk, so it fires on_module_start on entry but leaves the
* paired on_module_end to a structural driver; hand it tree() + events() to
* emit the leave events.  Swapping the flat walk for that structural one is a
* localized change behind the same event surface.
*
*   PORTABILITY:
*   C++11 minimum (lambdas, std::function via test_callable.hpp,
* perfect forwarding).
*
*
* TABLE OF CONTENTS
* =================
* I.    PORTABILITY CHECKS
* II.   DEFAULT TEST KINDS
* III.  RUN SUMMARY
* IV.   CALLABLE DETECTION (internal)
* V.    TEST BUILDER
* VI.   FACTORY FUNCTIONS
*
*
* path:      /inc/djinterp/test/test_builder.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.17
******************************************************************************/

#ifndef DJINTERP_TEST_BUILDER_
#define DJINTERP_TEST_BUILDER_ 1

#ifndef __cplusplus
    #error "test_builder.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <exception>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/event/event_common.hpp"
#include "../core/event/event_dispatcher.hpp"
#include "./test_common.hpp"
#include "./test_object.hpp"
#include "./test_kind.hpp"
#include "./test_callable.hpp"
#include "./test_event.hpp"
#include "./test_tree.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_builder.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                II.  DEFAULT TEST KINDS                                   ///
///////////////////////////////////////////////////////////////////////////////
//
//   The builder's default classification vocabulary: three kinds whose
// ranks descend module > block > test, with test the only leaf.  Because
// test_tree's append_child enforces rank monotonicity (child rank <=
// parent rank) and refuses children under a leaf, seeding these into the
// tree's kind set makes the nesting module -> block -> test structurally
// enforced rather than merely conventional.  The ids occupy a small
// reserved range; user kinds should live elsewhere.

// k_kind_module
//   constant: the id of the default module kind (interior, top rank).
D_STATIC D_CONSTEXPR test_type_id k_kind_module =
    static_cast<test_type_id>(1000);

// k_kind_block
//   constant: the id of the default block kind (interior, mid rank).
D_STATIC D_CONSTEXPR test_type_id k_kind_block =
    static_cast<test_type_id>(1001);

// k_kind_test
//   constant: the id of the default test kind (leaf, low rank).
D_STATIC D_CONSTEXPR test_type_id k_kind_test =
    static_cast<test_type_id>(1002);


// default_kinds
//   function: the three-record kind set the builder installs by default.
// Ranks 30 / 20 / 10 keep module > block > test; only test is a leaf.
D_INLINE std::vector<test_kind>
default_kinds()
{
    std::vector<test_kind> kinds;

    kinds.push_back(make_test_kind(k_kind_module, "module", 30, false));
    kinds.push_back(make_test_kind(k_kind_block,  "block",  20, false));
    kinds.push_back(make_test_kind(k_kind_test,   "test",   10, true));

    return kinds;
}


///////////////////////////////////////////////////////////////////////////////
///                III. RUN SUMMARY                                          ///
///////////////////////////////////////////////////////////////////////////////

// test_summary
//   struct: the tally a run() returns.  Structural counts (modules,
// blocks) come from what the builder added; status counts come from the
// evaluated forest.  `pending` is leaf-only - the structural interior
// nodes that always stay pending are subtracted out, so it counts exactly
// the not-yet-implemented tests.
struct test_summary
{
    std::size_t modules;
    std::size_t blocks;
    std::size_t tests;
    std::size_t inline_checks;

    std::size_t passed;
    std::size_t failed;
    std::size_t skipped;
    std::size_t errored;
    std::size_t pending;

    D_CONSTEXPR test_summary() D_NOEXCEPT
        : modules(0),
          blocks(0),
          tests(0),
          inline_checks(0),
          passed(0),
          failed(0),
          skipped(0),
          errored(0),
          pending(0)
    {}

    // all_passed
    //   true iff nothing failed, errored, or was left pending.
    D_CONSTEXPR bool
    all_passed() const D_NOEXCEPT
    {
        return ( (failed  == 0) &&
                 (errored == 0) &&
                 (pending == 0) );
    }

    // any_failed
    //   true iff at least one leaf failed or errored.
    D_CONSTEXPR bool
    any_failed() const D_NOEXCEPT
    {
        return ( (failed > 0) ||
                 (errored > 0) );
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  CALLABLE DETECTION (internal)                        ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_nullary_bool_callable
    //   trait: true iff `_Fn` can be invoked with no arguments and its
    // result is contextually convertible to bool.  Distinguishes a
    // deferred predicate (a lambda or `bool(*)()`) from a bare boolean
    // VALUE, so the builder can wrap each correctly into a thunk.  The
    // primary template is false; the specialization fires when the call
    // expression is well-formed.
    template<typename _Fn,
             typename = void>
    struct is_nullary_bool_callable : std::false_type
    {};

    template<typename _Fn>
    struct is_nullary_bool_callable<_Fn,
        void_t<decltype(static_cast<bool>(std::declval<_Fn&>()()))>>
        : std::true_type
    {};

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                V.   TEST BUILDER                                         ///
///////////////////////////////////////////////////////////////////////////////

// test_builder
//   class: the fluent, functional authoring surface over a test_tree.
// Holds the tree, the callable table its leaves bind into, a structural
// cursor (current module / block / test node), and small bookkeeping; every
// mutator returns `*this` for chaining.  See the module banner for the
// procedural / functional / hybrid idioms this single surface supports.
//
// Template parameters:
//   _Tree - the backing test container.  Defaults to test_tree<basic_test>.
//           Its value_type must be basic_test (the builder authors names
//           through test_metadata and status through the basic_test
//           protocol); the tree's backing and rank-validation flag are
//           free to vary.
//
// Usage:
//   test_builder<> suite;
//   suite.test_module("foo").test_block("props")
//        .test("is mutable").assert_([]{ return true; });
//   test_summary s = suite.run();
template<typename _Tree = test_tree<basic_test> >
class test_builder
{
    static_assert(
        std::is_same<typename _Tree::value_type, basic_test>::value,
        "`test_builder` v1 authors basic_test nodes; `_Tree::value_type` "
        "must be basic_test.");

public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using tree_type   = _Tree;
    using value_type  = basic_test;
    using node_type   = typename _Tree::node_type;
    using size_type   = std::size_t;
    using thunk_type  = test_callable_table::thunk_type;

    // body_type
    //   type: a scoped authoring body - the lambda the module()/block()
    // forms hand a reference to this builder so structure can be defined
    // by a higher-order function.
    using body_type   = std::function<void(test_builder&)>;


    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // test_builder
    //   constructor: default.  Installs the default module/block/test
    // kinds so the rank-checked build surface enforces the nesting, and
    // starts with an empty cursor (no module/block/test open).
    test_builder()
        : m_tree(default_kinds()),
          m_callables(),
          m_events(),
          m_module(nullptr),
          m_block(nullptr),
          m_test(nullptr),
          m_modules(0),
          m_blocks(0),
          m_tests(0),
          m_inline(0)
    {}

    // test_builder
    //   constructor: from a pre-built tree (move).  The tree keeps
    // whatever kinds it already carries; the default kinds are NOT
    // re-installed, so a caller supplying a tree owns its vocabulary.
    explicit test_builder(
        tree_type _tree
    )
        : m_tree(static_cast<tree_type&&>(_tree)),
          m_callables(),
          m_events(),
          m_module(nullptr),
          m_block(nullptr),
          m_test(nullptr),
          m_modules(0),
          m_blocks(0),
          m_tests(0),
          m_inline(0)
    {}


    // -----------------------------------------------------------------
    //  structure: flat / chained form
    // -----------------------------------------------------------------

    // test_module
    //   opens a module: a top-rank interior node under the tree's
    // conjunctive root.  Resets the block and test cursor, so subsequent
    // test_block()s nest under this module.  Returns *this.
    test_builder&
    test_module(
        const char* _name
    )
    {
        node_type* node = m_tree.add_root(make_node(k_kind_module, _name));

        if (node != nullptr)
        {
            m_module = node;
            m_block  = nullptr;
            m_test   = nullptr;
            ++m_modules;
        }

        return *this;
    }

    // test_block
    //   opens a block under the current module (or, with no module open,
    // as a top-level root).  Resets the test cursor.  Returns *this.
    test_builder&
    test_block(
        const char* _name
    )
    {
        node_type* node = attach(m_module, make_node(k_kind_block, _name));

        if (node != nullptr)
        {
            m_block = node;
            m_test  = nullptr;
            ++m_blocks;
        }

        return *this;
    }

    // test
    //   opens a leaf test under the current block (or the current module,
    // or a top-level root - whichever is the nearest open scope).  The
    // leaf starts with no clause bound, so it is `pending` until an
    // assert_ / test_fn gives it work.  Returns *this.
    test_builder&
    test(
        const char* _name
    )
    {
        node_type* parent = (m_block != nullptr) ? m_block : m_module;
        node_type* node   = attach(parent, make_node(k_kind_test, _name));

        if (node != nullptr)
        {
            m_test = node;
            ++m_tests;
        }

        return *this;
    }


    // -----------------------------------------------------------------
    //  structure: scoped / functional form
    // -----------------------------------------------------------------

    // module
    //   opens a module, runs _body against this builder (so the module's
    // contents are defined by a higher-order function), then restores the
    // cursor that was in effect before the call - so the scoped form is
    // self-contained and siblings compose cleanly.  Returns *this.
    test_builder&
    module(
        const char* _name,
        body_type   _body
    )
    {
        return scoped(_name, static_cast<body_type&&>(_body), k_kind_module);
    }

    // block
    //   opens a block, runs _body against this builder, then restores the
    // prior cursor.  As with module(), the body defines the block's
    // contents and the scope is self-contained.  Returns *this.
    test_builder&
    block(
        const char* _name,
        body_type   _body
    )
    {
        return scoped(_name, static_cast<body_type&&>(_body), k_kind_block);
    }


    // -----------------------------------------------------------------
    //  assertions (predicate algebra, conjoined into the current test)
    // -----------------------------------------------------------------

    // assert_
    //   conjoins a clause onto the current test under short-circuit AND.
    // `_cond` is either a deferred predicate (a lambda or `bool(*)()`,
    // evaluated at run()) or a bare boolean VALUE (captured now); the
    // builder tells them apart and wraps each correctly.  No-op if no test
    // is open.  Returns *this.
    //
    //   Spelled with a trailing underscore on purpose: a member named
    // `assert` is unusable because <cassert> may define `assert` as a
    // function-like macro that would rewrite the call.
    template<typename _Cond>
    test_builder&
    assert_(
        _Cond&& _cond
    )
    {
        return bind_clause(make_thunk(std::forward<_Cond>(_cond)));
    }

    // test_fn
    //   conjoins a test FUNCTION onto the current test - identical
    // mechanism to assert_, named for the intent of binding a standalone
    // `bool()` test routine (e.g. `&test_flat_static_mutable`).  Returns
    // *this.
    template<typename _Fn>
    test_builder&
    test_fn(
        _Fn&& _fn
    )
    {
        return bind_clause(make_thunk(std::forward<_Fn>(_fn)));
    }

    // assert_all
    //   conjoins the AND of every supplied clause onto the current test:
    // the test passes only if all of them pass.  Mirrors
    // functional/predicate.hpp's all_of.  Returns *this.
    template<typename... _Conds>
    test_builder&
    assert_all(
        _Conds&&... _conds
    )
    {
        std::vector<thunk_type> clauses =
            { make_thunk(std::forward<_Conds>(_conds))... };

        thunk_type all =
            [clauses]() -> bool
            {
                std::size_t i;

                for (i = 0; i < clauses.size(); ++i)
                {
                    if (!clauses[i]())
                    {
                        return false;
                    }
                }

                return true;
            };

        return bind_clause(static_cast<thunk_type&&>(all));
    }

    // assert_any
    //   conjoins the OR of every supplied clause onto the current test:
    // the clause passes if any of them passes.  Mirrors
    // functional/predicate.hpp's any_of.  Returns *this.
    template<typename... _Conds>
    test_builder&
    assert_any(
        _Conds&&... _conds
    )
    {
        std::vector<thunk_type> clauses =
            { make_thunk(std::forward<_Conds>(_conds))... };

        thunk_type any =
            [clauses]() -> bool
            {
                std::size_t i;

                for (i = 0; i < clauses.size(); ++i)
                {
                    if (clauses[i]())
                    {
                        return true;
                    }
                }

                return false;
            };

        return bind_clause(static_cast<thunk_type&&>(any));
    }


    // -----------------------------------------------------------------
    //  node operations on the current test
    // -----------------------------------------------------------------

    // skip
    //   marks the current test intentionally skipped (it will not be
    // evaluated at run()).  No-op if no test is open.  Returns *this.
    test_builder&
    skip()
    {
        if (m_test != nullptr)
        {
            m_test->data().skip();
        }

        return *this;
    }

    // tag
    //   writes a key/value pair onto the current test's metadata (e.g. a
    // category, an owner, a ticket).  No-op if no test is open.  Returns
    // *this.
    test_builder&
    tag(
        const char* _key,
        const char* _value
    )
    {
        if (m_test != nullptr)
        {
            m_test->data().metadata().set(_key, _value);
        }

        return *this;
    }


    // -----------------------------------------------------------------
    //  anonymous inline tests (evaluated immediately)
    // -----------------------------------------------------------------

    // check
    //   adds an ANONYMOUS leaf under the nearest open scope and evaluates
    // it RIGHT NOW from `_cond` (a predicate run immediately, or a bare
    // boolean value).  Its result is folded into the tree's counts but it
    // carries no name and no callable row, and it does NOT become the
    // current test.  Returns *this.
    template<typename _Cond>
    test_builder&
    check(
        _Cond&& _cond
    )
    {
        return inline_eval("(check)", make_thunk(std::forward<_Cond>(_cond)));
    }

    // expect
    //   a named alias of check(): the inline outcome is recorded under
    // `_name` for readability in the forest.  Returns *this.
    template<typename _Cond>
    test_builder&
    expect(
        const char* _name,
        _Cond&&     _cond
    )
    {
        return inline_eval(_name, make_thunk(std::forward<_Cond>(_cond)));
    }


    // -----------------------------------------------------------------
    //  composition (higher-order structure)
    // -----------------------------------------------------------------

    // add
    //   runs _body against this builder unconditionally - the imperative
    // escape hatch for splicing a reusable authoring function into a
    // chain.  The cursor is NOT saved or restored; _body sees and leaves
    // the live cursor.  Returns *this.
    test_builder&
    add(
        body_type _body
    )
    {
        if (_body)
        {
            _body(*this);
        }

        return *this;
    }

    // add_if
    //   runs _body against this builder only if `_cond` holds (a predicate
    // evaluated now, or a bare boolean).  Predicate-driven conditional
    // structure, the functional spelling of "include these tests when X".
    // Returns *this.
    template<typename _Cond>
    test_builder&
    add_if(
        _Cond&&   _cond,
        body_type _body
    )
    {
        // evaluate the condition immediately through the same wrapper the
        // assertions use, so a value or a predicate both work.
        if ( (make_thunk(std::forward<_Cond>(_cond))()) &&
             (_body) )
        {
            _body(*this);
        }

        return *this;
    }


    // -----------------------------------------------------------------
    //  events: subscription
    // -----------------------------------------------------------------

    // events
    //   mutable access to the owned event_dispatcher - for full control:
    // bind() returning a handler_id, unbind / enable / disable, queue /
    // process for deferred delivery, merge, and the staging compile().
    event_dispatcher&
    events() D_NOEXCEPT
    {
        return m_events;
    }

    // events (const)
    const event_dispatcher&
    events() const D_NOEXCEPT
    {
        return m_events;
    }

    // on
    //   subscribes _handler to _Event and returns *this - the chainable,
    // subscribe-and-forget spelling.  Reach for events().bind<_Event>() when
    // you need the handler_id back for a later unbind / enable / disable.
    // The handler must be invocable with _Event's payload and return void
    // (an always-pass handler) or `verdict`; the dispatcher's bind()
    // static_asserts that contract.
    template<typename _Event,
             typename _Callable>
    test_builder&
    on(
        _Callable&& _handler
    )
    {
        m_events.bind<_Event>(std::forward<_Callable>(_handler));

        return *this;
    }

    // on_passed / on_failed / on_skipped / on_error
    //   named sugar binding a handler to the matching per-test status event.
    // on_failed is the principled replacement for "do X when a test fails":
    // run() fires on_test_failed for every failing leaf, so the handler runs
    // once per failure with that leaf as a `const basic_test*`.  on_error's
    // handler additionally receives the diagnostic `const char*`.  Each
    // returns *this for chaining.
    template<typename _Callable>
    test_builder&
    on_passed(
        _Callable&& _handler
    )
    {
        m_events.bind<on_test_passed>(
            std::forward<_Callable>(_handler));

        return *this;
    }

    template<typename _Callable>
    test_builder&
    on_failed(
        _Callable&& _handler
    )
    {
        m_events.bind<on_test_failed>(
            std::forward<_Callable>(_handler));

        return *this;
    }

    template<typename _Callable>
    test_builder&
    on_skipped(
        _Callable&& _handler
    )
    {
        m_events.bind<on_test_skipped>(
            std::forward<_Callable>(_handler));

        return *this;
    }

    template<typename _Callable>
    test_builder&
    on_error(
        _Callable&& _handler
    )
    {
        m_events.bind<on_test_error>(
            std::forward<_Callable>(_handler));

        return *this;
    }


    // -----------------------------------------------------------------
    //  events: emission
    // -----------------------------------------------------------------

    // fire
    //   dispatches an occurrence of _Event immediately against the owned
    // dispatcher and returns *this, so a custom (e.g. value-tagged) event can
    // be emitted mid-chain.  The framework's lifecycle events are fired for
    // you by run(); reach for this to emit your OWN events.  The enriched
    // (count, verdict) dispatch_result is discarded here - call
    // events().fire<_Event>(...) directly when you want it.
    template<typename _Event,
             typename... _Args>
    test_builder&
    fire(
        _Args&&... _args
    )
    {
        m_events.fire<_Event>(std::forward<_Args>(_args)...);

        return *this;
    }


    // -----------------------------------------------------------------
    //  tree-level combinators (read-only traversal, functional shape)
    // -----------------------------------------------------------------

    // each
    //   applies _fn to every node in the forest (a mutating visitor over
    // value_type&).  Returns *this.
    template<typename _Fn>
    test_builder&
    each(
        _Fn _fn
    )
    {
        for (auto it = m_tree.begin(); it != m_tree.end(); ++it)
        {
            _fn(*it);
        }

        return *this;
    }

    // count_if
    //   counts the nodes for which _pred holds.
    template<typename _Pred>
    size_type
    count_if(
        _Pred _pred
    ) const
    {
        size_type n = 0;

        for (auto it = m_tree.begin(); it != m_tree.end(); ++it)
        {
            if (_pred(*it))
            {
                ++n;
            }
        }

        return n;
    }

    // fold
    //   left-folds _fn over every node, threading an accumulator from
    // _init.  The tree-shaped analogue of functional/reduce.hpp's
    // fold_left.
    template<typename _Acc,
             typename _Fn>
    _Acc
    fold(
        _Acc _init,
        _Fn  _fn
    ) const
    {
        for (auto it = m_tree.begin(); it != m_tree.end(); ++it)
        {
            _init = _fn(_init, *it);
        }

        return _init;
    }


    // -----------------------------------------------------------------
    //  terminal: evaluate
    // -----------------------------------------------------------------

    // run
    //   walks the forest once and fires the framework's lifecycle events as
    // it goes: one on_session_start up front, an on_module_start as each
    // interior (module / block) node is entered, and for every leaf test the
    // full on_test_start -> [evaluate] -> on_status_change? -> status event
    // (on_test_passed / on_test_failed / on_test_skipped / on_test_error) ->
    // on_test_end sequence, closing with on_session_end carrying the pass and
    // fail counts.  A leaf carrying a bound thunk is evaluated here (a throw
    // becomes `error` with the diagnostic captured for on_test_error); inline
    // and pending leaves keep the status they already hold.  Idempotent
    // enough to call again after adding more tests.
    //
    //   This is a FLAT pre-order walk, so on_module_start fires on entry but
    // the paired on_module_end is left to a structural driver (hand it
    // tree() + events()).  Conjunctive-root and unknown-kind nodes are
    // skipped.
    test_summary
    run()
    {
        std::size_t passed = 0;
        std::size_t failed = 0;

        safe_fire<on_session_start>();

        for (auto it = m_tree.begin(); it != m_tree.end(); ++it)
        {
            value_type& node = *it;

            if (is_interior_node(node))
            {
                safe_fire<on_module_start>(&node);
            }
            else if (is_test_node(node))
            {
                fire_test_lifecycle(node, passed, failed);
            }
        }

        safe_fire<on_session_end>(passed, failed);

        return summarize();
    }


    // -----------------------------------------------------------------
    //  accessors
    // -----------------------------------------------------------------

    // tree
    //   mutable access to the backing tree (to hand to a structural
    // handler, register extra kinds, etc.).
    tree_type&
    tree() D_NOEXCEPT
    {
        return m_tree;
    }

    // tree (const)
    const tree_type&
    tree() const D_NOEXCEPT
    {
        return m_tree;
    }

    // callables
    //   mutable access to the bound callable table.
    test_callable_table&
    callables() D_NOEXCEPT
    {
        return m_callables;
    }

    // callables (const)
    const test_callable_table&
    callables() const D_NOEXCEPT
    {
        return m_callables;
    }

    // clear
    //   drops the forest, the bound thunks, and the cursor, then re-installs
    // the default kinds.  The event subscriptions are deliberately left
    // intact, so reporting wired up before a rebuild survives it; call
    // events().clear() as well for a total reset.  Returns *this.
    test_builder&
    clear()
    {
        m_tree.clear();
        m_tree.kinds() = default_kinds();
        m_callables.clear();

        m_module  = nullptr;
        m_block   = nullptr;
        m_test    = nullptr;
        m_modules = 0;
        m_blocks  = 0;
        m_tests   = 0;
        m_inline  = 0;

        return *this;
    }


private:
    // -----------------------------------------------------------------
    //  internal: node construction
    // -----------------------------------------------------------------

    // make_node
    //   builds a pending leaf/interior value carrying _type_id and, when
    // _name is non-null, a "name" metadata entry.  Status is left pending;
    // evaluation (or an inline check) sets it later.
    static value_type
    make_node(
        test_type_id _type_id,
        const char*  _name
    )
    {
        value_type node(_type_id);
        node.set_status(value_type::status_pending);

        if (_name != nullptr)
        {
            node.metadata().set("name", _name);
        }

        return node;
    }

    // attach
    //   inserts _child under _parent (rank-checked), or as a top-level
    // root when _parent is null.  Returns the new node, or null if the
    // tree rejected the insertion.
    node_type*
    attach(
        node_type* _parent,
        value_type _child
    )
    {
        if (_parent == nullptr)
        {
            return m_tree.add_root(static_cast<value_type&&>(_child));
        }

        return m_tree.append_child(_parent, static_cast<value_type&&>(_child));
    }


    // -----------------------------------------------------------------
    //  internal: thunk construction (value vs predicate)
    // -----------------------------------------------------------------

    // make_thunk
    //   wraps `_cond` into a thunk, dispatching on whether it is a nullary
    // bool-callable (a deferred predicate) or a plain boolean value.
    template<typename _Cond>
    static thunk_type
    make_thunk(
        _Cond&& _cond
    )
    {
        using clean_cond = typename std::decay<_Cond>::type;

        return make_thunk_dispatch(
            std::forward<_Cond>(_cond),
            std::integral_constant<bool,
                internal::is_nullary_bool_callable<clean_cond>::value>{});
    }

    // make_thunk_dispatch (predicate)
    //   _cond is callable: store it by value and defer the call.
    template<typename _Cond>
    static thunk_type
    make_thunk_dispatch(
        _Cond&&         _cond,
        std::true_type
    )
    {
        typename std::decay<_Cond>::type pred =
            std::forward<_Cond>(_cond);

        return thunk_type(
            [pred]() -> bool
            {
                return static_cast<bool>(pred());
            });
    }

    // make_thunk_dispatch (value)
    //   _cond is a value: capture its truth now and return it on demand.
    template<typename _Cond>
    static thunk_type
    make_thunk_dispatch(
        _Cond&&          _cond,
        std::false_type
    )
    {
        bool value = static_cast<bool>(_cond);

        return thunk_type(
            [value]() -> bool
            {
                return value;
            });
    }


    // -----------------------------------------------------------------
    //  internal: clause binding
    // -----------------------------------------------------------------

    // bind_clause
    //   conjoins _clause onto the current test's callable row under
    // short-circuit AND, allocating the row on first use.  No-op if no
    // test is open.
    test_builder&
    bind_clause(
        thunk_type _clause
    )
    {
        if (m_test == nullptr)
        {
            return *this;
        }

        value_type&      node = m_test->data();
        test_callable_id id   = node.callable_id();

        if (id == k_no_callable)
        {
            id = m_callables.add(static_cast<thunk_type&&>(_clause));
            node.set_callable_id(id);
        }
        else
        {
            m_callables.compose_and(id, static_cast<thunk_type&&>(_clause));
        }

        return *this;
    }


    // -----------------------------------------------------------------
    //  internal: inline evaluation
    // -----------------------------------------------------------------

    // inline_eval
    //   adds an anonymous leaf under the nearest open scope, runs _thunk
    // immediately, writes the outcome onto the leaf, and tallies it.  The
    // leaf carries no callable row, so run() leaves its status intact.
    test_builder&
    inline_eval(
        const char* _name,
        thunk_type  _thunk
    )
    {
        node_type* parent = (m_block != nullptr) ? m_block : m_module;
        value_type leaf   = make_node(k_kind_test, _name);

        // evaluate now; a throwing inline thunk records as error.
        try
        {
            leaf.evaluate(_thunk());
        }
        catch (...)
        {
            leaf.set_status(value_type::status_error);
        }

        node_type* node = attach(parent, static_cast<value_type&&>(leaf));

        if (node != nullptr)
        {
            ++m_inline;
        }

        return *this;
    }


    // -----------------------------------------------------------------
    //  internal: scoped authoring
    // -----------------------------------------------------------------

    // scoped
    //   opens a node of _kind named _name, runs _body against this
    // builder, then restores the cursor triple in effect before the call
    // so the scope is self-contained and siblings compose.
    test_builder&
    scoped(
        const char*  _name,
        body_type    _body,
        test_type_id _kind
    )
    {
        node_type* saved_module = m_module;
        node_type* saved_block  = m_block;
        node_type* saved_test   = m_test;

        // open the requested scope via the flat form.
        if (_kind == k_kind_module)
        {
            test_module(_name);
        }
        else
        {
            test_block(_name);
        }

        // author the contents.
        if (_body)
        {
            _body(*this);
        }

        // restore the prior cursor.
        m_module = saved_module;
        m_block  = saved_block;
        m_test   = saved_test;

        return *this;
    }


    // -----------------------------------------------------------------
    //  internal: lifecycle event firing
    // -----------------------------------------------------------------

    // is_interior_node
    //   true if _node is a rank-aware interior node (a module or a block) -
    // the nodes for which run() fires on_module_start.
    static bool
    is_interior_node(
        const value_type& _node
    )
    {
        return ( (_node.type_id() == k_kind_module) ||
                 (_node.type_id() == k_kind_block) );
    }

    // is_test_node
    //   true if _node is a leaf test - the kind run() drives through the full
    // per-test event sequence.
    static bool
    is_test_node(
        const value_type& _node
    )
    {
        return (_node.type_id() == k_kind_test);
    }

    // fire_test_lifecycle
    //   fires the full per-leaf event sequence for one test node, evaluating
    // it first if it carries a bound thunk.  Threads the running pass / fail
    // counts for the closing on_session_end.
    void
    fire_test_lifecycle(
        value_type&  _node,
        std::size_t& _passed,
        std::size_t& _failed
    )
    {
        safe_fire<on_test_start>(&_node);

        test_status before = static_cast<test_status>(_node.status());

        // deferred leaves are evaluated now; inline and pending leaves keep
        // the status they already hold.
        if (_node.has_callable())
        {
            evaluate_node(_node);
        }

        test_status after = static_cast<test_status>(_node.status());

        if (after != before)
        {
            safe_fire<on_status_change>(&_node, before, after);
        }

        dispatch_status_event(_node, after, _passed, _failed);

        safe_fire<on_test_end>(&_node);

        return;
    }

    // dispatch_status_event
    //   fires the one status-specific event matching _status and bumps the
    // pass / fail counters.  `pending` - a not-yet-implemented test - has no
    // status event.
    void
    dispatch_status_event(
        value_type&  _node,
        test_status  _status,
        std::size_t& _passed,
        std::size_t& _failed
    )
    {
        switch (_status)
        {
            case test_status::passed:
                safe_fire<on_test_passed>(&_node);
                ++_passed;
                break;

            case test_status::failed:
                safe_fire<on_test_failed>(&_node);
                ++_failed;
                break;

            case test_status::skipped:
                safe_fire<on_test_skipped>(&_node);
                break;

            case test_status::error:
            {
                // keep the diagnostic alive across the synchronous fire so the
                // const char* payload cannot dangle inside a handler.
                std::string message = _node.metadata().get("error");

                safe_fire<on_test_error>(
                    &_node,
                    message.empty() ? nullptr : message.c_str());
                break;
            }

            case test_status::pending:
            default:
                break;
        }

        return;
    }

    // safe_fire
    //   dispatches _Event immediately, but contains a throwing handler: an
    // escaping exception is caught and re-reported as on_listener_threw
    // (named by _Event::name()) rather than aborting the walk.
    template<typename _Event,
             typename... _Args>
    void
    safe_fire(
        _Args&&... _args
    )
    {
        try
        {
            m_events.fire<_Event>(std::forward<_Args>(_args)...);
        }
        catch (const std::exception& _e)
        {
            guarded_listener_threw(_Event::name(), _e.what());
        }
        catch (...)
        {
            guarded_listener_threw(_Event::name(), "unknown exception");
        }

        return;
    }

    // guarded_listener_threw
    //   fires on_listener_threw, swallowing any further exception - a handler
    // for on_listener_threw must not throw, and one that does cannot be
    // allowed to take the run down with it.
    void
    guarded_listener_threw(
        const char* _event_name,
        const char* _what
    )
    {
        try
        {
            m_events.fire<on_listener_threw>(_event_name, _what);
        }
        catch (...)
        {
            // deliberately ignored.
        }

        return;
    }


    // -----------------------------------------------------------------
    //  internal: evaluation / tally
    // -----------------------------------------------------------------

    // evaluate_node
    //   invokes the node's bound thunk and writes pass/fail back.  A throw
    // records `error` and stashes the diagnostic under the "error" metadata
    // key, where fire_test_lifecycle reads it for the on_test_error payload.
    void
    evaluate_node(
        value_type& _node
    )
    {
        try
        {
            bool result = m_callables.invoke(_node.callable_id());
            _node.evaluate(result);
        }
        catch (const std::exception& _e)
        {
            _node.set_status(value_type::status_error);
            _node.metadata().set("error", _e.what());
        }
        catch (...)
        {
            _node.set_status(value_type::status_error);
            _node.metadata().set("error", "unknown exception");
        }

        return;
    }

    // summarize
    //   reads the evaluated forest into a tally.  pending is corrected to
    // leaf-only by subtracting the structural interior nodes (the modules,
    // the blocks, and the one conjunctive root) that always stay pending.
    test_summary
    summarize() const
    {
        test_summary s;

        s.modules       = m_modules;
        s.blocks        = m_blocks;
        s.tests         = m_tests;
        s.inline_checks = m_inline;

        s.passed  = m_tree.count_passed();
        s.failed  = m_tree.count_failed();
        s.skipped = m_tree.count_skipped();
        s.errored = m_tree.count_by_status(test_status::error);

        // interior structural nodes are perpetually pending; subtract them
        // so `pending` reflects only not-yet-implemented tests.
        size_type structural =
            ( m_modules +
              m_blocks +
              ((m_tree.root() != nullptr) ? 1u : 0u) );

        size_type pending_all = m_tree.count_pending();

        s.pending = (pending_all > structural)
                    ? (pending_all - structural)
                    : 0;

        return s;
    }


    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    tree_type           m_tree;
    test_callable_table m_callables;
    event_dispatcher    m_events;

    // structural cursor: the nearest open module / block / test node.
    node_type*          m_module;
    node_type*          m_block;
    node_type*          m_test;

    // authoring bookkeeping (drives the structural part of the summary).
    size_type           m_modules;
    size_type           m_blocks;
    size_type           m_tests;
    size_type           m_inline;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  FACTORY FUNCTIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// make_suite
//   function: a fresh default-backed builder, for chains that prefer to
// start from a free function rather than a named local.
//
// Usage:
//   make_suite().test_module("foo")
//               .test("smoke").assert_([]{ return true; })
//               .run();
D_INLINE test_builder<>
make_suite()
{
    return test_builder<>();
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_BUILDER_
