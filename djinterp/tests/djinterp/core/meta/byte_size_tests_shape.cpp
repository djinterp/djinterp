/******************************************************************************
* djinterp [test]                                     byte_size_tests_shape.cpp
*
*   Section III of byte_size.hpp: byte_shape_of, byte_own_dynamic (its three
* overloads) and byte_node_count (its two).  Exercised at ONE container level
* (no nesting -- that is section IV).
*
*   byte_shape_of is constexpr, so its classification is pinned with
* static_assert across all three shapes AND every way the contiguous
* conjunction (sited && data() && capacity()) can fail: not sited -> static;
* sited but no data() -> node; sited with data() but no capacity() -> node.
*
*   The own-footprint arithmetic is run-time (it reads capacity()/size()/
* distance), so those checks compare dynamic_byte_size against an expectation
* built from the same surface:
*     contiguous     -> capacity() * sizeof(cell)
*     node (size)    -> size()     * sizeof(cell)
*     node (no size) -> distance   * sizeof(cell)   (forward_list)
*     static         -> 0
*
*
* path:      /inc/djinterp/test/byte_size_tests_shape.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

#include "byte_size_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace byte_size_test_types;

namespace bi = ::djinterp::internal;


// =========================================================================
// I.   byte_shape_of  (all three shapes; every conjunction failure)
// =========================================================================

// contiguous: sited && data() && capacity()
static_assert(bi::byte_shape_of<std::vector<int>>() == bi::byte_shape::contiguous_dynamic,
              "shape: vector -> contiguous");
static_assert(bi::byte_shape_of<std::string>() == bi::byte_shape::contiguous_dynamic,
              "shape: string -> contiguous");
static_assert(bi::byte_shape_of<dyn_via_reserve_contig>() == bi::byte_shape::contiguous_dynamic,
              "shape: reserve-driven + data + capacity -> contiguous");

// node: sited but the contiguous conjunction fails
static_assert(bi::byte_shape_of<std::list<int>>() == bi::byte_shape::node_dynamic,
              "shape: list (no data) -> node");
static_assert(bi::byte_shape_of<std::deque<int>>() == bi::byte_shape::node_dynamic,
              "shape: deque (no data) -> node");
static_assert(bi::byte_shape_of<std::forward_list<int>>() == bi::byte_shape::node_dynamic,
              "shape: forward_list (no data) -> node");
static_assert(bi::byte_shape_of<dyn_data_no_capacity>() == bi::byte_shape::node_dynamic,
              "shape: sited + data but NO capacity -> node");
static_assert(bi::byte_shape_of<sited_alloc_only>() == bi::byte_shape::node_dynamic,
              "shape: sited, no data, no capacity -> node");

// static: not sited (regardless of data()/capacity())
static_assert(bi::byte_shape_of<std::array<int, 4>>() == bi::byte_shape::static_sited,
              "shape: array -> static");
static_assert(bi::byte_shape_of<inline_bag<int, 3>>() == bi::byte_shape::static_sited,
              "shape: inline_bag -> static");
static_assert(bi::byte_shape_of<has_nothing>() == bi::byte_shape::static_sited,
              "shape: no signals -> static");


// =========================================================================
// II.  RUN-TIME MIRRORS
// =========================================================================

bool
tests_byte_size_shape_of()
{
    bool _ok = true;

    _ok = _ok && (bi::byte_shape_of<std::vector<int>>()       == bi::byte_shape::contiguous_dynamic);
    _ok = _ok && (bi::byte_shape_of<std::string>()            == bi::byte_shape::contiguous_dynamic);
    _ok = _ok && (bi::byte_shape_of<dyn_via_reserve_contig>() == bi::byte_shape::contiguous_dynamic);
    _ok = _ok && (bi::byte_shape_of<std::list<int>>()         == bi::byte_shape::node_dynamic);
    _ok = _ok && (bi::byte_shape_of<std::forward_list<int>>() == bi::byte_shape::node_dynamic);
    _ok = _ok && (bi::byte_shape_of<dyn_data_no_capacity>()   == bi::byte_shape::node_dynamic);
    _ok = _ok && (bi::byte_shape_of<sited_alloc_only>()       == bi::byte_shape::node_dynamic);
    _ok = _ok && (bi::byte_shape_of<std::array<int, 4>>()     == bi::byte_shape::static_sited);
    _ok = _ok && (bi::byte_shape_of<inline_bag<int, 3>>()     == bi::byte_shape::static_sited);
    _ok = _ok && (bi::byte_shape_of<has_nothing>()            == bi::byte_shape::static_sited);

    return _ok;
}

bool
tests_byte_size_own_contiguous()
{
    bool _ok = true;

    // capacity() * sizeof(cell), across cell sizes
    std::vector<int> vi{1, 2, 3, 4, 5};
    _ok = _ok && (dynamic_byte_size(vi) == vi.capacity() * sizeof(int));

    std::vector<char> vc(10, 'x');
    _ok = _ok && (dynamic_byte_size(vc) == vc.capacity() * sizeof(char));

    std::vector<double> vd{1.0, 2.0, 3.0};
    _ok = _ok && (dynamic_byte_size(vd) == vd.capacity() * sizeof(double));

    std::vector<wide_cell> vw(4);
    _ok = _ok && (dynamic_byte_size(vw) == vw.capacity() * sizeof(wide_cell));

    // empty and single-element edges
    std::vector<int> empty_v;
    _ok = _ok && (dynamic_byte_size(empty_v) == empty_v.capacity() * sizeof(int));

    std::vector<int> one{42};
    _ok = _ok && (dynamic_byte_size(one) == one.capacity() * sizeof(int));

    // capacity() > size(): a contiguous store is sized by its ALLOCATION
    // (capacity), NOT its element count -- pin that distinction explicitly.
    std::vector<int> reserved;
    reserved.reserve(64);
    reserved.push_back(1);
    reserved.push_back(2);
    _ok = _ok && (reserved.capacity() > reserved.size());        // precondition
    _ok = _ok && (dynamic_byte_size(reserved) == reserved.capacity() * sizeof(int));
    _ok = _ok && (dynamic_byte_size(reserved) != reserved.size() * sizeof(int));

    return _ok;
}

bool
tests_byte_size_own_node_size()
{
    bool _ok = true;

    // size() * sizeof(cell)
    std::list<int> l{1, 2, 3};
    _ok = _ok && (dynamic_byte_size(l) == l.size() * sizeof(int));

    std::deque<int> d{1, 2, 3, 4, 5};
    _ok = _ok && (dynamic_byte_size(d) == d.size() * sizeof(int));

    // empty node store -> 0
    std::list<int> empty_l;
    _ok = _ok && (dynamic_byte_size(empty_l) == 0u);

    return _ok;
}

bool
tests_byte_size_own_node_distance()
{
    bool _ok = true;

    // forward_list has no size(): counted by iterator distance
    std::forward_list<int> fl{1, 2, 3, 4};
    _ok = _ok && (dynamic_byte_size(fl) == 4u * sizeof(int));

    std::forward_list<int> one{7};
    _ok = _ok && (dynamic_byte_size(one) == 1u * sizeof(int));

    std::forward_list<int> empty_fl;
    _ok = _ok && (dynamic_byte_size(empty_fl) == 0u);

    return _ok;
}

bool
tests_byte_size_own_static()
{
    bool _ok = true;

    // static-sited: no heap region
    std::array<int, 6> a{};
    _ok = _ok && (dynamic_byte_size(a) == 0u);

    std::array<wide_cell, 3> aw{};
    _ok = _ok && (dynamic_byte_size(aw) == 0u);

    inline_bag<int, 4> bag{};
    _ok = _ok && (dynamic_byte_size(bag) == 0u);

    return _ok;
}


NS_END  // testing
NS_END  // djinterp
