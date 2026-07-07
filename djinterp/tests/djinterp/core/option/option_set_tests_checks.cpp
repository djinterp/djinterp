/******************************************************************************
* djinterp [test]                                  option_set_tests_checks.cpp
*
*   Section III of the option_set.hpp suite: the construction-time checks that
* run over the flattened (post-expansion) option tuple.
*
*     internal::are_all_options<...>   - every entry satisfies is_option_v.
*     internal::all_same_type<...>     - every type in the pack is identical
*                                        (applied to the entries' key_types).
*     internal::run_set_checks<_Tuple> - the aggregate: fires three
*                                        static_asserts (all-options,
*                                        key_type-uniform, keys-unique) and
*                                        exposes ::value == true on success.
*
*   run_set_checks reports violations with hard static_asserts, so its REJECT
* behavior is a compile error by design and is not a runtime test here (the
* three underlying predicates are tested directly for their false cases, which
* IS where the detection logic lives).  What is asserted for run_set_checks is
* its two success paths: the empty flat tuple (an all-passthrough set) and a
* valid flat tuple.  These predicates are `internal::`; the tests reach them
* through a TU-local alias.
*
*
* path:      /tests/djinterp/core/option/option_set_tests_checks.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_set_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace  // internal-helper alias, local to this TU
{
    namespace ic = ::djinterp::internal;
}

// checks_are_all_options
//   are_all_options is true for the empty pack and for an all-option pack, and
// false as soon as any entry fails is_option_v (a non-option struct, or a bare
// built-in type).
bool
checks_are_all_options()
{
    constexpr bool ok =
        ic::are_all_options<>::value                                            &&
        ic::are_all_options<option<os_key::a>, option<os_key::b, int>>::value   &&
        !ic::are_all_options<option<os_key::a>, os_not_option>::value           &&
        !ic::are_all_options<int>::value;

    static_assert(ok, "are_all_options: true iff every entry satisfies is_option_v");
    return ok;
}

// checks_all_same_type
//   all_same_type is true for empty / single / all-identical packs and false
// on the first mismatch - shown on raw types and on the key_type enums the
// check is actually applied to.
bool
checks_all_same_type()
{
    constexpr bool ok =
        ic::all_same_type<>::value                                              &&
        ic::all_same_type<int>::value                                           &&
        ic::all_same_type<int, int, int>::value                                 &&
        !ic::all_same_type<int, int, char>::value                               &&
        ic::all_same_type<os_key, os_key>::value                                &&
        !ic::all_same_type<os_key, os_key2>::value;

    static_assert(ok, "all_same_type: true iff every type in the pack is identical");
    return ok;
}

// checks_run_set_checks_pass
//   run_set_checks exposes ::value == true on its two success paths: the empty
// flat tuple (an all-passthrough set) and a valid flat tuple of same-keyed,
// unique-keyed options.  (Violations are hard static_asserts by design and are
// exercised out of suite.)
bool
checks_run_set_checks_pass()
{
    constexpr bool ok =
        ic::run_set_checks<std::tuple<>>::value                                 &&
        ic::run_set_checks<std::tuple<option<os_key::a>,
                                      option<os_key::b>>>::value;

    static_assert(ok, "run_set_checks: ::value == true for empty and valid flat tuples");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_checks_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "III. construction checks";
    b.descriptor = "are_all_options / all_same_type predicates and run_set_checks success paths";
    b.tests = {
        { "checks_are_all_options",
          "are_all_options true iff every entry satisfies is_option_v",
          &checks_are_all_options },
        { "checks_all_same_type",
          "all_same_type true iff every type in the pack is identical",
          &checks_all_same_type },
        { "checks_run_set_checks_pass",
          "run_set_checks ::value true for empty and valid flat tuples",
          &checks_run_set_checks_pass },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
