/*******************************************************************************
* djinterp [testing]                  container_transform_traits_tests_stage.cpp
*
*   Section III - the stage at which a container admits mapping: compile-time when
* its data are compile-staged (a literal type, the map assumed statically
* evaluable), else runtime; none for a non-source.
*
*   The stage is independent of the capability and strategy: std::array is
* input-only with an EXTERNAL strategy yet is a literal type, so it maps at
* COMPILE-TIME, while the growable heap containers (std::vector, std::set, ...) are
* runtime.  A non-container short-circuits to none before the literal test is even
* reached.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_transform_traits_tests_stage.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_transform_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_transform_stage_name
//   the stable spellings.
bool tests_transform_stage_name()
{
    bool _tx_ok = true;

    D_TX_CHECK( (tx_cstr_eq(transform_stage_name(DTransformStage::none),         "none")) );
    D_TX_CHECK( (tx_cstr_eq(transform_stage_name(DTransformStage::runtime),      "runtime")) );
    D_TX_CHECK( (tx_cstr_eq(transform_stage_name(DTransformStage::compile_time), "compile_time")) );

    D_TX_CHECK( (!tx_cstr_eq(transform_stage_name(DTransformStage::runtime),
                             "compile_time")) );

    return _tx_ok;
}


// tests_transform_stage_of
//   compile-time for literal sources, runtime for the rest, none for non-sources.
bool tests_transform_stage_of()
{
    bool _tx_ok = true;

    // compile-time: literal-type sources (even the source-only, external ones)
    D_TX_CHECK( (transform_stage_of<std::array<int, 4>>::value == DTransformStage::compile_time) );  // input-only, yet compile-time
    D_TX_CHECK( (transform_stage_of<DNativeBox>::value         == DTransformStage::compile_time) );
    D_TX_CHECK( (transform_stage_of<DReadOnlySeq>::value      == DTransformStage::compile_time) );
    D_TX_CHECK( (transform_stage_of<Box<int>>::value           == DTransformStage::compile_time) );

    // runtime: heap-backed containers (not literal types)
    D_TX_CHECK( (transform_stage_of<std::vector<int>>::value        == DTransformStage::runtime) );
    D_TX_CHECK( (transform_stage_of<std::deque<int>>::value         == DTransformStage::runtime) );
    D_TX_CHECK( (transform_stage_of<std::forward_list<int>>::value  == DTransformStage::runtime) );
    D_TX_CHECK( (transform_stage_of<std::set<int>>::value           == DTransformStage::runtime) );
    D_TX_CHECK( (transform_stage_of<std::map<int, int>>::value      == DTransformStage::runtime) );
    D_TX_CHECK( (transform_stage_of<std::string>::value            == DTransformStage::runtime) );

    // none: not a source
    D_TX_CHECK( (transform_stage_of<int>::value == DTransformStage::none) );

    return _tx_ok;
}


NS_END  // testing
NS_END  // djinterp
