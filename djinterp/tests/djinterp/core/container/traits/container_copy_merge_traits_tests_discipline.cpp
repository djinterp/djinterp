/*******************************************************************************
* djinterp [core]               container_copy_merge_traits_tests_discipline.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_copy_merge_traits_tests_discipline.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_copy_merge_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. Discipline signals  (the local overlay reading)
// ===========================================================================
//   The header reads each operand's discipline from three structural probes
// rather than importing the overlay axis, to stay off that include graph.  The
// probes are tested in a fixed ORDER - keyed, then equivalence, then the
// sequence fallback - and the order is load-bearing: it decides what happens
// to a type that satisfies the keyed probe without being associative at all.
// tests_discipline_pair_sequence_misread is about exactly that.


/*
tests_discipline_signal_probes
  Verifies the three discipline probes in isolation, before any of them is
  combined into a verdict.
  Tests the following:
  - the equivalence probe detects a nested key_type, and access control
    participates - a private key_type is invisible
  - the keyed probe detects a pair-shaped value_type by its first_type and
    second_type members, and does NOT require a key_type
  - the unique probe detects a single-element insert whose result carries a
    `.second`; an insert of a different arity does not match, the probe calling
    with exactly one argument
  - the unique probe names the `.second` member without constraining its TYPE,
    so a result whose `.second` is not a bool still counts - a structural
    probe, not a semantic one
  - each probe is false where its mark is absent
*/
bool
tests_discipline_signal_probes()
{
    bool ok = true;

    // the equivalence mark
    D_CM_CHECK(internal::has_equivalence_helper<cm_set_like>::value  == true);
    D_CM_CHECK(internal::has_equivalence_helper<cm_map_like>::value  == true);
    D_CM_CHECK(internal::has_equivalence_helper<cm_seq>::value       == false);
    D_CM_CHECK(internal::has_equivalence_helper<cm_private_key>::value
                   == false);

    // the keyed mark, which does not require an equivalence mark
    D_CM_CHECK(internal::has_keyed_value_helper<cm_map_like>::value  == true);
    D_CM_CHECK(internal::has_keyed_value_helper<cm_pair_sequence>::value
                   == true);
    D_CM_CHECK(   ( internal::has_keyed_value_helper<
                        cm_pair_sequence>::value == true )
               && ( internal::has_equivalence_helper<
                        cm_pair_sequence>::value == false ));
    D_CM_CHECK(internal::has_keyed_value_helper<cm_seq>::value      == false);
    D_CM_CHECK(internal::has_keyed_value_helper<cm_set_like>::value == false);

    // ... and BOTH pair members are named, so a half-pair does not match
    D_CM_CHECK(internal::has_keyed_value_helper<
                   cm_half_pair_value>::value == false);
    D_CM_CHECK(merge_discipline_of<cm_half_pair_value>::value
                   == merge_discipline::sequence);

    // the unique mark
    D_CM_CHECK(internal::has_unique_insert_helper<cm_set_like>::value
                   == true);
    D_CM_CHECK(internal::has_unique_insert_helper<cm_multiset_like>::value
                   == false);
    D_CM_CHECK(internal::has_unique_insert_helper<cm_seq>::value == false);

    // arity matters - the probe calls with exactly one argument
    D_CM_CHECK(internal::has_unique_insert_helper<
                   cm_insert_wrong_arity>::value == false);

    // ... but the type of `.second` does not
    D_CM_CHECK(internal::has_unique_insert_helper<
                   cm_insert_second_not_bool>::value == true);

    // it is `.second` specifically, not merely some member: a result carrying
    // a `.first` and no `.second` does not match
    D_CM_CHECK(internal::has_unique_insert_helper<
                   cm_insert_first_only>::value == false);
    D_CM_CHECK(merge_discipline_of<cm_insert_first_only>::value
                   == merge_discipline::multiset);

    return ok;
}

/*
tests_discipline_boundedness_probes
  Verifies the five boundedness probes the overflow proviso is built from.

  These mirror the boundedness axis rather than importing it, so they are worth
  checking here even though the proviso that uses them lives in section V.

  Tests the following:
  - each of the four positive signals is detected on a carrier that has it
    alone, so none is standing in for another
  - std::array is detected through the tuple_size route specifically
  - reserve() is detected as the anti-signal, and a growable container carries
    BOTH capacity() and reserve() - which is what the proviso's conjunction
    exists to separate
  - all five are false on an unbounded carrier
*/
bool
tests_discipline_boundedness_probes()
{
    bool ok = true;

    // each signal alone
    D_CM_CHECK(   ( internal::has_extent_helper<cm_extent>::value == true )
               && ( internal::has_tuple_size_helper<cm_extent>::value
                        == false )
               && ( internal::has_static_bounds_helper<cm_extent>::value
                        == false )
               && ( internal::has_capacity_helper<cm_extent>::value
                        == false ));

    D_CM_CHECK(   ( internal::has_static_bounds_helper<
                        cm_static_bounds>::value == true )
               && ( internal::has_extent_helper<cm_static_bounds>::value
                        == false ));

    // both interval constants are named, so a half-open domain is not bounded
    D_CM_CHECK(internal::has_static_bounds_helper<
                   cm_lower_bound_only>::value == false);

    D_CM_CHECK(   ( internal::has_capacity_helper<cm_capacity_only>::value
                        == true )
               && ( internal::has_reserve_helper<cm_capacity_only>::value
                        == false ));

    // the tuple_size route, on the standard carrier
    D_CM_CHECK(   ( internal::has_tuple_size_helper<cm_array>::value == true )
               && ( internal::has_extent_helper<cm_array>::value == false ));
    D_CM_CHECK(internal::has_tuple_size_helper<cm_vector>::value == false);

    // the anti-signal, and the pair it must be separated from
    D_CM_CHECK(   ( internal::has_capacity_helper<cm_capacity_reserve>::value
                        == true )
               && ( internal::has_reserve_helper<cm_capacity_reserve>::value
                        == true ));
    D_CM_CHECK(   ( internal::has_capacity_helper<cm_vector>::value == true )
               && ( internal::has_reserve_helper<cm_vector>::value == true ));

    // all false on an unbounded carrier
    D_CM_CHECK(   ( internal::has_extent_helper<cm_unbounded>::value
                        == false )
               && ( internal::has_tuple_size_helper<cm_unbounded>::value
                        == false )
               && ( internal::has_static_bounds_helper<cm_unbounded>::value
                        == false )
               && ( internal::has_capacity_helper<cm_unbounded>::value
                        == false )
               && ( internal::has_reserve_helper<cm_unbounded>::value
                        == false ));

    return ok;
}

/*
tests_merge_discipline_enum
  Verifies the discipline enum's shape.
  Tests the following:
  - the six enumerators are pairwise distinct
  - the walker visits each exactly once, so the loops elsewhere in this suite
    cover the whole enum rather than a subset
  - unknown is the first enumerator, so it is the value a default-initialised
    merge_discipline takes - the safe default for a classification
*/
bool
tests_merge_discipline_enum()
{
    bool ok = true;

    // pairwise distinct
    for (int i = 0; i < cm_discipline_count; ++i)
    {
        for (int j = i + 1; j < cm_discipline_count; ++j)
        {
            D_CM_CHECK(cm_discipline_at(i) != cm_discipline_at(j));
        }
    }

    // the walker visits each exactly once
    D_CM_CHECK(cm_discipline_at(0) == merge_discipline::unknown);
    D_CM_CHECK(cm_discipline_at(1) == merge_discipline::sequence);
    D_CM_CHECK(cm_discipline_at(2) == merge_discipline::set);
    D_CM_CHECK(cm_discipline_at(3) == merge_discipline::multiset);
    D_CM_CHECK(cm_discipline_at(4) == merge_discipline::map);
    D_CM_CHECK(cm_discipline_at(5) == merge_discipline::multimap);

    // unknown is the zero value, so a default-initialised one is safe
    D_CM_STATIC_CHECK(static_cast<int>(merge_discipline::unknown) == 0);
    D_CM_CHECK(merge_discipline() == merge_discipline::unknown);

    return ok;
}

/*
tests_merge_discipline_name
  Verifies the discipline spellings.
  Tests the following:
  - each of the six enumerators names itself
  - the six spellings are pairwise distinct, so no two disciplines are
    confusable in a diagnostic
  - the function is usable in a constant expression
*/
bool
tests_merge_discipline_name()
{
    bool ok = true;

    D_CM_CHECK(cm_disc_is(merge_discipline::unknown,  "unknown"));
    D_CM_CHECK(cm_disc_is(merge_discipline::sequence, "sequence"));
    D_CM_CHECK(cm_disc_is(merge_discipline::set,      "set"));
    D_CM_CHECK(cm_disc_is(merge_discipline::multiset, "multiset"));
    D_CM_CHECK(cm_disc_is(merge_discipline::map,      "map"));
    D_CM_CHECK(cm_disc_is(merge_discipline::multimap, "multimap"));

    // pairwise distinct spellings
    for (int i = 0; i < cm_discipline_count; ++i)
    {
        for (int j = i + 1; j < cm_discipline_count; ++j)
        {
            D_CM_CHECK(std::strcmp(
                           merge_discipline_name(cm_discipline_at(i)),
                           merge_discipline_name(cm_discipline_at(j))) != 0);
        }
    }

    // constexpr
    D_CM_STATIC_CHECK(merge_discipline_name(merge_discipline::set)[0] == 's');

    return ok;
}

/*
tests_discipline_family_predicates
  Verifies the three family predicates that group the six disciplines.
  Tests the following:
  - each predicate holds exactly on its own family
  - the three families are mutually exclusive
  - they do NOT cover the enum: unknown belongs to none, which is what makes a
    non-container fall through to incompatible rather than joining a family
  - every discipline is in at most one family, walked over the whole enum
*/
bool
tests_discipline_family_predicates()
{
    bool ok = true;

    // each on its own family
    D_CM_CHECK(discipline_is_sequence(merge_discipline::sequence) == true);
    D_CM_CHECK(discipline_is_bag(merge_discipline::set)           == true);
    D_CM_CHECK(discipline_is_bag(merge_discipline::multiset)      == true);
    D_CM_CHECK(discipline_is_keyed(merge_discipline::map)         == true);
    D_CM_CHECK(discipline_is_keyed(merge_discipline::multimap)    == true);

    // and nowhere else
    D_CM_CHECK(discipline_is_sequence(merge_discipline::set)      == false);
    D_CM_CHECK(discipline_is_bag(merge_discipline::sequence)      == false);
    D_CM_CHECK(discipline_is_bag(merge_discipline::map)           == false);
    D_CM_CHECK(discipline_is_keyed(merge_discipline::multiset)    == false);

    // unknown belongs to no family
    D_CM_CHECK(   ( discipline_is_sequence(merge_discipline::unknown)
                        == false )
               && ( discipline_is_bag(merge_discipline::unknown) == false )
               && ( discipline_is_keyed(merge_discipline::unknown)
                        == false ));

    // at most one family each, over the whole enum
    for (int i = 0; i < cm_discipline_count; ++i)
    {
        const merge_discipline d = cm_discipline_at(i);
        const int              n = static_cast<int>(discipline_is_sequence(d))
                                 + static_cast<int>(discipline_is_bag(d))
                                 + static_cast<int>(discipline_is_keyed(d));

        D_CM_CHECK(n <= 1);

        // exactly one, except for unknown
        if (d != merge_discipline::unknown)
        {
            D_CM_CHECK(n == 1);
        }
        else
        {
            D_CM_CHECK(n == 0);
        }
    }

    // constexpr
    D_CM_STATIC_CHECK(discipline_is_bag(merge_discipline::set));

    return ok;
}

/*
tests_merge_discipline_of
  Verifies the per-operand verdict on the synthetic carriers, where each signal
  can be varied on its own.
  Tests the following:
  - a carrier for each of the six disciplines
  - the container guard short-circuits ahead of every signal, so a type
    carrying every associative mark but no begin()/end() is unknown
  - the uniqueness split, on both the bag and the keyed branch
  - a probe defeated by access control changes the verdict, so the reading is
    exactly as far-reaching as the probes are
*/
bool
tests_merge_discipline_of()
{
    bool ok = true;

    // one carrier per discipline
    D_CM_CHECK(merge_discipline_of<cm_seq>::value
                   == merge_discipline::sequence);
    D_CM_CHECK(merge_discipline_of<cm_set_like>::value
                   == merge_discipline::set);
    D_CM_CHECK(merge_discipline_of<cm_multiset_like>::value
                   == merge_discipline::multiset);
    D_CM_CHECK(merge_discipline_of<cm_map_like>::value
                   == merge_discipline::map);
    D_CM_CHECK(merge_discipline_of<cm_multimap_like>::value
                   == merge_discipline::multimap);
    D_CM_CHECK(merge_discipline_of<cm_not_container>::value
                   == merge_discipline::unknown);

    // the container guard runs first
    D_CM_CHECK(   ( internal::has_keyed_value_helper<
                        cm_key_no_iter>::value == true )
               && ( internal::has_equivalence_helper<
                        cm_key_no_iter>::value == true )
               && ( internal::has_unique_insert_helper<
                        cm_key_no_iter>::value == true )
               && ( merge_discipline_of<cm_key_no_iter>::value
                        == merge_discipline::unknown ));

    // the uniqueness split, on the bag branch
    D_CM_CHECK(   ( merge_discipline_of<cm_set_like>::value
                        == merge_discipline::set )
               && ( merge_discipline_of<cm_multiset_like>::value
                        == merge_discipline::multiset )
               && ( internal::has_unique_insert_helper<cm_set_like>::value
                        != internal::has_unique_insert_helper<
                               cm_multiset_like>::value ));

    // ... and on the keyed branch
    D_CM_CHECK(   ( merge_discipline_of<cm_map_like>::value
                        == merge_discipline::map )
               && ( merge_discipline_of<cm_multimap_like>::value
                        == merge_discipline::multimap ));

    // a probe defeated by access control changes the verdict
    D_CM_CHECK(merge_discipline_of<cm_private_key>::value
                   == merge_discipline::sequence);

    // ... and an insert of the wrong arity moves a set to a multiset
    D_CM_CHECK(merge_discipline_of<cm_insert_wrong_arity>::value
                   == merge_discipline::multiset);

    return ok;
}

/*
tests_merge_discipline_of_std_types
  Verifies that the LOCAL signals agree with the standard containers, which is
  the header's stated justification for detecting them here rather than
  importing the overlay axis.
  Tests the following:
  - every ordered and unordered standard container reads the discipline its
    name implies
  - the sequence containers all read sequence, whatever their internal shape
  - std::string reads sequence, being a container of characters
  - a std::pair is unknown, having no traversal
  - the unordered containers agree with their ordered counterparts, so the
    reading does not depend on the ordering the axis is named for
*/
bool
tests_merge_discipline_of_std_types()
{
    bool ok = true;

    // sequences
    D_CM_CHECK(merge_discipline_of<cm_vector>::value
                   == merge_discipline::sequence);
    D_CM_CHECK(merge_discipline_of<cm_array>::value
                   == merge_discipline::sequence);
    D_CM_CHECK(merge_discipline_of<cm_deque>::value
                   == merge_discipline::sequence);
    D_CM_CHECK(merge_discipline_of<cm_list>::value
                   == merge_discipline::sequence);
    D_CM_CHECK(merge_discipline_of<cm_string>::value
                   == merge_discipline::sequence);

    // associatives
    D_CM_CHECK(merge_discipline_of<cm_set>::value == merge_discipline::set);
    D_CM_CHECK(merge_discipline_of<cm_multiset>::value
                   == merge_discipline::multiset);
    D_CM_CHECK(merge_discipline_of<cm_map>::value == merge_discipline::map);
    D_CM_CHECK(merge_discipline_of<cm_multimap>::value
                   == merge_discipline::multimap);

    // the unordered counterparts agree
    D_CM_CHECK(merge_discipline_of<cm_uset>::value
                   == merge_discipline_of<cm_set>::value);
    D_CM_CHECK(merge_discipline_of<cm_umap>::value
                   == merge_discipline_of<cm_map>::value);

    // not a container
    D_CM_CHECK(merge_discipline_of<cm_pair>::value
                   == merge_discipline::unknown);
    D_CM_CHECK(merge_discipline_of<int>::value == merge_discipline::unknown);
    D_CM_CHECK(merge_discipline_of<void>::value == merge_discipline::unknown);

    return ok;
}

/*
tests_discipline_pair_sequence_misread
  Records a MISCLASSIFICATION: a plain sequence whose elements happen to be
  pairs is read as a keyed discipline.

  The verdict tests the keyed mark - a pair-shaped value_type - BEFORE the
  equivalence mark, and returns from the keyed branch without ever consulting
  key_type.  So a std::vector<std::pair<K, V>>, which carries no key_type and
  has no associative behaviour whatever, reads as a multimap; give it a
  single-element insert returning a `.second` and it reads as a map.

  The consequence is not confined to the discipline reading.  Downstream:

    - merge_kind_of<vector<pair>, map> reports keyed_merge, so the header
      claims a plain vector can be merged into a std::map BY KEY;
    - merge_has_key_conflict on that pair reports true, describing a conflict
      in an operand that has no keys;
    - merge_kind_of<vector<pair>, vector<int>> reports incompatible, refusing a
      concatenation of two sequences purely because one holds pairs.

  Every associative container in the standard library carries key_type
  alongside its pair value_type, so conjoining the equivalence mark into the
  keyed test would separate these cases without disturbing any of the verdicts
  in tests_merge_discipline_of_std_types.

  Tests the following:
  - the synthetic pair-valued sequence reads multimap, and its unique-insert
    variant reads map
  - the standard-library witness, std::vector<std::pair<int, int>>, reads the
    same way
  - the carriers have the keyed mark WITHOUT the equivalence mark, which is
    what distinguishes them from every genuine associative container
  - every real associative container has both marks, so the proposed
    conjunction would leave them alone
  - the three downstream consequences, stated directly
*/
bool
tests_discipline_pair_sequence_misread()
{
    bool ok = true;

    // the misread itself
    D_CM_CHECK(merge_discipline_of<cm_pair_sequence>::value
                   == merge_discipline::multimap);
    D_CM_CHECK(merge_discipline_of<cm_pair_sequence_unique>::value
                   == merge_discipline::map);
    D_CM_CHECK(merge_discipline_of<cm_pair_vector>::value
                   == merge_discipline::multimap);

    // ... on carriers with the keyed mark and NO equivalence mark
    D_CM_CHECK(   ( internal::has_keyed_value_helper<
                        cm_pair_sequence>::value == true )
               && ( internal::has_equivalence_helper<
                        cm_pair_sequence>::value == false ));
    D_CM_CHECK(   ( internal::has_keyed_value_helper<
                        cm_pair_vector>::value == true )
               && ( internal::has_equivalence_helper<
                        cm_pair_vector>::value == false ));

    // every genuine associative container carries BOTH marks
    D_CM_CHECK(   ( internal::has_keyed_value_helper<cm_map>::value == true )
               && ( internal::has_equivalence_helper<cm_map>::value
                        == true ));
    D_CM_CHECK(   ( internal::has_keyed_value_helper<cm_multimap>::value
                        == true )
               && ( internal::has_equivalence_helper<cm_multimap>::value
                        == true ));
    D_CM_CHECK(   ( internal::has_keyed_value_helper<cm_umap>::value == true )
               && ( internal::has_equivalence_helper<cm_umap>::value
                        == true ));

    // consequence 1: a keyed merge is claimed against a real map
    D_CM_CHECK(merge_kind_of<cm_pair_vector, cm_map>::value
                   == merge_kind::keyed_merge);
    D_CM_CHECK((is_mergeable<cm_pair_vector, cm_map>::value) == true);

    // consequence 2: a key conflict is reported for a keyless operand
    D_CM_CHECK((merge_has_key_conflict<cm_pair_vector, cm_map>::value)
                   == true);

    // consequence 3: two sequences are refused a concatenation
    D_CM_CHECK(merge_kind_of<cm_pair_vector, cm_vector>::value
                   == merge_kind::incompatible);
    D_CM_CHECK(merge_discipline_of<cm_pair_vector>::value
                   != merge_discipline_of<cm_vector>::value);

    return ok;
}

/*
tests_discipline_signal_order
  Verifies the ORDER in which the verdict consults its signals, independently
  of whether that order is the right one.

  The reading is a chain: not a container, else keyed, else equivalence, else
  sequence.  Each branch is reachable, and each earlier branch masks the later
  ones - which is what the misread above is a consequence of.

  Tests the following:
  - the container guard masks all three signals
  - the keyed branch masks the equivalence branch, shown by a carrier that
    satisfies both, and by one that satisfies only the keyed mark
  - the equivalence branch masks the sequence fallback
  - the fallback is reached only when all three earlier tests fail
  - each of the four branches is exercised by some carrier, so none is dead
*/
bool
tests_discipline_signal_order()
{
    bool ok = true;

    // branch 1: the container guard masks everything
    D_CM_CHECK(merge_discipline_of<cm_key_no_iter>::value
                   == merge_discipline::unknown);

    // branch 2: keyed, and it masks the equivalence branch
    D_CM_CHECK(   ( internal::has_keyed_value_helper<cm_map_like>::value
                        == true )
               && ( internal::has_equivalence_helper<cm_map_like>::value
                        == true )
               && ( merge_discipline_of<cm_map_like>::value
                        == merge_discipline::map ));

    // ... and is taken with the equivalence mark ABSENT
    D_CM_CHECK(   ( internal::has_equivalence_helper<
                        cm_pair_sequence>::value == false )
               && ( merge_discipline_of<cm_pair_sequence>::value
                        == merge_discipline::multimap ));

    // branch 3: equivalence, masking the fallback
    D_CM_CHECK(   ( internal::has_keyed_value_helper<cm_set_like>::value
                        == false )
               && ( internal::has_equivalence_helper<cm_set_like>::value
                        == true )
               && ( merge_discipline_of<cm_set_like>::value
                        == merge_discipline::set ));

    // branch 4: the fallback, all earlier tests failing
    D_CM_CHECK(   ( is_iterable_container<cm_seq>::value == true )
               && ( internal::has_keyed_value_helper<cm_seq>::value
                        == false )
               && ( internal::has_equivalence_helper<cm_seq>::value
                        == false )
               && ( merge_discipline_of<cm_seq>::value
                        == merge_discipline::sequence ));

    // every branch reached
    D_CM_CHECK(merge_discipline_of<cm_not_container>::value
                   == merge_discipline::unknown);
    D_CM_CHECK(merge_discipline_of<cm_multimap_like>::value
                   == merge_discipline::multimap);
    D_CM_CHECK(merge_discipline_of<cm_multiset_like>::value
                   == merge_discipline::multiset);
    D_CM_CHECK(merge_discipline_of<cm_vector>::value
                   == merge_discipline::sequence);

    return ok;
}

/*
tests_merge_discipline_cv_ref
  Verifies that the verdict and its probes honour the clean_t contract.
  Tests the following:
  - the verdict is unchanged across all seven cv- and reference-qualified
    spellings, on carriers of several disciplines
  - the three signal probes likewise
  - the volatile spellings in particular
*/
bool
tests_merge_discipline_cv_ref()
{
    bool ok = true;

    // the verdict
    D_CM_CHECK(merge_discipline_of<const cm_vector>::value
                   == merge_discipline_of<cm_vector>::value);
    D_CM_CHECK(merge_discipline_of<volatile cm_vector>::value
                   == merge_discipline_of<cm_vector>::value);
    D_CM_CHECK(merge_discipline_of<const volatile cm_vector>::value
                   == merge_discipline_of<cm_vector>::value);
    D_CM_CHECK(merge_discipline_of<cm_vector&>::value
                   == merge_discipline_of<cm_vector>::value);
    D_CM_CHECK(merge_discipline_of<const cm_vector&>::value
                   == merge_discipline_of<cm_vector>::value);
    D_CM_CHECK(merge_discipline_of<cm_vector&&>::value
                   == merge_discipline_of<cm_vector>::value);

    D_CM_CHECK(merge_discipline_of<const cm_map&>::value
                   == merge_discipline_of<cm_map>::value);
    D_CM_CHECK(merge_discipline_of<volatile cm_multiset>::value
                   == merge_discipline_of<cm_multiset>::value);
    D_CM_CHECK(merge_discipline_of<const volatile cm_set_like&>::value
                   == merge_discipline_of<cm_set_like>::value);
    D_CM_CHECK(merge_discipline_of<cm_pair_sequence&&>::value
                   == merge_discipline_of<cm_pair_sequence>::value);

    // the probes
    D_CM_CV_REF_AGREE(internal::has_equivalence_helper, cm_set_like,  true);
    D_CM_CV_REF_AGREE(internal::has_equivalence_helper, cm_seq,       false);
    D_CM_CV_REF_AGREE(internal::has_keyed_value_helper, cm_map_like,  true);
    D_CM_CV_REF_AGREE(internal::has_keyed_value_helper, cm_seq,       false);
    D_CM_CV_REF_AGREE(internal::has_unique_insert_helper, cm_set_like, true);
    D_CM_CV_REF_AGREE(internal::has_unique_insert_helper,
                      cm_multiset_like, false);

    return ok;
}

/*
tests_merge_discipline_companions
  Verifies the companions of the discipline verdict.
  Tests the following:
  - the `_t` alias is the integral_constant carrying the verdict, so a caller
    can tag-dispatch on it
  - the `_v` companion agrees with ::value
  - both are usable in a constant expression
  - the trait does NOT derive from integral_constant - it carries `value` and
    a nested `type` as members - so the standard members reachable through
    inheritance are absent here
  - the block is gated on variable-template support, as the header's own
    definition is
*/
bool
tests_merge_discipline_companions()
{
    bool ok = true;

    // the `_t` alias
    D_CM_STATIC_CHECK(
        std::is_same<merge_discipline_of_t<cm_vector>,
                     std::integral_constant<merge_discipline,
                                            merge_discipline::sequence>
                    >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_discipline_of_t<cm_map>,
                     std::integral_constant<merge_discipline,
                                            merge_discipline::map>
                    >::value);
    D_CM_CHECK(merge_discipline_of_t<cm_set>::value
                   == merge_discipline_of<cm_set>::value);

    // the trait is NOT itself an integral_constant
    D_CM_STATIC_CHECK(
        std::is_base_of<merge_discipline_of_t<cm_vector>,
                        merge_discipline_of<cm_vector> >::value == false);

    // constexpr
    D_CM_STATIC_CHECK(merge_discipline_of<cm_vector>::value
                          == merge_discipline::sequence);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CM_STATIC_CHECK(merge_discipline_of_v<cm_map> == merge_discipline::map);

    D_CM_CHECK(merge_discipline_of_v<cm_vector>
                   == merge_discipline_of<cm_vector>::value);
    D_CM_CHECK(merge_discipline_of_v<cm_multiset>
                   == merge_discipline_of<cm_multiset>::value);
    D_CM_CHECK(merge_discipline_of_v<cm_pair_vector>
                   == merge_discipline_of<cm_pair_vector>::value);
    D_CM_CHECK(merge_discipline_of_v<int>
                   == merge_discipline_of<int>::value);
    D_CM_CHECK(merge_discipline_of_v<const cm_map&>
                   == merge_discipline_of<cm_map>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
