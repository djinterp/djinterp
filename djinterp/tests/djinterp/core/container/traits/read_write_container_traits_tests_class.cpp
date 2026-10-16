/*******************************************************************************
* djinterp [core]                    read_write_container_traits_tests_class.cpp
*
*
* path:      /tests/djinterp/core/container/traits/read_write_container_traits_tests_class.cpp
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
//   access_container_class restates the whole axis for one type in eight
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
bool
tests_access_container_class_bits()
{
    bool ok = true;

    // the bits, restated
    D_RW_CHECK(access_container_class<rw_vector>::can_read
                   == access_capability_of<rw_vector>::can_read);
    D_RW_CHECK(access_container_class<rw_vector>::can_write
                   == access_capability_of<rw_vector>::can_write);
    D_RW_CHECK(access_container_class<rw_observer>::can_read
                   == access_capability_of<rw_observer>::can_read);
    D_RW_CHECK(access_container_class<rw_observer>::can_write
                   == access_capability_of<rw_observer>::can_write);
    D_RW_CHECK(access_container_class<rw_sink>::can_read
                   == access_capability_of<rw_sink>::can_read);
    D_RW_CHECK(access_container_class<rw_sink>::can_write
                   == access_capability_of<rw_sink>::can_write);
    D_RW_CHECK(access_container_class<rw_inert>::can_read
                   == access_capability_of<rw_inert>::can_read);
    D_RW_CHECK(access_container_class<rw_inert>::can_write
                   == access_capability_of<rw_inert>::can_write);

    // read literally
    D_RW_CHECK(   ( access_container_class<rw_observer>::can_read  == true )
               && ( access_container_class<rw_observer>::can_write == false ));
    D_RW_CHECK(   ( access_container_class<rw_sink>::can_read      == false )
               && ( access_container_class<rw_sink>::can_write     == true ));

    // the tag flag, restated
    D_RW_CHECK(access_container_class<rw_tag_read_only>::has_capability_tag
                   == true);
    D_RW_CHECK(access_container_class<rw_vector>::has_capability_tag
                   == false);
    D_RW_CHECK(access_container_class<rw_tag_overrides>::has_capability_tag
                   == access_capability_of<
                          rw_tag_overrides>::has_capability_tag);

    // and it is independent of the bits
    D_RW_CHECK(   ( access_container_class<rw_vector>::can_read
                        == access_container_class<
                               rw_tag_read_write>::can_read )
               && ( access_container_class<rw_vector>::has_capability_tag
                        != access_container_class<
                               rw_tag_read_write>::has_capability_tag ));

    // constexpr
    D_RW_STATIC_CHECK(access_container_class<rw_vector>::can_read);
    D_RW_STATIC_CHECK(access_container_class<rw_sink>::can_write);
    D_RW_STATIC_CHECK(
        access_container_class<rw_tag_read_only>::has_capability_tag);

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
bool
tests_access_container_class_kind()
{
    bool ok = true;

    // kind, per capability
    D_RW_CHECK(access_container_class<rw_inert>::kind
                   == access_capability::none);
    D_RW_CHECK(access_container_class<rw_observer>::kind
                   == access_capability::read_only);
    D_RW_CHECK(access_container_class<rw_sink>::kind
                   == access_capability::write_only);
    D_RW_CHECK(access_container_class<rw_vector>::kind
                   == access_capability::read_write);
    D_RW_CHECK(access_container_class<rw_tag_none_bits>::kind
                   == access_capability::none);

    // restating the classifier
    D_RW_CHECK(access_container_class<rw_set>::kind
                   == access_capability_of<rw_set>::value);
    D_RW_CHECK(access_container_class<rw_tag_overrides>::kind
                   == access_capability_of<rw_tag_overrides>::value);

    // kind_name is derived from kind
    D_RW_CHECK(std::strcmp(access_container_class<rw_vector>::kind_name,
                           access_capability_name(
                               access_container_class<rw_vector>::kind))
                   == 0);
    D_RW_CHECK(std::strcmp(access_container_class<rw_sink>::kind_name,
                           access_capability_name(
                               access_container_class<rw_sink>::kind))
                   == 0);

    // read literally
    D_RW_CHECK(std::strcmp(access_container_class<rw_vector>::kind_name,
                           "read_write") == 0);
    D_RW_CHECK(std::strcmp(access_container_class<rw_observer>::kind_name,
                           "read_only") == 0);
    D_RW_CHECK(std::strcmp(access_container_class<rw_sink>::kind_name,
                           "write_only") == 0);
    D_RW_CHECK(std::strcmp(access_container_class<rw_inert>::kind_name,
                           "none") == 0);

    // constexpr
    D_RW_STATIC_CHECK(access_container_class<rw_vector>::kind
                          == access_capability::read_write);
    D_RW_STATIC_CHECK(access_container_class<rw_inert>::kind_name[0] == 'n');

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
bool
tests_access_container_class_predicates()
{
    bool ok = true;

    // each true on its own capability
    D_RW_CHECK(access_container_class<rw_observer>::is_read_only  == true);
    D_RW_CHECK(access_container_class<rw_sink>::is_write_only     == true);
    D_RW_CHECK(access_container_class<rw_vector>::is_read_write   == true);

    // ... and false on the others
    D_RW_CHECK(access_container_class<rw_observer>::is_write_only == false);
    D_RW_CHECK(access_container_class<rw_observer>::is_read_write == false);
    D_RW_CHECK(access_container_class<rw_sink>::is_read_only      == false);
    D_RW_CHECK(access_container_class<rw_sink>::is_read_write     == false);
    D_RW_CHECK(access_container_class<rw_vector>::is_read_only    == false);
    D_RW_CHECK(access_container_class<rw_vector>::is_write_only   == false);

    // `none` is the absence of all three
    D_RW_CHECK(   ( access_container_class<rw_inert>::is_read_only  == false )
               && ( access_container_class<rw_inert>::is_write_only == false )
               && ( access_container_class<rw_inert>::is_read_write
                        == false ));
    D_RW_CHECK(   ( access_container_class<rw_tag_none_bits>::is_read_only
                        == false )
               && ( access_container_class<rw_tag_none_bits>::is_write_only
                        == false )
               && ( access_container_class<rw_tag_none_bits>::is_read_write
                        == false ));

    // mutually exclusive, over every carrier
    D_RW_CHECK(   ( static_cast<int>(
                        access_container_class<rw_vector>::is_read_only )
                  + static_cast<int>(
                        access_container_class<rw_vector>::is_write_only )
                  + static_cast<int>(
                        access_container_class<rw_vector>::is_read_write ) )
               == 1);
    D_RW_CHECK(   ( static_cast<int>(
                        access_container_class<rw_observer>::is_read_only )
                  + static_cast<int>(
                        access_container_class<rw_observer>::is_write_only )
                  + static_cast<int>(
                        access_container_class<rw_observer>::is_read_write ) )
               == 1);
    D_RW_CHECK(   ( static_cast<int>(
                        access_container_class<rw_inert>::is_read_only )
                  + static_cast<int>(
                        access_container_class<rw_inert>::is_write_only )
                  + static_cast<int>(
                        access_container_class<rw_inert>::is_read_write ) )
               == 0);

    // derived from kind
    D_RW_CHECK(access_container_class<rw_tag_overrides>::is_read_only
                   == ( access_container_class<rw_tag_overrides>::kind
                            == access_capability::read_only ));
    D_RW_CHECK(access_container_class<rw_set>::is_read_write
                   == ( access_container_class<rw_set>::kind
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
bool
tests_access_container_class_agreement()
{
    bool ok = true;

    // the predicate
    D_RW_CHECK(access_container_class<rw_vector>::is_read_write
                   == is_read_write_container<rw_vector>::value);
    D_RW_CHECK(access_container_class<rw_observer>::is_read_write
                   == is_read_write_container<rw_observer>::value);
    D_RW_CHECK(access_container_class<rw_sink>::is_read_write
                   == is_read_write_container<rw_sink>::value);
    D_RW_CHECK(access_container_class<rw_inert>::is_read_write
                   == is_read_write_container<rw_inert>::value);
    D_RW_CHECK(access_container_class<rw_set>::is_read_write
                   == is_read_write_container<rw_set>::value);
    D_RW_CHECK(access_container_class<rw_array>::is_read_write
                   == is_read_write_container<rw_array>::value);

    // the classification
    D_RW_CHECK(access_container_class<rw_flist>::kind
                   == access_capability_of<rw_flist>::value);
    D_RW_CHECK(access_container_class<rw_metadata_only>::kind
                   == access_capability_of<rw_metadata_only>::value);
    D_RW_CHECK(access_container_class<rw_write_clear>::kind
                   == access_capability_of<rw_write_clear>::value);

    // where the two signals disagree
    D_RW_CHECK(access_container_class<rw_tag_overrides>::kind
                   == access_capability_of<rw_tag_overrides>::value);
    D_RW_CHECK(access_container_class<rw_tag_overrides>::is_read_only
                   == true);
    D_RW_CHECK(access_container_class<rw_tag_overrides>::is_read_write
                   == is_read_write_container<rw_tag_overrides>::value);
    D_RW_CHECK(access_container_class<rw_tag_none_bits>::kind
                   == access_capability_of<rw_tag_none_bits>::value);
    D_RW_CHECK(access_container_class<rw_tag_read_only>::can_write
                   == access_capability_of<rw_tag_read_only>::can_write);

    // and where value_type decides the write surface
    D_RW_CHECK(access_container_class<rw_clear_no_value_type>::kind
                   == access_capability_of<rw_clear_no_value_type>::value);
    D_RW_CHECK(access_container_class<rw_push_back_no_value_type>::kind
                   == access_capability_of<
                          rw_push_back_no_value_type>::value);

    // the tag flag
    D_RW_CHECK(access_container_class<rw_tag_read_only>::has_capability_tag
                   == access_capability_of<
                          rw_tag_read_only>::has_capability_tag);
    D_RW_CHECK(access_container_class<rw_tag_partial>::has_capability_tag
                   == access_capability_of<
                          rw_tag_partial>::has_capability_tag);

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
bool
tests_access_container_class_cv_ref()
{
    bool ok = true;

    // kind, across the spellings
    D_RW_CHECK(access_container_class<const rw_vector>::kind
                   == access_container_class<rw_vector>::kind);
    D_RW_CHECK(access_container_class<volatile rw_vector>::kind
                   == access_container_class<rw_vector>::kind);
    D_RW_CHECK(access_container_class<const volatile rw_vector>::kind
                   == access_container_class<rw_vector>::kind);
    D_RW_CHECK(access_container_class<rw_vector&>::kind
                   == access_container_class<rw_vector>::kind);
    D_RW_CHECK(access_container_class<const rw_vector&>::kind
                   == access_container_class<rw_vector>::kind);
    D_RW_CHECK(access_container_class<rw_vector&&>::kind
                   == access_container_class<rw_vector>::kind);

    D_RW_CHECK(access_container_class<const rw_observer&>::kind
                   == access_capability::read_only);
    D_RW_CHECK(access_container_class<volatile rw_sink>::kind
                   == access_capability::write_only);
    D_RW_CHECK(access_container_class<const volatile rw_inert&>::kind
                   == access_capability::none);

    // the bits and the flag
    D_RW_CHECK(access_container_class<volatile rw_sink>::can_write
                   == access_container_class<rw_sink>::can_write);
    D_RW_CHECK(access_container_class<const volatile rw_observer&>::can_read
                   == access_container_class<rw_observer>::can_read);
    D_RW_CHECK(access_container_class<
                   volatile rw_tag_read_only>::has_capability_tag
                   == access_container_class<
                          rw_tag_read_only>::has_capability_tag);

    // the predicates
    D_RW_CHECK(access_container_class<const rw_vector&>::is_read_write
                   == access_container_class<rw_vector>::is_read_write);
    D_RW_CHECK(access_container_class<volatile rw_observer>::is_read_only
                   == access_container_class<rw_observer>::is_read_only);
    D_RW_CHECK(access_container_class<rw_sink&&>::is_write_only
                   == access_container_class<rw_sink>::is_write_only);

    // the spelling
    D_RW_CHECK(std::strcmp(
                   access_container_class<const volatile rw_vector&>::kind_name,
                   access_container_class<rw_vector>::kind_name) == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
