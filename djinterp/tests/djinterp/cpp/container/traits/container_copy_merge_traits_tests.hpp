/*******************************************************************************
* djinterp [testing]                       container_copy_merge_traits_tests.hpp
*
* Unit tests for container_copy_merge_traits.hpp:
*   Declarations, fixtures and assertion plumbing for the copy and merge
* operations.  Every test function is declared here and defined in the section
* .cpp that matches the numbered section of the header under test.
*
*   WHAT IS UNDER TEST.  Two operations founded on comparison.  COPY is the
* identity-preserving one and is nearly trivial at the type level - a
* capability test plus an unconditional note.  MERGE is the substantial one: it
* reads a DISCIPLINE off each operand from local structural signals, decides a
* combining kind from the pair, and attaches three provisos.
*
*   THE DISCIPLINE READING IS THE INTERESTING PART, and it is where the
* fixtures concentrate.  Discipline is decided by three probes - a nested
* key_type (the equivalence mark), a pair-shaped value_type (the keyed mark),
* and a single-element insert returning a `.second` (the unique mark) - tested
* in a fixed ORDER, keyed first.  Because the keyed branch is tested before the
* equivalence branch, a plain sequence whose elements happen to be pairs takes
* the keyed branch and never reaches the key_type test.  DCmPairSequence and
* the std::vector<std::pair> carrier pin that; see
* tests_discipline_pair_sequence_misread.
*
*   TWO-PARAMETER TRAITS.  Most of this header takes a Left and a Right, so the
* fixtures come in families that vary one side at a time, and the cv/ref macro
* has a two-operand form that varies each side in turn.
*
*   NOT EVERY TRAIT HERE IS AN integral_constant.  merge_discipline_of,
* merge_kind_of, DMergeMayOverflow and DMergeClass carry a bare `value`
* member rather than deriving from one, so the standard metafunction members
* are absent on those four; tests_traits_metafunction_shape records which is
* which.
*
*   FIXTURE NAMING.  Fixtures are prefixed cm_ and are free of commas, so they
* pass through the cv/ref macros without parenthesising.  Standard-library
* carriers with comma-bearing argument lists are typedef'd here for the same
* reason.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_copy_merge_traits_tests.hpp
*                                   container_copy_merge_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.20
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Assertion plumbing
      ------------------

II.   Enum walkers
      ------------

III.  Discipline fixtures
      -------------------

IV.   Boundedness fixtures
      --------------------

V.    Copy fixtures
      -------------

VI.   Element-compatibility fixtures
      ------------------------------

VII.  Standard-library carriers (comma-free typedefs)
      -----------------------------------------------

VIII. Test declarations
      -----------------
*/

#ifndef DJINTERP_TESTS_CONTAINER_COPY_MERGE_TRAITS_
#define DJINTERP_TESTS_CONTAINER_COPY_MERGE_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/container_copy_merge_traits.hpp"
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


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// cm_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = cm_check_failed(...)` on the failing path only.
inline
bool cm_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// D_CM_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_CM_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::cm_check_failed(#__VA_ARGS__,           \
                                                      __FILE__,               \
                                                      __LINE__);              \
        }                                                                     \
    } while (0)

// D_CM_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_CM_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_CM_CV_REF_AGREE
//   macro: a one-operand trait writes clean_t before asking its question, so
// all seven cv- and reference-qualified spellings must answer alike -
// including the volatile ones.  Not usable on void or a function type, since
// `void&` cannot be formed.
#define D_CM_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_CM_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_CM_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_CM_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_CM_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_CM_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_CM_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_CM_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// D_CM_CV_REF_AGREE2
//   macro: the two-operand form.  Qualifies each side in turn against a plain
// other side, so a trait that cleans only one of its two arguments is caught -
// which the one-operand macro cannot see.
#define D_CM_CV_REF_AGREE2(TRAIT, LEFT, RIGHT, EXPECT)                        \
    do                                                                        \
    {                                                                         \
        D_CM_CHECK((TRAIT<LEFT, RIGHT>::value)                == (EXPECT));   \
        D_CM_CHECK((TRAIT<const LEFT, RIGHT>::value)          == (EXPECT));   \
        D_CM_CHECK((TRAIT<volatile LEFT, RIGHT>::value)       == (EXPECT));   \
        D_CM_CHECK((TRAIT<const volatile LEFT, RIGHT>::value) == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT&, RIGHT>::value)               == (EXPECT));   \
        D_CM_CHECK((TRAIT<const LEFT&, RIGHT>::value)         == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT&&, RIGHT>::value)              == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT, const RIGHT>::value)          == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT, volatile RIGHT>::value)       == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT, const volatile RIGHT>::value) == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT, RIGHT&>::value)               == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT, const RIGHT&>::value)         == (EXPECT));   \
        D_CM_CHECK((TRAIT<LEFT, RIGHT&&>::value)              == (EXPECT));   \
        D_CM_CHECK((TRAIT<const LEFT&, RIGHT&&>::value)       == (EXPECT));   \
    } while (0)


// ===========================================================================
// II.  ENUM WALKERS
// ===========================================================================
//   Both enums are walked in full rather than sampled, so a new enumerator
// added upstream shows up as a failure rather than passing unnoticed.

// cm_discipline_count / cm_kind_count
//   constants: how many enumerators each enum declares.  NOTE that neither is
// a power of two, so casting an out-of-range value IS reachable and the naming
// functions' fall-through is a genuine default - see
// tests_enum_names_fall_through.
static const int cm_discipline_count = 6;
static const int cm_kind_count       = 5;

// cm_discipline_at
//   function: the _i-th discipline, in declaration order.  Out-of-range
// indices clamp to unknown so a loop bug cannot read past the enum.
inline
merge_discipline cm_discipline_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return merge_discipline::unknown;
        case 1:  return merge_discipline::sequence;
        case 2:  return merge_discipline::set;
        case 3:  return merge_discipline::multiset;
        case 4:  return merge_discipline::map;
        case 5:  return merge_discipline::multimap;
        default: return merge_discipline::unknown;
    }
}

// cm_kind_at
//   function: the _i-th merge kind, in declaration order.  Out-of-range
// indices clamp to incompatible.
inline
merge_kind cm_kind_at(
    int _i
)
{
    switch (_i)
    {
        case 0:  return merge_kind::concatenation;
        case 1:  return merge_kind::set_union;
        case 2:  return merge_kind::multiset_union;
        case 3:  return merge_kind::keyed_merge;
        case 4:  return merge_kind::incompatible;
        default: return merge_kind::incompatible;
    }
}

// cm_disc_is / cm_kind_is
//   functions: compare a value's spelling against an expected literal.
inline
bool cm_disc_is(
    merge_discipline _d,
    const char*      _expected
)
{
    return ( std::strcmp(merge_discipline_name(_d), _expected) == 0 );
}

inline
bool cm_kind_is(
    merge_kind  _k,
    const char* _expected
)
{
    return ( std::strcmp(merge_kind_name(_k), _expected) == 0 );
}


// ===========================================================================
// III. DISCIPLINE FIXTURES
// ===========================================================================
//   One carrier per discipline, built from the three local signals directly
// rather than borrowed from the standard library, so each signal can be varied
// on its own.  Every carrier is iterable, since a non-container short-circuits
// to unknown before any signal is read.

// DCmSeq
//   struct: a comparator-less sequence - iterable, no key_type, no pair
// value_type.  The baseline.
struct DCmSeq
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};

// DCmSetLike
//   struct: a key_type (the equivalence mark) and an insert returning a
// `.second` (the unique mark), and no pair value_type - a set.
struct DCmSetLike
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
    std::pair<const int*, bool> insert(
        const int& _v
    );
};

// DCmMultisetLike
//   struct: the same equivalence mark with an insert returning a plain
// iterator - a multiset.
struct DCmMultisetLike
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
    const int* insert(
        const int& _v
    );
};

// DCmMapLike
//   struct: a pair value_type (the keyed mark) with a unique insert - a map.
struct DCmMapLike
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    const value_type* begin() const;
    const value_type* end() const;
    std::pair<const value_type*, bool> insert(
        const value_type& _v
    );
};

// DCmMultimapLike
//   struct: a pair value_type with a plain-iterator insert - a multimap.
struct DCmMultimapLike
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    const value_type* begin() const;
    const value_type* end() const;
    const value_type* insert(
        const value_type& _v
    );
};

// DCmPairSequence
//   struct: a PLAIN SEQUENCE whose elements happen to be pairs - iterable,
// pair-shaped value_type, and NO key_type, so nothing about it is
// associative.  The discipline reading tests the keyed mark BEFORE the
// equivalence mark, so this takes the keyed branch and never reaches the
// key_type test; it reads multimap.  The carrier for
// tests_discipline_pair_sequence_misread.
struct DCmPairSequence
{
    using value_type = std::pair<int, int>;

    const value_type* begin() const;
    const value_type* end() const;
};

// DCmPairSequenceUnique
//   struct: the same misread reached on the other side of the uniqueness
// split - a pair-valued sequence that happens to offer a `.second`-returning
// single-element insert, so it reads map rather than multimap.
struct DCmPairSequenceUnique
{
    using value_type = std::pair<int, int>;

    const value_type* begin() const;
    const value_type* end() const;
    std::pair<const value_type*, bool> insert(
        const value_type& _v
    );
};

// DCmHalfPairValue
//   struct: the value_type carries a first_type but NO second_type, so it is
// not pair-shaped.  The keyed probe names both members, and this carrier is
// what shows the second of them is load-bearing - a probe requiring only
// first_type would read this as a keyed discipline.
struct DCmHalfPairValue
{
    struct half
    {
        using first_type = int;
    };

    using value_type = half;

    const half* begin() const;
    const half* end() const;
};

// DCmInsertFirstOnly
//   struct: an insert whose result carries a `.first` but no `.second`, as a
// positional insert's iterator return would.  The unique probe names `.second`
// specifically, and this carrier is what shows WHICH member it names - a probe
// naming `.first` would read this as unique.
struct DCmInsertFirstOnly
{
    struct DPosResult
    {
        const int* first;
    };

    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
    DPosResult insert(
        const int& _v
    );
};

// DCmNotContainer
//   struct: no begin()/end(), so not iterable - the discipline is unknown
// however else it is shaped.
struct DCmNotContainer
{
    using value_type = int;
    using key_type   = int;
};

// DCmKeyNoIter
//   struct: carries every associative mark but is NOT iterable, so the
// container guard short-circuits it to unknown before any signal is read.
struct DCmKeyNoIter
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    std::pair<const value_type*, bool> insert(
        const value_type& _v
    );
};

// DCmInsertWrongArity
//   struct: an insert taking a POSITION and a value, as a sequence container
// does.  The probe calls it with one argument, so the unique mark is absent
// and this reads multiset rather than set.
struct DCmInsertWrongArity
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
    std::pair<const int*, bool> insert(
        const int* _pos,
        const int& _v
    );
};

// DCmInsertSecondNotBool
//   struct: an insert whose result has a `.second` that is not a bool.  The
// probe names the member and never constrains its type, so this still reads
// as the unique mark - a structural probe, not a semantic one.
struct DCmInsertSecondNotBool
{
    struct odd_result
    {
        const int* first;
        char       second;
    };

    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
    odd_result insert(
        const int& _v
    );
};

// DCmPrivateKey
//   struct: key_type is private, so the equivalence mark is invisible and
// this reads as a plain sequence.
struct DCmPrivateKey
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;

private:
    using key_type = int;
};


// ===========================================================================
// IV.  BOUNDEDNESS FIXTURES
// ===========================================================================
//   The four routes to a capacity-bounded result, one carrier each, plus the
// growable carrier that shows the reserve() anti-signal switching the
// capacity route off.  All are sequences, so the merge is defined and the
// overflow proviso is reached.

// DCmExtent
//   struct: a static `extent` constant.
struct DCmExtent
{
    using value_type = int;

    static constexpr std::size_t extent = 8;

    const int* begin() const;
    const int* end() const;
};

// DCmStaticBounds
//   struct: static lower_bound / upper_bound - a closed-interval domain.
struct DCmStaticBounds
{
    using value_type = int;

    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 7;

    const int* begin() const;
    const int* end() const;
};

// DCmLowerBoundOnly
//   struct: a lower_bound with NO upper_bound - a half-open domain, which is
// not a finite interval and so not a capacity bound.  The probe names both
// constants, and this carrier is what shows the second is load-bearing.
struct DCmLowerBoundOnly
{
    using value_type = int;

    static constexpr int lower_bound = 0;

    const int* begin() const;
    const int* end() const;
};

// DCmCapacityOnly
//   struct: a const capacity() and NO reserve() - a fixed capacity.
struct DCmCapacityOnly
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
    std::size_t capacity() const;
};

// DCmCapacityReserve
//   struct: a capacity() WITH a reserve(), so the capacity is a hint and not
// a bound.  The anti-signal carrier.
struct DCmCapacityReserve
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
    std::size_t capacity() const;
    void reserve(
        std::size_t _n
    );
};

// DCmUnbounded
//   struct: none of the four signals.
struct DCmUnbounded
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};


// ===========================================================================
// V.   COPY FIXTURES
// ===========================================================================

// DCmMoveOnly
//   struct: an iterable container whose copy constructor is deleted - the
// capability half of is_copyable_container failing on a genuine container.
struct DCmMoveOnly
{
    using value_type = int;

    DCmMoveOnly() = default;
    DCmMoveOnly(
        const DCmMoveOnly&
    ) = delete;
    DCmMoveOnly(
        DCmMoveOnly&&
    ) = default;

    const int* begin() const;
    const int* end() const;
};

// DCmCopyableNotContainer
//   struct: freely copyable but not iterable - the container half failing.
struct DCmCopyableNotContainer
{
    int unrelated;
};


// ===========================================================================
// VI.  ELEMENT-COMPATIBILITY FIXTURES
// ===========================================================================
//   Element compatibility is same-type OR convertible in EITHER direction, so
// the fixtures come as a one-way-convertible pair to show the disjunction is
// not symmetric term by term even though the trait is.

// DCmFrom
//   struct: a sequence whose element converts TO DCmTo's element but not back.
struct DCmFrom
{
    struct elem
    {
    };

    using value_type = elem;

    const elem* begin() const;
    const elem* end() const;
};

// DCmTo
//   struct: a sequence whose element is constructible FROM DCmFrom's, giving a
// one-way conversion.
struct DCmTo
{
    struct elem
    {
        elem() = default;
        elem(
            const DCmFrom::elem& _other
        );
    };

    using value_type = elem;

    const elem* begin() const;
    const elem* end() const;
};

// DCmUnrelatedElem
//   struct: a sequence whose element converts neither way.
struct DCmUnrelatedElem
{
    struct elem
    {
        explicit
        elem(
            int _x
        );
    };

    using value_type = elem;

    const elem* begin() const;
    const elem* end() const;
};


// ===========================================================================
// VII. STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   The standard containers the local signals are meant to agree with.
// DCmPairVector is the standard-library witness for the pair-sequence
// misread, and DCmUniqueVector for the copy-capability false positive.

using DCmVector    = std::vector<int>;
using DCmVecDouble = std::vector<double>;
using DCmVecString = std::vector<std::string>;
using DCmArray     = std::array<int, 3>;
using DCmDeque     = std::deque<int>;
using DCmList      = std::list<int>;
using DCmString    = std::string;
using DCmSet       = std::set<int>;
using DCmMultiset  = std::multiset<int>;
using DCmMap       = std::map<int, int>;
using DCmMultimap  = std::multimap<int, int>;
using DCmUset      = std::unordered_set<int>;
using DCmUmap      = std::unordered_map<int, int>;
using DCmPair      = std::pair<int, int>;

// DCmPairVector
//   type: a std::vector of pairs - the standard-library witness that a plain
// sequence of pairs reads as a keyed discipline.
using DCmPairVector = std::vector<std::pair<int, int> >;

// DCmUniqueVector
//   type: a std::vector of move-only elements.  std::is_copy_constructible
// reports TRUE for it - the copy constructor is declared unconditionally and
// only becomes ill-formed when instantiated, which is outside the immediate
// context - so is_copyable_container inherits a false positive.  Pinned in
// tests_copy_capability_false_positive.
using DCmUniqueVector = std::vector<std::unique_ptr<int> >;
#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// VIII. TEST DECLARATIONS
// ===========================================================================

// I.   Operand prerequisites       (..._tests_prerequisites.cpp)
bool tests_merge_elements_compatible();
bool tests_merge_elements_needs_containers();
bool tests_merge_elements_convertibility_is_either_way();
bool tests_merge_elements_cv_ref();
bool tests_merge_elements_companions();

// II.  Copy                        (..._tests_copy.cpp)
bool tests_is_copyable_container();
bool tests_copy_needs_both_conjuncts();
bool tests_copy_capability_false_positive();
bool tests_copy_preserves_all_axes();
bool tests_copy_cv_ref();
bool tests_copy_companions();

// III. Discipline signals          (..._tests_discipline.cpp)
bool tests_discipline_signal_probes();
bool tests_discipline_boundedness_probes();
bool tests_merge_discipline_enum();
bool tests_merge_discipline_name();
bool tests_discipline_family_predicates();
bool tests_merge_discipline_of();
bool tests_merge_discipline_of_std_types();
bool tests_discipline_pair_sequence_misread();
bool tests_discipline_signal_order();
bool tests_merge_discipline_cv_ref();
bool tests_merge_discipline_companions();

// IV.  Merge kind                  (..._tests_merge_kind.cpp)
bool tests_merge_kind_enum();
bool tests_merge_kind_name();
bool tests_enum_names_fall_through();
bool tests_merge_kind_from_disciplines();
bool tests_merge_kind_from_disciplines_total();
bool tests_merge_kind_of();
bool tests_merge_kind_of_element_guard();
bool tests_merge_kind_of_std_types();
bool tests_is_mergeable();
bool tests_merge_kind_cv_ref();
bool tests_merge_kind_companions();

// V.   Merge provisos              (..._tests_provisos.cpp)
bool tests_merge_result_type();
bool tests_merge_has_key_conflict();
bool tests_merge_has_key_conflict_needs_keyed_merge();
bool tests_merge_may_overflow();
bool tests_merge_may_overflow_routes();
bool tests_merge_may_overflow_is_left_biased();
bool tests_provisos_cv_ref();
bool tests_provisos_companions();

// VI.  Aggregate snapshot          (..._tests_class.cpp)
bool tests_merge_class_disciplines();
bool tests_merge_class_kind();
bool tests_merge_class_provisos();
bool tests_merge_class_agreement();
bool tests_merge_class_cv_ref();
bool tests_traits_metafunction_shape();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// container_copy_merge_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
container_copy_merge_traits_spec()
{
    return dt::module_spec{
        "container_copy_merge_traits",
        "COPY and MERGE - two of the operations FOUNDED on comparison (the "
        "spec, Operations founded on...",
        {
            dt::block_spec{
                "prerequisites",
                "Operand prerequisites",
                {
                    { "tests_merge_elements_compatible",
                      "Verifies the element prerequisite on carriers of each "
                      "kind",
                      &tests_merge_elements_compatible },
                    { "tests_merge_elements_needs_containers",
                      "Verifies the two container conjuncts, which precede "
                      "the element clause",
                      &tests_merge_elements_needs_containers },
                    { "tests_merge_elements_convertibility_is_either_way",
                      "Verifies that the element clause admits a conversion "
                      "in EITHER direction, not only left-to-right",
                      &tests_merge_elements_convertibility_is_either_way },
                    { "tests_merge_elements_cv_ref",
                      "Verifies that the trait cleans BOTH of its operands",
                      &tests_merge_elements_cv_ref },
                    { "tests_merge_elements_companions",
                      "Verifies the hand-written `_v` companion",
                      &tests_merge_elements_companions },
                }
            },
            dt::block_spec{
                "copy",
                "Copy",
                {
                    { "tests_is_copyable_container",
                      "Verifies the copy capability on carriers of each kind",
                      &tests_is_copyable_container },
                    { "tests_copy_needs_both_conjuncts",
                      "Verifies that neither half of the conjunction is "
                      "redundant",
                      &tests_copy_needs_both_conjuncts },
                    { "tests_copy_capability_false_positive",
                      "Records a limitation the trait INHERITS from "
                      "std::is_copy_constructible",
                      &tests_copy_capability_false_positive },
                    { "tests_copy_preserves_all_axes",
                      "Verifies the NOTE trait, and the boundary its shape "
                      "invites getting wrong",
                      &tests_copy_preserves_all_axes },
                    { "tests_copy_cv_ref",
                      "Verifies that both copy traits honour the clean_t "
                      "contract",
                      &tests_copy_cv_ref },
                    { "tests_copy_companions",
                      "Verifies the `_v` companions, which the trait_detect "
                      "macro emits for both of these single-parameter traits",
                      &tests_copy_companions },
                }
            },
            dt::block_spec{
                "discipline",
                "Discipline signals",
                {
                    { "tests_discipline_signal_probes",
                      "Verifies the three discipline probes in isolation, "
                      "before any of them is combined into a verdict",
                      &tests_discipline_signal_probes },
                    { "tests_discipline_boundedness_probes",
                      "Verifies the five boundedness probes the overflow "
                      "proviso is built from",
                      &tests_discipline_boundedness_probes },
                    { "tests_merge_discipline_enum",
                      "Verifies the discipline enum's shape",
                      &tests_merge_discipline_enum },
                    { "tests_merge_discipline_name",
                      "Verifies the discipline spellings",
                      &tests_merge_discipline_name },
                    { "tests_discipline_family_predicates",
                      "Verifies the three family predicates that group the "
                      "six disciplines",
                      &tests_discipline_family_predicates },
                    { "tests_merge_discipline_of",
                      "Verifies the per-operand verdict on the synthetic "
                      "carriers, where each signal can be varied on its own",
                      &tests_merge_discipline_of },
                    { "tests_merge_discipline_of_std_types",
                      "Verifies that the LOCAL signals agree with the "
                      "standard containers, which is the header's stated "
                      "justification for detecting them here rather than "
                      "importing the overlay axis",
                      &tests_merge_discipline_of_std_types },
                    { "tests_discipline_pair_sequence_misread",
                      "Records a MISCLASSIFICATION: a plain sequence whose "
                      "elements happen to be pairs is read as a keyed "
                      "discipline",
                      &tests_discipline_pair_sequence_misread },
                    { "tests_discipline_signal_order",
                      "Verifies the ORDER in which the verdict consults its "
                      "signals, independently of whether that order is the "
                      "right one",
                      &tests_discipline_signal_order },
                    { "tests_merge_discipline_cv_ref",
                      "Verifies that the verdict and its probes honour the "
                      "clean_t contract",
                      &tests_merge_discipline_cv_ref },
                    { "tests_merge_discipline_companions",
                      "Verifies the companions of the discipline verdict",
                      &tests_merge_discipline_companions },
                }
            },
            dt::block_spec{
                "merge_kind",
                "Merge kind",
                {
                    { "tests_merge_kind_enum",
                      "Verifies the kind enum's shape",
                      &tests_merge_kind_enum },
                    { "tests_merge_kind_name",
                      "Verifies the kind spellings",
                      &tests_merge_kind_name },
                    { "tests_enum_names_fall_through",
                      "Records a boundary shared by both naming functions: "
                      "each is a chain of equality tests ending in an "
                      "UNGUARDED default, so a value outside the enum is "
                      "silently given the last spelling rather than being "
                      "reported as unknown",
                      &tests_enum_names_fall_through },
                    { "tests_merge_kind_from_disciplines",
                      "Verifies the combining function on the pairs that "
                      "matter, before the exhaustive walk below",
                      &tests_merge_kind_from_disciplines },
                    { "tests_merge_kind_from_disciplines_total",
                      "Walks the ENTIRE 6x6 discipline grid and checks the "
                      "function's structural properties, rather than sampling "
                      "it",
                      &tests_merge_kind_from_disciplines_total },
                    { "tests_merge_kind_of",
                      "Verifies the trait that feeds two detected disciplines "
                      "to the function",
                      &tests_merge_kind_of },
                    { "tests_merge_kind_of_element_guard",
                      "Verifies that the element guard is consulted BEFORE "
                      "the disciplines, and that it can refuse a pair the "
                      "disciplines would otherwise accept",
                      &tests_merge_kind_of_element_guard },
                    { "tests_merge_kind_of_std_types",
                      "Verifies the kind across the standard containers, "
                      "where the disciplines are the familiar ones",
                      &tests_merge_kind_of_std_types },
                    { "tests_is_mergeable",
                      "Verifies the boolean face of the kind",
                      &tests_is_mergeable },
                    { "tests_merge_kind_cv_ref",
                      "Verifies that the kind trait cleans BOTH of its "
                      "operands",
                      &tests_merge_kind_cv_ref },
                    { "tests_merge_kind_companions",
                      "Verifies the companions of the kind trait and of "
                      "is_mergeable",
                      &tests_merge_kind_companions },
                }
            },
            dt::block_spec{
                "provisos",
                "Merge provisos",
                {
                    { "tests_merge_result_type",
                      "Verifies the result-type alias",
                      &tests_merge_result_type },
                    { "tests_merge_has_key_conflict",
                      "Verifies the key-conflict proviso",
                      &tests_merge_has_key_conflict },
                    { "tests_merge_has_key_conflict_needs_keyed_merge",
                      "Verifies the FIRST conjunct - that the proviso is "
                      "gated on the merge being keyed at all, not merely on a "
                      "map being present",
                      &tests_merge_has_key_conflict_needs_keyed_merge },
                    { "tests_merge_may_overflow",
                      "Verifies the capacity proviso on carriers of each kind",
                      &tests_merge_may_overflow },
                    { "tests_merge_may_overflow_routes",
                      "Verifies each of the four boundedness routes "
                      "separately, and the anti-signal that switches the "
                      "fourth one off",
                      &tests_merge_may_overflow_routes },
                    { "tests_merge_may_overflow_is_left_biased",
                      "Verifies that the proviso reads the RESULT type - the "
                      "left operand - and not both operands",
                      &tests_merge_may_overflow_is_left_biased },
                    { "tests_provisos_cv_ref",
                      "Verifies that all three provisos clean both of their "
                      "operands",
                      &tests_provisos_cv_ref },
                    { "tests_provisos_companions",
                      "Verifies the hand-written `_v` companions of the two "
                      "boolean provisos",
                      &tests_provisos_companions },
                }
            },
            dt::block_spec{
                "class",
                "Aggregate snapshot",
                {
                    { "tests_merge_class_disciplines",
                      "Verifies the two discipline members",
                      &tests_merge_class_disciplines },
                    { "tests_merge_class_kind",
                      "Verifies the kind member and its spelling",
                      &tests_merge_class_kind },
                    { "tests_merge_class_provisos",
                      "Verifies the four boolean members",
                      &tests_merge_class_provisos },
                    { "tests_merge_class_agreement",
                      "Verifies that the aggregate restates the standalone "
                      "traits and does not shadow them - the property that "
                      "makes it safe to read either one",
                      &tests_merge_class_agreement },
                    { "tests_merge_class_cv_ref",
                      "Verifies that the aggregate cleans both of its operands",
                      &tests_merge_class_cv_ref },
                    { "tests_traits_metafunction_shape",
                      "Records which of this header's traits derive from "
                      "std::integral_constant and which merely carry a "
                      "`value` member",
                      &tests_traits_metafunction_shape },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONTAINER_COPY_MERGE_TRAITS_
