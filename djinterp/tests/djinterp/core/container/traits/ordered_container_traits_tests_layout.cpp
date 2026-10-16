/*******************************************************************************
* djinterp [core]                      ordered_container_traits_tests_layout.cpp
*
*
* path:      /tests/djinterp/core/container/traits/ordered_container_traits_tests_layout.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "ordered_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. The sequential layout
// ===========================================================================
//   A REPRESENTATION classification, offered for code that must dispatch on
// storage shape - the Order axis proper abstracts backing away.  Only an
// ordered container has a layout; within one, the checks run
// strongest-signal-first, so a text buffer is not mistaken for a bare array
// nor a deque for a list.


/*
tests_has_c_str_helper
  Verifies the text-buffer tell - a const-callable c_str() - which is the only
  signal in this header that is a member EXPRESSION rather than a nested alias.
  Tests the following:
  - std::string carries it; the sequence containers do not
  - std::string_view does NOT, which is the sharp case: it is contiguous and
    character-valued but offers no null-terminated accessor, so it reads
    array_like rather than string_like
  - a c_str() returning VOID counts, the helper naming the expression's type
    and never constraining it - these are structural probes, not semantic ones
  - a non-const, a private, and an argument-taking c_str() are all missed, so
    the detection is narrower than "declares a member of that name"
*/
bool
tests_has_c_str_helper()
{
    bool ok = true;

    // present
    D_OR_CHECK(internal::has_c_str_helper<ord_string>::value         == true);
    D_OR_CHECK(internal::has_c_str_helper<ord_text_forward>::value   == true);
    D_OR_CHECK(internal::has_c_str_helper<ord_text_contiguous>::value
                   == true);
    D_OR_CHECK(internal::has_c_str_helper<ord_text_keyed>::value     == true);

    // a void return still counts
    D_OR_CHECK(internal::has_c_str_helper<ord_text_void>::value == true);

    // absent
    D_OR_CHECK(internal::has_c_str_helper<ord_vector>::value == false);
    D_OR_CHECK(internal::has_c_str_helper<ord_deque>::value  == false);
    D_OR_CHECK(internal::has_c_str_helper<ord_list>::value   == false);
    D_OR_CHECK(internal::has_c_str_helper<ord_set>::value    == false);
    D_OR_CHECK(internal::has_c_str_helper<ord_bare>::value   == false);
    D_OR_CHECK(internal::has_c_str_helper<int>::value        == false);
    D_OR_CHECK(internal::has_c_str_helper<void>::value       == false);

    // narrower than "declares a member of that name"
    D_OR_CHECK(internal::has_c_str_helper<ord_text_nonconst>::value == false);
    D_OR_CHECK(internal::has_c_str_helper<ord_text_private>::value  == false);
    D_OR_CHECK(internal::has_c_str_helper<ord_text_arg>::value      == false);

    // ... and those types fall through to their iterator category
    D_OR_CHECK(sequential_layout_of<ord_text_nonconst>::value
                   == sequential_layout::array_like);
    D_OR_CHECK(sequential_layout_of<ord_text_private>::value
                   == sequential_layout::array_like);

    return ok;
}

/*
tests_sequential_layout_enum
  Verifies the enumeration itself.
  Tests the following:
  - it is a SCOPED enumeration
  - the six enumerators carry the implicit ordinals 0..5 in declaration order
    and are pairwise distinct
  - a value-initialised layout is none, the safe default
  - a layout is usable as a non-type template argument
  - the underlying type is deliberately not asserted, this enumeration fixing
    none; the consequence pinned instead is the 0..7 value range
*/
bool
tests_sequential_layout_enum()
{
    bool ok = true;

    // scoped
    D_OR_STATIC_CHECK(std::is_enum<sequential_layout>::value == true);
    D_OR_STATIC_CHECK(
        std::is_convertible<sequential_layout, int>::value == false);

    // the implicit ordinals
    D_OR_CHECK(static_cast<int>(sequential_layout::none)              == 0);
    D_OR_CHECK(static_cast<int>(sequential_layout::array_like)        == 1);
    D_OR_CHECK(static_cast<int>(sequential_layout::deque_like)        == 2);
    D_OR_CHECK(static_cast<int>(sequential_layout::list_like)         == 3);
    D_OR_CHECK(static_cast<int>(sequential_layout::forward_list_like) == 4);
    D_OR_CHECK(static_cast<int>(sequential_layout::string_like)       == 5);

    // a value-initialised layout claims nothing
    D_OR_CHECK(sequential_layout{} == sequential_layout::none);

    // pairwise distinct, over the whole enum
    for (int i = 0; i < ord_layout_count; ++i)
    {
        for (int j = 0; j < ord_layout_count; ++j)
        {
            const bool same = ( ord_layout_at(i) == ord_layout_at(j) );

            D_OR_CHECK(same == (i == j));
        }
    }

    // usable as a non-type template argument
    D_OR_STATIC_CHECK(
        std::integral_constant<sequential_layout,
                               sequential_layout::list_like>::value
            == sequential_layout::list_like);

    return ok;
}

/*
tests_sequential_layout_name
  Verifies the diagnostic spelling of each layout.
  Tests the following:
  - each of the six layouts spells its own name
  - the spellings are pairwise distinct
  - the function is constexpr
*/
bool
tests_sequential_layout_name()
{
    bool ok = true;

    // each layout's spelling
    D_OR_CHECK(ord_layout_name_is(sequential_layout::none, "none"));
    D_OR_CHECK(ord_layout_name_is(sequential_layout::array_like,
                                  "array_like"));
    D_OR_CHECK(ord_layout_name_is(sequential_layout::deque_like,
                                  "deque_like"));
    D_OR_CHECK(ord_layout_name_is(sequential_layout::list_like, "list_like"));
    D_OR_CHECK(ord_layout_name_is(sequential_layout::forward_list_like,
                                  "forward_list_like"));
    D_OR_CHECK(ord_layout_name_is(sequential_layout::string_like,
                                  "string_like"));

    // pairwise distinct spellings
    for (int i = 0; i < ord_layout_count; ++i)
    {
        for (int j = 0; j < ord_layout_count; ++j)
        {
            const bool same =
                ( std::strcmp(sequential_layout_name(ord_layout_at(i)),
                              sequential_layout_name(ord_layout_at(j)))
                      == 0 );

            D_OR_CHECK(same == (i == j));
        }
    }

    // constexpr
    D_OR_STATIC_CHECK(
        sequential_layout_name(sequential_layout::none)[0] == 'n');

    return ok;
}

/*
tests_sequential_layout_of_std_types
  Verifies the layout each standard container receives - where the
  classification meets real interfaces.
  Tests the following:
  - the five shapes the enum names each get their intended carrier: vector and
    array are array_like, deque is deque_like, list is list_like,
    forward_list is forward_list_like, string is string_like
  - std::vector<bool> is deque_like, NOT array_like.  Contiguity is inferred
    from a data() accessor, and the bit-packed specialisation offers none, so a
    vector reads as a deque.  The classification is honest about what it can
    see - there is no contiguous buffer of bools to hand out - but the reading
    surprises, so it is pinned rather than left to be discovered
  - the same reasoning applies to any random-access type without data(), shown
    on a fixture, so the behaviour is a rule and not a quirk of one
    specialisation
  - a nested sequence container is classified by its OWN storage, not its
    elements'
*/
bool
tests_sequential_layout_of_std_types()
{
    bool ok = true;

    // the intended carriers
    D_OR_CHECK(sequential_layout_of<ord_vector>::value
                   == sequential_layout::array_like);
    D_OR_CHECK(sequential_layout_of<ord_array>::value
                   == sequential_layout::array_like);
    D_OR_CHECK(sequential_layout_of<ord_deque>::value
                   == sequential_layout::deque_like);
    D_OR_CHECK(sequential_layout_of<ord_list>::value
                   == sequential_layout::list_like);
    D_OR_CHECK(sequential_layout_of<ord_flist>::value
                   == sequential_layout::forward_list_like);
    D_OR_CHECK(sequential_layout_of<ord_string>::value
                   == sequential_layout::string_like);

    // the bit-packed vector reads as a deque
    D_OR_CHECK(sequential_layout_of<ord_bool_vector>::value
                   == sequential_layout::deque_like);
    D_OR_CHECK(sequential_layout_of<ord_bool_vector>::value
                   != sequential_layout_of<ord_vector>::value);

    // ... because contiguity is inferred from data(), on a rule not a quirk
    D_OR_CHECK(   ( is_random_access_iterable<ord_bool_vector>::value
                        == true )
               && ( is_contiguous_iterable<ord_bool_vector>::value
                        == false ));
    D_OR_CHECK(   ( is_random_access_iterable<ord_random_no_data>::value
                        == true )
               && ( is_contiguous_iterable<ord_random_no_data>::value
                        == false )
               && ( sequential_layout_of<ord_random_no_data>::value
                        == sequential_layout::deque_like ));

    // a nested container is classified by its own storage
    D_OR_CHECK(sequential_layout_of<ord_nested>::value
                   == sequential_layout::array_like);

    return ok;
}

/*
tests_sequential_layout_of_precedence
  Verifies the ORDER of the cascade.  The checks run strongest-signal-first, so
  each of these fixtures satisfies more than one branch and pins the winner.
  Tests the following:
  - c_str() is tested before ANY iterator category, so a text buffer that is
    also contiguous is string_like, not array_like
  - and a text buffer that is only forward-traversable is string_like all the
    same, which is what "before any category" means
  - contiguity beats bare random access, so a vector is not read as a deque
  - random access beats bidirectionality, so a deque is not read as a list
  - bidirectionality beats forward, so a list is not read as a forward_list
  - the iterator categories nest, so the ordering is what does the work: every
    contiguous type is also random-access, bidirectional and forward
*/
bool
tests_sequential_layout_of_precedence()
{
    bool ok = true;

    // c_str() before any category
    D_OR_CHECK(   ( is_contiguous_iterable<ord_text_contiguous>::value
                        == true )
               && ( sequential_layout_of<ord_text_contiguous>::value
                        == sequential_layout::string_like ));
    D_OR_CHECK(   ( is_random_access_iterable<ord_text_forward>::value
                        == true )
               && ( sequential_layout_of<ord_text_forward>::value
                        == sequential_layout::string_like ));

    // contiguity before bare random access
    D_OR_CHECK(   ( is_contiguous_iterable<ord_vector>::value    == true )
               && ( is_random_access_iterable<ord_vector>::value == true )
               && ( sequential_layout_of<ord_vector>::value
                        == sequential_layout::array_like ));

    // random access before bidirectionality
    D_OR_CHECK(   ( is_contiguous_iterable<ord_deque>::value      == false )
               && ( is_random_access_iterable<ord_deque>::value   == true )
               && ( is_bidirectional_iterable<ord_deque>::value   == true )
               && ( sequential_layout_of<ord_deque>::value
                        == sequential_layout::deque_like ));

    // bidirectionality before forward
    D_OR_CHECK(   ( is_random_access_iterable<ord_list>::value  == false )
               && ( is_bidirectional_iterable<ord_list>::value  == true )
               && ( is_forward_iterable<ord_list>::value        == true )
               && ( sequential_layout_of<ord_list>::value
                        == sequential_layout::list_like ));

    // and forward last
    D_OR_CHECK(   ( is_bidirectional_iterable<ord_flist>::value == false )
               && ( is_forward_iterable<ord_flist>::value       == true )
               && ( sequential_layout_of<ord_flist>::value
                        == sequential_layout::forward_list_like ));

    // the categories nest, which is why the ordering carries the meaning
    D_OR_CHECK(   ( is_contiguous_iterable<ord_vector>::value    == true )
               && ( is_bidirectional_iterable<ord_vector>::value == true )
               && ( is_forward_iterable<ord_vector>::value       == true ));

    return ok;
}

/*
tests_sequential_layout_of_gate
  Verifies that only an ORDERED container has a layout.  Layout describes how
  positions are laid out, so a container without positional identity has none
  to describe - and the gate is checked before every signal, not after.
  Tests the following:
  - every associative container reads none, tree-backed and hash-backed alike
  - a non-container reads none
  - the gate runs BEFORE the c_str() test, shown on a keyed fixture that does
    carry c_str(): it is detected by the helper and still yields none
  - the gate is exactly is_ordered_container, re-derived on several carriers
*/
bool
tests_sequential_layout_of_gate()
{
    bool ok = true;

    // the associative containers have no layout
    D_OR_CHECK(sequential_layout_of<ord_set>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<ord_multiset>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<ord_map>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<ord_uset>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<ord_umap>::value
                   == sequential_layout::none);

    // nor do non-containers
    D_OR_CHECK(sequential_layout_of<ord_bare>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<ord_pair>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<ord_begin_only>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<int>::value
                   == sequential_layout::none);
    D_OR_CHECK(sequential_layout_of<void>::value
                   == sequential_layout::none);

    // the gate precedes the c_str() test
    D_OR_CHECK(   ( internal::has_c_str_helper<ord_text_keyed>::value == true )
               && ( is_ordered_container<ord_text_keyed>::value       == false )
               && ( sequential_layout_of<ord_text_keyed>::value
                        == sequential_layout::none ));

    // the gate is exactly is_ordered_container
    D_OR_CHECK(( sequential_layout_of<ord_vector>::value
                     != sequential_layout::none )
                   == is_ordered_container<ord_vector>::value);
    D_OR_CHECK(( sequential_layout_of<ord_map>::value
                     != sequential_layout::none )
                   == is_ordered_container<ord_map>::value);
    D_OR_CHECK(( sequential_layout_of<ord_bare>::value
                     != sequential_layout::none )
                   == is_ordered_container<ord_bare>::value);

    return ok;
}

/*
tests_sequential_layout_of_fallthrough
  Verifies the LAST branch of the cascade, which is otherwise unreachable by
  any standard container.

  An ordered container whose traversal is weaker than forward - an input-only
  iteration - satisfies the gate but no shape signal, so the cascade runs off
  its end and yields none.  That makes none an ambiguous answer: it means
  either "not an ordered container" or "an ordered container of no recognised
  shape", and the two are distinguished only by asking is_ordered_container
  separately.  Code dispatching on layout should therefore treat none as
  "unknown", not as "not a container".

  Tests the following:
  - an input-only ordered container reads none
  - it really is ordered, so none here is NOT the gate firing
  - it satisfies no category signal, which is what sends it to the last branch
  - the ambiguity is real: this fixture and a plain non-container give the same
    layout but differ on the gate
*/
bool
tests_sequential_layout_of_fallthrough()
{
    bool ok = true;

    // the last branch
    D_OR_CHECK(sequential_layout_of<ord_input_only>::value
                   == sequential_layout::none);

    // and it is NOT the gate firing
    D_OR_CHECK(is_ordered_container<ord_input_only>::value == true);
    D_OR_CHECK(ordering_kind_of<ord_input_only>::value
                   == ordering_kind::ordered);

    // no signal is satisfied
    D_OR_CHECK(internal::has_c_str_helper<ord_input_only>::value      == false);
    D_OR_CHECK(is_contiguous_iterable<ord_input_only>::value          == false);
    D_OR_CHECK(is_random_access_iterable<ord_input_only>::value       == false);
    D_OR_CHECK(is_bidirectional_iterable<ord_input_only>::value       == false);
    D_OR_CHECK(is_forward_iterable<ord_input_only>::value             == false);

    // the ambiguity, spelled out
    D_OR_CHECK(sequential_layout_of<ord_input_only>::value
                   == sequential_layout_of<ord_bare>::value);
    D_OR_CHECK(is_ordered_container<ord_input_only>::value
                   != is_ordered_container<ord_bare>::value);

    return ok;
}

/*
tests_sequential_layout_out_of_range
  Verifies what the spelling function does with a value that names no
  enumerator.

  sequential_layout_name is a chained ternary that tests five of the six
  layouts and FALLS THROUGH to "string_like" - the same shape ordering_kind_name
  has, and the same shape the sibling header's sortedness_name has.  So an
  unnamed value reads as the most specific layout rather than as none.

  The value range being probed is 0..7, since the enumeration fixes no
  underlying type and five is its largest enumerator, so a three-bit field
  bounds it.  6 and 7 are the valid values that name nothing.

  Tests the following:
  - both unnamed values in range read as "string_like"
  - they are therefore indistinguishable by name from string_like itself
  - the named layouts remain pairwise distinct by name
  - a value-initialised layout is none, so default construction is safe
*/
bool
tests_sequential_layout_out_of_range()
{
    bool ok = true;

    const sequential_layout six   = static_cast<sequential_layout>(6);
    const sequential_layout seven = static_cast<sequential_layout>(7);

    // the fall-through answers the most specific layout
    D_OR_CHECK(ord_layout_name_is(six,   "string_like"));
    D_OR_CHECK(ord_layout_name_is(seven, "string_like"));
    D_OR_CHECK(std::strcmp(
                   sequential_layout_name(six),
                   sequential_layout_name(sequential_layout::string_like))
                   == 0);

    // though the values themselves differ
    D_OR_CHECK(six != sequential_layout::string_like);
    D_OR_CHECK(six != seven);
    D_OR_CHECK(static_cast<int>(six) == 6);

    // the named layouts stay distinct
    for (int i = 0; i < ord_layout_count; ++i)
    {
        for (int j = 0; j < ord_layout_count; ++j)
        {
            const bool same =
                ( std::strcmp(sequential_layout_name(ord_layout_at(i)),
                              sequential_layout_name(ord_layout_at(j)))
                      == 0 );

            D_OR_CHECK(same == ( ord_layout_at(i) == ord_layout_at(j) ));
        }
    }

    // the default remains none
    D_OR_CHECK(sequential_layout{} == sequential_layout::none);
    D_OR_CHECK(ord_layout_name_is(sequential_layout{}, "none"));

    return ok;
}

/*
tests_sequential_layout_cv_ref
  Verifies that the classification honours the clean_t contract.
  Tests the following:
  - the layout is identical across all seven cv- and reference-qualified
    spellings, on a carrier for each shape
  - the volatile spellings in particular
*/
bool
tests_sequential_layout_cv_ref()
{
    bool ok = true;

    D_OR_CV_REF_AGREE(sequential_layout_of, ord_vector,
                      sequential_layout::array_like);
    D_OR_CV_REF_AGREE(sequential_layout_of, ord_deque,
                      sequential_layout::deque_like);
    D_OR_CV_REF_AGREE(sequential_layout_of, ord_list,
                      sequential_layout::list_like);
    D_OR_CV_REF_AGREE(sequential_layout_of, ord_flist,
                      sequential_layout::forward_list_like);
    D_OR_CV_REF_AGREE(sequential_layout_of, ord_string,
                      sequential_layout::string_like);
    D_OR_CV_REF_AGREE(sequential_layout_of, ord_map,
                      sequential_layout::none);
    D_OR_CV_REF_AGREE(sequential_layout_of, ord_bare,
                      sequential_layout::none);

    D_OR_CV_REF_AGREE(internal::has_c_str_helper, ord_string, true);
    D_OR_CV_REF_AGREE(internal::has_c_str_helper, ord_vector, false);

    // spelled out on one carrier
    D_OR_CHECK(sequential_layout_of<volatile ord_string>::value
                   == sequential_layout::string_like);
    D_OR_CHECK(sequential_layout_of<const volatile ord_list&>::value
                   == sequential_layout::list_like);

    return ok;
}

/*
tests_sequential_layout_companions
  Verifies the `_t` alias and the `_v` companion.
  Tests the following:
  - the `_t` alias is the integral_constant naming the layout
  - the `_v` companion agrees with ::value across the shapes
  - both are usable in a constant expression
  - the `_v` half is gated on variable-template support
*/
bool
tests_sequential_layout_companions()
{
    bool ok = true;

    // the _t alias is the integral_constant, exactly
    D_OR_STATIC_CHECK(
        std::is_same<sequential_layout_of_t<ord_vector>,
                     std::integral_constant<
                         sequential_layout,
                         sequential_layout::array_like> >::value);
    D_OR_STATIC_CHECK(
        std::is_same<sequential_layout_of_t<ord_string>,
                     std::integral_constant<
                         sequential_layout,
                         sequential_layout::string_like> >::value);
    D_OR_STATIC_CHECK(
        std::is_same<sequential_layout_of_t<ord_map>,
                     std::integral_constant<
                         sequential_layout,
                         sequential_layout::none> >::value);

    // ... and carries the value the trait reports
    D_OR_CHECK(sequential_layout_of_t<ord_list>::value
                   == sequential_layout_of<ord_list>::value);
    D_OR_CHECK(sequential_layout_of_t<ord_bool_vector>::value
                   == sequential_layout_of<ord_bool_vector>::value);

    // usable in a constant expression
    D_OR_STATIC_CHECK(sequential_layout_of<ord_flist>::value
                          == sequential_layout::forward_list_like);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_OR_STATIC_CHECK(sequential_layout_of_v<ord_string>
                          == sequential_layout::string_like);

    D_OR_CHECK(sequential_layout_of_v<ord_vector>
                   == sequential_layout_of<ord_vector>::value);
    D_OR_CHECK(sequential_layout_of_v<ord_deque>
                   == sequential_layout_of<ord_deque>::value);
    D_OR_CHECK(sequential_layout_of_v<ord_list>
                   == sequential_layout_of<ord_list>::value);
    D_OR_CHECK(sequential_layout_of_v<ord_flist>
                   == sequential_layout_of<ord_flist>::value);
    D_OR_CHECK(sequential_layout_of_v<ord_map>
                   == sequential_layout_of<ord_map>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
