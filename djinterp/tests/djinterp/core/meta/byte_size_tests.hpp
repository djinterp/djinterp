/******************************************************************************
* djinterp [test]                                           byte_size_tests.hpp
*
*   Unit-test suite for the container byte-footprint module `byte_size.hpp`.
* Tests are split across translation units, one per semantic section of the
* header:
*
*     I.   static_byte_size (+ _v)        -> byte_size_tests_static.cpp
*     II.  storage-shape signals          -> byte_size_tests_signals.cpp
*     III. shape + own dynamic footprint  -> byte_size_tests_shape.cpp
*     IV.  recursive dynamic descent      -> byte_size_tests_descent.cpp
*     V.   public footprint functions     -> byte_size_tests_public.cpp
*
*   STRUCTURE:
*   `static_byte_size` and the storage/shape signals are compile-time, so they
* are pinned with file-scope `static_assert`s AND mirrored at run time.  The
* footprint FUNCTIONS (dynamic_byte_size / total_byte_size) read capacity() /
* size() and walk the container, so they are not constexpr; those checks live
* only in the run-time predicates and compare the module's output against an
* expectation computed from the SAME public surface (capacity(), size(),
* iterator distance, sizeof) -- exact by construction and portable across
* library implementations.
*
*   API (matches AGENTS-cmake.md):
*   Every entry point is a nullary `bool tests_byte_size_*()`, FLAT in
* `djinterp::testing`, so the runner drives it as
* `&::djinterp::testing::tests_byte_size_*` through the D_BZ_RUN harness.
* Shared fixtures live in the nested `byte_size_test_types` namespace.
*
*   COVERAGE INTENT (100%):
*     - static_byte_size: sizeof(clean_t<>), the cv/ref stripping, and _v;
*     - every storage-signal detector (capacity / size / allocator_type /
*       reserve / c_str) at true AND false, the dynamically-sited disjunction
*       (allocator-only, reserve-only, both, neither), and the recurse-into-
*       element conjunction (nested container / scalar / c_str leaf);
*     - byte_shape_of at all three shapes, including every way the contiguous
*       conjunction can fail (not sited / no data() / no capacity());
*     - byte_own_dynamic's three overloads and byte_node_count's two
*       (size() vs iterator distance);
*     - byte_dynamic_helper's primary (leaf element) and recursing (nested
*       container element) specializations, incl. the c_str leaf frontier and
*       recursion through static-sited elements (which contribute zero).
*
*
* path:      /inc/djinterp/test/byte_size_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

#ifndef DJINTERP_BYTE_SIZE_TESTS_
#define DJINTERP_BYTE_SIZE_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "byte_size.hpp"   // system under test


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED FIXTURES                                     ///
///////////////////////////////////////////////////////////////////////////////
//   Minimal hand-built types that isolate exactly one storage signal each, so
// the internal detectors and the byte_shape_of conjunction can be driven down
// every branch without leaning on a library container's incidental surface.
// Nested so the flat tests_byte_size_* surface stays clean.

namespace byte_size_test_types
{

// ---- single-signal probes (one detector each) ----
struct has_capacity_only { std::size_t capacity() const { return 7; } };
struct has_size_only     { std::size_t size()     const { return 3; } };
struct has_allocator_only{ using allocator_type = std::allocator<int>; };
struct has_reserve_only  { void reserve(std::size_t) {} };
struct has_c_str_only    { const char* c_str() const { return ""; } };
struct has_nothing       { int _pad; };

// ---- dynamically-sited disjunction drivers (allocator OR reserve) ----
//   alloc-only and reserve-only isolate each side of the ||; sited_both and
// has_nothing give the true/true and false/false corners.
struct sited_alloc_only   { using allocator_type = std::allocator<int>; };
struct sited_reserve_only { void reserve(std::size_t) {} };
struct sited_both         { using allocator_type = std::allocator<int>;
                            void reserve(std::size_t) {} };
// (has_nothing is the "sited by neither" corner.)

// ---- byte_shape_of conjunction drivers (no value_type needed) ----
// dynamic + data() but NO capacity()  -> must fall through to node_dynamic.
struct dyn_data_no_capacity
{
    using allocator_type = std::allocator<int>;
    const int* data() const { return nullptr; }
};
// dynamic via RESERVE (not allocator) + data() + capacity() -> contiguous.
struct dyn_via_reserve_contig
{
    void reserve(std::size_t) {}
    const int* data()     const { return nullptr; }
    std::size_t capacity() const { return 4; }
};

// ---- a minimal STATIC-sited but ITERABLE container ----
//   Has begin()/end() and value_type (so it is an iterable container the
// descent will enter), but NO allocator_type and NO reserve() -> static-sited,
// hence zero dynamic bytes even as a nested element.
template<typename _Type, std::size_t _Count>
struct inline_bag
{
    using value_type = _Type;

    _Type _cells[_Count];

    const _Type* begin() const { return _cells; }
    const _Type* end()   const { return _cells + _Count; }
};

// ---- an element type with a large sizeof, for footprint arithmetic ----
struct wide_cell { char _bytes[32]; };

}  // namespace byte_size_test_types


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-SECTION TEST DECLARATIONS                       ///
///////////////////////////////////////////////////////////////////////////////
//   Nullary predicates, flat in djinterp::testing; each returns true iff its
// section's run-time re-checks pass (compile-time enforcement is the
// file-scope static_assert wall in each .cpp).

// ---- I.  static_byte_size ----
bool tests_byte_size_static_exact();
bool tests_byte_size_static_cleans();
bool tests_byte_size_static_v();

// ---- II. storage-shape signals ----
bool tests_byte_size_signal_capacity();
bool tests_byte_size_signal_size();
bool tests_byte_size_signal_allocator();
bool tests_byte_size_signal_reserve();
bool tests_byte_size_signal_c_str();
bool tests_byte_size_signal_dynamically_sited();
bool tests_byte_size_signal_recurse_into_element();
bool tests_byte_size_signal_cleans();

// ---- III. shape + own dynamic footprint ----
bool tests_byte_size_shape_of();
bool tests_byte_size_own_contiguous();
bool tests_byte_size_own_node_size();
bool tests_byte_size_own_node_distance();
bool tests_byte_size_own_static();

// ---- IV. recursive dynamic descent ----
bool tests_byte_size_descent_leaf_element();
bool tests_byte_size_descent_nested();
bool tests_byte_size_descent_c_str_frontier();
bool tests_byte_size_descent_static_elements();
bool tests_byte_size_descent_mixed();
bool tests_byte_size_descent_empty();

// ---- V. public footprint functions ----
bool tests_byte_size_total_identity();
bool tests_byte_size_total_static_sited();
bool tests_byte_size_total_const_input();
bool tests_byte_size_integration();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_BYTE_SIZE_TESTS_
