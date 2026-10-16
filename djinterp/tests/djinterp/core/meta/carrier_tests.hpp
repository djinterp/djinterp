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
* path:      /tests/djinterp/core/meta/carrier_tests.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_META_CARRIER_TESTS_
#define DJINTERP_META_CARRIER_TESTS_ 1

// std
#include <cstdio>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/core/meta/carrier.hpp"


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


NS_DJINTERP
NS_TESTING


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_META_CARRIER_TESTS_
