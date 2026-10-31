/*******************************************************************************
* djinterp [cpp]                        mutable_container_traits_tests_grade.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/mutable_container_traits_tests_grade.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "mutable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. The mutability grade
// ===========================================================================
//   The grade summarises the two independent capabilities as the four corners
// of the (element, structural) square.  Its value-level surface - the enum,
// the spelling, the four corner predicates and the two admission predicates -
// is exercised exhaustively: every predicate against every grade, so the truth
// tables are pinned entry by entry rather than sampled.


/*
tests_mutability_enum
  Verifies the grade enumeration itself.
  Tests the following:
  - it is an enumeration, and a SCOPED one: it does not implicitly convert to
    int, so a grade cannot be mistaken for a count or a bitmask
  - the four enumerators are pairwise distinct
  - a grade is usable as a non-type template argument, which is what lets
    mutability_of publish an integral_constant ::type
*/
bool tests_mutability_enum()
{
    bool ok = true;

    // an enumeration, and scoped
    D_MC_STATIC_CHECK(std::is_enum<mutability>::value == true);
    D_MC_STATIC_CHECK(std::is_convertible<mutability, int>::value == false);

    // pairwise distinct
    D_MC_CHECK(mutability::immutable       != mutability::element_mutable);
    D_MC_CHECK(mutability::immutable       != mutability::structural_mutable);
    D_MC_CHECK(mutability::immutable       != mutability::fully_mutable);
    D_MC_CHECK(mutability::element_mutable != mutability::structural_mutable);
    D_MC_CHECK(mutability::element_mutable != mutability::fully_mutable);
    D_MC_CHECK(mutability::structural_mutable != mutability::fully_mutable);

    // usable as a non-type template argument
    D_MC_STATIC_CHECK(
        std::integral_constant<mutability, mutability::fully_mutable>::value
            == mutability::fully_mutable);

    return ok;
}

/*
tests_mutability_name
  Verifies the stable spelling of each grade.
  Tests the following:
  - all four grades render their own name, exactly
  - the names are pairwise distinct, so a diagnostic cannot conflate two grades
  - the function is usable in a constant expression, which is what makes it
    legal as the initialiser of DMutableContainerClass::grade_name
  - the trailing conditional is the catch-all, reached here by fully_mutable
*/
bool tests_mutability_name()
{
    bool ok = true;

    // constant-expression usability, and first-character distinctness
    D_MC_STATIC_CHECK(mutability_name(mutability::immutable)[0] == 'i');
    D_MC_STATIC_CHECK(mutability_name(mutability::element_mutable)[0] == 'e');
    D_MC_STATIC_CHECK(
        mutability_name(mutability::structural_mutable)[0] == 's');
    D_MC_STATIC_CHECK(mutability_name(mutability::fully_mutable)[0] == 'f');

    // exact spellings
    D_MC_CHECK(std::strcmp(mutability_name(mutability::immutable),
                           "immutable") == 0);
    D_MC_CHECK(std::strcmp(mutability_name(mutability::element_mutable),
                           "element_mutable") == 0);
    D_MC_CHECK(std::strcmp(mutability_name(mutability::structural_mutable),
                           "structural_mutable") == 0);
    D_MC_CHECK(std::strcmp(mutability_name(mutability::fully_mutable),
                           "fully_mutable") == 0);

    // pairwise distinct spellings
    D_MC_CHECK(std::strcmp(mutability_name(mutability::immutable),
                           mutability_name(mutability::element_mutable))
                   != 0);
    D_MC_CHECK(std::strcmp(mutability_name(mutability::element_mutable),
                           mutability_name(mutability::structural_mutable))
                   != 0);
    D_MC_CHECK(std::strcmp(mutability_name(mutability::structural_mutable),
                           mutability_name(mutability::fully_mutable))
                   != 0);

    return ok;
}

/*
tests_mutability_grade_predicates
  Verifies the four corner predicates as a complete truth table - each
  predicate against each of the four grades, sixteen entries in all.  The four
  grades are bound to local names here so the table reads as a grid; the names
  are the enumerators and nothing else.
  Tests the following:
  - each predicate is true for its own grade and false for the other three
  - exactly one predicate holds for any given grade, so the four partition the
    grade space rather than overlapping or leaving a gap
  - the predicates are usable in a constant expression
*/
bool tests_mutability_grade_predicates()
{
    bool ok = true;

    const mutability imm  = mutability::immutable;
    const mutability elem = mutability::element_mutable;
    const mutability strc = mutability::structural_mutable;
    const mutability full = mutability::fully_mutable;

    // constant-expression usability
    D_MC_STATIC_CHECK(is_immutable_grade(mutability::immutable) == true);
    D_MC_STATIC_CHECK(is_fully_mutable_grade(mutability::immutable) == false);

    // is_immutable_grade
    D_MC_CHECK(is_immutable_grade(imm)  == true);
    D_MC_CHECK(is_immutable_grade(elem) == false);
    D_MC_CHECK(is_immutable_grade(strc) == false);
    D_MC_CHECK(is_immutable_grade(full) == false);

    // is_element_mutable_grade
    D_MC_CHECK(is_element_mutable_grade(imm)  == false);
    D_MC_CHECK(is_element_mutable_grade(elem) == true);
    D_MC_CHECK(is_element_mutable_grade(strc) == false);
    D_MC_CHECK(is_element_mutable_grade(full) == false);

    // is_structural_mutable_grade
    D_MC_CHECK(is_structural_mutable_grade(imm)  == false);
    D_MC_CHECK(is_structural_mutable_grade(elem) == false);
    D_MC_CHECK(is_structural_mutable_grade(strc) == true);
    D_MC_CHECK(is_structural_mutable_grade(full) == false);

    // is_fully_mutable_grade
    D_MC_CHECK(is_fully_mutable_grade(imm)  == false);
    D_MC_CHECK(is_fully_mutable_grade(elem) == false);
    D_MC_CHECK(is_fully_mutable_grade(strc) == false);
    D_MC_CHECK(is_fully_mutable_grade(full) == true);

    // exactly one predicate holds per grade - the four partition the space
    D_MC_CHECK(mc_grade_predicate_count(imm)  == 1);
    D_MC_CHECK(mc_grade_predicate_count(elem) == 1);
    D_MC_CHECK(mc_grade_predicate_count(strc) == 1);
    D_MC_CHECK(mc_grade_predicate_count(full) == 1);

    return ok;
}

/*
tests_mutability_grade_admits
  Verifies the two admission predicates, which ask what a grade PERMITS rather
  than what it is.
  Tests the following:
  - element admission holds for element_mutable and fully_mutable only
  - structural admission holds for structural_mutable and fully_mutable only
  - the two overlap exactly at fully_mutable and are both false exactly at
    immutable, so the pair reconstructs the two capability bits the grade was
    collapsed from - the collapse is lossless
  - both are usable in a constant expression
*/
bool tests_mutability_grade_admits()
{
    bool ok = true;

    const mutability imm  = mutability::immutable;
    const mutability elem = mutability::element_mutable;
    const mutability strc = mutability::structural_mutable;
    const mutability full = mutability::fully_mutable;

    // constant-expression usability
    D_MC_STATIC_CHECK(
        grade_admits_element_mutation(mutability::fully_mutable) == true);
    D_MC_STATIC_CHECK(
        grade_admits_structural_mutation(mutability::immutable) == false);

    // grade_admits_element_mutation
    D_MC_CHECK(grade_admits_element_mutation(imm)  == false);
    D_MC_CHECK(grade_admits_element_mutation(elem) == true);
    D_MC_CHECK(grade_admits_element_mutation(strc) == false);
    D_MC_CHECK(grade_admits_element_mutation(full) == true);

    // grade_admits_structural_mutation
    D_MC_CHECK(grade_admits_structural_mutation(imm)  == false);
    D_MC_CHECK(grade_admits_structural_mutation(elem) == false);
    D_MC_CHECK(grade_admits_structural_mutation(strc) == true);
    D_MC_CHECK(grade_admits_structural_mutation(full) == true);

    // the two admissions reconstruct the capability pair at every corner
    D_MC_CHECK(   ( grade_admits_element_mutation(imm)    == false )
               && ( grade_admits_structural_mutation(imm) == false ));
    D_MC_CHECK(   ( grade_admits_element_mutation(elem)    == true )
               && ( grade_admits_structural_mutation(elem) == false ));
    D_MC_CHECK(   ( grade_admits_element_mutation(strc)    == false )
               && ( grade_admits_structural_mutation(strc) == true ));
    D_MC_CHECK(   ( grade_admits_element_mutation(full)    == true )
               && ( grade_admits_structural_mutation(full) == true ));

    return ok;
}

/*
tests_mutability_of
  Verifies the grade computed for a type from its two capability bits.
  Tests the following:
  - all four corners are witnessed by real containers: fully (vector),
    structural only (set, list), element only (array), immutable (string_view)
  - a node sequence lands on structural_mutable, since the header scopes
    element mutation to a writable subscript or data()
  - a map lands on structural_mutable: its value_type is a pair, so no in-place
    whole-element overwrite is detected
  - a type with no mutators reads immutable whether or not it is a container -
    int and void included - which is the grade's documented independence from
    container-hood
*/
bool tests_mutability_of()
{
    bool ok = true;

    // fully mutable
    D_MC_CHECK(mutability_of<DMcVectorInt>::value
                   == mutability::fully_mutable);
    D_MC_CHECK(mutability_of<DMcVectorBool>::value
                   == mutability::fully_mutable);
    D_MC_CHECK(mutability_of<DMcDequeInt>::value
                   == mutability::fully_mutable);
    D_MC_CHECK(mutability_of<DMcString>::value
                   == mutability::fully_mutable);
    D_MC_CHECK(mutability_of<DMcAllSignals>::value
                   == mutability::fully_mutable);

    // structural only - node sequences and every associative discipline
    D_MC_CHECK(mutability_of<DMcListInt>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcFlistInt>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcSetInt>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcMultisetInt>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcMapIi>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcMultimapIi>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcUsetInt>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcUmapIi>::value
                   == mutability::structural_mutable);

    // element only - fixed extent, at a positive and a zero size
    D_MC_CHECK(mutability_of<DMcArray4>::value
                   == mutability::element_mutable);
    D_MC_CHECK(mutability_of<DMcArray0>::value
                   == mutability::element_mutable);

    // immutable
    D_MC_CHECK(mutability_of<DMcInitList>::value  == mutability::immutable);
    D_MC_CHECK(mutability_of<DMcSizedOnly>::value == mutability::immutable);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_MC_CHECK(mutability_of<DMcStringView>::value == mutability::immutable);
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_MC_CHECK(mutability_of<DMcSpanInt>::value
                   == mutability::element_mutable);
#endif

    // the grade does not require container-hood
    D_MC_CHECK(mutability_of<int>::value          == mutability::immutable);
    D_MC_CHECK(mutability_of<void>::value         == mutability::immutable);
    D_MC_CHECK(mutability_of<DMcBare>::value      == mutability::immutable);
    D_MC_CHECK(mutability_of<DMcPairIi>::value   == mutability::immutable);
    D_MC_CHECK(mutability_of<DMcTupleI>::value   == mutability::immutable);
    D_MC_CHECK(mutability_of<DMcIntPtr>::value   == mutability::immutable);
    D_MC_CHECK(mutability_of<DMcIntArray>::value == mutability::immutable);

    // ... and the immutable GRADE is not the immutable CONTAINER
    D_MC_CHECK(   ( mutability_of<int>::value == mutability::immutable )
               && ( is_immutable_container<int>::value == false ));
    D_MC_CHECK(   ( mutability_of<DMcSizedOnly>::value
                        == mutability::immutable )
               && ( is_immutable_container<DMcSizedOnly>::value == true ));

    // structural mutation does not imply dynamic storage: a bounded inline
    // buffer that grows within its capacity is fully mutable
    D_MC_CHECK(mutability_of<DMcFixedBuffer>::value
                   == mutability::fully_mutable);

    // each isolated structural signal reaches structural_mutable alone
    D_MC_CHECK(mutability_of<DMcPushBackOnly>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<DMcClearOnly>::value
                   == mutability::structural_mutable);
    D_MC_CHECK(mutability_of<mc_no_value_type>::value
                   == mutability::structural_mutable);

    // and each isolated element signal reaches element_mutable alone
    D_MC_CHECK(mutability_of<DMcSubscriptOnly>::value
                   == mutability::element_mutable);
    D_MC_CHECK(mutability_of<DMcDataOnly>::value
                   == mutability::element_mutable);

    return ok;
}

/*
tests_mutability_of_companions
  Verifies the ::type member and the `_v` companion of mutability_of.
  Tests the following:
  - ::type is exactly integral_constant<mutability, value>, for each of the
    four corners
  - mutability_of_t names that same type
  - the `_v` companion agrees with ::value and is usable in a constant
    expression, gated on the language's variable-template support
*/
bool tests_mutability_of_companions()
{
    bool ok = true;

    // ::type is the integral_constant carrying the grade
    D_MC_STATIC_CHECK(
        std::is_same<mutability_of<DMcVectorInt>::type,
                     std::integral_constant<mutability,
                                            mutability::fully_mutable>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<mutability_of<DMcSetInt>::type,
                     std::integral_constant<
                         mutability,
                         mutability::structural_mutable>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<mutability_of<DMcArray4>::type,
                     std::integral_constant<
                         mutability,
                         mutability::element_mutable>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<mutability_of<DMcBare>::type,
                     std::integral_constant<mutability,
                                            mutability::immutable>>::value);

    // the _t alias names the same type
    D_MC_STATIC_CHECK(
        std::is_same<mutability_of_t<DMcVectorInt>,
                     mutability_of<DMcVectorInt>::type>::value);
    D_MC_STATIC_CHECK(
        std::is_same<mutability_of_t<DMcSetInt>,
                     mutability_of<DMcSetInt>::type>::value);

    // and carries the same value
    D_MC_CHECK(mutability_of_t<DMcVectorInt>::value
                   == mutability_of<DMcVectorInt>::value);
    D_MC_CHECK(mutability_of_t<DMcArray4>::value
                   == mutability_of<DMcArray4>::value);
    D_MC_CHECK(mutability_of_t<DMcBare>::value
                   == mutability_of<DMcBare>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_MC_STATIC_CHECK(mutability_of_v<DMcVectorInt>
                          == mutability::fully_mutable);

    D_MC_CHECK(mutability_of_v<DMcVectorInt>
                   == mutability_of<DMcVectorInt>::value);
    D_MC_CHECK(mutability_of_v<DMcSetInt>
                   == mutability_of<DMcSetInt>::value);
    D_MC_CHECK(mutability_of_v<DMcArray4>
                   == mutability_of<DMcArray4>::value);
    D_MC_CHECK(mutability_of_v<DMcBare>
                   == mutability_of<DMcBare>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}

/*
tests_mutability_grade_aliases
  Verifies the dimension-naming aliases, which let a caller pair the grade with
  the access restriction without renaming anything underneath.
  Tests the following:
  - mutability_grade is the mutability enum itself, not a distinct type
  - mutability_grade_of<T> is mutability_of<T> itself, for several T
  - the aliases carry the value and the ::type through unchanged
*/
bool tests_mutability_grade_aliases()
{
    bool ok = true;

    // the grade dimension IS the mutability enum
    D_MC_STATIC_CHECK(std::is_same<mutability_grade, mutability>::value);

    // the grade trait IS mutability_of
    D_MC_STATIC_CHECK(
        std::is_same<mutability_grade_of<DMcVectorInt>,
                     mutability_of<DMcVectorInt>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<mutability_grade_of<DMcSetInt>,
                     mutability_of<DMcSetInt>>::value);
    D_MC_STATIC_CHECK(
        std::is_same<mutability_grade_of<void>, mutability_of<void>>::value);

    // and carries everything through
    D_MC_CHECK(mutability_grade_of<DMcVectorInt>::value
                   == mutability_of<DMcVectorInt>::value);
    D_MC_CHECK(mutability_grade_of<DMcArray4>::value
                   == mutability::element_mutable);
    D_MC_STATIC_CHECK(
        std::is_same<mutability_grade_of<DMcArray4>::type,
                     mutability_of<DMcArray4>::type>::value);

    return ok;
}

/*
tests_mutability_of_umbrella_agreement
  Verifies that the grade is a faithful re-encoding of the two capability bits
  it summarises - nothing is lost or invented in the collapse.
  Tests the following:
  - for every carrier, the grade's admissions reproduce the umbrellas exactly:
    element admission equals is_element_mutable_container, and structural
    admission equals is_structurally_mutable_container
  - is_immutable_grade holds exactly when neither umbrella does
  - the carriers span all four corners, so the round-trip is checked at each
*/
bool tests_mutability_of_umbrella_agreement()
{
    bool ok = true;

    // fully mutable corner
    D_MC_CHECK(grade_admits_element_mutation(
                   mutability_of<DMcVectorInt>::value)
                   == is_element_mutable_container<DMcVectorInt>::value);
    D_MC_CHECK(grade_admits_structural_mutation(
                   mutability_of<DMcVectorInt>::value)
                   == is_structurally_mutable_container<DMcVectorInt>::value);

    // structural corner
    D_MC_CHECK(grade_admits_element_mutation(mutability_of<DMcSetInt>::value)
                   == is_element_mutable_container<DMcSetInt>::value);
    D_MC_CHECK(grade_admits_structural_mutation(
                   mutability_of<DMcSetInt>::value)
                   == is_structurally_mutable_container<DMcSetInt>::value);

    // element corner
    D_MC_CHECK(grade_admits_element_mutation(mutability_of<DMcArray4>::value)
                   == is_element_mutable_container<DMcArray4>::value);
    D_MC_CHECK(grade_admits_structural_mutation(
                   mutability_of<DMcArray4>::value)
                   == is_structurally_mutable_container<DMcArray4>::value);

    // immutable corner
    D_MC_CHECK(grade_admits_element_mutation(
                   mutability_of<DMcSizedOnly>::value)
                   == is_element_mutable_container<DMcSizedOnly>::value);
    D_MC_CHECK(grade_admits_structural_mutation(
                   mutability_of<DMcSizedOnly>::value)
                   == is_structurally_mutable_container<DMcSizedOnly>::value);

    // the immutable grade is exactly "neither umbrella"
    D_MC_CHECK(is_immutable_grade(mutability_of<DMcSizedOnly>::value)
                   == !is_mutable_container<DMcSizedOnly>::value);
    D_MC_CHECK(is_immutable_grade(mutability_of<DMcVectorInt>::value)
                   == !is_mutable_container<DMcVectorInt>::value);
    D_MC_CHECK(is_immutable_grade(mutability_of<DMcListInt>::value)
                   == !is_mutable_container<DMcListInt>::value);
    D_MC_CHECK(is_immutable_grade(mutability_of<DMcArray4>::value)
                   == !is_mutable_container<DMcArray4>::value);

    return ok;
}

/*
tests_mutability_of_cv_ref_invariance
  Verifies that the grade inherits the clean_t contract.
  Tests the following:
  - all four grades are stable across every cv- and reference-qualified
    spelling of their carrier
  - const in particular: a const vector still grades fully_mutable, because the
    grade describes what the TYPE's interface admits, not what the handle in
    hand permits - the access WRAPPERS are the separate concern the header
    header-comment holds apart from this axis
*/
bool tests_mutability_of_cv_ref_invariance()
{
    bool ok = true;

    D_MC_CV_REF_VALUE(mutability_of, DMcVectorInt, mutability::fully_mutable);
    D_MC_CV_REF_VALUE(mutability_of, DMcString,     mutability::fully_mutable);
    D_MC_CV_REF_VALUE(mutability_of, DMcSetInt,
                      mutability::structural_mutable);
    D_MC_CV_REF_VALUE(mutability_of, DMcListInt,
                      mutability::structural_mutable);
    D_MC_CV_REF_VALUE(mutability_of, DMcArray4,  mutability::element_mutable);
    D_MC_CV_REF_VALUE(mutability_of, DMcSizedOnly, mutability::immutable);
    D_MC_CV_REF_VALUE(mutability_of, DMcBare,       mutability::immutable);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
