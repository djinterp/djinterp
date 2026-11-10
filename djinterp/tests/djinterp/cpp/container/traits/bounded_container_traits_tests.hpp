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
* path:      /tests/djinterp/cpp/container/traits/bounded_container_traits_tests.hpp
*                                        bounded_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.12
*                                                            revised: 2026.11.09
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
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/bounded_container_traits.hpp"      // the header under test
#include "djinterp/cpp/modules/test/test_traits.hpp"     // the trait-testing toolkit
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// the trait-testing toolkit, by name.  The traits under test need no
// qualification: djinterp::testing nests inside djinterp, so `is_bounded_
// container` and friends are found by ordinary unqualified lookup.
using ::djinterp::test::count_holds;
using ::djinterp::test::holds_for_all;
using ::djinterp::test::holds_for_any;
using ::djinterp::test::holds_for_none;
using ::djinterp::test::DCvrefReport;
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
D_INLINE
void bct_report_failure(
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
    static
    std::size_t extent();
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

// DBoundsLowerOnly
//   struct: half an interval.  has_static_bounds_signal requires BOTH
// endpoints, so this must not fire.
struct DBoundsLowerOnly
{
    static constexpr int lower_bound = 0;
};

// DBoundsUpperOnly
//   struct: the other half.
struct DBoundsUpperOnly
{
    static constexpr int upper_bound = 9;
};

// DBoundsBoth
//   struct: a closed interval I = [0, 9] and nothing else - no size(), no
// capacity().  Domain-bounded, and therefore capacity-bounded too: a finite
// domain has a finite carrier.
struct DBoundsBoth
{
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
};

// DBoundsPrivate
//   struct: both endpoints, both private.  Must decline.
struct DBoundsPrivate
{
private:
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
};

// DIntervalContainer
//   struct: the realistic domain-bounded container - a closed interval that
// also presents a size().
struct DIntervalContainer
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

// DCapacityConst
//   struct: capacity() on a const lvalue - the shape the signal probes.
struct DCapacityConst
{
    std::size_t capacity() const;
};

// DCapacityNonconst
//   struct: capacity() that is NOT const-callable.  The signal binds a const
// lvalue, so it declines - a deliberate narrowing, and one worth pinning.
struct DCapacityNonconst
{
    std::size_t capacity();
};

// DReserveSized
//   struct: reserve(size_t) - the growability anti-signal.
struct DReserveSized
{
    void reserve(
        std::size_t _n
    );
};

// DReserveInt
//   struct: reserve(int).  The probe passes a size_t, which converts, so the
// anti-signal still fires - the parameter type is not pinned, only callability.
struct DReserveInt
{
    void reserve(
        int _n
    );
};

// DReserveNullary
//   struct: reserve() taking no arguments.  The probe passes one, so the
// anti-signal does NOT fire.  Consequence: a growable container whose reserve
// takes no argument is classified BOUNDED (see DGrowableNullaryReserve).
struct DReserveNullary
{
    void reserve();
};

// DReserveConst
//   struct: a const-qualified reserve.  The probe binds a non-const lvalue,
// on which a const member is still callable, so the anti-signal fires.
struct DReserveConst
{
    void reserve(
        std::size_t _n
    ) const;
};

// DFixedCapacity
//   struct: capacity() and size(), no reserve() - the fourth disjunct's
// positive case.  Bounded.
struct DFixedCapacity
{
    std::size_t capacity() const;
    std::size_t size() const;
};

// growable
//   struct: capacity(), reserve() and size() - the fourth disjunct's negative
// case, and the shape of std::vector.  Unbounded.
struct DGrowable
{
    std::size_t capacity() const;
    std::size_t size() const;

    void reserve(
        std::size_t _n
    );
};

// DGrowableNullaryReserve
//   struct: growable, but its reserve takes no argument, so the anti-signal
// misses it and the type is classified bounded.  A false positive, pinned
// here so that fixing it is a visible change rather than a silent one.
struct DGrowableNullaryReserve
{
    std::size_t capacity() const;
    std::size_t size() const;

    void reserve();
};

// DReserveWithoutCapacity
//   struct: reserve() and size(), but no capacity().  The fourth disjunct
// needs capacity() as its subject, so nothing promotes: unbounded.
struct DReserveWithoutCapacity
{
    std::size_t size() const;

    void reserve(
        std::size_t _n
    );
};


// iv.  size / max_size shapes
//////////////////////////////////////////

// DSizeConst
//   struct: size() alone - "looks like a container", with no bounding
// evidence.  The canonical unbounded type.
struct DSizeConst
{
    std::size_t size() const;
};

// DSizeNonconst
//   struct: size() that is not const-callable, so the container guard does
// not fire.  Neither bounded nor unbounded: unknown.
struct DSizeNonconst
{
    std::size_t size();
};

// DMaxSizeOnly
//   struct: max_size() and nothing else.  max_size is informational, never
// promoting, and it is not the container guard either - so this is unknown,
// not unbounded.
struct DMaxSizeOnly
{
    std::size_t max_size() const;
};

// DSizedAndMaxed
//   struct: size() and max_size().  Still no bounding evidence - a growable
// vector exposes exactly this pair - so: unbounded.
struct DSizedAndMaxed
{
    std::size_t size() const;
    std::size_t max_size() const;
};


// v.   tuple_size conformers
//////////////////////////////////////////

// DTupleSizeConformer
//   struct: a fixed-size aggregate that says so the canonical way, by
// specializing std::tuple_size (below).  It has no size(), no capacity and no
// extent - the tuple_size signal alone promotes it.
struct DTupleSizeConformer
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

    void reserve(
        std::size_t _n
    );
};

// DBoundsAndGrowable
//   struct: static interval bounds AND a growable capacity.  Bounded, by the
// third disjunct.
struct DBoundsAndGrowable
{
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;

    std::size_t capacity() const;
    std::size_t size() const;

    void reserve(
        std::size_t _n
    );
};

// DEverySignal
//   struct: fires every one of the six signals the snapshot reports - extent,
// tuple_size (specialized below), static bounds, capacity, reserve, max_size -
// and presents a size() besides.  The all-ones row of the truth table.
struct DEverySignal
{
    static constexpr std::size_t extent      = 4;
    static constexpr int         lower_bound = 0;
    static constexpr int         upper_bound = 9;

    std::size_t capacity() const;
    std::size_t max_size() const;
    std::size_t size() const;

    void reserve(
        std::size_t _n
    );
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

    // tuple_size<djinterp::testing::DTupleSizeConformer>
    //   trait: opts the conformer fixture into the fixed-size-aggregate
    // protocol with a compile-time count of 5.
    template<>
    struct tuple_size< ::djinterp::testing::DTupleSizeConformer>
        : integral_constant<size_t, 5>
    {
    };

    // tuple_size<djinterp::testing::DEverySignal>
    //   trait: the same opt-in for the all-signals fixture.
    template<>
    struct tuple_size< ::djinterp::testing::DEverySignal>
        : integral_constant<size_t, 4>
    {
    };

}  // namespace std


NS_DJINTERP
NS_TESTING
#endif  // !DTEST_SPEC_MODE  (fixtures)



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


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// bounded_container_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
bounded_container_traits_spec()
{
    return dt::module_spec{
        "bounded_container_traits",
        "The boundedness axis: capacity signals, domain signals, and the "
        "classifications built on them",
        {
            dt::block_spec{
                "signals",
                "Structural signals",
                {
                    { "tests_signal_fixed_extent",
                      "has_fixed_extent_signal on the shape it was written for",
                      &tests_signal_fixed_extent },
                    { "tests_signal_fixed_extent_shapes",
                      "What ELSE `decltype(clean_t<_Type>::extent)` accepts",
                      &tests_signal_fixed_extent_shapes },
                    { "tests_signal_tuple_size",
                      "has_tuple_size_signal - the fixed-size-aggregate "
                      "protocol",
                      &tests_signal_tuple_size },
                    { "tests_signal_static_bounds",
                      "has_static_bounds_signal - the closed-interval domain "
                      "I = [x, y, z]",
                      &tests_signal_static_bounds },
                    { "tests_signal_capacity",
                      "has_capacity_signal - a capacity() accessor on a CONST "
                      "lvalue",
                      &tests_signal_capacity },
                    { "tests_signal_reserve",
                      "has_reserve_signal - the growability anti-signal",
                      &tests_signal_reserve },
                    { "tests_signal_max_size",
                      "has_max_size_signal - informational, never promoting",
                      &tests_signal_max_size },
                    { "tests_signal_size",
                      "has_size_signal - the \"looks like a container\" guard",
                      &tests_signal_size },
                    { "tests_signal_value_companions",
                      "Every signal trait carries a `_v` companion emitted by "
                      "D_TYPE_TRAIT_VALUE_BOOL",
                      &tests_signal_value_companions },
                }
            },
            dt::block_spec{
                "capacity",
                "Capacity classification",
                {
                    { "tests_bounded_extent_disjunct",
                      "is_bounded_container, first disjunct: a static `extent`",
                      &tests_bounded_extent_disjunct },
                    { "tests_bounded_tuple_size_disjunct",
                      "is_bounded_container, second disjunct: a "
                      "std::tuple_size specialization",
                      &tests_bounded_tuple_size_disjunct },
                    { "tests_bounded_static_bounds_disjunct",
                      "is_bounded_container, third disjunct: static interval "
                      "endpoints",
                      &tests_bounded_static_bounds_disjunct },
                    { "tests_bounded_capacity_disjunct",
                      "is_bounded_container, fourth disjunct: `capacity() && "
                      "!reserve()`",
                      &tests_bounded_capacity_disjunct },
                    { "tests_bounded_negative",
                      "The all-zeros row and its neighbours: types with no "
                      "promoting evidence at all",
                      &tests_bounded_negative },
                    { "tests_bounded_ignores_size",
                      "is_bounded_container is deliberately NOT gated on "
                      "size()",
                      &tests_bounded_ignores_size },
                    { "tests_bounded_first_signal_wins",
                      "The disjunction is not a vote",
                      &tests_bounded_first_signal_wins },
                    { "tests_unbounded_truth_table",
                      "is_unbounded_container = has_size_signal && "
                      "!is_bounded_container",
                      &tests_unbounded_truth_table },
                    { "tests_bounded_unbounded_exclusive",
                      "The two verdicts are mutually exclusive but NOT "
                      "exhaustive",
                      &tests_bounded_unbounded_exclusive },
                    { "tests_capacity_value_companions",
                      "The `_v` companions of the two capacity verdicts",
                      &tests_capacity_value_companions },
                }
            },
            dt::block_spec{
                "domain",
                "Domain classification",
                {
                    { "tests_domain_bounded",
                      "is_domain_bounded_container - the orthogonal sub-axis",
                      &tests_domain_bounded },
                    { "tests_domain_implies_bounded",
                      "Domain-bounded implies capacity-bounded",
                      &tests_domain_implies_bounded },
                    { "tests_domain_orthogonal",
                      "The two sub-axes are independent - all four quadrants "
                      "are populated",
                      &tests_domain_orthogonal },
                    { "tests_domain_value_companion",
                      "is_domain_bounded_container_v",
                      &tests_domain_value_companion },
                }
            },
            dt::block_spec{
                "summary",
                "Capacity-bound summary",
                {
                    { "tests_capacity_bound_enumerators",
                      "The capacity_bound enum itself",
                      &tests_capacity_bound_enumerators },
                    { "tests_capacity_bound_name",
                      "capacity_bound_name - the stable spelling",
                      &tests_capacity_bound_name },
                    { "tests_capacity_bound_name_out_of_range",
                      "What capacity_bound_name does with a value that is not "
                      "an enumerator",
                      &tests_capacity_bound_name_out_of_range },
                    { "tests_capacity_bound_of",
                      "capacity_bound_of - the three-way classification",
                      &tests_capacity_bound_of },
                    { "tests_capacity_bound_of_type",
                      "The nested ::type and the capacity_bound_of_t alias",
                      &tests_capacity_bound_of_type },
                    { "tests_capacity_bound_of_agrees",
                      "The summary must be a faithful re-statement of the two "
                      "predicates",
                      &tests_capacity_bound_of_agrees },
                    { "tests_capacity_bound_of_value_companion",
                      "capacity_bound_of_v - an inline variable on C++17+, a "
                      "plain variable template on C++14, and absent below that",
                      &tests_capacity_bound_of_value_companion },
                }
            },
            dt::block_spec{
                "snapshot",
                "Aggregate snapshot",
                {
                    { "tests_class_signals",
                      "DBoundedContainerClass - the six SIGNAL members",
                      &tests_class_signals },
                    { "tests_class_verdicts",
                      "DBoundedContainerClass - the three VERDICT members",
                      &tests_class_verdicts },
                    { "tests_class_summary",
                      "DBoundedContainerClass - the two SUMMARY members",
                      &tests_class_summary },
                    { "tests_class_agrees_with_traits",
                      "The snapshot must be a faithful re-statement of the "
                      "standalone traits - it is a convenience, and a "
                      "convenience that can disagree with its sources is "
                      "worse than no convenience at all",
                      &tests_class_agrees_with_traits },
                    { "tests_class_is_constexpr",
                      "checks (std::integral_constant<bool, "
                      "arr::is_bounded>::value)",
                      &tests_class_is_constexpr },
                }
            },
            dt::block_spec{
                "robustness",
                "Robustness  (cv-ref, hostile types, incomplete types, trait "
                "shape)",
                {
                    { "tests_cvref_signals",
                      "Every signal trait must answer identically for T, "
                      "const T, volatile T, const volatile T, T&, const T&, "
                      "T&& and const T&&",
                      &tests_cvref_signals },
                    { "tests_cvref_verdicts",
                      "The three verdicts, the summary and the snapshot all "
                      "clean their argument too",
                      &tests_cvref_verdicts },
                    { "tests_hostile_types_rejected",
                      "Every trait must answer FALSE for every type in the "
                      "toolkit's adversarial zoo: void, const void, function "
                      "types, function pointers and references, bounded and "
                      "unbounded arrays, lvalue and rvalue references, member "
                      "pointers, nullptr_t, enums, and the class-type "
                      "adversaries - an abstract class, a final class, a "
                      "class whose members are private, one whose members are "
                      "ambiguously inherited, one that converts to anything, "
                      "one that overloads operator& and operator,, and an "
                      "incomplete type",
                      &tests_hostile_types_rejected },
                    { "tests_hostile_types_survived",
                      "SFINAE-friendliness: the traits must REPORT false on a "
                      "hostile type, not DIAGNOSE it",
                      &tests_hostile_types_survived },
                    { "tests_incomplete_type",
                      "An incomplete type must be tolerated, not completed",
                      &tests_incomplete_type },
                    { "tests_trait_shape",
                      "Is each trait a well-formed bool trait?",
                      &tests_trait_shape },
                }
            },
            dt::block_spec{
                "stdlib",
                "Standard library",
                {
                    { "tests_stdlib_array",
                      "std::array - the bounded case the design is aimed at",
                      &tests_stdlib_array },
                    { "tests_stdlib_vector",
                      "std::vector - the unbounded case the design is aimed at",
                      &tests_stdlib_vector },
                    { "tests_stdlib_node_containers",
                      "The node-based sequences: no capacity(), no reserve(), "
                      "no extent",
                      &tests_stdlib_node_containers },
                    { "tests_stdlib_string",
                      "std::string and std::string_view",
                      &tests_stdlib_string },
                    { "tests_stdlib_tuple_pair",
                      "std::tuple and std::pair - bounded, and not containers",
                      &tests_stdlib_tuple_pair },
                    { "tests_stdlib_associative",
                      "The associative and unordered-associative containers",
                      &tests_stdlib_associative },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_BOUNDED_CONTAINER_TRAITS_TESTS_
