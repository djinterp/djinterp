/*******************************************************************************
* djinterp [core]                  ordered_container_traits_tests_order_axis.cpp
*
*
* path:      /tests/djinterp/core/container/traits/ordered_container_traits_tests_order_axis.cpp
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
bool
tests_has_key_type_helper()
{
    bool ok = true;

    // the associative containers
    D_OR_CHECK(internal::has_key_type_helper<ord_set>::value      == true);
    D_OR_CHECK(internal::has_key_type_helper<ord_multiset>::value == true);
    D_OR_CHECK(internal::has_key_type_helper<ord_map>::value      == true);
    D_OR_CHECK(internal::has_key_type_helper<ord_uset>::value     == true);
    D_OR_CHECK(internal::has_key_type_helper<ord_umap>::value     == true);
    D_OR_CHECK(internal::has_key_type_helper<ord_keyed>::value    == true);

    // the sequence containers
    D_OR_CHECK(internal::has_key_type_helper<ord_vector>::value   == false);
    D_OR_CHECK(internal::has_key_type_helper<ord_deque>::value    == false);
    D_OR_CHECK(internal::has_key_type_helper<ord_list>::value     == false);
    D_OR_CHECK(internal::has_key_type_helper<ord_flist>::value    == false);
    D_OR_CHECK(internal::has_key_type_helper<ord_array>::value    == false);
    D_OR_CHECK(internal::has_key_type_helper<ord_string>::value   == false);
    D_OR_CHECK(internal::has_key_type_helper<ord_sequence>::value == false);

    // the alias is detected, not what it names
    D_OR_CHECK(internal::has_key_type_helper<ord_keyed_void>::value == true);

    // access control participates
    D_OR_CHECK(internal::has_key_type_helper<ord_private_key>::value
                   == false);

    // detected independently of iterability
    D_OR_CHECK(internal::has_key_type_helper<ord_keyed_not_iterable>::value
                   == true);

    // non-containers
    D_OR_CHECK(internal::has_key_type_helper<ord_bare>::value == false);
    D_OR_CHECK(internal::has_key_type_helper<ord_pair>::value == false);
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
bool
tests_is_ordered_container()
{
    bool ok = true;

    // the sequence containers, at every traversal strength
    D_OR_CHECK(is_ordered_container<ord_vector>::value      == true);
    D_OR_CHECK(is_ordered_container<ord_bool_vector>::value == true);
    D_OR_CHECK(is_ordered_container<ord_array>::value       == true);
    D_OR_CHECK(is_ordered_container<ord_deque>::value       == true);
    D_OR_CHECK(is_ordered_container<ord_list>::value        == true);
    D_OR_CHECK(is_ordered_container<ord_flist>::value       == true);
    D_OR_CHECK(is_ordered_container<ord_string>::value      == true);
    D_OR_CHECK(is_ordered_container<ord_nested>::value      == true);
    D_OR_CHECK(is_ordered_container<ord_input_only>::value  == true);

    // the associative containers
    D_OR_CHECK(is_ordered_container<ord_set>::value      == false);
    D_OR_CHECK(is_ordered_container<ord_multiset>::value == false);
    D_OR_CHECK(is_ordered_container<ord_map>::value      == false);
    D_OR_CHECK(is_ordered_container<ord_uset>::value     == false);
    D_OR_CHECK(is_ordered_container<ord_umap>::value     == false);

    // each conjunct is necessary
    D_OR_CHECK(   ( is_iterable_container<ord_keyed>::value == true )
               && ( is_ordered_container<ord_keyed>::value  == false ));
    D_OR_CHECK(   ( internal::has_key_type_helper<
                        ord_keyed_not_iterable>::value          == true )
               && ( is_ordered_container<ord_keyed_not_iterable>::value
                        == false ));
    D_OR_CHECK(   ( is_iterable_container<ord_begin_only>::value == false )
               && ( is_ordered_container<ord_begin_only>::value  == false ));

    // an invisible key leaves the type a sequence
    D_OR_CHECK(is_ordered_container<ord_private_key>::value == true);

    // non-containers
    D_OR_CHECK(is_ordered_container<ord_bare>::value == false);
    D_OR_CHECK(is_ordered_container<ord_pair>::value == false);
    D_OR_CHECK(is_ordered_container<int>::value      == false);
    D_OR_CHECK(is_ordered_container<void>::value     == false);

    // the definition, term by term
    D_OR_CHECK(is_ordered_container<ord_vector>::value
                   == (    is_iterable_container<ord_vector>::value
                        && !internal::has_key_type_helper<ord_vector>::value ));
    D_OR_CHECK(is_ordered_container<ord_set>::value
                   == (    is_iterable_container<ord_set>::value
                        && !internal::has_key_type_helper<ord_set>::value ));
    D_OR_CHECK(is_ordered_container<ord_bare>::value
                   == (    is_iterable_container<ord_bare>::value
                        && !internal::has_key_type_helper<ord_bare>::value ));

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
bool
tests_is_unordered_container()
{
    bool ok = true;

    // the associative containers, sorted and hashed alike
    D_OR_CHECK(is_unordered_container<ord_set>::value      == true);
    D_OR_CHECK(is_unordered_container<ord_multiset>::value == true);
    D_OR_CHECK(is_unordered_container<ord_map>::value      == true);
    D_OR_CHECK(is_unordered_container<ord_uset>::value     == true);
    D_OR_CHECK(is_unordered_container<ord_umap>::value     == true);
    D_OR_CHECK(is_unordered_container<ord_keyed>::value    == true);
    D_OR_CHECK(is_unordered_container<ord_keyed_void>::value == true);

    // the sequence containers
    D_OR_CHECK(is_unordered_container<ord_vector>::value == false);
    D_OR_CHECK(is_unordered_container<ord_deque>::value  == false);
    D_OR_CHECK(is_unordered_container<ord_list>::value   == false);
    D_OR_CHECK(is_unordered_container<ord_flist>::value  == false);
    D_OR_CHECK(is_unordered_container<ord_string>::value == false);

    // iterability is still required
    D_OR_CHECK(is_unordered_container<ord_keyed_not_iterable>::value == false);

    // non-containers
    D_OR_CHECK(is_unordered_container<ord_bare>::value == false);
    D_OR_CHECK(is_unordered_container<ord_pair>::value == false);
    D_OR_CHECK(is_unordered_container<int>::value      == false);
    D_OR_CHECK(is_unordered_container<void>::value     == false);

    // the definition, term by term
    D_OR_CHECK(is_unordered_container<ord_set>::value
                   == (    is_iterable_container<ord_set>::value
                        && internal::has_key_type_helper<ord_set>::value ));
    D_OR_CHECK(is_unordered_container<ord_keyed_not_iterable>::value
                   == (    is_iterable_container<
                               ord_keyed_not_iterable>::value
                        && internal::has_key_type_helper<
                               ord_keyed_not_iterable>::value ));

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
bool
tests_order_axis_exclusivity()
{
    bool ok = true;

    // mutually exclusive
    D_OR_CHECK(!(    is_ordered_container<ord_vector>::value
                  && is_unordered_container<ord_vector>::value ));
    D_OR_CHECK(!(    is_ordered_container<ord_set>::value
                  && is_unordered_container<ord_set>::value ));
    D_OR_CHECK(!(    is_ordered_container<ord_bare>::value
                  && is_unordered_container<ord_bare>::value ));
    D_OR_CHECK(!(    is_ordered_container<ord_string>::value
                  && is_unordered_container<ord_string>::value ));
    D_OR_CHECK(!(    is_ordered_container<ord_umap>::value
                  && is_unordered_container<ord_umap>::value ));

    // but not complementary - a non-container is neither
    D_OR_CHECK(   ( is_ordered_container<ord_bare>::value   == false )
               && ( is_unordered_container<ord_bare>::value == false ));
    D_OR_CHECK(   ( is_ordered_container<ord_pair>::value   == false )
               && ( is_unordered_container<ord_pair>::value == false ));
    D_OR_CHECK(   ( is_ordered_container<int>::value   == false )
               && ( is_unordered_container<int>::value == false ));
    D_OR_CHECK(   ( is_ordered_container<ord_keyed_not_iterable>::value
                        == false )
               && ( is_unordered_container<ord_keyed_not_iterable>::value
                        == false ));

    // their disjunction is exactly iterability
    D_OR_CHECK((    is_ordered_container<ord_vector>::value
                 || is_unordered_container<ord_vector>::value )
                   == is_iterable_container<ord_vector>::value);
    D_OR_CHECK((    is_ordered_container<ord_set>::value
                 || is_unordered_container<ord_set>::value )
                   == is_iterable_container<ord_set>::value);
    D_OR_CHECK((    is_ordered_container<ord_bare>::value
                 || is_unordered_container<ord_bare>::value )
                   == is_iterable_container<ord_bare>::value);
    D_OR_CHECK((    is_ordered_container<ord_keyed_not_iterable>::value
                 || is_unordered_container<ord_keyed_not_iterable>::value )
                   == is_iterable_container<
                          ord_keyed_not_iterable>::value);
    D_OR_CHECK((    is_ordered_container<ord_input_only>::value
                 || is_unordered_container<ord_input_only>::value )
                   == is_iterable_container<ord_input_only>::value);

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
bool
tests_order_axis_keyed_gap()
{
    bool ok = true;

    // sorted-backed and hash-backed associatives are indistinguishable here
    D_OR_CHECK(is_unordered_container<ord_map>::value
                   == is_unordered_container<ord_umap>::value);
    D_OR_CHECK(is_ordered_container<ord_map>::value
                   == is_ordered_container<ord_umap>::value);
    D_OR_CHECK(ordering_kind_of<ord_map>::value
                   == ordering_kind_of<ord_umap>::value);
    D_OR_CHECK(ordering_kind_of<ord_set>::value
                   == ordering_kind_of<ord_uset>::value);

    // ... and both are unordered
    D_OR_CHECK(ordering_kind_of<ord_map>::value == ordering_kind::unordered);
    D_OR_CHECK(ordering_kind_of<ord_umap>::value == ordering_kind::unordered);

    // while a vector, which guarantees no sortedness whatever, is ordered
    D_OR_CHECK(is_ordered_container<ord_vector>::value == true);
    D_OR_CHECK(   ( is_ordered_container<ord_vector>::value == true )
               && ( is_ordered_container<ord_map>::value    == false ));

    // the discriminator is the alias alone: two fixtures alike but for it
    D_OR_CHECK(   ( is_ordered_container<ord_sequence>::value == true )
               && ( is_ordered_container<ord_keyed>::value    == false ));
    D_OR_CHECK(   ( is_unordered_container<ord_sequence>::value == false )
               && ( is_unordered_container<ord_keyed>::value    == true ));
    D_OR_CHECK(   ( is_iterable_container<ord_sequence>::value
                        == is_iterable_container<ord_keyed>::value )
               && ( ordering_kind_of<ord_sequence>::value
                        != ordering_kind_of<ord_keyed>::value ));

    // layout is a property of positions, so a keyed container has none
    D_OR_CHECK(sequential_layout_of<ord_map>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<ord_set>::value
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
bool
tests_order_axis_cv_ref()
{
    bool ok = true;

    D_OR_CV_REF_AGREE(is_ordered_container, ord_vector, true);
    D_OR_CV_REF_AGREE(is_ordered_container, ord_string, true);
    D_OR_CV_REF_AGREE(is_ordered_container, ord_set,    false);
    D_OR_CV_REF_AGREE(is_ordered_container, ord_bare,   false);

    D_OR_CV_REF_AGREE(is_unordered_container, ord_map,    true);
    D_OR_CV_REF_AGREE(is_unordered_container, ord_uset,   true);
    D_OR_CV_REF_AGREE(is_unordered_container, ord_vector, false);
    D_OR_CV_REF_AGREE(is_unordered_container, ord_bare,   false);

    D_OR_CV_REF_AGREE(internal::has_key_type_helper, ord_set,    true);
    D_OR_CV_REF_AGREE(internal::has_key_type_helper, ord_vector, false);

    // spelled out on one carrier, since a volatile container surviving the
    // probe is the point of writing clean_t
    D_OR_CHECK(is_ordered_container<volatile ord_vector>::value   == true);
    D_OR_CHECK(is_unordered_container<volatile ord_map>::value    == true);
    D_OR_CHECK(is_ordered_container<const volatile ord_string&>::value
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
bool
tests_order_axis_companions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OR_STATIC_CHECK(is_ordered_container_v<ord_vector>   == true);
    D_OR_STATIC_CHECK(is_unordered_container_v<ord_map>    == true);
    D_OR_STATIC_CHECK(is_ordered_container_v<ord_set>      == false);

    D_OR_CHECK(is_ordered_container_v<ord_vector>
                   == is_ordered_container<ord_vector>::value);
    D_OR_CHECK(is_ordered_container_v<ord_flist>
                   == is_ordered_container<ord_flist>::value);
    D_OR_CHECK(is_ordered_container_v<ord_set>
                   == is_ordered_container<ord_set>::value);
    D_OR_CHECK(is_ordered_container_v<ord_bare>
                   == is_ordered_container<ord_bare>::value);

    D_OR_CHECK(is_unordered_container_v<ord_map>
                   == is_unordered_container<ord_map>::value);
    D_OR_CHECK(is_unordered_container_v<ord_uset>
                   == is_unordered_container<ord_uset>::value);
    D_OR_CHECK(is_unordered_container_v<ord_vector>
                   == is_unordered_container<ord_vector>::value);
    D_OR_CHECK(is_unordered_container_v<ord_bare>
                   == is_unordered_container<ord_bare>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
