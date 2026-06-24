// std
#include <cstdint>
#include <type_traits>
#include <vector>
// djinterp
#include "test_kind_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- V. STRUCTURAL DETECTION                      ///
///////////////////////////////////////////////////////////////////////////////

// positive: any instantiation of the wrapper
static_assert(is_test_kind_set<test_kind_set<id_set> >::value,    "");
static_assert(is_test_kind_set<test_kind_set<id_set_nc> >::value, "");
static_assert(is_test_kind_set<test_kind_set<kind_set> >::value,  "");

// negative: non-instantiations
static_assert(!is_test_kind_set<int>::value,                      "");
static_assert(!is_test_kind_set<test_kind>::value,                "");
static_assert(!is_test_kind_set<id_set>::value,                   "");
static_assert(!is_test_kind_set<std::vector<test_kind> >::value,  "");
static_assert(!is_test_kind_set<test_type_id>::value,             "");

// clean_t strips cv and reference qualifiers ...
static_assert(is_test_kind_set<const test_kind_set<id_set> >::value,    "");
static_assert(is_test_kind_set<volatile test_kind_set<id_set> >::value, "");
static_assert(is_test_kind_set<test_kind_set<id_set>& >::value,         "");
static_assert(is_test_kind_set<const test_kind_set<id_set>& >::value,   "");
static_assert(is_test_kind_set<test_kind_set<id_set>&& >::value,        "");
// ... but NOT pointers
static_assert(!is_test_kind_set<test_kind_set<id_set>* >::value,        "");
static_assert(!is_test_kind_set<const test_kind_set<id_set>* >::value,  "");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_test_kind_set_v<test_kind_set<id_set> >,  "");
static_assert(!is_test_kind_set_v<int>,                    "");
static_assert(is_test_kind_set_v<const test_kind_set<id_set>& >, "");
#endif


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- V. STRUCTURAL DETECTION                                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_test_kind_set
  Verifies the is_test_kind_set trait's core classification.
  Tests the following:
  - it is true for every instantiation of test_kind_set, regardless of the
    wrapped container
  - it is false for unrelated types, including the bare container, a record,
    a plain vector of records, and scalars
*/
bool
tests_is_test_kind_set()
{
    bool ok = true;

    ok = D_TK_CHECK(is_test_kind_set<test_kind_set<id_set> >::value)    && ok;
    ok = D_TK_CHECK(is_test_kind_set<test_kind_set<id_set_nc> >::value) && ok;
    ok = D_TK_CHECK(is_test_kind_set<test_kind_set<kind_set> >::value)  && ok;

    ok = D_TK_CHECK(!is_test_kind_set<int>::value)                     && ok;
    ok = D_TK_CHECK(!is_test_kind_set<test_kind>::value)               && ok;
    ok = D_TK_CHECK(!is_test_kind_set<id_set>::value)                  && ok;
    ok = D_TK_CHECK(!is_test_kind_set<std::vector<test_kind> >::value) && ok;
    ok = D_TK_CHECK(!is_test_kind_set<test_type_id>::value)            && ok;

    return ok;
}


/*
tests_is_test_kind_set_cvref
  Verifies the trait's handling of qualified inputs.
  Tests the following:
  - cv-qualifiers and references are stripped before classification (clean_t),
    so qualified instantiations still register as true
  - pointers are NOT stripped, so a pointer-to-wrapper registers as false
*/
bool
tests_is_test_kind_set_cvref()
{
    bool ok = true;

    ok = D_TK_CHECK(is_test_kind_set<const test_kind_set<id_set> >::value)
         && ok;
    ok = D_TK_CHECK(is_test_kind_set<volatile test_kind_set<id_set> >::value)
         && ok;
    ok = D_TK_CHECK(is_test_kind_set<test_kind_set<id_set>& >::value)
         && ok;
    ok = D_TK_CHECK(is_test_kind_set<const test_kind_set<id_set>& >::value)
         && ok;
    ok = D_TK_CHECK(is_test_kind_set<test_kind_set<id_set>&& >::value)
         && ok;

    ok = D_TK_CHECK(!is_test_kind_set<test_kind_set<id_set>* >::value)
         && ok;
    ok = D_TK_CHECK(!is_test_kind_set<const test_kind_set<id_set>* >::value)
         && ok;

    return ok;
}


/*
tests_is_test_kind_set_variable
  Verifies the is_test_kind_set_v variable template where available.
  Tests the following:
  - it mirrors the trait's ::value for both positive and negative cases
  - on toolchains without variable templates the underlying trait is checked
    directly instead
*/
bool
tests_is_test_kind_set_variable()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = D_TK_CHECK(is_test_kind_set_v<test_kind_set<id_set> > == true)  && ok;
    ok = D_TK_CHECK(is_test_kind_set_v<int> == false)                   && ok;
    ok = D_TK_CHECK(is_test_kind_set_v<test_kind_set<id_set> >
                        == is_test_kind_set<test_kind_set<id_set> >::value)
         && ok;
#else
    // variable templates unavailable: exercise the trait the variable wraps
    ok = D_TK_CHECK(is_test_kind_set<test_kind_set<id_set> >::value == true)
         && ok;
    ok = D_TK_CHECK(is_test_kind_set<int>::value == false) && ok;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
