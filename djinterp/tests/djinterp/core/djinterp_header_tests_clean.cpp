/******************************************************************************
* djinterp [test]                           djinterp_header_tests_clean.cpp
*
*   Section III.iii tests: clean / clean_t (cv- and reference-stripper).
******************************************************************************/

#include "./djinterp_header_tests.hpp"


namespace
{

    // clean_fixture
    //   type: a class type used to confirm clean works on user types, not
    // just fundamentals.
    struct clean_fixture
    {};

}  // anonymous namespace


NS_DJINTERP
NS_TEST


/*
tests_clean
  Verifies the clean trait and its clean_t alias strip top-level cv-qualifiers
  and references while leaving everything else intact.
  Tests the following:
  - the unqualified type is returned unchanged
  - const, volatile, and const-volatile are removed
  - lvalue and rvalue references are removed
  - combined cv + reference is fully removed
  - pointers are preserved (only the pointer's own top-level cv is stripped);
    pointee qualification is left intact
  - arrays survive, with array cv stripped from the element type
  - the trait works on a user-defined class type
  - clean is idempotent
  - both clean<>::type (the struct) and clean_t (the alias) are exercised
*/
bool
tests_clean()
{
    bool ok = true;

    // identity.
    static_assert(std::is_same<clean_t<int>, int>::value,
                  "clean_t<int> must be int.");

    // cv removal.
    static_assert(std::is_same<clean_t<const int>, int>::value,
                  "clean_t must strip const.");
    static_assert(std::is_same<clean_t<volatile int>, int>::value,
                  "clean_t must strip volatile.");
    static_assert(std::is_same<clean_t<const volatile int>, int>::value,
                  "clean_t must strip const volatile.");

    // reference removal.
    static_assert(std::is_same<clean_t<int&>, int>::value,
                  "clean_t must strip lvalue ref.");
    static_assert(std::is_same<clean_t<int&&>, int>::value,
                  "clean_t must strip rvalue ref.");

    // combined cv + reference.
    static_assert(std::is_same<clean_t<const int&>, int>::value,
                  "clean_t must strip const ref.");
    static_assert(std::is_same<clean_t<const volatile int&>, int>::value,
                  "clean_t must strip const volatile ref.");

    // pointers: the pointer itself is cleaned, the pointee is not.
    static_assert(std::is_same<clean_t<int*>, int*>::value,
                  "clean_t must preserve the pointer type.");
    static_assert(std::is_same<clean_t<int* const>, int*>::value,
                  "clean_t must strip top-level const off a pointer.");
    static_assert(std::is_same<clean_t<const int*>, const int*>::value,
                  "clean_t must preserve pointee const.");

    // arrays survive; element cv is stripped.
    static_assert(std::is_same<clean_t<int[3]>, int[3]>::value,
                  "clean_t must preserve array type.");
    static_assert(std::is_same<clean_t<const int[3]>, int[3]>::value,
                  "clean_t must strip cv from an array's elements.");

    // user-defined class type.
    static_assert(std::is_same<clean_t<const clean_fixture&>,
                               clean_fixture>::value,
                  "clean_t must clean a user class type.");

    // idempotence.
    static_assert(std::is_same<clean_t<clean_t<const volatile int&>>,
                               int>::value,
                  "clean_t must be idempotent.");

    // exercise the struct form directly (not just the alias).
    static_assert(std::is_same<clean<const int&>::type, int>::value,
                  "clean<>::type must match clean_t.");

    // runtime mirror.
    ok = ok && std::is_same<clean_t<const volatile int&>, int>::value;
    ok = ok && std::is_same<clean_t<const int*>, const int*>::value;
    ok = ok && std::is_same<clean_t<const clean_fixture&>,
                            clean_fixture>::value;

    return ok;
}


NS_END  // test
NS_END  // djinterp
