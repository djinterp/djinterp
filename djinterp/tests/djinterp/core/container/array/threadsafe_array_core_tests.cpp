/******************************************************************************
* djinterp [testing]                            threadsafe_array_core_tests.cpp
*
*   Implementation of the threadsafe_array portion of Part B
* (threadsafe wrappers) of the array test suite under the
* return-a-subtree protocol declared in array_tests.hpp.
*
*   STRUCTURE:
*   Every category function below builds and returns a self-
* contained `array_test_tree` (the project's
* `test::test_tree` overlay backed by `djinterp::nary_tree`).
* Tests do NOT take a test_handler or a test_printer; they do
* NOT mutate any caller-supplied sink; they simply construct
* a small tree and return it by value.
*
*   This translation unit owns the threadsafe_array strategy:
* lock-policy-protected whole-array access via mutex / shared /
* timed locks (and a zero-overhead null policy for single-
* threaded use).  It is also the home of the suite-wide
* aggregate builder (make_threadsafe_array_test_subtree) and
* master runner (run_threadsafe_array_suite), and of the
* cross-cutting tests that touch all three wrappers — axis
* preservation, strategy-tag mutual disjointness, and the cube
* edge cases — which have no single natural home and so live
* with the suite's namesake.
*   The atomic_array and cow_array strategies live in their
* own translation units (atomic_array_core_tests.cpp and
* cow_array_core_tests.cpp); the aggregate builder pulls in
* their sub-builders.
*
*
* path:      /tests/djinterp/core/container/array/
*                 threadsafe_array_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.02
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

namespace {

    using node_alias =
        djinterp::nary_tree<array_test_obj>::node_type;


    // append_leaf
    //   helper: appends an assertion-kind leaf under _parent.
    inline node_alias*
    append_leaf(
        array_test_tree& _tree,
        node_alias*           _parent,
        bool                  _passed,
        const char*           _name
    )
    {
        return _tree.underlying().append_child(
            _parent,
            test::make_assert(_passed, _name));
    }


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
    //  cube-cell aliases for each wrapper
    //
    //   The threadsafe_array family preserves the lifetime x
    // iterability axes from the wrapped array.  For test
    // brevity we focus on the mutable / iterable cell since
    // the other cells are exercised by the trait-preservation
    // tests; trait specializations carry over the structural
    // properties from the wrapped array.
    // -------------------------------------------------------------------

    // null-locked threadsafe_array (zero-overhead)
    template<typename T, std::size_t N>
    using ts_arr = djinterp::threadsafe_array<T, N>;

    // exclusive-locked threadsafe_array (= mutex_array)
    template<typename T, std::size_t N>
    using ts_mtx_arr = djinterp::mutex_array<T, N>;

    // timed-locked threadsafe_array (= timed_array)
    template<typename T, std::size_t N>
    using ts_tim_arr = djinterp::timed_array<T, N>;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // shared-locked threadsafe_array (= shared_array, C++17+)
    template<typename T, std::size_t N>
    using ts_shr_arr = djinterp::shared_array<T, N>;
#endif

    // atomic_array
    template<typename T, std::size_t N>
    using at_arr = djinterp::atomic_array<T, N>;

    // null-locked cow_array
    template<typename T, std::size_t N>
    using cow_arr = djinterp::cow_array<T, N>;

    // base array aliases — used as the underlying for the
    // threadsafe wrappers' axis-preservation checks
    template<typename T, std::size_t N>
    using base_mi  = djinterp::mutable_iterable_array<T, N>;

    template<typename T, std::size_t N>
    using base_imi = djinterp::immutable_iterable_array<T, N>;


    // -------------------------------------------------------------------
    //  SFINAE expression detectors
    //
    //   These are intentionally narrow — each verifies
    // existence (or absence) of a single named member
    // operation on the wrapper.  Used by the trait-
    // conformance and SFINAE-deletion tests.
    // -------------------------------------------------------------------

    // has_size_lockfree
    template<typename T, typename = void>
    struct has_size_lockfree : std::false_type
    {};
    template<typename T>
    struct has_size_lockfree<T,
        decltype(void(std::declval<const T&>().size_lockfree()))>
        : std::true_type
    {};

    // has_version
    template<typename T, typename = void>
    struct has_version : std::false_type
    {};
    template<typename T>
    struct has_version<T,
        decltype(void(std::declval<const T&>().version()))>
        : std::true_type
    {};

    // has_snapshot
    template<typename T, typename = void>
    struct has_snapshot : std::false_type
    {};
    template<typename T>
    struct has_snapshot<T,
        decltype(void(std::declval<const T&>().snapshot()))>
        : std::true_type
    {};

    // has_load_index
    template<typename T, typename = void>
    struct has_load_index : std::false_type
    {};
    template<typename T>
    struct has_load_index<T,
        decltype(void(std::declval<const T&>().load(
            std::declval<std::size_t>())))>
        : std::true_type
    {};

    // has_fetch_add_index
    template<typename T, typename = void>
    struct has_fetch_add_index : std::false_type
    {};
    template<typename T>
    struct has_fetch_add_index<T,
        decltype(void(std::declval<T&>().fetch_add(
            std::declval<std::size_t>(),
            std::declval<typename T::value_type>())))>
        : std::true_type
    {};


    // -------------------------------------------------------------------
    //  thread-test helpers
    // -------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // run_threads
    //   helper: spawns _count threads each running _worker
    // with its index, then joins them all.
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
// II.  CATEGORY: TRAIT CONFORMANCE
// =========================================================================

array_test_tree
test_threadsafe_array_traits_axis_preservation(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree(
        "trait conformance: axis preservation across wrapping");
    auto* root = tree.underlying().root();

    // contiguity: wrapper inherits from wrapped array
    append_leaf(tree, root,
        djinterp::is_contiguous_array<ts_arr<int, 4>>::value ==
        djinterp::is_contiguous_array<base_mi<int, 4>>::value,
        "threadsafe_array: contiguity matches wrapped array");
    append_leaf(tree, root,
        djinterp::is_contiguous_array<at_arr<int, 4>>::value,
        "atomic_array: contiguous");
    append_leaf(tree, root,
        djinterp::is_contiguous_array<cow_arr<int, 4>>::value ==
        djinterp::is_contiguous_array<base_mi<int, 4>>::value,
        "cow_array: contiguity matches wrapped array");

    // iterability: must match the iterable cell's axis
    append_leaf(tree, root,
        djinterp::is_iterable_array<ts_arr<int, 4>>::value ==
        djinterp::is_iterable_array<base_mi<int, 4>>::value,
        "threadsafe_array: iterability matches wrapped array");
    append_leaf(tree, root,
        djinterp::is_iterable_array<cow_arr<int, 4>>::value ==
        djinterp::is_iterable_array<base_mi<int, 4>>::value,
        "cow_array: iterability matches wrapped array");

    // static extent: arrays always have static extent
    append_leaf(tree, root,
        djinterp::has_static_extent<ts_arr<int, 4>>::value,
        "threadsafe_array: has_static_extent == true");
    append_leaf(tree, root,
        djinterp::has_static_extent<at_arr<int, 4>>::value,
        "atomic_array: has_static_extent == true");
    append_leaf(tree, root,
        djinterp::has_static_extent<cow_arr<int, 4>>::value,
        "cow_array: has_static_extent == true");

    // lifetime preservation (mutable -> mutable)
    append_leaf(tree, root,
        djinterp::array_lifetime_of<ts_arr<int, 4>>::value ==
        djinterp::array_lifetime_of<base_mi<int, 4>>::value,
        "threadsafe_array: lifetime matches wrapped array");
    append_leaf(tree, root,
        djinterp::array_lifetime_of<cow_arr<int, 4>>::value ==
        djinterp::array_lifetime_of<base_mi<int, 4>>::value,
        "cow_array: lifetime matches wrapped array");

    // extent value carry-through
    append_leaf(tree, root,
        ts_arr<int, 16>::extent == 16,
        "threadsafe_array<int,16>::extent == 16");
    append_leaf(tree, root,
        at_arr<int, 16>::extent == 16,
        "atomic_array<int,16>::extent == 16");
    append_leaf(tree, root,
        cow_arr<int, 16>::extent == 16,
        "cow_array<int,16>::extent == 16");

    return tree;
}


array_test_tree
test_threadsafe_array_traits_strategy_locked(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree(
        "trait conformance: threadsafe_array -> locked strategy");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        djinterp::is_locked_container_v<ts_arr<int, 4>>,
        "is_locked_container_v<threadsafe_array> == true");
    append_leaf(tree, root,
        !djinterp::is_locked_container_v<base_mi<int, 4>>,
        "is_locked_container_v<plain array> == false");
    append_leaf(tree, root,
        djinterp::is_locked_container_v<ts_mtx_arr<int, 4>>,
        "is_locked_container_v<mutex_array> == true");

    // strategy tag check: the wrapper's tag is locked_strategy_tag
    append_leaf(tree, root,
        std::is_same<
            typename ts_arr<int, 4>::concurrency_strategy_tag,
            djinterp::locked_strategy_tag>::value,
        "threadsafe_array::concurrency_strategy_tag == "
        "locked_strategy_tag");

    return tree;
}


array_test_tree
test_threadsafe_traits_strategy_disjointness(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree(
        "trait conformance: strategies are mutually exclusive");
    auto* root = tree.underlying().root();

    // Each wrapper exposes exactly one strategy classification.
    // Plain array exposes none of them.

    append_leaf(tree, root,
        ( djinterp::is_locked_container_v<ts_arr<int, 4>>          &&
         !djinterp::is_atomic_container_v<ts_arr<int, 4>>          &&
         !djinterp::is_cow_container_v<ts_arr<int, 4>> ),
        "threadsafe_array: locked AND NOT (atomic OR cow)");

    append_leaf(tree, root,
        ( djinterp::is_atomic_container_v<at_arr<int, 4>>          &&
         !djinterp::is_locked_container_v<at_arr<int, 4>>          &&
         !djinterp::is_cow_container_v<at_arr<int, 4>> ),
        "atomic_array: atomic AND NOT (locked OR cow)");

    append_leaf(tree, root,
        ( djinterp::is_cow_container_v<cow_arr<int, 4>>            &&
         !djinterp::is_atomic_container_v<cow_arr<int, 4>> ),
        "cow_array: cow AND NOT atomic");

    append_leaf(tree, root,
        ( !djinterp::is_locked_container_v<base_mi<int, 4>>        &&
          !djinterp::is_atomic_container_v<base_mi<int, 4>>        &&
          !djinterp::is_cow_container_v<base_mi<int, 4>> ),
        "plain array: none of locked / atomic / cow");

    return tree;
}


// =========================================================================
// III. CATEGORY: THREADSAFE_ARRAY CONSTRUCTION
// =========================================================================

array_test_tree
test_threadsafe_array_default_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: default construction");
    auto* root = tree.underlying().root();

    // size() (locked) reflects the underlying array's
    // size, which equals the static extent N regardless
    // of whether assign() has been called.
    ts_arr<int, 4> a;
    append_leaf(tree, root,
        a.size() == 4,
        "default-constructed: size() == 4");

    append_leaf(tree, root,
        ( a.at(0) == 0 && a.at(1) == 0 &&
          a.at(2) == 0 && a.at(3) == 0 ),
        "default-constructed: every element zero-initialized");

    // Note: size_lockfree() reads from the atomic_state,
    // which the default constructor leaves at 0 because
    // the atomic_state is generic and only synchronized
    // through assign().  This is the documented contract.
    append_leaf(tree, root,
        a.size_lockfree() == 0,
        "default-constructed: size_lockfree() == 0 "
        "(atomic_state untouched until assign())");

    append_leaf(tree, root,
        a.version() == 0,
        "default-constructed: version() == 0");

    return tree;
}


array_test_tree
test_threadsafe_array_pack_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: parameter-pack construction");
    auto* root = tree.underlying().root();

    ts_arr<int, 4> a(10, 20, 30, 40);
    append_leaf(tree, root,
        ( a.at(0) == 10 && a.at(1) == 20 &&
          a.at(2) == 30 && a.at(3) == 40 ),
        "pack ctor: values forwarded to wrapped array in "
        "declaration order");

    // partial pack: tail elements zero-initialized by
    // the wrapped array's own ctor
    ts_arr<int, 4> b(7, 8);
    append_leaf(tree, root,
        ( b.at(0) == 7 && b.at(1) == 8 &&
          b.at(2) == 0 && b.at(3) == 0 ),
        "pack ctor: partial pack zeroes the tail");

    return tree;
}


array_test_tree
test_threadsafe_array_copy_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: copy construction");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4>        src(1, 2, 3, 4);
    // bind through a const ref so the copy constructor wins
    // overload resolution against threadsafe_array's variadic
    // forwarding constructor.
    const ts_mtx_arr<int, 4>& src_cref = src;
    ts_mtx_arr<int, 4>        dst(src_cref);

    append_leaf(tree, root,
        ( dst.at(0) == 1 && dst.at(1) == 2 &&
          dst.at(2) == 3 && dst.at(3) == 4 ),
        "copy ctor: destination matches source");

    // independence: mutating source must not affect dst
    src.set(0, 99);
    append_leaf(tree, root,
        dst.at(0) == 1,
        "copy ctor: destination is independent of source");

    return tree;
}


array_test_tree
test_threadsafe_array_copy_assignment(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: copy assignment");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4>  a(1, 2, 3, 4);
    ts_mtx_arr<int, 4>  b(9, 9, 9, 9);
    const std::uint64_t v_before = b.version();

    b = a;

    append_leaf(tree, root,
        ( b.at(0) == 1 && b.at(1) == 2 &&
          b.at(2) == 3 && b.at(3) == 4 ),
        "copy assignment: target receives source contents");

    append_leaf(tree, root,
        b.version() > v_before,
        "copy assignment: version bumped on target");

    // self-assignment must be a no-op (or at least
    // safe).  We just check that the contents are
    // unchanged.
    a = a;
    append_leaf(tree, root,
        ( a.at(0) == 1 && a.at(3) == 4 ),
        "copy assignment: self-assignment preserves contents");

    return tree;
}


array_test_tree
test_threadsafe_array_move_deletion_sfinae(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: move ctor / move assign deleted");
    auto* root = tree.underlying().root();

    using ts = ts_mtx_arr<int, 4>;

    // The wrapper deletes move construction and move
    // assignment because a mutex's address is part of
    // its identity and cannot be portably transferred.
    append_leaf(tree, root,
        !std::is_move_constructible<ts>::value,
        "is_move_constructible<threadsafe_array> == false");
    append_leaf(tree, root,
        !std::is_move_assignable<ts>::value,
        "is_move_assignable<threadsafe_array> == false");

    // Copy is preserved.
    append_leaf(tree, root,
        std::is_copy_constructible<ts>::value,
        "is_copy_constructible<threadsafe_array> == true");
    append_leaf(tree, root,
        std::is_copy_assignable<ts>::value,
        "is_copy_assignable<threadsafe_array> == true");

    return tree;
}


// =========================================================================
// IV.  CATEGORY: THREADSAFE_ARRAY LOCK-FREE QUERIES
// =========================================================================

array_test_tree
test_threadsafe_array_size_lockfree(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: size_lockfree query");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 8> a;

    // The atomic_state's size starts at 0; assign() is
    // the only path that calls store_size().
    append_leaf(tree, root,
        a.size_lockfree() == 0,
        "size_lockfree() == 0 before assign");

    djinterp::array<int, 8> src(1, 2, 3, 4, 5, 6, 7, 8);
    a.assign(src);

    append_leaf(tree, root,
        a.size_lockfree() == 8,
        "size_lockfree() == 8 after assign(src) of size 8");

    // size_lockfree must not throw and must not lock —
    // structurally we cannot directly assert "no lock,"
    // but we can assert it is callable on a const ref
    // and is noexcept-able.
    const auto& cref = a;
    append_leaf(tree, root,
        cref.size_lockfree() == 8,
        "size_lockfree() callable on const reference");

    return tree;
}


array_test_tree
test_threadsafe_array_version_query(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: version query");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4>  a(1, 2, 3, 4);
    const std::uint64_t v0 = a.version();

    a.set(0, 11);
    const std::uint64_t v1 = a.version();

    a.set(1, 22);
    const std::uint64_t v2 = a.version();

    append_leaf(tree, root,
        v1 > v0,
        "version monotonic: v1 > v0 after first set()");
    append_leaf(tree, root,
        v2 > v1,
        "version monotonic: v2 > v1 after second set()");

    // pure read operations leave the version untouched
    const std::uint64_t v_before_read = a.version();
    (void)a.at(2);
    (void)a.size();
    const std::uint64_t v_after_read = a.version();

    append_leaf(tree, root,
        v_after_read == v_before_read,
        "version unchanged across read-only operations");

    return tree;
}


array_test_tree
test_threadsafe_array_empty_lockfree(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: empty_lockfree query");
    auto* root = tree.underlying().root();

    // Default-constructed: size_lockfree == 0, so
    // empty_lockfree returns true even though the
    // wrapped array has N slots.
    ts_arr<int, 4> a;
    append_leaf(tree, root,
        a.empty_lockfree(),
        "empty_lockfree() == true before assign "
        "(atomic_state size == 0)");

    djinterp::array<int, 4> src(1, 2, 3, 4);
    a.assign(src);

    append_leaf(tree, root,
        !a.empty_lockfree(),
        "empty_lockfree() == false after assign(non-empty)");

    return tree;
}


// =========================================================================
// V.   CATEGORY: THREADSAFE_ARRAY SINGLE-OP ACCESS
// =========================================================================

array_test_tree
test_threadsafe_array_size_locked(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: size() locked");
    auto* root = tree.underlying().root();

    // size() (locked) reads the wrapped array's size,
    // which is N for a fixed-extent array regardless
    // of any assign().
    ts_mtx_arr<int, 5> a;
    append_leaf(tree, root,
        a.size() == 5,
        "size() == N for default-constructed fixed-extent");

    ts_mtx_arr<int, 5> b(10, 20, 30, 40, 50);
    append_leaf(tree, root,
        b.size() == 5,
        "size() == N regardless of pack-construction");

    // empty(): false for non-zero-extent array
    append_leaf(tree, root,
        !b.empty(),
        "empty() == false for non-zero-extent");

    return tree;
}


array_test_tree
test_threadsafe_array_at_returns_value(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: at(i) returns by value");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(1, 2, 3, 4);

    // Returning by value, not reference — the lock can
    // release before the caller observes the result.
    append_leaf(tree, root,
        std::is_same<
            decltype(a.at(0)),
            typename ts_mtx_arr<int, 4>::value_type>::value,
        "at(i) returns value_type by value (not by ref)");

    append_leaf(tree, root,
        a.at(0) == 1,
        "at(0) == 1");
    append_leaf(tree, root,
        a.at(3) == 4,
        "at(N-1) == 4");

    // const at()
    const ts_mtx_arr<int, 4>& cref = a;
    append_leaf(tree, root,
        cref.at(2) == 3,
        "at(i) callable on const reference");

    return tree;
}


array_test_tree
test_threadsafe_array_set_visible(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: set(i,v) visible to subsequent at(i)");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(0, 0, 0, 0);

    a.set(0, 100);
    a.set(1, 200);
    a.set(2, 300);
    a.set(3, 400);

    append_leaf(tree, root,
        ( a.at(0) == 100 && a.at(1) == 200 &&
          a.at(2) == 300 && a.at(3) == 400 ),
        "set() then at(): values observable in order");

    // overwrite test
    a.set(0, 999);
    append_leaf(tree, root,
        a.at(0) == 999,
        "set() overwrites previous value");

    return tree;
}


array_test_tree
test_threadsafe_array_set_bumps_version(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: set(i,v) bumps version");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(0, 0, 0, 0);

    const std::uint64_t v0 = a.version();
    a.set(0, 1);
    const std::uint64_t v1 = a.version();
    a.set(0, 2);
    const std::uint64_t v2 = a.version();

    append_leaf(tree, root,
        v1 > v0,
        "version strictly increased after first set()");
    append_leaf(tree, root,
        v2 > v1,
        "version strictly increased after second set() "
        "(even with same index)");

    // version must increase by at least 1 each call
    append_leaf(tree, root,
        ( (v1 - v0) >= 1 ) && ( (v2 - v1) >= 1 ),
        "version increment >= 1 per set()");

    return tree;
}


// =========================================================================
// VI.  CATEGORY: THREADSAFE_ARRAY HANDLE-BASED ACCESS
// =========================================================================

array_test_tree
test_threadsafe_array_read_access(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: read_access() handle");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(1, 2, 3, 4);

    {
        auto handle = a.read_access();

        append_leaf(tree, root,
            ( (*handle).size() == 4 ),
            "(*handle).size() == 4");
        append_leaf(tree, root,
            handle->size() == 4,
            "handle->size() == 4");
        append_leaf(tree, root,
            ( (*handle)[0] == 1 && (*handle)[3] == 4 ),
            "*handle indexes the underlying array");
    }
    // lock released here

    // Reading is still possible after the handle is gone
    append_leaf(tree, root,
        a.at(0) == 1,
        "at(0) accessible after read handle released");

    return tree;
}


array_test_tree
test_threadsafe_array_write_access(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: write_access() handle");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(0, 0, 0, 0);

    {
        auto handle = a.write_access();

        // mutate in-place via the handle
        (*handle)[0] = 11;
        (*handle)[1] = 22;
        (*handle)[2] = 33;
        (*handle)[3] = 44;
    }
    // lock released here

    append_leaf(tree, root,
        ( a.at(0) == 11 && a.at(1) == 22 &&
          a.at(2) == 33 && a.at(3) == 44 ),
        "writes through write_access() handle visible");

    // const access via write_access (writer lock is a
    // superset of reader lock)
    {
        auto handle = a.write_access();
        const auto* ptr = handle.operator->();

        append_leaf(tree, root,
            ( (*ptr)[0] == 11 ),
            "write_access().operator->() reaches contents");
    }

    return tree;
}


array_test_tree
test_threadsafe_array_handle_lifetime(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: handle scope lifetime");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(1, 2, 3, 4);

    // RAII: a write handle must be neither copyable nor
    // copy-assignable.  This protects against accidental
    // double-release.
    using locked_t =
        decltype(a.write_access());
    using const_locked_t =
        decltype(a.read_access());

    append_leaf(tree, root,
        !std::is_copy_constructible<locked_t>::value,
        "locked_ref: NOT copy-constructible");
    append_leaf(tree, root,
        !std::is_copy_assignable<locked_t>::value,
        "locked_ref: NOT copy-assignable");
    append_leaf(tree, root,
        !std::is_copy_constructible<const_locked_t>::value,
        "const_locked_ref: NOT copy-constructible");
    append_leaf(tree, root,
        !std::is_copy_assignable<const_locked_t>::value,
        "const_locked_ref: NOT copy-assignable");

    return tree;
}


// =========================================================================
// VII. CATEGORY: THREADSAFE_ARRAY BULK OPERATIONS
// =========================================================================

array_test_tree
test_threadsafe_array_assign(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: assign(src)");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(0, 0, 0, 0);
    djinterp::array<int, 4> src(11, 22, 33, 44);

    const std::uint64_t v_before = a.version();
    a.assign(src);
    const std::uint64_t v_after = a.version();

    append_leaf(tree, root,
        ( a.at(0) == 11 && a.at(1) == 22 &&
          a.at(2) == 33 && a.at(3) == 44 ),
        "assign(src): contents replaced wholesale");

    append_leaf(tree, root,
        v_after > v_before,
        "assign(): version bumped");

    // assign() also touches atomic_state.size_lockfree
    append_leaf(tree, root,
        a.size_lockfree() == 4,
        "assign(): size_lockfree() now matches src.size()");

    return tree;
}


array_test_tree
test_threadsafe_array_apply_write(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: apply(fn) under write lock");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(1, 2, 3, 4);
    const std::uint64_t v_before = a.version();

    // apply with a void-returning fn
    a.apply([](djinterp::array<int, 4>& _arr)
        {
            for (std::size_t i = 0; i < _arr.size(); ++i)
            {
                _arr[i] = _arr[i] * 10;
            }
        });

    append_leaf(tree, root,
        ( a.at(0) == 10 && a.at(1) == 20 &&
          a.at(2) == 30 && a.at(3) == 40 ),
        "apply(): mutations visible after the closure");

    append_leaf(tree, root,
        a.version() > v_before,
        "apply(): version bumped");

    // apply with a value-returning fn
    int sum = a.apply(
        [](djinterp::array<int, 4>& _arr) -> int
        {
            int s = 0;
            for (std::size_t i = 0; i < _arr.size(); ++i)
            {
                s += _arr[i];
            }
            return s;
        });

    append_leaf(tree, root,
        sum == 100,
        "apply(): value-returning fn forwards result");

    return tree;
}


array_test_tree
test_threadsafe_array_apply_read(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: apply_read(fn) under read lock");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4>  a(5, 6, 7, 8);
    const std::uint64_t v_before = a.version();

    int sum = a.apply_read(
        [](const djinterp::array<int, 4>& _arr) -> int
        {
            int s = 0;
            for (std::size_t i = 0; i < _arr.size(); ++i)
            {
                s += _arr[i];
            }
            return s;
        });

    append_leaf(tree, root,
        sum == 26,
        "apply_read(): value-returning fn computes 5+6+7+8 == 26");

    append_leaf(tree, root,
        a.version() == v_before,
        "apply_read(): version unchanged "
        "(no mutation expected)");

    return tree;
}


array_test_tree
test_threadsafe_array_batch_guard(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: batch_guard for multi-op atomicity");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(0, 0, 0, 0);

    // batch() returns a batch_guard that holds the write
    // lock for its scope.  std::mutex is non-recursive,
    // so the test thread itself MUST NOT call any other
    // locking method (write_access, set, at, assign) while
    // the batch is alive — those would attempt to re-acquire
    // the same mutex on the same thread, which is undefined
    // behavior (MSVC raises std::system_error in debug).
    //
    // The contract batch_guard offers is mutual exclusion
    // against OTHER threads.  We verify that contract by
    // spawning a writer that attempts a.set() while the
    // batch is alive; the writer must block until the
    // batch's scope ends.

    std::atomic<bool> writer_done{false};
    std::atomic<bool> writer_observed_during_batch{false};

    {
        auto guard = a.batch();

        guard.record();
        guard.record();
        guard.record();
        guard.record();

        // spawn the contender; it must block on a.set()
        // because we hold the write lock.
        std::thread writer(
            [&a, &writer_done]
            {
                a.set(0, 99);
                writer_done.store(true);
            });

        // give the writer a moment to reach the lock.  If
        // batch_guard does its job, writer_done stays false.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(20));

        if (!writer_done.load())
        {
            writer_observed_during_batch.store(true);
        }

        append_leaf(tree, root,
            guard.count() == 4,
            "batch_guard.count() reflects record() calls");

        append_leaf(tree, root,
            writer_observed_during_batch.load(),
            "batch_guard: contending writer blocks while batch alive");

        // releasing the batch unblocks the writer
        // (guard goes out of scope at the closing brace
        // below).
        writer.join();
    }
    // batch released here; writer has now committed its
    // store of 99 to slot 0.

    append_leaf(tree, root,
        a.at(0) == 99,
        "batch: contending writer's update visible after scope ends");

    // batch_guard cannot be copied
    using bg_t = decltype(a.batch());
    append_leaf(tree, root,
        !std::is_copy_constructible<bg_t>::value,
        "batch_guard: NOT copy-constructible");
    append_leaf(tree, root,
        !std::is_copy_assignable<bg_t>::value,
        "batch_guard: NOT copy-assignable");

    return tree;
}


// =========================================================================
// VIII. CATEGORY: THREADSAFE_ARRAY OPTIMISTIC READ
// =========================================================================

array_test_tree
test_threadsafe_array_optimistic_uncontested(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: optimistic() uncontested");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4>  a(10, 20, 30, 40);
    const std::uint64_t v_before = a.version();

    // No writer is contending — the optimistic read
    // should succeed on the first attempt.
    int sum = a.optimistic(
        [](const djinterp::array<int, 4>& _arr) -> int
        {
            int s = 0;
            for (std::size_t i = 0; i < _arr.size(); ++i)
            {
                s += _arr[i];
            }
            return s;
        });

    append_leaf(tree, root,
        sum == 100,
        "optimistic() (uncontested): result == 10+20+30+40");

    append_leaf(tree, root,
        a.version() == v_before,
        "optimistic() leaves version untouched");

    return tree;
}


array_test_tree
test_threadsafe_array_optimistic_fallback(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: optimistic() fallback path");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(1, 1, 1, 1);

    // We can't directly test the fallback without
    // genuine contention; instead, we verify that
    // optimistic() is callable with _max_retries=0,
    // forcing the immediate fall-through to a real
    // read lock.
    int v0 = a.optimistic(
        [](const djinterp::array<int, 4>& _arr) -> int
        {
            return _arr[0];
        },
        /*_max_retries=*/ 0);

    append_leaf(tree, root,
        v0 == 1,
        "optimistic(_max_retries=0): falls through to "
        "read lock and returns value");

    // verify that retries=1 still produces a value
    int v1 = a.optimistic(
        [](const djinterp::array<int, 4>& _arr) -> int
        {
            return _arr[3];
        },
        /*_max_retries=*/ 1);

    append_leaf(tree, root,
        v1 == 1,
        "optimistic(_max_retries=1): produces value");

    return tree;
}


// =========================================================================
// IX.  CATEGORY: THREADSAFE_ARRAY SNAPSHOT
// =========================================================================

array_test_tree
test_threadsafe_array_snapshot_content(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: snapshot content matches source");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(7, 8, 9, 10);

    auto snap = a.snapshot();

    append_leaf(tree, root,
        snap.size() == 4,
        "snapshot.size() == 4");
    append_leaf(tree, root,
        !snap.empty(),
        "snapshot non-empty");
    append_leaf(tree, root,
        ( snap[0] == 7 && snap[1] == 8 &&
          snap[2] == 9 && snap[3] == 10 ),
        "snapshot[i] == source[i] at capture");

    // iterable
    int sum = 0;
    for (auto it = snap.begin(); it != snap.end(); ++it)
    {
        sum += *it;
    }
    append_leaf(tree, root,
        sum == 34,
        "iterating snapshot sums elements correctly");

    return tree;
}


array_test_tree
test_threadsafe_array_snapshot_independence(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: snapshot independent of source");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(1, 2, 3, 4);
    auto snap = a.snapshot();

    // mutate the source after capturing the snapshot
    a.set(0, 999);
    a.set(1, 999);
    a.set(2, 999);
    a.set(3, 999);

    append_leaf(tree, root,
        ( snap[0] == 1 && snap[1] == 2 &&
          snap[2] == 3 && snap[3] == 4 ),
        "snapshot unaffected by post-capture writes");

    // source observably changed
    append_leaf(tree, root,
        ( a.at(0) == 999 && a.at(3) == 999 ),
        "source updated independently of snapshot");

    return tree;
}


// =========================================================================
// X.   CATEGORY: THREADSAFE_ARRAY CONVENIENCE ALIASES
// =========================================================================

array_test_tree
test_threadsafe_array_alias_mutex(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: mutex_array alias");
    auto* root = tree.underlying().root();

    using mtx_t = djinterp::mutex_array<int, 4>;

    // mutex_array must be a threadsafe_array with the
    // exclusive_lock_policy.
    append_leaf(tree, root,
        std::is_same<
            typename mtx_t::lock_policy_type,
            djinterp::exclusive_lock_policy>::value,
        "mutex_array uses exclusive_lock_policy");

    append_leaf(tree, root,
        mtx_t::is_threadsafe(),
        "mutex_array::is_threadsafe() == true");

    append_leaf(tree, root,
        !mtx_t::supports_shared(),
        "mutex_array::supports_shared() == false");

    return tree;
}


array_test_tree
test_threadsafe_array_alias_timed(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: timed_array alias");
    auto* root = tree.underlying().root();

    using tim_t = djinterp::timed_array<int, 4>;

    append_leaf(tree, root,
        std::is_same<
            typename tim_t::lock_policy_type,
            djinterp::timed_lock_policy>::value,
        "timed_array uses timed_lock_policy");

    append_leaf(tree, root,
        tim_t::is_threadsafe(),
        "timed_array::is_threadsafe() == true");

    append_leaf(tree, root,
        tim_t::supports_timed(),
        "timed_array::supports_timed() == true");

    return tree;
}


array_test_tree
test_threadsafe_array_alias_shared_cpp17(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: shared_array alias (C++17)");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using shr_t = djinterp::shared_array<int, 4>;

    append_leaf(tree, root,
        std::is_same<
            typename shr_t::lock_policy_type,
            djinterp::shared_lock_policy>::value,
        "shared_array uses shared_lock_policy");

    append_leaf(tree, root,
        shr_t::is_threadsafe(),
        "shared_array::is_threadsafe() == true");

    append_leaf(tree, root,
        shr_t::supports_shared(),
        "shared_array::supports_shared() == true");
#else
    append_leaf(tree, root,
        true,
        "shared_array test skipped (requires C++17)");
#endif

    return tree;
}


// =========================================================================
// XI.  CATEGORY: THREADSAFE_ARRAY POLICY VARIATION
// =========================================================================

array_test_tree
test_threadsafe_array_policy_null(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: null_lock_policy (zero overhead)");
    auto* root = tree.underlying().root();

    using null_t = djinterp::threadsafe_array<int, 4>;

    // null_lock_policy is the project default for the
    // primary template — verify it reports as such.
    append_leaf(tree, root,
        std::is_same<
            typename null_t::lock_policy_type,
            djinterp::default_lock_policy>::value,
        "default policy == default_lock_policy");

    null_t a(1, 2, 3, 4);
    a.set(0, 11);
    append_leaf(tree, root,
        a.at(0) == 11,
        "null-policy set/at round-trips correctly");

    return tree;
}


array_test_tree
test_threadsafe_array_policy_exclusive(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: exclusive_lock_policy");
    auto* root = tree.underlying().root();

    ts_mtx_arr<int, 4> a(1, 2, 3, 4);
    a.set(2, 99);

    append_leaf(tree, root,
        a.at(2) == 99,
        "exclusive-policy set/at round-trips correctly");

    {
        auto handle = a.write_access();
        (*handle)[0] = 100;
        // handle's destructor releases the lock at scope exit
    }

    append_leaf(tree, root,
        a.at(0) == 100,
        "exclusive-policy: write through handle survives "
        "scope exit");

    return tree;
}


array_test_tree
test_threadsafe_array_policy_timed(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: timed_lock_policy");
    auto* root = tree.underlying().root();

    ts_tim_arr<int, 4> a(5, 6, 7, 8);
    append_leaf(tree, root,
        ts_tim_arr<int, 4>::supports_timed(),
        "timed policy reports supports_timed() == true");

    a.set(0, 50);
    append_leaf(tree, root,
        a.at(0) == 50,
        "timed-policy set/at round-trips correctly");

    return tree;
}


array_test_tree
test_threadsafe_array_policy_shared_cpp17(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: shared_lock_policy (C++17)");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    ts_shr_arr<int, 4> a(1, 2, 3, 4);

    append_leaf(tree, root,
        ts_shr_arr<int, 4>::supports_shared(),
        "shared policy reports supports_shared() == true");

    // multiple read handles can coexist
    {
        auto h1 = a.read_access();
        auto h2 = a.read_access();
        append_leaf(tree, root,
            ( (*h1).size() == 4 && (*h2).size() == 4 ),
            "two concurrent read handles can coexist");
    }

    a.set(1, 99);
    append_leaf(tree, root,
        a.at(1) == 99,
        "shared-policy set/at round-trips correctly");
#else
    append_leaf(tree, root,
        true,
        "shared policy test skipped (requires C++17)");
#endif

    return tree;
}


// =========================================================================
// XII. CATEGORY: THREADSAFE_ARRAY CONCURRENT ACCESS
// =========================================================================

array_test_tree
test_threadsafe_array_concurrent_readers(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: concurrent readers");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    ts_mtx_arr<int, 4> a(10, 20, 30, 40);

    // four threads, each computes the sum 1000 times.
    // Every read must observe the same fixed value
    // since no writers are running.
    constexpr std::size_t kThreads    = 4;
    constexpr std::size_t kIterations = 1000;
    std::atomic<int>      mismatch_count(0);

    run_threads(kThreads,
        [&](std::size_t /*_id*/)
        {
            for (std::size_t i = 0; i < kIterations; ++i)
            {
                const int s = a.at(0) + a.at(1) +
                              a.at(2) + a.at(3);

                if (s != 100)
                {
                    mismatch_count.fetch_add(
                        1, std::memory_order_relaxed);
                }
            }
        });

    append_leaf(tree, root,
        mismatch_count.load() == 0,
        "concurrent readers: no inconsistent sum observed");

    append_leaf(tree, root,
        ( a.at(0) == 10 && a.at(3) == 40 ),
        "concurrent readers: source unchanged after run");
#else
    append_leaf(tree, root,
        true,
        "concurrent reader test skipped (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_threadsafe_array_concurrent_writers(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: concurrent writers");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    ts_mtx_arr<int, 4> a(0, 0, 0, 0);

    // four threads each call set(i, ...) on a single
    // dedicated index.  Since each thread owns its own
    // index, the final values must equal the writer's
    // assigned constant — this verifies serialization.
    constexpr std::size_t kThreads = 4;

    run_threads(kThreads,
        [&](std::size_t _id)
        {
            // each writer hammers its slot 500 times
            for (std::size_t i = 0; i < 500; ++i)
            {
                a.set(_id, static_cast<int>(_id + 1));
            }
        });

    append_leaf(tree, root,
        ( a.at(0) == 1 && a.at(1) == 2 &&
          a.at(2) == 3 && a.at(3) == 4 ),
        "concurrent writers (disjoint slots): "
        "final values match writer ids");
#else
    append_leaf(tree, root,
        true,
        "concurrent writer test skipped (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_threadsafe_array_concurrent_mixed(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: concurrent reader/writer mix");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    ts_mtx_arr<int, 4> a(0, 0, 0, 0);

    // 4 writers, 4 readers.  No invariant is checked
    // beyond "no torn reads / no crashes."  We rely on
    // the test harness's reader sum being a multiple of
    // some pattern that no incoherent state could
    // produce.

    constexpr std::size_t kTotal     = 8;
    std::atomic<int>      crash_flag(0);

    run_threads(kTotal,
        [&](std::size_t _id)
        {
            try
            {
                if (_id < 4)  // writers
                {
                    for (std::size_t i = 0; i < 500; ++i)
                    {
                        a.set(_id, static_cast<int>(i));
                    }
                }
                else          // readers
                {
                    for (std::size_t i = 0; i < 500; ++i)
                    {
                        const int v = a.at(_id - 4);
                        // any value in [0, 500) is OK;
                        // a value outside that range
                        // would indicate corruption.
                        if (v < 0 || v >= 500)
                        {
                            crash_flag.fetch_add(
                                1, std::memory_order_relaxed);
                        }
                    }
                }
            }
            catch (...)
            {
                crash_flag.fetch_add(
                    1, std::memory_order_relaxed);
            }
        });

    append_leaf(tree, root,
        crash_flag.load() == 0,
        "concurrent mixed: no out-of-range value, "
        "no exception observed");
#else
    append_leaf(tree, root,
        true,
        "concurrent mixed test skipped (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_threadsafe_array_concurrent_version_monotonic(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array: version monotonic under writers");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    ts_mtx_arr<int, 4> a(0, 0, 0, 0);

    constexpr std::size_t kThreads      = 4;
    constexpr std::size_t kPerThreadOps = 250;

    const std::uint64_t v_initial = a.version();

    run_threads(kThreads,
        [&](std::size_t _id)
        {
            for (std::size_t i = 0; i < kPerThreadOps; ++i)
            {
                a.set(_id, static_cast<int>(i));
            }
        });

    const std::uint64_t v_final = a.version();

    // version must have increased by exactly the
    // total number of set() calls (each set bumps by
    // 1).  The test verifies the version has at least
    // increased by that count — under contention each
    // call still increments.
    append_leaf(tree, root,
        v_final >= v_initial + (kThreads * kPerThreadOps),
        "version increased by >= total mutations after "
        "concurrent writers");
#else
    append_leaf(tree, root,
        true,
        "version monotonic test skipped (requires C++11)");
#endif

    return tree;
}


// XIII. CATEGORY: WRAPPER EDGE CASES (CROSS-CUTTING)
// =========================================================================

array_test_tree
test_threadsafe_edge_zero_extent(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "edge: zero-extent containers");

    root = tree.underlying().root();

    // zero-extent threadsafe_array
    {
        ts_arr<int, 0> a;

        append_leaf(tree, root,
            a.size() == 0u,
            "threadsafe_array<int,0>.size() == 0");

        append_leaf(tree, root,
            a.empty(),
            "threadsafe_array<int,0>.empty() == true");

        append_leaf(tree, root,
            a.size_lockfree() == 0u,
            "threadsafe_array<int,0>.size_lockfree() == 0");
    }

    // zero-extent atomic_array
    {
        at_arr<int, 0> a;

        append_leaf(tree, root,
            a.size() == 0u,
            "atomic_array<int,0>.size() == 0");

        append_leaf(tree, root,
            a.empty(),
            "atomic_array<int,0>.empty() == true");

        append_leaf(tree, root,
            a.begin() == a.end(),
            "atomic_array<int,0>: begin() == end()");
    }

    // zero-extent cow_array
    {
        cow_arr<int, 0> a;

        append_leaf(tree, root,
            a.size() == 0u,
            "cow_array<int,0>.size() == 0");

        append_leaf(tree, root,
            a.empty(),
            "cow_array<int,0>.empty() == true");

        // snapshot of a zero-extent cow_array must be
        // valid and observably empty.
        auto snap = a.snapshot();

        append_leaf(tree, root,
            snap->size() == 0u,
            "cow_array<int,0>: snapshot()->size() == 0");
    }

    return tree;
}


array_test_tree
test_threadsafe_edge_single_element(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "edge: single-element containers");

    root = tree.underlying().root();

    // single-element threadsafe_array
    {
        ts_arr<int, 1> a;

        a.set(0, 42);

        append_leaf(tree, root,
            a.size() == 1u && a.at(0) == 42,
            "threadsafe_array<int,1> set/get round-trip");

        append_leaf(tree, root,
            !a.empty(),
            "threadsafe_array<int,1>.empty() == false");
    }

    // single-element atomic_array
    {
        at_arr<int, 1> a;

        a.store(0, 7);
        int v = a.fetch_add(0, 5);

        append_leaf(tree, root,
            v == 7 && a.load(0) == 12,
            "atomic_array<int,1> store / fetch_add chain");
    }

    // single-element cow_array
    {
        cow_arr<int, 1> a;

        a.set(0, 99);

        append_leaf(tree, root,
            a.at(0) == 99 && a.version() > 0u,
            "cow_array<int,1> set / at / version");
    }

    return tree;
}


array_test_tree
test_threadsafe_edge_large_extent(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree;
    node_alias*          root;

    tree = make_block_tree(
        "edge: large-extent containers");

    root = tree.underlying().root();

    constexpr std::size_t kLarge = 1024;

    // large threadsafe_array
    {
        ts_arr<int, kLarge> a;

        for (std::size_t i = 0; i < kLarge; ++i)
        {
            a.set(i, static_cast<int>(i));
        }

        bool ok = true;
        for (std::size_t i = 0; i < kLarge; ++i)
        {
            if (a.at(i) != static_cast<int>(i))
            {
                ok = false;
                break;
            }
        }

        append_leaf(tree, root,
            ok,
            "threadsafe_array<int,1024>: 1024-element "
            "round-trip");

        append_leaf(tree, root,
            a.size() == kLarge,
            "threadsafe_array<int,1024>.size() == 1024");
    }

    // large atomic_array
    {
        at_arr<int, kLarge> a;

        a.fill(0);

        for (std::size_t i = 0; i < kLarge; ++i)
        {
            a.store(i, static_cast<int>(i));
        }

        bool ok = true;
        for (std::size_t i = 0; i < kLarge; ++i)
        {
            if (a.load(i) != static_cast<int>(i))
            {
                ok = false;
                break;
            }
        }

        append_leaf(tree, root,
            ok,
            "atomic_array<int,1024>: 1024-element "
            "store / load round-trip");
    }

    // large cow_array via single modify()
    {
        cow_arr<int, kLarge> a;

        a.modify(
            [](base_mi<int, kLarge>& _arr)
            {
                for (std::size_t i = 0; i < kLarge; ++i)
                {
                    _arr[i] = static_cast<int>(i);
                }
            });

        auto snap = a.snapshot();

        bool ok = true;
        for (std::size_t i = 0; i < kLarge; ++i)
        {
            if ((*snap)[i] != static_cast<int>(i))
            {
                ok = false;
                break;
            }
        }

        append_leaf(tree, root,
            ok,
            "cow_array<int,1024>: 1024-element bulk modify "
            "via single closure");

        append_leaf(tree, root,
            a.version() == 1u,
            "cow_array<int,1024>: bulk modify is one "
            "version bump");
    }

    return tree;
}


// =========================================================================
// XIV.  SUB-BUILDER:  threadsafe_array MODULE SUBTREE
// =========================================================================

// make_threadsafe_array_subtree
//   Aggregates the threadsafe_array category functions into a
// single subtree rooted under a threadsafe_array test-block.
// Called by the suite-level aggregate builder
// make_threadsafe_array_test_subtree below; also callable
// directly by users who want to drive only the threadsafe_array
// tests against a custom test_handler.
//   This sub-builder owns the cross-cutting tests too: axis
// preservation, strategy disjointness, and the cube edge cases —
// because each of those tests touches all three wrappers and so
// has no single natural home.  The other sub-builders
// (make_atomic_array_subtree, make_cow_array_subtree) cover
// only their own container.
array_test_tree
make_threadsafe_array_subtree(
    test::test_type_id _kind
)
{
    return combine_subtrees<array_test_tree>(
        array_test_obj(_kind, true,
            "threadsafe_array test module"),
        {
            // cross-cutting trait conformance
            test_threadsafe_array_traits_axis_preservation(_kind),
            test_threadsafe_traits_strategy_disjointness(_kind),

            // threadsafe_array's own strategy
            test_threadsafe_array_traits_strategy_locked(_kind),

            // construction
            test_threadsafe_array_default_construction(_kind),
            test_threadsafe_array_pack_construction(_kind),
            test_threadsafe_array_copy_construction(_kind),
            test_threadsafe_array_copy_assignment(_kind),
            test_threadsafe_array_move_deletion_sfinae(_kind),

            // lock-free queries
            test_threadsafe_array_size_lockfree(_kind),
            test_threadsafe_array_version_query(_kind),
            test_threadsafe_array_empty_lockfree(_kind),

            // single-op access
            test_threadsafe_array_size_locked(_kind),
            test_threadsafe_array_at_returns_value(_kind),
            test_threadsafe_array_set_visible(_kind),
            test_threadsafe_array_set_bumps_version(_kind),

            // handle-based access
            test_threadsafe_array_read_access(_kind),
            test_threadsafe_array_write_access(_kind),
            test_threadsafe_array_handle_lifetime(_kind),

            // bulk operations
            test_threadsafe_array_assign(_kind),
            test_threadsafe_array_apply_write(_kind),
            test_threadsafe_array_apply_read(_kind),
            test_threadsafe_array_batch_guard(_kind),

            // optimistic read
            test_threadsafe_array_optimistic_uncontested(_kind),
            test_threadsafe_array_optimistic_fallback(_kind),

            // snapshot
            test_threadsafe_array_snapshot_content(_kind),
            test_threadsafe_array_snapshot_independence(_kind),

            // convenience aliases
            test_threadsafe_array_alias_mutex(_kind),
            test_threadsafe_array_alias_timed(_kind),
            test_threadsafe_array_alias_shared_cpp17(_kind),

            // policy variation
            test_threadsafe_array_policy_null(_kind),
            test_threadsafe_array_policy_exclusive(_kind),
            test_threadsafe_array_policy_timed(_kind),
            test_threadsafe_array_policy_shared_cpp17(_kind),

            // concurrent access
            test_threadsafe_array_concurrent_readers(_kind),
            test_threadsafe_array_concurrent_writers(_kind),
            test_threadsafe_array_concurrent_mixed(_kind),
            test_threadsafe_array_concurrent_version_monotonic(_kind),

            // cross-cutting edge cases
            test_threadsafe_edge_zero_extent(_kind),
            test_threadsafe_edge_single_element(_kind),
            test_threadsafe_edge_large_extent(_kind),
        });
}


// =========================================================================
// XV.   AGGREGATE SUBTREE BUILDER
// =========================================================================

// make_threadsafe_array_test_subtree
//   suite-wide aggregate.  Composes the three per-container
// sub-builders (defined here and in atomic_array_core_tests.cpp /
// cow_array_core_tests.cpp) under a single module root.
//   Callers wanting only one container's tests can invoke the
// matching sub-builder directly.
array_test_tree
make_threadsafe_array_test_subtree(
    test::test_type_id _kind
)
{
    return combine_subtrees<array_test_tree>(
        array_test_obj(_kind, true,
            "threadsafe-array suite (aggregate)"),
        {
            make_threadsafe_array_subtree(_kind),
            make_atomic_array_subtree(_kind),
            make_cow_array_subtree(_kind),
        });
}


// =========================================================================
// XVI.  MASTER-SUITE RUNNER
// =========================================================================

test::session_verdict
run_threadsafe_array_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind,
    double*             _out_seconds
)
{
    test::test_session<array_test_obj,
                       djinterp::nary_tree<array_test_obj>> session;

    session.tree() = make_threadsafe_array_test_subtree(_kind);

    test::session_verdict v = session.run(_handler);

    if (_out_seconds != nullptr)
    {
        *_out_seconds =
            std::chrono::duration<double>(
                session.elapsed()).count();
    }

    return v;
}


// =========================================================================
// XVII. COMBINED SUITE: PART A + PART B
// =========================================================================

// make_combined_test_subtree
//   Aggregates Part A (base array container) and Part B
// (threadsafe wrappers) under one suite root, in document
// order.  Calls the existing top-level builders and combines
// the results via test_tree's combine_subtrees factory.
array_test_tree
make_combined_test_subtree(
    test::test_type_id _kind
)
{
    return test::combine_subtrees<array_test_tree>(
        array_test_obj(_kind, true,
            "array suite (combined: base + threadsafe)"),
        {
            make_array_test_subtree(_kind),
            make_threadsafe_array_test_subtree(_kind),
        });
}


// run_combined_suite
//   Drives the combined Part A + Part B subtree against the
// supplied handler.  Routes through test_session::run() so the
// session's intrinsic timer and counters are populated as a
// side-effect; the _out_seconds parameter is preserved for
// API compatibility with run_array_suite() / run_threadsafe_
// array_suite().
test::session_verdict
run_combined_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind,
    double*             _out_seconds
)
{
    test::test_session<array_test_obj,
                       djinterp::nary_tree<array_test_obj>> session;

    session.tree() = make_combined_test_subtree(_kind);

    test::session_verdict v = session.run(_handler);

    if (_out_seconds != nullptr)
    {
        *_out_seconds = 
            std::chrono::duration<double>(session.elapsed()).count();
    }

    return v;
}


NS_END  // testing
NS_END  // djinterp