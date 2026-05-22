/******************************************************************************
* djinterp [test]                                           dtuple_tests_2d.cpp
*
*   Unit tests for the 2D / jagged-tuple section of dtuple.hpp:
*     - is_2d_tuple / is_2d_tuple_v
*     - tuple_inner_sizes / tuple_inner_sizes_t
*     - tuple_outer_size / tuple_outer_size_v
*     - tuple_flatten_types / tuple_flatten_types_t
*     - is_uniform_2d_tuple / is_uniform_2d_tuple_v
*     - is_jagged_tuple / is_jagged_tuple_v
*     - tuple_total_elements / tuple_total_elements_v
*     - tuple_common_element_type / tuple_common_element_type_t
*     - make_2d_tuple_of / make_2d_tuple_of_t
*     - tuple_row_type / tuple_row_type_t
*     - tuple_row_size / tuple_row_size_v
*
*   Coverage notes:
*     - is_2d_tuple: empty outer is VACUOUSLY true; primary template
*       (non-tuple) is false; tuple of mixed tuple/non-tuple is false.
*       Inner tuples with cv- / ref-qualifiers on the row type are
*       still accepted because the helper applies `clean_t` first.
*     - is_uniform_2d_tuple: vacuously true for empty / single-row;
*       compares sizes pairwise via the all_sizes_equal helper, which
*       has its own empty / single / multi specializations exercised
*       here through deliberate row-count choices.
*     - is_jagged_tuple: requires at least 2 rows AND non-uniform sizes;
*       single-row 2D tuples are NOT jagged (matches documentation).
*     - tuple_common_element_type: empty -> void; otherwise delegates
*       to std::common_type over the flattened pack.
*     - tuple_row_type strips cv on the extracted row via clean_t, so
*       querying a row in a tuple of cv-qualified inner tuples yields
*       the bare row tuple.
*
*
* path:      /inc/djinterp/test/dtuple_tests_2d.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/
#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// ---- common fixtures shared across the section ----

using row_empty = std::tuple<>;
using row_1     = std::tuple<int>;
using row_2     = std::tuple<int, char>;
using row_3     = std::tuple<int, char, float>;

using empty_2d  = std::tuple<>;
using one_row   = std::tuple<row_2>;
using uniform2  = std::tuple<row_2, row_2>;
using uniform3  = std::tuple<row_2, row_2, row_2>;
using jagged    = std::tuple<row_1, row_2, row_3>;
using empty_rows= std::tuple<row_empty, row_empty>;


// =========================================================================
// I.   IS_2D_TUPLE  (compile-time)
// =========================================================================

// primary template (non-tuple) is false
static_assert(is_2d_tuple<int>::value == false,
              "is_2d_tuple<int>::value should be false");
static_assert(is_2d_tuple<alpha>::value == false,
              "is_2d_tuple<alpha>::value should be false");
static_assert(is_2d_tuple<std::pair<int, int>>::value == false,
              "is_2d_tuple<std::pair<int, int>>::value should be false");

// empty outer -- vacuously true
static_assert(is_2d_tuple<std::tuple<>>::value == true,
              "is_2d_tuple<std::tuple<>>::value should be true (vacuous)");

// 1D tuple is NOT 2D
static_assert(is_2d_tuple<std::tuple<int>>::value == false,
              "is_2d_tuple<std::tuple<int>>::value should be false");
static_assert(is_2d_tuple<std::tuple<int, char>>::value == false,
              "is_2d_tuple<std::tuple<int, char>>::value should be false");

// all-tuple rows -- true
static_assert(is_2d_tuple<std::tuple<std::tuple<int>>>::value == true,
              "is_2d_tuple<tuple<tuple<int>>>::value should be true");
static_assert(is_2d_tuple<uniform2>::value == true,
              "is_2d_tuple<uniform2>::value should be true");
static_assert(is_2d_tuple<jagged>::value == true,
              "is_2d_tuple<jagged>::value should be true (jagged is still 2D)");
static_assert(is_2d_tuple<empty_rows>::value == true,
              "is_2d_tuple<empty_rows>::value should be true (rows of empty tuples)");

// mixed tuple + non-tuple rows -- false
static_assert(is_2d_tuple<std::tuple<std::tuple<int>, int>>::value == false,
              "is_2d_tuple<tuple<tuple<int>, int>>::value should be false");
static_assert(is_2d_tuple<std::tuple<int, std::tuple<int>>>::value == false,
              "is_2d_tuple<tuple<int, tuple<int>>>::value should be false");

// rows with cv-/ref-qualified TUPLE types still count -- helper applies clean_t
static_assert(is_2d_tuple<std::tuple<const std::tuple<int>>>::value == true,
              "is_2d_tuple<tuple<const tuple<int>>>::value should be true (clean_t strips cv)");
static_assert(is_2d_tuple<std::tuple<std::tuple<int>&>>::value == true,
              "is_2d_tuple<tuple<tuple<int>&>>::value should be true (clean_t strips ref)");
static_assert(is_2d_tuple<std::tuple<const std::tuple<int>&,
                                      volatile std::tuple<char>>>::value == true,
              "is_2d_tuple<tuple<const tuple<int>&, volatile tuple<char>>>::value should be true");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_2d_tuple_v<empty_2d>            == true,
                  "is_2d_tuple_v<empty_2d> should be true");
    static_assert(is_2d_tuple_v<uniform2>            == true,
                  "is_2d_tuple_v<uniform2> should be true");
    static_assert(is_2d_tuple_v<std::tuple<int>>     == false,
                  "is_2d_tuple_v<tuple<int>> should be false");
    static_assert(is_2d_tuple_v<int>                 == false,
                  "is_2d_tuple_v<int> should be false");
#endif


// =========================================================================
// II.  TUPLE_INNER_SIZES  (compile-time)
// =========================================================================

// empty 2D tuple -- explicit specialization
static_assert(std::is_same<typename tuple_inner_sizes<empty_2d>::type,
                           std::index_sequence<>>::value,
              "tuple_inner_sizes<empty_2d>::type should be std::index_sequence<>");

// single-row 2D tuples
static_assert(std::is_same<typename tuple_inner_sizes<std::tuple<row_empty>>::type,
                           std::index_sequence<0>>::value,
              "tuple_inner_sizes<tuple<row_empty>>::type should be index_sequence<0>");
static_assert(std::is_same<typename tuple_inner_sizes<std::tuple<row_3>>::type,
                           std::index_sequence<3>>::value,
              "tuple_inner_sizes<tuple<row_3>>::type should be index_sequence<3>");

// multi-row uniform
static_assert(std::is_same<typename tuple_inner_sizes<uniform2>::type,
                           std::index_sequence<2, 2>>::value,
              "tuple_inner_sizes<uniform2>::type should be index_sequence<2, 2>");
static_assert(std::is_same<typename tuple_inner_sizes<uniform3>::type,
                           std::index_sequence<2, 2, 2>>::value,
              "tuple_inner_sizes<uniform3>::type should be index_sequence<2, 2, 2>");

// jagged
static_assert(std::is_same<typename tuple_inner_sizes<jagged>::type,
                           std::index_sequence<1, 2, 3>>::value,
              "tuple_inner_sizes<jagged>::type should be index_sequence<1, 2, 3>");

// rows of empty tuples -- sizes are all zero
static_assert(std::is_same<typename tuple_inner_sizes<empty_rows>::type,
                           std::index_sequence<0, 0>>::value,
              "tuple_inner_sizes<empty_rows>::type should be index_sequence<0, 0>");

// cv-/ref-qualified rows -- clean_t strips and reports the underlying size
static_assert(std::is_same<typename tuple_inner_sizes<std::tuple<const std::tuple<int, char>,
                                                                  std::tuple<float>&>>::type,
                           std::index_sequence<2, 1>>::value,
              "tuple_inner_sizes: cv/ref stripped before size measurement");

// tuple_inner_sizes_t alias
static_assert(std::is_same<tuple_inner_sizes_t<uniform2>,
                           std::index_sequence<2, 2>>::value,
              "tuple_inner_sizes_t: alias");


// =========================================================================
// III. TUPLE_OUTER_SIZE  (compile-time)
// =========================================================================

// non-tuple types -- primary template, 0
static_assert(tuple_outer_size<int>::value == 0,
              "tuple_outer_size<int>::value should be 0 (not a tuple)");
static_assert(tuple_outer_size<alpha>::value == 0,
              "tuple_outer_size<alpha>::value should be 0");

// 1D tuples -- specialization returns 0 because is_2d_tuple is false
static_assert(tuple_outer_size<std::tuple<int>>::value == 0,
              "tuple_outer_size<tuple<int>>::value should be 0 (1D, not 2D)");
static_assert(tuple_outer_size<std::tuple<int, char>>::value == 0,
              "tuple_outer_size<tuple<int, char>>::value should be 0 (1D, not 2D)");

// 2D tuples -- row count
static_assert(tuple_outer_size<empty_2d>::value == 0,
              "tuple_outer_size<empty_2d>::value should be 0");
static_assert(tuple_outer_size<one_row>::value == 1,
              "tuple_outer_size<one_row>::value should be 1");
static_assert(tuple_outer_size<uniform2>::value == 2,
              "tuple_outer_size<uniform2>::value should be 2");
static_assert(tuple_outer_size<uniform3>::value == 3,
              "tuple_outer_size<uniform3>::value should be 3");
static_assert(tuple_outer_size<jagged>::value == 3,
              "tuple_outer_size<jagged>::value should be 3");
static_assert(tuple_outer_size<empty_rows>::value == 2,
              "tuple_outer_size<empty_rows>::value should be 2");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(tuple_outer_size_v<uniform3>             == 3,
                  "tuple_outer_size_v<uniform3> should be 3");
    static_assert(tuple_outer_size_v<std::tuple<int>>      == 0,
                  "tuple_outer_size_v<tuple<int>> should be 0");
    static_assert(tuple_outer_size_v<int>                  == 0,
                  "tuple_outer_size_v<int> should be 0");
#endif


// =========================================================================
// IV.  TUPLE_FLATTEN_TYPES  (compile-time)
// =========================================================================

// empty outer
static_assert(std::is_same<typename tuple_flatten_types<empty_2d>::type,
                           std::tuple<>>::value,
              "tuple_flatten_types<empty_2d>::type should be std::tuple<>");

// single row
static_assert(std::is_same<typename tuple_flatten_types<one_row>::type,
                           std::tuple<int, char>>::value,
              "tuple_flatten_types<one_row>::type should be std::tuple<int, char>");

// uniform 2D
static_assert(std::is_same<typename tuple_flatten_types<uniform2>::type,
                           std::tuple<int, char, int, char>>::value,
              "tuple_flatten_types<uniform2>::type should concatenate two row_2");
static_assert(std::is_same<typename tuple_flatten_types<uniform3>::type,
                           std::tuple<int, char, int, char, int, char>>::value,
              "tuple_flatten_types<uniform3>::type should concatenate three row_2");

// jagged
static_assert(std::is_same<typename tuple_flatten_types<jagged>::type,
                           std::tuple<int, int, char, int, char, float>>::value,
              "tuple_flatten_types<jagged>::type should concat row_1 + row_2 + row_3");

// rows of empty tuples -- nothing to flatten
static_assert(std::is_same<typename tuple_flatten_types<empty_rows>::type,
                           std::tuple<>>::value,
              "tuple_flatten_types<empty_rows>::type should be std::tuple<>");

// empty + non-empty mix
static_assert(std::is_same<typename tuple_flatten_types<std::tuple<row_empty, row_2>>::type,
                           std::tuple<int, char>>::value,
              "tuple_flatten_types<tuple<empty, row_2>>::type should be std::tuple<int, char>");
static_assert(std::is_same<typename tuple_flatten_types<std::tuple<row_2, row_empty>>::type,
                           std::tuple<int, char>>::value,
              "tuple_flatten_types<tuple<row_2, empty>>::type should be std::tuple<int, char>");

// alias
static_assert(std::is_same<tuple_flatten_types_t<jagged>,
                           std::tuple<int, int, char, int, char, float>>::value,
              "tuple_flatten_types_t: alias");


// =========================================================================
// V.   IS_UNIFORM_2D_TUPLE  (compile-time)
// =========================================================================

// non-tuple -- primary template, false
static_assert(is_uniform_2d_tuple<int>::value == false,
              "is_uniform_2d_tuple<int>::value should be false");

// non-2D tuple -- false
static_assert(is_uniform_2d_tuple<std::tuple<int>>::value == false,
              "is_uniform_2d_tuple<tuple<int>>::value should be false (not 2D)");

// empty 2D -- vacuously true (all_sizes_equal<empty>)
static_assert(is_uniform_2d_tuple<empty_2d>::value == true,
              "is_uniform_2d_tuple<empty_2d>::value should be true (vacuous)");

// single-row 2D -- vacuously true (all_sizes_equal<single>)
static_assert(is_uniform_2d_tuple<one_row>::value == true,
              "is_uniform_2d_tuple<one_row>::value should be true (single-row vacuous)");
static_assert(is_uniform_2d_tuple<std::tuple<row_3>>::value == true,
              "is_uniform_2d_tuple<tuple<row_3>>::value should be true (single-row vacuous)");

// uniform multi-row -- true
static_assert(is_uniform_2d_tuple<uniform2>::value == true,
              "is_uniform_2d_tuple<uniform2>::value should be true");
static_assert(is_uniform_2d_tuple<uniform3>::value == true,
              "is_uniform_2d_tuple<uniform3>::value should be true");
static_assert(is_uniform_2d_tuple<empty_rows>::value == true,
              "is_uniform_2d_tuple<empty_rows>::value should be true (all 0)");

// jagged -- false
static_assert(is_uniform_2d_tuple<jagged>::value == false,
              "is_uniform_2d_tuple<jagged>::value should be false");
static_assert(is_uniform_2d_tuple<std::tuple<row_1, row_2>>::value == false,
              "is_uniform_2d_tuple<tuple<row_1, row_2>>::value should be false");
static_assert(is_uniform_2d_tuple<std::tuple<row_2, row_3, row_2>>::value == false,
              "is_uniform_2d_tuple<tuple<row_2, row_3, row_2>>::value should be false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_uniform_2d_tuple_v<uniform3>      == true,
                  "is_uniform_2d_tuple_v<uniform3> should be true");
    static_assert(is_uniform_2d_tuple_v<jagged>        == false,
                  "is_uniform_2d_tuple_v<jagged> should be false");
    static_assert(is_uniform_2d_tuple_v<empty_2d>      == true,
                  "is_uniform_2d_tuple_v<empty_2d> should be true");
    static_assert(is_uniform_2d_tuple_v<int>           == false,
                  "is_uniform_2d_tuple_v<int> should be false");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


// =========================================================================
// VI.  IS_JAGGED_TUPLE  (compile-time)
// =========================================================================

// non-tuple
static_assert(is_jagged_tuple<int>::value == false,
              "is_jagged_tuple<int>::value should be false");

// non-2D tuple
static_assert(is_jagged_tuple<std::tuple<int>>::value == false,
              "is_jagged_tuple<tuple<int>>::value should be false (not 2D)");

// empty -- not jagged (< 2 rows)
static_assert(is_jagged_tuple<empty_2d>::value == false,
              "is_jagged_tuple<empty_2d>::value should be false (< 2 rows)");

// single-row -- not jagged (< 2 rows; documented edge case)
static_assert(is_jagged_tuple<one_row>::value == false,
              "is_jagged_tuple<one_row>::value should be false (< 2 rows)");

// uniform multi-row -- not jagged
static_assert(is_jagged_tuple<uniform2>::value == false,
              "is_jagged_tuple<uniform2>::value should be false (uniform)");
static_assert(is_jagged_tuple<uniform3>::value == false,
              "is_jagged_tuple<uniform3>::value should be false (uniform)");

// jagged
static_assert(is_jagged_tuple<jagged>::value == true,
              "is_jagged_tuple<jagged>::value should be true");
static_assert(is_jagged_tuple<std::tuple<row_1, row_2>>::value == true,
              "is_jagged_tuple<tuple<row_1, row_2>>::value should be true");
static_assert(is_jagged_tuple<std::tuple<row_2, row_3>>::value == true,
              "is_jagged_tuple<tuple<row_2, row_3>>::value should be true");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_jagged_tuple_v<jagged>          == true,
                  "is_jagged_tuple_v<jagged> should be true");
    static_assert(is_jagged_tuple_v<uniform2>        == false,
                  "is_jagged_tuple_v<uniform2> should be false");
    static_assert(is_jagged_tuple_v<one_row>         == false,
                  "is_jagged_tuple_v<one_row> should be false");
    static_assert(is_jagged_tuple_v<empty_2d>        == false,
                  "is_jagged_tuple_v<empty_2d> should be false");
#endif


// =========================================================================
// VII. TUPLE_TOTAL_ELEMENTS  (compile-time)
// =========================================================================

static_assert(tuple_total_elements<empty_2d>::value   == 0,
              "tuple_total_elements<empty_2d>::value should be 0");
static_assert(tuple_total_elements<one_row>::value    == 2,
              "tuple_total_elements<one_row>::value should be 2 (row_2 has 2 elements)");
static_assert(tuple_total_elements<uniform2>::value   == 4,
              "tuple_total_elements<uniform2>::value should be 4 (2 rows * 2 elements)");
static_assert(tuple_total_elements<uniform3>::value   == 6,
              "tuple_total_elements<uniform3>::value should be 6 (3 rows * 2 elements)");
static_assert(tuple_total_elements<jagged>::value     == 6,
              "tuple_total_elements<jagged>::value should be 6 (1 + 2 + 3)");
static_assert(tuple_total_elements<empty_rows>::value == 0,
              "tuple_total_elements<empty_rows>::value should be 0");
static_assert(tuple_total_elements<std::tuple<row_empty, row_2, row_empty>>::value == 2,
              "tuple_total_elements: empty + row_2 + empty == 2");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(tuple_total_elements_v<jagged>      == 6,
                  "tuple_total_elements_v<jagged> should be 6");
    static_assert(tuple_total_elements_v<empty_2d>    == 0,
                  "tuple_total_elements_v<empty_2d> should be 0");
#endif


// =========================================================================
// VIII. TUPLE_COMMON_ELEMENT_TYPE  (compile-time)
// =========================================================================

// empty outer -- void
static_assert(std::is_same<typename tuple_common_element_type<empty_2d>::type, void>::value,
              "tuple_common_element_type<empty_2d>::type should be void");

// single-row, single-element
static_assert(std::is_same<typename tuple_common_element_type<std::tuple<row_1>>::type, int>::value,
              "tuple_common_element_type<tuple<row_1>>::type should be int");

// uniform with same element types
static_assert(std::is_same<typename tuple_common_element_type<
                                std::tuple<std::tuple<int>, std::tuple<int>>>::type, int>::value,
              "tuple_common_element_type<tuple<tuple<int>, tuple<int>>>::type should be int");

// uniform with promotable element types (int and long -> long)
static_assert(std::is_same<typename tuple_common_element_type<
                                std::tuple<std::tuple<int>, std::tuple<long>>>::type, long>::value,
              "tuple_common_element_type<tuple<tuple<int>, tuple<long>>>::type should be long");

// jagged with diverse but common-typeable elements (int, char -> int)
static_assert(std::is_same<typename tuple_common_element_type<
                                std::tuple<std::tuple<int, char>>>::type, int>::value,
              "tuple_common_element_type<tuple<tuple<int, char>>>::type should be int");

// alias
static_assert(std::is_same<tuple_common_element_type_t<std::tuple<std::tuple<int>, std::tuple<long>>>,
                           long>::value,
              "tuple_common_element_type_t: alias");


// =========================================================================
// IX.  MAKE_2D_TUPLE_OF  (compile-time)
// =========================================================================

// no row sizes -- empty 2D
static_assert(std::is_same<typename make_2d_tuple_of<int>::type, std::tuple<>>::value,
              "make_2d_tuple_of<int>::type should be std::tuple<>");
static_assert(std::is_same<typename make_2d_tuple_of<alpha>::type, std::tuple<>>::value,
              "make_2d_tuple_of<alpha>::type should be std::tuple<>");

// one row
static_assert(std::is_same<typename make_2d_tuple_of<int, 1>::type,
                           std::tuple<std::tuple<int>>>::value,
              "make_2d_tuple_of<int, 1>::type should be std::tuple<std::tuple<int>>");
static_assert(std::is_same<typename make_2d_tuple_of<int, 3>::type,
                           std::tuple<std::tuple<int, int, int>>>::value,
              "make_2d_tuple_of<int, 3>::type should be std::tuple<std::tuple<int, int, int>>");
static_assert(std::is_same<typename make_2d_tuple_of<int, 0>::type,
                           std::tuple<std::tuple<>>>::value,
              "make_2d_tuple_of<int, 0>::type should be std::tuple<std::tuple<>>");

// uniform multi-row
static_assert(std::is_same<typename make_2d_tuple_of<int, 2, 2>::type,
                           std::tuple<std::tuple<int, int>, std::tuple<int, int>>>::value,
              "make_2d_tuple_of<int, 2, 2>::type should be uniform 2x2 tuple of int");
static_assert(std::is_same<typename make_2d_tuple_of<int, 2, 2, 2>::type,
                           std::tuple<std::tuple<int, int>,
                                      std::tuple<int, int>,
                                      std::tuple<int, int>>>::value,
              "make_2d_tuple_of<int, 2, 2, 2>::type should be uniform 3x2");

// jagged multi-row
static_assert(std::is_same<typename make_2d_tuple_of<int, 1, 2, 3>::type,
                           std::tuple<std::tuple<int>,
                                      std::tuple<int, int>,
                                      std::tuple<int, int, int>>>::value,
              "make_2d_tuple_of<int, 1, 2, 3>::type should be jagged 1/2/3");

// empty + non-empty rows
static_assert(std::is_same<typename make_2d_tuple_of<int, 0, 0>::type,
                           std::tuple<std::tuple<>, std::tuple<>>>::value,
              "make_2d_tuple_of<int, 0, 0>::type should be two empty rows");
static_assert(std::is_same<typename make_2d_tuple_of<int, 0, 2>::type,
                           std::tuple<std::tuple<>, std::tuple<int, int>>>::value,
              "make_2d_tuple_of<int, 0, 2>::type should mix empty and row_2");

// non-trivial element type
static_assert(std::is_same<typename make_2d_tuple_of<alpha, 2, 1>::type,
                           std::tuple<std::tuple<alpha, alpha>,
                                      std::tuple<alpha>>>::value,
              "make_2d_tuple_of<alpha, 2, 1>::type should preserve element type");

// alias
static_assert(std::is_same<make_2d_tuple_of_t<int, 1, 2, 3>,
                           std::tuple<std::tuple<int>,
                                      std::tuple<int, int>,
                                      std::tuple<int, int, int>>>::value,
              "make_2d_tuple_of_t: alias");


// =========================================================================
// X.   TUPLE_ROW_TYPE / TUPLE_ROW_SIZE  (compile-time)
// =========================================================================

// tuple_row_type
static_assert(std::is_same<typename tuple_row_type<0, one_row>::type, row_2>::value,
              "tuple_row_type<0, one_row>::type should be row_2");
static_assert(std::is_same<typename tuple_row_type<0, uniform2>::type, row_2>::value,
              "tuple_row_type<0, uniform2>::type should be row_2");
static_assert(std::is_same<typename tuple_row_type<1, uniform2>::type, row_2>::value,
              "tuple_row_type<1, uniform2>::type should be row_2");
static_assert(std::is_same<typename tuple_row_type<0, jagged>::type, row_1>::value,
              "tuple_row_type<0, jagged>::type should be row_1");
static_assert(std::is_same<typename tuple_row_type<1, jagged>::type, row_2>::value,
              "tuple_row_type<1, jagged>::type should be row_2");
static_assert(std::is_same<typename tuple_row_type<2, jagged>::type, row_3>::value,
              "tuple_row_type<2, jagged>::type should be row_3");

// tuple_row_type strips cv on the extracted row via clean_t
static_assert(std::is_same<typename tuple_row_type<0,
                                std::tuple<const std::tuple<int, char>>>::type,
                           std::tuple<int, char>>::value,
              "tuple_row_type strips const off the extracted row");
static_assert(std::is_same<typename tuple_row_type<0,
                                std::tuple<std::tuple<int>&>>::type,
                           std::tuple<int>>::value,
              "tuple_row_type strips ref off the extracted row");

// alias
static_assert(std::is_same<tuple_row_type_t<1, uniform2>, row_2>::value,
              "tuple_row_type_t: alias");

// tuple_row_size
static_assert(tuple_row_size<0, one_row>::value == 2,
              "tuple_row_size<0, one_row>::value should be 2");
static_assert(tuple_row_size<0, jagged>::value == 1,
              "tuple_row_size<0, jagged>::value should be 1");
static_assert(tuple_row_size<1, jagged>::value == 2,
              "tuple_row_size<1, jagged>::value should be 2");
static_assert(tuple_row_size<2, jagged>::value == 3,
              "tuple_row_size<2, jagged>::value should be 3");
static_assert(tuple_row_size<0, std::tuple<row_empty>>::value == 0,
              "tuple_row_size<0, tuple<row_empty>>::value should be 0");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(tuple_row_size_v<2, jagged> == 3,
                  "tuple_row_size_v<2, jagged> should be 3");
    static_assert(tuple_row_size_v<0, one_row> == 2,
                  "tuple_row_size_v<0, one_row> should be 2");
#endif


// =========================================================================
// XI.  RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_2d_all(
    test_handler& _test_handler
)
{
    // ---- is_2d_tuple ----
    record_assertion(_test_handler, 
        is_2d_tuple<int>::value == false,
        "is_2d_tuple: non-tuple -> false");
    record_assertion(_test_handler, 
        is_2d_tuple<empty_2d>::value == true,
        "is_2d_tuple: empty tuple -> true (vacuous)");
    record_assertion(_test_handler, 
        is_2d_tuple<std::tuple<int>>::value == false,
        "is_2d_tuple: 1D tuple -> false");
    record_assertion(_test_handler, 
        is_2d_tuple<uniform2>::value == true,
        "is_2d_tuple: uniform 2D -> true");
    record_assertion(_test_handler, 
        is_2d_tuple<jagged>::value == true,
        "is_2d_tuple: jagged tuple is still 2D");
    record_assertion(_test_handler, 
        is_2d_tuple<std::tuple<std::tuple<int>, int>>::value == false,
        "is_2d_tuple: mixed tuple/non-tuple rows -> false");
    record_assertion(_test_handler, 
        is_2d_tuple<std::tuple<const std::tuple<int>>>::value == true,
        "is_2d_tuple: cv-qualified rows accepted (clean_t)");

    // ---- tuple_inner_sizes ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_inner_sizes<empty_2d>::type,
                     std::index_sequence<>>::value,
        "tuple_inner_sizes: empty 2D -> index_sequence<>");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_inner_sizes<uniform2>::type,
                     std::index_sequence<2, 2>>::value,
        "tuple_inner_sizes: uniform 2x2 -> index_sequence<2, 2>");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_inner_sizes<jagged>::type,
                     std::index_sequence<1, 2, 3>>::value,
        "tuple_inner_sizes: jagged -> index_sequence<1, 2, 3>");
    record_assertion(_test_handler, 
        std::is_same<tuple_inner_sizes_t<uniform2>,
                     std::index_sequence<2, 2>>::value,
        "tuple_inner_sizes_t: alias");

    // ---- tuple_outer_size ----
    record_assertion(_test_handler, 
        tuple_outer_size<int>::value == 0,
        "tuple_outer_size: non-tuple -> 0");
    record_assertion(_test_handler, 
        tuple_outer_size<std::tuple<int>>::value == 0,
        "tuple_outer_size: 1D tuple -> 0");
    record_assertion(_test_handler, 
        tuple_outer_size<empty_2d>::value == 0,
        "tuple_outer_size: empty 2D -> 0");
    record_assertion(_test_handler, 
        tuple_outer_size<uniform3>::value == 3,
        "tuple_outer_size: 3-row uniform -> 3");
    record_assertion(_test_handler, 
        tuple_outer_size<jagged>::value == 3,
        "tuple_outer_size: jagged 3-row -> 3");

    // ---- tuple_flatten_types ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_flatten_types<empty_2d>::type, std::tuple<>>::value,
        "tuple_flatten_types: empty 2D -> empty");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_flatten_types<uniform2>::type,
                     std::tuple<int, char, int, char>>::value,
        "tuple_flatten_types: uniform2 flattened");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_flatten_types<jagged>::type,
                     std::tuple<int, int, char, int, char, float>>::value,
        "tuple_flatten_types: jagged flattened");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_flatten_types<empty_rows>::type, std::tuple<>>::value,
        "tuple_flatten_types: rows of empty tuples -> empty");
    record_assertion(_test_handler, 
        std::is_same<tuple_flatten_types_t<jagged>,
                     std::tuple<int, int, char, int, char, float>>::value,
        "tuple_flatten_types_t: alias");

    // ---- is_uniform_2d_tuple ----
    record_assertion(_test_handler, 
        is_uniform_2d_tuple<empty_2d>::value == true,
        "is_uniform_2d_tuple: empty 2D vacuously uniform");
    record_assertion(_test_handler, 
        is_uniform_2d_tuple<one_row>::value == true,
        "is_uniform_2d_tuple: single row vacuously uniform");
    record_assertion(_test_handler, 
        is_uniform_2d_tuple<uniform2>::value == true,
        "is_uniform_2d_tuple: uniform 2x2 -> true");
    record_assertion(_test_handler, 
        is_uniform_2d_tuple<jagged>::value == false,
        "is_uniform_2d_tuple: jagged -> false");
    record_assertion(_test_handler, 
        is_uniform_2d_tuple<int>::value == false,
        "is_uniform_2d_tuple: non-tuple -> false");
    record_assertion(_test_handler, 
        is_uniform_2d_tuple<std::tuple<int>>::value == false,
        "is_uniform_2d_tuple: 1D tuple -> false");

    // ---- is_jagged_tuple ----
    record_assertion(_test_handler, 
        is_jagged_tuple<empty_2d>::value == false,
        "is_jagged_tuple: empty 2D -> false (< 2 rows)");
    record_assertion(_test_handler, 
        is_jagged_tuple<one_row>::value == false,
        "is_jagged_tuple: single-row -> false (< 2 rows)");
    record_assertion(_test_handler, 
        is_jagged_tuple<uniform2>::value == false,
        "is_jagged_tuple: uniform multi-row -> false");
    record_assertion(_test_handler, 
        is_jagged_tuple<jagged>::value == true,
        "is_jagged_tuple: jagged -> true");
    record_assertion(_test_handler, 
        is_jagged_tuple<std::tuple<row_1, row_2>>::value == true,
        "is_jagged_tuple: two-row jagged -> true");

    // ---- tuple_total_elements ----
    record_assertion(_test_handler, 
        tuple_total_elements<empty_2d>::value == 0,
        "tuple_total_elements: empty 2D -> 0");
    record_assertion(_test_handler, 
        tuple_total_elements<uniform3>::value == 6,
        "tuple_total_elements: uniform3 -> 6");
    record_assertion(_test_handler, 
        tuple_total_elements<jagged>::value == 6,
        "tuple_total_elements: jagged 1+2+3 -> 6");
    record_assertion(_test_handler, 
        tuple_total_elements<empty_rows>::value == 0,
        "tuple_total_elements: empty rows -> 0");

    // ---- tuple_common_element_type ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_common_element_type<empty_2d>::type, void>::value,
        "tuple_common_element_type: empty 2D -> void");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_common_element_type<
                            std::tuple<std::tuple<int>, std::tuple<int>>>::type, int>::value,
        "tuple_common_element_type: all int -> int");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_common_element_type<
                            std::tuple<std::tuple<int>, std::tuple<long>>>::type, long>::value,
        "tuple_common_element_type: int and long -> long");
    record_assertion(_test_handler, 
        std::is_same<tuple_common_element_type_t<
                            std::tuple<std::tuple<int>, std::tuple<long>>>, long>::value,
        "tuple_common_element_type_t: alias");

    // ---- make_2d_tuple_of ----
    record_assertion(_test_handler, 
        std::is_same<typename make_2d_tuple_of<int>::type, std::tuple<>>::value,
        "make_2d_tuple_of<int>: no rows -> empty 2D");
    record_assertion(_test_handler, 
        std::is_same<typename make_2d_tuple_of<int, 1>::type,
                     std::tuple<std::tuple<int>>>::value,
        "make_2d_tuple_of<int, 1>: single 1-element row");
    record_assertion(_test_handler, 
        std::is_same<typename make_2d_tuple_of<int, 2, 2>::type,
                     std::tuple<std::tuple<int, int>, std::tuple<int, int>>>::value,
        "make_2d_tuple_of<int, 2, 2>: uniform 2x2");
    record_assertion(_test_handler, 
        std::is_same<typename make_2d_tuple_of<int, 1, 2, 3>::type,
                     std::tuple<std::tuple<int>,
                                std::tuple<int, int>,
                                std::tuple<int, int, int>>>::value,
        "make_2d_tuple_of<int, 1, 2, 3>: jagged");
    record_assertion(_test_handler, 
        std::is_same<typename make_2d_tuple_of<int, 0>::type,
                     std::tuple<std::tuple<>>>::value,
        "make_2d_tuple_of<int, 0>: empty inner row");
    record_assertion(_test_handler, 
        std::is_same<make_2d_tuple_of_t<int, 1, 2, 3>,
                     std::tuple<std::tuple<int>,
                                std::tuple<int, int>,
                                std::tuple<int, int, int>>>::value,
        "make_2d_tuple_of_t: alias");

    // ---- tuple_row_type ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_row_type<0, uniform2>::type, row_2>::value,
        "tuple_row_type<0, uniform2> -> row_2");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_row_type<2, jagged>::type, row_3>::value,
        "tuple_row_type<2, jagged> -> row_3");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_row_type<0,
                            std::tuple<const std::tuple<int, char>>>::type,
                     std::tuple<int, char>>::value,
        "tuple_row_type: const-qualified inner stripped");
    record_assertion(_test_handler, 
        std::is_same<tuple_row_type_t<1, uniform2>, row_2>::value,
        "tuple_row_type_t: alias");

    // ---- tuple_row_size ----
    record_assertion(_test_handler, 
        tuple_row_size<0, one_row>::value == 2,
        "tuple_row_size<0, one_row> -> 2");
    record_assertion(_test_handler, 
        tuple_row_size<2, jagged>::value == 3,
        "tuple_row_size<2, jagged> -> 3");
    record_assertion(_test_handler, 
        tuple_row_size<0, std::tuple<row_empty>>::value == 0,
        "tuple_row_size<0, tuple<empty>> -> 0");

    return;
}


NS_END  // testing
NS_END  // djinterp