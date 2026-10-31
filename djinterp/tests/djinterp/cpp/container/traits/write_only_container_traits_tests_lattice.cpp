/*******************************************************************************
* djinterp [cpp]                   write_only_container_traits_tests_lattice.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/write_only_container_traits_tests_lattice.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "write_only_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// B.   Delegation, the metadata exclusion, the lattice, and orthogonality
// ===========================================================================
//   The claims the header makes beyond the predicate's plain reading: that it
// defers entirely to the baseline, that value-free metadata is not observation
// (which is what makes this corner reachable at all), that it names one corner
// of a four-point lattice rather than the complement of another, and that the
// capability it reports belongs to the HANDLE and not to the data behind it.


/*
tests_write_only_delegates
  Verifies that the predicate is exactly the baseline's write_only verdict, and
  carries no second opinion.

  The header states that it adds no detection of its own.  That is worth
  pinning rather than reading off the source, because a delegating predicate
  and an independently-implemented one agree on every ordinary carrier and
  diverge only where the baseline does something surprising - which is exactly
  where a duplicated implementation would drift.

  Tests the following:
  - the predicate equals `capability == write_only` on every carrier the suite
    defines, including the four capabilities and both detection routes
  - it agrees on the carriers where the baseline's two signals CONTRADICT one
    another, which is where an independent implementation would diverge first
  - it agrees on the carrier reached through non-const accessors, a baseline
    reading a re-implementation would be unlikely to reproduce
*/
bool tests_write_only_delegates()
{
    bool ok = true;

    // the predicate is the baseline's verdict, restated
    D_WO_CHECK(is_write_only_container<DWoSink>::value
                   == ( access_capability_of<DWoSink>::value
                            == access_capability::write_only ));
    D_WO_CHECK(is_write_only_container<DWoObserver>::value
                   == ( access_capability_of<DWoObserver>::value
                            == access_capability::write_only ));
    D_WO_CHECK(is_write_only_container<DWoBoth>::value
                   == ( access_capability_of<DWoBoth>::value
                            == access_capability::write_only ));
    D_WO_CHECK(is_write_only_container<DWoInert>::value
                   == ( access_capability_of<DWoInert>::value
                            == access_capability::write_only ));
    D_WO_CHECK(is_write_only_container<DWoVector>::value
                   == ( access_capability_of<DWoVector>::value
                            == access_capability::write_only ));
    D_WO_CHECK(is_write_only_container<DWoSet>::value
                   == ( access_capability_of<DWoSet>::value
                            == access_capability::write_only ));
    D_WO_CHECK(is_write_only_container<DWoPair>::value
                   == ( access_capability_of<DWoPair>::value
                            == access_capability::write_only ));

    // where the two signals contradict one another
    D_WO_CHECK(is_write_only_container<DWoTaggedOverride>::value
                   == ( access_capability_of<DWoTaggedOverride>::value
                            == access_capability::write_only ));
    D_WO_CHECK(is_write_only_container<DWoTaggedElsewhere>::value
                   == ( access_capability_of<DWoTaggedElsewhere>::value
                            == access_capability::write_only ));

    // ... and the two carriers land on opposite verdicts, so the agreement
    // above is not vacuous
    D_WO_CHECK(is_write_only_container<DWoTaggedOverride>::value
                   != is_write_only_container<DWoTaggedElsewhere>::value);

    // the predicate reads the bits the baseline resolved, not the surfaces
    D_WO_CHECK(is_write_only_container<DWoTaggedOverride>::value
                   == (    ( access_capability_of<
                                 DWoTaggedOverride>::can_read == false )
                        && ( access_capability_of<
                                 DWoTaggedOverride>::can_write == true ) ));

    // agreement on the non-const-accessor reading
    D_WO_CHECK(is_write_only_container<DWoNonconstObserver>::value
                   == ( access_capability_of<DWoNonconstObserver>::value
                            == access_capability::write_only ));

    return ok;
}

/*
tests_write_only_metadata_exclusion
  Verifies the design decision this whole corner rests on.

  size() and empty() reveal no ELEMENT VALUE, so neither counts as a read
  surface.  That exclusion is what makes write_only reachable: a realistic sink
  necessarily reports how much it has taken, and were metadata counted as
  observation every such type would classify read_write and this predicate
  would be true of almost nothing.

  Tests the following:
  - a sink WITH metadata and the same sink WITHOUT it classify identically, so
    the metadata contributes nothing to the verdict
  - piling on further value-free accessors - capacity, max_size - changes
    nothing either, so the exclusion is about the KIND of accessor and not
    about how many there are
  - adding a SINGLE value observer to the same sink flips it to read_write, so
    the read surface is otherwise sensitive and the exclusion is doing real
    work rather than the type being unclassifiable anyway
  - metadata alone, with no mutator, is `none` and not write_only - the
    exclusion cuts in both directions
*/
bool tests_write_only_metadata_exclusion()
{
    bool ok = true;

    // metadata contributes nothing
    D_WO_CHECK(is_write_only_container<DWoSink>::value      == true);
    D_WO_CHECK(is_write_only_container<DWoSinkBare>::value == true);
    D_WO_CHECK(access_capability_of<DWoSink>::value
                   == access_capability_of<DWoSinkBare>::value);
    D_WO_CHECK(access_capability_of<DWoSink>::can_read
                   == access_capability_of<DWoSinkBare>::can_read);

    // nor does more of it
    D_WO_CHECK(is_write_only_container<DWoSinkRichMetadata>::value == true);
    D_WO_CHECK(access_capability_of<DWoSinkRichMetadata>::value
                   == access_capability_of<DWoSinkBare>::value);
    D_WO_CHECK(internal::has_read_surface<DWoSinkRichMetadata>::value
                   == false);

    // but a single value observer does
    D_WO_CHECK(is_write_only_container<DWoSinkWithObserver>::value
                   == false);
    D_WO_CHECK(access_capability_of<DWoSinkWithObserver>::value
                   == access_capability::read_write);
    D_WO_CHECK(internal::has_read_surface<DWoSinkWithObserver>::value
                   == true);

    // ... so the read surface is otherwise sensitive
    D_WO_CHECK(access_capability_of<DWoSink>::value
                   != access_capability_of<DWoSinkWithObserver>::value);
    D_WO_CHECK(   ( access_capability_of<DWoSink>::can_write
                        == access_capability_of<
                               DWoSinkWithObserver>::can_write )
               && ( access_capability_of<DWoSink>::can_read
                        != access_capability_of<
                               DWoSinkWithObserver>::can_read ));

    // metadata alone is not a mutator either
    D_WO_CHECK(internal::has_read_surface<DWoMetadataOnly>::value  == false);
    D_WO_CHECK(internal::has_write_surface<DWoMetadataOnly>::value == false);
    D_WO_CHECK(access_capability_of<DWoMetadataOnly>::value
                   == access_capability::none);
    D_WO_CHECK(is_write_only_container<DWoMetadataOnly>::value == false);

    return ok;
}

/*
tests_write_only_lattice_exclusivity
  Verifies that write_only names a corner distinct from the other three, and
  that no type satisfies two at once.
  Tests the following:
  - write_only and read_write are never both true, over every carrier
  - write_only and the read_only verdict are never both true either - checked
    against the baseline classifier rather than the sibling header, so this
    suite stays uncoupled from it
  - each verdict is reached somewhere, so the exclusivity is not the trivial
    consequence of one never firing
  - every carrier lands on exactly one of the four capabilities
*/
bool tests_write_only_lattice_exclusivity()
{
    bool ok = true;

    // never both write_only and read_write
    D_WO_CHECK(!( is_write_only_container<DWoSink>::value
                  && is_read_write_container<DWoSink>::value ));
    D_WO_CHECK(!( is_write_only_container<DWoBoth>::value
                  && is_read_write_container<DWoBoth>::value ));
    D_WO_CHECK(!( is_write_only_container<DWoObserver>::value
                  && is_read_write_container<DWoObserver>::value ));
    D_WO_CHECK(!( is_write_only_container<DWoInert>::value
                  && is_read_write_container<DWoInert>::value ));
    D_WO_CHECK(!( is_write_only_container<DWoVector>::value
                  && is_read_write_container<DWoVector>::value ));
    D_WO_CHECK(!( is_write_only_container<DWoTaggedOverride>::value
                  && is_read_write_container<DWoTaggedOverride>::value ));

    // never both write_only and read_only
    D_WO_CHECK(!( is_write_only_container<DWoSink>::value
                  && ( access_capability_of<DWoSink>::value
                           == access_capability::read_only ) ));
    D_WO_CHECK(!( is_write_only_container<DWoObserver>::value
                  && ( access_capability_of<DWoObserver>::value
                           == access_capability::read_only ) ));
    D_WO_CHECK(!( is_write_only_container<DWoTaggedElsewhere>::value
                  && ( access_capability_of<DWoTaggedElsewhere>::value
                           == access_capability::read_only ) ));

    // each verdict is reached somewhere
    D_WO_CHECK(is_write_only_container<DWoSink>::value == true);
    D_WO_CHECK(is_read_write_container<DWoBoth>::value == true);
    D_WO_CHECK(access_capability_of<DWoObserver>::value
                   == access_capability::read_only);
    D_WO_CHECK(access_capability_of<DWoInert>::value
                   == access_capability::none);

    // exactly one capability per carrier
    D_WO_CHECK(   ( static_cast<int>(
                        is_write_only_container<DWoSink>::value )
                  + static_cast<int>(
                        is_read_write_container<DWoSink>::value )
                  + static_cast<int>(
                        access_capability_of<DWoSink>::value
                            == access_capability::read_only ) )
               == 1);
    D_WO_CHECK(   ( static_cast<int>(
                        is_write_only_container<DWoInert>::value )
                  + static_cast<int>(
                        is_read_write_container<DWoInert>::value )
                  + static_cast<int>(
                        access_capability_of<DWoInert>::value
                            == access_capability::read_only ) )
               == 0);

    return ok;
}

/*
tests_write_only_is_not_complement
  Verifies that write_only is NOT the negation of read_write, nor of read_only
  - the mistake the lattice's shape invites.

  Code written as `if (!is_read_write) treat_as_sink` would be wrong for every
  read_only view and every unclassifiable type; code written as
  `if (!is_read_only) treat_as_sink` would be wrong for every ordinary
  container.  Only the positive test is safe.

  Tests the following:
  - carriers where the negation of read_write and write_only disagree
  - carriers where the negation of the read_only verdict and write_only
    disagree
  - the implication that DOES hold - write_only implies not read_write - and
    the failure of its converse
*/
bool tests_write_only_is_not_complement()
{
    bool ok = true;

    // not read_write, and not write_only either
    D_WO_CHECK(   ( is_read_write_container<DWoObserver>::value == false )
               && ( is_write_only_container<DWoObserver>::value == false ));
    D_WO_CHECK(   ( is_read_write_container<DWoInert>::value == false )
               && ( is_write_only_container<DWoInert>::value == false ));
    D_WO_CHECK(   ( is_read_write_container<DWoMetadataOnly>::value
                        == false )
               && ( is_write_only_container<DWoMetadataOnly>::value
                        == false ));
    D_WO_CHECK(   ( is_read_write_container<DWoPair>::value == false )
               && ( is_write_only_container<DWoPair>::value == false ));

    // stated as the failing equivalence
    D_WO_CHECK(is_write_only_container<DWoObserver>::value
                   != !is_read_write_container<DWoObserver>::value);
    D_WO_CHECK(is_write_only_container<DWoInert>::value
                   != !is_read_write_container<DWoInert>::value);

    // nor is it the negation of read_only
    D_WO_CHECK(   ( ( access_capability_of<DWoVector>::value
                          == access_capability::read_only ) == false )
               && ( is_write_only_container<DWoVector>::value == false ));
    D_WO_CHECK(   ( ( access_capability_of<DWoInert>::value
                          == access_capability::read_only ) == false )
               && ( is_write_only_container<DWoInert>::value == false ));

    // the implication that DOES hold, in one direction only
    D_WO_CHECK(   !is_write_only_container<DWoSink>::value
               ||  !is_read_write_container<DWoSink>::value );
    D_WO_CHECK(   !is_write_only_container<DWoSinkView>::value
               ||  !is_read_write_container<DWoSinkView>::value );
    D_WO_CHECK(   !is_write_only_container<DWoTagged>::value
               ||  !is_read_write_container<DWoTagged>::value );

    // ... and its converse failing
    D_WO_CHECK(   ( !is_read_write_container<DWoObserver>::value == true )
               && ( is_write_only_container<DWoObserver>::value  == false ));

    return ok;
}

/*
tests_write_only_orthogonal_to_mutability
  Verifies the header's ORTHOGONALITY claim: the capability describes the
  HANDLE, not the data behind it.

  DWoSinkView and DWoFullView wrap the same std::vector<int> and differ only
  in what they forward.  If the capability were a property of the underlying
  data the two would classify alike; they do not.  The underlying container is
  itself read_write, so the restriction the sink handle imposes is visible as a
  change in capability.

  Tests the following:
  - two handles over the SAME underlying type classify differently, so the
    capability is not read off the data
  - the underlying container is read_write, and neither handle inherits that by
    default
  - the unrestricted handle DOES reach the same capability as the container,
    so the difference tracks what is forwarded and not merely the presence of a
    wrapper
  - const-qualifying a type spelling is not a handle restriction, which is the
    distinction that makes the overlay a separate idea from qualification
*/
bool tests_write_only_orthogonal_to_mutability()
{
    bool ok = true;

    // same underlying type, different capability
    D_WO_STATIC_CHECK(
        std::is_same<DWoSinkView::underlying,
                     DWoFullView::underlying>::value);
    D_WO_CHECK(is_write_only_container<DWoSinkView>::value == true);
    D_WO_CHECK(is_write_only_container<DWoFullView>::value == false);
    D_WO_CHECK(access_capability_of<DWoSinkView>::value
                   != access_capability_of<DWoFullView>::value);

    // the underlying container is read_write; the sink handle restricts it
    D_WO_CHECK(is_read_write_container<DWoSinkView::underlying>::value
                   == true);
    D_WO_CHECK(is_write_only_container<DWoSinkView::underlying>::value
                   == false);
    D_WO_CHECK(access_capability_of<DWoSinkView::underlying>::value
                   != access_capability_of<DWoSinkView>::value);

    // ... and the unrestricted handle preserves it
    D_WO_CHECK(access_capability_of<DWoFullView>::value
                   == access_capability_of<DWoFullView::underlying>::value);

    // the restriction is what is forwarded, not the wrapping
    D_WO_CHECK(   ( access_capability_of<DWoSinkView>::can_write
                        == access_capability_of<DWoFullView>::can_write )
               && ( access_capability_of<DWoSinkView>::can_read
                        != access_capability_of<DWoFullView>::can_read ));

    // const-qualifying a spelling is not a handle restriction
    D_WO_CHECK(is_write_only_container<const DWoVector>::value == false);
    D_WO_CHECK(access_capability_of<const DWoVector>::value
                   == access_capability_of<DWoVector>::value);
    D_WO_CHECK(access_capability_of<const DWoVector>::value
                   != access_capability_of<DWoSinkView>::value);

    return ok;
}

/*
tests_write_only_tag_agreement
  Verifies that the predicate and the baseline's tag re-emission name the same
  corner - so a type routed by access_tag_of and one tested by this predicate
  cannot disagree.
  Tests the following:
  - access_tag_of yields the write_only tag exactly when the predicate holds
  - the tag a write_only carrier re-emits carries bits matching the
    classification
  - a carrier of each other capability re-emits something else, so the
    correspondence is exact rather than one-directional
*/
bool tests_write_only_tag_agreement()
{
    bool ok = true;

    // the tag is write_only exactly when the predicate holds
    D_WO_CHECK(std::is_same<access_tag_of_t<DWoSink>, write_only>::value
                   == is_write_only_container<DWoSink>::value);
    D_WO_CHECK(std::is_same<access_tag_of_t<DWoSinkView>,
                            write_only>::value
                   == is_write_only_container<DWoSinkView>::value);
    D_WO_CHECK(std::is_same<access_tag_of_t<DWoTaggedOverride>,
                            write_only>::value
                   == is_write_only_container<DWoTaggedOverride>::value);
    D_WO_CHECK(std::is_same<access_tag_of_t<DWoObserver>, write_only>::value
                   == is_write_only_container<DWoObserver>::value);
    D_WO_CHECK(std::is_same<access_tag_of_t<DWoBoth>, write_only>::value
                   == is_write_only_container<DWoBoth>::value);
    D_WO_CHECK(std::is_same<access_tag_of_t<DWoInert>, write_only>::value
                   == is_write_only_container<DWoInert>::value);
    D_WO_CHECK(std::is_same<access_tag_of_t<DWoVector>, write_only>::value
                   == is_write_only_container<DWoVector>::value);

    // the tag's bits match the classification
    D_WO_CHECK(access_tag_of_t<DWoSink>::can_read  == false);
    D_WO_CHECK(access_tag_of_t<DWoSink>::can_write == true);
    D_WO_CHECK(access_tag_of_t<DWoSinkView>::can_read
                   == access_capability_of<DWoSinkView>::can_read);
    D_WO_CHECK(access_tag_of_t<DWoSinkView>::can_write
                   == access_capability_of<DWoSinkView>::can_write);

    // the other capabilities re-emit something else
    D_WO_STATIC_CHECK(
        std::is_same<access_tag_of_t<DWoObserver>, read_only>::value);
    D_WO_STATIC_CHECK(
        std::is_same<access_tag_of_t<DWoBoth>, read_write>::value);
    D_WO_STATIC_CHECK(std::is_same<access_tag_of_t<DWoInert>, void>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
