/******************************************************************************
* djinterp [test]                    djinterp_header_tests_self_resolve.cpp
*
*   Section III.vi tests: the self marker, the is_self / is_self_v detection
* traits, and the resolve_self / resolve_self_t family of specializations that
* rewrite the self marker into a concrete target type.
******************************************************************************/

#include "./djinterp_header_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_self_and_is_self
  Verifies the self marker type and the is_self trait that recognizes it.
  Tests the following:
  - is_self<self> is true (the dedicated specialization)
  - is_self<int> is false (the primary template)
  - is_self does NOT see through qualifiers or references: is_self<const self>,
    is_self<self&>, and is_self<self*> are all false, since only the exact
    self type is the marker
  - where variable templates are available (C++14+), is_self_v mirrors
    is_self<>::value; otherwise that leg is skipped (vacuously true) so the
    suite stays portable to C++11
*/
bool
tests_self_and_is_self()
{
    bool ok = true;

    // positive: the marker itself.
    static_assert(is_self<self>::value,
                  "is_self<self> must be true.");

    // negative: an unrelated type.
    static_assert(!is_self<int>::value,
                  "is_self<int> must be false.");

    // negative: only the exact, unqualified self type is the marker.
    static_assert(!is_self<const self>::value,
                  "is_self<const self> must be false.");
    static_assert(!is_self<self&>::value,
                  "is_self<self&> must be false.");
    static_assert(!is_self<self*>::value,
                  "is_self<self*> must be false.");

    ok = ok && is_self<self>::value;
    ok = ok && !is_self<int>::value;
    ok = ok && !is_self<const self>::value;
    ok = ok && !is_self<self&>::value;

    // is_self_v is a variable template -> C++14 or newer only.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_self_v<self>,
                  "is_self_v<self> must be true.");
    static_assert(!is_self_v<int>,
                  "is_self_v<int> must be false.");
    static_assert(is_self_v<self> == is_self<self>::value,
                  "is_self_v must mirror is_self<>::value.");

    ok = ok && is_self_v<self>;
    ok = ok && !is_self_v<int>;
#endif

    return ok;
}


/*
tests_resolve_self
  Verifies resolve_self / resolve_self_t rewrite the self marker into a target
  type across every specialization the header provides, and leave non-matching
  types untouched.
  Tests the following:
  - the bare marker resolves to the target: resolve_self_t<self, int> == int
  - a type with no self passes through unchanged (primary template):
    resolve_self_t<int, int> == int
  - the dedicated smart-pointer specializations:
      unique_ptr<self> -> unique_ptr<int>
      shared_ptr<self> -> shared_ptr<int>
      weak_ptr<self>   -> weak_ptr<int>
  - the dedicated raw-pointer specialization: self* -> int*
  - the variadic catch-all rewrites self inside arbitrary type-only class
    templates: std::vector<self> -> std::vector<int>,
    std::pair<int, self> -> std::pair<int, int>, and a user template
    user_box<self, char> -> user_box<int, char>
  - the catch-all recurses through nesting:
    std::vector<std::unique_ptr<self>> -> std::vector<std::unique_ptr<int>>
  - a template carrying a NON-type parameter does NOT match the catch-all and
    falls through unchanged: nontype_box<self, 3> stays nontype_box<self, 3>
  - the dedicated repeat specialization recurses into the repeated type:
    repeat<self, 3> -> std::tuple<int, int, int>
  - both resolve_self<>::type (the struct) and resolve_self_t (the alias) are
    exercised
*/
bool
tests_resolve_self()
{
    bool ok = true;

    // bare marker -> target (base-case specialization).
    static_assert(std::is_same<resolve_self_t<self, int>, int>::value,
                  "resolve_self_t<self, int> must be int.");

    // no self -> unchanged (primary passthrough template).
    static_assert(std::is_same<resolve_self_t<int, int>, int>::value,
                  "resolve_self_t<int, int> must be int.");
    static_assert(std::is_same<resolve_self_t<double, int>, double>::value,
                  "resolve_self_t passthrough must leave double alone.");

    // dedicated smart-pointer specializations.
    static_assert(std::is_same<resolve_self_t<std::unique_ptr<self>, int>,
                               std::unique_ptr<int>>::value,
                  "unique_ptr<self> must resolve to unique_ptr<int>.");
    static_assert(std::is_same<resolve_self_t<std::shared_ptr<self>, int>,
                               std::shared_ptr<int>>::value,
                  "shared_ptr<self> must resolve to shared_ptr<int>.");
    static_assert(std::is_same<resolve_self_t<std::weak_ptr<self>, int>,
                               std::weak_ptr<int>>::value,
                  "weak_ptr<self> must resolve to weak_ptr<int>.");

    // dedicated raw-pointer specialization.
    static_assert(std::is_same<resolve_self_t<self*, int>, int*>::value,
                  "self* must resolve to int*.");

    // variadic catch-all over type-only class templates.
    static_assert(std::is_same<resolve_self_t<std::vector<self>, int>,
                               std::vector<int>>::value,
                  "vector<self> must resolve to vector<int>.");
    static_assert(std::is_same<resolve_self_t<std::pair<int, self>, int>,
                               std::pair<int, int>>::value,
                  "pair<int, self> must resolve to pair<int, int>.");
    static_assert(std::is_same<
                      resolve_self_t<internal::user_box<self, char>, int>,
                      internal::user_box<int, char>>::value,
                  "user_box<self, char> must resolve to user_box<int, char>.");

    // catch-all recurses through nesting.
    static_assert(std::is_same<
                      resolve_self_t<std::vector<std::unique_ptr<self>>, int>,
                      std::vector<std::unique_ptr<int>>>::value,
                  "nested vector<unique_ptr<self>> must fully resolve.");

    // a non-type template parameter excludes the catch-all -> passthrough.
    static_assert(std::is_same<
                      resolve_self_t<internal::nontype_box<self, 3>, int>,
                      internal::nontype_box<self, 3>>::value,
                  "nontype_box<self, 3> must be left unchanged.");

    // dedicated repeat specialization recurses into the repeated type.
    static_assert(std::is_same<resolve_self_t<repeat<self, 3>, int>,
                               std::tuple<int, int, int>>::value,
                  "repeat<self, 3> must resolve to tuple<int, int, int>.");

    // exercise the struct form directly (not just the alias).
    static_assert(std::is_same<resolve_self<self, int>::type, int>::value,
                  "resolve_self<>::type must match resolve_self_t.");
    static_assert(std::is_same<
                      resolve_self<std::unique_ptr<self>, int>::type,
                      std::unique_ptr<int>>::value,
                  "resolve_self<>::type must match for unique_ptr<self>.");

    // runtime mirror.
    ok = ok && std::is_same<resolve_self_t<self, int>, int>::value;
    ok = ok && std::is_same<resolve_self_t<int, int>, int>::value;
    ok = ok && std::is_same<resolve_self_t<std::unique_ptr<self>, int>,
                            std::unique_ptr<int>>::value;
    ok = ok && std::is_same<resolve_self_t<self*, int>, int*>::value;
    ok = ok && std::is_same<resolve_self_t<std::vector<self>, int>,
                            std::vector<int>>::value;
    ok = ok && std::is_same<
                   resolve_self_t<std::vector<std::unique_ptr<self>>, int>,
                   std::vector<std::unique_ptr<int>>>::value;
    ok = ok && std::is_same<
                   resolve_self_t<internal::nontype_box<self, 3>, int>,
                   internal::nontype_box<self, 3>>::value;
    ok = ok && std::is_same<resolve_self_t<repeat<self, 3>, int>,
                            std::tuple<int, int, int>>::value;

    return ok;
}


NS_END  // test
NS_END  // djinterp
