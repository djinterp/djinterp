/*******************************************************************************
* djinterp [core]                     mutable_container_traits_tests_signals.cpp
*
*
* path:      /tests/djinterp/core/container/traits/mutable_container_traits_tests_signals.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "mutable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   SFINAE mutator detection
// ===========================================================================
//   One test per signal, each run against the std battery (the realism check)
// and against the mc_* probes (the precision check).  A probe carries exactly
// one lit signal, so a regression names the detector at fault.


/*
tests_has_push_back_signal
  Verifies the push_back detector.
  Tests the following:
  - the sequences that append at the back are detected (vector, deque, list,
    string)
  - forward_list is NOT detected: it appends at the front only
  - the associative and fixed-extent containers are NOT detected
  - the isolated probe is detected, and no other probe is
  - the member alone is insufficient: without a nested value_type the probe
    expression cannot be formed
*/
bool
tests_has_push_back_signal()
{
    bool ok = true;

    // sequences that append at the back
    D_MC_CHECK(has_push_back_signal<mc_vector_int>::value  == true);
    D_MC_CHECK(has_push_back_signal<mc_vector_bool>::value == true);
    D_MC_CHECK(has_push_back_signal<mc_deque_int>::value   == true);
    D_MC_CHECK(has_push_back_signal<mc_list_int>::value    == true);
    D_MC_CHECK(has_push_back_signal<mc_string>::value      == true);

    // forward_list grows at the front only
    D_MC_CHECK(has_push_back_signal<mc_flist_int>::value == false);

    // associative disciplines do not append positionally
    D_MC_CHECK(has_push_back_signal<mc_set_int>::value  == false);
    D_MC_CHECK(has_push_back_signal<mc_map_ii>::value    == false);
    D_MC_CHECK(has_push_back_signal<mc_uset_int>::value == false);

    // fixed extent cannot grow
    D_MC_CHECK(has_push_back_signal<mc_array_4>::value == false);

    // non-containers
    D_MC_CHECK(has_push_back_signal<int>::value        == false);
    D_MC_CHECK(has_push_back_signal<void>::value       == false);
    D_MC_CHECK(has_push_back_signal<mc_int_ptr>::value == false);

    // probes
    D_MC_CHECK(has_push_back_signal<mc_push_back_only>::value  == true);
    D_MC_CHECK(has_push_back_signal<mc_all_signals>::value     == true);
    D_MC_CHECK(has_push_back_signal<mc_push_front_only>::value == false);
    D_MC_CHECK(has_push_back_signal<mc_bare>::value            == false);

    // the member is present but value_type is not
    D_MC_CHECK(has_push_back_signal<mc_no_value_type>::value == false);

    return ok;
}

/*
tests_has_push_front_signal
  Verifies the push_front detector.
  Tests the following:
  - the sequences that prepend are detected (deque, list, forward_list)
  - vector and string, which have no push_front, are NOT detected
  - the isolated probe is detected and the push_back probe is not
  - the member alone is insufficient without a nested value_type
*/
bool
tests_has_push_front_signal()
{
    bool ok = true;

    // sequences that prepend
    D_MC_CHECK(has_push_front_signal<mc_deque_int>::value == true);
    D_MC_CHECK(has_push_front_signal<mc_list_int>::value  == true);
    D_MC_CHECK(has_push_front_signal<mc_flist_int>::value == true);

    // sequences that do not
    D_MC_CHECK(has_push_front_signal<mc_vector_int>::value == false);
    D_MC_CHECK(has_push_front_signal<mc_string>::value     == false);
    D_MC_CHECK(has_push_front_signal<mc_array_4>::value    == false);

    // associative disciplines
    D_MC_CHECK(has_push_front_signal<mc_set_int>::value == false);
    D_MC_CHECK(has_push_front_signal<mc_map_ii>::value  == false);

    // non-containers
    D_MC_CHECK(has_push_front_signal<int>::value  == false);
    D_MC_CHECK(has_push_front_signal<void>::value == false);

    // probes
    D_MC_CHECK(has_push_front_signal<mc_push_front_only>::value == true);
    D_MC_CHECK(has_push_front_signal<mc_all_signals>::value     == true);
    D_MC_CHECK(has_push_front_signal<mc_push_back_only>::value  == false);
    D_MC_CHECK(has_push_front_signal<mc_no_value_type>::value   == false);

    return ok;
}

/*
tests_has_insert_signal
  Verifies the insert detector, which probes the SINGLE-argument form.
  Tests the following:
  - the associative disciplines, whose insert takes the value alone, are
    detected (set, multiset, map, multimap, unordered_set, unordered_map)
  - the positional sequences are NOT detected: their insert wants an iterator
    first, so the one-argument call is ill-formed - this is the reason vector
    reads insert-dark despite having an insert
  - the isolated probe is detected; the two-argument probe is not
  - the member alone is insufficient without a nested value_type
*/
bool
tests_has_insert_signal()
{
    bool ok = true;

    // associative: insert(value_type)
    D_MC_CHECK(has_insert_signal<mc_set_int>::value      == true);
    D_MC_CHECK(has_insert_signal<mc_multiset_int>::value == true);
    D_MC_CHECK(has_insert_signal<mc_map_ii>::value       == true);
    D_MC_CHECK(has_insert_signal<mc_multimap_ii>::value  == true);
    D_MC_CHECK(has_insert_signal<mc_uset_int>::value     == true);
    D_MC_CHECK(has_insert_signal<mc_umap_ii>::value      == true);

    // positional: insert(iterator, value_type) - the one-arg call fails
    D_MC_CHECK(has_insert_signal<mc_vector_int>::value == false);
    D_MC_CHECK(has_insert_signal<mc_deque_int>::value  == false);
    D_MC_CHECK(has_insert_signal<mc_list_int>::value   == false);
    D_MC_CHECK(has_insert_signal<mc_string>::value     == false);
    D_MC_CHECK(has_insert_signal<mc_array_4>::value    == false);

    // non-containers
    D_MC_CHECK(has_insert_signal<int>::value  == false);
    D_MC_CHECK(has_insert_signal<void>::value == false);

    // probes
    D_MC_CHECK(has_insert_signal<mc_insert_only>::value    == true);
    D_MC_CHECK(has_insert_signal<mc_all_signals>::value    == true);
    D_MC_CHECK(has_insert_signal<mc_insert_two_arg>::value == false);
    D_MC_CHECK(has_insert_signal<mc_no_value_type>::value  == false);

    return ok;
}

/*
tests_has_erase_signal
  Verifies the erase detector, whose probe manufactures its argument by
  calling begin() on the same object.
  Tests the following:
  - every std container exposing erase(iterator) is detected
  - forward_list is NOT detected: it erases after a position, not at one
  - array, which erases nothing, is NOT detected
  - the isolated probe is detected
  - erase WITHOUT begin() is not detected: the probe cannot build an argument,
    so the member's presence alone does not carry the signal
  - the probe never names value_type, so a type lacking one is still detected
*/
bool
tests_has_erase_signal()
{
    bool ok = true;

    // sequences and associative alike expose erase(iterator)
    D_MC_CHECK(has_erase_signal<mc_vector_int>::value == true);
    D_MC_CHECK(has_erase_signal<mc_deque_int>::value  == true);
    D_MC_CHECK(has_erase_signal<mc_list_int>::value   == true);
    D_MC_CHECK(has_erase_signal<mc_string>::value     == true);
    D_MC_CHECK(has_erase_signal<mc_set_int>::value    == true);
    D_MC_CHECK(has_erase_signal<mc_map_ii>::value     == true);
    D_MC_CHECK(has_erase_signal<mc_uset_int>::value   == true);

    // forward_list erases AFTER a position
    D_MC_CHECK(has_erase_signal<mc_flist_int>::value == false);

    // fixed extent
    D_MC_CHECK(has_erase_signal<mc_array_4>::value == false);

    // non-containers
    D_MC_CHECK(has_erase_signal<int>::value  == false);
    D_MC_CHECK(has_erase_signal<void>::value == false);

    // probes
    D_MC_CHECK(has_erase_signal<mc_erase_only>::value     == true);
    D_MC_CHECK(has_erase_signal<mc_all_signals>::value    == true);
    D_MC_CHECK(has_erase_signal<mc_erase_no_begin>::value == false);

    // erase's probe expression never mentions value_type
    D_MC_CHECK(has_erase_signal<mc_no_value_type>::value == true);

    return ok;
}

/*
tests_has_clear_signal
  Verifies the clear detector.
  Tests the following:
  - every std container that can be emptied is detected
  - array, whose extent is fixed, is NOT detected
  - the isolated probe is detected
  - the probe never names value_type: mc_clear_only declares none and is still
    detected, as is mc_no_value_type
*/
bool
tests_has_clear_signal()
{
    bool ok = true;

    // anything that can be emptied
    D_MC_CHECK(has_clear_signal<mc_vector_int>::value == true);
    D_MC_CHECK(has_clear_signal<mc_deque_int>::value  == true);
    D_MC_CHECK(has_clear_signal<mc_list_int>::value   == true);
    D_MC_CHECK(has_clear_signal<mc_flist_int>::value  == true);
    D_MC_CHECK(has_clear_signal<mc_string>::value     == true);
    D_MC_CHECK(has_clear_signal<mc_set_int>::value    == true);
    D_MC_CHECK(has_clear_signal<mc_map_ii>::value     == true);
    D_MC_CHECK(has_clear_signal<mc_uset_int>::value   == true);

    // fixed extent
    D_MC_CHECK(has_clear_signal<mc_array_4>::value == false);

    // non-containers
    D_MC_CHECK(has_clear_signal<int>::value       == false);
    D_MC_CHECK(has_clear_signal<void>::value      == false);
    D_MC_CHECK(has_clear_signal<mc_pair_ii>::value == false);

    // probes - clear needs no value_type
    D_MC_CHECK(has_clear_signal<mc_clear_only>::value     == true);
    D_MC_CHECK(has_clear_signal<mc_all_signals>::value    == true);
    D_MC_CHECK(has_clear_signal<mc_no_value_type>::value  == true);
    D_MC_CHECK(has_clear_signal<mc_push_back_only>::value == false);

    return ok;
}

/*
tests_has_resize_signal
  Verifies the resize detector.
  Tests the following:
  - the sequences that resize are detected (vector, deque, list, forward_list,
    string)
  - the associative disciplines, which have no positional extent, are NOT
    detected
  - array is NOT detected
  - the isolated probe is detected, and needs no value_type
*/
bool
tests_has_resize_signal()
{
    bool ok = true;

    // sequences carry an extent that can be set
    D_MC_CHECK(has_resize_signal<mc_vector_int>::value == true);
    D_MC_CHECK(has_resize_signal<mc_deque_int>::value  == true);
    D_MC_CHECK(has_resize_signal<mc_list_int>::value   == true);
    D_MC_CHECK(has_resize_signal<mc_flist_int>::value  == true);
    D_MC_CHECK(has_resize_signal<mc_string>::value     == true);

    // associative disciplines have no positional extent
    D_MC_CHECK(has_resize_signal<mc_set_int>::value  == false);
    D_MC_CHECK(has_resize_signal<mc_map_ii>::value   == false);
    D_MC_CHECK(has_resize_signal<mc_uset_int>::value == false);

    // fixed extent
    D_MC_CHECK(has_resize_signal<mc_array_4>::value == false);
    D_MC_CHECK(has_resize_signal<mc_array_0>::value == false);

    // non-containers
    D_MC_CHECK(has_resize_signal<int>::value  == false);
    D_MC_CHECK(has_resize_signal<void>::value == false);

    // probes - resize needs no value_type
    D_MC_CHECK(has_resize_signal<mc_resize_only>::value    == true);
    D_MC_CHECK(has_resize_signal<mc_all_signals>::value    == true);
    D_MC_CHECK(has_resize_signal<mc_no_value_type>::value  == true);
    D_MC_CHECK(has_resize_signal<mc_push_back_only>::value == false);

    return ok;
}

/*
tests_has_mutable_subscript_signal
  Verifies the writable-subscript detector, whose probe is an ASSIGNMENT
  through operator[] - so it answers "is the subscript settable", not merely
  "does a subscript exist".
  Tests the following:
  - the random-access sequences are detected, including vector<bool>, whose
    proxy reference is assignable
  - array is detected at both a positive and a zero extent
  - list is NOT detected: it has no subscript at all, so a node sequence reads
    element-immutable even though its elements are reachable by iterator
  - map is NOT detected: its value_type is a pair, which does not assign to the
    mapped reference the subscript yields
  - a const-yielding subscript is NOT detected
  - a subscript whose lvalue does not accept value_type is NOT detected
*/
bool
tests_has_mutable_subscript_signal()
{
    bool ok = true;

    // random-access sequences
    D_MC_CHECK(has_mutable_subscript_signal<mc_vector_int>::value == true);
    D_MC_CHECK(has_mutable_subscript_signal<mc_deque_int>::value  == true);
    D_MC_CHECK(has_mutable_subscript_signal<mc_string>::value     == true);
    D_MC_CHECK(has_mutable_subscript_signal<mc_array_4>::value    == true);
    D_MC_CHECK(has_mutable_subscript_signal<mc_array_0>::value    == true);

    // the proxy reference of vector<bool> is assignable
    D_MC_CHECK(has_mutable_subscript_signal<mc_vector_bool>::value == true);

    // node sequences expose no subscript
    D_MC_CHECK(has_mutable_subscript_signal<mc_list_int>::value  == false);
    D_MC_CHECK(has_mutable_subscript_signal<mc_flist_int>::value == false);

    // a map's subscript yields the MAPPED type; value_type is the pair
    D_MC_CHECK(has_mutable_subscript_signal<mc_map_ii>::value  == false);
    D_MC_CHECK(has_mutable_subscript_signal<mc_umap_ii>::value == false);
    D_MC_CHECK(has_mutable_subscript_signal<mc_set_int>::value == false);

    // non-containers
    D_MC_CHECK(has_mutable_subscript_signal<int>::value        == false);
    D_MC_CHECK(has_mutable_subscript_signal<void>::value       == false);
    D_MC_CHECK(has_mutable_subscript_signal<mc_int_ptr>::value == false);

    // probes
    D_MC_CHECK(has_mutable_subscript_signal<mc_subscript_only>::value
                   == true);
    D_MC_CHECK(has_mutable_subscript_signal<mc_all_signals>::value
                   == true);
    D_MC_CHECK(has_mutable_subscript_signal<mc_const_subscript_only>::value
                   == false);
    D_MC_CHECK(has_mutable_subscript_signal<mc_subscript_wrong_value>::value
                   == false);
    D_MC_CHECK(has_mutable_subscript_signal<mc_no_value_type>::value
                   == false);

    return ok;
}

/*
tests_has_mutable_data_signal
  Verifies the writable-data detector, whose probe DEREFERENCES data() and
  assigns through it.
  Tests the following:
  - the contiguous sequences are detected (vector, array)
  - vector<bool> is NOT detected: it is contiguous in bits and exposes no
    data(), which is exactly why its element mutability rests on the subscript
    signal alone
  - the non-contiguous containers are NOT detected
  - a const-yielding data() is NOT detected
  - a void* data() is NOT detected: the probe's dereference is ill-formed
*/
bool
tests_has_mutable_data_signal()
{
    bool ok = true;

    // contiguous sequences
    D_MC_CHECK(has_mutable_data_signal<mc_vector_int>::value == true);
    D_MC_CHECK(has_mutable_data_signal<mc_array_4>::value    == true);
    D_MC_CHECK(has_mutable_data_signal<mc_array_0>::value    == true);

    // vector<bool> has no data()
    D_MC_CHECK(has_mutable_data_signal<mc_vector_bool>::value == false);

    // non-contiguous
    D_MC_CHECK(has_mutable_data_signal<mc_deque_int>::value == false);
    D_MC_CHECK(has_mutable_data_signal<mc_list_int>::value  == false);
    D_MC_CHECK(has_mutable_data_signal<mc_set_int>::value   == false);
    D_MC_CHECK(has_mutable_data_signal<mc_map_ii>::value    == false);

    // non-containers
    D_MC_CHECK(has_mutable_data_signal<int>::value        == false);
    D_MC_CHECK(has_mutable_data_signal<void>::value       == false);
    D_MC_CHECK(has_mutable_data_signal<mc_int_ptr>::value == false);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std::string gained a non-const data() in C++17
    D_MC_CHECK(has_mutable_data_signal<mc_string>::value == true);
#endif

    // probes
    D_MC_CHECK(has_mutable_data_signal<mc_data_only>::value       == true);
    D_MC_CHECK(has_mutable_data_signal<mc_all_signals>::value     == true);
    D_MC_CHECK(has_mutable_data_signal<mc_const_data_only>::value == false);
    D_MC_CHECK(has_mutable_data_signal<mc_void_data>::value       == false);
    D_MC_CHECK(has_mutable_data_signal<mc_no_value_type>::value   == false);

    return ok;
}

/*
tests_signal_value_type_dependency
  Pins WHICH signals depend on a nested value_type, by comparing two probes
  that differ in exactly that one typedef.
  Tests the following:
  - mc_all_signals declares every mutator AND a value_type: all eight signals
    read true
  - mc_no_value_type declares the same mutators and no value_type
  - the five probes whose expression names value_type go dark: push_back,
    push_front, insert, subscript, data
  - the three that never name it stay lit: erase, clear, resize
*/
bool
tests_signal_value_type_dependency()
{
    bool ok = true;

    // with a value_type, every signal is lit
    D_MC_CHECK(has_push_back_signal<mc_all_signals>::value         == true);
    D_MC_CHECK(has_push_front_signal<mc_all_signals>::value        == true);
    D_MC_CHECK(has_insert_signal<mc_all_signals>::value            == true);
    D_MC_CHECK(has_erase_signal<mc_all_signals>::value             == true);
    D_MC_CHECK(has_clear_signal<mc_all_signals>::value             == true);
    D_MC_CHECK(has_resize_signal<mc_all_signals>::value            == true);
    D_MC_CHECK(has_mutable_subscript_signal<mc_all_signals>::value == true);
    D_MC_CHECK(has_mutable_data_signal<mc_all_signals>::value      == true);

    // without one, the five value_type-dependent probes go dark
    D_MC_CHECK(has_push_back_signal<mc_no_value_type>::value         == false);
    D_MC_CHECK(has_push_front_signal<mc_no_value_type>::value        == false);
    D_MC_CHECK(has_insert_signal<mc_no_value_type>::value            == false);
    D_MC_CHECK(has_mutable_subscript_signal<mc_no_value_type>::value == false);
    D_MC_CHECK(has_mutable_data_signal<mc_no_value_type>::value      == false);

    // and the three independent probes stay lit
    D_MC_CHECK(has_erase_signal<mc_no_value_type>::value  == true);
    D_MC_CHECK(has_clear_signal<mc_no_value_type>::value  == true);
    D_MC_CHECK(has_resize_signal<mc_no_value_type>::value == true);

    return ok;
}

/*
tests_signal_cv_ref_invariance
  Verifies the clean_t contract at the signal layer: every probe strips
  cv-qualification and references before asking its question, so a signal
  answers identically for C, const C, volatile C, const volatile C, C&,
  const C& and C&&.
  Tests the following:
  - all eight signals, across a positive and a negative carrier each
  - the const spellings in particular, since a const container is exactly the
    case a naive probe would get wrong
*/
bool
tests_signal_cv_ref_invariance()
{
    bool ok = true;

    // structural signals, positive carriers
    D_MC_CV_REF_AGREE(has_push_back_signal,  mc_vector_int, true);
    D_MC_CV_REF_AGREE(has_push_front_signal, mc_deque_int,  true);
    D_MC_CV_REF_AGREE(has_insert_signal,     mc_set_int,    true);
    D_MC_CV_REF_AGREE(has_erase_signal,      mc_vector_int, true);
    D_MC_CV_REF_AGREE(has_clear_signal,      mc_vector_int, true);
    D_MC_CV_REF_AGREE(has_resize_signal,     mc_vector_int, true);

    // structural signals, negative carriers
    D_MC_CV_REF_AGREE(has_push_back_signal,  mc_set_int,    false);
    D_MC_CV_REF_AGREE(has_push_front_signal, mc_vector_int, false);
    D_MC_CV_REF_AGREE(has_insert_signal,     mc_vector_int, false);
    D_MC_CV_REF_AGREE(has_erase_signal,      mc_array_4,    false);
    D_MC_CV_REF_AGREE(has_clear_signal,      mc_array_4,    false);
    D_MC_CV_REF_AGREE(has_resize_signal,     mc_set_int,    false);

    // element signals
    D_MC_CV_REF_AGREE(has_mutable_subscript_signal, mc_vector_int, true);
    D_MC_CV_REF_AGREE(has_mutable_data_signal,      mc_vector_int, true);
    D_MC_CV_REF_AGREE(has_mutable_subscript_signal, mc_list_int,   false);
    D_MC_CV_REF_AGREE(has_mutable_data_signal,      mc_list_int,   false);

    // and on the probes
    D_MC_CV_REF_AGREE(has_push_back_signal, mc_push_back_only, true);
    D_MC_CV_REF_AGREE(has_clear_signal,     mc_clear_only,     true);
    D_MC_CV_REF_AGREE(has_push_back_signal, mc_bare,           false);

    return ok;
}

/*
tests_signal_value_companions
  Verifies the `_v` companions the D_TYPE_TRAIT_TRUE engine emits alongside
  each signal.
  Tests the following:
  - every companion agrees with its trait's ::value, on a positive and a
    negative carrier
  - the companions are usable in a constant expression
  - the whole body is gated on variable-template support, since the engine
    emits nothing below C++14 and the ::value member remains the portable
    spelling
*/
bool
tests_signal_value_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_MC_STATIC_CHECK(has_push_back_signal_v<mc_vector_int> == true);
    D_MC_STATIC_CHECK(has_clear_signal_v<mc_array_4>        == false);

    // structural companions
    D_MC_CHECK(has_push_back_signal_v<mc_vector_int>
                   == has_push_back_signal<mc_vector_int>::value);
    D_MC_CHECK(has_push_front_signal_v<mc_deque_int>
                   == has_push_front_signal<mc_deque_int>::value);
    D_MC_CHECK(has_insert_signal_v<mc_set_int>
                   == has_insert_signal<mc_set_int>::value);
    D_MC_CHECK(has_erase_signal_v<mc_vector_int>
                   == has_erase_signal<mc_vector_int>::value);
    D_MC_CHECK(has_clear_signal_v<mc_vector_int>
                   == has_clear_signal<mc_vector_int>::value);
    D_MC_CHECK(has_resize_signal_v<mc_vector_int>
                   == has_resize_signal<mc_vector_int>::value);

    // element companions
    D_MC_CHECK(has_mutable_subscript_signal_v<mc_vector_int>
                   == has_mutable_subscript_signal<mc_vector_int>::value);
    D_MC_CHECK(has_mutable_data_signal_v<mc_vector_int>
                   == has_mutable_data_signal<mc_vector_int>::value);

    // negative carriers
    D_MC_CHECK(has_push_back_signal_v<mc_set_int>            == false);
    D_MC_CHECK(has_push_front_signal_v<mc_vector_int>        == false);
    D_MC_CHECK(has_insert_signal_v<mc_vector_int>            == false);
    D_MC_CHECK(has_erase_signal_v<mc_array_4>                == false);
    D_MC_CHECK(has_clear_signal_v<mc_array_4>                == false);
    D_MC_CHECK(has_resize_signal_v<mc_array_4>               == false);
    D_MC_CHECK(has_mutable_subscript_signal_v<mc_list_int>   == false);
    D_MC_CHECK(has_mutable_data_signal_v<mc_list_int>        == false);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
