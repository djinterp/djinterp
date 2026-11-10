/*******************************************************************************
* djinterp [testing]                       container_comparison_traits_tests.hpp
*
*   Declarations, fixtures and the check macros for the container_comparison_traits
* suite.  The section TUs define the tests_* functions declared at the bottom;
* the runner calls them.
*
*   WHAT THIS HEADER IS TESTING.
*   container_comparison_traits.hpp compares two container TYPES on five axes and
* folds four of them into one verdict:
*
*       refinement = meet( meet( meet( discipline, lifetime ), iterability ),
*                          storage )
*
* CONTENT is the fifth axis and is deliberately NOT in the fold - the rung at
* which two types compare is a type-level fact, but whether their contents agree
* at that rung is a value-level one, and this module reports only the first.
*
*   So the suite has four jobs, and they are genuinely different:
*     1. does each axis rank its own inputs correctly, in isolation?
*     2. does the meet fold them correctly - all sixteen cells?
*     3. do the ORDER-THEORETIC LAWS hold?  Reflexivity, the direction contract
*        (greater one way is less the other), transitivity, and the exclusion of
*        content from the fold.  An axis that is right cell-by-cell and wrong as
*        an order is worse than useless, because it looks right
*     4. does the profile faithfully restate its axes?
*
*   THE FIXTURES ARE LEVERS.
*   Every axis has an independent opt-in, and the fixtures below pull exactly one
* at a time, from a single common origin (`DRuntimeSeq`).  That is what makes an
* answer attributable: when refinement_of<DCapacitySeq, DRuntimeSeq> says
* `greater`, it is because ONE overlay flag differs and nothing else does, and the
* suite knows which.  Two fixtures differing on two axes prove nothing about
* either.
*
*   The two couplings that could not be broken are called out where they bite:
*
*     an `extent`               sets capacity_bounded AND makes the lifetime
*                               compile-time.  `DCapacitySeq` uses a bare
*                               capacity() instead, which moves the discipline
*                               axis alone
*     interval bounds           set domain_bounded, capacity_bounded AND make the
*                               multiplicity `unique`.  There is no lever that
*                               moves domain alone, so the suite does not pretend
*                               there is
*
*   COMPLETENESS.
*   native_content_level_of, discipline_relation and storage_relation all tolerate
* an INCOMPLETE type.  lifetime_relation, DIterabilityRelation, DComparisonProfile
* and refinement_of do not: they route through container_lifetime -> lifetime_of
* -> is_literal_type -> std::is_trivially_destructible, whose mandate is a complete
* type.  The batteries therefore run the first three over D_TEST_HOSTILE_TYPES and
* the rest over D_TEST_HOSTILE_TYPES_COMPLETE, and tests_incomplete_type pins where
* the line falls.  The defect is lifetime.hpp's and is inherited here, exactly as
* it is inherited by constexpr_container_traits.hpp.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_comparison_traits_tests.hpp
*                                   container_comparison_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.13
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CHECK MACROS
      ------------

II.   FIXTURES
      --------
      i.    the origin
      ii.   content rungs
      iii.  discipline levers
      iv.   lifetime lever
      v.    iterability lattice
      vi.   storage levers

III.  TEST DECLARATIONS
      -----------------
*/

#ifndef DJINTERP_TESTING_CONTAINER_COMPARISON_TRAITS_TESTS_
#define DJINTERP_TESTING_CONTAINER_COMPARISON_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/container_comparison_traits.hpp"   // the header under test
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
// qualification: djinterp::testing nests inside djinterp, so `refinement_of`,
// `order_relation`, `content_level` and friends are found by ordinary unqualified
// lookup.
using ::djinterp::test::count_holds;
using ::djinterp::test::holds_for_all;
using ::djinterp::test::holds_for_none;
using ::djinterp::test::DCvrefReport;
using ::djinterp::test::trait_across_cvref;
using ::djinterp::test::trait_ignores_cvref;
using ::djinterp::test::is_bool_trait;
using ::djinterp::test::trait_is_well_formed;

namespace fixtures = ::djinterp::test::fixtures;


///////////////////////////////////////////////////////////////////////////////
///                I.   CHECK MACROS                                         ///
///////////////////////////////////////////////////////////////////////////////

// ccm_report_failure
//   function: prints one failed check, naming the assertion, its file and its
// line, so a red line in the report identifies the assertion rather than only
// the function that contained it.
D_INLINE
void ccm_report_failure(
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

// ccm_dual
//   function: the relation read the other way round.  The DIRECTION contract of
// the whole module - "a relation is read of the LEFT operand against the right" -
// says that swapping the operands must invert an ordering and preserve
// everything else:
//
//       greater  <->  less
//       equivalent    stays equivalent
//       incomparable  stays incomparable
//
//   That is a law, not an observation, and it is the law most easily broken by a
// one-character slip in a ternary chain - a chain that gets every INDIVIDUAL cell
// right and still fails it.  D_CCM_DUAL checks both directions at once so it
// cannot be forgotten.
D_INLINE D_CONSTEXPR
order_relation ccm_dual(
    order_relation _relation
)
{
    return ( _relation == order_relation::greater ) ? order_relation::less
         : ( _relation == order_relation::less )    ? order_relation::greater
         :                                            _relation;
}

// D_CCM_CHECK
//   macro: folds one boolean check into the enclosing test's verdict, and
// reports it if it fails.  Expects a local `bool ok` in scope - every tests_*
// function in this suite opens with `bool ok = true;` and closes with
// `return ok;`.  Variadic, so a check containing a template-argument comma
// passes through intact.
#define D_CCM_CHECK(...)                                                      \
    do                                                                        \
    {                                                                         \
        const bool ccm_result = static_cast<bool>(__VA_ARGS__);               \
                                                                              \
        if (!ccm_result)                                                      \
        {                                                                     \
            ::djinterp::testing::ccm_report_failure(__FILE__,                 \
                                                    __LINE__,                 \
                                                    #__VA_ARGS__);            \
        }                                                                     \
                                                                              \
        ok = (ok && ccm_result);                                              \
    } while (false)

// D_CCM_REL
//   macro: an order_relation check that, on failure, PRINTS the relation it
// actually got.  There are four of them and the interesting failures are the ones
// that land on the wrong one - `expected greater, got incomparable` is a bug
// report; `false` is a puzzle.
#define D_CCM_REL(EXPECTED, ...)                                              \
    do                                                                        \
    {                                                                         \
        D_CONSTEXPR ::djinterp::order_relation ccm_actual = (__VA_ARGS__);    \
                                                                              \
        if (ccm_actual != (EXPECTED))                                         \
        {                                                                     \
            std::printf("      !! %s:%d\n"                                    \
                        "         %s\n"                                       \
                        "         expected %s, got %s\n",                     \
                        __FILE__,                                             \
                        __LINE__,                                             \
                        #__VA_ARGS__,                                         \
                        ::djinterp::order_relation_name(EXPECTED),            \
                        ::djinterp::order_relation_name(ccm_actual));         \
        }                                                                     \
                                                                              \
        ok = ( ok && (ccm_actual == (EXPECTED)) );                            \
    } while (false)

// D_CCM_LEVEL
//   macro: the same, for content_level.  Five rungs, and a wrong one is a wrong
// rung, not a false.
#define D_CCM_LEVEL(EXPECTED, ...)                                            \
    do                                                                        \
    {                                                                         \
        D_CONSTEXPR ::djinterp::content_level ccm_actual = (__VA_ARGS__);     \
                                                                              \
        if (ccm_actual != (EXPECTED))                                         \
        {                                                                     \
            std::printf("      !! %s:%d\n"                                    \
                        "         %s\n"                                       \
                        "         expected %s, got %s\n",                     \
                        __FILE__,                                             \
                        __LINE__,                                             \
                        #__VA_ARGS__,                                         \
                        ::djinterp::content_level_name(EXPECTED),             \
                        ::djinterp::content_level_name(ccm_actual));          \
        }                                                                     \
                                                                              \
        ok = ( ok && (ccm_actual == (EXPECTED)) );                            \
    } while (false)

// D_CCM_DUAL
//   macro: checks refinement_of<LEFT, RIGHT> against EXPECTED **and**
// refinement_of<RIGHT, LEFT> against its dual, in one line.  The direction
// contract, enforced at every site rather than in one place that everybody
// forgets to add a case to.
#define D_CCM_DUAL(EXPECTED, LEFT, RIGHT)                                     \
    do                                                                        \
    {                                                                         \
        D_CCM_REL((EXPECTED),                                                 \
                  (::djinterp::refinement_of<LEFT, RIGHT>::value));           \
        D_CCM_REL(::djinterp::testing::ccm_dual(EXPECTED),                    \
                  (::djinterp::refinement_of<RIGHT, LEFT>::value));           \
    } while (false)


///////////////////////////////////////////////////////////////////////////////
///                II.  FIXTURES                                             ///
///////////////////////////////////////////////////////////////////////////////

// i.   the origin
//////////////////////////////////////////

// DRuntimeSeq
//   struct: the origin of the fixture lattice, and the point every other fixture
// is one lever away from.  A plain sequence:
//
//       content      seq        (a sequence's native rung)
//       discipline   mult = sequence, no keyed / sorted / capacity / domain
//       lifetime     runtime
//       iterability  iterable, runtime stage, NON-CONST mode
//       storage      unknown    (no opt-in)
//
//   Members are declared and never defined; every trait names them only in
// unevaluated operands.
struct DRuntimeSeq
{
    using value_type     = int;
    using iterator       = int*;
    using const_iterator = const int*;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    std::size_t size() const;
};


// ii.  content rungs
//////////////////////////////////////////
//   native_content_level_of reads the structure verdict first and the
// multiplicity kind second: a nested container compares STRUCTURALLY whatever its
// multiplicity, and only a flat one falls through to its mu.  So there are five
// rungs and one precedence rule, and there is a fixture for each.

// DNotAContainer
//   struct: no size(), no iterators, no value_type.  Its multiplicity is
// `unknown` and its content rung is `none`.
//
//   It is also the fixture that carries THE FINDING.  multiplicity_kind_rank
// gives `unknown` the value -1 - the LOWEST - and overlay_subsumes reads a lower
// rank as a TIGHTER multiplicity.  So an overlay with no multiplicity at all
// subsumes every overlay that has one, and this empty struct comes out with a
// STRONGER DISCIPLINE than std::set.  See tests_discipline_non_container.
struct DNotAContainer
{
};

// DUniqueSeq
//   struct: opts in to m = 1 - set semantics.  Native rung: set.  Its
// multiplicity rank (0) is the tightest real one, so it subsumes the sequences.
struct DUniqueSeq : DRuntimeSeq
{
    static constexpr std::size_t multiplicity = unique_multiplicity;
};

// DBoundedBag
//   struct: opts in to 1 < m < inf - a bounded multiset.  Native rung: bag.  Rank
// 1, between unique and sequence, so it sits strictly inside the discipline chain
// and is what makes a TRANSITIVITY test possible at all.
struct DBoundedBag : DRuntimeSeq
{
    static constexpr std::size_t multiplicity = 3;
};

// DUnboundedBag
//   struct: opts in to m = inf under an equivalence - a multiset.  Native rung:
// bag, the SAME as DBoundedBag, even though its rank (2) is the loosest.  The
// content hierarchy and the discipline order are different orders over the same
// data, and this pair is what says so.
struct DUnboundedBag : DRuntimeSeq
{
    static constexpr std::size_t multiplicity = unbounded_multiplicity;
};

// DNestedSeq
//   struct: a sequence whose element is itself a container.  Native rung: str.
//
//   Its OVERLAY is identical to DRuntimeSeq's - nesting is a structural fact and
// wears no restriction - so the two differ on the content axis and on nothing
// else.  That makes this the fixture that proves CONTENT IS NOT IN THE
// REFINEMENT: refinement_of<DNestedSeq, DRuntimeSeq> must be `equivalent` while
// their content rungs are str and seq.
struct DNestedSeq
{
    using value_type     = DRuntimeSeq;
    using iterator       = DRuntimeSeq*;
    using const_iterator = const DRuntimeSeq*;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    std::size_t size() const;
};

// DHierarchicalUnique
//   struct: nested AND m = 1.  The precedence case: a nested container compares
// structurally REGARDLESS of its multiplicity, so its native rung is str and not
// set.  A helper that read the multiplicity first would say `set`, and would be
// wrong in exactly the case the hierarchy exists for.
struct DHierarchicalUnique
{
    using value_type     = DRuntimeSeq;
    using iterator       = DRuntimeSeq*;
    using const_iterator = const DRuntimeSeq*;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    std::size_t size() const;

    static constexpr std::size_t multiplicity = unique_multiplicity;
};


// iii. discipline levers
//////////////////////////////////////////
//   overlay_subsumes is a conjunction over five components: the multiplicity rank
// and four flag-restrictions.  `a` subsumes `b` iff a's multiplicity is at least
// as tight AND a bears every flag b requires.  So there are two ways to be
// GREATER (tighter mu, or a superset of flags) and one way to be INCOMPARABLE
// (each bears a flag the other does not, or one is tighter on mu and looser on
// flags), and there is a lever for each.

// DSortedSeq
//   struct: opts in to the sorted invariant.  One flag, and no change to mu.
struct DSortedSeq : DRuntimeSeq
{
    static constexpr bool sorted_invariant = true;
};

// DCapacitySeq
//   struct: a capacity() with no reserve() - the bounded_container_traits signal
// for a fixed capacity.  One flag, and no change to mu.
//
//   Deliberately NOT spelled with a static `extent`, which would have been the
// obvious way: an extent sets capacity_bounded AND makes the lifetime
// compile-time, and a fixture that moves two axes at once cannot attribute a
// verdict to either.  A bare capacity() moves the discipline axis alone.
struct DCapacitySeq : DRuntimeSeq
{
    std::size_t capacity() const;
};

// DKeyedSeq
//   struct: a value_type that is a pair - the keyed restriction.  One flag, no
// change to mu (a sequence of pairs is keyed and wears no uniqueness), and the
// element stays assignable so the iteration mode stays non-const.
struct DKeyedSeq
{
    using value_type     = std::pair<int, int>;
    using iterator       = value_type*;
    using const_iterator = const value_type*;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    std::size_t size() const;
};


// iv.  lifetime lever
//////////////////////////////////////////

// DCompileSeq
//   struct: the constexpr-container opt-in tag, and nothing else.  Its lifetime
// is compile-time; its overlay, its iterability MODE and its storage are
// DRuntimeSeq's.
//
//   Note what it also moves, and cannot help moving: the iterability axis reads
// its compile-reachability from the SAME bit the lifetime axis reads (both go
// through is_compile_stage_helper).  So a fixture that is more static is
// necessarily also higher on the iterability lattice's stage component - the two
// axes are not independent, and tests_axis_coupling says so outright.
struct DCompileSeq : DRuntimeSeq
{
    using is_constexpr_container = std::true_type;
};


// v.   iterability lattice
//////////////////////////////////////////
//   Five elements: not-iterable at the bottom, then the product of
// compile-reachable x non-const-reachable.  The two MIXED kinds - compile/const
// and runtime/non-const - are incomparable, which is the whole reason the axis is
// a lattice and not a chain.  There is a fixture for each of the five.

// DNotIterable
//   struct: countable - it has a size() and a value_type, so the multiplicity
// axis calls it a sequence - and NOT traversable.  The bottom of the lattice.
//
//   It is also the second half of the finding: pit it against `DNotAContainer`
// and every axis but discipline comes out equivalent, so the bogus `greater` on
// discipline survives the meet.  See tests_refinement_non_container.
struct DNotIterable
{
    using value_type = int;

    std::size_t size() const;
};

// DConstSeq
//   struct: iterable, runtime stage, CONST-only mode.  Lattice cell (runtime,
// const) - the least capable of the four iterable kinds.
struct DConstSeq
{
    using value_type     = int;
    using const_iterator = const int*;

    const_iterator begin() const;
    const_iterator end() const;

    std::size_t size() const;
};

// DCompileConstSeq
//   struct: iterable, COMPILE stage, CONST-only mode.  Lattice cell (compile,
// const) - one of the two MIXED kinds.
//
//   Against DRuntimeSeq, which is the OTHER mixed kind (runtime, non-const), it
// is more static and less capable.  Neither refines the other, and
// DIterabilityRelation must say `incomparable`.  A "capability" axis that
// collapsed the two bits into one integer would have made this an ordering, and
// would have been wrong.
struct DCompileConstSeq : DConstSeq
{
    using is_constexpr_container = std::true_type;
};


// vi.  storage levers
//////////////////////////////////////////
//   An EQUIVALENCE axis: content is invariant under siting, so the verdict is only
// `equivalent` or `incomparable` and never an ordering.  storage_of is opt-in
// only, which has a consequence worth knowing: nothing in the standard library
// declares it, so every stdlib type reports `unknown` and the axis is INERT for
// all of them.  See tests_storage_inert.

// static_seq
//   struct: declares static (in-object) siting.
struct static_seq : DRuntimeSeq
{
    static constexpr storage_duration::value storage_duration_category =
        storage_duration::static_storage;
};

// dynamic_seq
//   struct: declares dynamic (allocated) siting.
struct dynamic_seq : DRuntimeSeq
{
    static constexpr storage_duration::value storage_duration_category =
        storage_duration::dynamic_storage;
};

// DHybridSeq
//   struct: declares both - small-storage optimization.  The TOP of the storage
// lattice, and still only `equivalent` or `incomparable` against the others,
// because the axis is an equivalence and does not order its own points.
struct DHybridSeq : DRuntimeSeq
{
    static constexpr storage_duration::value storage_duration_category =
        storage_duration::hybrid_storage;
};


// vii. spellings
//////////////////////////////////////////
//   Every trait here is BINARY, so every check macro takes two type arguments -
// and a macro argument containing a top-level comma is TWO arguments.  Only
// parentheses protect a comma; a template argument list does not.  So
// `std::array<int, 3>` cannot be passed to a two-argument macro at all, and the
// variadic trick that saves a UNARY macro does not save a binary one - it can only
// rescue the LAST parameter.
//
//   Aliasing is the fix, and it reads better than the alternative would have.
using std_vector    = std::vector<int>;
using std_list      = std::list<int>;
using std_deque     = std::deque<int>;
using std_array     = std::array<int, 3>;
using std_array_64  = std::array<int, 64>;
using std_str       = std::string;
using std_set       = std::set<int>;
using std_multiset  = std::multiset<int>;
using std_map       = std::map<int, int>;
using DStdUnordSet = std::unordered_set<int>;
using std_vec_vec   = std::vector<std::vector<int>>;

using fn_type    = int(int);
using arr_type   = int[3];
using lref_type  = int&;
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                III. TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// I.   comparison relations
bool tests_order_relation_enum();
bool tests_order_relation_name();
bool tests_order_relation_meet_table();
bool tests_order_relation_meet_laws();

// II.  content: the native level lambda
bool tests_content_level_enum();
bool tests_content_level_name();
bool tests_content_level_rank();
bool tests_content_level_coarser();
bool tests_native_content_level_rungs();
bool tests_native_content_level_precedence();
bool tests_native_content_level_value_companion();

// III. discipline: the overlay strength order
bool tests_discipline_equivalent();
bool tests_discipline_ordered();
bool tests_discipline_incomparable();
bool tests_discipline_non_container();

// IV.  realization preorders
bool tests_lifetime_relation();
bool tests_lifetime_is_total();
bool tests_iterability_bottom();
bool tests_iterability_lattice();
bool tests_iterability_mixed_kinds();
bool tests_storage_relation();
bool tests_storage_inert();

// V.   the comparison profile
bool tests_profile_members();
bool tests_profile_agrees_with_axes();
bool tests_refinement_composition();
bool tests_refinement_non_container();
bool tests_realization_equal();
bool tests_realization_comparable();
bool tests_profile_value_companions();

// VI.  the order-theoretic laws  (cross-cutting)
bool tests_law_reflexivity();
bool tests_law_direction();
bool tests_law_transitivity();
bool tests_law_content_excluded();
bool tests_axis_coupling();

// VII. robustness  (cross-cutting)
bool tests_cvref_axes();
bool tests_cvref_profile();
bool tests_hostile_types();
bool tests_incomplete_type();
bool tests_trait_shape();

// VIII. standard library
bool tests_stdlib_content_rungs();
bool tests_stdlib_sequences();
bool tests_stdlib_associative();
bool tests_stdlib_refinement();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// container_comparison_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
container_comparison_traits_spec()
{
    return dt::module_spec{
        "container_comparison_traits",
        "The comparison of two container TYPES: the content hierarchy, the "
        "discipline strength order, the three realization preorders, the "
        "product refinement they fold into - and the order-theoretic laws "
        "that fold has to obey",
        {
            dt::block_spec{
                "relations",
                "Comparison relations",
                {
                    { "tests_order_relation_enum",
                      "The order_relation enum itself",
                      &tests_order_relation_enum },
                    { "tests_order_relation_name",
                      "order_relation_name - the stable spelling",
                      &tests_order_relation_name },
                    { "tests_order_relation_meet_table",
                      "order_relation_meet - ALL SIXTEEN CELLS",
                      &tests_order_relation_meet_table },
                    { "tests_order_relation_meet_laws",
                      "The meet is not merely a table - it is an algebraic "
                      "operation, and the refinement folds four axes through "
                      "it in a fixed left-nested order: meet( meet( "
                      "meet(discipline, lifetime), iterability ), storage ) "
                      "If the meet were not ASSOCIATIVE and COMMUTATIVE, that "
                      "order would matter, and the answer would depend on "
                      "which axis the author happened to write first",
                      &tests_order_relation_meet_laws },
                }
            },
            dt::block_spec{
                "content",
                "Content: the native level lambda",
                {
                    { "tests_content_level_enum",
                      "checks std::is_enum<content_level>::value",
                      &tests_content_level_enum },
                    { "tests_content_level_name",
                      "content_level_name - the stable spelling",
                      &tests_content_level_name },
                    { "tests_content_level_rank",
                      "content_level_rank - fineness as an integer",
                      &tests_content_level_rank },
                    { "tests_content_level_coarser",
                      "content_level_coarser - the rung at which two types "
                      "compare",
                      &tests_content_level_coarser },
                    { "tests_native_content_level_rungs",
                      "native_content_level_of - the rung at which a type "
                      "NATIVELY compares",
                      &tests_native_content_level_rungs },
                    { "tests_native_content_level_precedence",
                      "THE PRECEDENCE RULE",
                      &tests_native_content_level_precedence },
                    { "tests_native_content_level_value_companion",
                      "native_content_level_of_v",
                      &tests_native_content_level_value_companion },
                }
            },
            dt::block_spec{
                "discipline",
                "Discipline: the overlay strength order",
                {
                    { "tests_discipline_equivalent",
                      "discipline_relation - mutual subsumption is "
                      "discipline-EQUALITY",
                      &tests_discipline_equivalent },
                    { "tests_discipline_ordered",
                      "discipline_relation - `greater` is the LEFT more "
                      "restrictive: it wears every restriction the right "
                      "does, and more",
                      &tests_discipline_ordered },
                    { "tests_discipline_incomparable",
                      "discipline_relation - `incomparable` is a genuine "
                      "mismatch: each type wears a restriction the other does "
                      "not, so neither overlay contains the other",
                      &tests_discipline_incomparable },
                    { "tests_discipline_non_container",
                      "THE FINDING",
                      &tests_discipline_non_container },
                }
            },
            dt::block_spec{
                "realization",
                "Realization preorders",
                {
                    { "tests_lifetime_relation",
                      "lifetime_relation - staticity, as a TWO-POINT total "
                      "order with the compile-time stage the stronger",
                      &tests_lifetime_relation },
                    { "tests_lifetime_is_total",
                      "lifetime_relation is a TOTAL order on two points - it "
                      "can never be incomparable",
                      &tests_lifetime_is_total },
                    { "tests_iterability_bottom",
                      "DIterabilityRelation - the BOTTOM of the lattice: "
                      "not-iterable",
                      &tests_iterability_bottom },
                    { "tests_iterability_lattice",
                      "DIterabilityRelation - the four iterable kinds, as the "
                      "PRODUCT of two bits: compile-reachable  x  "
                      "non-const-reachable const          non-const runtime "
                      "stage        (F, F)  DConstSeq       (F, T)  "
                      "DRuntimeSeq compile stage        (T, F)  compile_const "
                      "  (T, T)  DCompileSeq",
                      &tests_iterability_lattice },
                    { "tests_iterability_mixed_kinds",
                      "THE REASON THE AXIS IS A LATTICE",
                      &tests_iterability_mixed_kinds },
                    { "tests_storage_relation",
                      "storage_relation - an EQUIVALENCE axis",
                      &tests_storage_relation },
                    { "tests_storage_inert",
                      "THE FINDING",
                      &tests_storage_inert },
                }
            },
            dt::block_spec{
                "profile",
                "The comparison profile",
                {
                    { "tests_profile_members",
                      "DComparisonProfile - the eight members it exposes",
                      &tests_profile_members },
                    { "tests_profile_agrees_with_axes",
                      "The profile must be a faithful restatement of its axes "
                      "- a convenience that can disagree with what it "
                      "summarizes is worse than no convenience",
                      &tests_profile_agrees_with_axes },
                    { "tests_refinement_composition",
                      "refinement_of - the product over the four "
                      "type-decidable axes",
                      &tests_refinement_composition },
                    { "tests_refinement_non_container",
                      "THE FINDING, ESCAPING",
                      &tests_refinement_non_container },
                    { "tests_realization_equal",
                      "realization_equal - true iff the refinement is "
                      "`equivalent`: the two types agree on discipline and on "
                      "every realization axis",
                      &tests_realization_equal },
                    { "tests_realization_comparable",
                      "realization_comparable - true iff the two are ORDERED "
                      "(or equal) on the refinement: not incomparable",
                      &tests_realization_comparable },
                    { "tests_profile_value_companions",
                      "The `_v` companions of section V",
                      &tests_profile_value_companions },
                }
            },
            dt::block_spec{
                "laws",
                "The order-theoretic laws",
                {
                    { "tests_law_reflexivity",
                      "REFLEXIVITY",
                      &tests_law_reflexivity },
                    { "tests_law_direction",
                      "THE DIRECTION CONTRACT",
                      &tests_law_direction },
                    { "tests_law_transitivity",
                      "TRANSITIVITY",
                      &tests_law_transitivity },
                    { "tests_law_content_excluded",
                      "CONTENT IS NOT IN THE REFINEMENT - and that is "
                      "deliberate",
                      &tests_law_content_excluded },
                    { "tests_axis_coupling",
                      "THE AXES ARE NOT INDEPENDENT, and the product order "
                      "assumes they are",
                      &tests_axis_coupling },
                }
            },
            dt::block_spec{
                "robustness",
                "Robustness  (cv-ref, hostile types, completeness, trait "
                "shape)",
                {
                    { "tests_cvref_axes",
                      "Every axis must answer identically for T, const T, "
                      "volatile T, const volatile T, T&, const T&, T&& and "
                      "const T&& - on BOTH operands",
                      &tests_cvref_axes },
                    { "tests_cvref_profile",
                      "DComparisonProfile cleans BOTH operands - it declares "
                      "`using L = clean_t<_Left>` and `using R = "
                      "clean_t<_Right>` and passes those to every axis",
                      &tests_cvref_profile },
                    { "tests_hostile_types",
                      "Every trait must survive every adversarial shape - "
                      "void, function types, arrays, references, member "
                      "pointers, enums, and the class-type adversaries - and "
                      "must REPORT rather than DIAGNOSE",
                      &tests_hostile_types },
                    { "tests_incomplete_type",
                      "THE COMPLETENESS BOUNDARY",
                      &tests_incomplete_type },
                    { "tests_trait_shape",
                      "Is each trait well-shaped?",
                      &tests_trait_shape },
                }
            },
            dt::block_spec{
                "stdlib",
                "Standard library",
                {
                    { "tests_stdlib_content_rungs",
                      "Where the standard library sits on the content "
                      "hierarchy",
                      &tests_stdlib_content_rungs },
                    { "tests_stdlib_sequences",
                      "The standard sequences against each other",
                      &tests_stdlib_sequences },
                    { "tests_stdlib_associative",
                      "The associative containers",
                      &tests_stdlib_associative },
                    { "tests_stdlib_refinement",
                      "The whole module, on the standard library, as a caller "
                      "would read it",
                      &tests_stdlib_refinement },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_CONTAINER_COMPARISON_TRAITS_TESTS_
