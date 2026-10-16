/*******************************************************************************
* djinterp [core]                   mutable_container_traits_tests_umbrellas.cpp
*
*
* path:      /tests/djinterp/core/container/traits/mutable_container_traits_tests_umbrellas.cpp
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
bool
tests_is_element_mutable_container()
{
    bool ok = true;

    // random access and fixed extent
    D_MC_CHECK(is_element_mutable_container<mc_vector_int>::value  == true);
    D_MC_CHECK(is_element_mutable_container<mc_vector_bool>::value == true);
    D_MC_CHECK(is_element_mutable_container<mc_deque_int>::value   == true);
    D_MC_CHECK(is_element_mutable_container<mc_string>::value      == true);
    D_MC_CHECK(is_element_mutable_container<mc_array_4>::value     == true);
    D_MC_CHECK(is_element_mutable_container<mc_array_0>::value     == true);

    // node sequences expose no element handle
    D_MC_CHECK(is_element_mutable_container<mc_list_int>::value  == false);
    D_MC_CHECK(is_element_mutable_container<mc_flist_int>::value == false);

    // associative disciplines expose no element handle
    D_MC_CHECK(is_element_mutable_container<mc_set_int>::value      == false);
    D_MC_CHECK(is_element_mutable_container<mc_multiset_int>::value == false);
    D_MC_CHECK(is_element_mutable_container<mc_map_ii>::value       == false);
    D_MC_CHECK(is_element_mutable_container<mc_umap_ii>::value      == false);

    // non-containers
    D_MC_CHECK(is_element_mutable_container<int>::value        == false);
    D_MC_CHECK(is_element_mutable_container<void>::value       == false);
    D_MC_CHECK(is_element_mutable_container<mc_pair_ii>::value == false);
    D_MC_CHECK(is_element_mutable_container<mc_tuple_i>::value == false);

    // either signal alone is enough
    D_MC_CHECK(is_element_mutable_container<mc_subscript_only>::value == true);
    D_MC_CHECK(is_element_mutable_container<mc_data_only>::value      == true);

    // near-misses
    D_MC_CHECK(is_element_mutable_container<mc_const_subscript_only>::value
                   == false);
    D_MC_CHECK(is_element_mutable_container<mc_const_data_only>::value
                   == false);
    D_MC_CHECK(is_element_mutable_container<mc_void_data>::value
                   == false);
    D_MC_CHECK(is_element_mutable_container<mc_subscript_wrong_value>::value
                   == false);

    // a structural-only probe must not promote
    D_MC_CHECK(is_element_mutable_container<mc_push_back_only>::value
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
bool
tests_is_structurally_mutable_container()
{
    bool ok = true;

    // sequences
    D_MC_CHECK(is_structurally_mutable_container<mc_vector_int>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_deque_int>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_list_int>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_flist_int>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_string>::value
                   == true);

    // associative disciplines
    D_MC_CHECK(is_structurally_mutable_container<mc_set_int>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_multiset_int>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_map_ii>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_multimap_ii>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_uset_int>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_umap_ii>::value
                   == true);

    // fixed extent
    D_MC_CHECK(is_structurally_mutable_container<mc_array_4>::value == false);
    D_MC_CHECK(is_structurally_mutable_container<mc_array_0>::value == false);

    // non-containers
    D_MC_CHECK(is_structurally_mutable_container<int>::value  == false);
    D_MC_CHECK(is_structurally_mutable_container<void>::value == false);

    // each structural signal promotes on its own
    D_MC_CHECK(is_structurally_mutable_container<mc_push_back_only>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_push_front_only>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_insert_only>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_erase_only>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_clear_only>::value
                   == true);
    D_MC_CHECK(is_structurally_mutable_container<mc_resize_only>::value
                   == true);

    // element-only does not
    D_MC_CHECK(is_structurally_mutable_container<mc_subscript_only>::value
                   == false);
    D_MC_CHECK(is_structurally_mutable_container<mc_data_only>::value
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
bool
tests_is_mutable_container()
{
    bool ok = true;

    // both capabilities
    D_MC_CHECK(is_mutable_container<mc_vector_int>::value == true);
    D_MC_CHECK(is_mutable_container<mc_deque_int>::value  == true);
    D_MC_CHECK(is_mutable_container<mc_string>::value     == true);

    // structural only
    D_MC_CHECK(is_mutable_container<mc_list_int>::value  == true);
    D_MC_CHECK(is_mutable_container<mc_flist_int>::value == true);
    D_MC_CHECK(is_mutable_container<mc_set_int>::value   == true);
    D_MC_CHECK(is_mutable_container<mc_map_ii>::value    == true);

    // element only
    D_MC_CHECK(is_mutable_container<mc_array_4>::value == true);
    D_MC_CHECK(is_mutable_container<mc_array_0>::value == true);

    // neither
    D_MC_CHECK(is_mutable_container<mc_init_list>::value == false);
    D_MC_CHECK(is_mutable_container<mc_sized_only>::value == false);
    D_MC_CHECK(is_mutable_container<mc_bare>::value       == false);
    D_MC_CHECK(is_mutable_container<int>::value           == false);
    D_MC_CHECK(is_mutable_container<void>::value          == false);
    D_MC_CHECK(is_mutable_container<mc_pair_ii>::value    == false);
    D_MC_CHECK(is_mutable_container<mc_int_ptr>::value    == false);
    D_MC_CHECK(is_mutable_container<mc_int_array>::value  == false);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_MC_CHECK(is_mutable_container<mc_string_view>::value == false);
#endif

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // a span is non-owning yet writable through - element mutation without
    // ownership, so it is mutable
    D_MC_CHECK(is_mutable_container<mc_span_int>::value == true);
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
bool
tests_is_immutable_container()
{
    bool ok = true;

    // read-only containers
    D_MC_CHECK(is_immutable_container<mc_init_list>::value       == true);
    D_MC_CHECK(is_immutable_container<mc_sized_only>::value      == true);
    D_MC_CHECK(is_immutable_container<mc_immutable_sized>::value == true);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_MC_CHECK(is_immutable_container<mc_string_view>::value == true);
#endif

    // mutable containers never qualify
    D_MC_CHECK(is_immutable_container<mc_vector_int>::value == false);
    D_MC_CHECK(is_immutable_container<mc_list_int>::value   == false);
    D_MC_CHECK(is_immutable_container<mc_set_int>::value    == false);
    D_MC_CHECK(is_immutable_container<mc_map_ii>::value     == false);
    D_MC_CHECK(is_immutable_container<mc_array_4>::value    == false);

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // has a size(), but writes through - not immutable
    D_MC_CHECK(is_immutable_container<mc_span_int>::value == false);
#endif

    // no size() means no container-hood, so no immutable CONTAINER
    D_MC_CHECK(is_immutable_container<mc_bare>::value      == false);
    D_MC_CHECK(is_immutable_container<int>::value          == false);
    D_MC_CHECK(is_immutable_container<void>::value         == false);
    D_MC_CHECK(is_immutable_container<mc_pair_ii>::value   == false);
    D_MC_CHECK(is_immutable_container<mc_tuple_i>::value   == false);
    D_MC_CHECK(is_immutable_container<mc_int_ptr>::value   == false);
    D_MC_CHECK(is_immutable_container<mc_int_array>::value == false);

    // a probe that has the marks of a discipline but no mutators still
    // qualifies - the access dimension does not feed the grade
    D_MC_CHECK(is_immutable_container<mc_key_type_only>::value == false);

    // term-by-term agreement with the definition (has_size_signal is consumed
    // from bounded_container_traits.hpp, its single owner)
    D_MC_CHECK(is_immutable_container<mc_sized_only>::value
                   == (    has_size_signal<mc_sized_only>::value
                        && !is_mutable_container<mc_sized_only>::value ));
    D_MC_CHECK(is_immutable_container<mc_vector_int>::value
                   == (    has_size_signal<mc_vector_int>::value
                        && !is_mutable_container<mc_vector_int>::value ));
    D_MC_CHECK(is_immutable_container<mc_bare>::value
                   == (    has_size_signal<mc_bare>::value
                        && !is_mutable_container<mc_bare>::value ));

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
bool
tests_umbrella_signal_composition()
{
    bool ok = true;

    // element = subscript OR data
    D_MC_CHECK(is_element_mutable_container<mc_vector_int>::value
                   == (    has_mutable_subscript_signal<mc_vector_int>::value
                        || has_mutable_data_signal<mc_vector_int>::value ));
    D_MC_CHECK(is_element_mutable_container<mc_vector_bool>::value
                   == (    has_mutable_subscript_signal<mc_vector_bool>::value
                        || has_mutable_data_signal<mc_vector_bool>::value ));
    D_MC_CHECK(is_element_mutable_container<mc_list_int>::value
                   == (    has_mutable_subscript_signal<mc_list_int>::value
                        || has_mutable_data_signal<mc_list_int>::value ));

    // structural = the six-way disjunction
    D_MC_CHECK(is_structurally_mutable_container<mc_vector_int>::value
                   == (    has_push_back_signal<mc_vector_int>::value
                        || has_push_front_signal<mc_vector_int>::value
                        || has_insert_signal<mc_vector_int>::value
                        || has_erase_signal<mc_vector_int>::value
                        || has_clear_signal<mc_vector_int>::value
                        || has_resize_signal<mc_vector_int>::value ));
    D_MC_CHECK(is_structurally_mutable_container<mc_set_int>::value
                   == (    has_push_back_signal<mc_set_int>::value
                        || has_push_front_signal<mc_set_int>::value
                        || has_insert_signal<mc_set_int>::value
                        || has_erase_signal<mc_set_int>::value
                        || has_clear_signal<mc_set_int>::value
                        || has_resize_signal<mc_set_int>::value ));
    D_MC_CHECK(is_structurally_mutable_container<mc_array_4>::value
                   == (    has_push_back_signal<mc_array_4>::value
                        || has_push_front_signal<mc_array_4>::value
                        || has_insert_signal<mc_array_4>::value
                        || has_erase_signal<mc_array_4>::value
                        || has_clear_signal<mc_array_4>::value
                        || has_resize_signal<mc_array_4>::value ));

    // mutable = element OR structural
    D_MC_CHECK(is_mutable_container<mc_vector_int>::value
                   == (   is_element_mutable_container<mc_vector_int>::value
                       || is_structurally_mutable_container<
                              mc_vector_int>::value ));
    D_MC_CHECK(is_mutable_container<mc_array_4>::value
                   == (   is_element_mutable_container<mc_array_4>::value
                       || is_structurally_mutable_container<
                              mc_array_4>::value ));
    D_MC_CHECK(is_mutable_container<mc_set_int>::value
                   == (   is_element_mutable_container<mc_set_int>::value
                       || is_structurally_mutable_container<
                              mc_set_int>::value ));
    D_MC_CHECK(is_mutable_container<mc_sized_only>::value
                   == (   is_element_mutable_container<mc_sized_only>::value
                       || is_structurally_mutable_container<
                              mc_sized_only>::value ));

    // the two capabilities are independent: all four corners are occupied
    D_MC_CHECK(   ( is_element_mutable_container<mc_array_4>::value == true )
               && ( is_structurally_mutable_container<mc_array_4>::value
                        == false ));
    D_MC_CHECK(   ( is_element_mutable_container<mc_set_int>::value == false )
               && ( is_structurally_mutable_container<mc_set_int>::value
                        == true ));
    D_MC_CHECK(   ( is_element_mutable_container<mc_vector_int>::value
                        == true )
               && ( is_structurally_mutable_container<mc_vector_int>::value
                        == true ));
    D_MC_CHECK(   ( is_element_mutable_container<mc_sized_only>::value
                        == false )
               && ( is_structurally_mutable_container<mc_sized_only>::value
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
bool
tests_umbrella_cv_ref_invariance()
{
    bool ok = true;

    D_MC_CV_REF_AGREE(is_element_mutable_container, mc_vector_int, true);
    D_MC_CV_REF_AGREE(is_element_mutable_container, mc_array_4,    true);
    D_MC_CV_REF_AGREE(is_element_mutable_container, mc_set_int,    false);
    D_MC_CV_REF_AGREE(is_element_mutable_container, mc_list_int,   false);

    D_MC_CV_REF_AGREE(is_structurally_mutable_container, mc_vector_int, true);
    D_MC_CV_REF_AGREE(is_structurally_mutable_container, mc_set_int,    true);
    D_MC_CV_REF_AGREE(is_structurally_mutable_container, mc_array_4,    false);

    D_MC_CV_REF_AGREE(is_mutable_container, mc_vector_int,  true);
    D_MC_CV_REF_AGREE(is_mutable_container, mc_set_int,     true);
    D_MC_CV_REF_AGREE(is_mutable_container, mc_array_4,     true);
    D_MC_CV_REF_AGREE(is_mutable_container, mc_sized_only,  false);
    D_MC_CV_REF_AGREE(is_mutable_container, mc_bare,        false);

    D_MC_CV_REF_AGREE(is_immutable_container, mc_sized_only,  true);
    D_MC_CV_REF_AGREE(is_immutable_container, mc_init_list,   true);
    D_MC_CV_REF_AGREE(is_immutable_container, mc_vector_int,  false);
    D_MC_CV_REF_AGREE(is_immutable_container, mc_bare,        false);

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
bool
tests_umbrella_value_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_MC_STATIC_CHECK(is_mutable_container_v<mc_vector_int>    == true);
    D_MC_STATIC_CHECK(is_immutable_container_v<mc_sized_only>  == true);

    D_MC_CHECK(is_element_mutable_container_v<mc_vector_int>
                   == is_element_mutable_container<mc_vector_int>::value);
    D_MC_CHECK(is_element_mutable_container_v<mc_set_int>
                   == is_element_mutable_container<mc_set_int>::value);

    D_MC_CHECK(is_structurally_mutable_container_v<mc_set_int>
                   == is_structurally_mutable_container<mc_set_int>::value);
    D_MC_CHECK(is_structurally_mutable_container_v<mc_array_4>
                   == is_structurally_mutable_container<mc_array_4>::value);

    D_MC_CHECK(is_mutable_container_v<mc_vector_int>
                   == is_mutable_container<mc_vector_int>::value);
    D_MC_CHECK(is_mutable_container_v<mc_sized_only>
                   == is_mutable_container<mc_sized_only>::value);

    D_MC_CHECK(is_immutable_container_v<mc_sized_only>
                   == is_immutable_container<mc_sized_only>::value);
    D_MC_CHECK(is_immutable_container_v<mc_vector_int>
                   == is_immutable_container<mc_vector_int>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
