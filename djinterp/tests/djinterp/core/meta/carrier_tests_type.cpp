/*******************************************************************************
* djinterp [core]                                         carrier_tests_type.cpp
*
*
* path:      /tests/djinterp/core/meta/carrier_tests_type.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "carrier_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_carrier_type_t_carries_type
  Verifies type_t stores its argument as the nested `type`, verbatim.
  Tests the following:
  - type_t<T>::type is exactly T for a builtin, a user type, and void
  - cv-qualifiers and references on the carried type are preserved (type_t
    does not clean its own argument)
*/
bool
tests_carrier_type_t_carries_type()
{
    static_assert(std::is_same<type_t<int>::type, int>::value,
                  "type_t<int> must carry int");
    static_assert(std::is_same<type_t<sample>::type, sample>::value,
                  "type_t<sample> must carry sample");
    static_assert(std::is_same<type_t<void>::type, void>::value,
                  "type_t<void> must carry void");
    static_assert(std::is_same<type_t<int&>::type, int&>::value,
                  "a carried reference is preserved verbatim");
    static_assert(std::is_same<type_t<const int>::type, const int>::value,
                  "a carried cv-qualifier is preserved verbatim");

    return true;
}


/*
tests_carrier_type_t_recover
  Verifies carrier_type_t recovers the carried type from a type_t.
  Tests the following:
  - carrier_type_t<type_t<T>> yields T for a builtin, a user type, and void
*/
bool
tests_carrier_type_t_recover()
{
    static_assert(std::is_same<carrier_type_t<type_t<int>>, int>::value,
                  "recover int from type_t<int>");
    static_assert(std::is_same<carrier_type_t<type_t<sample>>, sample>::value,
                  "recover sample from type_t<sample>");
    static_assert(std::is_same<carrier_type_t<type_t<void>>, void>::value,
                  "recover void from type_t<void>");

    return true;
}


/*
tests_carrier_type_t_recover_cvref
  Verifies carrier_type_t ignores cv-qualifiers and references ON THE CARRIER
  (the clean_t step), exactly as the header documents.
  Tests the following:
  - a const, reference, const-reference, volatile, or rvalue-reference carrier
    all recover the same carried type
*/
bool
tests_carrier_type_t_recover_cvref()
{
    static_assert(std::is_same<carrier_type_t<const type_t<int>>, int>::value,
                  "const carrier");
    static_assert(std::is_same<carrier_type_t<type_t<int>&>, int>::value,
                  "lvalue-reference carrier");
    static_assert(std::is_same<carrier_type_t<const type_t<int>&>, int>::value,
                  "const lvalue-reference carrier");
    static_assert(std::is_same<carrier_type_t<volatile type_t<int>>, int>::value,
                  "volatile carrier");
    static_assert(std::is_same<carrier_type_t<type_t<int>&&>, int>::value,
                  "rvalue-reference carrier");

    return true;
}


/*
tests_carrier_type_t_nested
  Verifies a carrier can carry another carrier (carriers are just types).
  Tests the following:
  - type_t<type_t<int>> is itself a type carrier
  - carrier_type_t recovers exactly one layer, yielding the inner type_t<int>
*/
bool
tests_carrier_type_t_nested()
{
    static_assert(is_type_carrier<type_t<type_t<int>>>::value,
                  "a type_t carrying a type_t is still a type carrier");
    static_assert(std::is_same<carrier_type_t<type_t<type_t<int>>>,
                               type_t<int>>::value,
                  "recovery peels exactly one carrier layer");

    return true;
}


/*
tests_carrier_type_c_object
  Verifies the type-domain carrier OBJECT type_c (a C++17 facility).
  Tests the following:
  - type_c<T> is an inline constexpr object of type const type_t<T>
  - its carried type is recoverable through decltype, with the const cleaned
  - it is recognized by is_type_carrier_v
*/
bool
tests_carrier_type_c_object()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    static_assert(std::is_same<decltype(type_c<int>),
                               const type_t<int>>::value,
                  "type_c is a const carrier object");
    static_assert(std::is_same<carrier_type_t<decltype(type_c<int>)>,
                               int>::value,
                  "recover int through the object (const cleaned)");
    static_assert(is_type_carrier_v<decltype(type_c<int>)> == true,
                  "the object is detected as a type carrier");

#endif

    return true;
}


NS_END  // testing
NS_END  // djinterp
