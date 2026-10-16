/*******************************************************************************
* djinterp [testing]                                              flat_tests.hpp
*
* Unit test suite for flat.hpp -- the FLAT structural-kind tag.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_FL_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       flat_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  flat.hpp defines exactly one type: the empty tag
* `flat`, the structure-axis marker for a type that holds only leaves
* (depth 1, no node summand).  Its entire observable surface is three
* compile-time constants/functions:
*     static constexpr bool        nests     = false;
*     static constexpr std::size_t min_depth = 1;
*     static constexpr const char* name() noexcept;   // "flat"
* There is no runtime state and no algorithm -- the suite pins the three
* members' VALUES, their being genuine constant expressions (usable as template
* arguments and in static_assert), the emptiness of the tag, and the fact that
* `name()` is a function (so taking the tag never needs an out-of-line
* definition).
*
*   WHY nests IS THE POINT.  The structure traits read a tag through its `nests`
* bit; for flat that bit is false, asserting leaf-ness where only a weak
* heuristic (a value_type) would otherwise read as nested.  So
* `flat::nests == false` and `flat::min_depth == 1` are the load-bearing facts,
* pinned here as constants the traits can consult.
*
*   BUILD PREREQUISITE: none.  flat.hpp compiles as shipped (C++11 baseline).
*
* CONTENTS
*   I.    TAG   (flat_tests_tag.cpp)
*
*
* path:      /tests/djinterp/core/meta/flat_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_FLAT_TESTS_
#define DJINTERP_TESTS_FLAT_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>
// djinterp
// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/meta/flat.hpp"                             // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// flat_check
//   function: routes one D_FL_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>).
inline bool
flat_check(
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

// D_FL_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  The `FL` suffix is
// this suite's unique two letters.
#define D_FL_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::flat_check(                                 \
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

// flat_depth_probe
//   trait: instantiable only for a genuine std::size_t constant expression, so
// forming flat_depth_probe<flat::min_depth> proves min_depth is usable as a template
// argument (i.e. truly constexpr).
template<std::size_t _Depth>
struct flat_depth_probe
    : std::integral_constant<std::size_t, _Depth>
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   TAG   (flat_tests_tag.cpp)
bool tests_tag_flat_nests_is_false();
bool tests_tag_flat_min_depth_is_one();
bool tests_tag_flat_name_is_flat();
bool tests_tag_flat_is_empty_tag();
bool tests_tag_flat_members_are_constant_expressions();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// flat_spec
//   function: the authoritative description of this suite.
inline dt::module_spec
flat_spec()
{
    return dt::module_spec{
        "flat",
        "The FLAT structural-kind tag: the opt-in marker for a type that "
        "holds only leaves (depth 1, no node summand). Its whole surface is "
        "three compile-time members -- nests (false), min_depth (1), and a "
        "name() function returning \"flat\". This suite pins those values, their "
        "being genuine constant expressions usable as template arguments, and "
        "the emptiness of the tag. The nests bit is the load-bearing fact: the "
        "structure traits consult it to assert leaf-ness against a weak "
        "value_type heuristic.",
        {
            dt::block_spec{
                "tag",
                "The flat tag's three members and its shape: nests is false, "
                "min_depth is 1, name() is \"flat\", the tag is empty, and every "
                "member is a constant expression.",
                {
                    { "tests_tag_flat_nests_is_false",
                      "flat::nests is false -- flat has no node summand, the "
                      "signal the structure traits read for leaf-ness.",
                      &tests_tag_flat_nests_is_false },
                    { "tests_tag_flat_min_depth_is_one",
                      "flat::min_depth is 1 -- every component is a leaf.",
                      &tests_tag_flat_min_depth_is_one },
                    { "tests_tag_flat_name_is_flat",
                      "flat::name() returns the stable spelling \"flat\".",
                      &tests_tag_flat_name_is_flat },
                    { "tests_tag_flat_is_empty_tag",
                      "flat is an empty, trivially constructible tag type -- it "
                      "carries no state, only its static members.",
                      &tests_tag_flat_is_empty_tag },
                    { "tests_tag_flat_members_are_constant_expressions",
                      "nests and min_depth are usable in constant expressions "
                      "(as template arguments and in static_assert), and name() "
                      "is a constexpr function.",
                      &tests_tag_flat_members_are_constant_expressions },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_FLAT_TESTS_
