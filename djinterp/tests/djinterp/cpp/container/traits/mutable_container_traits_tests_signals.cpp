/*******************************************************************************
* djinterp [cpp]                      mutable_container_traits_tests_signals.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/mutable_container_traits_tests_signals.cpp
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
bool tests_has_push_back_signal()
{
    bool ok = true;

    // sequences that append at the back
    D_MC_CHECK(has_push_back_signal<DMcVectorInt>::value  == true);
    D_MC_CHECK(has_push_back_signal<DMcVectorBool>::value == true);
    D_MC_CHECK(has_push_back_signal<DMcDequeInt>::value   == true);
    D_MC_CHECK(has_push_back_signal<DMcListInt>::value    == true);
    D_MC_CHECK(has_push_back_signal<DMcString>::value      == true);

    // forward_list grows at the front only
    D_MC_CHECK(has_push_back_signal<DMcFlistInt>::value == false);

    // associative disciplines do not append positionally
    D_MC_CHECK(has_push_back_signal<DMcSetInt>::value  == false);
    D_MC_CHECK(has_push_back_signal<DMcMapIi>::value    == false);
    D_MC_CHECK(has_push_back_signal<DMcUsetInt>::value == false);

    // fixed extent cannot grow
    D_MC_CHECK(has_push_back_signal<DMcArray4>::value == false);

    // non-containers
    D_MC_CHECK(has_push_back_signal<int>::value        == false);
    D_MC_CHECK(has_push_back_signal<void>::value       == false);
    D_MC_CHECK(has_push_back_signal<DMcIntPtr>::value == false);

    // probes
    D_MC_CHECK(has_push_back_signal<DMcPushBackOnly>::value  == true);
    D_MC_CHECK(has_push_back_signal<DMcAllSignals>::value     == true);
    D_MC_CHECK(has_push_back_signal<DMcPushFrontOnly>::value == false);
    D_MC_CHECK(has_push_back_signal<DMcBare>::value            == false);

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
bool tests_has_push_front_signal()
{
    bool ok = true;

    // sequences that prepend
    D_MC_CHECK(has_push_front_signal<DMcDequeInt>::value == true);
    D_MC_CHECK(has_push_front_signal<DMcListInt>::value  == true);
    D_MC_CHECK(has_push_front_signal<DMcFlistInt>::value == true);

    // sequences that do not
    D_MC_CHECK(has_push_front_signal<DMcVectorInt>::value == false);
    D_MC_CHECK(has_push_front_signal<DMcString>::value     == false);
    D_MC_CHECK(has_push_front_signal<DMcArray4>::value    == false);

    // associative disciplines
    D_MC_CHECK(has_push_front_signal<DMcSetInt>::value == false);
    D_MC_CHECK(has_push_front_signal<DMcMapIi>::value  == false);

    // non-containers
    D_MC_CHECK(has_push_front_signal<int>::value  == false);
    D_MC_CHECK(has_push_front_signal<void>::value == false);

    // probes
    D_MC_CHECK(has_push_front_signal<DMcPushFrontOnly>::value == true);
    D_MC_CHECK(has_push_front_signal<DMcAllSignals>::value     == true);
    D_MC_CHECK(has_push_front_signal<DMcPushBackOnly>::value  == false);
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
bool tests_has_insert_signal()
{
    bool ok = true;

    // associative: insert(value_type)
    D_MC_CHECK(has_insert_signal<DMcSetInt>::value      == true);
    D_MC_CHECK(has_insert_signal<DMcMultisetInt>::value == true);
    D_MC_CHECK(has_insert_signal<DMcMapIi>::value       == true);
    D_MC_CHECK(has_insert_signal<DMcMultimapIi>::value  == true);
    D_MC_CHECK(has_insert_signal<DMcUsetInt>::value     == true);
    D_MC_CHECK(has_insert_signal<DMcUmapIi>::value      == true);

    // positional: insert(iterator, value_type) - the one-arg call fails
    D_MC_CHECK(has_insert_signal<DMcVectorInt>::value == false);
    D_MC_CHECK(has_insert_signal<DMcDequeInt>::value  == false);
    D_MC_CHECK(has_insert_signal<DMcListInt>::value   == false);
    D_MC_CHECK(has_insert_signal<DMcString>::value     == false);
    D_MC_CHECK(has_insert_signal<DMcArray4>::value    == false);

    // non-containers
    D_MC_CHECK(has_insert_signal<int>::value  == false);
    D_MC_CHECK(has_insert_signal<void>::value == false);

    // probes
    D_MC_CHECK(has_insert_signal<DMcInsertOnly>::value    == true);
    D_MC_CHECK(has_insert_signal<DMcAllSignals>::value    == true);
    D_MC_CHECK(has_insert_signal<DMcInsertTwoArg>::value == false);
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
bool tests_has_erase_signal()
{
    bool ok = true;

    // sequences and associative alike expose erase(iterator)
    D_MC_CHECK(has_erase_signal<DMcVectorInt>::value == true);
    D_MC_CHECK(has_erase_signal<DMcDequeInt>::value  == true);
    D_MC_CHECK(has_erase_signal<DMcListInt>::value   == true);
    D_MC_CHECK(has_erase_signal<DMcString>::value     == true);
    D_MC_CHECK(has_erase_signal<DMcSetInt>::value    == true);
    D_MC_CHECK(has_erase_signal<DMcMapIi>::value     == true);
    D_MC_CHECK(has_erase_signal<DMcUsetInt>::value   == true);

    // forward_list erases AFTER a position
    D_MC_CHECK(has_erase_signal<DMcFlistInt>::value == false);

    // fixed extent
    D_MC_CHECK(has_erase_signal<DMcArray4>::value == false);

    // non-containers
    D_MC_CHECK(has_erase_signal<int>::value  == false);
    D_MC_CHECK(has_erase_signal<void>::value == false);

    // probes
    D_MC_CHECK(has_erase_signal<DMcEraseOnly>::value     == true);
    D_MC_CHECK(has_erase_signal<DMcAllSignals>::value    == true);
    D_MC_CHECK(has_erase_signal<DMcEraseNoBegin>::value == false);

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
  - the probe never names value_type: DMcClearOnly declares none and is still
    detected, as is mc_no_value_type
*/
bool tests_has_clear_signal()
{
    bool ok = true;

    // anything that can be emptied
    D_MC_CHECK(has_clear_signal<DMcVectorInt>::value == true);
    D_MC_CHECK(has_clear_signal<DMcDequeInt>::value  == true);
    D_MC_CHECK(has_clear_signal<DMcListInt>::value   == true);
    D_MC_CHECK(has_clear_signal<DMcFlistInt>::value  == true);
    D_MC_CHECK(has_clear_signal<DMcString>::value     == true);
    D_MC_CHECK(has_clear_signal<DMcSetInt>::value    == true);
    D_MC_CHECK(has_clear_signal<DMcMapIi>::value     == true);
    D_MC_CHECK(has_clear_signal<DMcUsetInt>::value   == true);

    // fixed extent
    D_MC_CHECK(has_clear_signal<DMcArray4>::value == false);

    // non-containers
    D_MC_CHECK(has_clear_signal<int>::value       == false);
    D_MC_CHECK(has_clear_signal<void>::value      == false);
    D_MC_CHECK(has_clear_signal<DMcPairIi>::value == false);

    // probes - clear needs no value_type
    D_MC_CHECK(has_clear_signal<DMcClearOnly>::value     == true);
    D_MC_CHECK(has_clear_signal<DMcAllSignals>::value    == true);
    D_MC_CHECK(has_clear_signal<mc_no_value_type>::value  == true);
    D_MC_CHECK(has_clear_signal<DMcPushBackOnly>::value == false);

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
bool tests_has_resize_signal()
{
    bool ok = true;

    // sequences carry an extent that can be set
    D_MC_CHECK(has_resize_signal<DMcVectorInt>::value == true);
    D_MC_CHECK(has_resize_signal<DMcDequeInt>::value  == true);
    D_MC_CHECK(has_resize_signal<DMcListInt>::value   == true);
    D_MC_CHECK(has_resize_signal<DMcFlistInt>::value  == true);
    D_MC_CHECK(has_resize_signal<DMcString>::value     == true);

    // associative disciplines have no positional extent
    D_MC_CHECK(has_resize_signal<DMcSetInt>::value  == false);
    D_MC_CHECK(has_resize_signal<DMcMapIi>::value   == false);
    D_MC_CHECK(has_resize_signal<DMcUsetInt>::value == false);

    // fixed extent
    D_MC_CHECK(has_resize_signal<DMcArray4>::value == false);
    D_MC_CHECK(has_resize_signal<DMcArray0>::value == false);

    // non-containers
    D_MC_CHECK(has_resize_signal<int>::value  == false);
    D_MC_CHECK(has_resize_signal<void>::value == false);

    // probes - resize needs no value_type
    D_MC_CHECK(has_resize_signal<DMcResizeOnly>::value    == true);
    D_MC_CHECK(has_resize_signal<DMcAllSignals>::value    == true);
    D_MC_CHECK(has_resize_signal<mc_no_value_type>::value  == true);
    D_MC_CHECK(has_resize_signal<DMcPushBackOnly>::value == false);

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
bool tests_has_mutable_subscript_signal()
{
    bool ok = true;

    // random-access sequences
    D_MC_CHECK(has_mutable_subscript_signal<DMcVectorInt>::value == true);
    D_MC_CHECK(has_mutable_subscript_signal<DMcDequeInt>::value  == true);
    D_MC_CHECK(has_mutable_subscript_signal<DMcString>::value     == true);
    D_MC_CHECK(has_mutable_subscript_signal<DMcArray4>::value    == true);
    D_MC_CHECK(has_mutable_subscript_signal<DMcArray0>::value    == true);

    // the proxy reference of vector<bool> is assignable
    D_MC_CHECK(has_mutable_subscript_signal<DMcVectorBool>::value == true);

    // node sequences expose no subscript
    D_MC_CHECK(has_mutable_subscript_signal<DMcListInt>::value  == false);
    D_MC_CHECK(has_mutable_subscript_signal<DMcFlistInt>::value == false);

    // a map's subscript yields the MAPPED type; value_type is the pair
    D_MC_CHECK(has_mutable_subscript_signal<DMcMapIi>::value  == false);
    D_MC_CHECK(has_mutable_subscript_signal<DMcUmapIi>::value == false);
    D_MC_CHECK(has_mutable_subscript_signal<DMcSetInt>::value == false);

    // non-containers
    D_MC_CHECK(has_mutable_subscript_signal<int>::value        == false);
    D_MC_CHECK(has_mutable_subscript_signal<void>::value       == false);
    D_MC_CHECK(has_mutable_subscript_signal<DMcIntPtr>::value == false);

    // probes
    D_MC_CHECK(has_mutable_subscript_signal<DMcSubscriptOnly>::value
                   == true);
    D_MC_CHECK(has_mutable_subscript_signal<DMcAllSignals>::value
                   == true);
    D_MC_CHECK(has_mutable_subscript_signal<DMcConstSubscriptOnly>::value
                   == false);
    D_MC_CHECK(has_mutable_subscript_signal<DMcSubscriptWrongValue>::value
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
bool tests_has_mutable_data_signal()
{
    bool ok = true;

    // contiguous sequences
    D_MC_CHECK(has_mutable_data_signal<DMcVectorInt>::value == true);
    D_MC_CHECK(has_mutable_data_signal<DMcArray4>::value    == true);
    D_MC_CHECK(has_mutable_data_signal<DMcArray0>::value    == true);

    // vector<bool> has no data()
    D_MC_CHECK(has_mutable_data_signal<DMcVectorBool>::value == false);

    // non-contiguous
    D_MC_CHECK(has_mutable_data_signal<DMcDequeInt>::value == false);
    D_MC_CHECK(has_mutable_data_signal<DMcListInt>::value  == false);
    D_MC_CHECK(has_mutable_data_signal<DMcSetInt>::value   == false);
    D_MC_CHECK(has_mutable_data_signal<DMcMapIi>::value    == false);

    // non-containers
    D_MC_CHECK(has_mutable_data_signal<int>::value        == false);
    D_MC_CHECK(has_mutable_data_signal<void>::value       == false);
    D_MC_CHECK(has_mutable_data_signal<DMcIntPtr>::value == false);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std::string gained a non-const data() in C++17
    D_MC_CHECK(has_mutable_data_signal<DMcString>::value == true);
#endif

    // probes
    D_MC_CHECK(has_mutable_data_signal<DMcDataOnly>::value       == true);
    D_MC_CHECK(has_mutable_data_signal<DMcAllSignals>::value     == true);
    D_MC_CHECK(has_mutable_data_signal<DMcConstDataOnly>::value == false);
    D_MC_CHECK(has_mutable_data_signal<DMcVoidData>::value       == false);
    D_MC_CHECK(has_mutable_data_signal<mc_no_value_type>::value   == false);

    return ok;
}

/*
tests_signal_value_type_dependency
  Pins WHICH signals depend on a nested value_type, by comparing two probes
  that differ in exactly that one typedef.
  Tests the following:
  - DMcAllSignals declares every mutator AND a value_type: all eight signals
    read true
  - mc_no_value_type declares the same mutators and no value_type
  - the five probes whose expression names value_type go dark: push_back,
    push_front, insert, subscript, data
  - the three that never name it stay lit: erase, clear, resize
*/
bool tests_signal_value_type_dependency()
{
    bool ok = true;

    // with a value_type, every signal is lit
    D_MC_CHECK(has_push_back_signal<DMcAllSignals>::value         == true);
    D_MC_CHECK(has_push_front_signal<DMcAllSignals>::value        == true);
    D_MC_CHECK(has_insert_signal<DMcAllSignals>::value            == true);
    D_MC_CHECK(has_erase_signal<DMcAllSignals>::value             == true);
    D_MC_CHECK(has_clear_signal<DMcAllSignals>::value             == true);
    D_MC_CHECK(has_resize_signal<DMcAllSignals>::value            == true);
    D_MC_CHECK(has_mutable_subscript_signal<DMcAllSignals>::value == true);
    D_MC_CHECK(has_mutable_data_signal<DMcAllSignals>::value      == true);

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
bool tests_signal_cv_ref_invariance()
{
    bool ok = true;

    // structural signals, positive carriers
    D_MC_CV_REF_AGREE(has_push_back_signal,  DMcVectorInt, true);
    D_MC_CV_REF_AGREE(has_push_front_signal, DMcDequeInt,  true);
    D_MC_CV_REF_AGREE(has_insert_signal,     DMcSetInt,    true);
    D_MC_CV_REF_AGREE(has_erase_signal,      DMcVectorInt, true);
    D_MC_CV_REF_AGREE(has_clear_signal,      DMcVectorInt, true);
    D_MC_CV_REF_AGREE(has_resize_signal,     DMcVectorInt, true);

    // structural signals, negative carriers
    D_MC_CV_REF_AGREE(has_push_back_signal,  DMcSetInt,    false);
    D_MC_CV_REF_AGREE(has_push_front_signal, DMcVectorInt, false);
    D_MC_CV_REF_AGREE(has_insert_signal,     DMcVectorInt, false);
    D_MC_CV_REF_AGREE(has_erase_signal,      DMcArray4,    false);
    D_MC_CV_REF_AGREE(has_clear_signal,      DMcArray4,    false);
    D_MC_CV_REF_AGREE(has_resize_signal,     DMcSetInt,    false);

    // element signals
    D_MC_CV_REF_AGREE(has_mutable_subscript_signal, DMcVectorInt, true);
    D_MC_CV_REF_AGREE(has_mutable_data_signal,      DMcVectorInt, true);
    D_MC_CV_REF_AGREE(has_mutable_subscript_signal, DMcListInt,   false);
    D_MC_CV_REF_AGREE(has_mutable_data_signal,      DMcListInt,   false);

    // and on the probes
    D_MC_CV_REF_AGREE(has_push_back_signal, DMcPushBackOnly, true);
    D_MC_CV_REF_AGREE(has_clear_signal,     DMcClearOnly,     true);
    D_MC_CV_REF_AGREE(has_push_back_signal, DMcBare,           false);

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
bool tests_signal_value_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_MC_STATIC_CHECK(has_push_back_signal_v<DMcVectorInt> == true);
    D_MC_STATIC_CHECK(has_clear_signal_v<DMcArray4>        == false);

    // structural companions
    D_MC_CHECK(has_push_back_signal_v<DMcVectorInt>
                   == has_push_back_signal<DMcVectorInt>::value);
    D_MC_CHECK(has_push_front_signal_v<DMcDequeInt>
                   == has_push_front_signal<DMcDequeInt>::value);
    D_MC_CHECK(has_insert_signal_v<DMcSetInt>
                   == has_insert_signal<DMcSetInt>::value);
    D_MC_CHECK(has_erase_signal_v<DMcVectorInt>
                   == has_erase_signal<DMcVectorInt>::value);
    D_MC_CHECK(has_clear_signal_v<DMcVectorInt>
                   == has_clear_signal<DMcVectorInt>::value);
    D_MC_CHECK(has_resize_signal_v<DMcVectorInt>
                   == has_resize_signal<DMcVectorInt>::value);

    // element companions
    D_MC_CHECK(has_mutable_subscript_signal_v<DMcVectorInt>
                   == has_mutable_subscript_signal<DMcVectorInt>::value);
    D_MC_CHECK(has_mutable_data_signal_v<DMcVectorInt>
                   == has_mutable_data_signal<DMcVectorInt>::value);

    // negative carriers
    D_MC_CHECK(has_push_back_signal_v<DMcSetInt>            == false);
    D_MC_CHECK(has_push_front_signal_v<DMcVectorInt>        == false);
    D_MC_CHECK(has_insert_signal_v<DMcVectorInt>            == false);
    D_MC_CHECK(has_erase_signal_v<DMcArray4>                == false);
    D_MC_CHECK(has_clear_signal_v<DMcArray4>                == false);
    D_MC_CHECK(has_resize_signal_v<DMcArray4>               == false);
    D_MC_CHECK(has_mutable_subscript_signal_v<DMcListInt>   == false);
    D_MC_CHECK(has_mutable_data_signal_v<DMcListInt>        == false);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
