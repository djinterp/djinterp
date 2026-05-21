/******************************************************************************
* djinterp [test]                          djinterp_header_tests_void_t.cpp
*
*   Section III.i tests: void_t (the pre-C++17 SFINAE sink).
******************************************************************************/

#include "./djinterp_header_tests.hpp"


namespace
{

    // has_nested_type
    //   trait: a detection-idiom probe built on void_t.  Specializes to
    // true_type only when T::nested is a valid type, exercising void_t in
    // exactly the role it exists for.  Note the use of the *global* ::void_t,
    // confirming the alias is declared outside namespace djinterp.
    template<typename _Type,
             typename _Enable = void>
    struct has_nested_type : std::false_type
    {};

    template<typename _Type>
    struct has_nested_type<_Type,
                           ::void_t<typename _Type::nested>>
        : std::true_type
    {};

    // with_nested
    //   type: positive fixture for has_nested_type.
    struct with_nested
    {
        using nested = int;
    };

    // without_nested
    //   type: negative fixture for has_nested_type.
    struct without_nested
    {};

}  // anonymous namespace


NS_DJINTERP
NS_TEST


/*
tests_void_t
  Verifies the void_t alias template.
  Tests the following:
  - void_t is reachable at global scope (referenced as ::void_t) and is NOT
    inside namespace djinterp
  - void_t maps to void for zero, one, and many type arguments
  - void_t maps to void for cv-qualified, reference, and pointer arguments
  - void_t functions as a SFINAE sink: a detection trait built on it reports
    true for a type with the probed member and false for one without
*/
bool
tests_void_t()
{
    bool ok = true;

    // void_t collapses any type list -- including the empty list -- to void.
    static_assert(std::is_same< ::void_t<>,                void>::value,
                  "void_t<> must be void.");
    static_assert(std::is_same< ::void_t<int>,             void>::value,
                  "void_t<int> must be void.");
    static_assert(std::is_same< ::void_t<int, char, long>, void>::value,
                  "void_t<...> must be void for many args.");
    static_assert(std::is_same< ::void_t<const volatile int&, double*>,
                               void>::value,
                  "void_t must be void for qualified/ref/pointer args.");

    // void_t as a working detection sink.
    static_assert(has_nested_type<with_nested>::value,
                  "void_t-based detection must see the nested member.");
    static_assert(!has_nested_type<without_nested>::value,
                  "void_t-based detection must reject the missing member.");
    static_assert(!has_nested_type<int>::value,
                  "void_t-based detection must reject a fundamental type.");

    // runtime mirror
    ok = ok && std::is_same< ::void_t<>, void>::value;
    ok = ok && std::is_same< ::void_t<int, char, long>, void>::value;
    ok = ok && has_nested_type<with_nested>::value;
    ok = ok && (!has_nested_type<without_nested>::value);
    ok = ok && (!has_nested_type<int>::value);

    return ok;
}


NS_END  // test
NS_END  // djinterp
