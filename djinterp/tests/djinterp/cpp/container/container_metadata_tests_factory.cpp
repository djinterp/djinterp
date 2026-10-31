/*******************************************************************************
* djinterp [cpp]                            container_metadata_tests_factory.cpp
*
*
* path:      /tests/djinterp/cpp/container/container_metadata_tests_factory.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_metadata_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_container_metadata_factory
  make_container_metadata deduces both parameters and binds.
  Tests the following:
  - the returned type is DContainerMetadata<_Container, _Payload> with
    both arguments deduced, so a caller need not name either
  - the result is BOUND to the container that was passed, by address
  - the payload arrives with its value intact
  - deduction cleans its arguments: a const or reference-qualified payload
    argument still deduces the bare payload type, so the factory cannot
    produce a specialisation over a reference type
  - the deduced type satisfies is_container_metadata, which ties the
    factory to the trait rather than leaving them independently correct
*/
bool tests_container_metadata_factory()
{
    DCmBox box;
    box.n = 5;

    // deduction and binding
    {
        auto meta = make_container_metadata(box, DCmNote{12});
        D_CM_CHECK((std::is_same<decltype(meta), cm_meta>::value));
        D_CM_CHECK(meta.is_bound());
        D_CM_CHECK(meta.get_container() == &box);
        D_CM_CHECK(meta.payload().v == 12);
        D_CM_CHECK(is_container_metadata<decltype(meta)>::value);
    }

    // a const lvalue payload still deduces the bare payload type
    {
        const DCmNote note{13};
        auto          meta = make_container_metadata(box, note);
        D_CM_CHECK((std::is_same<decltype(meta), cm_meta>::value));
        D_CM_CHECK(meta.payload().v == 13);
    }

    // a non-const lvalue likewise
    {
        DCmNote note{14};
        auto    meta = make_container_metadata(box, note);
        D_CM_CHECK((std::is_same<decltype(meta), cm_meta>::value));
        D_CM_CHECK(meta.payload().v == 14);
    }

    // the payload type is never deduced as a reference or as const
    {
        DCmNote note{15};
        auto    meta = make_container_metadata(box, note);
        using   deduced = decltype(meta)::payload_type;
        D_CM_CHECK(!std::is_reference<deduced>::value);
        D_CM_CHECK(!std::is_const<deduced>::value);
        D_CM_CHECK((std::is_same<deduced, DCmNote>::value));
    }

    // the container type is deduced from the container argument
    {
        auto  meta = make_container_metadata(box, DCmNote{16});
        using deduced = decltype(meta)::container_type;
        D_CM_CHECK((std::is_same<deduced, DCmBox>::value));
        D_CM_CHECK(!std::is_reference<deduced>::value);
    }

    return true;
}

/*
tests_container_metadata_factory_forwarding
  The factory forwards its payload rather than copying it twice.
  Tests the following:
  - an rvalue payload reaches the stored member by MOVE, with no copy
  - an lvalue payload is copied exactly once, not copied and then moved
  - std::move on an lvalue reaches the move path
  These are measured with a counting payload.  The signature alone cannot
  establish them: a forwarding reference that is passed on as an lvalue
  would still compile, still be correct, and silently copy on every call,
  and only a count distinguishes that from the intended behaviour.
*/
bool tests_container_metadata_factory_forwarding()
{
    DCmBox box;

    // an rvalue is moved into place, never copied
    {
        DCmCounted::reset();
        auto meta = make_container_metadata(box, DCmCounted{1});
        D_CM_CHECK(DCmCounted::copies == 0);
        D_CM_CHECK(DCmCounted::moves == 1);
        D_CM_CHECK(meta.payload().v == 1);
    }

    // an lvalue is copied exactly once
    {
        DCmCounted source{2};
        DCmCounted::reset();
        auto meta = make_container_metadata(box, source);
        D_CM_CHECK(DCmCounted::copies == 1);
        D_CM_CHECK(DCmCounted::moves == 0);
        D_CM_CHECK(meta.payload().v == 2);
        D_CM_CHECK(source.v == 2);
    }

    // an explicit move reaches the move path
    {
        DCmCounted source{3};
        DCmCounted::reset();
        auto meta = make_container_metadata(box, std::move(source));
        D_CM_CHECK(DCmCounted::copies == 0);
        D_CM_CHECK(DCmCounted::moves == 1);
        D_CM_CHECK(meta.payload().v == 3);
    }

    // a const lvalue can only be copied
    {
        const DCmCounted source{4};
        DCmCounted::reset();
        auto meta = make_container_metadata(box, source);
        D_CM_CHECK(DCmCounted::copies == 1);
        D_CM_CHECK(DCmCounted::moves == 0);
        D_CM_CHECK(meta.payload().v == 4);
    }

    // the counting fixture itself distinguishes the two operations, so
    // the counts above are meaningful
    {
        DCmCounted::reset();
        DCmCounted a{5};
        DCmCounted b(a);
        DCmCounted c(std::move(a));
        D_CM_CHECK(DCmCounted::copies == 1);
        D_CM_CHECK(DCmCounted::moves == 1);
        D_CM_CHECK(b.v == 5 && c.v == 5);
    }

    return true;
}

/*
tests_container_metadata_protocol_composition
  DContainerMetadata does not participate in the metadata protocol that
  replaced metadata.hpp.
  Tests the following:
  - a DContainerMetadata satisfies NONE of has_metadata, has_metadata_type
    or has_metadata_container_type.  It exposes container_type,
    payload_type and payload(); the protocol looks for
    metadata_container_type, metadata_type and metadata.  The names were
    never brought across when metadata.hpp was superseded
  - a type written to the new names satisfies all three, so the failure
    above is a property of this module rather than of the protocol traits
  - the two vocabularies are otherwise sound in isolation: the protocol
    type is not a DContainerMetadata, and the DContainerMetadata is a
    DContainerMetadata, so neither trait is simply broken
  This section records the CURRENT state.  It is expected to fail if and
  when DContainerMetadata is brought onto the protocol, and that failure
  is the point: it is the notification that the rename happened, at which
  time these assertions should be inverted rather than deleted.
*/
bool tests_container_metadata_protocol_composition()
{
    // DContainerMetadata scores zero of three on the protocol
    D_CM_CHECK(!has_metadata<cm_meta>::value);
    D_CM_CHECK(!has_metadata_type<cm_meta>::value);
    D_CM_CHECK(!has_metadata_container_type<cm_meta>::value);

    // ... in every shape, not merely the nested-alias one
    D_CM_CHECK(!has_metadata_data_member<cm_meta>::value);
    D_CM_CHECK(!has_metadata_method<cm_meta>::value);
    D_CM_CHECK(!has_metadata_nested_type<cm_meta>::value);

    // a type written to the new names scores three of three, so the
    // protocol traits are working and the gap is on this side
    D_CM_CHECK(has_metadata<cm_protocol_type>::value);
    D_CM_CHECK(has_metadata_type<cm_protocol_type>::value);
    D_CM_CHECK(has_metadata_container_type<cm_protocol_type>::value);

    // the names DContainerMetadata actually exposes, for the record
    D_CM_CHECK(
     (std::is_same<cm_meta::container_type, DCmBox>::value));
    D_CM_CHECK(
     (std::is_same<cm_meta::payload_type, DCmNote>::value));

    // the two vocabularies are each internally consistent
    D_CM_CHECK(is_container_metadata<cm_meta>::value);
    D_CM_CHECK(!is_container_metadata<cm_protocol_type>::value);
    D_CM_CHECK(!has_metadata<cm_meta>::value &&
               is_container_metadata<cm_meta>::value);

    // and the protocol extractors work on the protocol type, so the
    // comparison above is between two live vocabularies
    D_CM_CHECK(
     (std::is_same<metadata_type_t<cm_protocol_type>, DCmNote>::value));
    D_CM_CHECK(
     (std::is_same<metadata_container_type_t<cm_protocol_type>,
                   DCmBox>::value));

    return true;
}


NS_END  // testing
NS_END  // djinterp
