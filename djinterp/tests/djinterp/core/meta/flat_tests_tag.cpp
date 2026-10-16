/*******************************************************************************
* djinterp [core]                                             flat_tests_tag.cpp
*
*
* path:      /tests/djinterp/core/meta/flat_tests_tag.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "flat_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_tag_flat_nests_is_false
  flat::nests is false.
  Tests the following:
  - the value is false
  - it is exactly false (a bool, compared as such)
  - it is of type bool
  - it is a constant expression (static_assert)
*/
bool
tests_tag_flat_nests_is_false()
{
    D_FL_CHECK(flat::nests == false);
    D_FL_CHECK(!flat::nests);
    D_FL_CHECK((std::is_same<decltype(flat::nests), const bool>::value));

    static_assert(flat::nests == false, "flat does not nest");

    return true;
}

/*
tests_tag_flat_min_depth_is_one
  flat::min_depth is 1.
  Tests the following:
  - the value is 1
  - it is of type std::size_t
  - it is strictly less than hierarchical's minimum (a flat type is
    shallower) -- checked here only against the literal 2 the model fixes
  - it is a constant expression
*/
bool
tests_tag_flat_min_depth_is_one()
{
    D_FL_CHECK(flat::min_depth == 1u);
    D_FL_CHECK((std::is_same<decltype(flat::min_depth), const std::size_t>::value));
    D_FL_CHECK(flat::min_depth < 2u);   // the hierarchical floor, per the model

    static_assert(flat::min_depth == 1u, "flat depth is 1");

    return true;
}

/*
tests_tag_flat_name_is_flat
  flat::name() returns "flat".
  Tests the following:
  - the returned string equals "flat"
  - the return type is const char*
  - the function is callable in a constant expression (constexpr)
  - repeated calls yield the same stable spelling
*/
bool
tests_tag_flat_name_is_flat()
{
    D_FL_CHECK(std::strcmp(flat::name(), "flat") == 0);
    D_FL_CHECK((std::is_same<decltype(flat::name()), const char*>::value));

    // constexpr-callable: usable to initialize a constexpr pointer
    constexpr const char* n = flat::name();
    D_FL_CHECK(n[0] == 'f');

    // stable across calls
    D_FL_CHECK(std::strcmp(flat::name(), flat::name()) == 0);

    return true;
}

/*
tests_tag_flat_is_empty_tag
  flat is an empty tag type.
  Tests the following:
  - it is a class type
  - it is empty (no non-static data members)
  - it is trivially default constructible
  - it is standard-layout
*/
bool
tests_tag_flat_is_empty_tag()
{
    D_FL_CHECK(std::is_class<flat>::value);
    D_FL_CHECK(std::is_empty<flat>::value);
    D_FL_CHECK(std::is_trivially_default_constructible<flat>::value);
    D_FL_CHECK(std::is_standard_layout<flat>::value);

    return true;
}

/*
tests_tag_flat_members_are_constant_expressions
  The members are usable where a constant expression is required.
  Tests the following:
  - min_depth instantiates a std::size_t-parameterised template
  - the instantiated constant equals min_depth
  - nests can drive a compile-time branch (std::conditional)
  - name() initializes a constexpr pointer
*/
bool
tests_tag_flat_members_are_constant_expressions()
{
    // min_depth as a template argument
    using probe = flat_depth_probe<flat::min_depth>;
    D_FL_CHECK(probe::value == flat::min_depth);
    D_FL_CHECK(probe::value == 1u);

    // nests in a compile-time branch: !nests selects the first type
    using branch = std::conditional<flat::nests, int, char>::type;
    D_FL_CHECK((std::is_same<branch, char>::value));

    // name() in a constexpr initializer
    constexpr const char* n = flat::name();
    D_FL_CHECK(n != nullptr);

    static_assert(flat_depth_probe<flat::min_depth>::value == 1u,
                  "min_depth is a constant expression");

    return true;
}

NS_END  // testing
NS_END  // djinterp
