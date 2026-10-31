/*******************************************************************************
* djinterp [cpp]                   ordered_container_traits_tests_order_axis.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/ordered_container_traits_tests_order_axis.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "ordered_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   Order-axis predicates
// ===========================================================================
//   Two predicates over one signal.  Both gate on iterability first, then
// split on whether an associative key_type is present: a keyed container's
// identity is its mapping, so no rearrangement of it is a different container
// and it has no positional order; an unkeyed iterable one is a positional
// sequence (e_1, ..., e_n).


/*
tests_has_key_type_helper
  Verifies the associative tell - a `key_type` alias - which is the single
  signal the whole order axis turns on.
  Tests the following:
  - the standard associative containers all carry it, ordered and hashed alike
  - the sequence containers do not
  - the helper detects the ALIAS and nothing about what it names, so an alias
    to void counts
  - a PRIVATE key_type does not count: access control participates, so such a
    type reads as an ordinary sequence
  - a non-container carrying the alias is still detected, since the helper runs
    ahead of the iterability gate rather than behind it
*/
bool tests_has_key_type_helper()
{
    bool ok = true;

    // the associative containers
    D_OR_CHECK(internal::has_key_type_helper<DOrdSet>::value      == true);
    D_OR_CHECK(internal::has_key_type_helper<DOrdMultiset>::value == true);
    D_OR_CHECK(internal::has_key_type_helper<DOrdMap>::value      == true);
    D_OR_CHECK(internal::has_key_type_helper<DOrdUset>::value     == true);
    D_OR_CHECK(internal::has_key_type_helper<DOrdUmap>::value     == true);
    D_OR_CHECK(internal::has_key_type_helper<DOrdKeyed>::value    == true);

    // the sequence containers
    D_OR_CHECK(internal::has_key_type_helper<DOrdVector>::value   == false);
    D_OR_CHECK(internal::has_key_type_helper<DOrdDeque>::value    == false);
    D_OR_CHECK(internal::has_key_type_helper<DOrdList>::value     == false);
    D_OR_CHECK(internal::has_key_type_helper<DOrdFlist>::value    == false);
    D_OR_CHECK(internal::has_key_type_helper<DOrdArray>::value    == false);
    D_OR_CHECK(internal::has_key_type_helper<DOrdString>::value   == false);
    D_OR_CHECK(internal::has_key_type_helper<DOrdSequence>::value == false);

    // the alias is detected, not what it names
    D_OR_CHECK(internal::has_key_type_helper<DOrdKeyedVoid>::value == true);

    // access control participates
    D_OR_CHECK(internal::has_key_type_helper<DOrdPrivateKey>::value
                   == false);

    // detected independently of iterability
    D_OR_CHECK(internal::has_key_type_helper<DOrdKeyedNotIterable>::value
                   == true);

    // non-containers
    D_OR_CHECK(internal::has_key_type_helper<DOrdBare>::value == false);
    D_OR_CHECK(internal::has_key_type_helper<DOrdPair>::value == false);
    D_OR_CHECK(internal::has_key_type_helper<int>::value      == false);
    D_OR_CHECK(internal::has_key_type_helper<void>::value     == false);

    return ok;
}

/*
tests_is_ordered_container
  Verifies the positional-identity predicate: iterable AND unkeyed.
  Tests the following:
  - every standard sequence container qualifies, whatever its traversal
    strength - a forward_list is as ordered as a vector, since the axis is
    about whether position MEANS something, not how fast it is reached
  - the associative containers do not qualify
  - both conjuncts are necessary, witnessed separately: an iterable keyed type
    fails on the key, and a keyed non-iterable type fails on iterability
  - a type with a private key_type qualifies, the alias being invisible
  - it equals its definition term by term
*/
bool tests_is_ordered_container()
{
    bool ok = true;

    // the sequence containers, at every traversal strength
    D_OR_CHECK(is_ordered_container<DOrdVector>::value      == true);
    D_OR_CHECK(is_ordered_container<DOrdBoolVector>::value == true);
    D_OR_CHECK(is_ordered_container<DOrdArray>::value       == true);
    D_OR_CHECK(is_ordered_container<DOrdDeque>::value       == true);
    D_OR_CHECK(is_ordered_container<DOrdList>::value        == true);
    D_OR_CHECK(is_ordered_container<DOrdFlist>::value       == true);
    D_OR_CHECK(is_ordered_container<DOrdString>::value      == true);
    D_OR_CHECK(is_ordered_container<DOrdNested>::value      == true);
    D_OR_CHECK(is_ordered_container<DOrdInputOnly>::value  == true);

    // the associative containers
    D_OR_CHECK(is_ordered_container<DOrdSet>::value      == false);
    D_OR_CHECK(is_ordered_container<DOrdMultiset>::value == false);
    D_OR_CHECK(is_ordered_container<DOrdMap>::value      == false);
    D_OR_CHECK(is_ordered_container<DOrdUset>::value     == false);
    D_OR_CHECK(is_ordered_container<DOrdUmap>::value     == false);

    // each conjunct is necessary
    D_OR_CHECK(   ( is_iterable_container<DOrdKeyed>::value == true )
               && ( is_ordered_container<DOrdKeyed>::value  == false ));
    D_OR_CHECK(   ( internal::has_key_type_helper<
                        DOrdKeyedNotIterable>::value          == true )
               && ( is_ordered_container<DOrdKeyedNotIterable>::value
                        == false ));
    D_OR_CHECK(   ( is_iterable_container<DOrdBeginOnly>::value == false )
               && ( is_ordered_container<DOrdBeginOnly>::value  == false ));

    // an invisible key leaves the type a sequence
    D_OR_CHECK(is_ordered_container<DOrdPrivateKey>::value == true);

    // non-containers
    D_OR_CHECK(is_ordered_container<DOrdBare>::value == false);
    D_OR_CHECK(is_ordered_container<DOrdPair>::value == false);
    D_OR_CHECK(is_ordered_container<int>::value      == false);
    D_OR_CHECK(is_ordered_container<void>::value     == false);

    // the definition, term by term
    D_OR_CHECK(is_ordered_container<DOrdVector>::value
                   == (    is_iterable_container<DOrdVector>::value
                        && !internal::has_key_type_helper<DOrdVector>::value ));
    D_OR_CHECK(is_ordered_container<DOrdSet>::value
                   == (    is_iterable_container<DOrdSet>::value
                        && !internal::has_key_type_helper<DOrdSet>::value ));
    D_OR_CHECK(is_ordered_container<DOrdBare>::value
                   == (    is_iterable_container<DOrdBare>::value
                        && !internal::has_key_type_helper<DOrdBare>::value ));

    return ok;
}

/*
tests_is_unordered_container
  Verifies the permutation-invariance predicate: iterable AND keyed.
  Tests the following:
  - every standard associative container qualifies, ORDERED and hashed alike -
    a std::map is as unordered on this axis as a std::unordered_map
  - the sequence containers do not
  - iterability is still required, so a keyed non-container does not qualify
  - it equals its definition term by term
*/
bool tests_is_unordered_container()
{
    bool ok = true;

    // the associative containers, sorted and hashed alike
    D_OR_CHECK(is_unordered_container<DOrdSet>::value      == true);
    D_OR_CHECK(is_unordered_container<DOrdMultiset>::value == true);
    D_OR_CHECK(is_unordered_container<DOrdMap>::value      == true);
    D_OR_CHECK(is_unordered_container<DOrdUset>::value     == true);
    D_OR_CHECK(is_unordered_container<DOrdUmap>::value     == true);
    D_OR_CHECK(is_unordered_container<DOrdKeyed>::value    == true);
    D_OR_CHECK(is_unordered_container<DOrdKeyedVoid>::value == true);

    // the sequence containers
    D_OR_CHECK(is_unordered_container<DOrdVector>::value == false);
    D_OR_CHECK(is_unordered_container<DOrdDeque>::value  == false);
    D_OR_CHECK(is_unordered_container<DOrdList>::value   == false);
    D_OR_CHECK(is_unordered_container<DOrdFlist>::value  == false);
    D_OR_CHECK(is_unordered_container<DOrdString>::value == false);

    // iterability is still required
    D_OR_CHECK(is_unordered_container<DOrdKeyedNotIterable>::value == false);

    // non-containers
    D_OR_CHECK(is_unordered_container<DOrdBare>::value == false);
    D_OR_CHECK(is_unordered_container<DOrdPair>::value == false);
    D_OR_CHECK(is_unordered_container<int>::value      == false);
    D_OR_CHECK(is_unordered_container<void>::value     == false);

    // the definition, term by term
    D_OR_CHECK(is_unordered_container<DOrdSet>::value
                   == (    is_iterable_container<DOrdSet>::value
                        && internal::has_key_type_helper<DOrdSet>::value ));
    D_OR_CHECK(is_unordered_container<DOrdKeyedNotIterable>::value
                   == (    is_iterable_container<
                               DOrdKeyedNotIterable>::value
                        && internal::has_key_type_helper<
                               DOrdKeyedNotIterable>::value ));

    return ok;
}

/*
tests_order_axis_exclusivity
  Verifies how the two predicates relate.  They are mutually exclusive but NOT
  complementary, because both are gated on iterability: a non-container is
  neither, which is what leaves room for the third state the classification
  carries.
  Tests the following:
  - no type is both, checked across every carrier the suite defines
  - a non-container is NEITHER, so `!is_ordered` does not mean "unordered"
  - their disjunction is exactly is_iterable_container, which is the precise
    statement of how the axis partitions the world
*/
bool tests_order_axis_exclusivity()
{
    bool ok = true;

    // mutually exclusive
    D_OR_CHECK(!(    is_ordered_container<DOrdVector>::value
                  && is_unordered_container<DOrdVector>::value ));
    D_OR_CHECK(!(    is_ordered_container<DOrdSet>::value
                  && is_unordered_container<DOrdSet>::value ));
    D_OR_CHECK(!(    is_ordered_container<DOrdBare>::value
                  && is_unordered_container<DOrdBare>::value ));
    D_OR_CHECK(!(    is_ordered_container<DOrdString>::value
                  && is_unordered_container<DOrdString>::value ));
    D_OR_CHECK(!(    is_ordered_container<DOrdUmap>::value
                  && is_unordered_container<DOrdUmap>::value ));

    // but not complementary - a non-container is neither
    D_OR_CHECK(   ( is_ordered_container<DOrdBare>::value   == false )
               && ( is_unordered_container<DOrdBare>::value == false ));
    D_OR_CHECK(   ( is_ordered_container<DOrdPair>::value   == false )
               && ( is_unordered_container<DOrdPair>::value == false ));
    D_OR_CHECK(   ( is_ordered_container<int>::value   == false )
               && ( is_unordered_container<int>::value == false ));
    D_OR_CHECK(   ( is_ordered_container<DOrdKeyedNotIterable>::value
                        == false )
               && ( is_unordered_container<DOrdKeyedNotIterable>::value
                        == false ));

    // their disjunction is exactly iterability
    D_OR_CHECK((    is_ordered_container<DOrdVector>::value
                 || is_unordered_container<DOrdVector>::value )
                   == is_iterable_container<DOrdVector>::value);
    D_OR_CHECK((    is_ordered_container<DOrdSet>::value
                 || is_unordered_container<DOrdSet>::value )
                   == is_iterable_container<DOrdSet>::value);
    D_OR_CHECK((    is_ordered_container<DOrdBare>::value
                 || is_unordered_container<DOrdBare>::value )
                   == is_iterable_container<DOrdBare>::value);
    D_OR_CHECK((    is_ordered_container<DOrdKeyedNotIterable>::value
                 || is_unordered_container<DOrdKeyedNotIterable>::value )
                   == is_iterable_container<
                          DOrdKeyedNotIterable>::value);
    D_OR_CHECK((    is_ordered_container<DOrdInputOnly>::value
                 || is_unordered_container<DOrdInputOnly>::value )
                   == is_iterable_container<DOrdInputOnly>::value);

    return ok;
}

/*
tests_order_axis_keyed_gap
  Verifies the reading this axis is easiest to get wrong.

  "Unordered" here means PERMUTATION-INVARIANT, not "not sorted".  A std::map
  is is_unordered_container even though enumerating it yields a perfectly
  stable, sorted sequence - because its identity is the mapping it holds, and
  no rearrangement of that mapping is a different map.  The everyday sense of
  "ordered" belongs to the SORTEDNESS axis in the sibling header, which
  classifies that same std::map as monotone.

  The consequence to keep straight is that std::map and std::unordered_map are
  INDISTINGUISHABLE on this axis, though one enumerates in comparator order and
  the other in bucket order.  The axis is not lying: neither carries positional
  identity.  It simply is not the axis that answers "does this come out
  sorted".

  Tests the following:
  - the tree-backed and hash-backed associatives receive the same answer, on
    every trait of the axis
  - a std::map is unordered while a std::vector, which has no sortedness
    guarantee at all, is ordered - so the axis is plainly not tracking
    sortedness
  - the discriminator really is the key_type alias and nothing else: adding one
    to an otherwise identical sequence fixture flips the classification
  - a keyed container has no layout, since layout is a property of positions
*/
bool tests_order_axis_keyed_gap()
{
    bool ok = true;

    // sorted-backed and hash-backed associatives are indistinguishable here
    D_OR_CHECK(is_unordered_container<DOrdMap>::value
                   == is_unordered_container<DOrdUmap>::value);
    D_OR_CHECK(is_ordered_container<DOrdMap>::value
                   == is_ordered_container<DOrdUmap>::value);
    D_OR_CHECK(ordering_kind_of<DOrdMap>::value
                   == ordering_kind_of<DOrdUmap>::value);
    D_OR_CHECK(ordering_kind_of<DOrdSet>::value
                   == ordering_kind_of<DOrdUset>::value);

    // ... and both are unordered
    D_OR_CHECK(ordering_kind_of<DOrdMap>::value == ordering_kind::unordered);
    D_OR_CHECK(ordering_kind_of<DOrdUmap>::value == ordering_kind::unordered);

    // while a vector, which guarantees no sortedness whatever, is ordered
    D_OR_CHECK(is_ordered_container<DOrdVector>::value == true);
    D_OR_CHECK(   ( is_ordered_container<DOrdVector>::value == true )
               && ( is_ordered_container<DOrdMap>::value    == false ));

    // the discriminator is the alias alone: two fixtures alike but for it
    D_OR_CHECK(   ( is_ordered_container<DOrdSequence>::value == true )
               && ( is_ordered_container<DOrdKeyed>::value    == false ));
    D_OR_CHECK(   ( is_unordered_container<DOrdSequence>::value == false )
               && ( is_unordered_container<DOrdKeyed>::value    == true ));
    D_OR_CHECK(   ( is_iterable_container<DOrdSequence>::value
                        == is_iterable_container<DOrdKeyed>::value )
               && ( ordering_kind_of<DOrdSequence>::value
                        != ordering_kind_of<DOrdKeyed>::value ));

    // layout is a property of positions, so a keyed container has none
    D_OR_CHECK(sequential_layout_of<DOrdMap>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<DOrdSet>::value
                   == sequential_layout::none);

    return ok;
}

/*
tests_order_axis_cv_ref
  Verifies that both predicates honour the clean_t contract.
  Tests the following:
  - both predicates are identical across all seven cv- and reference-qualified
    spellings, on a positive and a negative carrier each
  - the volatile spellings in particular, which a trait phrased on the
    unstripped type would lose
  - the underlying helper is likewise invariant, since it cleans before naming
    the nested alias
*/
bool tests_order_axis_cv_ref()
{
    bool ok = true;

    D_OR_CV_REF_AGREE(is_ordered_container, DOrdVector, true);
    D_OR_CV_REF_AGREE(is_ordered_container, DOrdString, true);
    D_OR_CV_REF_AGREE(is_ordered_container, DOrdSet,    false);
    D_OR_CV_REF_AGREE(is_ordered_container, DOrdBare,   false);

    D_OR_CV_REF_AGREE(is_unordered_container, DOrdMap,    true);
    D_OR_CV_REF_AGREE(is_unordered_container, DOrdUset,   true);
    D_OR_CV_REF_AGREE(is_unordered_container, DOrdVector, false);
    D_OR_CV_REF_AGREE(is_unordered_container, DOrdBare,   false);

    D_OR_CV_REF_AGREE(internal::has_key_type_helper, DOrdSet,    true);
    D_OR_CV_REF_AGREE(internal::has_key_type_helper, DOrdVector, false);

    // spelled out on one carrier, since a volatile container surviving the
    // probe is the point of writing clean_t
    D_OR_CHECK(is_ordered_container<volatile DOrdVector>::value   == true);
    D_OR_CHECK(is_unordered_container<volatile DOrdMap>::value    == true);
    D_OR_CHECK(is_ordered_container<const volatile DOrdString&>::value
                   == true);

    return ok;
}

/*
tests_order_axis_companions
  Verifies the `_v` companions of the two predicates.
  Tests the following:
  - each companion agrees with its trait's ::value on a positive and a negative
    carrier
  - both are usable in a constant expression
  - gated on variable-template support, since below C++14 the engine emits
    nothing and ::value remains the portable spelling
*/
bool tests_order_axis_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OR_STATIC_CHECK(is_ordered_container_v<DOrdVector>   == true);
    D_OR_STATIC_CHECK(is_unordered_container_v<DOrdMap>    == true);
    D_OR_STATIC_CHECK(is_ordered_container_v<DOrdSet>      == false);

    D_OR_CHECK(is_ordered_container_v<DOrdVector>
                   == is_ordered_container<DOrdVector>::value);
    D_OR_CHECK(is_ordered_container_v<DOrdFlist>
                   == is_ordered_container<DOrdFlist>::value);
    D_OR_CHECK(is_ordered_container_v<DOrdSet>
                   == is_ordered_container<DOrdSet>::value);
    D_OR_CHECK(is_ordered_container_v<DOrdBare>
                   == is_ordered_container<DOrdBare>::value);

    D_OR_CHECK(is_unordered_container_v<DOrdMap>
                   == is_unordered_container<DOrdMap>::value);
    D_OR_CHECK(is_unordered_container_v<DOrdUset>
                   == is_unordered_container<DOrdUset>::value);
    D_OR_CHECK(is_unordered_container_v<DOrdVector>
                   == is_unordered_container<DOrdVector>::value);
    D_OR_CHECK(is_unordered_container_v<DOrdBare>
                   == is_unordered_container<DOrdBare>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
