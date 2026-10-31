/*******************************************************************************
* djinterp [cpp]           read_write_container_traits_tests_capability_bits.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/read_write_container_traits_tests_capability_bits.cpp
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
bool tests_has_capability_bits_helper()
{
    bool ok = true;

    // the library's own tags
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagReadOnly>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagWriteOnly>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagReadWrite>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagOverrides>::value == true);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagNoneBits>::value == true);

    // both bits are required - neither half alone is trusted, so the
    // requirement is a conjunction and cannot be weakened on either side
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagPartial>::value == false);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagPartialWrite>::value == false);
    D_RW_CHECK(access_capability_of<DRwTagPartialWrite>::value
                   == access_capability::read_write);

    // the tag must name a type carrying bits
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagAliasNoBits>::value == false);
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagDataMember>::value == false);

    // access control participates
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagPrivate>::value == false);

    // ordinary containers carry none
    D_RW_CHECK(internal::has_capability_bits_helper<DRwVector>::value
                   == false);
    D_RW_CHECK(internal::has_capability_bits_helper<DRwSet>::value  == false);
    D_RW_CHECK(internal::has_capability_bits_helper<DRwInert>::value
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
  is_read_write_container, access_tag_of, DAccessContainerClass, and the two
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
bool tests_capability_tag_shapes()
{
    bool ok = true;

    // function-shaped bits: trusted, and both read true
    D_RW_CHECK(internal::has_capability_bits_helper<
                   DRwTagFunctions>::value == true);
    D_RW_CHECK(access_capability_of<DRwTagFunctions>::can_read  == true);
    D_RW_CHECK(access_capability_of<DRwTagFunctions>::can_write == true);
    D_RW_CHECK(access_capability_of<DRwTagFunctions>::value
                   == access_capability::read_write);

    // ... though the type has no write surface at all
    D_RW_CHECK(   ( internal::has_write_surface<DRwTagFunctions>::value
                        == false )
               && ( access_capability_of<DRwTagFunctions>::can_write
                        == true ));

    // both bits false: a working opt-out
    D_RW_CHECK(access_capability_of<DRwTagNoneBits>::can_read  == false);
    D_RW_CHECK(access_capability_of<DRwTagNoneBits>::can_write == false);
    D_RW_CHECK(access_capability_of<DRwTagNoneBits>::value
                   == access_capability::none);

    // ... though the type has BOTH structural surfaces
    D_RW_CHECK(   ( internal::has_read_surface<DRwTagNoneBits>::value
                        == true )
               && ( internal::has_write_surface<DRwTagNoneBits>::value
                        == true )
               && ( access_capability_of<DRwTagNoneBits>::value
                        == access_capability::none ));

    // the shapes correctly ignored all fall back to structure
    D_RW_CHECK(access_capability_of<DRwTagPartial>::value
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<DRwTagAliasNoBits>::value
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<DRwTagDataMember>::value
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<DRwTagPrivate>::value
                   == access_capability::read_write);

    // and a private tag reads exactly as if it were not there
    D_RW_CHECK(access_capability_of<DRwTagPrivate>::value
                   == access_capability_of<DRwBoth>::value);

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
bool tests_access_bits_structural()
{
    bool ok = true;

    // the bits are the surfaces
    D_RW_CHECK(internal::access_bits<DRwVector>::can_read
                   == internal::has_read_surface<DRwVector>::value);
    D_RW_CHECK(internal::access_bits<DRwVector>::can_write
                   == internal::has_write_surface<DRwVector>::value);
    D_RW_CHECK(internal::access_bits<DRwObserver>::can_read
                   == internal::has_read_surface<DRwObserver>::value);
    D_RW_CHECK(internal::access_bits<DRwObserver>::can_write
                   == internal::has_write_surface<DRwObserver>::value);
    D_RW_CHECK(internal::access_bits<DRwSink>::can_read
                   == internal::has_read_surface<DRwSink>::value);
    D_RW_CHECK(internal::access_bits<DRwSink>::can_write
                   == internal::has_write_surface<DRwSink>::value);
    D_RW_CHECK(internal::access_bits<DRwInert>::can_read
                   == internal::has_read_surface<DRwInert>::value);
    D_RW_CHECK(internal::access_bits<DRwInert>::can_write
                   == internal::has_write_surface<DRwInert>::value);

    // all four bit combinations occur
    D_RW_CHECK(   ( internal::access_bits<DRwInert>::can_read    == false )
               && ( internal::access_bits<DRwInert>::can_write   == false ));
    D_RW_CHECK(   ( internal::access_bits<DRwObserver>::can_read  == true )
               && ( internal::access_bits<DRwObserver>::can_write == false ));
    D_RW_CHECK(   ( internal::access_bits<DRwSink>::can_read     == false )
               && ( internal::access_bits<DRwSink>::can_write    == true ));
    D_RW_CHECK(   ( internal::access_bits<DRwBoth>::can_read     == true )
               && ( internal::access_bits<DRwBoth>::can_write    == true ));

    // and the classifier reports exactly those bits
    D_RW_CHECK(access_capability_of<DRwSink>::can_read
                   == internal::access_bits<DRwSink>::can_read);
    D_RW_CHECK(access_capability_of<DRwSink>::can_write
                   == internal::access_bits<DRwSink>::can_write);

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
bool tests_access_bits_tagged()
{
    bool ok = true;

    // each tag read back exactly
    D_RW_CHECK(   ( internal::access_bits<DRwTagReadOnly>::can_read  == true )
               && ( internal::access_bits<DRwTagReadOnly>::can_write
                        == false ));
    D_RW_CHECK(   ( internal::access_bits<DRwTagWriteOnly>::can_read
                        == false )
               && ( internal::access_bits<DRwTagWriteOnly>::can_write
                        == true ));
    D_RW_CHECK(   ( internal::access_bits<DRwTagReadWrite>::can_read == true )
               && ( internal::access_bits<DRwTagReadWrite>::can_write
                        == true ));
    D_RW_CHECK(   ( internal::access_bits<DRwTagNoneBits>::can_read
                        == false )
               && ( internal::access_bits<DRwTagNoneBits>::can_write
                        == false ));

    // the bits agree with the tag type's own members
    D_RW_CHECK(internal::access_bits<DRwTagReadOnly>::can_read
                   == read_only::can_read);
    D_RW_CHECK(internal::access_bits<DRwTagReadOnly>::can_write
                   == read_only::can_write);
    D_RW_CHECK(internal::access_bits<DRwTagWriteOnly>::can_read
                   == write_only::can_read);
    D_RW_CHECK(internal::access_bits<DRwTagWriteOnly>::can_write
                   == write_only::can_write);
    D_RW_CHECK(internal::access_bits<DRwTagReadWrite>::can_read
                   == read_write::can_read);
    D_RW_CHECK(internal::access_bits<DRwTagReadWrite>::can_write
                   == read_write::can_write);

    // and NOT from the structure, which says otherwise
    D_RW_CHECK(   ( internal::has_write_surface<DRwTagReadOnly>::value
                        == true )
               && ( internal::access_bits<DRwTagReadOnly>::can_write
                        == false ));
    D_RW_CHECK(   ( internal::has_read_surface<DRwTagWriteOnly>::value
                        == true )
               && ( internal::access_bits<DRwTagWriteOnly>::can_read
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
bool tests_access_bits_precedence()
{
    bool ok = true;

    // the tag overrides a positive structural signal
    D_RW_CHECK(   ( internal::has_read_surface<DRwTagOverrides>::value
                        == true )
               && ( internal::has_write_surface<DRwTagOverrides>::value
                        == true )
               && ( access_capability_of<DRwTagOverrides>::value
                        == access_capability::read_only ));
    D_RW_CHECK(access_capability_of<DRwTagOverrides>::can_write == false);

    // ... and would have been read_write without it
    D_RW_CHECK(access_capability_from_bits(
                   internal::has_read_surface<DRwTagOverrides>::value,
                   internal::has_write_surface<DRwTagOverrides>::value)
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<DRwTagOverrides>::value
                   != access_capability_from_bits(
                          internal::has_read_surface<DRwTagOverrides>::value,
                          internal::has_write_surface<
                              DRwTagOverrides>::value));

    // a tag asserting more than the structure supports is honoured too
    D_RW_CHECK(   ( internal::has_write_surface<DRwTagFunctions>::value
                        == false )
               && ( access_capability_of<DRwTagFunctions>::can_write
                        == true ));

    // and a tag denying everything is honoured over both surfaces
    D_RW_CHECK(   ( internal::has_read_surface<DRwTagNoneBits>::value
                        == true )
               && ( access_capability_of<DRwTagNoneBits>::value
                        == access_capability::none ));

    // which path was taken stays visible
    D_RW_CHECK(access_capability_of<DRwTagOverrides>::has_capability_tag
                   == true);
    D_RW_CHECK(access_capability_of<DRwObserver>::has_capability_tag
                   == false);

    // two routes to one capability
    D_RW_CHECK(   ( access_capability_of<DRwTagReadOnly>::value
                        == access_capability_of<DRwObserver>::value )
               && ( access_capability_of<DRwTagReadOnly>::has_capability_tag
                        != access_capability_of<
                               DRwObserver>::has_capability_tag ));
    D_RW_CHECK(   ( access_capability_of<DRwTagNoneBits>::value
                        == access_capability_of<DRwInert>::value )
               && ( access_capability_of<
                        DRwTagNoneBits>::has_capability_tag
                        != access_capability_of<
                               DRwInert>::has_capability_tag ));

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
bool tests_capability_bits_cv_ref()
{
    bool ok = true;

    D_RW_CV_REF_AGREE(internal::has_capability_bits_helper,
                      DRwTagReadOnly, true);
    D_RW_CV_REF_AGREE(internal::has_capability_bits_helper, DRwVector, false);
    D_RW_CV_REF_AGREE(internal::has_capability_bits_helper,
                      DRwTagPartial, false);

    D_RW_CV_REF_AGREE(internal::has_read_surface,  DRwVector,   true);
    D_RW_CV_REF_AGREE(internal::has_read_surface,  DRwSink,     false);
    D_RW_CV_REF_AGREE(internal::has_write_surface, DRwVector,   true);
    D_RW_CV_REF_AGREE(internal::has_write_surface, DRwObserver, false);

    // the resolved bits, spelled out - access_bits has two members rather than
    // a `value`, so the shared macro does not reach it
    D_RW_CHECK(internal::access_bits<const DRwSink>::can_write
                   == internal::access_bits<DRwSink>::can_write);
    D_RW_CHECK(internal::access_bits<volatile DRwSink>::can_write
                   == internal::access_bits<DRwSink>::can_write);
    D_RW_CHECK(internal::access_bits<const volatile DRwSink&>::can_read
                   == internal::access_bits<DRwSink>::can_read);
    D_RW_CHECK(internal::access_bits<DRwSink&&>::can_write
                   == internal::access_bits<DRwSink>::can_write);

    D_RW_CHECK(internal::access_bits<volatile DRwTagReadOnly>::can_read
                   == internal::access_bits<DRwTagReadOnly>::can_read);
    D_RW_CHECK(internal::access_bits<const DRwTagReadOnly&>::can_write
                   == internal::access_bits<DRwTagReadOnly>::can_write);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
