/******************************************************************************
* djinterp [test]                                          array_core_tests.cpp
*
*   Definitions for the array container test suite declared in
* array_tests.hpp.
*
*   Each function appends one or more basic_test objects to the
* `_out` sink, stamped with the caller-supplied `_kind`.  Tests
* cover:
*
*     - structural trait conformance for every cell of the
*       lifetime × iterability cube;
*     - construction, access, iteration, and mutation behavior;
*     - free-function bulk algorithms;
*     - constexpr usability (C++11 read-only path; C++14+ mutator
*       path);
*     - integration with the constexpr_iterator algorithm family.
*
*   Edge cases addressed throughout:
*     - extent == 0  (zero-element array; size()/empty() must
*       respect this; back() is undefined and not exercised).
*     - extent == 1  (front() == back(); single-step iteration).
*     - immutable cells must NOT expose mutator methods; we use
*       SFINAE-detected traits to assert the surface.
*     - non-iterable cells must NOT expose begin()/end(); same
*       SFINAE-based assertion.
*     - copy / move construction must produce element-wise equal
*       arrays without sharing storage with the source.
*     - moved-from arrays remain in a valid state but their
*       contents are unspecified — we test only validity, not
*       contents.
*
*   PORTABILITY:
*   C++11 baseline.  Concept-based assertions and C++14+
* relaxed-constexpr tests are gated.
*
*
* path:      /src/djinterp/test/container/array_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/
#include "./array_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace djinterp::test;

// ===========================================================================
// Helper aliases (internal to this translation unit)
// ===========================================================================
//   Naming convention:  alias_<storage><lifetime><iter><n>
//     storage   : cm  (compile-time mutable), ci (compile-time immutable)
//     lifetime  : the array_lifetime tag
//     iter      : I (iterable) / N (non-iterable)
//     n         : the extent
namespace
{
    using array_lifetime_t = array_lifetime;
    using array_iterability_t = array_iterability;

    // Mutable + iterable cells (extent 0, 1, 4)
    using mut_iter_0 = array<int, 0,
        array_lifetime_t::mutable_lifetime,
        array_iterability_t::iterable>;
    using mut_iter_1 = array<int, 1,
        array_lifetime_t::mutable_lifetime,
        array_iterability_t::iterable>;
    using mut_iter_4 = array<int, 4,
        array_lifetime_t::mutable_lifetime,
        array_iterability_t::iterable>;

    // Mutable + non-iterable cell
    using mut_nonit_4 = array<int, 4,
        array_lifetime_t::mutable_lifetime,
        array_iterability_t::non_iterable>;

    // Immutable + iterable cell
    using imm_iter_4 = array<int, 4,
        array_lifetime_t::immutable_lifetime,
        array_iterability_t::iterable>;

    // Immutable + non-iterable cell
    using imm_nonit_4 = array<int, 4,
        array_lifetime_t::immutable_lifetime,
        array_iterability_t::non_iterable>;

    // Constexpr-tagged + iterable cell
    using constexpr_iter_4 = array<int, 4,
        array_lifetime_t::constexpr_lifetime,
        array_iterability_t::iterable>;
}  // anonymous


// ===========================================================================
// I.   Compile-time trait conformance
// ===========================================================================

/*
test_array_axis_constexpr_runtime
  Verifies the constexpr / runtime axis of the trait system on
  every array cell.
  Tests the following:
  - mutable iterable cell registers as constexpr-capable
  - immutable iterable cell registers as constexpr-capable
  - constexpr-tagged cell registers as constexpr-capable
  - the same mutable cell does not also register as runtime-only
    (mutual exclusion of the axis pair)
*/
void
test_array_axis_constexpr_runtime(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // A static-extent array<int, N> with constexpr accessors should
    // be classified as constexpr-capable.  The mutable cell qualifies
    // because its read accessors are declared D_CONSTEXPR.
    bool ok_mut_iter_is_cx = is_constexpr_container<mut_iter_4>::value;
    bool ok_imm_iter_is_cx = is_constexpr_container<imm_iter_4>::value;
    bool ok_constexpr_iter_is_cx  = is_constexpr_container<constexpr_iter_4>::value;

    record_assertion(_handler, _out, ok_mut_iter_is_cx, "mutable iterable cell is classified constexpr-capable");
    record_assertion(_handler, _out, ok_imm_iter_is_cx, "immutable iterable cell is classified constexpr-capable");
    record_assertion(_handler, _out, ok_constexpr_iter_is_cx, "constexpr-tagged cell is classified constexpr-capable");

    // Runtime classification: by definition, a constexpr-capable
    // container is NOT a runtime-only container.
    bool ok_not_runtime =
        !is_runtime_container<mut_iter_4>::value;
    record_assertion(_handler, _out, ok_not_runtime, "constexpr-capable container is not classified runtime-only");
}


/*
test_array_axis_mutable_immutable
  Verifies the mutable / immutable axis on every array cell.
  Tests the following:
  - mutable cell trips the structural mutability detector
  - mutable cell does NOT trip the immutability detector
  - immutable cell trips the structural immutability detector
  - immutable cell does NOT trip the mutability detector
*/
void
test_array_axis_mutable_immutable(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // The mutable cell exposes a non-const operator[] and data(),
    // so the structural mutability detector should fire.
    bool mut_is_mut    = is_mutable_container<mut_iter_4>::value;
    bool mut_is_immut  = is_immutable_container<mut_iter_4>::value;

    // The immutable cell exposes only const accessors; mutable
    // detector must NOT fire, immutable must.
    bool imm_is_mut    = is_mutable_container<imm_iter_4>::value;
    bool imm_is_immut  = is_immutable_container<imm_iter_4>::value;

    record_assertion(_handler, _out, mut_is_mut, "mutable cell exposes a non-const subscript / data() surface");
    record_assertion(_handler, _out, !imm_is_mut, "immutable cell does not advertise mutating accessors");
    record_assertion(_handler, _out, imm_is_immut, "immutable cell is classified read-only");
    record_assertion(_handler, _out, !mut_is_immut, "mutable cell is not classified read-only");
}


/*
test_array_axis_iterable_non_iterable
  Verifies the iterable / non-iterable axis.
  Tests the following:
  - iterable cell exposes begin() and end()
  - non-iterable cell does NOT expose begin() and end()
  - non-iterable cell is positively classified by the
    is_non_iterable_container umbrella
*/
void
test_array_axis_iterable_non_iterable(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // The iterable cell exposes begin()/end(); the non-iterable
    // cell does not.  The classification umbrellas should match.
    bool iter_yes      = is_iterable_container<mut_iter_4>::value;
    bool nonit_no      = is_iterable_container<mut_nonit_4>::value;
    bool nonit_classed = is_non_iterable_container<mut_nonit_4>::value;

    record_assertion(_handler, _out, iter_yes, "iterable cell exposes begin() and end()");
    record_assertion(_handler, _out, !nonit_no, "non-iterable cell does not expose begin() / end()");
    record_assertion(_handler, _out, nonit_classed, "non-iterable cell is classified non-iterable");
}


/*
test_array_axis_bounded
  Verifies the bounded / unbounded axis on every array cell.
  Tests the following:
  - every array cell (mutable iterable, mutable non-iterable,
    immutable iterable, immutable non-iterable, constexpr
    iterable) is classified bounded
  - no array cell is classified unbounded — they all carry
    a compile-time extent
*/
void
test_array_axis_bounded(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // Every array<> in this framework has a compile-time extent,
    // so bounded must fire and unbounded must not — for every cell.
    bool ok_mut_iter   = is_bounded_container<mut_iter_4>::value;
    bool ok_mut_nonit  = is_bounded_container<mut_nonit_4>::value;
    bool ok_imm_iter   = is_bounded_container<imm_iter_4>::value;
    bool ok_imm_nonit  = is_bounded_container<imm_nonit_4>::value;
    bool ok_constexpr_iter    = is_bounded_container<constexpr_iter_4>::value;

    bool ok_not_unbounded =
          !is_unbounded_container<mut_iter_4>::value
       && !is_unbounded_container<imm_iter_4>::value;

    record_assertion(_handler, _out, ok_mut_iter, "mutable iterable cell is classified bounded");
    record_assertion(_handler, _out, ok_mut_nonit, "mutable non-iterable cell is classified bounded");
    record_assertion(_handler, _out, ok_imm_iter, "immutable iterable cell is classified bounded");
    record_assertion(_handler, _out, ok_imm_nonit, "immutable non-iterable cell is classified bounded");
    record_assertion(_handler, _out, ok_constexpr_iter, "constexpr iterable cell is classified bounded");
    record_assertion(_handler, _out, ok_not_unbounded, "no array cell is classified unbounded");
}


/*
test_array_axis_sorted_unsorted
  Verifies that array<> classifies as a sequence container, not
  an associative one.
  Tests the following:
  - array<> is NOT classified sorted (no key_compare alias)
  - array<> IS classified unsorted
*/
void
test_array_axis_sorted_unsorted(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // array<> is a sequence container — it does NOT expose
    // key_compare or value_compare, so it must classify as
    // unsorted.
    bool sorted_no   = !is_sorted_container<mut_iter_4>::value;
    bool unsorted_yes = is_unsorted_container<mut_iter_4>::value;

    record_assertion(_handler, _out, sorted_no, "array<> is not classified as sorted");
    record_assertion(_handler, _out, unsorted_yes, "array<> is classified as unsorted");
}


/*
test_array_axis_flat_hierarchical
  Verifies the flat / hierarchical axis using the strict
  recursive-depth definition.
  Tests the following:
  - array<int, N> classifies flat
  - array<int, N> does NOT classify hierarchical
  - array<array<int, M>, N> classifies hierarchical
  - container_depth on array<int, N> reports 1
  - container_depth on the nested array reports 2
*/
void
test_array_axis_flat_hierarchical(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // array<int, N>            -> flat,         depth 1
    // array<array<int, M>, N>  -> hierarchical, depth 2
    using nested =
        array<mut_iter_4, 3,
            array_lifetime_t::mutable_lifetime,
            array_iterability_t::iterable>;

    bool flat_int   = is_flat_container<mut_iter_4>::value;
    bool hier_no    = !is_hierarchical_container<mut_iter_4>::value;
    bool hier_yes   = is_hierarchical_container<nested>::value;
    bool depth_int  = (container_depth<mut_iter_4>::value == 1);
    bool depth_nest = (container_depth<nested>::value     == 2);

    record_assertion(_handler, _out, flat_int, "array<int, N> is classified flat (depth 1)");
    record_assertion(_handler, _out, hier_no, "array<int, N> is not classified hierarchical");
    record_assertion(_handler, _out, hier_yes, "array<array<int, M>, N> is classified hierarchical (depth 2)");
    record_assertion(_handler, _out, depth_int, "container_depth on array<int, N> reports 1");
    record_assertion(_handler, _out, depth_nest, "container_depth on nested arrays reports 2");
}


/*
test_array_axis_storage_kind
  Verifies that every array cell — having a compile-time extent
  — classifies as static-storage.
  Tests the following:
  - array<> classifies as static-storage
  - array<> does NOT classify as dynamic-storage
  - array<> does NOT classify as fixed-storage
  - storage_kind_of dispatches to storage_kind::static_storage
*/
void
test_array_axis_storage_kind(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // Every array<> has a compile-time extent; classify as static.
    bool is_static  = is_static_storage_container<mut_iter_4>::value;
    bool not_dyn    = !is_dynamic_storage_container<mut_iter_4>::value;
    bool not_fixed  = !is_fixed_storage_container<mut_iter_4>::value;
    bool kind_match =
        (storage_kind_of<mut_iter_4>::value
            == storage_kind::static_storage);

    record_assertion(_handler, _out, is_static, "array<> is classified as static-storage");
    record_assertion(_handler, _out, not_dyn, "array<> is not classified as dynamic-storage");
    record_assertion(_handler, _out, not_fixed, "array<> is not classified as fixed-storage");
    record_assertion(_handler, _out, kind_match, "storage_kind_of dispatches to storage_kind::static_storage");
}


/*
test_array_lifetime_taxonomy
  Verifies that array_lifetime_of reads the array's stamped
  template parameter and classifies the iterability axis.
  Tests the following:
  - array_lifetime_of returns mutable_lifetime for the mutable cell
  - array_lifetime_of returns immutable_lifetime for the immutable cell
  - array_lifetime_of returns constexpr_lifetime for the constexpr cell
  - is_iterable_array fires for the iterable cell
  - is_non_iterable_array fires for the non-iterable cell
*/
void
test_array_lifetime_taxonomy(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // array_lifetime_of from array_traits.hpp should return the
    // tag corresponding to each cell.
    bool mut_tag =
        (array_lifetime_of<mut_iter_4>::value
            == array_lifetime::mutable_lifetime);
    bool imm_tag =
        (array_lifetime_of<imm_iter_4>::value
            == array_lifetime::immutable_lifetime);
    bool constexpr_tag =
        (array_lifetime_of<constexpr_iter_4>::value
            == array_lifetime::constexpr_lifetime);

    record_assertion(_handler, _out, mut_tag, "array_lifetime_of returns mutable_lifetime for the mutable cell");
    record_assertion(_handler, _out, imm_tag, "array_lifetime_of returns immutable_lifetime for the immutable cell");
    record_assertion(_handler, _out, constexpr_tag, "array_lifetime_of returns constexpr_lifetime for the constexpr cell");

    // is_iterable_array / is_non_iterable_array should match the
    // template parameter on every cell.
    bool iter_classed =
        is_iterable_array<mut_iter_4>::value;
    bool nonit_classed =
        is_non_iterable_array<mut_nonit_4>::value;

    record_assertion(_handler, _out, iter_classed, "is_iterable_array fires for the iterable cell");
    record_assertion(_handler, _out, nonit_classed, "is_non_iterable_array fires for the non-iterable cell");
}


// ===========================================================================
// II.  Core construction and destruction
// ===========================================================================

/*
test_array_default_construction
  Exercises the default constructor across iterable and
  non-iterable cells.
  Tests the following:
  - default-constructed mutable iterable array has the right size
  - default-constructed extent-4 array reports empty() == false
  - default-constructed array zero-initializes its elements
  - default-constructed non-iterable cell behaves the same when
    inspected via data() instead of subscript
*/
void
test_array_default_construction(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // Default-constructed mutable arrays must report the right
    // size and produce zero-initialized elements when the element
    // type is a fundamental arithmetic type (per our T m_data{};
    // initializer in the array body).
    mut_iter_4 a;
    bool size_ok = (a.size() == 4);
    bool empty_ok = !a.empty();        // size 4 != 0

    bool zeros_ok = true;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != 0)
        {
            zeros_ok = false;
            break;
        }
    }

    record_assertion(_handler, _out, size_ok, "default-constructed array reports the right size");
    record_assertion(_handler, _out, empty_ok, "default-constructed array of extent 4 is not empty");
    record_assertion(_handler, _out, zeros_ok, "default-constructed array zero-initializes its elements");

    // Non-iterable cell: same expectations, exercised through
    // data() since begin() is not available.
    mut_nonit_4 b;
    bool nonit_size  = (b.size() == 4);
    bool nonit_zeros = true;
    for (std::size_t i = 0; i < b.size(); ++i)
    {
        if (b.data()[i] != 0)
        {
            nonit_zeros = false;
            break;
        }
    }
    record_assertion(_handler, _out, nonit_size, "default-constructed non-iterable array reports the right size");
    record_assertion(_handler, _out, nonit_zeros, "default-constructed non-iterable array zero-initializes its elements");
}


/*
test_array_pack_construction
  Exercises the parameter-pack constructor on mutable and
  immutable cells.
  Tests the following:
  - parameter-pack ctor populates a mutable array in argument order
  - parameter-pack ctor populates an immutable array in argument order
*/
void
test_array_pack_construction(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // Parameter-pack construction populates each element from the
    // pack, in order.  We exercise both mutable and immutable.
    mut_iter_4 a(7, 8, 9, 10);
    bool mut_ok =
        (a[0] == 7) && (a[1] == 8) && (a[2] == 9) && (a[3] == 10);
    record_assertion(_handler, _out, mut_ok, "parameter-pack ctor populates a mutable array element-by-element");

    const imm_iter_4 b(11, 22, 33, 44);
    bool imm_ok =
        (b[0] == 11) && (b[1] == 22) && (b[2] == 33) && (b[3] == 44);
    record_assertion(_handler, _out, imm_ok, "parameter-pack ctor populates an immutable array element-by-element");
}


/*
test_array_copy_construction
  Verifies that the copy constructor produces an independent
  element-wise equal array.
  Tests the following:
  - copy ctor produces an element-wise equal array
  - copy ctor allocates independent storage from the source
  - mutating the copy does not alter the source (no aliasing)
*/
void
test_array_copy_construction(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 src(1, 2, 3, 4);
    mut_iter_4 dst(src);

    // Element-wise equal but stored in different memory.
    bool elements_equal =
        (dst[0] == 1) && (dst[1] == 2)
     && (dst[2] == 3) && (dst[3] == 4);
    bool independent_storage = (src.data() != dst.data());

    // Mutating the copy must not alter the original.
    dst[0] = 99;
    bool no_aliasing = (src[0] == 1);

    record_assertion(_handler, _out, elements_equal, "copy ctor produces an element-wise equal array");
    record_assertion(_handler, _out, independent_storage, "copy ctor allocates independent storage from the source");
    record_assertion(_handler, _out, no_aliasing, "mutating the copy does not alter the source");
}


/*
test_array_move_construction
  Verifies the move constructor from the destination side only.
  Moved-from sources are 'valid but unspecified' by contract.
  Tests the following:
  - move ctor produces the expected destination contents
  - moved-from source still reports the type's intrinsic size
*/
void
test_array_move_construction(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // Move construction: post-move source is in a "valid but
    // unspecified" state.  We test the destination's contents
    // (well-defined) and that the source's size is unchanged
    // (size is intrinsic to the type, not the value).
    mut_iter_4 src(5, 6, 7, 8);
    mut_iter_4 dst(std::move(src));

    bool dst_ok =
        (dst[0] == 5) && (dst[1] == 6)
     && (dst[2] == 7) && (dst[3] == 8);
    bool src_size_ok = (src.size() == 4);

    record_assertion(_handler, _out, dst_ok, "move ctor produces the expected destination contents");
    record_assertion(_handler, _out, src_size_ok, "moved-from source retains the type's intrinsic size");
}


/*
test_array_zero_extent_edge_case
  Pins down the behavior of an extent-0 array — modeled after
  std::array<T, 0>.  front()/back()/operator[] are intentionally
  not exercised: their behavior on extent 0 is undefined.
  Tests the following:
  - extent-0 array reports size 0
  - extent-0 array reports empty() == true
  - extent-0 array's data() pointer is non-null
    (1-element placeholder)
*/
void
test_array_zero_extent_edge_case(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // extent 0 is a recognized edge case.  size() must report 0,
    // empty() must report true, and we MUST NOT call front() /
    // back() / operator[] (their behavior is undefined, intentionally,
    // matching std::array<T, 0>).  data() may legally return a
    // non-null pointer to a 1-element scratch (per the placeholder
    // we use to avoid a zero-length array warning).
    mut_iter_0 a;
    bool size_ok    = (a.size() == 0);
    bool empty_ok   = a.empty();
    bool data_ok    = (a.data() != nullptr);  // scratch placeholder

    record_assertion(_handler, _out, size_ok, "extent-0 array reports size 0");
    record_assertion(_handler, _out, empty_ok, "extent-0 array reports empty() == true");
    record_assertion(_handler, _out, data_ok, "extent-0 array's data() pointer is non-null (scratch placeholder)");
}


/*
test_array_single_extent_edge_case
  Pins down the behavior of an extent-1 array — the front()
  == back() corner.
  Tests the following:
  - extent-1 array reports size 1
  - extent-1 array satisfies front() == back()
  - extent-1 array round-trips its single element value
  - range-based for visits exactly one element with the right value
*/
void
test_array_single_extent_edge_case(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // extent 1 is the front()==back() corner.
    mut_iter_1 a(42);
    bool size_ok      = (a.size() == 1);
    bool front_eq_back = (a.front() == a.back());
    bool value_ok     = (a.front() == 42);

    // Range-based for must visit exactly one element.
    int  count        = 0;
    int  sum          = 0;
    for (int v : a)
    {
        ++count;
        sum += v;
    }
    bool single_iter_ok = (count == 1) && (sum == 42);

    record_assertion(_handler, _out, size_ok, "extent-1 array reports size 1");
    record_assertion(_handler, _out, front_eq_back, "extent-1 array satisfies front() == back()");
    record_assertion(_handler, _out, value_ok, "extent-1 array round-trips its single element value");
    record_assertion(_handler, _out, single_iter_ok, "range-based for over extent-1 array visits exactly one element");
}


// ===========================================================================
// III. Element access
// ===========================================================================

/*
test_array_subscript_access
  Verifies that operator[] supports both reads and writes on a
  mutable array.
  Tests the following:
  - operator[] returns the correct value at every index
  - operator[] returns a writable lvalue reference
*/
void
test_array_subscript_access(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(10, 20, 30, 40);

    // Read access.
    bool reads_ok =
        (a[0] == 10) && (a[1] == 20) && (a[2] == 30) && (a[3] == 40);

    // Write access; subscript returns a non-const lvalue ref.
    a[2] = 300;
    bool write_ok = (a[2] == 300);

    record_assertion(_handler, _out, reads_ok, "operator[] returns the value at every index");
    record_assertion(_handler, _out, write_ok, "operator[] returns a writable lvalue reference");
}


/*
test_array_at_access
  Verifies the at() accessor parallels operator[].  The project's
  at() is unchecked (no throw on out-of-range).
  Tests the following:
  - at(i) returns the correct value at every index
  - at(i) returns a writable lvalue reference
*/
void
test_array_at_access(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // The project's at() is an unchecked accessor that mirrors
    // operator[]; any bounds-checking / throw protocol would live
    // in a debug-build wrapper, not here.
    mut_iter_4 a(1, 2, 3, 4);
    bool reads_ok =
        (a.at(0) == 1) && (a.at(1) == 2)
     && (a.at(2) == 3) && (a.at(3) == 4);
    a.at(1) = 200;
    bool write_ok = (a.at(1) == 200);

    record_assertion(_handler, _out, reads_ok, "at(i) returns the value at every index");
    record_assertion(_handler, _out, write_ok, "at(i) returns a writable lvalue reference");
}


/*
test_array_front_back_access
  Verifies front() and back() accessors on both the read and
  write sides.
  Tests the following:
  - front() returns the first element
  - back() returns the last element
  - front() and back() return writable lvalue references
*/
void
test_array_front_back_access(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(100, 200, 300, 400);

    bool front_ok = (a.front() == 100);
    bool back_ok  = (a.back()  == 400);

    a.front() = 111;
    a.back()  = 444;
    bool mutate_ok =
        (a.front() == 111) && (a.back() == 444);

    record_assertion(_handler, _out, front_ok, "front() returns the first element");
    record_assertion(_handler, _out, back_ok, "back() returns the last element");
    record_assertion(_handler, _out, mutate_ok, "front() and back() return writable lvalue references");
}


/*
test_array_data_access
  Verifies the contiguous data() accessor and its aliasing
  contract with subscript.
  Tests the following:
  - data() and &arr[0] point to the same storage
  - sequential pointer reads through data() match subscript reads
  - writes through data() are visible through subscript
*/
void
test_array_data_access(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(1, 2, 3, 4);
    int* p = a.data();

    // data() must point to the same storage as &a[0].
    bool aliases_ok = (p == &a[0]);

    // Sequential reads through the pointer match subscript reads.
    bool reads_ok =
        (p[0] == 1) && (p[1] == 2) && (p[2] == 3) && (p[3] == 4);

    // Writes through the pointer are visible via subscript.
    p[1] = 22;
    bool write_visible = (a[1] == 22);

    record_assertion(_handler, _out, aliases_ok, "data() and &arr[0] point to the same storage");
    record_assertion(_handler, _out, reads_ok, "sequential reads through data() match subscript reads");
    record_assertion(_handler, _out, write_visible, "writes through data() are visible through subscript");
}


/*
test_array_const_access_paths
  Verifies const-correctness of the accessor surface using
  decltype-based type probes.
  Tests the following:
  - const-overload of operator[] returns const_reference
  - const-overload of data() returns const_pointer
*/
void
test_array_const_access_paths(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // A `const mut_iter_4` must dispatch to const overloads of
    // operator[], at(), front(), back(), data().  We check
    // through SFINAE: the const-overloads must yield the
    // const_reference / const_pointer types.
    using A = mut_iter_4;
    using cref_t = decltype(std::declval<const A&>()[0]);
    using cptr_t = decltype(std::declval<const A&>().data());

    bool cref_is_const =
        std::is_same<cref_t, const int&>::value;
    bool cptr_is_const =
        std::is_same<cptr_t, const int*>::value;

    record_assertion(_handler, _out, cref_is_const, "const-overload of operator[] returns const_reference");
    record_assertion(_handler, _out, cptr_is_const, "const-overload of data() returns const_pointer");
}


// ===========================================================================
// IV.  Iteration
// ===========================================================================

/*
test_array_begin_end
  Verifies the begin/end iterator pair on a mutable iterable
  cell.
  Tests the following:
  - end() - begin() equals size() (random-access distance)
  - sequential dereference through begin() yields the element
    values in order
*/
void
test_array_begin_end(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(1, 2, 3, 4);

    // end() - begin() must equal size().
    auto b = a.begin();
    auto e = a.end();
    bool dist_ok =
        (static_cast<std::size_t>(e - b) == a.size());

    // Sequential dereference matches values.
    bool seq_ok =
        (b[0] == 1) && (b[1] == 2) && (b[2] == 3) && (b[3] == 4);

    record_assertion(_handler, _out, dist_ok, "end() - begin() equals size()");
    record_assertion(_handler, _out, seq_ok, "sequential dereference through begin() yields the element values");
}


/*
test_array_const_iteration
  Verifies cbegin() / cend() over a const array.
  Tests the following:
  - const-iterator traversal sums every element correctly
  - cbegin() returns a const_iterator type
*/
void
test_array_const_iteration(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    const mut_iter_4 a(5, 10, 15, 20);

    // cbegin / cend always yield const_iterator.
    auto b = a.cbegin();
    auto e = a.cend();
    int sum = 0;
    for (auto it = b; it != e; ++it)
    {
        sum += *it;
    }
    bool sum_ok = (sum == 50);

    using cit_t = decltype(a.cbegin());
    bool ptr_const =
        std::is_same<cit_t, const int*>::value;

    record_assertion(_handler, _out, sum_ok, "const-iterator traversal sums every element correctly");
    record_assertion(_handler, _out, ptr_const, "cbegin() returns a const_iterator type");
}


/*
test_array_reverse_iteration
  Verifies rbegin() / rend() walk the array in reverse order.
  Tests the following:
  - rbegin -> rend yields elements in strict reverse order
*/
void
test_array_reverse_iteration(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(1, 2, 3, 4);

    // rbegin -> rend should visit elements in reverse order.
    int  expected[4] = {4, 3, 2, 1};
    int  observed[4] = {0};
    int  i = 0;
    for (auto it = a.rbegin(); it != a.rend(); ++it)
    {
        observed[i++] = *it;
    }
    bool order_ok = true;
    for (int j = 0; j < 4; ++j)
    {
        if (observed[j] != expected[j])
        {
            order_ok = false;
            break;
        }
    }
    record_assertion(_handler, _out, order_ok, "rbegin -> rend visits elements in reverse order");
}


/*
test_array_range_based_for
  Verifies range-based for compatibility on both mutable and
  const arrays.
  Tests the following:
  - range-for over a mutable array sums every element correctly
  - range-for over a mutable array visits every element exactly once
  - range-for over a const array sums every element correctly
*/
void
test_array_range_based_for(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(2, 4, 6, 8);
    int sum = 0;
    int n   = 0;
    for (auto& v : a)
    {
        sum += v;
        ++n;
    }
    bool sum_ok = (sum == 20);
    bool n_ok   = (n == 4);

    // Range-for over a const array.
    const mut_iter_4 b(1, 1, 1, 1);
    int  csum = 0;
    for (const auto& v : b)
    {
        csum += v;
    }
    bool csum_ok = (csum == 4);

    record_assertion(_handler, _out, sum_ok, "range-based for over mutable array sums every element");
    record_assertion(_handler, _out, n_ok, "range-based for over mutable array visits every element");
    record_assertion(_handler, _out, csum_ok, "range-based for over const array sums every element");
}


/*
test_array_non_iterable_sfinae
  Verifies the structural contract that the non-iterable cell
  hides begin() / end() behind SFINAE.
  Tests the following:
  - non-iterable cell does NOT advertise begin() / end()
    via the iterability detector
  - non-iterable cell IS positively classified by the umbrella
    is_non_iterable_container trait
*/
void
test_array_non_iterable_sfinae(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
        // The non-iterable cell must NOT advertise begin()/end()
    // through the iterability detector.  This is the structural
    // contract.
    bool no_iter_surface =
        !is_iterable_container<mut_nonit_4>::value;
    bool yes_classified =
        is_non_iterable_container<mut_nonit_4>::value;

    record_assertion(_handler, _out, no_iter_surface, "non-iterable cell does not advertise begin() / end()");
    record_assertion(_handler, _out, yes_classified, "non-iterable cell is classified non-iterable by the umbrella trait");
}


// ===========================================================================
// V.   Mutation
// ===========================================================================

/*
test_array_subscript_assignment
  Verifies that subscript assignment writes every element
  correctly across the full extent of the array.
  Tests the following:
  - assigning every index through operator[] yields the expected
    final values
*/
void
test_array_subscript_assignment(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a;
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    bool ok =
        (a[0] == 1) && (a[1] == 2) && (a[2] == 3) && (a[3] == 4);
    record_assertion(_handler, _out, ok, "subscript assignment writes every element correctly");
}


/*
test_array_fill
  Verifies fill(v) on three extents — including the corner cases.
  Tests the following:
  - fill(v) sets every element to v on a normal extent-4 array
  - fill(v) on extent-1 array sets the single element
  - fill(v) on extent-0 array is a no-op (no UB, no crash)
*/
void
test_array_fill(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a;
    a.fill(7);
    bool all_seven = true;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != 7)
        {
            all_seven = false;
            break;
        }
    }
    record_assertion(_handler, _out, all_seven, "fill(v) sets every element to v");

    // fill on a single-extent array.
    mut_iter_1 b;
    b.fill(99);
    record_assertion(_handler, _out, b[0] == 99, "fill(v) on extent-1 array sets the single element");

    // fill on extent-0: must be a no-op (no UB, no crash).
    mut_iter_0 z;
    z.fill(123);
    record_assertion(_handler, _out, z.size() == 0, "fill(v) on extent-0 array is a no-op");
}


/*
test_array_member_swap
  Verifies the member swap() exchanges contents in both
  directions.
  Tests the following:
  - after swap, a holds b's pre-swap values
  - after swap, b holds a's pre-swap values
*/
void
test_array_member_swap(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(1, 2, 3, 4);
    mut_iter_4 b(9, 8, 7, 6);
    a.swap(b);
    bool a_after = (a[0] == 9) && (a[1] == 8)
                && (a[2] == 7) && (a[3] == 6);
    bool b_after = (b[0] == 1) && (b[1] == 2)
                && (b[2] == 3) && (b[3] == 4);
    record_assertion(_handler, _out, a_after, "swap leaves a holding b's previous values");
    record_assertion(_handler, _out, b_after, "swap leaves b holding a's previous values");
}


/*
test_array_immutable_sfinae
  Verifies the structural contract that the immutable cell hides
  every mutator behind SFINAE.
  Tests the following:
  - immutable cell exposes NO mutating member functions
  - immutable cell IS positively classified by the umbrella
    is_immutable_container trait
*/
void
test_array_immutable_sfinae(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
        // The immutable cell must NOT advertise mutators.  The
    // mutability detector relies on detecting non-const subscript
    // assignment; for an immutable cell, that detection must fail.
    bool no_mutators = !is_mutable_container<imm_iter_4>::value;
    bool classified_immut = is_immutable_container<imm_iter_4>::value;

    record_assertion(_handler, _out, no_mutators, "immutable cell exposes no mutating member functions");
    record_assertion(_handler, _out, classified_immut, "immutable cell is classified immutable by the umbrella trait");
}


// ===========================================================================
// VI.  Free-function bulk algorithms
// ===========================================================================

/*
test_array_equal_function
  Verifies the array_equal free function across positive,
  negative, and edge-case inputs.
  Tests the following:
  - returns true for two equal same-shape arrays
  - returns true across mutable / immutable lifetimes when the
    elements match
  - returns false when any element differs
  - returns true for two extent-0 arrays (vacuously equal)
*/
void
test_array_equal_function(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(1, 2, 3, 4);

    // Same shape, same lifetime, same iterability -> equal.
    mut_iter_4 b(1, 2, 3, 4);
    bool eq_same_shape = array_equal(a, b);

    // Same shape, different lifetime -> still equal element-wise.
    const imm_iter_4 c(1, 2, 3, 4);
    bool eq_cross_lifetime = array_equal(a, c);

    // Different element values -> not equal.
    mut_iter_4 d(1, 2, 3, 5);
    bool ne_diff = !array_equal(a, d);

    record_assertion(_handler, _out, eq_same_shape, "array_equal returns true for two equal same-shape arrays");
    record_assertion(_handler, _out, eq_cross_lifetime, "array_equal compares across mutable / immutable lifetimes");
    record_assertion(_handler, _out, ne_diff, "array_equal returns false when any element differs");

    // Edge: zero-extent arrays are trivially equal.
    mut_iter_0 z1, z2;
    bool eq_zero = array_equal(z1, z2);
    record_assertion(_handler, _out, eq_zero, "array_equal returns true for two extent-0 arrays");
}


/*
test_array_copy_function
  Verifies the array_copy free function.
  Tests the following:
  - produces a destination element-wise equal to the source
  - works across iterable source and non-iterable destination
*/
void
test_array_copy_function(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 src(11, 22, 33, 44);
    mut_iter_4 dst;
    array_copy(src, dst);
    bool ok = array_equal(src, dst);
    record_assertion(_handler, _out, ok, "array_copy produces a destination element-wise equal to the source");

    // Cross-iterability copy: source iterable, destination not.
    mut_nonit_4 dst2;
    array_copy(src, dst2);
    bool cross =
        (dst2.data()[0] == 11) && (dst2.data()[1] == 22)
     && (dst2.data()[2] == 33) && (dst2.data()[3] == 44);
    record_assertion(_handler, _out, cross, "array_copy works between iterable source and non-iterable destination");
}


/*
test_array_swap_function
  Verifies the array_swap free function.
  Tests the following:
  - exchanges contents between two same-shape arrays
*/
void
test_array_swap_function(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    mut_iter_4 a(1, 2, 3, 4);
    mut_iter_4 b(5, 6, 7, 8);
    array_swap(a, b);
    bool ok =
           (a[0] == 5 && a[1] == 6 && a[2] == 7 && a[3] == 8)
        && (b[0] == 1 && b[1] == 2 && b[2] == 3 && b[3] == 4);
    record_assertion(_handler, _out, ok, "array_swap exchanges contents between two same-shape arrays");
}


// ===========================================================================
// VII. Constexpr usability
// ===========================================================================

/*
test_array_constexpr_construction
  Verifies that an immutable array can be constructed in a
  constant expression.
  Tests the following:
  - constexpr ctor produces an array with the right size at
    compile time
  - individual elements are accessible as constants at compile time
*/
void
test_array_constexpr_construction(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // Construction in a constant expression.  We use a
    // function-local constexpr to force evaluation at compile time;
    // the static_assert that follows confirms it.
    constexpr imm_iter_4 a(1, 2, 3, 4);
    static_assert(a.size() == 4,
        "constexpr immutable array<int,4> must report size 4");
    static_assert(a[0] == 1,
        "constexpr immutable array element 0 must equal 1");
    static_assert(a[3] == 4,
        "constexpr immutable array element 3 must equal 4");
    record_assertion(_handler, _out, true, "immutable array can be constructed in a constant expression");
}


/*
test_array_constexpr_access
  Verifies that every accessor — front(), back(), at(), data() —
  is constexpr-evaluable.
  Tests the following:
  - constexpr front()  evaluates at compile time
  - constexpr back()   evaluates at compile time
  - constexpr at(i)    evaluates at compile time
  - constexpr data()[i] evaluates at compile time
*/
void
test_array_constexpr_access(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    constexpr imm_iter_4 a(10, 20, 30, 40);

    // Compile-time access: every accessor under test runs in a
    // constant expression context.  If any of these fail to be
    // constexpr-evaluable, the static_asserts below will fire at
    // compile time, not runtime.
    static_assert(a.front() == 10,
        "constexpr front() must equal 10");
    static_assert(a.back()  == 40,
        "constexpr back() must equal 40");
    static_assert(a.at(2)   == 30,
        "constexpr at(2) must equal 30");
    static_assert(a.data()[1] == 20,
        "constexpr data()[1] must equal 20");
    record_assertion(_handler, _out, true, "front() / back() / at() / data() are constexpr-evaluable");
}


/*
test_array_constexpr_mutation_cpp14
  Exercises the C++14+ relaxed-constexpr mutator path; falls
  back to a noted skip on pre-C++14 toolchains.
  Tests the following:
  - constexpr lambda constructs, mutates, and sums an array
    entirely at compile time
  - the resulting compile-time value matches the expected total
*/
void
test_array_constexpr_mutation_cpp14(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // C++14+ relaxed-constexpr mutator path: build a constexpr
    // helper that constructs, mutates, and returns the post-mutation
    // sum.  Capture the compile-time value into a constexpr int and
    // record success.
    struct helper
    {
        static constexpr int compute()
        {
            mut_iter_4 a;
            a[0] = 1;
            a[1] = 2;
            a[2] = 3;
            a[3] = 4;
            return a[0] + a[1] + a[2] + a[3];
        }
    };
    constexpr int s = helper::compute();
    static_assert(s == 10,
        "constexpr mutator sum must equal 10 (1+2+3+4)");
    record_assertion(_handler, _out, true, "C++14+ relaxed-constexpr mutator path produces the expected sum");

#else
    // Pre-C++14: relaxed constexpr is unavailable.  Mark as
    // intentionally skipped so the test surface stays uniform.
    record_assertion(_handler, _out, true, "C++14+ relaxed-constexpr path skipped on pre-C++14 toolchains");
#endif
}


// ===========================================================================
// VIII. Iterator algorithm interop
// ===========================================================================

/*
test_array_constexpr_iterator_algorithms
  Verifies interop between array iterators and the
  constexpr_iterator algorithm family.
  Tests the following:
  - constexpr_find    locates a present value
  - constexpr_find    returns end() when the value is absent
  - constexpr_count_if reports the correct number of matches
  - constexpr_all_of  returns true when every element matches
  - constexpr_equal   returns true for two element-wise equal ranges
*/
void
test_array_constexpr_iterator_algorithms(
    std::vector<basic_test>& _out,
    test_handler&            _handler,
    test_type_id             _kind
)
{
    // Each algorithm runs on the array's begin/end pair.  Where
    // possible we lift the call into a constexpr context; at
    // minimum we exercise the runtime path.
    constexpr imm_iter_4 a(1, 2, 3, 4);

    // constexpr_find: locate the value 3.
    auto p = constexpr_find(a.begin(), a.end(), 3);
    bool found_ok = (p != a.end()) && (*p == 3);
    record_assertion(_handler, _out, found_ok, "constexpr_find locates a present value through array iterators");

    // constexpr_find for a missing value returns end.
    auto q = constexpr_find(a.begin(), a.end(), 999);
    bool not_found_ok = (q == a.end());
    record_assertion(_handler, _out, not_found_ok, "constexpr_find returns end() when the value is absent");

    // constexpr_count_if: count even elements.
    auto n_even = constexpr_count_if(
        a.begin(), a.end(),
        [](int v) { return (v % 2) == 0; });
    record_assertion(_handler, _out, n_even == 2, "constexpr_count_if reports the correct number of matches");

    // constexpr_all_of: every element > 0.
    bool all_pos = constexpr_all_of(
        a.begin(), a.end(),
        [](int v) { return v > 0; });
    record_assertion(_handler, _out, all_pos, "constexpr_all_of returns true when every element matches");

    // constexpr_equal: identical ranges.
    constexpr imm_iter_4 b(1, 2, 3, 4);
    bool eq = constexpr_equal(a.begin(), 
                              a.end(),
                              b.begin());
    record_assertion(_handler, _out, eq, "constexpr_equal returns true for two element-wise equal ranges");
}


// ===========================================================================
// IX.  Aggregate driver
// ===========================================================================

// ===========================================================================
// IX.  Aggregate driver
// ===========================================================================
//   Each top-level section is fronted by a `make_test_block`
// interior node so the printer can render numbered section
// headers between groups of leaves.  Inside each section, every
// test_array_* function is wrapped in `run_unit_test` so the
// suite-level unit-test count and pass/fail tally are advanced
// once per function, while assertion-level rows continue to flow
// through `record_assertion` per-leaf.

// begin_section
//   helper: appends an interior section-header node to the sink.
// `static` + explicit qualification keeps lookup unambiguous on
// MSVC without requiring an enclosing anonymous namespace.
static inline void
begin_section(
    std::vector<basic_test>& _out,
    const char*              _name)
{
    _out.push_back(test::make_test_block(_name));
    return;
}


std::vector<basic_test>
make_array_test_objects(
    test_handler&    _handler,
    unit_test_tally& _tally,
    test_type_id     _kind)
{
    std::vector<basic_test> out;
    out.reserve(192);

    // I.   Trait conformance
    begin_section(out, "Compile-time trait conformance");
    run_unit_test(_handler, out, _tally, "axis: constexpr / runtime",
        [&]{ test_array_axis_constexpr_runtime    (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "axis: mutable / immutable",
        [&]{ test_array_axis_mutable_immutable    (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "axis: iterable / non-iterable",
        [&]{ test_array_axis_iterable_non_iterable(out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "axis: bounded",
        [&]{ test_array_axis_bounded              (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "axis: sorted / unsorted",
        [&]{ test_array_axis_sorted_unsorted      (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "axis: flat / hierarchical",
        [&]{ test_array_axis_flat_hierarchical    (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "axis: storage kind",
        [&]{ test_array_axis_storage_kind         (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "lifetime taxonomy",
        [&]{ test_array_lifetime_taxonomy         (out, _handler, _kind); });

    // II.  Construction
    begin_section(out, "Construction and destruction");
    run_unit_test(_handler, out, _tally, "default construction",
        [&]{ test_array_default_construction      (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "parameter-pack construction",
        [&]{ test_array_pack_construction         (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "copy construction",
        [&]{ test_array_copy_construction         (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "move construction",
        [&]{ test_array_move_construction         (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "edge case: zero-extent array",
        [&]{ test_array_zero_extent_edge_case     (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "edge case: single-extent array",
        [&]{ test_array_single_extent_edge_case   (out, _handler, _kind); });

    // III. Element access
    begin_section(out, "Element access");
    run_unit_test(_handler, out, _tally, "subscript access",
        [&]{ test_array_subscript_access          (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "at() access",
        [&]{ test_array_at_access                 (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "front() / back() access",
        [&]{ test_array_front_back_access         (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "data() access",
        [&]{ test_array_data_access               (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "const-overload paths",
        [&]{ test_array_const_access_paths        (out, _handler, _kind); });

    // IV.  Iteration
    begin_section(out, "Iteration");
    run_unit_test(_handler, out, _tally, "begin() / end() round trip",
        [&]{ test_array_begin_end                 (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "cbegin() / cend() round trip",
        [&]{ test_array_const_iteration           (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "rbegin() / rend() round trip",
        [&]{ test_array_reverse_iteration         (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "range-based for traversal",
        [&]{ test_array_range_based_for           (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "non-iterable SFINAE contract",
        [&]{ test_array_non_iterable_sfinae       (out, _handler, _kind); });

    // V.   Mutation
    begin_section(out, "Mutation");
    run_unit_test(_handler, out, _tally, "subscript assignment",
        [&]{ test_array_subscript_assignment      (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "fill()",
        [&]{ test_array_fill                      (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "member swap()",
        [&]{ test_array_member_swap               (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "immutable SFINAE contract",
        [&]{ test_array_immutable_sfinae          (out, _handler, _kind); });

    // VI.  Bulk algorithms
    begin_section(out, "Free-function bulk algorithms");
    run_unit_test(_handler, out, _tally, "array_equal()",
        [&]{ test_array_equal_function            (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "array_copy()",
        [&]{ test_array_copy_function             (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "array_swap()",
        [&]{ test_array_swap_function             (out, _handler, _kind); });

    // VII. Constexpr usability
    begin_section(out, "Constexpr usability");
    run_unit_test(_handler, out, _tally, "constexpr construction",
        [&]{ test_array_constexpr_construction    (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "constexpr access",
        [&]{ test_array_constexpr_access          (out, _handler, _kind); });
    run_unit_test(_handler, out, _tally, "constexpr mutation (C++14+)",
        [&]{ test_array_constexpr_mutation_cpp14  (out, _handler, _kind); });

    // VIII. Iterator algorithm interop
    begin_section(out, "Iterator algorithm interop");
    run_unit_test(_handler, out, _tally, "constexpr_iterator algorithms",
        [&]{ test_array_constexpr_iterator_algorithms(out, _handler, _kind); });

    return out;
}


// ===========================================================================
// X.   Master-suite runner
// ===========================================================================

bool
run_array_suite(
    test_printer&    _printer,
    session_result*  _out_totals,
    unit_test_tally* _out_units,
    double*          _out_seconds)
{
    // Reset the printer's accumulator so any prior walk's counters
    // don't bleed into ours.
    _printer.reset_context();

    // Construct a session-scoped handler.  Every record_assertion
    // in array_core_tests.cpp goes through this handler, so its
    // session_result is the authoritative pass/fail tally for
    // assertions.  The unit_test_tally is incremented in
    // run_unit_test, so it's the authoritative tally for unit
    // tests.
    test_handler    handler;
    unit_test_tally tally;

    // Time the entire suite execution — from the first leaf
    // construction through the last printer flush.
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();

    // Build the suite — interior section headers + unit-test
    // wrappers + leaf assertions.
    std::vector<basic_test> tests =
        make_array_test_objects(handler, tally, 0);

    // Walk through the printer.  Five extraction lambdas map the
    // basic_test onto the printer's per-node schema; the depth
    // function returns 0 for section blocks, 1 for leaves.
    _printer.walk(
        tests,
        // name
        [](const basic_test& _t) -> std::string
        {
            return _t.name() ? std::string(_t.name())
                             : std::string("<unnamed>");
        },
        // message
        [](const basic_test& _t) -> std::string
        {
            return _t.message() ? std::string(_t.message())
                                : std::string();
        },
        // depth
        [](const basic_test& _t) -> std::size_t
        {
            return (_t.type_id() == D_TEST_KIND_TEST_BLOCK)
                       ? 0u
                       : 1u;
        },
        // status
        [](const basic_test& _t) -> test_status
        {
            using s = test_status;

            if (_t.passed())                                return s::passed;
            if (_t.failed())                                return s::failed;
            if (_t.status() == basic_test::status_skipped)  return s::skipped;
            if (_t.status() == basic_test::status_error)    return s::error;
            return s::pending;
        },
        // is_leaf
        [](const basic_test& _t) -> bool
        {
            // Section headers (test_block) are interior;
            // unit-test rollups (test_fn) and assertions (assert)
            // are leaves.
            return (_t.type_id() != D_TEST_KIND_TEST_BLOCK);
        },
        /*_with_header  =*/ false,
        /*_with_summary =*/ true,
        /*_with_footer  =*/ false);

    const auto t1 = clock::now();
    const double seconds =
        std::chrono::duration<double>(t1 - t0).count();

    // Forward authoritative tallies to the caller.
    if (_out_totals)  { *(_out_totals)  = handler.result(); }
    if (_out_units)   { *(_out_units)   = tally;            }
    if (_out_seconds) { *(_out_seconds) = seconds;          }

    const session_result& r = handler.result();
    return ( (r.failed == 0) &&
             (r.errors == 0) &&
             (r.total > 0) );
}


NS_END  // testing
NS_END  // djinterp
