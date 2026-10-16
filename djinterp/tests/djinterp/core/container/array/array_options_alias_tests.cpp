/*******************************************************************************
* djinterp [testing]                               array_options_alias_tests.cpp
*
*   Implementation of the convenience-alias resolution category of Part D
* (array options) of the array test suite, under the return-a-subtree protocol
* declared in array_tests.hpp.
* STRUCTURE:
*   Every category function below builds and returns a self-
* contained `array_test_tree`.  Tests do NOT take a test_handler
* or a test_printer; they simply construct a small tree and
* return it by value.
*   This translation unit owns the convenience-alias
* resolution category - the lifetime x iterability cube
* (six named cells: mutable_iterable, mutable_non_iterable,
* immutable_iterable, immutable_non_iterable, constexpr_
* iterable, constexpr_non_iterable) plus the thread-safety
* convenience aliases (mutex_array, timed_array,
* shared_array).  Each must resolve to a well-formed
* options-pack instantiation with the documented axis values.
*   The Part D aggregate builder and master runner live in
* array_options_core_tests.cpp; this file's functions are
* called from there.
*
*
* path:      /tests/djinterp/core/container/array/array_options_alias_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.10
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cstddef>
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
    //   helper: constructs a fresh array_test_tree whose root
    // is a test_block-kind interior node carrying _block_name.
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

}  // unnamed namespace


// =========================================================================
// II.  CATEGORY: CONVENIENCE ALIAS RESOLUTION
// =========================================================================

array_test_tree
test_options_alias_mutable_iterable(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "mutable_iterable_array<T, N> resolves to "
        "array<...> with mutable + iterable cell");
    auto* root = tree.underlying().root();

    using cell = djinterp::mutable_iterable_array<int, 4>;

    append_leaf(tree, root,
        ( cell::lifetime ==
          djinterp::array_lifetime::mutable_lifetime ),
        "mutable_iterable_array::lifetime == "
        "mutable_lifetime");

    append_leaf(tree, root,
        ( cell::iterability ==
          djinterp::array_iterability::iterable ),
        "mutable_iterable_array::iterability == iterable");

    append_leaf(tree, root,
        ( cell::extent == 4u ),
        "mutable_iterable_array<int, 4>::extent == 4");

    append_leaf(tree, root,
        std::is_same<typename cell::value_type, int>::value,
        "mutable_iterable_array<int, 4>::value_type == int");

    return tree;
}


array_test_tree
test_options_alias_mutable_non_iterable(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "mutable_non_iterable_array<T, N> resolves to "
        "array<...> with mutable + non-iterable cell");
    auto* root = tree.underlying().root();

    using cell = djinterp::mutable_non_iterable_array<int, 4>;

    append_leaf(tree, root,
        ( cell::lifetime ==
          djinterp::array_lifetime::mutable_lifetime ),
        "mutable_non_iterable_array::lifetime == "
        "mutable_lifetime");

    append_leaf(tree, root,
        ( cell::iterability ==
          djinterp::array_iterability::non_iterable ),
        "mutable_non_iterable_array::iterability == "
        "non_iterable");

    return tree;
}


array_test_tree
test_options_alias_immutable_iterable(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "immutable_iterable_array<T, N> resolves to "
        "array<...> with immutable + iterable cell");
    auto* root = tree.underlying().root();

    using cell = djinterp::immutable_iterable_array<int, 4>;

    append_leaf(tree, root,
        ( cell::lifetime ==
          djinterp::array_lifetime::immutable_lifetime ),
        "immutable_iterable_array::lifetime == "
        "immutable_lifetime");

    append_leaf(tree, root,
        ( cell::iterability ==
          djinterp::array_iterability::iterable ),
        "immutable_iterable_array::iterability == iterable");

    return tree;
}


array_test_tree
test_options_alias_immutable_non_iterable(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "immutable_non_iterable_array<T, N> resolves to "
        "array<...> with immutable + non-iterable cell");
    auto* root = tree.underlying().root();

    using cell = djinterp::immutable_non_iterable_array<int, 4>;

    append_leaf(tree, root,
        ( cell::lifetime ==
          djinterp::array_lifetime::immutable_lifetime ),
        "immutable_non_iterable_array::lifetime == "
        "immutable_lifetime");

    append_leaf(tree, root,
        ( cell::iterability ==
          djinterp::array_iterability::non_iterable ),
        "immutable_non_iterable_array::iterability == "
        "non_iterable");

    return tree;
}


array_test_tree
test_options_alias_constexpr_iterable(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "constexpr_iterable_array<T, N> resolves to "
        "array<...> with constexpr + iterable cell");
    auto* root = tree.underlying().root();

    using cell = djinterp::constexpr_iterable_array<int, 4>;

    append_leaf(tree, root,
        ( cell::lifetime ==
          djinterp::array_lifetime::constexpr_lifetime ),
        "constexpr_iterable_array::lifetime == "
        "constexpr_lifetime");

    append_leaf(tree, root,
        ( cell::iterability ==
          djinterp::array_iterability::iterable ),
        "constexpr_iterable_array::iterability == iterable");

    return tree;
}


array_test_tree
test_options_alias_constexpr_non_iterable(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "constexpr_non_iterable_array<T, N> resolves to "
        "array<...> with constexpr + non-iterable cell");
    auto* root = tree.underlying().root();

    using cell = djinterp::constexpr_non_iterable_array<int, 4>;

    append_leaf(tree, root,
        ( cell::lifetime ==
          djinterp::array_lifetime::constexpr_lifetime ),
        "constexpr_non_iterable_array::lifetime == "
        "constexpr_lifetime");

    append_leaf(tree, root,
        ( cell::iterability ==
          djinterp::array_iterability::non_iterable ),
        "constexpr_non_iterable_array::iterability == "
        "non_iterable");

    return tree;
}


array_test_tree
test_options_alias_mutex_array(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "mutex_array<...> alias resolves with "
        "exclusive_lock_policy");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using m_arr = djinterp::mutex_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        std::is_same<typename m_arr::lock_policy_type,
                     djinterp::exclusive_lock_policy>::value,
        "mutex_array<...>::lock_policy_type == "
        "exclusive_lock_policy");

    append_leaf(tree, root,
        std::is_same<typename m_arr::value_type, int>::value,
        "mutex_array<...>::value_type == int");

    append_leaf(tree, root,
        ( m_arr::extent == 4u ),
        "mutex_array<...>::extent == 4");

    // first-match-wins: an explicit policy supplied by the
    // user takes precedence over the alias's appended policy.
    using m_arr_user = djinterp::mutex_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lock_policy<
            djinterp::null_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename m_arr_user::lock_policy_type,
                     djinterp::null_lock_policy>::value,
        "user-supplied lock_policy wins over the alias's "
        "appended policy");
#else
    append_leaf(tree, root, true,
        "mutex_array alias test deferred (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_options_alias_timed_array(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "timed_array<...> alias resolves with "
        "timed_lock_policy");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using t_arr = djinterp::timed_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        std::is_same<typename t_arr::lock_policy_type,
                     djinterp::timed_lock_policy>::value,
        "timed_array<...>::lock_policy_type == "
        "timed_lock_policy");
#else
    append_leaf(tree, root, true,
        "timed_array alias test deferred (requires C++11)");
#endif

    return tree;
}


array_test_tree
test_options_alias_shared_array_cpp17(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "shared_array<...> alias resolves with "
        "shared_lock_policy (C++17+)");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using s_arr = djinterp::shared_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        std::is_same<typename s_arr::lock_policy_type,
                     djinterp::shared_lock_policy>::value,
        "shared_array<...>::lock_policy_type == "
        "shared_lock_policy");
#else
    append_leaf(tree, root, true,
        "shared_array alias test deferred (requires C++17)");
#endif

    return tree;
}


NS_END  // testing
NS_END  // djinterp
