/*******************************************************************************
* djinterp [testing]                                      hierarchical_tests.hpp
*
* Unit test suite for hierarchical.hpp -- the HIERARCHICAL structural-kind tag.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_HR_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       hierarchical_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  hierarchical.hpp defines exactly one type: the empty
* tag `hierarchical`, the structure-axis marker for a type that nests
* (depth >= 2, a node summand present).  Its entire observable surface is three
* compile-time constants/functions:
*     static constexpr bool        nests     = true;
*     static constexpr std::size_t min_depth = 2;
*     static constexpr const char* name() noexcept;   // "hierarchical"
* There is no runtime state and no algorithm -- the suite pins the three
* members' VALUES, their being genuine constant expressions, the emptiness of
* the tag, and that `name()` is a function.
*
*   WHY nests IS THE POINT.  The structure traits read a tag through its `nests`
* bit; for hierarchical that bit is true, asserting hierarchy that structural
* detection cannot always see (a composite whose recursion the value_type chain
* does not expose).  So `hierarchical::nests == true` and
* `hierarchical::min_depth == 2` are the load-bearing facts, pinned here as
* constants the traits consult.  This tag is the exact complement of `flat`;
* the two are the poles of the structure axis, and the suite pins the polarity
* against the literals the model fixes (nests true vs false, depth 2 vs 1).
*
*   BUILD PREREQUISITE: none.  hierarchical.hpp compiles as shipped (C++11
*   baseline).
*
* CONTENTS
*   I.    TAG   (hierarchical_tests_tag.cpp)
*
*
* path:      /tests/djinterp/core/meta/hierarchical_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_HIERARCHICAL_TESTS_
#define DJINTERP_TESTS_HIERARCHICAL_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>
// djinterp
// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/meta/hierarchical.hpp"                     // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// hierarchical_check
//   function: routes one D_HR_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>).
inline bool
hierarchical_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_HR_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  The `HR` suffix is
// this suite's unique two letters.
#define D_HR_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::hierarchical_check(                         \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   COMPILE-TIME PROBE                                    ///
///////////////////////////////////////////////////////////////////////////////

// hierarchical_depth_probe
//   trait: instantiable only for a genuine std::size_t constant expression, so
// forming hierarchical_depth_probe<hierarchical::min_depth> proves min_depth is usable as a
// template argument (i.e. truly constexpr).
template<std::size_t _Depth>
struct hierarchical_depth_probe
    : std::integral_constant<std::size_t, _Depth>
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   TAG   (hierarchical_tests_tag.cpp)
bool tests_tag_hierarchical_nests_is_true();
bool tests_tag_hierarchical_min_depth_is_two();
bool tests_tag_hierarchical_name_is_hierarchical();
bool tests_tag_hierarchical_is_empty_tag();
bool tests_tag_hierarchical_members_are_constant_expressions();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// hierarchical_spec
//   function: the authoritative description of this suite.
inline dt::module_spec
hierarchical_spec()
{
    return dt::module_spec{
        "hierarchical",
        "The HIERARCHICAL structural-kind tag: the opt-in marker for a type"
        "that nests (depth >= 2, a node summand present). Its whole "
        "surface is three compile-time members -- nests (true), min_depth (2), "
        "and a name() function returning \"hierarchical\". This suite pins those "
        "values, their being genuine constant expressions, and the emptiness of "
        "the tag. The nests bit is the load-bearing fact: the structure traits "
        "consult it to assert hierarchy that structural detection cannot see. "
        "The tag is the exact complement of flat.",
        {
            dt::block_spec{
                "tag",
                "The hierarchical tag's three members and its shape: nests is "
                "true, min_depth is 2, name() is \"hierarchical\", the tag is "
                "empty, and every member is a constant expression.",
                {
                    { "tests_tag_hierarchical_nests_is_true",
                      "hierarchical::nests is true -- a node summand is present, "
                      "the signal the structure traits read for hierarchy.",
                      &tests_tag_hierarchical_nests_is_true },
                    { "tests_tag_hierarchical_min_depth_is_two",
                      "hierarchical::min_depth is 2 -- some component is a node.",
                      &tests_tag_hierarchical_min_depth_is_two },
                    { "tests_tag_hierarchical_name_is_hierarchical",
                      "hierarchical::name() returns the stable spelling "
                      "\"hierarchical\".",
                      &tests_tag_hierarchical_name_is_hierarchical },
                    { "tests_tag_hierarchical_is_empty_tag",
                      "hierarchical is an empty, trivially constructible tag "
                      "type -- it carries no state, only its static members.",
                      &tests_tag_hierarchical_is_empty_tag },
                    { "tests_tag_hierarchical_members_are_constant_expressions",
                      "nests and min_depth are usable in constant expressions "
                      "(as template arguments and in static_assert), and name() "
                      "is a constexpr function.",
                      &tests_tag_hierarchical_members_are_constant_expressions },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_HIERARCHICAL_TESTS_
