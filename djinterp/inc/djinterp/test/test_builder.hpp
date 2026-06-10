/******************************************************************************
* djinterp [test]                                             test_builder.hpp
*
*   Functional suite builder for the DTest framework: a fluent, value-
* oriented front end that constructs a test_tree by COMPOSITION rather than
* by imperative tree surgery, and a run-as-a-fold driver that interprets the
* constructed tree.  This is the build layer of the framework's three-fold
* design:
*
*     build   - fold builder ops    -> a test_tree value   (pure; no I/O)
*     run     - fold the tree        -> a run_report         (the DRIVER)
*     report  - transduce the events -> a sink               (out of scope here)
*
*   THE DRIVER IS A FOLD:
*   In the previous design test_handler owned the tree walk.  Here the walk
* is a left fold whose reducer is (run_report, node) -> run_report - the same
* step/driver split as reduce.hpp.  test_handler is demoted to a pure event
* SINK: the fold fires lifecycle events through it but no longer depends on it
* to drive iteration.  A flat suite of constexpr assertions can be folded by
* reduce_ct at compile time with the identical reducer; this header supplies
* only the runtime driver (the tree is a runtime container), but the reducer
* body is written so the constexpr path can reuse it unchanged.
*
*   RANK-DRIVEN NESTING:
*   The fluent chain nests by rank, the same invariant test_tree enforces.
* Each structural call pops the scope stack to the nearest strictly-higher-
* rank ancestor, then attaches and (for interiors) pushes.  A new .test_block
* after a .test therefore closes the test and opens a sibling block under the
* enclosing module automatically - no explicit scope tokens.  The scope stack
* holds INTERIOR nodes only; leaves and effect markers attach under the
* current interior and are never pushed.
*
*   THREE IDIOMS, ONE MODEL:
*     procedural / fluent - suite_builder methods return *this&.
*     functional          - free combinators (dsl::) return a pure node_spec.
*     hybrid              - suite_builder::add(spec) splices a node_spec under
*                            the current scope without disturbing the chain.
*
*   EFFECTS ARE DATA:
*   .fire<E>() / .fire_if_failed<E>() record an effect node in the tree; the
* run-fold (which owns the handler) interprets it.  Building stays pure and
* .fire_if_failed is evaluated at run time against the fold's accumulated
* run_report.  Effect handles index a side table parallel to the callable
* table, exactly as deferred leaf bodies index the callable table.
*
*   PORTABILITY:
*   C++14 minimum (lambdas, return-type deduction, relaxed constexpr).  The
* structural detection it constrains on degrades to C++11 in the underlying
* headers, but the builder's deferred-evaluation closures and the run-fold
* assume C++14.  C++20 concepts are layered opportunistically.
*
*
* TABLE OF CONTENTS
* =================
* I.    KIND / SCOPE CONSTANTS
* II.   NODE SPEC (the functional face)
* III.  DSL COMBINATORS (free, return node_spec)
* IV.   SUITE BUILDER (the fluent face + hybrid add)
* V.    RUN (the fold driver)
* VI.   FREE CONVENIENCE (anonymous / inline)
*
*
* path:      /inc/djinterp/test/test_builder.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.08
******************************************************************************/

#ifndef DJINTERP_TEST_BUILDER_
#define DJINTERP_TEST_BUILDER_ 1

#ifndef __cplusplus
    #error "test_builder.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/functional/structural_traits.hpp"   // is_nullary_callable
#include "./test_common.hpp"
#include "./test_object.hpp"
#include "./test_tree.hpp"
#include "./test_callable_table.hpp"
#include "./test_handler.hpp"
#include "./test_event.hpp"
#include "./test_defaults.hpp"   // D_TEST_KIND_* + make_* factories


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   KIND / SCOPE CONSTANTS                              ///
///////////////////////////////////////////////////////////////////////////////

// k_kind_session
//   constant: synthetic interior at the top of every suite, one rank
// above module, so that several .test_module calls become siblings
// beneath one implicit session root.  Uses a framework-internal
// (non-negative is user space, so this stays in the reserved band's
// spirit while remaining a valid interior rank above module).
D_STATIC_CONSTEXPR test_type_id k_kind_session = D_TEST_KIND_MODULE + 1;

// k_kind_effect
//   constant: marker kind for an effect node (a .fire / .fire_if_failed
// site).  Negative per test_common's "negative ids are reserved for
// framework-internal use".  Effect nodes carry an effect-table handle in
// their callable_id slot and are skipped by status counting.
D_STATIC_CONSTEXPR test_type_id k_kind_effect = static_cast<test_type_id>(-1);


///////////////////////////////////////////////////////////////////////////////
///                II.  NODE SPEC                                           ///
///////////////////////////////////////////////////////////////////////////////

// test_probe
//   alias: a leaf's deferred evaluation - a nullary callable yielding a
// bool-convertible verdict.  The builder adapts it to the callable table's
// void(test_object&) signature at splice time.
using test_probe = std::function<bool()>;

// node_spec
//   struct: a pure, copyable description of a test subtree - the value the
// functional (dsl::) combinators produce and suite_builder::add consumes.
// Interiors carry children; leaves carry either an eager boolean verdict or
// a deferred probe.  No tree, no handler, no allocation beyond the children
// vector: a node_spec can be built, stored, and combined freely before any
// run exists.
struct node_spec
{
    test_type_id           kind;
    std::string            name;
    bool                   is_leaf;
    bool                   is_eager;      // leaf only: verdict already known
    bool                   eager_value;   // leaf only: the known verdict
    test_probe             probe;         // leaf only: deferred verdict
    std::vector<node_spec> children;

    node_spec()
        : kind(D_TEST_KIND_TEST),
          name(),
          is_leaf(false),
          is_eager(false),
          eager_value(false),
          probe(),
          children()
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                III. DSL COMBINATORS                                     ///
///////////////////////////////////////////////////////////////////////////////
//   Free factories returning node_spec.  These are the functional face:
// module("io", block("flat", test("x", assertion("a", p)))) is a value that
// suite_builder::add splices in one call.  Variadic children are gathered
// through a brace-init of node_spec so heterogeneous nesting reads naturally.

namespace dsl {


// gather_
//   helper: collects a variadic pack of node_spec into a vector.
NS_INTERNAL

    inline void
    gather_(
        std::vector<node_spec>&
    )
    {
        return;
    }

    template<typename... _Rest>
    inline void
    gather_(
        std::vector<node_spec>& _into,
        node_spec               _first,
        _Rest...                _rest
    )
    {
        _into.push_back(static_cast<node_spec&&>(_first));
        internal::gather_(_into, static_cast<_Rest&&>(_rest)...);

        return;
    }

NS_END  // internal


// interior_
//   helper: builds an interior node_spec of the given kind with the given
// children.
template<typename... _Children>
inline node_spec
interior_(
    test_type_id _kind,
    std::string  _name,
    _Children... _children
)
{
    node_spec s;
    s.kind    = _kind;
    s.name    = static_cast<std::string&&>(_name);
    s.is_leaf = false;
    internal::gather_(s.children, static_cast<_Children&&>(_children)...);

    return s;
}

// module
//   combinator: a module-rank interior subtree.
template<typename... _Children>
inline node_spec
module(
    std::string  _name,
    _Children... _children
)
{
    return interior_(D_TEST_KIND_MODULE,
                     static_cast<std::string&&>(_name),
                     static_cast<_Children&&>(_children)...);
}

// block
//   combinator: a test_block-rank interior subtree.
template<typename... _Children>
inline node_spec
block(
    std::string  _name,
    _Children... _children
)
{
    return interior_(D_TEST_KIND_TEST_BLOCK,
                     static_cast<std::string&&>(_name),
                     static_cast<_Children&&>(_children)...);
}

// test
//   combinator: a test-rank interior subtree (one logical test case).
template<typename... _Children>
inline node_spec
test(
    std::string  _name,
    _Children... _children
)
{
    return interior_(D_TEST_KIND_TEST,
                     static_cast<std::string&&>(_name),
                     static_cast<_Children&&>(_children)...);
}

// assertion
//   combinator: a deferred assertion leaf.  _probe is any nullary callable
// whose result is bool-convertible; it is evaluated during the run-fold.
template<typename _Probe,
         typename std::enable_if<
             is_nullary_callable<_Probe>::value, int>::type = 0>
inline node_spec
assertion(
    std::string _name,
    _Probe      _probe
)
{
    node_spec s;
    s.kind     = D_TEST_KIND_ASSERT;
    s.name     = static_cast<std::string&&>(_name);
    s.is_leaf  = true;
    s.is_eager = false;
    s.probe    = test_probe(static_cast<_Probe&&>(_probe));

    return s;
}

// assertion
//   combinator: an eager assertion leaf whose verdict is already a bool.
inline node_spec
assertion(
    std::string _name,
    bool        _value
)
{
    node_spec s;
    s.kind        = D_TEST_KIND_ASSERT;
    s.name        = static_cast<std::string&&>(_name);
    s.is_leaf     = true;
    s.is_eager    = true;
    s.eager_value = _value;

    return s;
}

// test_fn
//   combinator: a deferred test-function leaf.  Accepts a nullary callable
// (including a bool(*)() function pointer) whose result is bool-convertible.
template<typename _Fn,
         typename std::enable_if<
             is_nullary_callable<_Fn>::value, int>::type = 0>
inline node_spec
test_fn(
    std::string _name,
    _Fn         _fn
)
{
    node_spec s;
    s.kind     = D_TEST_KIND_TEST_FN;
    s.name     = static_cast<std::string&&>(_name);
    s.is_leaf  = true;
    s.is_eager = false;
    s.probe    = test_probe(static_cast<_Fn&&>(_fn));

    return s;
}


}  // namespace dsl


///////////////////////////////////////////////////////////////////////////////
///                IV.  SUITE BUILDER                                       ///
///////////////////////////////////////////////////////////////////////////////

// suite_builder
//   class: fluent constructor of a test_tree.  Owns the tree, the deferred-
// callable table, and the effect side table; tracks a scope stack of
// interior nodes.  Every structural method returns *this& so the procedural
// and single-statement idioms are the same object.  add(spec) splices a
// functional node_spec under the current scope for the hybrid idiom.
//
//   The builder is move-only: it owns a test_callable_table (itself move-
// only) so that registered closures and the tree that references them travel
// together to the run-fold.
//
// Template parameters:
//   _Element : the test object element type.  Default basic_test.
//
// Usage (the three idioms):
//   // fluent / single-statement
//   auto rep = suite()
//       .test_module("foo_tests.hpp")
//           .test_block("container properties")
//               .test("is flat, static, mutable")
//                   .assert_([]{ return is_flat; })
//                   .test_fn(&test_flat_static_mutable)
//           .fire_if_failed<events::on_test_failed>()
//       .run(handler);
//
//   // functional
//   suite().add(dsl::module("foo_tests.hpp",
//                  dsl::block("io",
//                      dsl::test("flat",
//                          dsl::assertion("a", []{ return ok; }),
//                          dsl::test_fn("f", &test_flat_static_mutable)))))
//          .run(handler);
//
//   // hybrid
//   suite().test_module("foo_tests.hpp")
//          .test_block("io")
//              .add(dsl::test("flat", dsl::assertion("a", p)))
//          .test("more").assert_(q)
//          .run(handler);
template<typename _Element = basic_test>
class suite_builder
{
public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using element_type   = _Element;
    using tree_type       = test_tree<_Element,
                                      ::djinterp::nary_tree<_Element>,
                                      false>;   // builder enforces nesting itself
    using table_type      = test_callable_table<
                                typename _Element::status_type,
                                typename _Element::id_type>;
    using node_ptr        = decltype(
        std::declval<typename tree_type::underlying_container_type&>().root());
    using run_report      = session_result;
    using effect_fn       = std::function<void(test_handler&,
                                               const run_report&)>;
    using size_type       = std::size_t;


    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // suite_builder
    //   constructor: seeds an implicit session root so that the first
    // .test_module attaches beneath it.  _name labels the session for
    // reports; it defaults to empty.
    explicit suite_builder(
        const char* _name = nullptr
    )
        : m_tree(),
          m_callables(),
          m_effects(1),     // slot 0 reserved (k_no_callable)
          m_cursor()
    {
        _Element root = make_interior(k_kind_session, _name);
        m_tree.underlying().emplace_root(static_cast<_Element&&>(root));
        m_cursor.push_back(m_tree.underlying().root());
    }

    suite_builder(const suite_builder&)            = delete;
    suite_builder& operator=(const suite_builder&) = delete;
    suite_builder(suite_builder&&)                 = default;
    suite_builder& operator=(suite_builder&&)      = default;
    ~suite_builder()                               = default;


    // -----------------------------------------------------------------
    //  structural interiors (rank-driven nesting)
    // -----------------------------------------------------------------

    // test_module
    //   opens a module-rank interior as a sibling beneath the session.
    suite_builder&
    test_module(
        const char* _name
    )
    {
        open_(D_TEST_KIND_MODULE, make_interior(D_TEST_KIND_MODULE, _name));

        return *this;
    }

    // test_block
    //   opens a test_block-rank interior beneath the nearest module.
    suite_builder&
    test_block(
        const char* _name
    )
    {
        open_(D_TEST_KIND_TEST_BLOCK,
              make_interior(D_TEST_KIND_TEST_BLOCK, _name));

        return *this;
    }

    // test
    //   opens a test-rank interior beneath the nearest block.
    suite_builder&
    test(
        const char* _name = nullptr
    )
    {
        open_(D_TEST_KIND_TEST, make_interior(D_TEST_KIND_TEST, _name));

        return *this;
    }


    // -----------------------------------------------------------------
    //  leaves (attach under the current interior; never pushed)
    // -----------------------------------------------------------------

    // assert_
    //   deferred assertion: _probe is a nullary callable evaluated during
    // the run-fold.  Underscored to avoid the assert() macro from <cassert>.
    template<typename _Probe,
             typename std::enable_if<
                 is_nullary_callable<_Probe>::value, int>::type = 0>
    suite_builder&
    assert_(
        _Probe      _probe,
        const char* _name      = nullptr,
        const char* _msg_pass  = nullptr,
        const char* _msg_fail  = nullptr
    )
    {
        attach_probe_leaf_(D_TEST_KIND_ASSERT,
                           test_probe(static_cast<_Probe&&>(_probe)),
                           _name, _msg_pass, _msg_fail);

        return *this;
    }

    // assert_
    //   eager assertion: verdict already known as a bool.
    suite_builder&
    assert_(
        bool        _value,
        const char* _name      = nullptr,
        const char* _msg_pass  = nullptr,
        const char* _msg_fail  = nullptr
    )
    {
        _Element leaf = make_assert(_value, _name, _msg_pass, _msg_fail);
        m_tree.underlying().append_child(m_cursor.back(),
                                         static_cast<_Element&&>(leaf));

        return *this;
    }

    // test_fn
    //   deferred test-function leaf: any nullary callable (including a
    // bool(*)()) whose result is bool-convertible.
    template<typename _Fn,
             typename std::enable_if<
                 is_nullary_callable<_Fn>::value, int>::type = 0>
    suite_builder&
    test_fn(
        _Fn         _fn,
        const char* _name = nullptr
    )
    {
        attach_probe_leaf_(D_TEST_KIND_TEST_FN,
                           test_probe(static_cast<_Fn&&>(_fn)),
                           _name, nullptr, nullptr);

        return *this;
    }


    // -----------------------------------------------------------------
    //  effects (recorded as data; interpreted by the run-fold)
    // -----------------------------------------------------------------

    // fire
    //   records an effect node that dispatches _Event with no payload when
    // the walk reaches it.
    template<typename _Event>
    suite_builder&
    fire()
    {
        attach_effect_([](test_handler& _h, const run_report&)
        {
            _h.template fire<_Event>();

            return;
        });

        return *this;
    }

    // fire
    //   records an effect node that dispatches _Event with one payload,
    // captured by value at build time.
    template<typename _Event,
             typename _Payload>
    suite_builder&
    fire(
        _Payload _payload
    )
    {
        _Payload captured = static_cast<_Payload&&>(_payload);
        attach_effect_([captured](test_handler& _h, const run_report&)
        {
            _h.template fire<_Event>(captured);

            return;
        });

        return *this;
    }

    // fire_if_failed
    //   records an effect node that dispatches _Event only if at least one
    // failure or error has been observed by the run-fold up to this point.
    template<typename _Event>
    suite_builder&
    fire_if_failed()
    {
        attach_effect_([](test_handler& _h, const run_report& _r)
        {
            if (_r.any_failed())
            {
                _h.template fire<_Event>();
            }

            return;
        });

        return *this;
    }

    // fire_if_failed
    //   one-payload form of the conditional dispatch above.
    template<typename _Event,
             typename _Payload>
    suite_builder&
    fire_if_failed(
        _Payload _payload
    )
    {
        _Payload captured = static_cast<_Payload&&>(_payload);
        attach_effect_([captured](test_handler& _h, const run_report& _r)
        {
            if (_r.any_failed())
            {
                _h.template fire<_Event>(captured);
            }

            return;
        });

        return *this;
    }


    // -----------------------------------------------------------------
    //  composition (functional + conditional)
    // -----------------------------------------------------------------

    // add
    //   splices a node_spec subtree beneath the CURRENT interior without
    // moving the scope cursor, so a functional spec drops cleanly into a
    // fluent chain (the hybrid idiom).
    suite_builder&
    add(
        const node_spec& _spec
    )
    {
        splice_(m_cursor.back(), _spec);

        return *this;
    }

    // add_if
    //   invokes _build(*this) only when _cond is true, letting a chain grow
    // a conditional subtree inline.  _build is any callable taking
    // suite_builder&.
    //
    //   NOTE: the sketch's single-lambda spelling is ambiguous; this is the
    // (condition, builder-fn) reading.  A predicate-valued overload can be
    // layered once the intended signature is settled.
    template<typename _BuildFn>
    suite_builder&
    add_if(
        bool     _cond,
        _BuildFn _build
    )
    {
        if (_cond)
        {
            _build(*this);
        }

        return *this;
    }


    // -----------------------------------------------------------------
    //  accessors
    // -----------------------------------------------------------------

    // tree
    //   returns the constructed tree (mutable).
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
    //   returns the deferred-callable table backing this builder's leaves.
    table_type&
    callables() D_NOEXCEPT
    {
        return m_callables;
    }


    // -----------------------------------------------------------------
    //  run (the fold driver) - see section V for the free form
    // -----------------------------------------------------------------

    // run
    //   folds the constructed tree, evaluating deferred leaves, interpreting
    // effects, and firing lifecycle events through _handler (used purely as
    // a sink).  Returns the accumulated run_report.
    run_report
    run(
        test_handler& _handler
    );


private:
    // -----------------------------------------------------------------
    //  nesting depth (distinct from structural rank/type_id)
    // -----------------------------------------------------------------

    // nest_depth_
    //   maps a kind to its nesting depth.  Both leaf kinds and effect
    // markers share depth 0 so they sit as siblings under the enclosing
    // interior; test=1, block=2, module=3, session=4.  Only the non-root
    // values are consulted by open_ (the size>1 guard pins the root).
    static D_CONSTEXPR int
    nest_depth_(
        test_type_id _kind
    ) D_NOEXCEPT
    {
        return ( (_kind == k_kind_session)
            ?  4
            :  ( (_kind == D_TEST_KIND_MODULE)
                 ?  3
                 :  ( (_kind == D_TEST_KIND_TEST_BLOCK)
                      ?  2
                      :  ( (_kind == D_TEST_KIND_TEST)
                           ?  1
                           :  0 ) ) ) );
    }

    // open_
    //   pops the scope stack to the nearest strictly-higher-depth interior,
    // attaches _node beneath it, and pushes _node as the new scope.  The
    // size>1 guard keeps the session root permanently at the floor.
    void
    open_(
        test_type_id _kind,
        _Element     _node
    )
    {
        int depth = nest_depth_(_kind);

        // pop every same-or-deeper interior so _node nests one level up
        while ( (m_cursor.size() > 1) &&
                (nest_depth_(m_cursor.back()->data().type_id()) <= depth) )
        {
            m_cursor.pop_back();
        }

        node_ptr placed =
            m_tree.underlying().append_child(m_cursor.back(),
                                             static_cast<_Element&&>(_node));
        m_cursor.push_back(placed);

        return;
    }

    // attach_probe_leaf_
    //   registers _probe into the callable table, builds a leaf of _kind
    // carrying the resulting handle, and attaches it under the current
    // interior.
    void
    attach_probe_leaf_(
        test_type_id _kind,
        test_probe   _probe,
        const char*  _name,
        const char*  _msg_pass,
        const char*  _msg_fail
    )
    {
        _Element leaf = (_kind == D_TEST_KIND_ASSERT)
            ? make_assert(false, _name, _msg_pass, _msg_fail)
            : make_test_fn(false, _name);

        leaf.set_callable_id(register_probe_(static_cast<test_probe&&>(_probe)));

        m_tree.underlying().append_child(m_cursor.back(),
                                         static_cast<_Element&&>(leaf));

        return;
    }

    // attach_effect_
    //   registers _fn into the effect table, builds an effect marker node
    // carrying the handle, and attaches it under the current interior.
    void
    attach_effect_(
        effect_fn _fn
    )
    {
        _Element node = make_interior(k_kind_effect, nullptr);
        node.set_callable_id(register_effect_(static_cast<effect_fn&&>(_fn)));

        m_tree.underlying().append_child(m_cursor.back(),
                                         static_cast<_Element&&>(node));

        return;
    }

    // splice_
    //   recursively materialises a node_spec subtree beneath _parent.  Leaf
    // specs register their probe (or stamp their eager verdict); interior
    // specs recurse over children.  Does NOT touch the scope cursor.
    node_ptr
    splice_(
        node_ptr         _parent,
        const node_spec& _spec
    )
    {
        _Element obj = node_from_spec_(_spec);
        node_ptr placed =
            m_tree.underlying().append_child(_parent,
                                             static_cast<_Element&&>(obj));

        // interiors recurse; leaves terminate
        std::size_t i;
        for (i = 0; i < _spec.children.size(); ++i)
        {
            splice_(placed, _spec.children[i]);
        }

        return placed;
    }

    // node_from_spec_
    //   builds the _Element for a node_spec, registering a deferred probe
    // into the callable table when the spec carries one.
    _Element
    node_from_spec_(
        const node_spec& _spec
    )
    {
        const char* name = _spec.name.empty() ? nullptr : _spec.name.c_str();

        if (!_spec.is_leaf)
        {
            return make_interior(_spec.kind, name);
        }

        // leaf: eager verdict or deferred probe
        _Element leaf = (_spec.kind == D_TEST_KIND_ASSERT)
            ? make_assert(_spec.is_eager ? _spec.eager_value : false, name)
            : make_test_fn(_spec.is_eager ? _spec.eager_value : false, name);

        if (!_spec.is_eager)
        {
            leaf.set_callable_id(register_probe_(_spec.probe));
        }

        return leaf;
    }

    // register_probe_
    //   wraps a nullary test_probe in the callable table's
    // void(test_object&) signature and registers it, returning the handle.
    // The wrapper writes the boolean verdict back onto the node's result /
    // status during the run-fold.
    test_callable_id
    register_probe_(
        test_probe _probe
    )
    {
        test_probe p = static_cast<test_probe&&>(_probe);

        return m_callables.register_callable(
            [p](typename table_type::test_object_type& _self)
            {
                // evaluate() sets result + status together (passed/failed),
                // which is exactly a probe's pass/fail verdict; it is the
                // element type's intended mutator and keeps this wrapper
                // valid for any element that satisfies the test-object
                // protocol, not just the flat basic_test layout.
                _self.evaluate(static_cast<bool>(p()));

                return;
            });
    }

    // register_effect_
    //   appends _fn to the effect table and returns its handle (dense,
    // starting at 1; slot 0 is the reserved k_no_callable sentinel).
    test_callable_id
    register_effect_(
        effect_fn _fn
    )
    {
        m_effects.push_back(static_cast<effect_fn&&>(_fn));

        return static_cast<test_callable_id>(m_effects.size() - 1);
    }


    // -----------------------------------------------------------------
    //  run helpers
    // -----------------------------------------------------------------

    // fold_ / to_status_ / bump_ are defined out-of-line immediately after
    // the class (section V) so the class body stays a readable surface.  The
    // status-event dispatch lives in a free helper (internal::
    // fire_status_event_) shared with the free expect() check, so there is a
    // single dispatch implementation; see section V.
    void           fold_(node_ptr _node, test_handler& _h, run_report& _rep);
    static test_status to_status_(typename _Element::status_type _raw) D_NOEXCEPT;
    static void    bump_(run_report& _rep, test_status _st) D_NOEXCEPT;


    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    tree_type             m_tree;
    table_type            m_callables;
    std::vector<effect_fn> m_effects;
    std::vector<node_ptr>  m_cursor;   // interior scopes, root..current
};


///////////////////////////////////////////////////////////////////////////////
///                V.   RUN (the fold driver)                               ///
///////////////////////////////////////////////////////////////////////////////
//   The reducer body.  fold_ is a pre-order walk applying the per-node step
// (run_report, node) -> run_report.  Leaves are evaluated (deferred bodies
// invoked through the callable table) and tallied; effects are interpreted;
// interiors recurse, with module entry/exit events bracketed.  A flat,
// constexpr-only suite could run this same step under reduce_ct - the step
// is written as a function of (report, node) precisely so that the driver
// (recursion vs. reduce_ct) is the only thing that differs.

// suite_builder::to_status_
//   maps the element's numeric status onto the framework test_status enum.
template<typename _Element>
test_status
suite_builder<_Element>::to_status_(
    typename _Element::status_type _raw
) D_NOEXCEPT
{
    switch (_raw)
    {
        case _Element::status_passed:  return test_status::passed;
        case _Element::status_failed:  return test_status::failed;
        case _Element::status_skipped: return test_status::skipped;
        case _Element::status_pending: return test_status::pending;
        case _Element::status_error:   return test_status::error;
        default:                       return test_status::pending;
    }
}

// suite_builder::bump_
//   advances the run_report counters for one observed status.
template<typename _Element>
void
suite_builder<_Element>::bump_(
    run_report& _rep,
    test_status _st
) D_NOEXCEPT
{
    switch (_st)
    {
        case test_status::passed:  ++_rep.passed;  break;
        case test_status::failed:  ++_rep.failed;  break;
        case test_status::skipped: ++_rep.skipped; break;
        case test_status::error:   ++_rep.errors;  break;
        case test_status::pending: ++_rep.pending; break;
    }

    ++_rep.total;

    return;
}

// internal::fire_status_event_
//   helper: dispatches the status-specific leaf event through _h, gated by
// listener presence so the no-listener path stays free.  Lifted out of
// suite_builder so the free expect() check (section VI) can reuse the exact
// same dispatch without reaching into the class.
NS_INTERNAL

template<typename _Element>
inline void
fire_status_event_(
    test_handler&   _h,
    const _Element* _obj,
    test_status     _st
)
{
    switch (_st)
    {
        case test_status::passed:
            if (_h.has_listeners_for<events::on_test_passed>())
            {
                _h.fire<events::on_test_passed>(_obj);
            }
            break;

        case test_status::failed:
            if (_h.has_listeners_for<events::on_test_failed>())
            {
                _h.fire<events::on_test_failed>(_obj);
            }
            break;

        case test_status::skipped:
            if (_h.has_listeners_for<events::on_test_skipped>())
            {
                _h.fire<events::on_test_skipped>(_obj);
            }
            break;

        case test_status::error:
            if (_h.has_listeners_for<events::on_test_error>())
            {
                _h.fire<events::on_test_error>(
                    _obj, static_cast<const char*>(nullptr));
            }
            break;

        case test_status::pending:
            break;
    }

    return;
}

NS_END  // internal

// suite_builder::fold_
//   the pre-order interpreter (the driver half of the split).
template<typename _Element>
void
suite_builder<_Element>::fold_(
    node_ptr      _node,
    test_handler& _h,
    run_report&   _rep
)
{
    if (_node == nullptr)
    {
        return;
    }

    _Element&    data = _node->data();
    test_type_id id   = data.type_id();

    // effect marker: interpret and stop (effects carry no children)
    if (id == k_kind_effect)
    {
        test_callable_id eid = data.callable_id();
        if ( (eid != k_no_callable) &&
             (static_cast<std::size_t>(eid) < m_effects.size()) )
        {
            m_effects[static_cast<std::size_t>(eid)](_h, _rep);
        }

        return;
    }

    // leaf: evaluate (deferred body if any), tally, fire status event
    if ( (id >= test_type_id{0}) &&
         (id <= D_TEST_KIND_TEST_FN) )
    {
        if ( data.has_callable() &&
             m_callables.contains(data.callable_id()) )
        {
            m_callables[data.callable_id()](data);
        }

        test_status st = to_status_(data.status());
        bump_(_rep, st);
        internal::fire_status_event_(_h, &data, st);

        return;
    }

    // interior: bracket modules, recurse children
    bool is_module = (id == D_TEST_KIND_MODULE);

    if ( is_module &&
         _h.has_listeners_for<events::on_module_start>() )
    {
        _h.fire<events::on_module_start>(&data);
    }

    for (node_ptr child = _node->first_child();
         child != nullptr;
         child = child->next_sibling())
    {
        fold_(child, _h, _rep);
    }

    if ( is_module &&
         _h.has_listeners_for<events::on_module_end>() )
    {
        _h.fire<events::on_module_end>(&data);
    }

    return;
}

// suite_builder::run
//   session-bracketed fold over the whole tree.
template<typename _Element>
typename suite_builder<_Element>::run_report
suite_builder<_Element>::run(
    test_handler& _handler
)
{
    run_report rep;

    if (_handler.has_listeners_for<events::on_session_start>())
    {
        _handler.fire<events::on_session_start>();
    }

    fold_(m_tree.underlying().root(), _handler, rep);

    if (_handler.has_listeners_for<events::on_session_end>())
    {
        _handler.fire<events::on_session_end>(rep.passed, rep.failed);
    }

    return rep;
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  FREE CONVENIENCE                                    ///
///////////////////////////////////////////////////////////////////////////////

// suite
//   factory: a fresh suite_builder over basic_test.  The entry point for the
// fluent and hybrid idioms: suite().test_module(...)... .
inline suite_builder<basic_test>
suite(
    const char* _name = nullptr
)
{
    return suite_builder<basic_test>(_name);
}

// expect
//   anonymous inline check: evaluates _probe once, fires the matching status
// event through _handler, and returns the boolean verdict.  No tree is built
// - this is the one-off assertion that does not belong to a named module.
template<typename _Probe,
         typename std::enable_if<
             is_nullary_callable<_Probe>::value, int>::type = 0>
inline bool
expect(
    test_handler& _handler,
    _Probe        _probe,
    const char*   _name = nullptr
)
{
    basic_test  t  = make_assert(static_cast<bool>(_probe()), _name);
    test_status st = t.m_result ? test_status::passed
                                : test_status::failed;

    internal::fire_status_event_(_handler, &t, st);

    return t.m_result;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_BUILDER_
