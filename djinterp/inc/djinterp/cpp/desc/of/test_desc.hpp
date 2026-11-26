/*******************************************************************************
* djinterp [cpp][desc]                                             test_desc.hpp
*
*   The C++ face of desc's describers of DTest's C types
* (desc/of/test_desc.h): units<T> and type_name<T> for a run's summary and
* options, and for a module's, a block's and a test's spec, so
* describe(summary) is the summary's description. Built where the C describers
* are (D_CFG_DESC_OF_TEST); from C++98.
*
* HOW TO USE
* ----------
*   djinterp::desc::describe(summary) is a run's summary at desc.ring;
* describe(spec, D_DESC_RING_DETAILED) is a module spec's detail.
*
* OPTIONS
* -------
*   desc's own, as for the C describers.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and D_CFG_DESC_OF_TEST (cfg_desc_of.h).
*
*
* path:      /inc/djinterp/cpp/desc/of/test_desc.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  UNITS
    -----
    1.  dj_test_summary, dj_test_suite_options, dj_test_module_spec,
        dj_test_block_spec, dj_test_case_spec
*/

#ifndef DJINTERP_DESC_OF_TEST_DESC_HPP
#define DJINTERP_DESC_OF_TEST_DESC_HPP 1

// djinterp
#include "./desc_of.hpp"                       // D_DESC_OF_UNITS
#include "../../../c/desc/of/test_desc.h" // the describers


#if D_INTERNAL_DESC_OF_TEST

NS_DJINTERP
D_NAMESPACE(desc)


//==============================================================================
// 1.  UNITS
//==============================================================================

// 1.1
// dj_test_summary, dj_test_suite_options, dj_test_module_spec,
// dj_test_block_spec, dj_test_case_spec
//   traits: each type's units and name, from its C description.
D_DESC_OF_UNITS(dj_test_summary);
D_DESC_OF_UNITS(dj_test_suite_options);
D_DESC_OF_UNITS(dj_test_module_spec);
D_DESC_OF_UNITS(dj_test_block_spec);
D_DESC_OF_UNITS(dj_test_case_spec);


NS_END  // desc
NS_END  // djinterp

#endif  // D_INTERNAL_DESC_OF_TEST


#endif  // DJINTERP_DESC_OF_TEST_DESC_HPP
