/******************************************************************************
* djinterp [test]                                       test_callable_table.hpp
*
*   Side-table of deferred-evaluation callables for the DTest framework.
*
*   PURPOSE:
*   test_object stores per-node test data inline (result, status,
* name, message), but to keep test_object trivially copyable and
* constexpr-friendly, it does NOT store callables (closures or
* function pointers with bound state) directly.  Instead, every
* test_object can carry a `test_callable_id` - an opaque integer
* handle that indexes into a separate table of callables owned
* by the test session.
*
*   This indirection enables the LAZY EXECUTION pattern: subtree
* construction registers the closure into the table and stores
* only its id on the node; the actual test work happens during
* the tree walk, when the handler looks up the id and invokes
* the bound callable.  This decouples expensive test bodies
* (concurrency, I/O, multi-thread spawn) from the eager
* combine_subtrees / make_*_subtree path, so the printer can
* stream output as each test executes rather than only after
* the whole suite has finished building.
*
*   USAGE:
*
*     test_callable_table  table;
*     array_test_tree      tree;
*
*     // builder phase: cheap registrations
*     test_callable_id id = table.register_callable(
*         [](test_object& self) {
*             // expensive work here, runs DURING the walk
*             bool ok = ...;
*             self.m_result = ok;
*             self.m_status = ok ? test_object<>::status_passed
*                                : test_object<>::status_failed;
*         });
*
*     // attach the id to a leaf in the tree
*     test_object obj(D_TEST_KIND_ASSERT, false, "expensive_test");
*     obj.set_callable_id(id);
*     tree.underlying().append_child(parent_node, obj);
*
*     // ... build more subtrees, register more callables ...
*
*     // run phase: handler is given a pointer to the table.
*     handler.set_callable_table(&table);
*     handler.run(tree.underlying());   // each leaf's closure
*                                       // fires during the walk
*
*   THREAD SAFETY:
*   The table itself is NOT thread-safe.  Registration is
* expected to happen single-threaded during builder/setup, and
* lookup is expected to happen single-threaded during the walk
* (the handler does NOT walk concurrently).  Individual
* callables are free to spawn threads internally; the framework
* makes no guarantees about cross-callable parallelism.
*
*   LIFETIME:
*   The table must outlive every tree that references it AND
* every handler walk that consults it.  The natural owner is
* the test_session; for ad-hoc setups, a local table whose
* scope encloses every handler.run() call is sufficient.
*
*   ID ALLOCATION:
*   Ids are dense, monotonically increasing, starting at 1.
* The reserved id 0 (k_no_callable) is never returned by
* register_callable - that value is the "no callable bound"
* sentinel.  Tables can hold up to ~4 billion entries before
* the id space exhausts; in practice no test suite approaches
* that limit.
*
*   CONSTEXPR / PORTABILITY:
*   Not constexpr.  Storage is a std::vector<std::function>;
* requires C++11.  All accessors are noexcept where the
* underlying std container's noexcept guarantees permit.
*
*
* path:      /inc/djinterp/test/test_callable_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.05
******************************************************************************/

#ifndef DJINTERP_TEST_CALLABLE_TABLE_
#define DJINTERP_TEST_CALLABLE_TABLE_ 1

// std
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"
#include "./test_object.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   CALLABLE SIGNATURE                                  ///
///////////////////////////////////////////////////////////////////////////////

// test_callable_signature
//   alias: the function-call signature every closure stored
// in a test_callable_table must satisfy.  The closure
// receives a mutable reference to the test_object whose
// m_callable_id selected it; the closure mutates that
// object directly to record the test's outcome
// (typically m_result, m_status, optionally m_message and
// m_name).
//
//   The signature deliberately does NOT take a test_handler
// reference.  Callables exist to deliver the *value* of a
// test, not to drive structural events; event firing is
// the handler's job.  If a callable needs to mutate the
// surrounding tree (e.g. dynamically expand children),
// that is out of scope for this primitive.
template<typename _StatusType = std::uint8_t,
         typename _IdType     = std::uint32_t>
using test_callable_signature =
    std::function<void(test_object<_StatusType, _IdType>&)>;


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST CALLABLE TABLE                                 ///
///////////////////////////////////////////////////////////////////////////////

// test_callable_table
//   class: side-table of deferred-evaluation callables.
//
//   The table is a peer to test_handler / test_session in
// the framework's runtime layer.  It is owned by a session
// (or by a local scope around a handler.run() invocation)
// and is referenced - never owned - by the handler during
// walks.
//
//   Ids are dense and start at 1.  Lookup is O(1) via
// vector indexing; entry zero is reserved for the
// k_no_callable sentinel and is never populated.
//
// Template parameters:
//   _StatusType : the status type used by the test_object
//                 the table's callables operate on.  Must
//                 match the test_object specialization
//                 used by the tree.
//   _IdType     : ditto for test_object's id type.
//
// Usage:
//   test_callable_table<>  table;
//   auto id = table.register_callable(my_closure);
//   table[id](some_test_object);   // invoke
template<typename _StatusType = std::uint8_t,
         typename _IdType     = std::uint32_t>
class test_callable_table
{
public:
    // ── type aliases ────────────────────────────────────────────────
    using test_object_type = test_object<_StatusType, _IdType>;
    using callable_type    =
        test_callable_signature<_StatusType, _IdType>;
    using size_type        = std::size_t;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // test_callable_table
    //   constructor: starts with one reserved entry at index
    // 0 so register_callable's first allocated id is 1.  This
    // keeps id 0 as the "no callable" sentinel without
    // requiring any subtraction or offset arithmetic at
    // lookup time.
    test_callable_table()
        : m_callables(1)
    {}

    test_callable_table(const test_callable_table&)            = delete;
    test_callable_table& operator=(const test_callable_table&) = delete;

    test_callable_table(test_callable_table&&)            = default;
    test_callable_table& operator=(test_callable_table&&) = default;

    ~test_callable_table() = default;

    // -----------------------------------------------------------------
    //  registration
    // -----------------------------------------------------------------

    // register_callable
    //   allocates a fresh id, moves _fn into the table at
    // that slot, and returns the new id.
    //
    //   The returned id is suitable for assignment to
    // test_object::set_callable_id().  Ids are stable for
    // the lifetime of the table; subsequent registrations
    // do not invalidate prior ids.
    //
    //   Storage is std::vector<std::function>; if the
    // vector reallocates on push_back, the addresses of
    // existing function objects move, but their ids do
    // not, so handles remain valid.
    test_callable_id
    register_callable(
        callable_type _fn
    )
    {
        m_callables.push_back(static_cast<callable_type&&>(_fn));

        // m_callables.size() is always >= 2 here (1 reserved
        // + 1 just pushed), so the cast to test_callable_id
        // never produces zero and the result is always a
        // valid handle.
        return static_cast<test_callable_id>(
            m_callables.size() - 1);
    }

    // -----------------------------------------------------------------
    //  lookup
    // -----------------------------------------------------------------

    // operator[]
    //   returns a reference to the callable at id _id.
    // Behavior is undefined if _id is k_no_callable or
    // exceeds size().  Out-of-range cases indicate a
    // programming error in the test runner; debug builds
    // may assert.
    const callable_type&
    operator[](
        test_callable_id _id
    ) const D_NOEXCEPT
    {
        return m_callables[static_cast<size_type>(_id)];
    }

    // contains
    //   returns true if _id refers to a registered
    // callable in this table.  k_no_callable always
    // returns false.
    bool
    contains(
        test_callable_id _id
    ) const D_NOEXCEPT
    {
        return ( (_id != k_no_callable) &&
                 (static_cast<size_type>(_id) <
                  m_callables.size()) );
    }

    // -----------------------------------------------------------------
    //  capacity
    // -----------------------------------------------------------------

    // size
    //   returns the number of REGISTERED callables in this
    // table (excluding the reserved zero slot).
    size_type
    size() const D_NOEXCEPT
    {
        // m_callables holds 1 reserved + N registered;
        // the public count is N.
        return (m_callables.size() > 0)
            ? (m_callables.size() - 1)
            : 0;
    }

    // empty
    //   returns true if no callables have been registered
    // (only the reserved slot is present).
    bool
    empty() const D_NOEXCEPT
    {
        return (size() == 0);
    }

    // clear
    //   discards every registered callable and resets the
    // id counter; leaves only the reserved zero slot.
    // Existing test_object instances that hold ids into
    // this table become DANGLING after clear() - the table
    // makes no attempt to track or invalidate them.
    void
    clear()
    {
        m_callables.clear();
        m_callables.resize(1);

        return;
    }

private:
    // m_callables
    //   index 0 is reserved (never invoked); index 1..N
    // hold registered callables.  Using a vector means
    // lookup is O(1) and ids are dense.  The reserved
    // zero slot's std::function default-constructs to the
    // "empty" state, so accidental invocation throws
    // std::bad_function_call - a clean diagnostic if a
    // node with k_no_callable somehow reaches the
    // dispatch path.
    std::vector<callable_type> m_callables;
};


///////////////////////////////////////////////////////////////////////////////
///                III. CONVENIENCE ALIAS                                   ///
///////////////////////////////////////////////////////////////////////////////

// basic_callable_table
//   alias: the table type matched to basic_test (the
// default test_object specialization).  Almost every
// caller wants this; the template is provided for the
// rare custom-test_object case.
using basic_callable_table = test_callable_table<>;


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_CALLABLE_TABLE_
