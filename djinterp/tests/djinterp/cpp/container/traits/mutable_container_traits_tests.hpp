/*******************************************************************************
* djinterp [test]                             mutable_container_traits_tests.hpp
*
*   Declarations and shared fixtures for the mutable_container_traits.hpp unit
* suite.  The suite is split one translation unit per section of the header
* under test, so a section's tests sit next to nothing else:
*
*     _signals.cpp     I.   SFINAE mutator detection
*     _umbrellas.cpp   II.  Capability umbrellas
*     _grade.cpp       III. The mutability grade
*     _access.cpp      IV.  The access dimension (the 2-D model)
*     _aggregate.cpp   V.   Aggregate snapshot
*
*   THE PROBE FIXTURES:
*   The std containers are the realism check; the `mc_*` probes below are the
* precision check.  Each isolates ONE signal, or one near-miss, so a failure
* names the exact detector at fault rather than "vector changed".  Two probes
* carry most of the weight: DMcAllSignals declares every mutator AND a nested
* value_type; mc_no_value_type declares the same mutators and NO value_type.
* Between them they pin the value_type dependency of all eight signals - the
* five whose probe expression names value_type (push_back / push_front /
* insert / subscript / data) must go dark without it, while the three that do
* not (erase / clear / resize) must stay lit.
*
*   PORTABILITY OF THE PROBES:
*   No probe takes a pointer, or a type constructible from a pointer, as its
* subscript key.  `std::size_t{}` is a constant expression of value zero, and
* compilers differ on whether that is a null pointer constant; a pointer-keyed
* operator[] would therefore be detected on some toolchains and not others.
* Keys are std::size_t exactly, so the answer is the same everywhere.
*
*   Every check is a compile-time constant read at run time, so the run-time
* verdict and the compile-time one cannot disagree.  Properties that ONLY hold
* at compile time - constant-expression usability, type identity - are pinned
* with D_MC_STATIC_CHECK instead, where a regression breaks the build.
*
*
* path:      /tests/djinterp/cpp/container/traits/mutable_container_traits_tests.hpp
*                                          mutable_container_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_MUTABLE_CONTAINER_TRAITS_TESTS_
#define DJINTERP_MUTABLE_CONTAINER_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <deque>
#include <forward_list>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/cpp/container/traits/mutable_container_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <string_view>
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // std
    #include <span>
#endif


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   ASSERTION PLUMBING
// ===========================================================================

// mc_check_failed
//   function: reports one failed assertion and yields false, so a check reads
// as `ok = mc_check_failed(...)` on the failing path only.
inline
bool mc_check_failed(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("      ! %s:%d\n        %s\n", _file, _line, _expr);

    return false;
}

// mc_grade_predicate_count
//   function: how many of the four corner predicates hold for one grade.  The
// four partition the grade space, so the answer must be exactly one for every
// grade - a property no single predicate can witness on its own.
inline
int mc_grade_predicate_count(
    mutability _grade
)
{
    return ( ( is_immutable_grade(_grade)          ? 1 : 0 )
           + ( is_element_mutable_grade(_grade)    ? 1 : 0 )
           + ( is_structural_mutable_grade(_grade) ? 1 : 0 )
           + ( is_fully_mutable_grade(_grade)      ? 1 : 0 ) );
}

// D_MC_CHECK
//   macro: run-time assertion.  Variadic so a template argument list's commas
// survive the preprocessor.  The enclosing test function must declare a local
// `bool ok = true;` - this macro clears it on failure and leaves it untouched
// otherwise, so one function reports every failure it finds, not just the
// first.
#define D_MC_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ok = ::djinterp::testing::mc_check_failed(#__VA_ARGS__,           \
                                                      __FILE__,               \
                                                      __LINE__);              \
        }                                                                     \
    } while (0)

// D_MC_STATIC_CHECK
//   macro: compile-time assertion, for properties a run-time read cannot
// witness - constant-expression usability and type identity.
#define D_MC_STATIC_CHECK(...)                                                \
    static_assert((__VA_ARGS__), #__VA_ARGS__)

// D_MC_CV_REF_AGREE
//   macro: asserts a unary bool trait answers EXPECT for TYPE in all seven cv-
// and reference-qualified spellings - the clean_t contract.  TYPE must be a
// single token or an alias, never a comma-bearing template-id.
#define D_MC_CV_REF_AGREE(TRAIT, TYPE, EXPECT)                                \
    do                                                                        \
    {                                                                         \
        D_MC_CHECK(TRAIT<TYPE>::value                == (EXPECT));            \
        D_MC_CHECK(TRAIT<const TYPE>::value          == (EXPECT));            \
        D_MC_CHECK(TRAIT<volatile TYPE>::value       == (EXPECT));            \
        D_MC_CHECK(TRAIT<const volatile TYPE>::value == (EXPECT));            \
        D_MC_CHECK(TRAIT<TYPE&>::value               == (EXPECT));            \
        D_MC_CHECK(TRAIT<const TYPE&>::value         == (EXPECT));            \
        D_MC_CHECK(TRAIT<TYPE&&>::value              == (EXPECT));            \
    } while (0)

// D_MC_CV_REF_VALUE
//   macro: the same seven spellings, for a trait whose ::value is a grade or a
// restriction rather than a bool.  One expansion, two names, so a reader can
// see at a glance which kind of trait is under test.
#define D_MC_CV_REF_VALUE(TRAIT, TYPE, EXPECT)                                \
    D_MC_CV_REF_AGREE(TRAIT, TYPE, EXPECT)


// ===========================================================================
// II.  COMMA-FREE ALIASES FOR THE std BATTERY
// ===========================================================================
//   Multi-argument template-ids cannot cross a macro boundary intact, so every
// type the check macros touch gets a single-token spelling here.

using DMcVectorInt   = std::vector<int>;
using DMcVectorBool  = std::vector<bool>;
using DMcDequeInt    = std::deque<int>;
using DMcListInt     = std::list<int>;
using DMcFlistInt    = std::forward_list<int>;
using DMcString       = std::string;
using DMcArray4      = std::array<int, 4>;
using DMcArray0      = std::array<int, 0>;

using DMcSetInt      = std::set<int>;
using DMcMultisetInt = std::multiset<int>;
using DMcMapIi       = std::map<int, int>;
using DMcMultimapIi  = std::multimap<int, int>;
using DMcUsetInt     = std::unordered_set<int>;
using DMcUmapIi      = std::unordered_map<int, int>;
using DMcMapSi       = std::map<std::string, int>;

using DMcInitList    = std::initializer_list<int>;
using DMcPairIi      = std::pair<int, int>;
using DMcVectorPair  = std::vector<std::pair<int, int>>;
using DMcTupleI      = std::tuple<int>;
using DMcIntPtr      = int*;
using DMcIntArray    = int[4];

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using DMcStringView = std::string_view;
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    using DMcSpanInt = std::span<int>;
#endif


// ===========================================================================
// III. PROBE FIXTURES
// ===========================================================================

// --- the two matrix probes (value_type dependency of every signal) ---

// DMcAllSignals
//   struct: every mutator the header probes for, plus the nested value_type
// the value_type-dependent probes require - all eight signals must read true.
struct DMcAllSignals
{
    using value_type = int;

    int* begin();
    void push_back(
        int _value
    );
    void push_front(
        int _value
    );
    void insert(
        int _value
    );
    void erase(
        int* _pos
    );
    void clear();
    void resize(
        std::size_t _count
    );
    int& operator[](
        std::size_t _index
    );
    int* data();
};

// mc_no_value_type
//   struct: DMcAllSignals with the nested value_type removed.  The five
// probes that name value_type must go dark; erase / clear / resize, whose
// probe expressions never mention it, must stay lit.
struct mc_no_value_type
{
    int* begin();
    void push_back(
        int _value
    );
    void push_front(
        int _value
    );
    void insert(
        int _value
    );
    void erase(
        int* _pos
    );
    void clear();
    void resize(
        std::size_t _count
    );
    int& operator[](
        std::size_t _index
    );
    int* data();
};

// --- structural single-signal probes (one lit signal each) ---

// DMcPushBackOnly
//   struct: push_back alone - the minimum that promotes to structural.
struct DMcPushBackOnly
{
    using value_type = int;

    void push_back(
        int _value
    );
};

// DMcPushFrontOnly
//   struct: push_front alone.
struct DMcPushFrontOnly
{
    using value_type = int;

    void push_front(
        int _value
    );
};

// DMcInsertOnly
//   struct: the single-argument insert alone.
struct DMcInsertOnly
{
    using value_type = int;

    void insert(
        int _value
    );
};

// DMcEraseOnly
//   struct: erase alone.  Carries begin(), which the erase probe calls to
// manufacture its argument.
struct DMcEraseOnly
{
    using value_type = int;

    int* begin();
    void erase(
        int* _pos
    );
};

// DMcClearOnly
//   struct: clear alone, and deliberately no value_type - clear's probe never
// names one.
struct DMcClearOnly
{
    void clear();
};

// DMcResizeOnly
//   struct: resize alone, and deliberately no value_type.
struct DMcResizeOnly
{
    void resize(
        std::size_t _count
    );
};

// --- element single-signal probes ---

// DMcSubscriptOnly
//   struct: a writable operator[] alone.
struct DMcSubscriptOnly
{
    using value_type = int;

    int& operator[](
        std::size_t _index
    );
};

// DMcDataOnly
//   struct: a writable data() alone.
struct DMcDataOnly
{
    using value_type = int;

    int* data();
};

// --- near-miss probes (each must NOT be detected) ---

// DMcEraseNoBegin
//   struct: erase without begin() - the probe cannot build its argument, so
// the signal stays dark even though the member exists.
struct DMcEraseNoBegin
{
    using value_type = int;

    void erase(
        int* _pos
    );
};

// DMcInsertTwoArg
//   struct: a positional two-argument insert - not the single-argument form
// the probe calls.  This is why std::vector reads insert-dark.
struct DMcInsertTwoArg
{
    using value_type = int;

    int* begin();
    void insert(
        int* _pos,
        int  _value
    );
};

// DMcConstSubscriptOnly
//   struct: operator[] yielding a const lvalue - readable, not writable.
struct DMcConstSubscriptOnly
{
    using value_type = int;

    const int& operator[](
        std::size_t _index
    ) const;
};

// DMcConstDataOnly
//   struct: data() yielding a pointer-to-const.
struct DMcConstDataOnly
{
    using value_type = int;

    const int* data() const;
};

// DMcVoidData
//   struct: data() yielding void* - the probe's dereference is ill-formed.
struct DMcVoidData
{
    using value_type = int;

    void* data();
};

// DMcTag
//   struct: an element type assignable from nothing, for the mismatch probe.
struct DMcTag
{
};

// DMcSubscriptWrongValue
//   struct: a writable operator[] whose lvalue is NOT assignable from
// value_type - the probe is an assignment, so the mismatch is caught.
struct DMcSubscriptWrongValue
{
    using value_type = DMcTag;

    int& operator[](
        std::size_t _index
    );
};

// --- container-hood probes (the size() marker) ---

// DMcSizedOnly
//   struct: a size() and nothing else - looks like a container, mutates in no
// way, so the one shape is_immutable_container exists to name.
struct DMcSizedOnly
{
    std::size_t size() const;
};

// DMcBare
//   struct: neither container-like nor mutable - immutable GRADE, but not an
// immutable CONTAINER, since container-hood was never established.
struct DMcBare
{
};

// DMcImmutableSized
//   struct: a const-observation container - size() plus a read-only
// operator[].
struct DMcImmutableSized
{
    using value_type = int;

    std::size_t size() const;
    const int& operator[](
        std::size_t _index
    ) const;
};

// --- realistic discipline shapes ---

// DMcFixedBuffer
//   struct: a bounded inline buffer that still grows within its capacity -
// fully_mutable without dynamic storage, the header's storage-orthogonality
// claim made concrete.
struct DMcFixedBuffer
{
    using value_type = int;

    std::size_t size() const;
    void push_back(
        int _value
    );
    int& operator[](
        std::size_t _index
    );
};

// DMcElementOnlyFixed
//   struct: a fixed inline array - elements overwrite, the set of them cannot
// change.
struct DMcElementOnlyFixed
{
    using value_type = int;

    std::size_t size() const;
    int& operator[](
        std::size_t _index
    );
};

// DMcSetShaped
//   struct: the set / multiset discipline - structural change only, element
// coordinate frozen.
struct DMcSetShaped
{
    using key_type   = int;
    using value_type = int;

    std::size_t size() const;
    void insert(
        int _value
    );
    void clear();
};

// DMcMapShaped
//   struct: the map / multimap discipline - structural change only, key
// coordinate frozen.
struct DMcMapShaped
{
    using key_type   = int;
    using value_type = std::pair<const int, int>;

    std::size_t size() const;
    void insert(
        std::pair<const int, int> _entry
    );
    void clear();
};

// --- access-dimension probes (grade held out of the way) ---

// DMcKeyTypeOnly
//   struct: the duplicate-equivalence mark alone.
struct DMcKeyTypeOnly
{
    using key_type = int;
};

// mc_pair_value_only
//   struct: the keyed mark alone.
struct mc_pair_value_only
{
    using value_type = std::pair<const int, int>;
};

// DMcKeyAndPair
//   struct: both marks - keyed must win, since a map carries a key_type too.
struct DMcKeyAndPair
{
    using key_type   = int;
    using value_type = std::pair<const int, int>;
};

// mc_plain_value_only
//   struct: a value_type that is not a pair and no key_type - no coordinate
// is frozen.
struct mc_plain_value_only
{
    using value_type = int;
};

// DMcFirstOnly
//   struct: half of the pair protocol, for the AND-shape probe below.
struct DMcFirstOnly
{
    using first_type = int;
};

// DMcSecondOnly
//   struct: the other half.
struct DMcSecondOnly
{
    using second_type = int;
};

// mc_half_pair_first
//   struct: a value_type carrying first_type but not second_type - the keyed
// detector requires BOTH, so this must not read as keyed.
struct mc_half_pair_first
{
    using value_type = DMcFirstOnly;
};

// mc_half_pair_second
//   struct: a value_type carrying second_type but not first_type.
struct mc_half_pair_second
{
    using value_type = DMcSecondOnly;
};

// --- independence witnesses (the two dimensions computed separately) ---

// DMcFullAndElementConst
//   struct: fully_mutable on the grade axis while the discipline freezes the
// ELEMENT coordinate - a combination no std container occupies, and precisely
// why the two dimensions are read by separate traits.
struct DMcFullAndElementConst
{
    using key_type   = int;
    using value_type = int;

    std::size_t size() const;
    void push_back(
        int _value
    );
    int& operator[](
        std::size_t _index
    );
};

// DMcElementAndKeyConst
//   struct: element_mutable on the grade axis while the discipline freezes the
// KEY coordinate.  The keyed mark is structural - a pair value_type - and does
// not require the key to be const-qualified.
struct DMcElementAndKeyConst
{
    using value_type = std::pair<int, int>;

    value_type* data();
};

// DMcElementAndElementConst
//   struct: element_mutable on the grade axis while the discipline freezes the
// ELEMENT coordinate.  Fills the last cell of the grade x access grid, so all
// twelve combinations are witnessed by some type.
struct DMcElementAndElementConst
{
    using key_type   = int;
    using value_type = int;

    int& operator[](
        std::size_t _index
    );
};


// ===========================================================================
// IV.  TEST DECLARATIONS
// ===========================================================================

// I.   SFINAE mutator detection            (..._tests_signals.cpp)
bool tests_has_push_back_signal();
bool tests_has_push_front_signal();
bool tests_has_insert_signal();
bool tests_has_erase_signal();
bool tests_has_clear_signal();
bool tests_has_resize_signal();
bool tests_has_mutable_subscript_signal();
bool tests_has_mutable_data_signal();
bool tests_signal_value_type_dependency();
bool tests_signal_cv_ref_invariance();
bool tests_signal_value_companions();

// II.  Capability umbrellas                (..._tests_umbrellas.cpp)
bool tests_is_element_mutable_container();
bool tests_is_structurally_mutable_container();
bool tests_is_mutable_container();
bool tests_is_immutable_container();
bool tests_umbrella_signal_composition();
bool tests_umbrella_cv_ref_invariance();
bool tests_umbrella_value_companions();

// III. The mutability grade                (..._tests_grade.cpp)
bool tests_mutability_enum();
bool tests_mutability_name();
bool tests_mutability_grade_predicates();
bool tests_mutability_grade_admits();
bool tests_mutability_of();
bool tests_mutability_of_companions();
bool tests_mutability_grade_aliases();
bool tests_mutability_of_umbrella_agreement();
bool tests_mutability_of_cv_ref_invariance();

// IV.  The access dimension                (..._tests_access.cpp)
bool tests_access_internal_marks();
bool tests_access_restriction_enum();
bool tests_access_restriction_name();
bool tests_access_restriction_of();
bool tests_access_restriction_of_companions();
bool tests_constrains_element();
bool tests_constrains_key();
bool tests_access_restriction_cv_ref_invariance();
bool tests_access_grade_orthogonality();

// V.   Aggregate snapshot                  (..._tests_aggregate.cpp)
bool tests_mutable_container_class_signals();
bool tests_mutable_container_class_capabilities();
bool tests_mutable_container_class_grade();
bool tests_mutable_container_class_access();
bool tests_mutable_container_class_agreement();
bool tests_mutable_container_class_cv_ref_invariance();
bool tests_mutable_container_class_edge_types();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_MUTABLE_CONTAINER_TRAITS_TESTS_
