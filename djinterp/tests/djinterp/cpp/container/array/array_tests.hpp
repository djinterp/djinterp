/*******************************************************************************
* djinterp [cpp][container]                                      array_tests.hpp
*
* Unit test suite for the array family under container/array/.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the headers under test and
*       exposes the fixtures and the D_AY_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       array_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.
*     array.hpp           array<Options...>: six lifetime x iterability
*                         cells, the element-list constructor, gated
*                         accessors and iteration, the D* aliases,
*                         make_array / make_immutable_array, and
*                         array_equal / array_copy / array_swap.
*     array_options.hpp   array_opt_type, DArrayOptExtent,
*                         array_from_options_t, make_array_from_options.
*     array_traits.hpp    the classification traits, on every cell, on
*                         std::array, std::vector, std::string, std::span,
*                         C arrays and the four wrappers.
*     array_concepts.hpp  the concepts over those traits (C++20).
*     array_iterator.hpp  DChunkRef, DCircularIterator, DChunkIterator,
*                         DWindowIterator and their factories.
*     atomic_array.hpp, cow_array.hpp, rcu_array.hpp, threadsafe_array.hpp
*                         the four concurrent wrappers, each including a
*                         multi-threaded check of its guarantee.
*   array_option_view.hpp is not covered here; it gets its own suite.
*
*   DEFECTS THIS SUITE FOUND (fixed with it, 2026.11.14):
*     - rcu_array could not be instantiated: it called DRcuProtected
*       members that don't exist (read(fn), update(fn), publish,
*       snapshot) and had no default constructor;
*     - make_window_iterator yielded partial windows when the range was
*       shorter than the window;
*     - DThreadsafeArray's size_lockfree() read 0 until assign(); its
*       forwarding constructor took a non-const DThreadsafeArray lvalue
*       away from the copy constructor (a compile error); and optimistic()
*       returned torn arrays under a concurrent writer;
*     - DCowArray defaulted to null_lock_policy: two threads, its
*       purpose, corrupted the heap;
*     - array<>'s element-list constructor was unconstrained, so
*       std::is_constructible said yes to too many or unconvertible
*       values, and it was noexcept even when a conversion could throw;
*     - is_dynamic_capacity was true for array<> (it has capacity());
*       has_static_extent was true for std::span<T> and std::pair; int[4]
*       read immutable; a non-iterable array<> read non-contiguous; an
*       immutable array read shiftable; a wrapper's specializations
*       didn't answer for `const W&`.
*
*   THE FIXTURE VOCABULARY (normal mode only)
*     mi4, mni4, ii4, ini4, ci4, cni4
*                     the six cells of array<int> with extent 4:
*                     {mutable, immutable, constexpr} x
*                     {iterable, non-iterable}.
*     mi0             a mutable iterable array<int> with extent 0.
*     ms3             a mutable iterable array<std::string> with extent 3.
*     atomic4, cow4, rcu4, ts4
*                     the wrappers over array_opt_type<int>, extent 4.
*     ay_throws       a value type whose converting constructor may throw.
*     ay_*_t          expression aliases for is_detected, to ask whether a
*                     gated member exists on a cell.
*
* CONTENTS
*   I.    CONSTRUCTION   (array_tests_construction.cpp)
*   II.   ACCESS         (array_tests_access.cpp)
*   III.  ITERATION      (array_tests_iteration.cpp)
*   IV.   ALGORITHMS     (array_tests_algorithms.cpp)
*   V.    TRAITS         (array_tests_traits.cpp)
*   VI.   ITERATORS      (array_tests_iterators.cpp)
*   VII.  ATOMIC         (array_tests_atomic.cpp)
*   VIII. COW            (array_tests_cow.cpp)
*   IX.   RCU            (array_tests_rcu.cpp)
*   X.    THREADSAFE     (array_tests_threadsafe.cpp)
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

#ifndef DJINTERP_TESTS_ARRAY_TESTS_
#define DJINTERP_TESTS_ARRAY_TESTS_ 1

// std
#include <cstddef>      // std::size_t
#include <cstdio>       // std::printf
#include <string>       // std::string
#include <type_traits>  // std::is_same
#include <utility>      // std::declval
// djinterp
// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/container/array/array.hpp>             // under test
#include <djinterp/cpp/container/array/array_concepts.hpp>    // under test
#include <djinterp/cpp/container/array/array_iterator.hpp>    // under test
#include <djinterp/cpp/container/array/array_options.hpp>     // under test
#include <djinterp/cpp/container/array/array_traits.hpp>      // under test
#include <djinterp/cpp/container/array/atomic_array.hpp>      // under test
#include <djinterp/cpp/container/array/cow_array.hpp>         // under test
#include <djinterp/cpp/container/array/rcu_array.hpp>         // under test
#include <djinterp/cpp/container/array/threadsafe_array.hpp>  // under test
#include <djinterp/cpp/meta/type_traits.hpp>                  // is_detected
#endif
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec (spec)
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// array_check
//   function: routes one D_AY_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.
inline
bool array_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_AY_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  `AY` (arraY) is this
// suite's unique two letters.
#define D_AY_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::array_check(                                \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   THE SIX CELLS                                         ///
///////////////////////////////////////////////////////////////////////////////

using mi4  = DMutableIterableArray<int, 4>;
using mni4 = DMutableNonIterableArray<int, 4>;
using ii4  = DImmutableIterableArray<int, 4>;
using ini4 = DImmutableNonIterableArray<int, 4>;
using ci4  = DConstexprIterableArray<int, 4>;
using cni4 = DConstexprNonIterableArray<int, 4>;
using mi0  = DMutableIterableArray<int, 0>;
using ms3  = DMutableIterableArray<std::string, 3>;


///////////////////////////////////////////////////////////////////////////////
///             F.2   THE WRAPPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

using atomic4 = DAtomicArray<DArrayOptType<int>, DArrayOptExtent<4>>;
using cow4    = DCowArray<DArrayOptType<int>, DArrayOptExtent<4>>;
using rcu4    = DRcuArray<DArrayOptType<int>, DArrayOptExtent<4>>;
using ts4     = DThreadsafeArray<DArrayOptType<int>, DArrayOptExtent<4>>;

// the non-iterable wrappers, to check that the axis carries through
using cow4_ni = DCowArray<
    DArrayOptType<int>,
    DArrayOptExtent<4>,
    DContainerOptIterability<DContainerIterability::non_iterable>>;
using ts4_ni = DThreadsafeArray<
    DArrayOptType<int>,
    DArrayOptExtent<4>,
    DContainerOptIterability<DContainerIterability::non_iterable>>;


///////////////////////////////////////////////////////////////////////////////
///             F.3   VALUE TYPES                                           ///
///////////////////////////////////////////////////////////////////////////////

// ay_throws
//   struct: converts from int through a constructor that may throw, so the
// element-list constructor's noexcept follows the conversion.
struct ay_throws
{
    int value = 0;

    ay_throws() noexcept = default;

    ay_throws(int _v) noexcept(false)  // NOLINT: implicit on purpose
        : value(_v)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///             F.4   DETECTION ALIASES                                     ///
///////////////////////////////////////////////////////////////////////////////

// ay_mut_index_t
//   alias: assignment through a non-const subscript.
template<typename Type>
using ay_mut_index_t = decltype(
    std::declval<Type&>()[0] = std::declval<typename Type::value_type>());

// ay_mut_at_t
//   alias: assignment through a non-const at().
template<typename Type>
using ay_mut_at_t = decltype(
    std::declval<Type&>().at(0) = std::declval<typename Type::value_type>());

// ay_begin_t / ay_cbegin_t / ay_rbegin_t
//   alias: begin(), cbegin() and rbegin() on a non-const object.
template<typename Type>
using ay_begin_t = decltype(std::declval<Type&>().begin());

template<typename Type>
using ay_cbegin_t = decltype(std::declval<Type&>().cbegin());

template<typename Type>
using ay_rbegin_t = decltype(std::declval<Type&>().rbegin());

// ay_fill_t / ay_swap_t
//   alias: the mutating fill() and swap() members.
template<typename Type>
using ay_fill_t = decltype(
    std::declval<Type&>().fill(std::declval<typename Type::value_type>()));

template<typename Type>
using ay_swap_t = decltype(std::declval<Type&>().swap(std::declval<Type&>()));

// ay_array_copy_t / ay_array_swap_t
//   alias: the free array_copy(Src, Dst) and array_swap(A, B).
template<typename Src,
         typename Dst>
using ay_array_copy_t = decltype(
    array_copy(std::declval<const Src&>(), std::declval<Dst&>()));

template<typename A,
         typename B>
using ay_array_swap_t = decltype(
    array_swap(std::declval<A&>(), std::declval<B&>()));

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.    CONSTRUCTION   (array_tests_construction.cpp)
bool tests_construction_default_value_initializes();
bool tests_construction_element_list();
bool tests_construction_is_constrained();
bool tests_construction_noexcept_follows_conversion();
bool tests_construction_copy_and_move();
bool tests_construction_make_array();
bool tests_construction_from_options();
bool tests_construction_cell_constants();
bool tests_construction_is_constexpr();

// II.   ACCESS         (array_tests_access.cpp)
bool tests_access_subscript_and_at();
bool tests_access_front_back_data();
bool tests_access_size_queries();
bool tests_access_immutable_is_read_only();
bool tests_access_constexpr_cell_is_writable();

// III.  ITERATION      (array_tests_iteration.cpp)
bool tests_iteration_begin_end();
bool tests_iteration_const_and_reverse();
bool tests_iteration_std_algorithms();
bool tests_iteration_is_gated();
bool tests_iteration_empty_array();

// IV.   ALGORITHMS     (array_tests_algorithms.cpp)
bool tests_algorithms_fill();
bool tests_algorithms_member_swap();
bool tests_algorithms_array_equal();
bool tests_algorithms_array_copy();
bool tests_algorithms_array_swap();
bool tests_algorithms_mutation_is_gated();
bool tests_algorithms_constexpr();

// V.    TRAITS         (array_tests_traits.cpp)
bool tests_traits_extent_and_capacity();
bool tests_traits_contiguity_and_metrics();
bool tests_traits_strategy();
bool tests_traits_lifetime();
bool tests_traits_c_arrays();
bool tests_traits_iterability();
bool tests_traits_shiftable();
bool tests_traits_qualified_types();
bool tests_traits_wrappers();
bool tests_traits_concepts();

// VI.   ITERATORS      (array_tests_iterators.cpp)
bool tests_iterators_chunk_ref();
bool tests_iterators_circular_traversal();
bool tests_iterators_circular_arithmetic();
bool tests_iterators_chunk();
bool tests_iterators_window();
bool tests_iterators_window_shorter_than_range();
bool tests_iterators_over_array();

// VII.  ATOMIC         (array_tests_atomic.cpp)
bool tests_atomic_construction();
bool tests_atomic_load_store_exchange();
bool tests_atomic_fetch_operations();
bool tests_atomic_compare_exchange();
bool tests_atomic_fill_and_iteration();
bool tests_atomic_concurrent_increments();

// VIII. COW            (array_tests_cow.cpp)
bool tests_cow_construction();
bool tests_cow_read_access();
bool tests_cow_snapshot_isolation();
bool tests_cow_clones_only_when_shared();
bool tests_cow_modify_and_replace();
bool tests_cow_lock_policy();
bool tests_cow_concurrent_snapshots_are_whole();

// IX.   RCU            (array_tests_rcu.cpp)
bool tests_rcu_construction();
bool tests_rcu_read_access();
bool tests_rcu_update_and_replace();
bool tests_rcu_snapshot();
bool tests_rcu_reader_never_sees_a_torn_array();

// X.    THREADSAFE     (array_tests_threadsafe.cpp)
bool tests_threadsafe_construction();
bool tests_threadsafe_lockfree_queries();
bool tests_threadsafe_element_access();
bool tests_threadsafe_bulk_operations();
bool tests_threadsafe_handles();
bool tests_threadsafe_snapshot_and_optimistic();
bool tests_threadsafe_copy_assignment();
bool tests_threadsafe_policies();
bool tests_threadsafe_concurrent_apply();
bool tests_threadsafe_optimistic_is_never_torn();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// array_spec_construction .. array_spec_threadsafe
//   function: one block each; array_spec() assembles them.
inline
dt::block_spec array_spec_construction()
{
    return dt::block_spec{
        "construction",
        "Building an array<>: value-initialization, the element-list "
        "constructor and its constraints and noexcept, copy and move, the "
        "factories, the options pack, the six cells' constants, and "
        "constant-expression construction.",
        {
            { "tests_construction_default_value_initializes",
              "A default-constructed array value-initializes every element, "
              "in every cell and at extent 0.",
              &tests_construction_default_value_initializes },
            { "tests_construction_element_list",
              "The element-list constructor stores its values in order, "
              "value-initializes the rest, and casts each to value_type.",
              &tests_construction_element_list },
            { "tests_construction_is_constrained",
              "std::is_constructible is true for up to extent castable "
              "values and false for more, or for one that doesn't cast.",
              &tests_construction_is_constrained },
            { "tests_construction_noexcept_follows_conversion",
              "The element-list constructor is noexcept exactly when no "
              "value's conversion can throw.",
              &tests_construction_noexcept_follows_conversion },
            { "tests_construction_copy_and_move",
              "Copy and move construct and assign, from const and non-const "
              "lvalues, and a moved-from string array is left valid.",
              &tests_construction_copy_and_move },
            { "tests_construction_make_array",
              "make_array and make_immutable_array deduce the element type "
              "from the first value and the extent from the count.",
              &tests_construction_make_array },
            { "tests_construction_from_options",
              "array_from_options_t names array<Options...>, and "
              "make_array_from_options appends the extent of its values.",
              &tests_construction_from_options },
            { "tests_construction_cell_constants",
              "Each alias's extent, lifetime, iterability and iterable, and "
              "their independence from option order.",
              &tests_construction_cell_constants },
            { "tests_construction_is_constexpr",
              "Arrays are built and read in constant expressions.",
              &tests_construction_is_constexpr },
        }
    };
}

inline
dt::block_spec array_spec_access()
{
    return dt::block_spec{
        "access",
        "Element access and size queries, and the gating that leaves an "
        "immutable cell read-only while a constexpr cell stays writable.",
        {
            { "tests_access_subscript_and_at",
              "operator[] and at() read and write the same element.",
              &tests_access_subscript_and_at },
            { "tests_access_front_back_data",
              "front(), back() and data() name the first, last and "
              "contiguous storage.",
              &tests_access_front_back_data },
            { "tests_access_size_queries",
              "size(), max_size() and capacity() are the extent; empty() is "
              "extent == 0.",
              &tests_access_size_queries },
            { "tests_access_immutable_is_read_only",
              "An immutable cell has const access only: no assignable "
              "subscript or at(), and data() yields const value_type*.",
              &tests_access_immutable_is_read_only },
            { "tests_access_constexpr_cell_is_writable",
              "A constexpr-lifetime cell keeps its mutable accessors.",
              &tests_access_constexpr_cell_is_writable },
        }
    };
}

inline
dt::block_spec array_spec_iteration()
{
    return dt::block_spec{
        "iteration",
        "begin/end and their const and reverse forms, standard algorithms, "
        "the non-iterable gate, and the empty array.",
        {
            { "tests_iteration_begin_end",
              "[begin, end) is the storage, range-for visits it in order, and "
              "a write through an iterator lands.",
              &tests_iteration_begin_end },
            { "tests_iteration_const_and_reverse",
              "cbegin/cend agree with begin/end; rbegin/rend visit in "
              "reverse.",
              &tests_iteration_const_and_reverse },
            { "tests_iteration_std_algorithms",
              "std::sort, std::accumulate, std::find, std::reverse and "
              "std::equal work over an array.",
              &tests_iteration_std_algorithms },
            { "tests_iteration_is_gated",
              "A non-iterable cell has no begin/end; an immutable iterable "
              "cell yields const iterators only.",
              &tests_iteration_is_gated },
            { "tests_iteration_empty_array",
              "At extent 0, begin == end and range-for visits nothing.",
              &tests_iteration_empty_array },
        }
    };
}

inline
dt::block_spec array_spec_algorithms()
{
    return dt::block_spec{
        "algorithms",
        "fill and swap, the free array_equal / array_copy / array_swap, "
        "their lifetime gates, and their use in constant expressions.",
        {
            { "tests_algorithms_fill",
              "fill() sets every element.",
              &tests_algorithms_fill },
            { "tests_algorithms_member_swap",
              "swap() exchanges every element, strings included.",
              &tests_algorithms_member_swap },
            { "tests_algorithms_array_equal",
              "array_equal compares element-wise, across cells of the same "
              "type and extent.",
              &tests_algorithms_array_equal },
            { "tests_algorithms_array_copy",
              "array_copy copies into a writable array, from any cell.",
              &tests_algorithms_array_copy },
            { "tests_algorithms_array_swap",
              "array_swap exchanges two writable arrays' elements.",
              &tests_algorithms_array_swap },
            { "tests_algorithms_mutation_is_gated",
              "fill, swap, and array_copy or array_swap into an immutable "
              "array, are not callable.",
              &tests_algorithms_mutation_is_gated },
            { "tests_algorithms_constexpr",
              "fill, swap, array_copy and array_equal run in a constant "
              "expression.",
              &tests_algorithms_constexpr },
        }
    };
}

inline
dt::block_spec array_spec_traits()
{
    return dt::block_spec{
        "traits",
        "array_traits.hpp's classification of every cell, the standard "
        "containers, C arrays and the wrappers, through cv and references, "
        "and the concepts over it.",
        {
            { "tests_traits_extent_and_capacity",
              "has_static_extent, is_fixed_capacity, is_dynamic_capacity and "
              "capacity_model_of.",
              &tests_traits_extent_and_capacity },
            { "tests_traits_contiguity_and_metrics",
              "is_contiguous_array in every cell, and the element type, "
              "size, alignment and stride.",
              &tests_traits_contiguity_and_metrics },
            { "tests_traits_strategy",
              "is_trivially_relocatable_array and array_strategy.",
              &tests_traits_strategy },
            { "tests_traits_lifetime",
              "is_constexpr_array, is_mutable_array, is_immutable_array and "
              "array_lifetime_of.",
              &tests_traits_lifetime },
            { "tests_traits_c_arrays",
              "int[4] is a mutable array and const int[4] an immutable one.",
              &tests_traits_c_arrays },
            { "tests_traits_iterability",
              "is_iterable_array and is_non_iterable_array.",
              &tests_traits_iterability },
            { "tests_traits_shiftable",
              "is_shiftable_array: contiguous, sized and not immutable.",
              &tests_traits_shiftable },
            { "tests_traits_qualified_types",
              "const, & and && leave every answer unchanged, for cells and "
              "wrappers alike.",
              &tests_traits_qualified_types },
            { "tests_traits_wrappers",
              "The wrappers keep the wrapped array's axes and register their "
              "concurrency strategy.",
              &tests_traits_wrappers },
            { "tests_traits_concepts",
              "The array concepts agree with the traits (C++20).",
              &tests_traits_concepts },
        }
    };
}

inline
dt::block_spec array_spec_iterators()
{
    return dt::block_spec{
        "iterators",
        "array_iterator.hpp: the chunk view, the circular iterator, and the "
        "chunk and window iterators with their factories.",
        {
            { "tests_iterators_chunk_ref",
              "DChunkRef (and DWindowRef) index, iterate and report empty.",
              &tests_iterators_chunk_ref },
            { "tests_iterators_circular_traversal",
              "A circular iterator visits a ring from its head and wraps.",
              &tests_iterators_circular_traversal },
            { "tests_iterators_circular_arithmetic",
              "Its random-access arithmetic, comparisons and indices.",
              &tests_iterators_circular_arithmetic },
            { "tests_iterators_chunk",
              "Chunks of n, the last one short, and the end factory.",
              &tests_iterators_chunk },
            { "tests_iterators_window",
              "Full windows of w, sliding by one, up to the end factory.",
              &tests_iterators_window },
            { "tests_iterators_window_shorter_than_range",
              "A range shorter than the window yields no window.",
              &tests_iterators_window_shorter_than_range },
            { "tests_iterators_over_array",
              "Chunk and window iterators over an array<>'s iterators.",
              &tests_iterators_over_array },
        }
    };
}

inline
dt::block_spec array_spec_atomic()
{
    return dt::block_spec{
        "atomic",
        "DAtomicArray: per-element atomic operations, and no lost update "
        "under contention.",
        {
            { "tests_atomic_construction",
              "Default and fill construction, constants, and no copy or "
              "move.",
              &tests_atomic_construction },
            { "tests_atomic_load_store_exchange",
              "load, store and exchange per element.",
              &tests_atomic_load_store_exchange },
            { "tests_atomic_fetch_operations",
              "fetch_add, fetch_sub, fetch_and, fetch_or and fetch_xor "
              "return the old value and store the new.",
              &tests_atomic_fetch_operations },
            { "tests_atomic_compare_exchange",
              "compare_exchange_strong and _weak succeed on a match and "
              "load the current value on a miss.",
              &tests_atomic_compare_exchange },
            { "tests_atomic_fill_and_iteration",
              "fill, iteration over the atomics, data() and is_lock_free.",
              &tests_atomic_fill_and_iteration },
            { "tests_atomic_concurrent_increments",
              "Four threads' fetch_adds on every element lose nothing.",
              &tests_atomic_concurrent_increments },
        }
    };
}

inline
dt::block_spec array_spec_cow()
{
    return dt::block_spec{
        "cow",
        "DCowArray: snapshots that writers never disturb, clones taken only "
        "when a snapshot shares the storage, the lock policy option, and "
        "whole snapshots under a concurrent writer.",
        {
            { "tests_cow_construction",
              "Default, copy and move construction from an array, and the "
              "constants.",
              &tests_cow_construction },
            { "tests_cow_read_access",
              "read(), at(), size() and empty().",
              &tests_cow_read_access },
            { "tests_cow_snapshot_isolation",
              "A snapshot keeps its values through set, modify and replace.",
              &tests_cow_snapshot_isolation },
            { "tests_cow_clones_only_when_shared",
              "A write reuses unshared storage and clones shared storage.",
              &tests_cow_clones_only_when_shared },
            { "tests_cow_modify_and_replace",
              "modify returns its function's result; every write advances "
              "version().",
              &tests_cow_modify_and_replace },
            { "tests_cow_lock_policy",
              "The default lock policy (a real lock), and the exclusive and "
              "null policies by option.",
              &tests_cow_lock_policy },
            { "tests_cow_concurrent_snapshots_are_whole",
              "Snapshots taken while a writer thread fills the array hold "
              "four equal values, never going backwards.",
              &tests_cow_concurrent_snapshots_are_whole },
        }
    };
}

inline
dt::block_spec array_spec_rcu()
{
    return dt::block_spec{
        "rcu",
        "rcu_array: guarded reads, copy-modify-publish writes, value "
        "snapshots, and a reader that never sees a half-written array.",
        {
            { "tests_rcu_construction",
              "Default and array construction, the constants, and its "
              "registration as an RCU container.",
              &tests_rcu_construction },
            { "tests_rcu_read_access",
              "read(fn), at(), size() and empty().",
              &tests_rcu_read_access },
            { "tests_rcu_update_and_replace",
              "update(fn) and replace() publish new values.",
              &tests_rcu_update_and_replace },
            { "tests_rcu_snapshot",
              "A snapshot keeps its values through later writes.",
              &tests_rcu_snapshot },
            { "tests_rcu_reader_never_sees_a_torn_array",
              "A reader thread, racing a writer that publishes uniform "
              "arrays, only ever sees uniform arrays.",
              &tests_rcu_reader_never_sees_a_torn_array },
        }
    };
}

inline
dt::block_spec array_spec_threadsafe()
{
    return dt::block_spec{
        "threadsafe",
        "DThreadsafeArray: locked single operations, locked handles, "
        "snapshots, the lock-free queries, the policy aliases, and no lost "
        "update under contention.",
        {
            { "tests_threadsafe_construction",
              "Default, value, array and copy construction, from const and "
              "non-const lvalues.",
              &tests_threadsafe_construction },
            { "tests_threadsafe_lockfree_queries",
              "size_lockfree and empty_lockfree agree with size and empty "
              "from construction on.",
              &tests_threadsafe_lockfree_queries },
            { "tests_threadsafe_element_access",
              "at and set, and set's version bump.",
              &tests_threadsafe_element_access },
            { "tests_threadsafe_bulk_operations",
              "assign, apply and apply_read.",
              &tests_threadsafe_bulk_operations },
            { "tests_threadsafe_handles",
              "read_access, write_access and batch.",
              &tests_threadsafe_handles },
            { "tests_threadsafe_snapshot_and_optimistic",
              "snapshot copies; optimistic returns its function's result.",
              &tests_threadsafe_snapshot_and_optimistic },
            { "tests_threadsafe_copy_assignment",
              "Copy assignment copies and advances version; self-assignment "
              "changes nothing.",
              &tests_threadsafe_copy_assignment },
            { "tests_threadsafe_policies",
              "The default policy, DMutexArray, DSharedArray, DTimedArray.",
              &tests_threadsafe_policies },
            { "tests_threadsafe_concurrent_apply",
              "Four threads' apply calls lose no increment.",
              &tests_threadsafe_concurrent_apply },
            { "tests_threadsafe_optimistic_is_never_torn",
              "optimistic() reads racing a writer thread never see a "
              "half-written array.",
              &tests_threadsafe_optimistic_is_never_torn },
        }
    };
}

// array_spec
//   function: the authoritative description of this suite.
inline
dt::module_spec array_spec()
{
    return dt::module_spec{
        "array",
        "The array family: array<Options...> in its six lifetime x "
        "iterability cells, its options pack and factories, the "
        "classification traits and concepts, the chunk, window and circular "
        "iterators, and the atomic, copy-on-write, RCU and lock-protected "
        "wrappers, each checked under contention.",
        {
            array_spec_construction(),
            array_spec_access(),
            array_spec_iteration(),
            array_spec_algorithms(),
            array_spec_traits(),
            array_spec_iterators(),
            array_spec_atomic(),
            array_spec_cow(),
            array_spec_rcu(),
            array_spec_threadsafe(),
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_ARRAY_TESTS_
