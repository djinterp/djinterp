/*******************************************************************************
* djinterp [testing]                             array_options_factory_tests.cpp
*
*   Implementation of the factory-function and option_list-
* manipulation categories of Part D (array options) of the
* array test suite, under the return-a-subtree protocol
* declared in array_tests.hpp.
*
*   STRUCTURE:
*   Every category function below builds and returns a self-
* contained `array_test_tree`.  Tests do NOT take a test_handler
* or a test_printer; they simply construct a small tree and
* return it by value.
*
*   This translation unit owns the runtime-deduction part of
* Part D - the `make_array`, `make_immutable_array`, and
* `make_array_from_options` factories - and the compile-time
* type-level option_list primitives (prepend / append /
* concat).  These two categories share enough scaffolding
* (variadic-template / alias-template gating, integral_
* constant probes) that they sit naturally in the same TU.
*
*   The Part D aggregate builder and master runner live in
* array_options_core_tests.cpp; this file's functions are
* called from there.
*
*
* path:      /tests/djinterp/core/container/array/array_options_factory_tests.cpp
*                 array_options_factory_tests.cpp
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
// II.  CATEGORY: FACTORY FUNCTIONS
// =========================================================================

array_test_tree
test_options_make_array_factory(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "make_array(...) deduces element type and extent "
        "from arguments");
    auto* root = tree.underlying().root();

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES)
    auto a3 = djinterp::make_array<int>(1, 2, 3);
    using a3_t = decltype(a3);

    append_leaf(tree, root,
        std::is_same<typename a3_t::value_type, int>::value,
        "make_array<int>(1,2,3): value_type == int");

    append_leaf(tree, root,
        ( a3_t::extent == 3u ),
        "make_array<int>(1,2,3): extent == 3 (deduced from "
        "argument count)");

    append_leaf(tree, root,
        ( a3_t::lifetime ==
          djinterp::array_lifetime::mutable_lifetime ),
        "make_array<int>(...): produces a mutable array");

    append_leaf(tree, root,
        ( a3_t::iterability ==
          djinterp::array_iterability::iterable ),
        "make_array<int>(...): produces an iterable array");

    append_leaf(tree, root,
        ( a3.size() == 3u ),
        "make_array<int>(1,2,3).size() == 3");

    append_leaf(tree, root,
        ( ( a3.data()[0] == 1 ) &&
          ( a3.data()[1] == 2 ) &&
          ( a3.data()[2] == 3 ) ),
        "make_array<int>(1,2,3) preserves argument values");
#else
    append_leaf(tree, root, true,
        "make_array test deferred (requires variadic + "
        "alias templates)");
#endif

    return tree;
}


array_test_tree
test_options_make_immutable_array_factory(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "make_immutable_array(...) deduces and stamps "
        "immutable lifetime");
    auto* root = tree.underlying().root();

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES)
    auto b = djinterp::make_immutable_array<int>(10, 20);
    using b_t = decltype(b);

    append_leaf(tree, root,
        std::is_same<typename b_t::value_type, int>::value,
        "make_immutable_array<int>(...): value_type == int");

    append_leaf(tree, root,
        ( b_t::extent == 2u ),
        "make_immutable_array<int>(10, 20): extent == 2");

    append_leaf(tree, root,
        ( b_t::lifetime ==
          djinterp::array_lifetime::immutable_lifetime ),
        "make_immutable_array<int>(...): lifetime == "
        "immutable_lifetime");
#else
    append_leaf(tree, root, true,
        "make_immutable_array test deferred (requires "
        "variadic + alias templates)");
#endif

    return tree;
}


array_test_tree
test_options_make_array_from_options_extent_deduction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "make_array_from_options<>(...) deduces extent "
        "from the argument count when none supplied");
    auto* root = tree.underlying().root();

    auto a = djinterp::make_array_from_options<
        djinterp::array_opt_type<int>>(1, 2, 3);
    using a_t = decltype(a);

    append_leaf(tree, root,
        std::is_same<typename a_t::value_type, int>::value,
        "value_type == int");

    append_leaf(tree, root,
        ( a_t::extent == 3u ),
        "extent == sizeof...(_Args) when no extent in pack");

    append_leaf(tree, root,
        ( a.size() == 3u ),
        "instance reports size 3");

    append_leaf(tree, root,
        ( ( a.data()[0] == 1 ) &&
          ( a.data()[1] == 2 ) &&
          ( a.data()[2] == 3 ) ),
        "argument values preserved");

    return tree;
}


array_test_tree
test_options_make_array_from_options_explicit_extent(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "make_array_from_options<>(...) honors a "
        "user-supplied extent");
    auto* root = tree.underlying().root();

    // The user pack already contains an extent option.  Per
    // first-match-wins, the user's extent must win even though
    // the factory injects an extent equal to sizeof...(_Args).
    auto b = djinterp::make_array_from_options<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>();
    using b_t = decltype(b);

    append_leaf(tree, root,
        ( b_t::extent == 8u ),
        "user-supplied array_opt_extent<8> wins over the "
        "factory's deduced extent");

    return tree;
}


// =========================================================================
// III. CATEGORY: OPTION LIST MANIPULATION
// =========================================================================

array_test_tree
test_options_option_list_prepend(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "option_list_prepend_t puts the new option at the "
        "front of the list");
    auto* root = tree.underlying().root();

    using base_list = djinterp::option_list<
        djinterp::option<djinterp::array_extent_key,
                         std::integral_constant<std::size_t, 4>>>;
    using prepended = djinterp::option_list_prepend_t<
        djinterp::option<djinterp::array_type_key, int>,
        base_list>;

    using expected = djinterp::option_list<
        djinterp::option<djinterp::array_type_key, int>,
        djinterp::option<djinterp::array_extent_key,
                         std::integral_constant<std::size_t, 4>>>;

    append_leaf(tree, root,
        std::is_same<prepended, expected>::value,
        "prepend places new option at index 0");

    append_leaf(tree, root,
        ( prepended::size == 2u ),
        "prepended list has size 2");

    return tree;
}


array_test_tree
test_options_option_list_append(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "option_list_append_t puts the new option at the "
        "end of the list");
    auto* root = tree.underlying().root();

    using base_list = djinterp::option_list<
        djinterp::option<djinterp::array_type_key, int>>;
    using appended = djinterp::option_list_append_t<
        base_list,
        djinterp::option<djinterp::array_extent_key,
                         std::integral_constant<std::size_t, 4>>>;

    using expected = djinterp::option_list<
        djinterp::option<djinterp::array_type_key, int>,
        djinterp::option<djinterp::array_extent_key,
                         std::integral_constant<std::size_t, 4>>>;

    append_leaf(tree, root,
        std::is_same<appended, expected>::value,
        "append places new option at the end");

    append_leaf(tree, root,
        ( appended::size == 2u ),
        "appended list has size 2");

    return tree;
}


array_test_tree
test_options_option_list_concat(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "option_list_concat_t merges two lists in order");
    auto* root = tree.underlying().root();

    using a = djinterp::option_list<
        djinterp::option<djinterp::array_type_key, int>,
        djinterp::option<djinterp::array_extent_key,
                         std::integral_constant<std::size_t, 4>>>;

    using b = djinterp::option_list<
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>>;

    using merged = djinterp::option_list_concat_t<a, b>;

    append_leaf(tree, root,
        ( merged::size == 3u ),
        "concat of size-2 + size-1 has size 3");

    using expected = djinterp::option_list<
        djinterp::option<djinterp::array_type_key, int>,
        djinterp::option<djinterp::array_extent_key,
                         std::integral_constant<std::size_t, 4>>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>>;

    append_leaf(tree, root,
        std::is_same<merged, expected>::value,
        "concat preserves left-then-right order");

    return tree;
}


array_test_tree
test_options_option_list_concat_empty(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "option_list_concat_t with an empty operand is "
        "identity");
    auto* root = tree.underlying().root();

    using non_empty = djinterp::option_list<
        djinterp::option<djinterp::array_type_key, int>>;
    using empty     = djinterp::option_list<>;

    using left_concat  = djinterp::option_list_concat_t<empty,
                                                         non_empty>;
    using right_concat = djinterp::option_list_concat_t<non_empty,
                                                         empty>;

    append_leaf(tree, root,
        std::is_same<left_concat, non_empty>::value,
        "empty + non_empty == non_empty");

    append_leaf(tree, root,
        std::is_same<right_concat, non_empty>::value,
        "non_empty + empty == non_empty");

    using both_empty = djinterp::option_list_concat_t<empty, empty>;
    append_leaf(tree, root,
        ( both_empty::size == 0u ),
        "empty + empty has size 0");

    return tree;
}


NS_END  // testing
NS_END  // djinterp
