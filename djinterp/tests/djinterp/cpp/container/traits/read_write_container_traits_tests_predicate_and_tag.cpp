/*******************************************************************************
* djinterp [cpp]         read_write_container_traits_tests_predicate_and_tag.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/read_write_container_traits_tests_predicate_and_tag.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// III. The read_write predicate, and tag re-emission
// ===========================================================================
//   The predicate names this header's own corner of the lattice.  access_tag_of
// then converts the verdict back into a capability TAG TYPE, closing the loop
// with the access/ headers that define the vocabulary - so a classified type
// can be tag-dispatched downstream.


/*
tests_is_read_write_container
  Verifies the predicate that names the unrestricted baseline.
  Tests the following:
  - true for a carrier holding both surfaces, reached structurally
  - true for one stamped read_write, reached through the tag
  - false for each of the OTHER three capabilities, so the predicate names one
    point of the lattice and not "anything classifiable"
  - false for a carrier that is structurally read_write but stamped otherwise,
    the tag being decisive here as everywhere
  - it agrees exactly with the classifier, on every carrier
*/
bool tests_is_read_write_container()
{
    bool ok = true;

    // true, structurally
    D_RW_CHECK(is_read_write_container<DRwBoth>::value  == true);
    D_RW_CHECK(is_read_write_container<DRwVector>::value == true);
    D_RW_CHECK(is_read_write_container<DRwWriteAssignableIndex>::value
                   == true);
    D_RW_CHECK(is_read_write_container<DRwWriteProxyIndex>::value == true);

    // true, through a tag
    D_RW_CHECK(is_read_write_container<DRwTagReadWrite>::value == true);

    // false for the other three capabilities
    D_RW_CHECK(is_read_write_container<DRwObserver>::value    == false);
    D_RW_CHECK(is_read_write_container<DRwSink>::value        == false);
    D_RW_CHECK(is_read_write_container<DRwInert>::value       == false);
    D_RW_CHECK(is_read_write_container<DRwMetadataOnly>::value == false);
    D_RW_CHECK(is_read_write_container<DRwTagReadOnly>::value  == false);
    D_RW_CHECK(is_read_write_container<DRwTagWriteOnly>::value == false);
    D_RW_CHECK(is_read_write_container<DRwTagNoneBits>::value  == false);

    // the tag is decisive
    D_RW_CHECK(   ( internal::has_read_surface<DRwTagOverrides>::value
                        == true )
               && ( internal::has_write_surface<DRwTagOverrides>::value
                        == true )
               && ( is_read_write_container<DRwTagOverrides>::value
                        == false ));

    // agreement with the classifier
    D_RW_CHECK(is_read_write_container<DRwVector>::value
                   == ( access_capability_of<DRwVector>::value
                            == access_capability::read_write ));
    D_RW_CHECK(is_read_write_container<DRwSink>::value
                   == ( access_capability_of<DRwSink>::value
                            == access_capability::read_write ));
    D_RW_CHECK(is_read_write_container<DRwObserver>::value
                   == ( access_capability_of<DRwObserver>::value
                            == access_capability::read_write ));
    D_RW_CHECK(is_read_write_container<DRwTagOverrides>::value
                   == ( access_capability_of<DRwTagOverrides>::value
                            == access_capability::read_write ));

    // non-containers
    D_RW_CHECK(is_read_write_container<DRwPair>::value == false);
    D_RW_CHECK(is_read_write_container<int>::value     == false);
    D_RW_CHECK(is_read_write_container<void>::value    == false);

    return ok;
}

/*
tests_is_read_write_std_types
  Verifies the predicate across the standard library, and records what that
  does and does not demonstrate.

  EVERY standard container reads read_write - sequence, associative, fixed and
  bit-packed alike - because each exposes both a const observer and some
  mutator.  So the standard library exercises exactly ONE of the four
  capabilities, and the other three are reachable only by the wrappers this
  axis exists to describe.  A suite pinned only to std:: types would therefore
  pass with three quarters of the axis unexercised, which is why the fixtures
  carry the rest.

  Tests the following:
  - each standard container reads read_write
  - a std::pair does not, having neither surface
  - the three other capabilities appear nowhere in the standard set, stated
    directly so the gap is recorded rather than assumed
*/
bool tests_is_read_write_std_types()
{
    bool ok = true;

    // every standard container
    D_RW_CHECK(is_read_write_container<DRwVector>::value      == true);
    D_RW_CHECK(is_read_write_container<DRwBoolVector>::value == true);
    D_RW_CHECK(is_read_write_container<DRwArray>::value       == true);
    D_RW_CHECK(is_read_write_container<DRwDeque>::value       == true);
    D_RW_CHECK(is_read_write_container<DRwList>::value        == true);
    D_RW_CHECK(is_read_write_container<DRwFlist>::value       == true);
    D_RW_CHECK(is_read_write_container<DRwString>::value      == true);
    D_RW_CHECK(is_read_write_container<DRwSet>::value         == true);
    D_RW_CHECK(is_read_write_container<DRwMap>::value         == true);
    D_RW_CHECK(is_read_write_container<DRwUmap>::value        == true);

    // a pair is not a container on this axis
    D_RW_CHECK(is_read_write_container<DRwPair>::value == false);

    // the standard set covers exactly one capability
    D_RW_CHECK(access_capability_of<DRwVector>::value
                   == access_capability_of<DRwSet>::value);
    D_RW_CHECK(access_capability_of<DRwArray>::value
                   == access_capability_of<DRwFlist>::value);
    D_RW_CHECK(access_capability_of<DRwBoolVector>::value
                   == access_capability_of<DRwUmap>::value);

    // ... and none of the other three appears in it
    D_RW_CHECK(access_capability_of<DRwString>::value
                   != access_capability::read_only);
    D_RW_CHECK(access_capability_of<DRwString>::value
                   != access_capability::write_only);
    D_RW_CHECK(access_capability_of<DRwString>::value
                   != access_capability::none);

    return ok;
}

/*
tests_access_tag_of
  Verifies the re-emission of a verdict as the corresponding capability TAG
  TYPE - the bridge back to the access/ vocabulary.
  Tests the following:
  - each of the three capabilities yields its own tag type, so all three
    specialisations of the internal dispatch are selected
  - the tag round-trips: the type it names carries the bits the classifier
    resolved
  - a carrier reaching a capability structurally and one reaching it through a
    stamped tag re-emit the SAME tag, the route not surviving into the result
  - the dispatch is keyed on the capability and not on the carrier, shown by
    unrelated carriers of one capability agreeing
*/
bool tests_access_tag_of()
{
    bool ok = true;

    // each capability yields its tag
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwObserver>, read_only>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwSink>, write_only>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwBoth>, read_write>::value);

    // the member spelling agrees with the alias
    D_RW_STATIC_CHECK(
        std::is_same<typename access_tag_of<DRwVector>::type,
                     access_tag_of_t<DRwVector> >::value);

    // the tag carries the bits the classifier resolved
    D_RW_CHECK(access_tag_of_t<DRwObserver>::can_read
                   == access_capability_of<DRwObserver>::can_read);
    D_RW_CHECK(access_tag_of_t<DRwObserver>::can_write
                   == access_capability_of<DRwObserver>::can_write);
    D_RW_CHECK(access_tag_of_t<DRwSink>::can_read
                   == access_capability_of<DRwSink>::can_read);
    D_RW_CHECK(access_tag_of_t<DRwSink>::can_write
                   == access_capability_of<DRwSink>::can_write);
    D_RW_CHECK(access_tag_of_t<DRwVector>::can_read
                   == access_capability_of<DRwVector>::can_read);
    D_RW_CHECK(access_tag_of_t<DRwVector>::can_write
                   == access_capability_of<DRwVector>::can_write);

    // the route does not survive into the result
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwObserver>,
                     access_tag_of_t<DRwTagReadOnly> >::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwSink>,
                     access_tag_of_t<DRwTagWriteOnly> >::value);

    // keyed on the capability, not the carrier
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwVector>,
                     access_tag_of_t<DRwSet> >::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwWriteClear>,
                     access_tag_of_t<DRwSink> >::value);

    return ok;
}

/*
tests_access_tag_of_none_is_void
  Verifies the primary template of the internal dispatch - the branch no
  capability tag exists for.

  There is no "none" tag in the access/ vocabulary, because a type granting
  neither capability is not on the axis at all; the dispatch therefore yields
  void.  That makes void the sentinel a caller must test for before using the
  result as a tag, and it is worth pinning because the primary template is
  selected by DEFAULT - any capability whose specialisation were removed would
  silently fall to void rather than fail to compile.

  Tests the following:
  - an unclassifiable carrier yields void, structurally and through a tag
  - void is yielded for none and for nothing else, so the sentinel is
    unambiguous
  - the three named capabilities all yield a non-void tag
*/
bool tests_access_tag_of_none_is_void()
{
    bool ok = true;

    // none yields void
    D_RW_STATIC_CHECK(std::is_same<access_tag_of_t<DRwInert>, void>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwMetadataOnly>, void>::value);
    D_RW_STATIC_CHECK(std::is_same<access_tag_of_t<int>, void>::value);
    D_RW_STATIC_CHECK(std::is_same<access_tag_of_t<DRwPair>, void>::value);

    // including when reached through a tag that denies both bits
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwTagNoneBits>, void>::value);

    // and for nothing else
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwObserver>, void>::value == false);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwSink>, void>::value == false);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwVector>, void>::value == false);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwTagReadOnly>, void>::value == false);

    // the sentinel tracks the capability exactly
    D_RW_CHECK(std::is_same<access_tag_of_t<DRwInert>, void>::value
                   == ( access_capability_of<DRwInert>::value
                            == access_capability::none ));
    D_RW_CHECK(std::is_same<access_tag_of_t<DRwSink>, void>::value
                   == ( access_capability_of<DRwSink>::value
                            == access_capability::none ));

    return ok;
}

/*
tests_access_tag_round_trip
  Verifies that classification and re-emission are inverse to one another over
  the three named capabilities - so a capability may be carried through the
  type system as a tag and recovered without loss.
  Tests the following:
  - a carrier stamped with a tag re-emits that same tag
  - re-emitting and re-classifying is the identity: a type stamped with the tag
    of a classified type classifies the same way
  - the three tags are pairwise distinct types, so the round trip cannot
    collapse two capabilities onto one
*/
bool tests_access_tag_round_trip()
{
    bool ok = true;

    // a stamped carrier re-emits its own tag
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwTagReadOnly>, read_only>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwTagWriteOnly>, write_only>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwTagReadWrite>, read_write>::value);

    // the tags are pairwise distinct
    D_RW_STATIC_CHECK(std::is_same<read_only, write_only>::value == false);
    D_RW_STATIC_CHECK(std::is_same<read_only, read_write>::value == false);
    D_RW_STATIC_CHECK(std::is_same<write_only, read_write>::value == false);

    // classify -> tag -> bits -> classify is the identity
    D_RW_CHECK(access_capability_from_bits(
                   access_tag_of_t<DRwObserver>::can_read,
                   access_tag_of_t<DRwObserver>::can_write)
                   == access_capability_of<DRwObserver>::value);
    D_RW_CHECK(access_capability_from_bits(
                   access_tag_of_t<DRwSink>::can_read,
                   access_tag_of_t<DRwSink>::can_write)
                   == access_capability_of<DRwSink>::value);
    D_RW_CHECK(access_capability_from_bits(
                   access_tag_of_t<DRwVector>::can_read,
                   access_tag_of_t<DRwVector>::can_write)
                   == access_capability_of<DRwVector>::value);
    D_RW_CHECK(access_capability_from_bits(
                   access_tag_of_t<DRwTagOverrides>::can_read,
                   access_tag_of_t<DRwTagOverrides>::can_write)
                   == access_capability_of<DRwTagOverrides>::value);

    // and the tag re-emitted is the one a carrier stamped with it would report
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwTagOverrides>,
                     access_tag_of_t<DRwObserver> >::value);

    return ok;
}

/*
tests_predicate_cv_ref
  Verifies that the predicate and the tag re-emission honour the clean_t
  contract.
  Tests the following:
  - the predicate is identical across all seven cv- and reference-qualified
    spellings, true and false alike
  - the re-emitted tag TYPE is identical across the same spellings, including
    the void case
  - the volatile spellings in particular
*/
bool tests_predicate_cv_ref()
{
    bool ok = true;

    D_RW_CV_REF_AGREE(is_read_write_container, DRwVector,  true);
    D_RW_CV_REF_AGREE(is_read_write_container, DRwBoth,    true);
    D_RW_CV_REF_AGREE(is_read_write_container, DRwSet,     true);
    D_RW_CV_REF_AGREE(is_read_write_container, DRwObserver, false);
    D_RW_CV_REF_AGREE(is_read_write_container, DRwSink,     false);
    D_RW_CV_REF_AGREE(is_read_write_container, DRwInert,    false);
    D_RW_CV_REF_AGREE(is_read_write_container, DRwTagReadOnly, false);

    // the tag type survives qualification unchanged
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<const DRwVector>, read_write>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<volatile DRwVector>, read_write>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<const volatile DRwVector&>,
                     read_write>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<DRwVector&&>, read_write>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<const DRwObserver&>, read_only>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<volatile DRwSink>, write_only>::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_tag_of_t<const DRwInert&>, void>::value);

    return ok;
}

/*
tests_predicate_companions
  Verifies the `_v` companion the trait_detect engine emits.
  Tests the following:
  - the companion agrees with ::value on carriers of every capability
  - it is usable in a constant expression
  - the trait derives from integral_constant, so it carries the standard
    value_type and type members a metafunction is expected to have
  - the block is gated on variable-template support, since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool tests_predicate_companions()
{
    bool ok = true;

    // the standard metafunction shape
    D_RW_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_read_write_container<DRwVector> >::value);
    D_RW_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_read_write_container<DRwSink> >::value);
    D_RW_STATIC_CHECK(
        std::is_same<is_read_write_container<DRwVector>::value_type,
                     bool>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_RW_STATIC_CHECK(is_read_write_container_v<DRwVector>);
    D_RW_STATIC_CHECK(is_read_write_container_v<DRwSink> == false);

    D_RW_CHECK(is_read_write_container_v<DRwVector>
                   == is_read_write_container<DRwVector>::value);
    D_RW_CHECK(is_read_write_container_v<DRwSet>
                   == is_read_write_container<DRwSet>::value);
    D_RW_CHECK(is_read_write_container_v<DRwObserver>
                   == is_read_write_container<DRwObserver>::value);
    D_RW_CHECK(is_read_write_container_v<DRwSink>
                   == is_read_write_container<DRwSink>::value);
    D_RW_CHECK(is_read_write_container_v<DRwInert>
                   == is_read_write_container<DRwInert>::value);
    D_RW_CHECK(is_read_write_container_v<DRwTagOverrides>
                   == is_read_write_container<DRwTagOverrides>::value);
    D_RW_CHECK(is_read_write_container_v<const DRwVector&>
                   == is_read_write_container<DRwVector>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
