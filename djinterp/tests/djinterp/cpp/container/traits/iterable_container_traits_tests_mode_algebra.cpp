/*******************************************************************************
* djinterp [testing]            iterable_container_traits_tests_mode_algebra.cpp
*
*   Section III, value level - the iteration_mode vocabulary and its capability
* order, all plain constexpr over the three-valued enum and so exercised
* exhaustively.  iteration_mode_name spells a mode; the three is_*_mode predicates
* recognise each value; mode_grants_observation / mode_grants_mutation are the two
* capability questions (any traversal at all / a settable one); iteration_mode_rank
* is the linear strength none=0 < const_only=1 < non_const=2 the formal preorder
* collapses to; and iteration_mode_subsumes is the >= over that rank.
*
*   Because the enum has only three values, the predicates are checked against ALL
* three each (not merely their positive case), and subsumption is checked over the
* complete 3x3 matrix - which pins reflexivity, the strict downward-only relation,
* and the fact that non_const subsumes every mode while none subsumes only itself.
*
*
* path:      /tests/djinterp/cpp/container/traits/iterable_container_traits_tests_mode_algebra.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "iterable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_iteration_mode_name
//   the stable spellings.
bool tests_iteration_mode_name()
{
    bool _ic_ok = true;

    D_IC_CHECK( (ic_cstr_eq(iteration_mode_name(DIterationMode::none),       "none")) );
    D_IC_CHECK( (ic_cstr_eq(iteration_mode_name(DIterationMode::const_only), "const_only")) );
    D_IC_CHECK( (ic_cstr_eq(iteration_mode_name(DIterationMode::non_const),  "non_const")) );

    // the spellings are distinct
    D_IC_CHECK( (!ic_cstr_eq(iteration_mode_name(DIterationMode::const_only),
                             iteration_mode_name(DIterationMode::non_const))) );
    D_IC_CHECK( (!ic_cstr_eq(iteration_mode_name(DIterationMode::none), "none_")) );

    return _ic_ok;
}


// tests_mode_value_predicates
//   is_no_iteration_mode / is_const_only_mode / is_non_const_mode, each against
//   all three values (exactly one fires per mode).
bool tests_mode_value_predicates()
{
    bool _ic_ok = true;

    // none
    D_IC_CHECK( (is_no_iteration_mode(DIterationMode::none)) );
    D_IC_CHECK( (!is_const_only_mode(DIterationMode::none)) );
    D_IC_CHECK( (!is_non_const_mode(DIterationMode::none)) );

    // const_only
    D_IC_CHECK( (!is_no_iteration_mode(DIterationMode::const_only)) );
    D_IC_CHECK( (is_const_only_mode(DIterationMode::const_only)) );
    D_IC_CHECK( (!is_non_const_mode(DIterationMode::const_only)) );

    // non_const
    D_IC_CHECK( (!is_no_iteration_mode(DIterationMode::non_const)) );
    D_IC_CHECK( (!is_const_only_mode(DIterationMode::non_const)) );
    D_IC_CHECK( (is_non_const_mode(DIterationMode::non_const)) );

    return _ic_ok;
}


// tests_mode_grants
//   the two capability questions: observation (any traversal) and mutation.
bool tests_mode_grants()
{
    bool _ic_ok = true;

    // observation - granted by BOTH iterable modes, denied only by none
    D_IC_CHECK( (!mode_grants_observation(DIterationMode::none)) );
    D_IC_CHECK( (mode_grants_observation(DIterationMode::const_only)) );
    D_IC_CHECK( (mode_grants_observation(DIterationMode::non_const)) );

    // mutation - granted by non_const alone
    D_IC_CHECK( (!mode_grants_mutation(DIterationMode::none)) );
    D_IC_CHECK( (!mode_grants_mutation(DIterationMode::const_only)) );
    D_IC_CHECK( (mode_grants_mutation(DIterationMode::non_const)) );

    // mutation implies observation, never the converse (the capability order)
    D_IC_CHECK( (!mode_grants_mutation(DIterationMode::const_only)
                 || mode_grants_observation(DIterationMode::const_only)) );
    D_IC_CHECK( (mode_grants_observation(DIterationMode::const_only)
                 && !mode_grants_mutation(DIterationMode::const_only)) );

    return _ic_ok;
}


// tests_iteration_mode_rank
//   the linear strength none=0 < const_only=1 < non_const=2.
bool tests_iteration_mode_rank()
{
    bool _ic_ok = true;

    D_IC_CHECK( (iteration_mode_rank(DIterationMode::none)       == 0) );
    D_IC_CHECK( (iteration_mode_rank(DIterationMode::const_only) == 1) );
    D_IC_CHECK( (iteration_mode_rank(DIterationMode::non_const)  == 2) );

    // strictly increasing along the capability order
    D_IC_CHECK( (iteration_mode_rank(DIterationMode::none)
                     < iteration_mode_rank(DIterationMode::const_only)) );
    D_IC_CHECK( (iteration_mode_rank(DIterationMode::const_only)
                     < iteration_mode_rank(DIterationMode::non_const)) );

    return _ic_ok;
}


// tests_iteration_mode_subsumes
//   the complete 3x3 relation over the rank order.
bool tests_iteration_mode_subsumes()
{
    bool _ic_ok = true;

    // reflexive on every value
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::none,       DIterationMode::none)) );
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::const_only, DIterationMode::const_only)) );
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::non_const,  DIterationMode::non_const)) );

    // non_const subsumes everything
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::non_const, DIterationMode::const_only)) );
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::non_const, DIterationMode::none)) );

    // const_only subsumes none, but NOT non_const
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::const_only, DIterationMode::none)) );
    D_IC_CHECK( (!iteration_mode_subsumes(DIterationMode::const_only, DIterationMode::non_const)) );

    // none subsumes only itself
    D_IC_CHECK( (!iteration_mode_subsumes(DIterationMode::none, DIterationMode::const_only)) );
    D_IC_CHECK( (!iteration_mode_subsumes(DIterationMode::none, DIterationMode::non_const)) );

    // antisymmetry on a strict pair, and transitivity through the middle rung
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::non_const, DIterationMode::const_only)
                 && !iteration_mode_subsumes(DIterationMode::const_only, DIterationMode::non_const)) );
    D_IC_CHECK( (iteration_mode_subsumes(DIterationMode::non_const, DIterationMode::const_only)
                 && iteration_mode_subsumes(DIterationMode::const_only, DIterationMode::none)
                 && iteration_mode_subsumes(DIterationMode::non_const, DIterationMode::none)) );

    return _ic_ok;
}


NS_END  // testing
NS_END  // djinterp
