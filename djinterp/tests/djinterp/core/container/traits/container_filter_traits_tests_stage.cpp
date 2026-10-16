/*******************************************************************************
* djinterp [testing]                     container_filter_traits_tests_stage.cpp
*
*   Section III - the filter_stage vocabulary and filter_stage_of, the stage a
* container admits selection at.  A non-source is `none`; a source whose data
* are compile-staged (compile-time size AND literal contents) admits a
* compile-time selection; every other source is `runtime`.
*
*   The one compile-time case among the standard containers is std::array of a
* literal element (fixed extent + literal contents).  All heap-backed
* containers - and the custom sources, whose size() is not constexpr - are
* `runtime`.  A worthwhile contrast the runtime set already captures: a
* std::array of std::string is NOT compile-staged (its contents are not
* literal), so it is a runtime source despite its fixed extent - which is why
* the compile-time check uses array<int, N>, not array<string, N>.
*
*
* path:      /tests/djinterp/core/container/traits/container_filter_traits_tests_stage.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_filter_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_filter_stage_name
//   the stable spellings.
bool
tests_filter_stage_name()
{
    bool _cf_ok = true;

    D_CF_CHECK( (cf_cstr_eq(filter_stage_name(filter_stage::none),         "none")) );
    D_CF_CHECK( (cf_cstr_eq(filter_stage_name(filter_stage::runtime),      "runtime")) );
    D_CF_CHECK( (cf_cstr_eq(filter_stage_name(filter_stage::compile_time), "compile_time")) );

    D_CF_CHECK( (!cf_cstr_eq(filter_stage_name(filter_stage::runtime),
                             "compile_time")) );

    return _cf_ok;
}


// tests_filter_stage_of
//   the stage verdict across all three states.
bool
tests_filter_stage_of()
{
    bool _cf_ok = true;

    // compile_time: a source that is compile-staged (fixed extent + literal element)
    D_CF_CHECK( (filter_stage_of<std::array<int, 4>>::value
                     == filter_stage::compile_time) );

    // runtime: heap-backed sources
    D_CF_CHECK( (filter_stage_of<std::vector<int>>::value      == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::deque<int>>::value       == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::list<int>>::value        == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::forward_list<int>>::value == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::set<int>>::value         == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::unordered_set<int>>::value == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::map<int, int>>::value    == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::string>::value           == filter_stage::runtime) );
    // custom sources: size() is not constexpr -> runtime
    D_CF_CHECK( (filter_stage_of<native_filterable>::value     == filter_stage::runtime) );
    D_CF_CHECK( (filter_stage_of<sorted_seq>::value            == filter_stage::runtime) );

    // none: not a source
    D_CF_CHECK( (filter_stage_of<int>::value               == filter_stage::none) );
    D_CF_CHECK( (filter_stage_of<value_only>::value        == filter_stage::none) );
    D_CF_CHECK( (filter_stage_of<iterable_no_value>::value == filter_stage::none) );

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
