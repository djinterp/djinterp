/*******************************************************************************
* djinterp [test]                                            semigroup_tests.hpp
*
*   Unit-test declarations for core/functional/semigroup.hpp.  One declaration
* group per section of the module under test:
*
*     semigroup_tests_protocol.cpp   -- I.   semigroup_traits, is_semigroup,
*                                             the Semigroup concept
*     semigroup_tests_mappend.cpp    -- II.  mappend, the associative combine
*
*   FIXTURES.  semigroup.hpp is the protocol and the operation ALONE -- every
* concrete instance (string, vector, the numeric newtypes) lives in monoid.hpp.
* So the suite supplies its own semigroups by specialising semigroup_traits at
* djinterp:: scope, chosen to cover the distinct behaviours:
*
*     DSgString   std::string under concatenation   -- runtime, NON-commutative
*     DSgSum      int under addition                 -- constexpr, commutative
*     DSgMax      int under max                      -- constexpr, IDEMPOTENT
*     DSgFirst    left projection combine(a,b)=a     -- constexpr, NON-commut.
*     z3 / z5     modular addition (Z/nZ)            -- a FAMILY instance keyed
*                                                       on the _Enable SFINAE
*                                                       hook, one specialisation
*                                                       covering both types
*
* Two negative fixtures pin the detector: DNotSemigroup (no specialisation at
* all) and DSgNoMarker (a semigroup_traits with combine but WITHOUT the
* is_specialized marker, which must therefore read as not-a-semigroup).  The
* constexpr fixtures also exercise mappend's conditional-constexpr / dual-domain
* behaviour under static_assert.
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/semigroup_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.11
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (semigroup instances, the family, negatives, generic helper)
      ----------------------------------------------------------------------

I.    PROTOCOL + DETECTION
      --------------------

II.   MAPPEND
      -------
*/


#ifndef DJINTERP_TEST_SEMIGROUP_TESTS_
#define DJINTERP_TEST_SEMIGROUP_TESTS_ 1

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
// djinterp (module under test)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/semigroup.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// DNotSemigroup
//   type: no semigroup_traits specialisation exists -- the plain negative case.
struct DNotSemigroup
{
};


// DSgString
//   fixture semigroup: std::string under concatenation.  Non-commutative and
// not a literal type, so it drives the runtime path.
struct DSgString
{
    std::string s;
};

inline
bool operator==(
    const DSgString& _a,
    const DSgString& _b
)
{
    return (_a.s == _b.s);
}


// DSgSum
//   fixture semigroup: int under addition.  A literal type with a constexpr
// combine, so mappend folds it at compile time.
struct DSgSum
{
    int v;
};

constexpr
bool operator==(
    const DSgSum& _a,
    const DSgSum& _b
)
{
    return (_a.v == _b.v);
}


// DSgMax
//   fixture semigroup: int under max -- commutative and idempotent
// (combine(a, a) == a).  Literal type, constexpr combine.
struct DSgMax
{
    int v;
};

constexpr
bool operator==(
    const DSgMax& _a,
    const DSgMax& _b
)
{
    return (_a.v == _b.v);
}


// DSgFirst
//   fixture semigroup: the left-projection combine(a, b) = a.  Associative but
// emphatically non-commutative; literal type, constexpr combine.
struct DSgFirst
{
    int v;
};

constexpr
bool operator==(
    const DSgFirst& _a,
    const DSgFirst& _b
)
{
    return (_a.v == _b.v);
}


// -- the modular family (Z/nZ), detected via the _Enable SFINAE hook ------

// z3 / z5
//   fixture semigroups: modular addition, with the modulus carried on the type
// so a SINGLE family specialisation of semigroup_traits (keyed on is_modular
// through the trait's _Enable parameter) serves both.
struct z3
{
    int                        v;
    static constexpr int       modulus = 3;
};

struct DZ5
{
    int                        v;
    static constexpr int       modulus = 5;
};

constexpr
bool operator==(
    const z3& _a,
    const z3& _b
)
{
    return (_a.v == _b.v);
}

constexpr
bool operator==(
    const DZ5& _a,
    const DZ5& _b
)
{
    return (_a.v == _b.v);
}

// is_modular
//   family trait: marks the members of the modular family.
template<typename _Type>
struct is_modular : std::false_type
{
};

template<>
struct is_modular<z3> : std::true_type
{
};

template<>
struct is_modular<DZ5> : std::true_type
{
};


// DSgNoMarker
//   type: has a semigroup_traits specialisation with combine but WITHOUT the
// is_specialized marker -- is_semigroup must read false, since detection keys
// on the marker rather than on the presence of combine.
struct DSgNoMarker
{
    int v;
};


// -- generic helper -------------------------------------------------------

// thrice
//   helper: the header's own generic example, x <> (x <> x), written against
// the Semigroup protocol so it works for any instance.
template<typename _Semigroup>
inline
_Semigroup thrice(
    const _Semigroup& _x
)
{
    return mappend(_x, mappend(_x, _x));
}

// can_mappend
//   trait: detects whether mappend(A, B) is well-formed -- used to confirm the
// operation requires both operands to be the SAME semigroup type.
template<typename _A, typename _B, typename = void>
struct can_mappend : std::false_type
{
};

template<typename _A, typename _B>
struct can_mappend<
    _A, _B,
    decltype((void)mappend(std::declval<_A>(), std::declval<_B>()))>
    : std::true_type
{
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    PROTOCOL + DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_semigroup_positive();
bool tests_is_semigroup_negative();
bool tests_is_semigroup_cvref();
bool tests_is_semigroup_family_sfinae();
bool tests_is_semigroup_requires_marker();
bool tests_semigroup_traits_members();
bool tests_semigroup_concept();


///////////////////////////////////////////////////////////////////////////////
///             II.   MAPPEND                                               ///
///////////////////////////////////////////////////////////////////////////////

bool tests_mappend_dispatch();
bool tests_mappend_associativity();
bool tests_mappend_order_preserved();
bool tests_mappend_idempotent_max();
bool tests_mappend_constexpr();
bool tests_mappend_family_modular();
bool tests_mappend_generic_thrice();
bool tests_mappend_cvref_args();
bool tests_mappend_return_type();
bool tests_mappend_requires_same_type();
bool tests_mappend_runtime_domain();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// semigroup_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
semigroup_spec()
{
    return dt::module_spec{
        "semigroup",
        "semigroup_traits, is_semigroup, the _Enable hook, the concept",
        {
            dt::block_spec{
                "protocol",
                "Protocol",
                {
                    { "tests_is_semigroup_positive",
                      "Every specialised fixture is recognised as a semigroup",
                      &tests_is_semigroup_positive },
                    { "tests_is_semigroup_negative",
                      "Non-semigroups are rejected, and the undefined primary "
                      "resolves cleanly (no hard error) on the detection path",
                      &tests_is_semigroup_negative },
                    { "tests_is_semigroup_cvref",
                      "Detection strips cv-qualifiers and references before "
                      "testing",
                      &tests_is_semigroup_cvref },
                    { "tests_is_semigroup_family_sfinae",
                      "A family instance -- one specialisation keyed on the "
                      "trait's _Enable SFINAE hook -- makes every tagged type "
                      "a semigroup at once, without over-matching",
                      &tests_is_semigroup_family_sfinae },
                    { "tests_is_semigroup_requires_marker",
                      "Detection keys on the is_specialized marker, not on "
                      "the mere presence of a combine",
                      &tests_is_semigroup_requires_marker },
                    { "tests_semigroup_traits_members",
                      "A specialisation exposes exactly the protocol surface",
                      &tests_semigroup_traits_members },
                    { "tests_semigroup_concept",
                      "The Semigroup concept mirrors is_semigroup (C++20 only)",
                      &tests_semigroup_concept },
                }
            },
            dt::block_spec{
                "mappend",
                "Mappend",
                {
                    { "tests_mappend_dispatch",
                      "mappend routes to each instance's combine",
                      &tests_mappend_dispatch },
                    { "tests_mappend_associativity",
                      "The defining law: mappend(a, mappend(b, c)) == "
                      "mappend(mappend(a, b), c), checked across instances -- "
                      "including the non-commutative ones, where the law is "
                      "the substantive claim",
                      &tests_mappend_associativity },
                    { "tests_mappend_order_preserved",
                      "mappend passes its operands to combine in order -- so "
                      "on a non-commutative instance the two orderings "
                      "differ, and the left-to-right result is the one "
                      "produced",
                      &tests_mappend_order_preserved },
                    { "tests_mappend_idempotent_max",
                      "The max instance is idempotent: combining a value with "
                      "itself returns it",
                      &tests_mappend_idempotent_max },
                    { "tests_mappend_constexpr",
                      "mappend folds inside a constant expression whenever "
                      "the instance's combine is constexpr over a literal "
                      "type -- the header's conditional-constexpr contract",
                      &tests_mappend_constexpr },
                    { "tests_mappend_family_modular",
                      "The family instance computes modular addition, with "
                      "the modulus taken from the operand type, wrapping "
                      "correctly",
                      &tests_mappend_family_modular },
                    { "tests_mappend_generic_thrice",
                      "A function generic over the Semigroup protocol works "
                      "for every instance",
                      &tests_mappend_generic_thrice },
                    { "tests_mappend_cvref_args",
                      "mappend decays its operand type, so const and lvalue "
                      "operands are accepted",
                      &tests_mappend_cvref_args },
                    { "tests_mappend_return_type",
                      "mappend returns the semigroup type (the decltype of "
                      "combine)",
                      &tests_mappend_return_type },
                    { "tests_mappend_requires_same_type",
                      "mappend deduces one semigroup type from both operands, "
                      "so mismatched operand types are not callable",
                      &tests_mappend_requires_same_type },
                    { "tests_mappend_runtime_domain",
                      "mappend also runs at runtime -- the other half of the "
                      "dual domain -- over operands whose values are not "
                      "known at compile time",
                      &tests_mappend_runtime_domain },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             FIXTURE SEMIGROUP INSTANCES  (djinterp scope)               ///
///////////////////////////////////////////////////////////////////////////////

// semigroup_traits<DSgString>  -- concatenation
#ifndef DTEST_SPEC_MODE  // fixture instances - normal mode only
template<>
struct semigroup_traits<testing::DSgString, void>
{
    using is_specialized = std::true_type;

    static
    testing::DSgString combine(
        const testing::DSgString& _a,
        const testing::DSgString& _b
    )
    {
        return testing::DSgString{ _a.s + _b.s };
    }
};

// semigroup_traits<DSgSum>  -- addition (constexpr)
template<>
struct semigroup_traits<testing::DSgSum, void>
{
    using is_specialized = std::true_type;

    static constexpr
    testing::DSgSum combine(
        const testing::DSgSum& _a,
        const testing::DSgSum& _b
    )
    {
        return testing::DSgSum{ _a.v + _b.v };
    }
};

// semigroup_traits<DSgMax>  -- max (constexpr, idempotent)
template<>
struct semigroup_traits<testing::DSgMax, void>
{
    using is_specialized = std::true_type;

    static constexpr
    testing::DSgMax combine(
        const testing::DSgMax& _a,
        const testing::DSgMax& _b
    )
    {
        return testing::DSgMax{ (_a.v > _b.v) ? _a.v : _b.v };
    }
};

// semigroup_traits<DSgFirst>  -- left projection (constexpr)
template<>
struct semigroup_traits<testing::DSgFirst, void>
{
    using is_specialized = std::true_type;

    static
    constexpr testing::DSgFirst
    combine(
        const testing::DSgFirst& _a,
        const testing::DSgFirst& /*_b*/
    )
    {
        return _a;
    }
};

// semigroup_traits<T : is_modular>  -- the family instance, via the _Enable
// SFINAE hook: one partial specialisation covers z3 and z5, taking the modulus
// from the type.
template<typename _Type>
struct semigroup_traits<
    _Type,
    typename std::enable_if<testing::is_modular<_Type>::value>::type>
{
    using is_specialized = std::true_type;

    static constexpr
    _Type combine(
        const _Type& _a,
        const _Type& _b
    )
    {
        return _Type{ (_a.v + _b.v) % _Type::modulus };
    }
};

// semigroup_traits<DSgNoMarker>  -- combine present, is_specialized ABSENT
// (so is_semigroup<DSgNoMarker> must be false).
template<>
struct semigroup_traits<testing::DSgNoMarker, void>
{
    static
    testing::DSgNoMarker
    combine(
        const testing::DSgNoMarker& _a,
        const testing::DSgNoMarker& /*_b*/
    )
    {
        return _a;
    }
};
#endif  // !DTEST_SPEC_MODE  (fixture instances)


NS_END  // djinterp


#endif  // DJINTERP_TEST_SEMIGROUP_TESTS_
