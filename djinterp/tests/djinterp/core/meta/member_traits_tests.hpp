/******************************************************************************
* djinterp [testing]                                   member_traits_tests.hpp
*
* Unit test suite for member_traits.hpp -- the DEPRECATED transitional shim.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_DT_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       member_traits_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  member_traits.hpp is a deprecated shim whose ENTIRE
* content is two macro definitions that forward the old spellings to the
* canonical macros in trait_detect.hpp:
*
*     D_DEFINE_HAS_MEMBER_TYPE(NAME)   ->  D_TYPE_TRAIT_HAS_TYPE(has_##NAME, NAME)
*     D_DEFINE_MEMBER_TYPE_OR(T,M,F)   ->  D_TYPE_TRAIT_MEMBER_TYPE_OR(T,M,F)
*
* plus the transitive guarantee that including the shim makes the new headers'
* concrete detectors (has_value_type, ...) and the pick_member_type helper
* visible, since the shim includes member_types.hpp.  There is no runtime code
* and no data structure here -- the only observable behaviour is what the two
* forwarded macros EXPAND TO.  So the suite's job is to INVOKE each forwarded
* spelling and prove the emitted trait behaves exactly like the canonical macro
* it forwards to.
*
*   WHY THE FIXTURES ARE MACRO INVOCATIONS.  A trait-emitting macro must be
* expanded at namespace scope; it cannot be exercised from inside a function
* body.  So this header invokes each forwarded spelling ONCE in the fixture
* region, at djinterp scope (the extractor macro opens an adjacent `internal`
* namespace and both mention clean_t, so djinterp scope is required), and the
* section TU then asserts against the traits those invocations produced:
*
*     shim_has_flavor_type    from D_DEFINE_HAS_MEMBER_TYPE(flavor_type)
*     shim_extract_flavor     from D_DEFINE_MEMBER_TYPE_OR(shim_extract_flavor,
*                                                          flavor_type, void)
*   For a differential check the header ALSO invokes the CANONICAL macros to
* build twin traits, and the tests prove the shim-built and canonical-built
* traits agree pointwise:
*     canon_has_flavor_type   from D_TYPE_TRAIT_HAS_TYPE(canon_has_flavor_type,
*                                                        flavor_type)
*     canon_extract_flavor    from D_TYPE_TRAIT_MEMBER_TYPE_OR(...)
*
*   THE NAME-DERIVATION GUARANTEE.  The old D_DEFINE_HAS_MEMBER_TYPE(NAME)
* auto-derived its trait name as `has_<NAME>`; the shim preserves that.  So the
* invocation D_DEFINE_HAS_MEMBER_TYPE(flavor_type) must produce a trait spelled
* exactly `has_flavor_type` -- the test names that identifier directly, which
* only compiles if the derivation is right.
*
*   BUILD PREREQUISITE: none.  member_traits.hpp compiles as shipped (it is
*   deprecated but still supported).
*
* CONTENTS
*   I.    FORWARDING   (member_traits_tests_forwarding.cpp)
*
* path:      /tests/djinterp/core/meta/member_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.25
******************************************************************************/

#ifndef DJINTERP_TESTS_MEMBER_TRAITS_TESTS_
#define DJINTERP_TESTS_MEMBER_TRAITS_TESTS_ 1

// std
#include <cstdio>
#include <type_traits>

// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "member_traits.hpp"                    // the deprecated shim under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// member_traits_check
//   function: routes one D_DT_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>).
inline bool
member_traits_check(
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

// D_DT_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  The `DT` suffix
// (Deprecated shim / member_Traits) is this suite's unique two letters.
#define D_DT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::member_traits_check(                        \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   PROBE TYPES                                           ///
///////////////////////////////////////////////////////////////////////////////

// has_flavor
//   struct: exposes a nested `flavor_type` typedef (the member the invoked
// detectors look for) aliased to a distinctive type.
struct has_flavor
{
    using flavor_type = long;
};

// no_flavor
//   struct: exposes no such typedef.
struct no_flavor
{};

// flavor_value_shaped
//   struct: a static DATA member named flavor_type -- a value, not a type, so
// the (typedef) detector must reject it.
struct flavor_value_shaped
{
    static constexpr int flavor_type = 0;
};


///////////////////////////////////////////////////////////////////////////////
///             F.2   TRAITS BUILT VIA THE DEPRECATED SPELLINGS             ///
///////////////////////////////////////////////////////////////////////////////
//   These invocations are the crux of the suite: they exercise the shim's two
// forwarded macro spellings.  If the forwards were broken, this region would
// not compile.

// has_flavor_type
//   trait: emitted by the OLD spelling D_DEFINE_HAS_MEMBER_TYPE(flavor_type).
// The name is auto-derived as has_<NAME>, so this invocation must yield exactly
// `has_flavor_type`.
D_DEFINE_HAS_MEMBER_TYPE(flavor_type)

// shim_extract_flavor
//   trait: emitted by the OLD spelling D_DEFINE_MEMBER_TYPE_OR, extracting
// ::flavor_type or falling back to void.
D_DEFINE_MEMBER_TYPE_OR(shim_extract_flavor, flavor_type, void)


///////////////////////////////////////////////////////////////////////////////
///             F.3   TWIN TRAITS BUILT VIA THE CANONICAL MACROS            ///
///////////////////////////////////////////////////////////////////////////////
//   The differential baseline: the same two traits built through the canonical
// macros the shim forwards to.  The tests prove the shim-built traits agree
// with these pointwise.

// canon_has_flavor_type
//   trait: emitted directly by the canonical D_TYPE_TRAIT_HAS_TYPE.
D_TYPE_TRAIT_HAS_TYPE(canon_has_flavor_type, flavor_type)

// canon_extract_flavor
//   trait: emitted directly by the canonical D_TYPE_TRAIT_MEMBER_TYPE_OR.
D_TYPE_TRAIT_MEMBER_TYPE_OR(canon_extract_flavor, flavor_type, void)

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   FORWARDING   (member_traits_tests_forwarding.cpp)
bool tests_forwarding_has_member_type_detects();
bool tests_forwarding_has_member_type_name_is_derived();
bool tests_forwarding_has_member_type_matches_canonical();
bool tests_forwarding_member_type_or_extracts();
bool tests_forwarding_member_type_or_falls_back();
bool tests_forwarding_member_type_or_matches_canonical();
bool tests_forwarding_shim_reexports_new_detectors();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// member_traits_spec
//   function: the authoritative description of this suite -- one block, one row
// per tests_* body, every node named and described.
inline dt::module_spec
member_traits_spec()
{
    return dt::module_spec{
        "member_traits",
        "The DEPRECATED transitional shim. Its entire content is two macro "
        "definitions forwarding the old spellings to the canonical macros in "
        "trait_detect.hpp -- D_DEFINE_HAS_MEMBER_TYPE(NAME) to "
        "D_TYPE_TRAIT_HAS_TYPE(has_##NAME, NAME), and D_DEFINE_MEMBER_TYPE_OR to "
        "D_TYPE_TRAIT_MEMBER_TYPE_OR -- together with the transitive re-export of "
        "member_types.hpp's concrete detectors. There is no runtime code, so the "
        "suite invokes each forwarded spelling and proves the emitted trait "
        "behaves exactly like the canonical macro it forwards to, that the "
        "has_<NAME> name derivation is preserved, and that including the shim "
        "makes the new detectors visible.",
        {
            dt::block_spec{
                "forwarding",
                "The two forwarded macro spellings and the transitive re-export: "
                "the detection macro emits a working detector under the derived "
                "name, the extractor macro emits a working extract-or-fall-back "
                "trait, both agree pointwise with their canonical equivalents, "
                "and the shim brings the new headers' detectors into scope.",
                {
                    { "tests_forwarding_has_member_type_detects",
                      "The trait emitted by D_DEFINE_HAS_MEMBER_TYPE(flavor_type) "
                      "is true for a type exposing ::flavor_type and false for "
                      "one that does not.",
                      &tests_forwarding_has_member_type_detects },
                    { "tests_forwarding_has_member_type_name_is_derived",
                      "The emitted trait is spelled exactly has_flavor_type -- "
                      "the old macro's has_<NAME> derivation is preserved, which "
                      "the test proves by naming that identifier directly.",
                      &tests_forwarding_has_member_type_name_is_derived },
                    { "tests_forwarding_has_member_type_matches_canonical",
                      "has_flavor_type agrees pointwise with a twin trait built "
                      "from the canonical D_TYPE_TRAIT_HAS_TYPE, including the "
                      "cv-ref stripping and the typedef-vs-value distinction.",
                      &tests_forwarding_has_member_type_matches_canonical },
                    { "tests_forwarding_member_type_or_extracts",
                      "The trait emitted by D_DEFINE_MEMBER_TYPE_OR extracts "
                      "::flavor_type when present, and its _t alias names the "
                      "same type.",
                      &tests_forwarding_member_type_or_extracts },
                    { "tests_forwarding_member_type_or_falls_back",
                      "The same extractor yields the fallback (void) when the "
                      "member is absent.",
                      &tests_forwarding_member_type_or_falls_back },
                    { "tests_forwarding_member_type_or_matches_canonical",
                      "The shim-built extractor agrees with a twin built from the "
                      "canonical D_TYPE_TRAIT_MEMBER_TYPE_OR on both the present "
                      "and the absent case.",
                      &tests_forwarding_member_type_or_matches_canonical },
                    { "tests_forwarding_shim_reexports_new_detectors",
                      "Including the shim makes member_types.hpp's concrete "
                      "detectors (has_value_type) and pick_member_type visible, "
                      "since the shim includes that header.",
                      &tests_forwarding_shim_reexports_new_detectors },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_MEMBER_TRAITS_TESTS_
