/*******************************************************************************
* djinterp [testing]                                   metadata_traits_tests.hpp
*
*   Unit-test suite for the metadata protocol traits (util/metadata/
* metadata_traits.hpp).  Two-faced per the DTest authoring guide: the section
* TUs include it in normal mode for the fixtures and the check macro, and the
* runner includes it with DTEST_SPEC_MODE defined for the spec provider.
*
*   BUILD PREREQUISITE:
*   metadata_traits.hpp does not compile as shipped.  It calls
* D_TRAIT_IS_DETECTED and D_TRAIT_HAS_TYPE, but core/meta/trait_detect.hpp
* renamed that whole family to the D_TYPE_TRAIT_ prefix; its own comment on
* D_TYPE_TRAIT_TRUE records the former spelling.  Two substitutions fix it:
*     D_TRAIT_IS_DETECTED  ->  D_TYPE_TRAIT_TRUE
*     D_TRAIT_HAS_TYPE     ->  D_TYPE_TRAIT_HAS_TYPE
* With those applied the header builds clean under -Wall -Wextra and every
* test below passes.  Nothing else about the module needs changing.
*
*   WHAT THE MODULE CLAIMS, AND WHAT IS ACTUALLY TRUE:
*   Three canonical names - metadata, metadata_type, metadata_container_type
* - are each detected across three member shapes: data member, member
* function, nested type alias.  Two properties of that design carry most of
* the risk, and both are pinned below.
*
*   First, all six expression probes go through `const _Type&`.  A member
* function that is not const-qualified is therefore NOT detected, even though
* it exists and is spelled correctly.  md_nonconst_method is the fixture for
* that, and it is the difference between "the name is present" and "the name
* is reachable through a const handle", which is what the traits answer.
*
*   Second, the header states the three shapes are "mutually exclusive within
* a single class (the C++ class member name space forbids overlap)".  That
* reasoning holds for the name space but not for the expressions: a data
* member of CALLABLE type makes both `t.metadata` and `t.metadata()`
* well-formed, so two shapes report true at once.  md_callable_member is that
* case.  The extractors survive it because their dispatch is a four-way bool
* selection with a documented precedence rather than the either/or the
* comment implies - so the code is more careful than its own documentation,
* and the extraction block pins the precedence that saves it.
*
*   CROSS-NAME INDEPENDENCE:
*   The detection and mirrors blocks are near-identical in shape, which is
* exactly why both exist.  The three name families are separate copy-pasted
* blocks in the module, so the failure they are most exposed to is a probe
* that mentions the wrong name - has_metadata_type_method reaching for
* `.metadata()` rather than `.metadata_type()`.  Only a fixture carrying one
* name and not the others can catch that, so every fixture below is
* deliberately single-name, and md_all_names is the sole exception.
*
*   NOT TESTED:
*   The module advertises C++11 support, but clean_t (used by
* D_TYPE_TRAIT_HAS_TYPE and by every extractor) is spelled without
* `typename` in djinterp.hpp, which is C++20-only syntax; the header does not
* build under c++11, c++14 or c++17 at all.  Stating that as a test would
* require a build the project cannot perform, so it is recorded here.
*
*
* path:      /tests/djinterp/core/util/metadata/metadata_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    check plumbing        (D_MT_CHECK)
      ----------------------------------

II.   shape fixtures        (md_*, mdt_*, mdc_*)
      ------------------------------------------

III.  probe helpers         (mt_extracts_*)
      -------------------------------------

IV.   test declarations
      -----------------

V.    spec provider         (metadata_traits_spec)
      --------------------------------------------
*/

#ifndef DJINTERP_TESTS_METADATA_TRAITS_TESTS_
#define DJINTERP_TESTS_METADATA_TRAITS_TESTS_ 1

// std
#include <cstdio>
#include <type_traits>
// djinterp
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/metadata/metadata_traits.hpp"              // the module under test
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

// D_MT_CHECK
//   macro: states one claim; on failure names the expression and its
// location and returns false from the enclosing test.  Variadic so a
// template argument list may carry a top-level comma.  Self-contained, so it
// sits outside the fixture guard.
#define D_MT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("    check failed: %s\n    at: %s:%d\n",              \
                        #__VA_ARGS__, __FILE__, __LINE__);                    \
            return false;                                                     \
        }                                                                     \
    } while (0)


#ifndef DTEST_SPEC_MODE

// ===========================================================================
// II.  shape fixtures
// ===========================================================================
//   Every fixture carries exactly ONE canonical name in exactly one shape,
// so a trait that reached for the wrong name or the wrong shape has no
// fixture that would let it pass by accident.

// md_payload
//   type: the thing being carried.  Distinct from int so an extractor that
// returned some incidental type cannot match by coincidence.
struct md_payload
{
    int v = 0;
};

// md_holder
//   type: a second distinct type, for the container-type name, so
// metadata_type_t and metadata_container_type_t cannot be confused.
struct md_holder
{
    int n = 0;
};

// md_callable
//   type: callable with no arguments.  Used as a data member's type to make
// `t.metadata` and `t.metadata()` simultaneously well-formed.
struct md_callable
{
    md_payload operator()() const
    {
        return md_payload{};
    }
};

// --- the `metadata` name, one shape each ---------------------------------

// md_member
//   fixture: `metadata` as a plain data member.
struct md_member
{
    md_payload metadata;
};

// md_method
//   fixture: `metadata` as a const-qualified member function.
struct md_method
{
    md_payload metadata() const
    {
        return md_payload{};
    }
};

// md_nested
//   fixture: `metadata` as a nested type alias.
struct md_nested
{
    using metadata = md_payload;
};

// md_callable_member
//   fixture: `metadata` as a data member whose TYPE is callable, so both the
// data-member and the member-function probes succeed.  This is the
// counterexample to the module's mutual-exclusivity comment.
struct md_callable_member
{
    md_callable metadata;
};

// md_nonconst_method
//   fixture: `metadata()` present but NOT const-qualified.  Every probe goes
// through a const handle, so this is detected by nothing.
struct md_nonconst_method
{
    md_payload metadata()
    {
        return md_payload{};
    }
};

// md_ref_member
//   fixture: `metadata` as a const reference member, so the extractor has
// cv- and reference-qualifiers to strip.
struct md_ref_member
{
    const md_payload& metadata;
};

// --- the `metadata_type` name --------------------------------------------

// mdt_member / mdt_method / mdt_nested
//   fixtures: `metadata_type` in each of the three shapes.
struct mdt_member
{
    md_payload metadata_type;
};

struct mdt_method
{
    md_payload metadata_type() const
    {
        return md_payload{};
    }
};

struct mdt_nested
{
    using metadata_type = md_payload;
};

// --- the `metadata_container_type` name ----------------------------------

// mdc_member / mdc_method / mdc_nested
//   fixtures: `metadata_container_type` in each of the three shapes.
struct mdc_member
{
    md_holder metadata_container_type;
};

struct mdc_method
{
    md_holder metadata_container_type() const
    {
        return md_holder{};
    }
};

struct mdc_nested
{
    using metadata_container_type = md_holder;
};

// --- negative and combined fixtures --------------------------------------

// md_none
//   fixture: a class type carrying none of the three names.
struct md_none
{
    int x = 0;
};

// md_similar
//   fixture: names that are near-misses for the canonical ones.  Detection
// is by exact name, so none of these should register.
struct md_similar
{
    md_payload meta_data;
    md_payload metadata_;
    using Metadata = md_payload;
    using metadata_types = md_payload;
};

// md_all_names
//   fixture: a type written to the protocol properly - all three canonical
// names, in the shapes real code would use them.  The only fixture where
// more than one name is present.
struct md_all_names
{
    using metadata_type           = md_payload;
    using metadata_container_type = md_holder;
    md_payload metadata;
};


// ===========================================================================
// III. probe helpers
// ===========================================================================

// mt_extracts_metadata
//   trait: whether `metadata_t<_Type>` is a well-formed type.  The module
// documents extraction as ill-formed when no shape is present; this asks
// that question without tripping the error, so a test can state the negative
// case rather than only demonstrating the positive one.
template<typename _Type,
         typename = void>
struct mt_extracts_metadata : std::false_type
{};

template<typename _Type>
struct mt_extracts_metadata<_Type, D_VOID_T<metadata_t<_Type>>>
    : std::true_type
{};

// mt_extracts_metadata_type
//   trait: as above, for `metadata_type_t<_Type>`.
template<typename _Type,
         typename = void>
struct mt_extracts_metadata_type : std::false_type
{};

template<typename _Type>
struct mt_extracts_metadata_type<_Type, D_VOID_T<metadata_type_t<_Type>>>
    : std::true_type
{};

// mt_extracts_container_type
//   trait: as above, for `metadata_container_type_t<_Type>`.
template<typename _Type,
         typename = void>
struct mt_extracts_container_type : std::false_type
{};

template<typename _Type>
struct mt_extracts_container_type<_Type,
    D_VOID_T<metadata_container_type_t<_Type>>>
    : std::true_type
{};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// ===========================================================================
// IV.  test declarations
// ===========================================================================

// I.   detection    (metadata_traits_tests_detection.cpp)
bool tests_metadata_shapes();
bool tests_metadata_unifier();
bool tests_metadata_const_requirement();

// II.  mirrors      (metadata_traits_tests_mirrors.cpp)
bool tests_metadata_type_shapes();
bool tests_metadata_container_type_shapes();
bool tests_metadata_name_independence();

// III. extraction   (metadata_traits_tests_extraction.cpp)
bool tests_metadata_extraction();
bool tests_metadata_extraction_dispatch();
bool tests_metadata_extraction_absent();

// IV.  protocol     (metadata_traits_tests_protocol.cpp)
bool tests_metadata_hostile_types();
bool tests_metadata_value_companions();
bool tests_metadata_qualifier_transparency();


// ===========================================================================
// V.   spec provider
// ===========================================================================

#ifdef DTEST_SPEC_MODE

// metadata_traits_spec
//   function: the declarative description of this suite - one block per
// section TU, one row per test, each carrying the descriptor the report
// renders.
inline dt::module_spec
metadata_traits_spec()
{
    return dt::module_spec{
        "metadata_traits",
        "Structural classification of types by their exposure of three "
        "canonical metadata names - metadata, metadata_type and "
        "metadata_container_type - across three member shapes, with `_t` "
        "aliases extracting the type behind whichever shape is present.",
        {
            dt::block_spec{
                "detection",
                "The `metadata` name across all three member shapes, the "
                "unifying trait over them, and what the const-qualified "
                "probes actually require.",
                {
                    { "tests_metadata_shapes",
                      "Each shape is detected by its own trait and by "
                      "neither of the other two; near-miss names such as "
                      "meta_data and Metadata do not register.",
                      &tests_metadata_shapes },
                    { "tests_metadata_unifier",
                      "has_metadata is true for any single shape, false "
                      "when none is present, and equals the OR of its three "
                      "component traits on every fixture.",
                      &tests_metadata_unifier },
                    { "tests_metadata_const_requirement",
                      "A non-const member function is detected by nothing, "
                      "and a data member of callable type satisfies the "
                      "data-member and method shapes at once - refuting the "
                      "module's mutual-exclusivity comment.",
                      &tests_metadata_const_requirement },
                }
            },
            dt::block_spec{
                "mirrors",
                "The metadata_type and metadata_container_type families, "
                "which the module implements as separate copy-pasted trait "
                "blocks, and their independence from one another.",
                {
                    { "tests_metadata_type_shapes",
                      "The metadata_type family detects each shape "
                      "independently and unifies over them.",
                      &tests_metadata_type_shapes },
                    { "tests_metadata_container_type_shapes",
                      "The metadata_container_type family does likewise, "
                      "and yields the holder type rather than the payload "
                      "type.",
                      &tests_metadata_container_type_shapes },
                    { "tests_metadata_name_independence",
                      "A type carrying exactly one canonical name satisfies "
                      "that family alone - the check that catches a probe "
                      "reaching for a prefix-sharing sibling name.",
                      &tests_metadata_name_independence },
                }
            },
            dt::block_spec{
                "extraction",
                "The three `_t` aliases: what they yield through each shape, "
                "which shape wins when two compete, and how they fail when "
                "no shape is present.",
                {
                    { "tests_metadata_extraction",
                      "Each alias resolves through all three shapes to the "
                      "same answer and strips cv- and reference qualifiers "
                      "from the extracted type.",
                      &tests_metadata_extraction },
                    { "tests_metadata_extraction_dispatch",
                      "Where a callable data member satisfies two shapes, "
                      "the data member wins - extraction yields the member's "
                      "own type, not the return type of calling it.",
                      &tests_metadata_extraction_dispatch },
                    { "tests_metadata_extraction_absent",
                      "Extraction with no shape present is a substitution "
                      "failure rather than a hard error, so a caller can "
                      "detect it; stated through a probe rather than by "
                      "triggering the error.",
                      &tests_metadata_extraction_absent },
                }
            },
            dt::block_spec{
                "protocol",
                "Claims that span all three name families: behaviour on "
                "non-class types, the `_v` companions, and transparency "
                "through qualified forms.",
                {
                    { "tests_metadata_hostile_types",
                      "No member of the adversarial type zoo carries the "
                      "protocol, and every trait remains a well-formed bool "
                      "trait on all of them.",
                      &tests_metadata_hostile_types },
                    { "tests_metadata_value_companions",
                      "All twelve `_v` companions agree with their class "
                      "traits - the nine generated by the detection macro "
                      "and the three hand-written beside the unifiers.",
                      &tests_metadata_value_companions },
                    { "tests_metadata_qualifier_transparency",
                      "const, volatile and reference forms answer as the "
                      "underlying type does, by two different mechanisms - "
                      "const-reference binding and type cleaning - without "
                      "inventing a protocol that was absent.",
                      &tests_metadata_qualifier_transparency },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_METADATA_TRAITS_TESTS_
