/*******************************************************************************
* djinterp [cpp][meta]                                lifetime_tests_algebra.cpp
*
*
* path:      /tests/djinterp/cpp/meta/lifetime_tests_algebra.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "lifetime_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_algebra_enum_bit_values
  The enum values are the expected bit patterns.
  Tests the following:
  - none is 0
  - compile_time is 1 (bit 0)
  - runtime is 2 (bit 1)
  - both is 3 and equals compile_time | runtime
*/
bool tests_algebra_enum_bit_values()
{
    D_LT_CHECK(static_cast<unsigned>(DLifetime::none) == 0u);
    D_LT_CHECK(static_cast<unsigned>(DLifetime::compile_time) == 1u);
    D_LT_CHECK(static_cast<unsigned>(DLifetime::runtime) == 2u);
    D_LT_CHECK(static_cast<unsigned>(DLifetime::both) == 3u);

    D_LT_CHECK(static_cast<unsigned>(DLifetime::both) ==
               (static_cast<unsigned>(DLifetime::compile_time) |
                static_cast<unsigned>(DLifetime::runtime)));

    return true;
}

/*
tests_algebra_meet_is_bitwise_and
  lifetime_meet equals the bitwise AND of the stage bits.
  Tests the following:
  - meet matches AND for all sixteen operand pairs
  - meet(both, runtime) is runtime (a representative case)
  - meet(compile_time, runtime) is none (disjoint stages)
  - meet is a constant expression
*/
bool tests_algebra_meet_is_bitwise_and()
{
    std::size_t n = 0;
    const DLifetime::value* vals = all_lifetimes(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        for (std::size_t j = 0; j < n; ++j)
        {
            unsigned expected =
                static_cast<unsigned>(vals[i]) & static_cast<unsigned>(vals[j]);
            D_LT_CHECK(static_cast<unsigned>(lifetime_meet(vals[i], vals[j])) ==
                       expected);
        }
    }

    // representative cases
    D_LT_CHECK(lifetime_meet(DLifetime::both, DLifetime::runtime) == DLifetime::runtime);
    D_LT_CHECK(lifetime_meet(DLifetime::compile_time, DLifetime::runtime) ==
               DLifetime::none);

    static_assert(lifetime_meet(DLifetime::both, DLifetime::compile_time) ==
                      DLifetime::compile_time,
                  "meet is constexpr");

    return true;
}

/*
tests_algebra_join_is_bitwise_or
  lifetime_join equals the bitwise OR of the stage bits.
  Tests the following:
  - join matches OR for all sixteen operand pairs
  - join(compile_time, runtime) is both
  - join(none, runtime) is runtime
  - join is a constant expression
*/
bool tests_algebra_join_is_bitwise_or()
{
    std::size_t n = 0;
    const DLifetime::value* vals = all_lifetimes(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        for (std::size_t j = 0; j < n; ++j)
        {
            unsigned expected =
                static_cast<unsigned>(vals[i]) | static_cast<unsigned>(vals[j]);
            D_LT_CHECK(static_cast<unsigned>(lifetime_join(vals[i], vals[j])) ==
                       expected);
        }
    }

    D_LT_CHECK(lifetime_join(DLifetime::compile_time, DLifetime::runtime) ==
               DLifetime::both);
    D_LT_CHECK(lifetime_join(DLifetime::none, DLifetime::runtime) == DLifetime::runtime);

    static_assert(lifetime_join(DLifetime::compile_time, DLifetime::runtime) ==
                      DLifetime::both,
                  "join is constexpr");

    return true;
}

/*
tests_algebra_meet_lattice_laws
  meet obeys the lattice laws.
  Tests the following:
  - commutativity across the whole lattice
  - idempotence (meet(a, a) = a)
  - both is the meet identity (meet(a, both) = a)
  - none is meet-absorbing (meet(a, none) = none)
*/
bool tests_algebra_meet_lattice_laws()
{
    std::size_t n = 0;
    const DLifetime::value* vals = all_lifetimes(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        // idempotence
        D_LT_CHECK(lifetime_meet(vals[i], vals[i]) == vals[i]);
        // both as identity
        D_LT_CHECK(lifetime_meet(vals[i], DLifetime::both) == vals[i]);
        // none as absorbing
        D_LT_CHECK(lifetime_meet(vals[i], DLifetime::none) == DLifetime::none);

        for (std::size_t j = 0; j < n; ++j)
        {
            // commutativity
            D_LT_CHECK(lifetime_meet(vals[i], vals[j]) ==
                       lifetime_meet(vals[j], vals[i]));
        }
    }

    return true;
}

/*
tests_algebra_join_lattice_laws
  join obeys the lattice laws.
  Tests the following:
  - commutativity across the whole lattice
  - idempotence (join(a, a) = a)
  - none is the join identity (join(a, none) = a)
  - both is join-absorbing (join(a, both) = both)
*/
bool tests_algebra_join_lattice_laws()
{
    std::size_t n = 0;
    const DLifetime::value* vals = all_lifetimes(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        D_LT_CHECK(lifetime_join(vals[i], vals[i]) == vals[i]);
        D_LT_CHECK(lifetime_join(vals[i], DLifetime::none) == vals[i]);
        D_LT_CHECK(lifetime_join(vals[i], DLifetime::both) == DLifetime::both);

        for (std::size_t j = 0; j < n; ++j)
        {
            D_LT_CHECK(lifetime_join(vals[i], vals[j]) ==
                       lifetime_join(vals[j], vals[i]));
        }
    }

    return true;
}

/*
tests_algebra_make_lifetime_from_stage_facts
  make_lifetime composes the four values from two stage booleans.
  Tests the following:
  - (true, true) is both
  - (true, false) is compile_time
  - (false, true) is runtime
  - (false, false) is none
*/
bool tests_algebra_make_lifetime_from_stage_facts()
{
    D_LT_CHECK(make_lifetime(true, true) == DLifetime::both);
    D_LT_CHECK(make_lifetime(true, false) == DLifetime::compile_time);
    D_LT_CHECK(make_lifetime(false, true) == DLifetime::runtime);
    D_LT_CHECK(make_lifetime(false, false) == DLifetime::none);

    static_assert(make_lifetime(true, true) == DLifetime::both, "make_lifetime constexpr");

    return true;
}

/*
tests_algebra_stage_predicates
  is_compile_time / is_runtime / is_both / is_none report stage membership.
  Tests the following:
  - is_compile_time is true for compile_time and both, false for runtime and none
  - is_runtime is true for runtime and both, false for compile_time and none
  - is_both is true only for both
  - is_none is true only for none
*/
bool tests_algebra_stage_predicates()
{
    // is_compile_time
    D_LT_CHECK(is_compile_time(DLifetime::compile_time));
    D_LT_CHECK(is_compile_time(DLifetime::both));
    D_LT_CHECK(!is_compile_time(DLifetime::runtime));
    D_LT_CHECK(!is_compile_time(DLifetime::none));

    // is_runtime
    D_LT_CHECK(is_runtime(DLifetime::runtime));
    D_LT_CHECK(is_runtime(DLifetime::both));
    D_LT_CHECK(!is_runtime(DLifetime::compile_time));
    D_LT_CHECK(!is_runtime(DLifetime::none));

    // is_both
    D_LT_CHECK(is_both(DLifetime::both));
    D_LT_CHECK(!is_both(DLifetime::compile_time));
    D_LT_CHECK(!is_both(DLifetime::runtime));
    D_LT_CHECK(!is_both(DLifetime::none));

    // is_none
    D_LT_CHECK(is_none(DLifetime::none));
    D_LT_CHECK(!is_none(DLifetime::both));
    D_LT_CHECK(!is_none(DLifetime::compile_time));

    return true;
}

/*
tests_algebra_exclusive_stage_predicates
  is_compile_time_only and is_runtime_only are the single-stage predicates.
  Tests the following:
  - is_compile_time_only is true only for compile_time
  - is_runtime_only is true only for runtime
  - neither is true for both
  - neither is true for none
*/
bool tests_algebra_exclusive_stage_predicates()
{
    // is_compile_time_only
    D_LT_CHECK(is_compile_time_only(DLifetime::compile_time));
    D_LT_CHECK(!is_compile_time_only(DLifetime::both));
    D_LT_CHECK(!is_compile_time_only(DLifetime::runtime));
    D_LT_CHECK(!is_compile_time_only(DLifetime::none));

    // is_runtime_only
    D_LT_CHECK(is_runtime_only(DLifetime::runtime));
    D_LT_CHECK(!is_runtime_only(DLifetime::both));
    D_LT_CHECK(!is_runtime_only(DLifetime::compile_time));
    D_LT_CHECK(!is_runtime_only(DLifetime::none));

    return true;
}

/*
tests_algebra_lifetime_name_spellings
  lifetime_name returns the stable spelling for each value.
  Tests the following:
  - both -> "both"
  - compile_time -> "compile_time"
  - runtime -> "runtime"
  - none -> "none"
*/
bool tests_algebra_lifetime_name_spellings()
{
    D_LT_CHECK(std::strcmp(lifetime_name(DLifetime::both), "both") == 0);
    D_LT_CHECK(std::strcmp(lifetime_name(DLifetime::compile_time), "compile_time") == 0);
    D_LT_CHECK(std::strcmp(lifetime_name(DLifetime::runtime), "runtime") == 0);
    D_LT_CHECK(std::strcmp(lifetime_name(DLifetime::none), "none") == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
