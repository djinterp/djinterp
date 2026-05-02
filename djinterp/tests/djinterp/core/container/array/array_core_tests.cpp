/******************************************************************************
* djinterp [testing]                                       array_core_tests.cpp
*
*   Implementation of the array test suite under the return-a-subtree
* protocol declared in array_tests.hpp.
*
*   STRUCTURE:
*   Every category function below builds and returns a self-contained
* `array_test_tree` (the project's `test::test_tree` overlay backed
* by `djinterp::nary_tree`).  Tests do NOT take a
* test_handler or a test_printer; they do NOT mutate any caller-
* supplied sink; they simply construct a small tree and return it
* by value.
*
*   THE LIFETIME x ITERABILITY CUBE:
*   The array template is parameterized on (Type, Extent, Lifetime,
* Iterability).  The Lifetime axis has three values
* (constexpr_lifetime, immutable_lifetime, mutable_lifetime) and the
* Iterability axis has two (iterable, non_iterable), giving six
* named cells.  The convenience aliases declared in array.hpp expose
* each cell ergonomically.  The tests below exercise each cell where
* the operation under test is meaningful.  Mutation tests skip
* immutable cells (they SFINAE-reject the mutator); iteration tests
* skip non-iterable cells (they SFINAE-reject begin/end); SFINAE-
* rejection tests do the inverse.
*
*
* path:      /tests/djinterp/core/container/array/array_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/
#include "./array_tests.hpp"

// std
#include <chrono>      // steady_clock + duration in run_array_suite


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
        node_alias*      _parent,
        bool             _passed,
        const char*      _name
    )
    {
        return _tree.underlying().append_child(
            _parent,
            test::make_assert(_passed, _name));
    }


    // make_block_tree
    //   helper: constructs a fresh array_test_tree whose root is
    // a test_block-kind interior node carrying _block_name.
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


    // graft_subtree_recursive
    //   helper: recursive worker for graft_subtree().
    inline void
    graft_subtree_recursive(
        array_test_tree&  _dest,
        node_alias*       _dest_parent,
        const node_alias* _src
    )
    {
        node_alias*       new_node;
        const node_alias* child;

        if (_src == nullptr)
        {
            return;
        }

        new_node = _dest.underlying().append_child(_dest_parent,
                                                   _src->data());

        child = _src->first_child();

        while (child != nullptr)
        {
            graft_subtree_recursive(_dest, new_node, child);
            child = child->next_sibling();
        }

        return;
    }


    // graft_subtree
    //   helper: copies _src's full structure under _dest_parent.
    inline void
    graft_subtree(
        array_test_tree&       _dest,
        node_alias*            _dest_parent,
        const array_test_tree& _src
    )
    {
        if (_dest_parent == nullptr)
        {
            return;
        }

        graft_subtree_recursive(_dest,
                                _dest_parent,
                                _src.underlying().root());

        return;
    }


    // -------------------------------------------------------------------
    //  cube-cell aliases
    // -------------------------------------------------------------------
    template<typename T, std::size_t N>
    using mi_arr  = djinterp::mutable_iterable_array<T, N>;

    template<typename T, std::size_t N>
    using mn_arr  = djinterp::mutable_non_iterable_array<T, N>;

    template<typename T, std::size_t N>
    using imi_arr = djinterp::immutable_iterable_array<T, N>;

    template<typename T, std::size_t N>
    using imn_arr = djinterp::immutable_non_iterable_array<T, N>;

    template<typename T, std::size_t N>
    using cxi_arr = djinterp::constexpr_iterable_array<T, N>;

    template<typename T, std::size_t N>
    using cxn_arr = djinterp::constexpr_non_iterable_array<T, N>;


    // -------------------------------------------------------------------
    //  SFINAE expression detectors
    // -------------------------------------------------------------------
    template<typename T, typename = void>
    struct has_begin : std::false_type {};
    template<typename T>
    struct has_begin<T,
        decltype(void(std::declval<T&>().begin()))>
        : std::true_type {};

    template<typename T, typename = void>
    struct has_end : std::false_type {};
    template<typename T>
    struct has_end<T,
        decltype(void(std::declval<T&>().end()))>
        : std::true_type {};

    template<typename T, typename = void>
    struct has_rbegin : std::false_type {};
    template<typename T>
    struct has_rbegin<T,
        decltype(void(std::declval<T&>().rbegin()))>
        : std::true_type {};

    template<typename T, typename = void>
    struct has_fill : std::false_type {};
    template<typename T>
    struct has_fill<T,
        decltype(void(std::declval<T&>().fill(
            std::declval<typename T::value_type>())))>
        : std::true_type {};

    template<typename T, typename = void>
    struct has_member_swap : std::false_type {};
    template<typename T>
    struct has_member_swap<T,
        decltype(void(std::declval<T&>().swap(
            std::declval<T&>())))>
        : std::true_type {};

}  // unnamed namespace


// =========================================================================
// II.  CATEGORY: TRAIT CONFORMANCE
// =========================================================================

array_test_tree
test_array_axis_constexpr_runtime(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("axis: constexpr / runtime");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        cxi_arr<int, 4>::lifetime == array_lifetime::constexpr_lifetime,
        "constexpr_iterable: lifetime == constexpr_lifetime");
    append_leaf(tree, root,
        cxn_arr<int, 4>::lifetime == array_lifetime::constexpr_lifetime,
        "constexpr_non_iterable: lifetime == constexpr_lifetime");
    append_leaf(tree, root,
        mi_arr<int, 4>::lifetime != array_lifetime::constexpr_lifetime,
        "mutable_iterable: lifetime != constexpr_lifetime");
    append_leaf(tree, root,
        imi_arr<int, 4>::lifetime != array_lifetime::constexpr_lifetime,
        "immutable_iterable: lifetime != constexpr_lifetime");
    append_leaf(tree, root,
        is_constexpr_array<cxi_arr<int, 4>>::value,
        "is_constexpr_array<cxi_arr> == true");

    return tree;
}


array_test_tree
test_array_axis_mutable_immutable(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("axis: mutable / immutable");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        mi_arr<int, 4>::lifetime == array_lifetime::mutable_lifetime,
        "mutable_iterable: lifetime == mutable_lifetime");
    append_leaf(tree, root,
        mn_arr<int, 4>::lifetime == array_lifetime::mutable_lifetime,
        "mutable_non_iterable: lifetime == mutable_lifetime");
    append_leaf(tree, root,
        imi_arr<int, 4>::lifetime == array_lifetime::immutable_lifetime,
        "immutable_iterable: lifetime == immutable_lifetime");
    append_leaf(tree, root,
        imn_arr<int, 4>::lifetime == array_lifetime::immutable_lifetime,
        "immutable_non_iterable: lifetime == immutable_lifetime");
    append_leaf(tree, root,
        is_mutable_array<mi_arr<int, 4>>::value,
        "is_mutable_array<mi_arr> == true");
    append_leaf(tree, root,
        is_immutable_array<imi_arr<int, 4>>::value,
        "is_immutable_array<imi_arr> == true");
    append_leaf(tree, root,
        !is_mutable_array<imi_arr<int, 4>>::value,
        "is_mutable_array<imi_arr> == false");
    append_leaf(tree, root,
        !is_immutable_array<mi_arr<int, 4>>::value,
        "is_immutable_array<mi_arr> == false");

    return tree;
}


array_test_tree
test_array_axis_iterable_non_iterable(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("axis: iterable / non-iterable");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        mi_arr<int, 4>::iterability == array_iterability::iterable,
        "mutable_iterable: iterability == iterable");
    append_leaf(tree, root,
        mn_arr<int, 4>::iterability == array_iterability::non_iterable,
        "mutable_non_iterable: iterability == non_iterable");
    append_leaf(tree, root,
        imi_arr<int, 4>::iterability == array_iterability::iterable,
        "immutable_iterable: iterability == iterable");
    append_leaf(tree, root,
        imn_arr<int, 4>::iterability == array_iterability::non_iterable,
        "immutable_non_iterable: iterability == non_iterable");
    append_leaf(tree, root,
        is_iterable_array<mi_arr<int, 4>>::value,
        "is_iterable_array<mi_arr> == true");
    append_leaf(tree, root,
        is_non_iterable_array<mn_arr<int, 4>>::value,
        "is_non_iterable_array<mn_arr> == true");

    return tree;
}


array_test_tree
test_array_axis_bounded(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("axis: bounded");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        is_bounded_container<mi_arr<int, 4>>::value,
        "is_bounded_container<mi_arr> == true");
    append_leaf(tree, root,
        is_bounded_container<imn_arr<int, 8>>::value,
        "is_bounded_container<imn_arr> == true");
    append_leaf(tree, root,
        has_fixed_extent_signal<cxi_arr<int, 4>>::value,
        "has_fixed_extent_signal<cxi_arr> == true");
    append_leaf(tree, root,
        !is_unbounded_container<mi_arr<int, 4>>::value,
        "is_unbounded_container<mi_arr> == false");

    mi_arr<int, 7> a;
    append_leaf(tree, root,
        a.size() == 7,
        "size() == 7");
    append_leaf(tree, root,
        a.max_size() == 7,
        "max_size() == 7");
    append_leaf(tree, root,
        a.capacity() == 7,
        "capacity() == 7");

    return tree;
}


array_test_tree
test_array_axis_sorted_unsorted(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("axis: sorted / unsorted");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        !is_sorted_container<mi_arr<int, 4>>::value,
        "is_sorted_container<mi_arr> == false");
    append_leaf(tree, root,
        is_unsorted_container<mi_arr<int, 4>>::value,
        "is_unsorted_container<mi_arr> == true");
    append_leaf(tree, root,
        !has_key_compare_alias<mi_arr<int, 4>>::value,
        "no key_compare alias");
    append_leaf(tree, root,
        !has_hasher_alias<mi_arr<int, 4>>::value,
        "no hasher alias");

    return tree;
}


array_test_tree
test_array_axis_flat_hierarchical(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("axis: flat / hierarchical");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        is_flat_container<mi_arr<int, 4>>::value,
        "is_flat_container<array<int>> == true");
    append_leaf(tree, root,
        !is_hierarchical_container<mi_arr<int, 4>>::value,
        "is_hierarchical_container<array<int>> == false");
    append_leaf(tree, root,
        container_depth<mi_arr<int, 4>>::value == 1,
        "container_depth<array<int>> == 1");

    using nested_t = mi_arr<mi_arr<int, 3>, 4>;
    append_leaf(tree, root,
        is_hierarchical_container<nested_t>::value,
        "is_hierarchical_container<array<array<int>>> == true");
    append_leaf(tree, root,
        container_depth<nested_t>::value == 2,
        "container_depth<array<array<int>>> == 2");

    return tree;
}


array_test_tree
test_array_axis_storage_kind(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("axis: storage kind");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        mi_arr<int, 4>::extent == 4,
        "static_extent: extent == N");
    append_leaf(tree, root,
        mi_arr<int, 4>::extent != djinterp::dynamic_extent,
        "static_extent: extent != dynamic_extent sentinel");
    append_leaf(tree, root,
        djinterp::dynamic_extent == static_cast<std::size_t>(-1),
        "dynamic_extent == size_t(-1)");
    append_leaf(tree, root,
        is_contiguous_array<mi_arr<int, 4>>::value,
        "is_contiguous_array<mi_arr> == true");

    return tree;
}


array_test_tree
test_array_lifetime_taxonomy(
    test::test_type_id /*_kind*/
)
{
    using namespace djinterp;

    array_test_tree tree = make_block_tree("lifetime taxonomy");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        ( mi_arr<int, 4>::lifetime    == array_lifetime::mutable_lifetime ) &&
        ( mi_arr<int, 4>::iterability == array_iterability::iterable ),
        "cell { mutable, iterable } pair correct");
    append_leaf(tree, root,
        ( mn_arr<int, 4>::lifetime    == array_lifetime::mutable_lifetime ) &&
        ( mn_arr<int, 4>::iterability == array_iterability::non_iterable ),
        "cell { mutable, non_iterable } pair correct");
    append_leaf(tree, root,
        ( imi_arr<int, 4>::lifetime    == array_lifetime::immutable_lifetime ) &&
        ( imi_arr<int, 4>::iterability == array_iterability::iterable ),
        "cell { immutable, iterable } pair correct");
    append_leaf(tree, root,
        ( imn_arr<int, 4>::lifetime    == array_lifetime::immutable_lifetime ) &&
        ( imn_arr<int, 4>::iterability == array_iterability::non_iterable ),
        "cell { immutable, non_iterable } pair correct");
    append_leaf(tree, root,
        ( cxi_arr<int, 4>::lifetime    == array_lifetime::constexpr_lifetime ) &&
        ( cxi_arr<int, 4>::iterability == array_iterability::iterable ),
        "cell { constexpr, iterable } pair correct");
    append_leaf(tree, root,
        ( cxn_arr<int, 4>::lifetime    == array_lifetime::constexpr_lifetime ) &&
        ( cxn_arr<int, 4>::iterability == array_iterability::non_iterable ),
        "cell { constexpr, non_iterable } pair correct");

    return tree;
}


// =========================================================================
// III. CATEGORY: CONSTRUCTION
// =========================================================================

array_test_tree
test_array_default_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("default construction");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a;
    append_leaf(tree, root,
        a.size() == 4,
        "mi_arr default: size() == 4");
    append_leaf(tree, root,
        a[0] == 0 && a[1] == 0 && a[2] == 0 && a[3] == 0,
        "mi_arr default: elements zero-initialized");

    mn_arr<int, 4> b;
    append_leaf(tree, root,
        b.size() == 4,
        "mn_arr default: size() == 4");
    append_leaf(tree, root,
        b.data()[0] == 0 && b.data()[3] == 0,
        "mn_arr default: data elements zero-initialized");

    cxi_arr<int, 4> c;
    append_leaf(tree, root,
        c.size() == 4,
        "cxi_arr default: size() == 4");

    return tree;
}


array_test_tree
test_array_pack_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("pack construction");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(10, 20, 30, 40);
    append_leaf(tree, root,
        a[0] == 10 && a[1] == 20 && a[2] == 30 && a[3] == 40,
        "mi_arr pack: values land in declaration order");

    imi_arr<int, 3> b(7, 8, 9);
    append_leaf(tree, root,
        b[0] == 7 && b[1] == 8 && b[2] == 9,
        "imi_arr pack: values land in declaration order");

    mi_arr<int, 4> c(1, 2);
    append_leaf(tree, root,
        c[0] == 1 && c[1] == 2 && c[2] == 0 && c[3] == 0,
        "mi_arr pack: tail zero-initialized when partial");

    return tree;
}


array_test_tree
test_array_copy_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("copy construction");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> src(1, 2, 3, 4);
    mi_arr<int, 4> dst(src);
    append_leaf(tree, root,
        dst[0] == 1 && dst[1] == 2 && dst[2] == 3 && dst[3] == 4,
        "mi_arr copy: values match source");

    src[0] = 99;
    append_leaf(tree, root,
        dst[0] == 1,
        "mi_arr copy: destination is independent of source");

    imi_arr<int, 3> isrc(5, 6, 7);
    imi_arr<int, 3> idst(isrc);
    append_leaf(tree, root,
        idst[0] == 5 && idst[1] == 6 && idst[2] == 7,
        "imi_arr copy: values match source");

    return tree;
}


array_test_tree
test_array_move_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("move construction");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> src(11, 22, 33, 44);
    mi_arr<int, 4> dst(std::move(src));
    append_leaf(tree, root,
        dst[0] == 11 && dst[1] == 22 && dst[2] == 33 && dst[3] == 44,
        "mi_arr move: values transferred to destination");

    mn_arr<int, 3> nsrc(1, 2, 3);
    mn_arr<int, 3> ndst(std::move(nsrc));
    append_leaf(tree, root,
        ndst.data()[0] == 1 && ndst.data()[1] == 2 && ndst.data()[2] == 3,
        "mn_arr move: data() values transferred");

    return tree;
}


array_test_tree
test_array_zero_extent_edge_case(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("zero-extent edge case");
    auto* root = tree.underlying().root();

    mi_arr<int, 0> a;
    append_leaf(tree, root,
        a.size() == 0,
        "mi_arr<int,0>: size() == 0");
    append_leaf(tree, root,
        a.empty(),
        "mi_arr<int,0>: empty() == true");
    append_leaf(tree, root,
        a.begin() == a.end(),
        "mi_arr<int,0>: begin() == end()");
    append_leaf(tree, root,
        mi_arr<int, 0>::extent == 0,
        "mi_arr<int,0>::extent == 0");

    return tree;
}


array_test_tree
test_array_single_extent_edge_case(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("single-extent edge case");
    auto* root = tree.underlying().root();

    mi_arr<int, 1> a(42);
    append_leaf(tree, root,
        a.size() == 1,
        "mi_arr<int,1>: size() == 1");
    append_leaf(tree, root,
        !a.empty(),
        "mi_arr<int,1>: empty() == false");
    append_leaf(tree, root,
        a[0] == 42,
        "mi_arr<int,1>: a[0] == 42");
    append_leaf(tree, root,
        &a.front() == &a.back(),
        "mi_arr<int,1>: front() and back() are the same element");

    return tree;
}


// =========================================================================
// IV.  CATEGORY: ELEMENT ACCESS
// =========================================================================

array_test_tree
test_array_subscript_access(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("subscript access");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(10, 20, 30, 40);
    append_leaf(tree, root,
        a[0] == 10,
        "operator[]: index 0");
    append_leaf(tree, root,
        a[3] == 40,
        "operator[]: index N-1");

    int* p0 = &a[0];
    int* p1 = &a[1];
    append_leaf(tree, root,
        p1 == p0 + 1,
        "operator[]: contiguous storage (consecutive addresses)");

    const mi_arr<int, 4>& c = a;
    append_leaf(tree, root,
        c[2] == 30,
        "operator[] const: read-only access");

    return tree;
}


array_test_tree
test_array_at_access(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("at() access");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(7, 8, 9, 10);
    append_leaf(tree, root,
        a.at(0) == 7,
        "at(0)");
    append_leaf(tree, root,
        a.at(3) == 10,
        "at(N-1)");
    append_leaf(tree, root,
        &a.at(2) == &a[2],
        "at(i) and operator[](i) yield the same lvalue");

    const mi_arr<int, 4>& cref = a;
    append_leaf(tree, root,
        cref.at(1) == 8,
        "at() const: read-only access");

    return tree;
}


array_test_tree
test_array_front_back_access(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("front()/back() access");
    auto* root = tree.underlying().root();

    mi_arr<int, 5> a(1, 2, 3, 4, 5);
    append_leaf(tree, root,
        a.front() == 1,
        "front() == first element");
    append_leaf(tree, root,
        a.back() == 5,
        "back() == last element");
    append_leaf(tree, root,
        &a.front() == a.data(),
        "&front() == data()");
    append_leaf(tree, root,
        &a.back() == a.data() + (a.size() - 1),
        "&back() == data() + N - 1");

    const mi_arr<int, 5>& cref = a;
    append_leaf(tree, root,
        cref.front() == 1 && cref.back() == 5,
        "const front()/back() agree");

    return tree;
}


array_test_tree
test_array_data_access(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("data() access");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(2, 4, 6, 8);
    int* p = a.data();
    append_leaf(tree, root,
        p != nullptr,
        "data() != nullptr for non-empty array");
    append_leaf(tree, root,
        p[0] == 2 && p[1] == 4 && p[2] == 6 && p[3] == 8,
        "data()[i] matches operator[](i)");
    append_leaf(tree, root,
        p == &a[0],
        "data() == &a[0]");

    p[2] = 99;
    append_leaf(tree, root,
        a[2] == 99,
        "writes through data() are observable via operator[]");

    return tree;
}


array_test_tree
test_array_const_access_paths(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("const access paths");
    auto* root = tree.underlying().root();

    const mi_arr<int, 4> a(1, 2, 3, 4);

    append_leaf(tree, root,
        std::is_same<decltype(a[0]),     const int&>::value,
        "const operator[] returns const_reference");
    append_leaf(tree, root,
        std::is_same<decltype(a.data()), const int*>::value,
        "const data() returns const_pointer");
    append_leaf(tree, root,
        std::is_same<decltype(a.front()), const int&>::value,
        "const front() returns const_reference");
    append_leaf(tree, root,
        std::is_same<decltype(a.back()), const int&>::value,
        "const back() returns const_reference");

    return tree;
}


// =========================================================================
// V.   CATEGORY: ITERATION
// =========================================================================

array_test_tree
test_array_begin_end(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("begin()/end()");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(10, 20, 30, 40);

    append_leaf(tree, root,
        (a.end() - a.begin()) == static_cast<std::ptrdiff_t>(a.size()),
        "end() - begin() == size()");
    append_leaf(tree, root,
        *a.begin() == 10,
        "*begin() == front()");
    append_leaf(tree, root,
        a.begin() == a.data(),
        "begin() == data()");

    int sum = 0;
    for (auto it = a.begin(); it != a.end(); ++it) { sum += *it; }
    append_leaf(tree, root,
        sum == 100,
        "sum over [begin, end) == 100");

    return tree;
}


array_test_tree
test_array_const_iteration(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("const iteration");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(1, 2, 3, 4);

    append_leaf(tree, root,
        std::is_same<
            decltype(a.cbegin()),
            decltype(a)::const_iterator
        >::value,
        "cbegin() returns const_iterator");
    append_leaf(tree, root,
        a.cbegin() == a.begin(),
        "cbegin() == begin() (same address)");

    const mi_arr<int, 4>& cref = a;
    int sum = 0;
    for (auto it = cref.begin(); it != cref.end(); ++it) { sum += *it; }
    append_leaf(tree, root,
        sum == 10,
        "iteration on const reference: sum == 10");

    return tree;
}


array_test_tree
test_array_reverse_iteration(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("reverse iteration");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(1, 2, 3, 4);

    append_leaf(tree, root,
        a.rbegin().base() == a.end(),
        "rbegin().base() == end()");
    append_leaf(tree, root,
        *a.rbegin() == a.back(),
        "*rbegin() == back()");

    int seen[4] = {0, 0, 0, 0};
    int  i      = 0;
    for (auto it = a.rbegin(); it != a.rend() && i < 4; ++it, ++i)
    {
        seen[i] = *it;
    }
    append_leaf(tree, root,
        ( seen[0] == 4 && seen[1] == 3 && seen[2] == 2 && seen[3] == 1 ),
        "reverse walk yields 4,3,2,1");

    return tree;
}


array_test_tree
test_array_range_based_for(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("range-based for");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(1, 2, 3, 4);

    int count = 0;
    for (auto& e : a) { (void)e; ++count; }
    append_leaf(tree, root,
        count == 4,
        "range-for visits every element");

    const mi_arr<int, 4>& cref = a;
    int sum = 0;
    for (const auto& e : cref) { sum += e; }
    append_leaf(tree, root,
        sum == 10,
        "range-for over const& sums to 10");

    return tree;
}


array_test_tree
test_array_non_iterable_sfinae(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("non-iterable SFINAE");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        !has_begin<mn_arr<int, 4>>::value,
        "mn_arr: begin() SFINAE-rejected");
    append_leaf(tree, root,
        !has_end<mn_arr<int, 4>>::value,
        "mn_arr: end() SFINAE-rejected");
    append_leaf(tree, root,
        !has_rbegin<mn_arr<int, 4>>::value,
        "mn_arr: rbegin() SFINAE-rejected");
    append_leaf(tree, root,
        !has_begin<imn_arr<int, 4>>::value,
        "imn_arr: begin() SFINAE-rejected");
    append_leaf(tree, root,
        !has_begin<cxn_arr<int, 4>>::value,
        "cxn_arr: begin() SFINAE-rejected");
    append_leaf(tree, root,
        has_begin<mi_arr<int, 4>>::value,
        "mi_arr: begin() exposed (sanity)");

    return tree;
}


// =========================================================================
// VI.  CATEGORY: MUTATION
// =========================================================================

array_test_tree
test_array_subscript_assignment(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("subscript assignment");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(0, 0, 0, 0);
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    a[3] = 40;
    append_leaf(tree, root,
        a[0] == 10 && a[1] == 20 && a[2] == 30 && a[3] == 40,
        "subscript assignment: values stored");

    a.at(2) = 99;
    append_leaf(tree, root,
        a[2] == 99,
        "at() assignment is observable through operator[]");

    return tree;
}


array_test_tree
test_array_fill(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("fill()");
    auto* root = tree.underlying().root();

    mi_arr<int, 5> a(0, 0, 0, 0, 0);
    a.fill(7);
    append_leaf(tree, root,
        a[0] == 7 && a[1] == 7 && a[2] == 7 && a[3] == 7 && a[4] == 7,
        "fill(7): every element == 7");

    mn_arr<int, 3> b;
    b.fill(42);
    append_leaf(tree, root,
        b.data()[0] == 42 && b.data()[1] == 42 && b.data()[2] == 42,
        "fill on mn_arr: data() observed");

    a.fill(0);
    append_leaf(tree, root,
        a[0] == 0 && a[4] == 0,
        "re-fill replaces previous contents");

    return tree;
}


array_test_tree
test_array_member_swap(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("member swap()");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(1, 2, 3, 4);
    mi_arr<int, 4> b(10, 20, 30, 40);
    a.swap(b);

    append_leaf(tree, root,
        a[0] == 10 && a[1] == 20 && a[2] == 30 && a[3] == 40,
        "after swap: a holds b's original values");
    append_leaf(tree, root,
        b[0] == 1 && b[1] == 2 && b[2] == 3 && b[3] == 4,
        "after swap: b holds a's original values");

    a.swap(b);
    append_leaf(tree, root,
        a[0] == 1 && b[0] == 10,
        "swap is its own inverse (two swaps == no-op)");

    return tree;
}


array_test_tree
test_array_immutable_sfinae(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("immutable SFINAE");
    auto* root = tree.underlying().root();

    append_leaf(tree, root,
        !has_fill<imi_arr<int, 4>>::value,
        "imi_arr: fill() SFINAE-rejected");
    append_leaf(tree, root,
        !has_member_swap<imi_arr<int, 4>>::value,
        "imi_arr: swap() SFINAE-rejected");
    append_leaf(tree, root,
        !has_fill<imn_arr<int, 4>>::value,
        "imn_arr: fill() SFINAE-rejected");
    append_leaf(tree, root,
        has_fill<mi_arr<int, 4>>::value,
        "mi_arr: fill() exposed (sanity)");
    append_leaf(tree, root,
        has_member_swap<mi_arr<int, 4>>::value,
        "mi_arr: swap() exposed (sanity)");

    return tree;
}


// =========================================================================
// VII. CATEGORY: BULK ALGORITHMS
// =========================================================================

array_test_tree
test_array_equal_function(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("array_equal()");
    auto* root = tree.underlying().root();

    mi_arr<int, 4>  a(1, 2, 3, 4);
    mi_arr<int, 4>  b(1, 2, 3, 4);
    mi_arr<int, 4>  c(1, 2, 3, 5);
    imi_arr<int, 4> d(1, 2, 3, 4);

    append_leaf(tree, root,
        djinterp::array_equal(a, b),
        "array_equal: identical elements -> true");
    append_leaf(tree, root,
        !djinterp::array_equal(a, c),
        "array_equal: differ at last element -> false");
    append_leaf(tree, root,
        djinterp::array_equal(a, d),
        "array_equal: works across mutable / immutable cells");
    append_leaf(tree, root,
        djinterp::array_equal(a, a),
        "array_equal: array equals itself");

    return tree;
}


array_test_tree
test_array_copy_function(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("array_copy()");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> src(7, 8, 9, 10);
    mi_arr<int, 4> dst(0, 0, 0, 0);
    djinterp::array_copy(src, dst);
    append_leaf(tree, root,
        djinterp::array_equal(src, dst),
        "array_copy: dst == src after copy");

    src[0] = 999;
    append_leaf(tree, root,
        dst[0] == 7,
        "array_copy: dst is independent of src after copy");

    imi_arr<int, 3> isrc(1, 2, 3);
    mi_arr<int, 3>  mdst(0, 0, 0);
    djinterp::array_copy(isrc, mdst);
    append_leaf(tree, root,
        mdst[0] == 1 && mdst[1] == 2 && mdst[2] == 3,
        "array_copy: immutable source -> mutable dest");

    return tree;
}


array_test_tree
test_array_swap_function(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("array_swap()");
    auto* root = tree.underlying().root();

    mi_arr<int, 4> a(1, 2, 3, 4);
    mi_arr<int, 4> b(10, 20, 30, 40);
    djinterp::array_swap(a, b);

    append_leaf(tree, root,
        a[0] == 10 && a[3] == 40,
        "array_swap: a holds b's original values");
    append_leaf(tree, root,
        b[0] == 1 && b[3] == 4,
        "array_swap: b holds a's original values");

    mi_arr<int, 3> p(100, 200, 300);
    mn_arr<int, 3> q(1, 2, 3);
    djinterp::array_swap(p, q);
    append_leaf(tree, root,
        p[0] == 1 && q.data()[0] == 100,
        "array_swap: works across iterability variants");

    return tree;
}


// =========================================================================
// VIII. CATEGORY: CONSTEXPR USABILITY
// =========================================================================

array_test_tree
test_array_constexpr_construction(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("constexpr construction");
    auto* root = tree.underlying().root();

    constexpr cxi_arr<int, 4> a;
    append_leaf(tree, root,
        a.size() == 4,
        "constexpr default: size() == 4");

    constexpr cxi_arr<int, 4> b(1, 2, 3, 4);
    append_leaf(tree, root,
        b[0] == 1 && b[3] == 4,
        "constexpr pack: values present at compile time");

    constexpr std::size_t sz = b.size();
    append_leaf(tree, root,
        sz == 4,
        "constexpr size(): result usable as constant expression");

    return tree;
}


array_test_tree
test_array_constexpr_access(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("constexpr access");
    auto* root = tree.underlying().root();

    constexpr cxi_arr<int, 4> a(10, 20, 30, 40);

    constexpr int v0 = a[0];
    constexpr int v3 = a[3];
    append_leaf(tree, root,
        v0 == 10 && v3 == 40,
        "constexpr operator[]: values usable as constant expressions");

    constexpr int va = a.at(2);
    append_leaf(tree, root,
        va == 30,
        "constexpr at(): result usable as constant expression");

    constexpr int f = a.front();
    constexpr int bk = a.back();
    append_leaf(tree, root,
        f == 10 && bk == 40,
        "constexpr front()/back(): results usable as constant expressions");

    return tree;
}


array_test_tree
test_array_constexpr_mutation_cpp14(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree("constexpr mutation (C++14+)");
    auto* root = tree.underlying().root();

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    struct cx14
    {
        static constexpr cxi_arr<int, 4> filled()
        {
            cxi_arr<int, 4> a(0, 0, 0, 0);
            a[0] = 1;
            a[1] = 2;
            a[2] = 3;
            a[3] = 4;
            return a;
        }
    };
    constexpr cxi_arr<int, 4> a = cx14::filled();
    append_leaf(tree, root,
        a[0] == 1 && a[3] == 4,
        "C++14: relaxed-constexpr mutation observable in constant expression");

    struct cx14_fill
    {
        static constexpr cxi_arr<int, 3> all_sevens()
        {
            cxi_arr<int, 3> b;
            b.fill(7);
            return b;
        }
    };
    constexpr cxi_arr<int, 3> b = cx14_fill::all_sevens();
    append_leaf(tree, root,
        b[0] == 7 && b[1] == 7 && b[2] == 7,
        "C++14: constexpr fill() yields all-sevens at compile time");
#else
    append_leaf(tree, root,
        true,
        "C++11: relaxed-constexpr mutation skipped (requires C++14)");
#endif

    return tree;
}


// =========================================================================
// IX.  CATEGORY: ITERATOR ALGORITHM INTEROP
// =========================================================================

array_test_tree
test_array_constexpr_iterator_algorithms(
    test::test_type_id /*_kind*/
)
{
    array_test_tree tree = make_block_tree(
        "constexpr_iterator algorithm interop");
    auto* root = tree.underlying().root();

    constexpr cxi_arr<int, 4> a(1, 2, 3, 4);
    constexpr cxi_arr<int, 4> b(1, 2, 3, 4);
    constexpr cxi_arr<int, 4> c(1, 2, 9, 4);

    append_leaf(tree, root,
        djinterp::array_equal(a, b),
        "constexpr cells: array_equal agrees on identical content");
    append_leaf(tree, root,
        !djinterp::array_equal(a, c),
        "constexpr cells: array_equal differentiates content");
    append_leaf(tree, root,
        a.size() == cxi_arr<int, 4>::extent,
        "constexpr cell: size() == extent");

    int sum = 0;
    for (auto it = a.begin(); it != a.end(); ++it) { sum += *it; }
    append_leaf(tree, root,
        sum == 10,
        "constexpr cell: runtime iteration sums to 10");

    return tree;
}


// =========================================================================
// X.   AGGREGATE SUBTREE BUILDER
// =========================================================================

array_test_tree
make_array_test_subtree(
    test::test_type_id _kind
)
{
    array_test_tree result;
    node_alias*     root;

    result.underlying().emplace_root(
        array_test_obj(_kind, true, "array test module"));

    root = result.underlying().root();

    // II. trait conformance
    graft_subtree(result, root, test_array_axis_constexpr_runtime(_kind));
    graft_subtree(result, root, test_array_axis_mutable_immutable(_kind));
    graft_subtree(result, root, test_array_axis_iterable_non_iterable(_kind));
    graft_subtree(result, root, test_array_axis_bounded(_kind));
    graft_subtree(result, root, test_array_axis_sorted_unsorted(_kind));
    graft_subtree(result, root, test_array_axis_flat_hierarchical(_kind));
    graft_subtree(result, root, test_array_axis_storage_kind(_kind));
    graft_subtree(result, root, test_array_lifetime_taxonomy(_kind));

    // III. construction
    graft_subtree(result, root, test_array_default_construction(_kind));
    graft_subtree(result, root, test_array_pack_construction(_kind));
    graft_subtree(result, root, test_array_copy_construction(_kind));
    graft_subtree(result, root, test_array_move_construction(_kind));
    graft_subtree(result, root, test_array_zero_extent_edge_case(_kind));
    graft_subtree(result, root, test_array_single_extent_edge_case(_kind));

    // IV. element access
    graft_subtree(result, root, test_array_subscript_access(_kind));
    graft_subtree(result, root, test_array_at_access(_kind));
    graft_subtree(result, root, test_array_front_back_access(_kind));
    graft_subtree(result, root, test_array_data_access(_kind));
    graft_subtree(result, root, test_array_const_access_paths(_kind));

    // V. iteration
    graft_subtree(result, root, test_array_begin_end(_kind));
    graft_subtree(result, root, test_array_const_iteration(_kind));
    graft_subtree(result, root, test_array_reverse_iteration(_kind));
    graft_subtree(result, root, test_array_range_based_for(_kind));
    graft_subtree(result, root, test_array_non_iterable_sfinae(_kind));

    // VI. mutation
    graft_subtree(result, root, test_array_subscript_assignment(_kind));
    graft_subtree(result, root, test_array_fill(_kind));
    graft_subtree(result, root, test_array_member_swap(_kind));
    graft_subtree(result, root, test_array_immutable_sfinae(_kind));

    // VII. bulk algorithms
    graft_subtree(result, root, test_array_equal_function(_kind));
    graft_subtree(result, root, test_array_copy_function(_kind));
    graft_subtree(result, root, test_array_swap_function(_kind));

    // VIII. constexpr usability
    graft_subtree(result, root, test_array_constexpr_construction(_kind));
    graft_subtree(result, root, test_array_constexpr_access(_kind));
    graft_subtree(result, root, test_array_constexpr_mutation_cpp14(_kind));

    // IX. iterator algorithm interop
    graft_subtree(result, root, test_array_constexpr_iterator_algorithms(_kind));

    return result;
}


// =========================================================================
// XI.  MASTER-SUITE RUNNER
// =========================================================================

test::session_verdict
run_array_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind,
    double*             _out_seconds
)
{
    array_test_tree                                    tree(
        djinterp::nary_tree<array_test_obj>{});
    std::chrono::steady_clock::time_point              start;
    std::chrono::steady_clock::time_point              end;
    std::chrono::duration<double>                      elapsed;

    tree = make_array_test_subtree(_kind);

    start = std::chrono::steady_clock::now();
    _handler.run(tree.underlying());
    end   = std::chrono::steady_clock::now();

    if (_out_seconds != nullptr)
    {
        elapsed       = end - start;
        *_out_seconds = elapsed.count();
    }

    return _handler.result().verdict();
}


NS_END  // testing
NS_END  // djinterp