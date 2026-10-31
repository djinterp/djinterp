/*******************************************************************************
* djinterp [cpp]          observational_comparison_traits_tests_shared_level.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/observational_comparison_traits_tests_shared_level.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "observational_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// IV.  The shared-interface principle
// ===========================================================================
//   To compare c and c' one may use only the operations BOTH expose, so the
// decidable comparison is complete for the finest level that shared interface
// resolves and no finer.  The trait is the principle in one operation: take
// each type's own level, then the coarser of the two.


/*
tests_shared_observational_level
  Verifies the level at which named pairs can be decided.
  Tests the following:
  - a pair of like types decides at their common level, at every rung from str
    down to none
  - a mixed pair decides at the coarser of the two, never the finer
  - the trait is SYMMETRIC in its arguments, since the meet is commutative -
    which matters, because a comparison is not directed
*/
bool tests_shared_observational_level()
{
    bool ok = true;

    // like pairs decide at their common level
    D_OC_CHECK(shared_observational_level<DOctNested, DOctNested>::value
                   == observational_level::str);
    D_OC_CHECK(shared_observational_level<DOctVector, DOctVector>::value
                   == observational_level::seq);
    D_OC_CHECK(shared_observational_level<DOctSet, DOctSet>::value
                   == observational_level::seq);
    D_OC_CHECK(shared_observational_level<DOctCountOnly,
                                          DOctCountOnly>::value
                   == observational_level::bag);
    D_OC_CHECK(shared_observational_level<DOctContainsOnly,
                                          DOctContainsOnly>::value
                   == observational_level::set);
    D_OC_CHECK(shared_observational_level<DOctSizeOnly,
                                          DOctSizeOnly>::value
                   == observational_level::size);
    D_OC_CHECK(shared_observational_level<DOctBare, DOctBare>::value
                   == observational_level::none);

    // mixed pairs decide at the coarser
    D_OC_CHECK(shared_observational_level<DOctNested, DOctVector>::value
                   == observational_level::seq);
    D_OC_CHECK(shared_observational_level<DOctVector, DOctCountOnly>::value
                   == observational_level::bag);
    D_OC_CHECK(shared_observational_level<DOctCountOnly,
                                          DOctContainsOnly>::value
                   == observational_level::set);
    D_OC_CHECK(shared_observational_level<DOctContainsOnly,
                                          DOctSizeOnly>::value
                   == observational_level::size);
    D_OC_CHECK(shared_observational_level<DOctNested, DOctBare>::value
                   == observational_level::none);

    // symmetric
    D_OC_CHECK(shared_observational_level<DOctNested, DOctVector>::value
                   == shared_observational_level<DOctVector,
                                                 DOctNested>::value);
    D_OC_CHECK(shared_observational_level<DOctVector, DOctSizeOnly>::value
                   == shared_observational_level<DOctSizeOnly,
                                                 DOctVector>::value);
    D_OC_CHECK(shared_observational_level<DOctBare, DOctFullInterface>::value
                   == shared_observational_level<DOctFullInterface,
                                                 DOctBare>::value);

    return ok;
}

/*
tests_shared_level_is_meet
  Verifies that the trait really is the meet of the two operands' levels, and
  inherits the meet's algebra.
  Tests the following:
  - the definitional identity, on a spread of pairs covering every combination
    of rungs that the fixtures reach
  - IDEMPOTENCE: a type paired with itself decides at its own level, so
    comparing like with like loses nothing
  - BOUNDEDNESS: the shared level is subsumed by each operand's own level, so
    pairing never sharpens what either type could resolve alone
  - the coarser operand DOMINATES: if one operand's level already subsumes the
    other's, the result is the other's exactly
*/
bool tests_shared_level_is_meet()
{
    bool ok = true;

    // the definitional identity, across the rungs
    D_OC_CHECK((oct_shared_is_meet<DOctNested,       DOctVector>()));
    D_OC_CHECK((oct_shared_is_meet<DOctVector,       DOctSet>()));
    D_OC_CHECK((oct_shared_is_meet<DOctSet,          DOctCountOnly>()));
    D_OC_CHECK((oct_shared_is_meet<DOctCountOnly,   DOctContainsOnly>()));
    D_OC_CHECK((oct_shared_is_meet<DOctContainsOnly, DOctSizeOnly>()));
    D_OC_CHECK((oct_shared_is_meet<DOctSizeOnly,    DOctBare>()));
    D_OC_CHECK((oct_shared_is_meet<DOctBare,         DOctNested>()));
    D_OC_CHECK((oct_shared_is_meet<DOctFullInterface, DOctFlist>()));
    D_OC_CHECK((oct_shared_is_meet<DOctRawArray,    DOctMapInt>()));
    D_OC_CHECK((oct_shared_is_meet<DOctPair,         DOctArray>()));

    // idempotent
    D_OC_CHECK(shared_observational_level<DOctNested, DOctNested>::value
                   == observational_level_of<DOctNested>::value);
    D_OC_CHECK(shared_observational_level<DOctSet, DOctSet>::value
                   == observational_level_of<DOctSet>::value);
    D_OC_CHECK(shared_observational_level<DOctSizeOnly,
                                          DOctSizeOnly>::value
                   == observational_level_of<DOctSizeOnly>::value);
    D_OC_CHECK(shared_observational_level<DOctBare, DOctBare>::value
                   == observational_level_of<DOctBare>::value);

    // bounded by each operand
    D_OC_CHECK(observational_level_subsumes(
                   observational_level_of<DOctNested>::value,
                   shared_observational_level<DOctNested,
                                              DOctSizeOnly>::value));
    D_OC_CHECK(observational_level_subsumes(
                   observational_level_of<DOctSizeOnly>::value,
                   shared_observational_level<DOctNested,
                                              DOctSizeOnly>::value));
    D_OC_CHECK(observational_level_subsumes(
                   observational_level_of<DOctVector>::value,
                   shared_observational_level<DOctVector,
                                              DOctContainsOnly>::value));
    D_OC_CHECK(observational_level_subsumes(
                   observational_level_of<DOctContainsOnly>::value,
                   shared_observational_level<DOctVector,
                                              DOctContainsOnly>::value));

    // the coarser operand dominates exactly
    D_OC_CHECK(shared_observational_level<DOctNested,
                                          DOctContainsOnly>::value
                   == observational_level_of<DOctContainsOnly>::value);
    D_OC_CHECK(shared_observational_level<DOctFullInterface,
                                          DOctSizeOnly>::value
                   == observational_level_of<DOctSizeOnly>::value);

    return ok;
}

/*
tests_shared_level_principle
  Verifies the principle as the header states it: an iterable container
  compared with an opaque one falls from seq to whatever else is shared -
  contains gives set, else size, else identity.  Each step of that sentence
  gets a check.

  This is also where the practical consequence lives.  A level the shared
  interface cannot reach is not thereby FALSE - two containers may well be
  equal at a finer level - it is merely unobservable through the operations
  they have in common.  The trait reports reach, not fact.

  Tests the following:
  - the descent named in the header, rung by rung: seq against a contains-only
    counterpart gives set, against a size-only one gives size, against a bare
    one gives none
  - a nested container compared with a flat one falls from str to seq: the
    shape is real on one side but unreadable through the shared surface
  - the fall is a property of the PAIR, not of either type: each operand keeps
    its own level, which is what makes conversion the remedy the header points
    to
  - pairing with a bare type annihilates, and pairing with a str-resolving type
    is the identity, matching the meet's absorbing and neutral elements
*/
bool tests_shared_level_principle()
{
    bool ok = true;

    // an iterable container against progressively more opaque counterparts
    D_OC_CHECK(shared_observational_level<DOctVector,
                                          DOctContainsOnly>::value
                   == observational_level::set);
    D_OC_CHECK(shared_observational_level<DOctVector, DOctSizeOnly>::value
                   == observational_level::size);
    D_OC_CHECK(shared_observational_level<DOctVector, DOctBare>::value
                   == observational_level::none);

    // ... and against one that keeps its counts
    D_OC_CHECK(shared_observational_level<DOctVector, DOctCountOnly>::value
                   == observational_level::bag);

    // a nested container against a flat one loses the shape
    D_OC_CHECK(shared_observational_level<DOctNested, DOctVector>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<DOctNested>::value
                   == observational_level::str);

    // the fall belongs to the pair - each operand keeps its own level
    D_OC_CHECK(observational_level_of<DOctVector>::value
                   == observational_level::seq);
    D_OC_CHECK(observational_level_of<DOctContainsOnly>::value
                   == observational_level::set);
    D_OC_CHECK(   ( shared_observational_level<DOctVector,
                                               DOctContainsOnly>::value
                        == observational_level::set )
               && ( observational_level_of<DOctVector>::value
                        == observational_level::seq ));

    // a bare counterpart annihilates, whatever the other side resolves
    D_OC_CHECK(shared_observational_level<DOctFullInterface,
                                          DOctBare>::value
                   == observational_level::none);
    D_OC_CHECK(shared_observational_level<DOctNested, DOctBare>::value
                   == observational_level::none);
    D_OC_CHECK(shared_observational_level<DOctSet, DOctBare>::value
                   == observational_level::none);

    // a str-resolving counterpart is neutral
    D_OC_CHECK(shared_observational_level<DOctNested, DOctSet>::value
                   == observational_level_of<DOctSet>::value);
    D_OC_CHECK(shared_observational_level<DOctNested,
                                          DOctCountOnly>::value
                   == observational_level_of<DOctCountOnly>::value);
    D_OC_CHECK(shared_observational_level<DOctNested,
                                          DOctFullInterface>::value
                   == observational_level::str);

    return ok;
}

/*
tests_shared_level_cv_ref
  Verifies that both operands are cleaned independently.

  As in section III, the cleaning happens twice: this trait writes clean_t
  around each operand before handing it to observational_level_of, which
  cleans it again.  The outer layer is redundant and its removal is
  unobservable, so what follows pins the invariance itself rather than the
  route taken to it.

  Tests the following:
  - the shared level is unchanged across cv- and reference-qualified spellings
    of either operand, and of both at once
  - carriers at several rungs, so the invariance is not an accident of one
    level
*/
bool tests_shared_level_cv_ref()
{
    bool ok = true;

    D_OC_CV_REF_PAIR_AGREE(shared_observational_level,
                           DOctNested, DOctVector,
                           observational_level::seq);
    D_OC_CV_REF_PAIR_AGREE(shared_observational_level,
                           DOctVector, DOctSizeOnly,
                           observational_level::size);
    D_OC_CV_REF_PAIR_AGREE(shared_observational_level,
                           DOctNested, DOctNested,
                           observational_level::str);
    D_OC_CV_REF_PAIR_AGREE(shared_observational_level,
                           DOctSet, DOctBare,
                           observational_level::none);
    D_OC_CV_REF_PAIR_AGREE(shared_observational_level,
                           DOctCountOnly, DOctContainsOnly,
                           observational_level::set);

    // both operands volatile at once
    D_OC_CHECK((shared_observational_level<volatile DOctNested,
                                           volatile DOctVector>::value
                    == observational_level::seq));

    return ok;
}

/*
tests_shared_level_companions
  Verifies the `_t` alias and the `_v` companion.
  Tests the following:
  - the `_t` alias is the integral_constant naming the shared level
  - the `_v` companion agrees with ::value across several pairings
  - both are usable in a constant expression
  - the `_v` half is gated on variable-template support
*/
bool tests_shared_level_companions()
{
    bool ok = true;

    // the _t alias is the integral_constant, exactly
    D_OC_STATIC_CHECK(
        std::is_same<shared_observational_level_t<DOctNested, DOctVector>,
                     std::integral_constant<observational_level,
                                            observational_level::seq> >::value);
    D_OC_STATIC_CHECK(
        std::is_same<shared_observational_level_t<DOctVector, DOctBare>,
                     std::integral_constant<
                         observational_level,
                         observational_level::none> >::value);

    // ... and carries the same value the trait reports
    D_OC_CHECK((shared_observational_level_t<DOctSet, DOctCountOnly>::value
                    == shared_observational_level<DOctSet,
                                                  DOctCountOnly>::value));

    // usable in a constant expression
    D_OC_STATIC_CHECK(
        shared_observational_level<DOctNested, DOctNested>::value
            == observational_level::str);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OC_STATIC_CHECK(
        (shared_observational_level_v<DOctVector, DOctSizeOnly>
             == observational_level::size));

    D_OC_CHECK((shared_observational_level_v<DOctNested, DOctVector>
                    == shared_observational_level<DOctNested,
                                                  DOctVector>::value));
    D_OC_CHECK((shared_observational_level_v<DOctSet, DOctBare>
                    == shared_observational_level<DOctSet, DOctBare>::value));
    D_OC_CHECK((shared_observational_level_v<DOctCountOnly,
                                             DOctContainsOnly>
                    == shared_observational_level<DOctCountOnly,
                                                  DOctContainsOnly>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
