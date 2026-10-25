/******************************************************************************
* djinterp [test]          concurrency_strategy_traits_tests_deduction.cpp
*
*   Section V - concurrency_strategy_of.  Covers every tag_to_strategy
* specialization, each single-strategy structure, the hybrid (hit_count >= 2)
* path, the tag-beats-hybrid precedence, and cv/ref stripping.  Defines the
* "deduction" block.
******************************************************************************/

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_deduction_tagged
//   Verifies each declared tag deduces its strategy - exercising every
//   tag_to_strategy specialization through the observable trait.
//   Tests the following:
//   - none / locked / cow / rcu / atomic / hazard / hybrid tags -> their value
bool
tests_deduction_tagged()
{
    D_CS_CHECK(concurrency_strategy_of_v<tag_none>   ==
               concurrency_strategy::none);
    D_CS_CHECK(concurrency_strategy_of_v<tag_locked> ==
               concurrency_strategy::locked);
    D_CS_CHECK(concurrency_strategy_of_v<tag_cow>    ==
               concurrency_strategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<tag_rcu>    ==
               concurrency_strategy::rcu);
    D_CS_CHECK(concurrency_strategy_of_v<tag_atomic> ==
               concurrency_strategy::atomic);
    D_CS_CHECK(concurrency_strategy_of_v<tag_hazard> ==
               concurrency_strategy::hazard);
    D_CS_CHECK(concurrency_strategy_of_v<tag_hybrid> ==
               concurrency_strategy::hybrid);
    return true;
}

// tests_deduction_structural
//   Verifies a single structural strategy deduces the matching value and the
//   empty type deduces none.
//   Tests the following:
//   - locked / cow / rcu / atomic / hazard structures -> their value
//   - the empty type -> none
bool
tests_deduction_structural()
{
    D_CS_CHECK(concurrency_strategy_of_v<st_locked_rw>    ==
               concurrency_strategy::locked);
    D_CS_CHECK(concurrency_strategy_of_v<st_cow_state>    ==
               concurrency_strategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<st_rcu_protected> ==
               concurrency_strategy::rcu);
    D_CS_CHECK(concurrency_strategy_of_v<st_atomic_value> ==
               concurrency_strategy::atomic);
    D_CS_CHECK(concurrency_strategy_of_v<st_hazard>       ==
               concurrency_strategy::hazard);
    D_CS_CHECK(concurrency_strategy_of_v<st_plain>        ==
               concurrency_strategy::none);
    return true;
}

// tests_deduction_hybrid
//   Verifies that two or more concurrent structural strategies, with no tag,
//   deduce hybrid.
//   Tests the following:
//   - locked+cow, atomic+hazard, and locked+cow+rcu -> hybrid
bool
tests_deduction_hybrid()
{
    D_CS_CHECK(concurrency_strategy_of_v<hy_locked_cow>   ==
               concurrency_strategy::hybrid);
    D_CS_CHECK(concurrency_strategy_of_v<hy_atomic_hazard> ==
               concurrency_strategy::hybrid);
    D_CS_CHECK(concurrency_strategy_of_v<hy_triple>       ==
               concurrency_strategy::hybrid);
    return true;
}

// tests_deduction_tag_over_structure
//   Verifies a tag is authoritative even when the structure alone would deduce
//   hybrid, and that an explicit none tag yields none despite structure.
//   Tests the following:
//   - cow tag over locked structure -> cow (not hybrid, not locked)
//   - locked tag over cow structure -> locked
//   - none tag over cow structure -> none
//   - an explicit hybrid tag -> hybrid
bool
tests_deduction_tag_over_structure()
{
    D_CS_CHECK(concurrency_strategy_of_v<pr_cowtag_lockstruct> ==
               concurrency_strategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<pr_locktag_cowstruct> ==
               concurrency_strategy::locked);
    D_CS_CHECK(concurrency_strategy_of_v<pr_nonetag_cowstruct> ==
               concurrency_strategy::none);
    D_CS_CHECK(concurrency_strategy_of_v<tag_hybrid> ==
               concurrency_strategy::hybrid);
    return true;
}

// tests_deduction_cvref
//   Verifies deduction (and the predicates it rests on) strip const, volatile
//   and reference qualifiers before classifying.
//   Tests the following:
//   - const / volatile / lvalue-ref / rvalue-ref of a tagged type deduce the
//     same strategy as the bare type
//   - a predicate agrees across a const reference
bool
tests_deduction_cvref()
{
    D_CS_CHECK(concurrency_strategy_of_v<const tag_cow> ==
               concurrency_strategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<volatile tag_cow> ==
               concurrency_strategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<tag_cow&> ==
               concurrency_strategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<tag_cow&&> ==
               concurrency_strategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<const tag_hazard&> ==
               concurrency_strategy::hazard);

    D_CS_CHECK(is_cow_container_v<const st_cow_state&> ==
               is_cow_container_v<st_cow_state>);
    D_CS_CHECK(concurrency_strategy_of_v<const st_locked_rw&> ==
               concurrency_strategy::locked);
    return true;
}

NS_END  // testing
NS_END  // djinterp
