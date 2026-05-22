/******************************************************************************
* djinterp [test]                                    dtuple_tests_selection.cpp
*
*   Unit tests for the type-selection section of dtuple.hpp:
*     - type_case
*     - type_selector / type_select_t / type_matched_v
*
*   type_selector picks the first matching `type_case` from a parameter
* pack.  Coverage targets:
*     - the empty-pack specialization (no cases supplied -> type=void,
*       matched=false)
*     - the primary template path for non-type_case arguments (also
*       type=void, matched=false)
*     - first-match-wins semantics when multiple `true` cases coexist
*     - the no-match cascade where every case is `false`
*     - mixed-match cascades where the first match appears deep in the
*       list
*
*   type_case is exercised directly (condition / type members) and
* indirectly through every type_selector test below.
*
*
* path:      /inc/djinterp/test/dtuple_tests_selection.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   TYPE_CASE  (compile-time)
// =========================================================================

// condition and type members
static_assert(type_case<true,  int>::condition == true,
              "type_case<true, int>::condition should be true");
static_assert(type_case<false, int>::condition == false,
              "type_case<false, int>::condition should be false");
static_assert(std::is_same<typename type_case<true,  int>::type, int>::value,
              "type_case<true, int>::type should be int");
static_assert(std::is_same<typename type_case<false, char>::type, char>::value,
              "type_case<false, char>::type should be char (regardless of condition)");
static_assert(std::is_same<typename type_case<true, std::tuple<int, char>>::type,
                           std::tuple<int, char>>::value,
              "type_case<true, std::tuple<int, char>>::type should be the same tuple");


// =========================================================================
// II.  TYPE_SELECTOR  (compile-time)
// =========================================================================

// ---- empty: <> specialization ----
static_assert(std::is_same<typename type_selector<>::type, void>::value,
              "type_selector<>::type should be void");
static_assert(type_selector<>::matched == false,
              "type_selector<>::matched should be false");

// ---- primary template (non-type_case args fall here and yield void / false) ----
static_assert(std::is_same<typename type_selector<int>::type, void>::value,
              "type_selector<int>::type (primary, non-type_case) should be void");
static_assert(type_selector<int>::matched == false,
              "type_selector<int>::matched (primary, non-type_case) should be false");

// ---- single type_case ----
static_assert(std::is_same<typename type_selector<type_case<true,  int>>::type, int>::value,
              "type_selector<type_case<true,  int>>::type should be int");
static_assert(type_selector<type_case<true,  int>>::matched == true,
              "type_selector<type_case<true,  int>>::matched should be true");
static_assert(std::is_same<typename type_selector<type_case<false, int>>::type, void>::value,
              "type_selector<type_case<false, int>>::type should be void");
static_assert(type_selector<type_case<false, int>>::matched == false,
              "type_selector<type_case<false, int>>::matched should be false");

// ---- first-match-wins (multiple true) ----
static_assert(std::is_same<typename type_selector<type_case<true,  int>,
                                                   type_case<true,  char>>::type,
                           int>::value,
              "type_selector<true int, true char>::type should be int (first match wins)");
static_assert(type_selector<type_case<true,  int>,
                            type_case<true,  char>>::matched == true,
              "type_selector<true int, true char>::matched should be true");

// ---- match in the SECOND position ----
static_assert(std::is_same<typename type_selector<type_case<false, int>,
                                                   type_case<true,  char>>::type,
                           char>::value,
              "type_selector<false int, true char>::type should be char (second matches)");
static_assert(type_selector<type_case<false, int>,
                            type_case<true,  char>>::matched == true,
              "type_selector<false int, true char>::matched should be true");

// ---- no match across the entire cascade ----
static_assert(std::is_same<typename type_selector<type_case<false, int>,
                                                   type_case<false, char>,
                                                   type_case<false, float>>::type,
                           void>::value,
              "type_selector<all false>::type should be void");
static_assert(type_selector<type_case<false, int>,
                            type_case<false, char>,
                            type_case<false, float>>::matched == false,
              "type_selector<all false>::matched should be false");

// ---- deep match (last in line) ----
static_assert(std::is_same<typename type_selector<type_case<false, int>,
                                                   type_case<false, char>,
                                                   type_case<false, float>,
                                                   type_case<true,  alpha>>::type,
                           alpha>::value,
              "type_selector<f, f, f, t alpha>::type should be alpha");
static_assert(type_selector<type_case<false, int>,
                            type_case<false, char>,
                            type_case<false, float>,
                            type_case<true,  alpha>>::matched == true,
              "type_selector<f, f, f, t alpha>::matched should be true");

// ---- middle-of-list match ----
static_assert(std::is_same<typename type_selector<type_case<false, int>,
                                                   type_case<true,  bravo>,
                                                   type_case<true,  charlie>,
                                                   type_case<false, delta>>::type,
                           bravo>::value,
              "type_selector<f, t bravo, t charlie, f>::type should be bravo (first match)");
static_assert(type_selector<type_case<false, int>,
                            type_case<true,  bravo>,
                            type_case<true,  charlie>,
                            type_case<false, delta>>::matched == true,
              "type_selector<f, t bravo, t charlie, f>::matched should be true");

// ---- type_select_t alias consistency ----
static_assert(std::is_same<type_select_t<>, void>::value,
              "type_select_t<> should be void");
static_assert(std::is_same<type_select_t<type_case<true, int>>, int>::value,
              "type_select_t<type_case<true, int>> should be int");
static_assert(std::is_same<type_select_t<type_case<false, int>,
                                          type_case<true,  char>>,
                           char>::value,
              "type_select_t<f int, t char> should be char");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(type_matched_v<>                                       == false,
                  "type_matched_v<> should be false");
    static_assert(type_matched_v<type_case<true,  int>>                  == true,
                  "type_matched_v<type_case<true,  int>> should be true");
    static_assert(type_matched_v<type_case<false, int>>                  == false,
                  "type_matched_v<type_case<false, int>> should be false");
    static_assert(type_matched_v<type_case<false, int>,
                                 type_case<true,  char>>                 == true,
                  "type_matched_v<f int, t char> should be true");
#endif


// =========================================================================
// III. RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_selection_all(
    test_handler& _test_handler
)
{
    // ---- type_case ----
    record_assertion(_test_handler, 
        type_case<true, int>::condition == true,
        "type_case<true, int>::condition is true");
    record_assertion(_test_handler, 
        type_case<false, int>::condition == false,
        "type_case<false, int>::condition is false");
    record_assertion(_test_handler, 
        std::is_same<typename type_case<false, char>::type, char>::value,
        "type_case::type is exposed regardless of condition");

    // ---- type_selector ----
    record_assertion(_test_handler, 
        ( std::is_same<typename type_selector<>::type, void>::value &&
          (type_selector<>::matched == false) ),
        "type_selector<> -> void / false");
    record_assertion(_test_handler, 
        ( std::is_same<typename type_selector<int>::type, void>::value &&
          (type_selector<int>::matched == false) ),
        "type_selector<int> (primary, non-case) -> void / false");
    record_assertion(_test_handler, 
        ( std::is_same<typename type_selector<type_case<true, int>>::type, int>::value &&
          (type_selector<type_case<true, int>>::matched == true) ),
        "type_selector<true int> -> int / true");
    record_assertion(_test_handler, 
        ( std::is_same<typename type_selector<type_case<false, int>>::type, void>::value &&
          (type_selector<type_case<false, int>>::matched == false) ),
        "type_selector<false int> -> void / false");
    record_assertion(_test_handler, 
        std::is_same<typename type_selector<type_case<true, int>,
                                             type_case<true, char>>::type, int>::value,
        "type_selector<true int, true char> -> int (first match)");
    record_assertion(_test_handler, 
        std::is_same<typename type_selector<type_case<false, int>,
                                             type_case<true, char>>::type, char>::value,
        "type_selector<false int, true char> -> char");
    record_assertion(_test_handler, 
        ( std::is_same<typename type_selector<type_case<false, int>,
                                               type_case<false, char>,
                                               type_case<false, float>>::type, void>::value &&
          (type_selector<type_case<false, int>,
                         type_case<false, char>,
                         type_case<false, float>>::matched == false) ),
        "type_selector<all false> -> void / false");
    record_assertion(_test_handler, 
        std::is_same<typename type_selector<type_case<false, int>,
                                             type_case<false, char>,
                                             type_case<false, float>,
                                             type_case<true,  alpha>>::type, alpha>::value,
        "type_selector<f, f, f, t alpha> -> alpha (last match)");
    record_assertion(_test_handler, 
        std::is_same<typename type_selector<type_case<false, int>,
                                             type_case<true,  bravo>,
                                             type_case<true,  charlie>,
                                             type_case<false, delta>>::type, bravo>::value,
        "type_selector<f, t bravo, t charlie, f> -> bravo (first true wins)");
    record_assertion(_test_handler, 
        std::is_same<type_select_t<type_case<false, int>,
                                    type_case<true,  char>>, char>::value,
        "type_select_t: alias consistent with ::type");

    return;
}


NS_END  // testing
NS_END  // djinterp
