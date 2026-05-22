/******************************************************************************
* djinterp [test]                                       dtuple_tests_access.cpp
*
*   Unit tests for the element-access section of dtuple.hpp:
*     - tuple_type_at / tuple_type_at_t   (compile-time index lookup)
*     - tuple_type_at_value               (runtime value retrieval)
*     - tuple_concat                      (runtime tuple concatenation)
*
*   tuple_type_at exercises the recursive helper through both its base
* case (index 0) and the index > 0 reduction.  We probe every valid index
* of small tuples to make sure the index decrement chain terminates
* correctly.
*
*   tuple_type_at_value is the runtime sibling: it delegates to
* `std::get<0>(tuple)` once the recursion bottoms out, so the same
* index sequences should retrieve the same logical values.
*
*   tuple_concat is a thin wrapper over `std::tuple_cat`; tests are
* purposely small (mostly behaviour-equivalence checks) but cover the
* zero-, one-, two-, and three-input cases plus lvalue / rvalue mixing.
*
*
* path:      /inc/djinterp/test/dtuple_tests_access.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   TUPLE_TYPE_AT  (compile-time)
// =========================================================================

// pack-shape: single-element tuple
static_assert(std::is_same<typename tuple_type_at<0, int>::type, int>::value,
              "tuple_type_at<0, int>::type should be int");
static_assert(std::is_same<typename tuple_type_at<0, alpha>::type, alpha>::value,
              "tuple_type_at<0, alpha>::type should be alpha");

// pack-shape: multi-element
static_assert(std::is_same<typename tuple_type_at<0, int, char, float>::type, int>::value,
              "tuple_type_at<0, int, char, float>::type should be int");
static_assert(std::is_same<typename tuple_type_at<1, int, char, float>::type, char>::value,
              "tuple_type_at<1, int, char, float>::type should be char");
static_assert(std::is_same<typename tuple_type_at<2, int, char, float>::type, float>::value,
              "tuple_type_at<2, int, char, float>::type should be float");

// tuple-shape: same answers
static_assert(std::is_same<typename tuple_type_at<0, std::tuple<int, char, float>>::type,
                           int>::value,
              "tuple_type_at<0, std::tuple<int, char, float>>::type should be int");
static_assert(std::is_same<typename tuple_type_at<1, std::tuple<int, char, float>>::type,
                           char>::value,
              "tuple_type_at<1, std::tuple<int, char, float>>::type should be char");
static_assert(std::is_same<typename tuple_type_at<2, std::tuple<int, char, float>>::type,
                           float>::value,
              "tuple_type_at<2, std::tuple<int, char, float>>::type should be float");

// long tuple -- exercises a deeper recursion through tuple_type_at_helper
static_assert(std::is_same<typename tuple_type_at<3, int, char, float, double, long>::type,
                           double>::value,
              "tuple_type_at<3, int, char, float, double, long>::type should be double");
static_assert(std::is_same<typename tuple_type_at<4, int, char, float, double, long>::type,
                           long>::value,
              "tuple_type_at<4, int, char, float, double, long>::type should be long");

// cv-/ref-qualified element types are preserved
static_assert(std::is_same<typename tuple_type_at<1, int, const char&, float*>::type,
                           const char&>::value,
              "tuple_type_at<1, int, const char&, float*>::type should be const char&");
static_assert(std::is_same<typename tuple_type_at<2, int, char, const volatile float*>::type,
                           const volatile float*>::value,
              "tuple_type_at<2, ...>::type should preserve const volatile pointer");

// alias consistency
static_assert(std::is_same<tuple_type_at_t<2, int, char, float, double>, float>::value,
              "tuple_type_at_t<2, int, char, float, double> should be float");
static_assert(std::is_same<tuple_type_at_t<0, std::tuple<alpha, bravo>>, alpha>::value,
              "tuple_type_at_t<0, std::tuple<alpha, bravo>> should be alpha");


// =========================================================================
// II.  RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_access_all(
    test_handler& _test_handler
)
{
    // ---- tuple_type_at (mirrored compile-time) ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<0, int>::type, int>::value,
        "tuple_type_at: index 0 of single-arg pack");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<0, int, char, float>::type, int>::value,
        "tuple_type_at: index 0 of multi-arg pack");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<1, int, char, float>::type, char>::value,
        "tuple_type_at: index 1 of multi-arg pack");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<2, int, char, float>::type, float>::value,
        "tuple_type_at: index 2 of multi-arg pack");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<0, std::tuple<int, char>>::type, int>::value,
        "tuple_type_at: tuple-shape index 0");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<1, std::tuple<int, char>>::type, char>::value,
        "tuple_type_at: tuple-shape index 1");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<4, int, char, float, double, long>::type, long>::value,
        "tuple_type_at: deep recursion (index 4)");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_type_at<1, int, const char&, float*>::type,
                     const char&>::value,
        "tuple_type_at: preserves cv + ref qualifiers");
    record_assertion(_test_handler, 
        std::is_same<tuple_type_at_t<2, int, char, float, double>, float>::value,
        "tuple_type_at_t: alias");

    // ---- tuple_type_at_value (runtime) ----
    {
        const std::tuple<int, char, double> t{42, 'q', 3.14};

        record_assertion(_test_handler, 
            (tuple_type_at_value<0>(t) == 42),
            "tuple_type_at_value: index 0 yields 42");
        record_assertion(_test_handler, 
            (tuple_type_at_value<1>(t) == 'q'),
            "tuple_type_at_value: index 1 yields 'q'");
        record_assertion(_test_handler, 
            (tuple_type_at_value<2>(t) == 3.14),
            "tuple_type_at_value: index 2 yields 3.14");
    }

    // single-element retrieval -- the index-0 base case in isolation
    {
        const std::tuple<int> t_single{7};

        record_assertion(_test_handler, 
            (tuple_type_at_value<0>(t_single) == 7),
            "tuple_type_at_value: single-element tuple index 0");
    }

    // ---- tuple_concat (runtime) ----

    // zero inputs -- yields an empty tuple
    {
        auto cat0 = tuple_concat();

        record_assertion(_test_handler, 
            (std::tuple_size<decltype(cat0)>::value == 0),
            "tuple_concat: zero inputs -> empty tuple");
        record_assertion(_test_handler, 
            std::is_same<decltype(cat0), std::tuple<>>::value,
            "tuple_concat: zero inputs -> std::tuple<>");
    }

    // one input -- the same tuple
    {
        std::tuple<int, char> a{1, 'a'};
        auto cat1 = tuple_concat(a);

        record_assertion(_test_handler, 
            std::is_same<decltype(cat1), std::tuple<int, char>>::value,
            "tuple_concat: single input yields same tuple type");
        record_assertion(_test_handler, 
            ((std::get<0>(cat1) == 1) && (std::get<1>(cat1) == 'a')),
            "tuple_concat: single input preserves values");
    }

    // two inputs -- concatenated
    {
        std::tuple<int, char> a{1, 'a'};
        std::tuple<double>    b{2.5};
        auto cat2 = tuple_concat(a, b);

        record_assertion(_test_handler, 
            std::is_same<decltype(cat2), std::tuple<int, char, double>>::value,
            "tuple_concat: two inputs concatenated");
        record_assertion(_test_handler, 
            ( (std::get<0>(cat2) == 1)   &&
              (std::get<1>(cat2) == 'a') &&
              (std::get<2>(cat2) == 2.5) ),
            "tuple_concat: two inputs preserve values");
    }

    // three inputs including an empty tuple -- empty contributes nothing
    {
        std::tuple<int>  a{1};
        std::tuple<>     b{};
        std::tuple<char> c{'z'};
        auto cat3 = tuple_concat(a, b, c);

        record_assertion(_test_handler, 
            std::is_same<decltype(cat3), std::tuple<int, char>>::value,
            "tuple_concat: empty tuple in the middle drops out");
        record_assertion(_test_handler, 
            ((std::get<0>(cat3) == 1) && (std::get<1>(cat3) == 'z')),
            "tuple_concat: empty-in-middle preserves remaining values");
    }

    // rvalue inputs -- tuple_concat forwards
    {
        auto cat_rv = tuple_concat(std::tuple<int>{9},
                                   std::tuple<char>{'r'});

        record_assertion(_test_handler, 
            std::is_same<decltype(cat_rv), std::tuple<int, char>>::value,
            "tuple_concat: rvalue inputs forwarded");
        record_assertion(_test_handler, 
            ((std::get<0>(cat_rv) == 9) && (std::get<1>(cat_rv) == 'r')),
            "tuple_concat: rvalue inputs preserve values");
    }

    return;
}


NS_END  // testing
NS_END  // djinterp
