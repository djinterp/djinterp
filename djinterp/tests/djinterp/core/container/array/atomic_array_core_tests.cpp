/******************************************************************************
* djinterp [testing]                               atomic_array_core_tests.cpp
*
*   Implementation of the atomic_array portion of Part B (threadsafe
* wrappers) of the array test suite under the return-a-subtree
* protocol declared in array_tests.hpp.
*
*   STRUCTURE:
*   Every category function below builds and returns a self-
* contained `array_test_tree` (the project's
* `test::test_tree` overlay backed by `djinterp::nary_tree`).
* Tests do NOT take a test_handler or a test_printer; they do
* NOT mutate any caller-supplied sink; they simply construct
* a small tree and return it by value.
*
*   This translation unit owns the atomic_array strategy: lock-free
* per-element std::atomic<T> storage with no locking.  Cross-
* container concerns (axis preservation across all three wrappers,
* strategy-tag mutual disjointness, and shared edge cases) live in
* threadsafe_array_core_tests.cpp; the suite-wide aggregate builder
* and master runner also live there and call into the sub-builder
* exposed at the bottom of this file.
*
*
* path:      /tests/djinterp/core/container/array/
*                 atomic_array_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.03
******************************************************************************/
#include "./array_tests.hpp"

// std
#include <chrono>           // steady_clock + duration in run_*_suite
#include <cstdint>          // std::uint64_t
#include <type_traits>      // is_*_constructible
#include <utility>          // std::move

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>       // std::atomic, memory_order
    #include <thread>       // std::thread for concurrent tests
    #include <vector>       // std::vector for thread aggregation
#endif


NS_DJINTERP
NS_TESTING


using namespace djinterp::test;


// =========================================================================
// I.   FILE-INTERNAL HELPERS
// =========================================================================
//   The eager `append_leaf` helper lives in `array_tests.hpp`
// so that every translation unit in the suite shares one
// definition.  Defining it again here would shadow the header
// version with an identical signature and produce ambiguous-
// call diagnostics at every assertion site, which is what the
// previous revision of this file did.

namespace {

    // make_block_tree
    //   helper: constructs a fresh array_test_tree whose
    // root is a test_block-kind interior node carrying
    // _block_name.
    inline array_test_tree
    make_block_tree(
        const char* _block_name
    )
    {
        array_test_tree result;

        result.underlying().emplace_root(
            test::make_test_block(_block_name));

        return result;
    }


    // -------------------------------------------------------------------
    //  cube-cell aliases
    //
    //   This translation unit only needs the atomic_array alias
    // and the base-array alias used to confirm static extent on
    // the wrapped type.
    // -------------------------------------------------------------------

    template<typename T, std::size_t N>
    using at_arr = djinterp::atomic_array<T, N>;

    template<typename T, std::size_t N>
    using base_mi = djinterp::mutable_iterable_array<T, N>;


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // run_threads
    //   helper: spawns _count threads, each invoking _worker(tid),
    // and joins them.  Used by the concurrent tests below.
    template<typename _Worker>
    inline void
    run_threads(
        std::size_t _count,
        _Worker     _worker
    )
    {
        std::vector<std::thread> threads;
        threads.reserve(_count);

        for (std::size_t i = 0; i < _count; ++i)
        {
            threads.emplace_back(_worker, i);
        }

        for (std::size_t i = 0; i < _count; ++i)
        {
            threads[i].join();
        }

        return;
    }

#endif  // C++11

}  // unnamed namespace


// =========================================================================
// II.  CATEGORY: ATOMIC_ARRAY TRAIT CONFORMANCE
// =========================================================================

array_test_tree
test_atomic_array_traits_strategy_atomic(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "trait conformance: atomic_array -> atomic strategy");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        djinterp::is_atomic_container<at_arr<int, 4>>::value,
        "is_atomic_container<atomic_array>::value == true");
    append_leaf(tree, root,
        !djinterp::is_atomic_container<base_mi<int, 4>>::value,
        "is_atomic_container<plain array>::value == false");
    append_leaf(tree, root,
        !djinterp::is_locked_container<at_arr<int, 4>>::value,
        "atomic_array NOT classified as locked");
    append_leaf(tree, root,
        !djinterp::is_cow_container<at_arr<int, 4>>::value,
        "atomic_array NOT classified as cow");

    // strategy tag check
    append_leaf(tree, root,
        std::is_same<
            typename at_arr<int, 4>::concurrency_strategy_tag,
            djinterp::atomic_strategy_tag>::value,
        "atomic_array::concurrency_strategy_tag == "
        "atomic_strategy_tag");

    return tree;
}

// =========================================================================
// XIII. CATEGORY: ATOMIC_ARRAY CONSTRUCTION
// =========================================================================

array_test_tree
test_atomic_array_default_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: default construction zero-initializes");
    auto* root = tree.underlying().root();

    at_arr<int, 8> a;

    append_leaf(tree, root,
        a.size() == 8,
        "default-constructed: size() == 8");

    // every slot must read 0 since the ctor stores
    // _T{} into every slot
    bool all_zero = true;

    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (a.load(i) != 0)
        {
            all_zero = false;
            break;
        }
    }

    append_leaf(tree, root,
        all_zero,
        "default-constructed: every slot == 0");

    return tree;
}


array_test_tree
test_atomic_array_fill_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: fill construction");
    auto* root = tree.underlying().root();

    at_arr<int, 6> a(42);

    bool all_42 = true;

    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (a.load(i) != 42)
        {
            all_42 = false;
            break;
        }
    }

    append_leaf(tree, root,
        all_42,
        "fill ctor: every slot == 42");

    // explicit fill ctor is not implicitly convertible
    // (it's marked explicit)
    append_leaf(tree, root,
        !std::is_convertible<int, at_arr<int, 6>>::value,
        "fill ctor is explicit (not implicit-convertible)");

    return tree;
}


array_test_tree
test_atomic_array_copy_move_deletion_sfinae(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: copy / move correctly deleted");
    auto* root = tree.underlying().root();

    using aa = at_arr<int, 4>;

    // std::atomic<T> is non-copyable and non-movable;
    // atomic_array inherits that constraint.
    append_leaf(tree, root,
        !std::is_copy_constructible<aa>::value,
        "is_copy_constructible<atomic_array> == false");
    append_leaf(tree, root,
        !std::is_copy_assignable<aa>::value,
        "is_copy_assignable<atomic_array> == false");
    append_leaf(tree, root,
        !std::is_move_constructible<aa>::value,
        "is_move_constructible<atomic_array> == false");
    append_leaf(tree, root,
        !std::is_move_assignable<aa>::value,
        "is_move_assignable<atomic_array> == false");

    return tree;
}


// =========================================================================
// XIV. CATEGORY: ATOMIC_ARRAY ELEMENT ACCESS
// =========================================================================

array_test_tree
test_atomic_array_load_store(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: load/store round-trip");
    auto* root = tree.underlying().root();

    at_arr<int, 4> a;

    a.store(0, 11);
    a.store(1, 22);
    a.store(2, 33);
    a.store(3, 44);

    append_leaf(tree, root,
        ( a.load(0) == 11 && a.load(1) == 22 &&
          a.load(2) == 33 && a.load(3) == 44 ),
        "store/load: values round-trip per-slot");

    // overwrite
    a.store(2, 999);
    append_leaf(tree, root,
        a.load(2) == 999,
        "store: overwrites previous value");

    // load on const ref
    const at_arr<int, 4>& cref = a;
    append_leaf(tree, root,
        cref.load(0) == 11,
        "load() callable on const reference");

    return tree;
}


array_test_tree
test_atomic_array_exchange(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: exchange returns prior value");
    auto* root = tree.underlying().root();

    at_arr<int, 4> a;
    a.store(0, 100);

    const int prior = a.exchange(0, 200);

    append_leaf(tree, root,
        prior == 100,
        "exchange returns the value present before the swap");

    append_leaf(tree, root,
        a.load(0) == 200,
        "exchange replaces the slot's value");

    return tree;
}


array_test_tree
test_atomic_array_memory_orderings(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: every memory_order accepted");
    auto* root = tree.underlying().root();

    at_arr<int, 4> a;

    // smoke test each memory_order with a load/store
    a.store(0, 1, std::memory_order_relaxed);
    append_leaf(tree, root,
        a.load(0, std::memory_order_relaxed) == 1,
        "relaxed: store/load round-trip");

    a.store(0, 2, std::memory_order_release);
    append_leaf(tree, root,
        a.load(0, std::memory_order_acquire) == 2,
        "release/acquire: store/load round-trip");

    a.store(0, 3, std::memory_order_seq_cst);
    append_leaf(tree, root,
        a.load(0, std::memory_order_seq_cst) == 3,
        "seq_cst: store/load round-trip");

    return tree;
}


array_test_tree
test_atomic_array_slot_independence(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: slots are independent");
    auto* root = tree.underlying().root();

    at_arr<int, 8> a;

    // store distinct values in each slot, verify no
    // cross-talk
    for (std::size_t i = 0; i < 8; ++i)
    {
        a.store(i, static_cast<int>(i * 100));
    }

    bool all_correct = true;
    for (std::size_t i = 0; i < 8; ++i)
    {
        if (a.load(i) != static_cast<int>(i * 100))
        {
            all_correct = false;
            break;
        }
    }

    append_leaf(tree, root,
        all_correct,
        "stores into slot i do not affect slot j (j != i)");

    // mutating one slot leaves others untouched
    a.store(3, 99999);

    append_leaf(tree, root,
        ( a.load(0) == 0 && a.load(1) == 100 &&
          a.load(2) == 200 && a.load(3) == 99999 &&
          a.load(4) == 400 && a.load(5) == 500 &&
          a.load(6) == 600 && a.load(7) == 700 ),
        "slot 3 mutation leaves all others unchanged");

    return tree;
}


// =========================================================================
// XV.  CATEGORY: ATOMIC_ARRAY ELEMENT UPDATES
// =========================================================================

array_test_tree
test_atomic_array_fetch_add(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: fetch_add returns prior, increments");
    auto* root = tree.underlying().root();

    at_arr<int, 2> a;

    a.store(0, 10);

    const int prior = a.fetch_add(0, 5);

    append_leaf(tree, root,
        prior == 10,
        "fetch_add returns the slot's value before the add");
    append_leaf(tree, root,
        a.load(0) == 15,
        "fetch_add updates the slot value");

    // chained
    a.fetch_add(0, 100);
    append_leaf(tree, root,
        a.load(0) == 115,
        "fetch_add accumulates correctly");

    return tree;
}


array_test_tree
test_atomic_array_fetch_sub(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: fetch_sub returns prior, decrements");
    auto* root = tree.underlying().root();

    at_arr<int, 2> a;

    a.store(0, 100);
    const int prior = a.fetch_sub(0, 30);

    append_leaf(tree, root,
        prior == 100,
        "fetch_sub returns prior value");
    append_leaf(tree, root,
        a.load(0) == 70,
        "fetch_sub decrements correctly");

    return tree;
}


array_test_tree
test_atomic_array_fetch_and(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: fetch_and bitwise");
    auto* root = tree.underlying().root();

    at_arr<unsigned, 2> a;

    a.store(0, 0xFFu);
    const unsigned prior = a.fetch_and(0, 0x0Fu);

    append_leaf(tree, root,
        prior == 0xFFu,
        "fetch_and returns prior value");
    append_leaf(tree, root,
        a.load(0) == 0x0Fu,
        "fetch_and clears upper bits");

    return tree;
}


array_test_tree
test_atomic_array_fetch_or(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: fetch_or bitwise");
    auto* root = tree.underlying().root();

    at_arr<unsigned, 2> a;

    a.store(0, 0x10u);
    const unsigned prior = a.fetch_or(0, 0x01u);

    append_leaf(tree, root,
        prior == 0x10u,
        "fetch_or returns prior value");
    append_leaf(tree, root,
        a.load(0) == 0x11u,
        "fetch_or sets bits without clearing");

    return tree;
}


array_test_tree
test_atomic_array_fetch_xor(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: fetch_xor bitwise");
    auto* root = tree.underlying().root();

    at_arr<unsigned, 2> a;

    a.store(0, 0xFFu);
    const unsigned prior = a.fetch_xor(0, 0x0Fu);

    append_leaf(tree, root,
        prior == 0xFFu,
        "fetch_xor returns prior value");
    append_leaf(tree, root,
        a.load(0) == 0xF0u,
        "fetch_xor toggles correct bits");

    // double xor returns to original
    a.fetch_xor(0, 0x0Fu);
    append_leaf(tree, root,
        a.load(0) == 0xFFu,
        "fetch_xor twice with same mask is identity");

    return tree;
}


// =========================================================================
// XVI. CATEGORY: ATOMIC_ARRAY ELEMENT CAS
// =========================================================================

array_test_tree
test_atomic_array_cas_strong_success(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: compare_exchange_strong success");
    auto* root = tree.underlying().root();

    at_arr<int, 2> a;
    a.store(0, 7);

    int  expected = 7;
    bool ok = a.compare_exchange_strong(0, expected, 99);

    append_leaf(tree, root,
        ok,
        "CAS strong: returns true when expected matches");
    append_leaf(tree, root,
        expected == 7,
        "CAS strong success: _expected unchanged");
    append_leaf(tree, root,
        a.load(0) == 99,
        "CAS strong success: slot updated to desired");

    return tree;
}


array_test_tree
test_atomic_array_cas_strong_failure(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: compare_exchange_strong failure");
    auto* root = tree.underlying().root();

    at_arr<int, 2> a;
    a.store(0, 7);

    int  expected = 5;  // wrong
    bool ok = a.compare_exchange_strong(0, expected, 99);

    append_leaf(tree, root,
        !ok,
        "CAS strong: returns false when expected mismatches");
    append_leaf(tree, root,
        expected == 7,
        "CAS strong failure: _expected updated to "
        "actual observed value");
    append_leaf(tree, root,
        a.load(0) == 7,
        "CAS strong failure: slot unchanged");

    return tree;
}


array_test_tree
test_atomic_array_cas_weak_loop(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: compare_exchange_weak loop convergence");
    auto* root = tree.underlying().root();

    at_arr<int, 2> a;
    a.store(0, 0);

    // single-threaded CAS-loop: increment 100 times.
    // weak CAS may spuriously fail; the loop must
    // eventually converge.
    for (int target = 1; target <= 100; ++target)
    {
        int observed = a.load(0);

        while (!a.compare_exchange_weak(0, observed, target))
        {
            // observed has been updated to the latest
            // value by compare_exchange_weak on failure
        }
    }

    append_leaf(tree, root,
        a.load(0) == 100,
        "CAS weak loop: 100 successful single-threaded updates");

    return tree;
}


// =========================================================================
// XVII. CATEGORY: ATOMIC_ARRAY BULK OPERATIONS
// =========================================================================

array_test_tree
test_atomic_array_size_empty(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: size / empty");
    auto* root = tree.underlying().root();

    at_arr<int, 8> a;

    append_leaf(tree, root,
        a.size() == 8,
        "size() == N for atomic_array<int,8>");
    append_leaf(tree, root,
        !a.empty(),
        "empty() == false for non-zero-extent");

    // size() / empty() are constexpr on a static-extent
    // atomic_array — verify by binding to a constexpr value
    constexpr std::size_t sz = decltype(a)::extent;
    append_leaf(tree, root,
        sz == 8,
        "extent usable as constant expression");

    return tree;
}


array_test_tree
test_atomic_array_fill(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: fill (per-element atomic)");
    auto* root = tree.underlying().root();

    at_arr<int, 6> a;
    a.fill(7);

    bool all_seven = true;

    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (a.load(i) != 7)
        {
            all_seven = false;
            break;
        }
    }

    append_leaf(tree, root,
        all_seven,
        "fill(7): every slot == 7");

    a.fill(0);
    bool all_zero = true;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (a.load(i) != 0)
        {
            all_zero = false;
            break;
        }
    }

    append_leaf(tree, root,
        all_zero,
        "fill(0): every slot reset to 0");

    return tree;
}


array_test_tree
test_atomic_array_is_lock_free(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: is_lock_free reports a bool");
    auto* root = tree.underlying().root();

    at_arr<int, 4> a;

    // is_lock_free is platform/type dependent.  We
    // just verify the call is well-formed and returns
    // some bool — we cannot portably assert true.
    bool lf = a.is_lock_free();

    append_leaf(tree, root,
        ( lf == true || lf == false ),
        "is_lock_free() returns a bool (platform-dependent value)");

    // zero-extent edge case: returns true vacuously
    at_arr<int, 0> empty;
    append_leaf(tree, root,
        empty.is_lock_free(),
        "is_lock_free() == true for zero-extent (vacuous)");

    return tree;
}


// =========================================================================
// XVIII. CATEGORY: ATOMIC_ARRAY ITERATION
// =========================================================================

array_test_tree
test_atomic_array_begin_end(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: begin / end iteration");
    auto* root = tree.underlying().root();

    at_arr<int, 4> a;
    for (std::size_t i = 0; i < 4; ++i)
    {
        a.store(i, static_cast<int>(i + 1));
    }

    append_leaf(tree, root,
        ( a.end() - a.begin() ) ==
            static_cast<std::ptrdiff_t>(a.size()),
        "end() - begin() == size()");

    // iterate via load on each slot
    int sum = 0;
    for (auto it = a.begin(); it != a.end(); ++it)
    {
        sum += it->load();
    }
    append_leaf(tree, root,
        sum == 10,
        "iteration with per-slot load() sums correctly");

    // const iteration
    const at_arr<int, 4>& cref = a;
    append_leaf(tree, root,
        cref.cbegin() != cref.cend(),
        "cbegin() != cend() for non-zero array");

    int csum = 0;
    for (auto it = cref.cbegin(); it != cref.cend(); ++it)
    {
        csum += it->load();
    }
    append_leaf(tree, root,
        csum == sum,
        "cbegin/cend iteration sums same as begin/end");

    return tree;
}


array_test_tree
test_atomic_array_data_pointer(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: data() pointer");
    auto* root = tree.underlying().root();

    at_arr<int, 4> a;
    a.store(0, 42);

    // data() returns std::atomic<T>* (non-const) or
    // const std::atomic<T>* (const).
    auto* p = a.data();
    append_leaf(tree, root,
        p != nullptr,
        "data() != nullptr");
    append_leaf(tree, root,
        p[0].load() == 42,
        "data()[i].load() reaches stored value");

    p[1].store(99);
    append_leaf(tree, root,
        a.load(1) == 99,
        "writes through data() observable via load(i)");

    // const variant
    const at_arr<int, 4>& cref = a;
    append_leaf(tree, root,
        cref.data()[0].load() == 42,
        "const data()[i].load() works on const reference");

    return tree;
}


array_test_tree
test_atomic_array_range_based_for(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: range-based for over atomic slots");
    auto* root = tree.underlying().root();

    // range-based for is a C++11 feature and the suite's
    // baseline is C++11, so no per-feature gate is needed
    // here.
    at_arr<int, 4> a;

    int n = 1;
    for (auto& slot : a)
    {
        slot.store(n);
        ++n;
    }

    append_leaf(tree, root,
        ( a.load(0) == 1 && a.load(1) == 2 &&
          a.load(2) == 3 && a.load(3) == 4 ),
        "range-based for: stores observable via load()");

    int total = 0;
    for (auto& slot : a)
    {
        total += slot.load();
    }
    append_leaf(tree, root,
        total == 10,
        "range-based for: sum over loads == 10");

    return tree;
}


// =========================================================================
// XIX. CATEGORY: ATOMIC_ARRAY CONCURRENT ACCESS
// =========================================================================

array_test_tree
test_atomic_array_concurrent_fetch_add(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: concurrent fetch_add on shared slot");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    at_arr<int, 1> a;
    a.store(0, 0);

    constexpr std::size_t kThreads      = 4;
    constexpr std::size_t kPerThreadOps = 1000;

    run_threads(kThreads,
        [&](std::size_t /*_id*/)
        {
            for (std::size_t i = 0; i < kPerThreadOps; ++i)
            {
                a.fetch_add(0, 1);
            }
        });

    const int expected =
        static_cast<int>(kThreads * kPerThreadOps);

    append_leaf(tree, root,
        a.load(0) == expected,
        "concurrent fetch_add: final == threads * iterations");
#else
    append_leaf(tree, root,
        true,
        "concurrent fetch_add test skipped (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_atomic_array_concurrent_disjoint_slots(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: concurrent disjoint slot fetch_add");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    constexpr std::size_t kThreads      = 4;
    constexpr std::size_t kPerThreadOps = 500;

    at_arr<int, kThreads> a;

    run_threads(kThreads,
        [&](std::size_t _id)
        {
            for (std::size_t i = 0; i < kPerThreadOps; ++i)
            {
                a.fetch_add(_id, 1);
            }
        });

    bool all_correct = true;
    for (std::size_t i = 0; i < kThreads; ++i)
    {
        if (a.load(i) != static_cast<int>(kPerThreadOps))
        {
            all_correct = false;
            break;
        }
    }

    append_leaf(tree, root,
        all_correct,
        "disjoint slots: each slot == iterations "
        "(no cross-slot interference)");
#else
    append_leaf(tree, root,
        true,
        "disjoint slot test skipped (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_atomic_array_concurrent_cas_loop(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array: concurrent CAS-loop increment");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    at_arr<int, 1> a;
    a.store(0, 0);

    constexpr std::size_t kThreads      = 4;
    constexpr std::size_t kPerThreadOps = 250;

    run_threads(kThreads,
        [&](std::size_t /*_id*/)
        {
            for (std::size_t i = 0; i < kPerThreadOps; ++i)
            {
                int observed = a.load(0);
                while (!a.compare_exchange_weak(
                           0, observed, observed + 1))
                {
                    // observed updated by CAS on failure
                }
            }
        });

    const int expected =
        static_cast<int>(kThreads * kPerThreadOps);

    append_leaf(tree, root,
        a.load(0) == expected,
        "CAS-loop concurrent increment: final == "
        "threads * iterations");
#else
    append_leaf(tree, root,
        true,
        "CAS-loop test skipped (requires C++11)");
#endif

    return tree;
}


// =========================================================================
// IX.  SUB-BUILDER:  atomic_array MODULE SUBTREE
// =========================================================================

// make_atomic_array_subtree
//   Aggregates every category function defined above into a single
// subtree rooted under an atomic_array test-block.  Called by the
// suite-level aggregate builder make_threadsafe_array_test_subtree
// (defined in threadsafe_array_core_tests.cpp); also callable
// directly by users who want to drive only the atomic_array tests
// against a custom test_handler.
array_test_tree
make_atomic_array_subtree(
    test::test_type_id _kind
)
{
    return combine_subtrees<array_test_tree>(
        array_test_obj(_kind, true,
            "atomic_array test module"),
        {
            // II.  trait conformance (atomic_array's own strategy)
            test_atomic_array_traits_strategy_atomic(_kind),

            // III. construction
            test_atomic_array_default_construction(_kind),
            test_atomic_array_fill_construction(_kind),
            test_atomic_array_copy_move_deletion_sfinae(_kind),

            // IV.  element access
            test_atomic_array_load_store(_kind),
            test_atomic_array_exchange(_kind),
            test_atomic_array_memory_orderings(_kind),
            test_atomic_array_slot_independence(_kind),

            // V.   element updates
            test_atomic_array_fetch_add(_kind),
            test_atomic_array_fetch_sub(_kind),
            test_atomic_array_fetch_and(_kind),
            test_atomic_array_fetch_or(_kind),
            test_atomic_array_fetch_xor(_kind),

            // VI.  CAS
            test_atomic_array_cas_strong_success(_kind),
            test_atomic_array_cas_strong_failure(_kind),
            test_atomic_array_cas_weak_loop(_kind),

            // VII. bulk operations
            test_atomic_array_size_empty(_kind),
            test_atomic_array_fill(_kind),
            test_atomic_array_is_lock_free(_kind),

            // VIII. iteration
            test_atomic_array_begin_end(_kind),
            test_atomic_array_data_pointer(_kind),
            test_atomic_array_range_based_for(_kind),

            // IX.  concurrent access
            test_atomic_array_concurrent_fetch_add(_kind),
            test_atomic_array_concurrent_disjoint_slots(_kind),
            test_atomic_array_concurrent_cas_loop(_kind),
        });
}


NS_END  // testing
NS_END  // djinterp
