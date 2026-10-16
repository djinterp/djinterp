/*******************************************************************************
* djinterp [core]                     bounded_container_traits_tests_signals.cpp
*
*
* path:      /tests/djinterp/core/container/traits/bounded_container_traits_tests_signals.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <list>
#include <tuple>
#include <utility>
#include <vector>
// djinterp
#include "bounded_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_signal_fixed_extent
  has_fixed_extent_signal on the shape it was written for.
  Tests the following:
  - fires on a static compile-time `extent`
  - fires on an extent of ZERO (a capacity of 0 is a bound, not the absence
    of one - a probe that reached for the value rather than its existence
    would get this backwards)
  - declines on a type with no `extent` at all
  - declines on the standard growable containers
*/
bool
tests_signal_fixed_extent()
{
    bool ok = true;

    D_BCT_CHECK(has_fixed_extent_signal<extent_static_member>::value);
    D_BCT_CHECK(has_fixed_extent_signal<extent_zero>::value);

    D_BCT_CHECK(!has_fixed_extent_signal<no_signal>::value);
    D_BCT_CHECK(!has_fixed_extent_signal<size_const>::value);
    D_BCT_CHECK(!has_fixed_extent_signal<std::vector<int>>::value);
    D_BCT_CHECK(!has_fixed_extent_signal<std::array<int, 3>>::value);

    return ok;
}

/*
tests_signal_fixed_extent_shapes
  What ELSE `decltype(clean_t<_Type>::extent)` accepts.
  Tests the following:
  - a NON-STATIC data member named `extent` fires the signal
  - a STATIC member FUNCTION named `extent` fires the signal
  - an ENUMERATOR named `extent` fires the signal
  - a non-static member function does NOT (naming it without an object is
    ill-formed)
  - a nested TYPE does NOT (decltype takes an expression, not a type)
  - a PRIVATE extent does NOT (access checking is part of substitution, so the
    failure is a substitution failure and not a diagnostic)

  The first three are false positives against the trait's stated contract - "a
  static `extent` constant - the djinterp compile-time fixed-capacity
  convention".  They are pinned rather than papered over: a type carrying an
  ordinary field called `extent` is currently classified BOUNDED.  Tightening
  the probe to `std::integral_constant<std::size_t, clean_t<_Type>::extent>`
  (or an is_const/is_integral guard on the decltype) would reject the data
  member and the function while keeping the constant and the enumerator; these
  three checks are the ones that would then flip.
*/
bool
tests_signal_fixed_extent_shapes()
{
    bool ok = true;

    // fires - intended
    D_BCT_CHECK(has_fixed_extent_signal<extent_static_member>::value);

    // fires - NOT intended (see the comment above)
    D_BCT_CHECK(has_fixed_extent_signal<extent_data_member>::value);
    D_BCT_CHECK(has_fixed_extent_signal<extent_static_function>::value);
    D_BCT_CHECK(has_fixed_extent_signal<extent_enumerator>::value);

    // declines - correctly
    D_BCT_CHECK(!has_fixed_extent_signal<extent_member_function>::value);
    D_BCT_CHECK(!has_fixed_extent_signal<extent_nested_type>::value);
    D_BCT_CHECK(!has_fixed_extent_signal<extent_private>::value);

    return ok;
}

/*
tests_signal_tuple_size
  has_tuple_size_signal - the fixed-size-aggregate protocol.
  Tests the following:
  - fires on std::array, including the zero-length one
  - fires on std::tuple, including the empty one, and on std::pair
  - fires on a user type that specializes std::tuple_size
  - declines on the growable containers - the std::tuple_size primary template
    is declared and never defined, so `::value` on it is an error in the
    immediate context and the probe reports false instead of diagnosing
  - declines on non-class types, including a raw array (which has an extent,
    but not a std::tuple_size specialization - see tests_hostile_types_rejected)
*/
bool
tests_signal_tuple_size()
{
    bool ok = true;

    D_BCT_CHECK(has_tuple_size_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(has_tuple_size_signal<std::array<int, 0>>::value);
    D_BCT_CHECK(has_tuple_size_signal<std::tuple<int, char>>::value);
    D_BCT_CHECK(has_tuple_size_signal<std::tuple<>>::value);
    D_BCT_CHECK(has_tuple_size_signal<std::pair<int, char>>::value);
    D_BCT_CHECK(has_tuple_size_signal<tuple_size_conformer>::value);
    D_BCT_CHECK(has_tuple_size_signal<every_signal>::value);

    D_BCT_CHECK(!has_tuple_size_signal<std::vector<int>>::value);
    D_BCT_CHECK(!has_tuple_size_signal<std::list<int>>::value);
    D_BCT_CHECK(!has_tuple_size_signal<no_signal>::value);
    D_BCT_CHECK(!has_tuple_size_signal<int>::value);
    D_BCT_CHECK(!has_tuple_size_signal<int[3]>::value);

    return ok;
}

/*
tests_signal_static_bounds
  has_static_bounds_signal - the closed-interval domain I = [x, y, z].
  Tests the following:
  - requires BOTH endpoints: neither half alone fires
  - fires when both are present
  - declines when both are present but private
*/
bool
tests_signal_static_bounds()
{
    bool ok = true;

    D_BCT_CHECK(has_static_bounds_signal<bounds_both>::value);
    D_BCT_CHECK(has_static_bounds_signal<interval_container>::value);
    D_BCT_CHECK(has_static_bounds_signal<every_signal>::value);

    D_BCT_CHECK(!has_static_bounds_signal<bounds_lower_only>::value);
    D_BCT_CHECK(!has_static_bounds_signal<bounds_upper_only>::value);
    D_BCT_CHECK(!has_static_bounds_signal<bounds_private>::value);
    D_BCT_CHECK(!has_static_bounds_signal<no_signal>::value);

    return ok;
}

/*
tests_signal_capacity
  has_capacity_signal - a capacity() accessor on a CONST lvalue.
  Tests the following:
  - fires on a const-qualified capacity()
  - declines on a capacity() that is not const-callable.  The probe binds a
    const lvalue, so this narrowing is deliberate; it is pinned because it is
    the kind of thing a later "simplification" of the probe would silently undo
  - fires on std::vector (which has one) and declines on std::list (which does
    not) - capacity() is not a universal container member
*/
bool
tests_signal_capacity()
{
    bool ok = true;

    D_BCT_CHECK(has_capacity_signal<capacity_const>::value);
    D_BCT_CHECK(has_capacity_signal<fixed_capacity>::value);
    D_BCT_CHECK(has_capacity_signal<growable>::value);
    D_BCT_CHECK(has_capacity_signal<std::vector<int>>::value);

    D_BCT_CHECK(!has_capacity_signal<capacity_nonconst>::value);
    D_BCT_CHECK(!has_capacity_signal<std::list<int>>::value);
    D_BCT_CHECK(!has_capacity_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(!has_capacity_signal<no_signal>::value);

    return ok;
}

/*
tests_signal_reserve
  has_reserve_signal - the growability anti-signal.
  Tests the following:
  - fires on reserve(size_type) for a non-const lvalue
  - fires on reserve(int): the probe passes a size_t and only requires the call
    to be well-formed, so the parameter type is not pinned
  - fires on a const-qualified reserve: a const member is callable on the
    non-const lvalue the probe binds
  - does NOT fire on a nullary reserve() - the probe passes an argument.  This
    is the edge that lets growable_nullary_reserve be misclassified as bounded
    (see tests_bounded_capacity_disjunct)
  - fires on std::vector, declines on std::list
*/
bool
tests_signal_reserve()
{
    bool ok = true;

    D_BCT_CHECK(has_reserve_signal<reserve_sized>::value);
    D_BCT_CHECK(has_reserve_signal<reserve_int>::value);
    D_BCT_CHECK(has_reserve_signal<reserve_const>::value);
    D_BCT_CHECK(has_reserve_signal<growable>::value);
    D_BCT_CHECK(has_reserve_signal<std::vector<int>>::value);

    D_BCT_CHECK(!has_reserve_signal<reserve_nullary>::value);
    D_BCT_CHECK(!has_reserve_signal<growable_nullary_reserve>::value);
    D_BCT_CHECK(!has_reserve_signal<capacity_const>::value);
    D_BCT_CHECK(!has_reserve_signal<std::list<int>>::value);
    D_BCT_CHECK(!has_reserve_signal<no_signal>::value);

    return ok;
}

/*
tests_signal_max_size
  has_max_size_signal - informational, never promoting.
  Tests the following:
  - fires on a const-callable max_size()
  - fires on BOTH std::array and std::vector - which is precisely why it cannot
    promote: it does not separate kappa = N from kappa = inf
  - a type carrying max_size() and nothing else is not thereby a container (see
    tests_capacity_bound_of: max_size_only classifies as `unknown`)
*/
bool
tests_signal_max_size()
{
    bool ok = true;

    D_BCT_CHECK(has_max_size_signal<max_size_only>::value);
    D_BCT_CHECK(has_max_size_signal<sized_and_maxed>::value);
    D_BCT_CHECK(has_max_size_signal<every_signal>::value);

    // the reason it is not a promoting signal: both sides of the axis have it
    D_BCT_CHECK(has_max_size_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(has_max_size_signal<std::vector<int>>::value);
    D_BCT_CHECK(has_max_size_signal<std::list<int>>::value);

    D_BCT_CHECK(!has_max_size_signal<no_signal>::value);
    D_BCT_CHECK(!has_max_size_signal<size_const>::value);

    return ok;
}

/*
tests_signal_size
  has_size_signal - the "looks like a container" guard.
  Tests the following:
  - fires on a const-callable size()
  - declines on a size() that is not const-callable
  - fires on every standard container
  - declines on the interval-only and extent-only fixtures, which have a bound
    but no size() - the guard is what separates "bounded container" from
    "bounded thing"
*/
bool
tests_signal_size()
{
    bool ok = true;

    D_BCT_CHECK(has_size_signal<size_const>::value);
    D_BCT_CHECK(has_size_signal<interval_container>::value);
    D_BCT_CHECK(has_size_signal<std::array<int, 3>>::value);
    D_BCT_CHECK(has_size_signal<std::vector<int>>::value);
    D_BCT_CHECK(has_size_signal<std::list<int>>::value);

    D_BCT_CHECK(!has_size_signal<size_nonconst>::value);
    D_BCT_CHECK(!has_size_signal<extent_static_member>::value);
    D_BCT_CHECK(!has_size_signal<bounds_both>::value);
    D_BCT_CHECK(!has_size_signal<max_size_only>::value);
    D_BCT_CHECK(!has_size_signal<no_signal>::value);

    return ok;
}

/*
tests_signal_value_companions
  Every signal trait carries a `_v` companion emitted by D_TYPE_TRAIT_VALUE_BOOL.
  Tests the following:
  - each of the seven `_v` shorthands agrees with its trait's ::value, on both
    a positive and a negative subject

  A stale `_v` - one left pointing at a renamed or re-based trait - inverts the
  answer at every call site that prefers the shorthand, and nothing else in the
  build says so.  D_TEST_TRAIT_V_AGREES is the check that does.
*/
bool
tests_signal_value_companions()
{
    bool ok = true;

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_fixed_extent_signal,
                                      extent_static_member));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_fixed_extent_signal, no_signal));

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_tuple_size_signal,
                                      std::array<int, 3>));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_tuple_size_signal, no_signal));

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_static_bounds_signal, bounds_both));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_static_bounds_signal, no_signal));

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_capacity_signal, capacity_const));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_capacity_signal, no_signal));

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_reserve_signal, reserve_sized));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_reserve_signal, no_signal));

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_max_size_signal, max_size_only));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_max_size_signal, no_signal));

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_size_signal, size_const));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(has_size_signal, no_signal));

    // and the shorthands really are the values, not merely equal booleans
    D_BCT_CHECK(has_fixed_extent_signal_v<extent_static_member>);
    D_BCT_CHECK(has_tuple_size_signal_v<std::array<int, 3>>);
    D_BCT_CHECK(has_static_bounds_signal_v<bounds_both>);
    D_BCT_CHECK(has_capacity_signal_v<capacity_const>);
    D_BCT_CHECK(has_reserve_signal_v<reserve_sized>);
    D_BCT_CHECK(has_max_size_signal_v<max_size_only>);
    D_BCT_CHECK(has_size_signal_v<size_const>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
