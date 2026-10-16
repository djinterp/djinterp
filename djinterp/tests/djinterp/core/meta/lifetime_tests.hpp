/*******************************************************************************
* djinterp [testing]                                          lifetime_tests.hpp
*
* Unit test suite for lifetime.hpp -- the compile-time / runtime lifetime
* vocabulary, its lattice algebra, and the type classifier built on it.
*   Two faces, selected by DTEST_SPEC_MODE (see djinterp_testing.md S3):
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the fixtures + the D_LT_CHECK macro.
*     - spec  mode (the runner):       pulls in test_defaults.hpp and exposes
*       lifetime_spec(), the module -> block -> test description.
*
*   WHAT IS UNDER TEST.  lifetime.hpp is a self-contained algebra plus a
* classifier:
*     - the `lifetime` enum: four values (none, compile_time, runtime, both),
*       one bit per evaluation stage, forming a lattice under bitwise ops;
*     - the algebra: lifetime_meet (bitwise AND), lifetime_join (bitwise OR),
*       make_lifetime, eight boolean stage predicates, and lifetime_name;
*     - lifetime_constant and the four named carriers;
*     - is_literal_type, the portable constexpr-capability structural probe;
*     - has_lifetime_category + internal::lifetime_category_member, the opt-in
*       detection;
*     - lifetime_of, the priority-ordered classifier (opt-in > literal > runtime
*       fallback), with its ::type carrier and lifetime_of_v companion;
*     - three derived boolean predicates (is_constexpr_lifetime,
*       is_runtime_only_lifetime, is_dual_lifetime).
*
*   THE TWO PROPERTIES MOST WORTH PINNING.
*     1. THE LATTICE.  meet is exactly bitwise AND and join exactly bitwise OR
*        over the two stage bits, so the four values obey the lattice laws:
*        commutativity, idempotence, `both` as meet-identity / join-absorbing,
*        `none` as meet-absorbing / join-identity.  meet is the canonical
*        COMPOSITION operator -- a composite is constexpr-capable iff every part
*        is -- and the suite pins it across the full 4x4 table.
*     2. THE CLASSIFIER PRIORITY.  lifetime_of consults, in order: an opt-in
*        `lifetime_category` member (highest), then is_literal_type (-> both),
*        then a conservative runtime fallback.  The decisive test is a type that
*        IS structurally a literal type but whose opt-in says `runtime`: the
*        opt-in must win, so lifetime_of reports runtime, not both.  The opt-in
*        wins even when it declares the bottom value `none`.
*
*   THE FIXTURE VOCABULARY (normal mode only)
*     opt_ct / opt_rt / opt_both / opt_none
*                     types declaring each of the four values via a
*                     `lifetime_category` member.  opt_rt and opt_none are also
*                     structurally literal, so they are the override probes: the
*                     opt-in must beat the literal-type default.
*     literal_noopt   a literal type WITHOUT any opt-in -> classifies as both.
*     nonliteral      a non-literal type (non-trivial destructor) without opt-in
*                     -> classifies via the runtime fallback.
*     all_lifetimes() a helper returning the four enum values, so the algebra
*                     tests can sweep the whole lattice.
*
*   BUILD PREREQUISITE: none.  lifetime.hpp compiles as shipped against
*   trait_detect.hpp and the core prelude.
*
* CONTENTS
*   I.    ALGEBRA          (lifetime_tests_algebra.cpp)
*   II.   LITERAL PROBE    (lifetime_tests_literal_probe.cpp)
*   III.  OPT-IN           (lifetime_tests_opt_in.cpp)
*   IV.   CLASSIFICATION   (lifetime_tests_classification.cpp)
*   V.    DERIVED          (lifetime_tests_derived.cpp)
*
*
* path:      /tests/djinterp/core/meta/lifetime_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_LIFETIME_TESTS_
#define DJINTERP_TESTS_LIFETIME_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
// djinterp
// -- (part 1) mode-gated includes --------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/meta/lifetime.hpp"                         // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY.
namespace dt = ::djinterp::test;


// lifetime_check
//   function: routes one D_LT_CHECK evaluation.  Prints the failing expression
// and its location and hands the condition back.  Self-contained (<cstdio>).
inline bool
lifetime_check(
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

// D_LT_CHECK
//   macro: evaluate a checked expression exactly once; on failure report it and
// early-return false from the enclosing tests_* body.  Variadic so a top-level
// comma inside a trait expression passes through whole.  The `LT` suffix is
// this suite's unique two letters.
#define D_LT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::lifetime_check(                             \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures -- normal mode only

///////////////////////////////////////////////////////////////////////////////
///             F.1   OPT-IN TYPES                                          ///
///////////////////////////////////////////////////////////////////////////////
//   Each declares its own lifetime via the opt-in static member.  opt_rt and
// opt_none are ALSO structurally literal types, so they double as the override
// probes: the opt-in must beat the literal-type default.

// opt_ct
//   struct: declares the compile-time-only lifetime.
struct opt_ct
{
    static constexpr lifetime lifetime_category = lifetime::compile_time;
};

// opt_rt
//   struct: declares the runtime-only lifetime.  Structurally literal, so it
// tests that the opt-in overrides the is_literal_type default.
struct opt_rt
{
    static constexpr lifetime lifetime_category = lifetime::runtime;
};

// opt_both
//   struct: declares the both-stages lifetime.
struct opt_both
{
    static constexpr lifetime lifetime_category = lifetime::both;
};

// opt_none
//   struct: declares the bottom lifetime.  Structurally literal, so it tests
// that the opt-in wins even when it names the bottom value.
struct opt_none
{
    static constexpr lifetime lifetime_category = lifetime::none;
};


///////////////////////////////////////////////////////////////////////////////
///             F.2   STRUCTURAL FIXTURES                                   ///
///////////////////////////////////////////////////////////////////////////////

// literal_noopt
//   struct: a literal type with NO opt-in -- classifies as both via
// is_literal_type.
struct literal_noopt
{
    int x;
};

// nonliteral
//   struct: a non-literal type (user-provided destructor) with no opt-in --
// classifies via the conservative runtime fallback.
struct nonliteral
{
    ~nonliteral() {}
};


///////////////////////////////////////////////////////////////////////////////
///             F.3   LATTICE SWEEP HELPER                                  ///
///////////////////////////////////////////////////////////////////////////////

// all_lifetimes
//   function: the four lattice values in a fixed order, so the algebra tests
// can iterate the whole lattice without restating the list.
inline const lifetime*
all_lifetimes(
    std::size_t& _count
)
{
    static const lifetime values[4] =
    {
        lifetime::none,
        lifetime::compile_time,
        lifetime::runtime,
        lifetime::both
    };

    _count = 4;
    return values;
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations -- visible in BOTH modes --------------------------

// I.   ALGEBRA   (lifetime_tests_algebra.cpp)
bool tests_algebra_enum_bit_values();
bool tests_algebra_meet_is_bitwise_and();
bool tests_algebra_join_is_bitwise_or();
bool tests_algebra_meet_lattice_laws();
bool tests_algebra_join_lattice_laws();
bool tests_algebra_make_lifetime_from_stage_facts();
bool tests_algebra_stage_predicates();
bool tests_algebra_exclusive_stage_predicates();
bool tests_algebra_lifetime_name_spellings();

// II.  LITERAL PROBE   (lifetime_tests_literal_probe.cpp)
bool tests_literal_probe_scalars_are_literal();
bool tests_literal_probe_trivial_class_is_literal();
bool tests_literal_probe_nonliteral_is_not();
bool tests_literal_probe_strips_cv_ref();
bool tests_literal_probe_value_companion();

// III. OPT-IN   (lifetime_tests_opt_in.cpp)
bool tests_opt_in_detects_category_member();
bool tests_opt_in_absent_is_false();
bool tests_opt_in_strips_cv_ref();
bool tests_opt_in_member_reads_declared_value();

// IV.  CLASSIFICATION   (lifetime_tests_classification.cpp)
bool tests_classification_opt_in_takes_priority();
bool tests_classification_opt_in_overrides_literal();
bool tests_classification_literal_without_opt_in_is_both();
bool tests_classification_nonliteral_falls_back_to_runtime();
bool tests_classification_carrier_type();
bool tests_classification_strips_cv_ref();
bool tests_classification_value_companion();

// V.   DERIVED   (lifetime_tests_derived.cpp)
bool tests_derived_is_constexpr_lifetime();
bool tests_derived_is_runtime_only_lifetime();
bool tests_derived_is_dual_lifetime();
bool tests_derived_predicates_agree_with_classifier();
bool tests_derived_trait_shape_and_companions();


// -- (part 3) the spec provider -- spec mode only ----------------------------
#ifdef DTEST_SPEC_MODE

// lifetime_spec
//   function: the authoritative description of this suite -- one block per
// section TU, one row per tests_* body, every node named and described.
inline dt::module_spec
lifetime_spec()
{
    return dt::module_spec{
        "lifetime",
        "The compile-time / runtime lifetime vocabulary and the classifier built "
        "on it. A lifetime is a set of evaluation stages (compile-time precedes "
        "runtime), encoded one bit per stage so the four values -- none, "
        "compile_time, runtime, both -- form a lattice. The module supplies the "
        "algebra (meet as bitwise AND, join as bitwise OR, make_lifetime, eight "
        "stage predicates, lifetime_name), type-level carriers, the portable "
        "is_literal_type probe, opt-in detection via a lifetime_category member, "
        "the priority-ordered lifetime_of classifier (opt-in > literal > runtime "
        "fallback), and three derived predicates. This suite pins the full 4x4 "
        "lattice and its laws, the literal-type probe, the opt-in detection, and "
        "-- centrally -- the classifier priority, including the case where an "
        "opt-in of runtime overrides a structurally-literal type.",
        {
            dt::block_spec{
                "algebra",
                "The lattice and its operations: the enum bit values, meet as "
                "bitwise AND and join as bitwise OR across the full 4x4 table, "
                "the lattice laws (commutativity, idempotence, identity and "
                "absorbing elements), make_lifetime, the eight stage predicates, "
                "and lifetime_name.",
                {
                    { "tests_algebra_enum_bit_values",
                      "The enum values are the expected bit patterns: none 0, "
                      "compile_time 1, runtime 2, both 3 (= compile_time | "
                      "runtime).",
                      &tests_algebra_enum_bit_values },
                    { "tests_algebra_meet_is_bitwise_and",
                      "lifetime_meet equals the bitwise AND of the stage bits "
                      "across all sixteen operand pairs.",
                      &tests_algebra_meet_is_bitwise_and },
                    { "tests_algebra_join_is_bitwise_or",
                      "lifetime_join equals the bitwise OR of the stage bits "
                      "across all sixteen operand pairs.",
                      &tests_algebra_join_is_bitwise_or },
                    { "tests_algebra_meet_lattice_laws",
                      "meet is commutative and idempotent, with both as identity "
                      "(meet(a, both) = a) and none as absorbing (meet(a, none) = "
                      "none), over the whole lattice.",
                      &tests_algebra_meet_lattice_laws },
                    { "tests_algebra_join_lattice_laws",
                      "join is commutative and idempotent, with none as identity "
                      "(join(a, none) = a) and both as absorbing (join(a, both) = "
                      "both), over the whole lattice.",
                      &tests_algebra_join_lattice_laws },
                    { "tests_algebra_make_lifetime_from_stage_facts",
                      "make_lifetime composes the four values from the two "
                      "independent stage booleans (compile-time?, runtime?).",
                      &tests_algebra_make_lifetime_from_stage_facts },
                    { "tests_algebra_stage_predicates",
                      "is_compile_time, is_runtime, is_both and is_none report "
                      "membership of each stage correctly for every value.",
                      &tests_algebra_stage_predicates },
                    { "tests_algebra_exclusive_stage_predicates",
                      "is_compile_time_only and is_runtime_only are true only for "
                      "the single-stage values, not for both or none.",
                      &tests_algebra_exclusive_stage_predicates },
                    { "tests_algebra_lifetime_name_spellings",
                      "lifetime_name returns the stable spelling for each of the "
                      "four values.",
                      &tests_algebra_lifetime_name_spellings },
                }
            },
            dt::block_spec{
                "literal_probe",
                "is_literal_type, the portable constexpr-capability structural "
                "signal: scalars and trivial class types are literal, "
                "non-literal types are not, cv-ref is stripped first, and the _v "
                "companion agrees.",
                {
                    { "tests_literal_probe_scalars_are_literal",
                      "Scalar types -- integers, floating point, pointers, enums "
                      "-- are literal types.",
                      &tests_literal_probe_scalars_are_literal },
                    { "tests_literal_probe_trivial_class_is_literal",
                      "A trivially default-constructible and trivially "
                      "destructible class type is a literal type.",
                      &tests_literal_probe_trivial_class_is_literal },
                    { "tests_literal_probe_nonliteral_is_not",
                      "A class with a user-provided destructor, and standard "
                      "types like std::string, are not literal types.",
                      &tests_literal_probe_nonliteral_is_not },
                    { "tests_literal_probe_strips_cv_ref",
                      "is_literal_type<const T&> agrees with is_literal_type<T> "
                      "in both directions.",
                      &tests_literal_probe_strips_cv_ref },
                    { "tests_literal_probe_value_companion",
                      "is_literal_type_v<T> equals is_literal_type<T>::value and "
                      "is a constant expression. [C++14+]",
                      &tests_literal_probe_value_companion },
                }
            },
            dt::block_spec{
                "opt_in",
                "has_lifetime_category and the category-member reader: detection "
                "of the opt-in static member, its absence, cv-ref stripping, and "
                "that the reader yields the declared value.",
                {
                    { "tests_opt_in_detects_category_member",
                      "has_lifetime_category is true for a type declaring the "
                      "lifetime_category member.",
                      &tests_opt_in_detects_category_member },
                    { "tests_opt_in_absent_is_false",
                      "has_lifetime_category is false for a type without the "
                      "member, including scalars and literal types.",
                      &tests_opt_in_absent_is_false },
                    { "tests_opt_in_strips_cv_ref",
                      "has_lifetime_category<const T&> agrees with the unqualified "
                      "form.",
                      &tests_opt_in_strips_cv_ref },
                    { "tests_opt_in_member_reads_declared_value",
                      "The internal category-member reader yields exactly the "
                      "declared lifetime for each opt-in type.",
                      &tests_opt_in_member_reads_declared_value },
                }
            },
            dt::block_spec{
                "classification",
                "lifetime_of, the priority-ordered classifier: opt-in first, then "
                "is_literal_type (-> both), then the runtime fallback -- including "
                "the decisive override case -- plus the ::type carrier, cv-ref "
                "stripping, and the lifetime_of_v companion.",
                {
                    { "tests_classification_opt_in_takes_priority",
                      "lifetime_of returns the opt-in value for each of the four "
                      "opt-in types.",
                      &tests_classification_opt_in_takes_priority },
                    { "tests_classification_opt_in_overrides_literal",
                      "The decisive case: a structurally-literal type whose "
                      "opt-in says runtime classifies as runtime, not both -- and "
                      "an opt-in of none wins over literal too.",
                      &tests_classification_opt_in_overrides_literal },
                    { "tests_classification_literal_without_opt_in_is_both",
                      "A literal type with no opt-in classifies as both; scalars "
                      "do too.",
                      &tests_classification_literal_without_opt_in_is_both },
                    { "tests_classification_nonliteral_falls_back_to_runtime",
                      "A non-literal type with no opt-in classifies as runtime "
                      "via the conservative fallback.",
                      &tests_classification_nonliteral_falls_back_to_runtime },
                    { "tests_classification_carrier_type",
                      "lifetime_of<T>::type and lifetime_of_t<T> are the "
                      "lifetime_constant carrier for the classified value.",
                      &tests_classification_carrier_type },
                    { "tests_classification_strips_cv_ref",
                      "lifetime_of<const T&> agrees with lifetime_of<T> for opt-in, "
                      "literal, and non-literal types.",
                      &tests_classification_strips_cv_ref },
                    { "tests_classification_value_companion",
                      "lifetime_of_v<T> equals lifetime_of<T>::value and is a "
                      "constant expression. [C++14+]",
                      &tests_classification_value_companion },
                }
            },
            dt::block_spec{
                "derived",
                "The three boolean projections of lifetime_of: "
                "is_constexpr_lifetime (includes compile-time), "
                "is_runtime_only_lifetime (runtime exclusively), and "
                "is_dual_lifetime (both stages), their agreement with the "
                "classifier, and their trait shape and _v companions.",
                {
                    { "tests_derived_is_constexpr_lifetime",
                      "is_constexpr_lifetime is true for constexpr-capable types "
                      "(literal, opt-in compile_time, opt-in both) and false for "
                      "runtime-only ones.",
                      &tests_derived_is_constexpr_lifetime },
                    { "tests_derived_is_runtime_only_lifetime",
                      "is_runtime_only_lifetime is true exactly for types whose "
                      "lifetime is runtime alone.",
                      &tests_derived_is_runtime_only_lifetime },
                    { "tests_derived_is_dual_lifetime",
                      "is_dual_lifetime is true exactly for types spanning both "
                      "stages (literal types and opt-in both).",
                      &tests_derived_is_dual_lifetime },
                    { "tests_derived_predicates_agree_with_classifier",
                      "Each derived predicate equals the corresponding projection "
                      "of lifetime_of's value across the fixture battery.",
                      &tests_derived_predicates_agree_with_classifier },
                    { "tests_derived_trait_shape_and_companions",
                      "The derived traits derive from integral_constant<bool>, "
                      "and their _v companions agree with ::value. [companions "
                      "C++14+]",
                      &tests_derived_trait_shape_and_companions },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_LIFETIME_TESTS_
