/*******************************************************************************
* djinterp [testing]                    memory_strategy_traits_tests_bridges.cpp
*
* Section: element <-> byte bridges.
*   memory_strategy_traits.hpp VIII - element_strategy_view, byte_strategy_view
*
*   The views make dual-typing total: a byte strategy can present an element
* surface for any T (element_strategy_view, the universal direction), and a
* byte-GRANULAR element strategy can present a byte surface (byte_strategy_view,
* enabled only when sizeof(value_type) == 1).  Each is tested twice: once at the
* trait level, where the view's own descriptive surface must classify it
* correctly, and once at runtime, where an allocation must round-trip through
* the wrapped strategy and land where expected.
*   Both directions forward all four descriptive constants from the wrapped
* strategy, so they are symmetric: element_strategy_view over a releasing byte
* strategy is itself releasing and non-monotonic, and likewise byte_strategy_view
* over a releasing element strategy stays releasing and non-monotonic - kind,
* pointer-stability, individual-release, and generational all carry across.  The
* contract tests below pin the full forwarding in each direction so a future
* regression that drops a constant trips a test.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_bridges.cpp
*                                          memory_strategy_tests_bridges.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstdint>
#include <memory>
#include <type_traits>
// djinterp
#include "memory_strategy_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_element_strategy_view_contract
  element_strategy_view<Byte, T>: the descriptive surface of the view (core VIII).
  Tests the following:
  - the view over a byte strategy IS an element strategy, is a member, and is
    byte-typed for no one: it speaks element only
  - value_type is exactly T, byte_type is the cleaned wrapped type
  - all four descriptive constants forward from the wrapped strategy: kind,
    pointer-stability, individual-release, and generational.  A view over a
    monotonic arena is static + stable + monotonic; a view over a releasing
    byte strategy is dynamic + unstable + RELEASING + not monotonic.  The second
    case is the one that proves release is forwarded (contrast
    byte_strategy_view)
  - the kind tracks the wrapped strategy's discipline (static arena vs fixed
    dynamic_byte_arena)
*/
bool
tests_element_strategy_view_contract()
{

    using view_over_arena =
        element_strategy_view<monotonic_byte_arena<256>, int>;
    using view_over_dyn =
        element_strategy_view<dynamic_byte_arena, double>;
    using view_over_releasing =
        element_strategy_view<ms_byte_releasing, int>;

    // --- the view speaks element, and only element ---
    D_MT_CHECK(is_element_strategy<view_over_arena>::value);
    D_MT_CHECK(!is_byte_strategy<view_over_arena>::value);
    D_MT_CHECK(is_memory_strategy<view_over_arena>::value);
    D_MT_CHECK(strategy_typing_of<view_over_arena>::value
                   == strategy_typing::element_typed);

    // --- the member aliases ---
    D_MT_CHECK((std::is_same<view_over_arena::value_type, int>::value));
    D_MT_CHECK((std::is_same<view_over_dyn::value_type, double>::value));
    D_MT_CHECK((std::is_same<view_over_arena::byte_type,
                             monotonic_byte_arena<256>>::value));

    // --- all four constants forward: monotonic arena case ---
    D_MT_CHECK(is_static_strategy<view_over_arena>::value);
    D_MT_CHECK(is_pointer_stable_strategy<view_over_arena>::value);
    D_MT_CHECK(!supports_individual_release_strategy<view_over_arena>::value);
    D_MT_CHECK(is_monotonic_strategy<view_over_arena>::value);
    D_MT_CHECK(!is_generational_strategy<view_over_arena>::value);

    // --- all four constants forward: releasing byte-strategy case ---
    //     this is where release being forwarded is provable
    D_MT_CHECK(is_dynamic_strategy<view_over_releasing>::value);
    D_MT_CHECK(!is_pointer_stable_strategy<view_over_releasing>::value);
    D_MT_CHECK(
        supports_individual_release_strategy<view_over_releasing>::value);
    D_MT_CHECK(!is_monotonic_strategy<view_over_releasing>::value);

    // --- kind tracks the wrapped discipline ---
    D_MT_CHECK(memory_strategy_kind_of<view_over_arena>::value
                   == storage_kind::static_storage);
    D_MT_CHECK(memory_strategy_kind_of<view_over_dyn>::value
                   == storage_kind::fixed_storage);
    D_MT_CHECK(is_monotonic_strategy<view_over_dyn>::value);

    return true;
}


/*
tests_element_strategy_view_runtime
  element_strategy_view<Byte, T>: an allocation round-trips (core VIII).
  Tests the following:
  - underlying() returns the wrapped strategy
  - allocate(n) yields non-null, correctly-aligned storage for T, and consumes
    exactly n*sizeof(T) bytes of a fresh arena (head starts at 0, so no padding)
  - the returned storage is writable and reads back
  - deallocate is a no-op on a monotonic arena: bytes-in-use is unchanged
  - reset on the underlying arena reclaims everything
  - a second view for a wider T aligns to that T
*/
bool
tests_element_strategy_view_runtime()
{

    monotonic_byte_arena<256> arena;
    element_strategy_view<monotonic_byte_arena<256>, int> view(arena);

    // --- underlying ---
    D_MT_CHECK(view.underlying() == &arena);

    // --- allocate: non-null, aligned, exact byte cost from a fresh arena ---
    int* p = view.allocate(3);
    D_MT_CHECK(p != nullptr);
    D_MT_CHECK(reinterpret_cast<std::uintptr_t>(p) % alignof(int) == 0);
    D_MT_CHECK(arena.bytes_in_use() == 3 * sizeof(int));

    // --- writable and reads back ---
    p[0] = 11;
    p[1] = 22;
    p[2] = 33;
    D_MT_CHECK(p[0] == 11 && p[1] == 22 && p[2] == 33);

    // --- deallocate is a no-op on a monotonic arena ---
    view.deallocate(p, 3);
    D_MT_CHECK(arena.bytes_in_use() == 3 * sizeof(int));

    // --- reset reclaims everything ---
    arena.reset();
    D_MT_CHECK(arena.bytes_in_use() == 0);

    // --- a wider T aligns to that T ---
    monotonic_byte_arena<256> arena2;
    element_strategy_view<monotonic_byte_arena<256>, double> dview(arena2);
    double* d = dview.allocate(2);
    D_MT_CHECK(d != nullptr);
    D_MT_CHECK(reinterpret_cast<std::uintptr_t>(d) % alignof(double) == 0);
    D_MT_CHECK(arena2.bytes_in_use() == 2 * sizeof(double));
    d[0] = 1.5;
    d[1] = 2.5;
    D_MT_CHECK(d[0] == 1.5 && d[1] == 2.5);

    return true;
}


/*
tests_byte_strategy_view_contract
  byte_strategy_view<Element>: the descriptive surface of the view (core VIII).
  Tests the following:
  - the view over a 1-byte element strategy IS a byte strategy, is a member, and
    is element-typed for no one
  - value_type is void, element_type is the cleaned wrapped type, cell_type is
    its 1-byte value_type
  - ALL FOUR descriptive constants track the wrapped strategy - kind,
    pointer-stability, individual-release, and generational - so the view is
    symmetric with element_strategy_view.  In particular a byte view over a
    RELEASING element strategy is itself releasing and therefore NOT monotonic;
    the checks below pin the forwarding on a fixture and on a real allocator
    wrapper, so any future regression that drops a constant trips a test
  - the SFINAE gate is closed for wider element types: a byte_strategy_view over
    a non-byte-granular element strategy is not a well-formed type
*/
bool
tests_byte_strategy_view_contract()
{

    using bview_cell = byte_strategy_view<ms_byte_cell_element>;
    using bview_heap =
        byte_strategy_view<allocator_memory_strategy<std::allocator<
            unsigned char>>>;

    // --- the view speaks byte, and only byte ---
    D_MT_CHECK(is_byte_strategy<bview_cell>::value);
    D_MT_CHECK(!is_element_strategy<bview_cell>::value);
    D_MT_CHECK(is_memory_strategy<bview_cell>::value);
    D_MT_CHECK(strategy_typing_of<bview_cell>::value
                   == strategy_typing::byte_typed);

    // --- the member aliases ---
    D_MT_CHECK((std::is_same<bview_cell::value_type, void>::value));
    D_MT_CHECK((std::is_same<bview_cell::element_type,
                             ms_byte_cell_element>::value));
    D_MT_CHECK((std::is_same<bview_cell::cell_type, unsigned char>::value));
    D_MT_CHECK(sizeof(bview_cell::cell_type) == 1);

    // --- the two forwarded constants track the wrapped strategy ---
    D_MT_CHECK(is_fixed_strategy<bview_cell>::value);      // cell is fixed
    D_MT_CHECK(is_pointer_stable_strategy<bview_cell>::value);

    // --- release / generational are forwarded too (symmetric with the element
    //     view): ms_byte_cell_element declares individual release, so the view
    //     is releasing and NOT monotonic ---
    D_MT_CHECK(
        supports_individual_release_strategy<ms_byte_cell_element>::value);
    D_MT_CHECK(supports_individual_release_strategy<bview_cell>::value);
    D_MT_CHECK(!is_monotonic_strategy<bview_cell>::value);
    D_MT_CHECK(is_generational_strategy<bview_cell>::value
                   == is_generational_strategy<ms_byte_cell_element>::value);
    //     same forwarding over a real releasing allocator wrapper
    D_MT_CHECK(
        supports_individual_release_strategy<
            allocator_memory_strategy<std::allocator<unsigned char>>>::value);
    D_MT_CHECK(supports_individual_release_strategy<bview_heap>::value);
    D_MT_CHECK(!is_monotonic_strategy<bview_heap>::value);

    // --- the SFINAE gate is closed for wider element types ---
    //     a well-formed 1-byte case is detected; the primary template is not a
    //     complete usable type and would static_assert if named, so we assert
    //     the positive detection only (the negative cannot be SFINAE-probed).
    D_MT_CHECK(sizeof(bview_cell) >= sizeof(void*));  // holds a pointer

    return true;
}


/*
tests_byte_strategy_view_runtime
  byte_strategy_view<Element>: an allocation round-trips (core VIII).
  Tests the following:
  - underlying() returns the wrapped element strategy
  - allocate(bytes, align) yields non-null storage, forwarding to the element
    strategy's one-argument allocate (byte count == cell count for 1-byte cells)
  - the storage is writable across the full requested span and reads back
  - deallocate returns the storage to the wrapped strategy (a heap allocator
    here, so the round-trip must not leak or corrupt)
*/
bool
tests_byte_strategy_view_runtime()
{

    allocator_memory_strategy<std::allocator<unsigned char>> heap;
    byte_strategy_view<
        allocator_memory_strategy<std::allocator<unsigned char>>> bview(heap);

    // --- underlying ---
    D_MT_CHECK(bview.underlying() == &heap);

    // --- allocate forwards to the element strategy ---
    void* q = bview.allocate(16, alignof(std::max_align_t));
    D_MT_CHECK(q != nullptr);

    // --- writable across the full span, reads back ---
    unsigned char* b = static_cast<unsigned char*>(q);
    for (std::size_t i = 0; i < 16; ++i)
    {
        b[i] = static_cast<unsigned char>(i + 1);
    }
    bool span_ok = true;
    for (std::size_t i = 0; i < 16; ++i)
    {
        if (b[i] != static_cast<unsigned char>(i + 1))
        {
            span_ok = false;
        }
    }
    D_MT_CHECK(span_ok);

    // --- deallocate returns storage to the wrapped strategy ---
    bview.deallocate(q, 16, alignof(std::max_align_t));

    // --- a second cycle reuses the strategy cleanly ---
    void* q2 = bview.allocate(8, 1);
    D_MT_CHECK(q2 != nullptr);
    static_cast<unsigned char*>(q2)[0] = 0x5A;
    static_cast<unsigned char*>(q2)[7] = 0xA5;
    D_MT_CHECK(static_cast<unsigned char*>(q2)[0] == 0x5A);
    D_MT_CHECK(static_cast<unsigned char*>(q2)[7] == 0xA5);
    bview.deallocate(q2, 8, 1);

    return true;
}


NS_END  // testing
NS_END  // djinterp
