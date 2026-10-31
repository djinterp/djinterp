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
bool tests_deduction_tagged()
{
    D_CS_CHECK(concurrency_strategy_of_v<DTagNone>   ==
               DConcurrencyStrategy::none);
    D_CS_CHECK(concurrency_strategy_of_v<DTagLocked> ==
               DConcurrencyStrategy::locked);
    D_CS_CHECK(concurrency_strategy_of_v<DTagCow>    ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<DTagRcu>    ==
               DConcurrencyStrategy::rcu);
    D_CS_CHECK(concurrency_strategy_of_v<DTagAtomic> ==
               DConcurrencyStrategy::atomic);
    D_CS_CHECK(concurrency_strategy_of_v<DTagHazard> ==
               DConcurrencyStrategy::hazard);
    D_CS_CHECK(concurrency_strategy_of_v<DTagHybrid> ==
               DConcurrencyStrategy::hybrid);
    return true;
}

// tests_deduction_structural
//   Verifies a single structural strategy deduces the matching value and the
//   empty type deduces none.
//   Tests the following:
//   - locked / cow / rcu / atomic / hazard structures -> their value
//   - the empty type -> none
bool tests_deduction_structural()
{
    D_CS_CHECK(concurrency_strategy_of_v<DStLockedRw>    ==
               DConcurrencyStrategy::locked);
    D_CS_CHECK(concurrency_strategy_of_v<DStCowState>    ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<DStRcuProtected> ==
               DConcurrencyStrategy::rcu);
    D_CS_CHECK(concurrency_strategy_of_v<st_atomic_value> ==
               DConcurrencyStrategy::atomic);
    D_CS_CHECK(concurrency_strategy_of_v<DStHazard>       ==
               DConcurrencyStrategy::hazard);
    D_CS_CHECK(concurrency_strategy_of_v<DStPlain>        ==
               DConcurrencyStrategy::none);
    return true;
}

// tests_deduction_hybrid
//   Verifies that two or more concurrent structural strategies, with no tag,
//   deduce hybrid.
//   Tests the following:
//   - locked+cow, atomic+hazard, and locked+cow+rcu -> hybrid
bool tests_deduction_hybrid()
{
    D_CS_CHECK(concurrency_strategy_of_v<DHyLockedCow>   ==
               DConcurrencyStrategy::hybrid);
    D_CS_CHECK(concurrency_strategy_of_v<DHyAtomicHazard> ==
               DConcurrencyStrategy::hybrid);
    D_CS_CHECK(concurrency_strategy_of_v<DHyTriple>       ==
               DConcurrencyStrategy::hybrid);
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
bool tests_deduction_tag_over_structure()
{
    D_CS_CHECK(concurrency_strategy_of_v<DPrCowtagLockstruct> ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<DPrLocktagCowstruct> ==
               DConcurrencyStrategy::locked);
    D_CS_CHECK(concurrency_strategy_of_v<DPrNonetagCowstruct> ==
               DConcurrencyStrategy::none);
    D_CS_CHECK(concurrency_strategy_of_v<DTagHybrid> ==
               DConcurrencyStrategy::hybrid);
    return true;
}

// tests_deduction_cvref
//   Verifies deduction (and the predicates it rests on) strip const, volatile
//   and reference qualifiers before classifying.
//   Tests the following:
//   - const / volatile / lvalue-ref / rvalue-ref of a tagged type deduce the
//     same strategy as the bare type
//   - a predicate agrees across a const reference
bool tests_deduction_cvref()
{
    D_CS_CHECK(concurrency_strategy_of_v<const DTagCow> ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<volatile DTagCow> ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<DTagCow&> ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<DTagCow&&> ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(concurrency_strategy_of_v<const DTagHazard&> ==
               DConcurrencyStrategy::hazard);

    D_CS_CHECK(is_cow_container_v<const DStCowState&> ==
               is_cow_container_v<DStCowState>);
    D_CS_CHECK(concurrency_strategy_of_v<const DStLockedRw&> ==
               DConcurrencyStrategy::locked);
    return true;
}

NS_END  // testing
NS_END  // djinterp
