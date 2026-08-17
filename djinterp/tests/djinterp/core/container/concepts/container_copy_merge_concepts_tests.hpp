/******************************************************************************
* djinterp [tests]                    container_copy_merge_concepts_tests.hpp
*
*   The DTest suite for container_copy_merge_concepts.hpp - the `requires`-
* facing view of container_copy_merge_traits.hpp.  The module under test defines
* exactly four concepts and claims, in its own preamble, that "THE CONCEPTS ADD
* NO POLICY.  Each is exactly its trait".  This suite exists to hold that claim
* to account, and it does so along six axes:
*
*     SATISFACTION   is each concept true of the types it should be true of, and
*                    false of the rest?                          (I - IV)
*     CV-REF         does each answer identically for _Type, const _Type&, and
*                    the other six qualified forms?  Every concept spells
*                    clean_t<...> around its trait, so this is a claim it makes
*                    structurally and must therefore honour.     (I - IV)
*     HOSTILITY      does each SURVIVE - compile and answer false for - the
*                    shapes nobody classified?  Run through count_holds, which
*                    instantiates every cell, so a concept whose definition is
*                    secretly ill-formed for a hostile shape breaks the build
*                    rather than hiding.                          (I - IV)
*     AGREEMENT      does the concept face give the same answer as the trait
*                    face, at a point, across a battery, across the hostile zoo,
*                    and across cv-ref - non-vacuously?                (V)
*     ORDERING       do the four stand in the SUBSUMPTION order their names
*                    imply, or are they merely related by implication?  (VI)
*     PARITY         which of the traits module's bool predicates have a concept
*                    face at all?                                       (V)
*
*   The concepts are a generated face, so most of the suite is a parity check by
* construction.  What is NOT by construction - and what the suite is really for
* - is the handful of places where the forwarding is lossy, silent, or simply
* wrong.  Seven such findings are recorded below.  Every one of them is asserted
* AS OBSERVED: the suite tests what the module does, never what its comments say
* it does, and the disagreements between the two are named here rather than
* quietly normalised into passing tests.
*
*
* FINDINGS
* ========
*
*   F1.  merge_may_overflow_into TAKES ITS OPERANDS IN THE OPPOSITE ORDER FROM
*        THE ONE ITS NAME AND COMMENT DESCRIBE.  The concept is spelled
*
*            template<typename _From, typename _To>
*            concept merge_may_overflow_into =
*                merge_may_overflow<clean_t<_From>, clean_t<_To>>::value;
*
*        and documented as "the merge CAN exceed _To's capacity.  Bounded
*        targets need this checked".  But merge_may_overflow reads its capacity
*        signals off merge_result_type_t<_Left, _Right>, which is clean_t<_Left>
*        - so the trait answers about the LEFT operand, and the concept
*        therefore answers about _From, the SOURCE, not _To, the target.
*
*        Concretely, with cm_fixed_capacity_seq bounded and cm_plain_seq not:
*
*            merge_may_overflow_into<cm_fixed_capacity_seq, cm_plain_seq> == true
*            merge_may_overflow_into<cm_plain_seq, cm_fixed_capacity_seq> == false
*
*        Read as the comment intends ("merge the first into the second"), both
*        lines are backwards: merging into an unbounded target cannot overflow,
*        and merging into a bounded one can.  The trait is self-consistent and
*        correctly documented ("The result type (the left operand) decides it");
*        it is the concept's _From / _To naming that imposes a directional
*        reading the forwarding does not honour.  Two repairs are available and
*        the suite deliberately does not choose between them:
*
*            (a) swap the arguments  - merge_may_overflow<clean_t<_To>,
*                                      clean_t<_From>> - keeping the name; or
*            (b) rename the parameters to _Left / _Right and the concept to
*                merge_may_overflow_with, keeping the forwarding.
*
*        tests_ccmc_overflow_reads_left_operand pins the OBSERVED behaviour, so
*        it will fail the day either repair lands - which is the point.  See
*        also tests_ccmc_mergeable_symmetry: mergeable_with and
*        merge_elements_compatible_with are both symmetric in their operands, so
*        their own _From / _To naming is harmless.  merge_may_overflow_into is
*        the only asymmetric member of the four, and so the only one for which
*        operand order is load-bearing.
*
*   F2.  copyable_container CANNOT BE ASKED ABOUT AN INCOMPLETE TYPE.  It routes
*        through std::is_copy_constructible, which the standard mandates be
*        given a complete type, and the conjunction in is_copyable_container
*        names that trait unconditionally - short-circuiting does not prevent
*        the instantiation.  Asking copyable_container<fixtures::incomplete>
*        therefore fails the BUILD, not the report.  The requirement is
*        inherited rather than chosen, but it is real, so the copy battery runs
*        over D_TEST_HOSTILE_TYPES_COMPLETE while the three merge concepts - none
*        of which touches a completeness-requiring trait - run over the full
*        D_TEST_HOSTILE_TYPES.  tests_ccmc_copy_hostile states the requirement
*        as a finding.
*
*   F3.  TWO ELEMENTLESS ITERABLE RANGES ARE MUTUALLY ELEMENT-COMPATIBLE, AND
*        THEREFORE MERGEABLE.  element_type_of_helper yields `void` for a type
*        with no nested value_type, and std::is_same<void, void> is true - so
*        merge_elements_compatible's element clause is satisfied by two ranges
*        that have no elements at all.  The only thing standing between that and
*        a false positive is the is_iterable_container guard, which member
*        begin()/end() detection makes narrow enough to exclude every hostile
*        fixture.  A type that passes the guard and still has no value_type -
*        cm_elementless_range here - lands in the gap:
*
*            merge_elements_compatible_with<cm_elementless_range,
*                                           cm_elementless_range> == true
*            mergeable_with<cm_elementless_range,
*                           cm_elementless_range>                 == true
*
*        The gap is self-contained: an elementless range is NOT compatible with
*        any container that does have elements, because is_convertible<void, T>
*        and is_convertible<T, void> are both false for non-void T.  So this is
*        a curiosity rather than a hazard, and it is recorded rather than
*        worked around.  tests_ccmc_elements_void_element pins it.
*
*   F4.  THE FOUR CONCEPTS ARE SIBLINGS, NOT A REFINEMENT LADDER.  By
*        SATISFACTION they form a chain - merge_may_overflow_into implies
*        mergeable_with implies merge_elements_compatible_with, each because the
*        underlying trait tests the next one first.  By SUBSUMPTION they are
*        unordered, because each is spelled as a fresh atomic constraint (a
*        `::value` expression) rather than in terms of the one below it.  This
*        is exactly the implication-is-not-subsumption trap test_concept.hpp
*        section III exists to catch, and the consequence is concrete: two
*        overloads constrained on any two of these four are AMBIGUOUS for every
*        type satisfying both, and no amount of "the stronger one should win"
*        will make the compiler prefer either.  Section VI reports the ordering
*        as observed (unordered at a common witness) and pins it, so the day
*        someone respells one concept in terms of another the change is visible.
*
*   F5.  THE CONCEPT FACE COVERS FOUR OF THE TRAIT MODULE'S SIX BOOL PREDICATES.
*        Present: is_copyable_container, merge_elements_compatible, is_mergeable,
*        merge_may_overflow.  Absent: copy_preserves_all_axes (the spec note) and
*        merge_has_key_conflict (the keyed-merge proviso).  The latter is the
*        substantive gap - key conflict is one of the three provisos the traits
*        preamble names, and it is the only one with no `requires`-facing form,
*        so a template that wants to constrain on it must still reach for the
*        trait.  Separately, merge_may_overflow is not a standard-shaped bool
*        trait at all: it is a plain struct carrying `static constexpr bool
*        value`, with no value_type, no nested type, and no bool_constant base,
*        so is_bool_trait rejects it where the other five pass.
*        tests_ccmc_agree_face_parity and tests_ccmc_agree_trait_shape record
*        both.
*
*   F6.  CV-REF AGREEMENT BETWEEN CONCEPT AND TRAIT HOLDS ONLY BY THE
*        DELEGATES' GRACE.  merge_elements_compatible cleans its two container
*        arguments before the is_iterable_container guard but passes _Left and
*        _Right RAW to elements_same_type / elements_convertible.  It is cv-ref
*        agnostic anyway - solely because those three traits clean internally.
*        The concept, which cleans up front, would silently diverge from its
*        trait on `const V&` the moment any of them stopped doing so.  Nothing
*        in either header states the dependency, so tests_ccmc_agree_cvref pins
*        it: agreement is asserted across all eight cv-ref forms, not just at
*        the bare type.
*
*   F7.  A SEQUENCE OF PAIRS IS CLASSIFIED AS A KEYED DISCIPLINE, AND MERGES BY
*        KEY WITH A std::map.  merge_discipline_of tests the KEYED signal - "the
*        value_type is a pair" - before the equivalence signal, and the keyed
*        test asks nothing about a key_type.  So:
*
*            merge_discipline_of<std::vector<std::pair<int, int>>> == multimap
*            merge_discipline_of<std::set<std::pair<int, int>>>    == map
*
*        A std::vector of pairs is read as a multimap because vector has no
*        single-argument insert to mark it unique; a std::set of pairs is read
*        as a map because set's insert returns a `.second`.  Since
*        std::pair<int, int> converts to std::pair<const int, int>, the element
*        clause is satisfied too, and the consequence is
*
*            mergeable_with<std::vector<std::pair<int, int>>,
*                           std::map<int, int>>            == true   (keyed_merge)
*
*        - an ordinary sequence merging by key with a map.  This follows
*        DIRECTLY from the signal the traits header documents ("a value_type
*        that is a pair is the KEYED mark (map / multimap)"), so it is a
*        consequence of the probe rather than a bug in its implementation.  What
*        it puts in question is the adjacent claim that "the verdicts agree with
*        the axis traits on the familiar containers": vector<pair<K, V>> is a
*        familiar container and its overlay-axis reading is very unlikely to be
*        multimap.  Reconciling the two is a follow-up this suite cannot settle,
*        container_overlay_traits.hpp not being in its dependency set;
*        tests_ccmc_mergeable_pair_valued pins the observed behaviour in the
*        meantime, so whichever way the reconciliation goes it is a deliberate
*        change and not a silent one.
*
*
* BUILD PREREQUISITE
* ==================
*   None.  container_copy_merge_concepts.hpp, container_copy_merge_traits.hpp,
* and their transitive dependencies compile as shipped; F1 - F6 are behavioural
* and shape findings, not build breaks.  No file under inc/ is modified by this
* delivery.
*
*
* SUITE SHAPE
* ===========
*   Seven blocks, one translation unit each, named for the .cpp that defines
* them:
*
*     gate             the language gate - the only block outside the C++20
*                      guard, so the suite reports something on every build
*     copy             copyable_container
*     merge_elements   merge_elements_compatible_with
*     mergeable        mergeable_with
*     overflow         merge_may_overflow_into
*     agreement        concept face vs trait face, and the parity map
*     ordering         subsumption, implication, and the ambiguity trap
*
*
* path:      /tests/djinterp/core/container/concepts/
*                                    container_copy_merge_concepts_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.29
******************************************************************************/

#ifndef DJINTERP_TESTS_CONTAINER_COPY_MERGE_CONCEPTS_TESTS_
#define DJINTERP_TESTS_CONTAINER_COPY_MERGE_CONCEPTS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <array>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "container_copy_merge_concepts.hpp"    // the header under test
#include "djinterp/test/test_concept.hpp"       // the concept-testing toolkit
#endif  // !DTEST_SPEC_MODE
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module
#endif  // DTEST_SPEC_MODE


NS_DJINTERP
NS_TESTING

// dt names the framework entities the suite stands on (djinterp::test).
// Declared UNCONDITIONALLY: the spec provider needs dt::module_spec, and the
// fixtures need the trait / concept toolkit.
namespace dt = ::djinterp::test;


// D_CM_CONCEPTS_ENABLED
//   macro: the gate the module under test self-suppresses below.  Spelled here
// once, as the identical condition, so the suite's blocks and the header's
// contents cannot drift apart.
#define D_CM_CONCEPTS_ENABLED                                                 \
    ( D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS )

// D_CM_CHECK
//   macro: evaluate a condition once; on failure print the stringised
// expression with its location and early-return false from the enclosing test.
// Variadic so a condition containing top-level commas - which every
// `holds_for_all<trait, A, B>::value` does, the preprocessor not knowing angle
// brackets from grouping - survives intact.  Two suite-unique letters (CM) so
// co-compiled suites never collide.  Depends on nothing from the header under
// test, so it sits OUTSIDE the fixture guard.
#define D_CM_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("      [FAIL] %s\n             %s:%d\n",              \
                        #__VA_ARGS__,                                         \
                        __FILE__,                                             \
                        __LINE__);                                            \
                                                                              \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (false)

// D_CM_NOTE
//   macro: print a finding line from inside a passing test, so a result the
// suite RECORDS rather than merely asserts reaches the console next to the
// assertion that pinned it.
#define D_CM_NOTE(TEXT)                                                       \
    std::printf("      [NOTE] %s\n", (TEXT))


#ifndef DTEST_SPEC_MODE  // ---- fixtures: normal mode only ------------------

///////////////////////////////////////////////////////////////////////////////
///                FIXTURES  (containers by discipline and bound)            ///
///////////////////////////////////////////////////////////////////////////////
//
//   The std containers cover the six disciplines the merge axis reads, and the
// traits preamble states its local signals agree with the axis traits on them -
// so they are the right witnesses for the ordinary cases.  What they cannot
// supply are the three boundedness signals that are not std conventions
// (`extent`, static interval bounds, a capacity() with no reserve()), the
// unique-insert split read in isolation, and the shapes that sit in the gaps
// (an iterable range with no value_type, an iterable range that cannot be
// copied, a value_type-bearing type with no traversal at all).  Those are
// hand-rolled below.
//
//   Every fixture DECLARES and does not DEFINE its members: the traits name
// them only in unevaluated operands, so nothing is odr-used and nothing needs
// to link.

// cm_plain_seq
//   struct: the minimal iterable sequence - value_type plus a begin()/end()
// pair, no capacity signal of any kind.  The unbounded baseline.
struct cm_plain_seq
{
    using value_type = int;

    int* begin();

    int* end();
};

// cm_extent_seq
//   struct: a sequence bounded by a static `extent` constant - the first of the
// four boundedness signals merge_may_overflow reads.
struct cm_extent_seq
{
    using value_type = int;

    static constexpr std::size_t extent = 8;

    int* begin();

    int* end();
};

// cm_interval_seq
//   struct: a sequence bounded by static lower_bound / upper_bound - the
// finite closed-interval domain signal.
struct cm_interval_seq
{
    using value_type = int;

    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 15;

    int* begin();

    int* end();
};

// cm_fixed_capacity_seq
//   struct: a sequence with a const-callable capacity() and NO reserve() - a
// capacity that means a fixed bound.
struct cm_fixed_capacity_seq
{
    using value_type = int;

    int* begin();

    int* end();

    std::size_t capacity() const;
};

// cm_growable_seq
//   struct: capacity() AND reserve() - the anti-signal pair.  Identical to
// cm_fixed_capacity_seq but for the reserve(), so the two isolate exactly the
// clause that keeps a capacity() from meaning a bound.
struct cm_growable_seq
{
    using value_type = int;

    int* begin();

    int* end();

    std::size_t capacity() const;

    void reserve(std::size_t _n);
};

// cm_elementless_range
//   struct: iterable - it has begin()/end() - but carries no value_type, so its
// element type degrades to void.  The witness for F3.
struct cm_elementless_range
{
    int* begin();

    int* end();
};

// cm_noncopyable_seq
//   struct: an iterable sequence whose copy constructor is deleted - the
// negative that separates copyable_container's two clauses.
struct cm_noncopyable_seq
{
    using value_type = int;

    cm_noncopyable_seq();

    cm_noncopyable_seq(const cm_noncopyable_seq& _other) = delete;

    int* begin();

    int* end();
};

// cm_positional
//   struct: a value_type and positional access, but NO traversal - the
// non-iterable container.  Fails the container guard every concept rests on
// while still looking like a container to a value_type-only probe.
struct cm_positional
{
    using value_type = int;

    std::size_t size() const;

    int& operator[](std::size_t _index);
};

// cm_unique_bag
//   struct: key_type present, single-element insert returning a `.second` -
// the unkeyed UNIQUE discipline (set) read from the local signals alone.
struct cm_unique_bag
{
    using value_type = int;
    using key_type   = int;

    int* begin();

    int* end();

    std::pair<int*, bool> insert(const int& _value);
};

// cm_repeat_bag
//   struct: key_type present, insert returning a bare iterator - the unkeyed
// REPEATABLE discipline (multiset).  Differs from cm_unique_bag only in the
// insert return, which is the whole of the multiplicity split.
struct cm_repeat_bag
{
    using value_type = int;
    using key_type   = int;

    int* begin();

    int* end();

    int* insert(const int& _value);
};

// cm_unique_keyed
//   struct: a pair value_type and a unique insert - the KEYED unique
// discipline (map).
struct cm_unique_keyed
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    value_type* begin();

    value_type* end();

    std::pair<value_type*, bool> insert(const value_type& _entry);
};

// cm_repeat_keyed
//   struct: a pair value_type and an iterator-returning insert - the KEYED
// repeatable discipline (multimap).
struct cm_repeat_keyed
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    value_type* begin();

    value_type* end();

    value_type* insert(const value_type& _entry);
};

// cm_bounded_bag
//   struct: a set discipline carrying a fixed capacity - the witness that the
// overflow proviso is read off the result type's bound and not off its
// discipline.
struct cm_bounded_bag
{
    using value_type = int;
    using key_type   = int;

    int* begin();

    int* end();

    std::size_t capacity() const;

    std::pair<int*, bool> insert(const int& _value);
};

// cm_from_int
//   struct: constructible from int, and convertible to nothing - the ONE-WAY
// element conversion that exercises each disjunct of the element clause
// separately.
struct cm_from_int
{
    cm_from_int(int _value);
};

// cm_opaque
//   struct: convertible neither to nor from int - the element type that makes
// two otherwise-identical sequences element-INCOMPATIBLE.
struct cm_opaque
{};


#if D_CM_CONCEPTS_ENABLED

///////////////////////////////////////////////////////////////////////////////
///                LIFTS  (concept -> trait, for the quantifiers)            ///
///////////////////////////////////////////////////////////////////////////////
//
//   A concept is not a template argument, so none of test_traits.hpp's
// machinery - the non-short-circuiting quantifiers, the cv-ref report, the
// hostile fixture lists - can receive one directly.  Lifting each concept into
// a one-parameter bool_constant trait opens all of it at once.

// cm_copyable_c
//   trait: copyable_container, lifted.
D_TEST_CONCEPT_TRAIT(cm_copyable_c, ::djinterp::copyable_container)

// cm_mergeable_c
//   trait: mergeable_with, lifted (binary).
D_TEST_CONCEPT_TRAIT_2(cm_mergeable_c, ::djinterp::mergeable_with)

// cm_elements_c
//   trait: merge_elements_compatible_with, lifted (binary).
D_TEST_CONCEPT_TRAIT_2(cm_elements_c,
                       ::djinterp::merge_elements_compatible_with)

// cm_overflow_c
//   trait: merge_may_overflow_into, lifted (binary).
D_TEST_CONCEPT_TRAIT_2(cm_overflow_c, ::djinterp::merge_may_overflow_into)

//   The three binary lifts are bound to the diagonal (_Type merged with itself)
// so they present the one-parameter shape the quantifiers and the cv-ref report
// take.  Spelled as structs rather than alias templates so they are usable as
// template template arguments without relying on alias deduction.

// cm_mergeable_self_c
//   trait: mergeable_with<_Type, _Type>, as a one-parameter trait.
template<typename _Type>
struct cm_mergeable_self_c : cm_mergeable_c<_Type, _Type>
{};

// cm_elements_self_c
//   trait: merge_elements_compatible_with<_Type, _Type>, one-parameter.
template<typename _Type>
struct cm_elements_self_c : cm_elements_c<_Type, _Type>
{};

// cm_overflow_self_c
//   trait: merge_may_overflow_into<_Type, _Type>, one-parameter.
template<typename _Type>
struct cm_overflow_self_c : cm_overflow_c<_Type, _Type>
{};


///////////////////////////////////////////////////////////////////////////////
///                AGREEMENT DECLARATORS  (concept face vs trait face)       ///
///////////////////////////////////////////////////////////////////////////////
//
//   Each turns "does the concept answer what the trait answers" into a trait
// the quantifiers can carry across a battery, the hostile zoo, and - through
// trait_ignores_cvref - all eight cv-ref forms.  Agreement alone is vacuously
// satisfied by two predicates that are both uniformly false, so every use of
// these is paired with a polarity pin in tests_ccmc_agree_nonvacuous.

// cm_copyable_agree
//   trait: is_copyable_container and copyable_container agree at _Type.
D_TEST_DECLARE_AGREEMENT(cm_copyable_agree,
                         ::djinterp::is_copyable_container,
                         ::djinterp::copyable_container)

// cm_elements_agree
//   trait: merge_elements_compatible and merge_elements_compatible_with agree.
D_TEST_DECLARE_AGREEMENT_2(cm_elements_agree,
                           ::djinterp::merge_elements_compatible,
                           ::djinterp::merge_elements_compatible_with)

// cm_mergeable_agree
//   trait: is_mergeable and mergeable_with agree at the pair.
D_TEST_DECLARE_AGREEMENT_2(cm_mergeable_agree,
                           ::djinterp::is_mergeable,
                           ::djinterp::mergeable_with)

// cm_overflow_agree
//   trait: merge_may_overflow and merge_may_overflow_into agree at the pair.
// Note this pins the FORWARDING, not the semantics: F1 is a defect in what the
// concept's operand names promise, and the concept forwards to the trait
// faithfully, so this agreement holds and should.
D_TEST_DECLARE_AGREEMENT_2(cm_overflow_agree,
                           ::djinterp::merge_may_overflow,
                           ::djinterp::merge_may_overflow_into)

// cm_elements_agree_self / cm_mergeable_agree_self / cm_overflow_agree_self
//   trait: the three binary agreements bound to the diagonal, so they reach the
// one-parameter quantifiers and the cv-ref report.
template<typename _Type>
struct cm_elements_agree_self : cm_elements_agree<_Type, _Type>
{};

template<typename _Type>
struct cm_mergeable_agree_self : cm_mergeable_agree<_Type, _Type>
{};

template<typename _Type>
struct cm_overflow_agree_self : cm_overflow_agree<_Type, _Type>
{};


///////////////////////////////////////////////////////////////////////////////
///                CONSTRAINTS  (the diagonal, and the restatement trap)     ///
///////////////////////////////////////////////////////////////////////////////
//
//   The ordering report takes one-parameter concepts, so the three binary
// concepts are bound to the diagonal by name.  Binding through D_TEST_CONSTRAINT
// preserves the constraint's SPELLING - each of these normalises to the atomic
// constraint of the concept it names - which is exactly what subsumption
// consults, and therefore what section VI is measuring.

// cm_mergeable_self
//   concept: mergeable_with<_Type, _Type>.
D_TEST_CONSTRAINT(cm_mergeable_self,
                  ::djinterp::mergeable_with<_Type, _Type>)

// cm_elements_self
//   concept: merge_elements_compatible_with<_Type, _Type>.
D_TEST_CONSTRAINT(cm_elements_self,
                  ::djinterp::merge_elements_compatible_with<_Type, _Type>)

// cm_overflow_self
//   concept: merge_may_overflow_into<_Type, _Type>.
D_TEST_CONSTRAINT(cm_overflow_self,
                  ::djinterp::merge_may_overflow_into<_Type, _Type>)

// cm_mergeable_restated
//   concept: the SAME satisfaction as cm_mergeable_self, spelled by reaching
// past the concept to the trait it forwards to rather than by naming it.  A
// fresh atomic constraint, so it does not subsume cm_mergeable_self even though
// the two are true of exactly the same types - the restates_a fixture of
// test_concept.hpp section V, made concrete for this module.
D_TEST_CONSTRAINT(cm_mergeable_restated,
                  ::djinterp::is_mergeable<
                      ::djinterp::clean_t<_Type>,
                      ::djinterp::clean_t<_Type>>::value)


///////////////////////////////////////////////////////////////////////////////
///                ORDERING REPORTS  (subsumption, observed)                 ///
///////////////////////////////////////////////////////////////////////////////
//
//   Each emits a carrier trait whose members report the constraint ordering of
// A against B at a witness: code, both, a_subsumes_b, b_subsumes_a, unordered,
// neither.  The direction bools are gated on `both`, so a witness satisfying
// only one concept cannot masquerade as a subsumption result.
//
//   Each also opens an `internal` namespace for its ranked helpers, which is
// why these are invoked here at namespace scope and not inside a test body.

// cm_order_merge_elem
//   trait: mergeable_with against merge_elements_compatible_with.
D_TEST_DECLARE_ORDERING(cm_order_merge_elem,
                        cm_mergeable_self,
                        cm_elements_self)

// cm_order_over_merge
//   trait: merge_may_overflow_into against mergeable_with.
D_TEST_DECLARE_ORDERING(cm_order_over_merge,
                        cm_overflow_self,
                        cm_mergeable_self)

// cm_order_restatement
//   trait: the restatement against the concept it restates.
D_TEST_DECLARE_ORDERING(cm_order_restatement,
                        cm_mergeable_restated,
                        cm_mergeable_self)


///////////////////////////////////////////////////////////////////////////////
///                IMPLICATION  (satisfaction, as against spelling)          ///
///////////////////////////////////////////////////////////////////////////////

// cm_implication_holds
//   trait: at _Type, the satisfaction chain holds - merge_may_overflow_into
// implies mergeable_with, and mergeable_with implies
// merge_elements_compatible_with.  Both hold by construction on the trait side
// (each trait tests the next before answering), and this is the statement that
// the concept face inherits them.  Spelled as a one-parameter trait so the
// claim can be quantified over a battery instead of asserted at a point.
template<typename _Type>
struct cm_implication_holds
    : ::djinterp::bool_constant<
        ( ( (!::djinterp::merge_may_overflow_into<_Type, _Type>)      ||
            ::djinterp::mergeable_with<_Type, _Type> )                &&
          ( (!::djinterp::mergeable_with<_Type, _Type>)               ||
            ::djinterp::merge_elements_compatible_with<_Type, _Type> ) )>
{};

// cm_diagonal_coextensive
//   trait: at _Type, mergeable_with and merge_elements_compatible_with give the
// SAME answer on the diagonal.  They do so for every type, because a merge of
// _Type with itself has trivially agreeing disciplines, leaving only the
// element clause - which is the other concept.  Recorded as a trait so the
// co-extension can be quantified rather than asserted at a point, and so
// section VII can say WHY its ordering report cannot separate
// "subsumption-equivalent" from "incomparable" for this pair.  The two concepts
// are of course distinct OFF the diagonal, which is what makes them two.
template<typename _Type>
struct cm_diagonal_coextensive
    : ::djinterp::bool_constant<
        (    ::djinterp::mergeable_with<_Type, _Type>
          == ::djinterp::merge_elements_compatible_with<_Type, _Type> )>
{};

// cm_restatement_faithful
//   trait: at _Type, the restatement and the concept it restates agree.  True
// everywhere, the two being the same expression evaluated twice - which is
// precisely what makes the ordering result of section VII interesting: identical
// satisfaction, and still no subsumption between them.
template<typename _Type>
struct cm_restatement_faithful
    : ::djinterp::bool_constant<
        ( cm_mergeable_restated<_Type> == cm_mergeable_self<_Type> )>
{};


///////////////////////////////////////////////////////////////////////////////
///                CONSTRAINED PROBES  (the concepts used AS constraints)    ///
///////////////////////////////////////////////////////////////////////////////
//
//   Evaluating `C<T>` as a bool is not the same as using C in a requires-clause,
// and the module's stated purpose is the latter: "spelled so it can constrain a
// template instead of gating one through enable_if".  Each pair below is a
// constrained overload and an unconstrained fallback with otherwise identical
// signatures, so the constrained one is chosen exactly when the concept is
// satisfied and the fallback exactly when it is not.

// cm_copy_probe
//   function: the copyable_container-constrained overload (returns 1).
template<typename _Type>
    requires ::djinterp::copyable_container<_Type>
constexpr int
cm_copy_probe()
{
    return 1;
}

// cm_copy_probe
//   function: the unconstrained fallback (returns 0).
template<typename _Type>
constexpr int
cm_copy_probe()
{
    return 0;
}

// cm_merge_probe
//   function: the mergeable_with-constrained overload (returns 1).
template<typename _From,
         typename _To>
    requires ::djinterp::mergeable_with<_From, _To>
constexpr int
cm_merge_probe()
{
    return 1;
}

// cm_merge_probe
//   function: the unconstrained fallback (returns 0).
template<typename _From,
         typename _To>
constexpr int
cm_merge_probe()
{
    return 0;
}

#endif  // D_CM_CONCEPTS_ENABLED  (fixtures)

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations -- visible in BOTH modes ----------------------------------

// I.   GATE                (container_copy_merge_concepts_tests_gate.cpp)
bool tests_ccmc_language_gate();

#if D_CM_CONCEPTS_ENABLED

// II.  COPY                (container_copy_merge_concepts_tests_copy.cpp)
bool tests_ccmc_copy_positive();
bool tests_ccmc_copy_negative();
bool tests_ccmc_copy_cvref();
bool tests_ccmc_copy_hostile();
bool tests_ccmc_copy_constrains();

// III. MERGE ELEMENTS
//                (container_copy_merge_concepts_tests_merge_elements.cpp)
bool tests_ccmc_elements_same_type();
bool tests_ccmc_elements_convertible();
bool tests_ccmc_elements_incompatible();
bool tests_ccmc_elements_container_guard();
bool tests_ccmc_elements_void_element();
bool tests_ccmc_elements_symmetry();
bool tests_ccmc_elements_cvref();
bool tests_ccmc_elements_hostile();

// IV.  MERGEABLE           (container_copy_merge_concepts_tests_mergeable.cpp)
bool tests_ccmc_mergeable_sequences();
bool tests_ccmc_mergeable_bags();
bool tests_ccmc_mergeable_keyed();
bool tests_ccmc_mergeable_cross_family();
bool tests_ccmc_mergeable_pair_valued();
bool tests_ccmc_mergeable_element_gate();
bool tests_ccmc_mergeable_symmetry();
bool tests_ccmc_mergeable_cvref();
bool tests_ccmc_mergeable_hostile();
bool tests_ccmc_mergeable_constrains();

// V.   OVERFLOW            (container_copy_merge_concepts_tests_overflow.cpp)
bool tests_ccmc_overflow_extent();
bool tests_ccmc_overflow_tuple_size();
bool tests_ccmc_overflow_static_bounds();
bool tests_ccmc_overflow_fixed_capacity();
bool tests_ccmc_overflow_reserve_anti_signal();
bool tests_ccmc_overflow_requires_mergeable();
bool tests_ccmc_overflow_reads_left_operand();
bool tests_ccmc_overflow_cvref();
bool tests_ccmc_overflow_hostile();

// VI.  AGREEMENT           (container_copy_merge_concepts_tests_agreement.cpp)
bool tests_ccmc_agree_copyable();
bool tests_ccmc_agree_elements();
bool tests_ccmc_agree_mergeable();
bool tests_ccmc_agree_overflow();
bool tests_ccmc_agree_cvref();
bool tests_ccmc_agree_nonvacuous();
bool tests_ccmc_agree_trait_shape();
bool tests_ccmc_agree_face_parity();

// VII. ORDERING            (container_copy_merge_concepts_tests_ordering.cpp)
bool tests_ccmc_order_implication();
bool tests_ccmc_order_mergeable_vs_elements();
bool tests_ccmc_order_overflow_vs_mergeable();
bool tests_ccmc_order_restatement_trap();
bool tests_ccmc_order_neither_witness();

#endif  // D_CM_CONCEPTS_ENABLED  (declarations)


// -- the spec provider -- spec mode only ------------------------------------

#ifdef DTEST_SPEC_MODE

// container_copy_merge_concepts_spec
//   function: the authoritative description of this suite - the module, its
// blocks, and every unit test with a body-accurate descriptor.
inline dt::module_spec
container_copy_merge_concepts_spec()
{
    return dt::module_spec{
        "container_copy_merge_concepts",
        "The C++20 concept face of the container COPY and MERGE axis - four "
        "concepts (copyable_container, mergeable_with, "
        "merge_elements_compatible_with, merge_may_overflow_into), each "
        "declared to be exactly its counterpart trait in "
        "container_copy_merge_traits.hpp and to add no policy of its own. The "
        "suite verifies satisfaction, cv-ref invariance, survival of the "
        "hostile type zoo, agreement with the trait face, and the subsumption "
        "ordering the four stand in - and records six findings where the "
        "generated face is lossy, under-covered, or contradicts its own "
        "documentation.",
        {
            dt::block_spec{
                "gate",
                "The language gate. The module under test self-suppresses to "
                "nothing below C++20 with concept support, so every other "
                "block in this suite is compiled under the identical "
                "condition. This block is the one that is not, so a build "
                "below the gate still produces a report rather than an empty "
                "one.",
                {
                    { "tests_ccmc_language_gate",
                      "Asserts the suite and the header agree on the gate: "
                      "above it, C++20 and the concepts feature macro are both "
                      "set and all four concepts are reachable and evaluable; "
                      "below it, records that the header is empty by design "
                      "and skips.",
                      &tests_ccmc_language_gate },
                }
            },
#if D_CM_CONCEPTS_ENABLED
            dt::block_spec{
                "copy",
                "copyable_container - iterable AND copy-constructible. The "
                "concept forwards to is_copyable_container_v, whose two "
                "clauses this block separates: a container that cannot be "
                "copied and a copyable type that is not a container each fail "
                "for their own reason.",
                {
                    { "tests_ccmc_copy_positive",
                      "The std containers (vector, set, map, array, string) "
                      "and the hand-rolled copyable sequences all satisfy "
                      "copyable_container, including the bounded and "
                      "elementless shapes, which are copyable regardless of "
                      "what they carry.",
                      &tests_ccmc_copy_positive },
                    { "tests_ccmc_copy_negative",
                      "Separates the two clauses: cm_noncopyable_seq is "
                      "iterable but has a deleted copy constructor and fails; "
                      "cm_positional has a value_type and no traversal and "
                      "fails the container guard; int and a bare struct fail "
                      "both.",
                      &tests_ccmc_copy_negative },
                    { "tests_ccmc_copy_cvref",
                      "The concept answers identically for all eight cv-ref "
                      "forms of a satisfying type and of a refuting one - "
                      "all() for std::vector<int>, none() for int - and names "
                      "the first disagreeing cell if one ever appears.",
                      &tests_ccmc_copy_cvref },
                    { "tests_ccmc_copy_hostile",
                      "Rejects every fixture in D_TEST_HOSTILE_TYPES_COMPLETE "
                      "while compiling for all of them. Runs over the COMPLETE "
                      "list rather than the full zoo, and records why: finding "
                      "F2, that copyable_container inherits "
                      "std::is_copy_constructible's completeness mandate and "
                      "so cannot be asked about an incomplete type at all.",
                      &tests_ccmc_copy_hostile },
                    { "tests_ccmc_copy_constrains",
                      "The concept works as a constraint and not merely as a "
                      "bool: a copyable_container-constrained overload is "
                      "chosen over an otherwise identical unconstrained one "
                      "for a satisfying type, and the fallback is chosen for a "
                      "refuting one.",
                      &tests_ccmc_copy_constrains },
                }
            },
            dt::block_spec{
                "merge_elements",
                "merge_elements_compatible_with - the element half of "
                "mergeability, constraining the value types without committing "
                "to a discipline. Both operands must be containers, and their "
                "elements must be the same type or convertible in one "
                "direction or the other.",
                {
                    { "tests_ccmc_elements_same_type",
                      "Containers sharing a value_type are compatible across "
                      "differing disciplines - vector<int> with set<int>, "
                      "map<int,int> with multimap<int,int> - since the element "
                      "clause is read independently of the discipline.",
                      &tests_ccmc_elements_same_type },
                    { "tests_ccmc_elements_convertible",
                      "Exercises each disjunct of the element clause "
                      "separately: vector<int> with vector<long> converts both "
                      "ways, and vector<int> with vector<cm_from_int> converts "
                      "one way only - which the clause admits, in either "
                      "operand position.",
                      &tests_ccmc_elements_convertible },
                    { "tests_ccmc_elements_incompatible",
                      "Elements that convert in neither direction refute the "
                      "concept even when both operands are containers of the "
                      "same shape - vector<int> with vector<cm_opaque> and "
                      "with vector<std::string>.",
                      &tests_ccmc_elements_incompatible },
                    { "tests_ccmc_elements_container_guard",
                      "The is_iterable_container guard is load-bearing and not "
                      "decoration: two non-containers have no value_type, so "
                      "their element types both degrade to void and the "
                      "element clause is satisfied - the guard is the only "
                      "thing refuting the pair. Asserted for int with int and "
                      "for cm_positional, which has a value_type but no "
                      "traversal.",
                      &tests_ccmc_elements_container_guard },
                    { "tests_ccmc_elements_void_element",
                      "Finding F3: two iterable ranges with no value_type ARE "
                      "mutually compatible, because is_same<void,void> holds. "
                      "Pins the behaviour and pins its containment - an "
                      "elementless range is compatible with no container that "
                      "does have elements, in either operand position.",
                      &tests_ccmc_elements_void_element },
                    { "tests_ccmc_elements_symmetry",
                      "The concept is symmetric in its operands: the element "
                      "clause ORs both conversion directions, so swapping "
                      "_From and _To never changes the answer. Checked across "
                      "the compatible, one-way-convertible, and incompatible "
                      "cases.",
                      &tests_ccmc_elements_symmetry },
                    { "tests_ccmc_elements_cvref",
                      "Invariant across all eight cv-ref forms in both operand "
                      "positions - which the concept guarantees structurally "
                      "by cleaning up front, and which the trait behind it "
                      "manages only because its element delegates clean too "
                      "(finding F6).",
                      &tests_ccmc_elements_cvref },
                    { "tests_ccmc_elements_hostile",
                      "Refutes every pairing of the full hostile zoo with "
                      "itself, and every pairing of a hostile fixture with a "
                      "real container, while compiling for all of them. No "
                      "completeness restriction applies here - unlike the copy "
                      "concept, nothing in this path names a "
                      "completeness-requiring trait.",
                      &tests_ccmc_elements_hostile },
                }
            },
            dt::block_spec{
                "mergeable",
                "mergeable_with - a merge of the pair is defined, meaning the "
                "elements combine AND the two disciplines agree. Two sequences "
                "concatenate, two bags union, two keyed maps merge by key, and "
                "anything reaching across those three families does not merge.",
                {
                    { "tests_ccmc_mergeable_sequences",
                      "Two sequences merge: vector with vector, vector with "
                      "array, string with string, and the hand-rolled "
                      "sequences with each other and with the std ones.",
                      &tests_ccmc_mergeable_sequences },
                    { "tests_ccmc_mergeable_bags",
                      "Two unkeyed bags merge in all three combinations - set "
                      "with set, set with multiset, multiset with multiset - "
                      "and the hand-rolled cm_unique_bag / cm_repeat_bag pair "
                      "merges identically, confirming the unique-insert split "
                      "is read from the local signal and not from the std "
                      "type.",
                      &tests_ccmc_mergeable_bags },
                    { "tests_ccmc_mergeable_keyed",
                      "Two keyed maps merge in all three combinations - map "
                      "with map, map with multimap, multimap with multimap - "
                      "and likewise for the hand-rolled cm_unique_keyed / "
                      "cm_repeat_keyed pair.",
                      &tests_ccmc_mergeable_keyed },
                    { "tests_ccmc_mergeable_cross_family",
                      "Every crossing of the three families is refused - "
                      "sequence with bag, sequence with keyed, bag with keyed "
                      "- including the case designed to isolate discipline "
                      "from elements: map<int,int> with "
                      "vector<pair<const int,int>>, whose value types are "
                      "identical and whose disciplines are not.",
                      &tests_ccmc_mergeable_cross_family },
                    { "tests_ccmc_mergeable_pair_valued",
                      "Finding F7: a sequence whose value_type is a pair is "
                      "classified as a KEYED discipline. "
                      "vector<pair<int,int>> reads as multimap and "
                      "set<pair<int,int>> as map, so a plain vector of pairs "
                      "merges by key with a std::map, the pair elements being "
                      "mutually convertible. Follows directly from the "
                      "documented pair-value signal - the keyed test is tried "
                      "first and asks nothing about a key_type - so it is a "
                      "consequence of the probe rather than a bug in it. "
                      "Pinned as observed pending reconciliation with "
                      "container_overlay_traits.hpp.",
                      &tests_ccmc_mergeable_pair_valued },
                    { "tests_ccmc_mergeable_element_gate",
                      "Matching disciplines are not enough: vector<int> with "
                      "vector<cm_opaque> and set<int> with set<std::string> "
                      "are both refused on elements alone, and each is a "
                      "witness that mergeable_with is strictly stronger than "
                      "merge_elements_compatible_with rather than equal to it.",
                      &tests_ccmc_mergeable_element_gate },
                    { "tests_ccmc_mergeable_symmetry",
                      "The concept is symmetric: the kind function treats its "
                      "two disciplines symmetrically and the element clause is "
                      "already symmetric, so _From and _To may be exchanged "
                      "freely. Checked across sequences, bags, keyed maps, and "
                      "both refusal modes - which is what makes the operand "
                      "naming harmless here and load-bearing in F1.",
                      &tests_ccmc_mergeable_symmetry },
                    { "tests_ccmc_mergeable_cvref",
                      "Invariant across all eight cv-ref forms in both operand "
                      "positions, for a merging pair and for a refusing one.",
                      &tests_ccmc_mergeable_cvref },
                    { "tests_ccmc_mergeable_hostile",
                      "Refutes the full hostile zoo against itself and against "
                      "real containers, while compiling for every cell.",
                      &tests_ccmc_mergeable_hostile },
                    { "tests_ccmc_mergeable_constrains",
                      "The binary concept works in a requires-clause: a "
                      "mergeable_with-constrained overload wins for a merging "
                      "pair and the unconstrained fallback is taken for a "
                      "cross-family pair.",
                      &tests_ccmc_mergeable_constrains },
                }
            },
            dt::block_spec{
                "overflow",
                "merge_may_overflow_into - the capacity proviso. A merge only "
                "grows, so a defined merge whose result type is capacity-"
                "bounded may exceed it. Four signals mark a bound (a static "
                "extent, a tuple_size, static interval bounds, or a capacity() "
                "with no reserve()), and this block exercises each in "
                "isolation before turning to the operand-order defect.",
                {
                    { "tests_ccmc_overflow_extent",
                      "A static `extent` constant marks the result bounded - "
                      "cm_extent_seq overflows where the otherwise identical "
                      "cm_plain_seq does not.",
                      &tests_ccmc_overflow_extent },
                    { "tests_ccmc_overflow_tuple_size",
                      "A std::tuple_size specialization marks the result "
                      "bounded: std::array overflows, and std::vector - which "
                      "differs from it in no other signal this trait reads - "
                      "does not.",
                      &tests_ccmc_overflow_tuple_size },
                    { "tests_ccmc_overflow_static_bounds",
                      "Static lower_bound / upper_bound mark a finite "
                      "closed-interval domain bounded, via cm_interval_seq.",
                      &tests_ccmc_overflow_static_bounds },
                    { "tests_ccmc_overflow_fixed_capacity",
                      "A const-callable capacity() with no reserve() marks the "
                      "result bounded, for a sequence (cm_fixed_capacity_seq) "
                      "and for a bag (cm_bounded_bag) alike - the proviso is "
                      "read off the bound, not off the discipline.",
                      &tests_ccmc_overflow_fixed_capacity },
                    { "tests_ccmc_overflow_reserve_anti_signal",
                      "reserve() is what keeps a capacity() from meaning a "
                      "fixed bound: cm_growable_seq differs from "
                      "cm_fixed_capacity_seq only in having one, and does not "
                      "overflow. std::vector and std::string, which have both "
                      "accessors, agree.",
                      &tests_ccmc_overflow_reserve_anti_signal },
                    { "tests_ccmc_overflow_requires_mergeable",
                      "Boundedness alone is not enough - the merge must be "
                      "defined first. A bounded sequence against a bag, and a "
                      "bounded sequence against an element-incompatible "
                      "sequence, both refute the concept despite the result "
                      "carrying every capacity signal.",
                      &tests_ccmc_overflow_requires_mergeable },
                    { "tests_ccmc_overflow_reads_left_operand",
                      "Finding F1, pinned as observed: the concept answers "
                      "about its FIRST operand, not the target its name and "
                      "comment describe. merge_may_overflow_into<bounded, "
                      "unbounded> is true and <unbounded, bounded> is false - "
                      "the exact inverse of the documented reading. This test "
                      "will fail if the operand order is repaired, which is "
                      "the intent; it is the alarm on the defect, not an "
                      "endorsement of it.",
                      &tests_ccmc_overflow_reads_left_operand },
                    { "tests_ccmc_overflow_cvref",
                      "Invariant across all eight cv-ref forms in both operand "
                      "positions, for an overflowing pair and a non-"
                      "overflowing one.",
                      &tests_ccmc_overflow_cvref },
                    { "tests_ccmc_overflow_hostile",
                      "Refutes the full hostile zoo - nothing that fails the "
                      "mergeability precondition can overflow - while "
                      "compiling for every cell.",
                      &tests_ccmc_overflow_hostile },
                }
            },
            dt::block_spec{
                "agreement",
                "The module's headline claim, tested rather than trusted: each "
                "concept is EXACTLY its trait. Agreement is checked at a "
                "point, across a battery, across the hostile zoo, and across "
                "all eight cv-ref forms - then paired with polarity pins, "
                "because two predicates that are both uniformly false agree "
                "vacuously and prove nothing. The block closes with the parity "
                "map: which of the trait module's predicates have a concept "
                "face at all, and which do not.",
                {
                    { "tests_ccmc_agree_copyable",
                      "is_copyable_container and copyable_container agree at "
                      "each container fixture, at each refuting type, and "
                      "across D_TEST_HOSTILE_TYPES_COMPLETE (the completeness "
                      "restriction of F2 applying to the agreement trait "
                      "exactly as it does to the concept).",
                      &tests_ccmc_agree_copyable },
                    { "tests_ccmc_agree_elements",
                      "merge_elements_compatible and "
                      "merge_elements_compatible_with agree across the "
                      "compatible, convertible, incompatible, non-container, "
                      "and elementless cases, and across the full hostile zoo.",
                      &tests_ccmc_agree_elements },
                    { "tests_ccmc_agree_mergeable",
                      "is_mergeable and mergeable_with agree across all six "
                      "disciplines, every cross-family refusal, the element "
                      "refusals, and the full hostile zoo.",
                      &tests_ccmc_agree_mergeable },
                    { "tests_ccmc_agree_overflow",
                      "merge_may_overflow and merge_may_overflow_into agree "
                      "across all four boundedness signals, the reserve "
                      "anti-signal, the non-mergeable cases, and the full "
                      "hostile zoo - including the asymmetric orderings, since "
                      "the concept forwards F1's operand order faithfully and "
                      "the two faces therefore share the defect rather than "
                      "differing over it.",
                      &tests_ccmc_agree_overflow },
                    { "tests_ccmc_agree_cvref",
                      "Finding F6: agreement itself is checked across all "
                      "eight cv-ref forms, not just at the bare type. The "
                      "concepts clean their arguments up front and the traits "
                      "clean internally, so the two faces cannot drift under "
                      "qualification - but for merge_elements_compatible that "
                      "holds only because its element delegates clean, a "
                      "dependency neither header states.",
                      &tests_ccmc_agree_cvref },
                    { "tests_ccmc_agree_nonvacuous",
                      "Closes the vacuity hole: each of the four concepts is "
                      "shown to be true somewhere and false somewhere, so no "
                      "agreement above is the trivial agreement of two "
                      "predicates that are never true. Uses holds_for_any over "
                      "known-satisfying types and holds_for_none over the "
                      "hostile zoo for each lifted concept in turn.",
                      &tests_ccmc_agree_nonvacuous },
                    { "tests_ccmc_agree_trait_shape",
                      "Half of finding F5: the traits the concepts forward to "
                      "are standard-shaped bool traits - value_type, ::value, "
                      "a bool_constant nested type and base - with one "
                      "exception. merge_may_overflow is a plain struct "
                      "carrying only a static constexpr bool, so is_bool_trait "
                      "rejects it where it accepts the other five.",
                      &tests_ccmc_agree_trait_shape },
                    { "tests_ccmc_agree_face_parity",
                      "The other half of F5: the concept module covers four of "
                      "the trait module's six bool predicates. "
                      "copy_preserves_all_axes and merge_has_key_conflict have "
                      "no requires-facing form, so a template wanting to "
                      "constrain on the key-conflict proviso - one of the "
                      "three the traits preamble names - must still reach for "
                      "the trait. Both are exercised directly here to show "
                      "they are live, non-vacuous predicates and not dead "
                      "code.",
                      &tests_ccmc_agree_face_parity },
                }
            },
            dt::block_spec{
                "ordering",
                "Subsumption - the axis with no analog on the trait side, and "
                "the one most easily gotten wrong. Two concepts can be "
                "satisfied by exactly the same types and still sit differently "
                "in the partial order the compiler consults between "
                "constrained overloads. This block separates implication (a "
                "fact about satisfaction) from subsumption (a fact about "
                "spelling), and reports finding F4.",
                {
                    { "tests_ccmc_order_implication",
                      "The satisfaction chain holds across a battery spanning "
                      "the bounded shapes, all six disciplines, the "
                      "non-containers and the full hostile zoo: "
                      "merge_may_overflow_into implies mergeable_with implies "
                      "merge_elements_compatible_with, at every type. Run "
                      "through holds_for_all, so every cell instantiates and "
                      "none is skipped by a short circuit.",
                      &tests_ccmc_order_implication },
                    { "tests_ccmc_order_mergeable_vs_elements",
                      "Finding F4: despite that implication, mergeable_with "
                      "does NOT subsume merge_elements_compatible_with. At "
                      "std::vector<int>, which satisfies both, the ordering "
                      "report reads unordered - two overloads constrained on "
                      "them are ambiguous. Both direction bools are asserted "
                      "false so the reading cannot be a mis-chosen witness.",
                      &tests_ccmc_order_mergeable_vs_elements },
                    { "tests_ccmc_order_overflow_vs_mergeable",
                      "The same result one rung up: merge_may_overflow_into "
                      "does not subsume mergeable_with either, read at "
                      "std::array<int,4>, which satisfies both. The four "
                      "concepts are siblings, not a ladder.",
                      &tests_ccmc_order_overflow_vs_mergeable },
                    { "tests_ccmc_order_restatement_trap",
                      "Demonstrates WHY, on this module's own terms: a "
                      "constraint spelled by reaching past mergeable_with to "
                      "the trait it forwards to is true of exactly the same "
                      "types and is still unordered with it, because atomic "
                      "constraints are identified by their source expression "
                      "and not by what they compute. Any respelling of a "
                      "concept, however faithful, forfeits its position in the "
                      "order - which is the trap the whole block guards.",
                      &tests_ccmc_order_restatement_trap },
                    { "tests_ccmc_order_neither_witness",
                      "The report is honest about a witness that observes "
                      "nothing: at int, which satisfies neither concept of "
                      "either pair, `neither` is true, `both` is false, and "
                      "all three of a_subsumes_b, b_subsumes_a and unordered "
                      "are false - so an unopposed win at a bad witness can "
                      "never be read as a subsumption result.",
                      &tests_ccmc_order_neither_witness },
                }
            },
#endif  // D_CM_CONCEPTS_ENABLED  (spec blocks)
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONTAINER_COPY_MERGE_CONCEPTS_TESTS_
