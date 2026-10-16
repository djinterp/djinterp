/*******************************************************************************
* djinterp [core]              runtime_container_traits_tests_classification.cpp
*
*
* path:      /tests/djinterp/core/container/traits/runtime_container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "runtime_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  Classification umbrellas
// ===========================================================================
//   Two traits, and they answer different questions.  is_runtime_container is
// a Lifetime statement conjoined with a shape test; requires_runtime_storage
// is a Storage statement with no shape test at all.  The header calls the
// second a "stronger signal", which it is not - see
// tests_requires_runtime_storage_is_independent.


/*
tests_is_runtime_container
  Verifies the umbrella on carriers of each kind.
  Tests the following:
  - a container-shaped type with a runtime-only lifetime is a runtime
    container, reached structurally and through the lifetime_category opt-in
  - the standard containers, all but the fixed array
  - the conjunction re-derived from its two parts on every carrier, so neither
    half can drift
  - types failing on either half, and on both
*/
bool
tests_is_runtime_container()
{
    bool ok = true;

    // true
    D_RT_CHECK(is_runtime_container<rt_shaped>::value == true);
    D_RT_CHECK(is_runtime_container<rt_literal_plain>::value == true);
    D_RT_CHECK(is_runtime_container<rt_category_runtime>::value == true);
    D_RT_CHECK(is_runtime_container<rt_vector>::value == true);
    D_RT_CHECK(is_runtime_container<rt_string>::value == true);
    D_RT_CHECK(is_runtime_container<rt_deque>::value  == true);
    D_RT_CHECK(is_runtime_container<rt_list>::value   == true);
    D_RT_CHECK(is_runtime_container<rt_set>::value    == true);
    D_RT_CHECK(is_runtime_container<rt_map>::value    == true);

    // false
    D_RT_CHECK(is_runtime_container<rt_array>::value   == false);
    D_RT_CHECK(is_runtime_container<rt_pair>::value    == false);
    D_RT_CHECK(is_runtime_container<rt_no_size>::value == false);
    D_RT_CHECK(is_runtime_container<int>::value        == false);
    D_RT_CHECK(is_runtime_container<void>::value       == false);

    // the conjunction, re-derived
    D_RT_CHECK(is_runtime_container<rt_vector>::value
                   == (    has_size_accessor_signal<rt_vector>::value
                        && is_runtime_only(
                               container_lifetime<rt_vector>::value) ));
    D_RT_CHECK(is_runtime_container<rt_array>::value
                   == (    has_size_accessor_signal<rt_array>::value
                        && is_runtime_only(
                               container_lifetime<rt_array>::value) ));
    D_RT_CHECK(is_runtime_container<rt_pair>::value
                   == (    has_size_accessor_signal<rt_pair>::value
                        && is_runtime_only(
                               container_lifetime<rt_pair>::value) ));
    D_RT_CHECK(is_runtime_container<rt_category_none>::value
                   == (    has_size_accessor_signal<rt_category_none>::value
                        && is_runtime_only(
                               container_lifetime<
                                   rt_category_none>::value) ));

    return ok;
}

/*
tests_is_runtime_container_needs_shape
  Verifies the FIRST conjunct - that a runtime lifetime alone is not enough.

  Almost every type has a runtime lifetime, since the shared verdict falls back
  to runtime for anything not literal and not opted in.  So without the shape
  test the umbrella would be true of most of the type universe, an int
  included.  The carriers below have the lifetime and lack the shape.

  Tests the following:
  - a type with a runtime lifetime and no size() is not a runtime container
  - the same for a plain scalar, and for a std::pair, which is a container in
    the colloquial sense but exposes no size()
  - a size() that the probe cannot reach - non-const, or private - fails the
    conjunct just as a missing one does
  - adding a const size() to an otherwise identical carrier flips the verdict,
    so it is the shape and nothing else that is doing the work
*/
bool
tests_is_runtime_container_needs_shape()
{
    bool ok = true;

    // the lifetime is present
    D_RT_CHECK(is_runtime_only(container_lifetime<rt_no_size>::value)
                   == true);
    D_RT_CHECK(is_runtime_only(container_lifetime<rt_pair>::value) == true);
    D_RT_CHECK(is_runtime_only(container_lifetime<int>::value)     == true);

    // ... but the shape is not, so the umbrella is false
    D_RT_CHECK(is_runtime_container<rt_no_size>::value == false);
    D_RT_CHECK(is_runtime_container<rt_pair>::value    == false);
    D_RT_CHECK(is_runtime_container<int>::value        == false);

    // an unreachable size() fails the conjunct too
    D_RT_CHECK(   ( is_runtime_only(
                        container_lifetime<rt_size_nonconst>::value) == true )
               && ( is_runtime_container<rt_size_nonconst>::value == false ));
    D_RT_CHECK(   ( is_runtime_only(
                        container_lifetime<rt_size_private>::value) == true )
               && ( is_runtime_container<rt_size_private>::value == false ));

    // the shape is what flips it
    D_RT_CHECK(   ( is_runtime_container<rt_no_size>::value == false )
               && ( is_runtime_container<rt_shaped>::value  == true )
               && ( is_runtime_only(container_lifetime<rt_no_size>::value)
                        == is_runtime_only(
                               container_lifetime<rt_shaped>::value) ));

    return ok;
}

/*
tests_is_runtime_container_needs_runtime_only
  Verifies the SECOND conjunct - that a container shape alone is not enough.
  Tests the following:
  - a container-shaped type whose lifetime carries the compile-time bit is not
    a runtime container, std::array being the standard example
  - a synthetic carrier reaching the same verdict through a constexpr size()
  - being a LITERAL type is not by itself disqualifying: the verdict is the
    meet of the size lifetime and the contents lifetime, and a literal type
    whose size() carries no constexpr signal still reads runtime-only.  This is
    worth pinning because the opposite is the natural guess
  - the shape is present on all of these, so it is the lifetime doing the work
*/
bool
tests_is_runtime_container_needs_runtime_only()
{
    bool ok = true;

    // the shape is present
    D_RT_CHECK(has_size_accessor_signal<rt_array>::value == true);
    D_RT_CHECK(has_size_accessor_signal<rt_constexpr_size>::value == true);

    // ... but the lifetime is not runtime-only, so the umbrella is false
    D_RT_CHECK(container_lifetime<rt_array>::value == lifetime::both);
    D_RT_CHECK(is_runtime_container<rt_array>::value == false);
    D_RT_CHECK(container_lifetime<rt_constexpr_size>::value
                   == lifetime::both);
    D_RT_CHECK(is_runtime_container<rt_constexpr_size>::value == false);

    // a literal type with a plain size() is STILL runtime-only
    D_RT_STATIC_CHECK(std::is_trivially_destructible<rt_literal_plain>::value);
    D_RT_CHECK(container_lifetime<rt_literal_plain>::value
                   == lifetime::runtime);
    D_RT_CHECK(is_runtime_container<rt_literal_plain>::value == true);

    // ... and adding a constexpr size() to it is what changes the verdict
    D_RT_CHECK(   ( container_lifetime<rt_literal_plain>::value
                        != container_lifetime<rt_constexpr_size>::value )
               && ( is_runtime_container<rt_literal_plain>::value
                        != is_runtime_container<rt_constexpr_size>::value ));

    // the lifetime is what flips it
    D_RT_CHECK(   ( has_size_accessor_signal<rt_array>::value
                        == has_size_accessor_signal<rt_vector>::value )
               && ( is_runtime_container<rt_array>::value
                        != is_runtime_container<rt_vector>::value ));

    return ok;
}

/*
tests_runtime_only_is_exact_equality
  Verifies that the lifetime half is EXACT EQUALITY with lifetime::runtime,
  not a test of the runtime bit.

  lifetime is a two-bit lattice, and is_runtime_only asks for the value
  `runtime` exactly - so `both` reads false because it also carries the
  compile-time bit, and `none` and `compile_time` read false because they do
  not carry the runtime bit at all.  Were the check written as a bit test
  instead, `both` would start reading as a runtime container and every
  std::array would be misclassified.

  The last two verdicts are reachable only through the opt-in
  lifetime_category member: the shared verdict is the MEET of the size lifetime
  and the contents lifetime, and structurally neither operand ever clears the
  runtime bit.  Without the rt_category_* fixtures the exactness would go
  untested on half its cases.

  Tests the following:
  - all four lifetimes, exactly one of which is runtime-only
  - a container-shaped carrier for each of the four verdicts, and the umbrella
    on each
  - the two verdicts that fail for lack of the runtime bit, distinguished from
    the one that fails for carrying an extra bit
  - a bit test would disagree, stated directly
*/
bool
tests_runtime_only_is_exact_equality()
{
    bool ok = true;

    // exactly one of the four lifetimes is runtime-only
    int runtime_only_count = 0;

    for (int i = 0; i < rt_lifetime_count; ++i)
    {
        if (is_runtime_only(rt_lifetime_at(i)))
        {
            ++runtime_only_count;

            D_RT_CHECK(rt_lifetime_at(i) == lifetime::runtime);
        }
    }

    D_RT_CHECK(runtime_only_count == 1);

    // a container-shaped carrier for each verdict
    D_RT_CHECK(container_lifetime<rt_category_none>::value == lifetime::none);
    D_RT_CHECK(container_lifetime<rt_category_compile_time_sized>::value
                   == lifetime::compile_time);
    D_RT_CHECK(container_lifetime<rt_shaped>::value == lifetime::runtime);
    D_RT_CHECK(container_lifetime<rt_constexpr_size>::value
                   == lifetime::both);

    // ... each shaped, so only the lifetime differs
    D_RT_CHECK(has_size_accessor_signal<rt_category_none>::value == true);
    D_RT_CHECK(has_size_accessor_signal<
                   rt_category_compile_time_sized>::value == true);

    // ... and only the runtime one is a runtime container
    D_RT_CHECK(is_runtime_container<rt_category_none>::value == false);
    D_RT_CHECK(is_runtime_container<
                   rt_category_compile_time_sized>::value == false);
    D_RT_CHECK(is_runtime_container<rt_shaped>::value == true);
    D_RT_CHECK(is_runtime_container<rt_constexpr_size>::value == false);

    // the meet of compile_time and runtime is the bottom
    D_RT_CHECK(container_lifetime<rt_category_compile_time>::value
                   == lifetime::none);
    D_RT_CHECK(is_runtime_container<rt_category_compile_time>::value
                   == false);

    // failing for lack of the bit, versus failing for carrying an extra one
    D_RT_CHECK(   ( is_runtime(container_lifetime<rt_category_none>::value)
                        == false )
               && ( is_runtime(container_lifetime<rt_constexpr_size>::value)
                        == true )
               && ( is_runtime_only(
                        container_lifetime<rt_constexpr_size>::value)
                        == false ));

    // a bit test would disagree on `both`
    D_RT_CHECK(   ( is_runtime(lifetime::both)      == true )
               && ( is_runtime_only(lifetime::both) == false ));
    D_RT_CHECK(rt_life_is(container_lifetime<rt_array>::value, "both"));

    return ok;
}

/*
tests_requires_runtime_storage
  Verifies the storage umbrella - a disjunction of the two heap signals.
  Tests the following:
  - either signal alone is enough
  - a type with neither is false
  - the disjunction re-derived term by term, so neither term can be dropped
  - it is a disjunction and not a conjunction, shown by carriers satisfying
    exactly one term - std::vector satisfies both, std::deque only the
    allocator, and a synthetic carrier only the reserve
*/
bool
tests_requires_runtime_storage()
{
    bool ok = true;

    // either signal alone
    D_RT_CHECK(requires_runtime_storage<rt_allocator>::value == true);
    D_RT_CHECK(requires_runtime_storage<rt_reserve>::value   == true);

    // neither
    D_RT_CHECK(requires_runtime_storage<rt_shaped>::value == false);
    D_RT_CHECK(requires_runtime_storage<rt_array>::value  == false);
    D_RT_CHECK(requires_runtime_storage<rt_pair>::value   == false);
    D_RT_CHECK(requires_runtime_storage<int>::value       == false);
    D_RT_CHECK(requires_runtime_storage<void>::value      == false);

    // the disjunction, term by term
    D_RT_CHECK(requires_runtime_storage<rt_vector>::value
                   == (    has_allocator_alias<rt_vector>::value
                        || has_reserve_method_signal<rt_vector>::value ));
    D_RT_CHECK(requires_runtime_storage<rt_deque>::value
                   == (    has_allocator_alias<rt_deque>::value
                        || has_reserve_method_signal<rt_deque>::value ));
    D_RT_CHECK(requires_runtime_storage<rt_array>::value
                   == (    has_allocator_alias<rt_array>::value
                        || has_reserve_method_signal<rt_array>::value ));
    D_RT_CHECK(requires_runtime_storage<rt_reserve>::value
                   == (    has_allocator_alias<rt_reserve>::value
                        || has_reserve_method_signal<rt_reserve>::value ));

    // a disjunction, not a conjunction: one term is enough
    D_RT_CHECK(   ( has_allocator_alias<rt_deque>::value       == true )
               && ( has_reserve_method_signal<rt_deque>::value == false )
               && ( requires_runtime_storage<rt_deque>::value  == true ));
    D_RT_CHECK(   ( has_allocator_alias<rt_reserve>::value       == false )
               && ( has_reserve_method_signal<rt_reserve>::value == true )
               && ( requires_runtime_storage<rt_reserve>::value  == true ));

    return ok;
}

/*
tests_requires_runtime_storage_is_independent
  Verifies that the second umbrella does NOT imply the first, contrary to what
  its documentation suggests.

  The header describes requires_runtime_storage as the "stronger signal", but
  it is a disjunction of two storage probes and consults neither the container
  shape nor the lifetime verdict.  So it is not stronger than
  is_runtime_container; it is independent of it, and all four combinations
  occur:

    shape + storage    std::vector
    shape, no storage  a plain non-literal type with a size()
    storage, no shape  a type carrying only an allocator_type alias
    neither            std::array, int

  The third quadrant is the one that matters: any allocator-aware type - a
  trait class, an allocator adaptor, a builder - satisfies
  requires_runtime_storage without being a container at all.  Code reading it
  as a refinement of is_runtime_container would accept those.

  Tests the following:
  - all four quadrants populated
  - the implication failing in the direction the naming suggests
  - the implication failing in the other direction too, so neither is a
    refinement of the other
  - the two umbrellas disagreeing on a carrier of each failing quadrant
*/
bool
tests_requires_runtime_storage_is_independent()
{
    bool ok = true;

    // quadrant 1: both
    D_RT_CHECK(   ( is_runtime_container<rt_vector>::value     == true )
               && ( requires_runtime_storage<rt_vector>::value == true ));

    // quadrant 2: a runtime container with no storage signal
    D_RT_CHECK(   ( is_runtime_container<rt_shaped>::value     == true )
               && ( requires_runtime_storage<rt_shaped>::value == false ));
    D_RT_CHECK(   ( is_runtime_container<rt_literal_plain>::value == true )
               && ( requires_runtime_storage<rt_literal_plain>::value
                        == false ));

    // quadrant 3: a storage signal with no container at all
    D_RT_CHECK(   ( is_runtime_container<rt_allocator_no_shape>::value
                        == false )
               && ( requires_runtime_storage<rt_allocator_no_shape>::value
                        == true ));
    D_RT_CHECK(   ( is_runtime_container<rt_reserve_no_shape>::value
                        == false )
               && ( requires_runtime_storage<rt_reserve_no_shape>::value
                        == true ));

    // quadrant 4: neither
    D_RT_CHECK(   ( is_runtime_container<rt_array>::value     == false )
               && ( requires_runtime_storage<rt_array>::value == false ));
    D_RT_CHECK(   ( is_runtime_container<rt_pair>::value     == false )
               && ( requires_runtime_storage<rt_pair>::value == false ));

    // the implication the naming suggests does not hold
    D_RT_CHECK(   ( requires_runtime_storage<rt_allocator_no_shape>::value
                        == true )
               && ( is_runtime_container<rt_allocator_no_shape>::value
                        == false ));

    // nor does its converse
    D_RT_CHECK(   ( is_runtime_container<rt_shaped>::value     == true )
               && ( requires_runtime_storage<rt_shaped>::value == false ));

    // stated as the failing equivalence
    D_RT_CHECK(requires_runtime_storage<rt_allocator_no_shape>::value
                   != is_runtime_container<rt_allocator_no_shape>::value);
    D_RT_CHECK(requires_runtime_storage<rt_shaped>::value
                   != is_runtime_container<rt_shaped>::value);

    return ok;
}

/*
tests_classification_std_types
  Verifies both umbrellas across the standard library.
  Tests the following:
  - every standard container but std::array is a runtime container
  - std::array is the sole exception, and fails on the lifetime half rather
    than the shape half
  - every allocator-aware standard container requires runtime storage
  - a std::pair is neither, failing on the shape half
*/
bool
tests_classification_std_types()
{
    bool ok = true;

    // runtime containers
    D_RT_CHECK(is_runtime_container<rt_vector>::value == true);
    D_RT_CHECK(is_runtime_container<rt_string>::value == true);
    D_RT_CHECK(is_runtime_container<rt_deque>::value  == true);
    D_RT_CHECK(is_runtime_container<rt_list>::value   == true);
    D_RT_CHECK(is_runtime_container<rt_set>::value    == true);
    D_RT_CHECK(is_runtime_container<rt_map>::value    == true);

    // the fixed array is the exception, and fails on the lifetime half
    D_RT_CHECK(   ( has_size_accessor_signal<rt_array>::value == true )
               && ( is_runtime_only(container_lifetime<rt_array>::value)
                        == false )
               && ( is_runtime_container<rt_array>::value == false ));

    // storage
    D_RT_CHECK(requires_runtime_storage<rt_vector>::value == true);
    D_RT_CHECK(requires_runtime_storage<rt_string>::value == true);
    D_RT_CHECK(requires_runtime_storage<rt_deque>::value  == true);
    D_RT_CHECK(requires_runtime_storage<rt_map>::value    == true);
    D_RT_CHECK(requires_runtime_storage<rt_array>::value  == false);

    // a pair is neither, and fails on the shape half
    D_RT_CHECK(   ( has_size_accessor_signal<rt_pair>::value == false )
               && ( is_runtime_container<rt_pair>::value     == false )
               && ( requires_runtime_storage<rt_pair>::value == false ));

    return ok;
}

/*
tests_classification_cv_ref
  Verifies that both umbrellas honour the clean_t contract.
  Tests the following:
  - each is identical across all seven cv- and reference-qualified spellings,
    true and false alike
  - the shared container_lifetime verdict likewise, since the umbrella reads it
  - the volatile spellings in particular
*/
bool
tests_classification_cv_ref()
{
    bool ok = true;

    D_RT_CV_REF_AGREE(is_runtime_container, rt_vector, true);
    D_RT_CV_REF_AGREE(is_runtime_container, rt_shaped, true);
    D_RT_CV_REF_AGREE(is_runtime_container, rt_map,    true);
    D_RT_CV_REF_AGREE(is_runtime_container, rt_array,  false);
    D_RT_CV_REF_AGREE(is_runtime_container, rt_pair,   false);
    D_RT_CV_REF_AGREE(is_runtime_container, rt_no_size, false);

    D_RT_CV_REF_AGREE(requires_runtime_storage, rt_vector, true);
    D_RT_CV_REF_AGREE(requires_runtime_storage, rt_deque,  true);
    D_RT_CV_REF_AGREE(requires_runtime_storage, rt_allocator_no_shape, true);
    D_RT_CV_REF_AGREE(requires_runtime_storage, rt_array,  false);
    D_RT_CV_REF_AGREE(requires_runtime_storage, rt_shaped, false);

    // the shared verdict the umbrella reads
    D_RT_CHECK(container_lifetime<const rt_vector>::value
                   == container_lifetime<rt_vector>::value);
    D_RT_CHECK(container_lifetime<volatile rt_vector>::value
                   == container_lifetime<rt_vector>::value);
    D_RT_CHECK(container_lifetime<const volatile rt_array&>::value
                   == container_lifetime<rt_array>::value);
    D_RT_CHECK(container_lifetime<rt_array&&>::value
                   == container_lifetime<rt_array>::value);

    return ok;
}

/*
tests_classification_companions
  Verifies the `_v` companions, and pins a SHAPE DIVERGENCE between the two
  umbrellas and every comparable predicate in the codebase.

  The three Section-I signals are emitted by the trait_detect macros and so
  derive from std::integral_constant: they carry value_type, carry a nested
  type, convert to bool, and can be tag-dispatched on.  The two umbrellas here
  are hand-written structs exposing a bare `static constexpr bool value` and
  derive from nothing, so they carry none of that.

  This is not a stylistic quibble - it makes them unusable wherever a standard
  metafunction is expected.  `f(is_runtime_container<T>{})` will not select an
  overload taking std::true_type, and the object does not convert to bool.
  Every comparable predicate elsewhere DOES derive from integral_constant,
  including is_constexpr_container in constexpr_container_traits.hpp, which is
  the complementary trait on this same axis in the header this one names as its
  structural complement.  So the two ends of the Lifetime axis have
  incompatible metafunction shapes.

  The `_v` companions are unaffected, the emitting macro needing only ::value.

  Tests the following:
  - the umbrellas' ::value is present and usable in a constant expression
  - the umbrellas do NOT derive from integral_constant, while the signals in
    the same header do - the divergence, stated directly
  - the umbrellas are not convertible to bool, nor tag-dispatchable
  - the complementary trait on the same axis has the shape these lack
  - each `_v` companion agrees with its ::value across carriers of every
    quadrant, and is gated on variable-template support since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool
tests_classification_companions()
{
    bool ok = true;

    // ::value is present, and constexpr
    D_RT_STATIC_CHECK(is_runtime_container<rt_vector>::value);
    D_RT_STATIC_CHECK(is_runtime_container<rt_array>::value == false);
    D_RT_STATIC_CHECK(requires_runtime_storage<rt_vector>::value);
    D_RT_STATIC_CHECK(requires_runtime_storage<rt_array>::value == false);

    // the signals in this header have the standard metafunction shape
    D_RT_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        has_size_accessor_signal<rt_vector> >::value);
    D_RT_STATIC_CHECK(rt_has_value_type<
                          has_size_accessor_signal<rt_vector> >::value);
    D_RT_STATIC_CHECK(rt_has_nested_type<
                          has_allocator_alias<rt_vector> >::value);

    // ... and the umbrellas do not
    D_RT_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_runtime_container<rt_vector> >::value == false);
    D_RT_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_runtime_container<rt_array> >::value == false);
    D_RT_STATIC_CHECK(rt_has_value_type<
                          is_runtime_container<rt_vector> >::value == false);
    D_RT_STATIC_CHECK(rt_has_nested_type<
                          is_runtime_container<rt_vector> >::value == false);
    D_RT_STATIC_CHECK(
        rt_has_value_type<
            requires_runtime_storage<rt_vector> >::value == false);
    D_RT_STATIC_CHECK(
        rt_has_nested_type<
            requires_runtime_storage<rt_vector> >::value == false);

    // ... so they do not convert to bool, and cannot be tag-dispatched on
    D_RT_STATIC_CHECK(
        std::is_convertible<is_runtime_container<rt_vector>,
                            bool>::value == false);
    D_RT_STATIC_CHECK(
        std::is_convertible<requires_runtime_storage<rt_vector>,
                            bool>::value == false);

    // the complementary trait on the same axis has the shape these lack
    D_RT_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_constexpr_container<rt_vector> >::value);
    D_RT_STATIC_CHECK(rt_has_value_type<
                          is_constexpr_container<rt_vector> >::value);
    D_RT_STATIC_CHECK(
        std::is_convertible<is_constexpr_container<rt_vector>, bool>::value);

    // ... and it agrees with this header on the axis it shares
    D_RT_CHECK(is_constexpr_container<rt_array>::value == true);
    D_RT_CHECK(is_runtime_container<rt_array>::value   == false);
    D_RT_CHECK(is_constexpr_container<rt_vector>::value == false);
    D_RT_CHECK(is_runtime_container<rt_vector>::value   == true);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // the companions are unaffected, needing only ::value
    D_RT_STATIC_CHECK(is_runtime_container_v<rt_vector>);
    D_RT_STATIC_CHECK(requires_runtime_storage_v<rt_array> == false);

    D_RT_CHECK(is_runtime_container_v<rt_vector>
                   == is_runtime_container<rt_vector>::value);
    D_RT_CHECK(is_runtime_container_v<rt_array>
                   == is_runtime_container<rt_array>::value);
    D_RT_CHECK(is_runtime_container_v<rt_shaped>
                   == is_runtime_container<rt_shaped>::value);
    D_RT_CHECK(is_runtime_container_v<rt_pair>
                   == is_runtime_container<rt_pair>::value);
    D_RT_CHECK(requires_runtime_storage_v<rt_vector>
                   == requires_runtime_storage<rt_vector>::value);
    D_RT_CHECK(requires_runtime_storage_v<rt_allocator_no_shape>
                   == requires_runtime_storage<rt_allocator_no_shape>::value);
    D_RT_CHECK(requires_runtime_storage_v<rt_array>
                   == requires_runtime_storage<rt_array>::value);
    D_RT_CHECK(is_runtime_container_v<const rt_vector&>
                   == is_runtime_container<rt_vector>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
