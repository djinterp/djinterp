/*******************************************************************************
* djinterp [testing]                     container_filter_traits_tests_stage.cpp
*
*   Section III - the DFilterStage vocabulary and filter_stage_of, the stage a
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
* path:      /tests/djinterp/cpp/container/traits/container_filter_traits_tests_stage.cpp
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
bool tests_filter_stage_name()
{
    bool _cf_ok = true;

    D_CF_CHECK( (cf_cstr_eq(filter_stage_name(DFilterStage::none),         "none")) );
    D_CF_CHECK( (cf_cstr_eq(filter_stage_name(DFilterStage::runtime),      "runtime")) );
    D_CF_CHECK( (cf_cstr_eq(filter_stage_name(DFilterStage::compile_time), "compile_time")) );

    D_CF_CHECK( (!cf_cstr_eq(filter_stage_name(DFilterStage::runtime),
                             "compile_time")) );

    return _cf_ok;
}


// tests_filter_stage_of
//   the stage verdict across all three states.
bool tests_filter_stage_of()
{
    bool _cf_ok = true;

    // compile_time: a source that is compile-staged (fixed extent + literal element)
    D_CF_CHECK( (filter_stage_of<std::array<int, 4>>::value
                     == DFilterStage::compile_time) );

    // runtime: heap-backed sources
    D_CF_CHECK( (filter_stage_of<std::vector<int>>::value      == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::deque<int>>::value       == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::list<int>>::value        == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::forward_list<int>>::value == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::set<int>>::value         == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::unordered_set<int>>::value == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::map<int, int>>::value    == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<std::string>::value           == DFilterStage::runtime) );
    // custom sources: size() is not constexpr -> runtime
    D_CF_CHECK( (filter_stage_of<DNativeFilterable>::value     == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of<DSortedSeq>::value            == DFilterStage::runtime) );

    // none: not a source
    D_CF_CHECK( (filter_stage_of<int>::value               == DFilterStage::none) );
    D_CF_CHECK( (filter_stage_of<value_only>::value        == DFilterStage::none) );
    D_CF_CHECK( (filter_stage_of<DIterableNoValue>::value == DFilterStage::none) );

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
