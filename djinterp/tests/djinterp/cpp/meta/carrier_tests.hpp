/*******************************************************************************
* djinterp [test]                                              carrier_tests.hpp
*
* djinterp carrier unit-test header:
*   Declarations, helper types, and a helper macro for the carrier.hpp unit
* suite. Each test is a parameterless `bool tests_*()` living flat in
* `djinterp::testing`; a test returns true on success and false on the first
* failed runtime check.
*
*   carrier.hpp is a compile-time facility, so the load-bearing assertions are
* `static_assert`s inside the test bodies: a violation fails the build. The
* runtime checks re-confirm, at run time, the facts that collapse to ordinary
* values (recovered NTTPs, the `_v` trait booleans, concept-driven dispatch),
* which is the carrier contract -- a carrier is itself a value.
*
*   PORTABILITY (mirrors carrier.hpp's own tiers):
*   - type carrier + detection traits compile at the C++14 floor;
*   - the value carrier and carrier objects are gated to C++17;
*   - the concept faces are gated to C++20.
* Every test function is always defined; standard-specific assertions are
* guarded inside the bodies, so the suite links and runs at every floor.
*
*
* path:      /tests/djinterp/cpp/meta/carrier_tests.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.24
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_META_CARRIER_TESTS_
#define DJINTERP_META_CARRIER_TESTS_ 1

// std
#include <cstdio>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/meta/carrier.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// D_CR_CHECK
//   macro: evaluates a boolean condition inside a carrier test. on failure it
// prints the failing expression and source location, then returns false from
// the enclosing test (which the runner reports as a FAIL). a trailing
// semicolon is expected at each call site.
#define D_CR_CHECK(_condition)                                          \
    do                                                                  \
    {                                                                   \
        if (!(_condition))                                             \
        {                                                               \
            std::printf("    [check] %s\n             at %s:%d\n",       \
                        #_condition,                                    \
                        __FILE__,                                       \
                        __LINE__);                                      \
                                                                        \
            return false;                                               \
        }                                                               \
    }                                                                   \
    while (0)


#endif  // !DTEST_SPEC_MODE
// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// -----------------------------------------------------------------
// helper carry-ables and decoys
// -----------------------------------------------------------------

// sample
//   helper: an ordinary empty class type used as a thing to carry, so the
// type-domain tests exercise a user-defined type and not only built-ins.
struct sample
{};

// decoy
//   helper: a NON-carrier that nonetheless exposes a nested `type` and a
// static `value`. It proves carrier detection is specialization-based
// (matching type_t / val_t exactly) rather than duck-typed on members:
// every is_*_carrier trait must reject it.
struct decoy
{
    using type = int;

    static constexpr int value = 5;
};

// color
//   helper: a scoped enumeration, used to confirm val_t carries NTTPs of
// non-integral-builtin type (an enumerator) and recovers them intact.
enum class color
{
    red,
    green,
    blue
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



// -----------------------------------------------------------------
// test declarations
// -----------------------------------------------------------------

// type carrier: type_t / carrier_type_t / type_c (carrier_tests_type.cpp)
bool tests_carrier_type_t_carries_type();
bool tests_carrier_type_t_recover();
bool tests_carrier_type_t_recover_cvref();
bool tests_carrier_type_t_nested();
bool tests_carrier_type_c_object();

// detection: is_type_carrier / is_value_carrier / is_carrier / _v
// (carrier_tests_detection.cpp)
bool tests_carrier_is_type_carrier();
bool tests_carrier_is_type_carrier_rejects();
bool tests_carrier_is_value_carrier();
bool tests_carrier_is_value_carrier_rejects();
bool tests_carrier_is_carrier_accepts_type();
bool tests_carrier_is_carrier_accepts_value();
bool tests_carrier_is_carrier_rejects();
bool tests_carrier_v_companions();

// value carrier: val_t / val / carrier_value_v (carrier_tests_value.cpp)
bool tests_carrier_val_t_carries_value();
bool tests_carrier_val_object();
bool tests_carrier_value_recover();
bool tests_carrier_value_recover_cvref();
bool tests_carrier_value_runtime();

// concept faces: TypeCarrier / ValueCarrier / Carrier
// (carrier_tests_concepts.cpp)
bool tests_carrier_concept_type_carrier();
bool tests_carrier_concept_value_carrier();
bool tests_carrier_concept_carrier();
bool tests_carrier_concept_constrains();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// carrier_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
carrier_spec()
{
    return dt::module_spec{
        "carrier",
        "Type and value carriers",
        {
            dt::block_spec{
                "type",
                "Type",
                {
                    { "tests_carrier_type_t_carries_type",
                      "Verifies type_t stores its argument as the nested "
                      "`type`, verbatim",
                      &tests_carrier_type_t_carries_type },
                    { "tests_carrier_type_t_recover",
                      "Verifies carrier_type_t recovers the carried type from "
                      "a type_t",
                      &tests_carrier_type_t_recover },
                    { "tests_carrier_type_t_recover_cvref",
                      "Verifies carrier_type_t ignores cv-qualifiers and "
                      "references ON THE CARRIER (the clean_t step), exactly "
                      "as the header documents",
                      &tests_carrier_type_t_recover_cvref },
                    { "tests_carrier_type_t_nested",
                      "Verifies a carrier can carry another carrier (carriers "
                      "are just types)",
                      &tests_carrier_type_t_nested },
                    { "tests_carrier_type_c_object",
                      "Verifies the type-domain carrier OBJECT type_c (a "
                      "C++17 facility)",
                      &tests_carrier_type_c_object },
                }
            },
            dt::block_spec{
                "detection",
                "Detection",
                {
                    { "tests_carrier_is_type_carrier",
                      "Verifies is_type_carrier accepts type_t "
                      "specializations (the raw success specialization) and "
                      "cleans cv-ref on its argument",
                      &tests_carrier_is_type_carrier },
                    { "tests_carrier_is_type_carrier_rejects",
                      "Verifies is_type_carrier rejects non-carriers, "
                      "including a decoy that has a nested `type` (so "
                      "detection is specialization-based, not duck-typed)",
                      &tests_carrier_is_type_carrier_rejects },
                    { "tests_carrier_is_value_carrier",
                      "Verifies is_value_carrier accepts val_t "
                      "specializations (the C++17 raw success specialization) "
                      "and cleans cv-ref",
                      &tests_carrier_is_value_carrier },
                    { "tests_carrier_is_value_carrier_rejects",
                      "Verifies is_value_carrier rejects non-carriers, "
                      "including a decoy that has a static `value` (so "
                      "detection is specialization-based, not duck-typed)",
                      &tests_carrier_is_value_carrier_rejects },
                    { "tests_carrier_is_carrier_accepts_type",
                      "Verifies is_carrier is satisfied via its FIRST "
                      "disjunct (the type-carrier branch) and cleans cv-ref",
                      &tests_carrier_is_carrier_accepts_type },
                    { "tests_carrier_is_carrier_accepts_value",
                      "Verifies is_carrier is satisfied via its SECOND "
                      "disjunct (the value-carrier branch), which requires "
                      "the first disjunct to be false",
                      &tests_carrier_is_carrier_accepts_value },
                    { "tests_carrier_is_carrier_rejects",
                      "Verifies is_carrier is false when BOTH disjuncts are "
                      "false",
                      &tests_carrier_is_carrier_rejects },
                    { "tests_carrier_v_companions",
                      "Verifies each `_v` companion agrees with its trait's "
                      "::value member, across positive and negative inputs",
                      &tests_carrier_v_companions },
                }
            },
            dt::block_spec{
                "value",
                "Value",
                {
                    { "tests_carrier_val_t_carries_value",
                      "Verifies val_t stores its NTTP argument as the static "
                      "`value`, keeping the NTTP's own type (the `auto "
                      "_Value` parameter)",
                      &tests_carrier_val_t_carries_value },
                    { "tests_carrier_val_object",
                      "Verifies the value-domain carrier OBJECT val (a C++17 "
                      "facility)",
                      &tests_carrier_val_object },
                    { "tests_carrier_value_recover",
                      "Verifies carrier_value_v recovers the carried value "
                      "from a val_t",
                      &tests_carrier_value_recover },
                    { "tests_carrier_value_recover_cvref",
                      "Verifies carrier_value_v ignores cv-qualifiers and "
                      "references ON THE CARRIER (the clean_t step), exactly "
                      "as the header documents",
                      &tests_carrier_value_recover_cvref },
                    { "tests_carrier_value_runtime",
                      "Verifies a value carrier collapses to an ordinary "
                      "runtime value -- the carrier contract",
                      &tests_carrier_value_runtime },
                }
            },
            dt::block_spec{
                "concepts",
                "Concepts",
                {
                    { "tests_carrier_concept_type_carrier",
                      "Verifies the TypeCarrier concept face (C++20)",
                      &tests_carrier_concept_type_carrier },
                    { "tests_carrier_concept_value_carrier",
                      "Verifies the ValueCarrier concept face (C++20)",
                      &tests_carrier_concept_value_carrier },
                    { "tests_carrier_concept_carrier",
                      "Verifies the Carrier concept face (C++20), satisfied "
                      "by either kind",
                      &tests_carrier_concept_carrier },
                    { "tests_carrier_concept_constrains",
                      "Verifies the Carrier concept actually constrains "
                      "overload resolution -- its intended use",
                      &tests_carrier_concept_constrains },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_META_CARRIER_TESTS_
