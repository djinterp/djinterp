/*******************************************************************************
* djinterp [testing]                                array_options_edge_tests.cpp
*
*   Implementation of the lock-policy-variation and edge-case
* categories of Part D (array options) of the array test
* suite, under the return-a-subtree protocol declared in
* array_tests.hpp.
*
*   STRUCTURE:
*   Every category function below builds and returns a self-
* contained `DArrayTestTree`.  Tests do NOT take a test_handler
* or a DTestPrinter; they simply construct a small tree and
* return it by value.
*
*   This translation unit owns the boundary-case categories of
* Part D - explicit lock-policy resolution across all
* documented policies (null / exclusive / timed / shared)
* and unusual configurations (extent 0 / 1 / 1024, plus
* pointer / user struct / signed&unsigned integer element
* types).
*
*   The Part D aggregate builder and master runner live in
* array_options_core_tests.cpp; this file's functions are
* called from there.
*
*
* path:      /tests/djinterp/cpp/container/array/array_options_edge_tests.cpp
*                 array_options_edge_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.10
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
// djinterp
#include "./array_tests.hpp"


NS_DJINTERP
NS_TESTING


using namespace djinterp::test;


// =========================================================================
// I.   FILE-INTERNAL HELPERS
// =========================================================================

namespace {

    // make_block_tree
    //   helper: constructs a fresh DArrayTestTree whose root
    // is a test_block-kind interior node carrying _block_name.
    inline
    DArrayTestTree make_block_tree(
        const char* _block_name
    )
    {
        DArrayTestTree result;

        result.underlying().emplace_root(
            test::make_test_block(_block_name));

        return result;
    }


    // DProbeStructValue
    //   type: a small trivially-copyable struct used as an
    // element type to verify that arrays accept user-defined
    // value types via array_opt_type<T>.
    struct DProbeStructValue
    {
        int  a;
        char b;

        bool operator==(
            const DProbeStructValue& _o
        ) const
        {
            return ( (a == _o.a) && (b == _o.b) );
        }
    };

}  // unnamed namespace


// =========================================================================
// II.  CATEGORY: LOCK POLICY VARIATION
// =========================================================================
//   Threadsafe and cow containers must accept every documented
// lock policy via DContainerOptLockPolicy<P>.

DArrayTestTree
test_options_lock_policy_explicit_null(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "container_opt_lock_policy<null_lock_policy> "
        "resolves on threadsafe_array and cow_array");
    auto* root = tree.underlying().root();

    using ts_null = djinterp::DThreadsafeArray<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<4>,
        djinterp::DContainerOptLockPolicy<
            djinterp::null_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename ts_null::lock_policy_type,
                     djinterp::null_lock_policy>::value,
        "threadsafe_array with null policy resolves "
        "correctly");

    using cow_null = djinterp::DCowArray<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<4>,
        djinterp::DContainerOptLockPolicy<
            djinterp::null_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename cow_null::lock_policy_type,
                     djinterp::null_lock_policy>::value,
        "cow_array with explicit null policy resolves "
        "correctly");

    return tree;
}


DArrayTestTree
test_options_lock_policy_explicit_exclusive(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "container_opt_lock_policy<exclusive_lock_policy> "
        "resolves on threadsafe_array and cow_array");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using ts_excl = djinterp::DThreadsafeArray<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<4>,
        djinterp::DContainerOptLockPolicy<
            djinterp::exclusive_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename ts_excl::lock_policy_type,
                     djinterp::exclusive_lock_policy>::value,
        "threadsafe_array with exclusive policy resolves "
        "correctly");

    using cow_excl = djinterp::DCowArray<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<4>,
        djinterp::DContainerOptLockPolicy<
            djinterp::exclusive_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename cow_excl::lock_policy_type,
                     djinterp::exclusive_lock_policy>::value,
        "cow_array with exclusive policy resolves correctly");
#else
    append_leaf(tree, root, true,
        "exclusive policy test deferred (requires C++11)");
#endif

    return tree;
}


DArrayTestTree
test_options_lock_policy_explicit_timed(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "container_opt_lock_policy<timed_lock_policy> "
        "resolves on threadsafe_array and cow_array");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using ts_tim = djinterp::DThreadsafeArray<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<4>,
        djinterp::DContainerOptLockPolicy<
            djinterp::timed_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename ts_tim::lock_policy_type,
                     djinterp::timed_lock_policy>::value,
        "threadsafe_array with timed policy resolves "
        "correctly");
#else
    append_leaf(tree, root, true,
        "timed policy test deferred (requires C++11)");
#endif

    return tree;
}


DArrayTestTree
test_options_lock_policy_explicit_shared_cpp17(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "container_opt_lock_policy<shared_lock_policy> "
        "resolves on threadsafe_array (C++17+)");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using ts_shr = djinterp::DThreadsafeArray<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<4>,
        djinterp::DContainerOptLockPolicy<
            djinterp::shared_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename ts_shr::lock_policy_type,
                     djinterp::shared_lock_policy>::value,
        "threadsafe_array with shared policy resolves "
        "correctly");
#else
    append_leaf(tree, root, true,
        "shared policy test deferred (requires C++17)");
#endif

    return tree;
}


// =========================================================================
// III. CATEGORY: EDGE CASES
// =========================================================================

DArrayTestTree
test_options_edge_extent_zero(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "edge case: array_opt_extent<0> compiles and "
        "produces a zero-size container");
    auto* root = tree.underlying().root();

    using zero_arr = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<0>>;

    append_leaf(tree, root,
        ( zero_arr::extent == 0u ),
        "extent == 0 compiles");

    zero_arr a;
    append_leaf(tree, root,
        ( a.size() == 0u ),
        "instance.size() == 0");

    append_leaf(tree, root,
        a.empty(),
        "instance.empty() == true");

    return tree;
}


DArrayTestTree
test_options_edge_extent_one(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "edge case: array_opt_extent<1> compiles and "
        "produces a single-element container");
    auto* root = tree.underlying().root();

    using one_arr = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::DArrayOptExtent<1>>;

    append_leaf(tree, root,
        ( one_arr::extent == 1u ),
        "extent == 1 compiles");

    one_arr a(42);
    append_leaf(tree, root,
        ( a.size() == 1u ),
        "instance.size() == 1");

    append_leaf(tree, root,
        ( a.front() == 42 ),
        "instance carries the supplied value");

    append_leaf(tree, root,
        ( a.front() == a.back() ),
        "front() == back() for single-element array");

    return tree;
}


DArrayTestTree
test_options_edge_extent_large(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "edge case: array_opt_extent<1024> compiles and "
        "operates correctly");
    auto* root = tree.underlying().root();

    using large_arr = djinterp::array<
        djinterp::array_opt_type<std::uint8_t>,
        djinterp::DArrayOptExtent<1024>>;

    append_leaf(tree, root,
        ( large_arr::extent == 1024u ),
        "extent == 1024 compiles");

    large_arr a;
    append_leaf(tree, root,
        ( a.size() == 1024u ),
        "instance.size() == 1024");

    a.fill(static_cast<std::uint8_t>(7));
    append_leaf(tree, root,
        ( ( a.front() == 7u ) &&
          ( a.back()  == 7u ) ),
        "fill() sweeps the whole large array");

    return tree;
}


DArrayTestTree
test_options_edge_pointer_element_type(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "edge case: pointer element type is accepted via "
        "array_opt_type<T*>");
    auto* root = tree.underlying().root();

    using ptr_arr = djinterp::array<
        djinterp::array_opt_type<int*>,
        djinterp::DArrayOptExtent<4>>;

    append_leaf(tree, root,
        std::is_same<typename ptr_arr::value_type, int*>::value,
        "value_type == int*");

    int x = 42;
    int y = 99;
    ptr_arr a;
    a[0] = &x;
    a[1] = &y;

    append_leaf(tree, root,
        ( ( a[0] == &x ) && ( a[1] == &y ) ),
        "pointer values stored and retrieved correctly");

    return tree;
}


DArrayTestTree
test_options_edge_struct_element_type(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "edge case: user struct element type is accepted "
        "via array_opt_type<MyStruct>");
    auto* root = tree.underlying().root();

    using struct_arr = djinterp::array<
        djinterp::array_opt_type<DProbeStructValue>,
        djinterp::DArrayOptExtent<2>>;

    append_leaf(tree, root,
        std::is_same<typename struct_arr::value_type,
                     DProbeStructValue>::value,
        "value_type == probe_struct_value");

    struct_arr a;
    a[0] = DProbeStructValue{1, 'a'};
    a[1] = DProbeStructValue{2, 'b'};

    append_leaf(tree, root,
        ( ( a[0] == DProbeStructValue{1, 'a'} ) &&
          ( a[1] == DProbeStructValue{2, 'b'} ) ),
        "struct values round-trip through subscript");

    return tree;
}


DArrayTestTree
test_options_edge_signed_unsigned_element_types(
    test::DTestTypeId /*_kind*/
)
{
    DArrayTestTree tree = make_block_tree(
        "edge case: every standard integer width is "
        "accepted as the element type");
    auto* root = tree.underlying().root();

    using a_i8  = djinterp::array<
        djinterp::array_opt_type<std::int8_t>,
        djinterp::DArrayOptExtent<2>>;
    using a_u8  = djinterp::array<
        djinterp::array_opt_type<std::uint8_t>,
        djinterp::DArrayOptExtent<2>>;
    using a_i16 = djinterp::array<
        djinterp::array_opt_type<std::int16_t>,
        djinterp::DArrayOptExtent<2>>;
    using a_u16 = djinterp::array<
        djinterp::array_opt_type<std::uint16_t>,
        djinterp::DArrayOptExtent<2>>;
    using a_i32 = djinterp::array<
        djinterp::array_opt_type<std::int32_t>,
        djinterp::DArrayOptExtent<2>>;
    using a_u32 = djinterp::array<
        djinterp::array_opt_type<std::uint32_t>,
        djinterp::DArrayOptExtent<2>>;
    using a_i64 = djinterp::array<
        djinterp::array_opt_type<std::int64_t>,
        djinterp::DArrayOptExtent<2>>;
    using a_u64 = djinterp::array<
        djinterp::array_opt_type<std::uint64_t>,
        djinterp::DArrayOptExtent<2>>;

    append_leaf(tree, root,
        std::is_same<typename a_i8::value_type,
                     std::int8_t>::value,
        "int8_t element type accepted");

    append_leaf(tree, root,
        std::is_same<typename a_u8::value_type,
                     std::uint8_t>::value,
        "uint8_t element type accepted");

    append_leaf(tree, root,
        std::is_same<typename a_i16::value_type,
                     std::int16_t>::value,
        "int16_t element type accepted");

    append_leaf(tree, root,
        std::is_same<typename a_u16::value_type,
                     std::uint16_t>::value,
        "uint16_t element type accepted");

    append_leaf(tree, root,
        std::is_same<typename a_i32::value_type,
                     std::int32_t>::value,
        "int32_t element type accepted");

    append_leaf(tree, root,
        std::is_same<typename a_u32::value_type,
                     std::uint32_t>::value,
        "uint32_t element type accepted");

    append_leaf(tree, root,
        std::is_same<typename a_i64::value_type,
                     std::int64_t>::value,
        "int64_t element type accepted");

    append_leaf(tree, root,
        std::is_same<typename a_u64::value_type,
                     std::uint64_t>::value,
        "uint64_t element type accepted");

    return tree;
}


NS_END  // testing
NS_END  // djinterp
