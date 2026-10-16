/*******************************************************************************
* djinterp [core]           read_write_container_traits_tests_classification.cpp
*
*
* path:      /tests/djinterp/core/container/traits/read_write_container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "read_write_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  Capability classification
// ===========================================================================
//   Four capabilities, and the pure function that maps a pair of bits onto
// them.  Unlike the classifications in the neighbouring axis headers, this
// enumeration's four states exactly fill the value range its enumerators
// imply, which removes a hazard those headers carry - see
// tests_access_capability_name_total.


/*
tests_access_capability_enum
  Verifies the enumeration itself.
  Tests the following:
  - it is a SCOPED enumeration, so a capability cannot be mistaken for an int
  - the four enumerators carry the implicit ordinals 0..3 in declaration order
    and are pairwise distinct
  - a value-initialised capability is none, the safe default: an unclassified
    type is granted nothing
  - a capability is usable as a non-type template argument, which is what lets
    access_capability_of derive its `type` member and access_tag_dispatch key
    its specialisations on it
*/
bool
tests_access_capability_enum()
{
    bool ok = true;

    // scoped
    D_RW_STATIC_CHECK(std::is_enum<access_capability>::value == true);
    D_RW_STATIC_CHECK(
        std::is_convertible<access_capability, int>::value == false);

    // the implicit ordinals
    D_RW_CHECK(static_cast<int>(access_capability::none)       == 0);
    D_RW_CHECK(static_cast<int>(access_capability::read_only)  == 1);
    D_RW_CHECK(static_cast<int>(access_capability::write_only) == 2);
    D_RW_CHECK(static_cast<int>(access_capability::read_write) == 3);

    // a value-initialised capability grants nothing
    D_RW_CHECK(access_capability{} == access_capability::none);

    // pairwise distinct, over the whole enum
    for (int i = 0; i < rw_capability_count; ++i)
    {
        for (int j = 0; j < rw_capability_count; ++j)
        {
            const bool same =
                ( rw_capability_at(i) == rw_capability_at(j) );

            D_RW_CHECK(same == (i == j));
        }
    }

    // usable as a non-type template argument
    D_RW_STATIC_CHECK(
        std::integral_constant<access_capability,
                               access_capability::read_write>::value
            == access_capability::read_write);

    return ok;
}

/*
tests_access_capability_name
  Verifies the diagnostic spelling of each capability.
  Tests the following:
  - each of the four capabilities spells its own name
  - the spellings are pairwise distinct
  - the function is constexpr and usable in a constant expression
  - each spelling matches the name() the corresponding capability TAG reports,
    so a diagnostic printed from the classifier and one printed from a tag
    cannot disagree
*/
bool
tests_access_capability_name()
{
    bool ok = true;

    // each capability's spelling
    D_RW_CHECK(rw_name_is(access_capability::none,       "none"));
    D_RW_CHECK(rw_name_is(access_capability::read_only,  "read_only"));
    D_RW_CHECK(rw_name_is(access_capability::write_only, "write_only"));
    D_RW_CHECK(rw_name_is(access_capability::read_write, "read_write"));

    // pairwise distinct spellings
    for (int i = 0; i < rw_capability_count; ++i)
    {
        for (int j = 0; j < rw_capability_count; ++j)
        {
            const bool same =
                ( std::strcmp(
                      access_capability_name(rw_capability_at(i)),
                      access_capability_name(rw_capability_at(j))) == 0 );

            D_RW_CHECK(same == (i == j));
        }
    }

    // constexpr
    D_RW_STATIC_CHECK(
        access_capability_name(access_capability::none)[0] == 'n');

    // the classifier and the tags agree on spelling
    D_RW_CHECK(std::strcmp(
                   access_capability_name(access_capability::read_only),
                   read_only::name()) == 0);
    D_RW_CHECK(std::strcmp(
                   access_capability_name(access_capability::write_only),
                   write_only::name()) == 0);
    D_RW_CHECK(std::strcmp(
                   access_capability_name(access_capability::read_write),
                   read_write::name()) == 0);

    return ok;
}

/*
tests_access_capability_name_total
  Verifies that the spelling function has no unnamed-value hazard.

  access_capability_name is a chained ternary that tests three of the four
  capabilities and falls through to "read_write" - the same shape the ordering,
  layout and sortedness spelling functions have.  In those the fall-through is
  a latent trap, because their enumerator counts leave unnamed values inside
  the well-defined range.  Here it is not: four enumerators with a largest
  ordinal of three need exactly two bits, and every one of the four values a
  two-bit field can hold names an enumerator.  There is no valid-but-unnamed
  capability for the fall-through to mislabel.

  This is worth pinning rather than assuming, because it holds only while the
  enumerator count is a power of two.  Adding a fifth capability would widen
  the range to eight values and immediately create three unnamed ones, all of
  which would then read as "read_write" - the most permissive capability, and
  the worst possible default for an access axis.

  Tests the following:
  - every value in the well-defined range names a capability
  - each spells its own name, so the fall-through is only ever reached by the
    capability it names
  - the safe default is preserved: a value-initialised capability is none
*/
bool
tests_access_capability_name_total()
{
    bool ok = true;

    // every value in range names a capability, and spells its own name
    D_RW_CHECK(rw_name_is(static_cast<access_capability>(0), "none"));
    D_RW_CHECK(rw_name_is(static_cast<access_capability>(1), "read_only"));
    D_RW_CHECK(rw_name_is(static_cast<access_capability>(2), "write_only"));
    D_RW_CHECK(rw_name_is(static_cast<access_capability>(3), "read_write"));

    // ... which is to say the range is exactly the enumerator set
    for (int v = 0; v < rw_capability_count; ++v)
    {
        const access_capability c = static_cast<access_capability>(v);

        D_RW_CHECK(c == rw_capability_at(v));
        D_RW_CHECK(std::strcmp(access_capability_name(c),
                               access_capability_name(rw_capability_at(v)))
                       == 0);
    }

    // the fall-through is reached only by the capability it names
    D_RW_CHECK(rw_name_is(access_capability::read_write, "read_write"));
    D_RW_CHECK(static_cast<int>(access_capability::read_write)
                   == rw_capability_count - 1);

    // the safe default is preserved
    D_RW_CHECK(access_capability{} == access_capability::none);
    D_RW_CHECK(rw_name_is(access_capability{}, "none"));

    return ok;
}

/*
tests_access_capability_from_bits
  Verifies the pure (can_read, can_write) -> capability mapping, exhaustively -
  it takes two bools, so four cases is the whole function.
  Tests the following:
  - all four input combinations, each reaching its intended capability
  - the mapping is injective, so no two bit pairs collapse onto one capability
    and the capability can be inverted back to the bits
  - it is constexpr, and the classifier uses it rather than duplicating the
    cascade - shown by re-deriving the classifier's verdict from its own bits
*/
bool
tests_access_capability_from_bits()
{
    bool ok = true;

    // the whole function
    D_RW_CHECK(access_capability_from_bits(false, false)
                   == access_capability::none);
    D_RW_CHECK(access_capability_from_bits(true, false)
                   == access_capability::read_only);
    D_RW_CHECK(access_capability_from_bits(false, true)
                   == access_capability::write_only);
    D_RW_CHECK(access_capability_from_bits(true, true)
                   == access_capability::read_write);

    // constexpr
    D_RW_STATIC_CHECK(access_capability_from_bits(true, true)
                          == access_capability::read_write);
    D_RW_STATIC_CHECK(access_capability_from_bits(false, false)
                          == access_capability::none);

    // injective: four inputs, four distinct outputs
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            const access_capability a =
                access_capability_from_bits((i & 1) != 0, (i & 2) != 0);
            const access_capability b =
                access_capability_from_bits((j & 1) != 0, (j & 2) != 0);

            D_RW_CHECK((a == b) == (i == j));
        }
    }

    // the classifier defers to it rather than duplicating the cascade
    D_RW_CHECK(access_capability_of<rw_vector>::value
                   == access_capability_from_bits(
                          access_capability_of<rw_vector>::can_read,
                          access_capability_of<rw_vector>::can_write));
    D_RW_CHECK(access_capability_of<rw_sink>::value
                   == access_capability_from_bits(
                          access_capability_of<rw_sink>::can_read,
                          access_capability_of<rw_sink>::can_write));
    D_RW_CHECK(access_capability_of<rw_observer>::value
                   == access_capability_from_bits(
                          access_capability_of<rw_observer>::can_read,
                          access_capability_of<rw_observer>::can_write));
    D_RW_CHECK(access_capability_of<rw_inert>::value
                   == access_capability_from_bits(
                          access_capability_of<rw_inert>::can_read,
                          access_capability_of<rw_inert>::can_write));

    return ok;
}

/*
tests_access_capability_of
  Verifies the classification itself, on a carrier for each capability.
  Tests the following:
  - a carrier for each of the four capabilities, reached structurally
  - a carrier for each reached instead through a stamped tag, so both routes
    are exercised on every capability
  - the classification is total: every carrier the suite defines lands on
    exactly one of the four
*/
bool
tests_access_capability_of()
{
    bool ok = true;

    // reached structurally
    D_RW_CHECK(access_capability_of<rw_inert>::value
                   == access_capability::none);
    D_RW_CHECK(access_capability_of<rw_metadata_only>::value
                   == access_capability::none);
    D_RW_CHECK(access_capability_of<rw_observer>::value
                   == access_capability::read_only);
    D_RW_CHECK(access_capability_of<rw_write_const_index>::value
                   == access_capability::read_only);
    D_RW_CHECK(access_capability_of<rw_sink>::value
                   == access_capability::write_only);
    D_RW_CHECK(access_capability_of<rw_write_clear>::value
                   == access_capability::write_only);
    D_RW_CHECK(access_capability_of<rw_both>::value
                   == access_capability::read_write);
    D_RW_CHECK(access_capability_of<rw_vector>::value
                   == access_capability::read_write);

    // reached through a tag
    D_RW_CHECK(access_capability_of<rw_tag_none_bits>::value
                   == access_capability::none);
    D_RW_CHECK(access_capability_of<rw_tag_read_only>::value
                   == access_capability::read_only);
    D_RW_CHECK(access_capability_of<rw_tag_write_only>::value
                   == access_capability::write_only);
    D_RW_CHECK(access_capability_of<rw_tag_read_write>::value
                   == access_capability::read_write);

    // non-containers
    D_RW_CHECK(access_capability_of<rw_pair>::value
                   == access_capability::none);
    D_RW_CHECK(access_capability_of<int>::value  == access_capability::none);
    D_RW_CHECK(access_capability_of<void>::value == access_capability::none);

    return ok;
}

/*
tests_access_capability_of_members
  Verifies the three members the classifier exposes alongside its verdict.
  Tests the following:
  - can_read and can_write carry the resolved bits, whichever path resolved
    them
  - has_capability_tag reports which path was taken, and is independent of the
    verdict: carriers agreeing on the capability may disagree on the flag
  - all three are usable in a constant expression
  - the verdict is exactly the two bits mapped through
    access_capability_from_bits, on every carrier
*/
bool
tests_access_capability_of_members()
{
    bool ok = true;

    // the bits, structural path
    D_RW_CHECK(   ( access_capability_of<rw_observer>::can_read  == true )
               && ( access_capability_of<rw_observer>::can_write == false ));
    D_RW_CHECK(   ( access_capability_of<rw_sink>::can_read      == false )
               && ( access_capability_of<rw_sink>::can_write     == true ));
    D_RW_CHECK(   ( access_capability_of<rw_inert>::can_read     == false )
               && ( access_capability_of<rw_inert>::can_write    == false ));

    // the bits, tagged path
    D_RW_CHECK(   ( access_capability_of<rw_tag_write_only>::can_read
                        == false )
               && ( access_capability_of<rw_tag_write_only>::can_write
                        == true ));

    // has_capability_tag reports the path, not the verdict
    D_RW_CHECK(access_capability_of<rw_vector>::has_capability_tag  == false);
    D_RW_CHECK(access_capability_of<rw_tag_read_write>::has_capability_tag
                   == true);
    D_RW_CHECK(   ( access_capability_of<rw_vector>::value
                        == access_capability_of<rw_tag_read_write>::value )
               && ( access_capability_of<rw_vector>::has_capability_tag
                        != access_capability_of<
                               rw_tag_read_write>::has_capability_tag ));

    // constexpr
    D_RW_STATIC_CHECK(access_capability_of<rw_vector>::can_read);
    D_RW_STATIC_CHECK(access_capability_of<rw_vector>::can_write);
    D_RW_STATIC_CHECK(
        access_capability_of<rw_tag_read_only>::has_capability_tag);

    // the verdict is the bits mapped through
    D_RW_CHECK(access_capability_of<rw_tag_overrides>::value
                   == access_capability_from_bits(
                          access_capability_of<rw_tag_overrides>::can_read,
                          access_capability_of<rw_tag_overrides>::can_write));
    D_RW_CHECK(access_capability_of<rw_set>::value
                   == access_capability_from_bits(
                          access_capability_of<rw_set>::can_read,
                          access_capability_of<rw_set>::can_write));

    return ok;
}

/*
tests_access_capability_cv_ref
  Verifies that the classification honours the clean_t contract.
  Tests the following:
  - the classification is identical across all seven cv- and reference-
    qualified spellings, on a carrier for each of the four capabilities
  - the bits and the tag flag likewise
  - the volatile spellings in particular
*/
bool
tests_access_capability_cv_ref()
{
    bool ok = true;

    D_RW_CV_REF_AGREE(access_capability_of, rw_vector,
                      access_capability::read_write);
    D_RW_CV_REF_AGREE(access_capability_of, rw_set,
                      access_capability::read_write);
    D_RW_CV_REF_AGREE(access_capability_of, rw_observer,
                      access_capability::read_only);
    D_RW_CV_REF_AGREE(access_capability_of, rw_sink,
                      access_capability::write_only);
    D_RW_CV_REF_AGREE(access_capability_of, rw_inert,
                      access_capability::none);
    D_RW_CV_REF_AGREE(access_capability_of, rw_tag_read_only,
                      access_capability::read_only);

    // the members, spelled out
    D_RW_CHECK(access_capability_of<volatile rw_sink>::can_write
                   == access_capability_of<rw_sink>::can_write);
    D_RW_CHECK(access_capability_of<const volatile rw_observer&>::can_read
                   == access_capability_of<rw_observer>::can_read);
    D_RW_CHECK(access_capability_of<
                   volatile rw_tag_read_only>::has_capability_tag
                   == access_capability_of<
                          rw_tag_read_only>::has_capability_tag);
    D_RW_CHECK(access_capability_of<rw_vector&&>::value
                   == access_capability::read_write);

    return ok;
}

/*
tests_access_capability_companions
  Verifies the `_t` alias and the `_v` companion.
  Tests the following:
  - the `_t` alias is the integral_constant naming the capability, so the
    verdict is carried in the type system and not merely as a value
  - the `_v` companion agrees with ::value across the four capabilities
  - both are usable in a constant expression
  - the `_v` half is gated on variable-template support, since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool
tests_access_capability_companions()
{
    bool ok = true;

    // the _t alias is the integral_constant, exactly
    D_RW_STATIC_CHECK(
        std::is_same<access_capability_of_t<rw_vector>,
                     std::integral_constant<
                         access_capability,
                         access_capability::read_write> >::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_capability_of_t<rw_observer>,
                     std::integral_constant<
                         access_capability,
                         access_capability::read_only> >::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_capability_of_t<rw_sink>,
                     std::integral_constant<
                         access_capability,
                         access_capability::write_only> >::value);
    D_RW_STATIC_CHECK(
        std::is_same<access_capability_of_t<rw_inert>,
                     std::integral_constant<
                         access_capability,
                         access_capability::none> >::value);

    // ... and carries the value the trait reports
    D_RW_CHECK(access_capability_of_t<rw_set>::value
                   == access_capability_of<rw_set>::value);
    D_RW_CHECK(access_capability_of_t<rw_tag_write_only>::value
                   == access_capability_of<rw_tag_write_only>::value);

    // usable in a constant expression
    D_RW_STATIC_CHECK(access_capability_of<rw_vector>::value
                          == access_capability::read_write);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_RW_STATIC_CHECK(access_capability_of_v<rw_sink>
                          == access_capability::write_only);

    D_RW_CHECK(access_capability_of_v<rw_vector>
                   == access_capability_of<rw_vector>::value);
    D_RW_CHECK(access_capability_of_v<rw_observer>
                   == access_capability_of<rw_observer>::value);
    D_RW_CHECK(access_capability_of_v<rw_sink>
                   == access_capability_of<rw_sink>::value);
    D_RW_CHECK(access_capability_of_v<rw_inert>
                   == access_capability_of<rw_inert>::value);
    D_RW_CHECK(access_capability_of_v<rw_tag_overrides>
                   == access_capability_of<rw_tag_overrides>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
