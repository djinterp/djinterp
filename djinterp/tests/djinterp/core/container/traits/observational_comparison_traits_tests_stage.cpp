/*******************************************************************************
* djinterp [core]                observational_comparison_traits_tests_stage.cpp
*
*
* path:      /tests/djinterp/core/container/traits/observational_comparison_traits_tests_stage.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "observational_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// V.   Stage of a comparison
// ===========================================================================
//   A comparison carries the latest stage of the accesses it uses, so it is
// compile-time only when BOTH operands are - the meet of meta/lifetime.hpp.
// Each operand's stage is PROXIED by a static size signal, which is a
// deliberate approximation: it is the tell a std::array carries and a
// std::vector does not.  The tests below pin both what the proxy catches and
// what it misses.


/*
tests_has_static_size_helper
  Verifies the internal helper that detects a std::tuple_size<T>::value.
  Tests the following:
  - std::array is detected, which is the signal the proxy exists to read
  - std::pair is detected too, being a tuple-like product - so the helper reads
    "has a compile-time size", not "is a container"
  - std::vector and a bare struct are not detected
  - a program-defined type WITH a std::tuple_size specialisation is detected,
    so the signal is an opt-in any type may declare and not a hard-coded list
  - a RAW ARRAY is NOT detected, although its bound is compile-time known.
    The standard does not specialise tuple_size for raw arrays, so the proxy
    misses a genuinely compile-time-sized type.  That is a limitation of the
    proxy rather than of the idea, and is pinned here so it stays visible
*/
bool
tests_has_static_size_helper()
{
    bool ok = true;

    // the standard tuple-like types
    D_OC_CHECK(internal::has_static_size_helper<oct_array>::value == true);
    D_OC_CHECK(internal::has_static_size_helper<oct_pair>::value  == true);

    // ... and the dynamically sized ones
    D_OC_CHECK(internal::has_static_size_helper<oct_vector>::value == false);
    D_OC_CHECK(internal::has_static_size_helper<oct_deque>::value  == false);
    D_OC_CHECK(internal::has_static_size_helper<oct_set>::value    == false);
    D_OC_CHECK(internal::has_static_size_helper<oct_string>::value == false);

    // an opt-in by a program-defined type
    D_OC_CHECK(
        internal::has_static_size_helper<oct_static_sized_bare>::value == true);
    D_OC_CHECK(
        internal::has_static_size_helper<oct_static_sized_size>::value == true);

    // non-containers
    D_OC_CHECK(internal::has_static_size_helper<oct_bare>::value == false);
    D_OC_CHECK(internal::has_static_size_helper<int>::value      == false);

    // the raw array the proxy misses
    D_OC_CHECK(internal::has_static_size_helper<oct_raw_array>::value == false);

    return ok;
}

/*
tests_observed_access_stage_of
  Verifies the stage at which a single type's contents are observable.
  Tests the following:
  - a static size reads as lifetime::both - observable in constant evaluation
    as well as at run time
  - everything else reads as lifetime::runtime
  - the answer tracks the helper exactly, term for term, so the proxy is the
    whole of the decision
  - a raw array reads runtime, following the helper's miss noted above
*/
bool
tests_observed_access_stage_of()
{
    bool ok = true;

    // static size -> both
    D_OC_CHECK(observed_access_stage_of<oct_array>::value == lifetime::both);
    D_OC_CHECK(observed_access_stage_of<oct_pair>::value  == lifetime::both);
    D_OC_CHECK(observed_access_stage_of<oct_static_sized_bare>::value
                   == lifetime::both);
    D_OC_CHECK(observed_access_stage_of<oct_static_sized_size>::value
                   == lifetime::both);

    // everything else -> runtime
    D_OC_CHECK(observed_access_stage_of<oct_vector>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<oct_set>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<oct_nested>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<oct_string>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<oct_bare>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<int>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<oct_raw_array>::value
                   == lifetime::runtime);

    // the stage tracks the helper exactly
    D_OC_CHECK(( internal::has_static_size_helper<oct_array>::value )
                   == ( observed_access_stage_of<oct_array>::value
                            == lifetime::both ));
    D_OC_CHECK(( internal::has_static_size_helper<oct_vector>::value )
                   == ( observed_access_stage_of<oct_vector>::value
                            == lifetime::both ));
    D_OC_CHECK(( internal::has_static_size_helper<oct_pair>::value )
                   == ( observed_access_stage_of<oct_pair>::value
                            == lifetime::both ));
    D_OC_CHECK(( internal::has_static_size_helper<oct_bare>::value )
                   == ( observed_access_stage_of<oct_bare>::value
                            == lifetime::both ));

    return ok;
}

/*
tests_observational_stage_of
  Verifies the stage a comparison of two types carries - the meet of their
  access stages.
  Tests the following:
  - both operands compile-observable gives both; either one runtime-only drags
    the pair to runtime, in either argument order
  - the definitional identity against lifetime_meet, on a spread of pairs
  - the meet is symmetric and idempotent, as the lattice operation is
  - a single runtime operand is enough to settle the pair, which is the whole
    content of "compile-time only when BOTH are"
*/
bool
tests_observational_stage_of()
{
    bool ok = true;

    // both compile-observable
    D_OC_CHECK((observational_stage_of<oct_array, oct_array>::value
                    == lifetime::both));
    D_OC_CHECK((observational_stage_of<oct_array, oct_pair>::value
                    == lifetime::both));
    D_OC_CHECK((observational_stage_of<oct_static_sized_bare,
                                       oct_array>::value == lifetime::both));

    // one runtime operand drags the pair down, in either order
    D_OC_CHECK((observational_stage_of<oct_array, oct_vector>::value
                    == lifetime::runtime));
    D_OC_CHECK((observational_stage_of<oct_vector, oct_array>::value
                    == lifetime::runtime));

    // neither compile-observable
    D_OC_CHECK((observational_stage_of<oct_vector, oct_set>::value
                    == lifetime::runtime));
    D_OC_CHECK((observational_stage_of<oct_bare, oct_bare>::value
                    == lifetime::runtime));

    // the definitional identity
    D_OC_CHECK((oct_stage_is_meet<oct_array,  oct_array>()));
    D_OC_CHECK((oct_stage_is_meet<oct_array,  oct_vector>()));
    D_OC_CHECK((oct_stage_is_meet<oct_vector, oct_vector>()));
    D_OC_CHECK((oct_stage_is_meet<oct_pair,   oct_bare>()));
    D_OC_CHECK((oct_stage_is_meet<oct_static_sized_size, oct_nested>()));
    D_OC_CHECK((oct_stage_is_meet<oct_raw_array, oct_array>()));

    // symmetric and idempotent
    D_OC_CHECK((observational_stage_of<oct_array, oct_vector>::value
                    == observational_stage_of<oct_vector, oct_array>::value));
    D_OC_CHECK((observational_stage_of<oct_array, oct_array>::value
                    == observed_access_stage_of<oct_array>::value));
    D_OC_CHECK((observational_stage_of<oct_vector, oct_vector>::value
                    == observed_access_stage_of<oct_vector>::value));

    return ok;
}

/*
tests_is_compile_time_comparison
  Verifies the predicate that reads the compile-time bit off the pair's stage.
  Tests the following:
  - true exactly when BOTH operands carry a static size
  - false as soon as either does not, in either argument order
  - it agrees with is_compile_time applied to the pair's stage, which is its
    definition
  - it agrees with the conjunction of the two operands' own helper readings,
    which is the property the header claims for it
*/
bool
tests_is_compile_time_comparison()
{
    bool ok = true;

    // both compile-observable
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_array>::value
                    == true));
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_pair>::value
                    == true));
    D_OC_CHECK((is_compile_time_comparison<oct_static_sized_bare,
                                           oct_static_sized_size>::value
                    == true));

    // either one not, in either order
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_vector>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<oct_vector, oct_array>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<oct_vector, oct_vector>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<oct_bare, oct_bare>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_raw_array>::value
                    == false));

    // its definition
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_array>::value
                    == is_compile_time(
                           observational_stage_of<oct_array,
                                                  oct_array>::value)));
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_vector>::value
                    == is_compile_time(
                           observational_stage_of<oct_array,
                                                  oct_vector>::value)));

    // the conjunction the header claims
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_vector>::value
                    == (   internal::has_static_size_helper<oct_array>::value
                        && internal::has_static_size_helper<
                               oct_vector>::value )));
    D_OC_CHECK((is_compile_time_comparison<oct_array, oct_pair>::value
                    == (   internal::has_static_size_helper<oct_array>::value
                        && internal::has_static_size_helper<
                               oct_pair>::value )));
    D_OC_CHECK((is_compile_time_comparison<oct_bare, oct_bare>::value
                    == (   internal::has_static_size_helper<oct_bare>::value
                        && internal::has_static_size_helper<
                               oct_bare>::value )));

    return ok;
}

/*
tests_stage_range
  Verifies which of the four lifetime values these traits can actually produce.

  The lattice has four elements, but observed_access_stage_of is a two-way
  branch that yields only both or runtime.  The meet of two such values is
  again only both or runtime, so lifetime::none and lifetime::compile_time are
  UNREACHABLE through this header.  Two consequences follow and are checked: a
  comparison is never compile-time ONLY, and a comparison is never
  unobservable outright.

  Tests the following:
  - every carrier's access stage is one of the two reachable values
  - every pairing's stage is likewise one of the two
  - so every comparison includes the runtime stage, whatever else it includes
  - and none is ever produced, so a stage is never empty
*/
bool
tests_stage_range()
{
    bool ok = true;

    // the single-type stage is one of two values
    D_OC_CHECK(   ( observed_access_stage_of<oct_array>::value
                        == lifetime::both )
               || ( observed_access_stage_of<oct_array>::value
                        == lifetime::runtime ));
    D_OC_CHECK(   ( observed_access_stage_of<oct_vector>::value
                        == lifetime::both )
               || ( observed_access_stage_of<oct_vector>::value
                        == lifetime::runtime ));
    D_OC_CHECK(observed_access_stage_of<oct_array>::value
                   != lifetime::compile_time);
    D_OC_CHECK(observed_access_stage_of<oct_array>::value != lifetime::none);
    D_OC_CHECK(observed_access_stage_of<oct_vector>::value
                   != lifetime::compile_time);
    D_OC_CHECK(observed_access_stage_of<oct_vector>::value != lifetime::none);
    D_OC_CHECK(observed_access_stage_of<oct_bare>::value   != lifetime::none);

    // and so is the pair's
    D_OC_CHECK((observational_stage_of<oct_array, oct_array>::value
                    != lifetime::none));
    D_OC_CHECK((observational_stage_of<oct_array, oct_vector>::value
                    != lifetime::none));
    D_OC_CHECK((observational_stage_of<oct_vector, oct_vector>::value
                    != lifetime::none));
    D_OC_CHECK((observational_stage_of<oct_array, oct_array>::value
                    != lifetime::compile_time));
    D_OC_CHECK((observational_stage_of<oct_array, oct_vector>::value
                    != lifetime::compile_time));

    // every comparison is at least runtime-capable
    D_OC_CHECK(is_runtime(observational_stage_of<oct_array,
                                                 oct_array>::value));
    D_OC_CHECK(is_runtime(observational_stage_of<oct_array,
                                                 oct_vector>::value));
    D_OC_CHECK(is_runtime(observational_stage_of<oct_vector,
                                                 oct_vector>::value));
    D_OC_CHECK(is_runtime(observational_stage_of<oct_bare, oct_bare>::value));

    return ok;
}

/*
tests_stage_level_orthogonality
  Verifies that the stage axis and the level axis are INDEPENDENT.  They answer
  different questions - "when can the contents be read" against "how finely can
  two be told apart" - and the header keeps them apart deliberately.

  The sharpest witness is std::pair, which resolves level none yet carries
  stage both: a comparison of two pairs is settled in constant evaluation, and
  settles nothing beyond identity.  Compile-time and informative are not the
  same property.

  Tests the following:
  - the four corners of the two axes: a type at a fine level with each stage,
    and a type at the coarsest level with each stage
  - so the level does not determine the stage, and the stage does not determine
    the level
  - is_compile_time_comparison can be true at level none, which is the striking
    case
  - moving one axis leaves the other where it was
*/
bool
tests_stage_level_orthogonality()
{
    bool ok = true;

    // fine level, compile-observable
    D_OC_CHECK(   ( observational_level_of<oct_array>::value
                        == observational_level::seq )
               && ( observed_access_stage_of<oct_array>::value
                        == lifetime::both ));

    // fine level, runtime only
    D_OC_CHECK(   ( observational_level_of<oct_vector>::value
                        == observational_level::seq )
               && ( observed_access_stage_of<oct_vector>::value
                        == lifetime::runtime ));

    // coarsest level, compile-observable
    D_OC_CHECK(   ( observational_level_of<oct_pair>::value
                        == observational_level::none )
               && ( observed_access_stage_of<oct_pair>::value
                        == lifetime::both ));
    D_OC_CHECK(   ( observational_level_of<oct_static_sized_bare>::value
                        == observational_level::none )
               && ( observed_access_stage_of<oct_static_sized_bare>::value
                        == lifetime::both ));

    // coarsest level, runtime only
    D_OC_CHECK(   ( observational_level_of<oct_bare>::value
                        == observational_level::none )
               && ( observed_access_stage_of<oct_bare>::value
                        == lifetime::runtime ));

    // a comparison may be settled at compile time and settle nothing
    D_OC_CHECK((is_compile_time_comparison<oct_pair, oct_pair>::value
                    == true));
    D_OC_CHECK((shared_observational_level<oct_pair, oct_pair>::value
                    == observational_level::none));

    // moving one axis leaves the other alone: same level, differing stage
    D_OC_CHECK(   ( observational_level_of<oct_static_sized_size>::value
                        == observational_level_of<oct_size_only>::value )
               && ( observed_access_stage_of<oct_static_sized_size>::value
                        != observed_access_stage_of<oct_size_only>::value ));

    // ... and same stage, differing level
    D_OC_CHECK(   ( observed_access_stage_of<oct_array>::value
                        == observed_access_stage_of<oct_pair>::value )
               && ( observational_level_of<oct_array>::value
                        != observational_level_of<oct_pair>::value ));

    return ok;
}

/*
tests_stage_cv_ref
  Verifies that the stage traits honour the clean_t contract.
  Tests the following:
  - the single-type stage is unchanged across all seven spellings, on a
    compile-observable and a runtime carrier
  - the pair traits clean each operand independently
  - is_compile_time_comparison, which forwards to the pair trait rather than
    cleaning for itself, is invariant by consequence
*/
bool
tests_stage_cv_ref()
{
    bool ok = true;

    D_OC_CV_REF_AGREE(observed_access_stage_of, oct_array,  lifetime::both);
    D_OC_CV_REF_AGREE(observed_access_stage_of, oct_pair,   lifetime::both);
    D_OC_CV_REF_AGREE(observed_access_stage_of, oct_vector, lifetime::runtime);
    D_OC_CV_REF_AGREE(observed_access_stage_of, oct_bare,   lifetime::runtime);

    D_OC_CV_REF_PAIR_AGREE(observational_stage_of, oct_array, oct_array,
                           lifetime::both);
    D_OC_CV_REF_PAIR_AGREE(observational_stage_of, oct_array, oct_vector,
                           lifetime::runtime);

    D_OC_CV_REF_PAIR_AGREE(is_compile_time_comparison, oct_array, oct_array,
                           true);
    D_OC_CV_REF_PAIR_AGREE(is_compile_time_comparison, oct_array, oct_vector,
                           false);

    // both operands volatile at once
    D_OC_CHECK((observational_stage_of<volatile oct_array,
                                       volatile oct_array>::value
                    == lifetime::both));
    D_OC_CHECK((is_compile_time_comparison<const volatile oct_array,
                                           oct_array&&>::value == true));

    return ok;
}

/*
tests_stage_companions
  Verifies the `_t` aliases and the `_v` companions of section V.
  Tests the following:
  - both `_t` aliases are the integral_constants naming the stage
  - the three `_v` companions agree with their traits' ::value
  - all are usable in a constant expression
  - the `_v` half is gated on variable-template support
*/
bool
tests_stage_companions()
{
    bool ok = true;

    // the _t aliases are the integral_constants, exactly
    D_OC_STATIC_CHECK(
        std::is_same<observed_access_stage_of_t<oct_array>,
                     std::integral_constant<lifetime,
                                            lifetime::both> >::value);
    D_OC_STATIC_CHECK(
        std::is_same<observed_access_stage_of_t<oct_vector>,
                     std::integral_constant<lifetime,
                                            lifetime::runtime> >::value);
    D_OC_STATIC_CHECK(
        std::is_same<observational_stage_of_t<oct_array, oct_vector>,
                     std::integral_constant<lifetime,
                                            lifetime::runtime> >::value);

    // ... and carry the same values the traits report
    D_OC_CHECK(observed_access_stage_of_t<oct_pair>::value
                   == observed_access_stage_of<oct_pair>::value);
    D_OC_CHECK((observational_stage_of_t<oct_array, oct_array>::value
                    == observational_stage_of<oct_array, oct_array>::value));

    // usable in a constant expression
    D_OC_STATIC_CHECK(observed_access_stage_of<oct_array>::value
                          == lifetime::both);
    D_OC_STATIC_CHECK(
        (is_compile_time_comparison<oct_array, oct_array>::value));

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OC_STATIC_CHECK(observed_access_stage_of_v<oct_array> == lifetime::both);
    D_OC_STATIC_CHECK((is_compile_time_comparison_v<oct_array, oct_array>));

    D_OC_CHECK(observed_access_stage_of_v<oct_vector>
                   == observed_access_stage_of<oct_vector>::value);
    D_OC_CHECK(observed_access_stage_of_v<oct_pair>
                   == observed_access_stage_of<oct_pair>::value);

    D_OC_CHECK((observational_stage_of_v<oct_array, oct_vector>
                    == observational_stage_of<oct_array, oct_vector>::value));
    D_OC_CHECK((observational_stage_of_v<oct_array, oct_array>
                    == observational_stage_of<oct_array, oct_array>::value));

    D_OC_CHECK((is_compile_time_comparison_v<oct_array, oct_array>
                    == is_compile_time_comparison<oct_array,
                                                  oct_array>::value));
    D_OC_CHECK((is_compile_time_comparison_v<oct_vector, oct_vector>
                    == is_compile_time_comparison<oct_vector,
                                                  oct_vector>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
