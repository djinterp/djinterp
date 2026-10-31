/*******************************************************************************
* djinterp [cpp]                     read_write_container_traits_tests_class.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/read_write_container_traits_tests_class.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// IV.  The aggregate snapshot
// ===========================================================================
//   DAccessContainerClass restates the whole axis for one type in eight
// members.  It introduces no new detection, so the tests here are about
// WIRING - that each member is fed from the trait it claims to restate, and
// that no two members are crossed.


/*
tests_access_container_class_bits
  Verifies the three boolean members the aggregate carries forward.
  Tests the following:
  - can_read and can_write restate the classifier's bits
  - has_capability_tag restates the classifier's flag
  - the three are independent, shown on carriers where they disagree
  - all three are usable in a constant expression
*/
bool tests_access_container_class_bits()
{
    bool ok = true;

    // the bits, restated
    D_RW_CHECK(DAccessContainerClass<DRwVector>::can_read
                   == access_capability_of<DRwVector>::can_read);
    D_RW_CHECK(DAccessContainerClass<DRwVector>::can_write
                   == access_capability_of<DRwVector>::can_write);
    D_RW_CHECK(DAccessContainerClass<DRwObserver>::can_read
                   == access_capability_of<DRwObserver>::can_read);
    D_RW_CHECK(DAccessContainerClass<DRwObserver>::can_write
                   == access_capability_of<DRwObserver>::can_write);
    D_RW_CHECK(DAccessContainerClass<DRwSink>::can_read
                   == access_capability_of<DRwSink>::can_read);
    D_RW_CHECK(DAccessContainerClass<DRwSink>::can_write
                   == access_capability_of<DRwSink>::can_write);
    D_RW_CHECK(DAccessContainerClass<DRwInert>::can_read
                   == access_capability_of<DRwInert>::can_read);
    D_RW_CHECK(DAccessContainerClass<DRwInert>::can_write
                   == access_capability_of<DRwInert>::can_write);

    // read literally
    D_RW_CHECK(   ( DAccessContainerClass<DRwObserver>::can_read  == true )
               && ( DAccessContainerClass<DRwObserver>::can_write == false ));
    D_RW_CHECK(   ( DAccessContainerClass<DRwSink>::can_read      == false )
               && ( DAccessContainerClass<DRwSink>::can_write     == true ));

    // the tag flag, restated
    D_RW_CHECK(DAccessContainerClass<DRwTagReadOnly>::has_capability_tag
                   == true);
    D_RW_CHECK(DAccessContainerClass<DRwVector>::has_capability_tag
                   == false);
    D_RW_CHECK(DAccessContainerClass<DRwTagOverrides>::has_capability_tag
                   == access_capability_of<
                          DRwTagOverrides>::has_capability_tag);

    // and it is independent of the bits
    D_RW_CHECK(   ( DAccessContainerClass<DRwVector>::can_read
                        == DAccessContainerClass<
                               DRwTagReadWrite>::can_read )
               && ( DAccessContainerClass<DRwVector>::has_capability_tag
                        != DAccessContainerClass<
                               DRwTagReadWrite>::has_capability_tag ));

    // constexpr
    D_RW_STATIC_CHECK(DAccessContainerClass<DRwVector>::can_read);
    D_RW_STATIC_CHECK(DAccessContainerClass<DRwSink>::can_write);
    D_RW_STATIC_CHECK(
        DAccessContainerClass<DRwTagReadOnly>::has_capability_tag);

    return ok;
}

/*
tests_access_container_class_kind
  Verifies the capability and its spelling.
  Tests the following:
  - kind restates the classifier's verdict, on a carrier for each capability
  - kind_name is that capability's spelling, and is derived from kind rather
    than assigned separately - shown by re-deriving it
  - both are usable in a constant expression
*/
bool tests_access_container_class_kind()
{
    bool ok = true;

    // kind, per capability
    D_RW_CHECK(DAccessContainerClass<DRwInert>::kind
                   == access_capability::none);
    D_RW_CHECK(DAccessContainerClass<DRwObserver>::kind
                   == access_capability::read_only);
    D_RW_CHECK(DAccessContainerClass<DRwSink>::kind
                   == access_capability::write_only);
    D_RW_CHECK(DAccessContainerClass<DRwVector>::kind
                   == access_capability::read_write);
    D_RW_CHECK(DAccessContainerClass<DRwTagNoneBits>::kind
                   == access_capability::none);

    // restating the classifier
    D_RW_CHECK(DAccessContainerClass<DRwSet>::kind
                   == access_capability_of<DRwSet>::value);
    D_RW_CHECK(DAccessContainerClass<DRwTagOverrides>::kind
                   == access_capability_of<DRwTagOverrides>::value);

    // kind_name is derived from kind
    D_RW_CHECK(std::strcmp(DAccessContainerClass<DRwVector>::kind_name,
                           access_capability_name(
                               DAccessContainerClass<DRwVector>::kind))
                   == 0);
    D_RW_CHECK(std::strcmp(DAccessContainerClass<DRwSink>::kind_name,
                           access_capability_name(
                               DAccessContainerClass<DRwSink>::kind))
                   == 0);

    // read literally
    D_RW_CHECK(std::strcmp(DAccessContainerClass<DRwVector>::kind_name,
                           "read_write") == 0);
    D_RW_CHECK(std::strcmp(DAccessContainerClass<DRwObserver>::kind_name,
                           "read_only") == 0);
    D_RW_CHECK(std::strcmp(DAccessContainerClass<DRwSink>::kind_name,
                           "write_only") == 0);
    D_RW_CHECK(std::strcmp(DAccessContainerClass<DRwInert>::kind_name,
                           "none") == 0);

    // constexpr
    D_RW_STATIC_CHECK(DAccessContainerClass<DRwVector>::kind
                          == access_capability::read_write);
    D_RW_STATIC_CHECK(DAccessContainerClass<DRwInert>::kind_name[0] == 'n');

    return ok;
}

/*
tests_access_container_class_predicates
  Verifies the three per-capability booleans.
  Tests the following:
  - each is true on exactly its own capability
  - they are MUTUALLY EXCLUSIVE, at most one holding for any type
  - none of the three holds for an unclassifiable type, so the aggregate has no
    boolean naming `none` and its absence must be read as all three false
  - each is derived from kind rather than recomputed
*/
bool tests_access_container_class_predicates()
{
    bool ok = true;

    // each true on its own capability
    D_RW_CHECK(DAccessContainerClass<DRwObserver>::is_read_only  == true);
    D_RW_CHECK(DAccessContainerClass<DRwSink>::is_write_only     == true);
    D_RW_CHECK(DAccessContainerClass<DRwVector>::is_read_write   == true);

    // ... and false on the others
    D_RW_CHECK(DAccessContainerClass<DRwObserver>::is_write_only == false);
    D_RW_CHECK(DAccessContainerClass<DRwObserver>::is_read_write == false);
    D_RW_CHECK(DAccessContainerClass<DRwSink>::is_read_only      == false);
    D_RW_CHECK(DAccessContainerClass<DRwSink>::is_read_write     == false);
    D_RW_CHECK(DAccessContainerClass<DRwVector>::is_read_only    == false);
    D_RW_CHECK(DAccessContainerClass<DRwVector>::is_write_only   == false);

    // `none` is the absence of all three
    D_RW_CHECK(   ( DAccessContainerClass<DRwInert>::is_read_only  == false )
               && ( DAccessContainerClass<DRwInert>::is_write_only == false )
               && ( DAccessContainerClass<DRwInert>::is_read_write
                        == false ));
    D_RW_CHECK(   ( DAccessContainerClass<DRwTagNoneBits>::is_read_only
                        == false )
               && ( DAccessContainerClass<DRwTagNoneBits>::is_write_only
                        == false )
               && ( DAccessContainerClass<DRwTagNoneBits>::is_read_write
                        == false ));

    // mutually exclusive, over every carrier
    D_RW_CHECK(   ( static_cast<int>(
                        DAccessContainerClass<DRwVector>::is_read_only )
                  + static_cast<int>(
                        DAccessContainerClass<DRwVector>::is_write_only )
                  + static_cast<int>(
                        DAccessContainerClass<DRwVector>::is_read_write ) )
               == 1);
    D_RW_CHECK(   ( static_cast<int>(
                        DAccessContainerClass<DRwObserver>::is_read_only )
                  + static_cast<int>(
                        DAccessContainerClass<DRwObserver>::is_write_only )
                  + static_cast<int>(
                        DAccessContainerClass<DRwObserver>::is_read_write ) )
               == 1);
    D_RW_CHECK(   ( static_cast<int>(
                        DAccessContainerClass<DRwInert>::is_read_only )
                  + static_cast<int>(
                        DAccessContainerClass<DRwInert>::is_write_only )
                  + static_cast<int>(
                        DAccessContainerClass<DRwInert>::is_read_write ) )
               == 0);

    // derived from kind
    D_RW_CHECK(DAccessContainerClass<DRwTagOverrides>::is_read_only
                   == ( DAccessContainerClass<DRwTagOverrides>::kind
                            == access_capability::read_only ));
    D_RW_CHECK(DAccessContainerClass<DRwSet>::is_read_write
                   == ( DAccessContainerClass<DRwSet>::kind
                            == access_capability::read_write ));

    return ok;
}

/*
tests_access_container_class_agreement
  Verifies that the aggregate restates the standalone traits and does not
  shadow them - the property that makes it safe to read either one.
  Tests the following:
  - is_read_write agrees with is_read_write_container, on every carrier
  - kind agrees with access_capability_of
  - the bits and the tag flag agree with the classifier's members
  - the agreement holds on the carriers where the two SIGNALS disagree, which
    is where a mis-wiring would most easily hide
*/
bool tests_access_container_class_agreement()
{
    bool ok = true;

    // the predicate
    D_RW_CHECK(DAccessContainerClass<DRwVector>::is_read_write
                   == is_read_write_container<DRwVector>::value);
    D_RW_CHECK(DAccessContainerClass<DRwObserver>::is_read_write
                   == is_read_write_container<DRwObserver>::value);
    D_RW_CHECK(DAccessContainerClass<DRwSink>::is_read_write
                   == is_read_write_container<DRwSink>::value);
    D_RW_CHECK(DAccessContainerClass<DRwInert>::is_read_write
                   == is_read_write_container<DRwInert>::value);
    D_RW_CHECK(DAccessContainerClass<DRwSet>::is_read_write
                   == is_read_write_container<DRwSet>::value);
    D_RW_CHECK(DAccessContainerClass<DRwArray>::is_read_write
                   == is_read_write_container<DRwArray>::value);

    // the classification
    D_RW_CHECK(DAccessContainerClass<DRwFlist>::kind
                   == access_capability_of<DRwFlist>::value);
    D_RW_CHECK(DAccessContainerClass<DRwMetadataOnly>::kind
                   == access_capability_of<DRwMetadataOnly>::value);
    D_RW_CHECK(DAccessContainerClass<DRwWriteClear>::kind
                   == access_capability_of<DRwWriteClear>::value);

    // where the two signals disagree
    D_RW_CHECK(DAccessContainerClass<DRwTagOverrides>::kind
                   == access_capability_of<DRwTagOverrides>::value);
    D_RW_CHECK(DAccessContainerClass<DRwTagOverrides>::is_read_only
                   == true);
    D_RW_CHECK(DAccessContainerClass<DRwTagOverrides>::is_read_write
                   == is_read_write_container<DRwTagOverrides>::value);
    D_RW_CHECK(DAccessContainerClass<DRwTagNoneBits>::kind
                   == access_capability_of<DRwTagNoneBits>::value);
    D_RW_CHECK(DAccessContainerClass<DRwTagReadOnly>::can_write
                   == access_capability_of<DRwTagReadOnly>::can_write);

    // and where value_type decides the write surface
    D_RW_CHECK(DAccessContainerClass<rw_clear_no_value_type>::kind
                   == access_capability_of<rw_clear_no_value_type>::value);
    D_RW_CHECK(DAccessContainerClass<rw_push_back_no_value_type>::kind
                   == access_capability_of<
                          rw_push_back_no_value_type>::value);

    // the tag flag
    D_RW_CHECK(DAccessContainerClass<DRwTagReadOnly>::has_capability_tag
                   == access_capability_of<
                          DRwTagReadOnly>::has_capability_tag);
    D_RW_CHECK(DAccessContainerClass<DRwTagPartial>::has_capability_tag
                   == access_capability_of<
                          DRwTagPartial>::has_capability_tag);

    return ok;
}

/*
tests_access_container_class_cv_ref
  Verifies that the aggregate honours the clean_t contract.
  Tests the following:
  - kind is identical across all seven cv- and reference-qualified spellings,
    on a carrier for each capability
  - the bits, the tag flag and the three predicates likewise
  - the spelling likewise
  - the volatile spellings in particular
*/
bool tests_access_container_class_cv_ref()
{
    bool ok = true;

    // kind, across the spellings
    D_RW_CHECK(DAccessContainerClass<const DRwVector>::kind
                   == DAccessContainerClass<DRwVector>::kind);
    D_RW_CHECK(DAccessContainerClass<volatile DRwVector>::kind
                   == DAccessContainerClass<DRwVector>::kind);
    D_RW_CHECK(DAccessContainerClass<const volatile DRwVector>::kind
                   == DAccessContainerClass<DRwVector>::kind);
    D_RW_CHECK(DAccessContainerClass<DRwVector&>::kind
                   == DAccessContainerClass<DRwVector>::kind);
    D_RW_CHECK(DAccessContainerClass<const DRwVector&>::kind
                   == DAccessContainerClass<DRwVector>::kind);
    D_RW_CHECK(DAccessContainerClass<DRwVector&&>::kind
                   == DAccessContainerClass<DRwVector>::kind);

    D_RW_CHECK(DAccessContainerClass<const DRwObserver&>::kind
                   == access_capability::read_only);
    D_RW_CHECK(DAccessContainerClass<volatile DRwSink>::kind
                   == access_capability::write_only);
    D_RW_CHECK(DAccessContainerClass<const volatile DRwInert&>::kind
                   == access_capability::none);

    // the bits and the flag
    D_RW_CHECK(DAccessContainerClass<volatile DRwSink>::can_write
                   == DAccessContainerClass<DRwSink>::can_write);
    D_RW_CHECK(DAccessContainerClass<const volatile DRwObserver&>::can_read
                   == DAccessContainerClass<DRwObserver>::can_read);
    D_RW_CHECK(DAccessContainerClass<
                   volatile DRwTagReadOnly>::has_capability_tag
                   == DAccessContainerClass<
                          DRwTagReadOnly>::has_capability_tag);

    // the predicates
    D_RW_CHECK(DAccessContainerClass<const DRwVector&>::is_read_write
                   == DAccessContainerClass<DRwVector>::is_read_write);
    D_RW_CHECK(DAccessContainerClass<volatile DRwObserver>::is_read_only
                   == DAccessContainerClass<DRwObserver>::is_read_only);
    D_RW_CHECK(DAccessContainerClass<DRwSink&&>::is_write_only
                   == DAccessContainerClass<DRwSink>::is_write_only);

    // the spelling
    D_RW_CHECK(std::strcmp(
                   DAccessContainerClass<const volatile DRwVector&>::kind_name,
                   DAccessContainerClass<DRwVector>::kind_name) == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
