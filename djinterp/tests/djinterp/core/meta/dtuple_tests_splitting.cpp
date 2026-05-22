/******************************************************************************
* djinterp [test]                                    dtuple_tests_splitting.cpp
*
*   Unit tests for the tuple-splitting section of dtuple.hpp:
*     - tuple_split   (with its `before` / `after` member exposures)
*     - tuple_subsequence / tuple_subsequence_t
*
*   tuple_split must satisfy:
*     - index 0 on a tuple of size N: before = empty, after = whole input
*     - index N on a tuple of size N: before = whole input, after = empty
*     - any intermediate index splits cleanly without losing elements,
*       preserving order within each half
*     - index == size on the empty tuple (i.e. 0 / 0) is the trivial
*       both-empty case
*
*   tuple_subsequence is the half-open `[Start, End)` slice variant.
* The implementation comment in dtuple.hpp explains an MSVC ambiguity
* in the older recursive helper that was rewritten as an index_sequence
* lookup -- we verify the rewritten implementation by hitting every
* relevant Start/End combination on a five-element fixture tuple,
* including:
*     - empty slice (Start == End)
*     - whole-tuple slice (0 .. size)
*     - prefix, middle, and suffix slices
*     - the trailing-empty case (Start == End == size) that triggered
*       the original ambiguity
*
*
* path:      /inc/djinterp/test/dtuple_tests_splitting.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   TUPLE_SPLIT  (compile-time)
// =========================================================================

// ---- empty input ----

static_assert(std::is_same<typename tuple_split<0, std::tuple<>>::before,
                           std::tuple<>>::value,
              "tuple_split<0, std::tuple<>>::before should be std::tuple<>");
static_assert(std::is_same<typename tuple_split<0, std::tuple<>>::after,
                           std::tuple<>>::value,
              "tuple_split<0, std::tuple<>>::after should be std::tuple<>");

// ---- pack-shape, size 3 (int, char, float) ----

// index 0 -- empty before, full after
static_assert(std::is_same<typename tuple_split<0, int, char, float>::before,
                           std::tuple<>>::value,
              "tuple_split<0, int, char, float>::before should be std::tuple<>");
static_assert(std::is_same<typename tuple_split<0, int, char, float>::after,
                           std::tuple<int, char, float>>::value,
              "tuple_split<0, int, char, float>::after should be std::tuple<int, char, float>");

// index 1 -- before has the first element
static_assert(std::is_same<typename tuple_split<1, int, char, float>::before,
                           std::tuple<int>>::value,
              "tuple_split<1, int, char, float>::before should be std::tuple<int>");
static_assert(std::is_same<typename tuple_split<1, int, char, float>::after,
                           std::tuple<char, float>>::value,
              "tuple_split<1, int, char, float>::after should be std::tuple<char, float>");

// index 2 -- before has the first two, after has the last
static_assert(std::is_same<typename tuple_split<2, int, char, float>::before,
                           std::tuple<int, char>>::value,
              "tuple_split<2, int, char, float>::before should be std::tuple<int, char>");
static_assert(std::is_same<typename tuple_split<2, int, char, float>::after,
                           std::tuple<float>>::value,
              "tuple_split<2, int, char, float>::after should be std::tuple<float>");

// index 3 (= size) -- full before, empty after
static_assert(std::is_same<typename tuple_split<3, int, char, float>::before,
                           std::tuple<int, char, float>>::value,
              "tuple_split<3, int, char, float>::before should be the whole input");
static_assert(std::is_same<typename tuple_split<3, int, char, float>::after,
                           std::tuple<>>::value,
              "tuple_split<3, int, char, float>::after should be std::tuple<>");

// ---- tuple-shape, size 3 (same expected output as pack-shape) ----

static_assert(std::is_same<typename tuple_split<0, std::tuple<int, char, float>>::before,
                           std::tuple<>>::value,
              "tuple_split tuple-shape index 0 ::before");
static_assert(std::is_same<typename tuple_split<2, std::tuple<int, char, float>>::after,
                           std::tuple<float>>::value,
              "tuple_split tuple-shape index 2 ::after");
static_assert(std::is_same<typename tuple_split<3, std::tuple<int, char, float>>::before,
                           std::tuple<int, char, float>>::value,
              "tuple_split tuple-shape index 3 ::before");

// ---- pack-shape, size 5 -- larger sample to exercise deeper recursion ----

static_assert(std::is_same<typename tuple_split<2, int, char, float, double, long>::before,
                           std::tuple<int, char>>::value,
              "tuple_split<2, 5-pack>::before should be std::tuple<int, char>");
static_assert(std::is_same<typename tuple_split<2, int, char, float, double, long>::after,
                           std::tuple<float, double, long>>::value,
              "tuple_split<2, 5-pack>::after should be std::tuple<float, double, long>");
static_assert(std::is_same<typename tuple_split<4, int, char, float, double, long>::before,
                           std::tuple<int, char, float, double>>::value,
              "tuple_split<4, 5-pack>::before should be std::tuple<int, char, float, double>");
static_assert(std::is_same<typename tuple_split<4, int, char, float, double, long>::after,
                           std::tuple<long>>::value,
              "tuple_split<4, 5-pack>::after should be std::tuple<long>");

// ---- size 1 (edge: minimal non-empty) ----

static_assert(std::is_same<typename tuple_split<0, int>::before,
                           std::tuple<>>::value,
              "tuple_split<0, int>::before should be std::tuple<>");
static_assert(std::is_same<typename tuple_split<0, int>::after,
                           std::tuple<int>>::value,
              "tuple_split<0, int>::after should be std::tuple<int>");
static_assert(std::is_same<typename tuple_split<1, int>::before,
                           std::tuple<int>>::value,
              "tuple_split<1, int>::before should be std::tuple<int>");
static_assert(std::is_same<typename tuple_split<1, int>::after,
                           std::tuple<>>::value,
              "tuple_split<1, int>::after should be std::tuple<>");

// ---- preserves cv-/ref-qualified element types ----

static_assert(std::is_same<typename tuple_split<1, std::tuple<const int, int&, int*>>::before,
                           std::tuple<const int>>::value,
              "tuple_split preserves cv on element 0");
static_assert(std::is_same<typename tuple_split<1, std::tuple<const int, int&, int*>>::after,
                           std::tuple<int&, int*>>::value,
              "tuple_split preserves ref + pointer on element 1, 2");

// ---- tuple_split_t alias ----

static_assert(std::is_same<typename tuple_split_t<1, std::tuple<int, char, float>>::before,
                           std::tuple<int>>::value,
              "tuple_split_t::before alias");
static_assert(std::is_same<typename tuple_split_t<1, std::tuple<int, char, float>>::after,
                           std::tuple<char, float>>::value,
              "tuple_split_t::after alias");


// =========================================================================
// II.  TUPLE_SUBSEQUENCE  (compile-time)
// =========================================================================

// fixture: a five-element tuple lets us hit every Start, End combo
using subseq_fixture = std::tuple<int, char, float, double, long>;

// ---- empty input ----

static_assert(std::is_same<typename tuple_subsequence<0, 0, std::tuple<>>::type,
                           std::tuple<>>::value,
              "tuple_subsequence<0, 0, std::tuple<>>::type should be std::tuple<>");

// ---- empty slice (Start == End) at various positions ----

static_assert(std::is_same<typename tuple_subsequence<0, 0, subseq_fixture>::type,
                           std::tuple<>>::value,
              "tuple_subsequence<0, 0, fixture>::type should be empty (Start == End at 0)");
static_assert(std::is_same<typename tuple_subsequence<2, 2, subseq_fixture>::type,
                           std::tuple<>>::value,
              "tuple_subsequence<2, 2, fixture>::type should be empty (Start == End in middle)");
static_assert(std::is_same<typename tuple_subsequence<5, 5, subseq_fixture>::type,
                           std::tuple<>>::value,
              "tuple_subsequence<5, 5, fixture>::type should be empty (Start == End at size)");

// ---- single-element slices ----

static_assert(std::is_same<typename tuple_subsequence<0, 1, subseq_fixture>::type,
                           std::tuple<int>>::value,
              "tuple_subsequence<0, 1, fixture>::type should be std::tuple<int>");
static_assert(std::is_same<typename tuple_subsequence<2, 3, subseq_fixture>::type,
                           std::tuple<float>>::value,
              "tuple_subsequence<2, 3, fixture>::type should be std::tuple<float>");
static_assert(std::is_same<typename tuple_subsequence<4, 5, subseq_fixture>::type,
                           std::tuple<long>>::value,
              "tuple_subsequence<4, 5, fixture>::type should be std::tuple<long>");

// ---- prefix slice ----

static_assert(std::is_same<typename tuple_subsequence<0, 3, subseq_fixture>::type,
                           std::tuple<int, char, float>>::value,
              "tuple_subsequence<0, 3, fixture>::type should be std::tuple<int, char, float>");

// ---- suffix slice ----

static_assert(std::is_same<typename tuple_subsequence<2, 5, subseq_fixture>::type,
                           std::tuple<float, double, long>>::value,
              "tuple_subsequence<2, 5, fixture>::type should be std::tuple<float, double, long>");

// ---- middle slice ----

static_assert(std::is_same<typename tuple_subsequence<1, 4, subseq_fixture>::type,
                           std::tuple<char, float, double>>::value,
              "tuple_subsequence<1, 4, fixture>::type should be std::tuple<char, float, double>");

// ---- whole-tuple slice ----

static_assert(std::is_same<typename tuple_subsequence<0, 5, subseq_fixture>::type,
                           subseq_fixture>::value,
              "tuple_subsequence<0, 5, fixture>::type should be the whole fixture");

// ---- cv-/ref-qualified element types preserved ----

static_assert(std::is_same<typename tuple_subsequence<1, 3,
                                std::tuple<int, const char, int&, float*>>::type,
                           std::tuple<const char, int&>>::value,
              "tuple_subsequence preserves cv + ref qualifiers in extracted range");

// ---- tuple_subsequence_t alias consistency ----

static_assert(std::is_same<tuple_subsequence_t<1, 4, subseq_fixture>,
                           std::tuple<char, float, double>>::value,
              "tuple_subsequence_t: alias");
static_assert(std::is_same<tuple_subsequence_t<0, 0, std::tuple<>>,
                           std::tuple<>>::value,
              "tuple_subsequence_t<0, 0, std::tuple<>> should be std::tuple<>");


// =========================================================================
// III. RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_splitting_all(
    test_handler& _test_handler
)
{
    // ---- tuple_split ----
    record_assertion(_test_handler, 
        ( std::is_same<typename tuple_split<0, std::tuple<>>::before,
                       std::tuple<>>::value &&
          std::is_same<typename tuple_split<0, std::tuple<>>::after,
                       std::tuple<>>::value ),
        "tuple_split: empty input both halves empty");
    record_assertion(_test_handler, 
        ( std::is_same<typename tuple_split<0, int, char, float>::before,
                       std::tuple<>>::value &&
          std::is_same<typename tuple_split<0, int, char, float>::after,
                       std::tuple<int, char, float>>::value ),
        "tuple_split<0>: empty before, whole input after");
    record_assertion(_test_handler, 
        ( std::is_same<typename tuple_split<1, int, char, float>::before,
                       std::tuple<int>>::value &&
          std::is_same<typename tuple_split<1, int, char, float>::after,
                       std::tuple<char, float>>::value ),
        "tuple_split<1>: one before, two after");
    record_assertion(_test_handler, 
        ( std::is_same<typename tuple_split<2, int, char, float>::before,
                       std::tuple<int, char>>::value &&
          std::is_same<typename tuple_split<2, int, char, float>::after,
                       std::tuple<float>>::value ),
        "tuple_split<2>: two before, one after");
    record_assertion(_test_handler, 
        ( std::is_same<typename tuple_split<3, int, char, float>::before,
                       std::tuple<int, char, float>>::value &&
          std::is_same<typename tuple_split<3, int, char, float>::after,
                       std::tuple<>>::value ),
        "tuple_split<3>: whole input before, empty after");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_split<2, int, char, float, double, long>::before,
                     std::tuple<int, char>>::value,
        "tuple_split<2, 5-pack>: before");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_split<2, int, char, float, double, long>::after,
                     std::tuple<float, double, long>>::value,
        "tuple_split<2, 5-pack>: after");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_split<1, std::tuple<const int, int&, int*>>::before,
                     std::tuple<const int>>::value,
        "tuple_split: preserves cv on element 0");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_split<1, std::tuple<const int, int&, int*>>::after,
                     std::tuple<int&, int*>>::value,
        "tuple_split: preserves ref + pointer in after-half");

    // ---- tuple_subsequence ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<0, 0, std::tuple<>>::type,
                     std::tuple<>>::value,
        "tuple_subsequence: empty input empty range");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<0, 0, subseq_fixture>::type,
                     std::tuple<>>::value,
        "tuple_subsequence: empty range at start");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<2, 2, subseq_fixture>::type,
                     std::tuple<>>::value,
        "tuple_subsequence: empty range in middle");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<5, 5, subseq_fixture>::type,
                     std::tuple<>>::value,
        "tuple_subsequence: empty range at end (the previously-ambiguous case)");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<0, 1, subseq_fixture>::type,
                     std::tuple<int>>::value,
        "tuple_subsequence: single-element prefix");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<4, 5, subseq_fixture>::type,
                     std::tuple<long>>::value,
        "tuple_subsequence: single-element suffix");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<0, 3, subseq_fixture>::type,
                     std::tuple<int, char, float>>::value,
        "tuple_subsequence: prefix of length 3");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<2, 5, subseq_fixture>::type,
                     std::tuple<float, double, long>>::value,
        "tuple_subsequence: suffix of length 3");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<1, 4, subseq_fixture>::type,
                     std::tuple<char, float, double>>::value,
        "tuple_subsequence: middle of length 3");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<0, 5, subseq_fixture>::type,
                     subseq_fixture>::value,
        "tuple_subsequence: whole-tuple slice");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_subsequence<1, 3,
                            std::tuple<int, const char, int&, float*>>::type,
                     std::tuple<const char, int&>>::value,
        "tuple_subsequence: preserves cv + ref qualifiers");
    record_assertion(_test_handler, 
        std::is_same<tuple_subsequence_t<1, 4, subseq_fixture>,
                     std::tuple<char, float, double>>::value,
        "tuple_subsequence_t: alias");

    return;
}


NS_END  // testing
NS_END  // djinterp
