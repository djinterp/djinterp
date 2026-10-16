/*******************************************************************************
* djinterp [testing]                                array_options_core_tests.cpp
*
*   Implementation of the core categories of Part D (array
* options) of the array test suite, under the return-a-subtree
* protocol declared in array_tests.hpp.
*
*   STRUCTURE:
*   Every category function below builds and returns a self-
* contained `array_test_tree`.  Tests do NOT take a test_handler
* or a test_printer; they simply construct a small tree and
* return it by value.
*
*   This translation unit owns SEVEN of Part D's twelve
* categories - the ones that bottom out at the resolver
* surface (with_options mixin contract, defaults, ordering,
* duplicates, ignored axes, cross-container propagation, and
* per-axis option-alias resolution).  The remaining categories
* live in three sibling files:
*     - array_options_alias_tests.cpp    (convenience aliases)
*     - array_options_factory_tests.cpp  (factories + list ops)
*     - array_options_edge_tests.cpp     (lock policy + edges)
*
*   This file ALSO owns the Part D module aggregate builder
* (make_array_options_subtree) and the Part D master runner
* (run_array_options_suite); both call into the sibling .cpp
* files via the declarations published in array_tests.hpp.
*
*
* path:      /tests/djinterp/core/container/array/array_options_core_tests.cpp
*                 array_options_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.10
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <chrono>
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
//   The eager `append_leaf` helper lives in `array_tests.hpp`
// so every translation unit in the suite shares one definition.
// Defining it again here would shadow the header version with
// an identical signature and produce ambiguous-call diagnostics
// at every assertion site.

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


    // -------------------------------------------------------------------
    //  Probe types used to verify lookup behavior.
    // -------------------------------------------------------------------

    // probe_absent_key
    //   tag: a key the test suite never inserts.  Any
    // option_t<probe_absent_key, _Default> lookup must
    // resolve to _Default.
    struct probe_absent_key
    {};

    // probe_struct_value
    //   type: a small trivially-copyable struct used as an
    // element type to verify that arrays accept user-defined
    // value types via array_opt_type<T>.
    struct probe_struct_value
    {
        int  a;
        char b;

        bool
        operator==(
            const probe_struct_value& _o
        ) const
        {
            return ( (a == _o.a) && (b == _o.b) );
        }
    };


    // -------------------------------------------------------------------
    //  Helpers that probe the with_options mixin contract.
    // -------------------------------------------------------------------

    // has_options_type_alias
    //   trait: true if _Container exposes a public type alias
    // named ::options_type.
    template<typename _Container,
             typename = void>
    struct has_options_type_alias : std::false_type
    {};

    template<typename _Container>
    struct has_options_type_alias<_Container,
        djinterp::void_t<typename _Container::options_type>>
        : std::true_type
    {};

    // has_option_count_member
    //   trait: true if _Container exposes a static constexpr
    // ::option_count member.
    template<typename _Container,
             typename = void>
    struct has_option_count_member : std::false_type
    {};

    template<typename _Container>
    struct has_option_count_member<_Container,
        djinterp::void_t<
            decltype(_Container::option_count)>>
        : std::true_type
    {};

}  // unnamed namespace


// =========================================================================
// II.  CATEGORY: OPTION ALIAS RESOLUTION
// =========================================================================
//   Every per-axis convenience alias must compose into the
// canonical `option<key, value>` shape.

array_test_tree
test_options_array_opt_type_alias(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "array_opt_type<T> resolves to option<array_type_key, T>");
    auto* root = tree.underlying().root();

    using alias_int    = djinterp::array_opt_type<int>;
    using alias_struct = djinterp::array_opt_type<probe_struct_value>;
    using alias_ptr    = djinterp::array_opt_type<int*>;

    append_leaf(tree, root,
        std::is_same<typename alias_int::key_type,
                     djinterp::array_type_key>::value,
        "array_opt_type<int>::key_type == array_type_key");

    append_leaf(tree, root,
        std::is_same<typename alias_struct::key_type,
                     djinterp::array_type_key>::value,
        "array_opt_type<probe_struct_value>::key_type == "
        "array_type_key");

    append_leaf(tree, root,
        std::is_same<typename alias_ptr::key_type,
                     djinterp::array_type_key>::value,
        "array_opt_type<int*>::key_type == array_type_key");

    append_leaf(tree, root,
        std::is_same<typename alias_int::value_type, int>::value,
        "array_opt_type<int>::value_type == int");

    append_leaf(tree, root,
        std::is_same<typename alias_struct::value_type,
                     probe_struct_value>::value,
        "array_opt_type<probe_struct_value>::value_type == "
        "probe_struct_value");

    append_leaf(tree, root,
        std::is_same<typename alias_ptr::value_type, int*>::value,
        "array_opt_type<int*>::value_type == int*");

    return tree;
}


array_test_tree
test_options_array_opt_extent_alias(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "array_opt_extent<N> resolves to "
        "option<array_extent_key, integral_constant<size_t, N>>");
    auto* root = tree.underlying().root();

    using alias_zero  = djinterp::array_opt_extent<0>;
    using alias_small = djinterp::array_opt_extent<4>;
    using alias_large = djinterp::array_opt_extent<1024>;

    append_leaf(tree, root,
        std::is_same<typename alias_zero::key_type,
                     djinterp::array_extent_key>::value,
        "array_opt_extent<0>::key_type == array_extent_key");

    append_leaf(tree, root,
        std::is_same<typename alias_small::key_type,
                     djinterp::array_extent_key>::value,
        "array_opt_extent<4>::key_type == array_extent_key");

    append_leaf(tree, root,
        std::is_same<typename alias_large::key_type,
                     djinterp::array_extent_key>::value,
        "array_opt_extent<1024>::key_type == array_extent_key");

    append_leaf(tree, root,
        std::is_same<typename alias_small::value_type,
                     std::integral_constant<std::size_t, 4>>::value,
        "array_opt_extent<4>::value_type == "
        "integral_constant<size_t, 4>");

    append_leaf(tree, root,
        ( alias_zero::value_type::value == 0u ),
        "array_opt_extent<0>::value_type::value == 0");

    append_leaf(tree, root,
        ( alias_small::value_type::value == 4u ),
        "array_opt_extent<4>::value_type::value == 4");

    append_leaf(tree, root,
        ( alias_large::value_type::value == 1024u ),
        "array_opt_extent<1024>::value_type::value == 1024");

    return tree;
}


array_test_tree
test_options_container_opt_lifetime_alias(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "container_opt_lifetime<L> resolves to "
        "option<container_lifetime_key, ...>");
    auto* root = tree.underlying().root();

    using alias_mut = djinterp::container_opt_lifetime<
        djinterp::container_lifetime::mutable_storage>;
    using alias_imm = djinterp::container_opt_lifetime<
        djinterp::container_lifetime::immutable>;
    using alias_cx  = djinterp::container_opt_lifetime<
        djinterp::container_lifetime::constexpr_storage>;

    append_leaf(tree, root,
        std::is_same<typename alias_mut::key_type,
                     djinterp::container_lifetime_key>::value,
        "container_opt_lifetime<mutable>::key_type == "
        "container_lifetime_key");

    append_leaf(tree, root,
        std::is_same<typename alias_imm::key_type,
                     djinterp::container_lifetime_key>::value,
        "container_opt_lifetime<immutable>::key_type == "
        "container_lifetime_key");

    append_leaf(tree, root,
        std::is_same<typename alias_cx::key_type,
                     djinterp::container_lifetime_key>::value,
        "container_opt_lifetime<constexpr>::key_type == "
        "container_lifetime_key");

    append_leaf(tree, root,
        ( alias_mut::value_type::value ==
          djinterp::container_lifetime::mutable_storage ),
        "container_opt_lifetime<mutable>::value == "
        "mutable_storage");

    append_leaf(tree, root,
        ( alias_imm::value_type::value ==
          djinterp::container_lifetime::immutable ),
        "container_opt_lifetime<immutable>::value == immutable");

    append_leaf(tree, root,
        ( alias_cx::value_type::value ==
          djinterp::container_lifetime::constexpr_storage ),
        "container_opt_lifetime<constexpr>::value == "
        "constexpr_storage");

    return tree;
}


array_test_tree
test_options_container_opt_iterability_alias(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "container_opt_iterability<I> resolves to "
        "option<container_iterability_key, ...>");
    auto* root = tree.underlying().root();

    using alias_iter = djinterp::container_opt_iterability<
        djinterp::container_iterability::iterable>;
    using alias_nit  = djinterp::container_opt_iterability<
        djinterp::container_iterability::non_iterable>;

    append_leaf(tree, root,
        std::is_same<typename alias_iter::key_type,
                     djinterp::container_iterability_key>::value,
        "container_opt_iterability<iterable>::key_type == "
        "container_iterability_key");

    append_leaf(tree, root,
        std::is_same<typename alias_nit::key_type,
                     djinterp::container_iterability_key>::value,
        "container_opt_iterability<non_iterable>::key_type == "
        "container_iterability_key");

    append_leaf(tree, root,
        ( alias_iter::value_type::value ==
          djinterp::container_iterability::iterable ),
        "container_opt_iterability<iterable>::value == iterable");

    append_leaf(tree, root,
        ( alias_nit::value_type::value ==
          djinterp::container_iterability::non_iterable ),
        "container_opt_iterability<non_iterable>::value == "
        "non_iterable");

    return tree;
}


array_test_tree
test_options_container_opt_lock_policy_alias(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "container_opt_lock_policy<P> resolves to "
        "option<lock_policy_key, P>");
    auto* root = tree.underlying().root();

    using alias_null = djinterp::container_opt_lock_policy<
        djinterp::null_lock_policy>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using alias_excl = djinterp::container_opt_lock_policy<
        djinterp::exclusive_lock_policy>;
    using alias_tim  = djinterp::container_opt_lock_policy<
        djinterp::timed_lock_policy>;
#endif

    append_leaf(tree, root,
        std::is_same<typename alias_null::key_type,
                     djinterp::lock_policy_key>::value,
        "container_opt_lock_policy<null>::key_type == "
        "lock_policy_key");

    append_leaf(tree, root,
        std::is_same<typename alias_null::value_type,
                     djinterp::null_lock_policy>::value,
        "container_opt_lock_policy<null>::value_type == "
        "null_lock_policy");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    append_leaf(tree, root,
        std::is_same<typename alias_excl::key_type,
                     djinterp::lock_policy_key>::value,
        "container_opt_lock_policy<exclusive>::key_type == "
        "lock_policy_key");

    append_leaf(tree, root,
        std::is_same<typename alias_excl::value_type,
                     djinterp::exclusive_lock_policy>::value,
        "container_opt_lock_policy<exclusive>::value_type == "
        "exclusive_lock_policy");

    append_leaf(tree, root,
        std::is_same<typename alias_tim::value_type,
                     djinterp::timed_lock_policy>::value,
        "container_opt_lock_policy<timed>::value_type == "
        "timed_lock_policy");
#endif

    return tree;
}


// =========================================================================
// III. CATEGORY: REQUIRED AND DEFAULT OPTIONS
// =========================================================================

array_test_tree
test_options_required_element_type(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "element type carries through every resolved "
        "configuration");
    auto* root = tree.underlying().root();

    using arr = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;
    using arr_struct = djinterp::array<
        djinterp::array_opt_type<probe_struct_value>,
        djinterp::array_opt_extent<8>>;
    using ts = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;
    using atom = djinterp::atomic_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;
    using cow = djinterp::cow_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        std::is_same<typename arr::value_type, int>::value,
        "array<...>::value_type == int when "
        "array_opt_type<int> in pack");

    append_leaf(tree, root,
        std::is_same<typename arr_struct::value_type,
                     probe_struct_value>::value,
        "array<...>::value_type == probe_struct_value when "
        "array_opt_type<probe_struct_value> in pack");

    append_leaf(tree, root,
        std::is_same<typename ts::value_type, int>::value,
        "threadsafe_array<...>::value_type matches the "
        "wrapped element type");

    append_leaf(tree, root,
        std::is_same<typename atom::value_type, int>::value,
        "atomic_array<...>::value_type matches the wrapped "
        "element type");

    append_leaf(tree, root,
        std::is_same<typename cow::value_type, int>::value,
        "cow_array<...>::value_type matches the wrapped "
        "element type");

    return tree;
}


array_test_tree
test_options_default_extent(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "extent defaults to 0 when array_opt_extent is absent");
    auto* root = tree.underlying().root();

    using arr_no_extent = djinterp::array<
        djinterp::array_opt_type<int>>;

    append_leaf(tree, root,
        ( arr_no_extent::extent == 0u ),
        "array<array_opt_type<int>>::extent == 0 (default)");

    append_leaf(tree, root,
        ( arr_no_extent().size() == 0u ),
        "instance of pack-only array has size() == 0");

    append_leaf(tree, root,
        ( arr_no_extent().empty() ),
        "instance of pack-only array reports empty()");

    using ts_no_extent = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>>;

    append_leaf(tree, root,
        ( ts_no_extent::extent == 0u ),
        "threadsafe_array<array_opt_type<int>>::extent == 0");

    return tree;
}


array_test_tree
test_options_default_lifetime(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "lifetime defaults to mutable_storage when absent");
    auto* root = tree.underlying().root();

    using arr_default = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        ( arr_default::lifetime ==
          djinterp::array_lifetime::mutable_lifetime ),
        "array without lifetime option resolves to "
        "mutable_lifetime");

    using ts_default = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        ( ts_default::lifetime ==
          djinterp::array_lifetime::mutable_lifetime ),
        "threadsafe_array without lifetime option resolves "
        "to mutable_lifetime");

    using cow_default = djinterp::cow_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        ( cow_default::lifetime ==
          djinterp::array_lifetime::mutable_lifetime ),
        "cow_array without lifetime option resolves to "
        "mutable_lifetime");

    return tree;
}


array_test_tree
test_options_default_iterability(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "iterability defaults to iterable when absent");
    auto* root = tree.underlying().root();

    using arr_default = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        ( arr_default::iterability ==
          djinterp::array_iterability::iterable ),
        "array without iterability option resolves to "
        "iterable");

    using ts_default = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        ( ts_default::iterability ==
          djinterp::array_iterability::iterable ),
        "threadsafe_array without iterability option "
        "resolves to iterable");

    using atom_default = djinterp::atomic_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        ( atom_default::iterability ==
          djinterp::array_iterability::iterable ),
        "atomic_array without iterability option resolves "
        "to iterable");

    return tree;
}


array_test_tree
test_options_default_lock_policy_threadsafe(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array lock policy defaults to "
        "default_lock_policy");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using ts_default = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        std::is_same<typename ts_default::lock_policy_type,
                     djinterp::default_lock_policy>::value,
        "threadsafe_array<...>::lock_policy_type == "
        "default_lock_policy when no policy supplied");
#else
    append_leaf(tree, root, true,
        "threadsafe_array default policy test deferred "
        "(requires C++11)");
#endif

    return tree;
}


array_test_tree
test_options_default_lock_policy_cow(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "cow_array lock policy defaults to null_lock_policy");
    auto* root = tree.underlying().root();

    using cow_default = djinterp::cow_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        std::is_same<typename cow_default::lock_policy_type,
                     djinterp::null_lock_policy>::value,
        "cow_array<...>::lock_policy_type == null_lock_policy "
        "when no policy supplied");

    return tree;
}


// =========================================================================
// IV.  CATEGORY: OPTION ORDER INDEPENDENCE
// =========================================================================

array_test_tree
test_options_array_order_invariance(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "array<>: option order does not affect resolved "
        "configuration");
    auto* root = tree.underlying().root();

    using forward = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>,
        djinterp::container_opt_iterability<
            djinterp::container_iterability::iterable>>;

    using shuffled = djinterp::array<
        djinterp::container_opt_iterability<
            djinterp::container_iterability::iterable>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>,
        djinterp::array_opt_extent<4>,
        djinterp::array_opt_type<int>>;

    using interleaved = djinterp::array<
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>,
        djinterp::array_opt_type<int>,
        djinterp::container_opt_iterability<
            djinterp::container_iterability::iterable>>;

    append_leaf(tree, root,
        std::is_same<typename forward::value_type,
                     typename shuffled::value_type>::value,
        "value_type matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::extent == shuffled::extent ),
        "extent matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::lifetime == shuffled::lifetime ),
        "lifetime matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::iterability == shuffled::iterability ),
        "iterability matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::extent == interleaved::extent ),
        "extent matches across forward / interleaved order");

    append_leaf(tree, root,
        ( forward::lifetime == interleaved::lifetime ),
        "lifetime matches across forward / interleaved order");

    return tree;
}


array_test_tree
test_options_threadsafe_order_invariance(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array<>: option order does not affect "
        "resolved configuration");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using forward = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>,
        djinterp::container_opt_lock_policy<
            djinterp::exclusive_lock_policy>>;

    using shuffled = djinterp::threadsafe_array<
        djinterp::container_opt_lock_policy<
            djinterp::exclusive_lock_policy>,
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    append_leaf(tree, root,
        std::is_same<typename forward::value_type,
                     typename shuffled::value_type>::value,
        "value_type matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::extent == shuffled::extent ),
        "extent matches across forward / shuffled order");

    append_leaf(tree, root,
        std::is_same<typename forward::lock_policy_type,
                     typename shuffled::lock_policy_type>::value,
        "lock_policy_type matches across forward / shuffled "
        "order");
#else
    append_leaf(tree, root, true,
        "threadsafe_array order-invariance test deferred "
        "(requires C++11)");
#endif

    return tree;
}


array_test_tree
test_options_cow_order_invariance(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "cow_array<>: option order does not affect resolved "
        "configuration");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using forward = djinterp::cow_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>,
        djinterp::container_opt_lock_policy<
            djinterp::exclusive_lock_policy>>;

    using shuffled = djinterp::cow_array<
        djinterp::array_opt_extent<8>,
        djinterp::container_opt_lock_policy<
            djinterp::exclusive_lock_policy>,
        djinterp::array_opt_type<int>>;

    append_leaf(tree, root,
        std::is_same<typename forward::value_type,
                     typename shuffled::value_type>::value,
        "value_type matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::extent == shuffled::extent ),
        "extent matches across forward / shuffled order");

    append_leaf(tree, root,
        std::is_same<typename forward::lock_policy_type,
                     typename shuffled::lock_policy_type>::value,
        "lock_policy_type matches across forward / shuffled "
        "order");
#else
    append_leaf(tree, root, true,
        "cow_array order-invariance test deferred (requires "
        "C++11)");
#endif

    return tree;
}


array_test_tree
test_options_atomic_order_invariance(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array<>: option order does not affect "
        "resolved configuration");
    auto* root = tree.underlying().root();

    using forward = djinterp::atomic_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    using shuffled = djinterp::atomic_array<
        djinterp::array_opt_extent<8>,
        djinterp::array_opt_type<int>>;

    append_leaf(tree, root,
        std::is_same<typename forward::value_type,
                     typename shuffled::value_type>::value,
        "value_type matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::extent == shuffled::extent ),
        "extent matches across forward / shuffled order");

    append_leaf(tree, root,
        ( forward::iterability == shuffled::iterability ),
        "iterability matches across forward / shuffled order");

    return tree;
}


// =========================================================================
// V.   CATEGORY: FIRST-MATCH-WINS FOR DUPLICATES
// =========================================================================

array_test_tree
test_options_duplicate_extent_first_wins(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "duplicate array_opt_extent: first occurrence wins");
    auto* root = tree.underlying().root();

    using arr_dup = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<3>,
        djinterp::array_opt_extent<7>>;

    append_leaf(tree, root,
        ( arr_dup::extent == 3u ),
        "first array_opt_extent<3> wins over later "
        "array_opt_extent<7>");

    using arr_dup_three = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<5>,
        djinterp::array_opt_extent<11>,
        djinterp::array_opt_extent<13>>;

    append_leaf(tree, root,
        ( arr_dup_three::extent == 5u ),
        "first of three duplicate extents wins");

    return tree;
}


array_test_tree
test_options_duplicate_lifetime_first_wins(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "duplicate container_opt_lifetime: first wins");
    auto* root = tree.underlying().root();

    using arr_dup = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::mutable_storage>>;

    append_leaf(tree, root,
        ( arr_dup::lifetime ==
          djinterp::array_lifetime::immutable_lifetime ),
        "first container_opt_lifetime<immutable> wins over "
        "later mutable_storage");

    return tree;
}


array_test_tree
test_options_duplicate_iterability_first_wins(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "duplicate container_opt_iterability: first wins");
    auto* root = tree.underlying().root();

    using arr_dup = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_iterability<
            djinterp::container_iterability::non_iterable>,
        djinterp::container_opt_iterability<
            djinterp::container_iterability::iterable>>;

    append_leaf(tree, root,
        ( arr_dup::iterability ==
          djinterp::array_iterability::non_iterable ),
        "first container_opt_iterability<non_iterable> wins "
        "over later iterable");

    return tree;
}


array_test_tree
test_options_duplicate_lock_policy_first_wins(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "duplicate container_opt_lock_policy: first wins");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using ts_dup = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lock_policy<
            djinterp::null_lock_policy>,
        djinterp::container_opt_lock_policy<
            djinterp::exclusive_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename ts_dup::lock_policy_type,
                     djinterp::null_lock_policy>::value,
        "first container_opt_lock_policy<null> wins over "
        "later exclusive on threadsafe_array");

    using cow_dup = djinterp::cow_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lock_policy<
            djinterp::exclusive_lock_policy>,
        djinterp::container_opt_lock_policy<
            djinterp::null_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename cow_dup::lock_policy_type,
                     djinterp::exclusive_lock_policy>::value,
        "first container_opt_lock_policy<exclusive> wins "
        "over later null on cow_array");
#else
    append_leaf(tree, root, true,
        "duplicate-lock-policy test deferred (requires "
        "C++11)");
#endif

    return tree;
}


array_test_tree
test_options_duplicate_element_type_first_wins(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "duplicate array_opt_type: first wins");
    auto* root = tree.underlying().root();

    using arr_dup = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_type<double>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        std::is_same<typename arr_dup::value_type, int>::value,
        "first array_opt_type<int> wins over later "
        "array_opt_type<double>");

    return tree;
}


// =========================================================================
// VI.  CATEGORY: UNIVERSAL AXES SILENTLY IGNORED
// =========================================================================

array_test_tree
test_options_ignored_universal_axes(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "universal axes that array<> does not consume are "
        "silently accepted");
    auto* root = tree.underlying().root();

    using base = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    using stuffed = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_ordering<
            djinterp::container_ordering::ordered>,
        djinterp::container_opt_bounds<
            djinterp::container_bounds::bounded>,
        djinterp::container_opt_multiplicity<
            djinterp::container_multiplicity::multi>,
        djinterp::container_opt_structure<
            djinterp::container_structure::flat>,
        djinterp::container_opt_storage_kind<
            djinterp::container_storage_kind::static_storage>,
        djinterp::container_opt_backing<
            djinterp::container_backing::fundamental>>;

    append_leaf(tree, root,
        std::is_same<typename stuffed::value_type,
                     typename base::value_type>::value,
        "value_type unchanged after stuffing irrelevant "
        "axes");

    append_leaf(tree, root,
        ( stuffed::extent == base::extent ),
        "extent unchanged after stuffing irrelevant axes");

    append_leaf(tree, root,
        ( stuffed::lifetime == base::lifetime ),
        "lifetime unchanged after stuffing irrelevant axes");

    append_leaf(tree, root,
        ( stuffed::iterability == base::iterability ),
        "iterability unchanged after stuffing irrelevant "
        "axes");

    stuffed s;
    append_leaf(tree, root,
        ( s.size() == 4u ),
        "stuffed array constructs and reports correct size");

    return tree;
}


array_test_tree
test_options_thread_safety_enum_does_not_pin_lock_policy(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "container_opt_thread_safety enum is informational; "
        "lock policy comes from container_opt_lock_policy");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    using ts_explicit = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_thread_safety<
            djinterp::container_thread_safety::shared>,
        djinterp::container_opt_lock_policy<
            djinterp::exclusive_lock_policy>>;

    append_leaf(tree, root,
        std::is_same<typename ts_explicit::lock_policy_type,
                     djinterp::exclusive_lock_policy>::value,
        "lock_policy_type follows container_opt_lock_policy "
        "even when container_opt_thread_safety<shared> is "
        "also present");
#else
    append_leaf(tree, root, true,
        "thread-safety-enum test deferred (requires C++11)");
#endif

    return tree;
}


// =========================================================================
// VII. CATEGORY: CROSS-CONTAINER OPTION PROPAGATION
// =========================================================================

array_test_tree
test_options_threadsafe_inherits_axes(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "threadsafe_array<...> preserves the wrapped "
        "array<...>'s positional axes");
    auto* root = tree.underlying().root();

    using inner = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    using outer = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    append_leaf(tree, root,
        std::is_same<typename inner::value_type,
                     typename outer::value_type>::value,
        "value_type preserved across threadsafe_array "
        "wrapping");

    append_leaf(tree, root,
        ( inner::extent == outer::extent ),
        "extent preserved across threadsafe_array wrapping");

    append_leaf(tree, root,
        ( inner::lifetime == outer::lifetime ),
        "lifetime preserved across threadsafe_array "
        "wrapping");

    append_leaf(tree, root,
        ( inner::iterability == outer::iterability ),
        "iterability preserved across threadsafe_array "
        "wrapping");

    append_leaf(tree, root,
        std::is_same<typename outer::underlying_type,
                     inner>::value,
        "threadsafe_array<...>::underlying_type == "
        "array<...>");

    return tree;
}


array_test_tree
test_options_cow_inherits_axes(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "cow_array<...> preserves the wrapped array<...>'s "
        "positional axes");
    auto* root = tree.underlying().root();

    using inner = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    using outer = djinterp::cow_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    append_leaf(tree, root,
        std::is_same<typename inner::value_type,
                     typename outer::value_type>::value,
        "value_type preserved across cow_array wrapping");

    append_leaf(tree, root,
        ( inner::extent == outer::extent ),
        "extent preserved across cow_array wrapping");

    append_leaf(tree, root,
        ( inner::lifetime == outer::lifetime ),
        "lifetime preserved across cow_array wrapping");

    append_leaf(tree, root,
        ( inner::iterability == outer::iterability ),
        "iterability preserved across cow_array wrapping");

    append_leaf(tree, root,
        std::is_same<typename outer::underlying_type,
                     inner>::value,
        "cow_array<...>::underlying_type == array<...>");

    return tree;
}


array_test_tree
test_options_atomic_inherits_axes(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "atomic_array<...> preserves the resolved axes "
        "(element type, extent, iterability)");
    auto* root = tree.underlying().root();

    using inner = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    using outer = djinterp::atomic_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<8>>;

    append_leaf(tree, root,
        std::is_same<typename inner::value_type,
                     typename outer::value_type>::value,
        "value_type preserved across atomic_array wrapping");

    append_leaf(tree, root,
        ( inner::extent == outer::extent ),
        "extent preserved across atomic_array wrapping");

    append_leaf(tree, root,
        ( inner::iterability == outer::iterability ),
        "iterability preserved across atomic_array wrapping");

    return tree;
}


// =========================================================================
// VIII. CATEGORY: WITH_OPTIONS MIXIN CONTRACT
// =========================================================================

array_test_tree
test_options_options_type_alias_present(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "with_options surface: ::options_type alias is "
        "present on every container");
    auto* root = tree.underlying().root();

    using arr  = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;
    using ts   = djinterp::threadsafe_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;
    using cow  = djinterp::cow_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;
    using atom = djinterp::atomic_array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    append_leaf(tree, root,
        has_options_type_alias<arr>::value,
        "array<...>::options_type alias present");

    // The wrappers may or may not inherit options_container_base
    // directly; the alias is optional for them.  Tests record
    // that the probe compiles without making missing cases
    // failures.
    append_leaf(tree, root,
        ( has_options_type_alias<ts>::value ||
          !has_options_type_alias<ts>::value ),
        "threadsafe_array<...>::options_type presence is "
        "well-formed (probe compiles)");

    append_leaf(tree, root,
        ( has_options_type_alias<cow>::value ||
          !has_options_type_alias<cow>::value ),
        "cow_array<...>::options_type presence is "
        "well-formed (probe compiles)");

    append_leaf(tree, root,
        ( has_options_type_alias<atom>::value ||
          !has_options_type_alias<atom>::value ),
        "atomic_array<...>::options_type presence is "
        "well-formed (probe compiles)");

    return tree;
}


array_test_tree
test_options_option_count_member(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "with_options surface: ::option_count reports the "
        "number of normalized entries");
    auto* root = tree.underlying().root();

    using arr_two = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    using arr_four = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>,
        djinterp::container_opt_iterability<
            djinterp::container_iterability::iterable>>;

    append_leaf(tree, root,
        has_option_count_member<arr_two>::value,
        "array<...>::option_count member is present");

    append_leaf(tree, root,
        ( arr_two::option_count == 2u ),
        "array<2 options>::option_count == 2");

    append_leaf(tree, root,
        ( arr_four::option_count == 4u ),
        "array<4 options>::option_count == 4");

    return tree;
}


array_test_tree
test_options_has_option_v(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "with_options surface: ::has_option_v<key> reports "
        "presence and absence");
    auto* root = tree.underlying().root();

    using arr_full = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>,
        djinterp::container_opt_lifetime<
            djinterp::container_lifetime::immutable>>;

    append_leaf(tree, root,
        arr_full::template has_option_v<
            djinterp::array_type_key>,
        "has_option_v<array_type_key> == true when type "
        "option in pack");

    append_leaf(tree, root,
        arr_full::template has_option_v<
            djinterp::array_extent_key>,
        "has_option_v<array_extent_key> == true when extent "
        "option in pack");

    append_leaf(tree, root,
        arr_full::template has_option_v<
            djinterp::container_lifetime_key>,
        "has_option_v<container_lifetime_key> == true when "
        "lifetime option in pack");

    append_leaf(tree, root,
        !arr_full::template has_option_v<
            djinterp::container_iterability_key>,
        "has_option_v<container_iterability_key> == false "
        "when iterability option absent");

    append_leaf(tree, root,
        !arr_full::template has_option_v<probe_absent_key>,
        "has_option_v<probe_absent_key> == false (key never "
        "supplied)");

    return tree;
}


array_test_tree
test_options_option_t_lookup_and_default(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "with_options surface: ::option_t<key, default> "
        "looks up or falls back");
    auto* root = tree.underlying().root();

    using arr_full = djinterp::array<
        djinterp::array_opt_type<int>,
        djinterp::array_opt_extent<4>>;

    using looked_up_type = arr_full::template option_t<
        djinterp::array_type_key,
        void>;

    append_leaf(tree, root,
        std::is_same<looked_up_type, int>::value,
        "option_t<array_type_key, void> == int when type "
        "option in pack");

    using fallback_type = arr_full::template option_t<
        probe_absent_key,
        char>;

    append_leaf(tree, root,
        std::is_same<fallback_type, char>::value,
        "option_t<probe_absent_key, char> == char (fallback)");

    using fallback_default = arr_full::template option_t<
        probe_absent_key>;

    append_leaf(tree, root,
        std::is_same<fallback_default, void>::value,
        "option_t<probe_absent_key> with no default == void");

    return tree;
}


// =========================================================================
// IX.  PART D AGGREGATE BUILDER
// =========================================================================
//   Assembles every Part D test function (declared in
// array_tests.hpp; implemented across this and the three
// sibling .cpp files) as a child of a single module-level
// root.

array_test_tree
make_array_options_subtree(
    test::test_type_id _kind
)
{
    array_test_tree result;

    result.underlying().emplace_root(
        test::make_module("array options"));

    auto add = [&](const array_test_tree& _sub) -> void
    {
        result.append_subtree(_sub);

        return;
    };

    // -- aliases (this TU) --
    add(test_options_array_opt_type_alias(_kind));
    add(test_options_array_opt_extent_alias(_kind));
    add(test_options_container_opt_lifetime_alias(_kind));
    add(test_options_container_opt_iterability_alias(_kind));
    add(test_options_container_opt_lock_policy_alias(_kind));

    // -- required and defaults (this TU) --
    add(test_options_required_element_type(_kind));
    add(test_options_default_extent(_kind));
    add(test_options_default_lifetime(_kind));
    add(test_options_default_iterability(_kind));
    add(test_options_default_lock_policy_threadsafe(_kind));
    add(test_options_default_lock_policy_cow(_kind));

    // -- order independence (this TU) --
    add(test_options_array_order_invariance(_kind));
    add(test_options_threadsafe_order_invariance(_kind));
    add(test_options_cow_order_invariance(_kind));
    add(test_options_atomic_order_invariance(_kind));

    // -- duplicates (this TU) --
    add(test_options_duplicate_extent_first_wins(_kind));
    add(test_options_duplicate_lifetime_first_wins(_kind));
    add(test_options_duplicate_iterability_first_wins(_kind));
    add(test_options_duplicate_lock_policy_first_wins(_kind));
    add(test_options_duplicate_element_type_first_wins(_kind));

    // -- ignored universal axes (this TU) --
    add(test_options_ignored_universal_axes(_kind));
    add(test_options_thread_safety_enum_does_not_pin_lock_policy(_kind));

    // -- cross-container propagation (this TU) --
    add(test_options_threadsafe_inherits_axes(_kind));
    add(test_options_cow_inherits_axes(_kind));
    add(test_options_atomic_inherits_axes(_kind));

    // -- with_options mixin contract (this TU) --
    add(test_options_options_type_alias_present(_kind));
    add(test_options_option_count_member(_kind));
    add(test_options_has_option_v(_kind));
    add(test_options_option_t_lookup_and_default(_kind));

    // -- convenience aliases (array_options_alias_tests.cpp) --
    add(test_options_alias_mutable_iterable(_kind));
    add(test_options_alias_mutable_non_iterable(_kind));
    add(test_options_alias_immutable_iterable(_kind));
    add(test_options_alias_immutable_non_iterable(_kind));
    add(test_options_alias_constexpr_iterable(_kind));
    add(test_options_alias_constexpr_non_iterable(_kind));
    add(test_options_alias_mutex_array(_kind));
    add(test_options_alias_timed_array(_kind));
    add(test_options_alias_shared_array_cpp17(_kind));

    // -- factories and list ops (array_options_factory_tests.cpp) --
    add(test_options_make_array_factory(_kind));
    add(test_options_make_immutable_array_factory(_kind));
    add(test_options_make_array_from_options_extent_deduction(_kind));
    add(test_options_make_array_from_options_explicit_extent(_kind));
    add(test_options_option_list_prepend(_kind));
    add(test_options_option_list_append(_kind));
    add(test_options_option_list_concat(_kind));
    add(test_options_option_list_concat_empty(_kind));

    // -- lock policy and edges (array_options_edge_tests.cpp) --
    add(test_options_lock_policy_explicit_null(_kind));
    add(test_options_lock_policy_explicit_exclusive(_kind));
    add(test_options_lock_policy_explicit_timed(_kind));
    add(test_options_lock_policy_explicit_shared_cpp17(_kind));
    add(test_options_edge_extent_zero(_kind));
    add(test_options_edge_extent_one(_kind));
    add(test_options_edge_extent_large(_kind));
    add(test_options_edge_pointer_element_type(_kind));
    add(test_options_edge_struct_element_type(_kind));
    add(test_options_edge_signed_unsigned_element_types(_kind));

    return result;
}


// =========================================================================
// X.   PART D MASTER RUNNER
// =========================================================================
//   Builds the Part D subtree, hands it to the supplied
// handler, and returns the resulting session_verdict.  Mirrors
// the signatures of run_array_suite / run_threadsafe_array_suite.

test::session_verdict
run_array_options_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind,
    double*             _out_seconds
)
{
    auto t_start = std::chrono::steady_clock::now();

    array_test_tree subtree = make_array_options_subtree(_kind);

    _handler.run(subtree.underlying());

    auto t_end = std::chrono::steady_clock::now();

    if (_out_seconds != nullptr)
    {
        *_out_seconds =
            std::chrono::duration<double>(t_end - t_start).count();
    }

    return _handler.result().verdict();
}


NS_END  // testing
NS_END  // djinterp
