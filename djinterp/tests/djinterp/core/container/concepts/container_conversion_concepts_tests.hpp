/******************************************************************************
* djinterp [test]                      container_conversion_concepts_tests.hpp
*
*   Suite header for container_conversion_concepts.hpp -- the C++20 concept
* face of the CONVERSION axis.  Six BINARY concepts over a four-tier
* classification:
*
*     I.   CONVERTIBILITY - container_convertible_to, losslessly_convertible_to,
*                           lossily_convertible_to: three readings of one tier
*     II.  TIERS          - the partition itself, its precedence order, and the
*                           gap the concept surface leaves in it
*     III. PATHS          - range_constructible_from, range_insertable_from
*     IV.  VIEW           - view_convertible_to, the eight-way conjunction
*     V.   AGREEMENT      - each concept against its trait
*     VI.  ROBUSTNESS     - direction, qualification, non-class shapes
*
*
*   READ THIS FIRST: THE TWO PATH CONCEPTS TAKE THEIR OPERANDS BACKWARDS.
*
*   container_conversion_traits.hpp declares
*
*       template<typename _From, typename _To> struct is_range_constructible;
*
*   and its body reads `range_constructible_helper<clean_t<_To>,
* clean_t<_From>>`, so the trait means "_To can be built from _From's range".
*
*   The concept then spells
*
*       template<typename _From, typename _To>
*       concept range_constructible_from =
*           is_range_constructible<clean_t<_To>, clean_t<_From>>::value;
*
*   - passing _To where the trait expects _From.  So the concept computes
* "_From can be built from _To's range", which is the REVERSE of its own
* documentation:
*
*       "_To can be built from _From's iterator range.  Note the direction:
*        the CONSTRAINT is on _To, the RANGE is _From."
*
*   The comment goes out of its way to fix the direction, and the code does the
* opposite.  It is not a harmless naming quibble - the two readings disagree on
* real pairs.  std::vector can be built from a std::array's range; a std::array
* cannot be built from a vector's range, being an aggregate with no iterator-
* pair constructor.  So:
*
*       range_constructible_from<array<int,4>, vector<int>>  documented: TRUE
*                                                            actual:    FALSE
*       range_constructible_from<vector<int>, array<int,4>>  documented: FALSE
*                                                            actual:    TRUE
*
*   range_insertable_from carries the identical swap.  The other four concepts
* forward their operands in order and are unaffected.
*
*   HOW THE SUITE HANDLES IT.  The module compiles, so - unlike an ill-formed
* concept - the tests can run either way.  Section III pins the CURRENT
* behaviour, so the suite is green against the code as it stands and a further
* accidental change would be caught; a dedicated test records the discrepancy
* against the documentation; and the documented semantics are pinned behind
* DJINTERP_CCONV_RANGE_DIRECTION_FIXED, which defaults to 0.  Set it to 1 after
* swapping the concept's arguments and section III flips to holding the
* documented reading instead.
*
*
*   THE SECOND FINDING: THE STRUCTURAL TIER HAS NO CONCEPT.
*
*   The tier is one of { view, constructive, lossy, structural, none }, and the
* concepts read it as:
*
*       container_convertible_to    tier != none
*       losslessly_convertible_to   tier == view || tier == constructive
*       lossily_convertible_to      tier == lossy
*
*   which leaves `structural` convertible and NEITHER lossless NOR lossy.  A
* flat-to-hierarchical conversion therefore satisfies container_convertible_to
* and is refuted by both of the concepts that would classify it.  The trait
* conversion_requires_restructuring exists and has no concept face, so the
* natural `lossless || lossy` reading of "convertible" is wrong.  Section II
* pins the gap.
*
*   A third property follows from the tier's precedence order and is worth
* stating: a topology change is judged BEFORE element compatibility, so a
* flat-to-hierarchical pair whose elements have nothing to do with each other
* is still `structural`, and still convertible.
*
*
*   BINARY CONCEPTS AND THE UNARY TOOLKIT:
*   Every macro in test_concept.hpp is unary and C++20 has no concept aliases,
* so the lifts below PARTIALLY APPLY each concept - binding one operand to a
* fixture - and are spelled by hand in the shape those macros emit.  Once
* bound, the framework's quantifiers and cv-ref report carry them unchanged.
*
*   TWO FACES (DTEST_SPEC_MODE):
*   Defining DTEST_SPEC_MODE selects the SPEC-PROVIDER face.
*
*
* path:      /test/djinterp/core/container/concepts/
*                                    container_conversion_concepts_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONVERSION_CONCEPTS_TESTS_
#define DJINTERP_CONTAINER_CONVERSION_CONCEPTS_TESTS_ 1

#ifndef __cplusplus
    #error "container_conversion_concepts_tests.hpp requires C++ compilation"
#endif  // __cplusplus

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "djinterp/core/container/traits/container_conversion_traits.hpp"
#include "djinterp/core/container/concepts/container_conversion_concepts.hpp"
#include "djinterp/test/test_concept.hpp"
#include "djinterp/test/test_defaults.hpp"


// DJINTERP_CCONV_TESTS_ACTIVE
//   macro: 1 iff the module under test actually emits concepts.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #define DJINTERP_CCONV_TESTS_ACTIVE   1
#else
    #define DJINTERP_CCONV_TESTS_ACTIVE   0
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// DJINTERP_CCONV_RANGE_DIRECTION_FIXED
//   macro: 0 while range_constructible_from and range_insertable_from pass
// their operands to the trait in the reverse of the documented order (see the
// file header), 1 once the arguments are swapped back.  Section III holds the
// current behaviour at 0 and the documented behaviour at 1, so the fix is a
// one-line change in the module and a one-line change here.
#ifndef DJINTERP_CCONV_RANGE_DIRECTION_FIXED
    #define DJINTERP_CCONV_RANGE_DIRECTION_FIXED 0
#endif  // DJINTERP_CCONV_RANGE_DIRECTION_FIXED


#if !defined(DTEST_SPEC_MODE) && DJINTERP_CCONV_TESTS_ACTIVE

// std  (fixture-only)
#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                A.   PATH FIXTURES                                        ///
///////////////////////////////////////////////////////////////////////////////
//
//   The two range-path concepts are decided by two independent facts: whether
// the SOURCE is iterable, and whether the TARGET admits the corresponding
// call.  Both must be movable independently, and - critically - the pair must
// be ASYMMETRIC, because a symmetric pair cannot distinguish a concept that
// takes its operands in one order from one that takes them in the other.
// Every fixture below therefore exists to break that symmetry.

// cconv_range_source
//   struct: iterable, and offering nothing else.  A pure SOURCE: it can supply
// a range and cannot be built from one.
struct cconv_range_source
{
    const int* begin() const;

    const int* end() const;
};

// cconv_range_target
//   struct: constructible from an iterator pair, and NOT iterable.  A pure
// TARGET, and the exact mirror of the fixture above - so the pair
// (source, target) admits a construction in one direction only, which is what
// makes it a direction probe rather than a mere positive witness.
struct cconv_range_target
{
    template<typename _Iter>
    cconv_range_target(_Iter _first, _Iter _last);
};

// cconv_range_both
//   struct: iterable AND range-constructible.  Conversions between this and
// either fixture above are defined in exactly one direction, so it supplies
// two more asymmetric pairs.
struct cconv_range_both
{
    cconv_range_both() = default;

    template<typename _Iter>
    cconv_range_both(_Iter _first, _Iter _last);

    const int* begin() const;

    const int* end() const;
};

// cconv_insert_target
//   struct: accepts a range through insert(first, last) and is not
// constructible from one - so it separates the two path concepts from each
// other as well as separating their operand orders.
struct cconv_insert_target
{
    template<typename _Iter>
    void insert(_Iter _first, _Iter _last);
};

// cconv_inert
//   struct: neither iterable nor range-constructible nor range-insertable.
// The floor for both path concepts, in both operand orders.
struct cconv_inert
{};


///////////////////////////////////////////////////////////////////////////////
///                B.   TYPE BATTERIES                                       ///
///////////////////////////////////////////////////////////////////////////////

// D_CCONV_PATH_FIXTURES
//   macro: the path fixtures.
#define D_CCONV_PATH_FIXTURES                                                 \
    ::djinterp::testing::cconv_range_source,                                  \
    ::djinterp::testing::cconv_range_target,                                  \
    ::djinterp::testing::cconv_range_both,                                    \
    ::djinterp::testing::cconv_insert_target,                                 \
    ::djinterp::testing::cconv_inert

// D_CCONV_SEQUENCES
//   macro: the flat sequence containers - the population among which views and
// constructions are expected to be found.
#define D_CCONV_SEQUENCES                                                     \
    std::vector<int>,                                                         \
    std::deque<int>,                                                          \
    std::list<int>,                                                           \
    std::array<int, 4>,                                                       \
    std::string

// D_CCONV_ASSOCIATIVE
//   macro: the order-blind and keyed containers - the targets of the canonical
// forgetting conversions.
#define D_CCONV_ASSOCIATIVE                                                   \
    std::set<int>,                                                            \
    std::multiset<int>,                                                       \
    std::map<int, int>

// D_CCONV_STDLIB
//   macro: both of the above together.
#define D_CCONV_STDLIB                                                        \
    D_CCONV_SEQUENCES,                                                        \
    D_CCONV_ASSOCIATIVE

// D_CCONV_NONCONTAINERS
//   macro: shapes with no elements and no range.
#define D_CCONV_NONCONTAINERS                                                 \
    int,                                                                      \
    double,                                                                   \
    void*,                                                                    \
    ::djinterp::testing::cconv_inert

// D_CCONV_BATTERY
//   macro: everything the laws are quantified over.
#define D_CCONV_BATTERY                                                       \
    D_CCONV_PATH_FIXTURES,                                                    \
    D_CCONV_STDLIB,                                                           \
    D_CCONV_NONCONTAINERS


///////////////////////////////////////////////////////////////////////////////
///                C.   TIER SHORTHANDS                                      ///
///////////////////////////////////////////////////////////////////////////////

// cconv_tier
//   value: the raw tier of a pair, which most tests read directly - the six
// concepts are all one-line readings of it, so reading it is how a wrong
// reading gets attributed to the right place.
template<typename _From,
         typename _To>
D_CONSTEXPR ::djinterp::conversion_tier cconv_tier =
    ::djinterp::conversion_tier_of< ::djinterp::clean_t<_From>,
                                    ::djinterp::clean_t<_To> >::value;

// cconv_is_structural
//   trait: the tier the concept surface has no concept for.  Supplied here so
// section II can quantify over the gap rather than only exhibit it.
template<typename _From,
         typename _To>
struct cconv_is_structural
    : ::djinterp::bool_constant<
          ( cconv_tier<_From, _To> ==
            ::djinterp::conversion_tier::structural ) >
{};


///////////////////////////////////////////////////////////////////////////////
///                D.   THE DOCUMENTED PATH PREDICATES                       ///
///////////////////////////////////////////////////////////////////////////////

// cconv_range_constructible_documented
//   trait: what range_constructible_from is documented to mean - "_To can be
// built from _From's iterator range" - which is the trait taken in its own
// declared order.  Section III holds this against the concept, and the two
// currently disagree.
template<typename _From,
         typename _To>
struct cconv_range_constructible_documented
    : ::djinterp::bool_constant<
          ::djinterp::is_range_constructible< ::djinterp::clean_t<_From>,
                                              ::djinterp::clean_t<_To> >::value >
{};

// cconv_range_insertable_documented
//   trait: the same for the insert path.
template<typename _From,
         typename _To>
struct cconv_range_insertable_documented
    : ::djinterp::bool_constant<
          ::djinterp::is_range_insertable< ::djinterp::clean_t<_From>,
                                           ::djinterp::clean_t<_To> >::value >
{};

// cconv_range_constructible_actual
//   trait: what the concept ACTUALLY computes - the trait with the operands
// exchanged.  Kept beside the documented form so the discrepancy can be
// measured rather than merely asserted.
template<typename _From,
         typename _To>
struct cconv_range_constructible_actual
    : ::djinterp::bool_constant<
          ::djinterp::is_range_constructible< ::djinterp::clean_t<_To>,
                                              ::djinterp::clean_t<_From> >::value >
{};

// cconv_range_insertable_actual
//   trait: the same for the insert path.
template<typename _From,
         typename _To>
struct cconv_range_insertable_actual
    : ::djinterp::bool_constant<
          ::djinterp::is_range_insertable< ::djinterp::clean_t<_To>,
                                           ::djinterp::clean_t<_From> >::value >
{};


///////////////////////////////////////////////////////////////////////////////
///                E.   PARTIALLY-APPLIED LIFTS                              ///
///////////////////////////////////////////////////////////////////////////////
//
//   A binary concept cannot be handed to D_TEST_CONCEPT_TRAIT, so each lift
// binds one operand and is spelled by hand in the shape that macro emits.
// Both directions are bound where the axis is directional - which is all of
// them, conversion being the framework's first genuinely asymmetric axis.

// cconv_c_convertible_to_vector
//   trait: container_convertible_to<_From, std::vector<int>>, lifted.
template<typename _From>
struct cconv_c_convertible_to_vector
    : ::djinterp::bool_constant<
          ::djinterp::container_convertible_to<_From, std::vector<int> > >
{};

// cconv_c_convertible_from_vector
//   trait: the OTHER direction - container_convertible_to<std::vector<int>,
// _To>.  Both are needed: a quantifier over one direction alone would miss an
// asymmetry entirely.
template<typename _To>
struct cconv_c_convertible_from_vector
    : ::djinterp::bool_constant<
          ::djinterp::container_convertible_to<std::vector<int>, _To> >
{};

// cconv_c_lossless_to_vector
//   trait: losslessly_convertible_to<_From, std::vector<int>>, lifted.
template<typename _From>
struct cconv_c_lossless_to_vector
    : ::djinterp::bool_constant<
          ::djinterp::losslessly_convertible_to<_From, std::vector<int> > >
{};

// cconv_c_lossy_to_set
//   trait: lossily_convertible_to<_From, std::set<int>>, lifted - bound to a
// target that FORGETS, so the predicate is live.
template<typename _From>
struct cconv_c_lossy_to_set
    : ::djinterp::bool_constant<
          ::djinterp::lossily_convertible_to<_From, std::set<int> > >
{};

// cconv_c_view_to_self_vector
//   trait: view_convertible_to<_From, std::vector<int>>, lifted.
template<typename _From>
struct cconv_c_view_to_vector
    : ::djinterp::bool_constant<
          ::djinterp::view_convertible_to<_From, std::vector<int> > >
{};

// cconv_c_range_constructible_from_vector
//   trait: range_constructible_from<_From, std::vector<int>>, lifted - the
// concept as it currently behaves, not as documented.
template<typename _From>
struct cconv_c_range_constructible_from_vector
    : ::djinterp::bool_constant<
          ::djinterp::range_constructible_from<_From, std::vector<int> > >
{};

// cconv_c_range_insertable_from_vector
//   trait: range_insertable_from<_From, std::vector<int>>, lifted.
template<typename _From>
struct cconv_c_range_insertable_from_vector
    : ::djinterp::bool_constant<
          ::djinterp::range_insertable_from<_From, std::vector<int> > >
{};


///////////////////////////////////////////////////////////////////////////////
///                F.   AGREEMENT LIFTS  (test_concept.hpp IV, by hand)      ///
///////////////////////////////////////////////////////////////////////////////
//
//   Note what the two path agreements compare.  Because the concept forwards
// with the operands exchanged, agreement with the trait TAKEN IN THE CONCEPT'S
// OWN ORDER is false - and that is the defect, not a test error.  The lifts
// below therefore compare the concept against the trait AS THE CONCEPT CALLS
// IT, which is the only agreement that currently holds, and section III
// separately measures the distance to the documented reading.

// cconv_agree_convertible_to_vector
//   trait: is_convertible_between vs container_convertible_to.
template<typename _From>
struct cconv_agree_convertible_to_vector
    : ::djinterp::bool_constant<
          ( ::djinterp::is_convertible_between< ::djinterp::clean_t<_From>,
                                                std::vector<int> >::value ==
            ::djinterp::container_convertible_to<_From, std::vector<int> > ) >
{};

// cconv_agree_lossless_to_vector
//   trait: is_lossless_conversion vs losslessly_convertible_to.
template<typename _From>
struct cconv_agree_lossless_to_vector
    : ::djinterp::bool_constant<
          ( ::djinterp::is_lossless_conversion< ::djinterp::clean_t<_From>,
                                                std::vector<int> >::value ==
            ::djinterp::losslessly_convertible_to<_From,
                                                  std::vector<int> > ) >
{};

// cconv_agree_lossy_to_set
//   trait: is_lossy_conversion vs lossily_convertible_to.
template<typename _From>
struct cconv_agree_lossy_to_set
    : ::djinterp::bool_constant<
          ( ::djinterp::is_lossy_conversion< ::djinterp::clean_t<_From>,
                                             std::set<int> >::value ==
            ::djinterp::lossily_convertible_to<_From, std::set<int> > ) >
{};

// cconv_agree_view_to_vector
//   trait: is_view_conversion vs view_convertible_to.
template<typename _From>
struct cconv_agree_view_to_vector
    : ::djinterp::bool_constant<
          ( ::djinterp::is_view_conversion< ::djinterp::clean_t<_From>,
                                            std::vector<int> >::value ==
            ::djinterp::view_convertible_to<_From, std::vector<int> > ) >
{};

// cconv_agree_range_constructible_as_called
//   trait: the path concept against the trait AS IT CALLS IT - operands
// exchanged.  This is the agreement that holds today.
template<typename _From>
struct cconv_agree_range_constructible_as_called
    : ::djinterp::bool_constant<
          ( cconv_range_constructible_actual<_From, std::vector<int> >::value ==
            ::djinterp::range_constructible_from<_From, std::vector<int> > ) >
{};

// cconv_agree_range_insertable_as_called
//   trait: the same for the insert path.
template<typename _From>
struct cconv_agree_range_insertable_as_called
    : ::djinterp::bool_constant<
          ( cconv_range_insertable_actual<_From, std::vector<int> >::value ==
            ::djinterp::range_insertable_from<_From, std::vector<int> > ) >
{};


///////////////////////////////////////////////////////////////////////////////
///                G.   QUANTIFIED LAWS                                      ///
///////////////////////////////////////////////////////////////////////////////

// cconv_law_view_implies_lossless
//   value: a reinterpretation moves no data, so it loses none.  The one
// entailment inside the lossless half.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_view_implies_lossless =
    ( ( ( !::djinterp::view_convertible_to<_Types, std::vector<int> > ) ||
        ::djinterp::losslessly_convertible_to<_Types,
                                              std::vector<int> > ) && ... );

// cconv_law_lossless_implies_convertible
//   value: a lossless conversion is a conversion.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_lossless_implies_convertible =
    ( ( ( !::djinterp::losslessly_convertible_to<_Types,
                                                 std::vector<int> > ) ||
        ::djinterp::container_convertible_to<_Types,
                                             std::vector<int> > ) && ... );

// cconv_law_lossy_implies_convertible
//   value: so is a lossy one.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_lossy_implies_convertible =
    ( ( ( !::djinterp::lossily_convertible_to<_Types, std::set<int> > ) ||
        ::djinterp::container_convertible_to<_Types,
                                             std::set<int> > ) && ... );

// cconv_law_lossless_excludes_lossy
//   value: the two are readings of disjoint tiers, so no pair is both.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_lossless_excludes_lossy =
    ( ( !( ::djinterp::losslessly_convertible_to<_Types, std::set<int> > &&
           ::djinterp::lossily_convertible_to<_Types,
                                              std::set<int> > ) ) && ... );

// cconv_law_convertible_decomposition
//   value: THE GAP.  Convertible is NOT lossless-or-lossy: the structural
// tier sits between them, convertible and classified by neither.  The law is
// stated in the true form - convertible equals lossless or lossy or
// structural - so that a future concept for the structural tier makes it a
// two-way reading rather than invalidating it.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_convertible_decomposition =
    ( ( ::djinterp::container_convertible_to<_Types, std::vector<int> > ==
        ( ::djinterp::losslessly_convertible_to<_Types, std::vector<int> > ||
          ::djinterp::lossily_convertible_to<_Types, std::vector<int> >    ||
          cconv_is_structural<_Types, std::vector<int> >::value ) ) && ... );

// cconv_law_tier_is_a_partition
//   value: exactly one tier holds per pair, so the five readings never
// overlap and never leave a pair unclassified.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_tier_is_a_partition =
    ( ( ( ( ::djinterp::view_convertible_to<_Types, std::vector<int> >
                ? 1 : 0 ) +
          ( ( ::djinterp::losslessly_convertible_to<_Types,
                                                    std::vector<int> > &&
              ( !::djinterp::view_convertible_to<_Types,
                                                 std::vector<int> > ) )
                ? 1 : 0 ) +
          ( ::djinterp::lossily_convertible_to<_Types, std::vector<int> >
                ? 1 : 0 ) +
          ( cconv_is_structural<_Types, std::vector<int> >::value
                ? 1 : 0 ) +
          ( ( !::djinterp::container_convertible_to<_Types,
                                                    std::vector<int> > )
                ? 1 : 0 ) ) == 1 ) && ... );

// cconv_law_self_conversion_is_a_view
//   value: converting a container to its own type moves nothing, so it is a
// view - every axis relation being equivalent with itself and the elements
// being identical.  The cheapest structural check on the reinterpretation
// conjunction, and the first thing a mis-ordered operand pair would break.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_self_conversion_is_a_view =
    ( ( ( !::djinterp::container_convertible_to<_Types, _Types> ) ||
        ::djinterp::view_convertible_to<_Types, _Types> ) && ... );

// cconv_law_range_concept_is_the_swapped_trait
//   value: the path concepts equal the trait with the operands EXCHANGED -
// the defect, stated as a law so it is quantified rather than exhibited.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_range_concept_is_the_swapped_trait =
    ( ( ( ::djinterp::range_constructible_from<_Types, std::vector<int> > ==
          cconv_range_constructible_actual<_Types,
                                           std::vector<int> >::value ) &&
        ( ::djinterp::range_insertable_from<_Types, std::vector<int> > ==
          cconv_range_insertable_actual<_Types,
                                        std::vector<int> >::value ) ) && ... );

// cconv_law_cvref_stable
//   value: every concept answers identically for _Type, const _Type, _Type&,
// const _Type& and _Type&& in the LEFT operand.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_cvref_stable =
    ( ( ( ::djinterp::container_convertible_to<_Types, std::vector<int> > ==
          ::djinterp::container_convertible_to<const _Types&,
                                               std::vector<int> > )       &&
        ( ::djinterp::container_convertible_to<_Types, std::vector<int> > ==
          ::djinterp::container_convertible_to<_Types&&,
                                               std::vector<int> > )       &&
        ( ::djinterp::losslessly_convertible_to<_Types, std::vector<int> > ==
          ::djinterp::losslessly_convertible_to<const _Types,
                                                std::vector<int> > )      &&
        ( ::djinterp::lossily_convertible_to<_Types, std::set<int> > ==
          ::djinterp::lossily_convertible_to<const _Types&,
                                             std::set<int> > )            &&
        ( ::djinterp::view_convertible_to<_Types, std::vector<int> > ==
          ::djinterp::view_convertible_to<_Types&, std::vector<int> > )   &&
        ( ::djinterp::range_constructible_from<_Types, std::vector<int> > ==
          ::djinterp::range_constructible_from<const _Types&,
                                               std::vector<int> > ) ) && ... );

// cconv_law_cvref_stable_right
//   value: the same in the RIGHT operand - the asymmetric failure a binary
// concept is uniquely exposed to.
template<typename... _Types>
D_CONSTEXPR bool cconv_law_cvref_stable_right =
    ( ( ( ::djinterp::container_convertible_to<std::vector<int>, _Types> ==
          ::djinterp::container_convertible_to<std::vector<int>,
                                               const _Types&> )          &&
        ( ::djinterp::losslessly_convertible_to<std::vector<int>, _Types> ==
          ::djinterp::losslessly_convertible_to<std::vector<int>,
                                                _Types&&> )              &&
        ( ::djinterp::range_constructible_from<std::vector<int>, _Types> ==
          ::djinterp::range_constructible_from<std::vector<int>,
                                               const _Types&> ) ) && ... );


NS_END  // testing
NS_END  // djinterp

#endif  // !defined(DTEST_SPEC_MODE) && DJINTERP_CCONV_TESTS_ACTIVE


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                H.   TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// I. convertibility
bool tests_convertible_positive();
bool tests_convertible_negative();
bool tests_lossless_convertible();
bool tests_lossy_convertible();

// II. the tiers
bool tests_tier_is_a_partition();
bool tests_tier_precedence();
bool tests_structural_gap();
bool tests_convertible_decomposition();

// III. the range paths
bool tests_range_direction_defect();
bool tests_range_constructible_behaviour();
bool tests_range_insertable_behaviour();
bool tests_range_paths_require_an_iterable_source();

// IV. views
bool tests_view_positive();
bool tests_view_negative();
bool tests_view_conjunction_is_genuine();
bool tests_self_conversion_is_a_view();

// V. agreement
bool tests_agreement_convertibility();
bool tests_agreement_view();
bool tests_agreement_range_paths();
bool tests_agreement_non_vacuity();

// VI. robustness
bool tests_conversion_is_directional();
bool tests_cvref_invariance();
bool tests_nonclass_shapes();
bool tests_independent_respelling();


///////////////////////////////////////////////////////////////////////////////
///                I.   SPEC PROVIDER                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
container_conversion_concepts_spec
  Builds the module_spec for the container_conversion_concepts.hpp suite: one
block per semantic section of the header under test, each naming the unit
tests the matching section TU defines.  The runner hands this straight to
run_module, which lowers it into both framework views (the six-kind tree and
the report / PDF).

Parameter(s):
  none.
Return:
  a module_spec naming every unit test in the suite.
*/
inline ::djinterp::test::module_spec
container_conversion_concepts_spec()
{
    namespace dt = ::djinterp::test;

    return dt::module_spec{
        "container_conversion_concepts.hpp",
        "C++20 concept face of the CONVERSION axis: the four-tier "
        "classification, the three readings taken off it, and the two range "
        "paths that feed it.",
        {
            dt::block_spec{
                "I. convertibility",
                "container_convertible_to / losslessly_convertible_to / "
                "lossily_convertible_to - three readings of one tier.",
                {
                    dt::test_spec{
                        "convertible: positive evidence",
                        "a container converts to itself, and to targets its "
                        "range can build - across the sequence and "
                        "associative populations.",
                        &tests_convertible_positive },
                    dt::test_spec{
                        "convertible: negative evidence",
                        "incompatible elements and absent paths refute it, "
                        "and non-containers convert to nothing.",
                        &tests_convertible_negative },
                    dt::test_spec{
                        "losslessly_convertible_to",
                        "a view or a data-preserving construction satisfies "
                        "it; a forgetting or clamping conversion does not.",
                        &tests_lossless_convertible },
                    dt::test_spec{
                        "lossily_convertible_to",
                        "forgetting order or multiplicity satisfies it, and "
                        "it is disjoint from the lossless reading.",
                        &tests_lossy_convertible }
                }
            },
            dt::block_spec{
                "II. the tiers",
                "the partition the six concepts read, its precedence order, "
                "and the gap the concept surface leaves in it.",
                {
                    dt::test_spec{
                        "the tier is a partition",
                        "exactly one of view / constructive / lossy / "
                        "structural / none holds per pair, so the readings "
                        "never overlap and never leave a pair unclassified.",
                        &tests_tier_is_a_partition },
                    dt::test_spec{
                        "tier precedence",
                        "a topology change is judged FIRST - before element "
                        "compatibility - then loss, then reinterpretation, "
                        "then construction.",
                        &tests_tier_precedence },
                    dt::test_spec{
                        "the structural gap",
                        "a structural conversion is convertible and NEITHER "
                        "lossless nor lossy: the tier has a trait and no "
                        "concept.",
                        &tests_structural_gap },
                    dt::test_spec{
                        "convertible decomposes into three, not two",
                        "convertible equals lossless or lossy or structural - "
                        "the natural two-way reading is wrong.",
                        &tests_convertible_decomposition }
                }
            },
            dt::block_spec{
                "III. the range paths",
                "range_constructible_from / range_insertable_from - and the "
                "operand order they are spelled with.",
                {
                    dt::test_spec{
                        "the operand direction defect",
                        "both concepts pass _To where the trait expects "
                        "_From, so each computes the REVERSE of its own "
                        "documentation - shown at a concrete standard pair.",
                        &tests_range_direction_defect },
                    dt::test_spec{
                        "range_constructible_from: behaviour",
                        "what the concept actually answers, pinned across the "
                        "asymmetric fixtures so a further accidental change "
                        "is caught.",
                        &tests_range_constructible_behaviour },
                    dt::test_spec{
                        "range_insertable_from: behaviour",
                        "the same for the insert path, which carries the "
                        "identical swap and is separable from the construct "
                        "path.",
                        &tests_range_insertable_behaviour },
                    dt::test_spec{
                        "the paths require an iterable source",
                        "the trait conjoins is_iterable_container on its "
                        "first operand, so a non-iterable source refutes both "
                        "paths whichever way they are read.",
                        &tests_range_paths_require_an_iterable_source }
                }
            },
            dt::block_spec{
                "IV. views",
                "view_convertible_to - the eight-way conjunction that decides "
                "whether any data moves.",
                {
                    dt::test_spec{
                        "view: positive evidence",
                        "a container viewed as itself is a view, elements, "
                        "level, layout and all four axis relations agreeing.",
                        &tests_view_positive },
                    dt::test_spec{
                        "view: negative evidence",
                        "a differing layout, lifetime, storage or element "
                        "type each defeats it - array to vector is a copy, "
                        "not a view.",
                        &tests_view_negative },
                    dt::test_spec{
                        "the conjunction is genuine",
                        "each conjunct is separately necessary, so no clause "
                        "of the reinterpretation predicate is dead.",
                        &tests_view_conjunction_is_genuine },
                    dt::test_spec{
                        "self-conversion is a view",
                        "converting a container to its own type moves "
                        "nothing - the cheapest structural check on the whole "
                        "conjunction.",
                        &tests_self_conversion_is_a_view }
                }
            },
            dt::block_spec{
                "V. agreement",
                "the concepts add no policy - each is exactly its trait.",
                {
                    dt::test_spec{
                        "agreement: the three tier readings",
                        "convertible / lossless / lossy agree with "
                        "is_convertible_between / is_lossless_conversion / "
                        "is_lossy_conversion across the battery.",
                        &tests_agreement_convertibility },
                    dt::test_spec{
                        "agreement: view",
                        "view_convertible_to agrees with is_view_conversion, "
                        "including at the near-miss pairs where one conjunct "
                        "fails.",
                        &tests_agreement_view },
                    dt::test_spec{
                        "agreement: the range paths",
                        "each path concept agrees with the trait AS IT CALLS "
                        "IT, and the distance to the documented reading is "
                        "measured rather than assumed.",
                        &tests_agreement_range_paths },
                    dt::test_spec{
                        "agreement: non-vacuity",
                        "each of the six predicates holds somewhere and fails "
                        "somewhere, so none of the agreements is vacuously "
                        "true.",
                        &tests_agreement_non_vacuity }
                }
            },
            dt::block_spec{
                "VI. robustness",
                "direction, qualification in both operands, non-class shapes, "
                "and an independent re-spelling.",
                {
                    dt::test_spec{
                        "conversion is directional",
                        "unlike the comparison axis, every concept here is "
                        "asymmetric - each has a witness pair it accepts one "
                        "way and refuses the other.",
                        &tests_conversion_is_directional },
                    dt::test_spec{
                        "cv-ref invariance",
                        "all six concepts answer identically for T, const T, "
                        "T&, const T& and T&& - in EITHER operand.",
                        &tests_cvref_invariance },
                    dt::test_spec{
                        "non-class shapes",
                        "scalars and pointers have no elements and no range, "
                        "so they convert to nothing and answer without a "
                        "diagnostic.",
                        &tests_nonclass_shapes },
                    dt::test_spec{
                        "independent re-spelling",
                        "each tier reading re-derived from the raw "
                        "conversion_tier by hand matches the module.",
                        &tests_independent_respelling }
                }
            }
        }
    };
}


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONVERSION_CONCEPTS_TESTS_
