/******************************************************************************
* djinterp [test]                                   option_tests_sentinels.cpp
*
*   Section I of the option.hpp suite: the reserved arg-search sentinels.
*
*     arg_not_found   - an empty tag type reserved as an arg-search MISS
*                       result, distinguishable from any real tag.
*     arg_npos        - a reserved std::size_t sentinel index for an
*                       arg-search miss (mirrors std::string::npos).
*
*   The framework's own tag-driven arg search was retired (2026.05.27); both
* survive purely as sentinels for user-defined arg-search helpers, so the
* contract to verify is small and exact: the type's shape and distinctness,
* and the value's magnitude and type.
*
*
* path:      /tests/djinterp/core/option/option_tests_sentinels.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
// djinterp
#include "option_tests.hpp"


NS_DJINTERP
NS_TESTING


// sentinel_arg_not_found_shape
//   arg_not_found is an empty, stateless class type that is trivially and
// nothrow default-constructible - the minimal "miss" tag it is documented to
// be.
bool
sentinel_arg_not_found_shape()
{
    constexpr bool ok =
        std::is_class<arg_not_found>::value                              &&
        std::is_empty<arg_not_found>::value                              &&
        std::is_trivially_default_constructible<arg_not_found>::value    &&
        std::is_nothrow_default_constructible<arg_not_found>::value      &&
        std::is_standard_layout<arg_not_found>::value;

    static_assert(ok, "arg_not_found must be an empty, trivially-constructible tag type");
    return ok;
}

// sentinel_arg_not_found_distinct
//   arg_not_found is its own distinct type - not void, not an option<>, and
// not confusable with the numeric sentinel's type.  A miss tag that collided
// with a real type would defeat its purpose.
bool
sentinel_arg_not_found_distinct()
{
    constexpr bool ok =
        !std::is_same<arg_not_found, void>::value                        &&
        !std::is_same<arg_not_found, std::size_t>::value                 &&
        !std::is_same<arg_not_found, option<opt_key::alpha>>::value      &&
        !is_option_v<arg_not_found>;

    static_assert(ok, "arg_not_found must be a distinct, non-option sentinel type");
    return ok;
}

// sentinel_arg_npos_value
//   arg_npos equals the all-ones std::size_t - static_cast<size_t>(-1),
// SIZE_MAX, and numeric_limits<size_t>::max() are three spellings of the same
// value and all must agree.
bool
sentinel_arg_npos_value()
{
    constexpr bool ok =
        (arg_npos == static_cast<std::size_t>(-1))                       &&
        (arg_npos == SIZE_MAX)                                           &&
        (arg_npos == (std::numeric_limits<std::size_t>::max)());

    static_assert(ok, "arg_npos must be the maximal std::size_t value");
    return ok;
}

// sentinel_arg_npos_type
//   arg_npos is a std::size_t (up to its top-level const), so it composes
// with size-typed arithmetic without a narrowing surprise.
bool
sentinel_arg_npos_type()
{
    constexpr bool ok =
        std::is_same<std::remove_const_t<decltype(arg_npos)>, std::size_t>::value &&
        std::is_integral<decltype(arg_npos)>::value                     &&
        std::is_unsigned<decltype(arg_npos)>::value;

    static_assert(ok, "arg_npos must be of type std::size_t");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_sentinels_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "sentinels";
    b.descriptor = "arg_not_found tag + arg_npos index (reserved arg-search miss sentinels)";
    b.tests      = {
        { "sentinel_arg_not_found_shape",
          "arg_not_found is an empty, trivially default-constructible tag type",
          &sentinel_arg_not_found_shape },
        { "sentinel_arg_not_found_distinct",
          "arg_not_found is a distinct, non-option sentinel type",
          &sentinel_arg_not_found_distinct },
        { "sentinel_arg_npos_value",
          "arg_npos == (size_t)-1 == SIZE_MAX == numeric_limits<size_t>::max()",
          &sentinel_arg_npos_value },
        { "sentinel_arg_npos_type",
          "arg_npos is a std::size_t (unsigned integral)",
          &sentinel_arg_npos_type },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
