/*******************************************************************************
* djinterp [testing]                          bounded_container_traits_tests.hpp
*
*   Declarations, fixtures and the check macro for the bounded_container_traits
* suite.  The section TUs define the tests_* functions declared at the bottom;
* the runner calls them.  Everything a section needs is here, and nothing a
* section needs is anywhere else.
*
*   WHAT THE FIXTURES ARE FOR:
*   The header under test decides boundedness from SIGNALS - a static `extent`,
* a std::tuple_size specialization, static interval bounds, a capacity()
* without a reserve().  A signal is a syntactic probe, and the interesting
* question is never "does it fire on the type it was written for" but "what
* ELSE does it fire on".  So the fixtures come in families: for each signal,
* the shape it was meant to catch, the shapes it catches by accident, and the
* shapes it correctly declines.  Several of them pin behaviour that is
* arguably a defect in the trait; those are called out at their check sites
* rather than quietly encoded.
*
*   The fixtures declare, and mostly do not define, their members: a trait only
* ever names them inside unevaluated operands, which does not odr-use them, so
* there is nothing to link.
*
*   COMPILE-TIME BY CONSTRUCTION:
*   Nearly every check below is a constant expression.  It is deliberately NOT
* spelled as a static_assert: a failed static_assert kills the build and the
* build produces no report, whereas a failed check produces a red line naming
* the exact assertion, its file and its line.  The handful of invariants whose
* regression genuinely should stop the line are pinned with D_TEST_STATIC (see
* the robustness section).
*
*
* path:      /tests/djinterp/core/container/traits/bounded_container_traits_tests.hpp
*                                        bounded_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.12
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CHECK MACRO
      -----------

II.   FIXTURES
      --------
      i.    extent shapes
      ii.   static-bound shapes
      iii.  capacity / reserve shapes
      iv.   size / max_size shapes
      v.    tuple_size conformers
      vi.   combinations

III.  TEST DECLARATIONS
      -----------------
*/

#ifndef DJINTERP_TESTING_BOUNDED_CONTAINER_TRAITS_TESTS_
#define DJINTERP_TESTING_BOUNDED_CONTAINER_TRAITS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../../../inc/djinterp/core/container/traits/bounded_container_traits.hpp"      // the header under test
#include "djinterp/test/test_traits.hpp"     // the trait-testing toolkit


NS_DJINTERP
NS_TESTING


// the trait-testing toolkit, by name.  The traits under test need no
// qualification: djinterp::testing nests inside djinterp, so `is_bounded_
// container` and friends are found by ordinary unqualified lookup.
using ::djinterp::test::count_holds;
using ::djinterp::test::holds_for_all;
using ::djinterp::test::holds_for_any;
using ::djinterp::test::holds_for_none;
using ::djinterp::test::cvref_report;
using ::djinterp::test::trait_across_cvref;
using ::djinterp::test::trait_ignores_cvref;
using ::djinterp::test::is_bool_trait;
using ::djinterp::test::trait_is_well_formed;


///////////////////////////////////////////////////////////////////////////////
///                I.   CHECK MACRO                                          ///
///////////////////////////////////////////////////////////////////////////////

// bct_report_failure
//   function: prints one failed check, naming the assertion, its file and its
// line.  A suite whose failures say only "tests_signal_capacity FAILED" makes
// the reader re-derive which of its twenty assertions broke; this makes the
// report say so.
D_INLINE void
bct_report_failure(
    const char* _file,
    int         _line,
    const char* _expression
)
{
    std::printf("      !! %s:%d\n         %s\n",
                _file,
                _line,
                _expression);

    return;
}

// D_BCT_CHECK
//   macro: folds one boolean check into the enclosing test's verdict, and
// reports it if it fails.  Expects a local `bool ok` in scope - every tests_*
// function in this suite opens with `bool ok = true;` and closes with
// `return ok;`.
//
//   The condition is evaluated exactly once, and the whole macro is a single
// statement, so it composes inside an if or a loop without surprises.
#define D_BCT_CHECK(...)                                                      \
    do                                                                        \
    {                                                                         \
        const bool bct_result = static_cast<bool>(__VA_ARGS__);               \
                                                                              \
        if (!bct_result)                                                      \
        {                                                                     \
            ::djinterp::testing::bct_report_failure(__FILE__,                 \
                                                    __LINE__,                 \
                                                    #__VA_ARGS__);            \
        }                                                                     \
                                                                              \
        ok = (ok && bct_result);                                              \
    } while (false)


///////////////////////////////////////////////////////////////////////////////
///                II.  FIXTURES                                             ///
///////////////////////////////////////////////////////////////////////////////

// i.   extent shapes
//////////////////////////////////////////
//   has_fixed_extent_signal probes `decltype(clean_t<_Type>::extent)`.  That
// expression is well-formed for considerably more than the "static extent
// constant" the convention intends, and these fixtures span the difference.

// extent_static_member
//   struct: the shape the convention intends - a static compile-time extent.
struct extent_static_member
{
    static constexpr std::size_t extent = 8;
};

// extent_zero
//   struct: a static extent of ZERO.  A capacity of 0 is a legitimate bound
// (the empty container is bounded, not unbounded), so the signal must fire on
// a falsy value - a probe written with `_Type::extent > 0` in mind would get
// this wrong.
struct extent_zero
{
    static constexpr std::size_t extent = 0;
};

// extent_data_member
//   struct: a NON-static data member named `extent`.  Naming a non-static
// member in an unevaluated operand is well-formed, so the signal fires - this
// type is classified bounded on the strength of an ordinary field.
struct extent_data_member
{
    std::size_t extent;
};

// extent_static_function
//   struct: a static member FUNCTION named `extent`.  `decltype(T::extent)`
// is its function type, so the signal fires here too.
struct extent_static_function
{
    static std::size_t extent();
};

// extent_member_function
//   struct: a non-static member function named `extent`.  Naming it without
// an object is ill-formed, so the signal correctly declines.
struct extent_member_function
{
    std::size_t extent();
};

// extent_nested_type
//   struct: a nested TYPE named `extent`.  decltype takes an expression, not
// a type, so the signal correctly declines.
struct extent_nested_type
{
    using extent = std::size_t;
};

// extent_private
//   struct: a private static extent.  Access checking happens during
// substitution, so the failure lands in the immediate context and the signal
// declines rather than diagnosing.
struct extent_private
{
private:
    static constexpr std::size_t extent = 8;
};

// extent_enumerator
//   struct: an ENUMERATOR named `extent`.  A compile-time constant by any
// other name, and the signal fires - which is very likely the right answer,
// but it is an answer the trait's comment does not mention.
struct extent_enumerator
{
    enum
    {
        extent = 8
    };
};


// ii.  static-bound shapes
//////////////////////////////////////////

// bounds_lower_only
//   struct: half an interval.  has_static_bounds_signal requires BOTH
// endpoints, so this must not fire.
struct bounds_lower_only
{
    static constexpr int lower_bound = 0;
};

// bounds_upper_only
//   struct: the other half.
struct bounds_upper_only
{
    static constexpr int upper_bound = 9;
};

// bounds_both
//   struct: a closed interval I = [0, 9] and nothing else - no size(), no
// capacity().  Domain-bounded, and therefore capacity-bounded too: a finite
// domain has a finite carrier.
struct bounds_both
{
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
};

// bounds_private
//   struct: both endpoints, both private.  Must decline.
struct bounds_private
{
private:
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
};

// interval_container
//   struct: the realistic domain-bounded container - a closed interval that
// also presents a size().
struct interval_container
{
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;

    std::size_t size() const;
};


// iii. capacity / reserve shapes
//////////////////////////////////////////
//   The fourth promoting disjunct is `capacity() && !reserve()`: a capacity
// you cannot grow is a fixed capacity.  Both halves of that conjunction are
// syntactic probes with their own edges.

// capacity_const
//   struct: capacity() on a const lvalue - the shape the signal probes.
struct capacity_const
{
    std::size_t capacity() const;
};

// capacity_nonconst
//   struct: capacity() that is NOT const-callable.  The signal binds a const
// lvalue, so it declines - a deliberate narrowing, and one worth pinning.
struct capacity_nonconst
{
    std::size_t capacity();
};

// reserve_sized
//   struct: reserve(size_t) - the growability anti-signal.
struct reserve_sized
{
    void reserve(std::size_t _n);
};

// reserve_int
//   struct: reserve(int).  The probe passes a size_t, which converts, so the
// anti-signal still fires - the parameter type is not pinned, only callability.
struct reserve_int
{
    void reserve(int _n);
};

// reserve_nullary
//   struct: reserve() taking no arguments.  The probe passes one, so the
// anti-signal does NOT fire.  Consequence: a growable container whose reserve
// takes no argument is classified BOUNDED (see growable_nullary_reserve).
struct reserve_nullary
{
    void reserve();
};

// reserve_const
//   struct: a const-qualified reserve.  The probe binds a non-const lvalue,
// on which a const member is still callable, so the anti-signal fires.
struct reserve_const
{
    void reserve(std::size_t _n) const;
};

// fixed_capacity
//   struct: capacity() and size(), no reserve() - the fourth disjunct's
// positive case.  Bounded.
struct fixed_capacity
{
    std::size_t capacity() const;
    std::size_t size() const;
};

// growable
//   struct: capacity(), reserve() and size() - the fourth disjunct's negative
// case, and the shape of std::vector.  Unbounded.
struct growable
{
    std::size_t capacity() const;
    std::size_t size() const;

    void reserve(std::size_t _n);
};

// growable_nullary_reserve
//   struct: growable, but its reserve takes no argument, so the anti-signal
// misses it and the type is classified bounded.  A false positive, pinned
// here so that fixing it is a visible change rather than a silent one.
struct growable_nullary_reserve
{
    std::size_t capacity() const;
    std::size_t size() const;

    void reserve();
};

// reserve_without_capacity
//   struct: reserve() and size(), but no capacity().  The fourth disjunct
// needs capacity() as its subject, so nothing promotes: unbounded.
struct reserve_without_capacity
{
    std::size_t size() const;

    void reserve(std::size_t _n);
};


// iv.  size / max_size shapes
//////////////////////////////////////////

// size_const
//   struct: size() alone - "looks like a container", with no bounding
// evidence.  The canonical unbounded type.
struct size_const
{
    std::size_t size() const;
};

// size_nonconst
//   struct: size() that is not const-callable, so the container guard does
// not fire.  Neither bounded nor unbounded: unknown.
struct size_nonconst
{
    std::size_t size();
};

// max_size_only
//   struct: max_size() and nothing else.  max_size is informational, never
// promoting, and it is not the container guard either - so this is unknown,
// not unbounded.
struct max_size_only
{
    std::size_t max_size() const;
};

// sized_and_maxed
//   struct: size() and max_size().  Still no bounding evidence - a growable
// vector exposes exactly this pair - so: unbounded.
struct sized_and_maxed
{
    std::size_t size() const;
    std::size_t max_size() const;
};


// v.   tuple_size conformers
//////////////////////////////////////////

// tuple_size_conformer
//   struct: a fixed-size aggregate that says so the canonical way, by
// specializing std::tuple_size (below).  It has no size(), no capacity and no
// extent - the tuple_size signal alone promotes it.
struct tuple_size_conformer
{
};


// vi.  combinations
//////////////////////////////////////////

// extent_and_growable
//   struct: a static extent AND a growable capacity.  The disjunction is not
// a vote: the first promoting signal wins, so this is bounded despite being
// demonstrably growable.
struct extent_and_growable
{
    static constexpr std::size_t extent = 4;

    std::size_t capacity() const;
    std::size_t size() const;

    void reserve(std::size_t _n);
};

// bounds_and_growable
//   struct: static interval bounds AND a growable capacity.  Bounded, by the
// third disjunct.
struct bounds_and_growable
{
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;

    std::size_t capacity() const;
    std::size_t size() const;

    void reserve(std::size_t _n);
};

// every_signal
//   struct: fires every one of the six signals the snapshot reports - extent,
// tuple_size (specialized below), static bounds, capacity, reserve, max_size -
// and presents a size() besides.  The all-ones row of the truth table.
struct every_signal
{
    static constexpr std::size_t extent      = 4;
    static constexpr int         lower_bound = 0;
    static constexpr int         upper_bound = 9;

    std::size_t capacity() const;
    std::size_t max_size() const;
    std::size_t size() const;

    void reserve(std::size_t _n);
};

// no_signal
//   struct: fires nothing.  The all-zeros row: unknown.
struct no_signal
{
};


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                II.v  TUPLE_SIZE SPECIALIZATIONS                          ///
///////////////////////////////////////////////////////////////////////////////
//   has_tuple_size_signal probes std::tuple_size, whose primary template is
// declared and never defined - which is exactly why the probe is SFINAE-clean
// for std::vector and friends.  A conforming fixed-size aggregate opts in by
// specializing it, and these two do.

namespace std
{

    // tuple_size<djinterp::testing::tuple_size_conformer>
    //   trait: opts the conformer fixture into the fixed-size-aggregate
    // protocol with a compile-time count of 5.
    template<>
    struct tuple_size< ::djinterp::testing::tuple_size_conformer>
        : integral_constant<size_t, 5>
    {
    };

    // tuple_size<djinterp::testing::every_signal>
    //   trait: the same opt-in for the all-signals fixture.
    template<>
    struct tuple_size< ::djinterp::testing::every_signal>
        : integral_constant<size_t, 4>
    {
    };

}  // namespace std


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                III. TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// I.   structural signals
bool tests_signal_fixed_extent();
bool tests_signal_fixed_extent_shapes();
bool tests_signal_tuple_size();
bool tests_signal_static_bounds();
bool tests_signal_capacity();
bool tests_signal_reserve();
bool tests_signal_max_size();
bool tests_signal_size();
bool tests_signal_value_companions();

// II.  capacity classification
bool tests_bounded_extent_disjunct();
bool tests_bounded_tuple_size_disjunct();
bool tests_bounded_static_bounds_disjunct();
bool tests_bounded_capacity_disjunct();
bool tests_bounded_negative();
bool tests_bounded_ignores_size();
bool tests_bounded_first_signal_wins();
bool tests_unbounded_truth_table();
bool tests_bounded_unbounded_exclusive();
bool tests_capacity_value_companions();

// III. domain classification
bool tests_domain_bounded();
bool tests_domain_implies_bounded();
bool tests_domain_orthogonal();
bool tests_domain_value_companion();

// IV.  capacity-bound summary
bool tests_capacity_bound_enumerators();
bool tests_capacity_bound_name();
bool tests_capacity_bound_name_out_of_range();
bool tests_capacity_bound_of();
bool tests_capacity_bound_of_type();
bool tests_capacity_bound_of_agrees();
bool tests_capacity_bound_of_value_companion();

// V.   aggregate snapshot
bool tests_class_signals();
bool tests_class_verdicts();
bool tests_class_summary();
bool tests_class_agrees_with_traits();
bool tests_class_is_constexpr();

// VI.  robustness  (cross-cutting)
bool tests_cvref_signals();
bool tests_cvref_verdicts();
bool tests_hostile_types_rejected();
bool tests_hostile_types_survived();
bool tests_incomplete_type();
bool tests_trait_shape();

// VII. standard library
bool tests_stdlib_array();
bool tests_stdlib_vector();
bool tests_stdlib_node_containers();
bool tests_stdlib_string();
bool tests_stdlib_tuple_pair();
bool tests_stdlib_associative();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_BOUNDED_CONTAINER_TRAITS_TESTS_
