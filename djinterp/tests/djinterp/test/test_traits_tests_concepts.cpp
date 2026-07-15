// djinterp
#include "test_traits_tests.hpp"

// std
#include <concepts>
#include <type_traits>


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP
NS_TESTING


/*
tests_trait_concept_agree
  D_TEST_TRAIT_CONCEPT_AGREE - does a trait and its concept face still give the
  same answer?

  The framework layers a concept over almost every trait and states, header after
  header, that "the two agree by construction".  They agree by construction only
  for as long as the concept keeps FORWARDING to the trait.  The day someone
  tightens one and not the other, the trait-constrained overload and the
  concept-constrained overload begin to disagree about the same type - silently,
  because both still compile - and nothing in the build says so.

  Tests the following:
  - a forwarding concept agrees with its trait, on a positive and a negative
    subject, and on a non-class type
  - a DRIFTED concept - the same face with one extra clause the trait has not
    grown - disagrees, and the macro reports false.  This is the check that
    proves the macro can fail: `value_typed_drifted_c` is exactly what a concept
    looks like the day after somebody hardens it and forgets the trait
  - it is variadic, so a multi-argument trait and concept pair works
*/
bool
tests_trait_concept_agree()
{
    bool ok = true;

    // forwarding: agrees everywhere
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                          value_typed_c, sized));
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                          value_typed_c, plain));
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                          value_typed_c, int));
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                          value_typed_c, void));
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                          value_typed_c,
                                          fixtures::incomplete));

    // DRIFTED: the concept has a clause the trait has not, and the macro says so
    D_TT_CHECK(!D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                           value_typed_drifted_c, sized));

    // ...and the disagreement is real, not an artefact of the macro
    D_TT_CHECK(has_member_value_type<sized>::value);
    D_TT_CHECK(!value_typed_drifted_c<sized>);

    // the drift is invisible on the types where the extra clause happens not to
    // bite, which is exactly why a suite must not test agreement on one subject
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                          value_typed_drifted_c, int));
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(has_member_value_type,
                                          value_typed_drifted_c, plain));

    // multi-argument
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(std::is_same, std::same_as,
                                          int, int));
    D_TT_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(std::is_same, std::same_as,
                                          int, char));

    return ok;
}

/*
tests_declare_subsumes_refines
  D_TEST_DECLARE_SUBSUMES - the positive case.  A refinement ladder built the
  right way, with the stronger concept written IN TERMS OF the weaker one.

      concept sized_refined_c = value_typed_c<_Type> && requires ...

  The atomic constraints of value_typed_c literally appear in sized_refined_c, so
  the compiler can see the subsumption, the more-constrained overload wins
  outright, and the emitted trait reports true.

  Tests the following:
  - a type satisfying both concepts, with the ladder built correctly, subsumes
  - the mechanism is real: the two ranked overloads exist, and the rank-2 one is
    chosen.  That is what the trait's `== 2` clause is checking
*/
bool
tests_declare_subsumes_refines()
{
    bool ok = true;

    // the subject satisfies both
    D_TT_CHECK(value_typed_c<sized>);
    D_TT_CHECK(sized_refined_c<sized>);

    // ...and the ladder subsumes
    D_TT_CHECK(refines_correctly<sized>::value);

    return ok;
}

/*
tests_declare_subsumes_implies_only
  THE WHOLE POINT.  A ladder built by COPY-PASTING requirements instead of
  composing them.

      concept sized_restated_c = requires { typename _Type::value_type; } &&
                                 requires(const _Type& t) { t.size(); };

  It IMPLIES value_typed_c - every type satisfying it satisfies that, which is
  all a trait can say and all an implication test can check.  It does not SUBSUME
  it, because the two `requires` expressions are distinct atomic constraints as
  far as the compiler is concerned, and the overload resolver therefore cannot
  order them.

  The consequence is not academic.  Every pair of overloads constrained on the
  two is AMBIGUOUS - and the ambiguity does not appear until somebody writes the
  second overload, which may be months later and in someone else's file.  Until
  then the ladder looks perfect: it passes every implication test anyone thinks
  to write.

  D_TEST_DECLARE_SUBSUMES is the only thing that catches it, and it catches it by
  making the ambiguous call inside a detection probe, where an ambiguous overload
  set is a substitution failure rather than a diagnostic.

  Tests the following:
  - the restated concept IMPLIES the weaker one: every type satisfying it
    satisfies that.  So an implication test would pass
  - and it does NOT subsume it: the trait reports false
  - the two facts hold of the very same subject, which is what makes the bug
    invisible to everything except this check
  - two entirely unrelated concepts also fail to subsume, for the same reason -
    ambiguity, not falsity
*/
bool
tests_declare_subsumes_implies_only()
{
    bool ok = true;

    // it IMPLIES - an implication test would be satisfied
    D_TT_CHECK(sized_restated_c<sized>);
    D_TT_CHECK(value_typed_c<sized>);
    D_TT_CHECK(!sized_restated_c<sized> || value_typed_c<sized>);
    D_TT_CHECK(!sized_restated_c<plain> || value_typed_c<plain>);
    D_TT_CHECK(!sized_restated_c<int> || value_typed_c<int>);

    // ...and it does NOT SUBSUME.  Same subject, same concepts, and this is the
    // only check in the language that can tell the difference
    D_TT_CHECK(!merely_implies<sized>::value);

    // for contrast, the properly-composed ladder over the same subject does
    D_TT_CHECK(refines_correctly<sized>::value);

    // ...and the two concepts accept exactly the same types, so nothing that
    // looks at their ANSWERS can distinguish them
    D_TT_CHECK(sized_refined_c<sized> == sized_restated_c<sized>);
    D_TT_CHECK(sized_refined_c<plain> == sized_restated_c<plain>);
    D_TT_CHECK(sized_refined_c<int> == sized_restated_c<int>);
    D_TT_CHECK(sized_refined_c<fixtures::empty> ==
               sized_restated_c<fixtures::empty>);

    // ...while the SUBSUMPTION traits disagree about them completely
    D_TT_CHECK(refines_correctly<sized>::value !=
               merely_implies<sized>::value);

    // two unrelated concepts: neither subsumes, for the same reason
    D_TT_CHECK(!subsumes_nothing<sized>::value);

    return ok;
}

/*
tests_declare_subsumes_partial
  The rows where the subject does not satisfy both concepts.  The emitted trait
  guards on `CONCEPT_LESS<_Type> && CONCEPT_MORE<_Type> && (rank == 2)`, and each
  conjunct needs a subject.

  Tests the following:
  - satisfies the WEAKER concept only -> false (CONCEPT_MORE fails)
  - satisfies the STRONGER concept only -> false (CONCEPT_LESS fails).  Only
    reachable with two unrelated concepts, since a genuine refinement cannot be
    satisfied without its base
  - satisfies NEITHER -> false, and the ranked call has no viable overload at
    all - which is a substitution failure inside the probe, so detected_or_t
    supplies the 0 and the `== 2` clause rejects it.  A trait that named the
    call directly would hard-error here
  - the whole hostile zoo satisfies none of them, and none of it explodes
*/
bool
tests_declare_subsumes_partial()
{
    bool ok = true;

    // a type with value_type and no size(): the WEAKER concept only
    D_TT_CHECK(value_typed_c<fixtures::value_type_a>);
    D_TT_CHECK(!sized_refined_c<fixtures::value_type_a>);
    D_TT_CHECK(!refines_correctly<fixtures::value_type_a>::value);

    // a type with size() and no value_type: for the unrelated pair, this
    // satisfies CONCEPT_MORE and not CONCEPT_LESS
    D_TT_CHECK(unrelated_c<throwing_swap> == false);   // no size() on that one
    D_TT_CHECK(!value_typed_c<plain>);
    D_TT_CHECK(!subsumes_nothing<plain>::value);

    // neither concept: no viable overload at all, and the probe absorbs it
    D_TT_CHECK(!value_typed_c<plain>);
    D_TT_CHECK(!sized_refined_c<plain>);
    D_TT_CHECK(!refines_correctly<plain>::value);
    D_TT_CHECK(!refines_correctly<int>::value);
    D_TT_CHECK(!refines_correctly<void>::value);
    D_TT_CHECK(!merely_implies<int>::value);

    // and the whole zoo, without a single diagnostic
    D_TT_CHECK((holds_for_none<refines_correctly,
                               D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((holds_for_none<merely_implies,
                               D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((holds_for_none<subsumes_nothing,
                               D_TEST_HOSTILE_TYPES>::value));

    return ok;
}

/*
tests_declare_subsumes_shape
  The trait D_TEST_DECLARE_SUBSUMES emits derives from bool_constant, so it is a
  well-formed bool trait and composes with everything else in the toolkit.
  Tests the following:
  - is_bool_trait accepts it, in both polarities
  - it is well-formed over the whole hostile zoo
  - and it therefore drops into the quantifiers, which is how the zoo check in
    tests_declare_subsumes_partial is written at all
*/
bool
tests_declare_subsumes_shape()
{
    bool ok = true;

    D_TT_CHECK(is_bool_trait<refines_correctly<sized>>::value);
    D_TT_CHECK(is_bool_trait<refines_correctly<plain>>::value);
    D_TT_CHECK(is_bool_trait<merely_implies<sized>>::value);
    D_TT_CHECK(is_bool_trait<subsumes_nothing<int>>::value);

    D_TT_CHECK((trait_is_well_formed<refines_correctly,
                                     D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((trait_is_well_formed<merely_implies,
                                     D_TEST_HOSTILE_TYPES>::value));

    // it composes with the cv-ref matrix, and the matrix has something to say
    // about it.  The emitted trait is exactly as cv-ref-transparent as the
    // CONCEPTS it wraps, and no more:
    //
    //   const, and references     TRANSPARENT.  has_member_value_type strips
    //                             through clean_t, and `requires(const _Type& t)`
    //                             collapses `const sized&` back to itself
    //   volatile                  BLIND.  `requires(const _Type& t)` on a
    //                             volatile subject binds a `const volatile
    //                             sized&`, and sized::size() is const-qualified
    //                             but not volatile-qualified, so it is not
    //                             callable and the concept is not satisfied
    //
    // The matrix names that cell exactly, which is the whole reason it keeps its
    // cells instead of ANDing them together.  A concept written as
    // `requires(const _Type& t)` is not volatile-transparent, and almost nobody
    // knows it until a report says "volatile _Type"
    D_TT_CHECK(refines_correctly<sized>::value);
    D_TT_CHECK(refines_correctly<const sized&>::value);
    D_TT_CHECK(refines_correctly<sized&&>::value);
    D_TT_CHECK(!refines_correctly<volatile sized>::value);
    D_TT_CHECK(!refines_correctly<const volatile sized&>::value);

    D_TT_CHECK((!trait_ignores_cvref<refines_correctly, sized>::value));
    D_TT_FIRST("volatile _Type", refines_correctly, sized);

    return ok;
}


NS_END  // testing
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
