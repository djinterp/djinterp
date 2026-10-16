/*******************************************************************************
* djinterp [core]                                     hierarchical_tests_tag.cpp
*
*
* path:      /tests/djinterp/core/meta/hierarchical_tests_tag.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "hierarchical_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_tag_hierarchical_nests_is_true
  hierarchical::nests is true.
  Tests the following:
  - the value is true
  - it is exactly true (a bool, compared as such)
  - it is of type bool
  - it is a constant expression (static_assert)
*/
bool
tests_tag_hierarchical_nests_is_true()
{
    D_HR_CHECK(hierarchical::nests == true);
    D_HR_CHECK(hierarchical::nests);
    D_HR_CHECK((std::is_same<decltype(hierarchical::nests), const bool>::value));

    static_assert(hierarchical::nests == true, "hierarchical nests");

    return true;
}

/*
tests_tag_hierarchical_min_depth_is_two
  hierarchical::min_depth is 2.
  Tests the following:
  - the value is 2
  - it is of type std::size_t
  - it is strictly greater than the flat floor (1), per the model
  - it is a constant expression
*/
bool
tests_tag_hierarchical_min_depth_is_two()
{
    D_HR_CHECK(hierarchical::min_depth == 2u);
    D_HR_CHECK((std::is_same<decltype(hierarchical::min_depth), const std::size_t>::value));
    D_HR_CHECK(hierarchical::min_depth > 1u);   // deeper than flat, per the model

    static_assert(hierarchical::min_depth == 2u, "hierarchical depth is 2");

    return true;
}

/*
tests_tag_hierarchical_name_is_hierarchical
  hierarchical::name() returns "hierarchical".
  Tests the following:
  - the returned string equals "hierarchical"
  - the return type is const char*
  - the function is callable in a constant expression (constexpr)
  - repeated calls yield the same stable spelling
*/
bool
tests_tag_hierarchical_name_is_hierarchical()
{
    D_HR_CHECK(std::strcmp(hierarchical::name(), "hierarchical") == 0);
    D_HR_CHECK((std::is_same<decltype(hierarchical::name()), const char*>::value));

    constexpr const char* n = hierarchical::name();
    D_HR_CHECK(n[0] == 'h');

    D_HR_CHECK(std::strcmp(hierarchical::name(), hierarchical::name()) == 0);

    return true;
}

/*
tests_tag_hierarchical_is_empty_tag
  hierarchical is an empty tag type.
  Tests the following:
  - it is a class type
  - it is empty (no non-static data members)
  - it is trivially default constructible
  - it is standard-layout
*/
bool
tests_tag_hierarchical_is_empty_tag()
{
    D_HR_CHECK(std::is_class<hierarchical>::value);
    D_HR_CHECK(std::is_empty<hierarchical>::value);
    D_HR_CHECK(std::is_trivially_default_constructible<hierarchical>::value);
    D_HR_CHECK(std::is_standard_layout<hierarchical>::value);

    return true;
}

/*
tests_tag_hierarchical_members_are_constant_expressions
  The members are usable where a constant expression is required.
  Tests the following:
  - min_depth instantiates a std::size_t-parameterised template
  - the instantiated constant equals min_depth
  - nests can drive a compile-time branch (std::conditional)
  - name() initializes a constexpr pointer
*/
bool
tests_tag_hierarchical_members_are_constant_expressions()
{
    // min_depth as a template argument
    using probe = hierarchical_depth_probe<hierarchical::min_depth>;
    D_HR_CHECK(probe::value == hierarchical::min_depth);
    D_HR_CHECK(probe::value == 2u);

    // nests in a compile-time branch: nests selects the first type
    using branch = std::conditional<hierarchical::nests, int, char>::type;
    D_HR_CHECK((std::is_same<branch, int>::value));

    // name() in a constexpr initializer
    constexpr const char* n = hierarchical::name();
    D_HR_CHECK(n != nullptr);

    static_assert(hierarchical_depth_probe<hierarchical::min_depth>::value == 2u,
                  "min_depth is a constant expression");

    return true;
}

NS_END  // testing
NS_END  // djinterp
