/*******************************************************************************
* djinterp [testing]               container_filter_traits_tests_result_type.cpp
*
*   Section V - filter_result_type_t, the container a selection builds.  A
* filterable type selects into its OWN type (selection is closed on it); a
* source that cannot build selects into a std::vector of its value_type (the
* survivors need an external home); and anything that is not even a source has
* no result type at all (void).
*
*
* path:      /tests/djinterp/core/container/traits/container_filter_traits_tests_result_type.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "container_filter_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_filter_result_type
//   the output container type across the three cases.
bool
tests_filter_result_type()
{
    bool _cf_ok = true;

    // filterable -> its own type
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::vector<int>>,
                              std::vector<int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::deque<int>>,
                              std::deque<int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::list<int>>,
                              std::list<int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::set<int>>,
                              std::set<int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::multiset<int>>,
                              std::multiset<int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::unordered_set<int>>,
                              std::unordered_set<int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::map<int, int>>,
                              std::map<int, int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::string>,
                              std::string>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<native_filterable>,
                              native_filterable>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<sorted_seq>,
                              sorted_seq>::value) );

    // input-only source -> std::vector<value_type>
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::array<int, 4>>,
                              std::vector<int>>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<std::forward_list<int>>,
                              std::vector<int>>::value) );

    // not a source -> void
    D_CF_CHECK( (std::is_same<filter_result_type_t<int>, void>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<value_only>, void>::value) );
    D_CF_CHECK( (std::is_same<filter_result_type_t<iterable_no_value>, void>::value) );

    // cv/ref qualification is stripped: a qualified source resolves as its clean
    // type (filterable -> itself), not to void
    D_CF_CHECK( (std::is_same<filter_result_type_t<const std::vector<int>&>,
                              std::vector<int>>::value) );

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
