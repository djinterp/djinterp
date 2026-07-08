/******************************************************************************
* djinterp [test]                                 option_builder_tests_wrap.cpp
*
*   Section I of the option_builder.hpp suite: the per-chunk wrapper.
*
*     internal::has_value_member   - SFINAE probe for a static constexpr
*                                    ::value member (validates a chunk's first
*                                    slot is a key carrier).
*     option_partition_wrap<_Key, _Rest...>
*                                  - rebuilds a chunk as option<_Key::value,
*                                    _Rest...>: the first slot's ::value becomes
*                                    the key NTTP, the rest are stored verbatim
*                                    as opaque option args.
*
*   The wrapper's guard - a hard static_assert when the first slot is not a key
* carrier - is a compile error by design and is exercised out of suite; the
* has_value_member probe underneath it is what is tested directly (true for a
* key carrier and for any type with a ::value member, false otherwise).  The
* "opaque args" contract is checked by wrapping arbitrary slot types the
* framework attaches no meaning to.  has_value_member is internal:: and is
* reached through a TU-local alias.
*
*
* path:      /tests/djinterp/core/option/option_builder_tests_wrap.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace  // internal-helper alias, local to this TU
{
    namespace ic = ::djinterp::internal;
}

// wrap_has_value_member
//   has_value_member is true for a key carrier and for any type exposing a
// static ::value (e.g. std::true_type), and false for a type without one - a
// plain struct or a built-in.
bool
wrap_has_value_member()
{
    constexpr bool ok =
        ic::has_value_member<ob_key<ob_enum::a>>::value                        &&
        ic::has_value_member<std::true_type>::value                            &&
        !ic::has_value_member<ob_not_key>::value                               &&
        !ic::has_value_member<int>::value;

    static_assert(ok, "has_value_member: true iff the type exposes a static ::value");
    return ok;
}

// wrap_key_and_args
//   option_partition_wrap lifts the first slot's ::value to the key NTTP and
// keeps the remaining slots verbatim as opaque option args.
bool
wrap_key_and_args()
{
    constexpr bool ok =
        std::is_same<option_partition_wrap<ob_key<ob_enum::a>, ob_slot<5>, ob_desc<1>>,
                     option<ob_enum::a, ob_slot<5>, ob_desc<1>>>::value;

    static_assert(ok, "option_partition_wrap: option<first::value, rest...> (rest kept verbatim)");
    return ok;
}

// wrap_key_only
//   a chunk of just a key carrier wraps to a unary option (no args).
bool
wrap_key_only()
{
    constexpr bool ok =
        std::is_same<option_partition_wrap<ob_key<ob_enum::b>>,
                     option<ob_enum::b>>::value;

    static_assert(ok, "option_partition_wrap: a lone key carrier -> unary option");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_builder_wrap_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "I. option_partition_wrap";
    b.descriptor = "has_value_member probe and the key-extracting chunk wrapper";
    b.tests = {
        { "wrap_has_value_member",
          "has_value_member: true iff the type exposes a static ::value",
          &wrap_has_value_member },
        { "wrap_key_and_args",
          "option_partition_wrap: option<first::value, rest...> verbatim",
          &wrap_key_and_args },
        { "wrap_key_only",
          "option_partition_wrap: lone key carrier -> unary option",
          &wrap_key_only },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
