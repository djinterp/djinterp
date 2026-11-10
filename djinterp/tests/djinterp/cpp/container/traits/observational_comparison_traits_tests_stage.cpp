/*******************************************************************************
* djinterp [cpp]                 observational_comparison_traits_tests_stage.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/observational_comparison_traits_tests_stage.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
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
bool tests_has_static_size_helper()
{
    bool ok = true;

    // the standard tuple-like types
    D_OC_CHECK(internal::has_static_size_helper<DOctArray>::value == true);
    D_OC_CHECK(internal::has_static_size_helper<DOctPair>::value  == true);

    // ... and the dynamically sized ones
    D_OC_CHECK(internal::has_static_size_helper<DOctVector>::value == false);
    D_OC_CHECK(internal::has_static_size_helper<DOctDeque>::value  == false);
    D_OC_CHECK(internal::has_static_size_helper<DOctSet>::value    == false);
    D_OC_CHECK(internal::has_static_size_helper<DOctString>::value == false);

    // an opt-in by a program-defined type
    D_OC_CHECK(
        internal::has_static_size_helper<DOctStaticSizedBare>::value == true);
    D_OC_CHECK(
        internal::has_static_size_helper<DOctStaticSizedSize>::value == true);

    // non-containers
    D_OC_CHECK(internal::has_static_size_helper<DOctBare>::value == false);
    D_OC_CHECK(internal::has_static_size_helper<int>::value      == false);

    // the raw array the proxy misses
    D_OC_CHECK(internal::has_static_size_helper<DOctRawArray>::value == false);

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
bool tests_observed_access_stage_of()
{
    bool ok = true;

    // static size -> both
    D_OC_CHECK(observed_access_stage_of<DOctArray>::value == lifetime::both);
    D_OC_CHECK(observed_access_stage_of<DOctPair>::value  == lifetime::both);
    D_OC_CHECK(observed_access_stage_of<DOctStaticSizedBare>::value
                   == lifetime::both);
    D_OC_CHECK(observed_access_stage_of<DOctStaticSizedSize>::value
                   == lifetime::both);

    // everything else -> runtime
    D_OC_CHECK(observed_access_stage_of<DOctVector>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<DOctSet>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<DOctNested>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<DOctString>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<DOctBare>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<int>::value
                   == lifetime::runtime);
    D_OC_CHECK(observed_access_stage_of<DOctRawArray>::value
                   == lifetime::runtime);

    // the stage tracks the helper exactly
    D_OC_CHECK(( internal::has_static_size_helper<DOctArray>::value )
                   == ( observed_access_stage_of<DOctArray>::value
                            == lifetime::both ));
    D_OC_CHECK(( internal::has_static_size_helper<DOctVector>::value )
                   == ( observed_access_stage_of<DOctVector>::value
                            == lifetime::both ));
    D_OC_CHECK(( internal::has_static_size_helper<DOctPair>::value )
                   == ( observed_access_stage_of<DOctPair>::value
                            == lifetime::both ));
    D_OC_CHECK(( internal::has_static_size_helper<DOctBare>::value )
                   == ( observed_access_stage_of<DOctBare>::value
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
bool tests_observational_stage_of()
{
    bool ok = true;

    // both compile-observable
    D_OC_CHECK((observational_stage_of<DOctArray, DOctArray>::value
                    == lifetime::both));
    D_OC_CHECK((observational_stage_of<DOctArray, DOctPair>::value
                    == lifetime::both));
    D_OC_CHECK((observational_stage_of<DOctStaticSizedBare,
                                       DOctArray>::value == lifetime::both));

    // one runtime operand drags the pair down, in either order
    D_OC_CHECK((observational_stage_of<DOctArray, DOctVector>::value
                    == lifetime::runtime));
    D_OC_CHECK((observational_stage_of<DOctVector, DOctArray>::value
                    == lifetime::runtime));

    // neither compile-observable
    D_OC_CHECK((observational_stage_of<DOctVector, DOctSet>::value
                    == lifetime::runtime));
    D_OC_CHECK((observational_stage_of<DOctBare, DOctBare>::value
                    == lifetime::runtime));

    // the definitional identity
    D_OC_CHECK((oct_stage_is_meet<DOctArray,  DOctArray>()));
    D_OC_CHECK((oct_stage_is_meet<DOctArray,  DOctVector>()));
    D_OC_CHECK((oct_stage_is_meet<DOctVector, DOctVector>()));
    D_OC_CHECK((oct_stage_is_meet<DOctPair,   DOctBare>()));
    D_OC_CHECK((oct_stage_is_meet<DOctStaticSizedSize, DOctNested>()));
    D_OC_CHECK((oct_stage_is_meet<DOctRawArray, DOctArray>()));

    // symmetric and idempotent
    D_OC_CHECK((observational_stage_of<DOctArray, DOctVector>::value
                    == observational_stage_of<DOctVector, DOctArray>::value));
    D_OC_CHECK((observational_stage_of<DOctArray, DOctArray>::value
                    == observed_access_stage_of<DOctArray>::value));
    D_OC_CHECK((observational_stage_of<DOctVector, DOctVector>::value
                    == observed_access_stage_of<DOctVector>::value));

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
bool tests_is_compile_time_comparison()
{
    bool ok = true;

    // both compile-observable
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctArray>::value
                    == true));
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctPair>::value
                    == true));
    D_OC_CHECK((is_compile_time_comparison<DOctStaticSizedBare,
                                           DOctStaticSizedSize>::value
                    == true));

    // either one not, in either order
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctVector>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<DOctVector, DOctArray>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<DOctVector, DOctVector>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<DOctBare, DOctBare>::value
                    == false));
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctRawArray>::value
                    == false));

    // its definition
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctArray>::value
                    == is_compile_time(
                           observational_stage_of<DOctArray,
                                                  DOctArray>::value)));
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctVector>::value
                    == is_compile_time(
                           observational_stage_of<DOctArray,
                                                  DOctVector>::value)));

    // the conjunction the header claims
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctVector>::value
                    == (   internal::has_static_size_helper<DOctArray>::value
                        && internal::has_static_size_helper<
                               DOctVector>::value )));
    D_OC_CHECK((is_compile_time_comparison<DOctArray, DOctPair>::value
                    == (   internal::has_static_size_helper<DOctArray>::value
                        && internal::has_static_size_helper<
                               DOctPair>::value )));
    D_OC_CHECK((is_compile_time_comparison<DOctBare, DOctBare>::value
                    == (   internal::has_static_size_helper<DOctBare>::value
                        && internal::has_static_size_helper<
                               DOctBare>::value )));

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
bool tests_stage_range()
{
    bool ok = true;

    // the single-type stage is one of two values
    D_OC_CHECK(   ( observed_access_stage_of<DOctArray>::value
                        == lifetime::both )
               || ( observed_access_stage_of<DOctArray>::value
                        == lifetime::runtime ));
    D_OC_CHECK(   ( observed_access_stage_of<DOctVector>::value
                        == lifetime::both )
               || ( observed_access_stage_of<DOctVector>::value
                        == lifetime::runtime ));
    D_OC_CHECK(observed_access_stage_of<DOctArray>::value
                   != lifetime::compile_time);
    D_OC_CHECK(observed_access_stage_of<DOctArray>::value != lifetime::none);
    D_OC_CHECK(observed_access_stage_of<DOctVector>::value
                   != lifetime::compile_time);
    D_OC_CHECK(observed_access_stage_of<DOctVector>::value != lifetime::none);
    D_OC_CHECK(observed_access_stage_of<DOctBare>::value   != lifetime::none);

    // and so is the pair's
    D_OC_CHECK((observational_stage_of<DOctArray, DOctArray>::value
                    != lifetime::none));
    D_OC_CHECK((observational_stage_of<DOctArray, DOctVector>::value
                    != lifetime::none));
    D_OC_CHECK((observational_stage_of<DOctVector, DOctVector>::value
                    != lifetime::none));
    D_OC_CHECK((observational_stage_of<DOctArray, DOctArray>::value
                    != lifetime::compile_time));
    D_OC_CHECK((observational_stage_of<DOctArray, DOctVector>::value
                    != lifetime::compile_time));

    // every comparison is at least runtime-capable
    D_OC_CHECK(is_runtime(observational_stage_of<DOctArray,
                                                 DOctArray>::value));
    D_OC_CHECK(is_runtime(observational_stage_of<DOctArray,
                                                 DOctVector>::value));
    D_OC_CHECK(is_runtime(observational_stage_of<DOctVector,
                                                 DOctVector>::value));
    D_OC_CHECK(is_runtime(observational_stage_of<DOctBare, DOctBare>::value));

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
bool tests_stage_level_orthogonality()
{
    bool ok = true;

    // fine level, compile-observable
    D_OC_CHECK(   ( observational_level_of<DOctArray>::value
                        == observational_level::seq )
               && ( observed_access_stage_of<DOctArray>::value
                        == lifetime::both ));

    // fine level, runtime only
    D_OC_CHECK(   ( observational_level_of<DOctVector>::value
                        == observational_level::seq )
               && ( observed_access_stage_of<DOctVector>::value
                        == lifetime::runtime ));

    // coarsest level, compile-observable
    D_OC_CHECK(   ( observational_level_of<DOctPair>::value
                        == observational_level::none )
               && ( observed_access_stage_of<DOctPair>::value
                        == lifetime::both ));
    D_OC_CHECK(   ( observational_level_of<DOctStaticSizedBare>::value
                        == observational_level::none )
               && ( observed_access_stage_of<DOctStaticSizedBare>::value
                        == lifetime::both ));

    // coarsest level, runtime only
    D_OC_CHECK(   ( observational_level_of<DOctBare>::value
                        == observational_level::none )
               && ( observed_access_stage_of<DOctBare>::value
                        == lifetime::runtime ));

    // a comparison may be settled at compile time and settle nothing
    D_OC_CHECK((is_compile_time_comparison<DOctPair, DOctPair>::value
                    == true));
    D_OC_CHECK((shared_observational_level<DOctPair, DOctPair>::value
                    == observational_level::none));

    // moving one axis leaves the other alone: same level, differing stage
    D_OC_CHECK(   ( observational_level_of<DOctStaticSizedSize>::value
                        == observational_level_of<DOctSizeOnly>::value )
               && ( observed_access_stage_of<DOctStaticSizedSize>::value
                        != observed_access_stage_of<DOctSizeOnly>::value ));

    // ... and same stage, differing level
    D_OC_CHECK(   ( observed_access_stage_of<DOctArray>::value
                        == observed_access_stage_of<DOctPair>::value )
               && ( observational_level_of<DOctArray>::value
                        != observational_level_of<DOctPair>::value ));

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
bool tests_stage_cv_ref()
{
    bool ok = true;

    D_OC_CV_REF_AGREE(observed_access_stage_of, DOctArray,  lifetime::both);
    D_OC_CV_REF_AGREE(observed_access_stage_of, DOctPair,   lifetime::both);
    D_OC_CV_REF_AGREE(observed_access_stage_of, DOctVector, lifetime::runtime);
    D_OC_CV_REF_AGREE(observed_access_stage_of, DOctBare,   lifetime::runtime);

    D_OC_CV_REF_PAIR_AGREE(observational_stage_of, DOctArray, DOctArray,
                           lifetime::both);
    D_OC_CV_REF_PAIR_AGREE(observational_stage_of, DOctArray, DOctVector,
                           lifetime::runtime);

    D_OC_CV_REF_PAIR_AGREE(is_compile_time_comparison, DOctArray, DOctArray,
                           true);
    D_OC_CV_REF_PAIR_AGREE(is_compile_time_comparison, DOctArray, DOctVector,
                           false);

    // both operands volatile at once
    D_OC_CHECK((observational_stage_of<volatile DOctArray,
                                       volatile DOctArray>::value
                    == lifetime::both));
    D_OC_CHECK((is_compile_time_comparison<const volatile DOctArray,
                                           DOctArray&&>::value == true));

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
bool tests_stage_companions()
{
    bool ok = true;

    // the _t aliases are the integral_constants, exactly
    D_OC_STATIC_CHECK(
        std::is_same<observed_access_stage_of_t<DOctArray>,
                     std::integral_constant<lifetime::value,
                                            lifetime::both> >::value);
    D_OC_STATIC_CHECK(
        std::is_same<observed_access_stage_of_t<DOctVector>,
                     std::integral_constant<lifetime::value,
                                            lifetime::runtime> >::value);
    D_OC_STATIC_CHECK(
        std::is_same<observational_stage_of_t<DOctArray, DOctVector>,
                     std::integral_constant<lifetime::value,
                                            lifetime::runtime> >::value);

    // ... and carry the same values the traits report
    D_OC_CHECK(observed_access_stage_of_t<DOctPair>::value
                   == observed_access_stage_of<DOctPair>::value);
    D_OC_CHECK((observational_stage_of_t<DOctArray, DOctArray>::value
                    == observational_stage_of<DOctArray, DOctArray>::value));

    // usable in a constant expression
    D_OC_STATIC_CHECK(observed_access_stage_of<DOctArray>::value
                          == lifetime::both);
    D_OC_STATIC_CHECK(
        (is_compile_time_comparison<DOctArray, DOctArray>::value));

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OC_STATIC_CHECK(observed_access_stage_of_v<DOctArray> == lifetime::both);
    D_OC_STATIC_CHECK((is_compile_time_comparison_v<DOctArray, DOctArray>));

    D_OC_CHECK(observed_access_stage_of_v<DOctVector>
                   == observed_access_stage_of<DOctVector>::value);
    D_OC_CHECK(observed_access_stage_of_v<DOctPair>
                   == observed_access_stage_of<DOctPair>::value);

    D_OC_CHECK((observational_stage_of_v<DOctArray, DOctVector>
                    == observational_stage_of<DOctArray, DOctVector>::value));
    D_OC_CHECK((observational_stage_of_v<DOctArray, DOctArray>
                    == observational_stage_of<DOctArray, DOctArray>::value));

    D_OC_CHECK((is_compile_time_comparison_v<DOctArray, DOctArray>
                    == is_compile_time_comparison<DOctArray,
                                                  DOctArray>::value));
    D_OC_CHECK((is_compile_time_comparison_v<DOctVector, DOctVector>
                    == is_compile_time_comparison<DOctVector,
                                                  DOctVector>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
