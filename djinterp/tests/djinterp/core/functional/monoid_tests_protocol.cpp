// djinterp [test]  monoid_tests_protocol.cpp
//   Section II -- MONOID PROTOCOL (is_monoid / is_monoid_v / Monoid).

// std
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_monoid_positive
  Confirms is_monoid is true for every standard instance.
  Tests the following:
  - std::string and std::vector<T> (container instances)
  - sum / product over int and double
  - all / any
  - min / max
*/
bool
tests_is_monoid_positive()
{
    bool ok = true;

    ok = ok && (is_monoid<std::string>::value);
    ok = ok && (is_monoid<std::vector<int> >::value);
    ok = ok && (is_monoid<std::vector<std::string> >::value);
    ok = ok && (is_monoid<sum<int> >::value);
    ok = ok && (is_monoid<sum<double> >::value);
    ok = ok && (is_monoid<product<int> >::value);
    ok = ok && (is_monoid<product<double> >::value);
    ok = ok && (is_monoid<all>::value);
    ok = ok && (is_monoid<any>::value);
    ok = ok && (is_monoid<min<int> >::value);
    ok = ok && (is_monoid<max<int> >::value);

    return ok;
}


/*
tests_is_monoid_negative
  Confirms is_monoid is false for types with no monoid_traits specialization,
  and -- critically -- that the detector resolves cleanly rather than hard-
  erroring on the undefined primary template.
  Tests the following:
  - scalars (int, double, char, bool)
  - an unrelated user struct (not_a_monoid)
  - pointers, including a pointer to a monoid type
*/
bool
tests_is_monoid_negative()
{
    bool ok = true;

    ok = ok && (!is_monoid<int>::value);
    ok = ok && (!is_monoid<double>::value);
    ok = ok && (!is_monoid<char>::value);
    ok = ok && (!is_monoid<bool>::value);
    ok = ok && (!is_monoid<not_a_monoid>::value);
    ok = ok && (!is_monoid<void*>::value);

    // a pointer to a monoid type is itself not a monoid.
    ok = ok && (!is_monoid<sum<int>*>::value);

    return ok;
}


/*
tests_is_monoid_decay
  Confirms is_monoid strips cv-qualifiers and references before testing.
  Tests the following:
  - const / lvalue-ref / const-ref / volatile qualified monoid types resolve true
  - a cv/ref qualified non-monoid resolves false
*/
bool
tests_is_monoid_decay()
{
    bool ok = true;

    ok = ok && (is_monoid<sum<int> >::value);
    ok = ok && (is_monoid<const sum<int> >::value);
    ok = ok && (is_monoid<sum<int>&>::value);
    ok = ok && (is_monoid<const sum<int>&>::value);
    ok = ok && (is_monoid<volatile sum<int> >::value);

    // decay of a non-monoid stays non-monoid.
    ok = ok && (!is_monoid<const int&>::value);

    return ok;
}


/*
tests_is_monoid_v
  Confirms the is_monoid_v variable-template shorthand agrees with the trait
  (skipped, reporting pass, where variable templates are unavailable).
  Tests the following:
  - is_monoid_v<T> == is_monoid<T>::value for positive and negative T
  - the concrete true / false values
*/
bool
tests_is_monoid_v()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    bool ok = true;

    // agreement with the trait.
    ok = ok && (is_monoid_v<sum<int> >    == is_monoid<sum<int> >::value);
    ok = ok && (is_monoid_v<std::string>  == is_monoid<std::string>::value);
    ok = ok && (is_monoid_v<int>          == is_monoid<int>::value);
    ok = ok && (is_monoid_v<not_a_monoid> == is_monoid<not_a_monoid>::value);

    // concrete values.
    ok = ok && (is_monoid_v<sum<int> > == true);
    ok = ok && (is_monoid_v<int>       == false);

    return ok;
#else
    // variable templates unavailable: nothing to check.
    return true;
#endif
}


/*
tests_monoid_concept
  Confirms the C++20 Monoid concept mirrors is_monoid (skipped, reporting pass,
  where concepts are unavailable).
  Tests the following:
  - instances satisfy Monoid
  - scalars and unrelated structs do not
*/
bool
tests_monoid_concept()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Monoid<sum<int> >,         "sum models Monoid");
    static_assert(Monoid<std::string>,       "string models Monoid");
    static_assert(Monoid<std::vector<int> >, "vector models Monoid");
    static_assert(Monoid<all>,               "all models Monoid");
    static_assert(Monoid<min<int> >,         "min models Monoid");

    static_assert(!Monoid<int>,              "int does not model Monoid");
    static_assert(!Monoid<not_a_monoid>,     "plain struct is not a Monoid");
#endif

    return true;
}


/*
tests_monoid_implies_semigroup
  Confirms the documented invariant: a monoid is necessarily a semigroup, so
  is_semigroup holds for every type is_monoid holds for.
  Tests the following:
  - for each standard instance, is_monoid AND is_semigroup are both true
*/
bool
tests_monoid_implies_semigroup()
{
    bool ok = true;

    ok = ok && ( is_monoid<std::string>::value &&
                 is_semigroup<std::string>::value );
    ok = ok && ( is_monoid<std::vector<int> >::value &&
                 is_semigroup<std::vector<int> >::value );
    ok = ok && ( is_monoid<sum<int> >::value &&
                 is_semigroup<sum<int> >::value );
    ok = ok && ( is_monoid<product<int> >::value &&
                 is_semigroup<product<int> >::value );
    ok = ok && ( is_monoid<all>::value &&
                 is_semigroup<all>::value );
    ok = ok && ( is_monoid<any>::value &&
                 is_semigroup<any>::value );
    ok = ok && ( is_monoid<min<int> >::value &&
                 is_semigroup<min<int> >::value );
    ok = ok && ( is_monoid<max<int> >::value &&
                 is_semigroup<max<int> >::value );

    return ok;
}


NS_END  // testing
NS_END  // djinterp
