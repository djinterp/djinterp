/*******************************************************************************
* djinterp [cpp]                    mutable_container_traits_tests_umbrellas.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/mutable_container_traits_tests_umbrellas.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "mutable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  Capability umbrellas
// ===========================================================================
//   The umbrellas are OR-combinations of the section I signals, plus one
// conjunction (is_immutable_container) that gates on the size() marker
// consumed from bounded_container_traits.hpp.  These tests fix both the
// per-type answers and the combination shape itself.


/*
tests_is_element_mutable_container
  Verifies the element-mutation umbrella: an EXISTING element can be
  overwritten in place.
  Tests the following:
  - the random-access sequences and array qualify
  - vector<bool> qualifies through the subscript signal alone, having no data()
  - the node sequences do NOT qualify: the header scopes element mutation to a
    writable operator[] or data(), so a list is element-immutable even though
    its elements are reachable and writable through an iterator
  - set and map do not qualify: neither exposes a writable element handle
  - either signal alone suffices, and neither near-miss does
*/
bool tests_is_element_mutable_container()
{
    bool ok = true;

    // random access and fixed extent
    D_MC_CHECK(is_element_mutable_container<DMcVectorInt>::value  == true);
    D_MC_CHECK(is_element_mutable_container<DMcVectorBool>::value == true);
    D_MC_CHECK(is_element_mutable_container<DMcDequeInt>::value   == true);
    D_MC_CHECK(is_element_mutable_container<DMcString>::value      == true);
    D_MC_CHECK(is_element_mutable_container<DMcArray4>::value     == true);
    D_MC_CHECK(is_element_mutable_container<DMcArray0>::value     == true);

    // node sequences expose no element handle
    D_MC_CHECK(is_element_mutable_container<DMcListInt>::value  == false);
    D_MC_CHECK(is_element_mutable_container<DMcFlistInt>::value == false);

    // associative disciplines expose no element handle
    D_MC_CHECK(is_element_mutable_container<DMcSetInt>::value      == false);
    D_MC_CHECK(is_element_mutable_container<DMcMultisetInt>::value == false);
    D_MC_CHECK(is_element_mutable_container<DMcMapIi>::value       == false);
    D_MC_CHECK(is_element_mutable_container<DMcUmapIi>::value      == false);

    // non-containers
    D_MC_CHECK(is_element_mutable_container<int>::value        == false);
    D_MC_CHECK(is_element_mutable_container<void>::value       == false);
    D_MC_CHECK(is_element_mutable_container<DMcPairIi>::value == false);
    D_MC_CHECK(is_element_mutable_container<DMcTupleI>::value == false);

    // either signal alone is enough
    D_MC_CHECK(is_element_mutable_container<DMcSubscriptOnly>::value == true);
    D_MC_CHECK(is_element_mutable_container<DMcDataOnly>::value      == true);

    // near-misses
    D_MC_CHECK(is_element_mutable_container<DMcConstSubscriptOnly>::value
                   == false);
    D_MC_CHECK(is_element_mutable_container<DMcConstDataOnly>::value
                   == false);
    D_MC_CHECK(is_element_mutable_container<DMcVoidData>::value
                   == false);
    D_MC_CHECK(is_element_mutable_container<DMcSubscriptWrongValue>::value
                   == false);

    // a structural-only probe must not promote
    D_MC_CHECK(is_element_mutable_container<DMcPushBackOnly>::value
                   == false);

    return ok;
}

/*
tests_is_structurally_mutable_container
  Verifies the structural-mutation umbrella: the SET of elements can change.
  Tests the following:
  - every growable or erasable std container qualifies, sequences and
    associative disciplines alike
  - array does NOT qualify: its extent is fixed
  - each of the six structural signals promotes on its own
  - an element-only probe does not promote
*/
bool tests_is_structurally_mutable_container()
{
    bool ok = true;

    // sequences
    D_MC_CHECK(is_structurally_mutable_container<DMcVectorInt>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcDequeInt>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcListInt>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcFlistInt>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcString>::value
                   == true);

    // associative disciplines
    D_MC_CHECK(is_structurally_mutable_container<DMcSetInt>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcMultisetInt>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcMapIi>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcMultimapIi>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcUsetInt>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcUmapIi>::value
                   == true);

    // fixed extent
    D_MC_CHECK(is_structurally_mutable_container<DMcArray4>::value == false);
    D_MC_CHECK(is_structurally_mutable_container<DMcArray0>::value == false);

    // non-containers
    D_MC_CHECK(is_structurally_mutable_container<int>::value  == false);
    D_MC_CHECK(is_structurally_mutable_container<void>::value == false);

    // each structural signal promotes on its own
    D_MC_CHECK(is_structurally_mutable_container<DMcPushBackOnly>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcPushFrontOnly>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcInsertOnly>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcEraseOnly>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcClearOnly>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<DMcResizeOnly>::value
                   == true);

    // element-only does not
    D_MC_CHECK(is_structurally_mutable_container<DMcSubscriptOnly>::value
                   == false);
    D_MC_CHECK(is_structurally_mutable_container<DMcDataOnly>::value
                   == false);

    return ok;
}

/*
tests_is_mutable_container
  Verifies the umbrella over both capabilities - element OR structural.
  Tests the following:
  - a container with either capability qualifies, and one with both does too
  - the corners are covered: element-only (array), structural-only (set), both
    (vector), neither (string_view)
  - a type with no mutators does not qualify, whether or not it is a container
*/
bool tests_is_mutable_container()
{
    bool ok = true;

    // both capabilities
    D_MC_CHECK(is_mutable_container<DMcVectorInt>::value == true);
    D_MC_CHECK(is_mutable_container<DMcDequeInt>::value  == true);
    D_MC_CHECK(is_mutable_container<DMcString>::value     == true);

    // structural only
    D_MC_CHECK(is_mutable_container<DMcListInt>::value  == true);
    D_MC_CHECK(is_mutable_container<DMcFlistInt>::value == true);
    D_MC_CHECK(is_mutable_container<DMcSetInt>::value   == true);
    D_MC_CHECK(is_mutable_container<DMcMapIi>::value    == true);

    // element only
    D_MC_CHECK(is_mutable_container<DMcArray4>::value == true);
    D_MC_CHECK(is_mutable_container<DMcArray0>::value == true);

    // neither
    D_MC_CHECK(is_mutable_container<DMcInitList>::value == false);
    D_MC_CHECK(is_mutable_container<DMcSizedOnly>::value == false);
    D_MC_CHECK(is_mutable_container<DMcBare>::value       == false);
    D_MC_CHECK(is_mutable_container<int>::value           == false);
    D_MC_CHECK(is_mutable_container<void>::value          == false);
    D_MC_CHECK(is_mutable_container<DMcPairIi>::value    == false);
    D_MC_CHECK(is_mutable_container<DMcIntPtr>::value    == false);
    D_MC_CHECK(is_mutable_container<DMcIntArray>::value  == false);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_MC_CHECK(is_mutable_container<DMcStringView>::value == false);
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // a span is non-owning yet writable through - element mutation without
    // ownership, so it is mutable
    D_MC_CHECK(is_mutable_container<DMcSpanInt>::value == true);
#endif

    return ok;
}

/*
tests_is_immutable_container
  Verifies the conjunction that names a type which LOOKS like a container - it
  has a size() - yet exposes no mutator at all.
  Tests the following:
  - the read-only views qualify (string_view, initializer_list) as do the
    const-observation probes
  - a mutable container never qualifies, however it mutates
  - a type with no size() does NOT qualify even with no mutators: container-
    hood was never established, which is the distinction this trait exists to
    draw against the immutable GRADE
  - span does NOT qualify: it has a size() but writes through
  - the trait equals its definition, size() AND no mutation, term by term
*/
bool tests_is_immutable_container()
{
    bool ok = true;

    // read-only containers
    D_MC_CHECK(is_immutable_container<DMcInitList>::value       == true);
    D_MC_CHECK(is_immutable_container<DMcSizedOnly>::value      == true);
    D_MC_CHECK(is_immutable_container<DMcImmutableSized>::value == true);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_MC_CHECK(is_immutable_container<DMcStringView>::value == true);
#endif

    // mutable containers never qualify
    D_MC_CHECK(is_immutable_container<DMcVectorInt>::value == false);
    D_MC_CHECK(is_immutable_container<DMcListInt>::value   == false);
    D_MC_CHECK(is_immutable_container<DMcSetInt>::value    == false);
    D_MC_CHECK(is_immutable_container<DMcMapIi>::value     == false);
    D_MC_CHECK(is_immutable_container<DMcArray4>::value    == false);

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // has a size(), but writes through - not immutable
    D_MC_CHECK(is_immutable_container<DMcSpanInt>::value == false);
#endif

    // no size() means no container-hood, so no immutable CONTAINER
    D_MC_CHECK(is_immutable_container<DMcBare>::value      == false);
    D_MC_CHECK(is_immutable_container<int>::value          == false);
    D_MC_CHECK(is_immutable_container<void>::value         == false);
    D_MC_CHECK(is_immutable_container<DMcPairIi>::value   == false);
    D_MC_CHECK(is_immutable_container<DMcTupleI>::value   == false);
    D_MC_CHECK(is_immutable_container<DMcIntPtr>::value   == false);
    D_MC_CHECK(is_immutable_container<DMcIntArray>::value == false);

    // a probe that has the marks of a discipline but no mutators still
    // qualifies - the access dimension does not feed the grade
    D_MC_CHECK(is_immutable_container<DMcKeyTypeOnly>::value == false);

    // term-by-term agreement with the definition (has_size_signal is consumed
    // from bounded_container_traits.hpp, its single owner)
    D_MC_CHECK(is_immutable_container<DMcSizedOnly>::value
                   == (    has_size_signal<DMcSizedOnly>::value
                        && !is_mutable_container<DMcSizedOnly>::value ));
    D_MC_CHECK(is_immutable_container<DMcVectorInt>::value
                   == (    has_size_signal<DMcVectorInt>::value
                        && !is_mutable_container<DMcVectorInt>::value ));
    D_MC_CHECK(is_immutable_container<DMcBare>::value
                   == (    has_size_signal<DMcBare>::value
                        && !is_mutable_container<DMcBare>::value ));

    return ok;
}

/*
tests_umbrella_signal_composition
  Verifies the SHAPE of the umbrellas, not merely their per-type answers: each
  is the disjunction of its constituent signals.
  Tests the following:
  - is_element_mutable_container equals subscript OR data
  - is_structurally_mutable_container equals the disjunction of all six
    structural signals
  - is_mutable_container equals element OR structural
  - the identities are checked across carriers that light different subsets, so
    a dropped or duplicated term cannot pass
*/
bool tests_umbrella_signal_composition()
{
    bool ok = true;

    // element = subscript OR data
    D_MC_CHECK(is_element_mutable_container<DMcVectorInt>::value
                   == (    has_mutable_subscript_signal<DMcVectorInt>::value
                        || has_mutable_data_signal<DMcVectorInt>::value ));
    D_MC_CHECK(is_element_mutable_container<DMcVectorBool>::value
                   == (    has_mutable_subscript_signal<DMcVectorBool>::value
                        || has_mutable_data_signal<DMcVectorBool>::value ));
    D_MC_CHECK(is_element_mutable_container<DMcListInt>::value
                   == (    has_mutable_subscript_signal<DMcListInt>::value
                        || has_mutable_data_signal<DMcListInt>::value ));

    // structural = the six-way disjunction
    D_MC_CHECK(is_structurally_mutable_container<DMcVectorInt>::value
                   == (    has_push_back_signal<DMcVectorInt>::value
                        || has_push_front_signal<DMcVectorInt>::value
                        || has_insert_signal<DMcVectorInt>::value
                        || has_erase_signal<DMcVectorInt>::value
                        || has_clear_signal<DMcVectorInt>::value
                        || has_resize_signal<DMcVectorInt>::value ));
    D_MC_CHECK(is_structurally_mutable_container<DMcSetInt>::value
                   == (    has_push_back_signal<DMcSetInt>::value
                        || has_push_front_signal<DMcSetInt>::value
                        || has_insert_signal<DMcSetInt>::value
                        || has_erase_signal<DMcSetInt>::value
                        || has_clear_signal<DMcSetInt>::value
                        || has_resize_signal<DMcSetInt>::value ));
    D_MC_CHECK(is_structurally_mutable_container<DMcArray4>::value
                   == (    has_push_back_signal<DMcArray4>::value
                        || has_push_front_signal<DMcArray4>::value
                        || has_insert_signal<DMcArray4>::value
                        || has_erase_signal<DMcArray4>::value
                        || has_clear_signal<DMcArray4>::value
                        || has_resize_signal<DMcArray4>::value ));

    // mutable = element OR structural
    D_MC_CHECK(is_mutable_container<DMcVectorInt>::value
                   == (   is_element_mutable_container<DMcVectorInt>::value
                       || is_structurally_mutable_container<
                              DMcVectorInt>::value ));
    D_MC_CHECK(is_mutable_container<DMcArray4>::value
                   == (   is_element_mutable_container<DMcArray4>::value
                       || is_structurally_mutable_container<
                              DMcArray4>::value ));
    D_MC_CHECK(is_mutable_container<DMcSetInt>::value
                   == (   is_element_mutable_container<DMcSetInt>::value
                       || is_structurally_mutable_container<
                              DMcSetInt>::value ));
    D_MC_CHECK(is_mutable_container<DMcSizedOnly>::value
                   == (   is_element_mutable_container<DMcSizedOnly>::value
                       || is_structurally_mutable_container<
                              DMcSizedOnly>::value ));

    // the two capabilities are independent: all four corners are occupied
    D_MC_CHECK(   ( is_element_mutable_container<DMcArray4>::value == true )
               && ( is_structurally_mutable_container<DMcArray4>::value
                        == false ));
    D_MC_CHECK(   ( is_element_mutable_container<DMcSetInt>::value == false )
               && ( is_structurally_mutable_container<DMcSetInt>::value
                        == true ));
    D_MC_CHECK(   ( is_element_mutable_container<DMcVectorInt>::value
                        == true )
               && ( is_structurally_mutable_container<DMcVectorInt>::value
                        == true ));
    D_MC_CHECK(   ( is_element_mutable_container<DMcSizedOnly>::value
                        == false )
               && ( is_structurally_mutable_container<DMcSizedOnly>::value
                        == false ));

    return ok;
}

/*
tests_umbrella_cv_ref_invariance
  Verifies that the umbrellas inherit the clean_t contract from the signals
  they combine.
  Tests the following:
  - all four umbrellas, across positive and negative carriers, in every cv- and
    reference-qualified spelling
  - const in particular: a const vector still reads fully capable, because the
    trait describes the TYPE's interface, not the access path in hand
*/
bool tests_umbrella_cv_ref_invariance()
{
    bool ok = true;

    D_MC_CV_REF_AGREE(is_element_mutable_container, DMcVectorInt, true);
    D_MC_CV_REF_AGREE(is_element_mutable_container, DMcArray4,    true);
    D_MC_CV_REF_AGREE(is_element_mutable_container, DMcSetInt,    false);
    D_MC_CV_REF_AGREE(is_element_mutable_container, DMcListInt,   false);

    D_MC_CV_REF_AGREE(is_structurally_mutable_container, DMcVectorInt, true);
    D_MC_CV_REF_AGREE(is_structurally_mutable_container, DMcSetInt,    true);
    D_MC_CV_REF_AGREE(is_structurally_mutable_container, DMcArray4,    false);

    D_MC_CV_REF_AGREE(is_mutable_container, DMcVectorInt,  true);
    D_MC_CV_REF_AGREE(is_mutable_container, DMcSetInt,     true);
    D_MC_CV_REF_AGREE(is_mutable_container, DMcArray4,     true);
    D_MC_CV_REF_AGREE(is_mutable_container, DMcSizedOnly,  false);
    D_MC_CV_REF_AGREE(is_mutable_container, DMcBare,        false);

    D_MC_CV_REF_AGREE(is_immutable_container, DMcSizedOnly,  true);
    D_MC_CV_REF_AGREE(is_immutable_container, DMcInitList,   true);
    D_MC_CV_REF_AGREE(is_immutable_container, DMcVectorInt,  false);
    D_MC_CV_REF_AGREE(is_immutable_container, DMcBare,        false);

    return ok;
}

/*
tests_umbrella_value_companions
  Verifies the `_v` companions of the four umbrellas.
  Tests the following:
  - each companion agrees with its trait's ::value
  - the companions are usable in a constant expression
  - gated on variable-template support, as in section I
*/
bool tests_umbrella_value_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_MC_STATIC_CHECK(is_mutable_container_v<DMcVectorInt>    == true);
    D_MC_STATIC_CHECK(is_immutable_container_v<DMcSizedOnly>  == true);

    D_MC_CHECK(is_element_mutable_container_v<DMcVectorInt>
                   == is_element_mutable_container<DMcVectorInt>::value);
    D_MC_CHECK(is_element_mutable_container_v<DMcSetInt>
                   == is_element_mutable_container<DMcSetInt>::value);

    D_MC_CHECK(is_structurally_mutable_container_v<DMcSetInt>
                   == is_structurally_mutable_container<DMcSetInt>::value);
    D_MC_CHECK(is_structurally_mutable_container_v<DMcArray4>
                   == is_structurally_mutable_container<DMcArray4>::value);

    D_MC_CHECK(is_mutable_container_v<DMcVectorInt>
                   == is_mutable_container<DMcVectorInt>::value);
    D_MC_CHECK(is_mutable_container_v<DMcSizedOnly>
                   == is_mutable_container<DMcSizedOnly>::value);

    D_MC_CHECK(is_immutable_container_v<DMcSizedOnly>
                   == is_immutable_container<DMcSizedOnly>::value);
    D_MC_CHECK(is_immutable_container_v<DMcVectorInt>
                   == is_immutable_container<DMcVectorInt>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
