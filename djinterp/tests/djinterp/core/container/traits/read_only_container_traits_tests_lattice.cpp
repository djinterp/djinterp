/*******************************************************************************
* djinterp [core]                   read_only_container_traits_tests_lattice.cpp
*
*
* path:      /tests/djinterp/core/container/traits/read_only_container_traits_tests_lattice.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_only_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// B.   Delegation, the lattice, and orthogonality
// ===========================================================================
//   The three claims the header makes beyond the predicate's plain reading:
// that it defers entirely to the baseline, that it names one corner of a
// four-point lattice rather than the complement of another, and that the
// capability it reports belongs to the HANDLE and not to the data behind it.


/*
tests_read_only_delegates
  Verifies that the predicate is exactly the baseline's read_only verdict, and
  carries no second opinion.

  The header states that it adds no detection of its own.  That is worth
  pinning rather than reading off the source, because a delegating predicate
  and an independently-implemented one agree on every ordinary carrier and
  diverge only where the baseline does something surprising - which is exactly
  where a duplicated implementation would drift.

  Tests the following:
  - the predicate equals `capability == read_only` on every carrier the suite
    defines, including the four capabilities and both detection routes
  - it agrees on the carriers where the baseline's two signals CONTRADICT one
    another, which is where an independent implementation would diverge first
  - it agrees on the carrier whose write surface turns on a value_type alias
    rather than on any mutator, a baseline quirk a re-implementation would be
    unlikely to reproduce
*/
bool
tests_read_only_delegates()
{
    bool ok = true;

    // the predicate is the baseline's verdict, restated
    D_RO_CHECK(is_read_only_container<ro_observer>::value
                   == ( access_capability_of<ro_observer>::value
                            == access_capability::read_only ));
    D_RO_CHECK(is_read_only_container<ro_sink>::value
                   == ( access_capability_of<ro_sink>::value
                            == access_capability::read_only ));
    D_RO_CHECK(is_read_only_container<ro_both>::value
                   == ( access_capability_of<ro_both>::value
                            == access_capability::read_only ));
    D_RO_CHECK(is_read_only_container<ro_inert>::value
                   == ( access_capability_of<ro_inert>::value
                            == access_capability::read_only ));
    D_RO_CHECK(is_read_only_container<ro_vector>::value
                   == ( access_capability_of<ro_vector>::value
                            == access_capability::read_only ));
    D_RO_CHECK(is_read_only_container<ro_set>::value
                   == ( access_capability_of<ro_set>::value
                            == access_capability::read_only ));
    D_RO_CHECK(is_read_only_container<ro_pair>::value
                   == ( access_capability_of<ro_pair>::value
                            == access_capability::read_only ));

    // where the two signals contradict one another
    D_RO_CHECK(is_read_only_container<ro_tagged_override>::value
                   == ( access_capability_of<ro_tagged_override>::value
                            == access_capability::read_only ));
    D_RO_CHECK(is_read_only_container<ro_tagged_elsewhere>::value
                   == ( access_capability_of<ro_tagged_elsewhere>::value
                            == access_capability::read_only ));

    // ... and the two carriers land on opposite verdicts, so the agreement
    // above is not vacuous
    D_RO_CHECK(is_read_only_container<ro_tagged_override>::value
                   != is_read_only_container<ro_tagged_elsewhere>::value);

    // the predicate reads the bits the baseline resolved, not the surfaces
    D_RO_CHECK(is_read_only_container<ro_tagged_override>::value
                   == (    ( access_capability_of<
                                 ro_tagged_override>::can_read == true )
                        && ( access_capability_of<
                                 ro_tagged_override>::can_write == false ) ));

    // agreement holds on a carrier whose classification turns on an alias
    D_RO_CHECK(is_read_only_container<ro_metadata_only>::value
                   == ( access_capability_of<ro_metadata_only>::value
                            == access_capability::read_only ));

    return ok;
}

/*
tests_read_only_lattice_exclusivity
  Verifies that read_only and read_write name DIFFERENT corners, and that no
  type satisfies both.
  Tests the following:
  - the two predicates are never both true, over every carrier the suite
    defines
  - each is true somewhere, so the exclusivity is not the trivial consequence
    of one never firing
  - a type satisfying neither exists in two distinct flavours - write_only and
    none - which is what makes the lattice four-pointed rather than binary
*/
bool
tests_read_only_lattice_exclusivity()
{
    bool ok = true;

    // never both
    D_RO_CHECK(!( is_read_only_container<ro_observer>::value
                  && is_read_write_container<ro_observer>::value ));
    D_RO_CHECK(!( is_read_only_container<ro_both>::value
                  && is_read_write_container<ro_both>::value ));
    D_RO_CHECK(!( is_read_only_container<ro_sink>::value
                  && is_read_write_container<ro_sink>::value ));
    D_RO_CHECK(!( is_read_only_container<ro_inert>::value
                  && is_read_write_container<ro_inert>::value ));
    D_RO_CHECK(!( is_read_only_container<ro_vector>::value
                  && is_read_write_container<ro_vector>::value ));
    D_RO_CHECK(!( is_read_only_container<ro_view>::value
                  && is_read_write_container<ro_view>::value ));
    D_RO_CHECK(!( is_read_only_container<ro_tagged_override>::value
                  && is_read_write_container<ro_tagged_override>::value ));

    // each fires somewhere
    D_RO_CHECK(is_read_only_container<ro_observer>::value  == true);
    D_RO_CHECK(is_read_write_container<ro_both>::value     == true);

    // and a type may satisfy neither, in two distinct ways
    D_RO_CHECK(   ( is_read_only_container<ro_sink>::value  == false )
               && ( is_read_write_container<ro_sink>::value == false )
               && ( access_capability_of<ro_sink>::value
                        == access_capability::write_only ));
    D_RO_CHECK(   ( is_read_only_container<ro_inert>::value  == false )
               && ( is_read_write_container<ro_inert>::value == false )
               && ( access_capability_of<ro_inert>::value
                        == access_capability::none ));
    D_RO_CHECK(access_capability_of<ro_sink>::value
                   != access_capability_of<ro_inert>::value);

    return ok;
}

/*
tests_read_only_is_not_complement
  Verifies that read_only is NOT the negation of read_write - the mistake the
  lattice's shape invites.

  Two predicates naming opposite-sounding corners of a four-point lattice look
  complementary, and code written as `if (!is_read_write) treat_as_read_only`
  would be wrong for half the types that fail the read_write test.  A
  write_only sink is not read_write and is not read_only; neither is an
  unclassifiable type.

  Tests the following:
  - carriers where the negation of read_write and read_only disagree, which is
    the whole content of the claim
  - the disagreement in both directions is impossible for read_only implying
    read_write, and possible only in the direction shown - so the relation is a
    strict implication and not an equivalence
  - the safe formulation: read_only implies NOT read_write, but not conversely
*/
bool
tests_read_only_is_not_complement()
{
    bool ok = true;

    // not read_write, and not read_only either
    D_RO_CHECK(   ( is_read_write_container<ro_sink>::value == false )
               && ( is_read_only_container<ro_sink>::value  == false ));
    D_RO_CHECK(   ( is_read_write_container<ro_inert>::value == false )
               && ( is_read_only_container<ro_inert>::value  == false ));
    D_RO_CHECK(   ( is_read_write_container<ro_metadata_only>::value
                        == false )
               && ( is_read_only_container<ro_metadata_only>::value
                        == false ));
    D_RO_CHECK(   ( is_read_write_container<ro_pair>::value == false )
               && ( is_read_only_container<ro_pair>::value  == false ));

    // stated as the failing equivalence
    D_RO_CHECK(is_read_only_container<ro_sink>::value
                   != !is_read_write_container<ro_sink>::value);
    D_RO_CHECK(is_read_only_container<ro_inert>::value
                   != !is_read_write_container<ro_inert>::value);

    // the implication that DOES hold, in one direction only
    D_RO_CHECK(   !is_read_only_container<ro_observer>::value
               ||  !is_read_write_container<ro_observer>::value );
    D_RO_CHECK(   !is_read_only_container<ro_view>::value
               ||  !is_read_write_container<ro_view>::value );
    D_RO_CHECK(   !is_read_only_container<ro_tagged>::value
               ||  !is_read_write_container<ro_tagged>::value );

    // ... and its converse failing
    D_RO_CHECK(   ( !is_read_write_container<ro_sink>::value == true )
               && ( is_read_only_container<ro_sink>::value   == false ));

    return ok;
}

/*
tests_read_only_orthogonal_to_mutability
  Verifies the header's ORTHOGONALITY claim: the capability describes the
  HANDLE, not the data behind it.

  ro_view and ro_mutable_view wrap the same std::vector<int> and differ only in
  what they forward.  If the capability were a property of the underlying data
  the two would classify alike; they do not.  Conversely ro_view and
  ro_view_of_immutable forward the same surface over data with quite different
  mutability, and classify identically.  Between them the two comparisons pin
  the claim in both directions without importing the mutability-grade header.

  Tests the following:
  - two handles over the SAME underlying type classify differently, so the
    capability is not read off the data
  - two handles over DIFFERENT underlying types classify alike, so the
    capability is not merely correlated with the data either
  - the underlying container itself classifies read_write, so the restriction
    the handle imposes is visible as a change in capability
  - a const-qualified spelling of a read_write container remains read_write,
    since const-ness of a type spelling is not a handle restriction - the
    distinction that makes the overlay meaningful
*/
bool
tests_read_only_orthogonal_to_mutability()
{
    bool ok = true;

    // same underlying type, different capability
    D_RO_STATIC_CHECK(
        std::is_same<ro_view::underlying,
                     ro_mutable_view::underlying>::value);
    D_RO_CHECK(is_read_only_container<ro_view>::value         == true);
    D_RO_CHECK(is_read_only_container<ro_mutable_view>::value == false);
    D_RO_CHECK(access_capability_of<ro_view>::value
                   != access_capability_of<ro_mutable_view>::value);

    // different underlying types, same capability
    D_RO_STATIC_CHECK(
        std::is_same<ro_view::underlying,
                     ro_view_of_immutable::underlying>::value == false);
    D_RO_CHECK(is_read_only_container<ro_view_of_immutable>::value == true);
    D_RO_CHECK(access_capability_of<ro_view>::value
                   == access_capability_of<ro_view_of_immutable>::value);

    // the underlying container is read_write; the handle restricts it
    D_RO_CHECK(is_read_write_container<ro_view::underlying>::value == true);
    D_RO_CHECK(is_read_only_container<ro_view::underlying>::value  == false);
    D_RO_CHECK(access_capability_of<ro_view::underlying>::value
                   != access_capability_of<ro_view>::value);

    // ... and the unrestricted handle preserves it
    D_RO_CHECK(access_capability_of<ro_mutable_view>::value
                   == access_capability_of<ro_mutable_view::underlying>::value);

    // const-qualifying a spelling is not a handle restriction
    D_RO_CHECK(is_read_only_container<const ro_vector>::value == false);
    D_RO_CHECK(access_capability_of<const ro_vector>::value
                   == access_capability_of<ro_vector>::value);
    D_RO_CHECK(access_capability_of<const ro_vector>::value
                   != access_capability_of<ro_view>::value);

    return ok;
}

/*
tests_read_only_tag_agreement
  Verifies that the predicate and the baseline's tag re-emission name the same
  corner - so a type routed by access_tag_of and one tested by this predicate
  cannot disagree.
  Tests the following:
  - access_tag_of yields the read_only tag exactly when the predicate holds
  - the tag a read_only carrier re-emits carries bits matching the
    classification
  - a carrier of each other capability re-emits something else, so the
    correspondence is exact rather than one-directional
*/
bool
tests_read_only_tag_agreement()
{
    bool ok = true;

    // the tag is read_only exactly when the predicate holds
    D_RO_CHECK(std::is_same<access_tag_of_t<ro_observer>, read_only>::value
                   == is_read_only_container<ro_observer>::value);
    D_RO_CHECK(std::is_same<access_tag_of_t<ro_view>, read_only>::value
                   == is_read_only_container<ro_view>::value);
    D_RO_CHECK(std::is_same<access_tag_of_t<ro_tagged_override>,
                            read_only>::value
                   == is_read_only_container<ro_tagged_override>::value);
    D_RO_CHECK(std::is_same<access_tag_of_t<ro_sink>, read_only>::value
                   == is_read_only_container<ro_sink>::value);
    D_RO_CHECK(std::is_same<access_tag_of_t<ro_both>, read_only>::value
                   == is_read_only_container<ro_both>::value);
    D_RO_CHECK(std::is_same<access_tag_of_t<ro_inert>, read_only>::value
                   == is_read_only_container<ro_inert>::value);
    D_RO_CHECK(std::is_same<access_tag_of_t<ro_vector>, read_only>::value
                   == is_read_only_container<ro_vector>::value);

    // the tag's bits match the classification
    D_RO_CHECK(access_tag_of_t<ro_observer>::can_read  == true);
    D_RO_CHECK(access_tag_of_t<ro_observer>::can_write == false);
    D_RO_CHECK(access_tag_of_t<ro_view>::can_read
                   == access_capability_of<ro_view>::can_read);
    D_RO_CHECK(access_tag_of_t<ro_view>::can_write
                   == access_capability_of<ro_view>::can_write);

    // the other capabilities re-emit something else
    D_RO_STATIC_CHECK(
        std::is_same<access_tag_of_t<ro_sink>, write_only>::value);
    D_RO_STATIC_CHECK(
        std::is_same<access_tag_of_t<ro_both>, read_write>::value);
    D_RO_STATIC_CHECK(std::is_same<access_tag_of_t<ro_inert>, void>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
