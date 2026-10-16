/*******************************************************************************
* djinterp [core]                       runtime_container_traits_tests_class.cpp
*
*
* path:      /tests/djinterp/core/container/traits/runtime_container_traits_tests_class.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "runtime_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. The aggregate snapshot
// ===========================================================================
//   runtime_container_class restates the axis for one type in five members.
// It introduces no new detection, so the tests here are about WIRING - that
// each member is fed from the trait it claims to restate, and that no two
// members are crossed.
//
//   Note that four of the five are booleans of the same type, so a crossed
// pair would compile silently and would show only on a carrier where the two
// disagree.  Every check below therefore uses a carrier chosen so that the
// member under test differs from its neighbours.


/*
tests_runtime_container_class_signals
  Verifies the two members that restate structural probes.
  Tests the following:
  - is_container_shape restates has_size_accessor_signal
  - has_allocator restates has_allocator_alias
  - the two are independent, shown on carriers where they disagree in each
    direction
  - both are usable in a constant expression
  - note the aggregate exposes no member for the reserve probe, so
    requires_runtime disjoins a signal that cannot be read back out
*/
bool
tests_runtime_container_class_signals()
{
    bool ok = true;

    // the shape member
    D_RT_CHECK(runtime_container_class<rt_vector>::is_container_shape
                   == has_size_accessor_signal<rt_vector>::value);
    D_RT_CHECK(runtime_container_class<rt_pair>::is_container_shape
                   == has_size_accessor_signal<rt_pair>::value);
    D_RT_CHECK(runtime_container_class<rt_no_size>::is_container_shape
                   == has_size_accessor_signal<rt_no_size>::value);

    // read literally
    D_RT_CHECK(runtime_container_class<rt_vector>::is_container_shape
                   == true);
    D_RT_CHECK(runtime_container_class<rt_pair>::is_container_shape == false);

    // the allocator member
    D_RT_CHECK(runtime_container_class<rt_vector>::has_allocator
                   == has_allocator_alias<rt_vector>::value);
    D_RT_CHECK(runtime_container_class<rt_array>::has_allocator
                   == has_allocator_alias<rt_array>::value);
    D_RT_CHECK(runtime_container_class<rt_allocator_private>::has_allocator
                   == has_allocator_alias<rt_allocator_private>::value);

    // read literally
    D_RT_CHECK(runtime_container_class<rt_vector>::has_allocator == true);
    D_RT_CHECK(runtime_container_class<rt_array>::has_allocator  == false);

    // the two are independent, in both directions
    D_RT_CHECK(   ( runtime_container_class<rt_shaped>::is_container_shape
                        == true )
               && ( runtime_container_class<rt_shaped>::has_allocator
                        == false ));
    D_RT_CHECK(   ( runtime_container_class<
                        rt_allocator_no_shape>::is_container_shape == false )
               && ( runtime_container_class<
                        rt_allocator_no_shape>::has_allocator == true ));

    // constexpr
    D_RT_STATIC_CHECK(runtime_container_class<rt_vector>::is_container_shape);
    D_RT_STATIC_CHECK(runtime_container_class<rt_vector>::has_allocator);

    return ok;
}

/*
tests_runtime_container_class_verdicts
  Verifies the two members that restate the umbrellas.
  Tests the following:
  - is_runtime restates is_runtime_container
  - requires_runtime restates requires_runtime_storage
  - the two are independent, shown on a carrier from each of the two mixed
    quadrants - which is exactly where a crossed pair would show
  - both are usable in a constant expression
*/
bool
tests_runtime_container_class_verdicts()
{
    bool ok = true;

    // the runtime verdict
    D_RT_CHECK(runtime_container_class<rt_vector>::is_runtime
                   == is_runtime_container<rt_vector>::value);
    D_RT_CHECK(runtime_container_class<rt_array>::is_runtime
                   == is_runtime_container<rt_array>::value);
    D_RT_CHECK(runtime_container_class<rt_shaped>::is_runtime
                   == is_runtime_container<rt_shaped>::value);
    D_RT_CHECK(runtime_container_class<rt_category_none>::is_runtime
                   == is_runtime_container<rt_category_none>::value);

    // the storage verdict
    D_RT_CHECK(runtime_container_class<rt_vector>::requires_runtime
                   == requires_runtime_storage<rt_vector>::value);
    D_RT_CHECK(runtime_container_class<rt_deque>::requires_runtime
                   == requires_runtime_storage<rt_deque>::value);
    D_RT_CHECK(runtime_container_class<rt_array>::requires_runtime
                   == requires_runtime_storage<rt_array>::value);
    D_RT_CHECK(runtime_container_class<rt_reserve>::requires_runtime
                   == requires_runtime_storage<rt_reserve>::value);

    // read literally
    D_RT_CHECK(   ( runtime_container_class<rt_vector>::is_runtime == true )
               && ( runtime_container_class<rt_vector>::requires_runtime
                        == true ));
    D_RT_CHECK(   ( runtime_container_class<rt_array>::is_runtime == false )
               && ( runtime_container_class<rt_array>::requires_runtime
                        == false ));

    // the mixed quadrants, where a crossed pair would show
    D_RT_CHECK(   ( runtime_container_class<rt_shaped>::is_runtime == true )
               && ( runtime_container_class<rt_shaped>::requires_runtime
                        == false ));
    D_RT_CHECK(   ( runtime_container_class<
                        rt_allocator_no_shape>::is_runtime == false )
               && ( runtime_container_class<
                        rt_allocator_no_shape>::requires_runtime == true ));

    // constexpr
    D_RT_STATIC_CHECK(runtime_container_class<rt_vector>::is_runtime);
    D_RT_STATIC_CHECK(runtime_container_class<rt_vector>::requires_runtime);
    D_RT_STATIC_CHECK(
        runtime_container_class<rt_array>::is_runtime == false);

    return ok;
}

/*
tests_runtime_container_class_life
  Verifies the one non-boolean member - the shared lifetime verdict itself.
  Tests the following:
  - life restates container_lifetime, on a carrier for each of the four
    verdicts
  - it is the raw verdict and not a reduction of it: the aggregate carries the
    distinction between `none` and `compile_time` that is_runtime collapses,
    so a reader can tell WHY a type is not a runtime container
  - it is usable in a constant expression, and names itself through
    lifetime_name
*/
bool
tests_runtime_container_class_life()
{
    bool ok = true;

    // restating the shared verdict
    D_RT_CHECK(runtime_container_class<rt_vector>::life
                   == container_lifetime<rt_vector>::value);
    D_RT_CHECK(runtime_container_class<rt_array>::life
                   == container_lifetime<rt_array>::value);
    D_RT_CHECK(runtime_container_class<rt_category_none>::life
                   == container_lifetime<rt_category_none>::value);
    D_RT_CHECK(runtime_container_class<
                   rt_category_compile_time_sized>::life
                   == container_lifetime<
                          rt_category_compile_time_sized>::value);

    // a carrier for each of the four verdicts
    D_RT_CHECK(runtime_container_class<rt_category_none>::life
                   == lifetime::none);
    D_RT_CHECK(runtime_container_class<
                   rt_category_compile_time_sized>::life
                   == lifetime::compile_time);
    D_RT_CHECK(runtime_container_class<rt_vector>::life == lifetime::runtime);
    D_RT_CHECK(runtime_container_class<rt_array>::life  == lifetime::both);

    // the raw verdict, not a reduction: three verdicts collapse to
    // is_runtime == false, and life keeps them apart
    D_RT_CHECK(   ( runtime_container_class<rt_category_none>::is_runtime
                        == runtime_container_class<rt_array>::is_runtime )
               && ( runtime_container_class<rt_category_none>::life
                        != runtime_container_class<rt_array>::life ));
    D_RT_CHECK(   ( runtime_container_class<
                        rt_category_compile_time_sized>::is_runtime
                        == runtime_container_class<rt_array>::is_runtime )
               && ( runtime_container_class<
                        rt_category_compile_time_sized>::life
                        != runtime_container_class<rt_array>::life ));

    // and is_runtime is exactly is_runtime_only of it, given the shape
    D_RT_CHECK(runtime_container_class<rt_vector>::is_runtime
                   == (    runtime_container_class<
                               rt_vector>::is_container_shape
                        && is_runtime_only(
                               runtime_container_class<rt_vector>::life) ));
    D_RT_CHECK(runtime_container_class<rt_array>::is_runtime
                   == (    runtime_container_class<
                               rt_array>::is_container_shape
                        && is_runtime_only(
                               runtime_container_class<rt_array>::life) ));

    // constexpr, and it names itself
    D_RT_STATIC_CHECK(runtime_container_class<rt_vector>::life
                          == lifetime::runtime);
    D_RT_CHECK(rt_life_is(runtime_container_class<rt_vector>::life,
                          "runtime"));
    D_RT_CHECK(rt_life_is(runtime_container_class<rt_array>::life, "both"));

    return ok;
}

/*
tests_runtime_container_class_agreement
  Verifies that the aggregate restates the standalone traits and does not
  shadow them - the property that makes it safe to read either one.
  Tests the following:
  - every member agrees with its standalone trait, across carriers spanning all
    four quadrants of the two umbrellas and all four lifetime verdicts
  - the agreement holds on the carriers where the two umbrellas DISAGREE, which
    is where a mis-wiring would most easily hide
  - the agreement holds where a probe is defeated by access control or by an
    argument type, so the aggregate inherits the probes' exact reach
*/
bool
tests_runtime_container_class_agreement()
{
    bool ok = true;

    // spanning the quadrants
    D_RT_CHECK(runtime_container_class<rt_vector>::is_runtime
                   == is_runtime_container<rt_vector>::value);
    D_RT_CHECK(runtime_container_class<rt_shaped>::requires_runtime
                   == requires_runtime_storage<rt_shaped>::value);
    D_RT_CHECK(runtime_container_class<rt_reserve_no_shape>::is_runtime
                   == is_runtime_container<rt_reserve_no_shape>::value);
    D_RT_CHECK(runtime_container_class<rt_reserve_no_shape>::requires_runtime
                   == requires_runtime_storage<rt_reserve_no_shape>::value);
    D_RT_CHECK(runtime_container_class<rt_pair>::is_runtime
                   == is_runtime_container<rt_pair>::value);

    // spanning the lifetime verdicts
    D_RT_CHECK(runtime_container_class<rt_literal_plain>::life
                   == container_lifetime<rt_literal_plain>::value);
    D_RT_CHECK(runtime_container_class<rt_constexpr_size>::life
                   == container_lifetime<rt_constexpr_size>::value);
    D_RT_CHECK(runtime_container_class<rt_category_compile_time>::life
                   == container_lifetime<
                          rt_category_compile_time>::value);
    D_RT_CHECK(runtime_container_class<rt_category_runtime>::life
                   == container_lifetime<rt_category_runtime>::value);

    // where the two umbrellas disagree
    D_RT_CHECK(   ( runtime_container_class<
                        rt_allocator_no_shape>::is_runtime
                        == is_runtime_container<
                               rt_allocator_no_shape>::value )
               && ( runtime_container_class<
                        rt_allocator_no_shape>::requires_runtime
                        == requires_runtime_storage<
                               rt_allocator_no_shape>::value )
               && ( runtime_container_class<
                        rt_allocator_no_shape>::is_runtime
                        != runtime_container_class<
                               rt_allocator_no_shape>::requires_runtime ));

    // where a probe is defeated by access control
    D_RT_CHECK(runtime_container_class<rt_size_private>::is_container_shape
                   == has_size_accessor_signal<rt_size_private>::value);
    D_RT_CHECK(runtime_container_class<rt_allocator_private>::has_allocator
                   == has_allocator_alias<rt_allocator_private>::value);

    // where a probe is defeated by an argument type
    D_RT_CHECK(runtime_container_class<rt_reserve_wrong_arg>::requires_runtime
                   == requires_runtime_storage<rt_reserve_wrong_arg>::value);
    D_RT_CHECK(runtime_container_class<rt_reserve_wrong_arg>::requires_runtime
                   == false);

    return ok;
}

/*
tests_runtime_container_class_cv_ref
  Verifies that the aggregate honours the clean_t contract.
  Tests the following:
  - every member is identical across all seven cv- and reference-qualified
    spellings, on carriers where the members disagree with one another
  - the lifetime member likewise
  - the volatile spellings in particular
*/
bool
tests_runtime_container_class_cv_ref()
{
    bool ok = true;

    // the booleans
    D_RT_CHECK(runtime_container_class<const rt_vector>::is_runtime
                   == runtime_container_class<rt_vector>::is_runtime);
    D_RT_CHECK(runtime_container_class<volatile rt_vector>::is_runtime
                   == runtime_container_class<rt_vector>::is_runtime);
    D_RT_CHECK(runtime_container_class<const volatile rt_vector>::is_runtime
                   == runtime_container_class<rt_vector>::is_runtime);
    D_RT_CHECK(runtime_container_class<rt_vector&>::is_runtime
                   == runtime_container_class<rt_vector>::is_runtime);
    D_RT_CHECK(runtime_container_class<const rt_vector&>::is_runtime
                   == runtime_container_class<rt_vector>::is_runtime);
    D_RT_CHECK(runtime_container_class<rt_vector&&>::is_runtime
                   == runtime_container_class<rt_vector>::is_runtime);

    D_RT_CHECK(runtime_container_class<volatile rt_array>::is_runtime
                   == runtime_container_class<rt_array>::is_runtime);
    D_RT_CHECK(runtime_container_class<
                   const volatile rt_shaped&>::is_container_shape
                   == runtime_container_class<rt_shaped>::is_container_shape);
    D_RT_CHECK(runtime_container_class<volatile rt_deque>::has_allocator
                   == runtime_container_class<rt_deque>::has_allocator);
    D_RT_CHECK(runtime_container_class<
                   const rt_allocator_no_shape&>::requires_runtime
                   == runtime_container_class<
                          rt_allocator_no_shape>::requires_runtime);

    // the lifetime member
    D_RT_CHECK(runtime_container_class<const rt_array&>::life
                   == runtime_container_class<rt_array>::life);
    D_RT_CHECK(runtime_container_class<volatile rt_vector>::life
                   == runtime_container_class<rt_vector>::life);
    D_RT_CHECK(runtime_container_class<
                   const volatile rt_category_none&>::life
                   == runtime_container_class<rt_category_none>::life);
    D_RT_CHECK(runtime_container_class<rt_constexpr_size&&>::life
                   == runtime_container_class<rt_constexpr_size>::life);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
