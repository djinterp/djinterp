/*******************************************************************************
* djinterp [testing]               container_memory_strategy_tests_container.cpp
*
* Section: the container binding.
*   container_memory_strategy.hpp - shape detectors (cms_has_*), DCmsInferKind,
*                                   DStrategyResolver, DContainerMemoryStrategy,
*                                   container_uses_*_storage_v, and the container
*                                   concepts
*
*   This is where the axis meets real containers.  It answers two questions that
* stay deliberately SEPARATE.  Resolution: does a concrete memory strategy fall
* out of the container's shape?  It runs a four-step precedence - an explicit
* member alias, then an allocator_type, then a compile-time extent, then
* nothing - and the tests walk each step and, crucially, the boundary between
* steps one and two (an alias must beat an allocator_type).  Inference: what
* storage discipline does the container's shape imply?  This is answered even
* when NO strategy resolves, so a growable container with no strategy alias
* resolves to void yet still infers dynamic storage.  The tests assert that
* independence directly, because collapsing the two answers would be the easy
* mistake.
*   The extent path deliberately forgets the true N (it tags the marker with 0),
* so the tests check the value_type of the resolved marker, not its extent.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_memory_strategy_tests_container.cpp
*                              container_memory_strategy_tests_container.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <array>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
#include "container_memory_strategy_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_container_shape_detection
  the cms_has_* shape detectors (DContainerMemoryStrategy internal I).
  Tests the following:
  - each detector fires on exactly the members it names, across the container
    fixtures: extent, tuple_size, capacity, reserve, allocator_type, value_type,
    and the explicit strategy alias
  - reserve is detected independently of capacity: the fixed-capacity fixture
    has capacity WITHOUT reserve, the growable one has both
  - the detectors agree on real standard containers: vector has capacity +
    reserve + allocator, array has tuple_size, tuple has tuple_size but no
    value_type
*/
bool tests_container_shape_detection()
{

    // --- extent ---
    D_CN_CHECK(internal::cms_has_extent<DCnContainerExtent>::value);
    D_CN_CHECK(internal::cms_has_extent<DCnContainerExtentNoVt>::value);
    D_CN_CHECK(!internal::cms_has_extent<DCnContainerGrowable>::value);

    // --- tuple_size ---
    D_CN_CHECK(internal::cms_has_tuple_size<std::array<int, 5>>::value);
    D_CN_CHECK((internal::cms_has_tuple_size<std::tuple<int, char>>::value));
    D_CN_CHECK(!internal::cms_has_tuple_size<DCnContainerExtent>::value);

    // --- capacity / reserve, independently ---
    D_CN_CHECK(internal::cms_has_capacity<DCnContainerGrowable>::value);
    D_CN_CHECK(internal::cms_has_reserve<DCnContainerGrowable>::value);
    D_CN_CHECK(internal::cms_has_capacity<DCnContainerFixedCap>::value);
    D_CN_CHECK(!internal::cms_has_reserve<DCnContainerFixedCap>::value);

    // --- allocator_type ---
    D_CN_CHECK(internal::cms_has_allocator<DCnContainerAllocator>::value);
    D_CN_CHECK(internal::cms_has_allocator<DCnContainerAllocAndAlias>::value);
    D_CN_CHECK(!internal::cms_has_allocator<DCnContainerExtent>::value);

    // --- value_type ---
    D_CN_CHECK(internal::cms_has_value_type<DCnContainerExtent>::value);
    D_CN_CHECK(!internal::cms_has_value_type<DCnContainerExtentNoVt>::value);
    D_CN_CHECK(!internal::cms_has_value_type<DCnContainerOpaque>::value);

    // --- explicit strategy alias ---
    D_CN_CHECK(internal::cms_has_strategy_alias<DCnContainerExplicit>::value);
    D_CN_CHECK(
        internal::cms_has_strategy_alias<DCnContainerAllocAndAlias>::value);
    D_CN_CHECK(!internal::cms_has_strategy_alias<DCnContainerAllocator>::value);

    // --- real standard containers ---
    D_CN_CHECK(internal::cms_has_capacity<std::vector<int>>::value);
    D_CN_CHECK(internal::cms_has_reserve<std::vector<int>>::value);
    D_CN_CHECK(internal::cms_has_allocator<std::vector<int>>::value);
    D_CN_CHECK(internal::cms_has_tuple_size<std::array<int, 5>>::value);
    D_CN_CHECK(!internal::cms_has_allocator<std::array<int, 5>>::value);
    D_CN_CHECK((!internal::cms_has_value_type<std::tuple<int, char>>::value));

    return true;
}


/*
tests_container_strategy_resolution
  DStrategyResolver / DContainerMemoryStrategy::type / resolved.
  Tests the following:
  - each precedence path resolves to the expected strategy TYPE: an alias yields
    the aliased strategy, an allocator_type yields an allocator strategy over it,
    an extent yields a static_extent marker over the (possibly defaulted)
    value_type, and no shape yields void
  - the extent path forgets N (tags the marker with 0), so the resolved marker's
    value_type is what is checked, not its extent
  - resolved is true exactly when the resolved type is non-void
  - real containers resolve as expected: vector -> allocator strategy, array ->
    static_extent<int, 0>, tuple -> static_extent<unsigned char, 0>
*/
bool tests_container_strategy_resolution()
{

    // --- alias path ---
    D_CN_CHECK((std::is_same<container_memory_strategy_t<DCnContainerExplicit>,
                             DCnElementDynamic>::value));
    // --- allocator path ---
    D_CN_CHECK((std::is_same<container_memory_strategy_t<DCnContainerAllocator>,
                   DAllocatorMemoryStrategy<std::allocator<char>>>::value));
    // --- extent path, with and without a value_type ---
    D_CN_CHECK((std::is_same<container_memory_strategy_t<DCnContainerExtent>,
                             DStaticExtentStrategy<double, 0>>::value));
    D_CN_CHECK(
        (std::is_same<container_memory_strategy_t<DCnContainerExtentNoVt>,
                      DStaticExtentStrategy<unsigned char, 0>>::value));
    // --- no shape: void ---
    D_CN_CHECK(std::is_void<
                   container_memory_strategy_t<DCnContainerGrowable>>::value);
    D_CN_CHECK(std::is_void<
                   container_memory_strategy_t<DCnContainerOpaque>>::value);

    // --- resolved is exactly non-void ---
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerExplicit>::resolved);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerExtent>::resolved);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerGrowable>::resolved);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerOpaque>::resolved);

    // --- real containers ---
    D_CN_CHECK((std::is_same<container_memory_strategy_t<std::vector<int>>,
                   DAllocatorMemoryStrategy<std::allocator<int>>>::value));
    D_CN_CHECK((std::is_same<container_memory_strategy_t<std::array<int, 5>>,
                             DStaticExtentStrategy<int, 0>>::value));
    D_CN_CHECK((std::is_same<container_memory_strategy_t<std::tuple<int, char>>,
                             DStaticExtentStrategy<unsigned char, 0>>::value));
    D_CN_CHECK(DContainerMemoryStrategy<std::vector<int>>::resolved);
    D_CN_CHECK(DContainerMemoryStrategy<std::array<int, 5>>::resolved);

    return true;
}


/*
tests_container_strategy_precedence
  the four-step precedence, especially the step-1 / step-2 boundary.
  Tests the following:
  - a container with BOTH an explicit alias and an allocator_type resolves to
    the ALIAS (step 1 beats step 2), and the two resolver specializations are
    not ambiguous - the whole reason the alias branch is written to exclude the
    allocator branch
  - the resolved strategy carries the alias's discipline, not the allocator's
    (static + pointer-stable here, not dynamic)
  - with the alias removed, the same shape would resolve through the allocator:
    the allocator-only fixture proves step 2 in isolation
  - pointer_stable on the binding reflects the RESOLVED strategy: true for the
    alias-to-a-stable-strategy case, false for the allocator case
*/
bool tests_container_strategy_precedence()
{

    // --- step 1 beats step 2: alias wins over allocator_type ---
    D_CN_CHECK(internal::cms_has_strategy_alias<DCnContainerAllocAndAlias>
                   ::value);
    D_CN_CHECK(internal::cms_has_allocator<DCnContainerAllocAndAlias>
                   ::value);
    D_CN_CHECK(
        (std::is_same<container_memory_strategy_t<DCnContainerAllocAndAlias>,
                      DCnElementStaticMonotonic>::value));

    // --- the resolved strategy carries the ALIAS's discipline ---
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerAllocAndAlias>
                   ::is_static);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerAllocAndAlias>
                   ::pointer_stable);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerAllocAndAlias>
                   ::is_dynamic);

    // --- step 2 in isolation: allocator-only resolves to allocator strategy ---
    D_CN_CHECK(
        (std::is_same<container_memory_strategy_t<DCnContainerAllocator>,
                      DAllocatorMemoryStrategy<std::allocator<char>>>::value));
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerAllocator>::is_dynamic);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerAllocator>
                   ::pointer_stable);

    // --- step 3 in isolation: extent-only resolves to the static marker ---
    D_CN_CHECK((std::is_same<container_memory_strategy_t<DCnContainerExtent>,
                             DStaticExtentStrategy<double, 0>>::value));
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerExtent>::is_static);

    // --- pointer_stable reflects the resolved strategy ---
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerExplicit>::pointer_stable
                   == is_pointer_stable_strategy<DCnElementDynamic>::value);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerExtent>::pointer_stable
                   == is_pointer_stable_strategy<DStaticExtentStrategy<
                          double, 0>>::value);

    return true;
}


/*
tests_container_kind_inference
  DCmsInferKind and the declared-kind-wins rule.
  Tests the following:
  - shape inference maps directly: extent or tuple_size -> static, capacity +
    reserve -> dynamic, capacity without reserve -> fixed, nothing -> unknown
  - when a strategy resolves AND declares a kind, the DECLARED kind is used (the
    allocator fixture resolves to a dynamic strategy, and the binding is dynamic)
  - THE INDEPENDENCE: a growable container resolves to VOID (no strategy) yet its
    binding kind is dynamic, inferred from the shape.  Resolution being false
    does not make the kind unknown.  Likewise fixed-capacity resolves to void but
    infers fixed
  - the opaque container is unknown and resolves to void: no shape, no strategy
  - is_static / is_fixed / is_dynamic on the binding track its kind and stay
    mutually exclusive
*/
bool tests_container_kind_inference()
{

    // --- shape inference maps directly ---
    D_CN_CHECK(internal::DCmsInferKind<DCnContainerExtent>::value
                   == storage_kind::static_storage);
    D_CN_CHECK(internal::DCmsInferKind<std::array<int, 5>>::value
                   == storage_kind::static_storage);
    D_CN_CHECK(internal::DCmsInferKind<DCnContainerGrowable>::value
                   == storage_kind::dynamic_storage);
    D_CN_CHECK(internal::DCmsInferKind<DCnContainerFixedCap>::value
                   == storage_kind::fixed_storage);
    D_CN_CHECK(internal::DCmsInferKind<DCnContainerOpaque>::value
                   == storage_kind::unknown);

    // --- declared kind wins when a strategy resolves ---
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerAllocator>::kind
                   == storage_kind::dynamic_storage);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerExplicit>::kind
                   == storage_kind::dynamic_storage);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerExtent>::kind
                   == storage_kind::static_storage);

    // --- INDEPENDENCE: resolves to void, yet the shape infers a kind ---
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerGrowable>::resolved);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerGrowable>::kind
                   == storage_kind::dynamic_storage);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerGrowable>::is_dynamic);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerFixedCap>::resolved);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerFixedCap>::kind
                   == storage_kind::fixed_storage);
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerFixedCap>::is_fixed);

    // --- opaque: unknown and void ---
    D_CN_CHECK(DContainerMemoryStrategy<DCnContainerOpaque>::kind
                   == storage_kind::unknown);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerOpaque>::resolved);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerOpaque>::is_static);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerOpaque>::is_fixed);
    D_CN_CHECK(!DContainerMemoryStrategy<DCnContainerOpaque>::is_dynamic);

    // --- is_static / is_fixed / is_dynamic mutually exclusive on the binding ---
    D_CN_CHECK((
        int(DContainerMemoryStrategy<std::vector<int>>::is_static)
        + int(DContainerMemoryStrategy<std::vector<int>>::is_fixed)
        + int(DContainerMemoryStrategy<std::vector<int>>::is_dynamic)) == 1);
    D_CN_CHECK((
        int(DContainerMemoryStrategy<DCnContainerOpaque>::is_static)
        + int(DContainerMemoryStrategy<DCnContainerOpaque>::is_fixed)
        + int(DContainerMemoryStrategy<DCnContainerOpaque>::is_dynamic)) == 0);

    return true;
}


/*
tests_container_strategy_concepts
  container_uses_*_storage_v and the container concepts.
  Tests the following:
  - the container_uses_*_storage_v variables agree with the binding's is_static /
    is_fixed / is_dynamic
  - strategy_resolvable_container is satisfied exactly when a strategy resolves:
    vector and array yes, growable and opaque no (even though growable infers a
    kind, no strategy resolves)
  - static_storage_container_strategy and dynamic_storage_container_strategy
    track the inferred discipline: array is static, vector is dynamic
  - the concepts agree with the underlying trait values
*/
bool tests_container_strategy_concepts()
{

    // --- uses_*_storage_v agree with the binding ---
    D_CN_CHECK(container_uses_dynamic_storage_v<std::vector<int>>
                   == DContainerMemoryStrategy<std::vector<int>>::is_dynamic);
    D_CN_CHECK(container_uses_static_storage_v<std::array<int, 5>>
                   == DContainerMemoryStrategy<std::array<int, 5>>::is_static);
    D_CN_CHECK(container_uses_fixed_storage_v<DCnContainerFixedCap>
                   == DContainerMemoryStrategy<DCnContainerFixedCap>
                          ::is_fixed);

    // --- strategy_resolvable_container: exactly when a strategy resolves ---
    D_CN_CHECK(strategy_resolvable_container<std::vector<int>>);
    D_CN_CHECK(strategy_resolvable_container<std::array<int, 5>>);
    D_CN_CHECK(strategy_resolvable_container<DCnContainerExplicit>);
    D_CN_CHECK(!strategy_resolvable_container<DCnContainerGrowable>);
    D_CN_CHECK(!strategy_resolvable_container<DCnContainerOpaque>);

    // --- discipline concepts track the inferred kind ---
    D_CN_CHECK(static_storage_container_strategy<std::array<int, 5>>);
    D_CN_CHECK(!static_storage_container_strategy<std::vector<int>>);
    D_CN_CHECK(dynamic_storage_container_strategy<std::vector<int>>);
    D_CN_CHECK(!dynamic_storage_container_strategy<std::array<int, 5>>);
    //     a growable container infers dynamic even without a resolved strategy
    D_CN_CHECK(dynamic_storage_container_strategy<DCnContainerGrowable>);

    // --- the concepts agree with the underlying trait values ---
    D_CN_CHECK(strategy_resolvable_container<std::vector<int>>
                   == DContainerMemoryStrategy<std::vector<int>>::resolved);
    D_CN_CHECK(static_storage_container_strategy<std::array<int, 5>>
                   == container_uses_static_storage_v<std::array<int, 5>>);
    D_CN_CHECK(dynamic_storage_container_strategy<std::vector<int>>
                   == container_uses_dynamic_storage_v<std::vector<int>>);

    return true;
}


NS_END  // testing
NS_END  // djinterp
