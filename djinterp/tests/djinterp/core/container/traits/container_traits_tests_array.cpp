/*******************************************************************************
* djinterp [testing]                            container_traits_tests_array.cpp
*
*   Section IV - array detection: is_c_array (a built-in T[N]) and is_std_array
* (a std::array<T,N>).  These are the fixed-extent signals the bounds and storage
* classifiers fold in, so both the positive forms and the near-misses (a
* std::array is not a C-array, a std::vector is neither) are checked.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_array.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_array_detection
//   is_c_array and is_std_array, with their mutual near-misses.
bool
tests_array_detection()
{
    bool _ct_ok = true;

    // C-arrays
    D_CT_CHECK( (is_c_array<int[4]>::value) );
    D_CT_CHECK( (is_c_array<double[10]>::value) );
    D_CT_CHECK( (!is_c_array<std::array<int, 4>>::value) );  // std::array is not a C-array
    D_CT_CHECK( (!is_c_array<std::vector<int>>::value) );
    D_CT_CHECK( (!is_c_array<int>::value) );

    // std::arrays
    D_CT_CHECK( (is_std_array<std::array<int, 4>>::value) );
    D_CT_CHECK( (is_std_array<std::array<double, 1>>::value) );
    D_CT_CHECK( (!is_std_array<int[4]>::value) );            // C-array is not a std::array
    D_CT_CHECK( (!is_std_array<std::vector<int>>::value) );
    D_CT_CHECK( (!is_std_array<int>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
