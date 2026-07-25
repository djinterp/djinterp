/******************************************************************************
* djinterp [testing]                                     passthrough_tests.hpp
*
* Unit test suite for passthrough.hpp -- the passthrough_marker opt-in base,
* the is_passthrough trait, and the C++20 Passthrough concept.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_PT_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       passthrough_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  passthrough.hpp is tiny and structural: an empty
* inheritable base passthrough_marker; a unary trait is_passthrough<T> defined
* as std::is_base_of<passthrough_marker, clean_t<T>>; its is_passthrough_v
* companion; and, under __cpp_concepts, the Passthrough concept aliasing the
* trait.  The opt-in is by INHERITANCE, and detection is by is_base_of, which
* gives the module three behaviours worth pinning precisely.
*
*   THE THREE PROPERTIES.
*     1. cv-ref stripping.  clean_t runs first, so is_passthrough<const D&>
*        agrees with is_passthrough<D>.
*     2. is_base_of semantics.  Detection inherits every quirk of
*        std::is_base_of: a class is its own base (is_passthrough<
*        passthrough_marker> is TRUE), inheritance is transitive (a grandchild
*        is a passthrough), and access does NOT matter (PRIVATE inheritance of
*        the marker still counts).  A pointer to a passthrough is not itself a
*        passthrough.
*     3. completeness.  std::is_base_of requires the derived type to be
*        complete, so is_passthrough<incomplete> is a HARD error -- the suite
*        therefore never instantiates it and documents the requirement rather
*        than probing it.  (Contrast the member_types detectors, which are a
*        soft false on incomplete types.)
*
*   THE FIXTURE VOCABULARY (normal mode only)
*     pt_direct       inherits passthrough_marker publicly (the ordinary case).
*     pt_child        a grandchild -- inherits pt_direct, so the marker is two
*                     levels up; pins transitivity.
*     pt_private      inherits passthrough_marker PRIVATELY; pins that is_base_of
*                     ignores access.
*     pt_multi        inherits the marker alongside an unrelated base; pins that
*                     a mixed base list still counts.
*     plain           inherits nothing; the negative control.
*     unrelated_base  a non-marker empty base, so "inherits SOMETHING" is not
*                     mistaken for "inherits the marker".
*
*   BUILD PREREQUISITE: none.  passthrough.hpp compiles as shipped.
*
* CONTENTS
*   I.    MARKER      (passthrough_tests_marker.cpp)
*   II.   SEMANTICS   (passthrough_tests_semantics.cpp)
*   III.  CONCEPT     (passthrough_tests_concept.cpp)
*
* path:      /tests/djinterp/core/meta/passthrough_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.25
******************************************************************************/

#ifndef DJINTERP_TESTS_PASSTHROUGH_TESTS_
#define DJINTERP_TESTS_PASSTHROUGH_TESTS_ 1

// std
#include <cstdio>
#include <string>
#include <type_traits>

// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "passthrough.hpp"                      // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// passthrough_check
//   function: routes one D_PT_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>).
inline bool
passthrough_check(
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

// D_PT_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  The `PT` suffix is
// this suite's unique two letters.
#define D_PT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::passthrough_check(                          \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   PASSTHROUGH FIXTURES                                  ///
///////////////////////////////////////////////////////////////////////////////

// pt_direct
//   struct: the ordinary case -- inherits passthrough_marker publicly.
struct pt_direct
    : passthrough_marker
{};

// pt_child
//   struct: a grandchild of the marker (marker is two levels up), pinning that
// detection is transitive.
struct pt_child
    : pt_direct
{};

// unrelated_base
//   struct: a non-marker empty base, so "inherits something" cannot be confused
// with "inherits the marker".
struct unrelated_base
{};

// pt_private
//   struct: inherits the marker PRIVATELY, pinning that is_base_of ignores
// access control.
struct pt_private
    : private passthrough_marker
{};

// pt_multi
//   struct: inherits the marker alongside an unrelated base, pinning that a
// mixed base list still counts as a passthrough.
struct pt_multi
    : unrelated_base,
      passthrough_marker
{};

// plain
//   struct: inherits nothing -- the negative control.
struct plain
{};

// derives_unrelated
//   struct: inherits only the non-marker base -- inherits SOMETHING, but not the
// marker, so must not be detected.
struct derives_unrelated
    : unrelated_base
{};


///////////////////////////////////////////////////////////////////////////////
///             F.2   OVERLOAD-RESOLUTION PROBES (C++20)                    ///
///////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_concepts)

    // taken_by
    //   function: constrained overload -- selected for passthroughs, being the
    // more constrained of the pair.
    template<Passthrough _Type>
    inline const char*
    taken_by(
        const _Type&
    )
    {
        return "passthrough";
    }

    // taken_by (unconstrained)
    //   function: the fallback, selected for everything else.
    template<typename _Type>
    inline const char*
    taken_by(
        const _Type&
    )
    {
        return "generic";
    }

#endif  // __cpp_concepts

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   MARKER   (passthrough_tests_marker.cpp)
bool tests_marker_is_empty_and_inheritable();
bool tests_marker_direct_inheritor_is_passthrough();
bool tests_marker_non_inheritor_is_not_passthrough();
bool tests_marker_fundamentals_and_void_are_not_passthrough();
bool tests_marker_variable_companion_agrees();

// II.  SEMANTICS   (passthrough_tests_semantics.cpp)
bool tests_semantics_strip_cv_ref();
bool tests_semantics_inheritance_is_transitive();
bool tests_semantics_private_inheritance_still_counts();
bool tests_semantics_multiple_inheritance_counts();
bool tests_semantics_marker_is_its_own_base();
bool tests_semantics_pointer_to_passthrough_is_not();
bool tests_semantics_trait_shape_is_integral_constant();

// III. CONCEPT   (passthrough_tests_concept.cpp)
#if defined(__cpp_concepts)
bool tests_concept_accepts_passthroughs();
bool tests_concept_rejects_non_passthroughs();
bool tests_concept_agrees_with_trait();
bool tests_concept_constrains_overload_resolution();
#endif


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// passthrough_spec
//   function: the authoritative description of this suite -- one block per
// section TU, one row per tests_* body, every node named and described.
inline dt::module_spec
passthrough_spec()
{
    return dt::module_spec{
        "passthrough",
        "The passthrough opt-in: an empty inheritable base passthrough_marker, "
        "the unary trait is_passthrough<T> = std::is_base_of<passthrough_marker, "
        "clean_t<T>>, its is_passthrough_v companion, and the C++20 Passthrough "
        "concept. The opt-in is by inheritance and detection is by is_base_of, "
        "so this suite pins the marker's shape, direct and inherited detection, "
        "the cv-ref stripping clean_t confers, and the is_base_of quirks the "
        "trait inherits -- transitivity, access-insensitivity (private "
        "inheritance still counts), a class being its own base, and pointer "
        "opacity -- plus the concept's agreement with the trait. is_base_of "
        "requires a complete derived type, so incomplete types are documented "
        "as a hard-error precondition rather than probed.",
        {
            dt::block_spec{
                "marker",
                "passthrough_marker and the core of is_passthrough: the marker "
                "is an empty, inheritable base; a direct public inheritor is a "
                "passthrough; a non-inheritor, a fundamental, and void are not; "
                "and the _v companion agrees.",
                {
                    { "tests_marker_is_empty_and_inheritable",
                      "passthrough_marker is an empty, standard-layout class that "
                      "can serve as a base, and inheriting it is what "
                      "is_passthrough keys on.",
                      &tests_marker_is_empty_and_inheritable },
                    { "tests_marker_direct_inheritor_is_passthrough",
                      "A struct that inherits passthrough_marker publicly is "
                      "detected as a passthrough.",
                      &tests_marker_direct_inheritor_is_passthrough },
                    { "tests_marker_non_inheritor_is_not_passthrough",
                      "A struct that inherits nothing, and one that inherits an "
                      "unrelated base, are both not passthroughs -- inheriting "
                      "SOMETHING is not inheriting the marker.",
                      &tests_marker_non_inheritor_is_not_passthrough },
                    { "tests_marker_fundamentals_and_void_are_not_passthrough",
                      "Fundamental types, void, and a pointer type are not "
                      "passthroughs.",
                      &tests_marker_fundamentals_and_void_are_not_passthrough },
                    { "tests_marker_variable_companion_agrees",
                      "is_passthrough_v<T> equals is_passthrough<T>::value across "
                      "positive and negative cases and is a constant expression.",
                      &tests_marker_variable_companion_agrees },
                }
            },
            dt::block_spec{
                "semantics",
                "The behaviour is_passthrough inherits from clean_t and "
                "std::is_base_of: cv-ref stripping, transitive inheritance, "
                "access-insensitivity, multiple inheritance, a class as its own "
                "base, pointer opacity, and the trait's integral_constant shape.",
                {
                    { "tests_semantics_strip_cv_ref",
                      "is_passthrough<const D&>, <volatile D>, <D&&> all agree "
                      "with is_passthrough<D>, in both directions, because "
                      "clean_t strips cv-ref before is_base_of.",
                      &tests_semantics_strip_cv_ref },
                    { "tests_semantics_inheritance_is_transitive",
                      "A grandchild of the marker (inheriting a class that "
                      "inherits the marker) is a passthrough.",
                      &tests_semantics_inheritance_is_transitive },
                    { "tests_semantics_private_inheritance_still_counts",
                      "Private inheritance of the marker still makes a type a "
                      "passthrough: std::is_base_of ignores access.",
                      &tests_semantics_private_inheritance_still_counts },
                    { "tests_semantics_multiple_inheritance_counts",
                      "A type inheriting the marker alongside an unrelated base "
                      "is a passthrough; one inheriting only the unrelated base "
                      "is not.",
                      &tests_semantics_multiple_inheritance_counts },
                    { "tests_semantics_marker_is_its_own_base",
                      "passthrough_marker itself is a passthrough, because "
                      "std::is_base_of reports a class as its own base.",
                      &tests_semantics_marker_is_its_own_base },
                    { "tests_semantics_pointer_to_passthrough_is_not",
                      "A pointer to a passthrough is not itself a passthrough, "
                      "and clean_t does not dereference it.",
                      &tests_semantics_pointer_to_passthrough_is_not },
                    { "tests_semantics_trait_shape_is_integral_constant",
                      "is_passthrough derives from std::true_type / "
                      "std::false_type with value_type bool, the matching ::type, "
                      "and a constant-expression ::value.",
                      &tests_semantics_trait_shape_is_integral_constant },
                }
            },
#if defined(__cpp_concepts)
            dt::block_spec{
                "concept",
                "The C++20 Passthrough concept: the capital-letter face of "
                "is_passthrough_v, its agreement with the trait, and its "
                "behaviour as a constraint in overload resolution. [C++20]",
                {
                    { "tests_concept_accepts_passthroughs",
                      "Passthrough is satisfied by direct, inherited, private, "
                      "and multiple-inheritance passthroughs, and by the marker "
                      "itself.",
                      &tests_concept_accepts_passthroughs },
                    { "tests_concept_rejects_non_passthroughs",
                      "Passthrough is not satisfied by a plain struct, an "
                      "unrelated-base struct, a fundamental, or void.",
                      &tests_concept_rejects_non_passthroughs },
                    { "tests_concept_agrees_with_trait",
                      "Passthrough<T> equals is_passthrough<T>::value term by "
                      "term over the battery, including cv-ref spellings.",
                      &tests_concept_agrees_with_trait },
                    { "tests_concept_constrains_overload_resolution",
                      "A Passthrough-constrained overload is chosen for "
                      "passthroughs and an unconstrained fallback for everything "
                      "else.",
                      &tests_concept_constrains_overload_resolution },
                }
            },
#endif  // __cpp_concepts
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_PASSTHROUGH_TESTS_
