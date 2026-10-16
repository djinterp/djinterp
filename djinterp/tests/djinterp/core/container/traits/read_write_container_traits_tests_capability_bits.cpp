/*******************************************************************************
* djinterp [core]          read_write_container_traits_tests_capability_bits.cpp
*
*
* path:      /tests/djinterp/core/container/traits/read_write_container_traits_tests_capability_bits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I-c. The capability tag, and the resolution of (can_read, can_write)
// ===========================================================================
//   The opt-in signal, and the two-specialisation dispatch that chooses
// between it and the structural surface.  access_bits_impl's primary template
// reads structure; its <_Type, true> specialisation reads the stamped tag.
// Which is selected is decided entirely by has_capability_bits_helper.


/*
tests_has_capability_bits_helper
  Verifies the opt-in probe: a `capability` naming a TYPE that carries BOTH
  can_read and can_write.
  Tests the following:
  - the library's own three capability tags are all recognised
  - a tag carrying only one bit is NOT recognised - both are required, so the
    conjunction cannot be weakened to a disjunction without a fixture noticing
  - a `capability` naming a type with no bits at all is not recognised
  - a `capability` that is a DATA member rather than a type is not recognised,
    since a value cannot open a nested-name-specifier
  - a private `capability` is not recognised, access control participating
  - an ordinary container carries no tag
*/
bool
tests_has_capability_bits_helper()
{
    bool ok = true;

    // the library's own tags
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_read_only>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_write_only>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_read_write>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_overrides>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_none_bits>::value == true);

    // both bits are required - neither half alone is trusted, so the
    // requirement is a conjunction and cannot be weakened on either side
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_partial>::value == false);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_partial_write>::value == false);
    D_RW_CHECK(access_capability_of<rw_tag_partial_write>::value
                   == access_capability::read_write);

    // the tag must name a type carrying bits
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_alias_no_bits>::value == false);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_data_member>::value == false);

    // access control participates
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_private>::value == false);

    // ordinary containers carry none
    D_RW_CHECK(internal::has_capability_bits_helper<rw_vector>::value
                   == false);
    D_RW_CHECK(internal::has_capability_bits_helper<rw_set>::value  == false);
    D_RW_CHECK(internal::has_capability_bits_helper<rw_inert>::value
                   == false);
    D_RW_CHECK(internal::has_capability_bits_helper<int>::value     == false);
    D_RW_CHECK(internal::has_capability_bits_helper<void>::value    == false);

    return ok;
}

/*
tests_capability_tag_shapes
  Verifies exactly which tag shapes the opt-in accepts, and documents the one
  that cannot be tested at all.

  THE UNTESTABLE SHAPE.  A capability tag whose can_read / can_write are
  NON-STATIC data members makes the header fail to compile.  The detection -
  decltype of the id-expressions - is unevaluated and so succeeds for non-static
  data members, selecting access_bits_impl's tagged specialisation; that
  specialisation's initialisers are then EVALUATED, and reading a non-static
  data member without an object is ill-formed.  The error is in the
  specialisation's body rather than its immediate context, so substitution
  failure cannot rescue it:

      error: invalid use of non-static data member 'X::c::can_read'

  It is raised by any use of access_capability_of, and so transitively by
  is_read_write_container, access_tag_of, access_container_class, and the two
  corner headers built on this one.  This is the one shape whose fixture would
  break the build rather than fail an assertion, so the suite carries no such
  fixture; the bound is described here instead.

  A tag whose bits are STATIC MEMBER FUNCTIONS is accepted, and both bits read
  TRUE - each function decays to a pointer, and a function pointer is never
  null.  So a tag declaring can_read() / can_write() reports full read_write
  capability whatever those functions would return, and the structural surface
  is skipped.  GCC does diagnose this at the point of use with -Waddress
  ("the address of ... will never be NULL"), so a build treating warnings as
  errors would catch it; the classification itself reports nothing amiss.

  Tests the following:
  - the function-shaped tag, both bits asserted, carried through to read_write
  - a tag with both bits FALSE, which is a working opt-out and reaches `none`
  - the three shapes that are correctly ignored, each falling back to structure
*/
bool
tests_capability_tag_shapes()
{
    bool ok = true;

    // function-shaped bits: trusted, and both read true
    D_RW_CHECK(internal::has_capability_bits_helper<
                   rw_tag_functions>::value == true);
    D_RW_CHECK(access_capability_of<rw_tag_functions>::can_read  == true);
    D_RW_CHECK(access_capability_of<rw_tag_functions>::can_write == true);
    D_RW_CHECK(access_capability_of<rw_tag_functions>::value
                   == access_capability::read_write);

    // ... though the type has no write surface at all
    D_RW_CHECK(   ( internal::has_write_surface<rw_tag_functions>::value
                        == false )
               && ( access_capability_of<rw_tag_functions>::can_write
                        == true ));

    // both bits false: a working opt-out
    D_RW_CHECK(access_capability_of<rw_tag_none_bits>::can_read  == false);
    D_RW_CHECK(access_capability_of<rw_tag_none_bits>::can_write == false);
    D_RW_CHECK(access_capability_of<rw_tag_none_bits>::value
                   == access_capability::none);

    // ... though the type has BOTH structural surfaces
    D_RW_CHECK(   ( internal::has_read_surface<rw_tag_none_bits>::value
                        == true )
               && ( internal::has_write_surface<rw_tag_none_bits>::value
                        == true )
               && ( access_capability_of<rw_tag_none_bits>::value
                        == access_capability::none ));

    // the shapes correctly ignored all fall back to structure
    D_RW_CHECK(access_capability_of<rw_tag_partial>::value
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<rw_tag_alias_no_bits>::value
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<rw_tag_data_member>::value
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<rw_tag_private>::value
                   == access_capability::read_write);

    // and a private tag reads exactly as if it were not there
    D_RW_CHECK(access_capability_of<rw_tag_private>::value
                   == access_capability_of<rw_both>::value);

    return ok;
}

/*
tests_access_bits_structural
  Verifies access_bits_impl's PRIMARY template - the untagged path, which
  copies the two structural surfaces straight through.
  Tests the following:
  - can_read equals has_read_surface and can_write equals has_write_surface,
    on carriers occupying each of the four capabilities
  - access_bits forwards to the impl unchanged
  - the bits are independent: all four combinations occur among the fixtures
*/
bool
tests_access_bits_structural()
{
    bool ok = true;

    // the bits are the surfaces
    D_RW_CHECK(internal::access_bits<rw_vector>::can_read
                   == internal::has_read_surface<rw_vector>::value);
    D_RW_CHECK(internal::access_bits<rw_vector>::can_write
                   == internal::has_write_surface<rw_vector>::value);
    D_RW_CHECK(internal::access_bits<rw_observer>::can_read
                   == internal::has_read_surface<rw_observer>::value);
    D_RW_CHECK(internal::access_bits<rw_observer>::can_write
                   == internal::has_write_surface<rw_observer>::value);
    D_RW_CHECK(internal::access_bits<rw_sink>::can_read
                   == internal::has_read_surface<rw_sink>::value);
    D_RW_CHECK(internal::access_bits<rw_sink>::can_write
                   == internal::has_write_surface<rw_sink>::value);
    D_RW_CHECK(internal::access_bits<rw_inert>::can_read
                   == internal::has_read_surface<rw_inert>::value);
    D_RW_CHECK(internal::access_bits<rw_inert>::can_write
                   == internal::has_write_surface<rw_inert>::value);

    // all four bit combinations occur
    D_RW_CHECK(   ( internal::access_bits<rw_inert>::can_read    == false )
               && ( internal::access_bits<rw_inert>::can_write   == false ));
    D_RW_CHECK(   ( internal::access_bits<rw_observer>::can_read  == true )
               && ( internal::access_bits<rw_observer>::can_write == false ));
    D_RW_CHECK(   ( internal::access_bits<rw_sink>::can_read     == false )
               && ( internal::access_bits<rw_sink>::can_write    == true ));
    D_RW_CHECK(   ( internal::access_bits<rw_both>::can_read     == true )
               && ( internal::access_bits<rw_both>::can_write    == true ));

    // and the classifier reports exactly those bits
    D_RW_CHECK(access_capability_of<rw_sink>::can_read
                   == internal::access_bits<rw_sink>::can_read);
    D_RW_CHECK(access_capability_of<rw_sink>::can_write
                   == internal::access_bits<rw_sink>::can_write);

    return ok;
}

/*
tests_access_bits_tagged
  Verifies access_bits_impl's <_Type, true> SPECIALISATION - the tagged path,
  which reads the stamped bits and never looks at the structure.
  Tests the following:
  - each of the library's three tags is read back exactly, on a carrier stamped
    with it
  - the bits come from the tag and not from the surfaces, shown on carriers
    whose structure would give a different answer
  - a tag with both bits false is read back as both false
*/
bool
tests_access_bits_tagged()
{
    bool ok = true;

    // each tag read back exactly
    D_RW_CHECK(   ( internal::access_bits<rw_tag_read_only>::can_read  == true )
               && ( internal::access_bits<rw_tag_read_only>::can_write
                        == false ));
    D_RW_CHECK(   ( internal::access_bits<rw_tag_write_only>::can_read
                        == false )
               && ( internal::access_bits<rw_tag_write_only>::can_write
                        == true ));
    D_RW_CHECK(   ( internal::access_bits<rw_tag_read_write>::can_read == true )
               && ( internal::access_bits<rw_tag_read_write>::can_write
                        == true ));
    D_RW_CHECK(   ( internal::access_bits<rw_tag_none_bits>::can_read
                        == false )
               && ( internal::access_bits<rw_tag_none_bits>::can_write
                        == false ));

    // the bits agree with the tag type's own members
    D_RW_CHECK(internal::access_bits<rw_tag_read_only>::can_read
                   == read_only::can_read);
    D_RW_CHECK(internal::access_bits<rw_tag_read_only>::can_write
                   == read_only::can_write);
    D_RW_CHECK(internal::access_bits<rw_tag_write_only>::can_read
                   == write_only::can_read);
    D_RW_CHECK(internal::access_bits<rw_tag_write_only>::can_write
                   == write_only::can_write);
    D_RW_CHECK(internal::access_bits<rw_tag_read_write>::can_read
                   == read_write::can_read);
    D_RW_CHECK(internal::access_bits<rw_tag_read_write>::can_write
                   == read_write::can_write);

    // and NOT from the structure, which says otherwise
    D_RW_CHECK(   ( internal::has_write_surface<rw_tag_read_only>::value
                        == true )
               && ( internal::access_bits<rw_tag_read_only>::can_write
                        == false ));
    D_RW_CHECK(   ( internal::has_read_surface<rw_tag_write_only>::value
                        == true )
               && ( internal::access_bits<rw_tag_write_only>::can_read
                        == false ));

    return ok;
}

/*
tests_access_bits_precedence
  Verifies that the tag is honoured FIRST - the priority the header documents,
  and the escape hatch that makes the structural surface's heuristics
  correctable.
  Tests the following:
  - a carrier that is structurally read_write but stamped read_only reads
    read_only, so the tag is not merely consulted but decisive
  - both structural surfaces are present on that carrier, so the tag is
    overriding a positive signal rather than filling a gap
  - the reverse direction too: a tag asserting a capability the structure does
    not support is honoured just the same
  - has_capability_tag reports which path was taken, so the two routes to a
    given capability remain distinguishable
  - an untagged carrier and a tagged one may reach the same capability by
    different routes
*/
bool
tests_access_bits_precedence()
{
    bool ok = true;

    // the tag overrides a positive structural signal
    D_RW_CHECK(   ( internal::has_read_surface<rw_tag_overrides>::value
                        == true )
               && ( internal::has_write_surface<rw_tag_overrides>::value
                        == true )
               && ( access_capability_of<rw_tag_overrides>::value
                        == access_capability::read_only ));
    D_RW_CHECK(access_capability_of<rw_tag_overrides>::can_write == false);

    // ... and would have been read_write without it
    D_RW_CHECK(access_capability_from_bits(
                   internal::has_read_surface<rw_tag_overrides>::value,
                   internal::has_write_surface<rw_tag_overrides>::value)
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<rw_tag_overrides>::value
                   != access_capability_from_bits(
                          internal::has_read_surface<rw_tag_overrides>::value,
                          internal::has_write_surface<
                              rw_tag_overrides>::value));

    // a tag asserting more than the structure supports is honoured too
    D_RW_CHECK(   ( internal::has_write_surface<rw_tag_functions>::value
                        == false )
               && ( access_capability_of<rw_tag_functions>::can_write
                        == true ));

    // and a tag denying everything is honoured over both surfaces
    D_RW_CHECK(   ( internal::has_read_surface<rw_tag_none_bits>::value
                        == true )
               && ( access_capability_of<rw_tag_none_bits>::value
                        == access_capability::none ));

    // which path was taken stays visible
    D_RW_CHECK(access_capability_of<rw_tag_overrides>::has_capability_tag
                   == true);
    D_RW_CHECK(access_capability_of<rw_observer>::has_capability_tag
                   == false);

    // two routes to one capability
    D_RW_CHECK(   ( access_capability_of<rw_tag_read_only>::value
                        == access_capability_of<rw_observer>::value )
               && ( access_capability_of<rw_tag_read_only>::has_capability_tag
                        != access_capability_of<
                               rw_observer>::has_capability_tag ));
    D_RW_CHECK(   ( access_capability_of<rw_tag_none_bits>::value
                        == access_capability_of<rw_inert>::value )
               && ( access_capability_of<
                        rw_tag_none_bits>::has_capability_tag
                        != access_capability_of<
                               rw_inert>::has_capability_tag ));

    return ok;
}

/*
tests_capability_bits_cv_ref
  Verifies that the signal probes and the bits resolution honour the clean_t
  contract.
  Tests the following:
  - the tag probe is identical across all seven cv- and reference-qualified
    spellings
  - the two surface aggregates likewise
  - the resolved bits likewise, on a tagged and an untagged carrier
  - the volatile spellings in particular
*/
bool
tests_capability_bits_cv_ref()
{
    bool ok = true;

    D_RW_CV_REF_AGREE(internal::has_capability_bits_helper,
                      rw_tag_read_only, true);
    D_RW_CV_REF_AGREE(internal::has_capability_bits_helper, rw_vector, false);
    D_RW_CV_REF_AGREE(internal::has_capability_bits_helper,
                      rw_tag_partial, false);

    D_RW_CV_REF_AGREE(internal::has_read_surface,  rw_vector,   true);
    D_RW_CV_REF_AGREE(internal::has_read_surface,  rw_sink,     false);
    D_RW_CV_REF_AGREE(internal::has_write_surface, rw_vector,   true);
    D_RW_CV_REF_AGREE(internal::has_write_surface, rw_observer, false);

    // the resolved bits, spelled out - access_bits has two members rather than
    // a `value`, so the shared macro does not reach it
    D_RW_CHECK(internal::access_bits<const rw_sink>::can_write
                   == internal::access_bits<rw_sink>::can_write);
    D_RW_CHECK(internal::access_bits<volatile rw_sink>::can_write
                   == internal::access_bits<rw_sink>::can_write);
    D_RW_CHECK(internal::access_bits<const volatile rw_sink&>::can_read
                   == internal::access_bits<rw_sink>::can_read);
    D_RW_CHECK(internal::access_bits<rw_sink&&>::can_write
                   == internal::access_bits<rw_sink>::can_write);

    D_RW_CHECK(internal::access_bits<volatile rw_tag_read_only>::can_read
                   == internal::access_bits<rw_tag_read_only>::can_read);
    D_RW_CHECK(internal::access_bits<const rw_tag_read_only&>::can_write
                   == internal::access_bits<rw_tag_read_only>::can_write);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
