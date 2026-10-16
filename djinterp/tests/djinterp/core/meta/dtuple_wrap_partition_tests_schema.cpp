/*******************************************************************************
* djinterp [test]                         dtuple_wrap_partition_tests_schema.cpp
*
*   Section IV of dtuple_wrap_partition.hpp: the two partition_wrap_except
* specializations and their precedence -- the schema may be a bare typename
* pack (primary) OR a single std::tuple<...> (more-specialized, unwrapped to
* the pack form).  Coverage:
*     - the bare-pack primary and the single-tuple specialization produce the
*       SAME result for the same logical schema (incl. embedded passthroughs);
*     - a pack that CONTAINS a tuple routes to the PRIMARY: the inner tuple is
*       an ordinary non-passthrough slot and gets wrapped -- it is NOT the
*       single-tuple unwrapping case;
*     - a lone std::tuple<> routes to the specialization and yields
*       std::tuple<> -- NOT the hard error the primary would raise if it treated
*       the empty tuple as one slot (proving the specialization is preferred);
*     - the empty pack yields std::tuple<>.
*
*
* path:      /tests/djinterp/core/meta/dtuple_wrap_partition_tests_schema.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "dtuple_wrap_partition_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace pwp_test_types;


// =========================================================================
// I.   BARE PACK  (primary)
// =========================================================================

static_assert(std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>>,
                           std::tuple<group<slot<0>, slot<1>, slot<2>>>>::value,
              "pack: primary form");
static_assert(std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, slot<2>, slot<3>>,
                           std::tuple<group<slot<0>, slot<1>>, pass_a, group<slot<2>, slot<3>>>>::value,
              "pack: primary with passthrough");


// =========================================================================
// II.  SINGLE std::tuple  (specialization unwraps)
// =========================================================================

static_assert(std::is_same<partition_grouped<3, std::tuple<slot<0>, slot<1>, slot<2>>>,
                           std::tuple<group<slot<0>, slot<1>, slot<2>>>>::value,
              "tuple: single-tuple schema unwraps to the pack form");
static_assert(std::is_same<partition_grouped<2, std::tuple<slot<0>, slot<1>, pass_a>>,
                           std::tuple<group<slot<0>, slot<1>>, pass_a>>::value,
              "tuple: single-tuple schema carries passthroughs too");
static_assert(std::is_same<partition_grouped<3, std::tuple<pass_a, pass_b>>,
                           std::tuple<pass_a, pass_b>>::value,
              "tuple: single tuple of only passthroughs");

// the two forms agree for the same logical schema
static_assert(std::is_same<partition_grouped<3, std::tuple<slot<0>, slot<1>, slot<2>>>,
                           partition_grouped<3, slot<0>, slot<1>, slot<2>>>::value,
              "tuple == pack for the same logical schema");


// =========================================================================
// III. PACK CONTAINING A TUPLE  (primary; the tuple is a slot)
// =========================================================================
//   Two schema args -> the specialization (which needs exactly one tuple) does
// NOT match, so the primary runs and the inner tuple is an ordinary slot.

static_assert(std::is_same<partition_grouped<2, std::tuple<slot<9>>, slot<0>>,
                           std::tuple<group<std::tuple<slot<9>>, slot<0>>>>::value,
              "pack-with-tuple: inner tuple is a wrapped slot, not unwrapped");

// a single tuple that CONTAINS a tuple: the outer unwraps one level, leaving
// the inner tuple as a slot -- same result as the two-arg pack above.
static_assert(std::is_same<partition_grouped<2, std::tuple<std::tuple<slot<9>>, slot<0>>>,
                           std::tuple<group<std::tuple<slot<9>>, slot<0>>>>::value,
              "single tuple-of-tuple: outer unwrapped once, inner stays a slot");


// =========================================================================
// IV.  EMPTY FORMS
// =========================================================================

// lone empty tuple -> specialization -> empty result (NOT a hard error; the
// primary would treat std::tuple<> as one slot and fail the multiple-of-N rule)
static_assert(std::is_same<partition_grouped<3, std::tuple<>>, std::tuple<>>::value,
              "empty tuple -> specialization -> std::tuple<> (proves precedence)");

// empty pack -> empty result
static_assert(std::is_same<partition_grouped<3>, std::tuple<>>::value,
              "empty pack -> std::tuple<>");
static_assert(std::is_same<partition_grouped<1>, std::tuple<>>::value,
              "empty pack -> std::tuple<> (N irrelevant)");


// =========================================================================
// V.   RUN-TIME MIRRORS
// =========================================================================

bool
tests_pwp_schema_pack()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>>,
                              std::tuple<group<slot<0>, slot<1>, slot<2>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, slot<2>, slot<3>>,
                              std::tuple<group<slot<0>, slot<1>>, pass_a,
                                         group<slot<2>, slot<3>>>>::value;
    return _ok;
}

bool
tests_pwp_schema_single_tuple()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, std::tuple<slot<0>, slot<1>, slot<2>>>,
                              std::tuple<group<slot<0>, slot<1>, slot<2>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<2, std::tuple<slot<0>, slot<1>, pass_a>>,
                              std::tuple<group<slot<0>, slot<1>>, pass_a>>::value;
    _ok = _ok && std::is_same<partition_grouped<3, std::tuple<slot<0>, slot<1>, slot<2>>>,
                              partition_grouped<3, slot<0>, slot<1>, slot<2>>>::value;
    return _ok;
}

bool
tests_pwp_schema_tuple_precedence()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<2, std::tuple<slot<9>>, slot<0>>,
                              std::tuple<group<std::tuple<slot<9>>, slot<0>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<2, std::tuple<std::tuple<slot<9>>, slot<0>>>,
                              std::tuple<group<std::tuple<slot<9>>, slot<0>>>>::value;
    return _ok;
}

bool
tests_pwp_schema_empty_tuple()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, std::tuple<>>, std::tuple<>>::value;
    return _ok;
}

bool
tests_pwp_schema_empty_pack()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3>, std::tuple<>>::value;
    _ok = _ok && std::is_same<partition_grouped<1>, std::tuple<>>::value;
    return _ok;
}


NS_END  // testing
NS_END  // djinterp
