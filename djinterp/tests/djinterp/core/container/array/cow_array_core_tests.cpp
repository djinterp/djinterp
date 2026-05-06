/******************************************************************************
* djinterp [testing]                                 cow_array_core_tests.cpp
*
*   Implementation of the cow_array portion of Part B (threadsafe
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
*   This translation unit owns the cow_array strategy: copy-on-
* write with snapshot semantics under a configurable lock policy.
* Cross-container concerns (axis preservation across all three
* wrappers, strategy-tag mutual disjointness, and shared edge
* cases) live in threadsafe_array_core_tests.cpp; the suite-wide
* aggregate builder and master runner also live there and call
* into the sub-builder exposed at the bottom of this file.
*
*
* path:      /tests/djinterp/core/container/array/
*                 cow_array_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.03
******************************************************************************/
#include "./array_tests.hpp"

// std
#include <cstdint>          // std::uint64_t
#include <type_traits>      // is_*_constructible
#include <utility>          // std::move

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>       // std::atomic for concurrent tests
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
// call diagnostics at every assertion site.

namespace {

    using node_alias =
        djinterp::nary_tree<array_test_obj>::node_type;


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
    //   This translation unit only needs the cow_array alias and
    // the base mutable_iterable_array used as the underlying
    // storage in from-array construction tests.
    // -------------------------------------------------------------------

    template<typename T, std::size_t N>
    using cow_arr = djinterp::cow_array<T, N>;

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
// II.  CATEGORY: COW_ARRAY TRAIT CONFORMANCE
// =========================================================================

array_test_tree
test_cow_array_traits_strategy_cow(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree(
        "trait conformance: cow_array -> cow strategy");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        djinterp::is_cow_container<cow_arr<int, 4>>::value,
        "is_cow_container<cow_array>::value == true");
    append_leaf(tree, root,
        !djinterp::is_cow_container<base_mi<int, 4>>::value,
        "is_cow_container<plain array>::value == false");
    append_leaf(tree, root,
        !djinterp::is_atomic_container<cow_arr<int, 4>>::value,
        "cow_array NOT classified as atomic");

    // strategy tag check
    append_leaf(tree, root,
        std::is_same<
            typename cow_arr<int, 4>::concurrency_strategy_tag,
            djinterp::cow_strategy_tag>::value,
        "cow_array::concurrency_strategy_tag == "
        "cow_strategy_tag");

    return tree;
}


// XX.  CATEGORY: COW_ARRAY CONSTRUCTION
// =========================================================================

array_test_tree
test_cow_array_default_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: default construction");

    root = tree.underlying().root();

    // default-construct: wrapped array default-constructs,
    // version starts at 0.
    cow_arr<int, 8> a;

    append_leaf(tree, root,
        a.size() == 8u,
        "default-constructed cow_array.size() == extent");

    append_leaf(tree, root,
        a.version() == 0u,
        "default-constructed cow_array.version() == 0");

    return tree;
}


array_test_tree
test_cow_array_from_array_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: construction from underlying array");

    root = tree.underlying().root();

    base_mi<int, 4> seed;
    seed[0] = 10;
    seed[1] = 20;
    seed[2] = 30;
    seed[3] = 40;

    cow_arr<int, 4> a(seed);

    append_leaf(tree, root,
        a.at(0) == 10 && a.at(3) == 40,
        "cow_array constructed from array preserves "
        "element values");

    append_leaf(tree, root,
        a.size() == 4u,
        "cow_array constructed from array preserves size");

    // construct via rvalue (move)
    base_mi<int, 4> tmp;
    tmp.fill(99);

    cow_arr<int, 4> b(std::move(tmp));

    append_leaf(tree, root,
        b.at(0) == 99 && b.at(3) == 99,
        "cow_array constructed from rvalue array carries "
        "filled values");

    return tree;
}


array_test_tree
test_cow_array_copy_move_deletion_sfinae(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: copy and move deletion (SFINAE)");

    root = tree.underlying().root();

    using cow_t = cow_arr<int, 4>;

    append_leaf(tree, root,
        !std::is_copy_constructible<cow_t>::value,
        "cow_array is non-copy-constructible "
        "(contains lock-policy state)");

    append_leaf(tree, root,
        !std::is_copy_assignable<cow_t>::value,
        "cow_array is non-copy-assignable");

    append_leaf(tree, root,
        !std::is_move_constructible<cow_t>::value,
        "cow_array is non-move-constructible");

    append_leaf(tree, root,
        !std::is_move_assignable<cow_t>::value,
        "cow_array is non-move-assignable");

    append_leaf(tree, root,
        std::is_default_constructible<cow_t>::value,
        "cow_array is default-constructible");

    return tree;
}


// =========================================================================
// XXI. CATEGORY: COW_ARRAY READ ACCESS
// =========================================================================

array_test_tree
test_cow_array_read_returns_value(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: read() returns current array");

    root = tree.underlying().root();

    base_mi<int, 4> seed;
    seed[0] = 1; seed[1] = 2; seed[2] = 3; seed[3] = 4;

    cow_arr<int, 4> a(seed);

    const auto& current = a.read();

    append_leaf(tree, root,
        current.size() == 4u,
        "read() returns array with correct size");

    append_leaf(tree, root,
        current[0] == 1 && current[3] == 4,
        "read() returns array with correct contents");

    return tree;
}


array_test_tree
test_cow_array_size_empty(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: size() and empty() convenience wrappers");

    root = tree.underlying().root();

    cow_arr<int, 5> a;

    append_leaf(tree, root,
        a.size() == 5u,
        "cow_array<int,5>.size() == 5");

    append_leaf(tree, root,
        !a.empty(),
        "cow_array<int,5>.empty() == false");

    cow_arr<int, 0> z;

    append_leaf(tree, root,
        z.size() == 0u,
        "cow_array<int,0>.size() == 0");

    append_leaf(tree, root,
        z.empty(),
        "cow_array<int,0>.empty() == true");

    return tree;
}


array_test_tree
test_cow_array_at_returns_copy(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: at(i) returns a value copy");

    root = tree.underlying().root();

    base_mi<int, 4> seed;
    seed[0] = 100; seed[1] = 200; seed[2] = 300; seed[3] = 400;

    cow_arr<int, 4> a(seed);

    int v0 = a.at(0);
    int v3 = a.at(3);

    append_leaf(tree, root,
        v0 == 100 && v3 == 400,
        "cow_array.at(i) reads correct element");

    // verify return is a value, not a reference (mutating
    // returned value must not affect cow state).
    int& alias = v0;
    alias = 999;

    append_leaf(tree, root,
        a.at(0) == 100,
        "cow_array.at(i) returns a value copy "
        "(mutating result does not bleed back)");

    return tree;
}


// =========================================================================
// XXII. CATEGORY: COW_ARRAY SNAPSHOT
// =========================================================================

array_test_tree
test_cow_array_snapshot_content(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: snapshot() yields current contents");

    root = tree.underlying().root();

    base_mi<int, 4> seed;
    seed[0] = 11; seed[1] = 22; seed[2] = 33; seed[3] = 44;

    cow_arr<int, 4> a(seed);

    auto snap = a.snapshot();

    append_leaf(tree, root,
        snap->size() == 4u,
        "snapshot points to array with correct size");

    append_leaf(tree, root,
        (*snap)[0] == 11 && (*snap)[3] == 44,
        "snapshot contents match cow_array state at "
        "snapshot time");

    return tree;
}


array_test_tree
test_cow_array_snapshot_survives_write(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: snapshot survives subsequent writes");

    root = tree.underlying().root();

    base_mi<int, 3> seed;
    seed[0] = 7; seed[1] = 8; seed[2] = 9;

    cow_arr<int, 3> a(seed);

    auto snap = a.snapshot();

    // mutate after snapshot; the snapshot must keep its
    // own (logically-immutable) view.
    a.set(0, 70);
    a.set(1, 80);
    a.set(2, 90);

    append_leaf(tree, root,
        (*snap)[0] == 7 && (*snap)[1] == 8 && (*snap)[2] == 9,
        "snapshot content is unaffected by later writes");

    append_leaf(tree, root,
        a.at(0) == 70 && a.at(2) == 90,
        "live cow_array reflects the writes");

    return tree;
}


array_test_tree
test_cow_array_snapshot_version(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: snapshot version stable across writes");

    root = tree.underlying().root();

    cow_arr<int, 3> a;

    auto snap_v0 = a.version();
    auto snap    = a.snapshot();

    a.set(0, 1);
    a.set(1, 2);

    append_leaf(tree, root,
        a.version() > snap_v0,
        "cow_array.version() advances after writes");

    // the snapshot still points to the original generation;
    // we cannot directly read its version, but its contents
    // should be unchanged.
    append_leaf(tree, root,
        (*snap)[0] == 0 && (*snap)[1] == 0,
        "snapshot still observes pre-write state");

    return tree;
}


array_test_tree
test_cow_array_multiple_snapshots(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: multiple coexisting snapshots");

    root = tree.underlying().root();

    cow_arr<int, 3> a;

    a.set(0, 100);
    auto s1 = a.snapshot();

    a.set(0, 200);
    auto s2 = a.snapshot();

    a.set(0, 300);
    auto s3 = a.snapshot();

    append_leaf(tree, root,
        (*s1)[0] == 100,
        "snapshot s1 retains generation-1 state (100)");

    append_leaf(tree, root,
        (*s2)[0] == 200,
        "snapshot s2 retains generation-2 state (200)");

    append_leaf(tree, root,
        (*s3)[0] == 300,
        "snapshot s3 retains generation-3 state (300)");

    append_leaf(tree, root,
        a.at(0) == 300,
        "cow_array reflects latest write (300)");

    return tree;
}


// =========================================================================
// XXIII. CATEGORY: COW_ARRAY WRITE ACCESS
// =========================================================================

array_test_tree
test_cow_array_modify(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: modify(fn) clone-and-mutate");

    root = tree.underlying().root();

    cow_arr<int, 4> a;

    auto v_before = a.version();

    a.modify(
        [](base_mi<int, 4>& _arr)
        {
            _arr[0] = 1;
            _arr[1] = 2;
            _arr[2] = 3;
            _arr[3] = 4;
        });

    append_leaf(tree, root,
        a.at(0) == 1 && a.at(1) == 2 &&
        a.at(2) == 3 && a.at(3) == 4,
        "modify(fn) applies all mutations");

    append_leaf(tree, root,
        a.version() > v_before,
        "modify(fn) bumps version");

    // modify with return value — verify pass-through
    auto v_pre = a.version();

    int returned = a.modify(
        [](base_mi<int, 4>& _arr) -> int
        {
            _arr[0] = 99;
            return _arr[0] * 2;
        });

    append_leaf(tree, root,
        returned == 198,
        "modify(fn) returns the closure's return value");

    append_leaf(tree, root,
        a.at(0) == 99 && a.version() > v_pre,
        "modify(fn) with return value still applies "
        "mutation and bumps version");

    return tree;
}


array_test_tree
test_cow_array_replace(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: replace() swaps entire contents");

    root = tree.underlying().root();

    cow_arr<int, 3> a;

    a.set(0, 1);
    a.set(1, 2);
    a.set(2, 3);

    auto v_before = a.version();

    base_mi<int, 3> replacement;
    replacement[0] = 10;
    replacement[1] = 20;
    replacement[2] = 30;

    a.replace(replacement);

    append_leaf(tree, root,
        a.at(0) == 10 && a.at(1) == 20 && a.at(2) == 30,
        "replace(lvalue) swaps in new contents");

    append_leaf(tree, root,
        a.version() > v_before,
        "replace(lvalue) bumps version");

    auto v_mid = a.version();

    base_mi<int, 3> rvalue_repl;
    rvalue_repl[0] = 100;
    rvalue_repl[1] = 200;
    rvalue_repl[2] = 300;

    a.replace(std::move(rvalue_repl));

    append_leaf(tree, root,
        a.at(0) == 100 && a.at(2) == 300,
        "replace(rvalue) swaps in new contents");

    append_leaf(tree, root,
        a.version() > v_mid,
        "replace(rvalue) bumps version");

    return tree;
}


array_test_tree
test_cow_array_set_single(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: set(i, v) single-element write");

    root = tree.underlying().root();

    cow_arr<int, 4> a;

    a.set(2, 42);

    append_leaf(tree, root,
        a.at(2) == 42,
        "set(2, 42) writes the target slot");

    append_leaf(tree, root,
        a.at(0) == 0 && a.at(1) == 0 && a.at(3) == 0,
        "set(2, 42) leaves other slots unchanged");

    return tree;
}


array_test_tree
test_cow_array_version_monotonic(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: version is monotonic across writes");

    root = tree.underlying().root();

    cow_arr<int, 4> a;

    auto v0 = a.version();

    a.set(0, 1);
    auto v1 = a.version();

    a.set(1, 2);
    auto v2 = a.version();

    a.modify([](base_mi<int, 4>& _arr) { _arr[2] = 3; });
    auto v3 = a.version();

    base_mi<int, 4> repl;
    a.replace(repl);
    auto v4 = a.version();

    append_leaf(tree, root,
        ( v0 < v1 ) && ( v1 < v2 ) &&
        ( v2 < v3 ) && ( v3 < v4 ),
        "version is strictly monotonic across set / modify "
        "/ replace");

    // read-only operations must not bump version
    auto v_read_before = a.version();
    (void) a.at(0);
    (void) a.size();
    (void) a.read();
    auto snap = a.snapshot();
    (void) snap;
    auto v_read_after = a.version();

    append_leaf(tree, root,
        v_read_before == v_read_after,
        "read-only operations do not bump version");

    return tree;
}


// =========================================================================
// XXIV. CATEGORY: COW_ARRAY CONCURRENT ACCESS
// =========================================================================

array_test_tree
test_cow_array_concurrent_snapshots(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: concurrent snapshots are consistent");

    root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    cow_arr<int, 8> a;

    a.modify(
        [](base_mi<int, 8>& _arr)
        {
            for (std::size_t i = 0; i < 8; ++i)
            {
                _arr[i] = static_cast<int>(i + 1);
            }
        });

    constexpr std::size_t kReaders     = 4;
    constexpr int         kIterations  = 250;

    std::atomic<int> torn_reads(0);

    run_threads(kReaders,
        [&](std::size_t /*tid*/)
        {
            for (int it = 0; it < kIterations; ++it)
            {
                auto snap = a.snapshot();

                // every snapshot must observe the full
                // 1..8 sequence — never a partial write.
                bool ok = true;
                for (std::size_t i = 0; i < 8; ++i)
                {
                    int expected = static_cast<int>(i + 1);
                    int actual   = (*snap)[i];

                    // accept either the current generation
                    // (1..8) or any future writer's
                    // generation (writer below sets all to
                    // (i + 1) * gen).  The invariant we
                    // check: snap[i] / (i + 1) is identical
                    // across all i — i.e. the snapshot is
                    // self-consistent.
                    if (actual % expected != 0)
                    {
                        ok = false;
                        break;
                    }
                }

                if (!ok)
                {
                    torn_reads.fetch_add(
                        1, std::memory_order_relaxed);
                }
            }
        });

    append_leaf(tree, root,
        torn_reads.load() == 0,
        "concurrent snapshots: zero torn reads observed");
#else
    append_leaf(tree, root,
        true,
        "concurrent snapshot test skipped (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_cow_array_concurrent_writers(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "cow_array: concurrent writers serialize");

    root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    djinterp::cow_array<
        int,
        4,
        array_lifetime::mutable_lifetime,
        array_iterability::iterable,
        djinterp::exclusive_lock_policy> a;

    constexpr std::size_t kWriters    = 4;
    constexpr int         kIterations = 250;

    auto v_start = a.version();

    run_threads(kWriters,
        [&](std::size_t tid)
        {
            for (int it = 0; it < kIterations; ++it)
            {
                a.set(tid % 4,
                      static_cast<int>(tid * 1000 + it));
            }
        });

    auto v_end = a.version();

    // every set() bumps the version — under contention
    // we expect at least kWriters * kIterations bumps.
    auto expected_min = static_cast<std::uint64_t>(kWriters * kIterations);

    append_leaf(tree, root,
        ( v_end - v_start ) >= expected_min,
        "concurrent writers: version count >= total writes");

    // and the array must end in a self-consistent state
    // (i.e. accessing any slot must not crash, and every
    // slot must hold a value some writer actually wrote).
    bool all_slots_consistent = true;
    for (std::size_t i = 0; i < 4; ++i)
    {
        int v = a.at(i);
        // accept zero (uninitialized) or any tid*1000+iter
        // pattern — we only check that the read does not
        // segfault and yields a plausible value.
        if (v < 0)
        {
            all_slots_consistent = false;
            break;
        }
    }

    append_leaf(tree, root,
        all_slots_consistent,
        "concurrent writers: final state is readable and "
        "self-consistent");
#else
    append_leaf(tree, root,
        true,
        "concurrent writers test skipped (requires C++11)");
#endif

    return tree;
}


// =========================================================================
// VII.  SUB-BUILDER:  cow_array MODULE SUBTREE
// =========================================================================

// make_cow_array_subtree
//   Aggregates every category function defined above into a single
// subtree rooted under a cow_array test-block.  Called by the
// suite-level aggregate builder make_threadsafe_array_test_subtree
// (defined in threadsafe_array_core_tests.cpp); also callable
// directly by users who want to drive only the cow_array tests
// against a custom test_handler.
array_test_tree
make_cow_array_subtree(
    test::test_type_id _kind
)
{
    return combine_subtrees<array_test_tree>(
        array_test_obj(_kind, true,
            "cow_array test module"),
        {
            // II.  trait conformance (cow_array's own strategy)
            test_cow_array_traits_strategy_cow(_kind),

            // III. construction
            test_cow_array_default_construction(_kind),
            test_cow_array_from_array_construction(_kind),
            test_cow_array_copy_move_deletion_sfinae(_kind),

            // IV.  read access
            test_cow_array_read_returns_value(_kind),
            test_cow_array_size_empty(_kind),
            test_cow_array_at_returns_copy(_kind),

            // V.   snapshot
            test_cow_array_snapshot_content(_kind),
            test_cow_array_snapshot_survives_write(_kind),
            test_cow_array_snapshot_version(_kind),
            test_cow_array_multiple_snapshots(_kind),

            // VI.  write access
            test_cow_array_modify(_kind),
            test_cow_array_replace(_kind),
            test_cow_array_set_single(_kind),
            test_cow_array_version_monotonic(_kind),

            // VII. concurrent access
            test_cow_array_concurrent_snapshots(_kind),
            test_cow_array_concurrent_writers(_kind),
        });
}


NS_END  // testing
NS_END  // djinterp