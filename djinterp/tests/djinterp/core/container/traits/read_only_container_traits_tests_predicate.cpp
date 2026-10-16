/*******************************************************************************
* djinterp [core]                 read_only_container_traits_tests_predicate.cpp
*
*
* path:      /tests/djinterp/core/container/traits/read_only_container_traits_tests_predicate.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_only_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// A.   The predicate
// ===========================================================================
//   is_read_only_container names one point of the access lattice.  It carries
// no detection of its own, so these tests are about which capability it
// selects and how faithfully it reports the baseline's verdict - not about how
// that verdict was reached, which the baseline suite covers.


/*
tests_is_read_only_container
  Verifies that the predicate selects the read_only capability and no other.
  Tests the following:
  - true for a carrier granting observation without mutation
  - false for each of the OTHER three capabilities, so the predicate names one
    corner of the lattice rather than "anything observable" or "anything not
    writable"
  - false in particular for an unclassifiable type: `none` grants no
    observation either, so it is not a weaker read_only
  - false for a type exposing only value-free metadata, which is `none` and not
    read_only - a predicate keyed on "has no mutator" would get this wrong
*/
bool
tests_is_read_only_container()
{
    bool ok = true;

    // true: observation without mutation
    D_RO_CHECK(is_read_only_container<ro_observer>::value    == true);
    D_RO_CHECK(is_read_only_container<ro_begin_only>::value  == true);
    D_RO_CHECK(is_read_only_container<ro_const_index>::value == true);

    // false: the other three capabilities
    D_RO_CHECK(is_read_only_container<ro_sink>::value  == false);
    D_RO_CHECK(is_read_only_container<ro_both>::value  == false);
    D_RO_CHECK(is_read_only_container<ro_inert>::value == false);

    // `none` is not a weaker read_only
    D_RO_CHECK(   ( access_capability_of<ro_inert>::value
                        == access_capability::none )
               && ( is_read_only_container<ro_inert>::value == false ));
    D_RO_CHECK(   ( access_capability_of<ro_metadata_only>::value
                        == access_capability::none )
               && ( is_read_only_container<ro_metadata_only>::value
                        == false ));

    // metadata is not observation
    D_RO_CHECK(is_read_only_container<ro_metadata_only>::value == false);

    // non-containers
    D_RO_CHECK(is_read_only_container<ro_pair>::value == false);
    D_RO_CHECK(is_read_only_container<int>::value     == false);
    D_RO_CHECK(is_read_only_container<void>::value    == false);

    return ok;
}

/*
tests_is_read_only_structural
  Verifies the structural route - a value observer present, every mutator
  absent.
  Tests the following:
  - each shape of const observer is enough on its own, so the predicate
    inherits the baseline's whole read surface rather than some subset
  - an operator[] yielding a CONST reference on every overload is read_only,
    which is the shape a restricting wrapper presents: the value is observable
    and no assignment through the subscript is well-formed
  - adding any mutator moves the carrier off read_only, so the absence of the
    write surface is doing real work
*/
bool
tests_is_read_only_structural()
{
    bool ok = true;

    // each observer shape suffices
    D_RO_CHECK(is_read_only_container<ro_observer>::value   == true);
    D_RO_CHECK(is_read_only_container<ro_begin_only>::value == true);

    // a const-yielding subscript is read_only
    D_RO_CHECK(is_read_only_container<ro_const_index>::value == true);
    D_RO_CHECK(access_capability_of<ro_const_index>::can_read  == true);
    D_RO_CHECK(access_capability_of<ro_const_index>::can_write == false);

    // adding a mutator moves it off read_only
    D_RO_CHECK(   ( is_read_only_container<ro_begin_only>::value == true )
               && ( is_read_only_container<ro_both>::value       == false ));
    D_RO_CHECK(access_capability_of<ro_both>::can_read
                   == access_capability_of<ro_begin_only>::can_read);
    D_RO_CHECK(access_capability_of<ro_both>::can_write
                   != access_capability_of<ro_begin_only>::can_write);

    // removing the observer moves it off too, in the other direction
    D_RO_CHECK(   ( access_capability_of<ro_sink>::can_read  == false )
               && ( access_capability_of<ro_sink>::can_write == true )
               && ( is_read_only_container<ro_sink>::value   == false ));

    return ok;
}

/*
tests_is_read_only_tagged
  Verifies the opt-in route - a stamped `capability` tag, which the baseline
  honours ahead of the structure.
  Tests the following:
  - a carrier stamped read_only reads read_only
  - a carrier that is structurally read_write but stamped read_only reads
    read_only, so the predicate follows the tag rather than the surfaces
  - the mirror case: a carrier that is structurally read_only but stamped
    write_only reads FALSE, so the tag can take a type off this corner as
    readily as it can put one on
  - the two routes are indistinguishable in the verdict, has_capability_tag
    being the only thing that separates them
*/
bool
tests_is_read_only_tagged()
{
    bool ok = true;

    // stamped read_only
    D_RO_CHECK(is_read_only_container<ro_tagged>::value == true);
    D_RO_CHECK(access_capability_of<ro_tagged>::has_capability_tag == true);

    // the tag overrides a read_write structure
    D_RO_CHECK(is_read_only_container<ro_tagged_override>::value == true);
    D_RO_CHECK(access_capability_of<ro_tagged_override>::can_write == false);

    // ... and the structure alone would have said otherwise
    D_RO_CHECK(access_capability_from_bits(
                   internal::has_read_surface<ro_tagged_override>::value,
                   internal::has_write_surface<ro_tagged_override>::value)
                   == access_capability::read_write);

    // the mirror: a tag takes a read_only-looking type off the corner
    D_RO_CHECK(is_read_only_container<ro_tagged_elsewhere>::value == false);
    D_RO_CHECK(access_capability_of<ro_tagged_elsewhere>::value
                   == access_capability::write_only);
    D_RO_CHECK(access_capability_from_bits(
                   internal::has_read_surface<ro_tagged_elsewhere>::value,
                   internal::has_write_surface<ro_tagged_elsewhere>::value)
                   == access_capability::read_only);

    // the routes agree in the verdict and differ only in the flag
    D_RO_CHECK(   ( is_read_only_container<ro_tagged>::value
                        == is_read_only_container<ro_observer>::value )
               && ( access_capability_of<ro_tagged>::has_capability_tag
                        != access_capability_of<
                               ro_observer>::has_capability_tag ));

    return ok;
}

/*
tests_is_read_only_std_types
  Verifies the predicate across the standard library, and records why it finds
  nothing there.

  Every standard container exposes both a const observer and some mutator, so
  the whole standard set reads read_write and NONE of it is read_only.  The
  corner is reachable only through a restricting handle - which is the point of
  the header, the capability being a property of the handle rather than of the
  container - but it does mean a suite pinned only to std:: types would never
  see this predicate return true at all.

  Tests the following:
  - the predicate is false for every standard container
  - it is false for a std::pair, which is `none` rather than read_only
  - a restricting handle over one of those containers IS read_only, so the
    corner is reachable and the negative results above are about the standard
    types and not about the predicate
*/
bool
tests_is_read_only_std_types()
{
    bool ok = true;

    // no standard container is read_only
    D_RO_CHECK(is_read_only_container<ro_vector>::value == false);
    D_RO_CHECK(is_read_only_container<ro_array>::value  == false);
    D_RO_CHECK(is_read_only_container<ro_string>::value == false);
    D_RO_CHECK(is_read_only_container<ro_set>::value    == false);
    D_RO_CHECK(is_read_only_container<ro_map>::value    == false);

    // ... because they are all read_write
    D_RO_CHECK(access_capability_of<ro_vector>::value
                   == access_capability::read_write);
    D_RO_CHECK(access_capability_of<ro_set>::value
                   == access_capability::read_write);

    // a pair is `none`, not read_only
    D_RO_CHECK(   ( access_capability_of<ro_pair>::value
                        == access_capability::none )
               && ( is_read_only_container<ro_pair>::value == false ));

    // but a restricting handle over one of them is read_only
    D_RO_CHECK(is_read_only_container<ro_view>::value == true);
    D_RO_CHECK(   ( is_read_only_container<ro_vector>::value == false )
               && ( is_read_only_container<ro_view>::value   == true ));

    return ok;
}

/*
tests_is_read_only_cv_ref
  Verifies that the predicate honours the clean_t contract.

  The header writes clean_t once itself and then hands the result to
  access_capability_of, which writes it again - so this holds even though only
  one of the two is strictly needed.

  Tests the following:
  - the predicate is identical across all seven cv- and reference-qualified
    spellings, on carriers for which it is true and false alike
  - the volatile spellings in particular, which a decay-based cleaner would
    handle but a remove_cvref-free one might not
*/
bool
tests_is_read_only_cv_ref()
{
    bool ok = true;

    D_RO_CV_REF_AGREE(is_read_only_container, ro_observer,        true);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_begin_only,      true);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_tagged,          true);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_tagged_override, true);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_view,            true);

    D_RO_CV_REF_AGREE(is_read_only_container, ro_sink,         false);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_both,         false);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_inert,        false);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_vector,       false);
    D_RO_CV_REF_AGREE(is_read_only_container, ro_mutable_view, false);

    // a const-qualified read_write container does not become read_only:
    // const-ness of the SPELLING is not a capability restriction
    D_RO_CHECK(is_read_only_container<const ro_vector>::value  == false);
    D_RO_CHECK(is_read_only_container<const ro_vector&>::value == false);

    return ok;
}

/*
tests_is_read_only_companions
  Verifies the `_v` companion the trait_detect engine emits.
  Tests the following:
  - the companion agrees with ::value on carriers of every capability
  - it is usable in a constant expression
  - the trait derives from integral_constant, so it carries the standard
    value_type and type members a metafunction is expected to have
  - the block is gated on variable-template support, since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool
tests_is_read_only_companions()
{
    bool ok = true;

    // the standard metafunction shape
    D_RO_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_read_only_container<ro_observer> >::value);
    D_RO_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_read_only_container<ro_vector> >::value);
    D_RO_STATIC_CHECK(
        std::is_same<is_read_only_container<ro_observer>::value_type,
                     bool>::value);
    D_RO_STATIC_CHECK(
        std::is_same<is_read_only_container<ro_observer>::type,
                     std::true_type>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_RO_STATIC_CHECK(is_read_only_container_v<ro_observer>);
    D_RO_STATIC_CHECK(is_read_only_container_v<ro_vector> == false);

    D_RO_CHECK(is_read_only_container_v<ro_observer>
                   == is_read_only_container<ro_observer>::value);
    D_RO_CHECK(is_read_only_container_v<ro_view>
                   == is_read_only_container<ro_view>::value);
    D_RO_CHECK(is_read_only_container_v<ro_tagged_override>
                   == is_read_only_container<ro_tagged_override>::value);
    D_RO_CHECK(is_read_only_container_v<ro_sink>
                   == is_read_only_container<ro_sink>::value);
    D_RO_CHECK(is_read_only_container_v<ro_both>
                   == is_read_only_container<ro_both>::value);
    D_RO_CHECK(is_read_only_container_v<ro_inert>
                   == is_read_only_container<ro_inert>::value);
    D_RO_CHECK(is_read_only_container_v<const ro_observer&>
                   == is_read_only_container<ro_observer>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
