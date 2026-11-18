/*******************************************************************************
* djinterp [cpp]                      bounded_container_traits_tests_summary.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/bounded_container_traits_tests_summary.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <cstring>
#include <list>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
#include "bounded_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_capacity_bound_enumerators
  The capacity_bound enum itself.
  Tests the following:
  - it is a scoped enum (no implicit conversion to its underlying type), so a
    caller cannot accidentally compare it against an int
  - the three enumerators are distinct
  - their VALUES are 0, 1, 2.  Pinned deliberately: the enum is the thing a
    diagnostic, a serialized trait dump or an agent-facing summary carries, and
    a reordering would silently change every recorded value.  If the enumerators
    are ever reordered, this is the check that says so
*/
bool tests_capacity_bound_enumerators()
{
    bool ok = true;

    using underlying = std::underlying_type<DCapacityBound>::type;

    D_BCT_CHECK(std::is_enum<DCapacityBound>::value);
    D_BCT_CHECK(!std::is_convertible<DCapacityBound, underlying>::value);

    D_BCT_CHECK(DCapacityBound::unknown != DCapacityBound::bounded);
    D_BCT_CHECK(DCapacityBound::bounded != DCapacityBound::unbounded);
    D_BCT_CHECK(DCapacityBound::unknown != DCapacityBound::unbounded);

    D_BCT_CHECK(static_cast<int>(DCapacityBound::unknown) == 0);
    D_BCT_CHECK(static_cast<int>(DCapacityBound::bounded) == 1);
    D_BCT_CHECK(static_cast<int>(DCapacityBound::unbounded) == 2);

    return ok;
}

/*
tests_capacity_bound_name
  capacity_bound_name - the stable spelling.
  Tests the following:
  - each enumerator maps to its own spelling
  - the spellings are distinct and none is null
  - the function is constexpr: its result is usable as a template argument, not
    merely computable at run time
  - the function is noexcept
*/
bool tests_capacity_bound_name()
{
    bool ok = true;

    D_BCT_CHECK(std::strcmp(capacity_bound_name(DCapacityBound::unknown),
                            "unknown") == 0);
    D_BCT_CHECK(std::strcmp(capacity_bound_name(DCapacityBound::bounded),
                            "bounded") == 0);
    D_BCT_CHECK(std::strcmp(capacity_bound_name(DCapacityBound::unbounded),
                            "unbounded") == 0);

    D_BCT_CHECK(capacity_bound_name(DCapacityBound::unknown) != nullptr);
    D_BCT_CHECK(capacity_bound_name(DCapacityBound::bounded) != nullptr);
    D_BCT_CHECK(capacity_bound_name(DCapacityBound::unbounded) != nullptr);

    // constexpr: the call survives constant evaluation.  A constexpr local is
    // the check - if capacity_bound_name ever stops being constexpr, this stops
    // compiling, which is the point
    D_CONSTEXPR const char* bounded_name =
        capacity_bound_name(DCapacityBound::bounded);

    D_BCT_CHECK(std::strcmp(bounded_name, "bounded") == 0);

    // noexcept
    D_BCT_CHECK(noexcept(capacity_bound_name(DCapacityBound::bounded)));

    return ok;
}

/*
tests_capacity_bound_name_out_of_range
  What capacity_bound_name does with a value that is not an enumerator.
  Tests the following:
  - a cast-in out-of-range value returns "unbounded", NOT "unknown" and not a
    null pointer

  The implementation is a two-test ternary chain whose final arm is the
  fall-through, so anything that is neither `unknown` nor `bounded` reads as
  `unbounded`.  For a scoped enum this is only reachable by an explicit cast,
  so it is not a live bug - but it is the wrong default: the fall-through arm
  should be the one that means "I do not know".  Pinned so that swapping the
  chain to `_b == unbounded ? "unbounded" : "unknown"` is a visible change.
*/
bool tests_capacity_bound_name_out_of_range()
{
    bool ok = true;

    const DCapacityBound out_of_range = static_cast<DCapacityBound>(42);

    D_BCT_CHECK(capacity_bound_name(out_of_range) != nullptr);
    D_BCT_CHECK(std::strcmp(capacity_bound_name(out_of_range),
                            "unbounded") == 0);
    D_BCT_CHECK(std::strcmp(capacity_bound_name(out_of_range),
                            "unknown") != 0);

    return ok;
}

/*
tests_capacity_bound_of
  capacity_bound_of - the three-way classification.
  Tests the following:
  - bounded   : any type with promoting evidence, whether or not it is a
                container (a bare std::tuple classifies as bounded)
  - unbounded : a container without such evidence
  - unknown   : everything else - an empty type, a non-container, a type whose
                only member is max_size(), a type whose size() is not
                const-callable
  - every branch of the nested conditional is exercised
*/
bool tests_capacity_bound_of()
{
    bool ok = true;

    // bounded
    D_BCT_CHECK(capacity_bound_of<std::array<int, 3>>::value ==
                DCapacityBound::bounded);
    D_BCT_CHECK(capacity_bound_of<DIntervalContainer>::value ==
                DCapacityBound::bounded);
    D_BCT_CHECK(capacity_bound_of<DFixedCapacity>::value ==
                DCapacityBound::bounded);
    D_BCT_CHECK(capacity_bound_of<extent_static_member>::value ==
                DCapacityBound::bounded);
    D_BCT_CHECK(capacity_bound_of<std::tuple<int, char>>::value ==
                DCapacityBound::bounded);
    D_BCT_CHECK(capacity_bound_of<DEverySignal>::value ==
                DCapacityBound::bounded);

    // unbounded
    D_BCT_CHECK(capacity_bound_of<std::vector<int>>::value ==
                DCapacityBound::unbounded);
    D_BCT_CHECK(capacity_bound_of<std::list<int>>::value ==
                DCapacityBound::unbounded);
    D_BCT_CHECK(capacity_bound_of<DSizeConst>::value ==
                DCapacityBound::unbounded);
    D_BCT_CHECK(capacity_bound_of<DGrowable>::value ==
                DCapacityBound::unbounded);

    // unknown
    D_BCT_CHECK(capacity_bound_of<no_signal>::value ==
                DCapacityBound::unknown);
    D_BCT_CHECK(capacity_bound_of<DMaxSizeOnly>::value ==
                DCapacityBound::unknown);
    D_BCT_CHECK(capacity_bound_of<DSizeNonconst>::value ==
                DCapacityBound::unknown);
    D_BCT_CHECK(capacity_bound_of<int>::value ==
                DCapacityBound::unknown);
    D_BCT_CHECK(capacity_bound_of<void>::value ==
                DCapacityBound::unknown);

    return ok;
}

/*
tests_capacity_bound_of_type
  The nested ::type and the capacity_bound_of_t alias.
  Tests the following:
  - ::type is exactly std::integral_constant<capacity_bound, value>
  - capacity_bound_of_t<T> names that same type
  - the constant's ::value round-trips to the trait's ::value

  Note the shape: capacity_bound_of PROVIDES a ::type but does not DERIVE from
  it, so it carries no ::value_type and no conversion.  It is a value trait, not
  a bool trait, and that is a deliberate difference - but it means
  `capacity_bound_of<T>{}` converts to nothing.  See tests_trait_shape.
*/
bool tests_capacity_bound_of_type()
{
    bool ok = true;

    using array_type  = capacity_bound_of<std::array<int, 3>>::type;
    using vector_type = capacity_bound_of<std::vector<int>>::type;
    using none_type   = capacity_bound_of<no_signal>::type;

    D_BCT_CHECK((std::is_same<array_type,
                              std::integral_constant<DCapacityBound,
                                  DCapacityBound::bounded>>::value));
    D_BCT_CHECK((std::is_same<vector_type,
                              std::integral_constant<DCapacityBound,
                                  DCapacityBound::unbounded>>::value));
    D_BCT_CHECK((std::is_same<none_type,
                              std::integral_constant<DCapacityBound,
                                  DCapacityBound::unknown>>::value));

    // the _t alias names the same type
    D_BCT_CHECK((std::is_same<capacity_bound_of_t<std::array<int, 3>>,
                              array_type>::value));
    D_BCT_CHECK((std::is_same<capacity_bound_of_t<std::vector<int>>,
                              vector_type>::value));

    // and the constant round-trips
    D_BCT_CHECK(capacity_bound_of_t<std::array<int, 3>>::value ==
                capacity_bound_of<std::array<int, 3>>::value);
    D_BCT_CHECK(capacity_bound_of_t<std::vector<int>>::value ==
                capacity_bound_of<std::vector<int>>::value);
    D_BCT_CHECK(capacity_bound_of_t<no_signal>::value ==
                capacity_bound_of<no_signal>::value);

    return ok;
}

/*
tests_capacity_bound_of_agrees
  The summary must be a faithful re-statement of the two predicates.
  Tests the following:
  - value == bounded    <=>  is_bounded_container
  - value == unbounded  <=>  is_unbounded_container
  - value == unknown    <=>  neither

  These are three biconditionals, checked over the whole fixture set.  They are
  what makes the summary safe to use INSTEAD of the predicates rather than
  alongside them - and they are the checks that break if a fourth promoting
  signal is ever added to is_bounded_container but not threaded through here.
*/
bool tests_capacity_bound_of_agrees()
{
    bool ok = true;

    #define D_BCT_AGREES(...)                                                 \
        D_BCT_CHECK((capacity_bound_of<__VA_ARGS__>::value ==                        \
                     DCapacityBound::bounded) ==                              \
                    is_bounded_container<__VA_ARGS__>::value);                       \
        D_BCT_CHECK((capacity_bound_of<__VA_ARGS__>::value ==                        \
                     DCapacityBound::unbounded) ==                            \
                    is_unbounded_container<__VA_ARGS__>::value);                     \
        D_BCT_CHECK((capacity_bound_of<__VA_ARGS__>::value ==                        \
                     DCapacityBound::unknown) ==                              \
                    (!is_bounded_container<__VA_ARGS__>::value &&                    \
                     !is_unbounded_container<__VA_ARGS__>::value))

    D_BCT_AGREES(std::array<int, 3>);
    D_BCT_AGREES(std::vector<int>);
    D_BCT_AGREES(std::list<int>);
    D_BCT_AGREES(DIntervalContainer);
    D_BCT_AGREES(DFixedCapacity);
    D_BCT_AGREES(DGrowable);
    D_BCT_AGREES(DGrowableNullaryReserve);
    D_BCT_AGREES(extent_static_member);
    D_BCT_AGREES(DBoundsBoth);
    D_BCT_AGREES(DTupleSizeConformer);
    D_BCT_AGREES(DSizeConst);
    D_BCT_AGREES(DSizeNonconst);
    D_BCT_AGREES(DMaxSizeOnly);
    D_BCT_AGREES(DSizedAndMaxed);
    D_BCT_AGREES(DEverySignal);
    D_BCT_AGREES(no_signal);
    D_BCT_AGREES(int);

    #undef D_BCT_AGREES

    return ok;
}

/*
tests_capacity_bound_of_value_companion
  capacity_bound_of_v - an inline variable on C++17+, a plain variable template
  on C++14, and absent below that.
  Tests the following:
  - the shorthand carries the same value as the trait
  - it is typed capacity_bound, not bool: this is the one `_v` in the header
    that is not a boolean, so a caller cannot fold it into a boolean expression
    by accident
*/
bool tests_capacity_bound_of_value_companion()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_BCT_CHECK(capacity_bound_of_v<std::array<int, 3>> ==
                capacity_bound_of<std::array<int, 3>>::value);
    D_BCT_CHECK(capacity_bound_of_v<std::vector<int>> ==
                capacity_bound_of<std::vector<int>>::value);
    D_BCT_CHECK(capacity_bound_of_v<no_signal> ==
                capacity_bound_of<no_signal>::value);

    D_BCT_CHECK(capacity_bound_of_v<std::array<int, 3>> ==
                DCapacityBound::bounded);
    D_BCT_CHECK(capacity_bound_of_v<std::vector<int>> ==
                DCapacityBound::unbounded);
    D_BCT_CHECK(capacity_bound_of_v<no_signal> == DCapacityBound::unknown);

    D_BCT_CHECK((std::is_same<decltype(capacity_bound_of_v<std::vector<int>>),
                              const DCapacityBound>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
