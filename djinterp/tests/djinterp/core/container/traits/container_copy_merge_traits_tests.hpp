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
* the keyed branch and never reaches the key_type test.  cm_pair_sequence and
* the std::vector<std::pair> carrier pin that; see
* tests_discipline_pair_sequence_misread.
*
*   TWO-PARAMETER TRAITS.  Most of this header takes a Left and a Right, so the
* fixtures come in families that vary one side at a time, and the cv/ref macro
* has a two-operand form that varies each side in turn.
*
*   NOT EVERY TRAIT HERE IS AN integral_constant.  merge_discipline_of,
* merge_kind_of, merge_may_overflow and merge_class carry a bare `value`
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
* path:      /tests/djinterp/core/container/traits/container_copy_merge_traits_tests.hpp
*                                   container_copy_merge_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.20
*                                                            revised: 2026.09.21
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
#include "../../../../../inc/djinterp/core/container/traits/container_copy_merge_traits.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// cm_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = cm_check_failed(...)` on the failing path only.
inline bool
cm_check_failed(
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
inline merge_discipline
cm_discipline_at(
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
inline merge_kind
cm_kind_at(
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
inline bool
cm_disc_is(
    merge_discipline _d,
    const char*      _expected
)
{
    return ( std::strcmp(merge_discipline_name(_d), _expected) == 0 );
}

inline bool
cm_kind_is(
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

// cm_seq
//   struct: a comparator-less sequence - iterable, no key_type, no pair
// value_type.  The baseline.
struct cm_seq
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};

// cm_set_like
//   struct: a key_type (the equivalence mark) and an insert returning a
// `.second` (the unique mark), and no pair value_type - a set.
struct cm_set_like
{
    using value_type = int;
    using key_type   = int;

    const int*                        begin() const;
    const int*                        end() const;
    std::pair<const int*, bool>       insert(const int& _v);
};

// cm_multiset_like
//   struct: the same equivalence mark with an insert returning a plain
// iterator - a multiset.
struct cm_multiset_like
{
    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
    const int* insert(const int& _v);
};

// cm_map_like
//   struct: a pair value_type (the keyed mark) with a unique insert - a map.
struct cm_map_like
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    const value_type*                        begin() const;
    const value_type*                        end() const;
    std::pair<const value_type*, bool>       insert(const value_type& _v);
};

// cm_multimap_like
//   struct: a pair value_type with a plain-iterator insert - a multimap.
struct cm_multimap_like
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    const value_type* begin() const;
    const value_type* end() const;
    const value_type* insert(const value_type& _v);
};

// cm_pair_sequence
//   struct: a PLAIN SEQUENCE whose elements happen to be pairs - iterable,
// pair-shaped value_type, and NO key_type, so nothing about it is
// associative.  The discipline reading tests the keyed mark BEFORE the
// equivalence mark, so this takes the keyed branch and never reaches the
// key_type test; it reads multimap.  The carrier for
// tests_discipline_pair_sequence_misread.
struct cm_pair_sequence
{
    using value_type = std::pair<int, int>;

    const value_type* begin() const;
    const value_type* end() const;
};

// cm_pair_sequence_unique
//   struct: the same misread reached on the other side of the uniqueness
// split - a pair-valued sequence that happens to offer a `.second`-returning
// single-element insert, so it reads map rather than multimap.
struct cm_pair_sequence_unique
{
    using value_type = std::pair<int, int>;

    const value_type*                        begin() const;
    const value_type*                        end() const;
    std::pair<const value_type*, bool>       insert(const value_type& _v);
};

// cm_half_pair_value
//   struct: the value_type carries a first_type but NO second_type, so it is
// not pair-shaped.  The keyed probe names both members, and this carrier is
// what shows the second of them is load-bearing - a probe requiring only
// first_type would read this as a keyed discipline.
struct cm_half_pair_value
{
    struct half
    {
        using first_type = int;
    };

    using value_type = half;

    const half* begin() const;
    const half* end() const;
};

// cm_insert_first_only
//   struct: an insert whose result carries a `.first` but no `.second`, as a
// positional insert's iterator return would.  The unique probe names `.second`
// specifically, and this carrier is what shows WHICH member it names - a probe
// naming `.first` would read this as unique.
struct cm_insert_first_only
{
    struct pos_result
    {
        const int* first;
    };

    using value_type = int;
    using key_type   = int;

    const int* begin() const;
    const int* end() const;
    pos_result insert(const int& _v);
};

// cm_not_container
//   struct: no begin()/end(), so not iterable - the discipline is unknown
// however else it is shaped.
struct cm_not_container
{
    using value_type = int;
    using key_type   = int;
};

// cm_key_no_iter
//   struct: carries every associative mark but is NOT iterable, so the
// container guard short-circuits it to unknown before any signal is read.
struct cm_key_no_iter
{
    using value_type = std::pair<const int, int>;
    using key_type   = int;

    std::pair<const value_type*, bool> insert(const value_type& _v);
};

// cm_insert_wrong_arity
//   struct: an insert taking a POSITION and a value, as a sequence container
// does.  The probe calls it with one argument, so the unique mark is absent
// and this reads multiset rather than set.
struct cm_insert_wrong_arity
{
    using value_type = int;
    using key_type   = int;

    const int*                  begin() const;
    const int*                  end() const;
    std::pair<const int*, bool> insert(const int* _pos, const int& _v);
};

// cm_insert_second_not_bool
//   struct: an insert whose result has a `.second` that is not a bool.  The
// probe names the member and never constrains its type, so this still reads
// as the unique mark - a structural probe, not a semantic one.
struct cm_insert_second_not_bool
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
    odd_result insert(const int& _v);
};

// cm_private_key
//   struct: key_type is private, so the equivalence mark is invisible and
// this reads as a plain sequence.
struct cm_private_key
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

// cm_extent
//   struct: a static `extent` constant.
struct cm_extent
{
    using value_type = int;

    static constexpr std::size_t extent = 8;

    const int* begin() const;
    const int* end() const;
};

// cm_static_bounds
//   struct: static lower_bound / upper_bound - a closed-interval domain.
struct cm_static_bounds
{
    using value_type = int;

    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 7;

    const int* begin() const;
    const int* end() const;
};

// cm_lower_bound_only
//   struct: a lower_bound with NO upper_bound - a half-open domain, which is
// not a finite interval and so not a capacity bound.  The probe names both
// constants, and this carrier is what shows the second is load-bearing.
struct cm_lower_bound_only
{
    using value_type = int;

    static constexpr int lower_bound = 0;

    const int* begin() const;
    const int* end() const;
};

// cm_capacity_only
//   struct: a const capacity() and NO reserve() - a fixed capacity.
struct cm_capacity_only
{
    using value_type = int;

    const int*  begin() const;
    const int*  end() const;
    std::size_t capacity() const;
};

// cm_capacity_reserve
//   struct: a capacity() WITH a reserve(), so the capacity is a hint and not
// a bound.  The anti-signal carrier.
struct cm_capacity_reserve
{
    using value_type = int;

    const int*  begin() const;
    const int*  end() const;
    std::size_t capacity() const;
    void        reserve(std::size_t _n);
};

// cm_unbounded
//   struct: none of the four signals.
struct cm_unbounded
{
    using value_type = int;

    const int* begin() const;
    const int* end() const;
};


// ===========================================================================
// V.   COPY FIXTURES
// ===========================================================================

// cm_move_only
//   struct: an iterable container whose copy constructor is deleted - the
// capability half of is_copyable_container failing on a genuine container.
struct cm_move_only
{
    using value_type = int;

    cm_move_only()                    = default;
    cm_move_only(const cm_move_only&) = delete;
    cm_move_only(cm_move_only&&)      = default;

    const int* begin() const;
    const int* end() const;
};

// cm_copyable_not_container
//   struct: freely copyable but not iterable - the container half failing.
struct cm_copyable_not_container
{
    int unrelated;
};


// ===========================================================================
// VI.  ELEMENT-COMPATIBILITY FIXTURES
// ===========================================================================
//   Element compatibility is same-type OR convertible in EITHER direction, so
// the fixtures come as a one-way-convertible pair to show the disjunction is
// not symmetric term by term even though the trait is.

// cm_from
//   struct: a sequence whose element converts TO cm_to's element but not back.
struct cm_from
{
    struct elem
    {
    };

    using value_type = elem;

    const elem* begin() const;
    const elem* end() const;
};

// cm_to
//   struct: a sequence whose element is constructible FROM cm_from's, giving a
// one-way conversion.
struct cm_to
{
    struct elem
    {
        elem() = default;
        elem(const cm_from::elem& _other);
    };

    using value_type = elem;

    const elem* begin() const;
    const elem* end() const;
};

// cm_unrelated_elem
//   struct: a sequence whose element converts neither way.
struct cm_unrelated_elem
{
    struct elem
    {
        explicit elem(int _x);
    };

    using value_type = elem;

    const elem* begin() const;
    const elem* end() const;
};


// ===========================================================================
// VII. STANDARD-LIBRARY CARRIERS
// ===========================================================================
//   The standard containers the local signals are meant to agree with.
// cm_pair_vector is the standard-library witness for the pair-sequence
// misread, and cm_unique_vector for the copy-capability false positive.

using cm_vector    = std::vector<int>;
using cm_vec_double = std::vector<double>;
using cm_vec_string = std::vector<std::string>;
using cm_array     = std::array<int, 3>;
using cm_deque     = std::deque<int>;
using cm_list      = std::list<int>;
using cm_string    = std::string;
using cm_set       = std::set<int>;
using cm_multiset  = std::multiset<int>;
using cm_map       = std::map<int, int>;
using cm_multimap  = std::multimap<int, int>;
using cm_uset      = std::unordered_set<int>;
using cm_umap      = std::unordered_map<int, int>;
using cm_pair      = std::pair<int, int>;

// cm_pair_vector
//   type: a std::vector of pairs - the standard-library witness that a plain
// sequence of pairs reads as a keyed discipline.
using cm_pair_vector = std::vector<std::pair<int, int> >;

// cm_unique_vector
//   type: a std::vector of move-only elements.  std::is_copy_constructible
// reports TRUE for it - the copy constructor is declared unconditionally and
// only becomes ill-formed when instantiated, which is outside the immediate
// context - so is_copyable_container inherits a false positive.  Pinned in
// tests_copy_capability_false_positive.
using cm_unique_vector = std::vector<std::unique_ptr<int> >;


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONTAINER_COPY_MERGE_TRAITS_
