/*******************************************************************************
* djinterp [core]                      bounded_container_traits_tests_domain.cpp
*
*
* path:      /tests/djinterp/core/container/traits/bounded_container_traits_tests_domain.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <vector>
// djinterp
#include "bounded_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_domain_bounded
  is_domain_bounded_container - the orthogonal sub-axis.
  Tests the following:
  - fires exactly when both static endpoints are present
  - one endpoint is not an interval
  - private endpoints are not endpoints
  - a fixed array is size-bounded and domain-FREE, so it must not fire
*/
bool
tests_domain_bounded()
{
    bool ok = true;

    D_BCT_CHECK(is_domain_bounded_container<bounds_both>::value);
    D_BCT_CHECK(is_domain_bounded_container<interval_container>::value);
    D_BCT_CHECK(is_domain_bounded_container<bounds_and_growable>::value);
    D_BCT_CHECK(is_domain_bounded_container<every_signal>::value);

    D_BCT_CHECK(!is_domain_bounded_container<bounds_lower_only>::value);
    D_BCT_CHECK(!is_domain_bounded_container<bounds_upper_only>::value);
    D_BCT_CHECK(!is_domain_bounded_container<bounds_private>::value);
    D_BCT_CHECK(!is_domain_bounded_container<no_signal>::value);

    D_BCT_CHECK(!is_domain_bounded_container<std::array<int, 3>>::value);
    D_BCT_CHECK(!is_domain_bounded_container<std::vector<int>>::value);

    return ok;
}

/*
tests_domain_implies_bounded
  Domain-bounded implies capacity-bounded.
  Tests the following:
  - the implication holds on every domain-bounded fixture: a finite domain has
    a finite carrier, so the static-bounds signal promotes on BOTH axes.  The
    header states this; the two traits read the same signal, so the implication
    is structural rather than coincidental, and this is the check that keeps it
    that way if either ever grows a second clause
  - it is a one-way implication: the converse is tested in tests_domain_orthogonal
*/
bool
tests_domain_implies_bounded()
{
    bool ok = true;

    D_BCT_CHECK(is_domain_bounded_container<bounds_both>::value &&
                is_bounded_container<bounds_both>::value);

    D_BCT_CHECK(is_domain_bounded_container<interval_container>::value &&
                is_bounded_container<interval_container>::value);

    D_BCT_CHECK(is_domain_bounded_container<bounds_and_growable>::value &&
                is_bounded_container<bounds_and_growable>::value);

    D_BCT_CHECK(is_domain_bounded_container<every_signal>::value &&
                is_bounded_container<every_signal>::value);

    // stated as an implication: domain -> capacity, i.e. never domain && !capacity
    D_BCT_CHECK(!(is_domain_bounded_container<bounds_both>::value &&
                  !is_bounded_container<bounds_both>::value));
    D_BCT_CHECK(!(is_domain_bounded_container<bounds_and_growable>::value &&
                  !is_bounded_container<bounds_and_growable>::value));

    return ok;
}

/*
tests_domain_orthogonal
  The two sub-axes are independent - all four quadrants are populated.
  Tests the following:
  - bounded + domain-bounded     : an interval
  - bounded + domain-free        : std::array, a bare extent (the header's own
                                   example: "a fixed array is size-bounded but
                                   domain-free")
  - unbounded + domain-free      : std::vector
  - unbounded + domain-bounded   : EMPTY, and necessarily so - the domain signal
                                   promotes capacity, so nothing can be
                                   domain-bounded and capacity-unbounded at once.
                                   The axes are orthogonal in the header's
                                   framing but not independent in this
                                   implementation, and that is the honest
                                   statement of it
*/
bool
tests_domain_orthogonal()
{
    bool ok = true;

    // bounded + domain-bounded
    D_BCT_CHECK(is_bounded_container<interval_container>::value &&
                is_domain_bounded_container<interval_container>::value);

    // bounded + domain-free
    D_BCT_CHECK(is_bounded_container<std::array<int, 3>>::value &&
                !is_domain_bounded_container<std::array<int, 3>>::value);
    D_BCT_CHECK(is_bounded_container<extent_static_member>::value &&
                !is_domain_bounded_container<extent_static_member>::value);
    D_BCT_CHECK(is_bounded_container<fixed_capacity>::value &&
                !is_domain_bounded_container<fixed_capacity>::value);

    // unbounded + domain-free
    D_BCT_CHECK(is_unbounded_container<std::vector<int>>::value &&
                !is_domain_bounded_container<std::vector<int>>::value);
    D_BCT_CHECK(is_unbounded_container<size_const>::value &&
                !is_domain_bounded_container<size_const>::value);

    // unbounded + domain-bounded is unreachable: the domain signal promotes
    // capacity, so domain-bounded && unbounded is a contradiction
    D_BCT_CHECK(!(is_domain_bounded_container<interval_container>::value &&
                  is_unbounded_container<interval_container>::value));
    D_BCT_CHECK(!(is_domain_bounded_container<bounds_and_growable>::value &&
                  is_unbounded_container<bounds_and_growable>::value));

    return ok;
}

/*
tests_domain_value_companion
  is_domain_bounded_container_v.
  Tests the following:
  - the `_v` shorthand tracks the trait on a positive and a negative subject
  - unlike the two capacity verdicts, this trait DOES derive from
    std::integral_constant, so it is a bool trait in the standard sense - see
    tests_trait_shape, where that asymmetry is the finding
*/
bool
tests_domain_value_companion()
{
    bool ok = true;

    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_domain_bounded_container,
                                      bounds_both));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_domain_bounded_container,
                                      std::array<int, 3>));
    D_BCT_CHECK(D_TEST_TRAIT_V_AGREES(is_domain_bounded_container, no_signal));

    D_BCT_CHECK(is_domain_bounded_container_v<interval_container>);
    D_BCT_CHECK(!is_domain_bounded_container_v<std::vector<int>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
