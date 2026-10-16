/*******************************************************************************
* djinterp [core]                write_only_container_traits_tests_predicate.cpp
*
*
* path:      /tests/djinterp/core/container/traits/write_only_container_traits_tests_predicate.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "write_only_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// A.   The predicate
// ===========================================================================
//   is_write_only_container names one point of the access lattice.  It carries
// no detection of its own, so these tests are about which capability it
// selects and how faithfully it reports the baseline's verdict - not about how
// that verdict was reached, which the baseline suite covers.


/*
tests_is_write_only_container
  Verifies that the predicate selects the write_only capability and no other.
  Tests the following:
  - true for a carrier granting mutation without value observation
  - false for each of the OTHER three capabilities, so the predicate names one
    corner of the lattice rather than "anything mutable"
  - false in particular for an unclassifiable type: `none` grants no mutation
    either, so it is not a weaker write_only
  - false for a type exposing only value-free metadata, which is `none` - the
    metadata is neither an observer nor a mutator, so it moves nothing
*/
bool
tests_is_write_only_container()
{
    bool ok = true;

    // true: mutation without observation
    D_WO_CHECK(is_write_only_container<wo_sink>::value      == true);
    D_WO_CHECK(is_write_only_container<wo_sink_bare>::value == true);

    // false: the other three capabilities
    D_WO_CHECK(is_write_only_container<wo_observer>::value == false);
    D_WO_CHECK(is_write_only_container<wo_both>::value     == false);
    D_WO_CHECK(is_write_only_container<wo_inert>::value    == false);

    // `none` is not a weaker write_only
    D_WO_CHECK(   ( access_capability_of<wo_inert>::value
                        == access_capability::none )
               && ( is_write_only_container<wo_inert>::value == false ));
    D_WO_CHECK(   ( access_capability_of<wo_metadata_only>::value
                        == access_capability::none )
               && ( is_write_only_container<wo_metadata_only>::value
                        == false ));

    // non-containers
    D_WO_CHECK(is_write_only_container<wo_pair>::value == false);
    D_WO_CHECK(is_write_only_container<int>::value     == false);
    D_WO_CHECK(is_write_only_container<void>::value    == false);

    return ok;
}

/*
tests_is_write_only_structural
  Verifies the structural route - a mutator present, every value observer
  absent.
  Tests the following:
  - each shape of mutator is enough on its own, so the predicate inherits the
    baseline's whole write surface rather than some subset of it
  - a type whose accessors exist but are NOT const-qualified reads write_only:
    nothing is observable on a const instance, while its non-const operator[]
    yields an assignable lvalue.  This is the least obvious route to the corner
    and the one most likely to surprise
  - adding any value observer moves the carrier off write_only, so the absence
    of the read surface is doing real work
  - removing the mutator moves it off in the other direction, to `none`
*/
bool
tests_is_write_only_structural()
{
    bool ok = true;

    // each mutator shape suffices
    D_WO_CHECK(is_write_only_container<wo_sink_bare>::value       == true);
    D_WO_CHECK(is_write_only_container<wo_emplace_sink>::value    == true);
    D_WO_CHECK(is_write_only_container<wo_push_front_sink>::value == true);
    D_WO_CHECK(is_write_only_container<wo_clear_sink>::value      == true);

    // non-const accessors are not observation, so this is a WRITER
    D_WO_CHECK(is_write_only_container<wo_nonconst_observer>::value == true);
    D_WO_CHECK(access_capability_of<wo_nonconst_observer>::can_read
                   == false);
    D_WO_CHECK(access_capability_of<wo_nonconst_observer>::can_write
                   == true);

    // adding an observer moves it off write_only
    D_WO_CHECK(   ( is_write_only_container<wo_sink>::value == true )
               && ( is_write_only_container<wo_sink_with_observer>::value
                        == false ));
    D_WO_CHECK(access_capability_of<wo_sink_with_observer>::can_write
                   == access_capability_of<wo_sink>::can_write);
    D_WO_CHECK(access_capability_of<wo_sink_with_observer>::can_read
                   != access_capability_of<wo_sink>::can_read);

    // removing the mutator moves it off in the other direction
    D_WO_CHECK(   ( access_capability_of<wo_metadata_only>::can_write
                        == false )
               && ( access_capability_of<wo_metadata_only>::can_read
                        == false )
               && ( is_write_only_container<wo_metadata_only>::value
                        == false ));

    return ok;
}

/*
tests_is_write_only_tagged
  Verifies the opt-in route - a stamped `capability` tag, which the baseline
  honours ahead of the structure.
  Tests the following:
  - a carrier stamped write_only reads write_only
  - a carrier that is structurally read_write but stamped write_only reads
    write_only, so the predicate follows the tag rather than the surfaces
  - the mirror case: a carrier that is structurally write_only but stamped
    read_only reads FALSE, so the tag can take a type off this corner as
    readily as it can put one on
  - the two routes are indistinguishable in the verdict, has_capability_tag
    being the only thing that separates them
*/
bool
tests_is_write_only_tagged()
{
    bool ok = true;

    // stamped write_only
    D_WO_CHECK(is_write_only_container<wo_tagged>::value == true);
    D_WO_CHECK(access_capability_of<wo_tagged>::has_capability_tag == true);

    // the tag overrides a read_write structure
    D_WO_CHECK(is_write_only_container<wo_tagged_override>::value == true);
    D_WO_CHECK(access_capability_of<wo_tagged_override>::can_read == false);

    // ... and the structure alone would have said otherwise
    D_WO_CHECK(access_capability_from_bits(
                   internal::has_read_surface<wo_tagged_override>::value,
                   internal::has_write_surface<wo_tagged_override>::value)
                   == access_capability::read_write);

    // the mirror: a tag takes a sink-looking type off the corner
    D_WO_CHECK(is_write_only_container<wo_tagged_elsewhere>::value == false);
    D_WO_CHECK(access_capability_of<wo_tagged_elsewhere>::value
                   == access_capability::read_only);
    D_WO_CHECK(access_capability_from_bits(
                   internal::has_read_surface<wo_tagged_elsewhere>::value,
                   internal::has_write_surface<wo_tagged_elsewhere>::value)
                   == access_capability::write_only);

    // the routes agree in the verdict and differ only in the flag
    D_WO_CHECK(   ( is_write_only_container<wo_tagged>::value
                        == is_write_only_container<wo_sink>::value )
               && ( access_capability_of<wo_tagged>::has_capability_tag
                        != access_capability_of<
                               wo_sink>::has_capability_tag ));

    return ok;
}

/*
tests_is_write_only_std_types
  Verifies the predicate across the standard library, and records why it finds
  nothing there.

  Every standard container exposes a const observer alongside its mutators, so
  the whole standard set reads read_write and NONE of it is write_only.  A sink
  is not a shape the standard library ships - which is the point of the header,
  the capability being a property of the handle rather than of the container -
  but it does mean a suite pinned only to std:: types would never see this
  predicate return true at all.

  Tests the following:
  - the predicate is false for every standard container
  - it is false for a std::pair, which is `none` rather than write_only
  - a restricting handle over one of those containers IS write_only, so the
    corner is reachable and the negative results above are about the standard
    types and not about the predicate
*/
bool
tests_is_write_only_std_types()
{
    bool ok = true;

    // no standard container is write_only
    D_WO_CHECK(is_write_only_container<wo_vector>::value == false);
    D_WO_CHECK(is_write_only_container<wo_array>::value  == false);
    D_WO_CHECK(is_write_only_container<wo_string>::value == false);
    D_WO_CHECK(is_write_only_container<wo_set>::value    == false);
    D_WO_CHECK(is_write_only_container<wo_map>::value    == false);

    // ... because they are all read_write
    D_WO_CHECK(access_capability_of<wo_vector>::value
                   == access_capability::read_write);
    D_WO_CHECK(access_capability_of<wo_map>::value
                   == access_capability::read_write);

    // a pair is `none`, not write_only
    D_WO_CHECK(   ( access_capability_of<wo_pair>::value
                        == access_capability::none )
               && ( is_write_only_container<wo_pair>::value == false ));

    // but a restricting handle over one of them is write_only
    D_WO_CHECK(is_write_only_container<wo_sink_view>::value == true);
    D_WO_CHECK(   ( is_write_only_container<wo_vector>::value    == false )
               && ( is_write_only_container<wo_sink_view>::value == true ));

    return ok;
}

/*
tests_is_write_only_cv_ref
  Verifies that the predicate honours the clean_t contract.

  The header writes clean_t once itself and then hands the result to
  access_capability_of, which writes it again - so this holds even though only
  one of the two is strictly needed.

  Tests the following:
  - the predicate is identical across all seven cv- and reference-qualified
    spellings, on carriers for which it is true and false alike
  - the volatile spellings in particular
  - a const-qualified spelling of a sink stays write_only: const-ness of a type
    SPELLING is not an access restriction, which is what makes the overlay a
    separate idea from qualification
*/
bool
tests_is_write_only_cv_ref()
{
    bool ok = true;

    D_WO_CV_REF_AGREE(is_write_only_container, wo_sink,             true);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_sink_bare,        true);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_clear_sink,       true);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_tagged,           true);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_tagged_override,  true);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_sink_view,        true);

    D_WO_CV_REF_AGREE(is_write_only_container, wo_observer,  false);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_both,      false);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_inert,     false);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_vector,    false);
    D_WO_CV_REF_AGREE(is_write_only_container, wo_full_view, false);

    // qualification is not restriction, in either direction
    D_WO_CHECK(is_write_only_container<const wo_sink>::value   == true);
    D_WO_CHECK(is_write_only_container<const wo_vector>::value == false);

    return ok;
}

/*
tests_is_write_only_companions
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
tests_is_write_only_companions()
{
    bool ok = true;

    // the standard metafunction shape
    D_WO_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_write_only_container<wo_sink> >::value);
    D_WO_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_write_only_container<wo_vector> >::value);
    D_WO_STATIC_CHECK(
        std::is_same<is_write_only_container<wo_sink>::value_type,
                     bool>::value);
    D_WO_STATIC_CHECK(
        std::is_same<is_write_only_container<wo_sink>::type,
                     std::true_type>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_WO_STATIC_CHECK(is_write_only_container_v<wo_sink>);
    D_WO_STATIC_CHECK(is_write_only_container_v<wo_vector> == false);

    D_WO_CHECK(is_write_only_container_v<wo_sink>
                   == is_write_only_container<wo_sink>::value);
    D_WO_CHECK(is_write_only_container_v<wo_sink_view>
                   == is_write_only_container<wo_sink_view>::value);
    D_WO_CHECK(is_write_only_container_v<wo_tagged_override>
                   == is_write_only_container<wo_tagged_override>::value);
    D_WO_CHECK(is_write_only_container_v<wo_observer>
                   == is_write_only_container<wo_observer>::value);
    D_WO_CHECK(is_write_only_container_v<wo_both>
                   == is_write_only_container<wo_both>::value);
    D_WO_CHECK(is_write_only_container_v<wo_inert>
                   == is_write_only_container<wo_inert>::value);
    D_WO_CHECK(is_write_only_container_v<const wo_sink&>
                   == is_write_only_container<wo_sink>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
