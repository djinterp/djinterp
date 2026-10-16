/*******************************************************************************
* djinterp [core]                                        carrier_tests_value.cpp
*
*
* path:      /tests/djinterp/core/meta/carrier_tests_value.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "carrier_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_carrier_val_t_carries_value
  Verifies val_t stores its NTTP argument as the static `value`, keeping the
  NTTP's own type (the `auto _Value` parameter). C++17 facility.
  Tests the following:
  - val_t<V>::value equals V for int, bool, char, an enumerator, and a
    negative value
  - the carried value's type matches the NTTP's type (int stays int, bool
    stays bool)
*/
bool
tests_carrier_val_t_carries_value()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    static_assert(val_t<10>::value == 10, "carry int 10");
    static_assert(val_t<true>::value == true, "carry bool true");
    static_assert(val_t<'a'>::value == 'a', "carry char 'a'");
    static_assert(val_t<color::green>::value == color::green, "carry enumerator");
    static_assert(val_t<-7>::value == -7, "carry a negative value");

    static_assert(std::is_same<std::remove_const_t<decltype(val_t<10>::value)>,
                               int>::value,
                  "an int NTTP stays int");
    static_assert(std::is_same<std::remove_const_t<decltype(val_t<true>::value)>,
                               bool>::value,
                  "a bool NTTP stays bool");

#endif

    return true;
}


/*
tests_carrier_val_object
  Verifies the value-domain carrier OBJECT val (a C++17 facility).
  Tests the following:
  - val<V> is an inline constexpr object of type const val_t<V>
  - its carried value is recoverable through decltype, with the const cleaned
  - it is recognized by is_value_carrier_v
*/
bool
tests_carrier_val_object()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    static_assert(std::is_same<decltype(val<10>), const val_t<10>>::value,
                  "val is a const carrier object");
    static_assert(carrier_value_v<decltype(val<10>)> == 10,
                  "recover 10 through the object (const cleaned)");
    static_assert(is_value_carrier_v<decltype(val<10>)> == true,
                  "the object is detected as a value carrier");

#endif

    return true;
}


/*
tests_carrier_value_recover
  Verifies carrier_value_v recovers the carried value from a val_t.
  Tests the following:
  - carrier_value_v<val_t<V>> yields V for int, bool, char, and an enumerator
*/
bool
tests_carrier_value_recover()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    static_assert(carrier_value_v<val_t<10>> == 10, "recover int");
    static_assert(carrier_value_v<val_t<true>> == true, "recover bool");
    static_assert(carrier_value_v<val_t<'a'>> == 'a', "recover char");
    static_assert(carrier_value_v<val_t<color::blue>> == color::blue,
                  "recover enumerator");

#endif

    return true;
}


/*
tests_carrier_value_recover_cvref
  Verifies carrier_value_v ignores cv-qualifiers and references ON THE CARRIER
  (the clean_t step), exactly as the header documents.
  Tests the following:
  - a const, reference, const-reference, or volatile carrier all recover the
    same carried value
*/
bool
tests_carrier_value_recover_cvref()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    static_assert(carrier_value_v<const val_t<10>> == 10, "const carrier");
    static_assert(carrier_value_v<val_t<10>&> == 10, "reference carrier");
    static_assert(carrier_value_v<const val_t<10>&> == 10, "const-ref carrier");
    static_assert(carrier_value_v<volatile val_t<10>> == 10, "volatile carrier");

#endif

    return true;
}


/*
tests_carrier_value_runtime
  Verifies a value carrier collapses to an ordinary runtime value -- the
  carrier contract. These are genuine run-time checks (not just compile-time
  assertions): the recovered NTTPs are read into runtime variables.
  Tests the following:
  - recovered int, bool, and char values read correctly at run time
  - a negative recovered value reads correctly at run time
*/
bool
tests_carrier_value_runtime()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    const int  i = carrier_value_v<val_t<42>>;
    const bool b = carrier_value_v<val_t<true>>;
    const char c = val_t<'Z'>::value;

    D_CR_CHECK(i == 42);
    D_CR_CHECK(b == true);
    D_CR_CHECK(c == 'Z');
    D_CR_CHECK(carrier_value_v<val_t<-7>> == -7);

#endif

    return true;
}


NS_END  // testing
NS_END  // djinterp
