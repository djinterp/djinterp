/******************************************************************************
* djinterp [testing]                                    member_types_tests.hpp
*
* Unit test suite for member_types.hpp -- the framework's shared nested-typedef
* detectors and the pick_member_type extraction helper.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_MT_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       member_types_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  member_types.hpp is macro-generated: it invokes
* D_TYPE_TRAIT_HAS_TYPE (from trait_detect.hpp) ten times to emit the concrete
* detectors has_input_type, has_result_type, has_item_type, has_kind_type,
* has_value_type, has_key_type, has_mapped_type, has_size_type,
* has_difference_type, has_allocator_type -- each a SFINAE bool trait probing
* `typename clean_t<_Type>::<name>`, each with a `_v` companion on C++14+.  It
* also defines internal::pick_member_type<Present, Extracted, Fallback>, a
* two-specialization chooser.  The tests treat the emitted traits as the public
* surface and verify the behaviour the macro confers, rather than re-testing the
* macro engine itself (that belongs to a trait_detect suite).
*
*   THE clean_t SEAM (the property most worth pinning).  D_TYPE_TRAIT_HAS_TYPE
* strips cv-ref through clean_t BEFORE probing, so has_X<const T&> agrees with
* has_X<T>.  It does NOT see through a pointer: has_X<T*> is false even when T
* has the member, because a pointer's pointee members are not the pointer's.
* And the probe is a TYPEDEF probe: a static data member named like the typedef
* (e.g. `static constexpr int value_type`) does not satisfy has_value_type.
*
*   COMPLETENESS.  Because the probe is `typename clean_t<_Type>::X` in an
* unevaluated SFINAE context, has_X<incomplete_type> is a clean false, not a
* hard error -- so the detectors are usable on forward-declared types.  (This
* is the opposite of is_passthrough in the sibling passthrough suite, which
* needs a complete type.)
*
*   THE FIXTURE VOCABULARY (normal mode only)
*     has_all         one struct exposing all ten detected typedefs at once.
*     has_none        an empty struct exposing none of them.
*     one_of<Which>   a family each exposing exactly one of the ten, so a
*                     detector can be shown to fire on its own typedef and on no
*                     other.  (Realized as ten small named structs, not a
*                     template, since the member NAME differs per case.)
*     value_shaped    a struct with `static constexpr int value_type` -- a VALUE
*                     named like the typedef, to prove the typedef probe rejects
*                     it.
*     void_typedef    a struct whose value_type is `void` -- a typedef to void is
*                     still a typedef, so detection is true.
*     incomplete_type a forward declaration, never defined, to pin the soft-false
*                     on an incomplete type.
*     probe_present / probe_fallback / probe_extracted
*                     distinct tag types for the pick_member_type branches, so
*                     "chose the extracted" and "chose the fallback" are provable
*                     by identity.
*
*   BUILD PREREQUISITE: none.  member_types.hpp compiles as shipped against
*   trait_detect.hpp and the core prelude.
*
* CONTENTS
*   I.    DETECTORS            (member_types_tests_detectors.cpp)
*   II.   DETECTOR SEMANTICS   (member_types_tests_semantics.cpp)
*   III.  EXTRACTION           (member_types_tests_extraction.cpp)
*
* path:      /tests/djinterp/core/meta/member_types_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.25
******************************************************************************/

#ifndef DJINTERP_TESTS_MEMBER_TYPES_TESTS_
#define DJINTERP_TESTS_MEMBER_TYPES_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "member_types.hpp"                     // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// member_types_check
//   function: routes one D_MT_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>).
inline bool
member_types_check(
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

// D_MT_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  The `MT` suffix is
// this suite's unique two letters.
#define D_MT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::member_types_check(                         \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   ALL / NONE                                            ///
///////////////////////////////////////////////////////////////////////////////

// has_all
//   struct: exposes all ten detected typedefs at once, with deliberately
// mismatched underlying types so a detector cannot accidentally key on the
// wrong one.
struct has_all
{
    using input_type      = int;
    using result_type     = long;
    using item_type       = char;
    using kind_type       = short;
    using value_type      = double;
    using key_type        = unsigned;
    using mapped_type     = float;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type  = void*;
};

// has_none
//   struct: exposes none of the ten typedefs.
struct has_none
{};


///////////////////////////////////////////////////////////////////////////////
///             F.2   ONE-OF FIXTURES (exactly one typedef each)            ///
///////////////////////////////////////////////////////////////////////////////
//   Each exposes exactly one of the ten typedefs, so a detector can be shown to
// fire on its own and stay silent on the other nine.

struct only_input       { using input_type      = int; };
struct only_result      { using result_type     = int; };
struct only_item        { using item_type       = int; };
struct only_kind        { using kind_type       = int; };
struct only_value       { using value_type      = int; };
struct only_key         { using key_type        = int; };
struct only_mapped      { using mapped_type     = int; };
struct only_size        { using size_type       = int; };
struct only_difference  { using difference_type = int; };
struct only_allocator   { using allocator_type  = int; };


///////////////////////////////////////////////////////////////////////////////
///             F.3   SEMANTIC EDGE FIXTURES                                ///
///////////////////////////////////////////////////////////////////////////////

// value_shaped
//   struct: a static DATA member named like a typedef.  The typedef probe must
// reject it -- `X::value_type` names a value here, not a type.
struct value_shaped
{
    static constexpr int value_type = 0;
};

// void_typedef
//   struct: value_type is void.  A typedef to void is still a typedef, so
// detection is true.
struct void_typedef
{
    using value_type = void;
};

// incomplete_type
//   struct: forward-declared, never defined.  has_X on it is a soft false.
struct incomplete_type;

// derived_from_has_all
//   struct: inherits the ten typedefs.  clean_t does not flatten inheritance
// away, but a nested typedef is visible through a derived class, so the
// detectors see the inherited members -- pinned to document the behaviour.
struct derived_from_has_all : has_all
{};


///////////////////////////////////////////////////////////////////////////////
///             F.4   EXTRACTION TAGS                                       ///
///////////////////////////////////////////////////////////////////////////////

// probe_present / probe_fallback / probe_extracted
//   struct: distinct tags so pick_member_type's two branches are provable by
// identity rather than by value.
struct probe_present   {};
struct probe_fallback  {};
struct probe_extracted {};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   DETECTORS   (member_types_tests_detectors.cpp)
bool tests_detectors_all_ten_fire_on_has_all();
bool tests_detectors_all_ten_silent_on_has_none();
bool tests_detectors_each_fires_only_on_its_own_typedef();
bool tests_detectors_on_standard_containers();
bool tests_detectors_associative_only_typedefs();
bool tests_detectors_value_companions_agree();

// II.  DETECTOR SEMANTICS   (member_types_tests_semantics.cpp)
bool tests_semantics_strip_cv_ref_via_clean_t();
bool tests_semantics_pointer_is_not_seen_through();
bool tests_semantics_value_member_is_not_a_typedef();
bool tests_semantics_void_typedef_is_present();
bool tests_semantics_incomplete_type_is_soft_false();
bool tests_semantics_inherited_typedef_is_visible();
bool tests_semantics_trait_shape_is_integral_constant();

// III. EXTRACTION   (member_types_tests_extraction.cpp)
bool tests_extraction_present_yields_extracted();
bool tests_extraction_absent_yields_fallback();
bool tests_extraction_fallback_ignores_extracted();
bool tests_extraction_distinct_types_are_distinguished();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// member_types_spec
//   function: the authoritative description of this suite -- one block per
// section TU, one row per tests_* body, every node named and described.
inline dt::module_spec
member_types_spec()
{
    return dt::module_spec{
        "member_types",
        "The framework's shared nested-typedef detectors -- has_input_type, "
        "has_result_type, has_item_type, has_kind_type, has_value_type, "
        "has_key_type, has_mapped_type, has_size_type, has_difference_type, "
        "has_allocator_type -- each emitted by D_TYPE_TRAIT_HAS_TYPE as a SFINAE "
        "bool trait that probes `typename clean_t<T>::<name>` (cv-ref stripped, "
        "with a `_v` companion on C++14+), plus internal::pick_member_type, the "
        "extract-or-fall-back chooser. This suite pins what the detectors report "
        "across present / absent / partial shapes and real STL types, the "
        "clean_t seam (cv-ref stripping, pointer opacity, typedef-vs-value, void "
        "typedefs, incomplete types), the trait's integral_constant shape, and "
        "the two branches of pick_member_type.",
        {
            dt::block_spec{
                "detectors",
                "The ten concrete detectors reporting presence and absence: all "
                "ten on a struct that has everything, none on an empty one, each "
                "one firing on exactly its own typedef, and correct behaviour on "
                "standard library containers.",
                {
                    { "tests_detectors_all_ten_fire_on_has_all",
                      "Every one of the ten detectors is true for a struct that "
                      "exposes all ten typedefs.",
                      &tests_detectors_all_ten_fire_on_has_all },
                    { "tests_detectors_all_ten_silent_on_has_none",
                      "Every one of the ten detectors is false for an empty "
                      "struct exposing none of them.",
                      &tests_detectors_all_ten_silent_on_has_none },
                    { "tests_detectors_each_fires_only_on_its_own_typedef",
                      "For each of the ten, a fixture exposing only that typedef "
                      "makes exactly that detector true and the other nine "
                      "false -- no detector keys on the wrong member.",
                      &tests_detectors_each_fires_only_on_its_own_typedef },
                    { "tests_detectors_on_standard_containers",
                      "std::vector and std::string expose value_type, size_type, "
                      "difference_type and allocator_type but no mapped_type; the "
                      "detectors agree.",
                      &tests_detectors_on_standard_containers },
                    { "tests_detectors_associative_only_typedefs",
                      "std::map exposes key_type and mapped_type where a sequence "
                      "container does not, so the associative detectors "
                      "distinguish the two.",
                      &tests_detectors_associative_only_typedefs },
                    { "tests_detectors_value_companions_agree",
                      "Each has_<name>_v companion equals has_<name><T>::value "
                      "across present and absent cases, and is a constant "
                      "expression. [C++14+]",
                      &tests_detectors_value_companions_agree },
                }
            },
            dt::block_spec{
                "semantics",
                "The resolution behaviour D_TYPE_TRAIT_HAS_TYPE confers through "
                "clean_t: cv-ref stripping, pointer opacity, the "
                "typedef-versus-value distinction, void typedefs, soft failure "
                "on incomplete types, visibility of inherited typedefs, and the "
                "trait's integral_constant shape.",
                {
                    { "tests_semantics_strip_cv_ref_via_clean_t",
                      "has_X<const T&>, has_X<volatile T> and has_X<T&&> all "
                      "agree with has_X<T>, because the probe strips cv-ref "
                      "first -- in both the true and the false direction.",
                      &tests_semantics_strip_cv_ref_via_clean_t },
                    { "tests_semantics_pointer_is_not_seen_through",
                      "has_X<T*> is false even when T has the typedef: a "
                      "pointer's pointee members are not the pointer's, and "
                      "clean_t does not dereference.",
                      &tests_semantics_pointer_is_not_seen_through },
                    { "tests_semantics_value_member_is_not_a_typedef",
                      "A static data member named like the typedef (static "
                      "constexpr int value_type) does not satisfy "
                      "has_value_type: the probe detects a TYPE, not a value.",
                      &tests_semantics_value_member_is_not_a_typedef },
                    { "tests_semantics_void_typedef_is_present",
                      "A typedef whose target is void is still a typedef, so "
                      "has_value_type is true for a struct whose value_type is "
                      "void.",
                      &tests_semantics_void_typedef_is_present },
                    { "tests_semantics_incomplete_type_is_soft_false",
                      "has_X on a forward-declared, never-defined type is a "
                      "clean false rather than a hard error, so the detectors "
                      "are usable on incomplete types.",
                      &tests_semantics_incomplete_type_is_soft_false },
                    { "tests_semantics_inherited_typedef_is_visible",
                      "A nested typedef inherited from a base class is visible to "
                      "the detector: a struct deriving from one that has the "
                      "typedefs is detected as having them.",
                      &tests_semantics_inherited_typedef_is_visible },
                    { "tests_semantics_trait_shape_is_integral_constant",
                      "Each detector derives from std::true_type / "
                      "std::false_type: value_type is bool, ::type is the "
                      "matching integral_constant, and ::value is a constant "
                      "expression.",
                      &tests_semantics_trait_shape_is_integral_constant },
                }
            },
            dt::block_spec{
                "extraction",
                "internal::pick_member_type<Present, Extracted, Fallback>: the "
                "two-specialization chooser behind the extract-or-fall-back "
                "macro. Which type each branch yields, and that the fallback "
                "branch ignores the extracted type entirely.",
                {
                    { "tests_extraction_present_yields_extracted",
                      "pick_member_type<true, Extracted, Fallback>::type is "
                      "Extracted.",
                      &tests_extraction_present_yields_extracted },
                    { "tests_extraction_absent_yields_fallback",
                      "pick_member_type<false, Extracted, Fallback>::type is "
                      "Fallback.",
                      &tests_extraction_absent_yields_fallback },
                    { "tests_extraction_fallback_ignores_extracted",
                      "The false branch yields the fallback regardless of what "
                      "the extracted type is -- even a distinct tag it never "
                      "names.",
                      &tests_extraction_fallback_ignores_extracted },
                    { "tests_extraction_distinct_types_are_distinguished",
                      "With three distinct tag types, the chosen branch is "
                      "provable by identity: present picks the extracted tag, "
                      "absent picks the fallback tag, and the two are not "
                      "confused.",
                      &tests_extraction_distinct_types_are_distinguished },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_MEMBER_TYPES_TESTS_
