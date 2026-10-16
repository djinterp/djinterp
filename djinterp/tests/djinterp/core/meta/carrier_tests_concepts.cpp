/*******************************************************************************
* djinterp [core]                                     carrier_tests_concepts.cpp
*
*
* path:      /tests/djinterp/core/meta/carrier_tests_concepts.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "carrier_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace
{
    // accept (fallback)
    //   a deliberately unconstrained overload, selected only when the
    // argument is not a Carrier.
    template<typename _Type>
    constexpr int
    accept(_Type)
    {
        return 0;
    }

    // accept (constrained)
    //   the Carrier-constrained overload; it is more specialized than the
    // fallback whenever the argument satisfies Carrier, so it wins for
    // carriers.
    template<Carrier _Type>
    constexpr int
    accept(_Type)
    {
        return 1;
    }
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


/*
tests_carrier_concept_type_carrier
  Verifies the TypeCarrier concept face (C++20).
  Tests the following:
  - a type_t satisfies TypeCarrier; a builtin and a value carrier do not
  - cv-ref is cleaned (the concept mirrors is_type_carrier, which cleans)
*/
bool
tests_carrier_concept_type_carrier()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    static_assert(TypeCarrier<type_t<int>>, "type_t satisfies TypeCarrier");
    static_assert(!TypeCarrier<int>, "int does not");
    static_assert(!TypeCarrier<val_t<5>>, "a value carrier does not");
    static_assert(TypeCarrier<const type_t<int>&>,
                  "cv-ref is cleaned via the trait");

#endif

    return true;
}


/*
tests_carrier_concept_value_carrier
  Verifies the ValueCarrier concept face (C++20).
  Tests the following:
  - a val_t satisfies ValueCarrier; a builtin and a type carrier do not
  - cv-ref is cleaned
*/
bool
tests_carrier_concept_value_carrier()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    static_assert(ValueCarrier<val_t<5>>, "val_t satisfies ValueCarrier");
    static_assert(!ValueCarrier<int>, "int does not");
    static_assert(!ValueCarrier<type_t<int>>, "a type carrier does not");
    static_assert(ValueCarrier<const val_t<5>&>, "cv-ref is cleaned");

#endif

    return true;
}


/*
tests_carrier_concept_carrier
  Verifies the Carrier concept face (C++20), satisfied by either kind.
  Tests the following:
  - both a type carrier and a value carrier satisfy Carrier
  - a builtin and the decoy do not
*/
bool
tests_carrier_concept_carrier()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    static_assert(Carrier<type_t<int>>, "a type carrier satisfies Carrier");
    static_assert(Carrier<val_t<5>>, "a value carrier satisfies Carrier");
    static_assert(!Carrier<int>, "a builtin does not");
    static_assert(!Carrier<decoy>, "the decoy does not");

#endif

    return true;
}


/*
tests_carrier_concept_constrains
  Verifies the Carrier concept actually constrains overload resolution -- its
  intended use. This is a genuine run-time check on the dispatch outcome.
  Tests the following:
  - carrier arguments (type_c, val) select the constrained overload (1)
  - non-carrier arguments (a builtin, a user type) fall back (0)
*/
bool
tests_carrier_concept_constrains()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    D_CR_CHECK(accept(type_c<int>) == 1);
    D_CR_CHECK(accept(val<5>)      == 1);
    D_CR_CHECK(accept(42)          == 0);
    D_CR_CHECK(accept(sample{})    == 0);

#endif

    return true;
}


NS_END  // testing
NS_END  // djinterp
