/*******************************************************************************
* djinterp [testing]                                 metadata_concepts_tests.hpp
*
*   Unit-test suite for the C++20 concept companions to the metadata protocol
* (util/metadata/metadata_concepts.hpp).  Two-faced per the DTest authoring
* guide: the section TUs include it in normal mode for the fixtures and the
* check macros, the runner with DTEST_SPEC_MODE defined for the provider.
*
*   BUILD PREREQUISITE:
*   metadata_concepts.hpp includes metadata_traits.hpp, which does not
* compile as shipped - it calls D_TRAIT_IS_DETECTED and D_TRAIT_HAS_TYPE,
* which core/meta/trait_detect.hpp renamed to the D_TYPE_TRAIT_ prefix.
* Substituting D_TYPE_TRAIT_TRUE and D_TYPE_TRAIT_HAS_TYPE respectively fixes
* both headers; see metadata_traits_tests.hpp for the detail.
*
*   CORRECTNESS PREREQUISITE:
*   The three nested-alias concepts must clean their argument, as their trait
* counterparts already do:
*     concept has_metadata_nested_type =
*         requires { typename clean_t<_Type>::metadata; };
* and likewise for metadata_type and metadata_container_type.  As shipped
* they name into `_Type` directly.  Cv-qualifiers are ignored inside a
* nested-name-specifier, so const and volatile forms behave correctly by
* accident, but a REFERENCE type cannot be named through at all - so
* has_metadata_nested_type<T&> is false where the trait of the same name is
* true.  The expression-based shapes are unaffected, since both
* implementations bind `const _Type&` and reference collapsing absorbs the
* qualifier for them; the divergence is confined to the alias shape.
*
*   This is reachable rather than theoretical: a forwarding-reference
* template deduces _Type as T& for any lvalue argument, so a function
* constrained on has_metadata would silently reject lvalues of a type that
* declares its protocol as a nested alias, while the trait-based spelling of
* the same constraint accepted them.  The agreement block's qualified test
* fails on four checks until the fix is applied.
*
*   THE CENTRAL CLAIM:
*   This module is a restatement of metadata_traits.hpp in concept form.  Its
* twelve concepts carry exactly the twelve trait names, differing only by
* namespace - which is why the concepts live in `djinterp::concepts`, since a
* concept and a class template of one name cannot share a scope.  Two
* parallel implementations of one predicate is precisely the shape that
* drifts, so the suite's centre of gravity is the agreement block: each
* concept is required to agree with the trait of the same name, on every
* fixture, on the hostile type zoo, and on qualified forms.  D_MC_AGREES
* states one such pair, taking the shared name once and reading it out of
* both namespaces, so a check cannot accidentally compare a name to itself.
*
*   WHY THE FIXTURES ARE NOT SHARED WITH THE TRAITS SUITE:
*   They are deliberately re-declared here rather than included from
* metadata_traits_tests.hpp.  The two suites build as separate executables,
* and a shared fixture would let an edit made for one silently change what
* the other asserts - which matters more than usual here, because the whole
* point of the agreement block is that these are two independent
* implementations being compared.
*
*   WHAT CONCEPTS ADD OVER TRAITS:
*   Agreement alone would be satisfied by a concept that merely wrapped
* `trait<T>::value`, and the module does not do that - each concept is an
* independent requires-expression.  What a concept can do that a bool cannot
* is participate in SUBSUMPTION, so the constraints block checks that the
* atomic concepts subsume the disjunction that unifies them: given overloads
* constrained on has_metadata_data_member and on has_metadata, a type
* satisfying the former selects it without ambiguity.  That is the property a
* `bool` non-type parameter could not provide, and it is the justification
* for the module existing alongside the traits.
*
*   NOT TESTED:
*   The module is empty below C++20, guarded on
* D_ENV_CPP_FEATURE_LANG_CONCEPTS.  The suite asserts the guard is open
* rather than testing the closed case, since a build with it closed has
* nothing to test and the project compiles tests at C++20 regardless.
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_concepts_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    check plumbing        (D_MC_CHECK, D_MC_AGREES)
      -----------------------------------------------

II.   shape fixtures        (mc_*)
      ----------------------------

III.  constrained vehicles  (mc_pick, mc_accepts)
      -------------------------------------------

IV.   test declarations
      -----------------

V.    spec provider         (metadata_concepts_spec)
      ----------------------------------------------
*/

#ifndef DJINTERP_TESTS_METADATA_CONCEPTS_TESTS_
#define DJINTERP_TESTS_METADATA_CONCEPTS_TESTS_ 1

// std
#include <cstdio>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/metadata/metadata_concepts.hpp"            // the module under test
#include "../../../../../inc/djinterp/core/util/metadata/metadata_traits.hpp"              // the traits it must agree with
#include "djinterp/test/test_traits.hpp"    // hostile type zoo, quantifiers
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"  // module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt
//   alias: the framework namespace.  Unconditional - the spec provider needs
// dt::module_spec in spec mode, and the fixtures need nothing from it.
namespace dt = ::djinterp::test;


// ===========================================================================
// I.   check plumbing
// ===========================================================================

// D_MC_CHECK
//   macro: states one claim; on failure names the expression and its
// location and returns false from the enclosing test.  Variadic so a
// template argument list may carry a top-level comma.
#define D_MC_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("    check failed: %s\n    at: %s:%d\n",              \
                        #__VA_ARGS__, __FILE__, __LINE__);                    \
            return false;                                                     \
        }                                                                     \
    } while (0)

// D_MC_AGREES
//   macro: states that the concept and the trait of the SAME name give the
// same answer for _Type.  The name is written once and read out of both
// namespaces, so the two sides cannot drift apart in the test itself the way
// they could if each were spelled separately.
#define D_MC_AGREES(_name, _Type)                                             \
    do                                                                        \
    {                                                                         \
        if ((::djinterp::concepts::_name<_Type>)                              \
                != (::djinterp::_name<_Type>::value))                         \
        {                                                                     \
            std::printf("    check failed: concepts::%s<%s>"                  \
                        " == %s<%s>::value\n    at: %s:%d\n",                 \
                        #_name, #_Type, #_name, #_Type,                       \
                        __FILE__, __LINE__);                                  \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE

static_assert(D_ENV_CPP_FEATURE_LANG_CONCEPTS,
    "metadata_concepts_tests requires language concept support; the module "
    "under test compiles to nothing without it");


// ===========================================================================
// II.  shape fixtures
// ===========================================================================
//   Re-declared rather than shared with the traits suite; see the header
// note above.  Each carries exactly one canonical name in one shape.

// mc_payload / mc_holder
//   types: two distinct carried types, so an extractor or probe that crossed
// the metadata_type and metadata_container_type families would yield a
// visibly wrong answer rather than a coincidentally right one.
struct mc_payload
{
    int v = 0;
};

struct mc_holder
{
    int n = 0;
};

// mc_callable
//   type: callable with no arguments, used as a data member's type so that
// `t.metadata` and `t.metadata()` are both well-formed.
struct mc_callable
{
    mc_payload operator()() const
    {
        return mc_payload{};
    }
};

// --- the `metadata` name --------------------------------------------------

// mc_member / mc_method / mc_nested
//   fixtures: `metadata` as a data member, a const member function, and a
// nested type alias respectively.
struct mc_member
{
    mc_payload metadata;
};

struct mc_method
{
    mc_payload metadata() const
    {
        return mc_payload{};
    }
};

struct mc_nested
{
    using metadata = mc_payload;
};

// mc_callable_member
//   fixture: satisfies the data-member and member-function shapes at once.
struct mc_callable_member
{
    mc_callable metadata;
};

// mc_nonconst_method
//   fixture: `metadata()` present but not const-qualified, so the
// requires-expressions - which all bind `const _Type&` - reject it.
struct mc_nonconst_method
{
    mc_payload metadata()
    {
        return mc_payload{};
    }
};

// --- the mirrored names ---------------------------------------------------

// mct_member / mct_method / mct_nested
//   fixtures: `metadata_type` in each of the three shapes.
struct mct_member
{
    mc_payload metadata_type;
};

struct mct_method
{
    mc_payload metadata_type() const
    {
        return mc_payload{};
    }
};

struct mct_nested
{
    using metadata_type = mc_payload;
};

// mcc_member / mcc_method / mcc_nested
//   fixtures: `metadata_container_type` in each of the three shapes.
struct mcc_member
{
    mc_holder metadata_container_type;
};

struct mcc_method
{
    mc_holder metadata_container_type() const
    {
        return mc_holder{};
    }
};

struct mcc_nested
{
    using metadata_container_type = mc_holder;
};

// --- negative and combined fixtures --------------------------------------

// mc_none
//   fixture: a class type carrying none of the three names.
struct mc_none
{
    int x = 0;
};

// mc_all_names
//   fixture: a protocol-conforming type carrying all three names.
struct mc_all_names
{
    using metadata_type           = mc_payload;
    using metadata_container_type = mc_holder;
    mc_payload metadata;
};


// ===========================================================================
// III. constrained vehicles
// ===========================================================================
//   Concepts are for constraining, and a concept that only ever appears
// inside a bool context has not been exercised as a concept at all.  These
// vehicles put them where they belong: on overloads, where the compiler must
// not merely evaluate them but ORDER them.

// mc_pick
//   function: two overloads, one constrained on the atomic data-member
// concept and one on the disjunction that unifies the three shapes.  A type
// satisfying the atomic one must select it unambiguously, which happens only
// if the atomic concept subsumes the disjunction; without subsumption both
// candidates are viable and the call is ambiguous, so this failing to
// compile IS the negative result.
template<::djinterp::concepts::has_metadata _Type>
constexpr int
mc_pick(const _Type&)
{
    return 1;
}

template<::djinterp::concepts::has_metadata_data_member _Type>
constexpr int
mc_pick(const _Type&)
{
    return 2;
}

// mc_accepts
//   function: constrained on the unifying concept alone, so a fixture can be
// checked for admissibility without the ordering question above.
template<::djinterp::concepts::has_metadata _Type>
constexpr int
mc_accepts(const _Type&)
{
    return 7;
}

// mc_is_admissible
//   trait: whether mc_accepts may be called with a _Type.  States the
// negative case - that a non-conforming type is REJECTED - as an assertion
// rather than as a compile error nobody can observe.
template<typename _Type,
         typename = void>
struct mc_is_admissible : std::false_type
{};

template<typename _Type>
struct mc_is_admissible<_Type,
    D_VOID_T<decltype(mc_accepts(std::declval<const _Type&>()))>>
    : std::true_type
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// ===========================================================================
// IV.  test declarations
// ===========================================================================

// I.   detection    (metadata_concepts_tests_detection.cpp)
bool tests_metadata_concepts_shapes();
bool tests_metadata_concepts_mirrors();
bool tests_metadata_concepts_edge_shapes();

// II.  agreement    (metadata_concepts_tests_agreement.cpp)
bool tests_metadata_concepts_agreement();
bool tests_metadata_concepts_agreement_hostile();
bool tests_metadata_concepts_agreement_qualified();

// III. constraints  (metadata_concepts_tests_constraints.cpp)
bool tests_metadata_concepts_constrained_use();
bool tests_metadata_concepts_subsumption();
bool tests_metadata_concepts_reexports();


// ===========================================================================
// V.   spec provider
// ===========================================================================

#ifdef DTEST_SPEC_MODE

// metadata_concepts_spec
//   function: the declarative description of this suite - one block per
// section TU, one row per test, each carrying the descriptor the report
// renders.
inline dt::module_spec
metadata_concepts_spec()
{
    return dt::module_spec{
        "metadata_concepts",
        "C++20 concept restatement of the metadata protocol traits: twelve "
        "concepts carrying the twelve trait names in the djinterp::concepts "
        "sub-namespace, each an independent requires-expression rather than "
        "a wrapper over the trait, plus re-exports of the `_t` aliases.",
        {
            dt::block_spec{
                "detection",
                "The twelve concepts in their own right - each shape "
                "detected independently, the mirrored name families, and "
                "the two cases where presence and satisfaction diverge.",
                {
                    { "tests_metadata_concepts_shapes",
                      "The `metadata` concepts detect a data member, a const "
                      "member function and a nested alias independently, and "
                      "the unifier holds for any one of them.",
                      &tests_metadata_concepts_shapes },
                    { "tests_metadata_concepts_mirrors",
                      "The metadata_type and metadata_container_type "
                      "families behave alike and stay independent of one "
                      "another despite sharing a name prefix.",
                      &tests_metadata_concepts_mirrors },
                    { "tests_metadata_concepts_edge_shapes",
                      "A non-const member function satisfies nothing, and a "
                      "callable data member satisfies two shapes at once - "
                      "checked in concept form because the concepts are a "
                      "separate implementation from the traits.",
                      &tests_metadata_concepts_edge_shapes },
                }
            },
            dt::block_spec{
                "agreement",
                "The central claim: every concept answers exactly as the "
                "trait of the same name, across ordinary fixtures, "
                "non-class types, and cv- and reference-qualified forms.",
                {
                    { "tests_metadata_concepts_agreement",
                      "All twelve concept/trait pairs agree on both "
                      "answers, over fixtures that satisfy them and "
                      "fixtures that do not, including the edge cases.",
                      &tests_metadata_concepts_agreement },
                    { "tests_metadata_concepts_agreement_hostile",
                      "Both implementations reject void, function types, "
                      "arrays, references, pointers and nullptr_t "
                      "identically, despite failing for different "
                      "underlying reasons.",
                      &tests_metadata_concepts_agreement_hostile },
                    { "tests_metadata_concepts_agreement_qualified",
                      "Agreement holds through const, volatile and "
                      "reference forms - the block that catches the "
                      "nested-alias concepts failing to clean their "
                      "argument.",
                      &tests_metadata_concepts_agreement_qualified },
                }
            },
            dt::block_spec{
                "constraints",
                "What a concept provides that a bool cannot: use on a "
                "signature, the subsumption partial order, and the type "
                "extraction re-exported into the concepts namespace.",
                {
                    { "tests_metadata_concepts_constrained_use",
                      "A constrained function template admits every "
                      "conforming shape and genuinely rejects "
                      "non-conforming types, the rejection stated through a "
                      "detection probe.",
                      &tests_metadata_concepts_constrained_use },
                    { "tests_metadata_concepts_subsumption",
                      "The atomic concepts subsume the disjunction that "
                      "unifies them, so overload resolution between the two "
                      "is ordered rather than ambiguous.",
                      &tests_metadata_concepts_subsumption },
                    { "tests_metadata_concepts_reexports",
                      "concepts::metadata_t and djinterp::metadata_t name "
                      "the same alias, and all three re-exports still "
                      "resolve through every shape.",
                      &tests_metadata_concepts_reexports },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_METADATA_CONCEPTS_TESTS_
